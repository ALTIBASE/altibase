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
 * $Id: smuHash.cpp 83870 2018-09-03 04:32:39Z kclee $
 **********************************************************************/

#include <smDef.h>
#include <smErrorCode.h>
#include <smuHash.h>
#include <smuProperty.h>

#if defined(SMALL_FOOTPRINT)
#define SMU_HASH_MAX_BUCKET_COUNT (8)
#endif

// 0 => No Latch, 1 => latch 
smuHashLatchFunc smuHash::mLatchVector[2] = 
{
    { 
        smuHash::findNodeNoLatch,  // NoLatch Array
        smuHash::insertNodeNoLatch,
        smuHash::deleteNodeNoLatch
    },
    {      
        smuHash::findNodeLatch,    // Latch Array
        smuHash::insertNodeLatch,
        smuHash::deleteNodeLatch
    }
};


IDE_RC smuHash::allocChain(smuHashBase *aBase, smuHashChain **aChain)
{
/*
 *  Chain 객체를  mempool로 부터 할당받아 되돌린다. 
 *  aBase 객체에 등록된 MemoryMgr로 부터 할당받는다.
 */
    /* smuHash_allocChain_alloc_Chain.tc */
    IDU_FIT_POINT("smuHash::allocChain::alloc::Chain");
    IDE_TEST(((iduMemPool *)(aBase->mMemPool))->alloc((void **)aChain) != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smuHash::freeChain(smuHashBase *aBase, smuHashChain *aChain)
{
/*
 *  Chain 객체를  mempool로 해제한다. 
 */
    IDE_TEST(((iduMemPool *)(aBase->mMemPool))->memfree((void *)aChain) != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC smuHash::initialize(smuHashBase    *aBase, 
                           UInt            aConcurrentLevel,/* > 0 */
                           UInt            aBucketCount,
                           UInt            aKeyLength,
                           idBool          aUseLatch,
                           smuHashGenFunc  aHashFunc,
                           smuHashCompFunc aCompFunc)
{
/*
 *  호출자는 객체 smuHashBase를 미리 할당받아 호출한다.
 *  aConcurrencyLevel에 따라 mempool 초기화시 고려해서 인자로 넘긴다.
 *  aBucketCount 만큼 calloc해서 aBase에 매단다.
 *  aKeyLength를 이용하여, smuHashChain의 실제 align된 크기를 계산하고,
 *  [ (ID_SIZEOF(smuHashChain) + aKeyLength)의 8의 align ]
 *  이 값을 mempool 초기화시에 인자로 넘긴다.
 *  Hash, Comp Callback 함수를 설정한다. 
 *  aUseLatch를 이용하여, 함수 포인터 Array vector를 설정한다. 
 */
    UInt            i;
    UInt            sChainSize;
    UInt            sState      = 0;
    UInt            sAllocCount = 0;
    ULong           sAllocSize;
    iduMemPool    * sPool;
    smuHashBucket * sBucket     = NULL;

#if defined(SMALL_FOOTPRINT)
    if( aBucketCount > SMU_HASH_MAX_BUCKET_COUNT )
    {
        aBucketCount = SMU_HASH_MAX_BUCKET_COUNT;
    }
#endif

    IDE_ASSERT(aConcurrentLevel > 0);
    //fix BUG-21311
    aBase->mDidAllocChain = ID_FALSE;
    
    IDE_TEST_RAISE(aBase->mMutex.initialize((SChar*)"SMU_HASH_MUTEX",
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, init_error);
    
    sChainSize = ID_SIZEOF(smuHashChain) + aKeyLength;
    sChainSize = idlOS::align8(sChainSize); // align to 8

    /* TC/FIT/Limit/sm/smu/smuHash_initialize_malloc1.sql */
    IDU_FIT_POINT_RAISE( "smuHash::initialize::malloc1",
                          insufficient_memory );

    /* - Allocate & Init MemoryPool for keyChain */
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMU,
                               ID_SIZEOF(iduMemPool),
                               (void**)&sPool) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;
    sPool = new (sPool) iduMemPool;

    IDE_TEST(sPool->initialize(IDU_MEM_SM_SMU,
                               (SChar*)"SMUHASH_OBJECT",
                               aConcurrentLevel,
                               sChainSize, 
                               64,
                               IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                               ID_TRUE,								/* UseMutex */
                               IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,		/* AlignByte */
                               ID_FALSE,							/* ForcePooling */
                               ID_TRUE,								/* GarbageCollection */
                               ID_TRUE,                             /* HWCacheLine */
                               IDU_MEMPOOL_TYPE_LEGACY              /* mempool type */) 
             != IDE_SUCCESS);			
    aBase->mMemPool = (void *)sPool;

    /* TC/FIT/Limit/sm/smu/smuHash_initialize_malloc2.sql */
    IDU_FIT_POINT_RAISE( "smuHash::initialize::malloc2",
                          insufficient_memory );

    /* BUG-41277 Unsiged Integer value overflows */
    sAllocSize = (ULong)aBucketCount * ID_SIZEOF(smuHashBucket);

    /* - Allocate  Bucket */
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMU,
                               sAllocSize,
                               (void **)&(aBase->mBucket)) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 2;

    /* - Init Bucket */
    aBase->mBucketCount = aBucketCount;

    for (i = 0; i < aBucketCount; i++)
    {
        sBucket = aBase->mBucket + i;

        /* To Fix BUG-24135 [SD] smxTouchPageList의 Hash Bucket 초기개수로
         *                   인해 메모리를 많이 차지함
         * Hash의 Latch 사용여부에 따라 iduLatchObj를 할당하도록 처리함 */
        if ( aUseLatch == ID_TRUE )
        {
            /* TC/FIT/Limit/sm/smu/smuHash_initialize_malloc3.sql */
            IDU_FIT_POINT_RAISE( "smuHash::initialize::malloc3",
                                  insufficient_memory );

            IDE_TEST_RAISE(iduMemMgr::malloc(
                              IDU_MEM_SM_SMU,
                              ID_SIZEOF(iduLatch),
                              (void **)&(sBucket->mLock)) != IDE_SUCCESS,
                            insufficient_memory );
            sAllocCount = i;
            sState      = 3;

            IDE_TEST( sBucket->mLock->initialize((SChar*)"SMU_HASH_LATCH" )
                      != IDE_SUCCESS);
        }
        else
        {
            sBucket->mLock = NULL;
        }

        sBucket->mCount = 0;
        SMU_LIST_INIT_BASE( &(sBucket->mBaseList));
    }

    /* - Init Function Callback Pointers */
    aBase->mHashFunc    = aHashFunc;
    aBase->mCompFunc    = aCompFunc;
    aBase->mLatchVector = &mLatchVector[aUseLatch == ID_FALSE ? 0 : 1 ];
    aBase->mKeyLength   = aKeyLength;

    aBase->mOpened      = ID_FALSE;
    aBase->mCurBucket   = 0;
    aBase->mCurChain    = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(init_error);
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            for( i = 0 ; i <= sAllocCount ; i++ )
            {
                sBucket = aBase->mBucket + i;
                IDE_ASSERT( iduMemMgr::free( sBucket->mLock ) == IDE_SUCCESS );
                sBucket->mLock = NULL;
            }
        case 2:
            IDE_ASSERT( iduMemMgr::free( aBase->mBucket ) == IDE_SUCCESS );
            aBase->mBucket = NULL;
        case 1:
            IDE_ASSERT( iduMemMgr::free( sPool ) == IDE_SUCCESS );
            sPool = NULL;
        default:
            break;
    }
    
    return IDE_FAILURE;
}

IDE_RC smuHash::reset(smuHashBase  *aBase)
{
    smuHashChain*  sCurChain;
    
    sCurChain = NULL;
    IDE_TEST(open(aBase) != IDE_SUCCESS);
    
    while (1)
    {
        IDE_TEST(cutNode(aBase, (void**)&sCurChain) != IDE_SUCCESS);

        if (sCurChain == NULL)
        {
            break;
        }
    }
    
    IDE_TEST(close(aBase) != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smuHash::destroy(smuHashBase  *aBase)
{
    UInt           i;
    smuHashChain*  sCurChain;
    
    /*  mempool을 해제한다. 
     *  단, 현재 매달린 chain이 0이어야 한다. 아닐 경우 ASSERT!!*/
    sCurChain = NULL;
    
    IDE_TEST(open(aBase) != IDE_SUCCESS);
    
    while (1) /* bucket traverse and free chain */
    {
        IDE_TEST(cutNode(aBase, (void**)&sCurChain) != IDE_SUCCESS);

        if (sCurChain == NULL)
        {
            break;
        }
    }
    
    IDE_TEST(close(aBase) != IDE_SUCCESS);
        
    /* Bucket Destroy */
    for (i = 0; i < aBase->mBucketCount; i++)
    {
        smuHashBucket *sBucket = aBase->mBucket + i;

        IDE_ASSERT(sBucket->mCount == 0);

        if ( sBucket->mLock != NULL )
        {
            IDE_TEST(sBucket->mLock->destroy() 
                     != IDE_SUCCESS);

            IDE_TEST(iduMemMgr::free(sBucket->mLock) 
                     != IDE_SUCCESS );
            sBucket->mLock = NULL;
        }
    }

    /* bucket Memory Free */
    IDE_TEST(iduMemMgr::free( (void *)aBase->mBucket) != IDE_SUCCESS);

    /* KeyChain mempool destroy */
    IDE_TEST( ((iduMemPool *)aBase->mMemPool)->destroy() != IDE_SUCCESS);

    /*  free mempool objct */
    IDE_TEST(iduMemMgr::free(aBase->mMemPool) != IDE_SUCCESS);

    IDE_TEST_RAISE(aBase->mMutex.destroy()
                   != IDE_SUCCESS, destroy_error);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(destroy_error);
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexDestroy));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC smuHash::findNode(smuHashBase  *aBase, void *aKeyPtr, void **aNode)
{
    UInt aHashValue;

    // Vector 호출 
    aHashValue = aBase->mHashFunc(aKeyPtr) % aBase->mBucketCount;
    return (*aBase->mLatchVector->findNode)(aBase, 
                                            &(aBase->mBucket[aHashValue]),
                                            aKeyPtr, 
                                            aNode);
}


IDE_RC smuHash::insertNode(smuHashBase  *aBase, void *aKeyPtr, void *aNode)
{
    UInt aHashValue;

    // Vector 호출 
    aHashValue = aBase->mHashFunc(aKeyPtr) % aBase->mBucketCount;
    return (*aBase->mLatchVector->insertNode)(aBase, 
                                            &(aBase->mBucket[aHashValue]),
                                              aKeyPtr, 
                                              aNode);
}

IDE_RC smuHash::deleteNode(smuHashBase  *aBase, void *aKeyPtr, void **aNode)
{
    UInt aHashValue;

    // Vector 호출 
    aHashValue = aBase->mHashFunc(aKeyPtr) % aBase->mBucketCount;
    return (*aBase->mLatchVector->deleteNode)( aBase, 
                                              &(aBase->mBucket[aHashValue]),
                                               aKeyPtr, 
                                               aNode);
}

// =============================================================================
//  No Latch 
// =============================================================================

smuHashChain* smuHash::findNodeInternal(smuHashBase   *aBase, 
                                        smuHashBucket *aBucket, 
                                        void          *aKeyPtr)
{
    smuList *sList;
    smuList *sBaseList;

    sBaseList = &(aBucket->mBaseList);
    
    for (sList = SMU_LIST_GET_NEXT(sBaseList);
         sList != sBaseList;
         sList = SMU_LIST_GET_NEXT(sList))
    {
        if (aBase->mCompFunc(aKeyPtr, ((smuHashChain *)(sList->mData))->mKey) == 0)
        {
            return (smuHashChain *)(sList->mData);
        }
    }
    return NULL;
}


IDE_RC smuHash::findNodeNoLatch(smuHashBase   *aBase, 
                                smuHashBucket *aBucket, 
                                void          *aKeyPtr, 
                                void         **aNode)
{
/* 
 *  Chain을 따라가면서, 동일한 key 값이 존재하는지 검사한다.
 *  있다면 해당 포인터, 없으면 NULL을 설정한다.
 */
    smuHashChain *sChain;
    
    sChain = smuHash::findNodeInternal(aBase, aBucket, aKeyPtr);

    *aNode = sChain == NULL ? NULL : sChain->mNode;

    return IDE_SUCCESS;
}

//#define USE_CLASSIC_MEMORY

IDE_RC smuHash::insertNodeNoLatch(smuHashBase   *aBase, 
                                  smuHashBucket *aBucket, 
                                  void          *aKeyPtr, 
                                  void          *aNode)
{
/*
 *  HashCahin을 mempool로 부터 할당받고, 키를 복사하고, 
 *  노드 포인터를 assign한다.
 *  이 Chain을 해당 Bucket 리스트 헤더에 삽입한다. 
 */
    UInt            sState  = 0;
    smuHashChain  * sChain;
    //fix BUG-21311
    aBase->mDidAllocChain = ID_TRUE;
    // alloc Chain & Init 
#if defined(USE_CLASSIC_MEMORY)


    sChain = (smuHashChain *)idlOS::malloc(idlOS::align8((UInt)ID_SIZEOF(smuHashChain) 
                                                         + aBase->mKeyLength));
    sState = 1;

    IDE_ASSERT(sChain != NULL);
#else
    IDE_TEST(allocChain(aBase, &sChain) != IDE_SUCCESS);
    
#endif
    sChain->mNode       = aNode;
    idlOS::memcpy(sChain->mKey, aKeyPtr, aBase->mKeyLength); // copy of the key

    // Add to List 
    sChain->mList.mData = sChain;
    SMU_LIST_ADD_FIRST( &(aBucket->mBaseList), &(sChain->mList));

    //aBucket->mCount++;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            idlOS::free(sChain);
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC smuHash::deleteNodeNoLatch(smuHashBase   *aBase, 
                                  smuHashBucket *aBucket, 
                                  void          *aKeyPtr, 
                                  void         **aNode)
{
/*
 *  Chain을 따라가면서, 동일한 key 값이 존재하는지 검사한다.
 *  찾을  Chain을 해당 Bucket 리스트에서 제거한다.
 *  *aNode에 해당 Node값을 설정한다.
 *  HashChain을 mempool로 해제한다. 
 */
    smuHashChain *sChain;

    sChain = smuHash::findNodeInternal(aBase, aBucket, aKeyPtr);

    IDE_ASSERT(sChain != NULL);

    SMU_LIST_DELETE(&(sChain->mList));

    *aNode = sChain == NULL ? NULL : sChain->mNode;

#if defined(USE_CLASSIC_MEMORY)
    idlOS::free(sChain);
#else
    IDE_TEST(freeChain(aBase, sChain) != IDE_SUCCESS);
#endif
    //aBucket->mCount--;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// =============================================================================
//  Use Latch 
// =============================================================================

IDE_RC smuHash::findNodeLatch(smuHashBase   *aBase, 
                              smuHashBucket *aBucket, 
                              void          *aKeyPtr, 
                              void         **aNode)
{
/* Lock을 잡는다. 
 * findNodeNoLatch() 호출 
 * UnLock();
 */
    IDE_TEST( aBucket->mLock->lockRead( NULL, /* idvSQL * */
                                  NULL  /* WeArgs */ ) != IDE_SUCCESS);
    
    IDE_TEST(smuHash::findNodeNoLatch(aBase, aBucket, aKeyPtr, aNode) != IDE_SUCCESS);
    
    IDE_TEST( aBucket->mLock->unlock(  ) != IDE_SUCCESS);
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC smuHash::insertNodeLatch(smuHashBase  *aBase, 
                                smuHashBucket *aBucket, 
                                void *aKeyPtr, 
                                void *aNode)
{
/* Lock을 잡는다. 
 * insertNodeNoLatch() 호출 
 * UnLock();
 */
    IDE_TEST( aBucket->mLock->lockWrite( NULL, /*idvSQL* */
                                   NULL /* WeArgs */ ) != IDE_SUCCESS);
    
    IDE_TEST(smuHash::insertNodeNoLatch(aBase, aBucket, aKeyPtr, aNode) != IDE_SUCCESS);
    
    IDE_TEST( aBucket->mLock->unlock(  ) != IDE_SUCCESS);
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smuHash::deleteNodeLatch(smuHashBase  *aBase, 
                                smuHashBucket *aBucket, 
                                void *aKeyPtr, 
                                void **aNode)
{
/* Lock을 잡는다. 
 * deleteNodeNoLatch() 호출 
 * UnLock();
 */
    IDE_TEST( aBucket->mLock->lockWrite( NULL, /* idvSQL* */
                                   NULL  /* WeArgs */ ) != IDE_SUCCESS);
    
    IDE_TEST(smuHash::deleteNodeNoLatch(aBase, aBucket, aKeyPtr, aNode) != IDE_SUCCESS);
    
    IDE_TEST( aBucket->mLock->unlock(  ) != IDE_SUCCESS);
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}



IDE_RC smuHash::lock(smuHashBase  *aBase)
{
/*
 * Hash 매니저에 대한 mutex를 잡는다.
 */
    IDE_TEST_RAISE(aBase->mMutex.lock(NULL) != IDE_SUCCESS, lock_error);
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(lock_error);
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC smuHash::unlock(smuHashBase  *aBase)
{
/*
 * Hash 매니저에 대한 mutex를 푼다. 
 */
    IDE_TEST_RAISE(aBase->mMutex.unlock() != IDE_SUCCESS, unlock_error);
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(unlock_error);
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

// =============================================================================
//  Use Latch 
// =============================================================================


/*
 *  Open시에 호출되며,  첫번째 HashNode를 찾아 놓는다.
 */

smuHashChain *smuHash::searchFirstNode(smuHashBase *aBase)
{
    UInt           i;
    smuList       *sList;
    smuList       *sBaseList;
    smuHashBucket *sBucket;
        
    IDE_ASSERT(aBase->mCurBucket < aBase->mBucketCount);
    IDE_ASSERT(aBase->mOpened == ID_TRUE);
    
    
    for (i = 0; i < aBase->mBucketCount; i++)
    {
        sBucket           = aBase->mBucket + i;
        sBaseList         = &(sBucket->mBaseList);
        
        aBase->mCurBucket = i;
        
        for (sList = SMU_LIST_GET_NEXT(sBaseList);
             sList != sBaseList;
             sList = SMU_LIST_GET_NEXT(sList))
        {
            return (smuHashChain *)(sList->mData);
        }
    }
    return NULL;
}

smuHashChain *smuHash::searchNextNode(smuHashBase *aBase)
{
    idBool         sFirst = ID_TRUE;
    
    UInt           i;
    smuList       *sStartList;
    smuList       *sCurList;
    smuList       *sBaseList;
    smuHashBucket *sBucket;
        
    IDE_ASSERT(aBase->mCurBucket < aBase->mBucketCount);
    IDE_ASSERT(aBase->mOpened == ID_TRUE);
    
    for (i = aBase->mCurBucket;
         i < aBase->mBucketCount;
         i++)
    {
        aBase->mCurBucket = i;
        sBucket           = aBase->mBucket + i;
        sBaseList         = &(sBucket->mBaseList);

        if (sFirst == ID_TRUE)
        {
            sStartList = &(aBase->mCurChain->mList);
            sFirst = ID_FALSE;
        }
        else
        {
            sStartList = sBaseList;
        }
        
        for (sCurList = SMU_LIST_GET_NEXT(sStartList);
             sCurList != sBaseList;
             sCurList = SMU_LIST_GET_NEXT(sCurList))
        {
            aBase->mCurChain = (smuHashChain *)(sCurList->mData);
            return (smuHashChain *)(sCurList->mData);
        }
    }
    aBase->mCurChain = NULL;
    return NULL;
}



IDE_RC smuHash::isEmpty( smuHashBase *aBase,
                         idBool      *aIsEmpty )
{
    IDE_ASSERT( aBase != NULL );
    IDE_ASSERT( aIsEmpty != NULL );

    IDE_TEST( open(aBase) != IDE_SUCCESS );

    if (aBase->mCurChain == NULL)
    {
       *aIsEmpty = ID_TRUE;
    }
    else
    {
       *aIsEmpty = ID_FALSE;
    }

    IDE_TEST( close(aBase) != IDE_SUCCESS );

   return IDE_SUCCESS;

   IDE_EXCEPTION_END;

   return IDE_FAILURE;
}

IDE_RC smuHash::delCurNode(smuHashBase *aBase, void **aNxtNode)
{
    smuHashChain  *sDeleteChain;

    if (aBase->mCurChain != NULL)
    {
        sDeleteChain = aBase->mCurChain;

        aBase->mCurChain = searchNextNode(aBase);
        
        SMU_LIST_DELETE(&(sDeleteChain->mList));
        IDE_TEST(freeChain(aBase, sDeleteChain) != IDE_SUCCESS);
    }
    
    if( aBase->mCurChain != NULL )
    {
        *aNxtNode = aBase->mCurChain->mNode;
    }
    else
    {
        *aNxtNode = NULL;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
