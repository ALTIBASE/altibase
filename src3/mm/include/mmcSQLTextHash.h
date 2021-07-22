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

#ifndef _O_MMC_SQLTEXT_HASH_H_
#define _O_MMC_SQLTEXT_HASH_H_ 1

#include <iduLatch.h>
#include <iduList.h>
#include <mmcParentPCO.h>
typedef struct mmcSQLTextHashBucket
{
    vULong      mMaxHashKeyVal;
    UInt        mNextParentPCOId;
    iduList     mChain;
    // PROJ-2408
    iduLatch mBucketLatch;
}mmcSQLTextHashBucket;

class mmcSQLTextHash
{
public:
    static IDE_RC initialize(const UInt aBucketCnt);
    static IDE_RC finalize();
    static inline UInt hash(vULong aKeyValue);
    
    static void  searchParentPCO(idvSQL          *aStatistics,
                                 const UInt      aBucket,
                                 SChar           *aSQLText,
                                 vULong           aHashKeyValue,
                                 UInt             aSQLTextLen,
                                 mmcParentPCO     **aFoundedParentPCO,
                                 iduListNode      **aInsertAfterNode,
                                 mmcPCOLatchState *aPCOLatchState);
    
    
    static void  latchBucketAsShared(idvSQL* aStatistics,
                                      const UInt aBucket);
    
    static void  latchBucketAsExeclusive(idvSQL* aStatistics,
                                          const UInt aBucket);
    static void  releaseBucketLatch(const UInt aBucket);
    static void  tryUpdateBucketMaxHashVal(UInt        aBucket,
                                           iduListNode *aNode,
                                           vULong      aHashKeyVal);
    
    static void  tryUpdateBucketMaxHashVal(UInt        aBucket,
                                           iduListNode *aNode);
    
    static IDE_RC buildRecordForSQLPlanCacheSQLText(idvSQL              * /*aStatistics*/,
                                                    void                 *aHeader,
                                                    void                 */*aDummyObj*/,
                                                    iduFixedTableMemory  *aMemory);
    
    static IDE_RC buildRecordForSQLPlanCachePCO(idvSQL              * /*aStatistics*/,
                                                void                 *aHeader,
                                                void                 */*aDummyObj*/,
                                                iduFixedTableMemory  *aMemory);
    static void  getCacheHitCount(ULong *aHitCount);
    
    static inline UInt   getSQLTextId(UInt aBucket);
    static inline void   incSQLTextId(UInt aBucket);

    static IDE_RC planCacheKeep(idvSQL              *aStatistics,  /* BUG-46158 */
                                SChar               *aSQLTextID,
                                mmcPCOPlanCacheKeep  aPlanKeep);
                                                    
private:
    static void  initBucket(mmcSQLTextHashBucket* aBucket);
    static void  freeBucketChain(iduList*  aChain);
    static mmcSQLTextHashBucket*  mBucketTable;
    static UInt                   mBucketCnt;
};

inline UInt   mmcSQLTextHash::hash(vULong aKeyValue)
{
    return (aKeyValue % mBucketCnt);
}
inline UInt   mmcSQLTextHash::getSQLTextId(UInt aBucket)
{
    return mBucketTable[aBucket].mNextParentPCOId;
}

inline void   mmcSQLTextHash::incSQLTextId(UInt aBucket)
{
    mBucketTable[aBucket].mNextParentPCOId++;
}

#endif
