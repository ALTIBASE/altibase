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

#ifndef MMC_PLAN_CACHE_LRU_LIST_H_
#define MMC_PLAN_CACHE_LRU_LIST_H_ 1
#include <idvType.h>
#include <iduList.h>
#include <iduMutex.h>
#include <idl.h>
#include <qci.h>
#include <mmcDef.h>
class mmcPCB;


typedef struct mmcLRuList
{
    ULong   mCurMemSize;
    iduList mLRuList;
}mmcLRUList;

class mmcPlanCacheLRUList
{
public:
    
    static void initialize();
    
    static void finalize();
    
    static void checkIn(idvSQL                 *aStatSQL,
                        mmcPCB                 *aPCB,
                        qciSQLPlanCacheContext *aPlanCacheContext,
                        idBool                 *aSuccess);
    
    /* BUG-47813 */
    static void moveToColdRegionTail(idvSQL* aStatSQL,
                                     mmcPCB* aPCB,
                                     mmcPCBLRURegion* aLRURegion);

    /*fix BUG-31245	The size of hot or cold LRU region can be incorrect
      when old PCO is freed directly while statement perform rebuilding a plan for old PCO.*/
    static void updateCacheSize(idvSQL                 *aStatSQL,
                                UInt                    aChildPCOSize,
                                mmcPCB                 *aUnUsedPCB);
    

    static void register4GC(idvSQL    *aStatSQL,
                            mmcPCB    *aPCB);
    
    static void compact(idvSQL  *aStatSQL);
    
    static void compactLRU(idvSQL  *aStatSQL,
                           iduList *aLRUList,
                           UInt    *aCacheOutCnt,
                           ULong   *VictimsSize);
    //for  performance view 
    static mmcLRuList mColdRegionList;
    static mmcLRuList mHotRegionList;
private:
    // for debug.
    static void validateLRULst(iduList       *aHead,
                               iduListNode   *aNode);
    
    static void replace(idvSQL  *aStatSQL,
                        ULong   aCurCacheMaxSize,
                        ULong   aReplaceSize,
                        iduList *aVictimLst,
                        iduList *aVictimParentPCOLst,
                        idBool  *aSuccess);
    
    static void tryMovePCBtoHotRegion(idvSQL  *aStatSQL,
                                      ULong   aCurCacheMaxSize,
                                      mmcPCB * aPCB,
                                      /* fix BUG-31172 When the hot region of plan cache overflows by a PCO moved
                                         from cold region,the time for scanning victims in cold region
                                         could be non deterministic. */
                                      iduList  *aPcbLstForMove,
                                      idBool *aSuccess);
    /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
      by moving free parent PCO to the out side of critical section */
    static void  freeVictims(iduList         *aVictimPcbLst);
    static void  freeVictimParentPCO(idvSQL    *aStatSQL,
                                     iduList *aVictimParentPCOLst);
    

    static iduMutex   mLRUMutex;
};

#endif
