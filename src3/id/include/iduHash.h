/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduHash.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_IDU_HASH_H_
#define _O_IDU_HASH_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduMutex.h>

typedef IDE_RC (*iduHashVisitor)( vULong   aKey,
                                  void   * aData,
                                  void   * aVisitorArg);

typedef struct iduHashBucket
{
    iduHashBucket *pPrev;
    iduHashBucket *pNext;
    vULong         nKey;
    void*          pData;
} iduHashBucket;

class iduHash
{
public:
    IDE_RC          initialize(iduMemoryClientIndex aIndex,
                               UInt                 aInitFreeBucketCount,
                               UInt                 aHashTableSize);
    
    IDE_RC          destroy();

    IDE_RC          insert(vULong aKey, void* aData);
    void*           search(vULong aKey, iduHashBucket** aHashBucket = NULL);
    IDE_RC          remove(vULong aKey);


    // 모든 (Hash Key, Data)를 순회한다
    IDE_RC          traverse( iduHashVisitor    aVisitor,
                              void            * aVisitorArg );
private:
    IDE_RC          allocHashBucket(iduHashBucket **aHashBucket);
    IDE_RC          freeHashBucket(iduHashBucket *aHashBucket);
    
    inline UInt     hash(vULong aKey);
    inline void     addHashBucketToFreeList(iduHashBucket *aBucket);
    inline void     removeHashBucketFromFreeList(iduHashBucket *aBucket);
    inline void     addHashBucketToList(iduHashBucket *aHeader,
                                        iduHashBucket *aBucket);
    
    inline void     removeHashBuckeFromList(iduHashBucket *aBucket);
    
    inline void     initHashBucket(iduHashBucket *aBucket);
    
public: /* POD class type should make non-static data members as public */
    UInt                  mHashTableSize;
    UInt                  mCashBucketCount;
    UInt                  mFreeBucketCount;
    
    iduHashBucket         mFreeBucketList;
    iduHashBucket        *mHashTable;
    iduMemoryClientIndex  mIndex;
};

inline void iduHash::addHashBucketToFreeList(iduHashBucket *aBucket)
{
    aBucket->pPrev = &mFreeBucketList;
    aBucket->pNext = mFreeBucketList.pNext;
    
    mFreeBucketList.pNext->pPrev = aBucket;
    mFreeBucketList.pNext = aBucket;
}

inline void iduHash::removeHashBucketFromFreeList(iduHashBucket *aBucket)
{
    aBucket->pPrev->pNext = aBucket->pNext;
    aBucket->pNext->pPrev = aBucket->pPrev;
    aBucket->pPrev = aBucket;
    aBucket->pNext = aBucket;
}

inline void iduHash::addHashBucketToList(iduHashBucket *aHeader,
                                         iduHashBucket *aBucket)
{
    iduHashBucket *sCurHashBucket;
    iduHashBucket *sPrvHashBucket;

    sPrvHashBucket = aHeader;
    sCurHashBucket = aHeader->pNext;

    while((sCurHashBucket != aHeader) && 
          (sCurHashBucket->nKey < aBucket->nKey)) 
    {
        sPrvHashBucket = sCurHashBucket;
        sCurHashBucket = sCurHashBucket->pNext;
    }

    sCurHashBucket = sPrvHashBucket;

    aBucket->pNext = sCurHashBucket->pNext;
    aBucket->pPrev = sCurHashBucket;

    sCurHashBucket->pNext->pPrev = aBucket;
    sCurHashBucket->pNext = aBucket;
}

inline void iduHash::removeHashBuckeFromList(iduHashBucket *aBucket)
{
    aBucket->pPrev->pNext = aBucket->pNext;
    aBucket->pNext->pPrev = aBucket->pPrev;
    aBucket->pPrev = aBucket;
    aBucket->pNext = aBucket;
}

inline void iduHash::initHashBucket(iduHashBucket *aBucket)
{
    aBucket->pPrev = aBucket;
    aBucket->pNext = aBucket;
    aBucket->nKey  = ID_UINT_MAX;
    aBucket->pData = NULL;
}

inline UInt iduHash::hash(vULong aKey)
{
    return aKey % mHashTableSize;
}

#endif  // _O_IDU_HASH_H_

