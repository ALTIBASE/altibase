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
 

#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smiDef.h>
#include <rpdSenderInfo.h>
#include <rpuProperty.h>
#include <rpxSender.h>

/***********************************************************************
 * Function    : initialize
 * Description : rpdSenderInfo의 initialize와 destroy는 rpcExecutor에
 *               의해서 호출되며, 즉 Executor의 생명 주기와 동일하고
 *               deActivate, activate는 Sender에 의해서 호출된다.
 *               SenderApply가 생성되면서 새로 activate시키며,
 *               종료할 때 deActivate한다.
 **********************************************************************/
IDE_RC rpdSenderInfo::initialize(UInt aSenderListIdx)
{
    SChar  sName[IDU_MUTEX_NAME_LEN];

    mLastProcessedSN        = SM_SN_NULL;
    mLastArrivedSN          = SM_SN_NULL;
    mRestartSN              = SM_SN_NULL;
    mRmtLastCommitSN        = SM_SN_NULL;
    mIsSenderSleep          = ID_FALSE;
    mActiveTransTbl         = NULL;
    mAssignedTransTable     = NULL;
    mIsActive               = ID_FALSE;
    mReplMode               = RP_USELESS_MODE;
    mOriginReplMode         = RP_USELESS_MODE;
    mRole                   = RP_ROLE_REPLICATION;

    mSenderStatus           = RP_SENDER_STOP;
    mIsPeerFailbackEnd      = ID_FALSE;
    mSyncPKListMaxSize      = smiGetTransTblSize() - 1;
    mSyncPKListCurSize      = 0;
    mIsSyncPKPoolAllocated  = ID_FALSE;
    mIsRebuildIndex         = ID_FALSE;
    mIsTruncate             = ID_FALSE;
    mReconnectCount         = 0;
    mTransTableSize         = smiGetTransTblSize();
    mIsSkipUpdateXSN        = ID_FALSE;
    mIsWaitOnFailback       = ID_FALSE;

    (void)idlOS::memset(mRepName, 0x00, QCI_MAX_NAME_LEN + 1);

    //------------------------------------------------------------------------//
    // 실패하면, 서버 비정상 종료를 발생시키는 작업들
    //------------------------------------------------------------------------//
    IDE_TEST_RAISE(mServiceWaitCV.initialize() != IDE_SUCCESS,
                   ERR_COND_INIT);

    IDE_TEST_RAISE(mSenderWaitCV.initialize() != IDE_SUCCESS,
                   ERR_COND_INIT);

    idlOS::snprintf(sName,
                    IDU_MUTEX_NAME_LEN,
                    "REPL_SENDER_SERVICE_WAIT_SN_%"ID_UINT32_FMT"_MUTEX",
                    aSenderListIdx);

    IDE_TEST_RAISE(mServiceSNMtx.initialize(sName,
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    idlOS::snprintf(sName,
                    IDU_MUTEX_NAME_LEN,
                    "REPL_SENDER_SN_%"ID_UINT32_FMT"_MUTEX",
                    aSenderListIdx);

    IDE_TEST_RAISE(mSenderSNMtx.initialize(sName,
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    idlOS::snprintf(sName,
                    IDU_MUTEX_NAME_LEN,
                    "REPL_SENDER_ACTIVE_TRANSACTION_TABLE_%"
                    ID_UINT32_FMT"_MUTEX",
                    aSenderListIdx);

    IDE_TEST_RAISE(mActTransTblMtx.initialize(sName,
                                             IDU_MUTEX_KIND_POSIX,
                                             IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    idlOS::snprintf(sName,
                    IDU_MUTEX_NAME_LEN,
                    "REPL_SENDER_SYNC_PK_%"ID_UINT32_FMT"_MUTEX",
                    aSenderListIdx);

    IDE_TEST_RAISE(mSyncPKMtx.initialize(sName,
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);

    IDU_LIST_INIT(&mSyncPKList);

    idlOS::snprintf( sName,
                     IDU_MUTEX_NAME_LEN,
                     "REPL_SENDER_NAME_%"ID_UINT32_FMT"_MUTEX",
                     aSenderListIdx );

    IDE_TEST_RAISE( mRepNameMtx.initialize( sName,
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    idlOS::snprintf( sName,
                     IDU_MUTEX_NAME_LEN,
                     "REPL_ASSIGNED_TRANS_TBL__%"ID_UINT32_FMT"_MUTEX",
                     aSenderListIdx );
    IDE_TEST_RAISE( mAssignedTransTableMutex.initialize( sName,
                                                         IDU_MUTEX_KIND_POSIX,
                                                         IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    IDU_FIT_POINT( "rpdSenderInfo::initialize::malloc::LastProcessedSNTable" );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                 mTransTableSize * ID_SIZEOF( rpdLastProcessedSNNode ),
                                 (void**)&mLastProcessedSNTable,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    
    initializeLastProcessedSNTable();

    mSenderListIndex = RPX_INVALID_SENDER_INDEX;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MUTEX_INIT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrMutexInit));
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] Mutex initialization error");
    }
    IDE_EXCEPTION(ERR_COND_INIT);
    {
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrCondInit));
        IDE_ERRLOG(IDE_RP_0);

        IDE_CALLBACK_FATAL("[Repl] idlOS::cond_init() error");
    }
    IDE_EXCEPTION_END;

    (void)mSenderWaitCV.destroy();
    (void)mServiceWaitCV.destroy();
    (void)mServiceSNMtx.destroy();
    (void)mSenderSNMtx.destroy();
    (void)mActTransTblMtx.destroy();
    (void)mSyncPKMtx.destroy();
    (void)mRepNameMtx.destroy();
    (void)mAssignedTransTableMutex.destroy();

    if ( mLastProcessedSNTable != NULL )
    {
        (void)iduMemMgr::free( mLastProcessedSNTable );
        mLastProcessedSNTable = NULL;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_FAILURE;
}

/**
 * @breif  Sync PK Pool을 초기화한다.
 *
 * @param  aReplName Replication Name
 *
 * @return 작업 성공/실패
 */
IDE_RC rpdSenderInfo::initSyncPKPool( SChar * aReplName )
{
    SChar  sName[IDU_MUTEX_NAME_LEN];
    idBool sIsMutexLock = ID_FALSE;

    idlOS::snprintf( sName,
                     IDU_MUTEX_NAME_LEN,
                     "REPL_SYNC_PK_POOL_%s",
                     aReplName );

    IDE_ASSERT( mSyncPKMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsMutexLock = ID_TRUE;

    if ( mIsSyncPKPoolAllocated == ID_FALSE )
    {
        IDU_FIT_POINT( "rpdSenderInfo::initSyncPKPool::lock::initialize" );
        IDE_TEST( mSyncPKPool.initialize( IDU_MEM_RP_RPD,
                                          sName,
                                          1,
                                          ID_SIZEOF( rpdSyncPKEntry ),
                                          mSyncPKListMaxSize,
                                          IDU_AUTOFREE_CHUNK_LIMIT, /* holding chunk count(default 5) */
                                          ID_FALSE,                 /* use mutex(no use)   */
                                          8,                        /* align byte(default) */
                                          ID_FALSE,					/* ForcePooling */
                                          ID_TRUE,					/* GarbageCollection */
                                          ID_TRUE,                  /* HWCacheLine */
                                          IDU_MEMPOOL_TYPE_LEGACY   /* mempool type */) 
                 != IDE_SUCCESS);			

        mIsSyncPKPoolAllocated = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    sIsMutexLock = ID_FALSE;
    IDE_ASSERT( mSyncPKMtx.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsMutexLock == ID_TRUE )
    {
        IDE_ASSERT( mSyncPKMtx.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/**
 * @breif  Sync PK List를 비우고, Sync PK Pool을 제거한다.
 *
 * @param  aSkip Sync PK List가 비어있지 않을 때, 작업을 건너뛸지 여부
 *
 * @return 작업 성공/실패
 */
IDE_RC rpdSenderInfo::destroySyncPKPool( idBool aSkip )
{
    idBool        sIsMutexLock = ID_FALSE;
    iduListNode * sNode;
    iduListNode * sDummy;

    IDE_ASSERT( mSyncPKMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsMutexLock = ID_TRUE;

    IDE_TEST_CONT( mIsSyncPKPoolAllocated == ID_FALSE, NORMAL_EXIT );

    IDE_TEST_CONT( ( aSkip == ID_TRUE ) &&
                    ( IDU_LIST_IS_EMPTY( &mSyncPKList ) == ID_FALSE ),
                    NORMAL_EXIT );

    IDU_LIST_ITERATE_SAFE( &mSyncPKList, sNode, sDummy )
    {
        IDU_LIST_REMOVE( sNode );
        (void)mSyncPKPool.memfree( sNode->mObj );
        mSyncPKListCurSize--;
    }
    IDE_DASSERT( mSyncPKListCurSize == 0 );

    IDU_FIT_POINT( "rpdSenderInfo::destroySyncPKPool::lock::destroy" );
    IDE_TEST( mSyncPKPool.destroy( ID_FALSE ) != IDE_SUCCESS );
    mIsSyncPKPoolAllocated = ID_FALSE;

    RP_LABEL( NORMAL_EXIT );

    sIsMutexLock = ID_FALSE;
    IDE_ASSERT( mSyncPKMtx.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsMutexLock == ID_TRUE )
    {
        IDE_ASSERT( mSyncPKMtx.unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

void rpdSenderInfo::destroy()
{
    if(mSenderWaitCV.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mServiceWaitCV.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mServiceSNMtx.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mSenderSNMtx.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if(mActTransTblMtx.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if ( destroySyncPKPool( ID_FALSE /* idBool aSkip */) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    if(mSyncPKMtx.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if ( mRepNameMtx.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    if ( mAssignedTransTableMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    if ( mLastProcessedSNTable != NULL )
    {
        (void)iduMemMgr::free( mLastProcessedSNTable );
        mLastProcessedSNTable = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return;
}
/***********************************************************************
 * Function    : finalize
 * Description : Sender는 SenderApply가 시작되기 전에 RestartSN을 Set하며,
 *               SenderApply가 종료되더라도 살아있고, SenderInfo의
 *               mRestartSN을 사용한다. 때문에 SenderApply가 없는 상황에서
 *               Sender가 mRestartSN을 사용할 수 있도록 하기 위해 Sender가
 *               시작하면서 XSN을 초기화 할 때 RestartSN을 set하고, Sender가
 *               종료될 때에만 mRestartSN을 초기화한다.
 **********************************************************************/
void rpdSenderInfo::finalize()
{
    iduListNode * sNode;
    iduListNode * sDummy;

    IDE_ASSERT( mAssignedTransTableMutex.lock( NULL ) == IDE_SUCCESS );
    IDE_ASSERT(mActTransTblMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    initializeLastProcessedSNTable();

    mLastProcessedSN      = SM_SN_NULL;
    mLastArrivedSN        = SM_SN_NULL;
    mIsSenderSleep        = ID_FALSE;
    mRmtLastCommitSN      = SM_SN_NULL;
    mActiveTransTbl       = NULL;
    mAssignedTransTable   = NULL;
    mReplMode             = RP_USELESS_MODE;
    mIsActive             = ID_FALSE;
    mRestartSN            = SM_SN_NULL;
    mSenderStatus         = RP_SENDER_STOP;
    mIsSkipUpdateXSN      = ID_FALSE;
    mSenderListIndex      = RPX_INVALID_SENDER_INDEX;

    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT( mAssignedTransTableMutex.unlock() == IDE_SUCCESS );

    mIsPeerFailbackEnd    = ID_FALSE;

    IDE_ASSERT(mSyncPKMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    IDU_LIST_ITERATE_SAFE(&mSyncPKList, sNode, sDummy)
    {
        IDU_LIST_REMOVE(sNode);
        (void)mSyncPKPool.memfree(sNode->mObj);
        mSyncPKListCurSize--;
    }
    IDE_DASSERT(mSyncPKListCurSize == 0);

    IDE_ASSERT(mSyncPKMtx.unlock() == IDE_SUCCESS);

    mThroughput           = 0;

    return;
}

/*
 * PROJ-2725 consistent replication
 *
 */
void rpdSenderInfo::checkAndRunDeactivate()
{
    if ( mReplMode != RP_CONSISTENT_MODE )
    {
        deactivate();
    }
    else
    {
        /* PROJ-2725
         * consistent mode의 senderInfo는 replication stop에서만 deactivate 된다.
         * 이는, senderInfo::finalize() 이다.
         */
    }
}

/***********************************************************************
 * Function    : deactivate, Activate
 * Description : checkAndRunDeactivate, activate는 Sender에 의해서 호출된다.
 *               SenderApply가 생성되면서 새로 activate시키며, 종료할 때
 *               deActivate가 호출된다.
 **********************************************************************/
void rpdSenderInfo::deactivate()
{
    //Exit(),Sync(),Disconnect()인 경우 mIsActive를 ID_FALSE로 set한다.
    IDE_ASSERT( mAssignedTransTableMutex.lock( NULL ) == IDE_SUCCESS );
    IDE_ASSERT(mActTransTblMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    //Service thread가 Sender 오류 상황에 대기하지 않도록하기 위해서
    mLastProcessedSN      = SM_SN_NULL;
    mLastArrivedSN        = SM_SN_NULL;

    mIsSenderSleep        = ID_FALSE;
    //Flush 명령이 Sender 오류 상황에 대기하지 않도록 하기 위해서
    mRmtLastCommitSN      = SM_SN_NULL;

    mActiveTransTbl       = NULL;
    mAssignedTransTable   = NULL;
    mSenderListIndex      = RPX_INVALID_SENDER_INDEX;
    mReplMode             = RP_USELESS_MODE;
    mRole                 = RP_ROLE_REPLICATION;
    mIsActive             = ID_FALSE;
    mIsSkipUpdateXSN      = ID_FALSE;

    //service thread wakeup
    IDE_ASSERT(mServiceWaitCV.broadcast() == IDE_SUCCESS);

    initializeLastProcessedSNTable();

    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT( mAssignedTransTableMutex.unlock() == IDE_SUCCESS );

    return;
}

void rpdSenderInfo::activate(rpdTransTbl *aActTransTbl,
                             smSN         aRestartSN,
                             SInt         aReplMode,
                             SInt         aRole,
                             UInt         aSenderListIdx,
                             rpxSender ** aAssignedTransTable )
{
    IDE_ASSERT( mAssignedTransTableMutex.lock( NULL ) == IDE_SUCCESS );
    IDE_ASSERT(mActTransTblMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    mLastProcessedSN    = aRestartSN;
    mLastArrivedSN      = aRestartSN;
    mRestartSN          = aRestartSN;
    mIsSenderSleep      = ID_FALSE;
    mRmtLastCommitSN    = 0;
    mReplMode           = aReplMode;
    mRole               = aRole;
    mIsActive           = ID_TRUE;
    mActiveTransTbl     = aActTransTbl;
    mAssignedTransTable = aAssignedTransTable;
    mIsSkipUpdateXSN    = ID_FALSE;
    mSenderListIndex    = aSenderListIdx;

    initializeLastProcessedSNTable();

    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
    IDE_ASSERT( mAssignedTransTableMutex.unlock() == IDE_SUCCESS );

    return;
}

smSN rpdSenderInfo::getRestartSN()
{
    smSN sRestartSN;

    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    sRestartSN = mRestartSN;
    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);

    return sRestartSN;
}

smSN rpdSenderInfo::getRmtLastCommitSN()
{
    smSN sRmtLastCommitSN = SM_SN_NULL;

    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    if(mIsActive == ID_TRUE)
    {
        sRmtLastCommitSN = mRmtLastCommitSN;
    }
    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);

    return sRmtLastCommitSN;
}

void rpdSenderInfo::setRmtLastCommitSN(smSN aRmtLastCommitSN)
{
    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    if(mIsActive == ID_TRUE)
    {
        if(aRmtLastCommitSN != SM_SN_NULL)
        {
            mRmtLastCommitSN = aRmtLastCommitSN;
        }
    }
    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);

    return;
}

smSN rpdSenderInfo::getLastProcessedSN()
{
    smSN sLastProcessedSN = SM_SN_NULL;

    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    if(mIsActive == ID_TRUE)
    {
        sLastProcessedSN = mLastProcessedSN;
    }
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);

    return sLastProcessedSN;
}

smSN rpdSenderInfo::getLastArrivedSN()
{
    smSN sLastArrivedSN = SM_SN_NULL;

    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    if(mIsActive == ID_TRUE)
    {
        sLastArrivedSN = mLastArrivedSN;
    }
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);

    return sLastArrivedSN;
}

/***********************************************************************
 * Function    : setRestartSN
 * Description : SenderApply가 종료되고 deactive된 경우에도 Sender에서
 *               restartSN을 보기 때문에 mIsActive를 체크하지 않는다.
 **********************************************************************/
void rpdSenderInfo::setRestartSN(smSN aRestartSN)
{
    IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    mRestartSN = aRestartSN;
    IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);

    return;
}

void rpdSenderInfo::setAckedValue( smSN     aLastProcessedSN,
                                   smSN     aLastArrivedSN,
                                   smSN     aAbortTxCount,
                                   rpTxAck *aAbortTxList,
                                   smSN     aClearTxCount,
                                   rpTxAck *aClearTxList,
                                   smTID    aTID )
{
    setAbortTransaction(aAbortTxCount, aAbortTxList);
    setClearTransaction(aClearTxCount, aClearTxList);

    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    if(mIsActive == ID_TRUE)
    {
        if(aLastProcessedSN != SM_SN_NULL)
        {
            mLastProcessedSN = aLastProcessedSN;
        }
        else
        {
            /* Nothing to do */
        }

        if(aLastArrivedSN != SM_SN_NULL)
        {
            mLastArrivedSN = aLastArrivedSN;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aTID != SM_NULL_TID )
        {
            IDE_DASSERT( aLastArrivedSN != SM_SN_NULL );
            setAckedSN( aTID, aLastArrivedSN );
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);

    return;
}

idBool rpdSenderInfo::isReplicationDDLTrans(smTID    aTID)
{
    idBool sIsDDLTrans = ID_FALSE;

    IDE_ASSERT(mActTransTblMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    //Exit(),Sync(),Disconnect()인 경우 skip한다.
    if((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE))
    {
        if(mActiveTransTbl->isATrans(aTID) == ID_TRUE)
        {
            sIsDDLTrans = mActiveTransTbl->isDDLTrans(aTID);
        }
        else
        {
            sIsDDLTrans = ID_FALSE;
        }
    }
    else
    {
        sIsDDLTrans = ID_FALSE;
    }
    IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);

    return sIsDDLTrans;
}

void rpdSenderInfo::setSkipUpdateXSN( idBool aIsSkip )
{
    IDE_ASSERT( mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS );
    mIsSkipUpdateXSN = aIsSkip;
    IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );
}

idBool rpdSenderInfo::getSkipUpdateXSN()
{
    idBool sIsSkipUpdateXSN= ID_FALSE;

    IDE_ASSERT( mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS );
    sIsSkipUpdateXSN = mIsSkipUpdateXSN;
    IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );

    return sIsSkipUpdateXSN;
}

void rpdSenderInfo::serviceWaitForNetworkError( void )
{
    if ( ( mOriginReplMode == RP_EAGER_MODE ) &&
         ( ( mSenderStatus != RP_SENDER_FAILBACK_EAGER ) &&
           ( mSenderStatus != RP_SENDER_FLUSH_FAILBACK ) &&
           ( mSenderStatus != RP_SENDER_IDLE ) ) &&
         ( RPU_REPLICATION_RECONNECT_MAX_COUNT > 0 ) )
    {
        while ( mReconnectCount <= RPU_REPLICATION_RECONNECT_MAX_COUNT )
        {
            if ( ( mIsActive == ID_TRUE ) && 
                 ( ( mSenderStatus == RP_SENDER_FAILBACK_EAGER ) ||
                     ( mSenderStatus == RP_SENDER_FLUSH_FAILBACK ) ||
                     ( mSenderStatus == RP_SENDER_IDLE ) ) &&
                 ( mReplMode == RP_EAGER_MODE ) )
            {
                break;
            }
            else if( mOriginReplMode != RP_EAGER_MODE )
            {
                break;
            }
            else
            {
                idlOS::sleep( 1 );
            }
        }
    }
    else
    {
        /* Nothing to do */
    }
}

/***********************************************************************
 * Description : eager/acked mode에서 service thr가 서비스 중인 트랜잭션의
 *               마지막 로그인 aLastSN을 받아 remote 에서 처리 여부를 확인하고
 *               처리가 되지 않았으면 대기한다.
 * return value: sender info가 acitve인지 아닌지 반환한다.
 **********************************************************************/
idBool rpdSenderInfo::serviceWaitBeforeCommit(smSN    aLastSN,
                                              UInt    aTxReplMode,
                                              smTID   aTransID,
                                              UInt    aRequestWaitMode,
                                              idBool *aWaitedLastSN)
{
    UInt   sMode;
    smSN   sDummySN = 0;

    *aWaitedLastSN = ID_FALSE;

    IDE_DASSERT(aTxReplMode != RP_LAZY_MODE);

    //트랜잭션이 지정한 Replication Mode가 있을 경우
    if((aTxReplMode != RP_DEFAULT_MODE))
    {
        sMode = aTxReplMode;
    }
    else //기본 모드
    {
        sMode = mReplMode;
    }

    IDE_TEST_CONT( ( sMode != RP_EAGER_MODE ) ||
                   ( mRole == RP_ROLE_ANALYSIS ) || 
                   ( mRole == RP_ROLE_ANALYSIS_PROPAGATION ),
                   NORMAL_EXIT );

    checkAndWaitToApply( aTransID,
                         sDummySN,
                         aLastSN,
                         ID_FALSE,
                         aRequestWaitMode ,
                         aWaitedLastSN );

    RP_LABEL(NORMAL_EXIT);

    return mIsActive;
}

void rpdSenderInfo::serviceWaitAfterCommit( smSN aBeginSN,
                                            smSN aLastSN,
                                            UInt aTxReplMode,
                                            smTID aTransID,
                                            UInt aRequestWaitMode )
{
    UInt    sMode;
    idBool  sDummy = ID_FALSE;

    IDE_DASSERT(aTxReplMode != RP_LAZY_MODE);

    //트랜잭션이 지정한 Replication Mode가 있을 경우
    if((aTxReplMode != RP_DEFAULT_MODE))
    {
        sMode = aTxReplMode;
    }
    else //기본 모드
    {
        sMode = mReplMode;
    }
    IDE_TEST_CONT( ( sMode != RP_EAGER_MODE ) && ( sMode != RP_CONSISTENT_MODE ),
                   NORMAL_EXIT );

    IDE_TEST_CONT( ( mRole == RP_ROLE_ANALYSIS ) ||
                   ( mRole == RP_ROLE_ANALYSIS_PROPAGATION ),
                   NORMAL_EXIT );

    checkAndWaitToApply( aTransID,
                         aBeginSN,
                         aLastSN,
                         ID_FALSE,
                         aRequestWaitMode,
                         &sDummy );

    removeAssignedSender( aTransID );
    
    RP_LABEL(NORMAL_EXIT);

    return;
}

void rpdSenderInfo::serviceWaitAfterPrepare( smSN   aLastSN, 
                                             smTID  aTransID,
                                             idBool aIsRequestNode )
{
    idBool  sDummy = ID_FALSE;

    IDE_TEST_CONT( ( mRole == RP_ROLE_ANALYSIS ) ||
                   ( mRole == RP_ROLE_ANALYSIS_PROPAGATION ),
                   NORMAL_EXIT );

    checkAndWaitToApply( aTransID,
                         SM_SN_NULL, // not use
                         aLastSN,
                         aIsRequestNode,
                         RP_CONSISTENT_MODE,
                         &sDummy );

    RP_LABEL(NORMAL_EXIT);

    return;
}


void rpdSenderInfo::wakeupEagerSender()
{
    if(mIsSenderSleep == ID_TRUE)
    {
        IDE_ASSERT(mSenderSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
        if(mIsSenderSleep == ID_TRUE)
        {
            //sender가 wait중이라면 작업을 진행하도록 깨운다
            IDE_ASSERT(mSenderWaitCV.signal() == IDE_SUCCESS);
        }
        IDE_ASSERT(mSenderSNMtx.unlock() == IDE_SUCCESS);
    }
}
void rpdSenderInfo::signalToAllServiceThr( idBool aForceAwake, smTID aTID )
{
    idBool  sIsSendSignal = ID_FALSE;

    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    //이 함수에서는 가능성있는 모든 서비스 스레드를 깨워주고
    //판단은 서비스 스레드가 직접 할 수 있도록 한다.
    
    if ( ( aForceAwake == ID_TRUE ) || ( mIsWaitOnFailback == ID_TRUE ) )
    {
        sIsSendSignal = ID_TRUE;
    }
    else
    {
        if ( ( mIsActive == ID_TRUE ) && ( aTID != SM_NULL_TID ) )
        {
            if ( isTransAcked( aTID ) == ID_TRUE )
            {
                sIsSendSignal = ID_TRUE;
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

    if ( sIsSendSignal == ID_TRUE )
    {
        IDE_ASSERT(mServiceWaitCV.broadcast() == IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);

    return;
}

void rpdSenderInfo::senderWait(PDL_Time_Value aAbsWaitTime)
{
    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    mIsSenderSleep = ID_TRUE;

    (void)mSenderWaitCV.timedwait(&mServiceSNMtx, &aAbsWaitTime);

    mIsSenderSleep = ID_FALSE;

    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);

    return;
}

void rpdSenderInfo::isTransAbort(smTID   aTID,
                                 UInt    aTxReplMode,
                                 idBool *aIsAbort,
                                 idBool *aIsActive)
{
    *aIsAbort  = ID_FALSE;
    *aIsActive = ID_TRUE;

    ACP_UNUSED(aTxReplMode);
    IDE_DASSERT(aTxReplMode != RP_USELESS_MODE);

    if(mReplMode == RP_EAGER_MODE)
    {
        IDE_ASSERT(mActTransTblMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
        //Exit(),Sync(),Disconnect()인 경우 skip한다.
        if((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE))
        {
            if(mActiveTransTbl->isATrans(aTID) == ID_TRUE)
            {
                *aIsAbort = mActiveTransTbl->getAbortFlag(aTID);
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            *aIsActive = ID_FALSE;
        }
        IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
    }
    else
    {
        /*nothing to do*/
    }

    return;
}

void rpdSenderInfo::setGlobalTxAckRecvSN( smTID aTID, smSN aSN )
{
    IDE_ASSERT( mActTransTblMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    if( ( mActiveTransTbl != NULL ) && ( mIsActive == ID_TRUE ) )
    {
        mActiveTransTbl->setGlobalTxAckRecvSN( aTID, aSN ); 
    }

    IDE_ASSERT( mActTransTblMtx.unlock() == IDE_SUCCESS );
}

smSN rpdSenderInfo::getGlobalTxAckRecvSN( smTID aTID )
{
    smSN sSN = SM_SN_MAX;

    IDE_ASSERT( mActTransTblMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    
    if( ( mActiveTransTbl != NULL ) && ( mIsActive == ID_TRUE ) )
    {
        sSN = mActiveTransTbl->getGlobalTxAckRecvSN( aTID );
    }

    IDE_ASSERT( mActTransTblMtx.unlock() == IDE_SUCCESS );

    return sSN;
}

idBool rpdSenderInfo::isActiveTrans(smTID aTID)
{
    idBool sIsATrans = ID_FALSE;

    IDE_ASSERT(mActTransTblMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    //Exit(),Sync(),Disconnect()인 경우 skip한다.
    if((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE))
    {
        sIsATrans = mActiveTransTbl->isATrans(aTID);
    }

    IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);

    return sIsATrans;
}
void rpdSenderInfo::setDDLTransaction(smTID aTransID)
{
    IDE_ASSERT(mActTransTblMtx.lock( NULL /* idvSQL* */) == IDE_SUCCESS);

    if((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE))
    {
        mActiveTransTbl->setDDLTrans(aTransID);
    }
    else
    {
        IDE_DASSERT((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE));
    }
    IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
}

void rpdSenderInfo::setAbortTransaction(UInt     aAbortTxCount,
                                        rpTxAck *aAbortTxList)
{
    UInt i;

    if(aAbortTxCount != 0)
    {
        IDE_ASSERT(mActTransTblMtx.lock( NULL /* idvSQL* */) == IDE_SUCCESS);
        if((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE))
        {
            /* BUG-18045 [Eager Mode] 같은 Tx Slot을 사용하는 이전 Tx의 영향을 배제한다.
             * (1) TID 0 Lazy Mode Transaction이 Standby Host에 반영 실패
             * (2) TID 0 Lazy Mode Transaction Commit
             * (3) TID 4096 Eager Mode Transaction Begin
             * (4) Sender가 ACK 수신 (TID 0 Transaction 실패) -> Abort Flag 설정
             * (5) TID 4096 Eager Mode Transacition Commit 실패
             * 위와 같은 경우 5번에서 eager transaction에 문제가 발생할
             * 수 있으므로, receiver에서 문제가 발생한 로그의 SN
             * 과 eager transaction의 Begin SN을 비교하여
             * 문제가 발생한 로그의 SN이 크거나 같은 경우에만
             * abort flag를 set한다.
             */
            IDE_ASSERT(mActiveTransTbl->mAbortTxMutex.lock( NULL /* idvSQL* */)
                       == IDE_SUCCESS);
            for(i = 0; i < aAbortTxCount; i++)
            {
                if(aAbortTxList[i].mSN >=
                   mActiveTransTbl->getBeginSN(aAbortTxList[i].mTID))
                {
                    mActiveTransTbl->setAbortInfo(aAbortTxList[i].mTID,
                                                  ID_TRUE,
                                                  aAbortTxList[i].mSN);
                }
            }
            IDE_ASSERT(mActiveTransTbl->mAbortTxMutex.unlock() == IDE_SUCCESS);
        }
        IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
    }

    return;
}

void rpdSenderInfo::setClearTransaction(UInt     aClearTxCount,
                                        rpTxAck *aClearTxList)
{
    UInt i;

    if(aClearTxCount != 0)
    {
        IDE_ASSERT(mActTransTblMtx.lock( NULL /* idvSQL* */) == IDE_SUCCESS);
        if((mActiveTransTbl != NULL) && (mIsActive == ID_TRUE))
        {
            /* BUG-18045 [Eager Mode] 같은 Tx Slot을 사용하는 이전 Tx의 영향을 배제한다.
             * (1) TID 0 Lazy Mode Transaction이 Standby Host에 반영 실패
             * (2) TID 0 Lazy Mode Transaction Commit
             * (3) TID 4096 Eager Mode Transaction Begin
             * (4) Sender가 ACK 수신 (TID 0 Transaction 실패) -> Abort Flag 설정
             * (5) TID 4096 Eager Mode Transacition Commit 실패
             * 위와 같은 경우 5번에서 eager transaction에 문제가 발생할
             * 수 있으므로, receiver에서 문제가 발생한 로그의 SN
             * 과 eager transaction의 Begin SN을 비교하여
             * 문제가 발생한 로그의 SN이 크거나 같은 경우에만
             * abort flag를 set한다.
             */
            IDE_ASSERT(mActiveTransTbl->mAbortTxMutex.lock( NULL /* idvSQL* */)
                       == IDE_SUCCESS);
            for(i = 0; i < aClearTxCount; i++)
            {
                if(aClearTxList[i].mSN >=
                   mActiveTransTbl->getBeginSN(aClearTxList[i].mTID))
                {
                    mActiveTransTbl->setAbortInfo(aClearTxList[i].mTID,
                                                  ID_FALSE,
                                                  SM_SN_NULL);
                }
            }
            IDE_ASSERT(mActiveTransTbl->mAbortTxMutex.unlock() == IDE_SUCCESS);
        }
        IDE_ASSERT(mActTransTblMtx.unlock() == IDE_SUCCESS);
    }

    return;
}

void rpdSenderInfo::setSenderStatus(RP_SENDER_STATUS aStatus)
{
    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    mSenderStatus = aStatus;
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);
    return;
}
RP_SENDER_STATUS rpdSenderInfo::getSenderStatus()
{
    RP_SENDER_STATUS sSenderStatus;
    IDE_ASSERT(mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    sSenderStatus = mSenderStatus;
    IDE_ASSERT(mServiceSNMtx.unlock() == IDE_SUCCESS);
    return sSenderStatus;
}

/*******************************************************************************
 * Description : Sync PK Pool에서 메모리를 할당받아,
 *               Sync PK를 Sync PK List의 마지막에 추가한다.
 *               (Called by Receiver)
 *
 ******************************************************************************/
IDE_RC rpdSenderInfo::addLastSyncPK(rpSyncPKType  aType,
                                    ULong         aTableOID,
                                    UInt          aPKColCnt,
                                    smiValue     *aPKCols,
                                    idBool       *aIsFull)
{
    rpdSyncPKEntry *sSyncPKEntry = NULL;
    idBool          sIsLocked    = ID_FALSE;

    IDE_ASSERT(mSyncPKMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    sIsLocked = ID_TRUE;

    IDE_TEST_CONT( mIsSyncPKPoolAllocated == ID_FALSE, NORMAL_EXIT );

    if(mSyncPKListCurSize >= mSyncPKListMaxSize)
    {
        *aIsFull = ID_TRUE;

        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
        *aIsFull = ID_FALSE;
    }
    IDU_FIT_POINT( "rpdSenderInfo::addLastSyncPK::lock::cralloc" );
    IDE_TEST(mSyncPKPool.cralloc((void**)&sSyncPKEntry) != IDE_SUCCESS);

    sSyncPKEntry->mType     = aType;
    sSyncPKEntry->mTableOID = aTableOID;
    sSyncPKEntry->mPKColCnt = aPKColCnt;

    if( ( aPKCols != NULL ) && ( aPKColCnt > 0 ) )
    {
        idlOS::memcpy(sSyncPKEntry->mPKCols,
                      aPKCols,
                      ID_SIZEOF(smiValue) * aPKColCnt);
    }
    else
    {
        idlOS::memset(sSyncPKEntry->mPKCols,
                      0x00,
                      ID_SIZEOF(smiValue) * QCI_MAX_KEY_COLUMN_COUNT);
    }

    IDU_LIST_INIT_OBJ(&(sSyncPKEntry->mNode), sSyncPKEntry);
    IDU_LIST_ADD_LAST(&mSyncPKList, &(sSyncPKEntry->mNode));

    mSyncPKListCurSize++;

    RP_LABEL(NORMAL_EXIT);

    sIsLocked = ID_FALSE;

    IDE_ASSERT(mSyncPKMtx.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // 메모리 할당에 실패하였으므로, 메모리를 해제하지 않는다.

    if(sIsLocked == ID_TRUE)
    {
        IDE_ASSERT(mSyncPKMtx.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Sync PK List에서 첫 번째 Sync PK Entry를 빼서 반환한다.
 *               (Called by Failback Master)
 *
 ******************************************************************************/
void rpdSenderInfo::getFirstSyncPKEntry(rpdSyncPKEntry **aSyncPKEntry)
{
    iduListNode *sNode;

    *aSyncPKEntry = NULL;

    IDE_ASSERT(mSyncPKMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    if(IDU_LIST_IS_EMPTY(&mSyncPKList) != ID_TRUE)
    {
        sNode = IDU_LIST_GET_FIRST(&mSyncPKList);
        *aSyncPKEntry = (rpdSyncPKEntry *)sNode->mObj;

        IDU_LIST_REMOVE(&(*aSyncPKEntry)->mNode);
        mSyncPKListCurSize--;
    }

    IDE_ASSERT(mSyncPKMtx.unlock() == IDE_SUCCESS);

    return;
}

/*******************************************************************************
 * Description : Sync PK Entry의 메모리를 제거한다.
 *               (Called by Failback Master)
 *
 ******************************************************************************/
void rpdSenderInfo::removeSyncPKEntry(rpdSyncPKEntry * aSyncPKEntry)
{
    UInt i;

    IDE_ASSERT(mSyncPKMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    // aXLog->mPKCols[i].value 메모리를 해제한다.
    for(i = 0; i < QCI_MAX_KEY_COLUMN_COUNT; i++)
    {
        if(aSyncPKEntry->mPKCols[i].value != NULL)
        {
            (void)iduMemMgr::free((void *)aSyncPKEntry->mPKCols[i].value);
        }
    }

    (void)mSyncPKPool.memfree(aSyncPKEntry);

    IDE_ASSERT(mSyncPKMtx.unlock() == IDE_SUCCESS);

    return;
}

/**
 * @breif  Replication Name을 복사하여 얻는다.
 *
 * @param  aOutRepName Replication Name
 */
void rpdSenderInfo::getRepName( SChar * aOutRepName )
{
    IDE_DASSERT( aOutRepName != NULL );

    IDE_ASSERT( mRepNameMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );

    if ( aOutRepName != NULL )
    {
        (void)idlOS::memcpy( aOutRepName,
                             mRepName,
                             QCI_MAX_NAME_LEN + 1 );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_ASSERT( mRepNameMtx.unlock() == IDE_SUCCESS );

    return;
}

/**
 * @breif  Replication Name을 설정한다.
 *
 * @param  aRepName Replication Name
 */
void rpdSenderInfo::setRepName( SChar * aRepName )
{
    IDE_DASSERT( aRepName != NULL );

    IDE_ASSERT( mRepNameMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );

    if ( aRepName != NULL )
    {
        (void)idlOS::memcpy( mRepName,
                             aRepName,
                             QCI_MAX_NAME_LEN + 1 );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_ASSERT( mRepNameMtx.unlock() == IDE_SUCCESS );

    return;
}

/********************************************************************
 * Description  : Sender 의 처리량과 현재 쌓여 있는 Gap 을 가지고
 *                Transaction 이 기다려야 하는 시간을 계산
 *
 * Argument : aCurrentSN [IN] : 현재 SM 에 기록된 최근 SN
 * Return : Microseconds
 * mThroughput : byte per sec
 * ********************************************************************/
ULong rpdSenderInfo::getWaitTransactionTime( smSN aCurrentSN )
{
    ULong   sWaitTimeMSec = 0;
    smSN    sCurrentGap = SM_SN_NULL;
    UInt    sThroughput;

    IDE_TEST_CONT( mReplMode != RP_LAZY_MODE, NORMAL_EXIT );

    if ( RPU_REPLICATION_GAPLESS_ALLOW_TIME != 0 )
    {
        sCurrentGap = calcurateCurrentGap( aCurrentSN );
        sThroughput = mThroughput;

        if ( sThroughput != 0 )
        {
            sWaitTimeMSec = ( sCurrentGap * 1000000 ) / sThroughput;


            if( sWaitTimeMSec > RPU_REPLICATION_GAPLESS_ALLOW_TIME )
            {
                sWaitTimeMSec = sWaitTimeMSec - RPU_REPLICATION_GAPLESS_ALLOW_TIME;
            }
            else
            {
                sWaitTimeMSec = 0;
            }
        }
        else
        {
            sWaitTimeMSec = 0;
        }
    }
    else /* must be no gap */
    {
        sWaitTimeMSec = ID_ULONG_MAX;
    }

    RP_LABEL(NORMAL_EXIT);

    return sWaitTimeMSec;
}

/********************************************************************
 * Description  : 현재 GAP 을 계산
 *
 * Argument : aCurrentSN [IN] : 현재 SM 에 기록된 최근 SN
 *
 * ********************************************************************/
smSN rpdSenderInfo::calcurateCurrentGap( smSN aCurrentSN )
{
    smSN    sCurrentGap = 0;
    smLSN   sCurrentLSN, sLastProcessedLSN;

    if ( ( mLastProcessedSN != SM_SN_NULL ) && ( mLastProcessedSN < aCurrentSN ) )
    {
        SM_MAKE_LSN( sCurrentLSN, aCurrentSN );
        SM_MAKE_LSN( sLastProcessedLSN, mLastProcessedSN );
        sCurrentGap = RP_GET_BYTE_GAP( sCurrentLSN, sLastProcessedLSN );
    }
    else
    {
        sCurrentGap = 0;
    }

    return sCurrentGap;
}

void rpdSenderInfo::setThroughput( UInt aThroughput )
{
    mThroughput = aThroughput;
}

void rpdSenderInfo::checkAndWaitToApply( smTID      aTransID,
                                         smSN       aBeginSN,
                                         smSN       aLastSN,
                                         idBool     aIsRequestNode,
                                         UInt       aRequestWaitMode,
                                         idBool   * aWaitedLastSN )
{
    SInt                i = 0;
    SInt                sWaitCount = 0;
    SInt                sTotalYieldCount = 0;
    RP_SENDER_STATUS    sSenderStatus = mSenderStatus;

    PDL_Time_Value sTvCpu;
    PDL_Time_Value sPDL_Time_Value;

    sPDL_Time_Value.initialize(0, 1000);

    while( 1 )
    {
        sTvCpu  = idlOS::gettimeofday();
        sTvCpu += sPDL_Time_Value;

        //Exit(),Sync(),Disconnect()인 경우 skip한다.
        if ( mIsActive != ID_TRUE )
        {
            break;
        }
        else
        {
            /* do noting */
        }

        if ( needWait( sSenderStatus,
                       aTransID,
                       aBeginSN,
                       aLastSN,
                       aIsRequestNode,
                       aRequestWaitMode,
                       ID_FALSE,
                       aWaitedLastSN ) == ID_FALSE )
        {
            break;
        }
        else
        {
            if ( sTotalYieldCount < RPU_REPLICATION_EAGER_MAX_YIELD_COUNT )
            {
                sWaitCount = sWaitCount + 2;
                for( i = 0; 
                     i < IDL_MIN( sWaitCount, (RPU_REPLICATION_EAGER_MAX_YIELD_COUNT - sTotalYieldCount) ); 
                     i++ )
                {
                    idlOS::thr_yield();
                }
                sTotalYieldCount = sTotalYieldCount + i;
            }
            else
            {
                IDE_ASSERT( mServiceSNMtx.lock(NULL /* idvSQL* */) == IDE_SUCCESS );

                // 위에서 lock 을 안 잡고 확인 했으므로 lock 을 잡고 sleep 하기 전에
                // 다시 확인 한다.
                if ( needWait( sSenderStatus,
                               aTransID,
                               aBeginSN,
                               aLastSN,
                               aIsRequestNode,
                               aRequestWaitMode,
                               ID_TRUE,
                               aWaitedLastSN ) == ID_FALSE )
                {
                    IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );
                    break;
                }
                else
                {
                    /* do noting */
                }

                sTotalYieldCount = 0;

                if ( ( sSenderStatus == RP_SENDER_FAILBACK_EAGER ) ||
                     ( sSenderStatus == RP_SENDER_FLUSH_FAILBACK ) )
                {
                    mIsWaitOnFailback = ID_TRUE;
                }
                else
                {
                    /* do nothing */
                }

                sTvCpu  = idlOS::gettimeofday();
                sTvCpu += sPDL_Time_Value;

                (void)mServiceWaitCV.timedwait( &mServiceSNMtx, &sTvCpu );

                mIsWaitOnFailback = ID_FALSE;

                IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );
            }
        }
    }
}

idBool rpdSenderInfo::needWait( RP_SENDER_STATUS    aSenderStatus,
                                smTID               aTransID,
                                smSN                /* aBeginSN R2HA*/ ,
                                smSN                aLastSN,
                                idBool              aIsRequestNode,
                                UInt                aRequestWaitMode,
                                idBool              aAlreadyLocked,
                                idBool            * aWaitedLastSN )
{
    idBool      sNeedWait = ID_TRUE;
    smSN        sLastSN   = aLastSN;

    switch( aSenderStatus )
    {
        case RP_SENDER_FAILBACK_EAGER:
        case RP_SENDER_FLUSH_FAILBACK:
            // Parent 만 온다.
            if ( sLastSN <= mLastProcessedSN )
            {
                if ( aAlreadyLocked == ID_FALSE )
                {
                    IDE_ASSERT( mServiceSNMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );

                    if ( mIsActive == ID_TRUE )
                    {
                        if ( sLastSN <= mLastProcessedSN )
                        {
                            *aWaitedLastSN = ID_TRUE;
                            sNeedWait = ID_FALSE;
                        }
                        else
                        {
                            /*do nothing*/
                        }
                    }
                    else
                    {
                        sNeedWait = ID_FALSE;
                    }

                    IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );
                }
                else
                {
                    *aWaitedLastSN = ID_TRUE;
                    sNeedWait = ID_FALSE;
                }
            }
            else
            {
                sNeedWait = ID_TRUE;
            }
            break;

        case RP_SENDER_IDLE:
            if ( isTransAcked( aTransID ) == ID_TRUE )
            {
                if ( aAlreadyLocked == ID_FALSE )
                {
                    IDE_ASSERT( mServiceSNMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );

                    if ( mIsActive == ID_TRUE )
                    {
                        if ( isTransAcked( aTransID ) == ID_TRUE )
                        {
                            *aWaitedLastSN = ID_TRUE;
                            sNeedWait = ID_FALSE;
                        }
                        else
                        {
                            /*do nothing*/
                        }
                    }
                    else
                    {
                        sNeedWait = ID_FALSE;
                    }

                    IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );
                }
                else
                {
                    *aWaitedLastSN = ID_TRUE;
                    sNeedWait = ID_FALSE;
                }
            }
            else
            {
                sNeedWait = ID_TRUE;
            }
            break;
        case RP_SENDER_RUN :
            if( aRequestWaitMode == RP_CONSISTENT_MODE )
            {
                if ( aIsRequestNode == ID_TRUE )
                { 
                    sLastSN =  getGlobalTxAckRecvSN( aTransID );
                    if ( aLastSN < sLastSN )
                    {
                        sLastSN = aLastSN;
                    }
                }
                
                if ( sLastSN <= mLastArrivedSN )
                {
                    if ( aAlreadyLocked == ID_FALSE )
                    {
                        IDE_ASSERT( mServiceSNMtx.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );

                        if ( mIsActive == ID_TRUE )
                        {
                            if ( sLastSN <= mLastArrivedSN )
                            {
                                sNeedWait = ID_FALSE;
                            }
                            else
                            {
                                sNeedWait = ID_TRUE;
                            }
                        }
                        else
                        {
                            sNeedWait = ID_FALSE;
                        }
                        IDE_ASSERT( mServiceSNMtx.unlock() == IDE_SUCCESS );
                    }
                    else
                    {
                        sNeedWait = ID_FALSE;
                    }
                }
                else
                {
                    sNeedWait = ID_TRUE;
                }

                if ( sNeedWait == ID_TRUE )
                {
                    if ( isReplicationDDLTrans(aTransID) == ID_TRUE )
                    {
                        sNeedWait = ID_FALSE;
                    }
                }
            }
            else
            {
                sNeedWait = ID_FALSE;
            }
            break;

        default:
            sNeedWait = ID_FALSE;
            break;
    }
    IDE_DASSERT(!( (sNeedWait == ID_TRUE) &&
                   (isReplicationDDLTrans(aTransID) == ID_TRUE) ));
    return sNeedWait;
}

rpdSenderInfo * rpdSenderInfo::getAssignedSenderInfo( smTID     aTransID )
{
    UInt              sSlotIndex = 0;
    rpxSender       * sSender = NULL;
    rpdSenderInfo   * sSenderInfo = NULL;

    sSlotIndex = aTransID % mTransTableSize;

    IDE_ASSERT( mAssignedTransTableMutex.lock( NULL ) == IDE_SUCCESS );

    if ( ( mAssignedTransTable != NULL ) &&
         ( ( mSenderStatus == RP_SENDER_FLUSH_FAILBACK ) ||
           ( mSenderStatus == RP_SENDER_IDLE ) ) )
    {
        sSender = mAssignedTransTable[sSlotIndex];
        if ( sSender != NULL )
        {
            sSenderInfo = sSender->getSenderInfo();
        }
        else
        {
            /* BUG-42138 */
            sSenderInfo = this;
        }
    }
    else
    {
        /*
         * mAssignedTransTable 에 할당되어 있지 않으면
         * Parent 에서 처리중인 Transaction 이다
         */
        sSenderInfo = this;
    }

    IDE_ASSERT( mAssignedTransTableMutex.unlock() == IDE_SUCCESS );

    return sSenderInfo;
}

void rpdSenderInfo::removeAssignedSender( smTID    aTransID )
{
    UInt    sSlotIndex = 0;

    sSlotIndex = aTransID % mTransTableSize;

    IDE_ASSERT( mAssignedTransTableMutex.lock( NULL ) == IDE_SUCCESS );
    if ( mAssignedTransTable != NULL )
    {
        mAssignedTransTable[sSlotIndex] = NULL;
    }
    else
    {
        /* Nothing to do */
    }
    IDE_ASSERT( mAssignedTransTableMutex.unlock() == IDE_SUCCESS );

    return;
}

void rpdSenderInfo::setLastSN( smTID aTID, smSN aLastSN )
{
    UInt    sIndex = 0;

    sIndex = aTID % mTransTableSize;
    
    IDE_DASSERT( mLastProcessedSNTable != NULL );

    mLastProcessedSNTable[sIndex].mLastSN = aLastSN;
}

void rpdSenderInfo::setAckedSN( smTID aTID, smSN aAckedSN )
{
    UInt    sIndex = 0;

    sIndex = aTID % mTransTableSize;

    IDE_DASSERT( mLastProcessedSNTable != NULL );

    mLastProcessedSNTable[sIndex].mAckedSN = aAckedSN;
}

idBool rpdSenderInfo::isTransAcked( smTID aTransID )
{
    UInt    sIndex = 0;
    idBool  sResult = ID_FALSE;
    
    sIndex = aTransID % mTransTableSize;

    if ( mLastProcessedSNTable[sIndex].mLastSN <= 
         mLastProcessedSNTable[sIndex].mAckedSN )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}

idBool rpdSenderInfo::isAllTransFlushed( smSN aCurrentSN )
{
    UInt    i = 0;
    idBool  sResult = ID_TRUE;
    
    for ( i = 0 ; i < mTransTableSize ; i++ )
    {
        if ( aCurrentSN > mLastProcessedSNTable[i].mAckedSN )
        {
            /* mLastSN과 mAckedSN이 같은 Transaction은 Flush 되었다고 판단한다. */
            if ( mLastProcessedSNTable[i].mLastSN != mLastProcessedSNTable[i].mAckedSN )
            {
                sResult = ID_FALSE;
                break;
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

    return sResult;
}

void rpdSenderInfo::initializeLastProcessedSNTable( void )
{
    UInt    i = 0;

    for ( i = 0 ; i < mTransTableSize ; i++ )
    {
        mLastProcessedSNTable[i].mLastSN = 0;
        mLastProcessedSNTable[i].mAckedSN = 0;
    }
}

IDE_RC rpdSenderInfo::waitLastProcessedSN( idvSQL * aStatistics,
                                           idBool * aExitFlag, 
                                           smSN     aLastSN )
{
    smSN sLastProcessedSN = getLastProcessedSN();

    while( ( sLastProcessedSN != SM_SN_NULL ) && 
           ( sLastProcessedSN < aLastSN ) && 
           ( *( aExitFlag ) != ID_TRUE ) )
    {
        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

        idlOS::sleep( 1 );
        sLastProcessedSN = getLastProcessedSN();
    }

    IDU_FIT_POINT( "rpdSenderInfo::waitLastProcessedSN" );

    IDE_TEST_RAISE( getSenderStatus() != RP_SENDER_RUN, ERR_SENDER_NOT_RUNNING ); 
    IDE_TEST_RAISE( *( aExitFlag ) == ID_TRUE, ERR_EXIT_FLAG );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIT_FLAG );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_EXIT_FLAG_SET ) );
    }
    IDE_EXCEPTION( ERR_SENDER_NOT_RUNNING )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_NOT_RUNNING, mRepName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool rpdSenderInfo::isWrittenCommitXLog( smSN aLastSN, smTID /* aTID R2HA */ )
{
    idBool sIsWritten = ID_FALSE;
    smSN sRemoteLastCommitSN = SM_SN_NULL;

    sRemoteLastCommitSN = getRmtLastCommitSN();

    if ( sRemoteLastCommitSN != SM_SN_NULL )
    {
        if ( sRemoteLastCommitSN >= aLastSN )
        {
            sIsWritten = ID_TRUE;
        }
    }
 
    return sIsWritten;
}
