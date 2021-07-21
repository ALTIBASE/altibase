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
 
#include <idl.h>
#include <ide.h>

#include <smiLogRec.h>

#include <rpcManager.h>
#include <rpdCatalog.h>
#include <rpdMeta.h>
#include <rpxSender.h>

#include <rpxReplicator.h>

#include <rpxAheadAnalyzer.h>

#include <rpcDDLASyncManager.h>

/*
 *
 */
rpxReplicator::rpxReplicator( void )
{

}

/*
 *
 */
IDE_RC rpxReplicator::initialize( iduMemAllocator   * aAllocator,
                                  rpxSender         * aSender,
                                  rpdSenderInfo     * aSenderInfo,
                                  rpdMeta           * aMeta,
                                  rpnMessenger      * aMessenger,
                                  rpdLogBufferMgr   * aRPLogBufMgr,
                                  idvSQL            * aOpStatistics,
                                  idvSession        * aStatSession,
                                  idBool              aIsEnableGrouping )
{
    SChar       sName[IDU_MUTEX_NAME_LEN];
    UInt        sStage = 0;
    UInt        sNumPool = 0; 
    idBool      sNeedLock = ID_FALSE;

    mSender = aSender;
    mSenderInfo = aSenderInfo;
    mMeta = aMeta;
    mMessenger = aMessenger;

    mOpStatistics =  aOpStatistics;
    mStatSession = aStatSession;

    mNeedSN = SM_SN_NULL;
    mFlushSN = SM_SN_NULL;

    mReadLogCount = 0;
    mSendLogCount = 0;

    mTransTbl = NULL;

    mSleepForKeepAliveCount = 0;

    mCurrFileNo = 0;

    mIsRPLogBufMode = ID_FALSE;
    mRPLogBufID = RP_BUF_NULL_ID;
    mRPLogBufMaxSN = SM_SN_MAX;
    mRPLogBufMinSN = 0;
    mRPLogBufMgr = aRPLogBufMgr;

    mAllocator = aAllocator;

    mAheadAnalyzer = NULL;
    mThroughput = 0;

    // BUG-29115
    if ( smiGetArchiveMode() == SMI_LOG_ARCHIVE )
    {
        idlOS::snprintf( mLogDirBuffer,
                         SM_MAX_FILE_NAME,
                         "%s",
                         smiGetArchiveDirPath() );

        mFirstArchiveLogDirPath[0] = (SChar *)mLogDirBuffer;
    }
    else
    {
        mFirstArchiveLogDirPath[0] = NULL;
    }

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_CHAINED_VALUE_POOL",
                     mSender->mRepName );

    if ( mMeta->mReplication.mReplMode != RP_EAGER_MODE )
    {
        sNumPool = 1;

        sNeedLock = ID_FALSE;
    }
    else
    {
        if ( idtCPUSet::getAvailableCPUCount() >= 2 )
        {
            sNumPool =  ( idtCPUSet::getAvailableCPUCount() / 2 );
        }
        else
        {
            sNumPool = 1;
        }

        sNeedLock = ID_TRUE;
    }

    IDE_TEST( mChainedValuePool.initialize( IDU_MEM_RP_RPX_SENDER,
                                            sName,
                                            sNumPool,
                                            RPU_REPLICATION_POOL_ELEMENT_SIZE,
                                            RPU_REPLICATION_POOL_ELEMENT_COUNT,
                                            IDU_AUTOFREE_CHUNK_LIMIT, //chunk max(default)
                                            sNeedLock,                //use mutex
                                            8,                        //align byte(default)
                                            ID_FALSE,				  //ForcePooling 
                                            ID_TRUE,				  //GarbageCollection
                                            ID_TRUE,                  /* HWCacheLine */
                                            IDU_MEMPOOL_TYPE_LEGACY   /* mempool type */
                ) != IDE_SUCCESS);			
    sStage = 1;

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN,
                     "RP_%s_SWITCH_LOG_MGR_MUTEX", mSender->mRepName );
    IDE_TEST_RAISE( mLogSwitchMtx.initialize( sName,
                                              IDU_MUTEX_KIND_POSIX,
                                              IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sStage = 2;

    mLogMgrInit = ID_FALSE;
    mLogMgrInitStatus = RP_LOG_MGR_INIT_NONE;
    mLogMgrStatus = RP_LOG_MGR_REDO; //redo log
    mNeedSwitchLogMgr = ID_FALSE;

    /* PROJ-1969 Replicated Transaction Group */
    mIsGroupingMode = aIsEnableGrouping;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl Sender] Mutex initialization error" );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)mLogSwitchMtx.destroy();
        case 1:
            (void)mChainedValuePool.destroy( ID_FALSE );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReplicator::destroy( void )
{
    if ( mTransTbl != NULL )
    {
        mTransTbl->destroy();

        (void)iduMemMgr::free( mTransTbl );
        mTransTbl = NULL;
    }

    if ( mLogSwitchMtx.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    if ( mChainedValuePool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    if ( mAheadAnalyzer != NULL )
    {
        mAheadAnalyzer->finalize();

        (void)iduMemMgr::free( mAheadAnalyzer );
        mAheadAnalyzer = NULL;

        (void)mDelayedLogQueue.finalize();
    }
    else
    {
        /* do nothing */
    }

}

/*
 *
 */
idBool rpxReplicator::isDMLLog( smiChangeLogType aTypeId )
{
    idBool sResult;

    switch ( aTypeId )
    {
        case SMI_CHANGE_MRDB_INSERT :
        case SMI_CHANGE_MRDB_DELETE :
        case SMI_CHANGE_MRDB_UPDATE :
        case SMI_CHANGE_MRDB_LOB_PARTIAL_WRITE :
        case SMI_PK_DRDB:                                   // PROJ-1705
        case SMI_REDO_DRDB_INSERT :                         // PROJ-1705
        case SMI_UNDO_DRDB_INSERT :                         // PROJ-1705
        case SMI_REDO_DRDB_DELETE :                         // PROJ-1705
        case SMI_UNDO_DRDB_DELETE :                         // PROJ-1705
        case SMI_REDO_DRDB_UPDATE :                         // PROJ-1705
        case SMI_REDO_DRDB_UPDATE_INSERT_ROW_PIECE :        // PROJ-1705
        case SMI_REDO_DRDB_UPDATE_DELETE_ROW_PIECE :        // PROJ-1705
        case SMI_REDO_DRDB_UPDATE_OVERWRITE :               // PROJ-1705
        case SMI_REDO_DRDB_UPDATE_DELETE_FIRST_COLUMN :     // PROJ-1705
        case SMI_UNDO_DRDB_UPDATE :                         // PROJ-1705
        case SMI_UNDO_DRDB_UPDATE_INSERT_ROW_PIECE :        // PROJ-1705
        case SMI_UNDO_DRDB_UPDATE_DELETE_ROW_PIECE :        // PROJ-1705
        case SMI_UNDO_DRDB_UPDATE_OVERWRITE :
        case SMI_UNDO_DRDB_UPDATE_DELETE_FIRST_COLUMN :
        case SMI_CHANGE_DRDB_LOB_PIECE_WRITE :
        case SMI_CHANGE_DRDB_LOB_PARTIAL_WRITE :
            sResult = ID_TRUE;
            break;

        default :
            sResult = ID_FALSE;
            break;
    }

    return sResult;
}

/*
 *
 */
idBool rpxReplicator::isLobControlLog( smiChangeLogType aTypeId )
{
    idBool sResult;

    switch ( aTypeId )
    {
        case SMI_CHANGE_MRDB_LOB_CURSOR_OPEN:
        case SMI_CHANGE_DRDB_LOB_CURSOR_OPEN:
        case SMI_CHANGE_LOB_CURSOR_CLOSE:
        case SMI_CHANGE_LOB_PREPARE4WRITE:
        case SMI_CHANGE_LOB_FINISH2WRITE:
        case SMI_CHANGE_LOB_TRIM:
            sResult = ID_TRUE;
            break;

        default :
            sResult = ID_FALSE;
            break;
    }

    return sResult;
}

IDE_RC rpxReplicator::checkAndSendImplSVP( smiLogRec * aLog, smTID aTID )
{
    smSN  sSN  = SM_SN_NULL;

    if ( ( aLog->checkSavePointFlag() == ID_TRUE ) &&
         ( mTransTbl->isATrans( aTID ) == ID_TRUE ) )
    {
        if ( mTransTbl->getBeginFlag( aTID ) == ID_FALSE )
        {
            mTransTbl->setBeginFlag(aTID, ID_TRUE);

            /* sender가 mBeginSN을 set하고 sender가 읽으므로
             * mBeginSN을 읽을 때 lock을 안 잡아도 됨
             */
            // BUG-17725
            sSN = mTransTbl->getTrNode( aTID )->mBeginSN;
            IDE_DASSERT( sSN != SM_SN_NULL );
        }
        else
        {
            sSN = aLog->getRecordSN();
        }

        IDE_TEST( addXLogImplSVP( aTID,
                                  sSN,
                                  aLog->getReplStmtDepth() )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


idBool rpxReplicator::needMakeMtdValue( rpdColumn * aRpdColumn )
{
    idBool sResult = ID_FALSE;

    if ( ( ( aRpdColumn->mColumn.column.flag & SMI_COLUMN_TYPE_MASK )
           == SMI_COLUMN_TYPE_LOB ) ||
         ( ( aRpdColumn->mColumn.column.flag & SMI_COLUMN_COMPRESSION_MASK )
           == SMI_COLUMN_COMPRESSION_TRUE ) )
    {
        sResult = ID_FALSE;
    }
    else
    {
        if ( ( ( aRpdColumn->mColumn.module->flag & MTD_DATA_STORE_MTDVALUE_MASK )
               == MTD_DATA_STORE_MTDVALUE_FALSE ) &&
             ( ( aRpdColumn->mColumn.module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK )
                == MTD_VARIABLE_LENGTH_TYPE_TRUE ) )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = ID_FALSE;
        }
    }
    return sResult;
}
/*
 *
 */
IDE_RC rpxReplicator::makeMtdValue( rpdLogAnalyzer * aLogAnlz,
                                    rpdMetaItem    * aMetaItem )
{
    UInt        i;
    rpdColumn * sRpdColumn = NULL;
    UInt        sCID;
    UShort      sColumnCount = 0;
    /*
     * 실제로 mtdValue를 만들지는 않는다. 이 함수에서는 데이터타입별로
     * mtdValueLen을 얻어 저장해둔다. 차후 sendValue 시에 mtdValueLen과
     * smiValue->value를 이어서 보내면, receiver는 mtdValue형태의 컬럼값을
     * 받게 되기 때문이다.
     */

    //BUG-23967 : NULL POINTER DEFENSE 추가
    IDE_ASSERT( aMetaItem != NULL );

    /* PK value
     * 1. PK COLUMN에는 null value와 lob이 해당사항이 없으므로 이들에 대한 고려가 필요없다.
     * 2. 인덱스에서 divisible 데이터는 여전히 mtdValue로 저장되고 있으므로,
     *    receiver에서 conflict체크를 위해 불러오는 PK value는 mtdValue형태이다.
     *    sender로 부터 받은 PK value와의 원활한 비교작업을 위해, sender에서 PK value를
     *    mtdValue형태로 만들어 보내도록한다.
     */
    for ( i = 0; i < aLogAnlz->mPKColCnt; i++ )
    {
        sCID = aLogAnlz->mPKCIDs[i];
        sRpdColumn = aMetaItem->getRpdColumn( sCID );

        /*BUG-26718 CodeSonar Null Pointer Dereference*/
        IDE_TEST_RAISE( sRpdColumn == NULL, COLUMN_NOT_FOUND );

        /* PK는 mtdValue로 저장되지 않는 가변길이 데이터 타입에 대해서만 처리한다.
         */
        if ( needMakeMtdValue( sRpdColumn ) == ID_TRUE )
        {
            setMtdValueLen( sRpdColumn,
                            (smiValue *)&(aLogAnlz->mPKCols[i]),
                            &(aLogAnlz->mPKMtdValueLen[i]),
                            0 );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BEFORE/AFTER IMAGE
    if ( ( aLogAnlz->mType == RP_X_INSERT ) ||
         ( aLogAnlz->mType == RP_X_UPDATE ) ||
         ( aLogAnlz->mType == RP_X_DELETE ) )
    {
        if ( aLogAnlz->mRedoAnalyzedColCnt > aLogAnlz->mUndoAnalyzedColCnt )
        {
            sColumnCount = aLogAnlz->mRedoAnalyzedColCnt;
        }
        else
        {
            sColumnCount = aLogAnlz->mUndoAnalyzedColCnt;
        }

        for ( i = 0; i < sColumnCount; i++ )
        {
            sCID = aLogAnlz->mCIDs[i];
            sRpdColumn = aMetaItem->getRpdColumn( sCID );

            /* BUG-26718 CodeSonar Null Pointer Dereference */
            IDE_TEST_RAISE( sRpdColumn == NULL, COLUMN_NOT_FOUND );

            if ( needMakeMtdValue( sRpdColumn ) == ID_TRUE )
            {
                //BEFORE
                if ( aLogAnlz->mBChainedCols[sCID].mColumn.length != 0 )
                {
                    setMtdValueLen( sRpdColumn,
                                    (smiValue *)&(aLogAnlz->mBChainedCols[sCID].mColumn),
                                    &(aLogAnlz->mBMtdValueLen[sCID]),
                                    aLogAnlz->mChainedValueTotalLen[sCID] );
                }
                else
                {
                    // Nothing to do.
                }
                //AFTER
                if ( aLogAnlz->mACols[sCID].length != 0 )
                {
                    setMtdValueLen( sRpdColumn,
                                    (smiValue *)&(aLogAnlz->mACols[sCID]),
                                    &(aLogAnlz->mAMtdValueLen[sCID]),
                                    0 );
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SENDER_NOT_FOUND_COLUMN ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReplicator::setMtdValueLen( rpdColumn  * aRpdColumn,
                                    smiValue   * aColValueArray,
                                    rpValueLen * aLenArray,
                                    UInt         aChainedValueTotalLen )
{
    UInt sMtdLenSize = 0;

    if ( ( aRpdColumn->mColumn.module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK )
         == MTD_VARIABLE_LENGTH_TYPE_FALSE )
    {
        aColValueArray->length = aRpdColumn->mColumn.column.size;

    }
    else
    {
        // 데이터 타입별 mtdValueLenSize를 얻는다.
        sMtdLenSize = aRpdColumn->mColumn.module->nullValueSize();
        aLenArray->lengthSize  = sMtdLenSize;

        // null value
        if ( aColValueArray->length == 0 )
        {
            IDE_DASSERT( aChainedValueTotalLen == 0 );
            aLenArray->lengthValue = 0;
        }
        // non-null value
        else
        {
            // before image
            if ( aChainedValueTotalLen > 0 )
            {
                aLenArray->lengthValue = (UShort)aChainedValueTotalLen;
            }
            // after image
            else
            {
                aLenArray->lengthValue = (UShort)aColValueArray->length;
            }
        }
    }
}

/*
 *
 */
IDE_RC rpxReplicator::checkEndOfLogFile( smiLogRec  * aLogRec,
                                         idBool     * aEndOfLog )
{
    *aEndOfLog = ID_FALSE;

    if ( aLogRec->getType() == SMI_LT_FILE_END )
    {
        // BUG-29837
        // 불필요한 로그 파일을 지울 수 있도록 mCommitXSN을 갱신한다.
        if ( mTransTbl->isThereATrans() != ID_TRUE )    // Transaction table에서 Active transaction이 없을 때
        {
            mSender->mCommitXSN = mSender->mXSN;
        }

        // Sender가 바쁜 상황에서도 Restart SN은 주기적으로 갱신
        IDE_TEST( mSender->addXLogKeepAlive() != IDE_SUCCESS );

        *aEndOfLog = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::checkAndWaitForLogSync( smiLogRec * aLog )
{
    UInt   i = 0;
    UInt   sGCCnt = 0;
    smTID  sTID = SM_NULL_TID;

    IDU_FIT_POINT_RAISE( "rpxReplicator::waitForLogSync::Erratic::rpERR_ABORT_INVALID_PARAM",
                         ERR_BAD_PARAM );
    IDE_TEST_RAISE( aLog == NULL, ERR_BAD_PARAM );

    /* BUG-15753
     * 프라퍼티가 설정되고 Active Transaction의 COMMIT 로그인 경우에만,
     * 로그가 디스크에 내려갈 때까지 기다린다.
     */
    if ( ( RPU_REPLICATION_SYNC_LOG != 0 ) && 
         ( mSender->mCurrentType != RP_OFFLINE ) )
    {
        // fix BUG-15753
        if ( aLog->needNormalReplicate() == ID_TRUE )
        {
            if ( aLog->getType() == SMI_LT_TRANS_COMMIT )
            {
                sTID = aLog->getTransID();

                IDE_TEST( waitForLogSync( sTID ) != IDE_SUCCESS );
            }
            else if ( aLog->getType() == SMI_LT_TRANS_GROUPCOMMIT )
            {
                sGCCnt = aLog->getGroupCommitCnt( aLog->getLogPtr() );

                for ( i = 0; i < sGCCnt; i++ )
                {
                    sTID = aLog->getNthTIDFromGroupCommit( aLog->getLogPtr(), i );

                    IDE_TEST( waitForLogSync( sTID ) != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BAD_PARAM );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PARAM ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::waitForLogSync( smTID  aTID )
{
    idBool sIsSynced = ID_FALSE;

    if ( ( mTransTbl->isATrans( aTID ) == ID_TRUE ) &&
         ( mTransTbl->getBeginFlag( aTID ) == ID_TRUE ) )
    {
        //To Fix PR-4559
        while ( mSender->checkInterrupt() == RP_INTR_NONE )
        {
            IDE_TEST( mLogMgr.isAllReadLogSynced( &sIsSynced )
                      != IDE_SUCCESS );

            if ( sIsSynced == ID_FALSE )
            {
                IDE_TEST( sleepForKeepAlive() != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
 *
 */
IDE_RC rpxReplicator::sleepForKeepAlive( void )
{
    PDL_Time_Value sTvCpu;
    PDL_Time_Value sSleepTv;
    
    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG );

    RP_OPTIMIZE_TIME_BEGIN( mOpStatistics, IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG );

    IDE_TEST_CONT( mSender->checkInterrupt() != RP_INTR_NONE, NORMAL_EXIT );

    //recovery sender
    IDE_TEST_CONT( mSender->mCurrentType == RP_RECOVERY, NORMAL_EXIT );

    //----------------------------------------------------------------//
    // reach to check keep alive
    //----------------------------------------------------------------//
    if ( mSleepForKeepAliveCount == 0 )
    {
        IDE_TEST( mSender->addXLogKeepAlive() != IDE_SUCCESS );
    }
    else
    {
        /*do nothing*/
    }
    mSleepForKeepAliveCount++;

    sSleepTv.initialize( 0, RPU_SENDER_SLEEP_TIME );

    if ( ( mMeta->getReplMode() == RP_LAZY_MODE ) ||
         ( mMeta->getReplMode() == RP_CONSISTENT_MODE ) )
    {
        idlOS::sleep( sSleepTv );
    }
    else
    {
        if ( mSleepForKeepAliveCount == RPU_KEEP_ALIVE_CNT )
        {
            sTvCpu  = idlOS::gettimeofday();
            sTvCpu += sSleepTv;

            IDE_ASSERT(mSender->mSenderInfo != NULL);
            mSender->mSenderInfo->senderWait(sTvCpu);
        }
        else
        {
            idlOS::thr_yield();
        }
    }

    //----------------------------------------------------------------//
    // reach to check keep alive
    //----------------------------------------------------------------//
    if ( mSleepForKeepAliveCount >= RPU_KEEP_ALIVE_CNT ) 
    {
        mSleepForKeepAliveCount = 0;
    }
    else
    {
        /* Nothing to do */
    }

    RP_LABEL( NORMAL_EXIT );

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG );

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReplicator::lockLogSwitch( idBool * aLocked )
{
    if ( *aLocked == ID_FALSE )
    {
        IDE_ASSERT( mLogSwitchMtx.lock( NULL ) == IDE_SUCCESS );
        *aLocked = ID_TRUE;
    }
}

/*
 *
 */
void rpxReplicator::unlockLogSwitch( idBool * aLocked )
{
    if ( *aLocked == ID_TRUE )
    {
        *aLocked = ID_FALSE;
        IDE_ASSERT( mLogSwitchMtx.unlock() == IDE_SUCCESS );
    }
}

/*
 *
 */
IDE_RC rpxReplicator::switchToArchiveLogMgr( smSN * aSN )
{
    if ( mLogMgrInit == ID_TRUE )
    {
        mLogMgrInit = ID_FALSE;
        IDE_TEST( mLogMgr.destroy() != IDE_SUCCESS );
    }

    ideLog::log( IDE_RP_0, RP_TRC_S_INIT_ARCHIVE_LOG_MGR );

    // archive log로 전환한다.
    mLogMgrStatus = RP_LOG_MGR_ARCHIVE;

    IDE_TEST( mLogMgr.initialize( mSender->mXSN,
                                  0,
                                  ID_TRUE, //리모트 로그
                                  smiGetLogFileSize(),
                                  mFirstArchiveLogDirPath,
                                  ID_TRUE ) // 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부
              != IDE_SUCCESS );

    IDE_ASSERT( getRemoteLastUsedGSN( aSN ) == IDE_SUCCESS );

    mLogMgrInit = ID_TRUE;

    ideLog::log( IDE_RP_0, RP_TRC_S_INIT_OK_LOG_MGR, *aSN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::waitForNewLogRecord( smSN              * aCurrentSN,
                                           RP_ACTION_ON_NOGAP  aAction )
{
    smSN sTmpSN = SM_SN_NULL;
    smSN sCurrentSNFromRPBuf = SM_SN_NULL;
    smSN sCurrentSNFromSM = SM_SN_NULL;

    while ( mSender->checkInterrupt() == RP_INTR_NONE )
    {
        /* BUG-31545
         * 통계정보를 세션에 반영하고 초기화한다.
         * IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG가 제대로 측정되기 위해서
         * 세션 통계정보를 업데이트할 가장 적절한 위치로 보임. 차후 위치 변경 가능.
         * 시스템 통계정보 업데이트는 주기적으로 MM쓰레드를 통해 이루어진다.
         */
        idvManager::applyOpTimeToSession( mStatSession, mOpStatistics );
        idvManager::initRPSenderAccumTime( mOpStatistics );

        if ( mSender->mXSN >= *aCurrentSN )
        {
            if( mRPLogBufMgr != NULL )
            {
                mRPLogBufMgr->getSN( &sTmpSN, &sCurrentSNFromRPBuf );
                if( sCurrentSNFromRPBuf == SM_SN_NULL )
                {
                    IDE_ASSERT( smiGetLastValidGSN( aCurrentSN ) == IDE_SUCCESS );
                }
                else
                {
                    IDE_ASSERT( smiGetLastValidGSN( &sCurrentSNFromSM ) == IDE_SUCCESS );
                    *aCurrentSN = (sCurrentSNFromRPBuf > sCurrentSNFromSM) ? sCurrentSNFromRPBuf:sCurrentSNFromSM;
                }
            }
            else
            {
                IDE_ASSERT( smiGetLastValidGSN( aCurrentSN ) == IDE_SUCCESS );
            }
        }

        if ( mSender->mXSN == *aCurrentSN )
        {
            IDE_TEST( sleepForKeepAlive() != IDE_SUCCESS );
            //gap is 0
            if ( aAction == RP_WAIT_ON_NOGAP )
            {
                //nothing todo
            }
            else
            {
                //if (aAction == RP_RETURN_ON_NOGAP) return;
                break;
            }
        }
        else // Ok. New Log Record Arrived..
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::sendXLog( rpdLogAnalyzer * aLogAnlz )
{
    IDE_RC   sRC;
    idBool   sNeedLock  = ID_FALSE;
    idBool   sNeedFlush = ID_FALSE;

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_S_SEND_XLOG );

    RP_OPTIMIZE_TIME_BEGIN( mOpStatistics, IDV_OPTM_INDEX_RP_S_SEND_XLOG );

    IDE_TEST_CONT( mSender->checkHBTFault() != IDE_SUCCESS,
                   NORMAL_EXIT );

    IDE_TEST_CONT( mSender->isSkipLog( aLogAnlz->mSN ) == ID_TRUE, NORMAL_EXIT );

    // TASK-2359
    if ( RPU_REPLICATION_PERFORMANCE_TEST <= 1 )
    {
        if ( ( mSender->getStatus() == RP_SENDER_FLUSH_FAILBACK ) ||
             ( mSender->getStatus() == RP_SENDER_IDLE ) )
        {
            sNeedLock  = ID_TRUE;
        }
        else
        {
            sNeedLock  = ID_FALSE;
        }

        if ( ( mMeta->getReplMode() == RP_EAGER_MODE ) ||
             ( mMeta->getReplMode() == RP_CONSISTENT_MODE ) )
        {
            sNeedFlush = ID_TRUE;
        }
        else
        {
            sNeedFlush = ID_FALSE;
        }

        sRC = mMessenger->sendXLog( mTransTbl,
                                    mSenderInfo,
                                    aLogAnlz,
                                    mFlushSN,
                                    sNeedLock,
                                    sNeedFlush );
    }
    else
    {
        sRC = IDE_SUCCESS;
    }
    
    mSender->increaseLogCount( aLogAnlz->mType, aLogAnlz->mTableOID );

    if ( sRC != IDE_SUCCESS )
    {
        IDE_TEST_RAISE( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
                        ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) || 
                        ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK );
        IDE_RAISE( ERR_ETC );
    }

    if ( ( mTransTbl->getSendBeginFlag( aLogAnlz->getSendTransID() ) == ID_FALSE ) &&
         ( mTransTbl->getRemoteTID( aLogAnlz->getSendTransID() ) == aLogAnlz->getSendTransID() ) )
    {

        mTransTbl->setSendBeginFlag( aLogAnlz->getSendTransID(), ID_TRUE );
    }

    IDE_TEST_CONT( mSender->checkHBTFault() != IDE_SUCCESS,
                   NORMAL_EXIT );

    RP_LABEL( NORMAL_EXIT );

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_SEND_XLOG );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_SEND_XLOG );

        mSender->setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( ERR_ETC );
    {
        // Recovery Sender가 Commit에서 rpnComm::recvAck()를 실패한 경우에도 여기로 온다.
    }
    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_SEND_XLOG );

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::addXLogSyncPK( rpdMetaItem    * aMetaItem,
                                     rpdLogAnalyzer * aLogAnlz )
{
    UInt        sPKColCnt;
    smiValue    sPKCols[QCI_MAX_KEY_COLUMN_COUNT];
    rpValueLen  sPKMtdValueLen[QCI_MAX_KEY_COLUMN_COUNT];

    qcmIndex   *sPKIndex = NULL;
    SInt        sIndex;
    UInt        sColID;
    IDE_RC      sRC      = IDE_SUCCESS;

    IDE_DASSERT( ( mSender->mStatus == RP_SENDER_FAILBACK_SLAVE ) ||
                 ( mSender->mCurrentType == RP_XLOGFILE_FAILBACK_SLAVE ) );

    switch ( aLogAnlz->mType )
    {
        case RP_X_INSERT :  // Priamry Key가 없으므로, After Image에서 얻는다.
            IDE_DASSERT( aMetaItem != NULL );

            // PK Column Count를 얻는다.
            sPKColCnt = aMetaItem->mPKColCount;

            // PK Index를 검색한다.
            for ( sIndex = 0; sIndex < aMetaItem->mIndexCount; sIndex++ )
            {
                if ( aMetaItem->mPKIndex.indexId
                     == aMetaItem->mIndices[sIndex].indexId )
                {
                    sPKIndex = &aMetaItem->mIndices[sIndex];
                    break;
                }
            }
            IDE_ASSERT( sPKIndex != NULL );

            // PK Column Value와 MT Length를 얻는다.
            idlOS::memset( sPKCols,
                           0x00,
                           ID_SIZEOF( smiValue ) * QCI_MAX_KEY_COLUMN_COUNT );
            idlOS::memset( sPKMtdValueLen,
                           0x00,
                           ID_SIZEOF( rpValueLen ) * QCI_MAX_KEY_COLUMN_COUNT );

            for ( sIndex = 0; sIndex < (SInt)sPKColCnt; sIndex++ )
            {
                sColID = sPKIndex->keyColumns[sIndex].column.id
                       & SMI_COLUMN_ID_MASK;
                IDE_ASSERT( sColID < aLogAnlz->mRedoAnalyzedColCnt );

                (void)idlOS::memcpy( &sPKCols[sIndex],
                                     &aLogAnlz->mACols[sColID],
                                     ID_SIZEOF(smiValue) );

                (void)idlOS::memcpy( &sPKMtdValueLen[sIndex],
                                     &aLogAnlz->mAMtdValueLen[sColID],
                                     ID_SIZEOF( rpValueLen ) );
            }
            break;

        case RP_X_UPDATE :  // 이미 Primary Key가 있으므로, 복사한다.
        case RP_X_DELETE :
        case RP_X_LOB_CURSOR_OPEN :
            sPKColCnt = aLogAnlz->mPKColCnt;
            (void)idlOS::memcpy( sPKCols,
                                 aLogAnlz->mPKCols,
                                 ID_SIZEOF( smiValue ) * QCI_MAX_KEY_COLUMN_COUNT );
            (void)idlOS::memcpy( sPKMtdValueLen,
                                 aLogAnlz->mPKMtdValueLen,
                                 ID_SIZEOF( rpValueLen ) * QCI_MAX_KEY_COLUMN_COUNT );
            break;

        default :
            IDE_CONT( NORMAL_EXIT );
    }


    IDE_TEST_CONT( mSender->checkHBTFault() != IDE_SUCCESS,
                   NORMAL_EXIT );

    sRC = mMessenger->sendXLogSyncPK( aLogAnlz->mTableOID,
                                      sPKColCnt,
                                      sPKCols,
                                      sPKMtdValueLen );
    if ( sRC != IDE_SUCCESS )
    {
        IDE_TEST_RAISE( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
                        ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) || 
                        ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK );
        IDE_RAISE( ERR_ETC );
    }

    IDE_TEST_CONT( mSender->checkHBTFault() != IDE_SUCCESS,
                   NORMAL_EXIT );

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        mSender->setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( ERR_ETC );
    {
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::addXLogImplSVP( smTID aTID,
                                      smSN  aSN,
                                      UInt  aReplStmtDepth )
{
    SChar  * sSpName    = NULL;
    UInt     sSpNameLen = 0;
    idBool   sNeedLock  = ID_FALSE;
    IDE_RC   sRC = IDE_SUCCESS;

    IDE_TEST_CONT( mSender->checkHBTFault() != IDE_SUCCESS,
                   NORMAL_EXIT );
    
    /* 상위 statement가 root가 아닌 statement에 대해서
     * rollback(partial-rollback)을 지원해야 하는
     * 버그를 수정하기 위해서 sm에서 implicit svp Log에 statement depth를
     * 추가하기로 하였다.
     * replication에서는 로그의 header에 statement depth를 보고 현재
     * implicit svp에 대해서 어떤 이름으로 savepoint를 지정해야 하는 지 알수
     * 있으며 따라서 기존에 implicit svp에 대해서 XLog header만 보내는 것을
     * 제거하고 implicit svp로그가 들어온 경우에 explicit svp(RP_X_SP_SET)로
     * 보내도록 하며, 이름은 $$IMPLICIT + depth로 지정하기로 하였다.
     * 좀 더 상세한 내용은 smxDef.h의 주석을 참조
     */
    //append sp $$IMPLICIT(SMR_IMPLICIT_SVP_NAME) + Replication Impl Svp Stmt Depth
    sSpName = rpcManager::getImplSPName( aReplStmtDepth );

    if ( mTransTbl->isSvpListSent( aTID ) != ID_TRUE )
    {
        // BUG-28206 불필요한 Transaction Begin을 방지
        sRC = mTransTbl->addLastSvpEntry( aTID,
                                          aSN,
                                          RP_SAVEPOINT_IMPLICIT,
                                          sSpName,
                                          aReplStmtDepth );
    }
    else
    {
        if ( ( mSender->getStatus() == RP_SENDER_FLUSH_FAILBACK ) ||
             ( mSender->getStatus() == RP_SENDER_IDLE ) )
        {
            sNeedLock  = ID_TRUE;
        }
        else
        {
            sNeedLock  = ID_FALSE;
        }

        sSpNameLen = idlOS::strlen( sSpName );
        sRC = mMessenger->sendXLogSPSet( aTID,
                                         aSN,
                                         mFlushSN,
                                         sSpNameLen,
                                         sSpName,
                                         sNeedLock );
    }

    if ( sRC != IDE_SUCCESS )
    {
        IDE_TEST_RAISE( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
                        ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
                        ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ),
                        ERR_NETWORK );
        IDE_RAISE( ERR_ETC );
    }

    IDE_TEST_CONT( mSender->checkHBTFault() != IDE_SUCCESS,
                   NORMAL_EXIT );

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        mSender->setNetworkErrorAndDeactivate();

        return IDE_SUCCESS;
    }
    IDE_EXCEPTION( ERR_ETC );
    {
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
extern "C" int compareCID( const void * aElem1, const void * aElem2 )
{
    IDE_DASSERT( (UInt *)aElem1 != NULL );
    IDE_DASSERT( (UInt *)aElem2 != NULL );

    if ( *((UInt *)aElem1) > *((UInt *)aElem2) )
    {
        return 1;
    }
    else if ( *((UInt *)aElem1) < *((UInt *)aElem2) )
    {
        return -1;
    }
    return 0;
}

IDE_RC rpxReplicator::convertBeforeColDisk( rpdMetaItem    * aMetaItem,
                                            rpdLogAnalyzer * aLogAnlz )
{
    UInt              i;
    UInt              sCID;
    smiChainedValue   sOIDTmp;
    void             *sValue = NULL;

    for ( i = 0 ; i < aMetaItem->mCompressColCount ; i++ )
    {
        sCID = aMetaItem->mCompressCID[i];

        if ( aLogAnlz->mBChainedCols[sCID].mColumn.length != 0 )
        {
            sOIDTmp.mColumn.value  = (void*)(aLogAnlz->mBChainedCols[sCID].mColumn.value);
            sOIDTmp.mColumn.length = aLogAnlz->mBChainedCols[sCID].mColumn.length;
            sOIDTmp.mAllocMethod   = aLogAnlz->mBChainedCols[sCID].mAllocMethod;

            convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                               aLogAnlz,
                               &(aLogAnlz->mBChainedCols[sCID].mColumn) );

            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                               aLogAnlz->mBChainedCols[sCID].mColumn.length,
                                               (void**)&sValue,
                                               IDU_MEM_IMMEDIATE,
                                               mAllocator )
                            != IDE_SUCCESS, ERR_MALLOC );

            aLogAnlz->mBChainedCols[sCID].mAllocMethod = SMI_NORMAL_ALLOC;
            
            idlOS::memcpy( sValue,
                           (const void*)(aLogAnlz->mBChainedCols[sCID].mColumn.value),
                           aLogAnlz->mBChainedCols[sCID].mColumn.length );

            aLogAnlz->mBChainedCols[sCID].mColumn.value = sValue;
            aLogAnlz->mChainedValueTotalLen[sCID]       = aLogAnlz->mBChainedCols[sCID].mColumn.length;
            
            switch ( sOIDTmp.mAllocMethod )
            {
                case SMI_NORMAL_ALLOC:
                    (void)iduMemMgr::free( (void*)(sOIDTmp.mColumn.value), mAllocator );
                    break;
                case SMI_MEMPOOL_ALLOC:
                    (void)mChainedValuePool.memfree( (void*)(sOIDTmp.mColumn.value) );
                    break;
                default:
                    break;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;
 
    IDE_EXCEPTION ( ERR_MALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReplicator::convertBeforeColDisk",
                                  "sValue" ) );

        aLogAnlz->mBChainedCols[sCID].mColumn.value  = sOIDTmp.mColumn.value;
        aLogAnlz->mBChainedCols[sCID].mColumn.length = sOIDTmp.mColumn.length;
        aLogAnlz->mChainedValueTotalLen[sCID]        = sOIDTmp.mColumn.length;
    }  
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::convertAfterColDisk( rpdMetaItem    * aMetaItem,
                                           rpdLogAnalyzer * aLogAnlz )
{
    UInt              i;
    UInt              sCID;
    smiValue          sOIDTmp;
    void             *sValue = NULL;

    for ( i = 0 ; i < aMetaItem->mCompressColCount ; i++ )
    {
        sCID = aMetaItem->mCompressCID[i];

        if ( aLogAnlz->mACols[sCID].length != 0 )
        {
            sOIDTmp.value  = (void*)(aLogAnlz->mACols[sCID].value);
            sOIDTmp.length = aLogAnlz->mACols[sCID].length;

            convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                               aLogAnlz,
                               &(aLogAnlz->mACols[sCID]) );

            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                               aLogAnlz->mACols[sCID].length,
                                               (void**)&sValue,
                                               IDU_MEM_IMMEDIATE,
                                               mAllocator )
                            != IDE_SUCCESS, ERR_MALLOC );

            idlOS::memcpy( sValue,
                           (const void*)(aLogAnlz->mACols[sCID].value),
                           aLogAnlz->mACols[sCID].length );

            aLogAnlz->mACols[sCID].value = sValue;
            
            (void)iduMemMgr::free( (void*)(sOIDTmp.value), mAllocator );
        }
        else
        {
            /* Nothing to do*/
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION ( ERR_MALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReplicator::convertAfterColDisk",
                                  "sValue" ) );

        aLogAnlz->mACols[sCID].value  = sOIDTmp.value;
        aLogAnlz->mACols[sCID].length = sOIDTmp.length; 

    }   
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::convertAllOIDToValue( rpdMetaItem    * aMetaItem,
                                            rpdLogAnalyzer * aLogAnlz,
                                            smiLogRec      * aLog )
{
    UInt        i;
    UInt        sCID;

    switch ( aLog->getType() )
    {
        case SMI_LT_MEMORY_CHANGE :

        switch ( aLogAnlz->mType )
        {
            case RP_X_INSERT :

                for ( i = 0 ; i < aMetaItem->mCompressColCount ; i++ )
                {
                    sCID = aMetaItem->mCompressCID[i];

                    convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                                       aLogAnlz,
                                       &(aLogAnlz->mACols[sCID]) );
                }
                break;
            case RP_X_UPDATE :

                switch ( aLog->getLogUpdateType() )
                {
                    case SMR_SMC_PERS_UPDATE_INPLACE_ROW:

                        for ( i = 0 ; i < aMetaItem->mCompressColCount ; i++ )
                        {
                            sCID = aMetaItem->mCompressCID[i];
                            if ( aLogAnlz->isInCIDArray( sCID ) == ID_TRUE )
                            {
                                convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                                                   aLogAnlz,
                                                   &(aLogAnlz->mBCols[sCID]) );

                                convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                                                   aLogAnlz,
                                                   &(aLogAnlz->mACols[sCID]) );
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }
                        break;
                    case SMR_SMC_PERS_UPDATE_VERSION_ROW:

                        for ( i = 0 ; i < aMetaItem->mCompressColCount ; i++ )
                        {
                            sCID = aMetaItem->mCompressCID[i];
                            if ( aLogAnlz->isInCIDArray( sCID ) == ID_TRUE )
                            {
                                convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                                                   aLogAnlz,
                                                   &(aLogAnlz->mBCols[sCID]) );
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                            convertOIDToValue( &(aMetaItem->mColumns[sCID].mColumn.column),
                                               aLogAnlz,
                                               &(aLogAnlz->mACols[sCID]) );
                        }
                        break;
                    default:
                        IDE_ASSERT( 0 );
                        break;
                }
                break;
            default:
                break;
        }
        break;

        case SMI_LT_DISK_CHANGE :

            switch ( aLogAnlz->mType )
            {
                case RP_X_UPDATE: 
                    IDE_TEST( convertBeforeColDisk( aMetaItem, aLogAnlz ) != IDE_SUCCESS );
                    /* fall through */
                case RP_X_INSERT:
                    IDE_TEST( convertAfterColDisk( aMetaItem, aLogAnlz ) != IDE_SUCCESS );
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReplicator::convertOIDToValue( smiColumn  * aColumn,
                                       rpdLogAnalyzer * aLogAnlz,
                                       smiValue   * aValue )
{
    rpdDictionaryValue  * sDictValue;
    smOID                 sCompOID;

    idlOS::memcpy( &sCompOID,
                   aValue->value,
                   ID_SIZEOF(vULong) );

    sDictValue = aLogAnlz->getDictValueFromList( sCompOID );

    if ( sDictValue == NULL )
    {

        aValue->value = smiGetCompressionColumnFromOID( &sCompOID,
                                                        aColumn,
                                                        &(aValue->length) );
    }
    else
    {
        aValue->value = sDictValue->mValue.value;
        aValue->length = sDictValue->mValue.length;
    }
}

/*
 *
 */
IDE_RC rpxReplicator::addXLog( smiLogRec             * aLog,
                               rpdMetaItem           * aMetaItem,
                               RP_ACTION_ON_ADD_XLOG   aAction,
                               iduMemPool            * aSNPool,
                               iduList               * aSNList,
                               RP_REPLICATIED_TRNAS_GROUP_OP    aOperation,
                               smTID                    aTID )
{
    idBool             sIsDML;
    rpdLogAnalyzer   * sLogAnlz = NULL;
    smiTableMeta     * sItemMeta = NULL;
    smiDDLStmtMeta   * sDDLStmtMeta = NULL;
    smiLogType         sLogType;
    smiChangeLogType   sChangeLogType;
    idBool             sNeedInitMtdValueLen = ID_FALSE;
    UInt               sTableColCount = 0;
    UShort             sColumnCount = 0;
    idBool             sIsExist = ID_FALSE;
    idBool             sNeedHandshake = ID_FALSE;
    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_S_LOG_ANALYZE );

    if ( ( aAction == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG ) ||
         ( aAction == RP_SEND_SYNC_PK_ON_ADD_XLOG ) )
    {
        IDE_ASSERT( aSNPool != NULL );
    }

    IDE_DASSERT( mTransTbl->isATrans( aTID ) == ID_TRUE );

    sLogAnlz = mTransTbl->getLogAnalyzer( aTID );
    sLogType = aLog->getType();
    sChangeLogType = aLog->getChangeType();

    sLogAnlz->setNeedFree( ID_FALSE );
    sLogAnlz->setSkipXLog( ID_FALSE );

    RP_OPTIMIZE_TIME_BEGIN( mOpStatistics, IDV_OPTM_INDEX_RP_S_LOG_ANALYZE );

    IDE_TEST( sLogAnlz->analyze( aLog,
                                 &sIsDML,
                                 aTID )
              != IDE_SUCCESS );
    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_LOG_ANALYZE );

    if ( ( sChangeLogType != SMI_CHANGE_MRDB_LOB_PARTIAL_WRITE ) &&
         ( sChangeLogType != SMI_CHANGE_DRDB_LOB_PARTIAL_WRITE ) &&
         ( sLogAnlz->isCont() == ID_TRUE ) )
    {
            IDE_RAISE( SKIP_XLOG );
    }
    else
    {
        if ( sIsDML == ID_TRUE )
        {
            // PROJ-1705 - UNDO LOG 또는 분석할 컬럼이 없거나 분석하지 않는 로그 타입
            if ( sLogAnlz->skipXLog() == ID_TRUE )
            {
                IDE_RAISE( SKIP_XLOG );
            }
        }
    }

    /* LOB Cursor Open은 로그 분석 후에 Table OID를 얻을 수 있다. */
    if ( ( sLogType == SMI_LT_LOB_FOR_REPL ) &&
         ( sLogAnlz->mType == RP_X_LOB_CURSOR_OPEN ) )
    {
        IDE_DASSERT( aMetaItem == NULL );
        aMetaItem = NULL;
        IDE_TEST( mMeta->searchTable( &aMetaItem, sLogAnlz->mTableOID )
                  != IDE_SUCCESS );
    }

    /* 빠른 변수 초기화를 위해, Table Column Count를 얻어놓는다. */
    if ( aMetaItem != NULL )
    {
        sTableColCount = (UInt)aMetaItem->mColCount;
    }

    /* BUG-24398 로그 타입이 LOB Cursor Open이면 Replication 대상인지 확인한다
     * 한 테이블에 Lob column이 여러 개일 때 Lob Cursor Open과 Close 사이에 또 다른 Cursor Open이 되는 경우에 대해서
     * Receiver쪽에서는  Lob column 반영시 각각 따로 반영해야하기 때문에 리스트로 관리하나
     * Sender쪽에서는 전부 보낼지 말지만을 판단하면 되므로 flag 하나로 해결이 됩니다.
     */
    if ( sLogType == SMI_LT_LOB_FOR_REPL )
    {
        if ( sLogAnlz->mType == RP_X_LOB_CURSOR_OPEN )
        {
            /*BUG-24417
             *Lob 관련 XLog에도 Invalid Max SN을 검사합니다.
             */
            if ( ( aMetaItem == NULL ) ||
                 ( aLog->getRecordSN() <= aMetaItem->mItem.mInvalidMaxSN ) )
            {
                mTransTbl->setSendLobLogFlag( aTID, ID_FALSE );
                /* Lob operation의 경우엔, addXLog까지 진행되었더라도, 이중화 대상 테이블이 아니거나
                 * invalid Max SN보다 작은 경우엔 sendXLog를 하지 않으므로, addXLog() 전에 증가시킨
                 * mSendLogCount를 다시 감소 시켜주어야한다.
                 */
                idCore::acpAtomicDec64( &mSendLogCount );
                IDE_RAISE( RESET_XLOG );
            }
            else
            {
                mTransTbl->setSendLobLogFlag( aTID, ID_TRUE );
            }
        }
        else
        {
            if ( mTransTbl->getSendLobLogFlag( aTID ) == ID_FALSE )
            {
                idCore::acpAtomicDec64( &mSendLogCount );
                IDE_RAISE( RESET_XLOG );
            }
        }
    }

    /*
     * PROJ-1705
     * column CID 정렬
     * column 분석 순서가 왔다갔다 하니, row 분석이 끝난 후 정렬해준다.
     */
    if ( sLogAnlz->mRedoAnalyzedColCnt > sLogAnlz->mUndoAnalyzedColCnt )
    {
        sColumnCount = sLogAnlz->mRedoAnalyzedColCnt;
    }
    else
    {
        sColumnCount = sLogAnlz->mUndoAnalyzedColCnt;
    }
    idlOS::qsort( sLogAnlz->mCIDs,
                  sColumnCount,
                  ID_SIZEOF( UInt ),
                  compareCID );

    /* PROJ-2397 Compressed Table Replication */
    if ( ( aMetaItem != NULL ) && ( aMetaItem->mCompressColCount > 0 ) )
    {
        IDE_TEST( convertAllOIDToValue( aMetaItem, sLogAnlz, aLog ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }


    if ( ( sLogType != SMI_LT_NULL ) || ( sChangeLogType != SMI_CHANGE_NULL ) )
    {
        /* Type과 Change Type이 위와 같이 되는 경우는 LOCK TABLE 등에서
         * update before와 같은 형태의 로그이지만, update column count가 0인 경우이다.
         * 이때는 실제적으로 할일이 아무것도 없으므로, 넘어갈수 있게 한다.
         */

        if ( sLogType == SMI_LT_TABLE_META )
        {
            sItemMeta = aLog->getTblMeta();
            ideLog::log( IDE_RP_0, RP_TRC_SX_NTC_TABLE_META_LOG,
                         (ULong)sItemMeta->mOldTableOID,
                         (ULong)sItemMeta->mTableOID,
                         sItemMeta->mUserName,
                         sItemMeta->mTableName,
                         sItemMeta->mPartName );

            // Table Meta Log Record를 Transaction Table에 보관한다.
            IDE_TEST( mTransTbl->addItemMetaEntry( aTID,
                                                   sItemMeta,
                                                   (const void *)aLog->getTblMetaLogBodyPtr(),
                                                   aLog->getTblMetaLogBodySize(),
                                                   aLog->getRecordSN())
                      != IDE_SUCCESS );
        }
        else if ( sLogType == SMI_LT_DDL_QUERY_STRING )
        {
            sDDLStmtMeta = aLog->getDDLStmtMeta();

            IDE_TEST( mTransTbl->setDDLStmtMetaLog( aTID,
                                                    sDDLStmtMeta,
                                                    (const void *)aLog->getDDLStmtMetaLogBodyPtr(),
                                                    aLog->getDDLStmtMetaLogBodySize() )
                      != IDE_SUCCESS );
        }
        else
        {
            /*
             * PROJ-1705 RP
             * mtdValue로 전송할 수 있도록 mtdValueLen정보를 얻는다.
             */
            if ( ( sLogType == SMI_LT_DISK_CHANGE ) ||
                 ( sChangeLogType == SMI_CHANGE_DRDB_LOB_CURSOR_OPEN ) ) // Disk Lob의 경우 PK value를 mtdValue로 변환한다.
            {
                IDE_TEST( makeMtdValue( sLogAnlz, aMetaItem ) != IDE_SUCCESS );
                sNeedInitMtdValueLen = ID_TRUE;
            }

            if ( mTransTbl->getBeginFlag( aTID ) == ID_FALSE )
            {
                mTransTbl->setBeginFlag( aTID, ID_TRUE );
                /* sender가 mBeginSN을 set하고 sender가 읽으므로
                 * mBeginSN을 읽을 때 lock을 안 잡아도 됨
                 */
                // BUG-17725
                sLogAnlz->mSN = mTransTbl->getTrNode( aTID )->mBeginSN;
                IDE_DASSERT( sLogAnlz->mSN != SM_SN_NULL );
            }

            switch ( aAction )
            {
                case RP_SEND_XLOG_ON_ADD_XLOG :
 
                    switch ( aOperation )
                    {
                        case RP_REPLICATIED_TRNAS_GROUP_NORMAL:

                            IDE_TEST( sendXLog( sLogAnlz ) != IDE_SUCCESS );
                            break;

                        case RP_REPLICATIED_TRNAS_GROUP_SEND:

                            sLogAnlz->setSendTransID( mAheadAnalyzer->getReplicatedTransactionGroupTID( aTID ) );
                            IDE_TEST( sendXLog( sLogAnlz ) != IDE_SUCCESS );
                            break;

                        case RP_REPLICATIED_TRNAS_GROUP_DONT_SEND:

                            if ( ( sLogAnlz->mType == RP_X_COMMIT ) &&
                                 ( mSender->mCurrentType == RP_RECOVERY ) )
                            {
                                mSender->mSNMapMgr->deleteTxByReplicatedCommitSN( sLogAnlz->mSN,
                                                                                  &sIsExist );

                                IDE_DASSERT( sIsExist == ID_TRUE );
                            }
                            else
                            {
                                /* do nothing */
                            }

                            /* DO NOTHING */
                            break;
                            
                        default:
                            IDE_DASSERT( 0 );
                            break;
                    }
                    break;

                case RP_COLLECT_BEGIN_SN_ON_ADD_XLOG :
                    // Failback Slave가 Committed Transaction의 Begin SN을 수집한다.
                    if ( sLogAnlz->mType == RP_X_COMMIT )
                    {
                        IDE_TEST( rpcManager::addLastSNEntry( aSNPool,
                                                              mTransTbl->getTrNode( aTID )->mBeginSN,
                                                              aSNList )
                                  != IDE_SUCCESS );
                    }
                    break;

                case RP_SEND_SYNC_PK_ON_ADD_XLOG :
                    // Failback Slave가 Incremental Sync에 필요한 Primary Key를 전송한다.
                    IDE_TEST( addXLogSyncPK( aMetaItem, sLogAnlz )
                              != IDE_SUCCESS );
                    break;

                default :
                    IDE_ASSERT( 0 );
            }
        }
    }
    else
    {
        /* BUG-23195
         * SMI_LT_NULL && SMI_CHANGE_NULL 인 경우에도
         * Implicit Savepoint가 설정되어 있으면
         * (1) Begin Flag가 미설정되어 있으면 Begin Flag를 설정하고,
         * (2) Implicit Savepoint XLog 전송합니다.
         */
        if ( sLogAnlz->mImplSPDepth != SMI_STATEMENT_DEPTH_NULL )
        {
            if ( mTransTbl->getBeginFlag( aTID ) == ID_FALSE )
            {
                mTransTbl->setBeginFlag( aTID, ID_TRUE );
                /* sender가 mBeginSN을 set하고 sender가 읽으므로
                 * mBeginSN을 읽을 때 lock을 안 잡아도 됨
                 */
                // BUG-17725
                sLogAnlz->mSN = mTransTbl->getTrNode(aTID)->mBeginSN;
                IDE_DASSERT( sLogAnlz->mSN != SM_SN_NULL );
            }

            if ( aAction == RP_SEND_XLOG_ON_ADD_XLOG )
            {
                IDE_TEST( addXLogImplSVP( aTID,
                                          sLogAnlz->mSN,
                                          sLogAnlz->mImplSPDepth )
                          != IDE_SUCCESS );
            }
        }
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * DDL Transaction이 Commit되면,
     * Table Meta Log Record를 반영하고 Handshake를 수행한다.
     */
    if ( ( sLogAnlz->mType == RP_X_COMMIT ) &&
         ( mTransTbl->isDDLTrans( aTID ) == ID_TRUE ) &&
         ( mTransTbl->existItemMeta( aTID ) == ID_TRUE ) )
    {
        // Failback Slave에서 Failback 전에 DDL을 수행하면 안 된다.
        IDE_ASSERT( aAction == RP_SEND_XLOG_ON_ADD_XLOG );

        if ( mTransTbl->existDDLStmtMetaLog( aTID ) == ID_TRUE )
        {
            IDE_TEST( rpcDDLASyncManager::ddlASynchronization( mSender,
                                                               aTID,
                                                               aLog->getRecordSN() ) 
                      != IDE_SUCCESS );

            mTransTbl->removeDDLStmtMetaLog( aTID );
        }

        if ( mSender->checkInterrupt() == RP_INTR_NONE )
        {
            IDE_TEST( applyTableMetaLog( aTID,
                                         mTransTbl->getTrNode( aTID )->mBeginSN,
                                         aLog->getRecordSN(),
                                         &sNeedHandshake )
                      != IDE_SUCCESS );
        }

        /* PROJ-2563
         * 만일 V6프로토콜을 사용할 때,
         * DDL 수행 후 LOB column 이 포함되어 있다면 시작을 실패해야한다.
         */
        if ( rpdMeta::isUseV6Protocol( &( mMeta->mReplication ) ) == ID_TRUE )
        {
            IDE_TEST_RAISE( mMeta->hasLOBColumn() == ID_TRUE,
                            ERR_NOT_SUPPORT_LOB_COLUMN_WITH_V6_PROTOCOL );
        }
        else
        {
            /* do nothing */
        }

        if ( (mSender->checkInterrupt() == RP_INTR_NONE) &&
             (sNeedHandshake == ID_TRUE) )
        {
            IDE_TEST( mSender->handshakeWithoutReconnect( aTID ) != IDE_SUCCESS );
        }
    }

    RP_LABEL( RESET_XLOG );

    if ( sLogAnlz->getNeedFree() == ID_TRUE )
    {
        sLogAnlz->freeColumnValue( ID_FALSE );    // PROJ-1705
        sLogAnlz->setNeedFree( ID_FALSE );
    }

    if ( ( sLogAnlz->mType == RP_X_COMMIT ) ||
         ( sLogAnlz->mType == RP_X_ABORT ) )
    {
        //------------------------------------------------------//
        // remove in transaction table
        //------------------------------------------------------//
        mTransTbl->removeTrans(aTID);
    }

    /* BUG-28564 pk column count변수 초기화 */
    sLogAnlz->resetVariables( sNeedInitMtdValueLen, sTableColCount );
    sNeedInitMtdValueLen = ID_FALSE;

    RP_LABEL( SKIP_XLOG );

    IDU_FIT_POINT( "1.TASK-2004@rpxReplicator::addXLog" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_LOB_COLUMN_WITH_V6_PROTOCOL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_SUPPORT_FEATURE_WITH_V6_PROTOCOL,
                                  "Replication with LOB columns") );
    }
    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_LOG_ANALYZE );

    IDE_ERRLOG( IDE_RP_0 );
    IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_ADD_XLOG,
                              aTID,
                              aLog->getRecordSN(),
                              sLogAnlz->mType,
                              sLogType,
                              sChangeLogType ) );

    // BUGBUG : sLogAnlz->analyze에서 할당한 메모리를 해제해야 합니다.

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::applyTableMetaLog( smTID aTID,
                                         smSN  aDDLBeginSN,
                                         smSN  aDDLCommitSN,
                                         idBool * aOutNeedHandshake )
{
    smiTrans           sTrans;
    SInt               sStage = 0;
    idBool             sIsTxBegin = ID_FALSE;
    smiStatement     * spRootStmt;    
    smiStatement       sSmiStmt;
    UInt               sFlag = SMI_ISOLATION_NO_PHANTOM |
                               SMI_TRANSACTION_NORMAL |
                               SMI_TRANSACTION_REPL_NONE |
                               SMI_COMMIT_WRITE_NOWAIT;

    rpdItemMetaEntry * sItemMetaEntry;
    PDL_Time_Value     sTimeValue;
    smOID              sOldTableOID = SM_OID_NULL;
    smOID              sNewTableOID = SM_OID_NULL;
    rpdMeta            sTempMeta;
    rpdMeta            sPreviousMeta;
    rpdMeta            sPostMeta;
    UInt               sLockWaitSec = 10;
    idBool             sIsUpdated = ID_FALSE;
    idBool             sIsMetaUpdated = ID_FALSE;
    idBool             sNeedRehandshake = ID_FALSE;

    sTimeValue.initialize( 1, 0 );
    sTempMeta.initialize();
    sPreviousMeta.initialize();
    sPostMeta.initialize();
    IDE_TEST_RAISE(mSender->mCurrentType == RP_OFFLINE, ERR_OFFLINE_SENDER );

    // BUG-24427 [RP] Network 작업 후, Meta Cache를 갱신해야 합니다
    // DDL 전에 발생한 DML이 Standby Server에 반영되기를 기다린다.
    while ( aDDLBeginSN > mSender->getLastProcessedSN() )
    {
        IDE_TEST( mSender->addXLogKeepAlive() != IDE_SUCCESS );
        IDE_TEST_CONT( mSender->checkInterrupt() != RP_INTR_NONE,
                       NORMAL_EXIT );

        idlOS::sleep( sTimeValue );
    }

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &spRootStmt, NULL, sFlag, RP_UNUSED_RECEIVER_INDEX )
              != IDE_SUCCESS );
    sIsTxBegin = ID_TRUE;
    sStage = 2;
    IDE_TEST( sTrans.setReplTransLockTimeout( sLockWaitSec ) != IDE_SUCCESS );

    for(;;)
    {
        // Table Meta와 Table Meta Cache를 갱신한다.
        IDE_TEST( sSmiStmt.begin( NULL,
                                  spRootStmt,
                                  SMI_STATEMENT_NORMAL |
                                  SMI_STATEMENT_MEMORY_CURSOR)
                  != IDE_SUCCESS );
        sStage = 3;

        if( sTempMeta.build(&sSmiStmt,
                             mSender->getRepName(),
                             ID_TRUE,
                             RP_META_BUILD_OLD,
                             SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS )
        {
            IDE_TEST(ideIsRetry() != IDE_SUCCESS);

            IDE_CLEAR();

            IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE)
                      != IDE_SUCCESS );
            sStage = 2;

            // retry.
            RP_DBG_PRINTLINE();
            continue;
        }
        break;
    }

    /* DDL 로그를 적용하기 전의 메타를 sPreviousMeta로 복사해 놓는다. */
    mMeta->copyMeta(&sPreviousMeta); 
    /* 신규로 build한 old meat를 Sender의 메타에 적용한다. */ 
    IDE_TEST(sTempMeta.copyMeta(mMeta) != IDE_SUCCESS);

    while ( mTransTbl->existItemMeta( aTID ) == ID_TRUE )
    {
        mTransTbl->getFirstItemMetaEntry( aTID, &sItemMetaEntry );

        sOldTableOID = sItemMetaEntry->mItemMeta.mOldTableOID;
        sNewTableOID = sItemMetaEntry->mItemMeta.mTableOID;

        IDE_TEST( updateMeta( &sSmiStmt,
                              &sTempMeta,
                              sItemMetaEntry,
                              sOldTableOID,
                              sNewTableOID,
                              aDDLCommitSN,
                              &sIsUpdated )
                  != IDE_SUCCESS );
        if ( sIsUpdated == ID_TRUE )
        {
            sIsMetaUpdated = ID_TRUE;
        }
        mTransTbl->removeFirstItemMetaEntry( aTID );
    }

    IDE_TEST( mSender->allocAndRebuildNewSentLogCount() != IDE_SUCCESS );

    if( sIsMetaUpdated == ID_TRUE )
    {
        /* DDL 로그로 인하여 mMeta가 변경되었다면 핸드쉐이크를 다시 해야한다. */
        sNeedRehandshake = ID_TRUE;
    }
    else
    {
        /* mMeta가 DDL로그로 변경되지 않았더라도 실시간으로 old에 내용을 바꾸는 DDL을 점검하기 위해서 
         * receiver와 핸드쉐이크를 시뮬레이션 해서 체크합니다. 
         * 이전에 sender가 receiver와 핸드쉐이크 성공한 sender의 meta(sPreviousMeta)를 receiver meta로 사용한다.
         * Sender의 새로 빌드된 meta를 sPostMeta로 복사하여 사용한다. 
         * 각각의 meta를 넣고 핸드쉐이크에서 사용하는 equals 함수를 사용하여 보수적으로 체크한다.  
         * sqlapply와 itemcountdiff는 ala,jdbcadapter를 고려하고 보수적으로 0으로 설정하여 체크한다. 
         * 핸드 쉐이크가 필요한지 가상 핸드쉐이크를 통해 비교하고 equals가 실패한다면 핸드 쉐이크를 다시 해야한다.
         */
        IDE_TEST(mMeta->copyMeta(&sPostMeta) != IDE_SUCCESS);
        sPreviousMeta.changeToRemoteMetaForSimulateHandshake();
        if( rpdMeta::equals( &sSmiStmt,
                             ID_FALSE,          /* aIsLocalReplication */
                             0,                 /* aSqlApplyEnable(UInt) */
                             0,                 /* aItemCountDiffEnable(UInt) */
                             &sPostMeta,        /* aRemoteMeta, sender */
                             &sPreviousMeta )   /* aLocalMeta, receiver */
            == IDE_SUCCESS )
        {
            sNeedRehandshake = ID_FALSE;
        }
        else
        {
            sNeedRehandshake = ID_TRUE;
        }
    }

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    sIsTxBegin = ID_FALSE;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    RP_LABEL( NORMAL_EXIT );

    sTempMeta.finalize();
    sPreviousMeta.finalize();
    sPostMeta.finalize();

    *aOutNeedHandshake = sNeedRehandshake;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OFFLINE_SENDER );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
        		"A DDL Log cannot process in offline sender" ) );
    }

    IDE_EXCEPTION_END;
    ideLog::log(IDE_RP_0,"[Sender] Failed to apply table meta log ");
    IDE_ERRLOG(IDE_RP_0);
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
            (void)sTrans.destroy( NULL );
        default:
            break;
    }
    sTempMeta.finalize();
    sPreviousMeta.finalize();
    sPostMeta.finalize();
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::insertNewTableInfo( smiStatement     * aSmiStmt, 
                                          rpdItemMetaEntry * aItemMetaEntry,
                                          smSN               aDDLCommitSN )

{
    IDE_TEST( mMeta->insertNewTableInfo( aSmiStmt, 
                                         &( aItemMetaEntry->mItemMeta ),
                                         (const void *)aItemMetaEntry->mLogBody,
                                         aDDLCommitSN,
                                         ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( mSender->allocAndRebuildNewSentLogCount() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::deleteOldTableInfo( smiStatement     * aSmiStmt, 
                                          rpdItemMetaEntry * aItemMetaEntry,
                                          smOID              aOldTableOID )
{
    idBool         sIsUpdateOldItem = ID_FALSE;

    if ( mSender->mCurrentType != RP_OFFLINE )
    {
        sIsUpdateOldItem = ID_TRUE;
    }
    else
    {
        sIsUpdateOldItem = ID_FALSE;
    }
 
   IDE_TEST( mMeta->deleteOldTableInfo( aSmiStmt,
                                         (const void *)aItemMetaEntry->mLogBody,
                                         aOldTableOID,
                                         sIsUpdateOldItem )
              != IDE_SUCCESS );

    IDE_TEST( mSender->allocAndRebuildNewSentLogCount() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::updateOldTableInfo( smiStatement     * aSmiStmt,
                                          rpdItemMetaEntry * aItemMetaEntry,
                                          smOID              aOldTableOID,
                                          smOID              aNewTableOID,
                                          smSN               aDDLCommitSN )

{
    rpdMetaItem  * sMetaItem = NULL;
    idBool         sIsUpdateOldItem = ID_FALSE;

    IDE_TEST( mMeta->searchTable( &sMetaItem, aOldTableOID ) != IDE_SUCCESS );
    
    IDU_FIT_POINT_RAISE( "rpxReplicator::updateOldTableInfo::Erratic::rpERR_ABORT_RP_META_NO_SUCH_DATA",
                         ERR_NOT_FOUND_TABLE );
    IDE_TEST_RAISE( sMetaItem == NULL,  ERR_NOT_FOUND_TABLE );

    if ( mSender->mCurrentType != RP_OFFLINE )
    {
        sIsUpdateOldItem = ID_TRUE;
    }
    else
    {
        sIsUpdateOldItem = ID_FALSE;
    }

    /* PROJ-1915 off-line sender의 경우 Meta를 갱신하지 않는다. */
    IDE_TEST( mMeta->updateOldTableInfo( aSmiStmt,
                                         sMetaItem,
                                         &( aItemMetaEntry->mItemMeta ),
                                         (const void *)aItemMetaEntry->mLogBody,
                                         aDDLCommitSN,
                                         sIsUpdateOldItem )
              != IDE_SUCCESS );

    IDE_TEST( mSender->updateSentLogCount( aNewTableOID,
                                           aOldTableOID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_META_NO_SUCH_DATA ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
static IDE_RC isAlreadyDroppedItemFromReplicationMeta( smiStatement * aSmiStmt,
                                                       SChar        * aRepName,
                                                       smOID          aTableOID,
                                                       idBool       * aOutIsDropped )
{
    idBool             sIsExist = ID_FALSE;

    IDE_TEST(rpdMeta::isExistItemInLastMeta(aSmiStmt,
                                            aRepName,
                                            aTableOID,
                                            &sIsExist) != IDE_SUCCESS);

    if ( sIsExist == ID_TRUE )
    {
        *aOutIsDropped = ID_FALSE;
    }
    else
    {
        IDE_TEST(rpdCatalog::isExistInReplItemsHistory( aSmiStmt,
                                                      aRepName,
                                                      aTableOID,
                                                      &sIsExist ) != IDE_SUCCESS);
        if ( sIsExist != ID_TRUE )
        {
            *aOutIsDropped = ID_TRUE;
        }
        else
        {
            *aOutIsDropped = ID_FALSE;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static idBool isAlreadyInsertedToReplicationMeta( rpdMeta *aMeta, smOID  aTableOID )
{
    rpdMetaItem  *     sMetaItem = NULL;
    idBool             sResult = ID_FALSE;

    IDE_ASSERT( aMeta->searchTable( &sMetaItem, aTableOID ) == IDE_SUCCESS );
    if ( sMetaItem == NULL )
    {
        sResult = ID_FALSE;
    }
    else
    {
        sResult = ID_TRUE;
    }
    return sResult;
}

static idBool isAlreadyAppliedToReplicationMeta( rpdMeta *aMeta, smOID  aTableOID, smSN aLogSN )
{
    rpdMetaItem  *     sMetaItem = NULL;
    idBool             sResult = ID_FALSE;

    IDE_ASSERT( aMeta->searchTable( &sMetaItem, aTableOID ) == IDE_SUCCESS );
    if ( sMetaItem != NULL )
    {
        if ( sMetaItem->mItem.mInvalidMaxSN < aLogSN )
        {
            sResult = ID_FALSE;
        }
        else
        {
            sResult = ID_TRUE;
        }
    }
    else
    {
        sResult = ID_TRUE; /* already deleted */
    }
    return sResult;
}

IDE_RC rpxReplicator::updateMeta( smiStatement     * aSmiStmt,
                                  rpdMeta          * aOldMeta,
                                  rpdItemMetaEntry * aItemMetaEntry,
                                  smOID              aOldTableOID,
                                  smOID              aNewTableOID,
                                  smSN               aDDLCommitSN,
                                  idBool           * aOutIsUpdated )
{
    idBool sIsAlreadyDropped = ID_FALSE;
    idBool sIsUpdated = ID_FALSE;
    switch( rpdMeta::getTableMetaType( aOldTableOID, aNewTableOID ) )
    {
        case RP_META_INSERT_ITEM:
            if ( isAlreadyInsertedToReplicationMeta(aOldMeta, aNewTableOID) != ID_TRUE )
            {
                IDE_TEST(isAlreadyDroppedItemFromReplicationMeta(aSmiStmt,
                                                                 mSender->getRepName(),
                                                                 aNewTableOID,
                                                                 &sIsAlreadyDropped) != IDE_SUCCESS);
                if ( sIsAlreadyDropped != ID_TRUE )
                {
                    IDE_TEST( insertNewTableInfo( aSmiStmt,
                                                  aItemMetaEntry,
                                                  aDDLCommitSN )
                              != IDE_SUCCESS );
                    sIsUpdated = ID_TRUE;
                }
                else
                {
                    /* already altered(truncate, alter repl drop, alter column..., oid change)
                     * replication item ddl log, skip : do nothing */
                }
            }
            else
            {
                /* already applied this ddl log, skip : do nothing */
            }
            break;
        case RP_META_DELETE_ITEM:
            if ( isAlreadyAppliedToReplicationMeta(aOldMeta, aOldTableOID, aItemMetaEntry->mLogSN) != ID_TRUE )
            {
                IDE_TEST( deleteOldTableInfo( aSmiStmt,
                                              aItemMetaEntry,
                                              aOldTableOID )
                          != IDE_SUCCESS );
                sIsUpdated = ID_TRUE;
            }
            else
            {
                /* already applied this ddl log, skip : do nothing */
            }
            break;
        case RP_META_UPDATE_ITEM:
            if ( isAlreadyAppliedToReplicationMeta(aOldMeta, aOldTableOID, aItemMetaEntry->mLogSN) != ID_TRUE )
            {
                IDE_TEST( updateOldTableInfo( aSmiStmt,
                                              aItemMetaEntry,
                                              aOldTableOID,
                                              aNewTableOID,
                                              aDDLCommitSN )
                             != IDE_SUCCESS );
                sIsUpdated = ID_TRUE;
            }
            else
            {
                /* already applied this ddl log, skip : do nothing */
            }
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }
    *aOutIsUpdated = sIsUpdated;
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
 *
 */
IDE_RC rpxReplicator::checkUsefulBySenderTypeNStatus( 
    smiLogRec             * aLog,
    idBool                * aIsOk,
    RP_ACTION_ON_ADD_XLOG   aAction,
    iduMemPool            * aSNPool,
    iduList               * aSNList,
    smTID                   aTID)
{
    smiLogType   sLogType;
    rpxSNEntry * sSNEntry = NULL;

    if ( ( aAction == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG ) ||
         ( aAction == RP_SEND_SYNC_PK_ON_ADD_XLOG ) )
    {
        IDE_ASSERT( aSNPool != NULL );
    }

    sLogType = aLog->getType();
    *aIsOk   = ID_TRUE;
     
    // Log에 대한 유효성을 검사한다.
    switch ( mSender->mCurrentType )
    {
        case RP_RECOVERY:
            if ( ( aLog->needReplRecovery() == ID_FALSE ) &&
                 ( ( aLog->needNormalReplicate() != ID_TRUE ) ||
                   ( aLog->getType() != SMI_LT_TRANS_GROUPCOMMIT ) ) )
            {
                //recovery flag가 설정되어있는지 확인
                *aIsOk = ID_FALSE;
                break;
            }

            if ( aLog->isBeginLog() == ID_TRUE )
            {
                if ( mSender->mSNMapMgr->needRecoveryTx( aLog->getRecordSN() )
                     == ID_TRUE )
                {
                    // begin log는 sn map에서 recovery해야 하는 트랜잭션인지 확인 후
                    // 처리할 것인지 결정한다.
                    // SN Map에 있는 트랜잭션만 recovery한다.
                    IDE_TEST_RAISE( mTransTbl->insertTrans( mAllocator,
                                                            aTID,
                                                            aLog->getRecordSN(),
                                                            &mChainedValuePool )
                                    != IDE_SUCCESS, ERR_TRANSTABLE_INSERT );
                }
                else
                {
                    *aIsOk = ID_FALSE;
                }
            }
            break;

        case RP_NORMAL:
        case RP_OFFLINE:
            if ( aLog->needNormalReplicate() == ID_FALSE )
            {
                if ( ( ( mSender->getRole() == RP_ROLE_PROPAGATION ) ||
                       ( mSender->getRole() == RP_ROLE_ANALYSIS_PROPAGATION ) ) &&
                     ( isReplPropagableLog( aLog ) == ID_TRUE ) )
                {
                    /*do nothing : to do analysis for propagation*/
                }
                else
                {
                    *aIsOk = ID_FALSE;
                    break;
                }
            }

            if ( aLog->isBeginLog() == ID_TRUE )
            {
                IDE_TEST_RAISE( mTransTbl->insertTrans( mAllocator,
                                                        aTID,
                                                        aLog->getRecordSN(),
                                                        &mChainedValuePool )
                                != IDE_SUCCESS, ERR_TRANSTABLE_INSERT );
            }
            break;

        case RP_PARALLEL:
        case RP_XLOGFILE_FAILBACK_SLAVE:
            if ( aLog->needNormalReplicate() == ID_FALSE )
            {
                if ( ( ( mSender->getRole() == RP_ROLE_PROPAGATION ) ||
                       ( mSender->getRole() == RP_ROLE_ANALYSIS_PROPAGATION ) ) && 
                     ( isReplPropagableLog( aLog ) == ID_TRUE ) )
                {
                    /*do nothing : to do analysis for propagation*/
                    /* consistent mode에서는 지원하지 않는다. */
                    IDE_DASSERT( mSender->mCurrentType != RP_XLOGFILE_FAILBACK_SLAVE ); 
                }
                else
                {
                    *aIsOk = ID_FALSE;
                    break;
                }
            }

            // 자신이 처리하는 Transaction의 Begin이면, Transaction Table에 등록한다.
            if ( aLog->isBeginLog() == ID_TRUE )
            {
                /* PROJ-2543 Eager Replication Performance Enhancement 
                   DDL의 경우 Eager Mode에서 지원하지 않는데 insert Trans를 하고
                   추후에 지운다. 이런 경우, Sender가 아직 DDL log를 처리하는 도중,
                   service thread가 다른 log를 처리하여 duplicate되는 경우가 생기며 이를
                   방지하기 위해 DDL의 경우 아예 insertTrans를 하지 않는다.*/
                if ( ( sLogType != SMI_LT_DDL ) ||
                     ( ( sLogType == SMI_LT_DDL ) &&
                       ( mSender->getStatus() < RP_SENDER_FAILBACK_EAGER ) ) )
                {
                    if ( aAction == RP_SEND_SYNC_PK_ON_ADD_XLOG )
                    {
                        sSNEntry = rpcManager::searchSNEntry( aSNList, aLog->getRecordSN() );

                        if ( sSNEntry != NULL )
                        {
                            rpcManager::removeSNEntry( aSNPool, sSNEntry );
                        }
                        else
                        {
                            *aIsOk = ID_FALSE;
                            break;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST_RAISE( mTransTbl->insertTrans( mAllocator,
                                                            aTID,
                                                            aLog->getRecordSN(),
                                                            &mChainedValuePool )
                                    != IDE_SUCCESS, ERR_TRANSTABLE_INSERT );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
            break;

        default:
            // Sync, Quick, Sync_only는 이 함수를 호출하지 않아야함
            *aIsOk = ID_FALSE;
            IDE_RAISE( ERR_ABNORMAL_TYPE );
            break;
    }

    // Active Transaction인지 검사한다.
    if ( mTransTbl->isATrans( aTID ) != ID_TRUE )
    {
        *aIsOk = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANSTABLE_INSERT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_TRANSACTION_TABLE_IN_INSERT ) );
    }
    IDE_EXCEPTION( ERR_ABNORMAL_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ABNORMAL_TYPE,
                                  mMeta->mReplication.mRepName,
                                  mSender->mParallelID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::checkUsefulLog( smiLogRec             * aLog,
                                      idBool                * aIsOk,
                                      rpdMetaItem          ** aMetaItem,
                                      RP_ACTION_ON_ADD_XLOG   aAction,
                                      iduMemPool            * aSNPool,
                                      iduList               * aSNList,
                                      smTID                   aTID )
{
    smiLogType          sLogType;
    smOID               sTableOID;

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG );

    RP_OPTIMIZE_TIME_BEGIN( mOpStatistics, IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG );

    if ( ( aAction == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG ) ||
         ( aAction == RP_SEND_SYNC_PK_ON_ADD_XLOG ) )
    {
        IDE_ASSERT( aSNPool != NULL );
    }

    IDU_FIT_POINT_RAISE( "rpxReplicator::checkUsefulLog::Erratic:rpERR_ABORT_INVALID_PARAM",
                         err_bad_param );
    IDE_TEST_RAISE( aLog == NULL || aIsOk == NULL , err_bad_param );

    //----------------------------------------------------------------//
    // check replication log
    //----------------------------------------------------------------//
    IDU_FIT_POINT( "rpxReplicator::checkUsefulLog::Erratic::rpERR_ABORT_RP_SENDER_CHECK_LOG" ); 
    IDE_TEST( checkUsefulBySenderTypeNStatus( aLog,
                                              aIsOk,
                                              aAction,
                                              aSNPool,
                                              aSNList,
                                              aTID )
              != IDE_SUCCESS );

    //안보는 로그임
    IDE_TEST_CONT( *aIsOk == ID_FALSE, NORMAL_EXIT );

    sLogType  = aLog->getType();
    sTableOID = aLog->getTableOID();

    IDE_TEST( getMetaItemByLogTypeAndTableOID( sLogType,
                                               sTableOID,
                                               aMetaItem )
              != IDE_SUCCESS );
    /* PROJ-1442 Replication Online 중 DDL 허용
     * DDL Transaction에서 DML Log Record를 무시한다.
     */
    switch ( sLogType )
    {
        case SMI_LT_TABLE_META :
        case SMI_LT_DDL_QUERY_STRING :
        case SMI_LT_TRANS_COMMIT :
        case SMI_LT_TRANS_GROUPCOMMIT :
        case SMI_LT_TRANS_ABORT :
        case SMI_LT_TRANS_PREABORT :
        case SMI_LT_XA_START_REQ :
        case SMI_LT_XA_PREPARE_REQ :
        case SMI_LT_XA_PREPARE :
        case SMI_LT_XA_END :
            break;

        default :
            if ( mTransTbl->isDDLTrans( aTID ) == ID_TRUE )
            {
                *aIsOk = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }
            break;
    }

    switch ( sLogType )
    {
        case SMI_LT_MEMORY_CHANGE:
        case SMI_LT_DISK_CHANGE:
            if ( isDMLLog( aLog->getChangeType() ) != ID_TRUE )
            {
                *aIsOk = ID_FALSE;
                IDE_TEST( checkAndSendImplSVP( aLog, aTID ) != IDE_SUCCESS );
                IDE_CONT( NORMAL_EXIT );
            }

            if ( *aMetaItem != NULL )
            {
                // PROJ-1602
                // 현재 읽고 있는 로그는 Restart SN 이후의 로그이므로,
                // INVALID_MAX_SN이 Restart SN 보다 작으면 계속 진행하게 됩니다.
                if ( (*aMetaItem)->mItem.mInvalidMaxSN < aLog->getRecordSN() )
                {
                    *aIsOk = ID_TRUE;
                }
                else
                {
                    *aIsOk = ID_FALSE;
                }
            }
            else // not found
            {
                /* PROJ-2397 Compressed Table Replication */
                /* 성능때문에 aMetaItem이 null인 경우 비교 */
                if ( ( aLog->getChangeType() == SMI_CHANGE_MRDB_INSERT ) )
                {
                    if ( mMeta->isMyDictTable( sTableOID ) == ID_TRUE )
                    {
                        IDE_TEST( insertDictionaryValue( aLog, aTID )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }

                *aIsOk = ID_FALSE;
            }

            /* BUG-20919
            * 분석 대상이 아닌 로그 레코드에
            * Implicit Savepoint가 설정되어 있고 Active Transaction일 때
            * (1) Begin Flag가 미설정되어 있으면 Begin Flag를 설정하고,
            * (2) Implicit Savepoint XLog 전송합니다.
            */
            if ( *aIsOk == ID_FALSE )
            {
                IDE_TEST( checkAndSendImplSVP( aLog, aTID ) != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            break;

        case SMI_LT_TABLE_META :    // Table Meta Log Record
            //-------------------------------------------------------------//
            // check replicative Table
            //-------------------------------------------------------------//
            *aIsOk = checkUsefulLogByTableMetaType( aLog );

            if ( *aMetaItem == NULL )
            { 
                /* PROJ-2397 Compressed Table Replication */
                /* 성능때문에 aMetaItem이 null인 경우 비교 */
                if ( ( aLog->getChangeType() == SMI_CHANGE_MRDB_INSERT ) )
                {
                    if ( mMeta->isMyDictTable( sTableOID ) == ID_TRUE )
                    {
                        IDE_TEST( insertDictionaryValue( aLog, aTID )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }

            /* BUG-20919
             * 분석 대상이 아닌 로그 레코드에
             * Implicit Savepoint가 설정되어 있고 Active Transaction일 때
             * (1) Begin Flag가 미설정되어 있으면 Begin Flag를 설정하고,
             * (2) Implicit Savepoint XLog 전송합니다.
             */
            if ( *aIsOk == ID_FALSE )
            {
                IDE_TEST( checkAndSendImplSVP( aLog, aTID ) != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            break;

        case SMI_LT_LOB_FOR_REPL:
            if ( isLobControlLog( aLog->getChangeType() ) != ID_TRUE )
            {
                *aIsOk = ID_FALSE;
                IDE_CONT( NORMAL_EXIT );
            }
            break;

        case SMI_LT_SAVEPOINT_SET:
        case SMI_LT_SAVEPOINT_ABORT :
            break;

        case SMI_LT_DDL :   // DDL Transaction Log Record
            mSenderInfo->setDDLTransaction(aTID);

            // BUG-23195 DDL Transaction을 BEGIN 없이 COMMIT/ABORT 하는 것을 방지한다.
            // BUG-28206 DML이 없는 Transaction은 전송하지 않는다.
            // BUG-28206 반영 후에 BUG-23195이 방지된다.
            // DDL Transaction은 DML이 없으므로, 그냥 Begin 상태로 만든다.
            mTransTbl->setBeginFlag( aTID, ID_TRUE );

            *aIsOk = ID_FALSE;
            break;

        case SMI_LT_TRANS_COMMIT :
        case SMI_LT_TRANS_GROUPCOMMIT :
        case SMI_LT_TRANS_ABORT :
        case SMI_LT_TRANS_PREABORT :
            //------------------------------------------------------//
            // check replicative log
            //------------------------------------------------------//
            if ( mTransTbl->getBeginFlag( aTID ) == ID_TRUE )
            {
                *aIsOk = ID_TRUE;
            }
            else
            {
                //------------------------------------------------------//
                // remove in transaction table
                //------------------------------------------------------//
                mTransTbl->removeTrans( aTID );

                *aIsOk = ID_FALSE;
            }
            mSender->mCommitXSN = aLog->getRecordSN();
            break;

        case SMI_LT_DDL_QUERY_STRING :
            if ( ( mTransTbl->existItemMeta( aTID ) == ID_TRUE ) &&
                 ( ( mMeta->mReplication.mOptions & RP_OPTION_DDL_REPLICATE_MASK ) 
                   == RP_OPTION_DDL_REPLICATE_SET ) )
            {
                *aIsOk = ID_TRUE;
            }
            else
            {
                *aIsOk = ID_FALSE;
            }
            break;
        
        case SMI_LT_XA_START_REQ :
        case SMI_LT_XA_PREPARE :
            if ( isReplUsingGlobalTx() == ID_TRUE )
            {
                if ( mTransTbl->getBeginFlag( aTID ) != ID_TRUE )
                {
                    mTransTbl->setBeginFlag( aTID, ID_TRUE );
                }
                mTransTbl->setGTrans( aTID );
                *aIsOk = ID_TRUE;
            }
            else
            {
                *aIsOk = ID_FALSE;
            }

            break;

        case SMI_LT_XA_PREPARE_REQ :
        case SMI_LT_XA_END :
            if ( isReplUsingGlobalTx() == ID_TRUE )
            {
                *aIsOk = ID_TRUE;
            }
            else
            {
                *aIsOk = ID_FALSE;
            }
            break;

        default:
            *aIsOk = ID_FALSE;
            break;
    }// switch

    RP_LABEL( NORMAL_EXIT );

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_bad_param );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PARAM ) );
    }
    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG );

    IDE_ERRLOG( IDE_RP_0 );

    if ( aLog != NULL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_CHECK_LOG,
                                  aLog->getRecordSN() ) );
    }

    return IDE_FAILURE;
}

idBool rpxReplicator::isReplUsingGlobalTx()
{
    idBool sIsUsing = ID_FALSE;

    if ( mMeta->getReplMode() == RP_CONSISTENT_MODE )
    {
        sIsUsing = ID_TRUE;
    }

    return sIsUsing;
}
/*
 *
 */
IDE_RC rpxReplicator::insertDictionaryValue( smiLogRec  * aLog, smTID aTID )
{
    rpdLogAnalyzer     * sLogAnlz   = NULL;
    rpdDictionaryValue * sDictValue = NULL;
    UInt                 sState     = 0;
    smOID                sRecordOID = SM_NULL_OID;
    scGRID               sGRID;

    sLogAnlz = mTransTbl->getLogAnalyzer( aTID );

    IDE_TEST( rpdLogAnalyzer::anlzInsDictionary( sLogAnlz,
                                                 aLog,
                                                 aTID,
                                                 aLog->getRecordSN() )
              != IDE_SUCCESS );
    sState = 1; 
    
    IDU_FIT_POINT_RAISE( "rpxReplicator::insertDictionaryValue::malloc::DictValue",
                          ERR_MEMORY_ALLOC_DICTVALUE,
                          rpERR_ABORT_MEMORY_ALLOC,
                          "rpxReplicator::insertDictionaryValue",
                          "sDictValue" );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                       ID_SIZEOF(rpdDictionaryValue),
                                       (void **)&sDictValue,
                                       IDU_MEM_IMMEDIATE,
                                       mAllocator )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_DICTVALUE );
    sState = 2;

    aLog->getDictGRID( &sGRID );

    sRecordOID = SM_MAKE_OID( sGRID.mPageID, sGRID.mOffset );

    sDictValue->mDictValueOID = sRecordOID;
    sDictValue->mValue.length = sLogAnlz->mACols[0].length;

    // null value
    if ( sDictValue->mValue.length == 0 )
    {
        sDictValue->mValue.value = NULL;
    }
    // non-null value
    else
    {
        IDU_FIT_POINT_RAISE( "rpxReplicator::insertDictionaryValue::malloc::DictValue_Value",
                              ERR_MEMORY_ALLOC_DICTVALUE_VALUE,
                              rpERR_ABORT_MEMORY_ALLOC,
                              "rpxReplicator::insertDictionaryValue",
                              "sDictValue->mValue.value" );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                           sDictValue->mValue.length,
                                           (void **)&sDictValue->mValue.value,
                                           IDU_MEM_IMMEDIATE,
                                           mAllocator )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_DICTVALUE_VALUE );
        sState = 3;

        idlOS::memcpy( (void*)( sDictValue->mValue.value ),
                       (const void*)( sLogAnlz->mACols[0].value ),
                       sDictValue->mValue.length );

        sLogAnlz->insertDictValue( sDictValue );
    }

    /* BUGBUG 딕셔너리 테이블 스키마가 변경되면
     * 아래 aTableColCount의 값을 변경해야함
     */
    sState = 0;
    sLogAnlz->resetVariables( ID_FALSE,
                              1 /*aTableColCount*/ ); /*A dictionary table has only one column. */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DICTVALUE )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReplicator::insertDictionaryValue",
                                  "sDictValue" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DICTVALUE_VALUE )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReplicator::insertDictionaryValue",
                                  "sDictValue->mValue.value" ) );
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            (void)iduMemMgr::free( (void *)sDictValue->mValue.value, mAllocator );
            /* fall through */ 
        case 2:
            (void)iduMemMgr::free( sDictValue, mAllocator );
            /* fall through */ 
        case 1:
            sLogAnlz->resetVariables( ID_FALSE,
                                      1 /*aTableColCount*/ ); /*A dictionary table has only one column. */
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}


IDE_RC rpxReplicator::replicateLogWithLogPtr( const SChar * aLogPtr ) 
{
    smiLogRec     sLog;
    smiLogHdr     sLogHead;
    smLSN         sDummyLSN;

    UInt          i         = 0;
    UInt          sGCCnt    = 0;
    smTID         sTID      = SM_NULL_TID;

    SM_LSN_INIT( sDummyLSN );
    
    smiLogRec::getLogHdr( (void *)aLogPtr, &sLogHead);
  
    sLog.initialize( (const void *)mMeta,
                     &mChainedValuePool,
                     RPU_REPLICATION_POOL_ELEMENT_SIZE );
    sLog.setCallbackFunction( rpdMeta::getMetaItem,
                              rpdMeta::getColumnCount,
                              rpdMeta::getSmiColumn );

    IDE_TEST( sLog.readFrom( &sLogHead, (void *)aLogPtr, &sDummyLSN )
              != IDE_SUCCESS );
 
    /*
     * 파일의 끝 log일 경우, Sender thread들을 wakeup시켜 KeepAlive를 보낸다.
     */
    if ( sLog.getType() == SMI_LT_FILE_END )
    {
        mSenderInfo->wakeupEagerSender();
        IDE_DASSERT(mSender->isParallelParent() == ID_TRUE);
        mSender->setCommitXSNForFileEndLog(sLog.getRecordSN());
    }
    else
    {
        /* Nothing to do */
    }

    idCore::acpAtomicInc64( &mReadLogCount );
  
    IDE_TEST_CONT(sLog.getTransID() == SM_NULL_TID, NORMAL_EXIT);

    IDE_TEST( checkAndWaitForLogSync( &sLog ) != IDE_SUCCESS );

    if ( sLog.getType() == SMI_LT_TRANS_GROUPCOMMIT )
    {
        sGCCnt = sLog.getGroupCommitCnt( sLog.getLogPtr() );

        for ( i = 0; i < sGCCnt; i++ )
        {
            sTID = sLog.getNthTIDFromGroupCommit( sLog.getLogPtr(), i );
            IDE_TEST( checkAndAddXLog( RP_SEND_XLOG_ON_ADD_XLOG,
                                       NULL, /* aSNPool */
                                       NULL, /* aSNList */
                                       NULL, /* RawLogPtr */
                                       &sLog,
                                       sDummyLSN,
                                       ID_FALSE, /* aIsStartedAheadAnalyzer */
                                       sTID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sTID = sLog.getTransID();
        IDE_TEST( checkAndAddXLog( RP_SEND_XLOG_ON_ADD_XLOG,
                                   NULL, /* aSNPool */
                                   NULL, /* aSNList */
                                   NULL, /* RawLogPtr */
                                   &sLog,
                                   sDummyLSN,
                                   ID_FALSE, /* aIsStartedAheadAnalyzer */
                                   sTID )
                  != IDE_SUCCESS );
    }

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}



IDE_RC rpxReplicator::checkAndAddXLog( RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                       iduMemPool            * aSNPool,
                                       iduList               * aSNList,
                                       SChar                 * aRawLogPtr,
                                       smiLogRec             * aLog,
                                       smLSN                   aReadLSN,
                                       idBool                  aIsStartedAheadAnalyzer,
                                       smTID                   aTID )
{
    idBool          sIsOK        = ID_FALSE;
    rpdMetaItem *   sMetaItem    = NULL;

    //----------------------------------------------------------------//
    // check whether if the log record is useful
    //----------------------------------------------------------------//

    IDE_TEST_RAISE( checkUsefulLog( aLog,
                                    &sIsOK,
                                    &sMetaItem,
                                    aActionAddXLog,
                                    aSNPool,
                                    aSNList,
                                    aTID )
                    != IDE_SUCCESS, ERR_CHECK_LOG_RECORD );

    if ( sIsOK != ID_TRUE )
    {
        /* Nothing to do */
    }
    else
    {
        // 너무 많은 KeepAlive 발생시키지 않도록 
        // 유의미한 로그 발생시 초기화 한다.
        mSleepForKeepAliveCount = 0;

        // Increase SEND_LOG_COUNT in V$REPSENDER
        idCore::acpAtomicInc64( &mSendLogCount );

        //----------------------------------------------------------------//
        // if useful, make xLog and add to Replication Buffer
        //----------------------------------------------------------------//

        if ( aIsStartedAheadAnalyzer == ID_FALSE )
        {
            IDE_TEST_RAISE( addXLog( aLog,
                                     sMetaItem,
                                     aActionAddXLog,
                                     aSNPool,
                                     aSNList,
                                     RP_REPLICATIED_TRNAS_GROUP_NORMAL,
                                     aTID )
                            != IDE_SUCCESS, ERR_MAKE_XLOG );
        }
        else
        {
            IDE_TEST( addXLogInGroupingMode( aLog,
                                             aRawLogPtr,
                                             aReadLSN,
                                             sMetaItem,
                                             aActionAddXLog,
                                             aSNPool,
                                             aSNList,
                                             aTID )
                     != IDE_SUCCESS );

        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_LOG_RECORD );
    {
        // error pass
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_MAKE_XLOG );
    {
        // error pass
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
 *
 */
IDE_RC rpxReplicator::replicateLogFiles( RP_ACTION_ON_NOGAP      aActionNoGap,
                                         RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                         iduMemPool            * aSNPool,
                                         iduList               * aSNList )
{
    idBool        sIsValid         = ID_FALSE;
    smiLogRec     sLog;
    smSN          sLstSN;
    smSN          sCurrentSN       = SM_SN_NULL;
    SChar       * sLogPtr;
    SChar       * sRawLogPtr;
    SChar       * sLogHeadPtr;
    smLSN         sReadLSN;
    smiLogHdr     sLogHead;
    idBool        sIsEnter         = ID_FALSE;
    idBool        sIsLeave         = ID_FALSE;
    smLSN         sBufLSN;
    smSN          sFlushSN         = SM_SN_NULL;
    idBool        sLogSwitchLocked = ID_FALSE;
    idBool        sIsFirst         = ID_TRUE;
    idBool        sIsStartedAheadAnalyzer = ID_FALSE;
    smLSN         sThroughputStartLSN;
    ULong         sProcessedXLogSize = 0;
    /*
     * PROJ-1969의 영향으로 Log File이 Switch 될때만 Log Buffer Mode로 다시 들어갈 수 있음.
     * sEndOfLog를 참조하여 Log File이 Switch 되었는지 확인 함.
     * 하지만 최소의 Log Buffer Mode로 동작하기 위해서 초기값을 ID_TRUE로 설정. */
    idBool        sEndOfLog = ID_TRUE;
    UInt          i         = 0;
    UInt          sGCCnt    = 0;
    smTID         sTID      = SM_NULL_TID;

    /* BUG-46944 CompLogHeader에 TableOID 추가 */

    SM_MAKE_LSN( sThroughputStartLSN, mSender->mXSN );
    mThroughputStartTime = idlOS::gettimeofday();

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER );

    if ( ( aActionAddXLog == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG ) ||
         ( aActionAddXLog == RP_SEND_SYNC_PK_ON_ADD_XLOG ) )
    {
        IDE_ASSERT( aSNPool != NULL );
    }

    SM_LSN_INIT( sBufLSN );

    // PROJ-1705 파라메터 추가
    sLog.initialize( (const void *)mMeta,
                     &mChainedValuePool,
                     RPU_REPLICATION_POOL_ELEMENT_SIZE );

    sLog.setCallbackFunction( rpdMeta::getMetaItem,
                              rpdMeta::getColumnCount,
                              rpdMeta::getSmiColumn );

    /* PROJ-1915 */
    if ( mSender->mCurrentType == RP_OFFLINE )
    {
        IDE_ASSERT( getRemoteLastUsedGSN( &sLstSN ) == IDE_SUCCESS );
    }
    else
    {
        /* 아카이브 로그상태이면 remote lastSN을 얻는다. */
        if ( mLogMgrStatus == RP_LOG_MGR_ARCHIVE )
        {
            IDE_ASSERT( getRemoteLastUsedGSN( &sLstSN )
                        == IDE_SUCCESS );
        }
        else
        {
            IDE_ASSERT( smiGetLastValidGSN( &sLstSN ) == IDE_SUCCESS );
        }
    }

    IDU_FIT_POINT( "rpxReplicator::replicateLogFiles::SLEEP" );

    // do while end of log file
    while ( mSender->checkInterrupt() == RP_INTR_NONE )
    {
        IDE_CLEAR();

        sLogPtr     = NULL;
        sRawLogPtr  = NULL;
        sLogHeadPtr = NULL;

        // BUG-29115
        unlockLogSwitch( &sLogSwitchLocked );

        if ( mSender->mCurrentType == RP_RECOVERY )
        {
            //recovery가 완료 되었으므로, 종료한다.
            if ( mSender->mSNMapMgr->isEmpty() == ID_TRUE )
            {
                mSender->setExitFlag();
                continue;
            }
        }

        /* PROJ-1915 : off-line replicator 를 종료 합니다. */
        if ( mSender->mCurrentType != RP_OFFLINE )
        {
            if( mSender->isArchiveALA() == ID_TRUE &&
                mLogMgrStatus == RP_LOG_MGR_ARCHIVE )
            {
                /* archive log의 끝에 도달하면 redo log로 전환한다. */
                if( mSender->mXSN >= sLstSN )
                {
                    ideLog::log( IDE_RP_0, RP_TRC_SX_REACH_END_ARCHIVE_LOG,
                                 mSender->mXSN, mCurrFileNo );
                    
                    /* FILE_END log를 고려하여 2를 증가시킨다. */
                    mSender->mXSN += 2;
                    
                    lockLogSwitch( &sLogSwitchLocked );
                    
                    /* redo log와 archive log간의 switch */
                    IDE_TEST( switchToRedoLogMgr( &sLstSN ) != IDE_SUCCESS);
                    sIsFirst = ID_TRUE;
                }
                else 
                {
                    /* Nothing to do */
                }
            }
            else 
            {
                /* Nothing to do */
            }
            
            /*
             * Waiting For New Log Record
             * 만약 return on no gap으로 들어오면 gap이 없을 때 기다리지 않고
             * 리턴되어 sLstSN와 mXSN가 같은 값을 갖게된다.
             */
            IDE_TEST( waitForNewLogRecord( &sLstSN, aActionNoGap )
                      != IDE_SUCCESS );
        }

        if ( ( sLstSN == mSender->mXSN) &&
             ( aActionNoGap == RP_RETURN_ON_NOGAP ) )
        {
            break;
        }

        if ( sFlushSN == SM_SN_NULL )
        {
            if ( mMeta->getReplMode() == RP_CONSISTENT_MODE )
            {
            /* R2HA PROJ-2742 Support data integrity after fail-back on 1:1 consistent replication 
             * 이중화 모드를 확인하여,
             * consistent mode 인 경우 checkpoint 시 제거해도 되는 로그 중 가장 큰 LSN 으로 셋팅,
             *                 아닌 경우, 기존 로직 smiGetSyncedMinFirstLogSN()  */
            
                smiGetRemoveMaxLastLogSN( &sFlushSN );
            }
            else
            {
                //proj-1608 recovery from replication, recovery support sender
                IDE_TEST( smiGetSyncedMinFirstLogSN( &sFlushSN )
                          != IDE_SUCCESS );
            }
            
            if ( sFlushSN != SM_SN_NULL )
            {
                mFlushSN = sFlushSN;
            }
        }

        // BUG-29115
        if ( mSender->isArchiveALA() == ID_TRUE )
        {
            if ( mLogMgrStatus == RP_LOG_MGR_REDO )
            {
                lockLogSwitch( &sLogSwitchLocked );

                // redo log와 archive log간의 switch
                if ( mNeedSwitchLogMgr == ID_TRUE )
                {
                    mSender->mXSN += 1;

                    IDE_TEST( switchToArchiveLogMgr( &sLstSN ) != IDE_SUCCESS );
                    mNeedSwitchLogMgr = ID_FALSE;
                    sIsFirst = ID_TRUE;
                }
            }
        }

        /*PROJ-1670 replication log buffer*/
        if ( ( mIsRPLogBufMode == ID_FALSE ) &&
             ( mRPLogBufMgr != NULL ) &&
             ( sEndOfLog == ID_TRUE ) )
        {
            sIsEnter = mRPLogBufMgr->tryEnter( mNeedSN,
                                               &mRPLogBufMinSN,
                                               &mRPLogBufMaxSN,
                                               &mRPLogBufID );
            if ( sIsEnter == ID_TRUE )
            {
                mIsRPLogBufMode = ID_TRUE;
                IDE_TEST( mLogMgr.stop() != IDE_SUCCESS );

                if ( mIsGroupingMode == ID_TRUE )
                {
                    mAheadAnalyzer->stopToAnalyze();
                    mDelayedLogQueue.dequeueALL();
                    sIsStartedAheadAnalyzer = ID_FALSE;
                }
                else
                {
                    /* do nothing */
                }
            }
        }
        if ( mIsRPLogBufMode == ID_TRUE )
        {
            /* buf 모드인 경우, 필요한 로그를 찾아야 함
             * 필요한 로그는 mXSN + 1 보다 큰 로그 중 가장 작은 SN에
             * 해당하는 로그임*/
            do
            {
                RP_OPTIMIZE_TIME_BEGIN( mOpStatistics,
                                        IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER);
                IDE_TEST( mRPLogBufMgr->readFromRPLogBuf( mRPLogBufID,
                                                          &sLogHead,
                                                          (void **)&sLogPtr,
                                                          &sCurrentSN,
                                                          &sBufLSN,
                                                          &sIsLeave )
                          != IDE_SUCCESS );
                RP_OPTIMIZE_TIME_END( mOpStatistics,
                                      IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER );

                if ( ( sCurrentSN >= mNeedSN ) ||
                     ( sIsLeave == ID_TRUE ) )
                {
                    break;
                }
            }
            while ( mSender->checkInterrupt() == RP_INTR_NONE );

            // BUG-23967
            // Sender Stop 에 의 해 mExitFilag가 세팅 되어 do {} 루프를 이탈 할때
            // 보통 mExitFlg는 본 함수에 맨 최상위에서 이탈 하게 되는데 느리게 동작 할 경우 위 do { } 진행 중에
            // 세팅 되어 이탈 하는 경우 가 있다. 이럴 경우 RPLogBuf 안에서 sCurrentSN이 현재 필요 한 mNeedSN보다
            // 작은 상태 에서 아래로 진행 하게 될경우 stop 메세지에 이전 SN을 싫어 RECEIVER에 전송 하여 다음 시작에 문제가 된다.
            // mNeedSN은 rpxSender::run에서 mXSN으로 세팅 되었으므로 아래 종료 상황에서 sCurrentSN으로 세팅 되어
            // mXSN이 되어야 한다.
            // 하여 mXSN = mNeedSN이 되어야 한다. 아래에 skip 상황만 제외 하고 모두 mXSN = mNeedSN으로 수정
            if ( ( mSender->isExit() == ID_TRUE ) &&
                 ( sCurrentSN < mNeedSN ) )
            {
                sCurrentSN = mNeedSN;
            }

            if ( sIsLeave != ID_TRUE )
            {
                if ( sLogPtr == NULL ) // wait for rp log write, skip
                {
                    /*sCurrentSN이 SM_SN_NULL인 경우 다시 시도해야함*/
                    if ( sCurrentSN != SM_SN_NULL )
                    {
                        mSender->mXSN = sCurrentSN;
                        mNeedSN = sCurrentSN + 1;
                    }

                    continue;
                }
                else
                {
                    /*BUG-27629 rp log buffer를 통해서 읽은 로그의 LSN은 알수 없으므로,
                     *rp log buffer를 통해서 읽으면, sReadLSN이 초기화 되지 않은 상태로 사용될 수 있다.
                     *그러므로, 0으로 초기화 함*/
                    SM_LSN_INIT( sReadLSN );
                    sLogHeadPtr = (SChar*)&sLogHead;
                }

                /* 
                 * If log buffer uess, LSN is not valid 
                 * so Current file no can not be gotten
                 */
                mCurrFileNo = ID_UINT_MAX;
            }
            else  /*sIsLeave == ID_TRUE*/
            {
                leaveLogBuffer();

                /*mLogMgr초기화 하기 */
                IDE_TEST( mLogMgr.startByLSN( sBufLSN ) != IDE_SUCCESS );

                /* PROJ-1969 Replicated Transaction Group */
                if ( mIsGroupingMode == ID_TRUE )
                {
                    IDE_TEST( mAheadAnalyzer->startToAnalyze( sBufLSN,
                                                              &sIsStartedAheadAnalyzer ) 
                              != IDE_SUCCESS ); 
                }
                else
                {
                    /* Nothing to do */
                }

                // BUG-29115
                // 현재 읽고 있는 file no
                mCurrFileNo = sBufLSN.mFileNo;

                continue;
            }
        }
        else
        {
            IDE_TEST( readMyLog( &sCurrentSN,
                                 &sReadLSN,
                                 (SChar*)&sLogHead,
                                 &sLogPtr,
                                 &sRawLogPtr,
                                 &sIsValid )
                      != IDE_SUCCESS );
            if ( sLogPtr != NULL )
            {
                sLogHeadPtr = (SChar*)&sLogHead;
            }
            else
            {
                if ( sIsValid == ID_FALSE )
                {
                    continue;
                }
                else
                {
                    IDE_CONT( NEXT_LOG );
                }
            }
        }

        // BUG-29115
        // debug용으로 현재 읽고있는 file no를 기록한다.
        if ( ( sIsFirst == ID_TRUE ) && ( mSender->isArchiveALA() == ID_TRUE ) )
        {
            if ( mLogMgrStatus == RP_LOG_MGR_REDO )
            {
                ideLog::log( IDE_RP_0, RP_TRC_SX_REPLICATE_REDO_LOG_NO,
                             mSender->mXSN, mCurrFileNo );
            }
            else
            {
                ideLog::log( IDE_RP_0, RP_TRC_SX_REPLICATE_ARCHIVE_LOG_NO,
                             mSender->mXSN, mCurrFileNo );
            }

            sIsFirst = ID_FALSE;
        }

        // sLog를 다시 쓰기 때문에 새로운 로그를 읽어들일때마다 초기화를 수행해야한다.
        sLog.reset();
        //----------------------------------------------------------------//
        //  현재 XLSN가 위치한 로그가 정확한 것인지 검사.
        //  Head와 Tail이 동일한 Type을 가져야 함.
        //  로그 화일의 마지막인지도 검사.
        //  Check End Of Log : If so, Loop-Out.
        //----------------------------------------------------------------//
        IDE_TEST( sLog.readFrom( sLogHeadPtr, sLogPtr, &sReadLSN )
                  != IDE_SUCCESS );

        IDE_TEST( checkEndOfLogFile( &sLog, &sEndOfLog )
                  != IDE_SUCCESS );

        if ( sEndOfLog == ID_TRUE ) 
        {
            sProcessedXLogSize = RP_GET_BYTE_GAP( sReadLSN, sThroughputStartLSN );
            calculateThroughput( sProcessedXLogSize );
            SM_MAKE_LSN( sThroughputStartLSN, mSender->mXSN );

            if ( mIsGroupingMode == ID_TRUE ) 
            {
                IDE_TEST( processEndOfLogFileInGrouping( aActionAddXLog,
                                                         aSNPool,
                                                         aSNList,
                                                         sLog.getRecordSN(),
                                                         sReadLSN.mFileNo,
                                                         &sIsStartedAheadAnalyzer )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
            IDE_CONT( NEXT_LOG );
        }
        else
        {
            /* do nothing */
        }

        //----------------------------------------------------------------//
        // Guarantee Synced mXLSN
        //----------------------------------------------------------------//
        // BUG-15753
        IDE_TEST( checkAndWaitForLogSync( &sLog ) != IDE_SUCCESS );

        // Increase READ_LOG_COUNT in V$REPSENDER
        idCore::acpAtomicInc64( &mReadLogCount );

        if ( sLog.getType() == SMI_LT_TRANS_GROUPCOMMIT )
        {
            sGCCnt = sLog.getGroupCommitCnt( sLog.getLogPtr() );

            for ( i = 0; i < sGCCnt; i++ )
            {
                sTID = sLog.getNthTIDFromGroupCommit( sLog.getLogPtr(), i );

                if ( isMyLog( sTID, sCurrentSN ) == ID_TRUE )
                {
                    IDE_TEST_RAISE ( checkAndAddXLog( aActionAddXLog,
                                                      aSNPool,
                                                      aSNList,
                                                      sRawLogPtr,
                                                      &sLog,
                                                      sReadLSN,
                                                      sIsStartedAheadAnalyzer,
                                                      sTID )
                                     != IDE_SUCCESS, ERR_CHECK_AND_MAKE_XLOG );
                }
                else
                {
                    /* do nothing */
                }
            }
        }
        else
        {
            sTID = sLog.getTransID();

            if ( isMyLog( sTID, sCurrentSN ) == ID_TRUE )
            {
                IDE_TEST_RAISE ( checkAndAddXLog( aActionAddXLog,
                                                  aSNPool,
                                                  aSNList,
                                                  sRawLogPtr,
                                                  &sLog,
                                                  sReadLSN,
                                                  sIsStartedAheadAnalyzer,
                                                  sTID )
                                 != IDE_SUCCESS, ERR_CHECK_AND_MAKE_XLOG );
            }
            else
            {
                /* do nothing */
            }
        }
        RP_LABEL( NEXT_LOG );

        mSender->mXSN = sCurrentSN;
        mNeedSN = sCurrentSN + 1;

        /*
         * PROJ-2453 Eager Replication Performance Enhancement
         * Sender 의 Active Transaction 이 존재 하지 않으면
         * 더이상 Sender Thread 에서 직접 보낼 로그들이 존재하지 않는다.
         * 그러므로 replicateLogFiles 는 종료 하고 IDLE 상태로 변경한다.
         */
        if ( ( mTransTbl->getATransCnt() == 0 ) &&
             ( ( mSender->mStatus == RP_SENDER_FLUSH_FAILBACK ) ||
               ( mSender->mStatus == RP_SENDER_CONSISTENT_FAILBACK ) ) &&
             ( mSender->getFailbackEndSN() < sCurrentSN ) )
        {
            IDE_TEST( mSender->addXLogKeepAlive() != IDE_SUCCESS );

            if ( mMeta->getReplMode() == RP_EAGER_MODE )
            {
                mSender->setStatus( RP_SENDER_IDLE );
            }
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    // BUG-29115
    unlockLogSwitch( &sLogSwitchLocked );

    if ( mIsGroupingMode == ID_TRUE )
    {
        mAheadAnalyzer->stopToAnalyze();
        mDelayedLogQueue.dequeueALL();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_AND_MAKE_XLOG );
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER );

    unlockLogSwitch( &sLogSwitchLocked );

    IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_MAKE_XLOG_IN_LOGFILE,
                              sCurrentSN ) );
    IDE_ERRLOG( IDE_RP_0 );

    /*
     *  sLogHeadPtr 할당 전에  IDE_TEST 존재
     */
    if ( sLogHeadPtr != NULL )
    {
        smiLogRec::dumpLogHead((smiLogHdr*)sLogHeadPtr, IDE_RP_0);
    }
    else
    {
        /* do nothing */
    }

    if ( mIsRPLogBufMode == ID_TRUE )
    {
        ideLog::log( IDE_RP_0, "log header" ); 
        ideLog::logMem( IDE_RP_0, (UChar*)(&sLogHead), ID_SIZEOF(smiLogHdr) );

        if ( sLogPtr != NULL )
        {
            ideLog::log( IDE_RP_0, "log pointer" ); 
            ideLog::logMem( IDE_RP_0, (UChar*)(sLogPtr), ID_SIZEOF(smiLogHdr) );
        }
        else
        {
            /* so nothing */
        }
    }
    else
    {
        /*nothing to do*/
    }

    if ( mIsGroupingMode == ID_TRUE )
    {
        mAheadAnalyzer->stopToAnalyze();
        mDelayedLogQueue.dequeueALL();
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReplicator::resetReadLogCount( void )
{
    idCore::acpAtomicSet64( &mReadLogCount, 0 );
}

/*
 *
 */
ULong rpxReplicator::getReadLogCount( void )
{
    return idCore::acpAtomicGet64( &mReadLogCount );
}

/*
 *
 */
void rpxReplicator::resetSendLogCount( void )
{
    idCore::acpAtomicSet64( &mSendLogCount, 0 );
}

/*
 *
 */
ULong rpxReplicator::getSendLogCount( void )
{
    return idCore::acpAtomicGet64( &mSendLogCount );
}

/*
 *
 */
IDE_RC rpxReplicator::initTransTable( void )
{
    rpdTransTbl  * sTransTbl    = NULL;
    idBool         sIsInit      = ID_FALSE;
    
    if ( mTransTbl == NULL )
    {
        mThroughput = 0;
    
        IDU_FIT_POINT_RAISE( "rpxReplicator::initTransTable::malloc::TransTbl",
                              ERR_MEMORY_ALLOC_TRANS_TABLE );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SENDER,
                                           ID_SIZEOF( rpdTransTbl ),
                                           (void**)&sTransTbl,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_TRANS_TABLE );
        new (sTransTbl) rpdTransTbl;
    
        IDE_TEST_RAISE( sTransTbl->initialize( RPD_TRANSTBL_USING_LOG_ANALYZER,
                                               0 /*sender does not use transaction pool*/ )
                        != IDE_SUCCESS, err_hash_init );
        sIsInit = ID_TRUE;

        IDU_FIT_POINT_RAISE( "1.TASK-2004@rpxReplicator::initTransTable", 
                              err_hash_init );
 
        mTransTbl = sTransTbl;
    }
    else 
    {
        mTransTbl->rollbackAllATrans();
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_TRANS_TABLE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxSender::initTransTable",
                                  "mTransTbl" ) );
    }
    IDE_EXCEPTION( err_hash_init );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_SENDER_HASH_INIT ) );
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if ( sTransTbl != NULL )
    {
        if ( sIsInit == ID_TRUE )
        {
            sIsInit = ID_FALSE;
            sTransTbl->destroy();
        }
        else
        {
           /* Nothing to do */
        }
        
        (void)iduMemMgr::free( sTransTbl );
        sTransTbl = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReplicator::rollbackAllTrans( void )
{
    if ( mTransTbl != NULL )
    {
        mTransTbl->rollbackAllATrans();
    }
}

/*
 *
 */
rpdTransTbl * rpxReplicator::getTransTbl( void )
{
    return mTransTbl;
}

idBool rpxReplicator::isExistActiveTrans()
{
    if ( mTransTbl != NULL )
    {
        return mTransTbl->isExistActiveTrans( );
    }
    else
    {
        return ID_FALSE;
    }
}
/*
 *
 */
void rpxReplicator::getMinTransFirstSN( smSN * aSN )
{
    if ( mTransTbl != NULL )
    {
        mTransTbl->getMinTransFirstSN( aSN );
    }
}

/*
 *
 */
IDE_RC rpxReplicator::getRemoteLastUsedGSN( smSN * aSN )
{
    IDE_TEST( mLogMgr.getRemoteLastUsedGSN( aSN ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReplicator::getAllLFGReadLSN( smLSN  * aArrReadLSN )
{ 
    mLogMgr.getReadLSN( aArrReadLSN );
}

/*
 *
 */
IDE_RC rpxReplicator::switchToRedoLogMgr( smSN * aSN )
{
    if ( mLogMgrInit == ID_TRUE )
    {
        mLogMgrInit = ID_FALSE;
        IDE_TEST( mLogMgr.destroy() != IDE_SUCCESS );
    }

    ideLog::log( IDE_RP_0, RP_TRC_S_INIT_REDO_LOG_MGR );

    // redo log로 전환한다.
    mLogMgrStatus = RP_LOG_MGR_REDO;
    if ( mLogMgr.initialize( mSender->mXSN,
                             0,         // BUG-29926 Archive ALA는 PreRead Thread 미사용
                             ID_FALSE,  // aIsRemoteLog 
                             0,         // aLogFileSize
                             NULL,      // aLogDirPath
                             ID_TRUE ) // 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부
         != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );

        ideLog::log( IDE_RP_0, RP_TRC_S_ERR_INIT_REDO_LOG_MGR );
        ideLog::log( IDE_RP_0, RP_TRC_S_INIT_ARCHIVE_LOG_MGR );

        // redo log에 없는 경우 다시 archive log로 전환한다.
        mLogMgrStatus = RP_LOG_MGR_ARCHIVE;
        IDE_TEST( mLogMgr.initialize( mSender->mXSN,
                                      0,
                                      ID_TRUE, //리모트 로그
                                      smiGetLogFileSize(),
                                      mFirstArchiveLogDirPath,
                                      ID_TRUE ) // 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부
                  != IDE_SUCCESS );

        IDE_ASSERT( getRemoteLastUsedGSN( aSN ) == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( smiGetLastValidGSN( aSN ) == IDE_SUCCESS );
    }

    mLogMgrInit = ID_TRUE;

    ideLog::log( IDE_RP_0, RP_TRC_S_INIT_OK_LOG_MGR, *aSN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::initializeLogMgr( smSN     aInitSN,
                                        UInt     aPreOpenFileCnt,
                                        idBool   aIsRemoteLog,
                                        ULong    aLogFileSize,
                                        UInt     /*aLFGCount*/, //[TASK-6757]LFG,SN 제거
                                        SChar ** aLogDirPath )
{
    if ( mLogMgrInit == ID_FALSE )
    {
        IDU_FIT_POINT( "rpxReplicator::initializeLogMgr::smrOpenLog::mLogMgr",
                        rpERR_ABORT_OPEN_LOG_FILE,
                        "rpxReplicator::initializeLogMgr",
                        "mLogMgr" );
        IDE_TEST( mLogMgr.initialize( aInitSN,
                                      aPreOpenFileCnt,
                                      aIsRemoteLog,
                                      aLogFileSize,
                                      aLogDirPath,
                                      ID_TRUE ) /* 압축된 로그파일의 경우 압축을 풀지않고 반환할지 여부 (압축된 형태로 반환) */
                  != IDE_SUCCESS );
        
        mLogMgrInit = ID_TRUE;

        if ( mIsGroupingMode == ID_TRUE ) 
        {
            IDE_TEST( startAheadAnalyzer( aInitSN ) != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
        mThroughput = 0;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReplicator::destroyLogMgr( void )
{
    if ( mLogMgrInit == ID_TRUE )
    {
        mLogMgrInit = ID_FALSE;
        IDE_TEST( mLogMgr.destroy() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
idBool rpxReplicator::isLogMgrInit( void )
{
    return mLogMgrInit;
}

/*
 *
 */
RP_LOG_MGR_INIT_STATUS rpxReplicator::getLogMgrInitStatus( void )
{
    return mLogMgrInitStatus;
}

/*
 *
 */
void rpxReplicator::setLogMgrInitStatus( RP_LOG_MGR_INIT_STATUS aLogMgrInitStatus ) 
{
    mLogMgrInitStatus = aLogMgrInitStatus;
}

/*
 *
 */
void rpxReplicator::checkAndSetSwitchToArchiveLogMgr(
    const UInt  * aLastArchiveFileNo,
    idBool      * aSetLogMgrSwitch )
{
    idBool sSetLogMgrSwitch = ID_TRUE;

    IDE_ASSERT( mLogSwitchMtx.lock( NULL ) == IDE_SUCCESS );

    if ( ( mLogMgrStatus == RP_LOG_MGR_REDO ) &&
         ( mNeedSwitchLogMgr == ID_FALSE ) )
    {
        // 현재 읽고 있는 fileNo가 archive되지 않았다면 log를 switch하지 않고
        // 현재 읽고 있는 redo log를 삭제하지 못하도록 한다.
        if ( ( mCurrFileNo > aLastArchiveFileNo[0] ) &&
             ( mCurrFileNo != ID_UINT_MAX ) )
        {
            ideLog::log( IDE_RP_0, RP_TRC_S_ERR_SET_SWITCH_ARCHIVE_LOG,
                         mSender->mXSN, mCurrFileNo );

            sSetLogMgrSwitch = ID_FALSE;
        }
        else
        {
            ideLog::log( IDE_RP_0, RP_TRC_S_SET_SWITCH_ARCHIVE_LOG,
                         mSender->mXSN, mCurrFileNo );

            mNeedSwitchLogMgr = ID_TRUE;
        }
    }

    IDE_ASSERT( mLogSwitchMtx.unlock() == IDE_SUCCESS );

    *aSetLogMgrSwitch = sSetLogMgrSwitch;
}

/*
 *
 */
void rpxReplicator::setNeedSN( smSN aNeedSN )
{
    mNeedSN = aNeedSN;
}

/*
 *
 */
void rpxReplicator::leaveLogBuffer( void )
{
    if ( ( mIsRPLogBufMode == ID_TRUE ) && ( mRPLogBufMgr != NULL ) )
    {
        mRPLogBufMgr->leave( mRPLogBufID );
        mRPLogBufMinSN    = 0;
        mRPLogBufMaxSN    = SM_SN_MAX;
        mRPLogBufID       = RP_BUF_NULL_ID;
        mIsRPLogBufMode   = ID_FALSE;
    }
}

/*
 *
 */
smSN rpxReplicator::getFlushSN( void )
{
    return mFlushSN;
}

void rpxReplicator::calculateThroughput( ULong aProcessedXLogSize )
{
    PDL_Time_Value      sEndTime;
    PDL_Time_Value      sTv;

    ULong               sSeconds = 0;
    ULong               sMicroSeconds = 0;

    ULong                sThroughput = 0;

    sEndTime = idlOS::gettimeofday();

    sTv = sEndTime - mThroughputStartTime;

    sSeconds = sTv.sec();

    if ( sSeconds == 0 ) 
    {
        sMicroSeconds = sTv.usec();

        if ( sMicroSeconds != 0 )
        {
            sThroughput = ( aProcessedXLogSize * 1000000 ) / sMicroSeconds;
        }
        else
        {
            sThroughput = aProcessedXLogSize * 1000000;
        }
    }
    else
    {
        sThroughput = aProcessedXLogSize / sSeconds;
    }

    mThroughputStartTime = sEndTime;

    mThroughput = sThroughput;

    if ( mSenderInfo != NULL )
    {
        mSenderInfo->setThroughput( sThroughput );
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxReplicator::buildRecordForReplicatedTransGroupInfo( SChar               * aRepName,
                                                              void                * aHeader,
                                                              void                * aDumpObj,
                                                              iduFixedTableMemory * aMemory )
{
    if ( mAheadAnalyzer != NULL )
    {
        IDE_TEST( mAheadAnalyzer->buildRecordForReplicatedTransGroupInfo( aRepName,
                                                                          aHeader,
                                                                          aDumpObj,
                                                                          aMemory )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                             void                * aHeader,
                                                             void                * aDumpObj,
                                                             iduFixedTableMemory * aMemory )
{
    if ( mAheadAnalyzer != NULL )
    {
        IDE_TEST( mAheadAnalyzer->buildRecordForReplicatedTransSlotInfo( aRepName,
                                                                         aHeader,
                                                                         aDumpObj,
                                                                         aMemory )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::buildRecordForAheadAnalyzerInfo( SChar               * aRepName,
                                                       void                * aHeader,
                                                       void                * aDumpObj,
                                                       iduFixedTableMemory * aMemory )
{
    if ( mAheadAnalyzer != NULL )
    {
        IDE_TEST( mAheadAnalyzer->buildRecordForAheadAnalyzerInfo( aRepName,
                                                                   aHeader,
                                                                   aDumpObj,
                                                                   aMemory )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::startAheadAnalyzer( smSN  aInitSN )
{
    UInt                  sStage = 0;

    if ( mAheadAnalyzer == NULL )
    {
        IDU_FIT_POINT_RAISE( "rpxAheadAnalyzer::startAheadAnalyzer::malloc::mAheadAnalyzer",
                             ERR_MEMORY_ALLOC );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                           ID_SIZEOF( rpxAheadAnalyzer ),
                                           (void**)&mAheadAnalyzer,
                                           IDU_MEM_IMMEDIATE,
                                           mAllocator )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );
        sStage = 1;

        new (mAheadAnalyzer) rpxAheadAnalyzer;

        IDE_TEST( mAheadAnalyzer->initialize( mSender ) != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST( mDelayedLogQueue.initialize( mSender->getRepName() )
                  != IDE_SUCCESS );
        sStage = 3;

        mAheadAnalyzer->setInitSN( aInitSN );

        IDU_FIT_POINT( "rpxAheadAnalyzer::startAheadAnalyzer::start::mAheadAnalyzer",
                       rpERR_ABORT_RP_INTERNAL_ARG,
                       "mAheadAnalyzer->start" );
        IDE_TEST( mAheadAnalyzer->start() != IDE_SUCCESS );
        sStage = 4;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "startAheadAnalyzer",
                                  "mAheadAnalyzer" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 4:
            mAheadAnalyzer->shutdown();
            (void)mAheadAnalyzer->join();
        case 3:
            mDelayedLogQueue.finalize();
        case 2:
            mAheadAnalyzer->finalize();
        case 1:
            (void)iduMemMgr::free( mAheadAnalyzer );
            mAheadAnalyzer = NULL;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

SInt rpxReplicator::getCurrentFileNo( void )
{
    return mCurrFileNo;
}

RP_REPLICATIED_TRNAS_GROUP_OP rpxReplicator::getReplicatedTransGroupOperation( smiLogRec    * aLog,
                                                                               smTID          aTID )
{
    smiLogType      sLogType;
    smSN            sSN = SM_SN_NULL;
    RP_REPLICATIED_TRNAS_GROUP_OP sOperation = RP_REPLICATIED_TRNAS_GROUP_NONE;

    idBool          sIsFirstGroup = ID_FALSE;
    idBool          sIsLastLog = ID_FALSE;
    rpdReplicatedTransGroupOperation sGroupOperation = RPD_REPLICATED_TRANS_GROUP_NONE;

    sLogType = aLog->getType();
    sSN = aLog->getRecordSN();

    mAheadAnalyzer->getReplicatedTransGroupInfo( aTID,
                                                 sSN,
                                                 &sIsFirstGroup,
                                                 &sIsLastLog,
                                                 &sGroupOperation );
    if ( sIsFirstGroup == ID_TRUE )
    {
        switch ( sGroupOperation )
        {
            case RPD_REPLICATED_TRANS_GROUP_DONT_SEND:
                /*
                 * DONT SEND
                 *      ABORT
                 */
                sOperation = RP_REPLICATIED_TRNAS_GROUP_DONT_SEND;
                break;

            case RPD_REPLICATED_TRANS_GROUP_SEND:

                switch ( sLogType )
                {
                    case SMI_LT_MEMORY_CHANGE:
                    case SMI_LT_DISK_CHANGE:
                    case SMI_LT_LOB_FOR_REPL:
                    case SMI_LT_TABLE_META:
                    case SMI_LT_SAVEPOINT_SET:
                    case SMI_LT_SAVEPOINT_ABORT:
                        sOperation = RP_REPLICATIED_TRNAS_GROUP_SEND;
                        break;

                    case SMI_LT_TRANS_COMMIT:
                    case SMI_LT_TRANS_GROUPCOMMIT:

                        if ( sIsLastLog == ID_TRUE )
                        {
                            sOperation = RP_REPLICATIED_TRNAS_GROUP_SEND;
                        }
                        else
                        {
                            sOperation = RP_REPLICATIED_TRNAS_GROUP_DONT_SEND;
                        }
                        break;

                    default:
                        IDE_DASSERT( 0 );
                        break;

                }
                break;

            default:
                IDE_DASSERT( 0 );
                break;
        }
    }
    else
    {
        switch ( sLogType )
        {
            case SMI_LT_TRANS_COMMIT:
            case SMI_LT_TRANS_GROUPCOMMIT:
            case SMI_LT_TRANS_PREABORT:
                IDE_DASSERT( mTransTbl->isATrans( aTID ) == ID_TRUE );

                if ( mTransTbl->getSendBeginFlag( aTID ) == ID_FALSE )
                {
                    sOperation = RP_REPLICATIED_TRNAS_GROUP_DONT_SEND;
                }
                else
                {
                    sOperation = RP_REPLICATIED_TRNAS_GROUP_NORMAL;
                }
                break;

            default:
                sOperation = RP_REPLICATIED_TRNAS_GROUP_KEEP;
                break;

        }
    }

    return sOperation;
}

IDE_RC rpxReplicator::addXLogInGroupingMode( smiLogRec             * aLog,
                                             SChar                 * aRawLogPtr,
                                             smLSN                   aCurrentLSN,
                                             rpdMetaItem           * aMetaItem,
                                             RP_ACTION_ON_ADD_XLOG   aAction,
                                             iduMemPool            * aSNPool,
                                             iduList               * aSNList,
                                             smTID                   aTID )
{
    smLSN sReadLSN;
    RP_REPLICATIED_TRNAS_GROUP_OP   sOperation;

    if ( ( aCurrentLSN.mFileNo < mAheadAnalyzer->getStartFileNo() ) ||
         ( mAheadAnalyzer->isThereGroupNode() != ID_TRUE ) )
    {
        sOperation = RP_REPLICATIED_TRNAS_GROUP_NORMAL;
    }
    else
    {
        sOperation = getReplicatedTransGroupOperation( aLog, aTID );
    }

    switch ( sOperation )
    {
        case RP_REPLICATIED_TRNAS_GROUP_KEEP:
            IDE_DASSERT( mTransTbl->isATrans( aTID ) == ID_TRUE );

            mLogMgr.getReadLSN( &sReadLSN );
            IDE_TEST ( mDelayedLogQueue.enqueue( aRawLogPtr, &sReadLSN ) != IDE_SUCCESS );
            break;

        case RP_REPLICATIED_TRNAS_GROUP_NORMAL:
            IDE_TEST_RAISE( addXLog( aLog,
                                     aMetaItem,
                                     aAction,
                                     aSNPool,
                                     aSNList,
                                     sOperation,
                                     aTID )
                            != IDE_SUCCESS, ERR_MAKE_XLOG );
            break;

        case RP_REPLICATIED_TRNAS_GROUP_SEND:
        case RP_REPLICATIED_TRNAS_GROUP_DONT_SEND:
            IDE_TEST_RAISE( addXLog( aLog,
                                     aMetaItem,
                                     aAction,
                                     aSNPool,
                                     aSNList,
                                     sOperation,
                                     aTID )
                            != IDE_SUCCESS, ERR_MAKE_XLOG );

            if ( mAheadAnalyzer->isLastTransactionInFirstGroup( aTID,
                                                                aLog->getRecordSN() )
                 == ID_TRUE )
            {
                mAheadAnalyzer->checkAndRemoveCompleteReplicatedTransGroup( aLog->getRecordSN() );

                IDE_TEST( dequeueAndSend( aAction,
                                          aSNPool,
                                          aSNList )
                          != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
            break;

        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MAKE_XLOG );
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::dequeueAndSend( RP_ACTION_ON_ADD_XLOG   aAction,
                                      iduMemPool            * aSNPool,
                                      iduList               * aSNList )
{
    smiLogHdr     sLogHead;
    SChar       * sLogPtr     = NULL;
    SChar       * sLogTempPtr = NULL;
    smLSN         sDummyLSN;
    idBool        sIsEmpty = ID_TRUE;

    smiLogRec     sLog;
    smiLogType    sLogType = SMI_LT_NULL;
    ULong         sLogTableOID = 0;
    smTID         sLogTransID = SM_NULL_TID;

    rpdMetaItem * sMetaItem = NULL;
    smLSN         sReadLSN;
    
    RP_REPLICATIED_TRNAS_GROUP_OP   sOperation = RP_REPLICATIED_TRNAS_GROUP_NONE;

    SM_LSN_INIT( sDummyLSN );

    sLog.initialize( (const void *)mMeta,
                      &mChainedValuePool,
                      RPU_REPLICATION_POOL_ELEMENT_SIZE );

    sLog.setCallbackFunction( rpdMeta::getMetaItem,
                              rpdMeta::getColumnCount,
                              rpdMeta::getSmiColumn );

    while ( 1 )
    {
        sLogTempPtr = NULL;
        IDE_TEST( mDelayedLogQueue.dequeue( (void**)&sLogTempPtr,
                                            &sReadLSN,
                                            &sIsEmpty )
                  != IDE_SUCCESS );

        if ( sIsEmpty == ID_FALSE )
        {
            sLog.reset();

            if ( mLogMgr.isCompressedLog( sLogTempPtr ) == ID_TRUE )
            {
                sLogPtr = NULL;

                IDE_TEST( mLogMgr.decompressLog( sLogTempPtr,
                                                 &sReadLSN,
                                                 (smiLogHdr*)&sLogHead,
                                                 &sLogPtr )
                          != IDE_SUCCESS );
            }
            else
            {                
                sLogPtr = sLogTempPtr;
                smiLogRec::getLogHdr( sLogPtr, &sLogHead );
            }

            IDE_TEST( sLog.readFrom( &sLogHead,
                                     sLogPtr,
                                     &sDummyLSN )
                      != IDE_SUCCESS );

            sLogType = sLog.getType();
            sLogTableOID = sLog.getTableOID();
            sLogTransID = sLog.getTransID();

            sOperation = getReplicatedTransGroupOperation( &sLog, sLogTransID );

            /* TODO comment */
            if ( sOperation == RP_REPLICATIED_TRNAS_GROUP_KEEP )
            {
                sOperation = RP_REPLICATIED_TRNAS_GROUP_SEND;
            }
            else
            {
                /* do nothing */
            }

            IDE_DASSERT( sLogType != SMI_LT_TRANS_COMMIT );
            IDE_DASSERT( sLogType != SMI_LT_TRANS_GROUPCOMMIT );
            IDE_DASSERT( sLogType != SMI_LT_TRANS_PREABORT );

             IDE_TEST( getMetaItemByLogTypeAndTableOID( sLogType,
                                                        sLogTableOID,
                                                        &sMetaItem )
                       != IDE_SUCCESS );

             IDE_TEST_RAISE( addXLog( &sLog,
                                      sMetaItem,
                                      aAction,
                                      aSNPool,
                                      aSNList,
                                      sOperation,
                                      sLogTransID )
                             != IDE_SUCCESS, ERR_MAKE_XLOG );
        }
        else
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MAKE_XLOG )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::getMetaItemByLogTypeAndTableOID( smiLogType       aLogType,
                                                       ULong            aTableOID,
                                                       rpdMetaItem   ** aMetaItem )
{
    switch ( aLogType )
    {
        case SMI_LT_MEMORY_CHANGE:
        case SMI_LT_DISK_CHANGE:
        case SMI_LT_TABLE_META:
            IDE_TEST( mMeta->searchTable( aMetaItem,
                                          aTableOID )
                      != IDE_SUCCESS );
            break;

        default:
            *aMetaItem = NULL;
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
                 
IDE_RC rpxReplicator::processEndOfLogFileInGrouping( RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                                     iduMemPool            * aSNPool,
                                                     iduList               * aSNList,
                                                     smSN                    aSN,
                                                     UInt                    aFileNo,
                                                     idBool                * aIsStartedAheadAnalyzer )
{
    idBool      sIsStarted = ID_FALSE;
    idBool      sIsStoped = ID_FALSE;

    if ( ( *aIsStartedAheadAnalyzer == ID_TRUE ) &&
         ( aFileNo >= mAheadAnalyzer->getStartFileNo() ) )
    {
        IDE_TEST( dequeueAndSend( aActionAddXLog,
                                  aSNPool,
                                  aSNList )
                  != IDE_SUCCESS );

        mAheadAnalyzer->checkAndRemoveCompleteReplicatedTransGroup( aSN );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( mAheadAnalyzer->checkAndStartOrStopAnalyze( mIsRPLogBufMode,
                                                          aFileNo,
                                                          &sIsStarted,
                                                          &sIsStoped ) 
              != IDE_SUCCESS );

    if ( sIsStarted == ID_TRUE )
    {
        *aIsStartedAheadAnalyzer = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }

    if ( sIsStoped == ID_TRUE )
    {
        *aIsStartedAheadAnalyzer = ID_FALSE;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReplicator::joinAheadAnalyzerThread( void )
{
    if ( mAheadAnalyzer != NULL )
    {
        if ( mAheadAnalyzer->join() != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
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

void rpxReplicator::shutdownAheadAnalyzerThread( void )
{
    if ( mAheadAnalyzer != NULL )
    {
        mAheadAnalyzer->shutdown();
    }
    else
    {
        /* do nothing */
    }
}

void rpxReplicator::setExitFlagAheadAnalyzerThread( void )
{
    if ( mAheadAnalyzer != NULL )
    {
        mAheadAnalyzer->setExitFlag();
    }
    else
    {
        /* do nothing */
    }
}

UInt rpxReplicator::getActiveTransCount( void )
{
    IDE_DASSERT( mTransTbl != NULL );

    return mTransTbl->getATransCnt();
}

idBool rpxReplicator::isMyLog( smTID  aTransID,
                               smSN   aCurrentSN )
{
    idBool      sRC = ID_FALSE;

    if ( mSender->mCurrentType == RP_PARALLEL )
    {
        IDE_DASSERT( mSender->isParallelParent() == ID_TRUE );

        switch ( mSender->getStatus() )
        {
            case RP_SENDER_FAILBACK_NORMAL:
            case RP_SENDER_FAILBACK_MASTER:
            case RP_SENDER_FAILBACK_SLAVE:
                sRC = ID_TRUE;
                break;
            case RP_SENDER_FLUSH_FAILBACK:
                if ( aCurrentSN <= mSender->getFailbackEndSN() )
                {
                    sRC = ID_TRUE;
                }
                else
                {
                    if ( mTransTbl->getBeginSN( aTransID ) <= mSender->getFailbackEndSN() )
                    {
                        sRC = ID_TRUE;
                    }
                    else
                    {
                        sRC = ID_FALSE;
                    }
                }
                break;
            case RP_SENDER_IDLE:
            case RP_SENDER_RETRY:
                IDE_DASSERT( 0 );
                break;
            case RP_SENDER_STOP:
                sRC = ID_FALSE;
                break;
            default:
                IDE_DASSERT( 0 );
                break;
        }
    }
    else
    {
        sRC = ID_TRUE;
    }

    return sRC;
}
idBool rpxReplicator::isReplPropagableLog( smiLogRec * aLog )
{
    /* BUG-42613 propagation use repl recovery log */
    return aLog->needReplRecovery();
}

idBool rpxReplicator::checkUsefulLogByTableMetaType( smiLogRec * aLog )
{
    idBool         sIsOk     = ID_FALSE;
    smiTableMeta * sItemMeta = aLog->getTblMeta();

    switch( rpdMeta::getTableMetaType( aLog->getTableOID(), sItemMeta->mTableOID ) )
    {
        case RP_META_INSERT_ITEM:
        case RP_META_UPDATE_ITEM:
                if ( idlOS::strncmp( mMeta->mReplication.mRepName,
                                     sItemMeta->mRepName,
                                     QCI_MAX_NAME_LEN + 1 )
                     == 0 )
                {
                    sIsOk = ID_TRUE;
                }

            break;
        
        case RP_META_DELETE_ITEM:
                if ( sItemMeta->mRepName[0] == 0 )
                {
                    /* ALTER TABLE ... */
                    sIsOk = ID_TRUE;
                }
                else
                {
                    /* ALTER REPLICATION ... */
                    if ( idlOS::strncmp( mMeta->mReplication.mRepName,
                                         sItemMeta->mRepName,
                                         QCI_MAX_NAME_LEN + 1 ) 
                         == 0 )
                    {
                        sIsOk = ID_TRUE;
                    }
                }
            break;

        default:
            IDE_DASSERT( 0 );
            break;
    }

    return sIsOk;
}

IDE_RC rpxReplicator::getDDLInfoFromDDLStmtLog( smTID   aTID, 
                                                SInt    aMaxDDLStmtLen,
                                                SChar * aUserName,
                                                SChar * aDDLStmt )
{
    UInt sOffset = 0;
    SInt sDDLStmtLen = 0;
    SChar * sLogBody = NULL;
    rpdDDLStmtMetaLog * sDDLStmtMetaLog = NULL;

    mTransTbl->getDDLStmtMetaLog( aTID, &sDDLStmtMetaLog );
    sLogBody = (SChar *)sDDLStmtMetaLog->mLogBody;

    idlOS::strncpy( aUserName,
                    sDDLStmtMetaLog->mDDLStmtMeta.mUserName,
                    SMI_MAX_NAME_LEN );
    aUserName[SMI_MAX_NAME_LEN] = '\0';

    idlOS::memcpy( (void *)&sDDLStmtLen,
                   (const void *)&sLogBody[sOffset],
                   ID_SIZEOF(SInt) );
    sOffset += ID_SIZEOF(SInt);

    IDE_TEST_RAISE( sDDLStmtLen > aMaxDDLStmtLen, ERR_TOO_LONG_DDL );

    idlOS::memcpy( (void *)aDDLStmt,
                   (const void *)&sLogBody[sOffset],
                   sDDLStmtLen );
    sOffset += sDDLStmtLen;

    aDDLStmt[sDDLStmtLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_DDL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "Too long DDL statement." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::getTargetNamesFromItemMetaEntry( smTID    aTID,
                                                       UInt   * aTargetCount,
                                                       SChar  * aTargetTableName,
                                                       SChar ** aTargetPartNames )
{
    UInt           sOffset       = 0;
    UInt           sTargetCount  = 0;
    smiTableMeta * sTableMeta    = NULL;
    iduListNode  * sNode         = NULL;
    iduList      * sItemMetaList = NULL;
    SChar        * sTargetPartName     = NULL;
    SChar        * sTargetPartNames    = NULL;
    rpdMetaItem      * sMetaItem       = NULL;
    rpdMetaItem      * sTargetMetaItem = NULL;
    rpdItemMetaEntry * sItemMetaEntry  = NULL;

    sItemMetaList = mTransTbl->getItemMetaList( aTID );

    IDU_LIST_ITERATE( sItemMetaList, sNode )
    {
        sItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
        sTableMeta = &( sItemMetaEntry->mItemMeta );

        IDE_TEST( mMeta->searchTable( &sMetaItem, (ULong)( sTableMeta->mOldTableOID ) ) 
                  != IDE_SUCCESS );
        if ( sMetaItem != NULL )
        {
            sTargetCount += 1;
        }
    }

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                 sTargetCount * 
                                 ( ID_SIZEOF(SChar) * ( QC_MAX_OBJECT_NAME_LEN + 1 ) ),
                                 (void**)&( sTargetPartNames ),
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    IDU_LIST_ITERATE( sItemMetaList, sNode )
    {
        sItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
        sTableMeta = &( sItemMetaEntry->mItemMeta );

        IDE_TEST( mMeta->searchTable( &sMetaItem, (ULong)( sTableMeta->mOldTableOID ) ) 
                  != IDE_SUCCESS );
        if ( sMetaItem != NULL )
        {
            sTargetMetaItem = sMetaItem;
            sTargetPartName = sTargetPartNames + sOffset;
                
            idlOS::memcpy( sTargetPartName,
                           sMetaItem->mItem.mLocalPartname,
                           QC_MAX_OBJECT_NAME_LEN + 1 );
            sTargetPartName[QC_MAX_OBJECT_NAME_LEN] = '\0';

            sOffset += QC_MAX_OBJECT_NAME_LEN + 1;
        }
    }

    if ( sTargetMetaItem != NULL ) 
    { 
        idlOS::memcpy( aTargetTableName,
                       sTargetMetaItem->mItem.mLocalTablename,
                       QC_MAX_OBJECT_NAME_LEN + 1 );           

        aTargetTableName[QC_MAX_OBJECT_NAME_LEN] = '\0';
    }

    *aTargetCount = sTargetCount;
    *aTargetPartNames = sTargetPartNames;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sTargetPartNames != NULL )
    {
        (void)iduMemMgr::free( sTargetPartNames );
        sTargetPartNames = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::readMyLog( smSN    * aCurrentSN, 
                                 smLSN   * aReadLSN, 
                                 SChar   * aLogHead,
                                 SChar  ** aLogPtr,
                                 SChar  ** aRawLogPtr,
                                 idBool  * aIsValid )
{
    SChar       * sLogPtr         = NULL;
    SChar		* sLogPtrTemp	  = NULL;
    SChar       * sLogHeadPtrTemp = NULL;
    idBool        sIsOfflineStatusFound = ID_FALSE;
    idBool        sIsNeedDecompress     = ID_FALSE;

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE );

    /* BUGBUG: For Parallel Logging */
    RP_OPTIMIZE_TIME_BEGIN( mOpStatistics,
                            IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE );

    IDE_TEST_RAISE( mLogMgr.readLog( aCurrentSN,
                                     aReadLSN,
                                     (void**)&sLogHeadPtrTemp,
                                     (void**)&sLogPtrTemp,
                                     aIsValid )
                    != IDE_SUCCESS, ERR_READ_LOG_RECORD );
    *aRawLogPtr = sLogPtrTemp;

    RP_OPTIMIZE_TIME_END( mOpStatistics,
                          IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE );

    if ( *aIsValid == ID_TRUE ) 
    {
        // BUG-29115
        // 현재 읽고 있는 file no
        mCurrFileNo = aReadLSN->mFileNo;

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
    else
    {
        /* GSN값을 증가시키고 로그를 기록하기 때문에 로그가 기록되기전에
           Sender가 읽을 경우 이런 경우가 발생함 정상상황임.*/
        if ( mSender->mCurrentType == RP_OFFLINE )
        {
            mSender->setExitFlag();
            rpcManager::setOfflineCompleteFlag( mSender->mRepName,
                                                ID_TRUE,
                                                &sIsOfflineStatusFound );
            IDE_ASSERT( sIsOfflineStatusFound == ID_TRUE );
        }
        else
        {
            // BUG-29115
            // archive log를 읽고 있는 상태에서는 이런 log가 발생할 수 없다.
            if ( mLogMgrStatus == RP_LOG_MGR_ARCHIVE )
            {
                // archive log를 읽는 경우는 ala일때 뿐이다.
                IDE_DASSERT( mSender->getRole() == RP_ROLE_ANALYSIS );

                // 예외처리에서 에러를 셋팅한다.
                IDE_RAISE( ERR_LOG_READ );
            }
        }

        *aLogPtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_READ_LOG_RECORD );
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_LOG_READ );
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( mOpStatistics, IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE );

    return IDE_FAILURE;
}

IDE_RC rpxReplicator::isNeedDecompress( SChar  * aRawLogPtr,
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
            IDE_TEST( mMeta->searchTable( &sMetaItem, sTableOID )
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
