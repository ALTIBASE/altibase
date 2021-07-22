/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id: iduStringHash.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_IDU_STRING_HASH_H_
#define _O_IDU_STRING_HASH_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduMutex.h>

typedef IDE_RC ( *iduStringHashVisitor )( SChar  * aString,
                                          void   * aData,
                                          void   * aVisitorArg );

typedef struct iduStringHashBucket
{
    iduStringHashBucket *pPrev;
    iduStringHashBucket *pNext;
    vULong               nKey;
    SChar               *nString;
    void*                pData;
} iduStringHashBucket;

class iduStringHash
{
public:
    IDE_RC       initialize( iduMemoryClientIndex aIndex,
                             UInt                 aInitFreeBucketCount,
                             UInt                 aHashTableSize );
    
    IDE_RC       destroy( );

    IDE_RC       insert( SChar * aString, void* aData );
    void*        search( SChar * aString, 
                         iduStringHashBucket** aHashBucket = NULL );
    IDE_RC       remove( SChar * aString );


    // 모든 ( Hash Key, Data )를 순회한다
    IDE_RC       traverse( iduStringHashVisitor    aVisitor,
                           void                  * aVisitorArg );
private:
    IDE_RC       allocHashBucket( iduStringHashBucket **aHashBucket );
    IDE_RC       freeHashBucket ( iduStringHashBucket  *aHashBucket );
    
    inline UInt  hash                         ( vULong aKey );
    inline void  addHashBucketToFreeList      ( iduStringHashBucket *aBucket );
    inline void  removeHashBucketFromFreeList ( iduStringHashBucket *aBucket );
    inline void  addHashBucketToList          ( iduStringHashBucket *aHeader,
                                                iduStringHashBucket *aBucket );
    inline void  removeHashBuckeFromList      ( iduStringHashBucket *aBucket );
    inline void  initHashBucket               ( iduStringHashBucket *aBucket );
 
public: /* POD class type should make non-static data members as public */
    UInt                  mHashTableSize;
    UInt                  mCashBucketCount;
    UInt                  mFreeBucketCount;
    
    iduStringHashBucket   mFreeBucketList;
    iduStringHashBucket  *mHashTable;
    iduMemoryClientIndex  mIndex;
};

inline void iduStringHash::addHashBucketToFreeList( iduStringHashBucket *aBucket )
{
    aBucket->pPrev = &mFreeBucketList;
    aBucket->pNext = mFreeBucketList.pNext;
    
    mFreeBucketList.pNext->pPrev = aBucket;
    mFreeBucketList.pNext = aBucket;
}

inline void iduStringHash::removeHashBucketFromFreeList( iduStringHashBucket *aBucket )
{
    aBucket->pPrev->pNext = aBucket->pNext;
    aBucket->pNext->pPrev = aBucket->pPrev;
    aBucket->pPrev = aBucket;
    aBucket->pNext = aBucket;
}

inline void iduStringHash::addHashBucketToList( iduStringHashBucket *aHeader,
                                                iduStringHashBucket *aBucket )
{
    iduStringHashBucket *sCurHashBucket;
    iduStringHashBucket *sPrvHashBucket;

    sPrvHashBucket = aHeader;
    sCurHashBucket = aHeader->pNext;

    while( ( sCurHashBucket != aHeader ) && 
           ( sCurHashBucket->nKey < aBucket->nKey ) ) 
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

inline void iduStringHash::removeHashBuckeFromList( iduStringHashBucket *aBucket )
{
    aBucket->pPrev->pNext = aBucket->pNext;
    aBucket->pNext->pPrev = aBucket->pPrev;
    aBucket->pPrev = aBucket;
    aBucket->pNext = aBucket;
}

inline void iduStringHash::initHashBucket( iduStringHashBucket *aBucket )
{
    aBucket->pPrev   = aBucket;
    aBucket->pNext   = aBucket;
    aBucket->nKey    = ID_UINT_MAX;
    aBucket->nString = NULL;
    aBucket->pData   = NULL;
}

inline UInt iduStringHash::hash( vULong aKey )
{
    return aKey % mHashTableSize;
}

#endif  // _O_IDU_STRING_HASH_H_

