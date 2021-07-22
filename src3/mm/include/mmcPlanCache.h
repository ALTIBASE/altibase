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

#ifndef _O_MMC_PLAN_CACHE_H_
#define _O_MMC_PLAN_CACHE_H_  1

#include <iduFixedTableDef.h>
#include <iduMemPool.h>
#include <mmuProperty.h>
#include <mmcSession.h>
#include <mmm.h>
#include <mmcDef.h>
#include <qci.h>

class mmcStatement;
class mmcParentPCO;
class mmcChildPCO;
class mmcPCB;

typedef struct mmcPlanCacheSystemInfo
{
    ULong       mCacheHitCnt;
    ULong       mCacheMissCnt;
    ULong       mCacheInFailCnt;
    ULong       mCacheOutCnt;
    ULong       mCacheInsertedCnt;
    UInt        mCurrentCacheObjCnt;
    ULong       mNoneCacheSQLTryCnt;
    ULong       mCurrentCacheSize;
    ULong       mMaxCacheSize;
    ULong       mHotLruSize;
    ULong       mColdLruSize;
}mmcPlanCacheSystemInfo;

class mmcPlanCache
{
public:
    enum { CACHE_KEYWORD_MAX_LEN=15};
    
    static IDE_RC initialize();
    
    static IDE_RC finalize();
    static IDE_RC freePCB(mmcPCB* aPCB);
    static IDE_RC freePCO(mmcParentPCO* aPCO);
    static IDE_RC freePCO(mmcChildPCO* aPCO);
    static inline idBool isEnable( mmcSession   * aSession,
                                   qciStatement * aQciStmt );

    static void  searchSQLText(mmcStatement  *aStatement,
                               mmcParentPCO  **aFoundedParentPCO,
                               vULong        *aHashKeyVal,
                               mmcPCOLatchState *aPCOLatchState);
    
    static IDE_RC  tryInsertSQLText(idvSQL           *aStatistics,
                                    SChar            *aSQLText,
                                    UInt             aSQLTextLen,
                                    vULong           aHaskKeyVal,
                                    mmcParentPCO     **aFoundedParentPCO,
                                    mmcPCB           **aPCB,
                                    idBool           *aSuccess,
                                    mmcPCOLatchState *aPCOLatchState);
    
    static idBool  isCacheAbleSQLText(SChar        *aSQLText);
    
    static IDE_RC    compact(void     *aStatement);
    static IDE_RC    reset(void     *aStatement);
    

    //parent PCO는 plan cache에 있지만
    //environment가 맞는 child PCO가 없는 경우에
    //child PCO의 중복생성을 막기위한 함수.
    static IDE_RC preventDupPlan(idvSQL* aStatistics,
                                 mmcParentPCO          *aParentPCO,
                                 mmcPCB                *aSafeGuardPCB,
                                 mmcPCB                **aNewPCB,
                                 mmcPCOLatchState *aPCOLatchState);
    //environement가 맞는 child PCO가 있지만,
    //plan이 valid하지 않아,새로운 child PCO의
    //중복생성을 막기위한 함수.
    static IDE_RC preventDupPlan(idvSQL                 *aStatistics,
                                 mmcStatement           *aStatement,
                                 mmcParentPCO           *aParentPCO,
                                 mmcPCB                 *aUnUsedPCB,
                                 mmcPCB                 **aNewPCB,
                                 mmcPCOLatchState       *aPCOLatchState,
                                 mmcChildPCOCR           aRecompileReason,
                                 UInt                    aPrevChildCreateCnt);
    
    static void checkIn(idvSQL                  *aStatSQL,
                        mmcPCB                  *aPCB,
                        qciSQLPlanCacheContext  *aPlanCacheContext,
                        idBool                  *aSuccess);
    
    static void register4GC(idvSQL                  *aStatSQL,
                            mmcPCB                  *aPCB);
    
    
    static void tryFreeParentPCO(idvSQL       *aStatistics,
                                 mmcParentPCO *aParentPCO);

    /*fix BUG-31245	The size of hot or cold LRU region can be incorrect
      when old PCO is freed directly while statement perform rebuilding
      a plan for old PCO.*/
    static void updateCacheSize(idvSQL                 *aStatSQL,
                                UInt                    aChildPCOSize,
                                mmcPCB                 *aUnUsedPCB);
    
    static  void latchShared4PerfV(idvSQL  *aStatSQL);
    static  void releaseLatchShared4PerfV();
    static  inline ULong getCurrentCacheSize();

    static void incCacheMissCnt(idvSQL  *aStatSQL);
    static void incCacheInFailCnt(idvSQL  *aStatSQL);
    static void incNoneCacheSQLTryCnt(idvSQL  *aStatSQL);

    static void incCacheSize(ULong    aAddedSize);

    static void updateCacheOutCntAndSize(idvSQL  *aStatSQL,
                                         UInt     aCacheOutCnt,
                                         ULong    aReducedSize);

    static void  incCacheInsertCnt(idvSQL  *aStatSQL);
    
    static IDE_RC buildRecordForSQLPlanCache(idvSQL              * /*aStatistics*/,
                                             void                 *aHeader,
                                             void                 *aDummyObj,
                                             iduFixedTableMemory  *aMemory);

    // PROJ-2163
    static IDE_RC movePCBOfChildToUnUsedLst(idvSQL *aStatistics,
                                            mmcPCB *aUnUsedPCB);

    /* BUG-46158 */
    static IDE_RC planCacheKeep(void *aMmStatement, SChar *aSQLTextID, idBool aIsKeep);

private:
    /* fix BUG-31232  Reducing x-latch duration of parent PCO
       while perform soft prepare .
     */
    static IDE_RC allocPlanCacheObjs(idvSQL          *aStatistics,
                                     mmcParentPCO    *aParentPCO,
                                     mmcChildPCOCR   aRecompileReason,
                                     const UInt      aPcoId,
                                     const UInt      aRebuildCount,
                                     qciPlanProperty *aEnv,
                                     mmcStatement    *aStatement,
                                     mmcPCB          **aNewPCB,
                                     mmcChildPCO     **aNewChildPCO,
                                     UChar           *aState);
    
    static void  freePlanCacheObjs(mmcPCB       *aNewPCB,
                                   mmcChildPCO  *aNewChildPCO,
                                   UChar        *aState);
    
    //performance view
    static mmcPlanCacheSystemInfo    mPlanCacheSystemInfo;
    static iduMemPool                mPCBMemPool;
    static iduMemPool                mParentPCOMemPool;
    static iduMemPool                mChildPCOMemPool;
    // PROJ-2408
    static iduLatch                  mPerfViewLatch;
    static const SChar  *const       mCacheAbleText[];
};

// BUG-23098 plan이 보였다 안보였다 합니다.
inline idBool mmcPlanCache::isEnable( mmcSession   * aSession,
                                      qciStatement * aQciStmt )
{
    /* BUG-36205 Plan Cache On/Off property for PSM */
    if ( qci::isCalledByPSM( aQciStmt ) == ID_TRUE )
    {
        IDE_TEST(mmuProperty::getSqlPlanCacheUseInPSM() != 1);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST(mmm::getCurrentPhase() != MMM_STARTUP_SERVICE);

    IDE_TEST(mmuProperty::getSqlPlanCacheSize() <= 0);

    IDE_TEST(aSession->getExplainPlan() == QCI_EXPLAIN_PLAN_ONLY);

    /* BUG-45899 */
    IDE_TEST( sdi::isAnalysisInfoPrintable( &aQciStmt->statement ) == ID_TRUE );

    return ID_TRUE;

    IDE_EXCEPTION_END;

    return ID_FALSE;
}

inline ULong mmcPlanCache::getCurrentCacheSize()
{
    return mPlanCacheSystemInfo.mCurrentCacheSize;
}


#endif
