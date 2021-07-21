/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduHash.cpp 35244 2009-09-02 23:52:47Z orc $
 **********************************************************************/

#include <idl.h>
#include <iduHash.h>

#if defined(SMALL_FOOTPRINT)
#define IDU_HASH_MAX_FREE_BUCKET_COUNT (1)
#define IDU_HASH_MAX_HASH_TABLE_SIZE   (8)
#endif

IDE_RC iduHash::initialize(iduMemoryClientIndex aIndex,
                           UInt                 aInitFreeBucketCount,
                           UInt                 aHashTableSize)
{
#define IDE_FN "iduHash::initialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    iduHashBucket *sNewBucket;

#if defined(SMALL_FOOTPRINT)
    if( aInitFreeBucketCount > IDU_HASH_MAX_FREE_BUCKET_COUNT )
    {
        aInitFreeBucketCount = IDU_HASH_MAX_FREE_BUCKET_COUNT;
    }

    if( aHashTableSize > IDU_HASH_MAX_HASH_TABLE_SIZE )
    {
        aHashTableSize = IDU_HASH_MAX_HASH_TABLE_SIZE;
    }
#endif
    
    mIndex = aIndex;
    mHashTableSize = aHashTableSize;
    mFreeBucketCount = aInitFreeBucketCount;
    mCashBucketCount = aInitFreeBucketCount;
    
    IDE_TEST(iduMemMgr::malloc(mIndex,
                               sizeof(iduHashBucket) * aHashTableSize,
                               (void**)&mHashTable)
             != IDE_SUCCESS);

    for(i = 0; i < mHashTableSize; i++)
    {
        (mHashTable + i)->pPrev = mHashTable + i;
        (mHashTable + i)->pNext = mHashTable + i;
        (mHashTable + i)->pData = NULL;
    }

    mFreeBucketList.pPrev = &mFreeBucketList;
    mFreeBucketList.pNext = &mFreeBucketList;
    
    for(i = 0; i < mFreeBucketCount; i++)
    {
        IDE_TEST(iduMemMgr::malloc(mIndex,
                                   sizeof(iduHashBucket),
                                   (void**)&sNewBucket)
                 != IDE_SUCCESS);

        initHashBucket(sNewBucket);
        addHashBucketToFreeList(sNewBucket);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduHash::destroy()
{
#define IDE_FN "iduHash::destroy"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduHashBucket *sCurHashBucket;
    iduHashBucket *sNxtHashBucket;
    
    sCurHashBucket = mFreeBucketList.pNext;

    // Free List에 있는 Hash Bucket을 반환한다.
    while(sCurHashBucket != &mFreeBucketList)
    {
        sNxtHashBucket = sCurHashBucket->pNext;
        
        IDE_TEST(iduMemMgr::free(sCurHashBucket) != IDE_SUCCESS);

        sCurHashBucket = sNxtHashBucket;
    }

    // Hash Table을 삭제
    IDE_TEST(iduMemMgr::free(mHashTable) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduHash::insert(vULong aKey, void* aData)
{
#define IDE_FN "iduHash::insert"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduHashBucket *sNewHashBucket;
    vULong         sHashValue;
    iduHashBucket *sBucketList;
	//SChar         *tmp = NULL;
    
    IDE_TEST(allocHashBucket(&sNewHashBucket) != IDE_SUCCESS);

    sNewHashBucket->nKey = aKey;
    sNewHashBucket->pData = aData;

    sHashValue = hash(aKey);
    
    sBucketList = mHashTable + sHashValue;
    
    addHashBucketToList(sBucketList, sNewHashBucket);
    //idlOS::strcpy(tmp, NULL);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

void* iduHash::search(vULong aKey, iduHashBucket **aHashBucket)
{
#define IDE_FN "iduHash::search"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    vULong          sHashValue;
    iduHashBucket  *sBucketList;
    iduHashBucket  *sCurHashBucket;
    void           *sData = NULL;
    
    sHashValue = hash(aKey);

    sBucketList = mHashTable + sHashValue;

    sCurHashBucket = sBucketList->pNext;
    
    while(sCurHashBucket != sBucketList)
    {
        if(sCurHashBucket->nKey > aKey)
        {
            break;
        }

        if(sCurHashBucket->nKey == aKey)
        {
            sData = sCurHashBucket->pData;
            
            break;
        }

        sCurHashBucket = sCurHashBucket->pNext;
    }

    if(aHashBucket != NULL)
    {
        *aHashBucket = (sData == NULL) ? NULL : sCurHashBucket;
    }
    
    return sData;
    
#undef IDE_FN
}

/*
    모든 (Hash Key, Data)를 순회한다
    [IN] aVisitor   - Hash Key의 Visitor Function
    [IN] VisitorArg - Visitor Function에 넘길 인자

    <참고사항>
     traverse도중에 hash에서 해당 bucket을 제거할 수도 있도록 설계되었다
 */
IDE_RC iduHash::traverse( iduHashVisitor    aVisitor,
                          void            * aVisitorArg )
{
    UInt            i;
    iduHashBucket  *sBucketList;
    iduHashBucket  *sCurHashBucket;
    iduHashBucket  *sNextHashBucket;
    
    for(i = 0; i < mHashTableSize; i++)
    {
        sBucketList = (mHashTable + i);

        sCurHashBucket = sBucketList->pNext;
    
        while(sCurHashBucket != sBucketList)
        {
            // Visitor가 현재 Bucket을 제거할 경우에 대비하여
            // 다음 Bucket을 미리 얻어놓는다.
            sNextHashBucket = sCurHashBucket->pNext;
            
            IDE_TEST( (*aVisitor)(sCurHashBucket->nKey,
                                  sCurHashBucket->pData,
                                  aVisitorArg )
                      != IDE_SUCCESS );

            sCurHashBucket = sNextHashBucket ;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC iduHash::remove(vULong aKey)
{
#define IDE_FN "iduHash::remove"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduHashBucket* sHashBucket;

    search(aKey, &sHashBucket);

    assert(sHashBucket != NULL);
    
    removeHashBuckeFromList(sHashBucket);

    IDE_TEST(freeHashBucket(sHashBucket) != IDE_SUCCESS);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduHash::allocHashBucket(iduHashBucket **aHashBucket)
{
#define IDE_FN "iduHash::allocHashBucket"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduHashBucket *sFreeHashBucket;
    
    if(mFreeBucketCount != 0)
    {
        assert(mFreeBucketList.pNext != &mFreeBucketList);
        
        sFreeHashBucket = mFreeBucketList.pNext;
        removeHashBucketFromFreeList(sFreeHashBucket);
        mFreeBucketCount--;
    }
    else
    {
        IDE_TEST(iduMemMgr::malloc(mIndex,
                                   sizeof(iduHashBucket),
                                   (void**)&sFreeHashBucket)
                 != IDE_SUCCESS);
    }

    initHashBucket(sFreeHashBucket);
    
    *aHashBucket = sFreeHashBucket;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduHash::freeHashBucket(iduHashBucket *aHashBucket)
{
#define IDE_FN "iduHash::freeHashBucket"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if(mFreeBucketCount >= mCashBucketCount)
    {
        IDE_TEST(iduMemMgr::free(aHashBucket) != IDE_SUCCESS);
    }
    else
    {
        mFreeBucketCount++;
        addHashBucketToFreeList(aHashBucket);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}





    


