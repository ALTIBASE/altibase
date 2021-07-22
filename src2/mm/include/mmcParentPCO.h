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

#ifndef _O_MMC_PARENT_PCO_H_
#define _O_MMC_PARENT_PCO_H_ 1

#include <iduLatch.h>
#include <iduList.h>
#include <mmcChildPCO.h>
class mmcPCB;
class mmcStatement;


class mmcParentPCO
{
public:
    
    
    IDE_RC   initialize(UInt     aSQLTextIdInBucket,
                        SChar   *aSQLString,
                        UInt     aSQLStringLen,
                        vULong   aHashKeyVal,
                        UInt     aBucket);    
    void   finalize();
    
    void   addPCBOfChild(mmcPCB* aPCB);
    IDE_RC searchChildPCO(mmcStatement          *aStatement,
                          mmcPCB               **aPCBofChildPCO,
                          mmcChildPCOPlanState *aChildPCOPlanState,
                          mmcPCOLatchState      *aPCOLatchState);
    
    
    
    inline void   incChildCreateCnt();
    inline void   getChildCreateCnt(UInt *aChildCreateCnt);
    inline UInt   getChildCnt();
    
    void   movePCBOfChildToUnUsedLst(mmcPCB* aPCBOfChild);
    
    void   deletePCBOfChild(mmcPCB* aPCBOfChild);
    
    void   latchPrepareAsShared(idvSQL* aStatistics);
    
    void   latchPrepareAsExclusive(idvSQL* aStatistics);

    /* BUG-35521 Add TryLatch in PlanCache. */
    void   tryLatchPrepareAsExclusive(idBool *aSuccess);

    void   releasePrepareLatch();

    mmcPCB* getSafeGuardPCB();

    inline SChar* getSQLString4HardPrepare();
    
    inline vULong getHashKeyValue();
    inline UInt   getBucket();
    inline iduListNode*  getBucketChainNode();
    /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
      by moving free parent PCO to the out side of critical section */
    inline iduListNode*  getVictimNode();
    inline iduList*      getUsedChildList();
    inline iduList*      getUnUsedChildList();
    
    inline idBool isSameSQLText(SChar* aSQLText,
                                UInt   aSQLTextLen);

    inline void setPlanCacheKeep(mmcPCOPlanCacheKeep aPlanCacheKeep);  /* BUG-46158 */
    inline mmcPCOPlanCacheKeep getPlanCacheKeep();

    //for performance view(private -> public)
    SChar        mSQLTextId[MMC_SQL_CACHE_TEXT_ID_LEN + 1];
    SChar*       mSQLString4SoftPrepare;
    UInt         mChildCreateCnt;
    UInt         mChildCnt;
private:
    void         validateChildLst(iduList       *aHead,
                                  mmcPCB        *aPCB);
    void         validateChildLst2(iduList       *aHead,
                                   mmcPCB        *aPCB);
    SChar*       mSQLString4HardPrepare;
    UInt         mSQLStringLen;
    UInt         mBucket;
    // PROJ-2408
    iduLatch  mPrepareLatch;
    // String에서 hash함수를 적용해서 나온결과.
    vULong       mHashKeyVal;

    iduList      mUsedChildLst;
    iduList      mUnUsedChildLst;
    iduListNode  mBucketChain;
    /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
      by moving free parent PCO to the out side of critical section */
    iduListNode  mVictimNode;

    mmcPCOPlanCacheKeep mPlanCacheKeep;  /* BUG-46158 PLAN_CACHE_KEEP */
};

inline void   mmcParentPCO::incChildCreateCnt()
{
    mChildCreateCnt++;
}

inline void  mmcParentPCO::getChildCreateCnt(UInt *aChildCreateCnt)
{
    *aChildCreateCnt = mChildCreateCnt ;
    
}

inline UInt  mmcParentPCO::getChildCnt()
{
    return mChildCnt;
}

inline idBool mmcParentPCO::isSameSQLText(SChar* aSQLText,
                                          UInt   aSQLTextLen)
{
    if(  aSQLTextLen != mSQLStringLen)
    {
        return ID_FALSE;
    }
    return( (idlOS::memcmp(aSQLText, mSQLString4SoftPrepare, aSQLTextLen) == 0) ? ID_TRUE: ID_FALSE);
}

inline vULong mmcParentPCO::getHashKeyValue()
{
    return mHashKeyVal;
}
inline UInt  mmcParentPCO::getBucket()
{
    return mBucket;
}

inline iduList*    mmcParentPCO::getUsedChildList()
{
    return  &mUsedChildLst;
}

inline iduList*    mmcParentPCO::getUnUsedChildList()
{
    return &mUnUsedChildLst;
}

iduListNode* mmcParentPCO::getBucketChainNode()
{
    return &mBucketChain;
}

/*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
  by moving free parent PCO to the out side of critical section */
iduListNode* mmcParentPCO::getVictimNode()
{
    return &mVictimNode;
}

SChar * mmcParentPCO::getSQLString4HardPrepare()
{
    return mSQLString4HardPrepare;
}

inline void mmcParentPCO::setPlanCacheKeep(mmcPCOPlanCacheKeep aPlanCacheKeep)
{
    mPlanCacheKeep = aPlanCacheKeep;
}
inline mmcPCOPlanCacheKeep mmcParentPCO::getPlanCacheKeep()
{
    return mPlanCacheKeep;
}

#endif
