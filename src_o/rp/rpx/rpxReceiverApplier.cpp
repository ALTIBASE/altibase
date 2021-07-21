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

#include <rpxReceiverApplier.h>

#include <rpcManager.h>

#define RPX_RECEIVER_APPLIER_SLEEP_SEC      ( 5 )

void rpxReceiverApplier::initialize( rpxReceiver           * aReceiver,
                                     SChar                 * aRepName,
                                     UInt                    aApplierIndex,
                                     rpReceiverStartMode     aStartMode,
                                     iduMemAllocator       * aAllocator )
{
    mReceiver = aReceiver;
    mRepName = aRepName;
    mMyIndex = aApplierIndex;
    mExitFlag = ID_FALSE;
    mAllocator = aAllocator;
    mStatus = RECV_APPLIER_STATUS_INITIALIZE;
    mStartMode = aStartMode;
    mProcessedSN = SM_SN_MIN;
    mProcessedLogCount = 0;
    mIsDoneInitializeThread = ID_FALSE;
    mSNMapMgr = NULL;
    mIsWait = ID_FALSE;
}

void rpxReceiverApplier::finalize( void )
{
    mExitFlag = ID_TRUE;

    if ( mIsDoneInitializeThread == ID_TRUE )
    {
        mApply.finalizeInLocalMemory();

        mQueue.setExitFlag();

        releaseQueue();

        mQueue.destroy();

        if ( mThreadJoinCV.destroy() != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* do nothing */
        }

        if ( mThreadJoinMutex.destroy() != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* do nothing */
        }

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
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxReceiverApplier::initializeThread( void )
{
    SChar       sName[IDU_MUTEX_NAME_LEN + 1] = { 0, };
    UInt        sStage = 0;

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initialize::mMutex",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mMutex->initialize" );

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_RECV_APPLIER_MUTEX", mRepName );
    IDE_TEST( mMutex.initialize( sName,
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initialize::mCV",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mCV->initialize" );

    IDE_TEST( mCV.initialize() != IDE_SUCCESS );
    sStage = 2;

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initialize::mThreadJoinMutex",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mThreadJoinMutex->initialize" );

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_RECV_APPLIER_MUTEX", mRepName );
    IDE_TEST( mThreadJoinMutex.initialize( sName,
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );
    sStage = 3;

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initialize::mThreadJoinCV",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mThreadJoinCV->initialize" );

    IDE_TEST( mThreadJoinCV.initialize() != IDE_SUCCESS );
    sStage = 4;

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initialize::mApply",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mApply->initialize" );

    IDE_TEST( mApply.initialize( mReceiver, mStartMode ) != IDE_SUCCESS );
    sStage = 5;

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initialize::mQueue",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mQueue->initialize" );

    IDE_TEST( mQueue.initialize( mRepName ) != IDE_SUCCESS );
    sStage = 6;
    

    IDU_FIT_POINT( "rpxReceiverApplier::initializeThread::initializeInLocalMemory::mApply",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mApply->initializeInLocalMemory" );

    IDE_TEST( mApply.initializeInLocalMemory() != IDE_SUCCESS );
    sStage = 7;

    mApply.setSNMapMgr( mSNMapMgr );

    mIsDoneInitializeThread = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 7:
            mApply.finalizeInLocalMemory();
        case 6:
            mQueue.destroy();
        case 5:
            mApply.shutdown();
        case 4:
            (void)mThreadJoinCV.destroy();
        case 3:
            (void)mThreadJoinMutex.destroy();
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

void rpxReceiverApplier::finalizeThread( void )
{
    mApply.shutdown();
}

void rpxReceiverApplier::updateProcessedSN( rpdXLog  * aXLog )
{
    /* 
     * Update Last Processed SN 
     * Incoming SN should be old SN but old SN must not set Processed sName
     * It may cause infinite loop  
     */
    if ( mProcessedSN < aXLog->mSN )
    {
        mProcessedSN = aXLog->mSN;
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiverApplier::releaseQueue( void )
{    
    rpdXLog         * sXLog     = NULL;

    /* Queue 에 남은 모든 XLog 를 Receiver 의 FreeQueue 로 보낸다. */
    mQueue.read( &sXLog );
    while( sXLog != NULL )
    {
        mReceiver->enqueueFreeXLogQueue( sXLog );
        sXLog = NULL;
    
        mQueue.read( &sXLog );
    }
}

void rpxReceiverApplier::run( void )
{
    rpdXLog         * sXLog = NULL;
    idBool            sIsEnd = ID_FALSE;

    IDE_CLEAR();

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_IS_START, mReceiver->mRepName, mMyIndex );

    while ( mExitFlag != ID_TRUE )
    {
        IDE_CLEAR();

        mStatus = RECV_APPLIER_STATUS_DEQUEUEING;

        IDE_TEST( dequeue( &sXLog ) != IDE_SUCCESS );

        IDE_TEST( processXLog( sXLog,
                               &sIsEnd ) 
                  != IDE_SUCCESS );

        mReceiver->enqueueFreeXLogQueue( sXLog );
        sXLog = NULL;
    }

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_PROCESSED_XLOG_COUNT, 
                           mReceiver->mRepName, mMyIndex, mProcessedLogCount );

    if ( sIsEnd != ID_TRUE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ALREADY_FINAL ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_IS_STOP, mReceiver->mRepName, mMyIndex );

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_REMAINED_QUEUE_INFO, mReceiver->mRepName, mMyIndex, mQueue.getSize() );

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_LAST_PROCESSED_SN, mReceiver->mRepName, mMyIndex, mApply.getApplyXSN() );

    mStatus = RECV_APPLIER_STATUS_STOP;

    return;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_GOT_FAILURE, mReceiver->mRepName, mMyIndex );

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_REMAINED_QUEUE_INFO, mReceiver->mRepName, mMyIndex, mQueue.getSize() );

    ideLog::log( IDE_RP_0, RP_TRC_X_APPLIER_LAST_PROCESSED_SN, mReceiver->mRepName, mMyIndex, mApply.getApplyXSN() );

    if ( sXLog != NULL )
    {
        mReceiver->enqueueFreeXLogQueue( sXLog );
    }
    else
    {
        /* do nothing */
    }

    mReceiver->shutdown();

    mStatus = RECV_APPLIER_STATUS_STOP;

    return;
}

IDE_RC rpxReceiverApplier::processXLog( rpdXLog    * aXLog,
                                        idBool     * aIsEnd )
{
    mStatus = RECV_APPLIER_STATUS_WAITING;
    IDE_TEST( mReceiver->checkAndWaitApplier( aXLog->mWaitApplierIndex,
                                              aXLog->mWaitSNFromOtherApplier,
                                              &mMutex,
                                              &mCV,
                                              &mIsWait )
              != IDE_SUCCESS );

    mStatus = RECV_APPLIER_STATUS_WORKING;

    IDU_FIT_POINT( "rpxReceiverApplier::run::apply::mApply",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mApply.apply" );
    IDE_TEST( mApply.apply( aXLog ) != IDE_SUCCESS );
    mProcessedLogCount++;

    updateProcessedSN( aXLog );

    switch( aXLog->mType )
    {
        case RP_X_COMMIT:
        case RP_X_XA_COMMIT:
        case RP_X_ABORT:
            mReceiver->wakeupReceiverApplier();
            break;

        case RP_X_REPL_STOP:
            ideLog::log( IDE_RP_0, RP_TRC_R_STOP_MSG_ARRIVED );
            mExitFlag = ID_TRUE;
            *aIsEnd = ID_TRUE;
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiverApplier::enqueue( rpdXLog     * aXLog )
{
    mQueue.write( aXLog );
}

IDE_RC rpxReceiverApplier::dequeue( rpdXLog    ** aXLog )
{
    IDE_TEST( mQueue.read( aXLog, RPX_RECEIVER_APPLIER_SLEEP_SEC ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiverApplier::setTransactionFlagReplReplicated( void )
{
    mApply.setTransactionFlagReplReplicated();
}

void rpxReceiverApplier::setTransactionFlagReplRecovery( void )
{
    mApply.setTransactionFlagReplRecovery();
}

void rpxReceiverApplier::setTransactionFlagCommitWriteWait( void )
{
    mApply.setTransactionFlagCommitWriteWait();
}

void rpxReceiverApplier::setTransactionFlagCommitWriteNoWait( void )
{
    mApply.setTransactionFlagCommitWriteNoWait();
}

void rpxReceiverApplier::setFlagToSendAckForEachTransactionCommit( void )
{
    mApply.setFlagToSendAckForEachTransactionCommit();
}

void rpxReceiverApplier::setFlagNotToSendAckForEachTransactionCommit( void )
{
    mApply.setFlagNotToSendAckForEachTransactionCommit();
}

void rpxReceiverApplier::setApplyPolicyCheck( void )
{
    mApply.setApplyPolicyCheck();
}

void rpxReceiverApplier::setApplyPolicyForce( void )
{
    mApply.setApplyPolicyForce();
}

void rpxReceiverApplier::setApplyPolicyByProperty( void )
{
    mApply.setApplyPolicyByProperty();
}

void rpxReceiverApplier::setSNMapMgr( rprSNMapMgr * aSNMapMgr )
{
    mSNMapMgr = aSNMapMgr;
}

smSN rpxReceiverApplier::getProcessedSN( void )
{
    return mProcessedSN;
}

smSN rpxReceiverApplier::getLastCommitSN( void )
{
    return mApply.getLastCommitSN();
}

SInt rpxReceiverApplier::getATransCntFromTransTbl( void )
{
    return mApply.getATransCntFromTransTbl();
}

UInt rpxReceiverApplier::getAssingedXLogCount( void )
{
    return mQueue.getSize();
}

void rpxReceiverApplier::setExitFlag( void )
{
    mExitFlag = ID_TRUE;

    if ( mIsDoneInitializeThread == ID_TRUE )
    {
        mQueue.setExitFlag();
    }
    else
    {
        /* Nothing to do */
    }
}

IDE_RC rpxReceiverApplier::allocRangeColumn( UInt   aCount )
{
    IDE_TEST( mApply.allocRangeColumn( aCount ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

smSN rpxReceiverApplier::getRestartSN( void )
{
    return mApply.getRestartSN();
}

void rpxReceiverApplier::setParallelApplyInfo( rpxReceiverParallelApplyInfo * aApplierInfo )
{
    aApplierInfo->mRepName = mRepName;
    aApplierInfo->mParallelApplyIndex = mMyIndex;
    aApplierInfo->mApplyXSN = mApply.getApplyXSN();

    aApplierInfo->mInsertSuccessCount = mApply.getInsertSuccessCount();
    aApplierInfo->mInsertFailureCount = mApply.getInsertFailureCount();
    aApplierInfo->mUpdateSuccessCount = mApply.getUpdateSuccessCount();
    aApplierInfo->mUpdateFailureCount = mApply.getUpdateFailureCount();
    aApplierInfo->mDeleteSuccessCount = mApply.getDeleteSuccessCount();
    aApplierInfo->mDeleteFailureCount = mApply.getDeleteFailureCount();
    aApplierInfo->mCommitCount = mApply.getCommitCount();
    aApplierInfo->mAbortCount = mApply.getAbortCount();
    aApplierInfo->mStatus = mStatus;
}

IDE_RC rpxReceiverApplier::buildRecordForReplReceiverParallelApply( void                * aHeader,
                                                                    void                * /* aDumpObj */,
                                                                    iduFixedTableMemory * aMemory )
{
    rpxReceiverParallelApplyInfo  sApplierInfo;

    setParallelApplyInfo( &sApplierInfo );

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sApplierInfo )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApplier::buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                                               void                    * aDumpObj,
                                                               iduFixedTableMemory     * aMemory,
                                                               UInt                      aParallelID )
{  

    IDE_TEST( mApply.buildRecordForReplReceiverTransTbl( aHeader,
                                                         aDumpObj,
                                                         aMemory,
                                                         mRepName,
                                                         aParallelID,
                                                         mMyIndex )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiverApplier::wakeup( void )
{
    IDE_ASSERT( mMutex.lock( NULL ) == IDE_SUCCESS );

    if ( mIsWait == ID_TRUE )
    {
        mCV.signal();
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
}


