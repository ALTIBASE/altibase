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
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 *
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: smxTrans.cpp 91255 2021-07-19 01:08:26Z emlee $
 **********************************************************************/

#include <ida.h>
#include <smErrorCode.h>
#include <smr.h>
#include <smp.h>
#include <svp.h>
#include <smc.h>
#include <sdc.h>
#include <smx.h>
#include <smxReq.h>
#include <sdr.h>
#include <svrRecoveryMgr.h>
#include <svcLob.h>
#include <smlLockMgr.h>

extern smLobModule sdcLobModule;
extern smLobModule sdiLobModule; // PROJ-2728

UInt           smxTrans::mAllocRSIdx = 0;
iduMemPool     smxTrans::mLobCursorPool;
iduMemPool     smxTrans::mLobColBufPool;
iduMemPool     smxTrans::mMutexListNodePool;
smrCompResPool smxTrans::mCompResPool;

iduMutex * smxTrans::mGCMutex;
smTID   ** smxTrans::mGCTIDArray;
smSCN   ** smxTrans::mGCCommitSCNArray;
UInt     * smxTrans::mGCCnt;
UInt     * smxTrans::mGCListID;
UInt       smxTrans::mGCList;
UInt       smxTrans::mGroupCnt;
UInt       smxTrans::mGCListCnt;
UInt     * smxTrans::mGCFlag;

static smxTrySetupViewSCNFuncs gSmxTrySetupViewSCNFuncs[SMI_STATEMENT_CURSOR_MASK+1]=
{
    NULL,
    smxTrans::trySetupMinMemViewSCN, // SMI_STATEMENT_MEMORY_CURSOR
    smxTrans::trySetupMinDskViewSCN, // SMI_STATEMENT_DISK_CURSOR
    smxTrans::trySetupMinAllViewSCN  // SMI_STATEMENT_ALL_CURSOR
};

IDE_RC smxTrans::initializeStatic()
{
    UInt i = 0;
    UInt j = 0;
    IDE_TEST( mCompResPool.initialize(
                  (SChar*)"TRANS_LOG_COMP_RESOURCE_POOL",
                  16, // aInitialResourceCount
                  smuProperty::getMinCompressionResourceCount(),
                  smuProperty::getCompressionResourceGCSecond() )
              != IDE_SUCCESS );

    IDE_TEST( mMutexListNodePool.initialize(
                  IDU_MEM_SM_TRANSACTION_TABLE,
                  (SChar *)"MUTEX_LIST_NODE_MEMORY_POOL",
                  ID_SCALABILITY_SYS,
                  ID_SIZEOF(smuList),
                  SMX_MUTEX_LIST_NODE_POOL_SIZE,
                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                  ID_TRUE,							/* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                  ID_FALSE,							/* ForcePooling */
                  ID_TRUE,							/* GarbageCollection */
                  ID_TRUE,                          /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type */) 
              != IDE_SUCCESS);			

    IDE_TEST( mLobCursorPool.initialize(
                  IDU_MEM_SM_SMX,
                  (SChar *)"LOB_CURSOR_POOL",
                  ID_SCALABILITY_SYS,
                  ID_SIZEOF(smLobCursor),
                  16,/* aElemCount */
                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                  ID_TRUE,							/* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                  ID_FALSE,							/* ForcePooling */
                  ID_TRUE,							/* GarbageCollection */
                  ID_TRUE,                          /* HWCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
               != IDE_SUCCESS);			

    IDE_TEST( mLobColBufPool.initialize(
                  IDU_MEM_SM_SMX,
                  (SChar *)"LOB_COLUMN_BUFFER_POOL",
                  ID_SCALABILITY_SYS,
                  ID_SIZEOF(sdcLobColBuffer),
                  16,/* aElemCount */
                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                  ID_TRUE,							/* UseMutex */
                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                  ID_FALSE,							/* ForcePooling */
                  ID_TRUE,							/* GarbageCollection */
                  ID_TRUE,							/* HwCacheLine */
                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
               != IDE_SUCCESS);			

    smcLob::initializeFixedTableArea();
    sdcLob::initializeFixedTableArea();

    mGroupCnt  = smuProperty::getGroupCommitCnt();
    mGCListCnt = smuProperty::getGroupCommitListCnt();

    if ( mGroupCnt != 0 )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                           ID_SIZEOF(iduMutex) * mGCListCnt,
                                           (void**)&mGCMutex ) != IDE_SUCCESS,
                        insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                           ID_SIZEOF(UInt) * mGCListCnt,
                                           (void**)&mGCCnt ) != IDE_SUCCESS,
                        insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                           ID_SIZEOF(UInt) * mGCListCnt,
                                           (void**)&mGCListID ) != IDE_SUCCESS,
                        insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                           ID_SIZEOF(UInt*) * mGCListCnt,
                                           (void**)&mGCTIDArray ) != IDE_SUCCESS,
                        insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                           ID_SIZEOF(UInt*) * mGCListCnt,
                                           (void**)&mGCCommitSCNArray ) != IDE_SUCCESS,
                        insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                           ID_SIZEOF(UInt) * mGCListCnt,
                                           (void**)&mGCFlag ) != IDE_SUCCESS,
                        insufficient_memory );

        for ( j = 0 ; j < mGCListCnt; j++ )
        {
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                               ID_SIZEOF(smTID) * mGroupCnt,
                                               (void**)&mGCTIDArray[j] ) != IDE_SUCCESS,
                            insufficient_memory );

            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                               ID_SIZEOF(smSCN) * mGroupCnt,
                                               (void**)&mGCCommitSCNArray[j] ) != IDE_SUCCESS,
                            insufficient_memory );

            IDE_TEST( mGCMutex[j].initialize( (SChar*)"GROUP_COMMIT_MUTEX",
                                                  IDU_MUTEX_KIND_NATIVE,
                                                  IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

            mGCCnt[j]    = 0;
            mGCListID[j] = j; /* GCListID는 각 SlotNum을 기준으로 시작해서 SlotNum 씩 증가함 */

            /* 기본적으로 비RP 로그로 판단. 모인 것 중에 하나라도 RP로그이면 RP로그로 판단함. */
            /* 기본적으로 COMMITSCN이 없는 로그로 판단.
               모인 것 중에 하나라도 COMMITSCN을 저장했으면 COMMITSCN 이 있는것으로 판단함. */
            mGCFlag[j] = SMR_LOG_TYPE_REPLICATED | SMR_LOG_COMMITSCN_NO ;

            for ( i = 0 ; i < mGroupCnt ; i++ )
            {
                mGCTIDArray[j][i] = SM_NULL_TID;
                mGCCommitSCNArray[j][i] = SM_SCN_INIT; 
            }
        }
    }
    mGCList = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxTrans::destroyStatic()
{
    UInt i = 0;

    if ( mGroupCnt != 0 )
    {
        for( i = 0 ; i < mGCListCnt ; i++ )
        {
            IDE_TEST( mGCMutex[i].destroy() != IDE_SUCCESS );

            IDE_TEST( iduMemMgr::free( mGCCommitSCNArray[i] ) != IDE_SUCCESS );
            mGCCommitSCNArray[i] = NULL;

            IDE_TEST( iduMemMgr::free( mGCTIDArray[i] ) != IDE_SUCCESS );
            mGCTIDArray[i] = NULL;
        }

        IDE_TEST( iduMemMgr::free( mGCFlag ) != IDE_SUCCESS );
        mGCFlag = NULL;

        IDE_TEST( iduMemMgr::free( mGCCommitSCNArray ) != IDE_SUCCESS );
        mGCCommitSCNArray = NULL;

        IDE_TEST( iduMemMgr::free( mGCTIDArray ) != IDE_SUCCESS );
        mGCTIDArray = NULL;

        IDE_TEST( iduMemMgr::free( mGCListID ) != IDE_SUCCESS );
        mGCListID = NULL;

        IDE_TEST( iduMemMgr::free( mGCCnt ) != IDE_SUCCESS );
        mGCCnt = NULL;

        IDE_TEST( iduMemMgr::free( mGCMutex ) != IDE_SUCCESS );
        mGCMutex = NULL;
    }

    IDE_TEST( mMutexListNodePool.destroy() != IDE_SUCCESS );

    IDE_TEST( mCompResPool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smxTrans::initialize(smTID aTransID, UInt aSlotMask)
{
    SChar sBuffer[128];
    UInt  sState = 0;

    idlOS::memset(sBuffer, 0, 128);
    idlOS::snprintf(sBuffer, 128, "TRANS_MUTEX_%"ID_UINT32_FMT, (UInt)aTransID);

    mCompRes        = NULL;

    mTransID        = aTransID;
#ifdef PROJ_2181_DBG
    mTransIDDBG   = aTransID;
#endif
    mSlotN          = mTransID & aSlotMask;
    mUpdateSize     = 0;
    mLogOffset      = 0;
    SM_LSN_MAX( mBeginLogLSN );
    SM_LSN_MAX( mCommitLogLSN );
    mLegacyTransCnt = 0;
    mStatus         = SMX_TX_END;
    mStatus4FT      = SMX_TX_END;

    mOIDToVerify         = NULL;
    mOIDList             = NULL;
    mLogBuffer           = NULL;
    mCacheOIDNode4Insert = NULL;
    mReplLockTimeout     = smuProperty::getReplLockTimeOut();

    mConnectDeleteThread = NULL;
    
    //PRJ-1476
    /* smxTrans_initialize_alloc_CacheOIDNode4Insert.tc */
    IDU_FIT_POINT("smxTrans::initialize::alloc::CacheOIDNode4Insert");
    IDE_TEST( smxOIDList::alloc(&mCacheOIDNode4Insert)
              != IDE_SUCCESS );

    sState = 1;

    /* smxTrans_initialize_malloc_OIDList.tc */
    IDU_FIT_POINT_RAISE("smxTrans::initialize::malloc::OIDList", insufficient_memory);
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                 ID_SIZEOF(smxOIDList),
                                 (void**)&mOIDList) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 2;

    IDE_TEST( mOIDList->initialize( this,
                                   mCacheOIDNode4Insert,
                                   ID_FALSE, // aIsUnique
                                   &smxOIDList::addOIDWithCheckFlag )
              != IDE_SUCCESS );

    IDE_TEST( mOIDFreeSlotList.initialize( this,
                                           NULL,
                                           ID_FALSE, // aIsUnique
                                           &smxOIDList::addOID )
              != IDE_SUCCESS );

    /* BUG-27122 Restart Recovery 시 Undo Trans가 접근하는 인덱스에 대한
     * Integrity 체크기능 추가 (__SM_CHECK_DISK_INDEX_INTEGRITY=2) */
    if ( smuProperty::getCheckDiskIndexIntegrity()
                      == SMU_CHECK_DSKINDEX_INTEGRITY_LEVEL2 )
    {
        /* TC/FIT/Limit/sm/smx/smxTrans_initialize_malloc1.sql */
        IDU_FIT_POINT_RAISE( "smxTrans::initialize::malloc1",
                              insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                     ID_SIZEOF(smxOIDList),
                                     (void**)&mOIDToVerify ) != IDE_SUCCESS,
                        insufficient_memory );
        sState = 3;

        IDE_TEST( mOIDToVerify->initialize( this,
                                            NULL,
                                            ID_TRUE, // aIsUnique
                                            &smxOIDList::addOIDToVerify )
                  != IDE_SUCCESS );
    }

    IDE_TEST( mTouchPageList.initialize( this ) != IDE_SUCCESS );

    IDE_TEST( mTableInfoMgr.initialize() != IDE_SUCCESS );
    IDE_TEST( init() != IDE_SUCCESS );

    /* smxTransFreeList::alloc, free시에 ID_TRUE, ID_FALSE로 설정됩니다. */
    mIsFree = ID_TRUE;

    IDE_TEST( mMutex.initialize( sBuffer,
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    idlOS::snprintf( sBuffer, 
                     ID_SIZEOF(sBuffer), 
                     "TRANS_COND_%"ID_UINT32_FMT,  
                     (UInt)aTransID);

    IDE_TEST_RAISE(mCondV.initialize(sBuffer) != IDE_SUCCESS, err_cond_var_init);

    SMU_LIST_INIT_BASE(&mMutexList);

    // 로그 버퍼 관련 멤버 초기화
    mLogBufferSize = SMX_TRANSACTION_LOG_BUFFER_INIT_SIZE;

    IDE_ASSERT( SMR_LOGREC_SIZE(smrUpdateLog) < mLogBufferSize );

    /* TC/FIT/Limit/sm/smx/smxTrans_initialize_malloc2.sql */
    IDU_FIT_POINT_RAISE( "smxTrans::initialize::malloc2",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_TRANSACTION_TABLE,
                                       mLogBufferSize,
                                       (void**)&mLogBuffer) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 4;

    if ( smuProperty::getLogCompResourceReuse() == 1 )
    {
        /* BUG-47365 로그 압축 리소스를 미리 받아둔다. */
        IDE_TEST( mCompResPool.allocCompRes( & mCompRes ) != IDE_SUCCESS );
        sState = 5;
    }

    // PrivatePageList 관련 멤버 초기화
    mPrivatePageListCachePtr = NULL;
    mVolPrivatePageListCachePtr = NULL;

    IDE_TEST( smuHash::initialize( &mPrivatePageListHashTable,
                                   1,                       // ConcurrentLevel
                                   SMX_PRIVATE_BUCKETCOUNT, // BucketCount
                                   (UInt)ID_SIZEOF(smOID),  // KeyLength
                                   ID_FALSE,                // UseLatch
                                   hash,                    // HashFunc
                                   isEQ )                   // CompFunc
             != IDE_SUCCESS );

    IDE_TEST( smuHash::initialize( &mVolPrivatePageListHashTable,
                                   1,                       // ConcurrentLevel
                                   SMX_PRIVATE_BUCKETCOUNT, // BucketCount
                                   (UInt)ID_SIZEOF(smOID),  // KeyLength
                                   ID_FALSE,                // UseLatch
                                   hash,                    // HashFunc
                                   isEQ )                   // CompFunc
             != IDE_SUCCESS );

    IDE_TEST( mPrivatePageListMemPool.initialize(
                                     IDU_MEM_SM_SMX,
                                     (SChar*)"SMP_PRIVATEPAGELIST",
                                     1,                                          // list_count
                                     (vULong)ID_SIZEOF(smpPrivatePageListEntry), // elem_size
                                     SMX_PRIVATE_BUCKETCOUNT,                    // elem_count
                                     IDU_AUTOFREE_CHUNK_LIMIT,					 // ChunkLimit
                                     ID_TRUE,									 // UseMutex
                                     IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,			 // AlignByte
                                     ID_FALSE,									 // ForcePooling 
                                     ID_TRUE,									 // GarbageCollection
                                     ID_TRUE,                                    /* HWCacheLine */
                                     IDU_MEMPOOL_TYPE_LEGACY                     /* mempool type*/) 
              != IDE_SUCCESS);			

    IDE_TEST( mVolPrivatePageListMemPool.initialize(
                                     IDU_MEM_SM_SMX,
                                     (SChar*)"SMP_VOLPRIVATEPAGELIST",
                                     1,                                          // list_count
                                     (vULong)ID_SIZEOF(smpPrivatePageListEntry), // elem_size
                                     SMX_PRIVATE_BUCKETCOUNT,                    // elem_count
                                     IDU_AUTOFREE_CHUNK_LIMIT,					 // ChunkLimit
                                     ID_TRUE,									 // UseMutex
                                     IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,			 // AlignByte
                                     ID_FALSE,									 // ForcePooling
                                     ID_TRUE,									 // GarbageCollection
                                     ID_TRUE,                                    /* HWCacheLine */
                                     IDU_MEMPOOL_TYPE_LEGACY                     /* mempool type */) 
              != IDE_SUCCESS);			

    //PROJ-1362
    //fix BUG-21311
    //fix BUG-40790
    IDE_TEST( smuHash::initialize( &mLobCursorHash,
                                   1,
                                   smuProperty::getLobCursorHashBucketCount(),
                                   ID_SIZEOF(smLobCursorID),
                                   ID_FALSE,
                                   genHashValueFunc,
                                   compareFunc )
             != IDE_SUCCESS );

    IDE_TEST( smuHash::initialize( &mShardLobCursorHash,
                                   1,
                                   smuProperty::getLobCursorHashBucketCount(),
                                   ID_SIZEOF(smLobCursorID),
                                   ID_FALSE,
                                   genHashValueFunc,
                                   compareFunc )
             != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    /* 멤버인 mVolatileLogEnv를 초기화한다. ID_TRUE는 align force 임. */
    IDE_TEST( svrLogMgr::initEnv(&mVolatileLogEnv, ID_TRUE ) != IDE_SUCCESS );


    /* PROJ-2162 RestartRiskReduction */
    smrRecoveryMgr::initRTOI( & mRTOI4UndoFailure );
    /* BUG-45711 */
    mIsUncompletedLogWait = smuProperty::isFastUnlockLogAllocMutex();

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_init);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondInit));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sState )
        {
        case 5:
            if ( mCompRes != NULL )
            {
                IDE_ASSERT( mCompResPool.freeCompRes( mCompRes ) == IDE_SUCCESS );
                mCompRes = NULL;
            }
        case 4:
            if ( mLogBuffer != NULL )
            {
                IDE_ASSERT( iduMemMgr::free(mLogBuffer) == IDE_SUCCESS );
                mLogBuffer = NULL;
            }
        case 3:
            if ( mOIDToVerify != NULL )
            {
                IDE_ASSERT( iduMemMgr::free(mOIDToVerify) == IDE_SUCCESS );
                mOIDToVerify = NULL;
            }
        case 2:
            if ( mOIDList != NULL )
            {
                IDE_ASSERT( iduMemMgr::free(mOIDList) == IDE_SUCCESS );
                mOIDList = NULL;
            }
        case 1:
            if ( mCacheOIDNode4Insert != NULL )
            {
                IDE_ASSERT( smxOIDList::freeMem(mCacheOIDNode4Insert)
                            == IDE_SUCCESS );
                mCacheOIDNode4Insert = NULL;
            }
        case 0:
            break;
        default:
            /* invalid case */
            IDE_ASSERT( 0 );
            break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smxTrans::destroy()
{
    IDE_ASSERT( mStatus == SMX_TX_END );

    /* BUG-47365 할당 받은 압축 리소스를 반환한다. */
    if ( mCompRes != NULL )
    {
        IDE_TEST( mCompResPool.freeCompRes( mCompRes ) != IDE_SUCCESS );
        mCompRes = NULL;
    }

    if ( mOIDToVerify != NULL )
    {
        IDE_TEST( mOIDToVerify->destroy() != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free(mOIDToVerify) != IDE_SUCCESS );
        mOIDToVerify = NULL;
    }

    /* PROJ-1381 Fetch Across Commits
     * smxTrans::destroy 함수가 여러 번 호출 될 수 있으므로
     * mOIDList가 NULL이 아닐 때만 OID List를 정리하도록 한다. */
    if ( mOIDList != NULL )
    {
        IDE_TEST( mOIDList->destroy() != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free(mOIDList) != IDE_SUCCESS );
        mOIDList = NULL;
    }

    IDE_TEST( mOIDFreeSlotList.destroy()!= IDE_SUCCESS );

    /* PROJ-1362 */
    IDE_TEST( smuHash::destroy(&mLobCursorHash) != IDE_SUCCESS );
    IDE_TEST( smuHash::destroy(&mShardLobCursorHash) != IDE_SUCCESS );

    /* PrivatePageList관련 멤버 제거 */
    IDE_DASSERT( mPrivatePageListCachePtr == NULL );
    IDE_DASSERT( mVolPrivatePageListCachePtr == NULL );

    IDE_TEST( smuHash::destroy(&mPrivatePageListHashTable) != IDE_SUCCESS );
    IDE_TEST( smuHash::destroy(&mVolPrivatePageListHashTable) != IDE_SUCCESS );

    IDE_TEST( mTouchPageList.destroy() != IDE_SUCCESS );

    IDE_TEST( mPrivatePageListMemPool.destroy() != IDE_SUCCESS );
    IDE_TEST( mVolPrivatePageListMemPool.destroy() != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    IDE_TEST( svrLogMgr::destroyEnv(&mVolatileLogEnv) != IDE_SUCCESS );

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    IDE_ASSERT( mLogBuffer != NULL );
    IDE_TEST( iduMemMgr::free(mLogBuffer) != IDE_SUCCESS );
    mLogBuffer = NULL;

    IDE_TEST( mTableInfoMgr.destroy() != IDE_SUCCESS );

    IDE_ASSERT( mCacheOIDNode4Insert != NULL );

    smxOIDList::freeMem(mCacheOIDNode4Insert);
    mCacheOIDNode4Insert = NULL;

    IDE_TEST_RAISE(mCondV.destroy() != IDE_SUCCESS,
                   err_cond_var_destroy);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_var_destroy);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* TASK-2398 로그압축
   트랜잭션의 로그 압축/압축해제에 사용할 리소스를 리턴한다

   [OUT] aCompRes - 로그 압축 리소스
*/
IDE_RC smxTrans::getCompRes(smrCompRes ** aCompRes)
{
    if ( mCompRes == NULL ) // Transaction Begin이후 처음 호출된 경우
    {
        // Log 압축 리소스를 Pool에서 가져온다.
        IDE_TEST( mCompResPool.allocCompRes( & mCompRes )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( mCompRes != NULL );

    *aCompRes = mCompRes;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 트랜잭션의 로그 압축/압축해제에 사용할 리소스를 리턴한다

   [IN] aTrans - 트랜잭션
   [OUT] aCompRes - 로그 압축 리소스
 */
IDE_RC smxTrans::getCompRes4Callback( void *aTrans, smrCompRes ** aCompRes )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aCompRes != NULL );

    smxTrans * sTrans = (smxTrans *) aTrans;

    IDE_TEST( sTrans->getCompRes( aCompRes )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxTrans::init()
{
    mIsUpdate        = ID_FALSE;

    /*PROJ-1541 Eager Replication
     *mReplMode는 Session별 Eager,Acked,Lazy를 지정할 수 있도록
     *하기 위해 존재하며, DEFAULT 값을 갖는다.
     */

    mFlag = 0;
    mFlag = SMX_REPL_DEFAULT | SMX_COMMIT_WRITE_NOWAIT;

    // For Global Transaction
    initXID();

    mCommitState          = SMX_XA_COMPLETE;
    mPreparedTime.tv_sec  = (long)0;
    mPreparedTime.tv_usec = (long)0;

    SMU_LIST_INIT_BASE(&mPendingOp);

    mUpdateSize           = 0;

    /* TASK-7219 Non-shard DML */
    mIsPartialStmt = ID_FALSE;
    mStmtSeq       = 0;

    /* PROJ-2734 */
    clearDistTxInfo();

    mDistDeadlock4FT.mDetection   = SMX_DIST_DEADLOCK_DETECTION_NONE;
    mDistDeadlock4FT.mDieWaitTime = 0;
    mDistDeadlock4FT.mElapsedTime = 0;

    /* PROJ-1381 Fetch Across Commits
     * Legacy Trans가 있으면 아래의 멤버를 초기화 하지 않는다.
     * MinViewSCNs       : aging 방지
     * mFstUndoNxtLSN    : 비정상 종료시 redo시점을 유지
     * initTransLockList : IS Lock 유지 */
    if ( mLegacyTransCnt == 0 )
    {
        mStatus    = SMX_TX_END;
        mStatus4FT = SMX_TX_END;

        SM_SET_SCN_INFINITE( &mMinMemViewSCN );
        SM_SET_SCN_INFINITE( &mMinDskViewSCN );
        SM_SET_SCN_INFINITE( &mFstDskViewSCN );

        /* 해당 Tx에서 holdable cursor가 열렸을때의 infinite SCN */
        SM_INIT_SCN( &mCursorOpenInfSCN ); 

        // BUG-26881 잘못된 CTS stamping으로 access할 수 없는 row를 접근함
        SM_SET_SCN_INFINITE( &mOldestFstViewSCN );

        SM_SET_SCN_INFINITE( &mLastRequestSCN );

        SM_LSN_MAX(mFstUndoNxtLSN);

        /* initialize lock slot */
        smLayerCallback::initTransLockList( mSlotN );
    }

    SM_SET_SCN_INFINITE( &mPrepareSCN );
    SM_SET_SCN_INFINITE( &mCommitSCN );

    mLogTypeFlag = SMR_LOG_TYPE_NORMAL;

    SM_LSN_MAX( mLstUndoNxtLSN );
    SM_LSN_MAX( mCurUndoNxtLSN );

    mLSLockFlag = SMX_TRANS_UNLOCKED;
    mDoSkipCheckSCN = ID_FALSE;

    //For Session Management
    mFstUpdateTime       = 0;
    mLogOffset           = 0;

    /* BUG-33895 [sm_recovery] add the estimate function 
     * the time of transaction undoing. */
    mUndoBeginTime         = 0;
    mTotalLogCount         = 0;
    mProcessedUndoLogCount = 0;
    // PROJ-1362 QP Large Record & Internal LOB
    mCurLobCursorID = 0;
    mCurShardLobCursorID = 0; // PROJ-2728

    mDoSkipCheck   = ID_FALSE;
    mIsDDL         = ID_FALSE;
    mIsFirstLog    = ID_TRUE;
    mIsTransWaitRepl = ID_FALSE;

    mTXSegEntryIdx = ID_UINT_MAX;
    mTXSegEntry    = NULL;

    IDE_TEST( mTableInfoMgr.init() != IDE_SUCCESS );
    mTableInfoPtr  = NULL;

    SM_LSN_MAX( mLastWritedLSN );

    // initialize PageListID
    mRSGroupID = SMP_PAGELISTID_NULL;

    //fix BUG-21311
    mMemLCL.initialize();
    mDiskLCL.initialize();
    mShardLCL.initialize();   // PROJ-2728

    /* Disk Insert Rollback (Partial Rollback 포함)시 Flag를 FALSE로
       하여, Commit 이나 Abort시에 Aging List에 추가할수 있게 한다. */
    mFreeInsUndoSegFlag = ID_TRUE;

    /* TASK-2401 MMAP Logging환경에서 Disk/Memory Log의 분리
       Disk/Memory Table에 접근했는지 여부를 초기화
     */
    mDiskTBSAccessed   = ID_FALSE;
    mMemoryTBSAccessed = ID_FALSE;
    mMetaTableModified = ID_FALSE;

    mIsReusableRollback = ID_TRUE;
    mIsCursorHoldable   = ID_FALSE;

    // PROJ-2068
    mDPathEntry = NULL;

    mIsGCTx = ID_FALSE;

    mIndoubtFetchTimeout = (UInt)IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_DEFAULT;
    mIndoubtFetchMethod  = (UInt)IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_DEFAULT;
    
    mGlobalSMNChangeFunc = NULL;
 
    mInternalTableSwap   = ID_FALSE;

    mGlobalTxId          = SM_INIT_GTX_ID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smxTrans::suspend( smxTrans * aWaitTrans,
                          iduMutex * aMutex,
                          ULong      aTotalWaitMicroSec )
{
    return suspend( aWaitTrans,
                    aMutex,
                    &aTotalWaitMicroSec,
                    NULL,
                    ID_FALSE,
                    SMX_DIST_DEADLOCK_DETECTION_NONE,
                    NULL,
                    NULL );
}

/***********************************************************************
 * Description: 1. aWaitTrans != NULL
 *                 aWaitTrans 가 잡고 있는 Resource가 Free될때까지 기다린다.
 *                 하지만 Transaction을 다른 event(query timeout, session
 *                 timeout)에 의해서 중지되지 않는 이상 aWaitMicroSec만큼
 *                 대기한다.
 *
 *              2. aMutex != NULL
 *                 Record Lock같은 경우는 Page Mutex를 잡고 Lock Validation을
 *                 수행한다. 그 Mutex를 Waiting하기전에 풀어야 한다. 여기서
 *                 풀고 다시 Transaction이 깨어났을 경우에 다시 Mutex를 잡고
 *                 실제로 해당 Record가 Lock을 풀렸는지 검사한다.
 *
 * aWaitTrans              - [IN] 해당 Resource를 가지고 있는 Transaction
 * aMutex                  - [IN] 해당 Resource( ex: Record)를 여러개의 트랜잭션이 동시에
 *                                접근하는것으로부터 보호하는 Mutex
 * aTotalWaitMicroSec      - [IN/OUT] WAIT 할 전체시간.
 * aElapsedMicroSec        - [IN/OUT] 전체시간 중 경과된 시간.
 * aCheckDistDeadlock      - [IN] WAIT 중에 주기적으로 분산데드락을 체크할지 여부.
 * aDistDeadlock           - [IN] suspend 들어오기 이전에 분산데드락이 탐지되었는지.
 * aIsReleasedDistDeadlock - [OUT] WAIT 중에 분산데드락 상황이 해제되었는지 여부.
 * aNewDistDeadlock        - [OUT] WAIT 중에 분산데드락이 새로 탐지되었는지.
 ***********************************************************************/
IDE_RC smxTrans::suspend( smxTrans                 * aWaitTrans,
                          iduMutex                 * aMutex,
                          ULong                    * aTotalWaitMicroSec,
                          ULong                    * aElapsedMicroSec,
                          idBool                     aCheckDistDeadlock,
                          smxDistDeadlockDetection   aDistDeadlock,
                          idBool                   * aIsReleasedDistDeadlock,
                          smxDistDeadlockDetection * aNewDistDeadlock )
{
    idBool            sWaited          = ID_FALSE;
    idBool            sMyMutexLocked   = ID_FALSE;
    idBool            sTxMutexUnlocked = ID_FALSE;

    ULong             sSleepMicroSec        = 0; /* Loop 한번에 대기하는 시간 */
    ULong             sTotalWaitMicroSec    = 0; /* 기다려야 전체시간 */
    ULong             sElapsedMicroSec      = 0; /* 경과된 시간 */
    ULong             sNewTotalWaitMicroSec = 0;
    ULong             sIntervalMicroSec     = 0;
    idBool            sDumped               = ID_FALSE;

    PDL_Time_Value    sTimeVal;
    PDL_Time_Value    sTimeWait;
    
    idBool                   sIsReleasedDistDeadlock = ID_FALSE;
    smxDistDeadlockDetection sDetection              = SMX_DIST_DEADLOCK_DETECTION_NONE;
    smxDistDeadlockDetection sNewDistDeadlock        = SMX_DIST_DEADLOCK_DETECTION_NONE;

    /*
     * Check whether this TX is waiting for itself
     * ASSERT in debug mode
     * Deadlock warning in release mode
     */
    if( aWaitTrans != NULL )
    {
        IDE_DASSERT( mSlotN != aWaitTrans->mSlotN );
        IDE_TEST_RAISE( mSlotN == aWaitTrans->mSlotN , err_selflock );
    }
    else
    {
        /* fall through */
    }

    if( aTotalWaitMicroSec != NULL )
    {
        sTotalWaitMicroSec = *aTotalWaitMicroSec;
    }
    else
    {
        idlOS::thr_yield();
    }

    if( aElapsedMicroSec != NULL )
    {
        sElapsedMicroSec = *aElapsedMicroSec;
    }

    /* always return true */
    lock();
    sMyMutexLocked = ID_TRUE;

    if ( aWaitTrans != NULL ) /* RECORD LOCK 이라면 */
    {
        IDE_ASSERT( aMutex == NULL );

        if ( smLayerCallback::didLockReleased( mSlotN, aWaitTrans->mSlotN )
             == ID_TRUE )
        {
            sMyMutexLocked = ID_FALSE;
            /* always return true */
            unlock();

            return IDE_SUCCESS;
        }
    }
    else
    {
        sTxMutexUnlocked = ID_TRUE;
        IDE_TEST_RAISE( aMutex->unlock() != IDE_SUCCESS,
                        err_mutex_unlock );
    }

    if ( mStatus == SMX_TX_BLOCKED )
    {
        IDE_TEST( smLayerCallback::lockWaitFunc( *aTotalWaitMicroSec, &sWaited )
                  != IDE_SUCCESS );
    }

    if ( aCheckDistDeadlock == ID_TRUE )
    {
        sIntervalMicroSec = (1 * 1000 * 100); /* 0.1 sec */
    }
    else
    {
        /* 분산데드락 탐지로 호출된것이 아니라면, 
           기존과 동일하게 대기 interval을 3초로 설정한다. */
        sIntervalMicroSec = (3 * 1000 * 1000); /* 3 sec */
    }

    while( mStatus == SMX_TX_BLOCKED )
    {
        /* BUG-18965: LOCK TABLE구문에서 QUERY_TIMEOUT이 동작하지
         * 않습니다.
         *
         * 사용자가 LOCK TIME OUT을 지정하게 되면 무조건 LOCK TIME
         * OUT때까지 무조건 기다렸다. 하지만 QUERY TIMEOUT, SESSION
         * TIMEOUT을 체크하지 않아서 제때 에러를 내지 못하고 LOCK
         * TIMEOUT때까지 기다려야 한다. 이런 문제를 해결하기 위해서
         * 주기적으로 깨서 mStatistics와 기다린 시간이 LOCK_TIMEOUT을
         * 넘었는지 검사한다.
         * */
        if ( sTotalWaitMicroSec <= sElapsedMicroSec )
        {
            break;
        }

        sSleepMicroSec = ( ( ( sTotalWaitMicroSec - sElapsedMicroSec ) < sIntervalMicroSec )
                           ? ( sTotalWaitMicroSec - sElapsedMicroSec ) : sIntervalMicroSec );

        sElapsedMicroSec += sSleepMicroSec;

        sTimeWait.microsec( sSleepMicroSec );
        sTimeVal = idlOS::gettimeofday();
        sTimeVal += sTimeWait;

        IDE_TEST_RAISE(mCondV.timedwait(&mMutex, &sTimeVal, IDU_IGNORE_TIMEDOUT)
                       != IDE_SUCCESS, err_cond_wait);

        /*******************************
         * PROJ-2734 
         * 분산데드락을 체크한다.
         ******************************/
        if ( aCheckDistDeadlock == ID_TRUE )
        {
            /* isCycle() 재호출하기전에 WaitForTable의 자신의 행을 정리해준다. */
            smlLockMgr::revertWaitItemColsOfTrans( mSlotN );

            if ( smLayerCallback::isCycle( mSlotN, ID_TRUE ) == ID_FALSE )
            {
                sDetection = smlLockMgr::detectDistDeadlock( mSlotN, &sNewTotalWaitMicroSec );

                /*************************************************/
                /* CASE 1 : 분산데드락 미탐지 -> 분산데드락 탐지 */
                /*************************************************/
                if ( SMX_DIST_DEADLOCK_IS_NOT_DETECTED( aDistDeadlock ) &&
                     SMX_DIST_DEADLOCK_IS_DETECTED( sDetection ) )
                {
                    /* LOCK WAIT중에 분산데드락을 새롭게 탐지 */
                    sNewDistDeadlock   = sDetection;
                    sTotalWaitMicroSec = sNewTotalWaitMicroSec;
                    break;
                }

                /*************************************************/
                /* CASE 2 : 분산데드락 탐지 -> 분산데드락 탐지 */
                /*************************************************/
                else if ( SMX_DIST_DEADLOCK_IS_DETECTED( aDistDeadlock ) &&
                          SMX_DIST_DEADLOCK_IS_DETECTED( sDetection ) )
                {
                    /* 분산레벨이 변경되었다면, TIMEOUT 시간이 변경될수있다. */
                    if ( sNewTotalWaitMicroSec != sTotalWaitMicroSec )
                    {
                        #ifdef DEBUG
                        ideLog::log( IDE_SD_19,
                                     "\n<Detected Distribution Deadlock : Change Die Timeout >\n"
                                     "WaiterTx(%"ID_UINT32_FMT") WaitTime : "
                                     "%"ID_UINT64_FMT"us -> %"ID_UINT64_FMT"us (elapsed %"ID_UINT64_FMT"us) \n",
                                     mTransID,
                                     sTotalWaitMicroSec,
                                     sNewTotalWaitMicroSec,
                                     sElapsedMicroSec );
                        #endif

                        /* 새로운 TOTAL WAIT 시간으로 변경 */
                        sTotalWaitMicroSec = sNewTotalWaitMicroSec;
                        /* Performance View를 위한 정보 갱신
                           (탐지원인은 변경하지 않는다.) */
                        mDistDeadlock4FT.mDieWaitTime = sNewTotalWaitMicroSec;
                    }

                    mDistDeadlock4FT.mElapsedTime = sElapsedMicroSec;
                }

                /*************************************************/
                /* CASE 3 : 분산데드락 탐지 -> 분산데드락 미탐지 */
                /*************************************************/
                else if ( SMX_DIST_DEADLOCK_IS_DETECTED( aDistDeadlock ) &&
                          SMX_DIST_DEADLOCK_IS_NOT_DETECTED( sDetection ) )
                {
                    /* 분산데드락이 풀렸다 */
                    sIsReleasedDistDeadlock = ID_TRUE;

                    /* Performance View를 위한 정보 갱신 */
                    mDistDeadlock4FT.mDetection = SMX_DIST_DEADLOCK_DETECTION_NONE;

                    break;
                }

                /*************************************************/
                /* CASE 4 : 분산데드락 미탐지 -> 분산데드락 미탐지 */
                /*************************************************/
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* 로컬데드락이 탐지되었다.
                   이경우는 로컬데드락을 새로 발생시킨 TX가 처리할것이므로,
                   내 TX는 아무것도 하지않아도 된다. */
            }
        } /* if ( aCheckDistDeadlokc == ID_TRUE ) */
        
        IDE_TEST( iduCheckSessionEvent( mStatistics )
                  != IDE_SUCCESS );

        /* BUG-47472 DBHang 현상 관련 디버그 코드 추가 */
        if( SMI_IS_LOCK_DEBUG_INFO_ENABLE(mFlag) )
        {
            if ( sDumped == ID_FALSE )
            {
                sDumped = ID_TRUE;

                smlLockMgr::dumpLockWait();
                smlLockMgr::dumpLockTBL();
            }
        }
    }

    if ( sWaited == ID_TRUE )
    {
        IDE_TEST( smLayerCallback::lockWakeupFunc() != IDE_SUCCESS );
    }

    sMyMutexLocked = ID_FALSE;
    /* always return true */
    unlock();

    if( aTotalWaitMicroSec != NULL )
    {
        *aTotalWaitMicroSec = sTotalWaitMicroSec;
    }

    if( aElapsedMicroSec != NULL )
    {
        *aElapsedMicroSec = sElapsedMicroSec;
    }

    if ( aNewDistDeadlock != NULL )
    {
        *aNewDistDeadlock = sNewDistDeadlock;
    }

    if ( aIsReleasedDistDeadlock != NULL )
    {
        *aIsReleasedDistDeadlock = sIsReleasedDistDeadlock;
    }

    if ( sTxMutexUnlocked == ID_TRUE )
    {
        sTxMutexUnlocked = ID_FALSE;
        IDE_TEST_RAISE( aMutex->lock( NULL ) != IDE_SUCCESS,
                       err_mutex_lock);
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(err_selflock);
    {
        ideLog::log(IDE_SM_0, SM_TRC_WAIT_SELF_WARNING, mSlotN);
        IDE_SET(ideSetErrorCode(smERR_ABORT_Aborted));
    }

    IDE_EXCEPTION(err_mutex_unlock);
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION(err_cond_wait);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION(err_mutex_lock);
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION_END;

    if ( sMyMutexLocked ==  ID_TRUE )
    {
        /* always return true */
        unlock();
    }

    if ( sTxMutexUnlocked == ID_TRUE )
    {
        IDE_ASSERT( aMutex->lock( NULL ) == IDE_SUCCESS );
    }

    // fix BUG-11228.
    if (sWaited == ID_TRUE )
    {
        IDE_ASSERT( smLayerCallback::lockWakeupFunc() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::resume()
{
    UInt sState = 0;

    /* always return true */
    lock();
    sState = 1;

    if( mStatus == SMX_TX_BLOCKED )
    {
        mStatus    = SMX_TX_BEGIN;
        mStatus4FT = SMX_TX_BEGIN;

        IDE_TEST_RAISE(mCondV.signal() != IDE_SUCCESS, err_cond_signal);
        //fix bug-9627.
    }
    else
    {
        /* BUG-43595 새로 alloc한 transaction 객체의 state가
         * begin인 경우가 있습니다. 로 인한 디버깅 정보 출력 추가*/
        ideLog::log(IDE_ERR_0,"Resume error, Transaction is not blocked.\n");
        dumpTransInfo();
        ideLog::logCallStack(IDE_ERR_0);
        IDE_DASSERT(0);
    }

    sState = 0;
    /* always return true */
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cond_signal);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        /* always return true */
        unlock();
    }

    return IDE_FAILURE;

}


IDE_RC smxTrans::begin( idvSQL * aStatistics,
                        UInt     aFlag,
                        UInt     aReplID,
                        idBool   aIsServiceTX )
{
    UInt sState = 0;
    UInt sFlag  = 0;

    mReplLockTimeout = smuProperty::getReplLockTimeOut();
    /* mBeginLogLSN과 CommitLogLSN은 recovery from replication에서
     * 사용하며, commit이 호출된 이후에 사용되므로, 반드시
     * begin에서 초기화 하여야 하며, init함수는 commit할 때
     * 호출되므로 init에서 초기화 하면 안된다.
     */
    SM_LSN_MAX( mBeginLogLSN );
    SM_LSN_MAX( mCommitLogLSN );

    SM_SET_SCN_INFINITE_AND_TID( &mInfinite, mTransID );

    IDE_TEST( mCommitState != SMX_XA_COMPLETE );

    /* PROJ-1381 Fetch Across Commits
     * SMX_TX_END가 아닌 TX의 MinViewSCN 중 가장 작은 값으로 aging하므로,
     * Legacy Trans가 있으면 mStatus를 SMX_TX_END로 변경해서는 안된다.
     * SMX_TX_END 상태로 변경하면 순간적으로 cursor의 view에 속한 row가
     * aging 대상이 되어 사라질 수 있다. */
    if ( mLegacyTransCnt == 0 )
    {
        IDE_TEST( mStatus != SMX_TX_END );
    }
#ifdef DEBUG
    // PROJ-2068 Begin시 DPathEntry는 NULL이어야 한다.
    IDE_TEST( mDPathEntry != NULL );
#endif
    /*
     * BUG-42927
     * mEnabledTransBegin == ID_FALSE 이면, smxTrans::begin()도 대기하도록 한다.
     *
     * 왜냐하면,
     * non-autocommit 트랜젝션의 경우는 statement 시작시
     * smxTransMgr::alloc()을 호출하지 않고, smxTrans::begin()만 호출하기 때문이다.
     */
    while ( 1 )
    {
        if ( smxTransMgr::mEnabledTransBegin == ID_TRUE )
        {
            break;
        }
        else
        {
            idlOS::thr_yield();
        }
    }

    mOIDList->init();
    mOIDFreeSlotList.init();

    mSvpMgr.initialize( this );
    mStatistics = aStatistics;
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    // transaction begin시 session id를 설정함.
    if ( aStatistics != NULL )
    {
        if ( aStatistics->mSess != NULL )
        {
            mSessionID = aStatistics->mSess->mSID;
        }
        else
        {
            mSessionID = ID_NULL_SESSION_ID;
        }
    }
    else
    {
        mSessionID = ID_NULL_SESSION_ID;
    }

    // Disk 트랜잭션을 위한 Touch Page List 초기화
    mTouchPageList.init( aStatistics );


    /* always return true */
    lock();
    sState = 1;

    mStatus    = SMX_TX_BEGIN;
    mStatus4FT = SMX_TX_BEGIN;

    // To Fix BUG-15396
    // mFlag에는 다음 세가지 정보가 설정됨
    // (1) transaction replication mode
    // (2) commit write wait mode
    mFlag = aFlag;

    // For XA: There is no sense for local transaction
    mCommitState = SMX_XA_START;

    /* BUG-47223 */
    mIsServiceTX = aIsServiceTX;

    //PROJ-1541 eager replication Flag Set
    /* PROJ-1608 Recovery From Replication
     * SMX_REPL_NONE(replication하지 않는 트랜잭션-system 트랜잭션) OR
     * SMX_REPL_REPLICATED(recovery를 지원하지 않는 receiver가 수행한 트랜잭션)인 경우에
     * 로그를 Normal Sender가 볼 필요가 없으므로, SMR_LOG_TYPE_REPLICATED로 설정
     * 그렇지 않고 SMX_REPL_RECOVERY(repl recovery를 지원하는 receiver가 수행한 트랜잭션)인
     * 경우 SMR_LOG_TYPE_REPL_RECOVERY로 설정하여 로그를 남길 때,
     * Recovery Sender가 볼 수 있도록 RP를 위한 정보를 남길 수 있도록 한다.
     */
    sFlag = mFlag & SMX_REPL_MASK;

    if ( ( sFlag == SMX_REPL_REPLICATED ) || ( sFlag == SMX_REPL_NONE ) )
    {
        mLogTypeFlag = SMR_LOG_TYPE_REPLICATED;
    }
    else
    {
        if ( ( sFlag == SMX_REPL_RECOVERY ) )
        {
            mLogTypeFlag = SMR_LOG_TYPE_REPL_RECOVERY;
        }
        else
        {
            mLogTypeFlag = SMR_LOG_TYPE_NORMAL;
        }
    }
    // PROJ-1553 Replication self-deadlock
    // tx를 begin할 때, replication에 의한 tx일 경우
    // mReplID를 받는다.
    mReplID = aReplID;
    //PROJ-1541 Eager/Acked replication
    SM_LSN_MAX( mLastWritedLSN );
    
    if ( SMI_IS_GCTX_ON( mFlag ) )
    {
        mIsGCTx = ID_TRUE;
    }

    /* BUG-48250 */
    if ( mIsGCTx == ID_TRUE )
    {
        mIndoubtFetchTimeout = smxTransMgr::getSessIndoubtFetchTimeout( aStatistics );
        mIndoubtFetchMethod  = smxTransMgr::getSessIndoubtFetchMethod( aStatistics );
    }
    else
    {
        /* GCTx가 아닌경우 사용되지 않지만, DEFAULT 값으로 넣어둔다. */
        mIndoubtFetchTimeout = (UInt)IDP_SHARD_PROPERTY_INDOUBT_FETCH_TIMEOUT_DEFAULT;
        mIndoubtFetchMethod  = (UInt)IDP_SHARD_PROPERTY_INDOUBT_FETCH_METHOD_DEFAULT;
    }

#ifdef DEBUG
    ideLog::log( IDE_SD_19,
                 "\n<smxTrans(%"ID_XINT64_FMT"-%"ID_UINT32_FMT")::begin>\n",
                 this, mTransID );
#endif

    sState = 0;
    /* always return true */
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    /* BUG-46782 Begin transaction 디버깅 정보 추가.
     * 기존에 ASSERT 였던 검증 값을 TEST로 올리고
     * 상위에서 ASSERT 여부를 판단하도록 수정*/
    smxTrans::dumpTransInfo();

    ideLog::log( IDE_ERR_0,
                 "begin Transaction error :\n"
                 "CommitState : %"ID_UINT32_FMT"\n"
                 "Status      : %"ID_UINT32_FMT"\n"
                 "CompResPtr  : %"ID_xPOINTER_FMT"\n"
                 "DPathEntry  : %"ID_xPOINTER_FMT"\n",
                 mCommitState,
                 mStatus,
                 mCompRes,
                 mDPathEntry );

    if ( sState != 0 )
    {
        /* always return true */
        unlock();
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::commit( smSCN  * aCommitSCN,
                         idBool   aIsLegacyTrans,
                         void  ** aLegacyTrans )
{
    smLSN           sCommitEndLSN   = {0, 0};
    sdcTSSegment  * sCTSegPtr;
    idBool          sWriteCommitLog = ID_FALSE;
    smxOIDList    * sOIDList        = NULL;
    UInt            sState          = 0;
    idBool          sNeedWaitTransForRepl = ID_FALSE;
    idvSQL        * sTempStatistics       = NULL;
    smTID           sTempTransID          = SM_NULL_TID;
    smLSN           sTempLastWritedLSN;
    UInt            sGCList = 0;
    //fux BUG-27468 ,Code-Sonar mmqQueueInfo::wakeup에서 aCommitSCN UMR
    smSCN           sCommitSCN = SM_SCN_INIT;
#ifdef DEBUG
    smSCN           sLstSystemSCN;
    smSCN           sDebugCommitSCN;
#endif 

    IDU_FIT_POINT("1.smxTrans::smxTrans:commit");

    IDE_DASSERT( mStatus    != SMX_TX_END );
    IDE_DASSERT( mStatus4FT == SMX_TX_BEGIN );

    mStatus4FT = SMX_TX_COMMIT;
    sState = 1;


    /* PROJ-2733 공유된 CommitSCN으로 커밋을 시도 한다. */
    if ( (aCommitSCN != NULL) && SM_SCN_IS_NOT_INIT(*aCommitSCN) )
    {
        IDE_DASSERT( mIsGCTx == ID_TRUE );
        IDE_DASSERT( SM_SCN_IS_VIEWSCN( *aCommitSCN ) == ID_FALSE );
        IDE_DASSERT( SM_SCN_IS_SYSTEMSCN( *aCommitSCN ) == ID_TRUE );

        SM_SET_SCN( &sCommitSCN, aCommitSCN );
    }
    else
    {
        /* Global Consistent Transaction도 commitSCN 을 보내지 않을수 있다.
         * e.g. 내부에서 사용하는거라던가.. */
    }

    // PROJ-2068 Direct-Path INSERT를 수행했을 경우 commit 작업을 수행한다.
    if ( mDPathEntry != NULL )
    {
        IDE_TEST( sdcDPathInsertMgr::commit( mStatistics,
                                             this,
                                             mDPathEntry )
                  != IDE_SUCCESS );

        IDE_TEST( mTableInfoMgr.processAtDPathInsCommit() != IDE_SUCCESS );
    }

    if ( mIsUpdate == ID_TRUE )
    {
        IDU_FIT_POINT( "1.BUG-23648@smxTrans::commit" );

        /* 갱신을 수행한 Transaction일 경우 */
        /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다. */
        if ( ( mIsTransWaitRepl == ID_TRUE ) && ( isReplTrans() == ID_FALSE ) )
        {
            if (smrRecoveryMgr::mIsReplCompleteBeforeCommitFunc != NULL )
            {
                IDE_TEST( smrRecoveryMgr::mIsReplCompleteBeforeCommitFunc(
                                                     mStatistics,
                                                     mTransID,
                                                     SM_MAKE_SN(mLastWritedLSN),
                                                     ( mFlag & SMX_REPL_MASK ))
                         != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            sNeedWaitTransForRepl = ID_TRUE;
        }
        else
        {
            /* do nothing */
        }

        IDU_FIT_POINT_RAISE("9.TASK-7220@smxTrans::commit",err_internal_server_error);
        IDU_FIT_POINT_RAISE("5.TASK-7220@smxTrans::commit",err_internal_server_error);

        /* FIT PROJ-2569 before commit logging and communicate */
        IDU_FIT_POINT( "smxTrans::commit::writeCommitLog::BEFORE_WRITE_COMMIT_LOG" );

        if ( ( mTXSegEntry == NULL ) && 
             ( hasPendingOp() == ID_FALSE ) && 
             ( mIsDDL  == ID_FALSE ) &&
             ( mGroupCnt != 0 ) &&
             ( sNeedWaitTransForRepl == ID_FALSE ) )
        {
            IDE_TEST( addTID4GroupCommit( &sGCList, 
                                          &sWriteCommitLog,
                                          sCommitSCN ) 
                      != IDE_SUCCESS );

            if ( sWriteCommitLog == ID_TRUE )
            {
                /* PROJ-1608 recovery from replication Commit Log LSN Set
                 * BUG-47865 내가 기록 했다면 동일해야 한다. */
                IDE_ASSERT( smrCompareLSN::isEQ( &mCommitLogLSN,
                                                 &mLastWritedLSN ) == ID_TRUE );

                /* addTID4GroupCommit에서 writeCommitLog가 기록되었다면 직접 기록한 경우이다.
                 * Tx의 LastWritedLSN이 CommitLog가 기록된 LSN 이다. */
                SM_GET_LSN( sCommitEndLSN, mCommitLogLSN );
        
                /* Commit Log가 Flush될때까지 기다리지 않은 경우 */
                IDE_TEST( flushLog( &sCommitEndLSN, ID_TRUE /* When Commit */ )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Commit Log를 기록한다 */
            /* BUG-32576  [sm_transaction] The server does not unlock
             * PageListMutex if the commit log writing occurs an exception.
             * CommitLog를 쓰던 중 예외가 발생하면 PageListMutex가 풀리지 않
             * 습니다. 그 외에도 CommitLog를 쓰다가 예외가 발생하면 DB가
             * 잘못될 수 있는 위험이 있습니다. */
            IDE_TEST( writeCommitLog( &sCommitEndLSN, sCommitSCN ) != IDE_SUCCESS );
            sWriteCommitLog = ID_TRUE;
            
            /* PROJ-1608 recovery from replication Commit Log LSN Set */
            SM_GET_LSN( mCommitLogLSN, mLastWritedLSN );

            /* Commit Log가 Flush될때까지 기다리지 않은 경우 */
            IDE_TEST( flushLog( &sCommitEndLSN, ID_TRUE /* When Commit */ )
                      != IDE_SUCCESS );

        }

        /* FIT PROJ-2569 After commit logging and communicate */
        IDU_FIT_POINT( "smxTrans::commit::writeCommitLog::AFTER_WRITE_COMMIT_LOG" );
    }
    else
    {
        if ( svrLogMgr::isOnceUpdated(&mVolatileLogEnv) == ID_TRUE )
        {
            /* 다른 TBS에 갱신이 없고 Volatile TBS에만 갱신이 있는 경우
               레코드 카운트를 증가시켜야 한다. */
            IDE_TEST( mTableInfoMgr.requestAllEntryForCheckMaxRow()
                      != IDE_SUCCESS );

            IDE_TEST( mTableInfoMgr.releaseEntryAndUpdateMemTableInfoForIns()
                      != IDE_SUCCESS );
        }
    }

    /* Tx's PrivatePageList를 정리한다.
       반드시 위에서 먼저 commit로그를 write하고 작업한다. */
    IDE_TEST( finAndInitPrivatePageList() != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS의 private page list도 정리한다. */
    IDE_TEST( finAndInitVolPrivatePageList() != IDE_SUCCESS );

    /* BUG-14093 Ager Tx가 FreeSlot한 것들을 실제 FreeSlotList에
     * 매단다. */
    IDE_TEST( smLayerCallback::processFreeSlotPending( this,
                                                       &mOIDFreeSlotList )
              != IDE_SUCCESS );

    /* BUG-47525 GroupCommit 이 활성화 되어 있어야만
     * mIsUpdate가 True일때 sWriteCommitLog가 False로 넘어올수 있다.
     * sWriteCommitLog가 False 일 경우.
     * 다른쪽에서 Write가 되었는지 확인하고 되었다면 Write 표시하고 넘어가고
     * 아직 안되었다면 직접 Write 하거나 write되길 기다렸다가 넘어가도록 한다. */
    if ( (mIsUpdate == ID_TRUE) && (sWriteCommitLog == ID_FALSE) )
    {
        IDE_TEST( waitGroupCommit( sGCList ) != IDE_SUCCESS );

        /* 여기까지 왔으면 직접 write를 하던가 write가 된걸 확인했다. */
        sWriteCommitLog = ID_TRUE;

        /* BUG-47865 group commit log write 하는 trans가
         * 나의 CommitLogLSN을 갱신해 두었다.
         * 내가 만약 직접 했어도 마찬가지 이다.*/
        SM_GET_LSN( sCommitEndLSN, mCommitLogLSN );

        /* PROJ-1608 recovery from replication Commit Log LSN Set
         * BUG-47865 Last Writed LSN도 갱신한다.
         * 만약 내가 직접 했다면 이미 갱신되어 있을것이다. */
        SM_GET_LSN( mLastWritedLSN, mCommitLogLSN );
        
        /* Commit Log가 Flush될때까지 기다리지 않은 경우 */
        IDE_TEST( flushLog( &sCommitEndLSN, ID_TRUE /* When Commit */ )
                  != IDE_SUCCESS );
    }
    
    /* 갱신을 수행한 Transaction일 경우 */
    /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다.
     * BUG-35452 lock 전에 commit log가 standby에 반영된 것을 확인해야한다. */
    /* BUG-42736 Loose Replication 역시 Remote 서버의 반영을 대기 하여야 합니다. */
    if ( sNeedWaitTransForRepl == ID_TRUE )
    {
        if ( smrRecoveryMgr::mIsReplCompleteAfterCommitFunc != NULL )
        {
            smrRecoveryMgr::mIsReplCompleteAfterCommitFunc(
                                                    mStatistics,
                                                    mTransID,
                                                    SM_MAKE_SN(mFstUndoNxtLSN),
                                                    SM_MAKE_SN(mLastWritedLSN),
                                                    ( mFlag & SMX_REPL_MASK ),
                                                    SMI_BEFORE_LOCK_RELEASE);
        }
    }
    else
    {
        /* do nothing */
    }

    if ( mGlobalSMNChangeFunc != NULL )
    {
        IDE_DASSERT(mStatistics != NULL);
        IDE_DASSERT(mStatistics->mSess != NULL);
        IDE_DASSERT(mStatistics->mSess->mSession != NULL);

        mGlobalSMNChangeFunc( mStatistics->mSess->mSession );
    }

    /* ovl에 있는 OID리스트의 lock과 SCN을 갱신하고
     * pol과 ovl을 logical ager에게 넘긴다. */
    if ( (mTXSegEntry != NULL ) ||
         (mOIDList->isEmpty() == ID_FALSE) )
    {
        /* PROJ-1381 Fetch Across Commits
         * Legacy Trans이면 OID List를 Logical Ager에 추가만 하고,
         * Commit 관련 연산은 Legacy Trans를 닫을 때 처리한다. */
        IDE_TEST( addOIDList2AgingList( SM_OID_ACT_AGING_COMMIT,
                                        SMX_TX_COMMIT,
                                        &sCommitEndLSN,
                                        &sCommitSCN,
                                        aIsLegacyTrans )
                  != IDE_SUCCESS );
#ifdef DEBUG
        /* 이 함수가 호출되기전에 aCommitSCN으로 SCN sync 되어 있는 상태. 
         * 그사이 LstSystemSCN 이 증가할수는 있지만 aCommitSCN보다 작을수는 없음 */
        sLstSystemSCN = smmDatabase::getLstSystemSCN();
        SM_GET_SCN( &sDebugCommitSCN, &sCommitSCN );
        if ( aIsLegacyTrans == ID_TRUE )
        {
            SM_CLEAR_SCN_LEGACY_BIT( &sDebugCommitSCN );
        }
        IDE_DASSERT( SM_SCN_IS_GE( &sLstSystemSCN, &sDebugCommitSCN ) );
        IDE_DASSERT( SM_SCN_IS_EQ( &sCommitSCN, &mCommitSCN ) ); 
#endif
    }
    else
    {
        mStatus = SMX_TX_COMMIT;
    }

    /* BUG-48586
     * CommitSCN이 구해진 다음에 호출되야 합니다. 
     * Partition Swap 은 X Lock을 잡고 수행되니 TX.commitSCN이 설정 <-(a)-> AccessSCN이 설정
     * 위 (a) 구간에서 begin stmt 가 되는것은 고려하지 않아도 됩니다. */
    if ( mInternalTableSwap == ID_TRUE )
    {
        smxTransMgr::setGlobalConsistentSCNAsSystemSCN();
    }

    /* PRJ-1704 Disk MVCC Renewal */
    if ( mTXSegEntry != NULL )
    {
        /*
         * [BUG-27542] [SD] TSS Page Allocation 관련 함수(smxTrans::allocTXSegEntry,
         *             sdcTSSegment::bindTSS)들의 Exception처리가 잘못되었습니다.
         */
        if ( mTXSegEntry->mTSSlotSID != SD_NULL_SID )
        {
            IDE_DASSERT( SM_SCN_IS_NOT_INIT(sCommitSCN) );

            sCTSegPtr = sdcTXSegMgr::getTSSegPtr( mTXSegEntry );

            IDE_TEST( sCTSegPtr->unbindTSS4Commit( mStatistics,
                                                   mTXSegEntry->mTSSlotSID,
                                                   &sCommitSCN )
                      != IDE_SUCCESS );

            /* BUG-29280 - non-auto commit D-Path Insert 과정중
             *             rollback 발생한 경우 commit 시 서버 죽는 문제
             *
             * DPath INSERT 에서는 Fast Stamping을 수행하지 않는다. */
            if ( mDPathEntry == NULL )
            {
                IDE_TEST( mTouchPageList.runFastStamping( &sCommitSCN )
                          != IDE_SUCCESS );
            }

            /*
             * 트랜잭션 Commit은 CommitSCN을 시스템으로부터 따야하기 때문에
             * Commit로그와 UnbindTSS 연산을 atomic하게 처리할 수 없다.
             * 그러므로, Commit 로깅후에 unbindTSS를 수행해야한다.
             * 주의할 점은 Commit 로깅시에 TSS에 대해서 변경없이
             * initSCN을 설정하는 로그를 남겨주어야 서버 Restart시에
             * Commit된 TSS가 InfinteSCN을 가지는 문제가 발생하지 않는다.
             */
            IDE_TEST( sdcTXSegMgr::markSCN( mStatistics,
                                            mTXSegEntry,
                                            &sCommitSCN ) 
                      != IDE_SUCCESS );
        }

        sdcTXSegMgr::freeEntry( mTXSegEntry,
                                ID_TRUE /* aMoveToFirst */ );
        mTXSegEntry = NULL;
    }

    /* 디스크 관리자를 위한 pending operation들을 수행 */
    IDU_FIT_POINT( "2.PROJ-1548@smxTrans::commit" );

    IDE_TEST( executePendingList( ID_TRUE ) != IDE_SUCCESS );

    IDU_FIT_POINT( "3.PROJ-1548@smxTrans::commit" );

    /* PROJ-1594 Volatile TBS */
    if (( mIsUpdate == ID_TRUE ) ||
        ( svrLogMgr::isOnceUpdated( &mVolatileLogEnv )
          == ID_TRUE ))
    {
        mTableInfoMgr.updateMemTableInfoForDel();
    }

    /* PROJ-1594 Volatile TBS */
    /* commit시에 로깅한 모든 로그들을 지운다. */
    if ( svrLogMgr::isOnceUpdated( &mVolatileLogEnv ) == ID_TRUE )
    {
        IDE_TEST( svrLogMgr::removeLogHereafter(
                             &mVolatileLogEnv,
                             SVR_LSN_BEFORE_FIRST )
                  != IDE_SUCCESS );
    }

    /* PROJ-1381 Fetch Across Commits - Legacy Trans를 List에 추가한다. */
    if ( aIsLegacyTrans == ID_TRUE )
    {
        IDE_TEST( smxLegacyTransMgr::addLegacyTrans( this,  
                                                     sCommitEndLSN,
                                                     aLegacyTrans,
                                                     ID_TRUE /* aIsCommit */ )
                  != IDE_SUCCESS );

        /* PROJ-1381 Fetch Across Commits
         * smxOIDList를 Legacy Trans에 달아주었으므로,
         * 새로운 smxOIDList를 트랜젝션에 할당한다.
         *
         * Memory 할당에 실패하면 그냥 예외 처리한다.
         * trunk에서는 예외 처리시 바로 ASSERT로 종료한다. */
        /* smxTrans_commit_malloc_OIDList.tc */
        IDU_FIT_POINT("smxTrans::commit::malloc::OIDList");
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                     ID_SIZEOF(smxOIDList),
                                     (void**)&sOIDList )
                  != IDE_SUCCESS );
        sState = 2;

        IDE_TEST( sOIDList->initialize( this,
                                        mCacheOIDNode4Insert,
                                        ID_FALSE, // aIsUnique
                                        &smxOIDList::addOIDWithCheckFlag )
                  != IDE_SUCCESS );

        mOIDList = sOIDList;

        mLegacyTransCnt++;
    }

    sTempStatistics   = mStatistics;
    sTempTransID      = mTransID;
    SM_GET_LSN( sTempLastWritedLSN, mLastWritedLSN );

    /* 트랜잭션이 획득한 모든 lock을 해제하고 트랜잭션 엔트리를
     * 초기화한 후 반환한다. */
    IDE_TEST( end() != IDE_SUCCESS );

    /* 갱신을 수행한 Transaction일 경우 */
    /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다.
     * BUG-35452 lock 전에 commit log가 standby에 반영된 것을 확인해야한다.*/
    /* BUG-42736 Loose Replication 역시 Remote 서버의 반영을 대기 하여야 합니다. */
    if ( sNeedWaitTransForRepl == ID_TRUE )
    {
        if (smrRecoveryMgr::mIsReplCompleteAfterCommitFunc != NULL )
        {
            smrRecoveryMgr::mIsReplCompleteAfterCommitFunc(
                                                sTempStatistics,
                                                sTempTransID,
                                                SM_MAKE_SN( mFstUndoNxtLSN ),
                                                SM_MAKE_SN( sTempLastWritedLSN ),
                                                ( mFlag & SMX_REPL_MASK ),
                                                SMI_AFTER_LOCK_RELEASE);
        }
    }
    else
    {
        /* do nothing */
    }

    if ( aCommitSCN != NULL )
    {
        if ( aIsLegacyTrans == ID_TRUE )
        {
            SM_CLEAR_SCN_LEGACY_BIT( &sCommitSCN );
        }
        SM_SET_SCN( aCommitSCN, &sCommitSCN );
        IDE_DASSERT( SM_SCN_IS_SYSTEMSCN( *aCommitSCN ) )
    }
    
    return IDE_SUCCESS;

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( err_internal_server_error )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL) );
    }
#endif
    IDE_EXCEPTION_END;

#ifdef DEBUG
    extern UInt ideNoErrorYet();
    
    if( ideNoErrorYet() == ID_TRUE )
    {
        /* 에러메시지를 설정하지 않으면 에러를 무시할 수 있습니다. 
         * 어디서 설정하지 않는지 확인이 필요합니다. */
        IDE_DASSERT(0);
    }
#endif

    switch( sState )
    {
        case 2:
            IDE_ASSERT( iduMemMgr::free( sOIDList ) == IDE_SUCCESS );
            sOIDList = NULL;
        case 1:
            mStatus4FT = SMX_TX_BEGIN; 
        default:
            break;
    }

    /* Commit Log를 남긴 후에는 예외처리하면 안된다. */
    IDE_ASSERT( sWriteCommitLog == ID_FALSE );

    return IDE_FAILURE;
}

/*
    TASK-2401 MMAP Logging환경에서 Disk/Memory Log의 분리

    Disk 에 속한 Transaction이 Memory Table을 참조한 경우
    Disk 의 Log를 Flush

    aLSN - [IN] Sync를 어디까지 할것인가?
*/
IDE_RC smxTrans::flushLog( smLSN *aLSN, idBool aIsCommit )
{
    smLSN           sLstLSN;
    PDL_Time_Value  sSleep; 
    UInt            sSleepNSec = 0;


    /* BUG-21626 : COMMIT WRITE WAIT MODE가 정상동작하고 있지 않습니다. */
    if ( ( IDL_LIKELY_FALSE( ( mFlag & SMX_COMMIT_WRITE_MASK ) == SMX_COMMIT_WRITE_WAIT ) )&&
         ( aIsCommit == ID_TRUE ) && 
         ( !( SM_IS_LSN_MAX(*aLSN) ) ) )
    {
        IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_TRX,
                                           aLSN )
                  != IDE_SUCCESS );
    }
    else
    {
        /* BUG-45711 commit 이전에 dummy가 없어야 한다. */
        if ( ( IDL_LIKELY_TRUE( mIsUncompletedLogWait == ID_TRUE ) )  &&
             ( IDL_LIKELY_TRUE( !SM_IS_LSN_MAX(*aLSN) ) ) )
        {
            smrLogMgr::getUncompletedLstLSN( &sLstLSN );

            if (smrCompareLSN::isGT( aLSN, &sLstLSN ) )
            {
                /* UncompletedLstLSN 을 갱신해준다. */
                smrLogMgr::rebuildMinUCSN();
                smrLogMgr::getUncompletedLstLSN( &sLstLSN );

                while ( smrCompareLSN::isGT( aLSN, &sLstLSN ) )
                {
                    /* 잔다. */
                    sSleep.set( 0, sSleepNSec );
                    idlOS::sleep( sSleep );

                    if ( sSleepNSec == 0 )
                    {
                        sSleepNSec++;
                    }
                
                    /* 다시 확인 */
                    smrLogMgr::rebuildMinUCSN();
                    smrLogMgr::getUncompletedLstLSN( &sLstLSN );
                } // while 
            } //if
        } // if
        else
        {
            /* nothing to do */
        }

    }//else 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************
 *
 * BUG-22576
 *  transaction이 partial rollback될 때 DB로부터 할당한
 *  페이지들이 있다면 이들을 table에 미리 매달아야 한다.
 *  왜냐하면 partial rollback될 때 객체에 대한 lock을 풀 경우가
 *  있는데, 그러면 그 객체가 어떻게 될지 모르기 때문에
 *  미리 page들을 table에 매달아놔야 한다.
 *
 *  이 함수는 partial abort시 lock을 풀기 전에
 *  반드시 불려야 한다.
 *****************************************************************/
IDE_RC smxTrans::addPrivatePageListToTableOnPartialAbort()
{
    // private page list를 table에 달기 전에
    // 반드시 log를 sync해야 한다.
    IDE_TEST( flushLog(&mLstUndoNxtLSN, ID_FALSE /*not commit*/)
              != IDE_SUCCESS );

    IDE_TEST( finAndInitPrivatePageList() != IDE_SUCCESS );

    IDE_TEST( finAndInitVolPrivatePageList() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxTrans::abort( idBool    aIsLegacyTrans, 
                        void   ** aLegacyTrans )
{
    smLSN      sAbortEndLSN = {0,0};
    smSCN      sDummySCN;
    smSCN      sSysSCN;
    idBool     sNeedWaitTransForRepl = ID_FALSE;
    idvSQL   * sTempStatistics       = NULL;
    smTID      sTempTransID          = SM_NULL_TID;
    smLSN      sTempLastWritedLSN;
    smxStatus  sPreStatus4FT = mStatus4FT;
    smxStatus  sPreStatus    = mStatus;
    UInt       sState = 0;
    
    IDE_DASSERT( mStatus    != SMX_TX_END );
    IDE_DASSERT( mStatus4FT == SMX_TX_BEGIN );
     
    mStatus4FT = SMX_TX_ABORT;
    mStatus    = SMX_TX_ABORT;

    // PROJ-2068 Direct-Path INSERT를 수행하였을 경우 abort 작업을 수행해 준다.
    if ( mDPathEntry != NULL )
    {
        IDE_TEST( sdcDPathInsertMgr::abort( mDPathEntry )
                  != IDE_SUCCESS );
    }

    IDU_FIT_POINT("2.TASK-7220@smxTrans::abort::writeAbortLogAndUndoTrans");
    /* Transaction Undo시키고 Abort log를 기록. */
    IDE_TEST( writeAbortLogAndUndoTrans( &sAbortEndLSN )
              != IDE_SUCCESS );

    // Tx's PrivatePageList를 정리한다.
    // 반드시 위에서 먼저 abort로그를 write하고 작업한다.
    // BUGBUG : 로그 write할때 flush도 반드시 필요하다.
    IDE_TEST( finAndInitPrivatePageList() != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS의 private page list도 정리한다. */
    IDE_TEST( finAndInitVolPrivatePageList() != IDE_SUCCESS );

    // BUG-14093 Ager Tx가 FreeSlot한 것들을 실제 FreeSlotList에 매단다.
    IDE_TEST( smLayerCallback::processFreeSlotPending(
                                             this,
                                             &mOIDFreeSlotList )
              != IDE_SUCCESS );

    if ( ( mIsUpdate == ID_TRUE ) &&
         ( mIsTransWaitRepl == ID_TRUE ) && 
         ( isReplTrans() == ID_FALSE ) )
    {
        if (smrRecoveryMgr::mIsReplCompleteAfterCommitFunc != NULL )
        {
            smrRecoveryMgr::mIsReplCompleteAfterCommitFunc(
                                            mStatistics,
                                            mTransID,
                                            SM_MAKE_SN(mFstUndoNxtLSN),
                                            SM_MAKE_SN(mLastWritedLSN),
                                            ( mFlag & SMX_REPL_MASK ),
                                            SMI_BEFORE_LOCK_RELEASE);
        }
        else
        {
            /* Nothing to do */
        }

        sNeedWaitTransForRepl = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }
    
    sState = 1;
    /* ovl에 있는 OID 리스트의 lock과 SCN을 갱신하고
     * pol과 ovl을 logical ager에게 넘긴다. */
    if ( ( mTXSegEntry != NULL ) ||
         ( mOIDList->isEmpty() == ID_FALSE ) )
    {
        IDE_TEST( addOIDList2AgingList( SM_OID_ACT_AGING_ROLLBACK,
                                        SMX_TX_ABORT,
                                        &sAbortEndLSN,
                                        &sDummySCN,
                                        ID_FALSE /* aIsLegacyTrans */ )
                  != IDE_SUCCESS );
    }
    else
    {
        mStatus = SMX_TX_ABORT;
    }

    if ( mTXSegEntry != NULL )
    {
        if (smrRecoveryMgr::isRestart() == ID_FALSE )
        {
            /*
             * [BUG-27542] [SD] TSS Page Allocation 관련 함수(smxTrans::allocTXSegEntry,
             *             sdcTSSegment::bindTSS)들의 Exception처리가 잘못되었습니다.
             */
            if ( mTXSegEntry->mTSSlotSID != SD_NULL_SID )
            {
                /* BUG-29918 - tx의 abort시 사용했던 undo extent dir에
                 *             잘못된 SCN을 써서 재사용되지 못할 ext dir을
                 *             재사용하고 있습니다.
                 *
                 * markSCN()에 INITSCN을 넘겨주는 것이 아니라 GSCN을 넘겨주도록
                 * 수정한다. */
                smxTransMgr::mGetSmmViewSCN( &sSysSCN );

                IDE_TEST( sdcTXSegMgr::markSCN(
                                              mStatistics,
                                              mTXSegEntry,
                                              &sSysSCN ) != IDE_SUCCESS );


                /* BUG-31055 Can not reuse undo pages immediately after 
                 * it is used to aborted transaction 
                 * 즉시 재활용 할 수 있도록, ED들을 Shrink한다. */
                IDE_TEST( sdcTXSegMgr::shrinkExts( mStatistics,
                                                   this,
                                                   mTXSegEntry )
                          != IDE_SUCCESS );
            }
        }

        sdcTXSegMgr::freeEntry( mTXSegEntry,
                                ID_TRUE /* aMoveToFirst */ );
        mTXSegEntry = NULL;
    }

    /* ================================================================
     * [3] 디스크 관리자를 위한 pending operation들을 수행
     * ================================================================ */
    IDE_TEST( executePendingList( ID_FALSE ) != IDE_SUCCESS );

    /* PROJ-2694 Fetch Across Rollback */

    rollbackDDLTargetTableInfo();

    if ( aIsLegacyTrans == ID_TRUE )
    {
        /* rollback시에는 OID list를 모두 정리하므로 OID list를 달아줄 필요가 없다. */
        IDE_TEST( smxLegacyTransMgr::addLegacyTrans( this,
                                                     sAbortEndLSN,
                                                     aLegacyTrans,
                                                     ID_FALSE /* aIsCommit*/ )
                  != IDE_SUCCESS );

        mLegacyTransCnt++;
    }
    else
    {
        /* do nothing... */
    }

    sTempStatistics    = mStatistics;
    sTempTransID       = mTransID;
    SM_GET_LSN( sTempLastWritedLSN, mLastWritedLSN );

    IDE_TEST( end() != IDE_SUCCESS );

    /* 
     * BUG-42736 Loose Replication 역시 Remote 서버의 반영을 대기 하여야 합니다.
     */
    if ( sNeedWaitTransForRepl == ID_TRUE )
    {
        if (smrRecoveryMgr::mIsReplCompleteAfterCommitFunc != NULL )
        {
            smrRecoveryMgr::mIsReplCompleteAfterCommitFunc(
                                                sTempStatistics,
                                                sTempTransID,
                                                SM_MAKE_SN( mFstUndoNxtLSN ),
                                                SM_MAKE_SN( sTempLastWritedLSN ),
                                                ( mFlag & SMX_REPL_MASK ),
                                                SMI_AFTER_LOCK_RELEASE);
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if ( sState == 0 )
    {
        mStatus4FT = sPreStatus4FT;
        mStatus    = sPreStatus; 
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::end()
{
    UInt  sState = 0;
    smTID sNxtTransID;
    smTID sOldTransID;

#ifdef DEBUG
    ideLog::log( IDE_SD_19,
                 "\n<smxTrans(%"ID_XINT64_FMT"-%"ID_UINT32_FMT")::end>\n",
                 this, mTransID );
#endif

    IDU_FIT_POINT_RAISE("5.TASK-7220@smxTrans::end", err_internal_server_error);
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    // transaction end시 session id를 null로 설정.
    mSessionID = ID_NULL_SESSION_ID;

    /* PROJ-1381 Fetch Across Commits
     * TX를 종료할 때 모든 table lock을 해제해야 한다.
     * 하지만 Legacy Trans가 남아있다면 IS Lock을 유지해야 한다. */
    if ( mLegacyTransCnt == 0 )
    {
        IDE_TEST( smLayerCallback::freeAllItemLock( mSlotN )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( smLayerCallback::freeAllItemLockExceptIS( mSlotN )
                  != IDE_SUCCESS );
    }

    //Free All Record Lock
    /* always return true */
    lock();
    sState = 1;

    (void)smLayerCallback::freeAllRecordLock( mSlotN );

    // PROJ-1362 lob curor hash, list에서 노드를 삭제한다.
    IDE_TEST( closeAllLobCursors() != IDE_SUCCESS );

    // PROJ-2068 Direct-Path INSERT를 수행한 적이 있으면 관련 객체를 파괴한다.
    if ( mDPathEntry != NULL )
    {
        IDE_TEST( sdcDPathInsertMgr::destDPathEntry( mDPathEntry )
                  != IDE_SUCCESS );
        mDPathEntry = NULL;
    }

    if ( mLegacyTransCnt == 0 )
    {
        //Update Transaction ID & change commit status
        mStatus    = SMX_TX_END;
        mStatus4FT = SMX_TX_END;
    }
    else
    {
        /* nothing to do */
    }
    IDL_MEM_BARRIER;

    sOldTransID = mTransID;
    sNxtTransID = mTransID;

    do
    {
        sNxtTransID = sNxtTransID + smxTransMgr::mTransCnt;

        /*
         * PROJ-2728 Sharding LOB
         *   [ Shard flag | TransactionID ] = TransID
         *      1bit      |  31bit          =  32bit.
         */
        sNxtTransID &= ~(SMI_LOB_LOCATOR_SHARD_MASK);

        /* PROJ-1381 Fetch Across Commits
         * smxTrans에 Legacy Trans가 있으면 smxTrans와 Legacy Trans의 TransID가
         * 같을 수 있으므로, 해당 TransID가 Legacy Trans와 같지 않도록 한다. */
        if ( mLegacyTransCnt != 0 )
        {
            /* TransID 재사용 주기와 TX 내 동시 사용가능한 Statement 개수를
             * 고려하면 같은 TransID를 사용할 리가 없다. */
            if ( sOldTransID == sNxtTransID )
            {
                ideLog::log( IDE_SM_0,
                             "INVALID TID : OLD TID : [%u], "
                             "NEW TID : [%u]\n",
                             sOldTransID, sNxtTransID );
                IDE_ASSERT( 0 );
            }
            else
            {
                /* nothing to do */
            }

            if ( !SM_SCN_IS_MAX( smxLegacyTransMgr::getCommitSCN( sNxtTransID ) ) )
            {
                continue;
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
    } while ( ( sNxtTransID == 0 ) || ( sNxtTransID == SM_NULL_TID ) );

    mTransID = sNxtTransID;

#ifdef PROJ_2181_DBG
    ULong sNext;
    sNext = mTransIDDBG + smxTransMgr::mTransCnt;

    while ( (sNext == 0LL) || (sNext == ID_ULONG_MAX) )
    {
        sNext = sNext+ smxTransMgr::mTransCnt;
    }

    mTransIDDBG = sNext;
#endif

    IDL_MEM_BARRIER;

    IDE_TEST( init() != IDE_SUCCESS ); //checkpoint thread 고려..

    sState = 0;
    /* always return true */
    unlock();

    //Savepoint Resource를 반환한다.
    IDE_TEST( mSvpMgr.destroy( this->mSmiTransPtr ) != IDE_SUCCESS );
    IDE_TEST( removeAllAndReleaseMutex() != IDE_SUCCESS );

    mStatistics = NULL;

    if ( smuProperty::getLogCompResourceReuse() == 1 )
    {
        /* BUG-47365 CompRes를 재사용할수 있도록 한다.
         * 너무 큰 Size를 가지고 있으면 낭비이기 때문에 조절할수 있으면 조절한다. */
        IDE_TEST( mCompResPool.tuneCompRes( mCompRes, smuProperty::getCompResTuneSize() )
                  != IDE_SUCCESS );
    }
    else
    {
        /* CompRes를 재사용 하지 않는다면 반납해야한다. */
        if ( mCompRes != NULL )
        {
           // Log 압축 리소스를 Pool에 반납한다.
            IDE_TEST( mCompResPool.freeCompRes( mCompRes )
                      != IDE_SUCCESS );
            mCompRes = NULL;
        }
    }

    // 트랜잭션이 사용한 Touch Page List를 해제한다.
    IDE_ASSERT( mTouchPageList.reset() == IDE_SUCCESS );

    return IDE_SUCCESS;

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( err_internal_server_error )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL) );
    }
#endif
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        /* always return true */
        unlock();
        IDE_POP();
    }

    return IDE_FAILURE;

}

void smxTrans::setLstUndoNxtLSN( smLSN aLSN )
{
    /* always return true */
    lock();

    if ( mIsUpdate == ID_FALSE )
    {
        /* Tx의 첫 로그를 기록할때만 mIsUpdate == ID_FALSE */
        mIsUpdate = ID_TRUE;

        /* 일반적인 Tx의 경우, mFstUndoNxtLSN이 SM_LSN_MAX 이다. */
        if ( SM_IS_LSN_MAX( mFstUndoNxtLSN ) )
        {
            SM_GET_LSN( mFstUndoNxtLSN, aLSN );
        }
        else
        {
            /* BUG-39404
             * Legacy Tx를 가진 Tx의 경우, mFstUndoNxtLSN을
             * 최초 Legacy Tx의 mFstUndoNxtLSN 을 유지해야 함 */
        }

        mFstUpdateTime = smLayerCallback::smiGetCurrentTime();
    
    }
    else
    {
        /* nothing to do */
    }

    /* mBeginLogLSN은 recovery from replication에서 사용하며 
     * begin에서 초기화 하며, 
     * Active Transaction List 에 등록될때 값을 설정한다.
     * FAC 때문에 mFstUndoNxtLSN <= mBeginLogLSN 임.
     */
    if ( SM_IS_LSN_MAX( mBeginLogLSN ) )
    {
        SM_GET_LSN( mBeginLogLSN, aLSN ); 
    }
    else
    {
        /* nothing to do */
    }

    /* BUG-33895 [sm_recovery] add the estimate function
     * the time of transaction undoing. */
    mTotalLogCount ++;
    mLstUndoNxtLSN = aLSN;
    SM_GET_LSN( mLastWritedLSN, aLSN );
    
    /* always return true */
    unlock();
}

/*******************************************************************
 * BUG-16368을 위해 생성한 함수
 * Description:
 *              smxTrans::addListToAgerAtCommit 또는
 *              smxTrans::addListToAgerAtABort 에서 불리우는 함수.
 * aCommitSCN       [IN]    - 생성되는 OidList의 SCN을 결정
 * aAgingState      [IN]    - SMX_TX_COMMT OR SMX_TX_ABORT
 * aAgerNormalList  [OUT]   - List에 add한 결과 리턴
 *******************************************************************/
IDE_RC smxTrans::addOIDList2LogicalAger( smSCN        * aCommitSCN,
                                         UInt           aAgingState,
                                         void        ** aAgerNormalList )
{
    IDE_DASSERT( SM_SCN_IS_EQ(aCommitSCN,&mCommitSCN) ); 
    IDE_DASSERT( (aAgingState == SM_OID_ACT_AGING_COMMIT)   ||
                 (aAgingState == SM_OID_ACT_AGING_ROLLBACK) );

    // PRJ-1476
    // cached insert oid는 memory ager에게 넘어가지 않도록 조치를 취한다.
    IDE_TEST( mOIDList->cloneInsertCachedOID() != IDE_SUCCESS );

    mOIDList->mOIDNodeListHead.mPrvNode->mNxtNode = NULL;
    mOIDList->mOIDNodeListHead.mNxtNode->mPrvNode = NULL;

    IDE_TEST( smLayerCallback::addList2LogicalAger( mTransID,
                                                    mIsDDL,
                                                    aCommitSCN,
                                                    aAgingState,
                                                    mOIDList,
                                                    aAgerNormalList )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/***********************************************************************
 * Transaction의 Commit로그를 기록한다.
 *
 * aEndLSN    - [OUT] 기록된 Commit Log의 LSN
 * aCommitSCN - [IN] Transaction의 CommitSCN
 **********************************************************************/
IDE_RC smxTrans::writeCommitLog( smLSN* aEndLSN, smSCN aCommitSCN )
{
    /* =======================
     * [1] write precommit log
     * ======================= */
    /* ------------------------------------------------
     * 1) tss와 관련없는 tx라면 commit 로깅을 하고 addListToAger를 처리한다.
     * commit 로깅후에 tx 상태(commit 상태)를 변경한다.
     * -> commit 로깅 -> commit 상태로 변경 -> commit SCN 설정 -> tx end
     *
     * 2) tss와 관련된 tx(disk tx)는 addListToAger에서 commit SCN을 할당한후
     * tss 관련 작업을 한후에 commit 로깅을 한 다음, commit SCN을 설정한다.
     * add TSS commit list -> commit 로깅 -> commit상태로변경->commit SCN 설정
     * tss에 commitSCN설정 -> tx end
     * ----------------------------------------------*/
    if ( mTXSegEntry == NULL  )
    {
        IDE_TEST( writeCommitLog4Memory( aEndLSN, aCommitSCN ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( writeCommitLog4Disk( aEndLSN, aCommitSCN ) != IDE_SUCCESS );

        IDU_FIT_POINT( "1.PROJ-1548@smxTrans::writeCommitLog" );
    }

    /* BUG-19503: DRDB의 smcTableHeader의 mFixed:.Mutex의 Duration Time이 깁니다.
     *
     *            RecordCount 갱신을 위해서 Mutex Lock을 잡은 상태에서 RecordCount
     *            정보를 갱신하고 있습니다. 그 로그가 Commit로그인데 문제는
     *            Transaction의 Durability를 보장한다면 Commit로그가 디스크에 sync
     *            될때까지 기다려야 하고 타 Transaction또한 이를 기다리는 문제가
     *            있습니다. 하여 Commit로그 기록시에 Flush하는 것이 아니라 추후에
     *            Commit로그가 Flush가 필요있을시 Flush하는것으로 수정했습니다.*/

    /* Durability level에 따라 commit 로그는 sync를 보장하기도 한다.
     * 함수안에서 Flush가 필요하는지를 Check합니다. */

    // log가 disk에 기록될때까지 기다릴지에 대한 정보 획득
    if ( ( hasPendingOp() == ID_TRUE ) ||
         ( isCommitWriteWait() == ID_TRUE ) )
    {
        IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_TRX,
                                           aEndLSN )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do ... */
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * 만약 Tranasction이 갱신연산을 수행한 Transaction이라면
 *   1. Abort Prepare Log를 기록한다.
 *   2. Undo를 수행한다.
 *   3. Abort Log를 기록한다.
 *   4. Volatile Table에 대한 갱신 연산이 있었다면 Volatile에
 *      대해서 Undo작업을 수행한다.
 *
 * aEndLSN - [OUT] Abort Log의 End LSN.
 *
 **********************************************************************/
IDE_RC smxTrans::writeAbortLogAndUndoTrans( smLSN* aEndLSN )
{
    // PROJ-1553 Replication self-deadlock
    smrTransPreAbortLog  sTransPreAbortLog;
    smrTransAbortLog     sAbortLog;
    smrTransAbortLog   * sAbortLogHead;
    smLSN                sLSN;
    smSCN                sDummySCN;
    sdrMtx               sMtx;
    smuDynArrayBase    * sDynArrayPtr;
    UInt                 sDskRedoSize;
    smLSN                sEndLSNofAbortLog;
    sdcTSSegment       * sCTSegPtr;
    idBool               sIsDiskTrans = ID_FALSE;

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS에 대한 갱신 연산을 모두 undo한다. */
    if ( svrLogMgr::isOnceUpdated( &mVolatileLogEnv ) == ID_TRUE )
    {
        IDE_TEST( svrRecoveryMgr::undoTrans( &mVolatileLogEnv,
                                             SVR_LSN_BEFORE_FIRST )
                 != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    if ( mIsUpdate == ID_TRUE )
    {
        // PROJ-1553 Replication self-deadlock
        // undo 작업을 수행하기 전에 pre-abort log를 찍는다.
        // 외부에서 직접 undoTrans()를 호출하는 것에 대해서는
        // 신경쓰지 않아도 된다.
        // 왜냐하면 SM 자체적으로 undo를 수행하는 것에 대해서는
        // replication의 receiver쪽에서 self-deadlock의 요인이
        // 안되기 때문이다.
        initPreAbortLog( &sTransPreAbortLog );

        // write pre-abort log
        IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                       this,
                                       (SChar*)&sTransPreAbortLog,
                                       NULL,  // Previous LSN Ptr
                                       NULL,  // Log LSN Ptr
                                       &sEndLSNofAbortLog, // End LSN Ptr
                                       SM_NULL_OID )
                  != IDE_SUCCESS );

        // Hybrid Tx : 자신이 기록한 Log를 Flush
        IDE_TEST( flushLog( &sEndLSNofAbortLog,
                            ID_FALSE /* When Abort */ ) != IDE_SUCCESS );

        // pre-abort log를 찍은 후 undo 작업을 수행한다.
        SM_LSN_MAX( sLSN );
        IDE_TEST( smrRecoveryMgr::undoTrans( mStatistics,
                                             this,
                                             &sLSN) != IDE_SUCCESS );

        /* ------------------------------------------------
         * 1) tss와 관련없는 tx라면 abort 로깅을 하고 addListToAger를 처리한다.
         * abort 로깅후에 tx 상태(in-memory abort 상태)를 변경한다.
         * -> abort 로깅 -> tx in-memory abort 상태로 변경 -> tx end
         *
         * 2) 하지만 tss와 관련된 tx는 addListToAger에서 freeTSS
         * tss 관련 작업을 한후에 abort 로깅을 한다.
         * -> tx in-memory abort 상태로 변경 -> abort 로깅 -> tx end
         * ----------------------------------------------*/
        // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
        // log buffer 초기화
        initLogBuffer();

        // BUG-27542 : (mTXSegEntry != NULL) &&
        // (mTXSegEntry->mTSSlotSID != SD_NULL_SID) 이라야 Disk Tx이다
        if ( mTXSegEntry != NULL )
        {
            if ( mTXSegEntry->mTSSlotSID != SD_NULL_SID )
            {
                sIsDiskTrans = ID_TRUE;
            }
        }

        if ( sIsDiskTrans == ID_TRUE )
        {
            initAbortLog( &sAbortLog, SMR_LT_DSKTRANS_ABORT );

            // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
            // abort 로그 기록
            IDE_TEST( writeLogToBuffer(
                          &sAbortLog,
                          SMR_LOGREC_SIZE(smrTransAbortLog) ) != IDE_SUCCESS );

            // BUG-31504: During the cached row's rollback, it can be read.
            // abort 중인 row는 다른 트랜잭션에 의해 읽혀지면 안된다.
            SM_SET_SCN_INFINITE( &sDummySCN );


            IDE_TEST( sdrMiniTrans::begin( mStatistics,
                                           &sMtx,
                                           this,
                                           SDR_MTX_LOGGING,
                                           ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                           SM_DLOG_ATTR_DEFAULT |
                                           SM_DLOG_ATTR_TRANS_LOGBUFF )
                      != IDE_SUCCESS );

            /*
             * 트랜잭션 Abort시에는 CommitSCN을 시스템으로부터 따올 필요가
             * 없기 때문에 Abort로그로 Unbind TSS를 바로 수행한다.
             */
            sCTSegPtr = sdcTXSegMgr::getTSSegPtr( mTXSegEntry );

            IDE_TEST( sCTSegPtr->unbindTSS4Abort(
                                          mStatistics,
                                          &sMtx,
                                          mTXSegEntry->mTSSlotSID,
                                          &sDummySCN ) != IDE_SUCCESS );

            sDynArrayPtr = &(sMtx.mLogBuffer);
            sDskRedoSize = smuDynArray::getSize( sDynArrayPtr );

            IDE_TEST( writeLogToBufferUsingDynArr(
                                          sDynArrayPtr,
                                          sDskRedoSize ) != IDE_SUCCESS );

            // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
            // abort 로그의 tail 기록
            IDE_TEST( writeLogToBuffer( &(sAbortLog.mHead.mType),
                                        ID_SIZEOF( smrLogType ) )
                      != IDE_SUCCESS );

            // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
            // abort 로그의 logHead에 disk redo log size를 기록
            sAbortLogHead = (smrTransAbortLog*)mLogBuffer;

            smrLogHeadI::setSize( &sAbortLogHead->mHead, mLogOffset );
            sAbortLogHead->mDskRedoSize = sDskRedoSize;

            IDE_TEST( sdrMiniTrans::commit( &sMtx,
                                            SMR_CT_END,
                                            aEndLSN,
                                            SMR_RT_DISKONLY )
                      != IDE_SUCCESS );
        }
        else
        { 
            initAbortLog( &sAbortLog, SMR_LT_MEMTRANS_ABORT );

            // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
            // abort 로그 기록
            IDE_TEST( writeLogToBuffer(
                          &sAbortLog,
                          SMR_LOGREC_SIZE(smrTransAbortLog) ) != IDE_SUCCESS );

            // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
            // abort 로그의 tail 기록
            IDE_TEST( writeLogToBuffer( &(sAbortLog.mHead.mType),
                                        ID_SIZEOF(smrLogType) ) 
                      != IDE_SUCCESS );

            IDE_TEST( smrLogMgr::writeLog( mStatistics, /* idvSQL* */
                                           this,
                                           (SChar*)mLogBuffer,
                                           NULL,   // Previous LSN Ptr
                                           NULL,   // Log LSN Ptr
                                           aEndLSN,// End LSN Ptr
                                           SM_NULL_OID )
                      != IDE_SUCCESS );
        }

        /* LogFile을 sync해야 되는지 여부를 확인 후 필요하다면 sync 한다. */
        if ( hasPendingOp() == ID_TRUE )
        {
            IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_TRX,
                                               aEndLSN )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do ... */
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : MMDB(Main Memory DB)에 대해 갱신을 작업을 수행한 Transaction
 *               의 Commit Log를 기록한다.
 *
 * aEndLSN    - [OUT] Commit Log의 EndLSN
 * aCommitSCN - [IN] Transaction의 CommitSCN
 **********************************************************************/
IDE_RC smxTrans::writeCommitLog4Memory( smLSN * aEndLSN, smSCN aCommitSCN )
{
    smrTransMemCommitLog sCommitLog;

#ifdef DEBUG
    // 넘어와도 무시하면 되니 디버그에서만 ASSERT
    if ( SM_SCN_IS_NOT_INIT(aCommitSCN) )
    {
        IDE_ASSERT( mIsGCTx == ID_TRUE );
    }
#endif

    IDE_TEST( mTableInfoMgr.requestAllEntryForCheckMaxRow()
              != IDE_SUCCESS );

    IDE_TEST( mTableInfoMgr.releaseEntryAndUpdateMemTableInfoForIns()
              != IDE_SUCCESS );

    sCommitLog.mHead.mFlag = mLogTypeFlag;
    sCommitLog.mHead.mSize = SMR_LOGREC_SIZE(smrTransMemCommitLog);
    sCommitLog.mHead.mType = SMR_LT_MEMTRANS_COMMIT;
    sCommitLog.mHead.mTransID = mTransID;

    sCommitLog.mDskRedoSize=0;

    sCommitLog.mGlobalTxId = mGlobalTxId;
    sCommitLog.mCommitSCN  = aCommitSCN;
    sCommitLog.mTail       = SMR_LT_MEMTRANS_COMMIT;    

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   this,
                                   (SChar*)&sCommitLog,
                                   NULL,    // Previous LSN Ptr
                                   NULL,    // Log Begin LSN
                                   aEndLSN, // End LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "1.PROJ-1548@smxTrans::writeCommitLog4Memory" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : DRDB(Disk Resident DB)에 대해 갱신을 작업을 수행한 Transaction
 *               의 Commit Log를 기록한다.
 *
 * aEndLSN    - [OUT] Commit Log의 EndLSN
 * aCommitSCN - [IN] Transaction의 CommitSCN
 **********************************************************************/
IDE_RC smxTrans::writeCommitLog4Disk( smLSN * aEndLSN, smSCN aCommitSCN )
{
    IDE_TEST( mTableInfoMgr.requestAllEntryForCheckMaxRow()
              != IDE_SUCCESS );

    IDE_TEST( mTableInfoMgr.releaseEntryAndUpdateMemTableInfoForIns()
              != IDE_SUCCESS );

    /* Table Header의 Record Count를 갱신한후에 Commit로그를 기록한다.*/
    IDE_TEST( mTableInfoMgr.releaseEntryAndUpdateDiskTableInfoWithCommit(
                                                      mStatistics,
                                                      this,     /* aTransPtr */
                                                      aCommitSCN,
                                                      aEndLSN ) /* aEndLSN   */
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * BUG-47525 Group Commit
 * 자신의 TID를 Group Commit에 등록한후
 * 어느 List에 등록되었는지를 return 받는다.
 * Group 최대치에 도달하였다면 writeLog를 수행 한다.
 **********************************************************/
IDE_RC smxTrans::addTID4GroupCommit( UInt    * aGCList, 
                                     idBool  * aWriteCommitLog, 
                                     smSCN     aCommitSCN )
{
    UInt   sListNumber;
    UInt   sGCCnt;
    UInt   sGCList;
    idBool sWriteCommitLog = ID_FALSE;

    IDE_TEST( mTableInfoMgr.requestAllEntryForCheckMaxRow()
              != IDE_SUCCESS );

    IDE_TEST( mTableInfoMgr.releaseEntryAndUpdateMemTableInfoForIns()
              != IDE_SUCCESS );

    /* GCList는 DirtyRead를 하여도 크게 상관없기에
     * AtomicGet을 사용하지 않도록 하였다. */
    sGCList = mGCList;
    sListNumber = sGCList % mGCListCnt;

    mGCMutex[sListNumber].lock(NULL);

    /* lock을 획득하는 동안 해당 List의 ID가 변경되었을수 있기 때문에
     * lock을 잡은후에 정확한 ID를 가져가야한다. */
    sGCList = mGCListID[sListNumber];
    sGCCnt  = mGCCnt[sListNumber]; 

    mGCTIDArray[sListNumber][sGCCnt] = mTransID;
    mGCCnt[sListNumber]++;

    if ( ( mLogTypeFlag & SMR_LOG_TYPE_MASK ) != SMR_LOG_TYPE_REPLICATED )
    {
        mGCFlag[sListNumber] &= ~(SMR_LOG_TYPE_MASK);
    }

    if ( SM_SCN_IS_NOT_INIT(aCommitSCN) ) 
    {
        IDE_DASSERT( mIsGCTx == ID_TRUE );

        mGCFlag[sListNumber] |= SMR_LOG_COMMITSCN_OK; 
        mGCCommitSCNArray[sListNumber][sGCCnt] = aCommitSCN;     
    }

    if ( mGCCnt[sListNumber] == mGroupCnt )
    {
        mGCList++;
        writeGroupCommitLog( sListNumber );
        /* Write가 끝났으면 GCListNumber를 높여서 wait 중인 Tx가 알게 한다. */
        mGCListID[sListNumber] += mGCListCnt;
        sWriteCommitLog = ID_TRUE;
    }
    mGCMutex[sListNumber].unlock();

    *aWriteCommitLog = sWriteCommitLog;
    *aGCList = sGCList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * BUG-47525 Group Commit
 * Group Commit Log를 기록하는 함수 
 * 함수를 호출하기 전에 항상 aListNumber에 해당하는 List의 lock을 획득하여야 한다.
 **********************************************************/
IDE_RC smxTrans::writeGroupCommitLog( UInt aListNumber )
{
    UInt       sLogSize    = 0;
    UInt       sOffset     = 0;
    UInt       i           = 0;
    smTID      sLocalTxID  = SM_NULL_TID;
    smTID      sGlobalTxID = SM_NULL_TID;
    smrLogType sLogType;
    smSCN      sCommitSCN;

    smrTransGroupCommitLog sCommitLog;

    /* Size 계산 LogHead + LogBody( TID Array ) + LogTail
     * TID Array는 LocalTID와 GlobalTID 가 저장되므로 Size 계산에 2배 적용 */
    sLogSize = SMR_LOGREC_SIZE(smrTransGroupCommitLog) +
               ( mGCCnt[aListNumber] * ID_SIZEOF( smTID ) * 2 ) +
               ID_SIZEOF(smrLogTail); 

    if ( ( mGCFlag[aListNumber] & SMR_LOG_COMMITSCN_MASK ) == SMR_LOG_COMMITSCN_OK )
    {
        /* 다시 Size 계산 LogHead + LogBody( TID Array + CommitSCN ) + LogTail 
         * commitSCN 기록이 필요하면 Size 계산에 CommitSCN 을 기록할 만큼의 크기를 더함 */ 
        sLogSize += mGCCnt[aListNumber] * ID_SIZEOF(smSCN) ;
    }

    /* GroupCommit Log Header를 초기화 */
    smrLogHeadI::setType( &sCommitLog.mHead, SMR_LT_MEMTRANS_GROUPCOMMIT );
    smrLogHeadI::setSize( &sCommitLog.mHead, sLogSize );
    smrLogHeadI::setTransID( &sCommitLog.mHead, mTransID );

    if ( ( mGCFlag[aListNumber] & SMR_LOG_TYPE_MASK ) == SMR_LOG_TYPE_NORMAL )
    {
        /* BUG-47525 Group화된 Log중 1개라도 RP대상이 있다면 FLAG를 Normal로 변경.
         * SMR_LOG_TYPE_NORMAL = 0x00000000 */
        mLogTypeFlag &= ~(SMR_LOG_TYPE_MASK); 
    }

    smrLogHeadI::setFlag( &sCommitLog.mHead, mLogTypeFlag );

    sCommitLog.mGroupCnt = mGCCnt[aListNumber];

    sOffset = 0;
    /* Log Header를 Transaction Log Buffer에 기록 */
    IDE_TEST( writeLogToBuffer( &sCommitLog, /* Group Commit Log Header */
                                sOffset,
                                SMR_LOGREC_SIZE(smrTransGroupCommitLog) )
              != IDE_SUCCESS );
    sOffset += SMR_LOGREC_SIZE(smrTransGroupCommitLog);

    /* Group화된 TID를 모두 모아 저장. */
    for ( i = 0 ; i < mGCCnt[aListNumber] ; i++ )
    {
        sLocalTxID  = mGCTIDArray[aListNumber][i];
        sGlobalTxID = (smTID)(smxTransMgr::getTransByTID( sLocalTxID )->mGlobalTxId);
       
        IDE_TEST( writeLogToBuffer( &sLocalTxID, 
                                    sOffset,
                                    ID_SIZEOF(smTID) )
                  != IDE_SUCCESS );
        sOffset += ID_SIZEOF(smTID);

        IDE_TEST( writeLogToBuffer( &sGlobalTxID, 
                                    sOffset,
                                    ID_SIZEOF(smTID) )
                  != IDE_SUCCESS );
        sOffset += ID_SIZEOF(smTID);
    }

    /* Group화된 Log중 1개라도 CommitSCN이 기록되어있다면 SCN 을 기록해야 한다. */
    if ( ( mGCFlag[aListNumber] & SMR_LOG_COMMITSCN_MASK ) == SMR_LOG_COMMITSCN_OK )
    {
        for ( i = 0 ; i < mGCCnt[aListNumber] ; i++ )
        {
            sCommitSCN = mGCCommitSCNArray[aListNumber][i];

            IDE_TEST( writeLogToBuffer( &sCommitSCN, 
                                        sOffset,
                                        ID_SIZEOF(smSCN) )
                      != IDE_SUCCESS );
            sOffset += ID_SIZEOF(smSCN);
        }
    }
   
    /* Log Tail을 Transaciton Log Buffer에 기록 */
    sLogType = smrLogHeadI::getType(&sCommitLog.mHead);
    IDE_TEST( writeLogToBuffer( &sLogType,
                                sOffset,
                                ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );

    // 트랜잭션 로그 버퍼의 로그를 파일로 기록한다.
    IDE_TEST( smrLogMgr::writeLog( NULL,
                                   this,
                                   mLogBuffer,
                                   NULL,  // Previous LSN Ptr
                                   NULL,  // Log LSN Ptr
                                   NULL,  // End LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );

    IDE_DASSERT( SM_IS_LSN_MAX( mCommitLogLSN ) );

    /* BUG-47865 mCommitLogLSN을 정확한 값으로 갱신 하여야 하다. */
    for ( i = 0 ; i < mGCCnt[aListNumber] ; i++ )
    {
        smxTransMgr::setCommitLogLSN( mGCTIDArray[aListNumber][i],
                                      mLastWritedLSN );

        mGCTIDArray[aListNumber][i] = SM_NULL_TID;
        mGCCommitSCNArray[aListNumber][i] = SM_SCN_INIT;
    }

    /* BUG-47865 나의 mCommitLogLSN 도 갱신 되었을 것이다. */
    IDE_ASSERT( smrCompareLSN::isEQ( &mCommitLogLSN,
                                     &mLastWritedLSN ) == ID_TRUE );

    mGCCnt[aListNumber]  = 0;
                          /* 0x00000001            | 0x00000000             */
    mGCFlag[aListNumber] = SMR_LOG_TYPE_REPLICATED | SMR_LOG_COMMITSCN_NO ;
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * BUG-47525 Group Commit
 * 내가 add한 Group Commit List가 write되었는지 확인 및 대기하는 함수.
 * write된 걸 보장하여야 한다.
 * 아무도 Write 하지 않았다면 직접 Write 한다.
 *
 * aGCList    [IN] add 시에 받았던 내 ListID 
 **********************************************************/
IDE_RC smxTrans::waitGroupCommit( UInt aGCList )
{
    UInt sListNumber;

    sListNumber = aGCList % mGCListCnt;

    /* ListID가 내 ID와 같다면 아직 write가 되지 않았다. */
    if ( aGCList == mGCListID[sListNumber] )
    {
        mGCMutex[sListNumber].lock(NULL);

        /* lock을 획득하는 동안 write가 일어났을수 있다.
         * ListID가 내 ID와 같은지 확인한다. */
        if ( aGCList == mGCListID[sListNumber] )
        {
            mGCList++;
            writeGroupCommitLog( sListNumber );
            /* Write가 끝났으면 GCListID를 높여서 wait 중인 Tx가 알게 한다. */
            mGCListID[sListNumber] += mGCListCnt;
        }

        mGCMutex[sListNumber].unlock();
    }

    return IDE_SUCCESS;
}

/*********************************************************
  function description: Aging List에 Transaction의 OID List
  를 추가한다.
   -  다음과 같은 조건을 만족하는 경우 Commit SCN을 할당한다.
     1. OID List or Delete OID List가 비어있지 않은 경우
     2. Disk관련 DML을 수행한 경우

   - For MMDB
     1. If commit, 할당된 Commit SCN을 Row Header, Table Header
        에 Setting한다.

   - For DRDB
     1. Transaction이 디스크 테이블에 대하여 DML을 한경우에는
        TSS slot에 commitSCN을 setting하고 tss slot list의 head
        에 단다.

        * 위의 두가지 Action을 하나의 Mini Transaction으로 처리하고
          Logging을 하지 않는다.

   - For Exception
     1. 만약 위에 mini transaction 수행중에 crash되면,
        restart recovery과정에서,트랜잭션이  tss slot을
        할당받았고, commit log를 찍었는데,
        TSS slot의 commitSCN이 infinite이면 다음과 같이
        redo를 한다.
        -  TSS slot에 commitSCN을  0으로 setting하고, 해당
           TSS slot을 tss list에 매달아서 GC가 이루어지도록
           한다.
***********************************************************/
IDE_RC smxTrans::addOIDList2AgingList( SInt       aAgingState,
                                       smxStatus  aStatus,
                                       smLSN    * aEndLSN,
                                       smSCN    * aCommitSCN,
                                       idBool     aIsLegacyTrans )
{
    // oid list가 비어 있는가?
    smxProcessOIDListOpt sProcessOIDOpt;
    void               * sAgerNormalList = NULL;
    ULong                sAgingOIDCnt;

    IDE_DASSERT( aCommitSCN != NULL );

    /* PROJ-1381 */
    IDE_DASSERT( ( aIsLegacyTrans == ID_TRUE  ) ||
                 ( aIsLegacyTrans == ID_FALSE ) );

    // tss commit list가 commitSCN순으로 정렬되어 insert된다.
    // -> smxTransMgr::mMutex가 잡힌 상태이기때문에.
    /* BUG-47367 더이상 TransMgr의 lock을 획득하지 않기 때문에
     * commitSCN 순서대로 List에 등록되지는 않는다.
     */

    if ( aStatus == SMX_TX_COMMIT )
    {
        /*  트랜잭션의 oid list또는  deleted oid list가 비어 있지 않으면,
            commitSCN을 system으로 부터 할당받는다.
            트랜잭션의 statement가 모두 disk 관련 DML이었으면
            oid list가 비어있을 수 있다.
            --> 이때도 commitSCN을 따야 한다. */

        if ( SM_SCN_IS_INIT(*aCommitSCN) )
        { 
            /* Global Consistent Transaction도 aCommitSCN 을 보내지 않을수 있다. 
             * e.g. 내부에서 사용하는거라던가, prepare라던가.. */

            //commit SCN을 할당받는다. 안에서 transaction 상태 변경
            //tx의 in-memory COMMIT or ABORT 상태
            /* smmDatabase::getCommitSCN 이 호출된다. */
            IDE_TEST( smxTransMgr::mGetSmmCommitSCN( this,
                                                     aIsLegacyTrans,
                                                     (void *)&aStatus )
                     != IDE_SUCCESS );

            SM_SET_SCN( aCommitSCN, &mCommitSCN );  
        }
        else
        {
            /* 공유된 CommitSCN으로 커밋을 시도 한다.
             * Global Consistent Transaction이 aCommitSCN 을 보내지 않는 경우는 없다.
             * 분산레벨과 상관없다. */
            IDE_DASSERT( mIsGCTx == ID_TRUE );

            // transaction 상태 변경
            setTransSCNnStatus( this, aIsLegacyTrans, aCommitSCN, (void *)&aStatus );
        }

        /* BUG-41814
         * Fetch Cursor를 연 메모리 Stmt를 가지지 않은 트랜잭션은 commit 과정에서
         * 열려있는 메모리 Stmt가 0개 이며, 따라서 mMinMemViewSCN 이 무한대 이다.
         * mMinMemViewSCN이 무한대인 트랜잭션은 Ager가 전체 트랜잭션의 minMemViewSCN을
         * 계산할때 무시하게 되는데, 무시 하지 않도록 commitSCN을 임시로 박아둔다. */
        if ( SM_SCN_IS_INFINITE( ((smxTrans *)this)->mMinMemViewSCN ) )
        {
            SM_SET_SCN( &(((smxTrans *)this)->mMinMemViewSCN), aCommitSCN );
        }
        else
        {
            /* Nothing to do */
        }

        IDU_FIT_POINT( "1.BUG-21950@smxTrans::addOIDList2AgingList" );
    }
    else
    {
        /* fix BUG-9734, rollback시에 system scn을 증가시키는것을
         * 막기 위하여 그냥 0을 setting한다. */
        /* BUG-47367 Ager에서 OIDHead의 SCN으로 drop 여부를 확인하고 있다.
         * rollback에서 0으로 넘기게 되면 항상 drop으로 취급되게 된다. 
         * system scn을 증가시킬 필요는 없고 현재 시점의 scn 정보를 넘겨주면 된다.
         * getLstSystemSCN은 lock을 잡지 않고 값만 가져오는 함수이다. */
        IDE_DASSERT( aStatus == SMX_TX_ABORT);

        *aCommitSCN = smmDatabase::getLstSystemSCN();
        setTransCommitSCN( this, aCommitSCN, &aStatus );
    }

    /* PROJ-2462 ResultCache */
    mTableInfoMgr.addTablesModifyCount();

    // To fix BUG-14126
    if ( mOIDList->isEmpty() == ID_FALSE )
    {
        if ( ( mOIDList->needAging() == ID_FALSE ) &&
             ( aStatus == SMX_TX_COMMIT ) )
        {
            /* PROJ-1381 Fetch Across Commits
             * Insert만 수행하면 addOIDList2LogicalAger를 호출하지 않아서
             * OIDList내의 mCacheOIDNode4Insert를 복사하지 않는다.
             * FAC는 Commit 이후에도 OIDList를 사용하므로 복사하도록 한다. */
            if ( aIsLegacyTrans == ID_TRUE )
            {
                IDE_TEST( mOIDList->cloneInsertCachedOID()
                          != IDE_SUCCESS );
            }

            /* Aging List에 OID List를 추가하지 않는다면 Transaction이
               직접 OID List를 Free시킨다. */
            sProcessOIDOpt = SMX_OIDLIST_DEST;
        }
        else
        {
            IDE_TEST( addOIDList2LogicalAger( aCommitSCN,
                                              aAgingState,
                                              &sAgerNormalList )
                      != IDE_SUCCESS );

            /* Aging List에 OID List를 추가한다면 Ager Thread가
               OID List를 Free시킨다. */
            sProcessOIDOpt = SMX_OIDLIST_INIT;
        }

        IDU_FIT_POINT( "BUG-45654@smxTrans::addOIDList2AgingList::beforeProcessOIDList" );

        IDE_TEST( mOIDList->processOIDList( aAgingState,
                                            aEndLSN,
                                            *aCommitSCN,
                                            sProcessOIDOpt,
                                            &sAgingOIDCnt,
                                            aIsLegacyTrans )
                  != IDE_SUCCESS );


        /* BUG-17417 V$Ager정보의 Add OID갯수는 실제 Ager가
         *                     해야할 작업의 갯수가 아니다.
         *
         * Aging OID갯수를 더해준다. */
        if ( sAgingOIDCnt != 0 )
        {
            smLayerCallback::addAgingRequestCnt( sAgingOIDCnt );
        }
        
        if ( sAgerNormalList != NULL )
        {
            smLayerCallback::setOIDListFinished( sAgerNormalList,
                                                 ID_TRUE );
        }
    }

    // TSS에서 commitSCN으로 setting한다.
    IDU_FIT_POINT( "8.PROJ-1552@smxTrans::addOIDList2AgingList" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxTrans::removeAllAndReleaseMutex()
{
    iduMutex * sMutex;
    smuList  * sIterator;
    smuList  * sNodeNext;

    for (sIterator = SMU_LIST_GET_FIRST(&mMutexList),
         sNodeNext = SMU_LIST_GET_NEXT(sIterator)
         ;
         sIterator != &mMutexList
         ;
         sIterator = sNodeNext,
         sNodeNext = SMU_LIST_GET_NEXT(sIterator))
    {
        sMutex = (iduMutex*)sIterator->mData;
        IDE_TEST_RAISE( sMutex->unlock() != IDE_SUCCESS,
                        mutex_unlock_error);
        SMU_LIST_DELETE(sIterator);
        IDE_TEST( mMutexListNodePool.memfree((void *)sIterator) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/* ------------------------------------------------
             For Global Transaction
    xa_commit, xa_rollback, xa_start는
    local transaction의 인터페이스를 그대로 사용함.
   ------------------------------------------------ */
/* BUG-18981 */
IDE_RC smxTrans::prepare( ID_XID *aXID, smSCN * aPrepareSCN, idBool aLogging )
{

    PDL_Time_Value    sTmVal;
    smxStatus         sStatus = SMX_TX_PREPARED;

    IDU_FIT_POINT_RAISE("9.TASK-7220@smxTrans::prepare",err_internal_server_error);
    IDU_FIT_POINT_RAISE("5.TASK-7220@smxTrans::prepare",err_internal_server_error);

    if ( ( mIsUpdate == ID_TRUE ) && ( aLogging == ID_TRUE ) )
    {
        if ( mTXSegEntry != NULL )
        {
            IDE_TEST( smrUpdate::writeXaSegsLog(
                         NULL, /* idvSQL * */
                         (void*)this,
                         aXID,
                         mLogBuffer,
                         mTXSegEntryIdx,
                         mTXSegEntry->mExtRID4TSS,
                         mTXSegEntry->mTSSegmt.getFstPIDOfCurAllocExt(),
                         mTXSegEntry->mFstExtRID4UDS,
                         mTXSegEntry->mLstExtRID4UDS,
                         mTXSegEntry->mUDSegmt.getFstPIDOfCurAllocExt(),
                         mTXSegEntry->mFstUndoPID,
                         mTXSegEntry->mLstUndoPID ) != IDE_SUCCESS );
        }

        /* ---------------------------------------------------------
           table lock을 prepare log의 데이타로 로깅
           record lock과 OID 정보는 재시작 회복의 재수행 단계에서 수집해야 함
           ---------------------------------------------------------*/
        IDE_TEST( smlLockMgr::logLocks( this, aXID )
                  != IDE_SUCCESS );

    }

    /* ----------------------------------------------------------
       트랜잭션 commit 상태 변경 및 Gloabl Transaction ID setting
       ---------------------------------------------------------- */
    sTmVal = idlOS::gettimeofday();

    /* always return true */
    lock();

    IDE_DASSERT( mStatus == SMX_TX_BEGIN );
    IDE_DASSERT( mCommitState == SMX_XA_START );

    mCommitState  = SMX_XA_PREPARED;
    mPreparedTime = (timeval)sTmVal;
    mXaTransID    = *aXID;

    /* always return true */
    unlock();

    if( mIsGCTx == ID_TRUE )
    {
        if ( aPrepareSCN != NULL )
        {
            /* smmDatabase::getCommitSCN 을 호출한다 */
            smxTransMgr::mGetSmmCommitSCN( this,
                                           ID_FALSE,           /* aIsLegacyTrans */
                                           (void *)&sStatus ); /* aStatus */

            SM_GET_SCN( aPrepareSCN, &mPrepareSCN );
        }
    }
    else
    {
        if ( aPrepareSCN != NULL )
        {
            SM_INIT_SCN( aPrepareSCN );
        }
    }

    IDU_FIT_POINT_RAISE("2.TASK-7220@smxTrans::prepare",err_internal_server_error);

    return IDE_SUCCESS;
    
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( err_internal_server_error )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL) );
    }
#endif
    IDE_EXCEPTION_END;

#ifdef DEBUG
    extern UInt ideNoErrorYet();
    
    if( ideNoErrorYet() == ID_TRUE )
    {
        /* 에러메시지를 설정하지 않으면 에러를 무시할 수 있습니다. 
         * 어디서 설정하지 않는지 확인이 필요합니다. */
        IDE_DASSERT(0);
    }
#endif

    return IDE_FAILURE;
}

/*
IDE_RC smxTrans::forget(XID *aXID, idBool a_isRecovery)
{

    smrXaForgetLog s_forgetLog;

    if ( (mIsUpdate == ID_TRUE) && (a_isRecovery != ID_TRUE ) )
    {
        s_forgetLog.mHead.mTransID  = mTransID;
        s_forgetLog.mHead.mType     = SMR_LT_XA_FORGET;
        s_forgetLog.mHead.mSize     = SMR_LOGREC_SIZE(smrXaForgetLog);
        s_forgetLog.mHead.mFlag     = mLogTypeFlag;
        s_forgetLog.mXaTransID      = mXaTransID;
        s_forgetLog.mTail           = SMR_LT_XA_FORGET;

        IDE_TEST( smrLogMgr::writeLog(this, (SChar*)&s_forgetLog) != IDE_SUCCESS );
    }

    mTransID   = mTransID + smxTransMgr::mTransCnt;

    initXID();

    IDE_TEST( init() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
*/

IDE_RC smxTrans::freeOIDList()
{
    if ( mOIDToVerify != NULL )
    {
        IDE_TEST( mOIDToVerify->freeOIDList() != IDE_SUCCESS ); 
    }

    IDE_TEST( mOIDList->freeOIDList() != IDE_SUCCESS );
    IDE_TEST( mOIDFreeSlotList.freeOIDList() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smxTrans::showOIDList()
{

    mOIDList->dump();
    mOIDFreeSlotList.dump();

}

/* BUG-18981 */
IDE_RC smxTrans::getXID( ID_XID *aXID )
{
    idBool sIsValidXID;

    sIsValidXID = isValidXID();
    IDE_TEST( sIsValidXID != ID_TRUE );
    *aXID = mXaTransID;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void smxTrans::initLogBuffer()
{
    mLogOffset = 0;
}

IDE_RC smxTrans::writeLogToBufferUsingDynArr(smuDynArrayBase* aLogBuffer,
                                             UInt             aLogSize)
{
    IDE_TEST( writeLogToBufferUsingDynArr( aLogBuffer,
                                           mLogOffset,
                                           aLogSize ) != IDE_SUCCESS );
    mLogOffset += aLogSize;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smxTrans::writeLogToBufferUsingDynArr(smuDynArrayBase* aLogBuffer,
                                             UInt             aOffset,
                                             UInt             aLogSize)
{
    SChar * sLogBuffer;
    UInt    sState  = 0;

    if ( (aOffset + aLogSize) >= mLogBufferSize )
    {
        mLogBufferSize = idlOS::align( mLogOffset+aLogSize,
                                       SMX_TRANSACTION_LOG_BUFFER_ALIGN_SIZE );

        sLogBuffer = NULL;

        /* smxTrans_writeLogToBufferUsingDynArr_malloc_LogBuffer.tc */
        IDU_FIT_POINT("smxTrans::writeLogToBufferUsingDynArr::malloc::LogBuffer");
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TRANSACTION_TABLE,
                                     mLogBufferSize,
                                     (void**)&sLogBuffer,
                                     IDU_MEM_FORCE )
                 != IDE_SUCCESS );
        sState = 1;

        if ( aOffset != 0 )
        {
            idlOS::memcpy(sLogBuffer, mLogBuffer, aOffset);
        }

        IDE_TEST( iduMemMgr::free(mLogBuffer) != IDE_SUCCESS );

        mLogBuffer = sLogBuffer;

        // 압축 로그버퍼를 이용하여  압축되는 로그의 크기의 범위는
        // 이미 정해져있기 때문에 압축로그 버퍼의 크기는 변경할 필요가 없다.
    }

    smuDynArray::load( aLogBuffer, 
                       (mLogBuffer + aOffset),
                       mLogBufferSize - aOffset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLogBuffer ) == IDE_SUCCESS );
            sLogBuffer = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::writeLogToBuffer(const void *aLog, UInt aLogSize)
{
    IDE_TEST( writeLogToBuffer( aLog, mLogOffset, aLogSize ) != IDE_SUCCESS );
    mLogOffset += aLogSize;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smxTrans::writeLogToBuffer(const void *aLog,
                                  UInt        aOffset,
                                  UInt        aLogSize)
{
    SChar * sLogBuffer;
    UInt    sBfrBuffSize;
    UInt    sState  = 0;

    if ( (aOffset + aLogSize) >= mLogBufferSize)
    {
        sBfrBuffSize   = mLogBufferSize;
        mLogBufferSize = idlOS::align( aOffset + aLogSize,
                                       SMX_TRANSACTION_LOG_BUFFER_ALIGN_SIZE);

        sLogBuffer = NULL;

        /* smxTrans_writeLogToBuffer_malloc_LogBuffer.tc */
        IDU_FIT_POINT("smxTrans::writeLogToBuffer::malloc::LogBuffer");
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TRANSACTION_TABLE,
                                     mLogBufferSize,
                                     (void**)&sLogBuffer,
                                     IDU_MEM_FORCE )
                 != IDE_SUCCESS );
        sState = 1;

        if ( sBfrBuffSize != 0 )
        {
            idlOS::memcpy( sLogBuffer, mLogBuffer, sBfrBuffSize );
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( iduMemMgr::free(mLogBuffer) != IDE_SUCCESS );

        mLogBuffer = sLogBuffer;
    }
    else
    {
        /* nothing to do */
    }

    idlOS::memcpy(mLogBuffer + aOffset, aLog, aLogSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLogBuffer ) == IDE_SUCCESS );
            sLogBuffer = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

void smxTrans::getTransInfo(void*   aTrans,
                            SChar** aTransLogBuffer,
                            smTID*  aTID,
                            UInt*   aTransLogType)
{
    smxTrans* sTrans = (smxTrans*) aTrans;

    *aTransLogBuffer = sTrans->getLogBuffer();
    *aTID =  sTrans->mTransID;
    *aTransLogType = sTrans->mLogTypeFlag;
}

smLSN smxTrans::getTransLstUndoNxtLSN(void* aTrans)
{
    return ((smxTrans*)aTrans)->mLstUndoNxtLSN;
}

/* Transaction의 현재 UndoNxtLSN을 return */
smLSN smxTrans::getTransCurUndoNxtLSN(void* aTrans)
{
    return ((smxTrans*)aTrans)->mCurUndoNxtLSN;
}

/* Transaction의 현재 UndoNxtLSN을 Set */
void smxTrans::setTransCurUndoNxtLSN(void* aTrans, smLSN *aLSN)
{
    /* BUG-33895 [sm_recovery] add the estimate function 
     * the time of transaction undoing. */
    if ( ((smxTrans*)aTrans)->mProcessedUndoLogCount == 0 )
    {
        /* 최초 갱신 */
        ((smxTrans*)aTrans)->mUndoBeginTime = 
            smLayerCallback::smiGetCurrentTime();
    }
    ((smxTrans*)aTrans)->mProcessedUndoLogCount ++;
    ((smxTrans*)aTrans)->mCurUndoNxtLSN = *aLSN;
}

smLSN* smxTrans::getTransLstUndoNxtLSNPtr(void* aTrans)
{
    return &( ((smxTrans*)aTrans)->mLstUndoNxtLSN );
}

void smxTrans::setTransLstUndoNxtLSN( void   * aTrans, 
                                      smLSN    aLSN )
{
    ((smxTrans*)aTrans)->setLstUndoNxtLSN( aLSN );
}

void smxTrans::getTxIDAnLogType( void    * aTrans, 
                                 smTID   * aTID, 
                                 UInt    * aLogType )
{
    smxTrans* sTrans = (smxTrans*) aTrans;

    *aTID =  sTrans->mTransID;
    *aLogType = sTrans->mLogTypeFlag;
}

idBool smxTrans::isTxBeginStatus( void  * aTrans )
{
    if ( ((smxTrans*)aTrans)->mStatus == SMX_TX_BEGIN )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/***************************************************************************
 *
 * Description: Startup 과정에서 Verify할 IndexOID를 추가한다.
 *
 * aTrans    [IN] - 트랜잭션
 * aTableOID [IN] - aTableOID
 * aIndexOID [IN] - Verify할 aIndexOID
 * aSpaceID  [IN] - 해당 Tablespace ID
 *
 ***************************************************************************/
IDE_RC smxTrans::addOIDToVerify( void *    aTrans,
                                 smOID     aTableOID,
                                 smOID     aIndexOID,
                                 scSpaceID aSpaceID )
{
    smxTrans  * sTrans;

    IDE_ASSERT( aTrans    != NULL );
    IDE_ASSERT( aTableOID != SM_NULL_OID );
    IDE_ASSERT( aIndexOID != SM_NULL_OID );

    sTrans = (smxTrans*) aTrans;
    IDE_ASSERT( sTrans->mStatus == SMX_TX_BEGIN );

    return sTrans->mOIDToVerify->add( aTableOID,
                                      aIndexOID,
                                      aSpaceID,
                                      SM_OID_NULL /* unuseless */);
}

IDE_RC  smxTrans::addOIDByTID( smTID     aTID,
                               smOID     aTblOID,
                               smOID     aRecordOID,
                               scSpaceID aSpaceID,
                               UInt      aFlag )
{
    smxTrans* sCurTrans;

    sCurTrans = smxTransMgr::getTransByTID(aTID);

    /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
       수행되어야 한다.*/
    if (sCurTrans->mStatus == SMX_TX_BEGIN )
    {
        return sCurTrans->addOID(aTblOID,aRecordOID,aSpaceID,aFlag);
    }

    return IDE_SUCCESS;
}

idBool  smxTrans::isXAPreparedCommitState( void  * aTrans )
{
    return ( ((smxTrans*)aTrans)->mCommitState == SMX_XA_PREPARED ? ID_TRUE : ID_FALSE);
}

void smxTrans::addMutexToTrans (void *aTrans, void* aMutex)
{
    /* BUG-17569 SM 뮤텍스 개수 제한으로 인한 서버 사망:
       기존에는 SMX_MUTEX_COUNT(=10) 개의 엔트리를 갖는 배열을 사용하여,
       뮤텍스 개수가 SMX_MUTEX_COUNT를 넘어서면 서버가 사망했다.
       이 제한을 없애고자 배열을 링크드리스트로 변경하였다. */
    smxTrans *sTrans = (smxTrans*)aTrans;
    smuList  *sNode  = NULL;

    IDE_ASSERT( mMutexListNodePool.alloc((void **)&sNode) == IDE_SUCCESS );
    sNode->mData = (void *)aMutex;
    SMU_LIST_ADD_LAST(&sTrans->mMutexList, sNode);
}

IDE_RC smxTrans::writeLogBufferWithOutOffset( void       * aTrans, 
                                              const void * aLog,
                                              UInt         aLogSize )
{
    return ((smxTrans*)aTrans)->writeLogToBuffer(aLog,aLogSize);
}


SChar * smxTrans::getTransLogBuffer (void *aTrans)
{
    return ((smxTrans*)aTrans)->getLogBuffer();
}

UInt   smxTrans::getLogTypeFlagOfTrans(void * aTrans)
{
    return ((smxTrans*) aTrans)->mLogTypeFlag;
}

void   smxTrans::addToUpdateSizeOfTrans (void *aTrans, UInt aSize)
{
    ((smxTrans*) aTrans)->mUpdateSize += aSize;
}


idvSQL * smxTrans::getStatistics( void* aTrans )
{
    return ((smxTrans*)aTrans)->mStatistics;
}

IDE_RC smxTrans::resumeTrans(void * aTrans)
{
    return ((smxTrans*)aTrans)->resume();
}

/*******************************************************************************
 * Description: 이 함수는 어떤 트랜잭션이 Tuple의 SCN을 읽었을 때,
 *      그 값이 무한대일 경우 이 Tuple에 대해 연산을 수행한 트랜잭션이
 *      commit 상태인지 판단하는 함수이다.
 *      즉, Tuple의 SCN이 무한대이더라도 읽어야만 하는 경우를 검사하기 위함이다.
 *
 * MMDB:
 *  현재 slotHeader의 mSCN값이 정확한 값인지 알기 위해 이함수를 호출한다.
 * slotHeader에대해 트랜잭션이 연산을 수행하면, 그 트랜잭션이 종료하기 전에는
 * slotHeader의 mSCN은 무한대값으로 설정되고, 그 트랜잭션이 commit하게 되면 그
 * commitSCN이 slotHeader에 저장되게 된다.
 *  이때 트랜잭션은 먼저 자신의 commitSCN을 설정하고, 자신이 연산을 수행한
 * slotHeader에 대해 mSCN값을 변경하게 된다. 만약 이 사이에 slot의 mSCN값을
 * 가져오게 되면 문제가 발생할 수 있다.
 *  그렇기 때문에 slot의 mSCN값을 가져온 이후에는 이 함수를 수행하여 정확한 값을
 * 얻어온다.
 *  만약 slotHeader의 mSCN값이 무한대라면, 트랜잭션이 commit되었는지 여부를 보고
 * commit되었다면 그 commitSCN을 현재 slot의 mSCN이라고 리턴한다.
 *
 * DRDB:
 *  MMDB와 유사하게 Tx가 Commit된 이후에 TSS에 CommitSCN이 설정되는 사이에
 * Record의 CommitSCN을 확인하려 했을 때 TSS에 아직 CommitSCN이 설정되어 있지
 * 않을 수 있다. 따라서 TSS의 CSCN이 무한대일 때 본 함수를 한번 더 호출해서
 * Tx로부터 현재의 정확한 CommitSCN을 다시 확인한다.
 *
 * Parameters:
 *  aRecTID        - [IN]  TID on Tuple: read only
 *  aRecSCN        - [IN]  SCN on Tuple or TSSlot: read only
 *  aOutSCN        - [OUT] Output SCN
 ******************************************************************************/
void smxTrans::getTransCommitSCN( smTID         aRecTID,
                                  const smSCN * aRecSCN,
                                  smSCN       * aOutSCN )
{
    smxTrans     * sTrans   = NULL;
    smTID          sObjTID  = SM_NULL_TID;
    smxStatus      sStatus  = SMX_TX_BEGIN;
    smSCN          sCommitSCN;
    smSCN          sLegacyTransCommitSCN;

    if ( SM_SCN_IS_NOT_INFINITE( *aRecSCN ) )
    {
        SM_GET_SCN( aOutSCN, aRecSCN );

        IDE_CONT( return_immediately );
    }

    SM_MAX_SCN( &sCommitSCN );
    SM_MAX_SCN( &sLegacyTransCommitSCN );

    /* BUG-47367 recheck를 수행하는 도중 Tx가 이미 end 될수 있다.
     * Tx의 정보를 다시 수집해야한다. */
    IDE_EXCEPTION_CONT( recheck_commitscn );

    /* BUG-45147 */
    ID_SERIAL_BEGIN(sTrans = smxTransMgr::getTransByTID(aRecTID));

    ID_SERIAL_EXEC( sStatus = sTrans->mStatus, 1);

    ID_SERIAL_EXEC( SM_GET_SCN( &sCommitSCN, &(sTrans->mCommitSCN) ), 2 );

    ID_SERIAL_END(sObjTID = sTrans->mTransID);

    if ( aRecTID == sObjTID )
    {
        /* bug-9750 sStatus를 copy할때는  commit상태였지만
         * sCommitSCN을 copy하기전에 tx가 end될수 있다(end이면
         * commitSCN이 inifinite가 된다. */
        if ( (sStatus == SMX_TX_COMMIT) &&
             SM_SCN_IS_NOT_INFINITE(sCommitSCN) )
        {
            if ( SM_SCN_IS_DELETED( *aOutSCN ) )
            {
                SM_SET_SCN_DELETE_BIT( &sCommitSCN );
            }
            else
            {
                /* Do Nothing */
            }

            /* this transaction is committing, so use Tx-CommitSCN */
            SM_SET_SCN(aOutSCN, &sCommitSCN);
        }
        else
        {
            /* BUG-47367 PreCommit은 매우 짧은 시간 유지된다.
             * 대부분의 경우 PreCommit은 아닐것이다. */
            if ( sStatus != SMX_TX_PRECOMMIT )
            {
                /* Tx가 COMMIT 상태가 아니면 aRecSCN의 infinite SCN을 넘겨준다. */
                SM_GET_SCN( aOutSCN, aRecSCN );
            }
            else
            {
                /* BUG-47367 Tx의 Status가 PreCommit 이라면
                 * 곧 정상적인 CommitSCN이 Tx에 설정될 것이다.
                 * Tx의 CommitSCN을 다시 한번 확인해 본다. */
                IDE_CONT( recheck_commitscn );
            }
        }
    }
    else
    {
        /* Legacy Trans인지 확인하고 Legacy Transaction이면
         * Legacy Trans List에서 확인해서 commit SCN을 반환한다. */
        if ( sTrans->mLegacyTransCnt != 0 )
        {
            sLegacyTransCommitSCN = smxLegacyTransMgr::getCommitSCN( aRecTID );

            IDE_ASSERT( SM_SCN_IS_NOT_INFINITE( sLegacyTransCommitSCN ) );
        }

        if ( SM_SCN_IS_MAX( sLegacyTransCommitSCN ) )
        {
            /* already end. another tx is running. */
            SM_GET_SCN( aOutSCN, aRecSCN );
        }
        else
        {
            if ( SM_SCN_IS_DELETED( *aOutSCN ) )
            {
                SM_SET_SCN_DELETE_BIT( &sLegacyTransCommitSCN );
            }
            else
            {
                /* Do Nothing */
            }

            SM_GET_SCN( aOutSCN, &sLegacyTransCommitSCN );
        }
    }

    IDE_EXCEPTION_CONT( return_immediately );
}

/*
 * =============================================================================
 *  현재 statement의 DB접근 형태 즉,
 *  Disk only, memory only, disk /memory애 따라  현재 설정된 transaction의
 *  memory viewSCN, disk viewSCN과 비교하여  더 작은 SCN으로 변경한다.
 * =============================================================================
 *                        !!!!!!!!!!! WARNING  !!!!!!!!!!!!!1
 * -----------------------------------------------------------------------------
 *  아래의 getViewSCN() 함수는 반드시 SMX_TRANS_LOCKED, SMX_TRANS_UNLOCKED
 *  범위 내부에서 수행되어야 한다. 그렇지 않으면,
 *  smxTransMgr::getMinMemViewSCNofAll()에서 이 할당받은 SCN값을 SKIP하는 경우가
 *  발생하고, 이 경우 Ager의 비정상 동작을 초래하게 된다.
 *  (읽기 대상 Tuple의 Aging 발생!!!)
 * =============================================================================
 */
IDE_RC smxTrans::allocViewSCN( UInt aStmtFlag, smSCN * aStmtViewSCN, smSCN aRequestSCN )
{
    UInt     sStmtFlag;
    smSCN    sAccessSCN;
    smSCN    sLstSystemSCN;
    SM_MAX_SCN( &sAccessSCN );

    mLSLockFlag = SMX_TRANS_LOCKED;

    IDL_MEM_BARRIER;

    sStmtFlag = aStmtFlag & SMI_STATEMENT_CURSOR_MASK;
    IDE_DASSERT( (sStmtFlag == SMI_STATEMENT_ALL_CURSOR)  ||
                 (sStmtFlag == SMI_STATEMENT_DISK_CURSOR) ||
                 (sStmtFlag == SMI_STATEMENT_MEMORY_CURSOR) );

    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    // 트랜잭션 begin시 active 트랜잭션 중 oldestFstViewSCN을 설정함
    if ( SM_SCN_IS_INFINITE( mOldestFstViewSCN ) )
    {
        if ( (sStmtFlag == SMI_STATEMENT_ALL_CURSOR) ||
             (sStmtFlag == SMI_STATEMENT_DISK_CURSOR) )
        {
            smxTransMgr::getSysMinDskFstViewSCN( &mOldestFstViewSCN );

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

    // 1.Statement 는 각자 다른 viewSCN를 가질수 있다
    // 2.분산레벨 multi 이하에서는 최신 View를 요구 한다.
    // 3.하나의 Statement가 2개이상으로 쪼개져서 수행되는경우 이전 Statement와 View가 동일해야 한다. 
    //   2랑 3이랑 구분이 안되므로 요구자 viewSCN 있으면 그냥 쓴다. 

    if ( SM_SCN_IS_INIT(aRequestSCN) )
    {
        /* Global Consistent Transaction도 RequestSCN 보내지 않을수 있다. 
         * e.g. 내부에서 사용하는거라던가, prepare라던가.. */

        /* smmDatabase::getViewSCN */
        smxTransMgr::mGetSmmViewSCN( aStmtViewSCN );
    }
    else
    {
        /* Single node 정합성
           -> 하나의 statement 가 분리되어서 두개 이상의 statement 로 동작하는 경우
              요구자 viewSCN이 올수 있다 : 분산레벨을 확인할수 없다. */
        IDE_DASSERT( mIsGCTx == ID_TRUE );   
        IDE_DASSERT( smiGetStartupPhase() == SMI_STARTUP_SERVICE );
        IDE_DASSERT( SM_SCN_IS_NOT_INIT(aRequestSCN) );
        IDE_DASSERT( smuProperty::getShardEnable() == ID_TRUE );
        IDE_DASSERT( SMI_STATEMENT_VIEWSCN_IS_REQUESTED( aStmtFlag ) );

        /* 이 함수가 호출되기전에 aRequestSCN으로 SCN sync 되어 있는 상태. 
         * 그사이 LstSystemSCN 이 증가할수는 있지만 aRequestSCN보다 작을수는 없음  */
        sLstSystemSCN = smmDatabase::getLstSystemSCN();
        IDE_TEST_RAISE( SM_SCN_IS_LT( &sLstSystemSCN, &aRequestSCN ), err_invaild_SCN );
        IDE_TEST_RAISE( SM_SCN_IS_SYSTEMSCN(aRequestSCN) != ID_TRUE, err_invaild_SCN );

        SM_SET_SCN_VIEW_BIT(&aRequestSCN);

        if ( mIsServiceTX == ID_TRUE )
        {
            /* 1.첫번째 stmt는 accessSCN보다 작은 viewSCN 을 허용하지 않는다. */
            if ( SM_SCN_IS_INFINITE( mLastRequestSCN ) )
            {
                smxTransMgr::getAccessSCN( &sAccessSCN );
                IDE_TEST_RAISE( SM_SCN_IS_LT( &aRequestSCN, &sAccessSCN ), err_StatementTooOld );
            }
            else
            {
                /* 2.stmt 마다 다른 ViewSCN 을 가질수 있지만
                   기존에 수행된 stmt 보다 작은 viewSCN 을 허용하지 않는다. */
                IDE_TEST_RAISE( SM_SCN_IS_LT( &aRequestSCN, &mLastRequestSCN ), err_StatementTooOld );
            }
        }

        /* 32비트를 지원해야 하는 경우는 smmDatabase::getViewSCN 을 참고해서 수정되어야 한다. */
        *aStmtViewSCN = aRequestSCN;

        IDE_DASSERT( SM_SCN_IS_VIEWSCN( *aStmtViewSCN ) );
    }

    if ( SMI_STATEMENT_VIEWSCN_IS_REQUESTED( aStmtFlag ) )
    {
        IDE_DASSERT( mIsGCTx == ID_TRUE );   

        /* LastRequestSCN 은 메모리/디스크 공통. 
         * 트랜잭션의 요구자 SCN 을 갱신한다. */
        SM_SET_SCN( &mLastRequestSCN, aStmtViewSCN );

        IDE_DASSERT( SM_SCN_IS_VIEWSCN( mLastRequestSCN ) );
    }

    gSmxTrySetupViewSCNFuncs[sStmtFlag]( this, aStmtViewSCN );

    IDL_MEM_BARRIER;

    mLSLockFlag = SMX_TRANS_UNLOCKED;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invaild_SCN )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_SCN, aRequestSCN ) )
    }
    IDE_EXCEPTION( err_StatementTooOld )
    {
        SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];
        idlOS::snprintf( sMsgBuf,
                         SMI_MAX_ERR_MSG_LEN,
                         "[VIEW VALIDATION] "
                         "StatementViewSCN:%"ID_UINT64_FMT", "
                         "AccessSCN:%"ID_UINT64_FMT", "
                         "LastRequestSCN:%"ID_UINT64_FMT,
                         SM_SCN_TO_LONG(aRequestSCN),
                         SM_SCN_TO_LONG(sAccessSCN),
                         SM_SCN_TO_LONG(mLastRequestSCN) );

        IDE_SET( ideSetErrorCode( smERR_ABORT_StatementTooOld, sMsgBuf ) )

        IDE_ERRLOG( IDE_SD_19 );
    }
    IDE_EXCEPTION_END;

    mLSLockFlag = SMX_TRANS_UNLOCKED;

    return IDE_FAILURE;
}

/*****************************************************************
 *
 * Description: 트랜잭션의 MinDskViewSCN 혹은 MinMemViewSCN 혹은 둘다
 *              갱신 시도한다.
 *
 * aTrans        - [IN] 트랜잭션 포인터
 * aViewSCN      - [IN] Stmt의 ViewSCN
 *****************************************************************/
void smxTrans::trySetupMinAllViewSCN( void   * aTrans,
                                      smSCN  * aViewSCN )
{
    trySetupMinMemViewSCN( aTrans, aViewSCN );

    trySetupMinDskViewSCN( aTrans, aViewSCN );
}


/*****************************************************************
 *
 * Description: 트랜잭션의 유일한 DskStmt라면 트랜잭션의 MinDskViewSCN을
 *              갱신시도한다.
 *
 * 만약 트랜잭션에 MinDskViewSCN이 INFINITE 설정되어 있다는 것은 다른 DskStmt
 * 가 존재하지 않는다는 것을 의미하며 이때, MinDskViewSCN을 설정한다.
 * 단 FstDskViewSCN은 트랜잭션의 첫번째 DskStmt의 SCN으로 설정해주어야 한다.
 *
 * aTrans        - [IN] 트랜잭션 포인터
 * aDskViewSCN   - [IN] DskViewSCN
 *********************************************************************/
void smxTrans::trySetupMinDskViewSCN( void   * aTrans,
                                      smSCN  * aDskViewSCN )
{
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_ASSERT( SM_SCN_IS_VIEWSCN( *aDskViewSCN ) ||
                SM_SCN_IS_INIT(*aDskViewSCN) );

    /* DskViewSCN값을 설정한다. 이미 이함수를 call하는 최상위 함수인
     * smxTrans::allocViewSCN에서 mLSLockFlag를 SMX_TRANS_LOCKED로 설정하였기
     * setSCN대신에 바로 SM_SET_SCN을 이용한다. */
    if ( SM_SCN_IS_INFINITE(sTrans->mMinDskViewSCN) )
    {
        SM_SET_SCN( &sTrans->mMinDskViewSCN, aDskViewSCN );

        if ( SM_SCN_IS_INFINITE( sTrans->mFstDskViewSCN ) )
        {
            /* 트랜잭션의 첫번째 Disk Stmt의 SCN을 설정한다. */
            SM_SET_SCN( &sTrans->mFstDskViewSCN, aDskViewSCN );
        }
    }
    else
    {
        /* 이미 다른 DskStmt가 존재하는 경우이고 현재 SCN이 그 DskStmt의
         * SCN보다 같거나 큰 경우만 존재한다. */
        IDE_ASSERT( SM_SCN_IS_GE( aDskViewSCN,
                                  &sTrans->mMinDskViewSCN ) );
    }
}

/*****************************************************************
 *
 * Description: 트랜잭션의 유일한 MemStmt라면 트랜잭션의 MinMemViewSCN을
 *              갱신시도한다.
 *
 * 만약 트랜잭션에 MinMemViewSCN이 INFINITE 설정되어 있다는 것은 다른 MemStmt
 * 가 존재하지 않는다는 것을 의미하며 이때, MinMemViewSCN을 설정한다.
 *
 * aTrans        - [IN] 트랜잭션 포인터
 * aMemViewSCN   - [IN] MemViewSCN
 *****************************************************************/
void smxTrans::trySetupMinMemViewSCN( void  * aTrans,
                                      smSCN * aMemViewSCN )
{
    smxTrans* sTrans = (smxTrans*) aTrans;

    IDE_ASSERT( SM_SCN_IS_VIEWSCN( *aMemViewSCN ) ||
                SM_SCN_IS_INIT(*aMemViewSCN) );

    /* MemViewSCN값을 설정한다. 이미 이함수를 call하는 최상위 함수인
     * smxTrans::allocViewSCN에서 mLSLockFlag를 SMX_TRANS_LOCKED로
     * 설정하였기 setSCN대신에 바로 SM_SET_SCN을 이용한다. */
    if ( SM_SCN_IS_INFINITE( sTrans->mMinMemViewSCN) )
    {
        SM_SET_SCN( &sTrans->mMinMemViewSCN, aMemViewSCN );
    }
    else
    {
        /* 이미 다른 MemStmt가 존재하는 경우이고 현재 SCN이 그 MemStmt의
         * SCN보다 같거나 큰 경우만 존재한다. */
        IDE_ASSERT( SM_SCN_IS_GE( aMemViewSCN, &sTrans->mMinMemViewSCN ) );
    }
}


/* <<CALLBACK FUNCTION>>
 * 의도 : commit을 수행하는 트랜잭션은 CommitSCN을 할당받은 후에
 *        자신의 상태를 아래와 같은 순서로 변경해야 한다. 
 */

void smxTrans::setTransCommitSCN( void      * aTrans,
                                  smSCN     * aSCN,
                                  void      * aStatus )
{
    smxTrans *sTrans = NULL;

    IDE_DASSERT( ( *(smxStatus *)aStatus == SMX_TX_COMMIT ) ||
                 ( *(smxStatus *)aStatus == SMX_TX_ABORT ) );

    ID_SERIAL_BEGIN( sTrans = (smxTrans *)aTrans );
    ID_SERIAL_EXEC( SM_SET_SCN(&(sTrans->mCommitSCN), aSCN), 1 );
    ID_SERIAL_END( sTrans->mStatus = *(smxStatus *)aStatus );
}
/***********************************************************************
 * Description : PROJ-2733 분산 트랜잭션 정합성
 *
 * 주의주의: aStatus 는 SMX_TX_PREPARED 지만 설정되는 sTrans->mStatus 는 SMX_TX_BEGIN.
 *           rp.sd가 prepared 라는걸 모르는 경우가 있다. 
 **********************************************************************/
void smxTrans::setTransPrepareSCN( void      * aTrans,
                                   smSCN     * aSCN,
                                   void      * aStatus )
{
    smxTrans *sTrans = NULL;

#ifdef DEBUG
    IDE_ASSERT( ((smxTrans *)aTrans)->mStatus == SMX_TX_PRECOMMIT );
    IDE_ASSERT( *(smxStatus *)aStatus == SMX_TX_PREPARED );
    IDE_ASSERT( SM_SCN_IS_INFINITE( ((smxTrans *)aTrans)->mPrepareSCN) );
    IDE_ASSERT( SM_SCN_IS_INFINITE( ((smxTrans *)aTrans)->mCommitSCN) );
#else
    ACP_UNUSED( aStatus );
#endif

    ID_SERIAL_BEGIN( sTrans = (smxTrans *)aTrans );
    ID_SERIAL_EXEC( SM_SET_SCN(&(sTrans->mPrepareSCN), aSCN), 1 );
    ID_SERIAL_END( sTrans->mStatus = SMX_TX_BEGIN );
}

void smxTrans::setTransStatus( void      *aTrans,
                               UInt       aStatus )
{
    smxTrans *sTrans = (smxTrans*) aTrans;
    sTrans->mStatus  = (smxStatus) aStatus;
}

/******************************************************************************
 * 4. Callback for strict ordered setting of Tx SCN & Status
 *
 * aTrans == NULL인 경우는 System SCN을 증가만 시킬경우이다.
 * Delete Thread에서 남아 있는 Aging대상 OID들을 처리하기위해
 * Commit SCN을 증가시킨다.
 *
 * BUG-30911 - 2 rows can be selected during executing index scan
 *             on unique index.
 *
 * LstSystemSCN을 먼저 증가시키고 트랜잭션에 CommitSCN을 설정해야 한다.
 * 로 수정했었는데, BUG-31248 로 인해 다시 원복 합니다.
 *******************************************************************************/
void smxTrans::setTransSCNnStatus( void     * aTrans,
                                   idBool     aIsLegacyTrans,
                                   smSCN    * aSCN,
                                   void     * aStatus )
{
    IDE_DASSERT( aTrans != NULL );

    if( aIsLegacyTrans == ID_TRUE )
    {
        SM_SET_SCN_LEGACY_BIT( aSCN );
    }
    else
    {
        /* do nothing */
    }

    if ( ( *(smxStatus *)aStatus == SMX_TX_COMMIT ) ||
         ( *(smxStatus *)aStatus == SMX_TX_ABORT  ) )
    {
        setTransCommitSCN( aTrans,
                           aSCN,
                           aStatus );

    }
    else
    {
        IDE_DASSERT( *(smxStatus *)aStatus == SMX_TX_PREPARED );
        IDE_DASSERT( ((smxTrans *)aTrans)->mCommitState == SMX_XA_PREPARED );

        setTransPrepareSCN( aTrans,
                            aSCN,
                            aStatus );
    }
}

/**********************************************************************
 *
 * Description : 첫번째 Disk Stmt의 ViewSCN을 설정한다.
 *
 * aTrans         - [IN] 트랜잭션 포인터
 * aFstDskViewSCN - [IN] 첫번째 Disk Stmt의 ViewSCN
 *
 **********************************************************************/
void smxTrans::setFstDskViewSCN( void  * aTrans,
                                 smSCN * aFstDskViewSCN )
{
    SM_SET_SCN( &((smxTrans*)aTrans)->mFstDskViewSCN,
                aFstDskViewSCN );
}

/**********************************************************************
 *
 * Description : system에서 모든 active 트랜잭션들의
 *               oldestFstViewSCN을 반환
 *     BY  BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
 *
 * aTrans - [IN] 트랜잭션 포인터
 *
 **********************************************************************/
smSCN smxTrans::getOldestFstViewSCN( void * aTrans )
{
    smSCN   sOldestFstViewSCN;

    SM_GET_SCN( &sOldestFstViewSCN, &((smxTrans*)aTrans)->mOldestFstViewSCN );

    return sOldestFstViewSCN;
}

// for  fix bug-8084.
IDE_RC smxTrans::begin4LayerCall(void   * aTrans,
                                 UInt     aFlag,
                                 idvSQL * aStatistics)
{
    IDE_ASSERT( aTrans != NULL );

    IDE_ASSERT(((smxTrans*)aTrans)->begin( aStatistics,
                                           aFlag,
                                           SMX_NOT_REPL_TX_ID ) == IDE_SUCCESS );
    return  IDE_SUCCESS;
}
// for  fix bug-8084
IDE_RC smxTrans::abort4LayerCall(void* aTrans)
{

    IDE_ASSERT( aTrans != NULL );

    return  ((smxTrans*)aTrans)->abort( ID_FALSE, /* aIsLegacyTrans */
                                        NULL      /* aLegacyTrans */ );
}
// for  fix bug-8084.
IDE_RC smxTrans::commit4LayerCall(void* aTrans)
{
    IDE_ASSERT( aTrans != NULL );

    return  ((smxTrans*)aTrans)->commit();
}

/***********************************************************************
 *
 * Description : Implicit Savepoint를 기록한다. Implicit SVP에 sStamtDepth
 *               를 같이 기록한다.
 *
 * aSavepoint     - [IN] Savepoint
 * aStmtDepth     - [IN] Statement Depth
 *
 ***********************************************************************/
IDE_RC smxTrans::setImpSavepoint( smxSavepoint **aSavepoint,
                                  UInt           aStmtDepth)
{
    return mSvpMgr.setImpSavepoint( aSavepoint,
                                    aStmtDepth,
                                    mOIDList->mOIDNodeListHead.mPrvNode,
                                    &mLstUndoNxtLSN,
                                    svrLogMgr::getLastLSN( &mVolatileLogEnv ),
                                    smLayerCallback::getLastLockSequence( mSlotN ) );
}

/***********************************************************************
 * Description : Statement가 종료시 설정했던 Implicit SVP를 Implici SVP
 *               List에서 제거한다.
 *
 * aSavepoint     - [IN] Savepoint
 * aStmtDepth     - [IN] Statement Depth
 ***********************************************************************/
IDE_RC smxTrans::unsetImpSavepoint( smxSavepoint *aSavepoint )
{
    return mSvpMgr.unsetImpSavepoint( aSavepoint );
}

IDE_RC smxTrans::setExpSavepoint(const SChar *aExpSVPName)
{
    return mSvpMgr.setExpSavepoint( this,
                                    aExpSVPName,
                                    SM_OID_NULL,
                                    0,
                                    NULL,
                                    SM_OID_NULL,
                                    0,
                                    NULL,
                                    mOIDList->mOIDNodeListHead.mPrvNode,
                                    &mLstUndoNxtLSN,
                                    svrLogMgr::getLastLSN( &mVolatileLogEnv ),
                                    smLayerCallback::getLastLockSequence( mSlotN ) );
}

void smxTrans::reservePsmSvp( idBool aIsShard )
{
    mSvpMgr.reservePsmSvp( mOIDList->mOIDNodeListHead.mPrvNode,
                           &mLstUndoNxtLSN,
                           svrLogMgr::getLastLSN( &mVolatileLogEnv ),
                           smLayerCallback::getLastLockSequence( mSlotN ),
                           aIsShard );
};

/* BUG-48489 */
idBool smxTrans::isExistExpSavepoint(const SChar *aSavepointName)
{
    return mSvpMgr.isExistExpSavepoint(aSavepointName);
}

IDE_RC smxTrans::setExpSvpForBackupDDLTargetTableInfo( smOID   aOldTableOID, 
                                                       UInt    aOldPartOIDCount,
                                                       smOID * aOldPartOIDArray,
                                                       smOID   aNewTableOID,
                                                       UInt    aNewPartOIDCount,
                                                       smOID * aNewPartOIDArray )
{
    return mSvpMgr.setExpSavepoint( this,
                                    SM_DDL_INFO_SAVEPOINT,
                                    aOldTableOID,
                                    aOldPartOIDCount,
                                    aOldPartOIDArray,
                                    aNewTableOID,
                                    aNewPartOIDCount,
                                    aNewPartOIDArray,
                                    mOIDList->mOIDNodeListHead.mPrvNode,
                                    &mLstUndoNxtLSN,
                                    svrLogMgr::getLastLSN( &mVolatileLogEnv ),
                                    smLayerCallback::getLastLockSequence( mSlotN ) );
}

void smxTrans::rollbackDDLTargetTableInfo()
{
    mSvpMgr.rollbackDDLTargetTableInfo( NULL );
}

/*****************************************************************
 * Description: Transaction Status Slot 할당
 *
 * [ 설명 ]
 *
 * 디스크 갱신 트랜잭션이 TSS도 필요하고, UndoRow를 기록하기도
 * 해야하기 때문에 트랜잭션 세그먼트 엔트리를 할당하여 TSS세그먼트와
 * 언두 세그먼트를 확보해야 한다.
 *
 * 확보된 TSS 세그먼트로부터 TSS를 할당하여 트랜잭션에 설정한다.
 *
 * [ 인자 ]
 *
 * aStatistics      - [IN] 통계정보
 * aStartInfo       - [IN] 트랜잭션 및 로깅정보
 *
 *****************************************************************/
IDE_RC smxTrans::allocTXSegEntry( idvSQL          * aStatistics,
                                  sdrMtxStartInfo * aStartInfo )
{
    smxTrans      * sTrans;
    sdcTXSegEntry * sTXSegEntry;
    UInt            sManualBindingTxSegByEntryID;

    IDE_ASSERT( aStartInfo != NULL );

    sTrans = (smxTrans*)aStartInfo->mTrans;

    if ( sTrans->mTXSegEntry == NULL )
    {
        IDE_ASSERT( sTrans->mTXSegEntryIdx == ID_UINT_MAX );

        sManualBindingTxSegByEntryID = smuProperty::getManualBindingTXSegByEntryID();

        // BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
        // 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
        if ( sManualBindingTxSegByEntryID ==
             SMX_AUTO_BINDING_TRANSACTION_SEGMENT_ENTRY )
        {
            IDE_TEST( sdcTXSegMgr::allocEntry( aStatistics, 
                                               aStartInfo,
                                               &sTXSegEntry )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sdcTXSegMgr::allocEntryBySegEntryID(
                                              sManualBindingTxSegByEntryID,
                                              &sTXSegEntry )
                      != IDE_SUCCESS );
        }

        sTrans->mTXSegEntryIdx = sTXSegEntry->mEntryIdx;
        sTrans->mTXSegEntry    = sTXSegEntry;

        /*
         * [BUG-27542] [SD] TSS Page Allocation 관련 함수(smxTrans::allocTXSegEntry,
         *             sdcTSSegment::bindTSS)들의 Exception처리가 잘못되었습니다.
         */
        IDU_FIT_POINT( "1.BUG-27542@smxTrans::allocTXSegEntry" );
    }
    else
    {
        sTXSegEntry = sTrans->mTXSegEntry;
    }

    /*
     * 해당 트랜잭션이 한번도 Bind된 적이 없다면 bindTSS를 수행한다.
     */
    if ( sTXSegEntry->mTSSlotSID == SD_NULL_SID )
    {
        IDE_TEST( sdcTXSegMgr::getTSSegPtr(sTXSegEntry)->bindTSS( aStatistics,
                                                                  aStartInfo )
                  != IDE_SUCCESS );
    }

    IDE_ASSERT( sTXSegEntry->mExtRID4TSS != SD_NULL_RID );
    IDE_ASSERT( sTXSegEntry->mTSSlotSID  != SD_NULL_SID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxTrans::executePendingList( idBool aIsCommit )
{

    smuList *sOpNode;
    smuList *sBaseNode;
    smuList *sNextNode;

    sBaseNode = &mPendingOp;

    for ( sOpNode = SMU_LIST_GET_FIRST(sBaseNode);
           sOpNode != sBaseNode;
           sOpNode = sNextNode )
    {
        IDE_TEST( sctTableSpaceMgr::executePendingOperation( mStatistics,
                                                             sOpNode->mData,
                                                             aIsCommit)
                  != IDE_SUCCESS );

        sNextNode = SMU_LIST_GET_NEXT(sOpNode);

        SMU_LIST_DELETE(sOpNode);

        IDE_TEST( iduMemMgr::free(sOpNode->mData) != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free(sOpNode) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/******************************************************
 * Description: pending operation을 pending list에 추가
 *******************************************************/
void  smxTrans::addPendingOperation( void*    aTrans,
                                     smuList* aPendingOp )
{

    smxTrans *sTrans;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPendingOp != NULL );

    sTrans = (smxTrans *)aTrans;
    SMU_LIST_ADD_LAST(&(sTrans->mPendingOp), aPendingOp);

    return;

}

/**********************************************************************
 * Tx's PrivatePageList를 반환한다.
 *
 * aTrans           : 작업대상 트랜잭션 객체
 * aTableOID        : 작업대상 테이블 OID
 * aPrivatePageList : 반환하려는 PrivatePageList 포인터
 **********************************************************************/
IDE_RC smxTrans::findPrivatePageList(
                            void*                     aTrans,
                            smOID                     aTableOID,
                            smpPrivatePageListEntry** aPrivatePageList )
{
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_DASSERT( aTrans != NULL );

    *aPrivatePageList = NULL;

#ifdef DEBUG
    if ( sTrans->mPrivatePageListCachePtr == NULL )
    {
        // Cache된 PrivatePageList가 비었다면 HashTable에도 없어야 한다.
        IDE_TEST( smuHash::findNode( &(sTrans->mPrivatePageListHashTable),
                                     &aTableOID,
                                     (void**)aPrivatePageList )
                  != IDE_SUCCESS );

        IDE_DASSERT( *aPrivatePageList == NULL );
    }
#endif /* DEBUG */

    if ( sTrans->mPrivatePageListCachePtr != NULL )
    {
        // cache된 PrivatePageList에서 검사한다.
        if ( sTrans->mPrivatePageListCachePtr->mTableOID == aTableOID )
        {
            *aPrivatePageList = sTrans->mPrivatePageListCachePtr;
        }
        else
        {
            // cache된 PrivatePageList가 아니라면 HashTable을 검사한다.
            IDE_TEST( smuHash::findNode( &(sTrans->mPrivatePageListHashTable),
                                         &aTableOID,
                                         (void**)aPrivatePageList )
                      != IDE_SUCCESS );

            if ( *aPrivatePageList != NULL )
            {
                // 새로 찾은 PrivatePageList를 cache한다.
                sTrans->mPrivatePageListCachePtr = *aPrivatePageList;
            }
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Tx's PrivatePageList를 반환한다.
 *
 * aTrans           : 작업대상 트랜잭션 객체
 * aTableOID        : 작업대상 테이블 OID
 * aPrivatePageList : 반환하려는 PrivatePageList 포인터
 **********************************************************************/
IDE_RC smxTrans::findVolPrivatePageList(
                            void*                     aTrans,
                            smOID                     aTableOID,
                            smpPrivatePageListEntry** aPrivatePageList )
{
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_DASSERT( aTrans != NULL );

    *aPrivatePageList = NULL;

#ifdef DEBUG
    if ( sTrans->mVolPrivatePageListCachePtr == NULL )
    {
        // Cache된 PrivatePageList가 비었다면 HashTable에도 없어야 한다.
        IDE_TEST( smuHash::findNode( &(sTrans->mVolPrivatePageListHashTable),
                                     &aTableOID,
                                     (void**)aPrivatePageList )
                  != IDE_SUCCESS );

        IDE_DASSERT( *aPrivatePageList == NULL );
    }
#endif /* DEBUG */

    if ( sTrans->mVolPrivatePageListCachePtr != NULL )
    {
        // cache된 PrivatePageList에서 검사한다.
        if ( sTrans->mVolPrivatePageListCachePtr->mTableOID == aTableOID )
        {
            *aPrivatePageList = sTrans->mVolPrivatePageListCachePtr;
        }
        else
        {
            // cache된 PrivatePageList가 아니라면 HashTable을 검사한다.
            IDE_TEST( smuHash::findNode( &(sTrans->mVolPrivatePageListHashTable),
                                         &aTableOID,
                                         (void**)aPrivatePageList )
                      != IDE_SUCCESS );

            if ( *aPrivatePageList != NULL )
            {
                // 새로 찾은 PrivatePageList를 cache한다.
                sTrans->mVolPrivatePageListCachePtr = *aPrivatePageList;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Tx's PrivatePageList를 추가한다.
 *
 * aTrans           : 작업대상 트랜잭션 객체
 * aTableOID        : 작업대상 테이블 OID
 * aPrivatePageList : 추가하려는 PrivatePageList
 **********************************************************************/

IDE_RC smxTrans::addPrivatePageList(
                                void*                    aTrans,
                                smOID                    aTableOID,
                                smpPrivatePageListEntry* aPrivatePageList )
{
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPrivatePageList != NULL );
    IDE_DASSERT( aPrivatePageList != sTrans->mPrivatePageListCachePtr);

    IDE_TEST( smuHash::insertNode( &(sTrans->mPrivatePageListHashTable),
                                   &aTableOID,
                                   aPrivatePageList)
              != IDE_SUCCESS );

    // 새로 입력된 PrivatePageList를 cache한다.
    sTrans->mPrivatePageListCachePtr = aPrivatePageList;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Tx's PrivatePageList를 추가한다.
 *
 * aTrans           : 작업대상 트랜잭션 객체
 * aTableOID        : 작업대상 테이블 OID
 * aPrivatePageList : 추가하려는 PrivatePageList
 **********************************************************************/
IDE_RC smxTrans::addVolPrivatePageList(
                                void*                    aTrans,
                                smOID                    aTableOID,
                                smpPrivatePageListEntry* aPrivatePageList )
{
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPrivatePageList != NULL );
    IDE_DASSERT( aPrivatePageList != sTrans->mVolPrivatePageListCachePtr );

    IDE_TEST( smuHash::insertNode( &(sTrans->mVolPrivatePageListHashTable),
                                   &aTableOID,
                                   aPrivatePageList)
              != IDE_SUCCESS );

    // 새로 입력된 PrivatePageList를 cache한다.
    sTrans->mVolPrivatePageListCachePtr = aPrivatePageList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * PrivatePageList 생성
 *
 * aTrans           : 추가하려는 트랜잭션
 * aTableOID        : 추가하려는 PrivatePageList의 테이블 OID
 * aPrivatePageList : 추가하려는 PrivateFreePageList의 포인터
 **********************************************************************/
IDE_RC smxTrans::createPrivatePageList(
                            void                      * aTrans,
                            smOID                       aTableOID,
                            smpPrivatePageListEntry  ** aPrivatePageList )
{
    UInt      sVarIdx;
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPrivatePageList != NULL );

    /* smxTrans_createPrivatePageList_alloc_PrivatePageList.tc */
    IDU_FIT_POINT("smxTrans::createPrivatePageList::alloc::PrivatePageList");
    IDE_TEST( sTrans->mPrivatePageListMemPool.alloc((void**)aPrivatePageList)
              != IDE_SUCCESS );

    (*aPrivatePageList)->mTableOID          = aTableOID;
    (*aPrivatePageList)->mFixedFreePageHead = NULL;
    (*aPrivatePageList)->mFixedFreePageTail = NULL;

    for(sVarIdx = 0;
        sVarIdx < SM_VAR_PAGE_LIST_COUNT;
        sVarIdx++)
    {
        (*aPrivatePageList)->mVarFreePageHead[sVarIdx] = NULL;
    }

    IDE_TEST( addPrivatePageList(aTrans,
                                 aTableOID,
                                 *aPrivatePageList)
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Failure of Create Tx's Private Page List");

    return IDE_FAILURE;
}

/**********************************************************************
 * PrivatePageList 생성
 *
 * aTrans           : 추가하려는 트랜잭션
 * aTableOID        : 추가하려는 PrivatePageList의 테이블 OID
 * aPrivatePageList : 추가하려는 PrivateFreePageList의 포인터
 **********************************************************************/
IDE_RC smxTrans::createVolPrivatePageList(
                            void                      * aTrans,
                            smOID                       aTableOID,
                            smpPrivatePageListEntry  ** aPrivatePageList )
{
    UInt      sVarIdx;
    smxTrans* sTrans = (smxTrans*)aTrans;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aPrivatePageList != NULL );

    /* smxTrans_createVolPrivatePageList_alloc_PrivatePageList.tc */
    IDU_FIT_POINT("smxTrans::createVolPrivatePageList::alloc::PrivatePageList");
    IDE_TEST( sTrans->mVolPrivatePageListMemPool.alloc((void**)aPrivatePageList)
              != IDE_SUCCESS );

    (*aPrivatePageList)->mTableOID          = aTableOID;
    (*aPrivatePageList)->mFixedFreePageHead = NULL;
    (*aPrivatePageList)->mFixedFreePageTail = NULL;

    for(sVarIdx = 0;
        sVarIdx < SM_VAR_PAGE_LIST_COUNT;
        sVarIdx++)
    {
        (*aPrivatePageList)->mVarFreePageHead[sVarIdx] = NULL;
    }

    IDE_TEST( addVolPrivatePageList(aTrans,
                                    aTableOID,
                                   *aPrivatePageList)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Failure of Create Tx's Private Page List");

    return IDE_FAILURE;
}

/**********************************************************************
 * Tx's PrivatePageList의 모든 FreePage들을 테이블에 보내고,
 * PrivatePageList를 제거한다.
 **********************************************************************/
IDE_RC smxTrans::finAndInitPrivatePageList()
{
    smcTableHeader*          sTableHeader;
    UInt                     sVarIdx;
    smpPrivatePageListEntry* sPrivatePageList = NULL;

    // PrivatePageList의 HashTable에서 하나씩 전부 가져온다.
    IDE_TEST( smuHash::open(&mPrivatePageListHashTable) != IDE_SUCCESS );
    IDE_TEST( smuHash::cutNode( &mPrivatePageListHashTable,
                                (void**)&sPrivatePageList )
             != IDE_SUCCESS );

    IDE_DASSERT( ( mPrivatePageListCachePtr != NULL &&
                   sPrivatePageList != NULL ) ||
                 ( mPrivatePageListCachePtr == NULL &&
                   sPrivatePageList == NULL ) );

    // merge Private Page List To TABLE
    while ( sPrivatePageList != NULL )
    {
        IDE_ASSERT( smcTable::getTableHeaderFromOID( 
                                            sPrivatePageList->mTableOID,
                                            (void**)&sTableHeader )
                    == IDE_SUCCESS );

        // for FixedEntry
        if ( sPrivatePageList->mFixedFreePageHead != NULL )
        {
            IDE_TEST( smpFreePageList::addFreePagesToTable(
                                        this,
                                        &(sTableHeader->mFixed.mMRDB),
                                        sPrivatePageList->mFixedFreePageHead)
                      != IDE_SUCCESS );
        }

        // for VarEntry
        for(sVarIdx = 0;
            sVarIdx < SM_VAR_PAGE_LIST_COUNT;
            sVarIdx++)
        {
            if ( sPrivatePageList->mVarFreePageHead[sVarIdx] != NULL )
            {
                IDE_TEST( smpFreePageList::addFreePagesToTable(
                                     this,
                                     &(sTableHeader->mVar.mMRDB[sVarIdx]),
                                     sPrivatePageList->mVarFreePageHead[sVarIdx])
                         != IDE_SUCCESS );
            }
        }

        IDE_TEST( mPrivatePageListMemPool.memfree(sPrivatePageList)
                  != IDE_SUCCESS );

        // PrivatePageList의 HashTable에서 다음 것을 가져온다.
        IDE_TEST( smuHash::cutNode( &mPrivatePageListHashTable,
                                    (void**)&sPrivatePageList )
                  != IDE_SUCCESS );
    }

    // PrivatePageList의 HashTable이 사용됐다면 제거한다.
    IDE_TEST( smuHash::close(&mPrivatePageListHashTable) != IDE_SUCCESS );

    mPrivatePageListCachePtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Failure of Destroy Tx's Private Page List");

    return IDE_FAILURE;
}

/**********************************************************************
 * Tx's PrivatePageList의 모든 FreePage들을 테이블에 보내고,
 * PrivatePageList를 제거한다.
 **********************************************************************/
IDE_RC smxTrans::finAndInitVolPrivatePageList()
{
    smcTableHeader*          sTableHeader;
    UInt                     sVarIdx;
    smpPrivatePageListEntry* sPrivatePageList = NULL;

    // PrivatePageList의 HashTable에서 하나씩 전부 가져온다.
    IDE_TEST( smuHash::open(&mVolPrivatePageListHashTable) != IDE_SUCCESS );
    IDE_TEST( smuHash::cutNode( &mVolPrivatePageListHashTable,
                                (void**)&sPrivatePageList )
              != IDE_SUCCESS );

    IDE_DASSERT( ( mVolPrivatePageListCachePtr != NULL &&
                   sPrivatePageList != NULL ) ||
                 ( mVolPrivatePageListCachePtr == NULL &&
                   sPrivatePageList == NULL ) );

    // merge Private Page List To TABLE
    while ( sPrivatePageList != NULL )
    {
        IDE_ASSERT( smcTable::getTableHeaderFromOID( 
                                            sPrivatePageList->mTableOID,
                                            (void**)&sTableHeader )
                    == IDE_SUCCESS );

        // for FixedEntry
        if ( sPrivatePageList->mFixedFreePageHead != NULL )
        {
            IDE_TEST( svpFreePageList::addFreePagesToTable(
                                     this,
                                     &(sTableHeader->mFixed.mVRDB),
                                     sPrivatePageList->mFixedFreePageHead)
                     != IDE_SUCCESS );
        }

        // for VarEntry
        for(sVarIdx = 0;
            sVarIdx < SM_VAR_PAGE_LIST_COUNT;
            sVarIdx++)
        {
            if ( sPrivatePageList->mVarFreePageHead[sVarIdx] != NULL )
            {
                IDE_TEST( svpFreePageList::addFreePagesToTable(
                                     this,
                                     &(sTableHeader->mVar.mVRDB[sVarIdx]),
                                     sPrivatePageList->mVarFreePageHead[sVarIdx])
                          != IDE_SUCCESS );
            }
        }

        IDE_TEST( mVolPrivatePageListMemPool.memfree(sPrivatePageList)
                  != IDE_SUCCESS );

        // PrivatePageList의 HashTable에서 다음 것을 가져온다.
        IDE_TEST( smuHash::cutNode( &mVolPrivatePageListHashTable,
                                   (void**)&sPrivatePageList )
                 != IDE_SUCCESS );
    }

    // PrivatePageList의 HashTable이 사용됐다면 제거한다.
    IDE_TEST( smuHash::close(&mVolPrivatePageListHashTable) != IDE_SUCCESS );

    mVolPrivatePageListCachePtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Failure of Destroy Tx's Private Page List");

    return IDE_FAILURE;
}


/**********************************************************************
 * BUG-30871 When excuting ALTER TABLE in MRDB, the Private Page Lists of
 * new and old table are registered twice.
 * PrivatePageList를 때어냅니다.
 * 이 시점에서 이미 해당 Page들은 TableSpace로 반환한 상태이기 때문에
 * Page에 달지 않습니다.
 **********************************************************************/
IDE_RC smxTrans::dropMemAndVolPrivatePageList(void           * aTrans,
                                              smcTableHeader * aSrcHeader )
{
    smxTrans               * sTrans;
    smpPrivatePageListEntry* sPrivatePageList = NULL;
    smpPrivatePageListEntry* sVolPrivatePageList = NULL;

    sTrans = (smxTrans*)aTrans;

    IDE_TEST( smuHash::findNode( &sTrans->mPrivatePageListHashTable,
                                 &aSrcHeader->mSelfOID,
                                 (void**)&sPrivatePageList )
              != IDE_SUCCESS );
    if ( sPrivatePageList != NULL )
    {
        IDE_TEST( smuHash::deleteNode( 
                                    &sTrans->mPrivatePageListHashTable,
                                    &aSrcHeader->mSelfOID,
                                    (void**)&sPrivatePageList )
                   != IDE_SUCCESS );

        if ( sTrans->mPrivatePageListCachePtr == sPrivatePageList )
        {
            sTrans->mPrivatePageListCachePtr = NULL;
        }

        /* BUG-34384  
         * If an exception occurs while changing memory or vlatile
         * table(alter table) the server is abnormal shutdown
         */
        IDE_TEST( sTrans->mPrivatePageListMemPool.memfree(sPrivatePageList)
                  != IDE_SUCCESS );

    }

    IDE_TEST( smuHash::findNode( &sTrans->mVolPrivatePageListHashTable,
                                 &aSrcHeader->mSelfOID,
                                 (void**)&sVolPrivatePageList )
              != IDE_SUCCESS );

    if ( sVolPrivatePageList != NULL )
    {
        IDE_TEST( smuHash::deleteNode( 
                                &sTrans->mVolPrivatePageListHashTable,
                                &aSrcHeader->mSelfOID,
                                (void**)&sVolPrivatePageList )
                   != IDE_SUCCESS );

        if ( sTrans->mVolPrivatePageListCachePtr == sVolPrivatePageList )
        {
            sTrans->mVolPrivatePageListCachePtr = NULL;
        }

        /* BUG-34384  
         * If an exception occurs while changing memory or vlatile
         * table(alter table) the server is abnormal shutdown
         */
        IDE_TEST( sTrans->mVolPrivatePageListMemPool.memfree(sVolPrivatePageList)
                  != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL("Failure of Destroy Tx's Private Page List");

    return IDE_FAILURE;
}



IDE_RC smxTrans::undoInsertOfTableInfo(void* aTrans, smOID aOIDTable)
{
    return (((smxTrans*)aTrans)->undoInsert(aOIDTable));
}

IDE_RC smxTrans::undoDeleteOfTableInfo(void* aTrans, smOID aOIDTable)
{
    return (((smxTrans*)aTrans)->undoDelete(aOIDTable));
}

void smxTrans::updateSkipCheckSCN(void * aTrans,idBool aDoSkipCheckSCN)
{
    ((smxTrans*)aTrans)->mDoSkipCheckSCN  = aDoSkipCheckSCN;
}

// 특정 Transaction의 RSID를 가져온다.
UInt smxTrans::getRSGroupID(void* aTrans)
{
    smxTrans *sTrans  = (smxTrans*)aTrans;
    UInt sRSGroupID ;

    // SCN로그, Temp table 로그, 등등의 경우 aTrans가 NULL일 수 있다.
    if ( sTrans == NULL )
    {
        // 0번 RSID를 사용
        sRSGroupID = 0 ;
    }
    else
    {
        sRSGroupID = sTrans->mRSGroupID ;
    }

    return sRSGroupID;
}

/*
 * 특정 Transaction에 RSID를 aIdx로 바꾼다.
 *
 * aTrans       [IN]  트랜잭션 객체
 * aIdx         [IN]  Resource ID
 */
void smxTrans:: setRSGroupID(void* aTrans, UInt aIdx)
{
    smxTrans *sTrans  = (smxTrans*)aTrans;
    sTrans->mRSGroupID = aIdx;
}

/*
 * 특정 Transaction에 RSID를 부여한다.
 *
 * 0 < 리스트 ID < Page List Count로 부여된다.
 *
 * aTrans       [IN]  트랜잭션 객체
 * aPageListIdx [OUT] 트랜잭션에게 할당된 Page List ID
 */
void smxTrans::allocRSGroupID(void             *aTrans,
                              UInt             *aPageListIdx)
{
    UInt              sAllocPageListID;
    smxTrans         *sTrans  = (smxTrans*)aTrans;
    static UInt       sPageListCnt = smuProperty::getPageListGroupCount();

    if ( aTrans == NULL )
    {
        // Temp TABLE일 경우 aTrans가 NULL이다.
        sAllocPageListID = 0;
    }
    else
    {
        sAllocPageListID = sTrans->mRSGroupID;

        if ( sAllocPageListID == SMP_PAGELISTID_NULL )
        {
            sAllocPageListID = mAllocRSIdx++ % sPageListCnt;

            sTrans->mRSGroupID = sAllocPageListID;
        }
        else
        {
            /* nothing to do ..  */
        }
    }

    if ( aPageListIdx != NULL )
    {
        *aPageListIdx = sAllocPageListID;
    }

}


/*
 * 특정 Transaction의 로그 버퍼의 내용을 로그파일에 기록한다.
 *
 * aTrans  [IN] 트랜잭션 객체
 */
IDE_RC smxTrans::writeTransLog(void *aTrans, smOID aTableOID )
{
    smxTrans     *sTrans;

    IDE_DASSERT( aTrans != NULL );

    sTrans     = (smxTrans*)aTrans;

    // 트랜잭션 로그 버퍼의 로그를 파일로 기록한다.
    IDE_TEST( smrLogMgr::writeLog( smxTrans::getStatistics( aTrans ),
                                   aTrans,
                                   sTrans->mLogBuffer,
                                   NULL,  // Previous LSN Ptr
                                   NULL,  // Log LSN Ptr
                                   NULL,  // End LSN Ptr
                                   aTableOID )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   callback으로 사용될  isReadOnly 함수

   aTrans [IN] - Read Only 인지  검사할  Transaction 객체.
*/
idBool smxTrans::isReadOnly4Callback(void * aTrans)
{
    smxTrans * sTrans = (smxTrans*) aTrans;
    return sTrans->isReadOnly();
}



// PROJ-1362 QP-Large Record & Internal LOB
// memory lob cursor open function.
IDE_RC  smxTrans::openLobCursor(idvSQL            * aStatistics,
                                void              * aTable,
                                smiLobCursorMode    aOpenMode,
                                smSCN               aLobViewSCN,
                                smSCN               aInfinite,
                                void              * aRow,
                                const smiColumn   * aColumn,
                                UInt                aInfo,
                                smLobLocator      * aLobLocator)
{
    UInt            sState  = 0;
    smLobCursor   * sLobCursor;
    smcLobDesc    * sLobDesc;

    /* TASK-7219 Non-shard DML */
    idBool          sMutexLocked = ID_FALSE;

    IDE_ASSERT( aRow    != NULL );
    IDE_ASSERT( aTable  != NULL );

    /* PROJ-2174 Supporting LOB in the volatile tablespace 
     * memory tablespace 뿐만아니라 volatile tablespace도 가능하다. */
    IDE_ASSERT( (SMI_TABLE_TYPE_IS_MEMORY(   (smcTableHeader*)aTable ) == ID_TRUE ) ||
                (SMI_TABLE_TYPE_IS_VOLATILE( (smcTableHeader*)aTable ) == ID_TRUE ) );
    /*
     * alloc Lob Cursor
     */
 
    IDE_TEST_RAISE( mCurLobCursorID == ID_UINT_MAX, overflowLobCursorID);

    /* TC/FIT/Limit/sm/smx/smxTrans_openLobCursor1_malloc.sql */
    IDU_FIT_POINT_RAISE( "smxTrans::openLobCursor1::malloc",
						  insufficient_memory );

    IDE_TEST_RAISE( mLobCursorPool.alloc((void**)&sLobCursor) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    /*
     * Set Lob Cursor
     */

    /* TASK-7219 Non-shard DML */
    lock();
    sMutexLocked = ID_TRUE;

    sLobCursor->mLobCursorID          = mCurLobCursorID;

    /*
     * Set Lob View Env
     */
 
    sLobCursor->mLobViewEnv.mTable    = aTable;

    sLobCursor->mLobViewEnv.mRow      = aRow;
    idlOS::memcpy( &sLobCursor->mLobViewEnv.mLobCol, aColumn, ID_SIZEOF( smiColumn ) );

    sLobCursor->mLobViewEnv.mTID      = mTransID;
    sLobCursor->mLobViewEnv.mSCN      = aLobViewSCN;
    sLobCursor->mLobViewEnv.mInfinite = aInfinite;
    sLobCursor->mLobViewEnv.mOpenMode = aOpenMode;

    sLobCursor->mLobViewEnv.mWriteOffset = 0;
    sLobCursor->mLobViewEnv.mWritePhase = SM_LOB_WRITE_PHASE_NONE;
    sLobCursor->mLobViewEnv.mWriteError = ID_FALSE;

  
    /* 
     * set version
     */
    sLobDesc = (smcLobDesc*)( (SChar*)aRow + aColumn->offset );

    if ( (sLobDesc->flag & SM_VCDESC_MODE_MASK) == SM_VCDESC_MODE_OUT )
    {
        /* Lob Version */
        IDE_TEST_RAISE( sLobDesc->mLobVersion == ID_ULONG_MAX,
                        error_version_overflow );

        if ( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_WRITE_MODE )
        {
            sLobCursor->mLobViewEnv.mLobVersion = sLobDesc->mLobVersion + 1;
        }
        else
        {
            IDE_ERROR( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_MODE );
            
            sLobCursor->mLobViewEnv.mLobVersion = sLobDesc->mLobVersion;
        }
    }
    else
    {
        sLobCursor->mLobViewEnv.mLobVersion = 0;
    }

    // PROJ-1862 Disk In Mode LOB 에서 추가, 메모리에서는 사용하지 않음
    sLobCursor->mLobViewEnv.mLobColBuf = NULL;
 
    sLobCursor->mInfo                 = aInfo;

    /* PROJ-2174 Supporting LOB in the volatile tablespace 
     * memory tbs와 volatile tbs를 분리해서 처리한다. */
    if ( SMI_TABLE_TYPE_IS_MEMORY( (smcTableHeader*)aTable ) )
    {
        sLobCursor->mModule = &smcLobModule;
    }
    else /* SMI_TABLE_VOLATILE */
    {
        sLobCursor->mModule = &svcLobModule;
    }

    /* TASK-7219 Non-shard DML */
    if ( aStatistics != NULL )
    {
        sLobCursor->mShardLobCursor.mMmSessId = aStatistics->mSess->mSID;
    }
    else
    {
        // Initial value
        sLobCursor->mShardLobCursor.mMmSessId = 0;
    }

    /*
     * hash에 등록
     */
    IDE_TEST( smuHash::insertNode( &mLobCursorHash,
                                   &(sLobCursor->mLobCursorID),
                                   sLobCursor )
              != IDE_SUCCESS );

    *aLobLocator = SMI_MAKE_LOB_LOCATOR(mTransID, mCurLobCursorID);

    /*
     * memory lob cursor list에 등록.
     */
    
    mMemLCL.insert(sLobCursor);

    /* TASK-7219 Non-shard DML */
    sMutexLocked = ID_FALSE;
    unlock();

    //for replication
    /* PROJ-2174 Supporting LOB in the volatile tablespace 
     * volatile tablespace는 replication이 안된다. */
    if ( (sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_WRITE_MODE) &&
         (smcTable::needReplicate((smcTableHeader*)sLobCursor->mLobViewEnv.mTable,
                                 this) == ID_TRUE ) )
    {
        IDE_ASSERT( SMI_TABLE_TYPE_IS_MEMORY( (smcTableHeader*)aTable ) );

        IDE_TEST( sLobCursor->mModule->mWriteLog4LobCursorOpen(
                                              aStatistics,
                                              this,
                                              *aLobLocator,
                                              &(sLobCursor->mLobViewEnv))
                  != IDE_SUCCESS ) ;
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( sLobCursor->mModule->mOpen() != IDE_SUCCESS );

    mCurLobCursorID++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(overflowLobCursorID);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_overflowLobCursorID));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION( error_version_overflow )
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_INTERNAL_ARG, 
                                 "open:version overflow") );
    }
    IDE_EXCEPTION_END;
    {
        if ( sState == 1 )
        {
            IDE_PUSH();
            IDE_ASSERT( mLobCursorPool.memfree((void*)sLobCursor)
                        == IDE_SUCCESS );
            IDE_POP();
        }

        if ( sMutexLocked == ID_TRUE )
        {
            unlock();
        }
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : disk lob cursor-open
 * Implementation :
 *
 *  aStatistics    - [IN]  통계정보
 *  aTable         - [IN]  LOB Column이 위치한 Table의 Table Header
 *  aOpenMode      - [IN]  LOB Cursor Open Mode
 *  aLobViewSCN    - [IN]  봐야 할 SCN
 *  aInfinite4Disk - [IN]  Infinite SCN
 *  aRowGRID       - [IN]  해당 Row의 위치
 *  aColumn        - [IN]  LOB Column의 Column 정보
 *  aInfo          - [IN]  not null 제약등 QP에서 사용함.
 *  aLobLocator    - [OUT] Open 한 LOB Cursor에 대한 LOB Locator
 **********************************************************************/
IDE_RC smxTrans::openLobCursor(idvSQL            * aStatistics,
                               void              * aTable,
                               smiLobCursorMode    aOpenMode,
                               smSCN               aLobViewSCN,
                               smSCN               aInfinite4Disk,
                               scGRID              aRowGRID,
                               smiColumn         * aColumn,
                               UInt                aInfo,
                               smLobLocator      * aLobLocator)
{
    UInt              sState = 0;
    smLobCursor     * sLobCursor;
    smLobViewEnv    * sLobViewEnv;
    sdcLobColBuffer * sLobColBuf;

    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aRowGRID) );
    IDE_ASSERT( aTable != NULL );

    /* TASK-7219 Non-shard DML */
    idBool          sMutexLocked = ID_FALSE;

    //disk table이어야 한다.
    IDE_ASSERT( SMI_TABLE_TYPE_IS_DISK( (smcTableHeader*)aTable ) == ID_TRUE );

    IDE_TEST_RAISE( mCurLobCursorID == ID_UINT_MAX, overflowLobCursorID);

    /*
     * alloc Lob Cursor
     */
    
    /* TC/FIT/Limit/sm/smx/smxTrans_openLobCursor2_malloc1.sql */
    IDU_FIT_POINT_RAISE( "smxTrans::openLobCursor2::malloc1",
                          insufficient_memory );

    IDE_TEST_RAISE( mLobCursorPool.alloc((void**)&sLobCursor) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    /* TC/FIT/Limit/sm/smx/smxTrans_openLobCursor2_malloc2.sql */
    IDU_FIT_POINT_RAISE( "smxTrans::openLobCursor2::malloc2",
						  insufficient_memory );

    IDE_TEST_RAISE( mLobColBufPool.alloc((void**)&sLobColBuf) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 2;
    
    /* TASK-7219 Non-shard DML */
    lock();
    sMutexLocked = ID_TRUE;

    sLobColBuf->mBuffer     = NULL;
    sLobColBuf->mInOutMode  = SDC_COLUMN_IN_MODE;
    sLobColBuf->mLength     = 0;
    sLobColBuf->mIsNullLob  = ID_FALSE;

    /*
     * Set Lob Cursor
     */
    
    sLobCursor->mLobCursorID = mCurLobCursorID;
   
    /*
     * Set Lob View Env
     */
    
    sLobViewEnv = &(sLobCursor->mLobViewEnv);

    sdcLob::initLobViewEnv( sLobViewEnv );

    sLobViewEnv->mTable          = aTable;

    SC_COPY_GRID( aRowGRID, sLobCursor->mLobViewEnv.mGRID );

    /* TASK-7219 Non-shard DML */
    if ( aStatistics != NULL )
    {
        sLobCursor->mShardLobCursor.mMmSessId = aStatistics->mSess->mSID;
    }
    else
    {
        // Initial value
        sLobCursor->mShardLobCursor.mMmSessId = 0;
    }
    
    idlOS::memcpy( &sLobViewEnv->mLobCol, aColumn, ID_SIZEOF(smiColumn) );

    sLobViewEnv->mTID            = mTransID;
    sLobViewEnv->mSCN            = aLobViewSCN;
    sLobViewEnv->mInfinite       = aInfinite4Disk;
    sLobViewEnv->mOpenMode       = aOpenMode;

    sLobViewEnv->mWriteOffset    = 0;
    sLobViewEnv->mWritePhase     = SM_LOB_WRITE_PHASE_NONE;
    sLobViewEnv->mWriteError     = ID_FALSE;
    
    /*
     * For Disk LOB
     */

    sLobViewEnv->mLastReadOffset  = 0;
    sLobViewEnv->mLastReadLeafNodePID = SD_NULL_PID;

    sLobViewEnv->mLastWriteOffset = 0;
    sLobViewEnv->mLastWriteLeafNodePID = SD_NULL_PID;

    /*
     * set version
     */


    sLobViewEnv->mLobColBuf      = (void*)sLobColBuf;
    IDE_TEST( sdcLob::readLobColBuf( aStatistics,
                                     this,
                                     sLobViewEnv )
              != IDE_SUCCESS );

    IDE_TEST( sdcLob::adjustLobVersion(sLobViewEnv) != IDE_SUCCESS );

    sLobCursor->mInfo        = aInfo;
    sLobCursor->mModule      = &sdcLobModule;
 
    /*
     * hash에 등록
     */

    IDE_TEST( smuHash::insertNode( &mLobCursorHash,
                                   &(sLobCursor->mLobCursorID),
                                   sLobCursor )
              != IDE_SUCCESS );
    
    *aLobLocator = SMI_MAKE_LOB_LOCATOR(mTransID, mCurLobCursorID);

    /*
     * disk lob cursor list에 등록.
     */
    
    mDiskLCL.insert(sLobCursor);

    /* TASK-7219 Non-shard DML */
    sMutexLocked = ID_FALSE;
    unlock();

    /*
     * for replication
     */
    
    if ( (sLobViewEnv->mOpenMode == SMI_LOB_READ_WRITE_MODE) &&
         (smcTable::needReplicate( (smcTableHeader*)sLobViewEnv->mTable,
                                   this) == ID_TRUE ) )
    {
        IDE_TEST( sLobCursor->mModule->mWriteLog4LobCursorOpen(
                                           aStatistics,
                                           this,
                                           *aLobLocator,
                                           sLobViewEnv ) != IDE_SUCCESS ) ;
    }

    IDE_TEST( sLobCursor->mModule->mOpen() != IDE_SUCCESS );

    mCurLobCursorID++;

    return IDE_SUCCESS;

    IDE_EXCEPTION(overflowLobCursorID);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_overflowLobCursorID));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    {
        IDE_PUSH();

        switch( sState )
        {
            case 2:
                IDE_ASSERT( mLobColBufPool.memfree((void*)sLobColBuf)
                            == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( mLobCursorPool.memfree((void*)sLobCursor)
                            == IDE_SUCCESS );
                break;
            default:
                break;
        }
        
        IDE_POP();

        /* TASK-7219 Non-shard DML */
        if ( sMutexLocked == ID_TRUE )
        {
            unlock();
        }
    }
    
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : close lob cursor
 * Implementation :
 *
 *  aLobLocator - [IN] 닫을 LOB Cursor의 ID
 **********************************************************************/
IDE_RC smxTrans::closeLobCursor(idvSQL*       aStatistics,
                                smLobCursorID aLobCursorID,
                                idBool        aIsShardLobCursor )
{
    smLobCursor   * sLobCursor  = NULL;
    smuHashBase   * sLobCursorHash = NULL;

    /* TASK-7219 Non-shard DML */
    idBool          sMutexLocked = ID_FALSE;

    lock();
    sMutexLocked = ID_TRUE;

    if ( aIsShardLobCursor == ID_TRUE )
    {
        sLobCursorHash =  &mShardLobCursorHash;
    }
    else
    {
        sLobCursorHash =  &mLobCursorHash;
    }

    /* BUG-40084 */
    IDE_TEST( smuHash::findNode( sLobCursorHash,
                                 &aLobCursorID,
                                 (void **)&sLobCursor ) != IDE_SUCCESS );

    if ( sLobCursor != NULL )
    {
        // hash에서 찾고, hash에서 제거.
        IDE_TEST( smuHash::deleteNode( sLobCursorHash,
                                       &aLobCursorID,
                                       (void **)&sLobCursor )
                  != IDE_SUCCESS );

        /* TASK-7219 Non-shard DML */
        sMutexLocked = ID_FALSE;
        unlock();

        // for Replication
        /* PROJ-2174 Supporting LOB in the volatile tablespace 
         * volatile tablespace는 replication이 안된다. */
        if ( ( aIsShardLobCursor == ID_FALSE ) &&
             ( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_WRITE_MODE ) &&
             ( smcTable::needReplicate((smcTableHeader*)sLobCursor->mLobViewEnv.mTable,
                                      this ) == ID_TRUE ) )
        {
            IDE_ASSERT( ( ( (smcTableHeader *)sLobCursor->mLobViewEnv.mTable )->mFlag &
                          SMI_TABLE_TYPE_MASK )
                        != SMI_TABLE_VOLATILE );

            IDE_TEST( smrLogMgr::writeLobCursorCloseLogRec(
                                        NULL, /* idvSQL* */
                                        this,
                                        SMI_MAKE_LOB_LOCATOR(mTransID, aLobCursorID),
                                        ( (smcTableHeader *)sLobCursor->mLobViewEnv.mTable )->mSelfOID )
                      != IDE_SUCCESS ) ;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( closeLobCursorInternal( aStatistics, sLobCursor ) 
                  != IDE_SUCCESS );
    }
    else
    {
        /* TASK-7219 Non-shard DML */
        sMutexLocked = ID_FALSE;
        unlock();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        /* TASK-7219 Non-shard DML */
        if ( sMutexLocked == ID_TRUE )
        {
            unlock();
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : LobCursor를 실질적으로 닫는 함수
 *
 *  aLobCursor  - [IN]  닫을 LobCursor
 **********************************************************************/
IDE_RC smxTrans::closeLobCursorInternal(idvSQL      * aStatistics,
                                        smLobCursor * aLobCursor)
{
    smSCN             sMemLobSCN;
    smSCN             sDiskLobSCN;
    sdcLobColBuffer * sLobColBuf;

    /* memory이면, memory lob cursor list에서 제거. */
    /* PROJ-2174 Supporting LOB in the volatile tablespace
     * volatile은 memory와 동일하게 처리 */
    if ( (aLobCursor->mModule == &smcLobModule) ||
         (aLobCursor->mModule == &svcLobModule) )
    {
        mMemLCL.remove(aLobCursor);
    }
    else if ( aLobCursor->mModule == &sdcLobModule )
    {
        // disk lob cursor list에서 제거.
        mDiskLCL.remove(aLobCursor);
    }
    else
    {
        /* PROJ-2728 Sharding LOB */
        // shard lob cursor list에서 제거.
        // Sharding LOB이어야 한다.
        IDE_ASSERT( aLobCursor->mModule == &sdiLobModule );
        mShardLCL.remove(aLobCursor);
    }

    // fix BUG-19687
    /* BUG-31315 [sm_resource] Change allocation disk in mode LOB buffer, 
     * from Open disk LOB cursor to prepare for write 
     * LobBuffer 삭제 */
    sLobColBuf = (sdcLobColBuffer*) aLobCursor->mLobViewEnv.mLobColBuf;
    if ( sLobColBuf != NULL )
    {
        IDE_TEST( sdcLob::finalLobColBuffer(sLobColBuf) != IDE_SUCCESS );

        IDE_ASSERT( mLobColBufPool.memfree((void*)sLobColBuf) == IDE_SUCCESS );
        sLobColBuf = NULL;
    }

    (void) aLobCursor->mModule->mClose( aStatistics,
                                        this,
                                        &(aLobCursor->mLobViewEnv) );

    // memory 해제.
    IDE_TEST( mLobCursorPool.memfree((void*)aLobCursor) != IDE_SUCCESS );

    // 모든 lob cursor가 닫혔다면 ,현재 lob cursor id를 0으로 한다.
    mDiskLCL.getOldestSCN(&sDiskLobSCN);
    mMemLCL.getOldestSCN(&sMemLobSCN);

    if ( (SM_SCN_IS_INFINITE(sDiskLobSCN)) &&
         (SM_SCN_IS_INFINITE(sMemLobSCN)) )
    {
        mCurLobCursorID = 0;
    }
    // PROJ-2728 shard lob cursor는 SCN이 없으므로 갯수로 검사한다.
    if ( mShardLCL.getLobCursorCnt(0, NULL) == 0 )
    {
        mCurShardLobCursorID = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : LobCursorID에 해당하는 LobCursor object를 return한다.
 * Implementation :
 *
 *  aLobLocator - [IN]  LOB Cursor ID
 *  aLobLocator - [OUT] LOB Cursor
 **********************************************************************/
IDE_RC smxTrans::getLobCursor( smLobCursorID  aLobCursorID,
                               smLobCursor**  aLobCursor,
                               idBool         aIsShardLobCursor )
{
    smuHashBase   * sLobCursorHash = NULL;

    /* TASK-7219 Non-shard DML */
    idBool          sMutexLocked = ID_FALSE;

    /* TASK-7219 Non-shard DML */
    lock();
    sMutexLocked = ID_TRUE;

    if ( aIsShardLobCursor == ID_TRUE )
    {
        sLobCursorHash = &mShardLobCursorHash;
    }
    else
    {
        sLobCursorHash = &mLobCursorHash;
    }
    IDE_TEST( smuHash::findNode(sLobCursorHash,
                                &aLobCursorID,
                                (void **)aLobCursor)
              != IDE_SUCCESS );

    /* TASK-7219 Non-shard DML */
    sMutexLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sMutexLocked == ID_TRUE )
        {
            unlock();
        }
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 모든 LOB Cursor를 Close한다.
 * Implementation :
 **********************************************************************/
IDE_RC smxTrans::closeAllLobCursors()
{
    smLobCursor          * sLobCursor;
    smSCN                  sDiskLobSCN;
    smSCN                  sMemLobSCN;
    UInt                   sState = 0;

    /* PROJ-2728 Sharding LOB */
    (void) closeAllShardLobCursors();

    //fix BUG-21311
    if ( mCurLobCursorID  != 0 )
    {
        IDE_TEST( smuHash::open( &mLobCursorHash ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( smuHash::cutNode( &mLobCursorHash,
                                    (void **)&sLobCursor ) != IDE_SUCCESS );
 
        while ( sLobCursor != NULL )
        {
            IDE_TEST( closeLobCursorInternal( NULL, /* idvSQL* */
                                              sLobCursor )
                      != IDE_SUCCESS );
            IDE_TEST( smuHash::cutNode( &mLobCursorHash,
                                        (void **)&sLobCursor)
                     != IDE_SUCCESS );
        }

        IDE_TEST( smuHash::close(&mLobCursorHash) != IDE_SUCCESS );

        mDiskLCL.getOldestSCN(&sDiskLobSCN);
        mMemLCL.getOldestSCN(&sMemLobSCN);

        IDE_ASSERT( (SM_SCN_IS_INFINITE(sDiskLobSCN)) && (SM_SCN_IS_INFINITE(sMemLobSCN)));
        IDE_ASSERT( mCurLobCursorID == 0 );
    }
    else
    {
        // zero
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sState != 0 )
        {
            IDE_PUSH();
            IDE_ASSERT( smuHash::close( &mLobCursorHash ) == IDE_SUCCESS );
            IDE_POP();
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든 LOB Cursor를 Close한다.
 * Implementation :
 **********************************************************************/
IDE_RC smxTrans::closeAllLobCursorsWithRPLog()
{
    smLobCursor          * sLobCursor;
    smSCN                  sDiskLobSCN;
    smSCN                  sMemLobSCN;
    UInt                   sState = 0;

    //fix BUG-21311
    if ( mCurLobCursorID  != 0 )
    {
        IDE_TEST( smuHash::open( &mLobCursorHash ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( smuHash::cutNode( &mLobCursorHash,
                                    ( void ** )&sLobCursor ) != IDE_SUCCESS );

        while( sLobCursor != NULL )
        {
            // for Replication
            /* PROJ-2174 Supporting LOB in the volatile tablespace 
             * volatile tablespace는 replication이 안된다. */
            if ( ( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_WRITE_MODE ) &&
                 ( smcTable::needReplicate( ( smcTableHeader* )sLobCursor->mLobViewEnv.mTable,
                                            this ) == ID_TRUE ) )
            {
                IDE_ASSERT( ( ( ( smcTableHeader* )sLobCursor->mLobViewEnv.mTable )->mFlag &
                              SMI_TABLE_TYPE_MASK )
                            != SMI_TABLE_VOLATILE );

                IDE_TEST( smrLogMgr::writeLobCursorCloseLogRec(
                        NULL, /* idvSQL* */
                        this,
                        SMI_MAKE_LOB_LOCATOR( mTransID, sLobCursor->mLobCursorID ),
                        ( ( smcTableHeader* )sLobCursor->mLobViewEnv.mTable )->mSelfOID )
                    != IDE_SUCCESS ) ;
            }
            else
            {
                /* do nothing */
            }

            IDE_TEST( closeLobCursorInternal( NULL, /* idvSQL* */
                                              sLobCursor ) 
                      != IDE_SUCCESS );
            IDE_TEST( smuHash::cutNode( &mLobCursorHash,
                                        (void **)&sLobCursor )
                     != IDE_SUCCESS );
        }

        IDE_TEST( smuHash::close( &mLobCursorHash ) != IDE_SUCCESS );

        mDiskLCL.getOldestSCN( &sDiskLobSCN );
        mMemLCL.getOldestSCN( &sMemLobSCN );

        IDE_ASSERT( ( SM_SCN_IS_INFINITE( sDiskLobSCN ) ) && ( SM_SCN_IS_INFINITE( sMemLobSCN ) ) );
        IDE_ASSERT( mCurLobCursorID == 0 );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sState != 0 )
        {
            IDE_PUSH();
            IDE_ASSERT( smuHash::close( &mLobCursorHash ) == IDE_SUCCESS );
            IDE_POP();
        }
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::closeAllLobCursors( idvSQL *aStatistics,
                                     UInt    aInfo,
                                     idBool  aIsClosingShardLobCursors )
{
    /* TASK-7219 Non-shard DML */
    idBool          sMutexLocked = ID_FALSE;

    if ( aIsClosingShardLobCursors == ID_TRUE )
    {
        (void) closeAllShardLobCursors( aStatistics, aInfo );
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7219 Non-shard DML */
    lock();
    sMutexLocked = ID_TRUE;

    IDE_TEST( closeAllLobCursors( aStatistics, aInfo ) != IDE_SUCCESS );

    /* TASK-7219 Non-shard DML */
    sMutexLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sMutexLocked == ID_TRUE )
        {
            unlock();
        }
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::closeAllShardLobCursors( idvSQL *aStatistics,
                                          UInt    aInfo )
{
    smLobCursor          * sLobCursor;
    UInt                   sState = 0;
    UInt                   sSessID = 0;

    /* TASK-7219 Non-shard DML */
    if ( aStatistics != NULL )
    {
        sSessID = aStatistics->mSess->mSID;
    }
    else
    {
        // Initial value
        sSessID = 0;
    }

    //fix BUG-21311
    if ( mCurShardLobCursorID  != 0 )
    {
        IDE_TEST( smuHash::open( &mShardLobCursorHash ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( smuHash::getCurNode( &mShardLobCursorHash,
                                       (void **)&sLobCursor ) != IDE_SUCCESS );
        while( sLobCursor != NULL )
        {
            /* BUG-48034 자기(session)가 생성한 shard lob cursor만 닫도록 함 */
            // BUG-40427 
            // Client에서 사용하는 LOB Cursor가 아닌 Cursor를 모두 닫는다.
            // aInfo는 CLIENT_TRUE 이다.
            if ( ( sLobCursor->mShardLobCursor.mMmSessId == sSessID ) &&
                 ( ( sLobCursor->mInfo & aInfo ) != aInfo ) )
            {
                IDE_TEST( closeLobCursorInternal( aStatistics, sLobCursor )
                          != IDE_SUCCESS );

                IDE_TEST( smuHash::delCurNode( &mShardLobCursorHash,
                                               (void **)&sLobCursor ) != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( smuHash::getNxtNode( &mShardLobCursorHash,
                                               (void **)&sLobCursor ) != IDE_SUCCESS );
            }
        }

        IDE_TEST( smuHash::close(&mShardLobCursorHash) != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sState != 0 )
        {
            IDE_PUSH();
            IDE_ASSERT( smuHash::close( &mShardLobCursorHash ) == IDE_SUCCESS );
            IDE_POP();
        }
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::closeAllLobCursors( idvSQL *aStatistics,
                                     UInt    aInfo )
{
    smLobCursor          * sLobCursor;
    UInt                   sState = 0;
    UInt                   sSessID = 0;

    if ( aStatistics != NULL )
    {
        sSessID = aStatistics->mSess->mSID;
    }
    else
    {
        // Initial value
        sSessID = 0;
    }

    //fix BUG-21311
    if ( mCurLobCursorID  != 0 )
    {
        IDE_TEST( smuHash::open( &mLobCursorHash ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( smuHash::getCurNode( &mLobCursorHash,
                                       (void **)&sLobCursor ) != IDE_SUCCESS );
        while( sLobCursor != NULL )
        {
            // BUG-40427 
            // Client에서 사용하는 LOB Cursor가 아닌 Cursor를 모두 닫는다.
            // aInfo는 CLIENT_TRUE 이다.
            if ( ( sLobCursor->mShardLobCursor.mMmSessId == sSessID ) &&
                 ( ( sLobCursor->mInfo & aInfo ) != aInfo ) )
            {
                // for Replication
                /* PROJ-2174 Supporting LOB in the volatile tablespace 
                 * volatile tablespace는 replication이 안된다. */
                if ( ( sLobCursor->mLobViewEnv.mOpenMode == SMI_LOB_READ_WRITE_MODE ) &&
                     ( smcTable::needReplicate( ( smcTableHeader* )sLobCursor->mLobViewEnv.mTable,
                                                 this ) == ID_TRUE ) )
                {
                    IDE_ASSERT( ( ( ( smcTableHeader* )sLobCursor->mLobViewEnv.mTable )->mFlag &
                                  SMI_TABLE_TYPE_MASK )
                                != SMI_TABLE_VOLATILE );

                    IDE_TEST( smrLogMgr::writeLobCursorCloseLogRec(
                                                        NULL, /* idvSQL* */
                                                        this,
                                                        SMI_MAKE_LOB_LOCATOR( mTransID, 
                                                                              sLobCursor->mLobCursorID ),
                                                        ( ( smcTableHeader* )sLobCursor->mLobViewEnv.mTable )->mSelfOID )
                              != IDE_SUCCESS ) ;
                }
                else
                {
                    /* do nothing */
                }
                IDE_TEST( closeLobCursorInternal( NULL, /* idvSQL* */
                                                  sLobCursor )
                          != IDE_SUCCESS );

                IDE_TEST( smuHash::delCurNode( &mLobCursorHash,
                                               (void **)&sLobCursor ) != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( smuHash::getNxtNode( &mLobCursorHash,
                                               (void **)&sLobCursor ) != IDE_SUCCESS );
            }
        }

        IDE_TEST( smuHash::close(&mLobCursorHash) != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sState != 0 )
        {
            IDE_PUSH();
            IDE_ASSERT( smuHash::close( &mLobCursorHash ) == IDE_SUCCESS );
            IDE_POP();
        }
    }

    return IDE_FAILURE;
}

IDE_RC smxTrans::closeAllShardLobCursors()
{
    smLobCursor          * sLobCursor;
    UInt                   sState = 0;

    if ( mCurShardLobCursorID  != 0 )
    {
        IDE_TEST( smuHash::open( &mShardLobCursorHash ) != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( smuHash::cutNode( &mShardLobCursorHash,
                                    (void **)&sLobCursor ) != IDE_SUCCESS );
 
        while ( sLobCursor != NULL )
        {
            /* shard lob cursor는 각 노드에서 close 될 것이므로
             * 여기에서는 hash, list로부터 제거만 한다.*/
            (void) deleteShardLobCursor( sLobCursor );

            IDE_TEST( smuHash::cutNode( &mShardLobCursorHash,
                                        (void **)&sLobCursor)
                     != IDE_SUCCESS );
        }

        IDE_TEST( smuHash::close(&mShardLobCursorHash) != IDE_SUCCESS );

        IDE_ASSERT( mShardLCL.getLobCursorCnt(0, NULL) == 0 );
        IDE_ASSERT( mCurShardLobCursorID == 0 );
    }
    else
    {
        // zero
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sState != 0 )
        {
            IDE_PUSH();
            IDE_ASSERT( smuHash::close( &mShardLobCursorHash ) == IDE_SUCCESS );
            IDE_POP();
        }
    }

    return IDE_FAILURE;
}

// PROJ-2728 Sharding LOB
// shard lob cursor open function.
IDE_RC  smxTrans::openShardLobCursor(
                                idvSQL            * aStatistics,
                                UInt                aMmSessId,
                                UInt                aMmStmtId,
                                UInt                aRemoteStmtId,
                                UInt                aNodeId,
                                SShort              aLobLocatorType,
                                smLobLocator        aRemoteLobLocator,
                                UInt                aInfo,
                                smiLobCursorMode    aOpenMode,
                                smLobLocator      * aLobLocator )
{
    UInt            sState  = 0;
    smLobCursor   * sLobCursor;

    /* TASK-7219 Non-shard DML */
    idBool          sMutexLocked = ID_FALSE;

    ACP_UNUSED( aStatistics );

    IDE_TEST_RAISE( mCurShardLobCursorID == ID_UINT_MAX, overflowLobCursorID);

    IDE_TEST_RAISE( mLobCursorPool.alloc((void**)&sLobCursor) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    /* TASK-7219 Non-shard DML */
    lock();
    sMutexLocked = ID_TRUE;

    /*
     * Set Lob Cursor
     */

    sLobCursor->mLobCursorID          = mCurShardLobCursorID;

    /*
     * Set Lob View Env
     */
 
    sLobCursor->mLobViewEnv.mTable    = NULL;
    sLobCursor->mLobViewEnv.mRow      = NULL;

    sLobCursor->mLobViewEnv.mTID      = mTransID;
    sLobCursor->mLobViewEnv.mSCN      = 0;
    sLobCursor->mLobViewEnv.mInfinite = 0;
    sLobCursor->mLobViewEnv.mOpenMode = aOpenMode;

    sLobCursor->mLobViewEnv.mWriteOffset = 0;
    sLobCursor->mLobViewEnv.mWritePhase = SM_LOB_WRITE_PHASE_NONE;
    sLobCursor->mLobViewEnv.mWriteError = ID_FALSE;

    sLobCursor->mLobViewEnv.mLobVersion = 0;
    // PROJ-1862 Disk In Mode LOB 에서 추가, 메모리에서는 사용하지 않음
    sLobCursor->mLobViewEnv.mLobColBuf = NULL;

    sLobCursor->mInfo = aInfo;

    // set shard node info
    sLobCursor->mShardLobCursor.mLobLocatorType   = aLobLocatorType;
    sLobCursor->mShardLobCursor.mMmSessId         = aMmSessId;
    sLobCursor->mShardLobCursor.mMmStmtId         = aMmStmtId;
    sLobCursor->mShardLobCursor.mRemoteStmtId     = aRemoteStmtId;
    sLobCursor->mShardLobCursor.mNodeId           = aNodeId;
    sLobCursor->mShardLobCursor.mRemoteLobLocator = aRemoteLobLocator;
    sLobCursor->mLobViewEnv.mShardLobCursor = &(sLobCursor->mShardLobCursor);

    sLobCursor->mModule = &sdiLobModule;

    // hash에 등록.
    IDE_TEST( smuHash::insertNode( &mShardLobCursorHash,
                                   &(sLobCursor->mLobCursorID),
                                   sLobCursor )
              != IDE_SUCCESS );

    *aLobLocator = SMI_MAKE_SHARD_LOB_LOCATOR(mTransID, sLobCursor->mLobCursorID);

    //shard lob cursor list에 등록.
    mShardLCL.insert(sLobCursor);

    // open에서 하는 것이 없음.
    //IDE_TEST( sLobCursor->mModule->mOpen() != IDE_SUCCESS );

    mCurShardLobCursorID++;

    /* TASK-7219 Non-shard DML */
    sMutexLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION(overflowLobCursorID);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_overflowLobCursorID));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    {
        if ( sState == 1 )
        {
            IDE_PUSH();
            IDE_ASSERT( mLobCursorPool.memfree((void*)sLobCursor)
                        == IDE_SUCCESS );
            IDE_POP();
        }

        if ( sMutexLocked == ID_TRUE )
        {
            unlock();
        }
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Shard Lob Cursor를 삭제한다.
 * Implementation :
 *   각 노드에서 close 될 것이므로 여기에서는 List로부터 제거만 한다.
 **********************************************************************/
IDE_RC smxTrans::deleteShardLobCursor( smLobCursor *aLobCursor )
{
    mShardLCL.remove(aLobCursor);

    // memory 해제.
    (void) mLobCursorPool.memfree((void*)aLobCursor);

    // PROJ-2728 shard lob cursor는 SCN이 없으므로 갯수로 검사한다.
    if ( mShardLCL.getLobCursorCnt(0, NULL) == 0 )
    {
        mCurShardLobCursorID = 0;
    }

    return IDE_SUCCESS;
}

UInt smxTrans::getMemLobCursorCnt(void  *aTrans, UInt aColumnID, void *aRow)
{
    smxTrans* sTrans;

    sTrans = (smxTrans*)aTrans;

    return sTrans->mMemLCL.getLobCursorCnt(aColumnID, aRow);
}

/***********************************************************************
 * Description : 현재 Transaction에서 최근에 Begin한 Normal Statment에
 *               Replication을 위한 Savepoint가 설정되었는지 Check하고
 *               설정이 안되어 있다면 Replication을 위해서 Savepoint를
 *               설정한다. 설정되어 있으면 ID_TRUE, else ID_FALSE
 *
 * aTrans - [IN]  Transaction Pointer
 ***********************************************************************/
idBool smxTrans::checkAndSetImplSVPStmtDepth4Repl(void* aTrans)
{
    smxTrans * sTrans = (smxTrans*) aTrans;

    return sTrans->mSvpMgr.checkAndSetImpSVP4Repl();
}

/***********************************************************************
 * Description : Transaction log buffer의 크기를 return한다.
 *
 * aTrans - [IN]  Transaction Pointer
 ***********************************************************************/
SInt smxTrans::getLogBufferSize(void* aTrans)
{
    smxTrans * sTrans = (smxTrans*) aTrans;

    return sTrans->mLogBufferSize;
}

/***********************************************************************
 * Description : Transaction log buffer의 크기를 Need Size 이상으로 설정
 *
 * Implementation :
 *    Transaction Log Buffer Size가 Need Size 보다 크거나 같은 경우,
 *        nothing to do
 *    Transaction Log Buffer Size가 Need Size 보다 작은 경우,
 *        log buffer 확장
 *
 * aNeedSize - [IN]  Need Log Buffer Size
 *
 * Related Issue:
 *     PROJ-1665 Parallel Direct Path Insert
 *
 ***********************************************************************/
IDE_RC smxTrans::setLogBufferSize( UInt  aNeedSize )
{
    SChar * sLogBuffer;
    UInt    sState  = 0;

    if ( aNeedSize > mLogBufferSize )
    {
        mLogBufferSize = idlOS::align(aNeedSize,
                                      SMX_TRANSACTION_LOG_BUFFER_ALIGN_SIZE);

        sLogBuffer = NULL;

        IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_TRANSACTION_TABLE,
                                    mLogBufferSize,
                                    (void**)&sLogBuffer,
                                    IDU_MEM_FORCE)
                  != IDE_SUCCESS );
        sState = 1;

        if ( mLogOffset != 0 )
        {
            idlOS::memcpy( sLogBuffer, mLogBuffer, mLogOffset );
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( iduMemMgr::free(mLogBuffer) != IDE_SUCCESS );

        mLogBuffer = sLogBuffer;

        // 압축 로그버퍼를 이용하여  압축되는 로그의 크기의 범위는
        // 이미 정해져있기 때문에 압축로그 버퍼의 크기는 변경할 필요가 없다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLogBuffer ) == IDE_SUCCESS );
            sLogBuffer = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Transaction log buffer의 크기를 Need Size 이상으로 설정
 *
 * aTrans    - [IN]  Transaction Pointer
 * aNeedSize - [IN]  Need Log Buffer Size
 *
 * Related Issue:
 *     PROJ-1665 Parallel Direct Path Insert
 *
 ***********************************************************************/
IDE_RC smxTrans::setTransLogBufferSize( void * aTrans,
                                        UInt   aNeedSize )
{
   smxTrans * sTrans = (smxTrans*) aTrans;

   return sTrans->setLogBufferSize( aNeedSize );
}

void smxTrans::setFreeInsUndoSegFlag( void   * aTrans,
                                      idBool   aFlag )
{
    IDE_DASSERT( aTrans != NULL );

    ((smxTrans*)aTrans)->mFreeInsUndoSegFlag = aFlag;

    return;
}

void smxTrans::setMemoryTBSAccessed4Callback(void * aTrans)
{
    smxTrans * sTrans;

    IDE_DASSERT( aTrans != NULL );

    sTrans = (smxTrans*) aTrans;

    sTrans->setMemoryTBSAccessed();
}

/***********************************************************************
 * Description : 서버 복구시 Prepare 트랜잭션의 트랜잭션 세그먼트 정보 복원
 ***********************************************************************/
void smxTrans::setXaSegsInfo( void    * aTrans,
                              UInt      aTxSegEntryIdx,
                              sdRID     aExtRID4TSS,
                              scPageID  aFstPIDOfLstExt4TSS,
                              sdRID     aFstExtRID4UDS,
                              sdRID     aLstExtRID4UDS,
                              scPageID  aFstPIDOfLstExt4UDS,
                              scPageID  aFstUndoPID,
                              scPageID  aLstUndoPID )
{
    sdcTXSegEntry * sTXSegEntry;

    IDE_ASSERT( aTrans != NULL );

    sTXSegEntry = ((smxTrans*) aTrans)->mTXSegEntry;

    IDE_ASSERT( sTXSegEntry != NULL );

    ((smxTrans*)aTrans)->mTXSegEntryIdx = aTxSegEntryIdx;
    sTXSegEntry->mExtRID4TSS            = aExtRID4TSS;

    sTXSegEntry->mTSSegmt.setCurAllocInfo( aExtRID4TSS,
                                           aFstPIDOfLstExt4TSS,
                                           SD_MAKE_PID(sTXSegEntry->mTSSlotSID) );

    if ( sTXSegEntry->mFstExtRID4UDS == SD_NULL_RID )
    {
        IDE_ASSERT( sTXSegEntry->mLstExtRID4UDS == SD_NULL_RID );
    }

    if ( sTXSegEntry->mFstUndoPID == SD_NULL_PID )
    {
        IDE_ASSERT( sTXSegEntry->mLstUndoPID == SD_NULL_PID );
    }

    sTXSegEntry->mFstExtRID4UDS = aFstExtRID4UDS;
    sTXSegEntry->mLstExtRID4UDS = aLstExtRID4UDS;
    sTXSegEntry->mFstUndoPID    = aFstUndoPID;
    sTXSegEntry->mLstUndoPID    = aLstUndoPID;

    sTXSegEntry->mUDSegmt.setCurAllocInfo( aLstExtRID4UDS,
                                           aFstPIDOfLstExt4UDS,
                                           aLstUndoPID );
}


/* TableInfo를 검색하여 HintDataPID를 반환한다. */
void smxTrans::getHintDataPIDofTableInfo( void       *aTableInfo,
                                          scPageID   *aHintDataPID )
{
    smxTableInfo *sTableInfoPtr = (smxTableInfo*)aTableInfo;

    if ( sTableInfoPtr != NULL )
    {
        smxTableInfoMgr::getHintDataPID(sTableInfoPtr, aHintDataPID );
    }
    else
    {
        *aHintDataPID = SD_NULL_PID;
    }
}

/* TableInfo를 검색하여 HintDataPID를 설정한다.. */
void smxTrans::setHintDataPIDofTableInfo( void       *aTableInfo,
                                          scPageID    aHintDataPID )
{
    smxTableInfo *sTableInfoPtr = (smxTableInfo*)aTableInfo;

    if (sTableInfoPtr != NULL )
    {
        smxTableInfoMgr::setHintDataPID( sTableInfoPtr, aHintDataPID );
    }
}

idBool smxTrans::isNeedLogFlushAtCommitAPrepare( void * aTrans )
{
    smxTrans * sTrans = (smxTrans*)aTrans;
    return sTrans->isNeedLogFlushAtCommitAPrepareInternal();
}

/*******************************************************************************
 * Description : DDL Transaction임을 나타내는 Log Record를 기록한다.
 ******************************************************************************/
IDE_RC smxTrans::writeDDLLog()
{
    smrDDLLog  sLogHeader;
    smrLogType sLogType = SMR_LT_DDL;

    initLogBuffer();

    /* Log header를 구성한다. */
    idlOS::memset(&sLogHeader, 0, ID_SIZEOF(smrDDLLog));

    smrLogHeadI::setType(&sLogHeader.mHead, sLogType);

    smrLogHeadI::setSize(&sLogHeader.mHead,
                         SMR_LOGREC_SIZE(smrDDLLog) +
                         ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sLogHeader.mHead, mTransID);

    smrLogHeadI::setPrevLSN(&sLogHeader.mHead, mLstUndoNxtLSN);

    // BUG-23045 [RP] SMR_LT_DDL의 Log Type Flag는
    //           Transaction Begin에서 결정된 것을 사용해야 합니다
    smrLogHeadI::setFlag(&sLogHeader.mHead, mLogTypeFlag);

    /* BUG-24866
     * [valgrind] SMR_SMC_PERS_WRITE_LOB_PIECE 로그에 대해서
     * Implicit Savepoint를 설정하는데, mReplSvPNumber도 설정해야 합니다. */
    smrLogHeadI::setReplStmtDepth( &sLogHeader.mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    /* Write log header */
    IDE_TEST( writeLogToBuffer( (const void *)&sLogHeader,
                                SMR_LOGREC_SIZE(smrDDLLog) )
             != IDE_SUCCESS );

    /* Write log tail */
    IDE_TEST( writeLogToBuffer( (const void *)&sLogType,
                                ID_SIZEOF(smrLogType) )
             != IDE_SUCCESS );

    IDE_TEST( writeTransLog( this, SM_NULL_OID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxTrans::addTouchedPage( scSpaceID   aSpaceID,
                                 scPageID    aPageID,
                                 SShort      aCTSlotNum )
{
    /* BUG-34446 DPath INSERT를 수행할때는 TPH을 구성하면 안됩니다. */
    if ( mDPathEntry == NULL )
    {
        IDE_TEST( mTouchPageList.add( aSpaceID,
                                      aPageID,
                                      aCTSlotNum )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Staticstics를 세트한다.
 *
 *  BUG-22651  smrLogMgr::updateTransLSNInfo에서
 *             비정상종료되는 경우가 종종있습니다.
 ******************************************************************************/
void smxTrans::setStatistics( idvSQL * aStatistics )
{
    mStatistics = aStatistics;
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    // transaction을 사용하는 session의 변경.
    if ( aStatistics != NULL )
    {
        if ( aStatistics->mSess != NULL )
        {
            mSessionID = aStatistics->mSess->mSID;
        }
        else
        {
            mSessionID = ID_NULL_SESSION_ID;
        }
    }
    else
    {
        mSessionID = ID_NULL_SESSION_ID;
    }
}

/***********************************************************************
 *
 * Description :
 *  infinite scn값을 증가시키고,
 *  output parameter로 증가된 infinite scn값을 반환한다.
 *
 *  aSCN    - [OUT] 증가된 infinite scn 값
 *
 **********************************************************************/
IDE_RC smxTrans::incInfiniteSCNAndGet(smSCN *aSCN)
{
    smSCN sTempSCN = mInfinite;

    SM_ADD_INF_SCN( &sTempSCN );
    IDE_TEST_RAISE( SM_SCN_IS_LT(&sTempSCN, &mInfinite) == ID_TRUE,
                    ERR_OVERFLOW );

    SM_ADD_INF_SCN( &mInfinite );

    *aSCN = mInfinite;
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OVERFLOW );
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiUpdateOverflow ) );
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

ULong smxTrans::getLockTimeoutByUSec( )
{
    return getLockTimeoutByUSec ( smuProperty::getLockTimeOut() );
}
/*
 * BUG-20589 Receiver만 REPLICATION_LOCK_TIMEOUT 적용
 * BUG-33539
 * receiver에서 lock escalation이 발생하면 receiver가 self deadlock 상태가 됩니다
 */
ULong smxTrans::getLockTimeoutByUSec( ULong aLockWaitMicroSec )
{
    ULong sLockTimeOut;

    if ( isReplTrans() == ID_TRUE )
    {
        sLockTimeOut = mReplLockTimeout * 1000000;
    }
    else
    {
        sLockTimeOut = aLockWaitMicroSec;
    }

    return sLockTimeOut;
}

IDE_RC smxTrans::setReplLockTimeout( UInt aReplLockTimeout )
{
    IDE_TEST_RAISE( ( mStatus == SMX_TX_END ) || ( isReplTrans() != ID_TRUE ), ERR_SET_LOCKTIMEOUT );

    mReplLockTimeout = aReplLockTimeout;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_LOCKTIMEOUT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/***********************************************************************
 * Description : BUG-43595 로 인하여 transaction 맴버 변수 출력
 *
 **********************************************************************/
void smxTrans::dumpTransInfo()
{
    ideLog::log(IDE_ERR_0,
                "Dump Transaction Info [%"ID_UINT32_FMT"] %"ID_UINT32_FMT"\n"
                "TransID         : %"ID_UINT32_FMT"\n"
                "mSlotN          : %"ID_UINT32_FMT"\n"
                "Flag            : %"ID_UINT32_FMT"\n"
                "SessionID       : %"ID_UINT32_FMT"\n"
                "MinMemViewSCN   : 0x%"ID_XINT64_FMT"\n"
                "MinDskViewSCN   : 0x%"ID_XINT64_FMT"\n"
                "FstDskViewSCN   : 0x%"ID_XINT64_FMT"\n"
                "OldestFstViewSCN: 0x%"ID_XINT64_FMT"\n"
                "CursorOpenSCN   : 0x%"ID_XINT64_FMT"\n"
                "LastRequestSCN  : 0x%"ID_XINT64_FMT"\n"
                "PrepareSCN      : 0x%"ID_XINT64_FMT"\n"
                "CommitSCN       : 0x%"ID_XINT64_FMT"\n"
                "Infinite        : 0x%"ID_XINT64_FMT"\n"
                "Status          : 0x%"ID_XINT32_FMT"\n"
                "Status4FT       : 0x%"ID_XINT32_FMT"\n"
                "LSLockFlag      : %"ID_UINT32_FMT"\n"
                "IsUpdate        : %"ID_UINT32_FMT"\n"
                "IsTransWaitRepl : %"ID_UINT32_FMT"\n"
                "IsFree          : %"ID_UINT32_FMT"\n"
                "ReplID          : %"ID_UINT32_FMT"\n"
                "LogTypeFlag     : %"ID_UINT32_FMT"\n"
                "CommitState     : %"ID_XINT32_FMT"\n"
                "TransFreeList   : 0x%"ID_XPOINTER_FMT"\n"
                "FstUndoNxtLSN   : ( %"ID_INT32_FMT", %"ID_INT32_FMT" )\n"
                "LstUndoNxtLSN   : ( %"ID_INT32_FMT", %"ID_INT32_FMT" )\n"
                "TotalLogCount   : %"ID_UINT32_FMT"\n"
                "ProcessedUndoLogCount : %"ID_UINT32_FMT"\n"
                "UndoBeginTime   : %"ID_UINT32_FMT"\n"
                "UpdateSize      : %"ID_UINT64_FMT"\n"
                "FstUpdateTime   : %"ID_UINT32_FMT"\n"
                "LogBufferSize   : %"ID_UINT32_FMT"\n"
                "LogOffset       : %"ID_UINT32_FMT"\n"
                "DoSkipCheck     : %"ID_UINT32_FMT"\n"
                "DoSkipCheckSCN  : %"ID_XINT64_FMT"\n"
                "IsDDL           : %"ID_UINT32_FMT"\n"
                "IsFirstLog      : %"ID_UINT32_FMT"\n"
                "LegacyTransCnt  : %"ID_UINT32_FMT"\n"
                "TXSegEntryIdx   : %"ID_INT32_FMT"\n"
                "CurUndoNxtLSN   : ( %"ID_INT32_FMT", %"ID_INT32_FMT" )\n"
                "LastWritedLSN   : ( %"ID_INT32_FMT", %"ID_INT32_FMT" )\n"
                "BeginLogLSN     : ( %"ID_INT32_FMT", %"ID_INT32_FMT" )\n"
                "CommitLogLSN    : ( %"ID_INT32_FMT", %"ID_INT32_FMT" )\n"
                "RSGroupID       : %"ID_INT32_FMT"\n"
                "CurLobCursorID  : %"ID_UINT32_FMT"\n"
                "FreeInsUndoSegFlag : %"ID_UINT32_FMT"\n"
                "DiskTBSAccessed    : %"ID_UINT32_FMT"\n"
                "MemoryTBSAccessed  : %"ID_UINT32_FMT"\n"
                "MetaTableModified  : %"ID_UINT32_FMT"\n"
                "IsReusableRollback : %"ID_UINT32_FMT"\n",
                mSlotN,
                mTransID,
                mTransID,
                mSlotN,
                mFlag,
                mSessionID,
                mMinMemViewSCN,
                mMinDskViewSCN,
                mFstDskViewSCN,
                mOldestFstViewSCN,
                mCursorOpenInfSCN,
                mLastRequestSCN,
                mPrepareSCN,
                mCommitSCN,
                mInfinite,
                mStatus,
                mStatus4FT,
                mLSLockFlag,
                mIsUpdate,
                mIsTransWaitRepl,
                mIsFree,
                mReplID,
                mLogTypeFlag,
                mCommitState,
                mTransFreeList, // pointer
                mFstUndoNxtLSN.mFileNo,
                mFstUndoNxtLSN.mOffset,
                mLstUndoNxtLSN.mFileNo,
                mLstUndoNxtLSN.mOffset,
                mTotalLogCount,
                mProcessedUndoLogCount,
                mUndoBeginTime,
                mUpdateSize,
                mFstUpdateTime,
                mLogBufferSize,
                mLogOffset,
                mDoSkipCheck,
                mDoSkipCheckSCN,
                mIsDDL,
                mIsFirstLog,
                mLegacyTransCnt,
                mTXSegEntryIdx,
                mCurUndoNxtLSN.mFileNo,
                mCurUndoNxtLSN.mOffset,
                mLastWritedLSN.mFileNo,
                mLastWritedLSN.mOffset,
                mBeginLogLSN.mFileNo,
                mBeginLogLSN.mOffset,
                mCommitLogLSN.mFileNo,
                mCommitLogLSN.mOffset,
                mRSGroupID,
                mCurLobCursorID,
                mFreeInsUndoSegFlag,
                mDiskTBSAccessed,
                mMemoryTBSAccessed,
                mMetaTableModified,
                mIsReusableRollback );
}

/* BUG-48282 */
typedef enum smxDistInfoType
{
    SMX_DIST_INFO_TYPE_INIT,  /* 초기값 */
    SMX_DIST_INFO_TYPE_DUMMY, /* DUMMY 분산정보 */
    SMX_DIST_INFO_TYPE_SET    /* NORMAL 분산정보 */
} smxDistInfoType;

smxDistInfoType getSmxDistType( smiDistTxInfo * aInfo )
{
    if ( aInfo->mDistLevel == SMI_DIST_LEVEL_INIT )
    {
        return SMX_DIST_INFO_TYPE_INIT;
    }
    else if ( ( aInfo->mDistLevel != SMI_DIST_LEVEL_INIT ) &&
              SM_SCN_IS_INIT( aInfo->mFirstStmtViewSCN ) )
    {
        return SMX_DIST_INFO_TYPE_DUMMY;
    }
    else /* ( ( aInfo->mDistLevel != SMI_DIST_LEVEL_INIT ) &&
              ( aInfo->mFirstStmtViewSCN > 0 ) ) */
    {
        return SMX_DIST_INFO_TYPE_SET;
    }
}

/* BUG-48282
   분산데드락 탐지를 위해 TX에 분산정보가 설정된다.
   DUMMY분산정보가 설정되면 이후 NORMAL 분산정보로 변경될수있다 (반대로는 불가) */
void smxTrans::setDistTxInfo( smiDistTxInfo * aNewInfo )
{
    smxDistInfoType sCurInfoType = getSmxDistType( &mDistTxInfo );
    smxDistInfoType sNewInfoType = getSmxDistType( aNewInfo );

    switch ( sCurInfoType )
    {
        case SMX_DIST_INFO_TYPE_INIT:

            /* 저장된 분산정보가 없다. 새로 저장한다.  */
            SMI_SET_SMI_DIST_TX_INFO( &mDistTxInfo,
                                      aNewInfo->mFirstStmtViewSCN, /* mFirstStmtViewSCN */
                                      aNewInfo->mFirstStmtTime,    /* mFirstStmtTime */
                                      aNewInfo->mShardPin,         /* mShardPin */
                                      aNewInfo->mDistLevel );      /* mDistLevel */

            break;

        case SMX_DIST_INFO_TYPE_DUMMY:

            /* DUMMY 분산정보 이후 NORMAL 분산정보가 들어온 경우 새로운 정보로 덮어쓴다. */
            if ( sNewInfoType == SMX_DIST_INFO_TYPE_SET )
            {
                SMI_SET_SMI_DIST_TX_INFO( &mDistTxInfo,
                                          aNewInfo->mFirstStmtViewSCN, /* mFirstStmtViewSCN */
                                          aNewInfo->mFirstStmtTime,    /* mFirstStmtTime */
                                          aNewInfo->mShardPin,         /* mShardPin */
                                          aNewInfo->mDistLevel );      /* mDistLevel */
            }

            break;

        case SMX_DIST_INFO_TYPE_SET:

            if ( sNewInfoType == SMX_DIST_INFO_TYPE_SET )
            {
                /* 저장된 분산정보가 있다면, 새로운 분산정보도 동일한 값이어야 한다.
                   단, 분산레벨은 변경될수있다. */
                /* BUG-48829  
                 * GTx Level 이 변경될때 mFirstStmtViewSCN 이 변경될수 있다. 
                 * 분산정보가 변경되어도 이전 값을 사용하면된다. 
                IDE_DASSERT( aNewInfo->mFirstStmtViewSCN == mDistTxInfo.mFirstStmtViewSCN );
                */
                IDE_DASSERT( aNewInfo->mFirstStmtTime    == mDistTxInfo.mFirstStmtTime );
                IDE_DASSERT( aNewInfo->mShardPin         == mDistTxInfo.mShardPin );

                mDistTxInfo.mDistLevel = aNewInfo->mDistLevel;
            }

            break;

        default:
            IDE_DASSERT(0);
    }
}

/* PROJ-2734 */
/* smxTrans initialize와 end시 init() 함수 호출된다. */
void smxTrans::clearDistTxInfo()
{
    SM_INIT_SCN( &mDistTxInfo.mFirstStmtViewSCN );
    mDistTxInfo.mFirstStmtTime.initialize();
    mDistTxInfo.mShardPin  = SMI_SHARD_PIN_INVALID;
    mDistTxInfo.mDistLevel = SMI_DIST_LEVEL_INIT;
}

/***********************************************************************
 * Description : PROJ-2733 분산 트랜잭션 정합성
 *  PrepareSCN 때문에 대기가 발생하는 경우를 생각해 봅니다.

    1. insert
     a.viewSCN < PrepareSCN : CreateSCN은 PrepareSCN보다 늘 같거나 큼. 아직 삽입전 => visible = FALSE 
       => PrepareSCN 관계없이 viewSCN < CreateSCN 이므로 대기할 필요가 없다.
     b.PrepareSCN < viewSCN 인 경우
      PrepareSCN < CreateSCN < viewSCN : 삽입 완료 => visible = TRUE
      PrepareSCN < viewSCN < CreateSCN : 아직 삽입전 => visible = FALSE
       => 둘중에 어떤 경우가 될지 모르니 대기 할 필요 있음. 

    2. delete
     a.viewSCN < PrepareSCN : limitSCN 은 PrepareSCN 보다 같거나 큼. 아직 삭제 전 => visible = TRUE 
      => PrepareSCN 관계없이 viewSCN < limitSCN 이므로 대기할 필요가 없다.  
     b.PrepareSCN < viewSCN 의 경우
      PrepareSCN < limitSCN < viewSCN : 이미 삭제 됨 => visible = FALSE
      PrepareSCN < viewSCN < limitSCN : 아직 삭제 전 => visible = TRUE 
      => 둘중에 어떤 경우가 될지 모르니 대기 할 필요 있음  


     가능한 Tx 상태 정리 
     상태,           sCommitState,    sRowTransStatus, sRowTID,     sPrepareSCN,     sCommitSCN
     prepare 안옴,   SMX_XA_START,    SMX_TX_BEGIN,    sTransID,     SM_SCN_INFINITE, SM_SCN_INFINITE,

     prepare 설정중, SMX_XA_PREPARED, SMX_TX_BEGIN,    sTransID,     SM_SCN_INFINITE, SM_SCN_INFINITE,
                                      SMX_TX_PRECOMMIT

     pending 상태 : prepare 되고 commit 이 아직 안옴
                     SMX_XA_PREPARED, SMX_TX_BEGIN,    sTransID,     aSCN,            SM_SCN_INFINITE,

     commit 설정중,  SMX_XA_PREPARED, SMX_TX_BEGIN,    sTransID,     aSCN,            SM_SCN_INFINITE,
                                      SMX_TX_PRECOMMIT,

     commit/abort/end 됨. 재활용까지됨
                     SMX_XA_COMPLETE, SMX_TX_COMMIT    sTransID,     aSCN,            aSCN, 
                                      SMX_TX_ABORT      != sTransID, SM_SCN_INFINITE, SM_SCN_INFINITE,
                                      SMX_TX_END
                                      SMX_TX_BEGIN,  

 **********************************************************************/
IDE_RC smxTrans::waitPendingTx( smxTrans * aTrans, 
                                smSCN      aRowSCN,
                                smSCN      aViewSCN )
{
    smxTrans  * sRowTrans;
    smSCN       sPrepareSCN;
    smxStatus   sRowTransStatus;
    smTID       sRowTransID = SM_NULL_TID;
    smTID       sRowTID     = SMP_GET_TID( aRowSCN );
#ifdef DEBUG
    smSCN           sCommitSCN;
    smiCommitState  sCommitState;
#endif
    UShort          sTransSlotN;    
    UShort          sRowTransSlotN; 
    UInt            sState = 0;
    PDL_Time_Value  sCurrentTime;
    PDL_Time_Value  sFetchTimeout;
    PDL_Time_Value  sTimeoutVal;
    UInt            sLoop;
    UInt            sIndoubtFetchTimeout;
    UInt            sIndoubtFetchMethod;

    /* 이미 GCTx 인거 확인하고 왔으니 디버그 에서만 죽이자. */
    IDE_DASSERT( aTrans->mIsGCTx == ID_TRUE );
    /* 이미 INFINITE 인거 확인하고 왔으니 디버그 에서만 죽이자. */
    IDE_DASSERT( SM_SCN_IS_INFINITE( aRowSCN ) );
  
    /* 내가 Global Consistent Transaction 이 아니면 끝. */
    IDE_TEST_CONT( aTrans->mIsGCTx == ID_FALSE, return_immediately );
    /* target Row 가 Commit 되었으면 끝. */
    IDE_TEST_CONT( SM_SCN_IS_NOT_INFINITE( aRowSCN ), return_immediately );

    sRowTrans = smxTransMgr::getTransByTID(sRowTID);

    /* target Row 가 Global Consistent Transaction이 수정한 레코드가 아니면 끝. */
    IDE_TEST_CONT( sRowTrans->mIsGCTx == ID_FALSE, return_immediately );

    /* XA_PREPARED 된 tx 만 Commit 대기 */
    IDE_TEST_CONT( sRowTrans->mCommitState != SMX_XA_PREPARED, return_immediately );

    /* BUG-48244 */
    IDE_DASSERT( sRowTID != aTrans->mTransID );
    IDE_TEST_CONT( sRowTID == aTrans->mTransID, return_immediately );

    IDU_FIT_POINT("3.TASK-7220@smxTrans::waitPendingTx");

    /* Row를 수정한 Tx가 정확한 PrepareSCN 을 설정할때까지 대기
       잠깐 동안일것이므로 sleep 하지 않는다. */
    sLoop = 0;
    do
    {
        if ( (++sLoop % SMX_INDOUBT_FETCH_SLEEP_COUNT ) == 0 )
        {
            idlOS::thr_yield();
        } 

        ID_SERIAL_BEGIN( sRowTrans = smxTransMgr::getTransByTID(sRowTID) );
        ID_SERIAL_EXEC( sRowTransStatus = sRowTrans->mStatus, 1);
        ID_SERIAL_EXEC( SM_GET_SCN( &sPrepareSCN, &(sRowTrans->mPrepareSCN) ), 2 );
#ifdef DEBUG
        ID_SERIAL_EXEC( SM_GET_SCN( &sCommitSCN, &(sRowTrans->mCommitSCN) ), 3 );
        ID_SERIAL_EXEC( SM_GET_SCN( &sCommitState, &(sRowTrans->mCommitState) ), 4 );
#endif
        ID_SERIAL_END( sRowTransID = sRowTrans->mTransID );

    } while( SM_SCN_IS_INFINITE(sPrepareSCN) && (sRowTID == sRowTransID) );
    
    /* Row를 수정중이던 TX 가 Commit 하고 나갔으면 끝. */
    IDE_TEST_CONT( (sRowTransStatus != SMX_TX_BEGIN) || (sRowTID != sRowTransID ), return_immediately );

    IDE_DASSERT( sRowTransStatus == SMX_TX_BEGIN );
    IDE_DASSERT( sCommitState == SMX_XA_PREPARED );
    IDE_DASSERT( SM_SCN_IS_SYSTEMSCN(sPrepareSCN) );
    IDE_DASSERT( SM_SCN_IS_VIEWSCN(aViewSCN) );

    /* PrepareSCN > viewSCN 면 끝.  */
    IDE_TEST_CONT( SM_SCN_IS_GT( &sPrepareSCN, &aViewSCN ), return_immediately );

    sTransSlotN          = aTrans->getSlotID();
    sRowTransSlotN       = sRowTrans->getSlotID();

    /* BUG-48250 */
    sIndoubtFetchTimeout = aTrans->mIndoubtFetchTimeout;
    sIndoubtFetchMethod  = aTrans->mIndoubtFetchMethod;

    sFetchTimeout = idlOS::gettimeofday();
    sTimeoutVal.initialize( sIndoubtFetchTimeout, 0 );
    sFetchTimeout += sTimeoutVal;
    
    // X$PendingWait 
    smxTransMgr::registPendingTable( sTransSlotN, sRowTransSlotN );
    sState = 1;

    sLoop = 0;
    do
    { 
        if ( (++sLoop % SMX_INDOUBT_FETCH_SLEEP_COUNT ) == 0 )
        {
            sCurrentTime = idlOS::gettimeofday();
 
            /* INDOUBT_FETCH_TIMEOUT : 0 이면 무한대기 */
            if ( sIndoubtFetchTimeout != 0 )
            {
                if ( sCurrentTime > sFetchTimeout )
                {
                    /* INDOUBT_FETCH_METHOD 가 0 (skip)이 아니면 예외처리 */
                    IDE_TEST_RAISE( sIndoubtFetchMethod != 0, err_IndoubtFetchTimeout );
                    break;
                }
            }

            if( aTrans->mStatistics != NULL )
            {
                /* SessionEvent 에 의한 에러는 INDOUBT_FETCH_METHOD 를 보지 않는다. */
                IDE_TEST( iduCheckSessionEvent(aTrans->mStatistics)
                          != IDE_SUCCESS );
            }

            idlOS::thr_yield();
        }

        ID_SERIAL_BEGIN( sRowTrans = smxTransMgr::getTransByTID(sRowTID) );
        ID_SERIAL_EXEC( sRowTransStatus = sRowTrans->mStatus, 1);
        ID_SERIAL_END( sRowTransID = sRowTrans->mTransID );

    }while( (sRowTransStatus == SMX_TX_BEGIN) && (sRowTID == sRowTransID) );

    sState = 0;
    smxTransMgr::clearPendingTable( sTransSlotN, sRowTransSlotN ); 

    IDE_EXCEPTION_CONT( return_immediately );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_IndoubtFetchTimeout )
    {
        UChar   sXidString[SMI_XID_STRING_LEN];
        ID_XID  sXID;
        initXID( &sXID );
        sRowTrans->getXID( &sXID );
        (void)idaXaConvertXIDToString( NULL,
                                       &sXID,
                                       sXidString,
                                       SMI_XID_STRING_LEN );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INDOUBT_FETCH_TIMEOUT, sXidString ) )

        IDE_ERRLOG( IDE_SD_19 );
    }
    IDE_EXCEPTION_END;
    
    switch ( sState )
    {
    case 1:
        smxTransMgr::clearPendingTable( sTransSlotN, sRowTransSlotN );
    default:
        break;
    } 

    return IDE_FAILURE;
}

void smxTrans::initXID( ID_XID * aXID )
{
    aXID->formatID     = (vULong)-1;
    aXID->gtrid_length = (vULong)-1;
    aXID->bqual_length = (vULong)-1;
    idlOS::memset( aXID->data, 0x00, ID_MAXXIDDATASIZE );
}
