/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: 
 **********************************************************************/

#include <ide.h>

#include <rpcManager.h>
#include <rpdReplicatedTransGroupNode.h>
#include <rpxAheadAnalyzer.h>

#define RPX_AHEAD_ANALYZER_SLEEP_SEC    ( 5 )

IDE_RC rpxAheadAnalyzer::initialize( rpxSender      * aSender )
{
    SChar       sName[IDU_MUTEX_NAME_LEN + 1] = { 0, };
    UInt        sStage = 0;

    mExitFlag = ID_FALSE;
    mIsAnalyzing = ID_FALSE;

    mSenderReadLogFileNo = 0;
    mAheadReadLogFileNo = 0;

    mAheadStartLogFileNo = UINT_MAX;

    mStatus = RPX_AHEAD_ANALYZER_STATUS_NONE;

    mSender = aSender;

    mInitSN = SM_SN_NULL;

    mTransTableSize = smiGetTransTblSize();

    IDU_FIT_POINT( "rpxAheadAnalyzer::initialize::initialize::mMutex",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mMutex->initialize" );

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_AHEAD_ANALYZER", mSender->getRepName() );
    IDE_TEST( mMutex.initialize( sName,
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT( "rpxAheadAnalyzer::initialize::initialize::mCV",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mCV->initialize" );

    IDE_TEST( mCV.initialize() != IDE_SUCCESS );
    sStage = 2;

    IDU_FIT_POINT( "rpxAheadAnalyzer::initialize::initialize::mTransTable",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mTransTable->initialize" );

    IDE_TEST( mTransTable.initialize() != IDE_SUCCESS );
    sStage = 3;

    mReplicatedTransGroupMaxCount = RPU_REPLICATION_GROUPING_TRANSACTION_MAX_COUNT;

    IDU_FIT_POINT( "rpxAheadAnalyzer::initialize::initialize::mReplicatedTransGroup",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mReplicatedTransGroup->initialize" );

    IDE_TEST( mReplicatedTransGroup.initialize( mSender->getRepName(),
                                                mReplicatedTransGroupMaxCount )
              != IDE_SUCCESS );
    sStage = 4;

    mIsLogMgrStart = ID_FALSE;
    mIsLogMgrInit = ID_FALSE;

    mReadSN = SM_SN_NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 4:
            mReplicatedTransGroup.finalize();
        case 3:
            mTransTable.finalize();
        case 2:
            (void)mCV.destroy();
        case 1:
            (void)mMutex.destroy();
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxAheadAnalyzer::finalize( void )
{
    mExitFlag = ID_TRUE;

    mTransTable.clearActiveTransTable();
    mReplicatedTransGroup.clearReplicatedTransGroup();

    mTransTable.finalize();
    mReplicatedTransGroup.finalize();

    if ( mCV.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    if ( mMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxAheadAnalyzer::initializeThread( void )
{
    UInt        sStage = 0;
    SChar       sName[IDU_MUTEX_NAME_LEN + 1] = { 0, };

    mIsAnalyzing = ID_FALSE;
    mIsLogMgrStart = ID_FALSE;

    IDU_FIT_POINT( "rpxAheadAnalyzer::logMgrInitialize::initialize::mLogMgr",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mLogMgr->initialize" );
    IDE_TEST( mLogMgr.initialize( mInitSN,
                                  RPU_PREFETCH_LOGFILE_COUNT,
                                  ID_FALSE,  // aIsRemoteLog
                                  0,         // aLogFileSize
                                  NULL,      // aLogDirPath
                                  ID_TRUE ) // 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부
              != IDE_SUCCESS );
    sStage = 1;
    mIsLogMgrInit = ID_TRUE;
    mIsLogMgrStart = ID_TRUE;

    mIsLogMgrStart = ID_FALSE;
    IDU_FIT_POINT( "rpxAheadAnalyzer::runAnalyze::stop::mLogMgr",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mLogMgr.stop" );
    IDE_TEST( mLogMgr.stop() != IDE_SUCCESS );

    IDU_FIT_POINT( "rpxAheadAnalyzer::initialize::initialize::mChainedValuePool",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mChainedValuePool->initialize" );
    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_CHAINED_VALUE_POOL",
                     mSender->mRepName );
    IDE_TEST( mChainedValuePool.initialize( IDU_MEM_RP_RPX_SENDER,
                                            sName,
                                            1,
                                            RPU_REPLICATION_POOL_ELEMENT_SIZE,
                                            RPU_REPLICATION_POOL_ELEMENT_COUNT,
                                            IDU_AUTOFREE_CHUNK_LIMIT, //chunk max(default)
                                            ID_FALSE,                 //use mutex(no use)
                                            8,                        //align byte(default)
                                            ID_FALSE,				  //ForcePooling
                                            ID_TRUE,				  //GarbageCollection
                                            ID_TRUE,                  /* HWCacheLine */
                                            IDU_MEMPOOL_TYPE_LEGACY   /* mempool type*/) 
              != IDE_SUCCESS);			
    sStage = 2;

    mMeta.initialize();

    IDE_TEST( mMeta.buildWithNewTransaction( NULL,
                                             mSender->mRepName,
                                             ID_FALSE,
                                             RP_META_BUILD_AUTO )
              != IDE_SUCCESS );
    sStage = 3;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            mMeta.finalize();
        case 2:
            (void)mChainedValuePool.destroy();
        case 1:
            mIsLogMgrInit = ID_FALSE;
            (void)mLogMgr.destroy();
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxAheadAnalyzer::setInitSN( smSN      aInitSN )
{
    mInitSN = aInitSN;
}

void rpxAheadAnalyzer::finalizeThread( void )
{
    IDE_DASSERT( mIsLogMgrInit == ID_TRUE );

    mIsLogMgrInit = ID_FALSE;
    if ( mLogMgr.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );        
    }
    else
    {
        /* do nothing */
    }

    if ( mChainedValuePool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {  
        IDE_ERRLOG( IDE_RP_0 );        
    } 
    else
    {
        /* do nothing */
    }

    mMeta.finalize();
}

void rpxAheadAnalyzer::run( void )
{
    idBool              sIsLocked = ID_FALSE;
    PDL_Time_Value      sCheckTime;
    PDL_Time_Value      sSleepTime;

    IDE_CLEAR();

    sCheckTime.initialize();
    sSleepTime.initialize();

    ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_IS_START );

    IDE_ASSERT( mMutex.lock( NULL ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    while ( mExitFlag != ID_TRUE )
    {
        IDE_CLEAR();

        /* 
         * Thread 가 시작하기전에 분석 시작 Flag 가 설정되어 
         * 있을수 있다. 그렇게 때문에 timedwait 으로 들어 가기전
         * mIsAnalyzing Flag 을 확인한다.
         */
        if ( mIsAnalyzing == ID_TRUE ) 
        {
            sIsLocked = ID_FALSE;
            IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );

            IDE_TEST( runAnalyze() != IDE_SUCCESS );

            IDE_ASSERT( mMutex.lock( NULL ) == IDE_SUCCESS );
            sIsLocked = ID_TRUE;
        }
        else
        {
            /* do nothing */
        }

        if ( mExitFlag != ID_TRUE )
        {
            /* 
             * 할일을 완료 하였으면 
             * Sender 가 지시하기전까지 sleep 상태로 변경한다.
             */
            mStatus = RPX_AHEAD_ANALYZER_STATUS_WAIT_ANALYZE;

            sSleepTime.set( RPX_AHEAD_ANALYZER_SLEEP_SEC );

            sCheckTime = idlOS::gettimeofday() + sSleepTime;

            IDU_FIT_POINT( "rpxAheadAnalyzer::run::lock::mCV",
                           rpERR_ABORT_RP_INTERNAL_ARG,
                           "mCV.timedwait" );
            if ( mCV.timedwait( &mMutex, &sCheckTime ) != IDE_SUCCESS )
            {
                IDE_TEST_RAISE( mCV.isTimedOut() != ID_TRUE, ERR_TIMED_WAIT );
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    mStatus = RPX_AHEAD_ANALYZER_STATUS_NONE;

    sIsLocked = ID_FALSE;
    IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );

    ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_IS_END );

    return;
    
    IDE_EXCEPTION ( ERR_TIMED_WAIT )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_AHEAD_ANALYZER_TIMED_WAIT_ERROR ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    mStatus = RPX_AHEAD_ANALYZER_STATUS_ERROR;

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_CLEAR();

    return;
}

IDE_RC rpxAheadAnalyzer::runAnalyze( void )
{
    smiLogRec       sLog;

    mStatus = RPX_AHEAD_ANALYZER_STATUS_ANALYZE;

    sLog.initialize( (const void *)&mMeta,
                     &mChainedValuePool,
                     RPU_REPLICATION_POOL_ELEMENT_SIZE );

    sLog.setCallbackFunction( rpdMeta::getMetaItem,
                              rpdMeta::getColumnCount,
                              rpdMeta::getSmiColumn );

    ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_ANALYZE_IS_START );

    mTransTable.clearActiveTransTable();
    mReplicatedTransGroup.clearReplicatedTransGroup();

    while ( ( mExitFlag != ID_TRUE ) &&
            ( mIsAnalyzing == ID_TRUE ) )
    {
        IDE_TEST( analyze( &sLog ) != IDE_SUCCESS );

        waitForNewRecord();
    }

    ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_ANALYZE_IS_STOP );

    mAheadReadLogFileNo = 0;
    mAheadStartLogFileNo = UINT_MAX;

    mIsLogMgrStart = ID_FALSE;
    IDU_FIT_POINT( "rpxAheadAnalyzer::runAnalyze::stop::mLogMgr",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mLogMgr.stop" );
    IDE_TEST( mLogMgr.stop() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( mIsLogMgrStart == ID_TRUE )
    {
        mIsLogMgrStart = ID_FALSE;
        (void)mLogMgr.stop();
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::analyze( smiLogRec     * aLog )
{
    idBool          sIsValid   = ID_FALSE;
    smSN            sCurrentSN = SM_SN_NULL;
    smLSN           sReadLSN;
    smiLogHdr       sLogHead;
    SChar         * sMyLogPtr = NULL;

    idBool          sIsEndOfLogFile = ID_FALSE;

    smiLogType      sLogType;
    smTID           sTransID = SM_NULL_TID;
    rpdMetaItem   * sMetaItem = NULL;
    smiTableMeta  * sItemMeta = NULL;
    
    UInt            i = 0;
    UInt            sGCCnt = SM_NULL_TID;

    IDU_FIT_POINT( "rpxAheadAnalyzer::analyze::readLog::mLogMgr",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mLogMgr.readLog" );
    IDE_TEST( readMyLog( &sCurrentSN,
                         &sReadLSN,
                         (SChar*)&sLogHead,
                         &sMyLogPtr,
                         &sIsValid )
              != IDE_SUCCESS );

    if ( ( sIsValid == ID_TRUE ) && ( sMyLogPtr != NULL ) )
    {
        aLog->reset();

        IDU_FIT_POINT( "rpxAheadAnalyzer::analyze::readFrom::aLog",
                       rpERR_ABORT_RP_INTERNAL_ARG,
                       "aLog->readFrom" );
        IDE_TEST( aLog->readFrom( &sLogHead,
                                  sMyLogPtr,
                                  &sReadLSN )
                  != IDE_SUCCESS );

        mReadSN = aLog->getRecordSN();

        sLogType = aLog->getType();
        sTransID = aLog->getTransID();

        switch ( sLogType )
        {
            case SMI_LT_FILE_END:
                IDE_TEST( addCompletedReplicatedTransGroupFromAllActiveTrans( aLog->getRecordSN() )
                          != IDE_SUCCESS );
                sIsEndOfLogFile = ID_TRUE;
                break;

            case SMI_LT_TABLE_META:
                sItemMeta = aLog->getTblMeta();

                // Table Meta Log Record를 Transaction Table에 보관한다.
                IDE_TEST( mTransTable.addItemMetaEntry( sTransID,
                                                        sItemMeta,
                                                        (const void*)aLog->getTblMetaLogBodyPtr(),
                                                        aLog->getTblMetaLogBodySize(),
                                                        aLog->getRecordSN())
                          != IDE_SUCCESS );
                break;
            case SMI_LT_TRANS_COMMIT:
                if ( ( mTransTable.isDDLTrans( sTransID ) == ID_TRUE ) &&
                     ( mTransTable.existItemMeta( sTransID ) == ID_TRUE ) )
                {
                    IDE_TEST( applyTableMetaLog( sTransID ) != IDE_SUCCESS );
                }
                else
                {
                    /* do nothing */
                }
                break;
            case SMI_LT_TRANS_GROUPCOMMIT:
                /* GROUP COMMIT은 DDL Tx을 포함하지 않는다.
                 * 추후 변경시 고려해 주어야 함. */
                break;

            default:
                break;
        }

        if ( sIsEndOfLogFile == ID_FALSE )
        {
            IDE_TEST( searchTable( sLogType,
                                   aLog->getTableOID(),
                                   &sMetaItem )
                      != IDE_SUCCESS );

            if ( sLogType == SMI_LT_TRANS_GROUPCOMMIT )
            {
                sGCCnt = aLog->getGroupCommitCnt( aLog->getLogPtr() );

                for ( i = 0; i < sGCCnt; i++ )
                {
                    sTransID = aLog->getNthTIDFromGroupCommit( aLog->getLogPtr(), i );

                    buildAnalyzingTable( aLog,
                                         sMetaItem,
                                         sTransID );

                    IDE_TEST( checkAndAddReplicatedTransGroup( aLog, sTransID ) != IDE_SUCCESS );
                }
            }
            else
            {
                buildAnalyzingTable( aLog,
                                     sMetaItem,
                                     sTransID );

                IDE_TEST( checkAndAddReplicatedTransGroup( aLog, sTransID ) != IDE_SUCCESS );
            }

        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxAheadAnalyzer::buildAnalyzingTable( smiLogRec     * aLog,
                                            rpdMetaItem   * aMetaItem,
                                            smTID           aTransID )
{
    smSN        sSN = SM_SN_NULL;
    smiLogType  sLogType;

    sSN = aLog->getRecordSN();
    sLogType = aLog->getType();

    if ( aLog->isBeginLog() == ID_TRUE )
    {
        mTransTable.setActiveTrans( aTransID, sSN );
    }
    else
    {
        /* do nothing */
    }

    /* TODO add comment */
    if ( mTransTable.isActiveTrans( aTransID ) != ID_TRUE )
    {
        mTransTable.setActiveTrans( aTransID, sSN );
        mTransTable.setDisableToGroup( aTransID );
    }
    else
    {
        /* do nothing */
    }

    if ( aMetaItem != NULL )
    {
        mTransTable.setReplicationTrans( aTransID );
    }
    else
    {
        /* do noting */
    }

    switch ( sLogType )
    {
        case SMI_LT_DDL:
            mTransTable.setDDLTrans( aTransID );
            /* fall through */
        case SMI_LT_TABLE_META:
        case SMI_LT_SAVEPOINT_ABORT:
            mTransTable.setDisableToGroup( aTransID );
            break;

        default:
            break;
    }

}

IDE_RC rpxAheadAnalyzer::checkAndAddReplicatedTransGroupInCommit( smiLogRec     * aLog,
                                                                  smTID           aTransID )
{
    smSN        sSN = SM_SN_NULL;
    smSN        sBeginSN = SM_SN_NULL;

    sSN = aLog->getRecordSN();

    IDE_DASSERT( ( aLog->getType() == SMI_LT_TRANS_COMMIT ) ||
                 ( aLog->getType() == SMI_LT_TRANS_GROUPCOMMIT ) );

    sBeginSN = mTransTable.getBeginSN( aTransID );

    if ( mTransTable.shouldBeGroup( aTransID ) == ID_TRUE )
    {
        mReplicatedTransGroup.insertCompleteTrans( aTransID,
                                                   sBeginSN,
                                                   sSN );

        IDE_TEST( mReplicatedTransGroup.checkAndAddReplicatedTransGroupToComplete( 
                        RPD_REPLICATED_TRANS_GROUP_SEND )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( mReplicatedTransGroup.getReplicatedTransCount() != 0 )
        {
            IDE_TEST( mReplicatedTransGroup.addCompletedReplicatedTransGroup( 
                            RPD_REPLICATED_TRANS_GROUP_SEND ) 
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }

        mReplicatedTransGroup.insertCompleteTrans( aTransID,
                                                   sBeginSN,
                                                   sSN );

        IDE_TEST( mReplicatedTransGroup.addCompletedReplicatedTransGroup( 
                        RPD_REPLICATED_TRANS_GROUP_SEND ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::checkAndAddReplicatedTransGroupInAbort( smiLogRec     * aLog )
{
    smTID       sTransID = SM_NULL_TID;
    smSN        sSN = SM_SN_NULL;
    smSN        sBeginSN = SM_SN_NULL;

    sTransID = aLog->getTransID();
    sSN = aLog->getRecordSN();

    IDE_DASSERT( aLog->getType() == SMI_LT_TRANS_PREABORT );

    sBeginSN = mTransTable.getBeginSN( sTransID );

    if ( mReplicatedTransGroup.getReplicatedTransCount() != 0 )
    {
        IDE_TEST( mReplicatedTransGroup.addCompletedReplicatedTransGroup( 
                        RPD_REPLICATED_TRANS_GROUP_SEND ) 
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    mReplicatedTransGroup.insertCompleteTrans( sTransID,
                                               sBeginSN,
                                               sSN );

    IDE_TEST( mReplicatedTransGroup.addCompletedReplicatedTransGroup( 
                    RPD_REPLICATED_TRANS_GROUP_DONT_SEND ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::checkAndAddReplicatedTransGroup( smiLogRec   * aLog,
                                                          smTID         aTransID )
{
    smiLogType  sLogType;

    IDE_DASSERT( mTransTable.isActiveTrans( aTransID ) == ID_TRUE );

    sLogType = aLog->getType();

    switch ( sLogType )
    {
        case SMI_LT_TRANS_GROUPCOMMIT:
        case SMI_LT_TRANS_COMMIT:

            if ( mTransTable.isReplicationTrans( aTransID ) == ID_TRUE )
            {
                IDE_TEST( checkAndAddReplicatedTransGroupInCommit( aLog, aTransID )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            mTransTable.clearTransNodeFromTransID( aTransID );

            break;

        case SMI_LT_TRANS_PREABORT:

            if ( mTransTable.isReplicationTrans( aTransID ) == ID_TRUE )
            {
                IDE_TEST( checkAndAddReplicatedTransGroupInAbort( aLog )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            mTransTable.clearTransNodeFromTransID( aTransID );

            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxAheadAnalyzer::waitForNewRecord( void )
{
    smSN            sCurrentSN = SM_SN_NULL;
    PDL_Time_Value  sSleepTv;

    sSleepTv.initialize( 0, RPU_SENDER_SLEEP_TIME );

    while ( ( mExitFlag != ID_TRUE ) &&
            ( mIsAnalyzing == ID_TRUE ) )
    {
        IDE_ASSERT( smiGetLastValidGSN( &sCurrentSN ) == IDE_SUCCESS );

        if ( mReadSN == sCurrentSN )
        {
            idlOS::sleep( sSleepTv );
        }
        else
        {
            break;
        }
    }
}

IDE_RC rpxAheadAnalyzer::startToAnalyze( smLSN       aStartLSN,
                                         idBool    * aIsStart )
{
    smLSN   sNextFileLSN;
    smLSN   sLastLSN;

    UInt    sFileNo = 0;

    *aIsStart = ID_FALSE;

    IDE_DASSERT( mIsAnalyzing != ID_TRUE );

    IDE_TEST_CONT( ( mExitFlag == ID_TRUE ) ||
                   ( mIsLogMgrInit != ID_TRUE ), NORMAL_EXIT );

    IDE_TEST( smiGetLstLSN( &sLastLSN ) != IDE_SUCCESS );

    mSenderReadLogFileNo = aStartLSN.mFileNo;

    sFileNo = aStartLSN.mFileNo + RPU_REPLICATION_GROUPING_AHEAD_READ_NEXT_LOG_FILE;

    if ( sLastLSN.mFileNo >= sFileNo )
    {
        if ( mStatus != RPX_AHEAD_ANALYZER_STATUS_ERROR )
        {
            sNextFileLSN.mFileNo = sFileNo;
            sNextFileLSN.mOffset = 0;

            mAheadReadLogFileNo = sFileNo;
            mAheadStartLogFileNo = sFileNo;

            IDU_FIT_POINT( "rpxAheadAnalyzer::startToAnalyze:startByLSN::mLogMgr",
                    rpERR_ABORT_RP_INTERNAL_ARG,
                    "mLogMgr.startByLSN" );
            IDE_TEST( mLogMgr.startByLSN( sNextFileLSN ) != IDE_SUCCESS );

            ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_START_FILENO, sFileNo );

            mIsLogMgrStart = ID_TRUE;

            mIsAnalyzing = ID_TRUE;
            *aIsStart = ID_TRUE;

            wakeup();
        }
        else
        {
            ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_CAN_NOT_ANALYZE );
        }
    }
    else
    {
        ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_SKIP_FILENO, sFileNo );
    }

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxAheadAnalyzer::stopToAnalyze( void )
{
    if ( mIsAnalyzing == ID_TRUE )
    {
        ideLog::log( IDE_RP_0, RP_TRC_X_AHEAD_ANALYZER_TRY_STOP_ANALYZE );

        mIsAnalyzing = ID_FALSE;
        mAheadStartLogFileNo = UINT_MAX;
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxAheadAnalyzer::checkAndStartOrStopAnalyze( idBool    aIsRPLogBufMode,
                                                     UInt      aFileNo,
                                                     idBool  * aIsStarted,
                                                     idBool  * aIsStoped )
{
    smLSN       sNextFileLSN;

    *aIsStarted = ID_FALSE;
    *aIsStoped = ID_FALSE;

    mSenderReadLogFileNo = aFileNo;

    *aIsStarted = ID_FALSE;
    *aIsStoped = ID_FALSE;

    if ( mExitFlag == ID_FALSE )
    {
        if ( ( mAheadReadLogFileNo != 0 ) &&
             ( mAheadReadLogFileNo <= aFileNo + 1 ) )
        {
            ideLog::log( IDE_RP_2, RP_TRC_X_AHEAD_ANALYZER_SET_FILENO, mAheadReadLogFileNo, aFileNo );
            stopToAnalyze();
            *aIsStoped = ID_TRUE;
        }
        else
        {
            if ( ( aIsRPLogBufMode == ID_FALSE ) && 
                 ( mIsAnalyzing == ID_FALSE ) )
            {
                sNextFileLSN.mFileNo = aFileNo + 1;
                sNextFileLSN.mOffset = 0;

                IDE_TEST( startToAnalyze( sNextFileLSN,
                                          aIsStarted ) 
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        *aIsStoped = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

smTID rpxAheadAnalyzer::getReplicatedTransactionGroupTID( smTID aTID )
{
    return mReplicatedTransGroup.getReplicatedTransGroupTransID( aTID );
}

idBool rpxAheadAnalyzer::isLastTransactionInFirstGroup( smTID    aTransID,
                                                        smSN     aSN )
{
    return mReplicatedTransGroup.isLastTransactionInFirstGroup( aTransID,
                                                                aSN );
}

IDE_RC rpxAheadAnalyzer::buildRecordForReplicatedTransGroupInfo(  SChar               * aRepName,
                                                                  void                * aHeader,
                                                                  void                * aDumpObj,
                                                                  iduFixedTableMemory * aMemory )
{
    IDE_TEST( mReplicatedTransGroup.buildRecordForReplicatedTransGroupInfo( aRepName, 
                                                                            aHeader,
                                                                            aDumpObj,
                                                                            aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                                void                * aHeader,
                                                                void                * aDumpObj,
                                                                iduFixedTableMemory * aMemory )
{
    IDE_TEST( mReplicatedTransGroup.buildRecordForReplicatedTransSlotInfo( aRepName, 
                                                                           aHeader,
                                                                           aDumpObj,
                                                                           aMemory )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::buildRecordForAheadAnalyzerInfo(  SChar               * aRepName,
                                                           void                * aHeader,
                                                           void                * /* aDumpObj */,
                                                           iduFixedTableMemory * aMemory )
{
    rpdAheadAnalyzerInfo    sInfo;

    sInfo.mRepName = aRepName;
    sInfo.mReadLogFileNo = mAheadReadLogFileNo;
    sInfo.mReadSN = mReadSN;
    sInfo.mStatus = mStatus;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sInfo )
              != IDE_SUCCESS );    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TODO comment */
IDE_RC rpxAheadAnalyzer::addCompletedReplicatedTransGroupFromAllActiveTrans( smSN   aEndSN )
{
    smTID       sTransID = SM_NULL_TID;
    smSN        sBeginSN = SM_SN_NULL;

    UInt        i = 0;

    if ( mReplicatedTransGroup.getReplicatedTransCount() != 0 )
    {
        IDE_TEST( mReplicatedTransGroup.addCompletedReplicatedTransGroup( 
                RPD_REPLICATED_TRANS_GROUP_SEND )
            != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    for ( i = 0; i < mTransTableSize; i++ )
    {
        if ( ( mTransTable.isActiveTransByIndex( i ) == ID_TRUE ) &&
             ( mTransTable.isReplicationTransByIndex( i ) == ID_TRUE ) )
        {
            sTransID = mTransTable.getTransIDByIndex( i );
            sBeginSN = mTransTable.getBeginSNByIndex( i );

            mTransTable.setDisableToGroupByIndex( i );

            mReplicatedTransGroup.insertCompleteTrans( sTransID,
                                                       sBeginSN,
                                                       aEndSN );

            IDE_TEST( mReplicatedTransGroup.addCompletedReplicatedTransGroup(
                      RPD_REPLICATED_TRANS_GROUP_SEND )
                != IDE_SUCCESS );

            mTransTable.clearTransNodeFromTransID( sTransID );
        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::searchTable( smiLogType      aLogType,
                                      ULong           aTableOID,
                                      rpdMetaItem  ** aMetaItem )
{
    switch ( aLogType )
    {
        case SMI_LT_MEMORY_CHANGE:
        case SMI_LT_DISK_CHANGE:
        case SMI_LT_TABLE_META:
            IDU_FIT_POINT( "rpxAheadAnalyzer::searchTable::searchTable::mMeta",
                           rpERR_ABORT_RP_INTERNAL_ARG,
                           "mMeta->searchTable" );
            IDE_TEST( mMeta.searchTable( aMetaItem,
                                         aTableOID )
                      != IDE_SUCCESS );
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt rpxAheadAnalyzer::getStartFileNo( void )
{
    return mAheadStartLogFileNo;
}

void rpxAheadAnalyzer::checkAndRemoveCompleteReplicatedTransGroup( smSN     aEndSN )
{
    mReplicatedTransGroup.checkAndRemoveCompleteReplicatedTransGroup( aEndSN );
}

void rpxAheadAnalyzer::getReplicatedTransGroupInfo( smTID           aTransID,
                                                    smSN            aSN,
                                                    idBool        * aIsFirstGroup,
                                                    idBool        * aIsLastLog,
                                                    rpdReplicatedTransGroupOperation    * aGroupOperation )
{
    mReplicatedTransGroup.getReplicatedTransGroupInfo( aTransID,
                                                       aSN,
                                                       aIsFirstGroup,
                                                       aIsLastLog,
                                                       aGroupOperation );
}

void rpxAheadAnalyzer::shutdown( void )
{
    mExitFlag = ID_TRUE;

    wakeup();
}

void rpxAheadAnalyzer::wakeup( void )
{
    IDE_ASSERT( mMutex.lock( NULL ) == IDE_SUCCESS );

    IDE_ASSERT( mCV.signal() == IDE_SUCCESS );

    IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
}

void rpxAheadAnalyzer::setExitFlag( void )
{
    mExitFlag = ID_TRUE;
}

IDE_RC rpxAheadAnalyzer::applyTableMetaLog( smTID aTID )
{
    smiTrans           sTrans;
    SInt               sStage = 0;
    idBool             sIsTxBegin = ID_FALSE;
    smiStatement     * spRootStmt = NULL;
    smiStatement       sSmiStmt;
    UInt               sFlag = RPU_ISOLATION_LEVEL |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_NOWAIT;

    rpdItemMetaEntry * sItemMetaEntry = NULL;
    smOID              sOldTableOID = SM_OID_NULL;
    smOID              sNewTableOID = SM_OID_NULL;

    IDU_FIT_POINT( "rpxAheadAnalyzer::applyTableMetaLog::initialize::sTrans",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "sTrans->initialize" );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT( "rpxAheadAnalyzer::applyTableMetaLog::begin::sTrans",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "sTrans->begin" );

    IDE_TEST( sTrans.begin( &spRootStmt, NULL, sFlag, SMX_NOT_REPL_TX_ID )
              != IDE_SUCCESS );
    sIsTxBegin = ID_TRUE;
    sStage = 2;

    while ( mTransTable.existItemMeta( aTID ) == ID_TRUE )
    {
        mTransTable.getFirstItemMetaEntry( aTID, &sItemMetaEntry );

        sOldTableOID = sItemMetaEntry->mItemMeta.mOldTableOID;
        sNewTableOID = sItemMetaEntry->mItemMeta.mTableOID;

        IDU_FIT_POINT( "rpxAheadAnalyzer::applyTableMetaLog::begin::sSmiStmt",
                       rpERR_ABORT_RP_INTERNAL_ARG,
                       "sSmiStmt->begin" );

        // Table Meta와 Table Meta Cache를 갱신한다.
        IDE_TEST( sSmiStmt.begin( NULL,
                                  spRootStmt,
                                  SMI_STATEMENT_NORMAL |
                                  SMI_STATEMENT_MEMORY_CURSOR)
                  != IDE_SUCCESS );
        sStage = 3;

        IDE_TEST( updateMeta( &sSmiStmt,
                              sItemMetaEntry,
                              sOldTableOID,
                              sNewTableOID )
                  != IDE_SUCCESS );

        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sStage = 2;

        mTransTable.removeFirstItemMetaEntry( aTID );
    }
    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sIsTxBegin = ID_FALSE;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // 이후 Sender가 종료되므로, Table Meta Cache를 복원하지 않는다
    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
            sIsTxBegin = ID_FALSE;
        case 1:
            if ( sIsTxBegin == ID_TRUE )
            {
                IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
                sIsTxBegin = ID_FALSE;
            }
            else
            {
                /* do nothing */
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::updateMeta( smiStatement     * aSmiStmt,
                                     rpdItemMetaEntry * aItemMetaEntry,
                                     smOID              aOldTableOID,
                                     smOID              aNewTableOID )
{
    rpdMetaItem  * sMetaItem = NULL;

    switch( rpdMeta::getTableMetaType( aOldTableOID, aNewTableOID ) )
    {
        case RP_META_INSERT_ITEM:
            IDE_TEST( mMeta.insertNewTableInfo( aSmiStmt,
                                                &( aItemMetaEntry->mItemMeta ),
                                                (const void *)aItemMetaEntry->mLogBody,
                                                SM_SN_NULL, // INVALID_MAX_SN
                                                ID_FALSE )
                      != IDE_SUCCESS );
            break;
        case RP_META_DELETE_ITEM:
            IDE_TEST( mMeta.deleteOldTableInfo( aSmiStmt, 
                                                (const void *)aItemMetaEntry->mLogBody,
                                                aOldTableOID,
                                                ID_FALSE ) 
                      != IDE_SUCCESS );
            break;
        case RP_META_UPDATE_ITEM:
            IDE_TEST( mMeta.searchTable( &sMetaItem, aOldTableOID ) != IDE_SUCCESS );
            IDE_TEST_RAISE( sMetaItem == NULL,  ERR_NOT_FOUND_TABLE );

            IDE_TEST( mMeta.updateOldTableInfo( aSmiStmt,
                                                sMetaItem,
                                                &( aItemMetaEntry->mItemMeta ),
                                                (const void *)aItemMetaEntry->mLogBody,
                                                SM_SN_NULL, // INVALID_MAX_SN
                                                ID_FALSE )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool rpxAheadAnalyzer::isThereGroupNode( void )
{
    return mReplicatedTransGroup.isThereGroupNode();
}

IDE_RC rpxAheadAnalyzer::readMyLog( smSN    * aCurrentSN,
                                    smLSN   * aReadLSN,
                                    SChar   * aLogHead,
                                    SChar  ** aLogPtr,
                                    idBool  * aIsValid )
{
    SChar       * sLogPtr         = NULL;
    SChar       * sLogPtrTemp     = NULL;
    SChar       * sLogHeadPtrTemp = NULL;
    idBool        sIsNeedDecompress = ID_FALSE;

    IDE_TEST_RAISE( mLogMgr.readLog( aCurrentSN,
                                     aReadLSN,
                                     (void**)&sLogHeadPtrTemp,
                                     (void**)&sLogPtrTemp,
                                     aIsValid )
                    != IDE_SUCCESS, ERR_READ_LOG_RECORD );

    if ( *aIsValid == ID_TRUE )
    {
        mAheadReadLogFileNo = aReadLSN->mFileNo;

        if ( mLogMgr.isCompressedLog( sLogPtrTemp ) != ID_TRUE )
        {
            *aLogPtr = sLogPtrTemp;
            idlOS::memcpy( aLogHead, sLogHeadPtrTemp, ID_SIZEOF(smiLogHdr) );
        }
        else
        {
            IDE_TEST( isNeedDecompress( sLogPtrTemp,
                                        sLogHeadPtrTemp,
                                        &sIsNeedDecompress )
                      != IDE_SUCCESS );

            if ( sIsNeedDecompress == ID_TRUE )
            {
                IDE_TEST( mLogMgr.decompressLog( sLogPtrTemp,
                                                 aReadLSN,
                                                 (smiLogHdr*)aLogHead,
                                                 &sLogPtr )
                          != IDE_SUCCESS );

                *aLogPtr = sLogPtr;
            }
            else
            {
                *aLogPtr = NULL;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_READ_LOG_RECORD );
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxAheadAnalyzer::isNeedDecompress( SChar  * aRawLogPtr,
                                           SChar  * aRawHeadLogPtr,
                                           idBool * aIsNeedDecompress )
{
    smOID         sTableOID = SM_OID_NULL;
    rpdMetaItem * sMetaItem = NULL;

    if ( smiLogRec::isNeedDecompressFromHdr( (smiLogHdr*)aRawHeadLogPtr ) == ID_TRUE )
    {
        *aIsNeedDecompress = ID_TRUE;
    }
    else
    {
        sTableOID = mLogMgr.getTableOID( aRawLogPtr );
        if ( sTableOID == SM_OID_NULL )
        {
            *aIsNeedDecompress = ID_TRUE;
        }
        else
        {
            IDE_TEST( mMeta.searchTable( &sMetaItem, sTableOID )
                      != IDE_SUCCESS );
            if ( sMetaItem != NULL )
            {
                *aIsNeedDecompress = ID_TRUE;
            }
            else
            {
                *aIsNeedDecompress = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

