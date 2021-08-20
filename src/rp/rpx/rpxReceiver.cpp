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
 * $Id: rpxReceiver.cpp 91356 2021-07-27 06:07:52Z donghyun1 $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idm.h>
#include <smi.h>
#include <qcuError.h>
#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcManager.h>
#include <rpxReceiver.h>
#include <rpxReceiverApply.h>
#include <rpxReceiverApplier.h>

/* PROJ-2240 */
#include <rpdCatalog.h>
#include <rpcDDLASyncManager.h>

#include <rpdConvertSQL.h>

#include <dki.h>

#define RPX_INDEX_INIT      (-1)


rpxReceiver::rpxReceiver() : idtBaseThread()
{
    mAllocator      = NULL;
    mAllocatorState = 0;
}

void rpxReceiver::destroy()
{
    removeGlobalTxList( &mGlobalTxList );

    finalizeAndDestroyXLogTransfer();

    finalizeAndDestroyXLogfileManager();

    finalizeParallelApplier();

    removeNewMeta();

    if ( mStartMode == RP_RECEIVER_XLOGFILE_RECOVERY )
    {
        if(mXFRecoveryWaitMutex.destroy() != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
        }

        if ( mXFRecoveryWaitCV.destroy() != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* do nothing */
        }
    }

    if(mHashMutex.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if ( mHashCV.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    /* BUG-22703 thr_join Replace */
    if(mThreadJoinCV.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mThreadJoinMutex.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    mApply.finalizeInLocalMemory();
    mApply.destroy();
    mMeta.finalize();

    return;
}

/*
 * @brief It sets receiver applier's transaction flag
 */
void rpxReceiver::setTransactionFlag( void )
{
    /* recovery or replicated */
    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_SYNC_CONDITIONAL:
            if ( ( mMeta.mReplication.mOptions & RP_OPTION_RECOVERY_MASK ) == 
                 RP_OPTION_RECOVERY_SET )
            {
                setTransactionFlagReplRecovery();
            }
            else
            {
                setTransactionFlagReplReplicated();
            }
            break;

        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_RECOVERY:
        case RP_RECEIVER_OFFLINE:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            setTransactionFlagReplReplicated();
            break;
    }

    /* wait or nowait */
    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_OFFLINE:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            setTransactionFlagCommitWriteNoWait();
            break;

        case RP_RECEIVER_RECOVERY:
            setTransactionFlagCommitWriteWait();
            break;
    }
}

void rpxReceiver::setTransactionFlagReplReplicated( void )
{
    UInt    i = 0;

    mApply.setTransactionFlagReplReplicated();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setTransactionFlagReplReplicated();
        }
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiver::setTransactionFlagReplRecovery( void )
{
    UInt    i = 0;

    mApply.setTransactionFlagReplRecovery();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setTransactionFlagReplRecovery();
        }
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiver::setTransactionFlagCommitWriteWait( void )
{
    UInt    i = 0;

    mApply.setTransactionFlagCommitWriteWait();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setTransactionFlagCommitWriteWait();
        }
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiver::setTransactionFlagCommitWriteNoWait( void )
{
    UInt    i = 0;

    mApply.setTransactionFlagCommitWriteNoWait();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setTransactionFlagCommitWriteNoWait();
        }
    }
    else
    {
        /* do nothing */
    }
}

/*
 *
 */
void rpxReceiver::setApplyPolicy( void )
{
    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_OFFLINE:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            switch ( mMeta.mReplication.mConflictResolution )
            {
                case RP_CONFLICT_RESOLUTION_MASTER:
                    setApplyPolicyCheck();
                    break;
                    
                case RP_CONFLICT_RESOLUTION_SLAVE:
                    setApplyPolicyForce();
                    break;
                    
                case RP_CONFLICT_RESOLUTION_NONE:
                default:
                    setApplyPolicyByProperty();
                    break;
            }
            break;

        case RP_RECEIVER_RECOVERY:
            setApplyPolicyForce();
            break;
    }
}

void rpxReceiver::setApplyPolicyCheck( void )
{
    UInt    i = 0;

    mApply.setApplyPolicyCheck();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setApplyPolicyCheck();
        }
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiver::setApplyPolicyForce( void )
{
    UInt    i = 0;


    mApply.setApplyPolicyForce();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setApplyPolicyForce();
        }
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiver::setApplyPolicyByProperty( void )
{
    UInt    i = 0;

    mApply.setApplyPolicyByProperty();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setApplyPolicyByProperty();
        }
    }
    else
    {
        /* do nothing */
    }
}

/*
 *
 */
void rpxReceiver::setSendAckFlag( void )
{
    switch ( mStartMode )
    {
        case RP_RECEIVER_RECOVERY:
            setFlagToSendAckForEachTransactionCommit();
            break;

        default:
            setFlagNotToSendAckForEachTransactionCommit();
            break;
    }
}

void rpxReceiver::setFlagToSendAckForEachTransactionCommit( void )
{
    UInt    i = 0;

    mApply.setFlagToSendAckForEachTransactionCommit();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setFlagToSendAckForEachTransactionCommit();
        }
    }
    else
    {
        /* do nothing */
    }
}

void rpxReceiver::setFlagNotToSendAckForEachTransactionCommit( void )
{
    UInt    i = 0;

    mApply.setFlagNotToSendAckForEachTransactionCommit();

    if ( isSync() == ID_FALSE )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setFlagNotToSendAckForEachTransactionCommit();
        }
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxReceiver::initializeParallelApplier( UInt     aParallelApplierCount )
{
    UInt                    i = 0;
    UInt                    sIndex = 0;
    rpxReceiverApplier    * sApplier = NULL;
    idBool                  sIsAlloc = ID_FALSE;
    idBool                  sIsApplierIndexAlloc = ID_FALSE;
    UInt                    sIsInitailzedFreeXLogQueue = ID_FALSE;

    IDU_FIT_POINT_RAISE( "rpxReceiver::initializeParallelApplier::malloc::mApplier",
                         ERR_MEMORY_ALLOC_MAPPLIER );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                       ID_SIZEOF( rpxReceiverApplier ) * aParallelApplierCount,
                                       (void**)&mApplier,
                                       IDU_MEM_IMMEDIATE,
                                       mAllocator )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_MAPPLIER );
    sIsAlloc = ID_TRUE;

    for ( sIndex = 0; sIndex < aParallelApplierCount; sIndex++ )
    {
        sApplier = &mApplier[sIndex];

        new (sApplier) rpxReceiverApplier;

        sApplier->initialize( this,
                              mRepName,
                              sIndex,
                              mStartMode,
                              mAllocator );

        if ( mSNMapMgr != NULL )
        {
            sApplier->setSNMapMgr( mSNMapMgr );
        }
        else
        {
            /* do nothing */
        }

        sApplier = NULL;
    }

    IDU_FIT_POINT_RAISE( "rpxReceiver::initializeParallelApplier::malloc::mTransToApplierIndex",
                         ERR_MEMORY_ALLOC_APPLIER_INDEX );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                       ID_SIZEOF( SInt ) * mTransactionTableSize,
                                       (void**)&mTransToApplierIndex,
                                       IDU_MEM_IMMEDIATE,
                                       mAllocator )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_APPLIER_INDEX );
    sIsApplierIndexAlloc = ID_TRUE;

    for ( i = 0; i < mTransactionTableSize; i++ )
    {
        mTransToApplierIndex[i] = -1;
    }

    IDE_TEST( initializeFreeXLogQueue() != IDE_SUCCESS );
    sIsInitailzedFreeXLogQueue = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_MAPPLIER )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initializeParallelApplier",
                                  "mApplier" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_APPLIER_INDEX )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initializeParallelApplier",
                                  "mTransToApplierIndex" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsApplierIndexAlloc == ID_TRUE )
    {
        (void)iduMemMgr::free( mTransToApplierIndex );
        mTransToApplierIndex = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    for ( i = 0 ; i < sIndex; i++ )
    {
        mApplier[i].finalize();
    }

    if ( sIsAlloc == ID_TRUE )
    {
        (void)iduMemMgr::free( mApplier );
        mApplier = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( sIsInitailzedFreeXLogQueue == ID_TRUE )
    {
        finalizeFreeXLogQueue();
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxReceiver::finalizeParallelApplier( void )
{
    UInt    i = 0;

    if ( mApplier != NULL )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
             mApplier[i].finalize();
        }

        (void)iduMemMgr::free( mApplier );
        mApplier = NULL;

        finalizeFreeXLogQueue();
    }
    else
    {
        /* do nothing */
    }

    if ( mTransToApplierIndex != NULL )
    {
        (void)iduMemMgr::free( mTransToApplierIndex );
        mTransToApplierIndex = NULL;
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpxReceiver::initialize( cmiProtocolContext * aProtocolContext,
                                smiStatement       * aStatement,
                                SChar              * aRepName,
                                rpdMeta            * aRemoteMeta,
                                rpdMeta             *aMeta,
                                rpReceiverStartMode  aStartMode,
                                rpxReceiverErrorInfo aErrorInfo,
                                UInt                 aReplID )
{
    SChar    sName[IDU_MUTEX_NAME_LEN + 1] = { 0, };

    mEndianDiff         = ID_FALSE;
    mExitFlag           = ID_FALSE;
    mNetworkError       = ID_FALSE;

    mProtocolContext    = aProtocolContext;
    mIsRecoveryComplete = ID_FALSE;
    mIsFailoverStepEnd  = ID_FALSE;
    mErrorInfo          = aErrorInfo;

    mAllocator      = NULL;
    mAllocatorState = 0;

    mProcessedXLogCount = 0;
    mSNMapMgr = NULL;
    mRestartSN = SM_SN_NULL;
    
    mRemoteCheckpointSN = SM_SN_NULL;

    mTransToApplierIndex = NULL;

    mXLogPtr = NULL;

    mApplierQueueSize = 0;
    mXLogSize = 0;

    mXLogfileManager = NULL;
    mXLogTransfer = NULL;

    mStartMode = aStartMode;

    IDU_LIST_INIT( &mGlobalTxList );

    RPX_RECEIVER_INIT_READ_CONTEXT( &mReadContext );

    setSelfExecuteDDLTransID( SM_NULL_TID );

    idlOS::memset( mMyIP, 0x00, RP_IP_ADDR_LEN );
    idlOS::memset( mPeerIP, 0x00, RP_IP_ADDR_LEN );

    mEventFlag = ID_ULONG(0);

    /* BUG-31545 : receiver 통계정보 수집을 위한 임의 session을 초기화 */
    idvManager::initSession(&mStatSession, 0 /* unuse */, NULL /* unuse */);
    idvManager::initSession(&mOldStatSession, 0 /* unuse */, NULL /* unuse */);
    idvManager::initSQL(&mStatistics,
                        &mStatSession,
                        &mEventFlag, 
                        NULL, 
                        NULL, 
                        NULL, 
                        IDV_OWNER_UNKNOWN);
    // PROJ-2453
    mAckEachDML = ID_FALSE;

    mApplier = NULL;

    mMeta.initialize();
    mNewMeta = NULL;

    mXLogTransfer = NULL;
    mXLogfileManager = NULL;

    if ( useNetworkResourceMode() == ID_TRUE )
    {
        IDE_DASSERT( aProtocolContext != NULL );
        cmiGetLinkForProtocolContext( aProtocolContext, &mLink );
        setTcpInfo();
    }
    
    IDE_TEST( buildMeta( aStatement,
                         aRepName )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "1.BUG-17029@rpxReceiver::initialize", 
                          ERR_TRANS_COMMIT );

    if( rpdMeta::isUseV6Protocol( &( mMeta.mReplication ) ) == ID_TRUE )
    {
        IDE_TEST_RAISE( mMeta.hasLOBColumn() == ID_TRUE,
                        ERR_NOT_SUPPORT_LOB_COLUMN_WITH_V6_PROTOCOL );
    }
    else
    {
        /* do nothing */
    }

    // PROJ-1537
    IDE_TEST_RAISE( ( mMeta.mReplication.mRole == RP_ROLE_ANALYSIS ) ||
                    ( mMeta.mReplication.mRole == RP_ROLE_ANALYSIS_PROPAGATION ), ERR_ROLE );

    mIsWait = ID_FALSE;
    IDE_TEST_RAISE( mHashCV.initialize() != IDE_SUCCESS, ERR_COND_INIT );

    idlOS::memset( (void *)&mHashMutex,
                   0,
                   ID_SIZEOF(iduMutex) );

    idlOS::snprintf(sName, IDU_MUTEX_NAME_LEN, "RP_%s_RECV_MUTEX", aRepName);
    IDE_TEST_RAISE(mHashMutex.initialize((SChar *)sName,
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    /* BUG-22703 thr_join Replace */
    mIsThreadDead = ID_FALSE;
    IDE_TEST_RAISE(mThreadJoinCV.initialize() != IDE_SUCCESS,
                   ERR_COND_INIT);
    IDE_TEST_RAISE(mThreadJoinMutex.initialize((SChar *)"REPL_RECV_THR_JOIN_MUTEX",
                                               IDU_MUTEX_KIND_POSIX,
                                               IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    if ( mStartMode == RP_RECEIVER_XLOGFILE_RECOVERY )
    {
        IDE_TEST_RAISE( mXFRecoveryWaitCV.initialize() != IDE_SUCCESS, ERR_COND_INIT );

        idlOS::memset( (void *)&mXFRecoveryWaitMutex,
                       0,
                       ID_SIZEOF(iduMutex) );

        idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RPX_%s_RECV_XF_RECOVERY_MUTEX", aRepName );
        IDE_TEST_RAISE( mXFRecoveryWaitMutex.initialize( (SChar *)sName,
                                                           IDU_MUTEX_KIND_POSIX,
                                                           IDV_WAIT_INDEX_NULL )
                        != IDE_SUCCESS, ERR_MUTEX_INIT);

        mXFRecoveryStatus = RPX_XF_RECOVERY_INIT;
    }
    else
    {
        mXFRecoveryStatus = RPX_XF_RECOVERY_NONE;
    }

    // For Fixed Table(Foreign key)
    //idlOS::strcpy(mRepName, aRepName);
    idlOS::strncpy( mRepName, mMeta.mReplication.mRepName, QCI_MAX_OBJECT_NAME_LEN );
    mRepName[QCI_MAX_OBJECT_NAME_LEN] = '\0';

    mParallelID = aMeta->mReplication.mParallelID;
    //PROJ-1915
    if ( ( mStartMode != RP_RECEIVER_PARALLEL ) || ( mParallelID == RP_PARALLEL_PARENT_ID ) )
    {
        mRemoteMeta = aRemoteMeta;
    }
    else
    {
        mRemoteMeta = NULL;
    }

    IDE_TEST_RAISE( mApply.initialize( this, mStartMode ) != IDE_SUCCESS, ERR_APPLY_INIT);

    mApplierCount = getParallelApplierCount();
    mTransactionTableSize = smiGetTransTblSize();

    mRestartSN = getRestartSN();

    mLastWaitApplierIndex = 0;
    mLastWaitSN = 0;

    // PROJ-1553 Self Deadlock
    mReplID = aReplID;

    mLastReceivedSN = SM_SN_NULL;

    IDE_TEST( initializeXLogfileContents( aStatement ) != IDE_SUCCESS );
    
    mReadContext = setReadContext( mProtocolContext, mXLogfileManager );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_LOB_COLUMN_WITH_V6_PROTOCOL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_SUPPORT_FEATURE_WITH_V6_PROTOCOL,
                                  "Replication with LOB columns") );
    }
    IDE_EXCEPTION(ERR_ROLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_ROLE_NOT_SUPPORT_RECEIVER));
    }
    IDE_EXCEPTION(ERR_TRANS_COMMIT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_TX_COMMIT));
    }
    IDE_EXCEPTION( ERR_COND_INIT );
    {
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrCondInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl Receiver] Condition variable initialization error" );
    }
    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl Receiver] Mutex initialization error" );
    }
    IDE_EXCEPTION(ERR_APPLY_INIT);
    {
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    IDE_PUSH();

    if ( mXLogTransfer != NULL )
    {
        finalizeAndDestroyXLogTransfer();
    }

    if ( mXLogfileManager != NULL )
    {
        finalizeAndDestroyXLogfileManager();
    }

    mMeta.freeMemory();

    IDE_POP();

    /* BUG-22703 thr_join Replace */
    mIsThreadDead = ID_TRUE;

    if ( ( mPeerPort != 0 ) && ( aRepName != NULL ) )
    {
        ideLog::log( IDE_RP_0, RP_TRC_R_PEER_IP_PORT_NAME, mPeerIP, mPeerPort, aRepName );
    }

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::initializeThread()
{
    idCoreAclMemAllocType sAllocType = (idCoreAclMemAllocType)iduMemMgr::getAllocatorType();
    idCoreAclMemTlsfInit  sAllocInit = {0};
    rpdMeta             * sRemoteMeta = NULL;
    UInt                  sStage = 0;

    /* Thread의 run()에서만 사용하는 메모리를 할당한다. */

    if ( sAllocType != ACL_MEM_ALLOC_LIBC )
    {
        IDU_FIT_POINT_RAISE( "rpxReceiver::initializeThread::malloc::MemAllocator",
                              ERR_MEMORY_ALLOC_ALLOCATOR );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_RECEIVER,
                                           ID_SIZEOF( iduMemAllocator ),
                                           (void **)&mAllocator,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_ALLOCATOR );
        mAllocatorState = 1;

        sAllocInit.mPoolSize = RP_MEMPOOL_SIZE;

        IDE_TEST( iduMemMgr::createAllocator( mAllocator,
                                              sAllocType,
                                              &sAllocInit )
                  != IDE_SUCCESS );
        mAllocatorState = 2;
    }
    else
    {
        /* Nothing to do */
    }

    /* mMeta는 rpcManager Thread가 Handshake를 수행할 때에도 사용하므로, 여기에 오면 안 된다. */

    IDE_TEST( mApply.initializeInLocalMemory() != IDE_SUCCESS );

    if ( ( mApplierCount == 0 ) || ( isSync() == ID_TRUE ) )
    {
        /* do nothing */
    }
    else
    {
        IDE_TEST( initializeParallelApplier( mApplierCount ) != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( startApplier() != IDE_SUCCESS );
        sStage = 2;
    }

    setApplyPolicy();

    setTransactionFlag();

    /* PROJ-2725 Consistent replication
     * consistent mode에서 개념적으로 commit 마다 ack를 보내야 하므로,
     * setSendAckFlag()를 호출해줘야 할 것 같다고 생각하기 쉽다.
     * 하지만, consistent mode에서는 xlog transfer가 ack를 send하므로,
     * receiver가 확인하여 전송하는 mAckForTransactionCommit Flag를 set 할 필요가 없다.
     */
    setSendAckFlag();

    /* mHashMutex, mThreadJoinCV, mThreadJoinMutex는 Thread 종료 시에 사용하므로, 여기에 오면 안 된다. */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_ALLOCATOR )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::initializeThread",
                                  "mAllocator" ) );
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG( IDE_RP_0 );

    IDE_PUSH();

    if ( sRemoteMeta != NULL )
    {
        (void)iduMemMgr::free( sRemoteMeta );
        sRemoteMeta = NULL;
    }
    else
    {
        /* do nothing */
    }

    switch( sStage )
    {
        case 2:
            joinApplier();
        case 1:
            finalizeParallelApplier();
        default:
            break;
    }

    switch ( mAllocatorState )
    {
        case 2 :
            (void)iduMemMgr::freeAllocator( mAllocator );
            /* fall through */
        case 1 :
            (void)iduMemMgr::free( mAllocator );
            break;
        default:
            break;
    }

    mAllocator      = NULL;
    mAllocatorState = 0;

    IDE_POP();

    // BUG-22703 thr_join Replace
    signalThreadJoin();

    return IDE_FAILURE;
}

idBool rpxReceiver::isAllApplierFinish( void )
{
    UInt   i         = 0;
    idBool sIsFinish = ID_TRUE;

    for ( i = 0; i < mApplierCount; i++ )
    {
        if ( mApplier[i].getQueSize() > 0 )
        {
            sIsFinish = ID_FALSE;
            break;
        }
        else
        {
            /* nothing to do */
        }
    }

    return sIsFinish;
}

void rpxReceiver::waitAllApplierComplete( void )
{
    PDL_Time_Value  sTimeValue;

    sTimeValue.initialize( 0, 1000 );

    /* Stop 으로 인한 종료가 아닐 경우 Queue 에 남은 XLog 들을 다 반영할때까지 기다린다. */ 
    /* 기다리는 동안 Applier 에서 변경된 mRestartSN 을 반영한다. */
    while ( ( isAllApplierFinish() == ID_FALSE ) && ( mExitFlag != ID_TRUE ) )
    {
        mRestartSN = getRestartSNInParallelApplier();
        saveRestartSNAtRemoteMeta( mRestartSN );

        idlOS::sleep( sTimeValue );
    }
}

void rpxReceiver::waitAllApplierCompleteWhileFailbackSlave( void )
{
    PDL_Time_Value  sTimeValue;

    sTimeValue.initialize( 0, 1000 );

    while ( ( mFreeXLogQueue.getSize() < mApplierQueueSize -1 ) && 
            ( mExitFlag != ID_TRUE ) )
    {
        idlOS::sleep( sTimeValue );
    }
}

IDE_RC rpxReceiver::updateRemoteXSN( smSN aSN )
{
    smiTrans          sTrans;
    SInt              sStage = 0;
    smiStatement     *spRootStmt;
    UInt              sFlag = 0;

    IDE_TEST_CONT((aSN == SM_SN_NULL) || (aSN == 0), NORMAL_EXIT);

    IDE_TEST_RAISE( sTrans.initialize() != IDE_SUCCESS, ERR_TRANS_INIT );
    sStage = 1;

    sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = (sFlag & ~SMI_TRANSACTION_MASK) | SMI_TRANSACTION_NORMAL;
    sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
    sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

    IDE_TEST_RAISE( sTrans.begin(&spRootStmt, &mStatistics, sFlag, mReplID )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );
    sStage = 2;

    IDE_TEST( sTrans.setReplTransLockTimeout( 0 ) != IDE_SUCCESS );

    IDE_TEST_RAISE( rpcManager::updateRemoteXSN( spRootStmt,
                                                 mRepName,
                                                 aSN )
                    != IDE_SUCCESS, ERR_UPDATE_REMOTE_XSN );

    IDE_TEST_RAISE( sTrans.commit() != IDE_SUCCESS, ERR_TRANS_COMMIT );
    sStage = 1;

    sStage = 0;
    IDE_TEST_RAISE( sTrans.destroy( NULL ) != IDE_SUCCESS, ERR_TRANS_DESTROY );

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_INIT )
    {
        IDE_WARNING( IDE_RP_0, "[Receiver] Trans init() error in updateRemoteXSN()" );
    }
    IDE_EXCEPTION( ERR_TRANS_BEGIN )
    {
        IDE_WARNING( IDE_RP_0, "[Receiver] Trans begin() error in updateRemoteXSN()" );
    }
    IDE_EXCEPTION( ERR_UPDATE_REMOTE_XSN )
    {
        ideLog::log( IDE_RP_0, "[Receiver] updateRemoteXSN error" );
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT )
    {
        IDE_WARNING( IDE_RP_0, "[Receiver] Trans commit() error in updateRemoteXSN()" );
    }
    IDE_EXCEPTION( ERR_TRANS_DESTROY )
    {
        IDE_WARNING( IDE_RP_0, "[Receiver] Trans destroy() error in updateRemoteXSN()" );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
			IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
        case 1:
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


void rpxReceiver::finalizeThread()
{
    if ( mApplier != NULL )
    {
        joinApplier();
    }
    else
    {
        /* nothing to do */
    }

    switch ( mAllocatorState )
    {
        case 2 :
            (void)iduMemMgr::freeAllocator( mAllocator );
            /* fall through */
        case 1 :
            (void)iduMemMgr::free( mAllocator );
            break;
        default:
            break;
    }

    mAllocator      = NULL;
    mAllocatorState = 0;

    // BUG-22703 thr_join Replace
    signalThreadJoin();

    return;
}

void rpxReceiver::shutdown() // call by executor
{
    IDE_ASSERT(lock() == IDE_SUCCESS);

    mExitFlag = ID_TRUE;
    IDU_SESSION_SET_CANCELED( *( mStatistics.mSessionEvent ) );

    if ( mApplier != NULL )
    {
        mFreeXLogQueue.setExitFlag();
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return;
}

void rpxReceiver::shutdownAllApplier()
{
    UInt i = 0;

    if ( mApplier != NULL )
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            mApplier[i].setExitFlag();
        }
    }
    else
    {
        /* do nothing */
    }
}


void rpxReceiver::finalize() // call by receiver itself
{
    rpcManager::setDDLSyncCancelEvent( mRepName );

	/* BUG-44863 Applier 가 Queue 에 남은 Log 를 처리하다가 Active Server 가 다시 살아날 경우
     * Receiver 가 종료당하게 되고 updateRemoteXSN 도 locktimeout 에 의해 
     * mRestartSN 이 갱신되지 않는다.
     * 따라서 mRestartSN 변화량을 최소화 하기 위하여 먼저 업데이트를 한번 한다. */
    if ( updateRemoteXSN( mRestartSN ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* nothing to do */
    }

    if ( mApplier != NULL )
    {
        if ( mNetworkError != ID_TRUE )
        {
            /* nothing to do */
        }
        else
        {
            waitAllApplierComplete();            
        }

        shutdownAllApplier();

        mRestartSN = getRestartSNInParallelApplier();
        saveRestartSNAtRemoteMeta( mRestartSN );
    }
    else
    {
        /* nothing to do */
    }

    if ( mStartMode == RP_RECEIVER_XLOGFILE_RECOVERY )
    {
        while( ( mXFRecoveryStatus != RPX_XF_RECOVERY_EXIT ) &&
               ( mXFRecoveryStatus != RPX_XF_RECOVERY_INIT ) )
        {
            idlOS::sleep( 1 );
            //need timeout?
            //if timeout allowed, service thread access fault memory address
        }
    }

    if ( mXLogTransfer != NULL )
    {
        finalizeAndDestroyXLogTransfer();
    }

    if ( mXLogfileManager != NULL )
    {
        finalizeAndDestroyXLogfileManager();
    }

    if ( updateRemoteXSN( mRestartSN ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT(lock() == IDE_SUCCESS);

    mExitFlag = ID_TRUE;

    mApply.shutdown();

    /* BUG-16807
     * if( idlOS::thr_join(mApply.getHandle(), NULL ) != 0 )
     * {
     *    ideLog::log(IDE_RP_0, RP_TRC_R_ERR_FINAL_THREAD_JOIN);
     * }
     */

    if ( useNetworkResourceMode() == ID_TRUE )
    {
        if( mStartMode == RP_RECEIVER_USING_TRANSFER )
        {
            mProtocolContext = mReadContext.mCMContext;
        }

        IDE_ASSERT(mLink != NULL);

        if ( cmiFreeCmBlock( mProtocolContext ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
            IDE_SET( ideSetErrorCode( rpERR_ABORT_FREE_CM_BLOCK ) );
            IDE_ERRLOG( IDE_RP_0 );
        }

        // BUG-16258
        if(cmiShutdownLink(mLink, CMI_DIRECTION_RDWR) != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_SHUTDOWN_LINK));
            IDE_ERRLOG(IDE_RP_0);
        }
        if(cmiCloseLink(mLink) != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_CLOSE_LINK));
            IDE_ERRLOG(IDE_RP_0);
        }

        /* receiver initialize 시에 executor로 부터 받은 protocol context이다. alloc은 executor에서 했지만,
         * free는 receiver종료시에 한다. */
        (void)iduMemMgr::free( mProtocolContext );

        if(mLink != NULL)
        {
            if(cmiFreeLink(mLink) != IDE_SUCCESS)
            {
                IDE_ERRLOG(IDE_RP_0);
                IDE_SET(ideSetErrorCode(rpERR_ABORT_FREE_LINK));
                IDE_ERRLOG(IDE_RP_0);
            }
            mLink = NULL;
        }
    }

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    ideLog::log( IDE_RP_0, RP_TRC_R_LAST_RECEIVED_SN, mRepName, mLastReceivedSN );

    return;
}

/*
 * @brief receive XLog and covert endian of XLog's value
 */
IDE_RC rpxReceiver::receiveAndConvertXLog( rpdXLog * aXLog )
{
    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_R_RECV_XLOG);

    RP_OPTIMIZE_TIME_BEGIN( &mStatistics, IDV_OPTM_INDEX_RP_R_RECV_XLOG );

    IDE_TEST_RAISE( rpnComm::recvXLog( mAllocator,
                                       mReadContext,
                                       &mExitFlag,
                                       &mMeta,    // BUG-20506
                                       aXLog,
                                       RPU_REPLICATION_RECEIVE_TIMEOUT )
                    != IDE_SUCCESS, ERR_RECEIVE_XLOG );
   
    RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_RECV_XLOG );

    if ( aXLog->mSN != SM_SN_NULL )
    {
        mLastReceivedSN = aXLog->mSN;
    }
    else
    {
        /* do nothing */
    }

    if ( mEndianDiff == ID_TRUE )
    {
        IDE_TEST_RAISE( convertEndian( aXLog ) != IDE_SUCCESS,
                        ERR_CONVERT_ENDIAN );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_XLOG );
    {
        if ( mReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK ||
           ( mXLogfileManager->getWaitWrittenXLog() == ID_TRUE ) ) 
        {
            mNetworkError = ID_TRUE;

            if ( isSync() == ID_FALSE )
            {
                IDE_ERRLOG( IDE_RP_0 );
                IDE_SET( ideSetErrorCode( rpERR_ABORT_ERROR_RECVXLOG2, mRepName ) );
            }
            else
            {
                /* nothing to do */
            }
        }
            
        RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_RECV_XLOG );
    }
    IDE_EXCEPTION( ERR_CONVERT_ENDIAN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_CONVERT_ENDIAN ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::sendAckWhenConditionMeetInParallelAppier( rpdXLog * aXLog )
{
    rpXLogAck sAck;

    idBool sNeedSendAck = ID_FALSE;

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_R_SEND_ACK );

    IDE_DASSERT( isEagerReceiver() == ID_FALSE );

    if( mStartMode != RP_RECEIVER_USING_TRANSFER )
    {
        if ( ( mProcessedXLogCount > RPU_REPLICATION_ACK_XLOG_COUNT ) ||
             ( aXLog->mType == RP_X_KEEP_ALIVE ) ||
             ( aXLog->mType == RP_X_REPL_STOP ) ||
             ( aXLog->mType == RP_X_HANDSHAKE ) )
        {
            sNeedSendAck = ID_TRUE;
        }
    }
    else
    {
        if ( ( aXLog->mType == RP_X_HANDSHAKE ) ||
             ( aXLog->mType == RP_X_REPL_STOP ) )
        {
            sNeedSendAck = ID_TRUE;
        }
    }

    if( sNeedSendAck == ID_TRUE )
    {
        IDU_FIT_POINT( "rpxReceiverApply::sendAckWhenConditionMeetInParallelAppier::buildXLogAckInParallelAppiler",
                    rpERR_ABORT_RP_INTERNAL_ARG,
                    "buildXLogAckInParallelAppiler" );
        IDE_TEST( buildXLogAckInParallelAppiler( aXLog, 
                                                &sAck ) 
                != IDE_SUCCESS );

        RP_OPTIMIZE_TIME_BEGIN( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );
        IDE_TEST( sendAck( &sAck) != IDE_SUCCESS );
        RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

        mProcessedXLogCount = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

    return IDE_FAILURE;
}

/*
 * @brief Ack is sent after checking applier's condition and XLog's type 
 */
IDE_RC rpxReceiver::sendAckWhenConditionMeet( rpdXLog * aXLog )
{
    rpXLogAck sAck;

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_R_SEND_ACK );

    if ( ( mProcessedXLogCount > RPU_REPLICATION_ACK_XLOG_COUNT ) ||
         ( mApply.isTimeToSendAck() == ID_TRUE ) ||
         ( aXLog->mType == RP_X_KEEP_ALIVE ) ||
         ( aXLog->mType == RP_X_REPL_STOP )  ||
         ( aXLog->mType == RP_X_HANDSHAKE )  ||          
         ( aXLog->mType == RP_X_DDL_REPLICATE_HANDSHAKE ) || 
         ( aXLog->mType == RP_X_TRUNCATE ) ||
         ( aXLog->mType == RP_X_SYNC_END ) || 
         ( aXLog->mType == RP_X_XA_START_REQ ) ||
         ( aXLog->mType == RP_X_XA_PREPARE_REQ ) ||
         ( aXLog->mType == RP_X_XA_PREPARE ) ||
         ( mAckEachDML == ID_TRUE )
       )
    {
        IDE_TEST( mApply.buildXLogAck( aXLog, &sAck ) != IDE_SUCCESS );

        RP_OPTIMIZE_TIME_BEGIN( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );
        if ( isAckWithTID( aXLog ) != ID_TRUE )
        {
            IDE_TEST( sendAck( &sAck ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sendAckWithTID( &sAck ) != IDE_SUCCESS );
        }
        RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

        mProcessedXLogCount = 0;
        mApply.resetCounterForNextAck();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

    return IDE_FAILURE;
}

idBool rpxReceiver::isAckWithTID( rpdXLog * aXLog )
{
    idBool sWithTID = ID_FALSE;

    if ( isEagerReceiver() != ID_TRUE )
    {
        if ( ( aXLog->mType == RP_X_XA_START_REQ ) ||
             ( aXLog->mType == RP_X_XA_PREPARE_REQ ) ||
             ( aXLog->mType == RP_X_XA_PREPARE ) )
        {
            sWithTID = ID_TRUE;
        }
        else
        {
            sWithTID = ID_FALSE;
        }
    }
    else
    {
        sWithTID = ID_TRUE;
    }

    return sWithTID;
}

IDE_RC rpxReceiver::sendAckForLooseEagerCommit( rpdXLog * aXLog )
{
    rpXLogAck sAck;
    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_R_SEND_ACK );

    if ( ( isEagerReceiver() == ID_TRUE ) && 
         ( ( aXLog->mType == RP_X_COMMIT ) || 
           ( aXLog->mType == RP_X_XA_COMMIT ) )&& 
         ( RPU_REPLICATION_STRICT_EAGER_MODE == 0 ) )
    {
        RP_OPTIMIZE_TIME_BEGIN( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );
        IDE_TEST( mApply.buildXLogAck( aXLog, &sAck ) != IDE_SUCCESS );
        IDE_TEST( sendAckWithTID( &sAck ) != IDE_SUCCESS );
        RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );
        
        mProcessedXLogCount = 0;
        mApply.resetCounterForNextAck();
    }
    else
    {
        /*do nothing*/
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReceiver::sendStopAckForReplicationStop( rpdXLog * aXLog )
{
    rpXLogAck sAck;

    RP_CREATE_FLAG_VARIABLE( IDV_OPTM_INDEX_RP_R_SEND_ACK );

    if ( aXLog->mType == RP_X_REPL_STOP )
    {
        idlOS::memset( &sAck, 0x00, ID_SIZEOF( rpXLogAck ) );
        sAck.mAckType = RP_X_STOP_ACK;
        RP_OPTIMIZE_TIME_BEGIN( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

        IDE_TEST( sendAck( &sAck) != IDE_SUCCESS );

        RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

        mProcessedXLogCount = 0;
        mApply.resetCounterForNextAck();
            
        if ( aXLog->mRestartSN != SM_SN_NULL )
        {
            mRestartSN = aXLog->mRestartSN;
        }

    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    RP_OPTIMIZE_TIME_END( &mStatistics, IDV_OPTM_INDEX_RP_R_SEND_ACK );

    return IDE_FAILURE;
}

void rpxReceiver::setKeepAlive( rpdXLog     * aSrcXLog,
                                rpdXLog     * aDestXLog )
{
    aDestXLog->mType = aSrcXLog->mType;
    aDestXLog->mTID = aSrcXLog->mTID;
    aDestXLog->mSN = aSrcXLog->mSN;
    aDestXLog->mSyncSN = aSrcXLog->mSN;
    aDestXLog->mRestartSN = aSrcXLog->mRestartSN;
    aDestXLog->mWaitApplierIndex = aSrcXLog->mWaitApplierIndex;
    aDestXLog->mWaitSNFromOtherApplier = aSrcXLog->mWaitSNFromOtherApplier;
}

IDE_RC rpxReceiver::enqueueAllApplier( rpdXLog  * aXLog )
{
    rpdXLog   * sXLog = NULL;
    UInt        i = 0;

    for ( i = 0; i < mApplierCount - 1; i++ )
    {
        IDE_TEST( dequeueFreeXLogQueue( &sXLog ) != IDE_SUCCESS );

        setKeepAlive( aXLog, sXLog );
        mApplier[i].enqueue( sXLog );
    }

    mApplier[mApplierCount - 1].enqueue( aXLog );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpxReceiver::enqueueXLog( rpdXLog * aXLog )
{
    UInt        sApplyIndex = 0;

    rpdQueue::setWaitCondition( aXLog, mLastWaitApplierIndex, mLastWaitSN );

    switch ( aXLog->mType )
    {
        case RP_X_KEEP_ALIVE:
            IDE_TEST( enqueueAllApplier( aXLog ) != IDE_SUCCESS );
            break;
        case RP_X_HANDSHAKE:
        case RP_X_REPL_STOP:
            IDE_TEST( enqueueAllApplier( aXLog ) != IDE_SUCCESS );
            IDE_TEST( checkAndWaitAllApplier( aXLog->mSN ) != IDE_SUCCESS );
            break;

        case RP_X_FLUSH:
            IDE_RAISE(enqueueXLog_error);
            break;
        
        case RP_X_FAILBACK_END:
            sApplyIndex = assignApplyIndex( aXLog->mTID );
            waitAllApplierCompleteWhileFailbackSlave();
            mApplier[sApplyIndex].enqueue( aXLog );
            break;

        case RP_X_COMMIT:
        case RP_X_XA_COMMIT:
        case RP_X_ABORT:
            sApplyIndex = assignApplyIndex( aXLog->mTID );
            mLastWaitApplierIndex = sApplyIndex;
            mLastWaitSN = aXLog->mSN;

            deassignApplyIndex( aXLog->mTID );
            mApplier[sApplyIndex].enqueue( aXLog );
            break;

        case RP_X_XA_START_REQ:
        case RP_X_XA_PREPARE_REQ:
        case RP_X_XA_PREPARE:
        default:
            sApplyIndex = assignApplyIndex( aXLog->mTID );
            mApplier[sApplyIndex].enqueue( aXLog );
            break;
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION( enqueueXLog_error );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "enqueue protocol error: flush protocol was not permitted." ) );
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
 *
 */
IDE_RC rpxReceiver::applyXLogAndSendAckInParallelAppiler( rpdXLog * aXLog )
{

    IDE_TEST( enqueueXLog( aXLog ) != IDE_SUCCESS );

    mProcessedXLogCount++;

    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_OFFLINE:
            IDE_TEST( sendAckWhenConditionMeetInParallelAppier( aXLog ) != IDE_SUCCESS );
            break;

        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_RECOVERY:
            IDE_TEST( sendStopAckForReplicationStop( aXLog ) != IDE_SUCCESS );
            break;

        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            /* Nothing to do */
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReceiver::applyXLogAndSendAck( rpdXLog * aXLog )
{
    IDE_TEST( sendAckForLooseEagerCommit( aXLog ) != IDE_SUCCESS );
    IDE_TEST( mApply.apply( aXLog ) != IDE_SUCCESS );

    mProcessedXLogCount++;

    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_OFFLINE:
            IDE_TEST( sendAckWhenConditionMeet( aXLog ) != IDE_SUCCESS );
            break;

        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_RECOVERY:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            IDE_TEST( sendStopAckForReplicationStop( aXLog ) != IDE_SUCCESS );
            break;
        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            break;
    }

    mApply.checkAndResetCounter();
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::processXLogInSync( rpdXLog  * aXLog, 
                                       idBool   * aIsSyncEnd )
{

    IDE_TEST( receiveAndConvertXLog( aXLog ) != IDE_SUCCESS );

    /* first step */
    switch ( aXLog->mType )
    {
        case RP_X_SYNC_START:
        case RP_X_KEEP_ALIVE:
        case RP_X_HANDSHAKE:
            *aIsSyncEnd = ID_FALSE;
            IDE_TEST( applyXLogAndSendAck( aXLog ) != IDE_SUCCESS );
            enqueueFreeXLogQueue( aXLog );
            saveRestartSNAtRemoteMeta( mApply.mRestartSN );
            break;

        case RP_X_SYNC_END:
            *aIsSyncEnd = ID_TRUE;
            IDE_TEST( applyXLogAndSendAck( aXLog ) != IDE_SUCCESS );
            enqueueFreeXLogQueue( aXLog );

            saveRestartSNAtRemoteMeta( mApply.mRestartSN );
            break;

        case RP_X_FLUSH:
            *aIsSyncEnd = ID_TRUE;
            enqueueFreeXLogQueue( aXLog );
            break;
        default:
            *aIsSyncEnd = ID_TRUE;
            IDE_TEST( applyXLogAndSendAckInParallelAppiler( aXLog ) != IDE_SUCCESS );

            saveRestartSNAtRemoteMeta( mRestartSN );
            break;
    }

    /* second step */
    switch ( aXLog->mType )
    {
        case RP_X_REPL_STOP:
            ideLog::log( IDE_RP_0, RP_TRC_R_STOP_MSG_ARRIVED );

            shutdown();

            /* it does not have to wait applier thread */

            break;
            
        case RP_X_HANDSHAKE:
            IDE_TEST( handshakeWithoutReconnect( aXLog ) != IDE_SUCCESS );
            break;

        default:
            /* do nothing */
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::processXLogInParallelApplier( rpdXLog    * aXLog,
                                                 idBool     * aIsEnd )
{
    IDE_TEST( receiveAndConvertXLog( aXLog ) != IDE_SUCCESS );

    IDE_TEST( applyXLogAndSendAckInParallelAppiler( aXLog ) != IDE_SUCCESS );

    switch ( aXLog->mType )
    {
        case RP_X_REPL_STOP:
            ideLog::log( IDE_RP_0, RP_TRC_R_STOP_MSG_ARRIVED );

            mExitFlag = ID_TRUE;
            *aIsEnd = ID_TRUE;

            break;
            
        case RP_X_HANDSHAKE:
            if ( mStartMode == RP_RECEIVER_USING_TRANSFER )
            {
                IDE_TEST ( updateCurrentInfoForConsistentModeWithNewTransaction() != IDE_SUCCESS );
            }
            IDE_TEST( handshakeWithoutReconnect( aXLog ) != IDE_SUCCESS );
            break;

        case RP_X_KEEP_ALIVE:
            if ( mStartMode == RP_RECEIVER_USING_TRANSFER )
            {
                if ( updateCurrentInfoForConsistentModeWithNewTransaction() != IDE_SUCCESS )
                {
                    IDE_WARNING( IDE_RP_0, "[Consistent Receiver] Failed to update CurrentReadLSN" );
                }

                if ( ( aXLog->mSyncSN != SM_SN_NULL ) &&
                     ( mRemoteCheckpointSN == SM_SN_NULL || mRemoteCheckpointSN < aXLog->mSyncSN ) )
                {
                    mRemoteCheckpointSN = aXLog->mSyncSN;
                }
            }
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReceiver::processXLog( rpdXLog * aXLog, idBool *aIsEnd )
{
    IDE_TEST( receiveAndConvertXLog( aXLog ) != IDE_SUCCESS );

    IDE_TEST( applyXLogAndSendAck( aXLog ) != IDE_SUCCESS );

    switch ( aXLog->mType )
    {
        case RP_X_REPL_STOP:
            ideLog::log( IDE_RP_0, RP_TRC_R_STOP_MSG_ARRIVED );

            shutdown();
            *aIsEnd = ID_TRUE;
            break;
            
        case RP_X_HANDSHAKE:
            IDE_TEST( handshakeWithoutReconnect( aXLog ) != IDE_SUCCESS );
            break;

        case RP_X_DDL_REPLICATE_HANDSHAKE:
            IDE_TEST( rpcDDLASyncManager::ddlASynchronizationInternal( this ) 
                      != IDE_SUCCESS );
            break;
        
        case RP_X_SYNC_PK_END:
            if ( mStartMode == RP_RECEIVER_XLOGFILE_FAILBACK_MASTER )
            {
                IDE_TEST( failbackSlaveWithXLogfiles() != IDE_SUCCESS );
            }
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

/*
 *
 */
void rpxReceiver::saveRestartSNAtRemoteMeta( smSN aRestartSN )
{
    switch ( mStartMode )
    {
        case RP_RECEIVER_OFFLINE:
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            if ( ( mRemoteMeta != NULL ) && 
                 ( aRestartSN != SM_SN_NULL ) )
            {
                mRemoteMeta->mReplication.mXSN = aRestartSN;
            }
            else
            {
                /* nothing to do */
            }
            break;

        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
        default:
            break;
    }            
}

IDE_RC rpxReceiver::runNormal( void )
{
    idBool      sIsLocked = ID_FALSE;
    rpdXLog     sXLog;
    idBool      sIsInitializedXLog = ID_FALSE;
    idBool      sIsEnd = ID_FALSE;
    idBool      sIsLob = ID_FALSE;
    UInt        sMaxPkColCount = 0;

    sIsLob = isLobColumnExist();
    IDE_TEST( rpdQueue::initializeXLog( &sXLog,
                                        getBaseXLogBufferSize( &mMeta ),
                                        sIsLob,
                                        mAllocator )
              != IDE_SUCCESS );
    sIsInitializedXLog = ID_TRUE;

    sMaxPkColCount = mMeta.getMaxPkColCountInAllItem();
    if ( sMaxPkColCount == 0 )
    {
        sMaxPkColCount = 1;
    }

    IDE_TEST( mApply.allocRangeColumn( sMaxPkColCount ) != IDE_SUCCESS );

    while ( mExitFlag != ID_TRUE )
    {
        IDE_CLEAR();

        idvManager::applyOpTimeToSession( &mStatSession, &mStatistics );
        idvManager::initRPReceiverAccumTime( &mStatistics );

        IDE_TEST_RAISE( processXLog( &sXLog, &sIsEnd ) != IDE_SUCCESS,
                        recvXLog_error);

        saveRestartSNAtRemoteMeta( mApply.mRestartSN );

        if ( mErrorInfo.mErrorXSN < mApply.getApplyXSN() )
        {
            mErrorInfo.mErrorXSN = SM_SN_NULL;
            mErrorInfo.mErrorStopCount = 0;
        }
        else
        {
            /*nothing to do*/
        }

        rpdQueue::recycleXLog( &sXLog, mAllocator );
    }

    // Sender로부터 Replication 종료 메세지가 도착하지 않은 경우
    if ( sIsEnd != ID_TRUE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ALREADY_FINAL ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    sIsInitializedXLog = ID_FALSE;
    rpdQueue::destroyXLog( &sXLog, mAllocator );

    ideLog::log( IDE_RP_0, RP_TRC_R_LAST_PROCESSED_SN, mRepName, mApply.getApplyXSN() );

    return IDE_SUCCESS;

    IDE_EXCEPTION( recvXLog_error );
    {
        if ( isSync() == ID_FALSE )
        {
            IDE_SET( ideSetErrorCode( rpERR_ABORT_RECVXLOG_RUN, mRepName ) );
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* do nothing */
        }
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    if ( sIsInitializedXLog == ID_TRUE )
    {
        rpdQueue::destroyXLog( &sXLog, mAllocator );
    }
    else
    {
        /* do nothing */
    }

    ideLog::log( IDE_RP_0, RP_TRC_R_LAST_PROCESSED_SN, mRepName, mApply.getApplyXSN() );

    // Executor가 (1)Handshake를 하고 (2)Receiver Thread를 동작시킨다.
    // Sender 또는 Executor의 요청이 아니고 Network 오류가 아닌데도 Receiver Thread가 비정상 종료하면,
    // 복구 불가능한 데이터 불일치가 확산되는 것을 막기 위해,
    // Eager Receiver인지 확인하여 Server를 비정상 종료한다.
    if ( (mExitFlag != ID_TRUE) && (mNetworkError != ID_TRUE) )
    {
        mErrorInfo.mErrorStopCount++;
        mErrorInfo.mErrorXSN = mApply.getApplyXSN();

        if ( RPU_REPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT != 0 )
        {
            if ( ( isEagerReceiver() == ID_TRUE ) &&
                 ( mErrorInfo.mErrorStopCount >=
                   RPU_REPLICATION_EAGER_RECEIVER_MAX_ERROR_COUNT ) )
            {
                IDE_SET(ideSetErrorCode(rpERR_ABORT_END_THREAD, mRepName));
                IDE_ERRLOG(IDE_RP_0);

                ideLog::log(IDE_RP_0, "Fatal Stop!\n");
                ideLog::log(IDE_RP_0, RP_TRC_R_ERR_EAGER_ABNORMAL_STOP, mRepName);

                IDE_ASSERT(0);
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
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::runSync( void ) 
{
    rpdXLog     * sXLog = NULL;
    idBool        sIsSyncEnd = ID_FALSE;

    do  
    {
        IDE_TEST( dequeueFreeXLogQueue( &sXLog ) != IDE_SUCCESS );

        rpdQueue::recycleXLog( sXLog, mAllocator );

        IDE_TEST( processXLogInSync( sXLog,
                                     &sIsSyncEnd )
                  != IDE_SUCCESS );

    } while ( ( sIsSyncEnd == ID_FALSE ) &&
              ( mExitFlag != ID_TRUE ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::runParallelAppiler( void )
{
    rpdXLog   * sXLog = NULL;
    idBool      sIsEnd = ID_FALSE;

    IDE_TEST( allocRangeColumnInParallelAppiler() != IDE_SUCCESS );

    IDE_TEST( runSync() != IDE_SUCCESS );

    if ( mStartMode == RP_RECEIVER_USING_TRANSFER )
    {
        setNetworkResourcesToXLogTransfer();

        IDE_TEST ( processRemainXLogInXLogfile( ID_TRUE ) != IDE_SUCCESS );

        IDE_TEST( updateCurrentInfoForConsistentModeWithNewTransaction() != IDE_SUCCESS );

        IDE_TEST( startXLogTrasnsfer() != IDE_SUCCESS );
    }

    while ( mExitFlag != ID_TRUE )
    {
        IDE_CLEAR();

        idvManager::applyOpTimeToSession( &mStatSession, &mStatistics );
        idvManager::initRPReceiverAccumTime( &mStatistics );

        IDE_TEST( dequeueFreeXLogQueue( &sXLog ) != IDE_SUCCESS );

        rpdQueue::recycleXLog( sXLog, mAllocator );

        IDE_TEST_RAISE ( processXLogInParallelApplier( sXLog, &sIsEnd ) != IDE_SUCCESS,
                         recvXLog_error );

        saveRestartSNAtRemoteMeta( mRestartSN );
    }


    /*
     * consistent replication은 xlogfile에 남은 xlog를 다 처리 한 뒤 종료한다.
     */
    if ( mStartMode == RP_RECEIVER_USING_TRANSFER )
    {
        finalizeXLogTransfer();

        if ( isGottenNetworkResoucesFromXLogTransfer() == ID_TRUE )
        {
            setNetworkResourcesToXLogTransfer();
        }

        IDE_TEST ( processRemainXLogInXLogfile( ID_FALSE ) != IDE_SUCCESS );

        IDE_TEST( updateCurrentInfoForConsistentModeWithNewTransaction() != IDE_SUCCESS );
    }

    // Sender로부터 Replication 종료 메세지가 도착하지 않은 경우
    if ( sIsEnd != ID_TRUE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ALREADY_FINAL ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( recvXLog_error );
    {
        if ( isSync() == ID_FALSE )
        {
            IDE_SET( ideSetErrorCode( rpERR_ABORT_RECVXLOG_RUN, mRepName ) );
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* do nothing */
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::runRecoveryXlogfiles()
{
    IDE_DASSERT( mMeta.getReplMode() == RP_CONSISTENT_MODE );

    IDE_TEST( allocRangeColumnInParallelAppiler() != IDE_SUCCESS );

    setReadXLogfileMode();

    mXFRecoveryStatus = RPX_XF_RECOVERY_PROCESS;
    IDE_TEST ( processRemainXLogInXLogfile(ID_TRUE) != IDE_SUCCESS );

    if ( updateCurrentInfoForConsistentModeWithNewTransaction() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    IDE_TEST( checkAndSetXFRecoveryStatusEND( RPX_XF_RECOVERY_DONE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    (void)checkAndSetXFRecoveryStatusEND( RPX_XF_RECOVERY_ERROR );
    
    IDE_POP();

    return IDE_FAILURE;
}

/* call by receiver
 * receiver에서 xlogfile flush 종료 시 mXFRecoveryStatus 를 설정한다.
 * rpcManager에서 이미 종료되었다면 RPX_XF_RECOVERY_EXIT로 설정한다. */
IDE_RC rpxReceiver::checkAndSetXFRecoveryStatusEND( RPX_RECEIVER_XLOGFILE_RECOVERY_STATUS aStatus )
{
    idBool sIsLocked = ID_FALSE;

    IDE_ASSERT( mXFRecoveryWaitMutex.lock( &mStatistics ) == IDE_SUCCESS );
    sIsLocked = ID_TRUE;

    if ( mXFRecoveryStatus != RPX_XF_RECOVERY_TIMEOUT )
    {
        mXFRecoveryStatus = aStatus;
        IDE_TEST( mXFRecoveryWaitCV.signal() != IDE_SUCCESS );
    }
    else
    {
        mXFRecoveryStatus = RPX_XF_RECOVERY_EXIT;
    }     

    IDE_ASSERT( mXFRecoveryWaitMutex.unlock() == IDE_SUCCESS );
    sIsLocked = ID_FALSE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        IDE_ASSERT( mXFRecoveryWaitMutex.unlock() == IDE_SUCCESS );
    }
    return IDE_FAILURE;
}

/* call by executor (rpcManager)
 * receiver에서 xlogfile flush 종료되고 rpcManager에서 종료된다면 mXFRecoveryStatus 를 RPX_XF_RECOVERY_EXIT 로 설정한다.
 * receiver가 아직 종료되지 않았다면, 상태를 변경하지 않는다. */
void rpxReceiver::checkAndSetXFRecoveryStatusExit( )
{
    IDE_ASSERT( mXFRecoveryWaitMutex.lock( &mStatistics ) == IDE_SUCCESS );

    if ( mXFRecoveryStatus != RPX_XF_RECOVERY_TIMEOUT )
    {
        mXFRecoveryStatus = RPX_XF_RECOVERY_EXIT;
    }

    IDE_ASSERT( mXFRecoveryWaitMutex.unlock() == IDE_SUCCESS );
}

IDE_RC rpxReceiver::runFailoverUsingXLogFiles()
{
    iduList         sGlobalTxList;
    PDL_Time_Value  sTimeValue;

    IDE_DASSERT( mMeta.getReplMode() == RP_CONSISTENT_MODE );
    IDE_DASSERT( mXLogfileManager != NULL );

    setReadXLogfileMode();

    mXLogfileManager->setWaitWrittenXLog( ID_FALSE );

    IDU_LIST_INIT( &sGlobalTxList );
    if ( collectUnCompleteGlobalTxList( &sGlobalTxList ) != IDE_SUCCESS )
    {
        IDE_TEST( ideGetErrorCode() != rpERR_IGNORE_RPX_END_OF_XLOGFILES );
    }
    IDE_TEST_CONT( IDU_LIST_IS_EMPTY( &sGlobalTxList ) == ID_TRUE, NORMAL_EXIT );

    finalizeAndDestroyXLogfileManager();
    mXLogfileManager = NULL;

    IDE_TEST( initializeXLogfileContents() != IDE_SUCCESS );
    mReadContext = setReadContext( mProtocolContext, mXLogfileManager );
    setReadXLogfileMode();

    mXLogfileManager->setWaitWrittenXLog( ID_FALSE );

    if ( applyUnCompleteGlobalTxLog( &sGlobalTxList ) != IDE_SUCCESS )
    {
        IDE_TEST( ideGetErrorCode() != rpERR_IGNORE_RPX_END_OF_XLOGFILES );
    }

    if ( IDU_LIST_IS_EMPTY( &sGlobalTxList ) != ID_TRUE )
    {
        IDU_LIST_JOIN_LIST( &mGlobalTxList, &sGlobalTxList );
    }

    mIsFailoverStepEnd = ID_TRUE;

    sTimeValue.initialize( 0, 1000 );
    while( mExitFlag != ID_TRUE )
    {
        idlOS::sleep( sTimeValue );
    }

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    mIsFailoverStepEnd = ID_TRUE;

    return IDE_FAILURE;
}

void rpxReceiver::run()
{
    IDE_CLEAR();

    switch ( mStartMode )
    {
        case RP_RECEIVER_RECOVERY:
            ideLog::log( IDE_RP_0, RP_TRC_R_RECO_RECEIVER_START, mRepName );
            break;
        case RP_RECEIVER_SYNC_CONDITIONAL:
            ideLog::log( IDE_RP_0, RP_TRC_R_RECEIVER_START, mRepName );
            IDE_TEST( recoveryCondition( ID_TRUE ) != IDE_SUCCESS );
            break;
        case RP_RECEIVER_XLOGFILE_RECOVERY:
            ideLog::log( IDE_RP_0, RP_TRC_R_CONSISTENT_RECO_RECEIVER_START, mRepName );
            break;
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            ideLog::log( IDE_RP_0, RP_TRC_R_CONSISTENT_FAILOVER_RECEIVER_START, mRepName );
            break;
        default:
            ideLog::log( IDE_RP_0, RP_TRC_R_RECEIVER_START, mRepName );
            break;
    }

    mMeta.printItemActionInfo();

    // Handshake를 정상적으로 수행한 후, Receiver Thread가 동작한다.
    mNetworkError = ID_FALSE;
    setReadNetworkMode();

    switch ( mStartMode )
    {
        case RP_RECEIVER_SYNC:
            IDE_TEST( runNormal() != IDE_SUCCESS );
            break;

        case RP_RECEIVER_XLOGFILE_RECOVERY:
            IDE_TEST( runRecoveryXlogfiles() != IDE_SUCCESS );
            break;

        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            IDE_TEST( runNormal() != IDE_SUCCESS );
            break;

        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            IDE_TEST( runFailoverUsingXLogFiles() != IDE_SUCCESS );
            break;
        
        default:
            if ( mApplierCount == 0 )
            {
                IDE_TEST( runNormal() != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( runParallelAppiler() != IDE_SUCCESS );
            }
            break;
    }

    finalize();

    if ( isSync() == ID_FALSE )
    {
        ideLog::log(IDE_RP_0, RP_TRC_R_NORMAL_STOP );
        ideLog::log(IDE_RP_0, RP_TRC_R_RECEIVER_STOP, mRepName);
    }
    else
    {
        /* do nothing */
    }

    mIsRecoveryComplete = ID_TRUE;

    return;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    finalize();

    if ( isSync() == ID_FALSE )
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_END_THREAD, mRepName));
        IDE_ERRLOG(IDE_RP_0);

        ideLog::log(IDE_RP_0, "Error Stop!\n");
        ideLog::log(IDE_RP_0, RP_TRC_R_RECEIVER_STOP, mRepName);
    }
    else
    {
        /* do nothing */
    }

    mIsRecoveryComplete = ID_FALSE;

    return;
}

/*
 * @brief It updates offline restart information and copy Remote meta
 */
IDE_RC rpxReceiver::buildRemoteMeta( rpdMeta * aMeta )
{
    switch ( mStartMode )
    {
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            mRestartSN = aMeta->mReplication.mXSN;
            /* fall through */

        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_OFFLINE:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
            if ( mRemoteMeta != NULL )
            {
                IDE_TEST( aMeta->copyMeta( mRemoteMeta ) != IDE_SUCCESS );

                mRemoteMeta->mReplication.mXSN = mRestartSN;
            }
            else
            {
                /* do nothing */
            }
            break;

        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpxReceiver::checkSelfReplication( idBool    aIsLocalReplication,
                                          rpdMeta * aMeta )
{
    SChar sPeerIP[RP_IP_ADDR_LEN];

    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_RECOVERY:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            /* BUG-45236 Local Replication 지원
             *  Local Replication이면, 이후에 Table OID를 추가적으로 검사한다.
             */
            IDE_TEST_RAISE( ( idlOS::strcmp( aMeta->mReplication.mServerID,
                                             mMeta.mReplication.mServerID ) == 0 ) &&
                            ( aIsLocalReplication != ID_TRUE ),
                            ERR_SELF_REPLICATION );
            break;

        case RP_RECEIVER_OFFLINE:
            /* PROJ-1915 : Off-line Replicator는 Local Replication으로 접속한다. */
            break;
        case RP_RECEIVER_XLOGFILE_RECOVERY:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            break;
        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SELF_REPLICATION );
    {
        idlOS::memset( sPeerIP, 0x00, RP_IP_ADDR_LEN );
        (void)cmiGetLinkInfo( mLink,
                              sPeerIP,
                              RP_IP_ADDR_LEN,
                              CMI_LINK_INFO_LOCAL_IP_ADDRESS);

        idlOS::snprintf( mMeta.mErrMsg, RP_ACK_MSG_LEN,
                         "The self replication case has been detected."
                         " (Peer=%s:%"ID_UINT32_FMT")",
                         sPeerIP,
                         RPU_REPLICATION_PORT_NO );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_REPLICATION_SELF_REPLICATION,
                                  sPeerIP,
                                  RPU_REPLICATION_PORT_NO ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE; 
}

/*
 * @brief It checks given remote meta by using local meta
 */
IDE_RC rpxReceiver::checkMeta( smiTrans         * aTrans,
                               rpdMeta          * aRemoteMeta )
{
    UInt    sSqlApplyEnable         = RPU_REPLICATION_SQL_APPLY_ENABLE;
    UInt    sItemCountDiffEnable    = RPU_REPLICATION_META_ITEM_COUNT_DIFF_ENABLE;
    idBool  sIsLocalReplication     = ID_FALSE;
    
    smiStatement    sStatement;
    idBool          sIsBegin = ID_FALSE;

    /* BUG-45236 Local Replication 지원
     *  Receiver에서 Meta를 비교하므로, Sender의 Meta를 사용하여 Local Replication인지 확인한다.
     */
    sIsLocalReplication = rpdMeta::isLocalReplication( aRemoteMeta );

    IDE_TEST( checkSelfReplication( sIsLocalReplication, aRemoteMeta ) != IDE_SUCCESS );

    IDE_TEST( sStatement.begin( aTrans->getStatistics(),
                                aTrans->getStatement(),
                                SMI_STATEMENT_NORMAL |
                                SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    IDE_TEST_RAISE( rpdMeta::equals( &sStatement,
                                     sIsLocalReplication,
                                     sSqlApplyEnable,
                                     sItemCountDiffEnable,
                                     aRemoteMeta,
                                     &mMeta ) 
                    != IDE_SUCCESS, ERR_META_COMPARE );

    sIsBegin = ID_FALSE;
    IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    mMeta.setSqlApplyTableCount();
    
    /* check offline replication log info */
    if ( ( mMeta.mReplication.mOptions & RP_OPTION_OFFLINE_MASK ) ==
         RP_OPTION_OFFLINE_SET )
    {
        IDE_TEST_RAISE( checkOfflineReplAvailable( aRemoteMeta ) != IDE_SUCCESS,
                        ERR_CANNOT_OFFLINE );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_COMPARE );
    {
        IDE_ERRLOG( IDE_RP_0 );

        idlOS::snprintf( mMeta.mErrMsg,
                         RP_ACK_MSG_LEN,
                         "%s",
                         ideGetErrorMsg( ideGetErrorCode() ) );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_META_MISMATCH ) );
    }
    IDE_EXCEPTION( ERR_CANNOT_OFFLINE )
    {
        idlOS::snprintf( mMeta.mErrMsg, RP_ACK_MSG_LEN,
                         "Offline log information mismatch." );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)sStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 * @brief It compares remote and local endian, then decide endian conversion
 */
void rpxReceiver::decideEndianConversion( rpdMeta * aRemoteMeta )
{
    if ( rpdMeta::getReplFlagEndian( &aRemoteMeta->mReplication )
         != rpdMeta::getReplFlagEndian( &mMeta.mReplication ) )
    {
        mEndianDiff = ID_TRUE;
    }
    else
    {
        mEndianDiff = ID_FALSE;
    }
}

/*
 * @brief It sends handshake ack with failback status
 */
IDE_RC rpxReceiver::sendHandshakeAckWithFailbackStatus( rpdMeta * aMeta )
{
    SInt        sFailbackStatus = RP_FAILBACK_NONE;

    sFailbackStatus = decideFailbackStatus( aMeta );
    
    IDU_FIT_POINT_RAISE( "rpxReceiver::sendHandshakeAckWithFailbackStatus::Erratic::rpERR_ABORT_SEND_ACK",
                         ERR_SEND_ACK );

    IDE_TEST_RAISE( rpnComm::sendHandshakeAck( mProtocolContext,
                                               &mExitFlag,
                                               RP_MSG_OK,
                                               sFailbackStatus,
                                               mRestartSN,
                                               mMeta.mErrMsg,
                                               RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT )
                    != IDE_SUCCESS, ERR_SEND_ACK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEND_ACK );
    {
        if ( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
             ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) ||
             ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
             ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
             ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) )
        {
            mNetworkError = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        IDE_SET(ideSetErrorCode(rpERR_ABORT_SEND_ACK));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::checkConditionAndSendResult( )
{
     rpdConditionItemInfo *sRecvConditionInfo = NULL;
     UInt                  sRecvConditionCnt = 0;
    
     rpdConditionItemInfo *sMyConditionInfo = NULL;
    
     rpdConditionActInfo  *sConditionAct = NULL;
    
     smiTrans              sTrans;
     smiStatement        * spRootStmt = NULL;
     smiStatement          sSmiStmt;
     SInt                  sStage = 0;
     UInt                  sFlag = 0;
     SChar                 sErrMsg[RP_ACK_MSG_LEN];

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkConditionAndSendResult::calloc::sMyConditionInfo",
                           ERR_MEMORY_ALLOC_CONDITION_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       mMeta.mReplication.mItemCount,
                                       ID_SIZEOF(rpdConditionItemInfo),
                                       (void **)&sMyConditionInfo,
                                       IDU_MEM_IMMEDIATE)
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CONDITION_LIST );

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkConditionAndSendResult::calloc::sRecvConditionInfo",
                           ERR_MEMORY_ALLOC_RECEIVE_CONDITION_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       mMeta.mReplication.mItemCount,
                                       ID_SIZEOF(rpdConditionItemInfo),
                                       (void **)&sRecvConditionInfo,
                                       IDU_MEM_IMMEDIATE)
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_RECEIVE_CONDITION_LIST );

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkConditionAndSendResult::calloc::sConditionAct",
                           ERR_MEMORY_ALLOC_CONDITION_ACTION_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       mMeta.mReplication.mItemCount,
                                       ID_SIZEOF(rpdConditionActInfo),
                                       (void **)&sConditionAct,
                                       IDU_MEM_IMMEDIATE)
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CONDITION_ACTION_LIST );


    IDE_TEST( rpnComm::recvConditionInfo( mProtocolContext,
                                          &mExitFlag,
                                          sRecvConditionInfo,
                                          &sRecvConditionCnt,
                                          RPU_REPLICATION_SENDER_SEND_TIMEOUT ) != IDE_SUCCESS );

    IDE_TEST_RAISE( (UInt)mMeta.mReplication.mItemCount != sRecvConditionCnt,
                    ERR_RECV_CONDITION_INFO_ITEM_COUNT_MISMATCH );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    sFlag = ( sFlag & ~SMI_ISOLATION_MASK ) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = ( sFlag & ~SMI_TRANSACTION_MASK ) | SMI_TRANSACTION_NORMAL;
    sFlag = ( sFlag & ~SMI_TRANSACTION_REPL_MASK ) | SMI_TRANSACTION_REPL_NONE;
    sFlag = ( sFlag & ~SMI_COMMIT_WRITE_MASK ) | SMI_COMMIT_WRITE_NOWAIT;

    IDE_TEST( sTrans.begin( &spRootStmt, &mStatistics, sFlag, RP_UNUSED_RECEIVER_INDEX )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( sTrans.getStatistics(),
                              spRootStmt,
                              SMI_STATEMENT_NORMAL |
                              SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( rpdMeta::makeConditionInfoWithItems( &sSmiStmt,
                                                   mMeta.mReplication.mItemCount,
                                                   mMeta.mItemsOrderByRemoteTableOID,
                                                   sMyConditionInfo )
              != IDE_SUCCESS );

    rpdMeta::compareCondition( sRecvConditionInfo,
                               sMyConditionInfo,
                               mMeta.mReplication.mItemCount,
                               sConditionAct );

    IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS)
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy(NULL) != IDE_SUCCESS );
    IDE_TEST( rpnComm::sendConditionInfoResult( mProtocolContext,
                                                &mExitFlag,
                                                sConditionAct,
                                                mMeta.mReplication.mItemCount,
                                                RPU_REPLICATION_SENDER_SEND_TIMEOUT ) != IDE_SUCCESS );

    (void)iduMemMgr::free( sConditionAct );
    sConditionAct = NULL;

    (void)iduMemMgr::free( sRecvConditionInfo );
    sRecvConditionInfo = NULL;

    (void)iduMemMgr::free( sMyConditionInfo );
    sMyConditionInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CONDITION_LIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::checkConditionAndSendResult",
                                  "sMyConditionList" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RECEIVE_CONDITION_LIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::checkConditionAndSendResult",
                                  "sRecvConditionInfo" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CONDITION_ACTION_LIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::checkConditionAndSendResult",
                                  "sConditionAct" ) );
    }
    IDE_EXCEPTION( ERR_RECV_CONDITION_INFO_ITEM_COUNT_MISMATCH )
    {

        idlOS::snprintf( sErrMsg, RP_ACK_MSG_LEN,
                         "The number of items for conditional action received does not match."
                         "Item count :%"ID_UINT32_FMT" , Received item count :%"ID_UINT32_FMT,
                         mMeta.mReplication.mItemCount,
                         sRecvConditionCnt );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch ( sStage )
    {
        case 3:
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
        case 1:
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    if ( sConditionAct != NULL )
    {
        (void)iduMemMgr::free( sConditionAct );
        sConditionAct = NULL;
    }

    if ( sRecvConditionInfo != NULL )
    {
        (void)iduMemMgr::free( sRecvConditionInfo );
        sRecvConditionInfo = NULL;
    }

    if ( sMyConditionInfo != NULL )
    {
        (void)iduMemMgr::free( sMyConditionInfo );
        sMyConditionInfo = NULL;
    }

    IDE_POP();
    
    return IDE_FAILURE; 
}

IDE_RC rpxReceiver::copyNewMeta()
{
    if( mNewMeta != NULL )
    {
        IDE_TEST( mNewMeta->copyMeta( &mMeta ) != IDE_SUCCESS );

        mNewMeta->finalize();
        (void)iduMemMgr::free( mNewMeta );
        mNewMeta = NULL;

        ideLog::log( IDE_RP_0, "[Receiver] New receiver meta copy success <%s>", mRepName );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[Receiver] New receiver meta copy failure <%s>", mRepName );

    return IDE_FAILURE; 
}

/*
 * @brief It analyzes remote meta and send handshake ack as a result
 */
IDE_RC rpxReceiver::processMetaAndSendHandshakeAck( smiTrans        * aTrans,
                                                    rpdMeta         * aMeta )
{
    IDE_TEST( copyNewMeta() != IDE_SUCCESS );

    IDE_TEST( checkMeta( aTrans, aMeta ) != IDE_SUCCESS );

    IDE_TEST( buildRemoteMeta( aMeta ) != IDE_SUCCESS );

    decideEndianConversion( aMeta );

    IDE_TEST( sendHandshakeAckWithFailbackStatus( aMeta ) != IDE_SUCCESS );

    if ( rpdMeta::needToProcessProtocolOperation( RP_META_COMPRESSTYPE,
                                                  aMeta->mReplication.mRemoteVersion )
         == ID_TRUE )
    {
        cmiSetDecompressType( mProtocolContext, aMeta->mReplication.mCompressType );
        ideLog::log( IDE_RP_0, "[Receiver] RepName : %s DecompressType : %"ID_UINT32_FMT, aMeta->mReplication.mRepName, aMeta->mReplication.mCompressType );
    }
    
    if( ( rpdMeta::isRpSyncCondition( &(aMeta->mReplication) ) == ID_TRUE ) || 
        ( rpdMeta::isRpStartCondition( &(aMeta->mReplication) ) == ID_TRUE ) )
    {
        if ( aMeta->mReplication.mItemCount != 0 )
        {
            IDE_TEST( checkConditionAndSendResult( ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(ideGetErrorCode() != rpERR_ABORT_SEND_ACK)
    {
        (void)rpnComm::sendHandshakeAck( mProtocolContext,
                                         &mExitFlag,
                                         RP_MSG_META_DIFF,
                                         RP_FAILBACK_NONE,
                                         SM_SN_NULL,
                                         mMeta.mErrMsg,
                                         RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT );
    }

    IDE_POP();

    IDE_ERRLOG(IDE_RP_0);
             
    return IDE_FAILURE;
}

void rpxReceiver::setTcpInfo()
{
    SInt sIndex;
    SChar sPort[RP_PORT_LEN];

    idlOS::memset( mMyIP, 0x00, RP_IP_ADDR_LEN );
    idlOS::memset( mPeerIP, 0x00, RP_IP_ADDR_LEN );

    mMyPort = 0;
    mPeerPort = 0;

    if( rpnComm::isConnected( mLink ) != ID_TRUE )
    {
        // Connection Closed
        //----------------------------------------------------------------//
        // get my host ip & port
        //----------------------------------------------------------------//
        idlOS::snprintf(mMyIP, RP_IP_ADDR_LEN, "127.0.0.1");
        mMyPort = RPU_REPLICATION_PORT_NO;

        //----------------------------------------------------------------//
        // get peer host ip & port
        //----------------------------------------------------------------//
        if( rpdCatalog::getIndexByAddr( mMeta.mReplication.mLastUsedHostNo,
                                     mMeta.mReplication.mReplHosts,
                                     mMeta.mReplication.mHostCount,
                                     &sIndex ) == IDE_SUCCESS )
        {
            idlOS::sprintf( mPeerIP,
                            "%s",
                            mMeta.mReplication.mReplHosts[sIndex].mHostIp );
            mPeerPort = mMeta.mReplication.mReplHosts[sIndex].mPortNo;
        }
        else
        {
            idlOS::memcpy( mPeerIP, "127.0.0.1", 10 );
            mPeerPort = RPU_REPLICATION_PORT_NO;
        }
    }
    else
    {
        // Connection Established
        (void)cmiGetLinkInfo(mLink,
                             mMyIP,
                             RP_IP_ADDR_LEN,
                             CMI_LINK_INFO_LOCAL_IP_ADDRESS);
        (void)cmiGetLinkInfo(mLink,
                             mPeerIP,
                             RP_IP_ADDR_LEN,
                             CMI_LINK_INFO_REMOTE_IP_ADDRESS);

        // BUG-15944
        idlOS::memset(sPort, 0x00, RP_PORT_LEN);
        (void)cmiGetLinkInfo(mLink,
                             sPort,
                             RP_PORT_LEN,
                             CMI_LINK_INFO_LOCAL_PORT);
        mMyPort = idlOS::atoi(sPort);
        idlOS::memset(sPort, 0x00, RP_PORT_LEN);
        (void)cmiGetLinkInfo(mLink,
                             sPort,
                             RP_PORT_LEN,
                             CMI_LINK_INFO_REMOTE_PORT);
        mPeerPort = idlOS::atoi(sPort);
    }
}

IDE_RC rpxReceiver::convertEndian(rpdXLog *aXLog)
{
    RP_CREATE_FLAG_VARIABLE(IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN);

    RP_OPTIMIZE_TIME_BEGIN(&mStatistics, IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN);

    switch(aXLog->mType)
    {
        case RP_X_INSERT:
        case RP_X_SYNC_INSERT:
            IDE_TEST(convertEndianInsert(aXLog) != IDE_SUCCESS);
            break;

        case RP_X_UPDATE:
            IDE_TEST(convertEndianUpdate(aXLog) != IDE_SUCCESS);
            break;

        case RP_X_DELETE:
        case RP_X_LOB_CURSOR_OPEN:                //BUG-24418
        case RP_X_SYNC_PK:
            IDE_TEST(convertEndianPK(aXLog) != IDE_SUCCESS);
            break;

        default:
            break;
    }

    RP_OPTIMIZE_TIME_END(&mStatistics, IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    RP_OPTIMIZE_TIME_END(&mStatistics, IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN);

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::convertEndianInsert(rpdXLog *aXLog)
{
    return convertEndianInsert( &mMeta, aXLog ); 
}

IDE_RC rpxReceiver::convertEndianInsert( rpdMeta * aMeta, rpdXLog *aXLog)
{
    UInt               i;
    rpdMetaItem       *sItem = NULL;
    rpdColumn         *sColumn = NULL;

    (void)aMeta->searchRemoteTable(&sItem, aXLog->mTableOID);
    IDE_TEST_RAISE(sItem == NULL, ERR_NOT_FOUND_TABLE);

    for(i = 0; i < aXLog->mColCnt; i ++)
    {
        sColumn = sItem->getRpdColumn(i);
        IDE_TEST_RAISE(sColumn == NULL, ERR_NOT_FOUND_COLUMN);

        IDE_TEST( checkAndConvertEndian( (void *)aXLog->mACols[i].value, 
                                         sColumn->mColumn.type.dataTypeId,
                                         sColumn->mColumn.column.flag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_COLUMN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_COLUMN,
                                aXLog->mSN,
                                aXLog->mTableOID,
                                i));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpxReceiver::convertEndianUpdate(rpdXLog *aXLog)
{
    UInt               i;
    rpdMetaItem       *sItem = NULL;
    rpdColumn         *sColumn = NULL;

    (void)mMeta.searchRemoteTable(&sItem, aXLog->mTableOID);
    IDE_TEST_RAISE(sItem == NULL, ERR_NOT_FOUND_TABLE);

    for(i = 0; i < aXLog->mPKColCnt; i ++)
    {
        sColumn = sItem->getPKRpdColumn(i);

        IDU_FIT_POINT_RAISE( "rpxReceiver::convertEndianUpdate::Erratic::rpERR_ABORT_RECEIVER_NOT_FOUND_PK_COLUMN",
                             ERR_NOT_FOUND_PK_COLUMN ); 
        IDE_TEST_RAISE(sColumn == NULL, ERR_NOT_FOUND_PK_COLUMN);

        IDE_TEST( checkAndConvertEndian( (void *)aXLog->mPKCols[i].value, 
                                         sColumn->mColumn.type.dataTypeId,
                                         sColumn->mColumn.column.flag )
                  != IDE_SUCCESS );
    }

    for(i = 0; i < aXLog->mColCnt; i++)
    {
        sColumn = sItem->getRpdColumn(aXLog->mCIDs[i]);
        IDE_TEST_RAISE(sColumn == NULL, ERR_NOT_FOUND_COLUMN);

        IDE_TEST( checkAndConvertEndian( (void *)aXLog->mACols[i].value, 
                                         sColumn->mColumn.type.dataTypeId,
                                         sColumn->mColumn.column.flag )
                  != IDE_SUCCESS );
        IDE_TEST( checkAndConvertEndian( (void *)aXLog->mBCols[i].value, 
                                         sColumn->mColumn.type.dataTypeId,
                                         sColumn->mColumn.column.flag )
                  != IDE_SUCCESS );        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_PK_COLUMN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_PK_COLUMN,
                                aXLog->mSN,
                                aXLog->mTableOID,
                                i));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_COLUMN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_COLUMN,
                                aXLog->mSN,
                                aXLog->mTableOID,
                                aXLog->mCIDs[i]));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::convertEndianPK(rpdXLog *aXLog)
{
    UInt               i;
    rpdMetaItem       *sItem = NULL;
    rpdColumn         *sColumn = NULL;

    (void)mMeta.searchRemoteTable(&sItem, aXLog->mTableOID);
    IDE_TEST_RAISE(sItem == NULL, ERR_NOT_FOUND_TABLE);

    for(i = 0; i < aXLog->mPKColCnt; i ++)
    {
        sColumn = sItem->getPKRpdColumn(i);

        IDU_FIT_POINT_RAISE( "rpxReceiver::convertEndianPK::Erratic::rpERR_ABORT_RECEIVER_NOT_FOUND_PK_COLUMN",
                             ERR_NOT_FOUND_PK_COLUMN ); 
        IDE_TEST_RAISE(sColumn == NULL, ERR_NOT_FOUND_PK_COLUMN);

        IDE_TEST( checkAndConvertEndian( (void *)aXLog->mPKCols[i].value, 
                                         sColumn->mColumn.type.dataTypeId,
                                         sColumn->mColumn.column.flag )
                  != IDE_SUCCESS );     
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_TABLE);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                aXLog->mSN,
                                aXLog->mTableOID));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_PK_COLUMN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RECEIVER_NOT_FOUND_PK_COLUMN,
                                aXLog->mSN,
                                aXLog->mTableOID,
                                i));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::checkAndConvertEndian( void    *aValue,
                                           UInt     aDataTypeId,
                                           UInt     aFlag )
{
    const mtdModule   *sMtd = NULL;    

    if ( ( aValue != NULL ) && 
         ( ( aFlag & SMI_COLUMN_TYPE_MASK ) != SMI_COLUMN_TYPE_LOB ) )
    {
        IDE_TEST_RAISE( mtd::moduleById(&sMtd, aDataTypeId)
                       != IDE_SUCCESS, ERR_GET_MODULE );
        sMtd->endian( aValue );
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_GET_MODULE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_GET_MODULE ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiver::setSNMapMgr(rprSNMapMgr* aSNMapMgr)
{
    mSNMapMgr = aSNMapMgr;

    mApply.setSNMapMgr( aSNMapMgr );

    return;
}

IDE_RC
rpxReceiver::waitThreadJoin(idvSQL *aStatistics)
{
    PDL_Time_Value sTvCpu;
    PDL_Time_Value sPDL_Time_Value;
    idBool         sIsLock = ID_FALSE;
    UInt           sTimeoutMilliSec = 0;    

    IDE_ASSERT(threadJoinMutex_lock() == IDE_SUCCESS);
    sIsLock = ID_TRUE;
    
    sPDL_Time_Value.initialize(0, 1000);

    while(mIsThreadDead != ID_TRUE)
    {
        sTvCpu  = idlOS::gettimeofday();
        sTvCpu += sPDL_Time_Value; 

        (void)mThreadJoinCV.timedwait(&mThreadJoinMutex, &sTvCpu);

        if ( aStatistics != NULL )
        {
            // BUG-22637 MM에서 QUERY_TIMEOUT, Session Closed를 설정했는지 확인
            IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
        }
        else
        {
            // 5 Sec
            IDE_TEST_RAISE( sTimeoutMilliSec >= 5000, ERR_TIMEOUT );
            sTimeoutMilliSec++;
        }
    }

    sIsLock = ID_FALSE;
    IDE_ASSERT(threadJoinMutex_unlock() == IDE_SUCCESS);

    if(join() != IDE_SUCCESS)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_JOIN_THREAD));
        IDE_ERRLOG(IDE_RP_0);
        IDE_CALLBACK_FATAL("[Repl Receiver] Thread join error");
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TIMEOUT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, 
                                  "rpxReceiver::waitThreadJoin exceeds timeout" ) );
    }
    IDE_EXCEPTION_END;

    if(sIsLock == ID_TRUE)
    {
        IDE_ASSERT(threadJoinMutex_unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

void rpxReceiver::signalThreadJoin()
{
    IDE_ASSERT(threadJoinMutex_lock() == IDE_SUCCESS);

    mIsThreadDead = ID_TRUE;
    IDE_ASSERT(mThreadJoinCV.signal() == IDE_SUCCESS);

    IDE_ASSERT(threadJoinMutex_unlock() == IDE_SUCCESS);

    return;
}

SInt rpxReceiver::decideFailbackStatus(rpdMeta * aPeerMeta)
{
    SInt sFailbackStatus = RP_FAILBACK_NONE;
    SInt sCompare;

    IDE_DASSERT(aPeerMeta != NULL);

    // Eager Replication에만 Failback을 적용한다.
    if((aPeerMeta->mReplication.mReplMode == RP_EAGER_MODE) &&
       (mMeta.mReplication.mReplMode == RP_EAGER_MODE))
    {
        /* 이전 상태가 둘 중 하나라도 Stop이면, Failback-Normal이다.
         * REPLICATION_FAILBACK_INCREMENTAL_SYNC = 0 이면, Failback-Normal이다.
         */
        if((aPeerMeta->mReplication.mIsStarted == 0) ||
           (mMeta.mReplication.mIsStarted == 0) ||
           (RPU_REPLICATION_FAILBACK_INCREMENTAL_SYNC != 1))
        {
            sFailbackStatus = RP_FAILBACK_NORMAL;
        }
        else
        {
            // Receiver가 Failback 상태를 결정하므로, Sender를 기준으로
            // Receiver보다 Remote Fault Detect Time이 늦으면, Failback-Master이다.
            // Receiver보다 Remote Fault Detect Time이 이르면, Failback-Slave이다.
            sCompare = idlOS::strncmp(aPeerMeta->mReplication.mRemoteFaultDetectTime,
                                      mMeta.mReplication.mRemoteFaultDetectTime,
                                      RP_DEFAULT_DATE_FORMAT_LEN);
            if(sCompare > 0)
            {
                sFailbackStatus = RP_FAILBACK_MASTER;
            }
            else if(sCompare < 0)
            {
                sFailbackStatus = RP_FAILBACK_SLAVE;
            }
            else
            {
                // 장애 감지 시간까지 같은 경우, Sender를 기준으로
                // Receiver보다 Server ID가 크면, Failback-Master이다.
                // Receiver보다 Server ID가 작으면, Failback-Slave이다.
                sCompare = idlOS::strncmp(aPeerMeta->mReplication.mServerID,
                                          mMeta.mReplication.mServerID,
                                          IDU_SYSTEM_INFO_LENGTH);
                if(sCompare > 0)
                {
                    sFailbackStatus = RP_FAILBACK_MASTER;
                }
                else if(sCompare < 0)
                {
                    sFailbackStatus = RP_FAILBACK_SLAVE;
                }
                else
                {
                    // Self-Replication
                }
            }

            /* Startup 단계에서만 Incremetal Sync를 허용한다. */
            if ( sFailbackStatus == RP_FAILBACK_MASTER )
            {
                if ( rpcManager::isStartupFailback() != ID_TRUE )
                {
                    sFailbackStatus = RP_FAILBACK_NORMAL;
                }
                else
                {
                    // Nothing to do
                }
            }
            else if ( sFailbackStatus == RP_FAILBACK_SLAVE )
            {
                if ( rpdMeta::isRpFailbackServerStartup(
                            &(aPeerMeta->mReplication) )
                     != ID_TRUE )
                {
                    sFailbackStatus = RP_FAILBACK_NORMAL;
                }
                else
                {
                    // Nothing to do
                }
            }
            else
            {
                // Nothing to do
            }
        }
    }
    else
    {
        // Nothing to do
    }

    return sFailbackStatus;
}

/*******************************************************************************
 * Description : 핸드쉐이크시에 오프라인 리플리케이터 옵션이 있다면
 * 다음과 같은 정보를 검사한다.
 * LFG count , Compile bit, SM version, OS info, Log file size
 * 디렉토리 및 파일 존재 검사를 하지 않는다.
 * 핸드쉐이크시에는 오프라인 옵션으로 아직 정해지지 않은 경로를 설정 할수있다.
 * 장애 발생후 마운트 또는 ftp 를 통한 오프라인 로그 접근 하는 경로가 변경 될수 있기 때문에
 * 디렉토리 및 파일 존재 검사는 오프라인 센더 구동시에 하고 핸드쉐이크시에는 하지 않는다.
 ******************************************************************************/
IDE_RC rpxReceiver::checkOfflineReplAvailable(rpdMeta  * aMeta)
{
    UInt   sCompileBit;
    SChar  sOSInfo[QC_MAX_NAME_LEN + 1];
    UInt   sSmVersionID;
    UInt   sSmVer1;
    UInt   sSmVer2;

#ifdef COMPILE_64BIT
    sCompileBit = 64;
#else
    sCompileBit = 32;
#endif

    sSmVersionID = smiGetSmVersionID();

    idlOS::snprintf(sOSInfo,
                    ID_SIZEOF(sOSInfo),
                    "%s %"ID_INT32_FMT" %"ID_INT32_FMT"",
                    OS_TARGET,
                    OS_MAJORVER,
                    OS_MINORVER);

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkOfflineReplAvailable::Erratic::rpERR_ABORT_MISMATCH_OFFLINE_LOG_LFGCOUNT",
                         ERR_LFGCOUNT_MISMATCH );
    IDE_TEST_RAISE(1 != aMeta->mReplication.mLFGCount,//[TASK-6757]LFG,SN 제거
                   ERR_LFGCOUNT_MISMATCH);

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkOfflineReplAvailable::Erratic::rpERR_ABORT_MISMATCH_OFFLINE_LOG_COMPILEBIT",
                         ERR_COMPILEBIT_MISMATCH ); 
    IDE_TEST_RAISE(sCompileBit != aMeta->mReplication.mCompileBit,
                   ERR_COMPILEBIT_MISMATCH);

    //sm Version 은 마스크 해서 검사 한다.
    sSmVer1 = sSmVersionID & SM_CHECK_VERSION_MASK;
    sSmVer2 = aMeta->mReplication.mSmVersionID & SM_CHECK_VERSION_MASK;

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkOfflineReplAvailable::Erratic::rpERR_ABORT_MISMATCH_OFFLINE_LOG_SMVERSION",
                         ERR_SMVERSION_MISMATCH );
    IDE_TEST_RAISE(sSmVer1 != sSmVer2,
                   ERR_SMVERSION_MISMATCH);

    IDU_FIT_POINT_RAISE( "rpxReceiver::checkOfflineReplAvailable::Erratic::rpERR_ABORT_MISMATCH_OFFLINE_LOG_OSVERSION",
                         ERR_OSVERSION_MISMATCH );
    IDE_TEST_RAISE(idlOS::strcmp(sOSInfo, aMeta->mReplication.mOSInfo) != 0,
                   ERR_OSVERSION_MISMATCH);

    IDE_TEST_RAISE(smiGetLogFileSize() != aMeta->mReplication.mLogFileSize,
                   ERR_LOGFILESIZE_MISMATCH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_LFGCOUNT_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MISMATCH_OFFLINE_LOG_LFGCOUNT));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_OSVERSION_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MISMATCH_OFFLINE_LOG_OSVERSION));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_SMVERSION_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MISMATCH_OFFLINE_LOG_SMVERSION));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_COMPILEBIT_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MISMATCH_OFFLINE_LOG_COMPILEBIT));
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION(ERR_LOGFILESIZE_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MISMATCH_OFFLINE_LOG_FILESIZE));
        IDE_ERRLOG(IDE_RP_0);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
* @breif XLog에서 재사용할 버퍼의 크기를 구한다.
*
* 버퍼 공간이 부족하여 확장한 크기는 재사용하지 않는다.
*
* @return XLog에서 재사용할 버퍼의 크기
*/
ULong rpxReceiver::getBaseXLogBufferSize( rpdMeta * aMeta )
{
    ULong   sBufferSize = 0;
    ULong   sMax        = idlOS::align8( RP_SAVEPOINT_NAME_LEN + 1 );
    SInt    sIndex      = 0;

    /* 기본적으로 할당할 공간에서 Geometry(최대 100MB)와 LOB(최대 4GB)은 제외한다. */
    for( sIndex = 0; sIndex < aMeta->mReplication.mItemCount; sIndex++ )
    {
        sMax = IDL_MAX( sMax,
                        aMeta->mItems[sIndex].getTotalColumnSizeExceptGeometryAndLob() );
    }

    sBufferSize += sMax;

    /* 버퍼 크기에 Header가 포함되어 있으므로, Header 크기를 더한다. */
    sBufferSize += idlOS::align8( ID_SIZEOF(iduMemoryHeader) );

    return sBufferSize;
}

idBool rpxReceiver::isLobColumnExist()
{
    return mMeta.isLobColumnExist();
}

/*
 * @brief given ACK EAGER message is sent
 */
IDE_RC rpxReceiver::sendAckWithTID( rpXLogAck * aAck )
{
    IDE_TEST( rpnComm::sendAckEager( mProtocolContext, 
                                     &mExitFlag, 
                                     aAck,
                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
              != IDE_SUCCESS )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
         ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) ||
         ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
         ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
         ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) )
    {
        mNetworkError = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/*
 * @brief given ACK message is sent
 */
IDE_RC rpxReceiver::sendAck( rpXLogAck * aAck )
{
    idBool sIsGotNetworkResources = ID_FALSE;

    if( mStartMode == RP_RECEIVER_USING_TRANSFER )
    {
        if ( isGottenNetworkResoucesFromXLogTransfer() == ID_FALSE )
        {
            getNetworkResourcesFromXLogTransfer();
            sIsGotNetworkResources = ID_TRUE;
        }
    }

    IDE_TEST( rpnComm::sendAck( mProtocolContext, 
                                &mExitFlag, 
                                aAck,
                                RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    if( sIsGotNetworkResources == ID_TRUE )
    {
        setNetworkResourcesToXLogTransfer();
        sIsGotNetworkResources = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
         ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) ||
         ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
         ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
         ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) )
    {
        mNetworkError = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }

    if( sIsGotNetworkResources == ID_TRUE )
    {
        setNetworkResourcesToXLogTransfer();
    }

    return IDE_FAILURE;
}

/*
 *
 */
smSN rpxReceiver::getApplyXSN( void )
{
    return mApply.getApplyXSN();
}

/*
 *
 */
IDE_RC rpxReceiver::searchRemoteTable( rpdMetaItem ** aItem, ULong aRemoteTableOID )
{
    IDE_TEST( mMeta.searchRemoteTable( aItem, aRemoteTableOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::searchTableFromRemoteMeta( rpdMetaItem ** aItem, 
                                               ULong          aTableOID )
{
    rpdMetaItem     * sItem = NULL;
    SChar             sErrorMessage[128] = { 0, };

    IDE_DASSERT( mRemoteMeta != NULL );

    IDE_TEST( mRemoteMeta->searchTable( &sItem,
                                        aTableOID ) 
              != IDE_SUCCESS);

    IDU_FIT_POINT_RAISE( "rpxReceiver::searchTableFromRemoteMeta::aItem",
                         ERR_NOT_EXIST_TABLE );
    IDE_TEST_RAISE( sItem == NULL, ERR_NOT_EXIST_TABLE );

    *aItem = sItem;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE );
    {

        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "Table(OID : %"ID_UINT64_FMT") does not exit in remote meta",
                         aTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  sErrorMessage ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 이 함수를 호출 한 곳에서 aRemoteTable에 대한 메모리 해제를 해주어야한다.
 */
IDE_RC rpxReceiver::recvSyncTablesInfo( UInt         * aSyncTableNumber,
                                        rpdMetaItem ** aRemoteTable )
{
    UInt i;
    rpdMetaItem *sRemoteTable = NULL;

    IDE_ASSERT( *aRemoteTable == NULL );

    /* Sync할 테이블의 갯수를 받아온다. */
    IDE_TEST( rpnComm::recvSyncTableNumber( mProtocolContext,
                                            aSyncTableNumber,
                                            RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS);

    IDE_TEST_RAISE ( *aSyncTableNumber == 0, ERR_INVALID_SYNC_TABLE_NUMBER );

    /* Sync table 정보를 담을 공간 할당.
     * 메모리 해제는 receiver apply의 finalize단계에서 한다. */
    IDU_FIT_POINT_RAISE( "rpxReceiver::recvSyncTablesInfo::calloc::RemoteTable",
                          ERR_MEMORY_ALLOC_TABLE );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPD_META,
                                       *aSyncTableNumber,
                                       ID_SIZEOF(rpdMetaItem),
                                       (void **)(&sRemoteTable),
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_TABLE );

    /* SyncTableNumber만큼 Meta를 받아온다. */
    for ( i = 0; i < *aSyncTableNumber; i++ )
    {
        IDE_TEST( rpnComm::recvMetaReplTbl( mProtocolContext,
                                            &mExitFlag,
                                            sRemoteTable + i,
                                            RPU_REPLICATION_RECEIVE_TIMEOUT )
                  != IDE_SUCCESS );
    }

    *aRemoteTable = sRemoteTable;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::recvSyncTablesInfo",
                                  "SyncTables" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SYNC_TABLE_NUMBER );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_SYNC_TABLE_NUMBER ) );
    }
    IDE_EXCEPTION_END;

    if ( sRemoteTable != NULL )
    {
        (void) iduMemMgr::free( sRemoteTable );
        sRemoteTable = NULL;
    }

    return IDE_FAILURE;
}

smSN rpxReceiver::getRestartSN( void )
{
    smSN sRestartSN = SM_SN_NULL;

    if ( mRemoteMeta != NULL )
    {
        if ( mMeta.getRemoteXSN() == SM_SN_NULL )
        {
            sRestartSN = mRemoteMeta->mReplication.mXSN;
        }
        else if ( mRemoteMeta->mReplication.mXSN == SM_SN_NULL )
        {
            sRestartSN = mMeta.getRemoteXSN();
        }
        else
        {
            sRestartSN = IDL_MAX( mRemoteMeta->mReplication.mXSN, mMeta.getRemoteXSN() );
        }
    }
    else
    {
        /* nothing to do */
    }

    return sRestartSN;
}

UInt rpxReceiver::getParallelApplierCount( void )
{
    UInt sApplierCount = 0;

    switch ( mStartMode )
    {
        case RP_RECEIVER_NORMAL:
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_PARALLEL:
        case RP_RECEIVER_SYNC_CONDITIONAL:
        case RP_RECEIVER_SYNC:
        case RP_RECEIVER_RECOVERY:
        case RP_RECEIVER_OFFLINE:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
            sApplierCount = mMeta.getParallelApplierCount();
            break;

        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            sApplierCount = 0; 
            break;

        default :
            IDE_DASSERT( 0 );
    }

    return sApplierCount;
}

ULong rpxReceiver::getApplierInitBufferSize( void )
{
    return mMeta.getApplierInitBufferSize();
}

ULong rpxReceiver::getApplierQueueSize( ULong aRowSize, ULong aBufferSize )
{
    ULong sQueSize  = 0;

    mXLogSize = aRowSize + ID_SIZEOF( rpdXLog );
    
    /* BufferSize 가 XLogSize 보다 작을 경우 기존 프로퍼티값을 반환한다. 
     * 사이즈의 최소값은 프로퍼티값으로 한다. */
    if ( aBufferSize <= ( mXLogSize * RPU_REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE ) )
    {
        sQueSize = RPU_REPLICATION_RECEIVER_APPLIER_QUEUE_SIZE;
    }
    else
    {
        sQueSize = aBufferSize / mXLogSize;
    }

    ideLog::log( IDE_RP_0, "[Receiver] Initialize Applier XLog Queue Size : %"ID_UINT64_FMT"\n"\
                 "[Initialize Applier Buffer Size : %"ID_UINT64_FMT" Bytes, Applier XLog Size :%"ID_UINT64_FMT" Bytes]",
                 sQueSize, aBufferSize, mXLogSize );

    /* 구문 또는 프로퍼티로 얻은 사이즈를 반환한다 */
    return sQueSize;
}

IDE_RC rpxReceiver::initializeFreeXLogQueue( void )
{
    rpdXLog     * sXLog = NULL;
    UInt          i = 0;
    ULong         sBufferSize = 0;
    ULong         sApplierInitBufferSize = 0;
    idBool        sIsAlloc = ID_FALSE;
    idBool        sIsLob = ID_FALSE;
    idBool        sIsInitialized = ID_FALSE;
    UInt          sInitializeXLogCount = 0;

    sBufferSize = getBaseXLogBufferSize( &mMeta );
    sApplierInitBufferSize = getApplierInitBufferSize();

    mApplierQueueSize = getApplierQueueSize( sBufferSize, sApplierInitBufferSize );

    IDE_TEST( mFreeXLogQueue.initialize( mRepName ) != IDE_SUCCESS );
    sIsInitialized = ID_TRUE;

    IDU_FIT_POINT_RAISE( "rpxReceiver::initializeFreeXLogQueue::malloc::sXLog",
                             ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       ID_SIZEOF( rpdXLog ) * mApplierQueueSize, 
                                       (void**)&mXLogPtr,
                                       IDU_MEM_IMMEDIATE,
                                       mAllocator )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sIsAlloc = ID_TRUE;

    sIsLob = isLobColumnExist();
    for ( i = 0; i < mApplierQueueSize; i++ )
    {
        IDE_TEST( rpdQueue::initializeXLog( &( mXLogPtr[i] ), 
                                            sBufferSize,
                                            sIsLob,
                                            mAllocator )
                  != IDE_SUCCESS );
        sInitializeXLogCount++;

        enqueueFreeXLogQueue( &( mXLogPtr[i] ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initializeFreeXLogQueue",
                                  "sXLog" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsInitialized == ID_TRUE )
    {
        mFreeXLogQueue.read( &sXLog );
        while ( sXLog != NULL )
        {
            mFreeXLogQueue.read( &sXLog );            
        }

        sIsInitialized = ID_FALSE;
        mFreeXLogQueue.destroy();
    }
    else
    {
        /* do nothing */
    }

    if ( sIsAlloc == ID_TRUE )
    {
        for ( i = 0; i < sInitializeXLogCount; i++ )
        {
            rpdQueue::destroyXLog( &mXLogPtr[i], mAllocator );
        }
        
        sIsAlloc = ID_FALSE;
        (void)iduMemMgr::free( mXLogPtr );
        mXLogPtr = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxReceiver::finalizeFreeXLogQueue( void )
{
    rpdXLog     * sXLog = NULL;
    UInt          i = 0;

    mFreeXLogQueue.setExitFlag();

    mFreeXLogQueue.read( &sXLog );
    while ( sXLog != NULL )
    {
        mFreeXLogQueue.read( &sXLog );
    }

    mFreeXLogQueue.destroy();

    if ( mXLogPtr != NULL )
    { 
        for ( i = 0; i < mApplierQueueSize; i++ )
        {
            rpdQueue::destroyXLog( &mXLogPtr[i], mAllocator );
        }
   
        (void)iduMemMgr::free( mXLogPtr, mAllocator );
        mXLogPtr = NULL;
    }
    else
    {
        /* nothing to do */
    }
}

void rpxReceiver::enqueueFreeXLogQueue( rpdXLog     * aXLog )
{
    mFreeXLogQueue.write( aXLog );
}

IDE_RC rpxReceiver::dequeueFreeXLogQueue( rpdXLog   ** aXLog )
{
    IDE_TEST( mFreeXLogQueue.read( aXLog, 1 ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt rpxReceiver::assignApplyIndex( smTID aTID )
{
    UInt    sIndex = 0;
    SInt    sApplyIndex = 0;

    sIndex = aTID % mTransactionTableSize;
    sApplyIndex = mTransToApplierIndex[sIndex];

    /* 없으면 Transaction 의 시작 이다 */

    if ( sApplyIndex == -1 )
    {
        sApplyIndex = getIdleReceiverApplyIndex();
        mTransToApplierIndex[sIndex] = sApplyIndex ;
    }
    else
    {
        /* Nothing to do */
    }

    return sApplyIndex;
}

void rpxReceiver::deassignApplyIndex( smTID aTID )
{
    UInt    sIndex = 0;

    sIndex = aTID % mTransactionTableSize;
    mTransToApplierIndex[sIndex] = RPX_INDEX_INIT;
}

SInt rpxReceiver::getIdleReceiverApplyIndex( void )
{
    SInt sApplyIndex = 0;
    UInt sTransactionCount = 0;
    UInt sAssignedXLogCount = 0;
    UInt sMinTransactionCount = UINT_MAX;
    UInt sMinAssignedXLogCount = UINT_MAX;

    UInt i = 0;

    for ( i = 0; i < mApplierCount; i++ )
    {
        sTransactionCount = mApplier[i].getATransCntFromTransTbl();
        sAssignedXLogCount = mApplier[i].getAssingedXLogCount();

        if ( RPU_REPLICATION_RECEIVER_APPLIER_ASSIGN_MODE == 0 )
        {
            /* Transaction Count Mode */
            if ( sMinTransactionCount > sTransactionCount )
            {
                sMinTransactionCount = sTransactionCount;
                sMinAssignedXLogCount = sAssignedXLogCount;
                sApplyIndex = i;
            }
            else if ( sMinTransactionCount == sTransactionCount )
            {
                if ( sMinAssignedXLogCount > sAssignedXLogCount )
                {
                    sMinTransactionCount = sTransactionCount;
                    sMinAssignedXLogCount = sAssignedXLogCount;
                    sApplyIndex = i;
                }
                else
                {
                    /* do nothing */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Queue Count Mode */
            if ( sMinAssignedXLogCount > sAssignedXLogCount )  
            {
                sMinTransactionCount = sTransactionCount;
                sMinAssignedXLogCount = sAssignedXLogCount;
                sApplyIndex = i;
            }
            else if ( sMinAssignedXLogCount == sAssignedXLogCount )
            {
                if ( sMinTransactionCount > sTransactionCount )
                {
                    sMinTransactionCount = sTransactionCount;
                    sMinAssignedXLogCount = sAssignedXLogCount;
                    sApplyIndex = i;
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
    }

    return sApplyIndex;
}

smSN rpxReceiver::getRestartSNInParallelApplier( void )
{
    smSN        sMinRestartSN = SM_SN_NULL;
    smSN        sRestartSN = SM_SN_NULL;
    UInt        i = 0;

    sMinRestartSN = mApplier[0].getRestartSN();
    for ( i = 1; i < mApplierCount; i++ )
    {
        sRestartSN = mApplier[i].getRestartSN();
        if ( sMinRestartSN > sRestartSN )
        {
            sMinRestartSN = sRestartSN;
        }
        else
        {
            /* do nothing */
        }
    }

    return sMinRestartSN;
}

IDE_RC rpxReceiver::getLocalFlushedRemoteSNInParallelAppiler( rpdXLog        * aXLog,
                                                              smSN           * aLocalFlushedRemoteSN )
{
    IDE_TEST( getLocalFlushedRemoteSN( aXLog->mSyncSN,
                                       aXLog->mRestartSN,
                                       aLocalFlushedRemoteSN )
              != IDE_SUCCESS ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiver::getLastCommitAndProcessedSNInParallelAppiler( smSN    * aLastCommitSN,
                                                                smSN    * aLastProcessedSN )
{
    UInt    i = 0;
    smSN    sLastCommitSN = SM_SN_NULL;
    smSN    sLastProcessedSN = SM_SN_NULL;

    *aLastCommitSN = mApplier[0].getLastCommitSN();
    *aLastProcessedSN = mApplier[0].getProcessedSN();

    for ( i = 1; i < mApplierCount; i++ )
    {
        sLastCommitSN = mApplier[i].getLastCommitSN();
        sLastProcessedSN = mApplier[i].getProcessedSN();

        if ( sLastCommitSN > *aLastCommitSN )
        {
            *aLastCommitSN = sLastCommitSN;
        }
        else
        {
            /* do nothing */
        }

        if ( ( *aLastProcessedSN == SM_SN_MIN ) || ( sLastProcessedSN < *aLastProcessedSN ) )
        {
            *aLastProcessedSN = sLastProcessedSN;
        }
        else
        {
            /* do nothing */
        }
    }   /* end for */
}

IDE_RC rpxReceiver::buildXLogAckInParallelAppiler( rpdXLog      * aXLog,
                                                   rpXLogAck    * aAck )
{
    smSN        sLastCommitSN = SM_SN_NULL;
    smSN        sLastProcessedSN = SM_SN_NULL;

    switch ( aXLog->mType )
    {
        case RP_X_HANDSHAKE:   // PROJ-1442 Replication Online 중 DDL 허용
            IDE_WARNING( IDE_RP_0, RP_TRC_RA_NTC_HANDSHAKE_XLOG );
            aAck->mAckType = RP_X_HANDSHAKE_ACK;
            break;

        case RP_X_REPL_STOP :
            IDE_WARNING( IDE_RP_0, RP_TRC_RA_NTC_REPL_STOP_XLOG );
            aAck->mAckType = RP_X_STOP_ACK;
            break;

        default :
            aAck->mAckType = RP_X_ACK;
            break;
    }

    if ( ( aXLog->mType == RP_X_KEEP_ALIVE ) ||
         ( aXLog->mType == RP_X_REPL_STOP ) )
    {
        aAck->mRestartSN = getRestartSNInParallelApplier();
        if ( aAck->mRestartSN != SM_SN_NULL )
        {
            mRestartSN = aAck->mRestartSN;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( getLocalFlushedRemoteSNInParallelAppiler( aXLog,
                                                            &(aAck->mFlushSN) )
                  != IDE_SUCCESS );
    }
    else
    {
        aAck->mRestartSN = SM_SN_NULL;
        aAck->mFlushSN = SM_SN_NULL;
    }

    getLastCommitAndProcessedSNInParallelAppiler( &sLastCommitSN,
                                                  &sLastProcessedSN );

    if ( aXLog->mSN == 0 )
    {
        aAck->mLastArrivedSN   = SM_SN_NULL;
        sLastProcessedSN = SM_SN_NULL;
    }
    else
    {
        aAck->mLastArrivedSN     = aXLog->mSN;
    }

    aAck->mLastCommitSN = sLastCommitSN;
    aAck->mLastProcessedSN = sLastProcessedSN;

    aAck->mAbortTxCount = 0;
    aAck->mAbortTxList = NULL;
    aAck->mClearTxCount = 0;
    aAck->mClearTxList = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::startApplier( void )
{
    UInt        i = 0;
    UInt        sSuccessCount = 0;

    for ( i = 0; i < mApplierCount; i++ )
    {
        IDU_FIT_POINT( "rpxReceiver::startApplier::start::mApplier",
                       rpERR_ABORT_RP_INTERNAL_ARG,
                       "mApplier->start" );
        IDE_TEST( mApplier[i].start() != IDE_SUCCESS );
        sSuccessCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    for ( i = 0; i < mApplierCount; i++ )
    {
        mApplier[i].setExitFlag();
    }

    for ( i = 0; i < sSuccessCount; i++ )
    {
        (void)mApplier[i].join();
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxReceiver::joinApplier( void )
{
    UInt    i = 0;

    for ( i = 0; i < mApplierCount; i++ )
    {
        if ( mApplier[i].join() != IDE_SUCCESS )
        {
            IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_JOIN_THREAD));
            IDE_ERRLOG(IDE_RP_0);
        }
        else
        {
            /* Nothing to do */
        }
    }
}

IDE_RC rpxReceiver::getLocalFlushedRemoteSN( smSN aRemoteFlushSN,
                                             smSN aRestartSN,
                                             smSN * aLocalFlushedRemoteSN )
{
    smSN         sLocalFlushSN          = SM_SN_NULL;
    smSN         sLocalFlushedRemoteSN  = SM_SN_NULL;

    if ( mSNMapMgr != NULL )
    {
        IDE_TEST( smiGetSyncedMinFirstLogSN( &sLocalFlushSN )
                  != IDE_SUCCESS );

        if ( sLocalFlushSN != SM_SN_NULL )
        {
            mSNMapMgr->getLocalFlushedRemoteSN( sLocalFlushSN,
                                                aRemoteFlushSN,
                                                aRestartSN,
                                                &sLocalFlushedRemoteSN );
        }   
        else    
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }   

    *aLocalFlushedRemoteSN = sLocalFlushedRemoteSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::checkAndWaitAllApplier( smSN   aWaitSN )
{
    UInt          i = 0;

    for ( i = 0; i < mApplierCount; i++ )
    {
        IDE_TEST( checkAndWaitApplier( i, 
                                       aWaitSN,
                                       &mHashMutex,
                                       &mHashCV,
                                       &mIsWait )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::checkAndWaitApplier( UInt                 aApplierIndex,
                                         smSN                 aWaitSN,
                                         iduMutex           * aMutex,
                                         iduCond            * aCondition,
                                         idBool             * aIsWait )
{
    UInt              sYieldCount = 0;
    PDL_Time_Value    sCheckTime;
    PDL_Time_Value    sSleepSec;

    while ( mApplier[aApplierIndex].getProcessedSN() < aWaitSN )
    {
        if ( sYieldCount < RPU_REPLICATION_RECEIVER_APPLIER_YIELD_COUNT )
        {
            idlOS::thr_yield();
            sYieldCount++;
        }
        else
        {
            sCheckTime.initialize();
            sSleepSec.initialize();

            sSleepSec.set( 1 );
            sCheckTime = idlOS::gettimeofday() + sSleepSec;

            IDE_ASSERT( aMutex->lock( NULL ) == IDE_SUCCESS );

            *aIsWait = ID_TRUE;
            (void)aCondition->timedwait( aMutex, &sCheckTime, IDU_IGNORE_TIMEDOUT );
            *aIsWait = ID_FALSE;

            IDE_ASSERT( aMutex->unlock() == IDE_SUCCESS );
        }

        IDE_TEST_RAISE( mExitFlag == ID_TRUE, ERR_SET_EXIT_FLAG );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_EXIT_FLAG )
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_EXIT_FLAG_SET ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiver::wakeupReceiverApplier( void )
{
    UInt    i = 0;

    for ( i = 0; i < mApplierCount; i++ )
    {
        mApplier[i].wakeup();
    }

    wakeup();
}

IDE_RC rpxReceiver::allocRangeColumnInParallelAppiler( void )
{
    UInt        i = 0;
    UInt        sMaxPkColCount = 0;

    sMaxPkColCount = mMeta.getMaxPkColCountInAllItem();
    if ( sMaxPkColCount == 0 )
    {
        sMaxPkColCount = 1;
    }
    IDE_TEST( mApply.allocRangeColumn( sMaxPkColCount ) != IDE_SUCCESS );

    for ( i = 0; i < mApplierCount; i++ )
    {
        IDE_TEST( mApplier[i].allocRangeColumn( sMaxPkColCount ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiver::setParallelApplyInfo( UInt aIndex, rpxReceiverParallelApplyInfo * aApplierInfo )
{
    mApplier[aIndex].setParallelApplyInfo( aApplierInfo );
}

IDE_RC rpxReceiver::buildRecordForReplReceiverParallelApply( void                * aHeader,
                                                             void                * aDumpObj,
                                                             iduFixedTableMemory * aMemory )
{
    UInt                  i = 0;

    for ( i = 0; i < mApplierCount; i++ )
    {
        IDE_TEST( mApplier[i].buildRecordForReplReceiverParallelApply( aHeader,
                                                                       aDumpObj,
                                                                       aMemory )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                                        void                    * aDumpObj,
                                                        iduFixedTableMemory     * aMemory )
{
    UInt    i = 0;

    if ( isApplierExist() != ID_TRUE )
    {
        IDE_TEST( mApply.buildRecordForReplReceiverTransTbl( aHeader,
                                                             aDumpObj,
                                                             aMemory,
                                                             mRepName,
                                                             mParallelID,
                                                             -1 /* Applier Index */)
                  != IDE_SUCCESS );
    }
    else
    {
        for ( i = 0; i < mApplierCount; i++ )
        {
            IDE_TEST( mApplier[i].buildRecordForReplReceiverTransTbl( aHeader,
                                                                      aDumpObj,
                                                                      aMemory,
                                                                      mParallelID )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

ULong rpxReceiver::getApplierInitBufferUsage( void )
{
    ULong sSize = 0;
    ULong sQueueSize = 0;

     /* 현재 Applier 들이 가지고 있는 queue 들의 갯수를 구한다. */
    sQueueSize = mApplierQueueSize - mFreeXLogQueue.getSize();
    sSize = sQueueSize * mXLogSize;

    return sSize;
}

ULong rpxReceiver::getReceiveDataSize( void )
{
    ULong sReceiveDataSize;
    
    IDE_ASSERT( lock() == IDE_SUCCESS );
    if ( ( mExitFlag != ID_TRUE ) && ( mProtocolContext != NULL ) )
    {
        sReceiveDataSize =  mProtocolContext->mReceiveDataSize;
    }
    else
    {
        sReceiveDataSize = 0;
    }
    IDE_ASSERT( unlock() == IDE_SUCCESS );
    
    return sReceiveDataSize;
}

ULong rpxReceiver::getReceiveDataCount( void )
{
    ULong sReceiveDataCount;
    
    IDE_ASSERT( lock() == IDE_SUCCESS );
    if ( ( mExitFlag != ID_TRUE ) && ( mProtocolContext != NULL ) )
    {
        sReceiveDataCount = mProtocolContext->mReceiveDataCount;
    }
    else
    {
        sReceiveDataCount = 0;
    }
    IDE_ASSERT( unlock() == IDE_SUCCESS );
    
    return sReceiveDataCount;
}

IDE_RC rpxReceiver::buildNewMeta( smiStatement * aStatement )
{
    IDE_DASSERT( mNewMeta == NULL );

    IDU_FIT_POINT_RAISE( "rpxReceiver::buildNewMeta::calloc::mNewMeta",
                         ERR_MEMORY_ALLOC_NEW_META );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX,
                                       1,
                                       ID_SIZEOF( rpdMeta ),
                                       (void**)&mNewMeta,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_NEW_META );

    mNewMeta->initialize();
    IDE_TEST( mNewMeta->build( aStatement,
                               mRepName,
                               ID_TRUE,
                               RP_META_BUILD_LAST,
                               SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_NEW_META )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::buildNewMeta",
                                  "mNewMeta" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( mNewMeta != NULL )
    {
        mNewMeta->finalize();
        (void)iduMemMgr::free( mNewMeta );
        mNewMeta = NULL;
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpxReceiver::removeNewMeta()
{
    if ( mNewMeta != NULL )
    {
        mNewMeta->finalize();

        (void)iduMemMgr::free( mNewMeta );
        mNewMeta = NULL;
    }
    else
    {
        /* nothing to do */
    }
}

void rpxReceiver::wakeup( void )
{
    IDE_ASSERT( lock() == IDE_SUCCESS );

    if ( mIsWait == ID_TRUE )
    {
        mHashCV.signal();
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT( unlock() == IDE_SUCCESS );
}

IDE_RC rpxReceiver::sendDDLASyncStartAck( UInt aType, ULong aSendTimeout )
{
    return rpnComm::sendDDLASyncStartAck( mProtocolContext,
                                          &mExitFlag,
                                          aType,
                                          aSendTimeout );

}

IDE_RC rpxReceiver::recvDDLASyncExecute( UInt         * aType,
                                         SChar        * aUserName,
                                         UInt         * aDDLEnableLevel,
                                         UInt         * aTargetCount,
                                         SChar        * aTargetTableName,
                                         SChar       ** aTargetPartNames,
                                         smSN         * aDDLCommitSN,
                                         rpdVersion   * aReplVersion,
                                         SChar       ** aDDLStmt,
                                         ULong          aRecvTimeout )
{
    return rpnComm::recvDDLASyncExecute( mProtocolContext,
                                         &mExitFlag,
                                         aType,
                                         aUserName,
                                         aDDLEnableLevel,
                                         aTargetCount,
                                         aTargetTableName,
                                         aTargetPartNames,
                                         aDDLCommitSN,
                                         aReplVersion,
                                         aDDLStmt,
                                         aRecvTimeout );
}

IDE_RC rpxReceiver::sendDDLASyncExecuteAck( UInt    aType,
                                            UInt    aIsSuccess,
                                            UInt    aErrCode,
                                            SChar * aErrMsg,
                                            ULong   aSendTimeout )
{
    return rpnComm::sendDDLASyncExecuteAck( mProtocolContext,
                                            &mExitFlag,
                                            aType,
                                            aIsSuccess,
                                            aErrCode,
                                            aErrMsg,
                                            aSendTimeout );
}

idBool rpxReceiver::isAlreadyAppliedDDL( smSN aRemoteDDLCommitSN )
{
    smSN   sRemoteLastDDLXSN = SM_SN_NULL;
    idBool sIsAlreadyApplied = ID_FALSE;

    sRemoteLastDDLXSN = mMeta.getRemoteLastDDLXSN();
    if ( ( sRemoteLastDDLXSN == SM_SN_NULL ) ||
         ( sRemoteLastDDLXSN < aRemoteDDLCommitSN ) )
    {
        sIsAlreadyApplied = ID_FALSE;
    }
    else
    {
        sIsAlreadyApplied = ID_TRUE;
    }

    return sIsAlreadyApplied;
}

IDE_RC rpxReceiver::checkLocalAndRemoteNames( SChar  * aUserName,
                                              UInt     aTargetCount,
                                              SChar  * aTargetTableName,
                                              SChar  * aTargetPartNames )
{
    UInt i = 0;
    UInt sOffset     = 0;
    UInt sMatchCount = 0;
    SChar * sTargetPartName = NULL;

    for ( i = 0; i < aTargetCount; i++ )
    {
        sTargetPartName = aTargetPartNames + sOffset;

        if ( mMeta.isTargetItem( aUserName,
                                 aTargetTableName,
                                 sTargetPartName )
             == ID_TRUE )
        {
            sMatchCount += 1;
        }

        sOffset += QC_MAX_OBJECT_NAME_LEN + 1;
    }
    IDE_TEST_RAISE( aTargetCount != sMatchCount, ERR_MISSMATCH_TARGET );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MISSMATCH_TARGET );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "Target name miss matched") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::metaRebuild( smiTrans * aTrans )
{
    idBool       sIsStatementBegin = ID_FALSE;
    smiStatement sStatement;
    SChar        sRepName[QC_MAX_NAME_LEN + 1] = { 0, };

    IDE_TEST( sStatement.begin( aTrans->getStatistics(),
                                aTrans->getStatement(),
                                SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) 
              != IDE_SUCCESS );
    sIsStatementBegin = ID_TRUE;

    idlOS::strncpy( sRepName,
                    mRepName,
                    QC_MAX_NAME_LEN );
    sRepName[QC_MAX_NAME_LEN] = '\0';

    mMeta.finalize();
    mMeta.initialize();

    IDE_TEST( mMeta.build( &sStatement,
                           sRepName,
                           ID_TRUE,
                           RP_META_BUILD_LAST,
                           SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsStatementBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsStatementBegin == ID_TRUE )
    {
        sIsStatementBegin = ID_FALSE;
        (void)sStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::recoveryCondition( idBool aIsNeedToRebuildMeta )
{
    SInt               i = 0;
    SChar              sRepName[QC_MAX_NAME_LEN + 1] = { 0, };
    smiTrans           sTrans;
    smiStatement    * sRootStatement = NULL;
    smiStatement      sStatement;
    idBool            sIsTxBegin = ID_FALSE;
    UInt              sStage = 0;
    idBool            sIsDoBuildMeta = ID_TRUE;

    for( i = 0; i < mMeta.mReplication.mItemCount ; i++ )
    {
        if ( mMeta.mItemsOrderByTableOID[i]->mItem.mIsConditionSynced == ID_TRUE )
        {
            IDU_FIT_POINT( "rpxReceiver::recoveryCondition::before::executeTruncate" );
            if ( rpxReceiverApply::executeTruncate( this, mMeta.mItemsOrderByTableOID[i], ID_TRUE ) 
                 != IDE_SUCCESS )
            {
                ideLog::log(IDE_RP_0, 
                            "[Receiver] An error occurred while executing TRUNCATE the table that conditional synchronization was not completed. [Table: %s.%s %s]", 
                            mMeta.mItemsOrderByTableOID[i]->mItem.mLocalUsername, 
                            mMeta.mItemsOrderByTableOID[i]->mItem.mLocalTablename, 
                            mMeta.mItemsOrderByTableOID[i]->mItem.mLocalPartname );
            }
        }
    }

    if ( aIsNeedToRebuildMeta == ID_TRUE )
    {
        idlOS::strncpy( sRepName,
                        mRepName,
                        QC_MAX_NAME_LEN );
        sRepName[QC_MAX_NAME_LEN] = '\0';
        
        mMeta.finalize();
        mMeta.initialize();

        IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( sTrans.begin( &sRootStatement,
                                &mStatistics,
                                (UInt)RPU_ISOLATION_LEVEL       |
                                SMI_TRANSACTION_NORMAL          |
                                SMI_TRANSACTION_REPL_REPLICATED |
                                SMI_COMMIT_WRITE_NOWAIT,
                                RP_UNUSED_RECEIVER_INDEX )
                  != IDE_SUCCESS );
        sIsTxBegin = ID_TRUE;
        sStage = 2;

        while ( sIsDoBuildMeta == ID_TRUE )
        {
            IDE_TEST( sStatement.begin( &mStatistics,
                                        sRootStatement,
                                        SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
                      != IDE_SUCCESS );
            sStage = 3;

            if ( buildMeta( &sStatement,
                            sRepName )
                 == IDE_SUCCESS )
            {
                IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
                sStage = 2;

                sIsDoBuildMeta = ID_FALSE;
            }
            else
            {
                IDE_TEST( ideIsRetry() != IDE_SUCCESS );

                IDE_CLEAR();

                sStage = 2;
                IDE_TEST( sStatement.end( SMI_STATEMENT_RESULT_FAILURE ) != IDE_SUCCESS );
                sIsDoBuildMeta = ID_TRUE;
            }
        }

        sStage = 1;
        IDE_TEST( sTrans.commit() != IDE_SUCCESS );
        sIsTxBegin = ID_FALSE;

        sStage = 0;
        IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

        rpdMeta::remappingTableOID( mRemoteMeta, &mMeta );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            (void)sStatement.end( SMI_STATEMENT_RESULT_FAILURE );
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

    IDE_POP();

    return IDE_FAILURE;

}

IDE_RC rpxReceiver::buildMeta( smiStatement       * aStatement,
                               SChar              * aRepName )
{
    smiTrans    * sTrans = NULL;
    UInt          sBackupTimeout = 0;

    /* BUG-42734 */
    sTrans = aStatement->getTrans();
    sBackupTimeout = sTrans->getReplTransLockTimeout();
    IDE_TEST( sTrans->setReplTransLockTimeout( 3 ) != IDE_SUCCESS ); /*wait 3 seconds*/

    IDE_TEST( mMeta.build( aStatement,
                           aRepName,
                           ID_TRUE,
                           RP_META_BUILD_LAST,
                           SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    /* receiver 에서만 필요한 정보라 receiver 에서만 build */
    IDE_TEST( mMeta.buildIndexTableRef( aStatement ) != IDE_SUCCESS );

    IDE_TEST( sTrans->setReplTransLockTimeout( sBackupTimeout ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::createAndInitializeXLogTransfer( rpxXLogTransfer ** aXLogTransfer,
                                                     rpdXLogfileMgr   * aXLogfileManager )
{
    rpxXLogTransfer * sXLogTransfer;
    UInt sStage = 0;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_RECEIVER, 
                                       ID_SIZEOF( rpxXLogTransfer), 
                                       (void **)&sXLogTransfer, 
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMALLOC );
    sStage = 1;
    new ( sXLogTransfer ) rpxXLogTransfer;

    sXLogTransfer->initialize( this, aXLogfileManager );

    *aXLogTransfer = sXLogTransfer;

    ideLog::log( IDE_RP_0, "[Receiver] %s XLogTransfer has been initialized", mRepName );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMALLOC )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "createAndInitializeXLogTransfer",
                                  "rpxXLogTransfer" ) );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1 :
            (void)iduMemMgr::free( sXLogTransfer );
        default:
            break;
    }

    return IDE_FAILURE;
}

void rpxReceiver::finalizeXLogTransfer()
{
    if( mXLogTransfer != NULL )
    {
        if ( mXLogTransfer->isStarted() == ID_TRUE )
        {
            mXLogTransfer->setExit( ID_TRUE );

            if ( mXLogTransfer->isWaitFromReceiverProcessDone() == ID_TRUE )
            {
                mXLogTransfer->signalToWaitedTransfer();
            }

            if(mXLogTransfer->join() != IDE_SUCCESS)
            {
                IDE_ERRLOG(IDE_RP_0);
                IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_JOIN_THREAD));
                IDE_ERRLOG(IDE_RP_0);
            }
        }
    }
}

void rpxReceiver::finalizeAndDestroyXLogTransfer()
{
    if( mXLogTransfer != NULL )
    {
        finalizeXLogTransfer();

        if ( isGottenNetworkResoucesFromXLogTransfer() == ID_FALSE )
        {
            getNetworkResourcesFromXLogTransfer();
        }

        mXLogTransfer->destroy();

        (void)iduMemMgr::free( mXLogTransfer );
        mXLogTransfer = NULL;

        ideLog::log( IDE_RP_0, "[Receiver] %s XLogTransfer has been finalized", mRepName );
    }
}

IDE_RC rpxReceiver::startXLogTrasnsfer()
{
    UInt sWaitTime = 10;

    IDE_DASSERT( mXLogTransfer != NULL );

    IDE_TEST(mXLogTransfer->start() != IDE_SUCCESS);

    IDE_TEST( mXLogTransfer->waitToStart( sWaitTime ) != IDE_SUCCESS );

    ideLog::log( IDE_RP_0, "[Receiver] %s XLogTransfer Started ...", mRepName );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    getNetworkResourcesFromXLogTransfer();

    return IDE_FAILURE;
}

UInt rpxReceiver::getCurrentFileNumberOfRead()
{
    return mMeta.mReplication.mCurrentReadLSNFromXLogfile.mFileNo;
}

/* R2HA
 * 기존 설계는 receiver만 떠서 xlogfile의 xlog를 읽어서 반영하는 것인데,
 * handshake 과정이 누락되었기 때문에, xlog 내 remote table OID가 receiver가 반영해야 할 target TID를 찾을 수 없다.
 * 따라서, xlog file만으로는 복구가 불가능하다.
 */
IDE_RC rpxReceiver::processXLogInXLogFile()
{
    rpdXLog sXLog;
    idBool sIsLob = ID_FALSE;
    idBool sIsInitializedXLog = ID_FALSE;

    idBool sIsDone = ID_FALSE;


    IDE_TEST( rpdQueue::initializeXLog( &sXLog, rpxReceiver::getBaseXLogBufferSize( &mMeta ), sIsLob, mAllocator ) != IDE_SUCCESS );
    sIsInitializedXLog = ID_TRUE;

    while( sIsDone != ID_TRUE )
    {
        IDE_TEST( receiveAndConvertXLog( &sXLog ) != IDE_SUCCESS );

        IDE_TEST( applyXLogAndSendAckInParallelAppiler( &sXLog) != IDE_SUCCESS );

        /* log type에 따른 별다른 처리는 무시한다?? handshake와 같은 것들은 어떻게 진행하나???
        * 설계 miss!!
        * 
        * 처리해야 할 log type
        * 1. replication stop
        * stop 의 경우 무시해도 된다. ack를 전송해야 하지만, 이전것이기 때문에 record 반영과 관련이 없다.
        * 따라서 무시한다.
        * 
        * 2. handshake 
        * handshake의 경우 receiver에서 수행해야 할 대표적인 xlog이다. 이는 ack가 전송되지 못했으므로,  PASSIVE 쪽에서 재요청하였을 것이다.
        * 따라서 무시한다.
        * 
        */

        rpdQueue::recycleXLog( &sXLog, mAllocator );
    }

    sIsInitializedXLog = ID_FALSE;
    rpdQueue::destroyXLog( &sXLog, mAllocator );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsInitializedXLog == ID_TRUE )
    {
        rpdQueue::destroyXLog( &sXLog, mAllocator );
    }

    return IDE_FAILURE;
}

void rpxReceiver::setNetworkResourcesToXLogTransfer()
{
    IDE_DASSERT( mXLogTransfer != NULL || mProtocolContext != NULL );

    mXLogTransfer->handOverNetworkResources( mProtocolContext );
    mProtocolContext = NULL;

    setReadXLogfileMode();
}

void rpxReceiver::getNetworkResourcesFromXLogTransfer()
{
    mProtocolContext = mXLogTransfer->takeAwayNetworkResources();

    IDE_DASSERT( mReadContext.mCMContext == mProtocolContext );

    setReadNetworkMode();
}

idBool rpxReceiver::isGottenNetworkResoucesFromXLogTransfer()
{
    idBool sIsGottenNetworkResources = ID_FALSE;

    if ( ( mReadContext.mCurrentMode != RPX_RECEIVER_READ_XLOGFILE ) &&
         ( mProtocolContext != NULL ) )
    {
        sIsGottenNetworkResources = ID_TRUE;
    }

    return sIsGottenNetworkResources;
}

void rpxReceiver::wakeupXLogTansfer()
{
    IDE_ASSERT( getXLogTransfer() != NULL );

    mXLogTransfer->signalToWaitedTransfer();
}

rpxReceiverReadContext rpxReceiver::setReadContext( cmiProtocolContext * aCMContext, rpdXLogfileMgr * aXLogfileContext )
{
    rpxReceiverReadContext sReceiverContext;

    sReceiverContext.mCMContext = aCMContext;
    sReceiverContext.mXLogfileContext = aXLogfileContext;
    sReceiverContext.mCurrentMode = RPX_RECEIVER_READ_UNSET;

    return sReceiverContext;
}

cmiProtocolContext * rpxReceiver::getCMReadContext( rpxReceiverReadContext * aReadContext )
{
    return aReadContext->mCMContext;
}

rpdXLogfileMgr * rpxReceiver::getXLogfileReadContext( rpxReceiverReadContext * aReadContext )
{
    return aReadContext->mXLogfileContext;
}

void rpxReceiver::setReadNetworkMode()
{
    mReadContext.mCurrentMode = RPX_RECEIVER_READ_NETWORK;

    ideLog::log( IDE_RP_6, "[Receiver] Context switching: xlogfile to network");
}

void rpxReceiver::setReadXLogfileMode()
{
    mReadContext.mCurrentMode = RPX_RECEIVER_READ_XLOGFILE;

    ideLog::log( IDE_RP_6, "[Receiver] Context switching: network to xlogfile");
}

IDE_RC rpxReceiver::createAndInitializeXLogfileManager( smiStatement    * aStatement,
                                                        rpdXLogfileMgr ** aXLogfileManager,
                                                        rpXLogLSN         aInitLSN )
{
    rpdXLogfileMgr * sXLogfileManager = NULL;
    UInt sStage = 0;
    smLSN sReadLSN;
    rpXLogLSN sCurrentReadLSN;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_RECEIVER, 
                                       ID_SIZEOF( rpdXLogfileMgr), 
                                       (void **)&sXLogfileManager, 
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMALLOC );
    sStage = 1;

    
    IDE_TEST( sXLogfileManager->initialize( mMeta.mReplication.mRepName,
                                            aInitLSN,
                                            RPU_REPLICATION_RECEIVE_TIMEOUT,
                                            &mExitFlag,
                                            this,
                                            RP_XLOGLSN_INIT)
              != IDE_SUCCESS );
    sStage = 2;

    sXLogfileManager->getReadInfo( &sCurrentReadLSN );
    RP_GET_XLOGLSN( sReadLSN.mFileNo, sReadLSN.mOffset, sCurrentReadLSN );
    mMeta.setCurrentReadXLogfileLSN( sReadLSN );

    if ( mStartMode != RP_RECEIVER_XLOGFILE_FAILBACK_MASTER )
    {
        /* retry 처리는 상위에서 한다. */
        IDE_TEST_RAISE( rpdCatalog::updateCurrentXLogfileLSN( aStatement,
                                                              mRepName,
                                                              sReadLSN )
                        != IDE_SUCCESS, ERR_UPDATE_CURRENT_READ_LSN );
    }
    
    *aXLogfileManager = sXLogfileManager;
    
    ideLog::log( IDE_RP_0, "[Receiver] %s XLogfileMgr has been initialized", mRepName );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMALLOC )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "createAndInitializeXLogfileManager",
                                  "sXLogfileManager" ) );
    }
    IDE_EXCEPTION( ERR_UPDATE_CURRENT_READ_LSN )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_INTERNAL_ARG,
                                  "rpxReceiver::updateCurrentInfoForConsistentMode") );
    }

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 2 :
            sXLogfileManager->finalize();
        case 1 :
            (void)iduMemMgr::free( sXLogfileManager );
        default:
            break;
    }

    return IDE_FAILURE;

}

void rpxReceiver::finalizeAndDestroyXLogfileManager()
{
    if ( mXLogfileManager != NULL )
    {
        mXLogfileManager->finalize();

        (void)iduMemMgr::free( mXLogfileManager );
        mXLogfileManager = NULL;

        ideLog::log( IDE_RP_0, "[Receiver] %s XLogfileMgr has been finalized", mRepName );
    }
}

/*
 * mLastProcessedSN
 */
void rpxReceiver::updateSNsForXLogTransfer( smSN aLastProcessedSN, smSN aRestartSN )
{

    if ( aLastProcessedSN != SM_SN_NULL )
    {
        mXLogTransfer->setLastProcessedSN( aLastProcessedSN );
    }

    if ( aRestartSN != SM_SN_NULL )
    {
        mXLogTransfer->setRestartSN( aRestartSN );
    }
}

IDE_RC rpxReceiver::updateCurrentInfoForConsistentModeWithNewTransaction( void )
{
    smiTrans          sTrans;
    SInt              sStage = 0;
    smiStatement    * sRootStatement = NULL;
    UInt              sFlag = 0;

    IDE_TEST_RAISE( sTrans.initialize() != IDE_SUCCESS, ERR_TRANS_INIT );
    sStage = 1;

    sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = (sFlag & ~SMI_TRANSACTION_MASK) | SMI_TRANSACTION_NORMAL;
    sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
    sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_WAIT;

    IDE_TEST_RAISE( sTrans.begin( &sRootStatement,
                                  &mStatistics, 
                                  sFlag,
                                  mReplID )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );
    sStage = 2;

    IDE_TEST( sTrans.setReplTransLockTimeout( 0 ) != IDE_SUCCESS );

    IDE_TEST_RAISE( updateCurrentInfoForConsistentMode( sRootStatement ) != IDE_SUCCESS,
                    ERR_UPDATE_CURRENT_READ_LSN );

    IDE_TEST_RAISE( sTrans.commit() != IDE_SUCCESS, ERR_TRANS_COMMIT );
    sStage = 1;

    sStage = 0;
    IDE_TEST_RAISE( sTrans.destroy( NULL ) != IDE_SUCCESS, ERR_TRANS_DESTROY );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_INIT )
    {
        IDE_WARNING( IDE_RP_0, 
                     "[Receiver] Trans init() error in updateCurrentInfoForConsistentModeWithNewTransaction()" );
    }
    IDE_EXCEPTION( ERR_TRANS_BEGIN )
    {
        IDE_WARNING( IDE_RP_0, 
                     "[Receiver] Trans begin() error in updateCurrentInfoForConsistentModeWithNewTransaction()" );
    }
    IDE_EXCEPTION( ERR_UPDATE_CURRENT_READ_LSN )
    {
        ideLog::log( IDE_RP_0, 
                     "[Receiver] updateCurrentInfoForConsistentModeWithNewTransaction error" );
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT )
    {
        IDE_WARNING( IDE_RP_0, 
                     "[Receiver] Trans commit() error in updateCurrentInfoForConsistentModeWithNewTransaction()" );
    }
    IDE_EXCEPTION( ERR_TRANS_DESTROY )
    {
        IDE_WARNING( IDE_RP_0, 
                     "[Receiver] Trans destroy() error in updateCurrentInfoForConsistentModeWithNewTransaction()" );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
        case 1:
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::updateCurrentInfoForConsistentMode( smiStatement    * aParentStatement )
{
    smLSN sReadLSN;
    rpXLogLSN sCurrentReadLSN;

    setRestartSNAndLastProcessedSN( mXLogTransfer );

    mXLogfileManager->getReadInfo( &sCurrentReadLSN );
    RP_GET_XLOGLSN( sReadLSN.mFileNo, sReadLSN.mOffset, sCurrentReadLSN );

    mMeta.setCurrentReadXLogfileLSN( sReadLSN );

    IDE_TEST_RAISE( rpcManager::updateXLogfileCurrentLSN( aParentStatement,
                                                          mRepName,
                                                          sReadLSN )
                    != IDE_SUCCESS, ERR_UPDATE_CURRENT_READ_LSN );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UPDATE_CURRENT_READ_LSN )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_INTERNAL_ARG,
                                  "rpxReceiver::updateCurrentInfoForConsistentMode") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiver::setRestartSNAndLastProcessedSN( rpxXLogTransfer * aXLogtransfer )
{
    smSN sLastProcessedSN;
    smSN sRestartSN;
    smSN sDummySN;

    sRestartSN = getRestartSNInParallelApplier();
    getLastCommitAndProcessedSNInParallelAppiler( &sDummySN, &sLastProcessedSN );

    if ( sLastProcessedSN != SM_SN_NULL )
    {
        aXLogtransfer->setLastProcessedSN( sLastProcessedSN );
    }

    if ( sRestartSN != SM_SN_NULL )
    {
        aXLogtransfer->setRestartSN( sRestartSN );

    }
}

/*
 * R2HA
 * 현재 applier의 Queue안에 존재하는 xlog의 fileNo를 알수 있는 방법이 없다. 따라서, 설정된 size를 통해 보수적으로 접근한다.
 * queue size가 17M, Applier count 3개, XLog File size가 10MB 이면,
 * 51M/10M = 5.1 개의 log count가 나온다. 따라서, 현재 xlog no의 - 5.1 부터 삭제하면 안된다. (그래서 + 1 해줌)
 */
UInt rpxReceiver::getMinimumUsingXLogFileNo()
{
    smLSN sCurrentReadXLogLSN = mMeta.getCurrentReadXLogfileLSN();
    UInt sCurrentReadXLogFileNo = 0;

    UInt sTotalFileCountAsAppliersQueueSize = 0;
    sCurrentReadXLogFileNo = sCurrentReadXLogLSN.mFileNo;

    sTotalFileCountAsAppliersQueueSize =  ( ( getApplierInitBufferSize() * getParallelApplierCount() )
            / RPU_REPLICATION_XLOGFILE_SIZE ) + 1;

    sCurrentReadXLogFileNo -= sTotalFileCountAsAppliersQueueSize;

    return sCurrentReadXLogFileNo;
}
/* PROJ-2742 Support data integrity after fail-back on 1:1 consistent replication
 * active node에서 반영이 완료되어 제거해도 되는 SN을 KeepAlive 로그에 mSyncSN으로 보낸다.
 * mSyncSN 까지가 저장된 xlogfile 은 삭제해도 된다. 
 * mSyncSN이 SM_SN_NULL 인 경우는 0을 리턴한다. */ 
IDE_RC rpxReceiver::findXLogfileNoByRemoteCheckpointSN( UInt *aFileNo )
{
    rpXLogLSN sXLogLSN = RP_XLOGLSN_INIT;
    UInt      sFileNo = 0;

    if ( mRemoteCheckpointSN != SM_SN_NULL )
    {
        IDE_TEST( rpdXLogfileMgr::getXLogLSNFromXSNAndReplName( mRemoteCheckpointSN,
                                                                mMeta.mReplication.mRepName,
                                                                &sXLogLSN )
                  != IDE_SUCCESS );
        RP_GET_FILENO_FROM_XLOGLSN( sFileNo, sXLogLSN );
    }

    *aFileNo = sFileNo ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 
 * XLogfile에 남아있는 XLog를 반영하는 함수
 * aIsSkipStopXLog 가 ID_TRUE 인 경우, RP_X_REPL_STOP 로그는 반영하지 않는다.
 */
IDE_RC rpxReceiver::processRemainXLogInXLogfile( idBool aIsSkipStopXLog )
{
    rpdXLog   * sXLog = NULL;
    idBool      sIsLastXLogInFile = ID_TRUE;
    rpXLogAck   sAck;

    mXLogfileManager->setWaitWrittenXLog( ID_FALSE );
    sIsLastXLogInFile = mXLogfileManager->isLastXLog();

    while  ( sIsLastXLogInFile != ID_TRUE )
    {
        IDE_CLEAR();

        IDE_TEST( dequeueFreeXLogQueue( &sXLog ) != IDE_SUCCESS );

        rpdQueue::recycleXLog( sXLog, mAllocator );

        if ( receiveAndConvertXLog( sXLog ) != IDE_SUCCESS )
        {
            IDE_TEST( ideGetErrorCode() != rpERR_IGNORE_RPX_END_OF_XLOGFILES );
            IDE_RAISE( NORMAL_EXIT );
        }

        if ( ( aIsSkipStopXLog != ID_TRUE ) ||
             ( sXLog->mType != RP_X_REPL_STOP ) )
        {
            IDE_TEST( enqueueXLog( sXLog ) != IDE_SUCCESS );
        }
        
        switch ( sXLog->mType )
        {
            case RP_X_XA_START_REQ:
            case RP_X_XA_PREPARE_REQ:
            case RP_X_XA_PREPARE:
            case RP_X_KEEP_ALIVE:
            case RP_X_HANDSHAKE:
                if ( useNetworkResourceMode() == ID_TRUE )
                {
                    getNetworkResourcesFromXLogTransfer();

                    buildDummyXLogAckForConsistent( sXLog, &sAck);
                    IDE_TEST( sendAckWithTID( &sAck ) != IDE_SUCCESS );

                    setNetworkResourcesToXLogTransfer();
                }
                break;

            default:
                break;
        }

        saveRestartSNAtRemoteMeta( mRestartSN );

        sIsLastXLogInFile = mXLogfileManager->isLastXLog();
    }
    
    RP_LABEL(NORMAL_EXIT);
    mXLogfileManager->setWaitWrittenXLog( ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );
    mXLogfileManager->setWaitWrittenXLog( ID_TRUE );

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::collectUnCompleteGlobalTxList( iduList * aGlobalTxList )
{
    rpdXLog   sXLog;
    iduList   sGlobalTxList;
    idBool    sIsLastXLogInFile  = ID_FALSE;
    idBool    sIsLob             = ID_FALSE;
    idBool    sIsInitializedXLog = ID_FALSE;
    smSN      sLastXSN           = SM_SN_NULL;
    dkiUnCompleteGlobalTxInfo * sGlobalTxNode = NULL;

    IDU_LIST_INIT( &sGlobalTxList );
    
    sIsLob = isLobColumnExist();
    IDE_TEST( rpdQueue::initializeXLog( &sXLog,
                                        getBaseXLogBufferSize( &mMeta ),
                                        sIsLob,
                                        mAllocator )
              != IDE_SUCCESS );
    sIsInitializedXLog = ID_TRUE;

    while  ( sIsLastXLogInFile != ID_TRUE )
    {
        IDE_CLEAR();

        rpdQueue::recycleXLog( &sXLog, mAllocator );
        IDE_TEST( receiveAndConvertXLog( &sXLog ) != IDE_SUCCESS );

        if ( ( sLastXSN == SM_SN_NULL ) || ( sLastXSN < sXLog.mSN ) ) 
        {
            sLastXSN = sXLog.mSN;

            switch ( sXLog.mType )
            {
                case RP_X_XA_START_REQ:
                    IDE_TEST( createNAddGlobalTxNodeToList( &(sXLog.mXID), /* GlobalXID */
                                                            sXLog.mTID, 
                                                            SMI_DTX_NONE,
                                                            &sGlobalTxList,
                                                            ID_TRUE ) 
                              != IDE_SUCCESS );
                    break;

                case RP_X_XA_PREPARE:
                    sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)findGlobalTxNodeFromList( sXLog.mTID, 
                                                                                          &sGlobalTxList );
                    if ( sGlobalTxNode == NULL )
                    {
                        IDE_TEST( createNAddGlobalTxNodeToList( &(sXLog.mXID),
                                                                sXLog.mTID, 
                                                                SMI_DTX_PREPARE,
                                                                &sGlobalTxList,
                                                                ID_FALSE ) 
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        sGlobalTxNode->mResultType = SMI_DTX_PREPARE;
                    }
                    break;

                case RP_X_XA_PREPARE_REQ:
                    break;

                case RP_X_XA_COMMIT:
                case RP_X_ABORT:
                    sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)findGlobalTxNodeFromList( sXLog.mTID, 
                                                                                          &sGlobalTxList );
                    if ( sGlobalTxNode != NULL )
                    {
                        if ( sXLog.mType == RP_X_XA_COMMIT )
                        {
                            sGlobalTxNode->mResultType = SMI_DTX_COMMIT;
                            SM_SET_SCN( &(sGlobalTxNode->mGlobalCommitSCN), &(sXLog.mGlobalCommitSCN) );
                        }
                        else
                        {
                            sGlobalTxNode->mResultType = SMI_DTX_ROLLBACK;
                        }

                        if ( sGlobalTxNode->mIsRequestNode != ID_TRUE )
                        {
                            removeGlobalTxNode( sGlobalTxNode );
                            sGlobalTxNode = NULL;
                        }
                    }
                    break;

                case RP_X_XA_END:
                    sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)findGlobalTxNodeFromList( sXLog.mTID, 
                                                                                          &sGlobalTxList );
                    if ( sGlobalTxNode != NULL )
                    {
                        removeGlobalTxNode( sGlobalTxNode );
                        sGlobalTxNode = NULL;
                    }
                    break;
                default:
                    break;
            }
        }
        sGlobalTxNode = NULL;

        sIsLastXLogInFile = mXLogfileManager->isLastXLog();
    }
    
    sIsInitializedXLog = ID_FALSE;
    rpdQueue::destroyXLog( &sXLog, mAllocator );

    if ( IDU_LIST_IS_EMPTY( &sGlobalTxList ) != ID_TRUE )
    {
        IDU_LIST_JOIN_LIST( aGlobalTxList, &sGlobalTxList );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsInitializedXLog == ID_TRUE )
    {
        rpdQueue::destroyXLog( &sXLog, mAllocator );
    }

    removeGlobalTxList( &sGlobalTxList );

    IDE_ERRLOG( IDE_RP_0 );


    return IDE_FAILURE;
}

IDE_RC rpxReceiver::applyUnCompleteGlobalTxLog( iduList * aGlobalTxList )
{
    rpdXLog    sXLog;
    smSN       sLastXSN           = SM_SN_NULL;
    smSCN      sDummyPrepareSCN;
    iduList    sGlobalTxList;
    idBool     sIsLastXLogInFile  = ID_FALSE;
    idBool     sIsLob             = ID_FALSE;
    idBool     sIsInitializedXLog = ID_FALSE;
    smiTrans * sTrans             = NULL;
    dkiUnCompleteGlobalTxInfo * sGlobalTxNode = NULL;

    IDU_LIST_INIT( &sGlobalTxList );
    
    sIsLob = isLobColumnExist();
    IDE_TEST( rpdQueue::initializeXLog( &sXLog,
                                        getBaseXLogBufferSize( &mMeta ),
                                        sIsLob,
                                        mAllocator )
              != IDE_SUCCESS );
    sIsInitializedXLog = ID_TRUE;

    while ( sIsLastXLogInFile != ID_TRUE )
    {
        IDE_CLEAR();

        rpdQueue::recycleXLog( &sXLog, mAllocator );

        IDE_TEST( receiveAndConvertXLog( &sXLog ) != IDE_SUCCESS );

        sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)findGlobalTxNodeFromList( sXLog.mTID, 
                                                                              aGlobalTxList );
        if ( sGlobalTxNode == NULL )
        {
            /* nothing to do */
        }
        else
        {
            if ( ( sGlobalTxNode->mIsRequestNode != ID_TRUE ) &&
                 ( ( sLastXSN == SM_SN_NULL ) ||
                   ( sLastXSN < sXLog.mSN ) ) )
            {
                sLastXSN = sXLog.mSN;

                switch ( sXLog.mType )
                {
                    case RP_X_XA_PREPARE:
                        SM_INIT_SCN( &sDummyPrepareSCN );
                        sTrans = mApply.getTransTbl()->getSMITrans( sXLog.mTID );
                        IDE_TEST( sTrans->prepare( &(sXLog.mXID),
                                                   &sDummyPrepareSCN,
                                                   ID_TRUE )
                                  != IDE_SUCCESS );

                        sTrans->setStatistics( NULL ); /* For End Pending */
                        
                        sGlobalTxNode->mTrans = mApply.getTransTbl()->getRpdTrans( sXLog.mTID );
                        sTrans = NULL;
                        break;

                    case RP_X_XA_START_REQ:
                    case RP_X_XA_PREPARE_REQ:
                    case RP_X_XA_COMMIT:
                    case RP_X_COMMIT:
                    case RP_X_ABORT:
                        break;

                    default:
                        mApply.apply( &sXLog );
                        break;
                }
            }
        }
        sGlobalTxNode = NULL;

        sIsLastXLogInFile = mXLogfileManager->isLastXLog();
    }
    
    sIsInitializedXLog = ID_FALSE;
    rpdQueue::destroyXLog( &sXLog, mAllocator );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsInitializedXLog == ID_TRUE )
    {
        rpdQueue::destroyXLog( &sXLog, mAllocator );
    }

    IDE_ERRLOG( IDE_RP_0 );

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::createNAddGlobalTxNodeToList( ID_XID        * aXID,
                                                  smTID           aTID,
                                                  smiDtxLogType   aResultType,
                                                  iduList       * aGlobalTxList,
                                                  idBool          aIsRequestNode )
{
    dkiUnCompleteGlobalTxInfo * sGlobalTxNode = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                       ID_SIZEOF(dkiUnCompleteGlobalTxInfo),
                                       (void**)&sGlobalTxNode,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    dkiCopyXID( &(sGlobalTxNode->mXID), aXID );
    sGlobalTxNode->mTID = aTID;
    sGlobalTxNode->mIsRequestNode = aIsRequestNode;
    sGlobalTxNode->mResultType = aResultType;
    SMI_INIT_SCN( &(sGlobalTxNode->mGlobalCommitSCN) );

    IDU_LIST_INIT_OBJ( &(sGlobalTxNode->mNode), sGlobalTxNode );
    IDU_LIST_ADD_LAST( aGlobalTxList, &(sGlobalTxNode->mNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::createNAddGlobalTxNodeToList",
                                  "sGlobalTxNode" ) );
    }
    IDE_EXCEPTION_END;

   return IDE_FAILURE;
}

void rpxReceiver::removeGlobalTxList( iduList * aGlobalTxList )
{
    iduListNode * sNode  = NULL;
    iduListNode * sDummy = NULL;
    dkiUnCompleteGlobalTxInfo * sGlobalTxNode = NULL;

    if ( IDU_LIST_IS_EMPTY( aGlobalTxList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( aGlobalTxList, sNode, sDummy )
        {
            sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)sNode->mObj;
            removeGlobalTxNode( sGlobalTxNode );
            sGlobalTxNode = NULL;
        }
    }
}

void rpxReceiver::removeGlobalTxNode( void * aGlobalTxNode )
{
    IDE_DASSERT( aGlobalTxNode != NULL );

    IDU_LIST_REMOVE( &( ((dkiUnCompleteGlobalTxInfo*)aGlobalTxNode)->mNode ) );
    iduMemMgr::free( aGlobalTxNode );
    aGlobalTxNode = NULL;
}

void * rpxReceiver::findGlobalTxNodeFromList( smTID aTID, iduList * aGlobalTxList )
{
    iduListNode * sNode         = NULL;
    iduListNode * sDummy        = NULL;
    dkiUnCompleteGlobalTxInfo * sGlobalTxNode = NULL;

    if ( aTID != SM_NULL_TID )
    {
        if ( IDU_LIST_IS_EMPTY( aGlobalTxList ) != ID_TRUE )
        {
            IDU_LIST_ITERATE_SAFE( aGlobalTxList, sNode, sDummy )
            {
                sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)sNode->mObj;
                if ( sGlobalTxNode->mTID == aTID )
                {
                    break;
                }
                sGlobalTxNode = NULL;
            }
        }
    }

    return (void*)sGlobalTxNode;
}

/* PROJ-2725
 * CONSISTENT mode는 Transaction 당 Commit ack가 항상 전송되어야 한다.
 * runSync() 내 에서 handshake가 수행될 경우 ack를 전송할 수 없다.
 * 따라서 dummy ack를 만들어 보낸다. */
void rpxReceiver::buildDummyXLogAckForConsistent( rpdXLog * aXLog, rpXLogAck * aAck )
{
    if ( aXLog->mType == RP_X_XA_START_REQ )
    {
        aAck->mAckType = RP_X_XA_START_REQ_ACK;
    }
    else
    {
        aAck->mAckType = RP_X_ACK_WITH_TID;
    }

    aAck->mTID = aXLog->mTID;
    if ( aXLog->mSN == 0 )
    {
        aAck->mLastArrivedSN   = SM_SN_NULL;
        aAck->mLastProcessedSN = SM_SN_NULL;
    }
    else
    {
        aAck->mLastArrivedSN     = aXLog->mSN;
        aAck->mLastProcessedSN   = aXLog->mSN;
    }

    aAck->mAbortTxCount      = 0;
    aAck->mClearTxCount      = 0;
    aAck->mAbortTxList       = NULL;
    aAck->mClearTxList       = NULL;
    aAck->mFlushSN           = SM_SN_NULL;
    aAck->mRestartSN         = SM_SN_NULL;
    aAck->mLastCommitSN      = SM_SN_NULL;
}

IDE_RC rpxReceiver::initializeXLogfileContents( void )
{
    smiTrans          sTrans;
    SInt              sStage = 0;
    idBool            sIsTxBegin = ID_FALSE;
    smiStatement    * spRootStmt;
    smiStatement      sSmiStmt;

    IDE_TEST(sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin(&spRootStmt,
                           NULL,
                           (SMI_ISOLATION_NO_PHANTOM |
                            SMI_TRANSACTION_NORMAL   |
                            SMI_TRANSACTION_REPL_NONE|
                            SMI_COMMIT_WRITE_NOWAIT),
                           SMX_NOT_REPL_TX_ID)
              != IDE_SUCCESS );
    sIsTxBegin = ID_TRUE;
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL, spRootStmt,
                              SMI_STATEMENT_NORMAL |
                              SMI_STATEMENT_MEMORY_CURSOR)
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( initializeXLogfileContents( &sSmiStmt ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );
    sStage = 2;

    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sIsTxBegin = ID_FALSE;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch(sStage)
    {
        case 3:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
            sIsTxBegin = ID_FALSE;

        case 1:
            if(sIsTxBegin == ID_TRUE)
            {
                IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default :
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::getInitLSN( rpXLogLSN * aInitLSN )
{
    rpXLogLSN sXLogLSN = RP_XLOGLSN_INIT;

    switch ( mStartMode )
    {
        case RP_RECEIVER_USING_TRANSFER:
            sXLogLSN = RP_SET_XLOGLSN( mMeta.mReplication.mCurrentReadLSNFromXLogfile.mFileNo,
                                       mMeta.mReplication.mCurrentReadLSNFromXLogfile.mOffset );
            break;

        case RP_RECEIVER_XLOGFILE_RECOVERY:
            if ( mMeta.mReplication.mRemoteXSN != SM_SN_NULL )
            {
                IDE_TEST( rpdXLogfileMgr::getXLogLSNFromXSNAndReplName( mMeta.mReplication.mRemoteXSN,
                                                                        mMeta.mReplication.mRepName,
                                                                        &sXLogLSN )
                          != IDE_SUCCESS );
            }
            else
            {
                sXLogLSN = RP_SET_XLOGLSN( mMeta.mReplication.mCurrentReadLSNFromXLogfile.mFileNo,
                                           mMeta.mReplication.mCurrentReadLSNFromXLogfile.mOffset );
            }
            break;

        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            //BUG-48331
            //xsn -> file no, offset을 찾은 xlog가, file의 가장 처음의 xlog이기 때문에,
            //그 이후, ddl이 들어왔다면, old meta를 참조하지 않는 receiver는 문제가 발생할 수 있다.
            //ex)
            //file header에 offset이 100으로 기록: 가장 처음 정상적인 xlog
            //ddl xlog가 offset 150에 기록(sender측 ddl 수행 로그), 이후 receiver도 ddl 수행하여 meta가 맞혀진 상>황에서,
            //receiver가 다시 시작하여 100부터 읽게되면, 현재 meta와 다른 old meta에 기록 된 내용을 읽게되면서 문>제가 발생함.
            if ( mRemoteMeta->mReplication.mRPRecoverySN != SM_SN_NULL )
            {
                IDE_TEST( rpdXLogfileMgr::getXLogLSNFromXSNAndReplName( mRemoteMeta->mReplication.mRPRecoverySN,
                                                                        mMeta.mReplication.mRepName,
                                                                        &sXLogLSN )
                          != IDE_SUCCESS );
            }
            break;

        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
            IDE_TEST( rpdXLogfileMgr::findRemainFirstXLogLSN( mMeta.mReplication.mRepName,
                                                              &sXLogLSN )
                      != IDE_SUCCESS );
            break;

        default:
            break;
    }

    *aInitLSN = sXLogLSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::initializeXLogfileContents( smiStatement    * aStatement )
{
    rpXLogLSN         sXLogLSN         = RP_XLOGLSN_INIT;
    rpdXLogfileMgr  * sXLogfileManager = NULL;
    rpxXLogTransfer * sXLogTransfer    = NULL;

    IDE_TEST( getInitLSN( &sXLogLSN ) != IDE_SUCCESS );
    switch ( mStartMode )
    {
        case RP_RECEIVER_USING_TRANSFER:
        case RP_RECEIVER_XLOGFILE_RECOVERY:
            IDE_TEST( createAndInitializeXLogfileManager( aStatement,
                                                          &sXLogfileManager, 
                                                          sXLogLSN ) 
                      != IDE_SUCCESS );
            mXLogfileManager = sXLogfileManager;

            IDE_TEST( createAndInitializeXLogTransfer( &sXLogTransfer,
                                                       mXLogfileManager ) 
                      != IDE_SUCCESS );
            mXLogTransfer = sXLogTransfer;
            break;

        case RP_RECEIVER_FAILOVER_USING_XLOGFILE:
        case RP_RECEIVER_XLOGFILE_FAILBACK_MASTER:
            IDE_TEST( createAndInitializeXLogfileManager( aStatement,
                                                          &sXLogfileManager, 
                                                          sXLogLSN ) 
                      != IDE_SUCCESS );
            mXLogfileManager = sXLogfileManager;

            break;
    
        default:
            mXLogfileManager = NULL;
            mXLogTransfer = NULL;
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC  rpxReceiver::waitXlogfileRecoveryDone( idvSQL *aStatistics )
{
    PDL_Time_Value sTvCpu;
    PDL_Time_Value sPDL_Time_Value;
    idBool         sIsLock = ID_FALSE;

    IDE_DASSERT( mStartMode == RP_RECEIVER_XLOGFILE_RECOVERY );

    sPDL_Time_Value.initialize(0, 100000); /* 100 msec*/

    IDE_ASSERT(mXFRecoveryWaitMutex.lock( aStatistics ) == IDE_SUCCESS);
    sIsLock = ID_TRUE;

    while( mXFRecoveryStatus <= RPX_XF_RECOVERY_PROCESS )
    {
        sTvCpu  = idlOS::gettimeofday();
        sTvCpu += sPDL_Time_Value;

        IDU_FIT_POINT( "rpxReceiver::waitXlogfileRecoveryDone::timeout1" );
        IDU_FIT_POINT( "rpxReceiver::waitXlogfileRecoveryDone::timeout2" );
        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

        IDE_TEST( mXFRecoveryWaitCV.timedwait(&mXFRecoveryWaitMutex, &sTvCpu, IDU_IGNORE_TIMEDOUT)
                  != IDE_SUCCESS );
    }

    sIsLock = ID_FALSE;
    IDE_ASSERT(mXFRecoveryWaitMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sIsLock != ID_TRUE)
    {
        IDE_ASSERT(mXFRecoveryWaitMutex.lock( aStatistics ) == IDE_SUCCESS);
    }
    
    if ( mXFRecoveryStatus <= RPX_XF_RECOVERY_PROCESS )
    {
        mXFRecoveryStatus = RPX_XF_RECOVERY_TIMEOUT;
    }

    IDE_ASSERT(mXFRecoveryWaitMutex.unlock() == IDE_SUCCESS);

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::failbackSlaveWithXLogfiles()
{
    PDL_Time_Value sTimeValue;
    rpdSenderInfo       * sSenderInfo;
    UInt                  sSecond = 0;
    idBool                sIsFull = ID_FALSE;
 
    iduMemPool     sBeginSNPool;
    idBool         sBeginSNPoolInit = ID_FALSE;
    iduList        sBeginSNList;

    sTimeValue.initialize(1, 0);

    IDE_DASSERT( mMeta.mReplication.mReplMode == RP_CONSISTENT_MODE );

    IDE_TEST(sBeginSNPool.initialize(IDU_MEM_RP_RPX_RECEIVER,
                                     (SChar *)"RP_BEGIN_SN_POOL",
                                     1,
                                     ID_SIZEOF(rpxSNEntry),
                                     smiGetTransTblSize(),
                                     IDU_AUTOFREE_CHUNK_LIMIT, //chunk max(default)
                                     ID_FALSE,                 //use mutex(no use)
                                     8,                        //align byte(default)
                                     ID_FALSE,				   //ForcePooling
                                     ID_TRUE,				   //GarbageCollection
                                     ID_TRUE,                          /* HWCacheLine */
                                     IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS);			
    sBeginSNPoolInit = ID_TRUE;

    IDU_LIST_INIT(&sBeginSNList);

    /* step 1.  Add RP_SYNC_PK_BEGIN */

    // Sender를 검색하여 SenderInfo를 얻는다.
    // Failback Master가 없으면, 다시 Failback 절차를 수행해야 한다.
    IDE_TEST_RAISE( rpcManager::isAliveSender( mRepName ) != ID_TRUE,
                    ERR_FAILBACK_SENDER_NOT_EXIST );

    sSenderInfo = rpcManager::getSenderInfo(mRepName);
    IDE_TEST_RAISE( sSenderInfo == NULL, ERR_SENDER_INFO_NOT_EXIST );

    // Incremental Sync Primary Key Begin를 Queue에 추가한다.
    while(sSecond < RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
    {
        IDE_TEST( sSenderInfo->addLastSyncPK( RP_SYNC_PK_BEGIN,
                                              0,
                                              0,
                                              NULL,
                                              &sIsFull )
                  != IDE_SUCCESS );
        if(sIsFull != ID_TRUE)
        {
            break;
        }

        idlOS::sleep(sTimeValue);
        sSecond++;
    }

    IDE_TEST_RAISE(sSecond >= RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT,
                   ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);

    // Phase 1 : Commit된 Transaction의 Begin SN을 수집한다.
    IDE_TEST ( readXLogfileAndMakePKList( sSenderInfo,
                                          RP_COLLECT_BEGIN_SN_ON_ADD_XLOG,
                                          &sBeginSNPool,
                                          &sBeginSNList)
               != IDE_SUCCESS );

    finalizeAndDestroyXLogfileManager();
    mXLogfileManager = NULL;

    IDE_TEST( initializeXLogfileContents() != IDE_SUCCESS );
    mReadContext = setReadContext( mProtocolContext, mXLogfileManager );

    // Phase 2 : Commit된 Transaction에서 DML의 Primary Key를 추출하여 전송한다.
    IDE_TEST ( readXLogfileAndMakePKList( sSenderInfo,
                                          RP_SEND_SYNC_PK_ON_ADD_XLOG,
                                          &sBeginSNPool,
                                          &sBeginSNList)
               != IDE_SUCCESS );

    /* step 4. Add RP_SYNC_PK_END */
    // Incremental Sync Primary Key End를 Queue에 추가한다.
   
    sSecond = 0;
    while(sSecond < RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
    {
        IDE_TEST( sSenderInfo->addLastSyncPK( RP_SYNC_PK_END,
                                              0,
                                              0,
                                              NULL,
                                              &sIsFull )
                  != IDE_SUCCESS );
        if(sIsFull != ID_TRUE)
        {
            break;
        }

        idlOS::sleep(sTimeValue);
        sSecond++;
    }
    IDE_TEST_RAISE(sSecond >= RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT,
                   ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
   
    sSenderInfo = NULL; 

    sBeginSNPoolInit = ID_FALSE;
    if(sBeginSNPool.destroy(ID_FALSE) != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_FAILBACK_SENDER_NOT_EXIST);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_SENDER_NOT_EXIST));
    }
    IDE_EXCEPTION( ERR_SENDER_INFO_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SENDER_INFO_NOT_EXIST,
                                  mRepName ) );
    }
    IDE_EXCEPTION(ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED));
    }

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    if(sBeginSNPoolInit == ID_TRUE)
    {
        (void)sBeginSNPool.destroy(ID_FALSE);
    }

    return IDE_FAILURE;
}

IDE_RC rpxReceiver::readXLogfileAndMakePKList( rpdSenderInfo         * aSenderInfo,
                                               RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                               iduMemPool            * aSNPool,
                                               iduList               * aSNList )
{
    rpdXLog     sXLog;
    idBool      sIsLastXLogInFile = ID_TRUE;
    rpdTransTbl  * sTransTbl = NULL;

    rpxSNEntry * sSNEntry = NULL;
   
    idBool      sIsLob = ID_FALSE;

    UInt        sStage = 0;
    
    if ( ( aActionAddXLog == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG ) ||
         ( aActionAddXLog == RP_SEND_SYNC_PK_ON_ADD_XLOG ) )
    {
        IDE_ASSERT( aSNPool != NULL );
    }
    else
    {
        IDE_DASSERT ( 0 ); 
    }

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_RECEIVER,
                                       1,
                                       ID_SIZEOF( rpdTransTbl ),
                                       (void **)&sTransTbl,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_TRANS_TABLE );
    sStage = 1;

    IDE_TEST( sTransTbl->initialize( RPD_TRANSTBL_USING_TRANS_POOL,
                                     RPU_REPLICATION_TRANSACTION_POOL_SIZE )
              != IDE_SUCCESS );
    sStage = 2;

    setReadXLogfileMode();
    mXLogfileManager->setWaitWrittenXLog( ID_FALSE );
    sIsLastXLogInFile = mXLogfileManager->isLastXLog();

    sIsLob = isLobColumnExist();
    IDE_TEST( rpdQueue::initializeXLog( &sXLog,
                                        getBaseXLogBufferSize( &mMeta ),
                                        sIsLob,
                                        mAllocator )
              != IDE_SUCCESS );
    sStage = 3;

    while  ( sIsLastXLogInFile != ID_TRUE )
    {
        IDE_CLEAR();

        rpdQueue::recycleXLog( &sXLog, mAllocator );

        if ( receiveAndConvertXLog( &sXLog ) != IDE_SUCCESS )
        {
            IDE_TEST_CONT( ideGetErrorCode() == rpERR_IGNORE_RPX_END_OF_XLOGFILES, NORMAL_EXIT );
            IDE_TEST( ideGetErrorCode() != rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE );
            continue;
        }

        if( sTransTbl->isATrans(sXLog.mTID) != ID_TRUE)
        {
            switch(sXLog.mType)
            {
                case RP_X_INSERT:
                case RP_X_UPDATE:
                case RP_X_DELETE:
                case RP_X_LOB_CURSOR_OPEN:
                case RP_X_SYNC_INSERT:

                    if ( aActionAddXLog == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG )
                    {
                        IDE_TEST_RAISE( sTransTbl->insertTrans( mAllocator, /* memory allocator : not used */
                                                                sXLog.mTID,
                                                                sXLog.mSN,
                                                                NULL )
                                        != IDE_SUCCESS, ERR_TRANSTABLE_INSERT );
                    }
                    else if ( aActionAddXLog == RP_SEND_SYNC_PK_ON_ADD_XLOG )
                    {
                        sSNEntry = rpcManager::searchSNEntry( aSNList, sXLog.mSN );
                        if ( sSNEntry != NULL )
                        {
                            rpcManager::removeSNEntry( aSNPool, sSNEntry );
                            IDE_TEST_RAISE( sTransTbl->insertTrans( mAllocator, /* memory allocator : not used */
                                                                    sXLog.mTID,
                                                                    sXLog.mSN,
                                                                    NULL )
                                            != IDE_SUCCESS, ERR_TRANSTABLE_INSERT );
                            sSNEntry = NULL;
                            
                            IDE_TEST( addLastSyncPK( aSenderInfo,
                                                     &sXLog )
                                      != IDE_SUCCESS );
                        }

                    }
                    break;
                default:
                    break;
            }
        }
        else
        {   
            switch(sXLog.mType)
            { 
                case RP_X_INSERT:
                case RP_X_UPDATE:
                case RP_X_DELETE:
                case RP_X_LOB_CURSOR_OPEN:
                case RP_X_SYNC_INSERT:
                    if ( aActionAddXLog == RP_SEND_SYNC_PK_ON_ADD_XLOG )
                    {
                        IDE_TEST( addLastSyncPK( aSenderInfo,
                                                 &sXLog )
                                  != IDE_SUCCESS );
                    }
                    break;

                case RP_X_COMMIT:
                case RP_X_XA_COMMIT:
                    if ( aActionAddXLog == RP_COLLECT_BEGIN_SN_ON_ADD_XLOG )
                    {
                        IDE_TEST( rpcManager::addLastSNEntry( aSNPool,
                                                              sTransTbl->getTrNode( sXLog.mTID )->mBeginSN,
                                                              aSNList )
                                  != IDE_SUCCESS );
                    }
                    /* fall through */

                case RP_X_ABORT:
                    sTransTbl->removeTrans( sXLog.mTID );
                    break;

                default:
                    break;
            }
        }
    
        sIsLastXLogInFile = mXLogfileManager->isLastXLog();
    }

    RP_LABEL(NORMAL_EXIT);
    IDE_CLEAR();
  
    mXLogfileManager->setWaitWrittenXLog( ID_TRUE );
    setReadNetworkMode();

    rpdQueue::destroyXLog( &sXLog, mAllocator );
    sTransTbl->rollbackAllATrans();
    sTransTbl->destroy();
    (void)iduMemMgr::free( sTransTbl );
    sTransTbl = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_TRANS_TABLE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::readXLogfileAndMakePKList",
                                  "sTransTbl" ) );
    }
    IDE_EXCEPTION( ERR_TRANSTABLE_INSERT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_TRANSACTION_TABLE_IN_INSERT ) );
    }

    IDE_EXCEPTION_END;
    
    IDE_ERRLOG( IDE_RP_0 );
    
    IDE_PUSH();

    mXLogfileManager->setWaitWrittenXLog( ID_TRUE );
    setReadNetworkMode();
    
    switch ( sStage )
    {
        case 3:
            rpdQueue::destroyXLog( &sXLog, mAllocator );
        case 2:
            sTransTbl->rollbackAllATrans();
            sTransTbl->destroy();
        case 1:
            (void)iduMemMgr::free( sTransTbl );
            sTransTbl = NULL;
        default:
            break;
    }
    IDE_POP();
    
    return IDE_FAILURE;
}

IDE_RC rpxReceiver::addLastSyncPK( rpdSenderInfo  * aSenderInfo,
                                   rpdXLog        * aXLog )
{
    UInt           sPKColCnt;
    SInt           sColID;
    smiValue       sPKCols[QCI_MAX_KEY_COLUMN_COUNT];
    qcmIndex     * sPKIndex = NULL;
    UInt           sPKCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    SChar          sValueData[QD_MAX_SQL_LENGTH + 1] = { 0, };

    rpdMetaItem  * sMetaItem = NULL;
    rpdMetaItem  * sRemoteMetaItem = NULL;
    SInt        sIndex;
    UInt        i;
    
    UInt        sSecond = 0;
    idBool      sIsFull = ID_FALSE;
    idBool      sIsValid = ID_FALSE;
    idBool      sIsSyncPK = ID_FALSE;

    PDL_Time_Value   sTimeValue;
    sTimeValue.initialize(1, 0);

    IDE_TEST( mRemoteMeta->searchTable( &sRemoteMetaItem,
                                        aXLog->mTableOID )
              != IDE_SUCCESS );

    switch(aXLog->mType)
    {
        case RP_X_INSERT:
            // PK Column Count를 얻는다.
            sPKColCnt = sRemoteMetaItem->mPKColCount;

            // PK Index를 검색한다.
            for ( sIndex = 0; sIndex < sRemoteMetaItem->mIndexCount; sIndex++ )
            {
                if ( sRemoteMetaItem->mPKIndex.indexId
                     == sRemoteMetaItem->mIndices[sIndex].indexId )
                {
                    sPKIndex = &sRemoteMetaItem->mIndices[sIndex];
                    break;
                }
            }
            IDE_ASSERT( sPKIndex != NULL );

            // PK Column Value와 MT Length를 얻는다.
            idlOS::memset( &sPKCols,
                           0x00,
                           ID_SIZEOF( smiValue ) * QCI_MAX_KEY_COLUMN_COUNT );

            for ( sIndex = 0; sIndex < (SInt)sPKColCnt; sIndex++ )
            {
                sColID = sPKIndex->keyColumns[sIndex].column.id
                    & SMI_COLUMN_ID_MASK;
                IDE_ASSERT( sColID < sRemoteMetaItem->mColCount );

                // sXLog.mMemory 는 recycle 되면서 날라가기 때문에 &sXLog.mACols[sColID].value 는 recycle 과 함께 날라간다.
                // 즉 PKCols[sIndex].value  에 length 만큼 할당 받고, 여기에 sXLog->mACols[sColID].value 를 복사한다.
                // 해제는 removePKColEntry 할때 알아서 한다.
                // same as rpnComm::recvValueA7()
                IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                                   aXLog->mACols[sColID].length,
                                                   (void**)&(sPKCols[sIndex].value),
                                                   IDU_MEM_IMMEDIATE )
                                != IDE_SUCCESS, ERR_MEMORY_ALLOC_PKCOLS );

                (void)idlOS::memcpy( (void*)sPKCols[sIndex].value,
                                     (void*)aXLog->mACols[sColID].value,
                                     aXLog->mACols[sColID].length );
                sPKCols[sIndex].length = aXLog->mACols[sColID].length;
            }
            sIsSyncPK = ID_TRUE;
            break;

        case RP_X_UPDATE :  // 이미 Primary Key가 있으므로, 복사한다.
        case RP_X_DELETE :
        case RP_X_LOB_CURSOR_OPEN :
            sPKColCnt = aXLog->mPKColCnt;

            idlOS::memset( &sPKCols,
                           0x00,
                           ID_SIZEOF( smiValue ) * QCI_MAX_KEY_COLUMN_COUNT );

            // rpnCommA7 3220 line up delete 는 id 상관없이 순서대로 받는다.
            for ( sIndex = 0; sIndex < (SInt)sPKColCnt; sIndex++ )
            {
                IDE_DASSERT( aXLog->mPKCols[sIndex].length > 0 );

                IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX,
                                                   aXLog->mPKCols[sIndex].length,
                                                   (void**)&(sPKCols[sIndex].value),
                                                   IDU_MEM_IMMEDIATE )
                                != IDE_SUCCESS, ERR_MEMORY_ALLOC_PKCOLS );

                (void)idlOS::memcpy( (void*)sPKCols[sIndex].value,
                                     (void*)aXLog->mPKCols[sIndex].value,
                                     aXLog->mPKCols[sIndex].length );
                sPKCols[sIndex].length = aXLog->mPKCols[sIndex].length;
            }

            sIsSyncPK = ID_TRUE;
            break;

        default :
            sIsSyncPK = ID_FALSE;
            break;

    }

    if ( sIsSyncPK != ID_FALSE )
    {
        // check existence of TABLE
        IDE_TEST( searchRemoteTable( &sMetaItem,
                                     aXLog->mTableOID )
                  != IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPK::Erratic::rpERR_ABORT_NOT_EXIST_TABLE",
                             ERR_NOT_EXIST_TABLE ); 
        IDE_TEST_RAISE(sMetaItem == NULL, ERR_NOT_EXIST_TABLE);

        IDU_FIT_POINT_RAISE( "rpxReceiverApply::applySyncPK::Erratic::rpERR_ABORT_COLUMN_COUNT_MISMATCH",
                             ERR_COL_COUNT_MISMATCH );
        IDE_TEST_RAISE(sPKColCnt != sMetaItem->mPKIndex.keyColCount,
                       ERR_COL_COUNT_MISMATCH);

        // PK 정보를 로그에 기록한다.

        for ( i = 0; i < sMetaItem->mPKIndex.keyColCount; i++ )
        {
            sPKCIDArray[i] = sMetaItem->mPKIndex.keyColumns[i].column.id;
        }

        rpdConvertSQL::getColumnListClause( sMetaItem,
                                            sMetaItem,
                                            sMetaItem->mPKIndex.keyColCount,
                                            sPKCIDArray,
                                            sMetaItem->mMapColID,
                                            sPKCols,
                                            ID_TRUE,
                                            ID_TRUE,
                                            ID_TRUE,
                                            (SChar*)",",
                                            sValueData,
                                            QD_MAX_SQL_LENGTH + 1,
                                            &sIsValid );

        ideLog::log( IDE_RP_0, "[Receiver] Incremental Sync Primary Key" 
                     " User Name=%s, Table Name=%s, PK=[%s]",
                     sMetaItem->mItem.mLocalUsername,
                     sMetaItem->mItem.mLocalTablename,
                     sValueData );

        // Incremental Sync Primary Key를 Queue에 추가한다.
        while(sSecond < RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT)
        {
            IDE_TEST( aSenderInfo->addLastSyncPK(RP_SYNC_PK,
                                                 sMetaItem->mItem.mTableOID,
                                                 sPKColCnt,
                                                 sPKCols,
                                                 &sIsFull)
                      != IDE_SUCCESS);
            if(sIsFull != ID_TRUE)
            {
                break;
            }

            idlOS::sleep(sTimeValue);
            sSecond++;
        }

        IDE_TEST_RAISE(sSecond >= RPU_REPLICATION_FAILBACK_PK_QUEUE_TIMEOUT,
                       ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_PKCOLS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxReceiver::readXLogfileAndMakePKList",
                                  "sPKCols" ) );
    }

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        ideLog::log(IDE_RP_0, RP_TRC_RA_ERR_INVALID_XLOG,
                              aXLog->mType,
                              aXLog->mTID,
                              aXLog->mSN,
                              aXLog->mTableOID);

        IDE_SET(ideSetErrorCode(rpERR_ABORT_NOT_EXIST_TABLE));
    }
    IDE_EXCEPTION(ERR_COL_COUNT_MISMATCH);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_COLUMN_COUNT_MISMATCH,
                                "[Sync PK] PK",
                                sMetaItem->mItem.mLocalTablename,
                                sPKColCnt,
                                sMetaItem->mPKIndex.keyColCount));
    }
    IDE_EXCEPTION(ERR_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_FAILBACK_PK_QUEUE_TIMEOUT_EXCEED));
    }


    IDE_EXCEPTION_END;
 
    IDE_PUSH();

    // sPKCols[i].value 메모리를 해제한다.
    for(i = 0; i < QCI_MAX_KEY_COLUMN_COUNT; i++)
    {
        if(sPKCols[i].value != NULL)
        {
            (void)iduMemMgr::free((void *)sPKCols[i].value);
        }
    }
   
    IDE_POP();
    
    return IDE_FAILURE;
}

idBool rpxReceiver::useNetworkResourceMode()

{
    idBool sUseNetwork = ID_FALSE;

    switch( mStartMode )
    {
        case RP_RECEIVER_XLOGFILE_RECOVERY :
        case RP_RECEIVER_FAILOVER_USING_XLOGFILE :
            sUseNetwork = ID_FALSE;
            break;

        default :
            sUseNetwork = ID_TRUE;
            break;
    }

    return sUseNetwork;
}

