/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemPool.cpp 84924 2019-02-25 04:58:21Z djin $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduMemList.h>
#include <iduMemPool.h>
#include <iduMemPoolMgr.h>


#undef IDU_ALIGN
#define IDU_ALIGN(x)          idlOS::align(x,mAlignByte)

extern const vULong gFence;

/*
 * 해당 mem pool이 mem pool mgr에 의해서 관리되어질수 있는가?
 */
#define IDU_ADD_OR_DEL_MEMPOOL ( (iduMemMgr::isServerMemType() == ID_TRUE) &&\
                                 (mFlag & IDU_MEMPOOL_GARBAGE_COLLECT )    &&\
                                 (mFlag & IDU_MEMPOOL_USE_MUTEX )          &&\
                                 (mFlag & IDU_MEMPOOL_USE_POOLING )  )  

#define IDU_MEMPOOL_MIN_SIZE_FOR_HW_CACHE_ALIGN    (1024*4)  

/* ------------------------------------------------

   MemPool 간략한 설명 ( 2005.3.30 by lons )


   개요
   mempool은 매번 malloc system call을 호출하지 않고
   memory를 할당하기 위한 목적을 가진 클래스이다.
   하나의 mempool에서 할당해 줄 수 있는 메모리의 크기는 항상 고정된다.
   여러 메모리 크기를 할당하고자 한다면 다른 크기로
   init된 mempool을 2개 이상 만들어야 한다.


   구성요소( class )
   iduMemPool : mem list를 관리한다. 하나의 mem pool에서 mem list는
                다수개일 수 있다. 이것은 system cpu 개수와 관계있음.
   iduMemList : 실제 memory를 할당하여 확보하고 있는 list
                memChunk의 리스트로 구성된다.
   iduMemChunk : iduMemSlot list를 유지한다. mempool에서 할당해줄
                 메모리가 모자랄때마다 하나씩 추가된다.
                 iduMemSlot의 free count, max count를 유지한다.
   iduMemSlot : 실제 할당해줄 메모리

    -------      -------
   | chunk | -  | chunk | - ...
    -------      -------
      |
    ---------      ---------
   | element |  - | element | - ....
    ---------      ---------

    (eg.)

    iduMemPool::initialize( cpu개수, ID_SIZEOF(UInt), 5 )을 수행하면
    하나의 chunk와 5개의 element가 생긴다.
    element의 개수는 몇개 만큼의 메모리를 미리 할당하느냐를 결정한다.

    -------
   | chunk |
    -------
      |
    ---------      ---------
   | element |  - | element | - ...
    ---------      ---------

    iduMemPool::alloc을 수행하면
    element 하나가 return 된다.
    만약 element가 부족하면 chunk 하나와 element 5개를 더 할당한다.
 * ----------------------------------------------*/
 /*************************************************************
  * BUG-46165 
  *   [MUST]
  *   when IDU_MEMPOOL_TYPE_TIGHT,the following specifications "MUST" be met.
  *   otherwise, you will encounter assert error.
  *    -  aElemSize must be 2^n, 
  *    -  aElemCount must be 2^n,
  *    -  aElemSize == aAlignByte
  *    -  aElemSize >=8
  *   
  *   [INFO] 
  *   (following things are just for your information.)
  *   In IDU_MEMPOOL_TYPE_TIGHT,
  *   - aHWCacheLine is ignored  
  *   - aElemCount is set to 2 if it is 1.
  *   - __MEMPOOL_MINIMUM_SLOT_COUNT is ignored.
  *************************************************************/
/*-----------------------------------------------------------
 * Description: iduMemPool을 초기화 한다.
 *
 * aIndex            - [IN] 이 함수를 호출한 모듈
 *                        (어디서 호출하는지 정보를 유지하기 위해 필요)
 * aName             - [IN] iduMemPool의 식별 이름
 * aListCount
 * aElemSize         - [IN] 할당할 메모리의 단위크기
 * aElemCount        - [IN] element count
 * aChunkLimit       - [IN] memPool에서 유지할 chunk개수의 최대치.
 *             (단 모든 chunk가 다 할당중인 경우에는 이 개수에 포함되지 않는다.)
 * aUseMutex         - [IN] mutex사용 여부
 * aAlignByte        - [IN] Align 할 Byte 단위.
 * aForcePooling     - [IN] property에서 mem pool을 사용하지 않도록 설정했더라도 
 *                          강제로 생성.(BUG-21547)
 * aGarbageCollection - [IN] Garbage collection에 기여하도록 할것인가 결정.
 *                           ID_TRUE일때 mem pool manager에 의해서 관리되어짐
 * aHWCacheLine       - [IN] H/W cache line align 을 사용할것인가
 *                           it's ignored in IDU_MEMPOOL_TYPE_TIGHT.
 * aType              - [IN] mempool type. legacy or tight
 * ---------------------------------------------------------*/
IDE_RC iduMemPool::initialize(iduMemoryClientIndex aIndex,
                              SChar*               aName,
                              UInt                 aListCount,
                              vULong               aElemSize,
                              vULong               aElemCount,
                              vULong               aChunkLimit,
                              idBool               aUseMutex,
                              UInt                 aAlignByte,
                              idBool               aForcePooling,
                              idBool               aGarbageCollection,
                              idBool               aHWCacheLine,
                              iduMemPoolType       aType)
{
    UInt  i, j;
    ULong sMinSlotCnt = iduProperty::getMinMemPoolSlotCount();

    IDE_DASSERT( aName != NULL );
    IDE_DASSERT( (aType == IDU_MEMPOOL_TYPE_LEGACY) ||
                 (aType == IDU_MEMPOOL_TYPE_TIGHT) );

#ifdef MEMORY_ASSERT
    mType = IDU_MEMPOOL_TYPE_LEGACY;
#else
#if defined(ALTI_CFG_OS_AIX)
    /* BUG-46668 MEMPOOL_TYPE_TIGHT is not working in AIX */
    mType = IDU_MEMPOOL_TYPE_LEGACY;
#else
    mType = aType;
#endif
#endif

    // mFlag setting. ////////////////////////////////////
    mFlag = 0;

    //aUseMutex
    if( aUseMutex == ID_TRUE )
    {
        mFlag |=  IDU_MEMPOOL_USE_MUTEX;
    } 

    //aForcePooling
    if (IDU_USE_MEMORY_POOL == 1)
    {
        mFlag |=  IDU_MEMPOOL_USE_POOLING;
    }
    else
    {
        if (aForcePooling == ID_TRUE)
        {
            mFlag |=  IDU_MEMPOOL_USE_POOLING;
        }
        else
        {
            mFlag &=  ~IDU_MEMPOOL_USE_POOLING;
        }
    }

    //aGarbageCollection
    if( aGarbageCollection == ID_TRUE )
    {
        mFlag |=  IDU_MEMPOOL_GARBAGE_COLLECT;
    } 

    //aHWCacheLine
    if( aHWCacheLine == ID_TRUE )
    {
        mFlag |=  IDU_MEMPOOL_CACHE_LINE_IDU_ALIGN;
    } 
    ///////////////////////////////////////////////////// 


    mAutoFreeChunkLimit = aChunkLimit;

#if defined(ALTIBASE_MEMORY_CHECK)
    mAlignByte = aAlignByte;
    mElemSize  = aElemSize;
#else
    if( (mFlag & IDU_MEMPOOL_CACHE_LINE_IDU_ALIGN) &&
        (aElemSize <= IDU_MEMPOOL_MIN_SIZE_FOR_HW_CACHE_ALIGN) )
    {
        mAlignByte = IDL_CACHE_LINE;
    }
    else
    {
        mAlignByte =  aAlignByte;
    }

    mElemSize   =  idlOS::align8( aElemSize );
#endif

#ifdef MEMORY_ASSERT
    mSlotSize = IDU_ALIGN( mElemSize +
                           ID_SIZEOF(gFence)       +
                           ID_SIZEOF(iduMemSlot *) +
                           ID_SIZEOF(gFence));

    mElemCnt    = ( aElemCount < sMinSlotCnt ) ? sMinSlotCnt  : aElemCount;
    mChunkSize  = IDL_CACHE_LINE + CHUNK_HDR_SIZE + ( mSlotSize* mElemCnt );
#else

    if( mType == IDU_MEMPOOL_TYPE_TIGHT )
    {
        //these values must be given well by the caller!
        IDE_ASSERT( aElemSize >= 8 );
        IDE_ASSERT( checkSize4Tight(aElemSize) == ID_TRUE );
        IDE_ASSERT( checkSize4Tight(aElemCount) == ID_TRUE );
        IDE_ASSERT( aElemSize == aAlignByte );

        mElemSize   = aElemSize;  
        mSlotSize   = mElemSize;

        mAlignByte =  aAlignByte;

        //element count must be more than or equal to 2.
        if( aElemCount >= 2 )
        {
            mElemCnt = aElemCount;
        }
        else
        {
            mElemCnt = 2;
        }
        mChunkSize  = mElemSize*mElemCnt; //header size is not considered..
    }
    else if( mType == IDU_MEMPOOL_TYPE_LEGACY )
    {
        mSlotSize   = IDU_ALIGN(mElemSize + ID_SIZEOF(iduMemSlot *));
        mElemCnt    = ( aElemCount < sMinSlotCnt ) ? sMinSlotCnt  : aElemCount;
        mChunkSize  = IDL_CACHE_LINE + CHUNK_HDR_SIZE + ( mSlotSize* mElemCnt );
    }
#endif

    if (aName == NULL)
    {
        aName = (SChar *)"noname";
    }

    idlOS::snprintf( mName, 
                     IDU_MEM_POOL_NAME_LEN, 
                     "%s",  
                     aName );

    mListCount   = aListCount;
    mIndex       = aIndex;

    mNext = NULL;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        IDE_TEST(iduMemMgr::calloc(mIndex,
                                   mListCount,
                                   ID_SIZEOF(iduMemList *),
                                   (void**)&mMemList)
                 != IDE_SUCCESS);

        for (i = 0; i < mListCount; i++)
        {
            mMemList[i] = NULL;

            IDE_TEST(iduMemMgr::malloc(mIndex,
                                       ID_SIZEOF(iduMemList),
                                       (void**)&(mMemList[i]))
                     != IDE_SUCCESS);

#ifdef __CSURF__
            IDE_ASSERT( mMemList[i] != NULL );
            
            new (mMemList[i]) iduMemList();
#else
            mMemList[i] = new (mMemList[i]) iduMemList();
#endif
            IDU_FIT_POINT("iduMemPool::initialize::mMemList::initialize");
            IDE_TEST(  mMemList[i]->initialize( i, this) != IDE_SUCCESS);
        }
    }
    else
    {
        /* pass through */
    }

    if( IDU_ADD_OR_DEL_MEMPOOL == ID_TRUE )
    {
       IDE_TEST( iduMemPoolMgr::addMemPool(this)  !=  IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if( mFlag & IDU_MEMPOOL_USE_POOLING )
        {
            if ( mMemList != NULL )
            {
                for (j = 0; j < mListCount; j++)
                {
                    if (mMemList[j] != NULL)
                    {
                        if ( j < i )
                        {
                            IDE_ASSERT(mMemList[j]->destroy() == IDE_SUCCESS);
                        }
    
                        IDE_ASSERT(iduMemMgr::free(mMemList[j]) == IDE_SUCCESS);
    
                        mMemList[j] = NULL;
                    }
                }
                
                IDE_ASSERT( iduMemMgr::free(mMemList) == IDE_SUCCESS );
                mMemList = NULL;
            }
        }
        else
        {
            /* fall through */
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduMemPool::destroy(idBool aCheck)
{
    UInt    i;

#if defined(ALTIBASE_PRODUCT_XDB)
    if( iduGetShmProcType() == IDU_PROC_TYPE_USER )
    {
        aCheck = ID_FALSE;
    }
#endif

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        if( IDU_ADD_OR_DEL_MEMPOOL == ID_TRUE )
        {
           IDE_TEST( iduMemPoolMgr::delMemPool(this)  !=  IDE_SUCCESS);
        }

        for (i = 0; i < mListCount; i++)
        {
            IDE_TEST( mMemList[i]->destroy(aCheck)!= IDE_SUCCESS);

            IDE_TEST(iduMemMgr::free(mMemList[i])
                     != IDE_SUCCESS);
        }

        IDE_TEST(iduMemMgr::free(mMemList)
                 != IDE_SUCCESS);
    }
    else
    {
        /* fall through */
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC iduMemPool::cralloc(void **aMem)
{
    SInt i = 0;
    SInt sState = 0;
    
    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        i  = (idlOS::getParallelIndex() % mListCount);

        if( mFlag &  IDU_MEMPOOL_USE_MUTEX )
        {
            IDE_TEST(mMemList[i]->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);
            sState = 1;
    
            IDE_TEST(mMemList[i]->cralloc(aMem) != IDE_SUCCESS);

            sState = 0;
            IDE_TEST(mMemList[i]->mMutex.unlock() != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(mMemList[i]->cralloc(aMem) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(iduMemMgr::calloc(mIndex, 1, mElemSize, aMem) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        if(sState != 0)
        {
            (void)mMemList[i]->mMutex.unlock();
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduMemPool::alloc(void **aMem)
{
    SInt i = 0;
    SInt sState = 0;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        i  = (idlOS::getParallelIndex() % mListCount);

        if( mFlag &  IDU_MEMPOOL_USE_MUTEX )
        {
            IDE_TEST(mMemList[i]->mMutex.lock(NULL /* idvSQL* */) != IDE_SUCCESS);
            sState = 1;

            IDE_TEST(mMemList[i]->alloc(aMem) != IDE_SUCCESS);

            sState = 0;
            IDE_TEST(mMemList[i]->mMutex.unlock() != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(mMemList[i]->alloc(aMem) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(iduMemMgr::malloc(mIndex, mElemSize, aMem) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        if(sState != 0)
        {
            (void)mMemList[i]->mMutex.unlock();
        }
    }

    return IDE_FAILURE;
}

IDE_RC iduMemPool::memfree(void *aMem)
{
    iduMemList  *sCurList  = NULL;
    iduMemChunk *sCurChunk = NULL;
    SInt         sState    = 0;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
#if defined(ALTIBASE_MEMORY_CHECK)

        sCurList = mMemList[0];

#else

        // Memory Check Option에서는 이 과정을 생략해야 함.
# ifdef MEMORY_ASSERT
        sCurChunk         = *((iduMemChunk **)((UChar *)aMem + mElemSize +
                                           ID_SIZEOF(vULong)));
# else
    if( mType == IDU_MEMPOOL_TYPE_TIGHT )
    {
        sCurChunk = (iduMemChunk *)( ( (ULong)aMem  & (ULong)( ~(mChunkSize - 1 ))) 
                                     + mChunkSize ); 
    }
    else if( mType == IDU_MEMPOOL_TYPE_LEGACY )
    {
        sCurChunk = *((iduMemChunk **)((UChar *)aMem + mElemSize));
    }
# endif

        sCurList = (iduMemList*)sCurChunk->mParent;

#endif

        if( mFlag &  IDU_MEMPOOL_USE_MUTEX )
        {
            IDE_TEST(sCurList->mMutex.lock(NULL) != IDE_SUCCESS);
            sState = 1;

            IDE_TEST(sCurList->memfree(aMem) != IDE_SUCCESS);

            sState = 0;
            IDE_TEST(sCurList->mMutex.unlock() != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(sCurList->memfree(aMem) != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST(iduMemMgr::free(aMem) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        if(sState != 0)
        {
            (void)sCurList->mMutex.unlock();
        }
    }

    return IDE_FAILURE;
}

/*
 * PROJ-2065 Garbage collection 
 *
 * 사용하지 않는 free chunk들을 OS에 반납하여 메모리공간을 확보한다.
 */
IDE_RC iduMemPool::shrink(UInt *aSize )
{
    UInt         i;
    UInt         sState=0;
    UInt         sSize=0;
    iduMemList  *sCurList=NULL;

    *aSize=0; 

    if ( (mFlag & IDU_MEMPOOL_USE_POOLING) &&
         (mFlag & IDU_MEMPOOL_GARBAGE_COLLECT ) &&
         (iduMemMgr::isServerMemType() == ID_TRUE) )
    {
        for (i = 0; i < mListCount; i++)
        {
            sCurList = mMemList[i];

            IDE_ASSERT( sCurList != NULL ); 

            if( mFlag &  IDU_MEMPOOL_USE_MUTEX )
            {
                IDE_TEST( sCurList->mMutex.lock(NULL) != IDE_SUCCESS );
                sState = 1;
            }

            IDE_TEST( sCurList->shrink( &sSize ) != IDE_SUCCESS );

            *aSize += sSize;

            if( mFlag &  IDU_MEMPOOL_USE_MUTEX )
            {
                sState = 0;
                IDE_TEST( sCurList->mMutex.unlock() != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)sCurList->mMutex.unlock();
    }

    return IDE_FAILURE;
}

void iduMemPool::dumpState(SChar *aBuffer, UInt aBufferSize)
{
    UInt sSize = 0;
    UInt i;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        for (i = 0; i < mListCount; i++)
        {
            IDE_ASSERT(mMemList[i]->mMutex.lock(NULL) == IDE_SUCCESS);

            sSize += mMemList[i]->getUsedMemory();

            IDE_ASSERT(mMemList[i]->mMutex.unlock() == IDE_SUCCESS);
        }

        idlOS::snprintf(aBuffer, aBufferSize, "MemPool Used Memory:%"ID_UINT32_FMT"\n",sSize);
    }
}

void iduMemPool::status()
{
    UInt i;
    SChar sBuffer[IDE_BUFFER_SIZE];

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    { 
        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer), "  - Memory List Count:%5"ID_UINT32_FMT"\n",
                    (UInt)mListCount);
        IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);

        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer), "  - Slot   Size:%5"ID_UINT32_FMT"\n",
                        (UInt)( mSlotSize));
        IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);

        for(i = 0; i < mListCount; i++)
        {
            idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer), "  - Memory List *** [%"ID_UINT32_FMT"]\n",
                            (UInt)(i + 1));
            IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);

            mMemList[i]->status();
        }
    }
}

/* added by mskim for check memory statement */
ULong iduMemPool::getMemorySize()
{
    UInt i;
    UInt sMemSize = 0;

    if( mFlag & IDU_MEMPOOL_USE_POOLING )
    {
        for(i = 0; i < mListCount; i++)
        {
            sMemSize += mMemList[i]->getUsedMemory();
        }
    }

    return sMemSize;
}


//check if the size is 2^n.
//well... UInt is big enough to contain element size or element count.
idBool iduMemPool::checkSize4Tight(UInt  aSize)
{
        UInt sMask = 0x80000000; 
        UInt sCnt=0;
        while( sMask > 0 )
        {
            if( aSize & sMask )
            {
                sCnt++;
            }
            sMask >>= 1;
        }
        return  (idBool)(sCnt == 1);
}
