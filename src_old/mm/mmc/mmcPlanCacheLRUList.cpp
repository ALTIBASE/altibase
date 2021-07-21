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

#include <mmcPlanCacheLRUList.h>
#include <mmcPlanCache.h>
#include <mmcParentPCO.h>
#include <mmcPCB.h>

mmcLRuList mmcPlanCacheLRUList::mColdRegionList;
mmcLRuList mmcPlanCacheLRUList::mHotRegionList;
iduMutex   mmcPlanCacheLRUList::mLRUMutex;

void mmcPlanCacheLRUList::initialize()
{
    mColdRegionList.mCurMemSize = 0;
    IDU_LIST_INIT(&(mColdRegionList.mLRuList));
    mHotRegionList.mCurMemSize = 0;
    IDU_LIST_INIT(&(mHotRegionList.mLRuList));
    IDE_ASSERT( mLRUMutex.initialize((SChar*)"SQL_PLAN_CACHE_LRU_MUTEX",
                                      IDU_MUTEX_KIND_NATIVE2,
                                      IDV_WAIT_INDEX_LATCH_FREE_PLAN_CACHE_LRU_LIST_MUTEX)
                == IDE_SUCCESS);                                      
}

void mmcPlanCacheLRUList::finalize()
{
    iduList sLRuList;
    iduListNode   *sIterator;
    iduListNode   *sNodeNext;
    mmcPCB        *sPCB;
    mmcChildPCO   *sChildPCO;
    
    IDU_LIST_INIT(&sLRuList);
    
    IDE_ASSERT( mLRUMutex.lock(NULL) == IDE_SUCCESS);
    
    IDU_LIST_JOIN_LIST(&sLRuList,&(mColdRegionList.mLRuList));
    IDU_LIST_JOIN_LIST(&sLRuList,&(mHotRegionList.mLRuList));

    IDU_LIST_ITERATE_SAFE(&sLRuList,sIterator,sNodeNext)   
    {
        sPCB = (mmcPCB*)sIterator->mObj;
        sChildPCO = sPCB->getChildPCO();
        if(sChildPCO != NULL)
        {    
            sChildPCO->finalize();
            IDE_ASSERT(mmcPlanCache::freePCO(sChildPCO) == IDE_SUCCESS);
        }
        sPCB->finalize();
        IDE_ASSERT(mmcPlanCache::freePCB(sPCB) == IDE_SUCCESS);
    }
    IDE_ASSERT( mLRUMutex.unlock() == IDE_SUCCESS);
    IDE_ASSERT( mLRUMutex.destroy() == IDE_SUCCESS);
}

//rebuild또는 plan validation에 의하여 unused PCB의
//child PCO를 직접해제하는 경우에 불린다.
void mmcPlanCacheLRUList::updateCacheSize(idvSQL                 *aStatSQL,
                                          UInt                    aChildPCOSize,
                                          mmcPCB                 *aUnUsedPCB)
{
    mmcPCBLRURegion   sLRURegion;
    IDE_ASSERT( mLRUMutex.lock(aStatSQL) == IDE_SUCCESS);

    /*fix BUG-31245	The size of hot or cold LRU region can be incorrect
              when old PCO is freed directly while statement perform rebuilding
              a plan for old PCO.
     Get correct LRU region while holding LRU latch,
     and reset the frequency of old PCB.
    */
    aUnUsedPCB->getLRURegion(aStatSQL,
                             &sLRURegion);
    aUnUsedPCB->resetFrequency(aStatSQL);
    if(sLRURegion == MMC_PCB_IN_HOT_LRU_REGION)
    {
        // memory size adjust 
        mHotRegionList.mCurMemSize -= aChildPCOSize;
    }
    else
    {
        mColdRegionList.mCurMemSize -= aChildPCOSize;    
    }
    /*fix BUG-31245	The size of hot or cold LRU region can be incorrect
              when old PCO is freed directly while statement perform rebuilding
              a plan for old PCO.
      move old PCB to the tail of cold LRU region.
    */


    IDU_LIST_REMOVE(aUnUsedPCB->getLRuListNode());
    IDU_LIST_ADD_LAST(&(mColdRegionList.mLRuList),
                      aUnUsedPCB->getLRuListNode());
    mmcPlanCache::updateCacheOutCntAndSize(aStatSQL,
                                           1,
                                           (ULong)aChildPCOSize);


    IDE_ASSERT( mLRUMutex.unlock() == IDE_SUCCESS);
}

void mmcPlanCacheLRUList::validateLRULst(iduList       *aHead,
                                         iduListNode   *aNode)
{
    iduListNode   *sIterator;
    IDU_LIST_ITERATE(aHead,sIterator)
    {
        if(aNode == sIterator)
        {
            IDE_ASSERT(0);
        }
    }        

}

// PCB를 cold region LRU List의 head에 insert한다.
void mmcPlanCacheLRUList::checkIn(idvSQL                 *aStatSQL,
                                  mmcPCB                 *aPCB,
                                  qciSQLPlanCacheContext *aPlanCacheContext,
                                  idBool                 *aSuccess)
{
    ULong             sNewCacheSize;
    /* fix BUG-31131, The approach for reducing the latch duration of plan cache LRU
       by moving codes which is not related with critical section. */
    ULong             sAddedSize;
    mmcChildPCO      *sChildPCO;
    /* fix BUG-31439   An abnormal exit might happen if newly created plan cache object became an old plan cache object
       as soon as check in .  */
    mmcParentPCO     *sParentPCO;
    ULong             sCurCacheMaxSize;
    iduList           sVictimPcbLst;
    iduList           sVictimParentPCOLst;
    
    
    IDE_ASSERT(aPCB != NULL);
    /*fix BUG-30701 , plan cahce replace algorithm을 개선하여 한다.
      LRU latch duration 줄이기.
     */
    IDU_LIST_INIT(&sVictimPcbLst);
    /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
      by moving free parent PCO to the out side of critical section */
    IDU_LIST_INIT(&sVictimParentPCOLst);
    /* fix BUG-31131, The approach for reducing the latch duration of plan cache LRU
       by moving codes which is not related with critical section. */
    sAddedSize = aPlanCacheContext->mSharedPlanSize+ aPlanCacheContext->mPrepPrivateTemplateSize;
    sCurCacheMaxSize= mmuProperty::getSqlPlanCacheSize();
    /* fix BUG-31131, The approach for reducing the latch duration of plan cache LRU
       by moving codes which is not related with critical section. */
    sChildPCO = aPCB->getChildPCO();
    IDE_ASSERT(sChildPCO != NULL);

    
    IDE_ASSERT( mLRUMutex.lock(aStatSQL) == IDE_SUCCESS);
    /* fix BUG-31131, The approach for reducing the latch duration of plan cache LRU
       by moving codes which is not related with critical section. */
    sNewCacheSize = mmcPlanCache::getCurrentCacheSize() + sAddedSize;
    if( sNewCacheSize > sCurCacheMaxSize  )
    {
        IDV_SQL_OPTIME_BEGIN(aStatSQL, IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE);

        // victim을 찾아서 replace시킨다.
        replace(aStatSQL,
                sCurCacheMaxSize,
                (sNewCacheSize - sCurCacheMaxSize),
                /*fix BUG-30701 , plan cahce replace algorith을 개선하여 한다.
                  LRU latch duration 줄이기.
                */
                &sVictimPcbLst,
                /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
                  by moving free parent PCO to the out side of critical section */
                &sVictimParentPCOLst,
                aSuccess);

        IDV_SQL_OPTIME_END(aStatSQL, IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE);
    }
    else
    {
        *aSuccess = ID_TRUE;
    }
    
    
    if(*aSuccess == ID_TRUE)
    {
        /* fix BUG-31439   An abnormal exit might happen if newly created plan cache object became an old plan cache object
           as soon as check in .  */
        sParentPCO = aPCB->getParentPCO();
        sParentPCO->latchPrepareAsShared(aStatSQL);
        
        sChildPCO->assignEnv(aPlanCacheContext->mPlanEnv,
                             &(aPlanCacheContext->mPlanBindInfo)); // PROJ-2163
        sChildPCO->assignPlan(aPlanCacheContext->mSharedPlanMemory,
                              aPlanCacheContext->mSharedPlanSize,
                              aPlanCacheContext->mPrepPrivateTemplate,
                              aPlanCacheContext->mPrepPrivateTemplateSize);
        aPCB->planFix(aStatSQL);
        sParentPCO->releasePrepareLatch();
        
        IDU_LIST_ADD_FIRST(&(mColdRegionList.mLRuList),
                           aPCB->getLRuListNode());
        /* fix BUG-31131, The approach for reducing the latch duration of plan cache LRU
       by moving codes which is not related with critical section. */
        mColdRegionList.mCurMemSize += sAddedSize;
        mmcPlanCache::incCacheSize(sAddedSize);
    }
    else
    {
        //garbage collecting을 위하여 LRU list에 달아논다.
        IDU_LIST_ADD_LAST(&(mColdRegionList.mLRuList),
                          aPCB->getLRuListNode());
        sChildPCO->updatePlanState(MMC_CHILD_PCO_PLAN_NEED_HARD_PREPARE);
    }
    aPCB->resetFrequency(aStatSQL);
    aPCB->setLRURegion(aStatSQL,
                       MMC_PCB_IN_COLD_LRU_REGION);

    // prepare-latch를 푼다.
    sChildPCO->releasePrepareLatch();
    
    IDE_ASSERT( mLRUMutex.unlock() == IDE_SUCCESS);
    /* fix BUG-31131, The approach for reducing the latch duration of plan cache LRU
       by moving codes which is not related with critical section. */    
    if(*aSuccess == ID_TRUE)
    {
        mmcPlanCache::incCacheInsertCnt(aStatSQL);
    }
    
    /*fix BUG-30701 , plan cahce replace algorith을 개선하여 한다.
      LRU latch duration 줄이기.
     */    
    if(IDU_LIST_IS_EMPTY(&sVictimPcbLst) == ID_FALSE)
    {
        freeVictims(&sVictimPcbLst);
        
    }
    /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
      by moving free parent PCO to the out side of critical section */
    if(IDU_LIST_IS_EMPTY(&sVictimParentPCOLst) == ID_FALSE)
    {
        freeVictimParentPCO(aStatSQL,&sVictimParentPCOLst);   
    }
}
/*fix BUG-30701 , plan cahce replace algorith을 개선하여 한다.
  LRU latch duration 줄이기.
*/    
void mmcPlanCacheLRUList::freeVictims(iduList * aVictimPcbLst)
{
    iduListNode     *sIterator;
    iduListNode     *sNodeNext;
    mmcPCB          *sPCB;
    mmcChildPCO     *sChildPCO;
    
    
    IDU_LIST_ITERATE_SAFE(aVictimPcbLst,sIterator,sNodeNext)
    {
        sPCB = (mmcPCB*)sIterator->mObj;
        sChildPCO = sPCB->getChildPCO();
        if(sChildPCO != NULL)
        {
            //child PCO 해제
            sChildPCO->finalize();
            mmcPlanCache::freePCO(sChildPCO);
        }
        //PCB해제
        sPCB->finalize();
        mmcPlanCache::freePCB(sPCB);
        
    }

}

/*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
     by moving free parent PCO to the out side of critical section */
void mmcPlanCacheLRUList::freeVictimParentPCO(idvSQL    *aStatSQL,
                                              iduList   *aVictimParentPCOLst)
{
    iduListNode     *sIterator;
    iduListNode     *sNodeNext;
    mmcParentPCO    *sParentPCO;
    
    
    IDU_LIST_ITERATE_SAFE(aVictimParentPCOLst,sIterator,sNodeNext)
    {
        sParentPCO = (mmcParentPCO*)sIterator->mObj;
        mmcPlanCache::tryFreeParentPCO(aStatSQL,
                                       sParentPCO);
        
    }

}

void mmcPlanCacheLRUList::register4GC(idvSQL   *aStatSQL,
                                      mmcPCB   *aPCB)
{
    
    IDE_ASSERT(aPCB != NULL);    

    IDE_ASSERT( mLRUMutex.lock(aStatSQL) == IDE_SUCCESS);

    // PROJ-2163
    // bindParam 은 childPCO 가 갖고 있는 QC_QMP_MEM 의 PlanBindInfo 를 포인팅하고 있다.
    // childPCO 가 지워질 경우 포인팅 정보를 NULL 로 세팅한다.
    aPCB->getChildPCO()->getPlanBindInfo()->bindParam = NULL;
 
    //garbage collecting을 위하여 LRU list에 달아논다.
    IDU_LIST_ADD_LAST(&(mColdRegionList.mLRuList),
                      aPCB->getLRuListNode());
    aPCB->setLRURegion(aStatSQL,
                       MMC_PCB_IN_COLD_LRU_REGION);
    (aPCB->getChildPCO())->updatePlanState(MMC_CHILD_PCO_PLAN_NEED_HARD_PREPARE);
    aPCB->resetFrequency(aStatSQL);
    // prepare-latch를 푼다.
    (aPCB->getChildPCO())->releasePrepareLatch();
    IDE_ASSERT( mLRUMutex.unlock() == IDE_SUCCESS);

}

// plan rebuild 또는 plan validation검사시 invalid한 plan을
// 담고 있는 PCB를  cold region LRU list의 tail로 옮긴다.

void mmcPlanCacheLRUList::moveToColdRegionTail(idvSQL* aStatSQL,
                                               mmcPCB* aPCB,
                                               mmcPCBLRURegion* aLRURegion)
{
    mmcPCBLRURegion  sLRURegion;
    
    IDE_ASSERT(aPCB != NULL);
    IDE_ASSERT( mLRUMutex.lock(aStatSQL) == IDE_SUCCESS);
    aPCB->getLRURegion(aStatSQL,
                       &sLRURegion);

    IDU_LIST_REMOVE(aPCB->getLRuListNode());

    aPCB->resetFrequency(aStatSQL);
    if(sLRURegion == MMC_PCB_IN_HOT_LRU_REGION)
    {
        // memory size adjust 
        mHotRegionList.mCurMemSize -= aPCB->getChildPCOSize();
        mColdRegionList.mCurMemSize += aPCB->getChildPCOSize();
        aPCB->setLRURegion(aStatSQL,
                           MMC_PCB_IN_COLD_LRU_REGION);
    }
    else
    {
        // COLD REGION, nothing to do.
    }
    IDU_LIST_ADD_LAST(&(mColdRegionList.mLRuList),
                      aPCB->getLRuListNode());

    /* BUG-47813 */
    if (aLRURegion != NULL)
    {
        aPCB->getLRURegion(aStatSQL, aLRURegion);
    }
    
    IDE_ASSERT( mLRUMutex.unlock() == IDE_SUCCESS);
    
}

// aReplaceSize만큼  cache size를 줄이기 위하여
// victim들을 찾아서 replace 시킨다.
void mmcPlanCacheLRUList::replace(idvSQL  *aStatSQL,
                                  ULong   aCurCacheMaxSize,
                                  ULong   aReplaceSize,
                                  iduList *aVictimLst,
                                  iduList *aVictimParentPCOLst,
                                  idBool  *aSuccess)
{
    /* fix BUG-31172 When the hot region of plan cache overflows by a PCO moved from cold region,
       the time for scanning victims in cold region could be non deterministic. */
    iduList        sPcbLstForMove;
    iduListNode   *sIterator;
    iduListNode   *sNodeNext;
    mmcPCB        *sPCB;
    UInt           sFixCount;
    UInt           sFrequency;
    UInt           sPCOSize;
    idBool         sSuccess;
    mmcParentPCO  *sParentPCO;
    mmcChildPCO   *sChildPCO;
    UInt           sChildCnt;
    UInt           sCacheOutCnt =0;
    ULong          sVictimsSize = 0;
    UInt           sHotRegionLruFrequency = mmuProperty::getFrequencyForHotLruRegion();
    UInt           sIndex = IDV_STAT_INDEX_PLAN_CACHE_PPCO_MISS_X_TRY_LATCH_COUNT;
    

    /* fix BUG-31172 When the hot region of plan cache overflows by a PCO moved from cold region,
       the time for scanning victims in cold region could be non deterministic. */
    IDU_LIST_INIT(&sPcbLstForMove);
    IDU_LIST_ITERATE_BACK_SAFE(&(mColdRegionList.mLRuList),sIterator,sNodeNext)   
    {
        sPCB = (mmcPCB*)sIterator->mObj;
        sPCB->getFixCountFrequency(aStatSQL,
                                   &sFixCount,
                                   &sFrequency);
        if(sFrequency >= sHotRegionLruFrequency )
        {
            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_BEGIN(aStatSQL,
                                     IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT);
            tryMovePCBtoHotRegion(aStatSQL,
                                  aCurCacheMaxSize,
                                  sPCB,
                                  &sPcbLstForMove,
                                  &sSuccess);
            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_END(aStatSQL,
                               IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT);
            if(sSuccess == ID_TRUE)
            {
                //goto next PCB
                continue;
            }
            else
            {
                // move failure, nothing to do
                // now, aPCB can be victim.
            }
        }
        if(sFixCount == 0)
        {
            sParentPCO = sPCB->getParentPCO();
            //parent가 이미 해제되었으면 안된다.
            IDE_DASSERT(sParentPCO->getSQLString4HardPrepare() != NULL);

            //fix BUG-30855 It needs to describe soft prepare time in detail for problem tracking.
            IDV_SQL_OPTIME_BEGIN(aStatSQL,
                                     IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO);
            /* BUG-35521 Add TryLatch in PlanCache. */
            sParentPCO->tryLatchPrepareAsExclusive(&sSuccess);
            if (sSuccess == ID_FALSE)
            {
                /* BUG-35631 Add x-trylatch count into stat */
                if (aStatSQL->mSess != NULL)
                {
                    IDV_SESS_ADD_DIRECT(aStatSQL->mSess,
                                        IDV_STAT_INDEX_PLAN_CACHE_PPCO_MISS_X_TRY_LATCH_COUNT,
                                        1);

                    /* 서버에 접속이 안될때를 대비해 1,000,000의 배수에 로그를 출력한다. */
                    if (aStatSQL->mSess->mStatEvent[sIndex].mValue % 1000000 == 0)
                    {
                        ideLog::log(IDE_SERVER_0,
                                    "[WARNING-PLANCACHE-replace] missing trylatch count = %d",
                                    aStatSQL->mSess->mStatEvent[sIndex].mValue);
                    }
                    else
                    {
                        /* Nothing */
                    }
                }
                else
                {
                    /* Nothing */
                }

                continue;
            }
            else
            {
                /* Nothing */
            }
            IDV_SQL_OPTIME_END(aStatSQL,
                               IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO);
            // 다시 fix-count를 확인.
            sPCB->getFixCount(aStatSQL,&sFixCount);
            if(sFixCount == 0)
            {
                IDV_SQL_OPTIME_BEGIN(aStatSQL,
                                     IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE);

                // rebuild또는 plan validation에 바로 child PCO를
                // 해제할수 있다.
                if(sPCB->getChildPCO() != NULL)
                {
                    sChildPCO = sPCB->getChildPCO();

                    /* BUG-46158 */
                    if ((sParentPCO->getPlanCacheKeep() == MMC_PCO_PLAN_CACHE_KEEP) &&
                        (sChildPCO->getPlanState() != MMC_CHILD_PCO_PLAN_IS_UNUSED))
                    {
                        sParentPCO->releasePrepareLatch();
                        continue;
                    }

                    sPCOSize = sChildPCO->getSize();
                    sVictimsSize += sPCOSize;
                    //parent PCO에서 삭제.
                    sParentPCO->deletePCBOfChild(sPCB);
                    if(sPCOSize > 0)
                    {    
                        sCacheOutCnt++;
                    }               
                }
                else
                {
                    sParentPCO->deletePCBOfChild(sPCB);
                }
                // LRU List에서 삭제.
                IDU_LIST_REMOVE(sPCB->getLRuListNode());
                /*fix BUG-30701 , plan cahce replace algorith을 개선하여 한다.
                  LRU latch duration 줄이기.
                  victim list에 추가.
                */
                IDU_LIST_ADD_LAST(aVictimLst,sPCB->getLRuListNode());
                
                //parent prepare-latch를 풀기전에 child 갯수를 저장.
                sChildCnt = sParentPCO->getChildCnt();
                sParentPCO->releasePrepareLatch();
                if(sChildCnt == 0)
                {
                    /*fix BUG-31050 The approach for reducing the latch duration of plan cache LRU
                      by moving free parent PCO to the out side of critical section */
                    IDU_LIST_ADD_LAST(aVictimParentPCOLst,sParentPCO->getVictimNode());
                }

                IDV_SQL_OPTIME_END(aStatSQL,
                                   IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE);
            }
            else
            {
                sParentPCO->releasePrepareLatch();
                //goto next PCB
                continue;
            }
        }
        else
        {
            //nothing to do.    
        }//else

        // fix BUG-30706
        // 원하는 크기만큼 할당 받으면 Cold List 탐색 종료
        if (sVictimsSize >= aReplaceSize)
        {
            break;
        }
    }//IDU_LIST_ITERATE_SAFE
    *aSuccess =  (sVictimsSize >= aReplaceSize) ? ID_TRUE: ID_FALSE;
    
    /* fix BUG-31172 When the hot region of plan cache overflows by a PCO moved from cold region,
       the time for scanning victims in cold region could be non deterministic. */
    IDU_LIST_ITERATE_BACK_SAFE(&(sPcbLstForMove),sIterator,sNodeNext)   
    {
        sPCB = (mmcPCB*)sIterator->mObj;
        IDU_LIST_REMOVE(sPCB->getLRuListNode());
        IDU_LIST_ADD_FIRST(&(mColdRegionList.mLRuList),
                           sPCB->getLRuListNode());
    }
    if( sCacheOutCnt  > 0)
    {
        mColdRegionList.mCurMemSize -= sVictimsSize;
        mmcPlanCache::updateCacheOutCntAndSize(aStatSQL,
                                               sCacheOutCnt,
                                               sVictimsSize);
    }
}

//replace 과정에서 frequency >= HOT_REGION_LRU_FREQUECY
//인 PCB를 cold region LRU list에서 hot region LRU list
//으로 옮기려고 시도 한다. 
void mmcPlanCacheLRUList::tryMovePCBtoHotRegion(idvSQL  *aStatSQL,
                                                ULong    aCurCacheMaxSize,
                                                mmcPCB  *aPCB,
                                                iduList  *aPcbLstForMove,
                                                idBool   *aSuccess)
{
    mmcPCB *sHotPCB;
    ULong  sNewHotRegionSize;
    ULong  sMoveSize;
    ULong  sCurMoveSize;
    ULong  sHotRegionUpperBoundSize = ((aCurCacheMaxSize * mmuProperty::getSqlPlanCacheHotRegionRatio() ) /100);
    iduListNode     *sIterator;
    iduListNode     *sNodeNext;
    
    sNewHotRegionSize = mHotRegionList.mCurMemSize + aPCB->getChildPCOSize();
    *aSuccess = ID_FALSE;

    if( sNewHotRegionSize > sHotRegionUpperBoundSize)
    {
        if(aPCB->getChildPCOSize() <= sHotRegionUpperBoundSize)
        {
            sMoveSize = sNewHotRegionSize - sHotRegionUpperBoundSize;
            sCurMoveSize = 0;
            //HOT Region LRU list에서 PCB를 제거한다.
            IDU_LIST_ITERATE_BACK_SAFE(&(mHotRegionList.mLRuList),sIterator,sNodeNext)   
            {
                sHotPCB = (mmcPCB*)sIterator->mObj;
                /* fix BUG-31212
                   When a PCO move from cold to hot lru region, it would be better to
                   update the frequency of a pco as
                   (the frequency - SQL_PLAN_CACHE_HOT_REGION_FREQUENCY) instead of 0.*/
                sHotPCB->decFrequency(aStatSQL);
                IDU_LIST_REMOVE(sHotPCB->getLRuListNode());
                /* fix BUG-31172 When the hot region of plan cache overflows by a PCO moved from cold region,
                   the time for scanning victims in cold region could be non deterministic. */
                IDU_LIST_ADD_FIRST(aPcbLstForMove,
                                  sHotPCB->getLRuListNode());
                mColdRegionList.mCurMemSize += sHotPCB->getChildPCOSize();
                mHotRegionList.mCurMemSize -=  sHotPCB->getChildPCOSize();
                sHotPCB->setLRURegion(aStatSQL,
                                      MMC_PCB_IN_COLD_LRU_REGION);

                sCurMoveSize  += sHotPCB->getChildPCOSize();
                if(sCurMoveSize >= sMoveSize)
                {
                    *aSuccess = ID_TRUE;
                    break;
                }
            
            }//IDU_LIST_ITERATE_SAFE
            if(*aSuccess == ID_TRUE)
            {
                // 내가 들어갈수 있다.
                IDU_LIST_REMOVE(aPCB->getLRuListNode());
                mColdRegionList.mCurMemSize -= aPCB->getChildPCOSize();
                IDU_LIST_ADD_FIRST(&(mHotRegionList.mLRuList),
                                   aPCB->getLRuListNode());
                aPCB->setLRURegion(aStatSQL,
                                   MMC_PCB_IN_HOT_LRU_REGION);
                mHotRegionList.mCurMemSize += aPCB->getChildPCOSize();
            }
            else
            {
                //nothing to do
                //PCB를 COLD에서 HOT으로 옮길수 없었다.
            }
        }
        else
        {
            // PCB의 plan 자체가 너무 커서 hot-region으로 옮길수 없다.
        }   
    }
    else
    {
        // COLD -> HOT region으로 옮길수 있다.
        IDU_LIST_REMOVE(aPCB->getLRuListNode());
        mColdRegionList.mCurMemSize -= aPCB->getChildPCOSize();
        IDU_LIST_ADD_FIRST(&(mHotRegionList.mLRuList),
                           aPCB->getLRuListNode());
        mHotRegionList.mCurMemSize += aPCB->getChildPCOSize();
        aPCB->setLRURegion(aStatSQL,
                           MMC_PCB_IN_HOT_LRU_REGION);
        *aSuccess = ID_TRUE;
    }
}

void mmcPlanCacheLRUList::compact(idvSQL  *aStatSQL)
{
    UInt  sCacheOutCnt;
    ULong sColdRegionVictimSize;
    ULong sHotRegionVictimSize;

    IDE_ASSERT( mLRUMutex.lock(aStatSQL) == IDE_SUCCESS);
    sCacheOutCnt =0;
    sColdRegionVictimSize  =0;
    sHotRegionVictimSize   =0;
    
    compactLRU(aStatSQL,
               &(mColdRegionList.mLRuList),
               &sCacheOutCnt,
               &sColdRegionVictimSize);
    
    compactLRU(aStatSQL,
               &(mHotRegionList.mLRuList),
               &sCacheOutCnt,
               &sHotRegionVictimSize);
    
    mColdRegionList.mCurMemSize -= sColdRegionVictimSize;
    mHotRegionList.mCurMemSize  -= sHotRegionVictimSize;
    
    
    mmcPlanCache::updateCacheOutCntAndSize(aStatSQL,
                                           sCacheOutCnt,
                                           (sColdRegionVictimSize+
                                           sHotRegionVictimSize));
    
    IDE_ASSERT( mLRUMutex.unlock() == IDE_SUCCESS);

}

// 주어진 LRU list에서 fix-count가 0인 PCB를
// 모두 replace시킨다.
void mmcPlanCacheLRUList::compactLRU(idvSQL  *aStatSQL,
                                     iduList *aLRUList,
                                     UInt    *aCacheOutCnt,
                                     ULong   *aVictimsSize)
{
    iduListNode   *sIterator;
    iduListNode   *sNodeNext;
    mmcPCB        *sPCB;
    UInt           sFixCount;
    mmcParentPCO  *sParentPCO;
    mmcChildPCO   *sChildPCO;
    UInt           sChildCnt;
    UInt           sPCOSize;

    IDU_LIST_ITERATE_BACK_SAFE(aLRUList,sIterator,sNodeNext)   
    {
        sPCB = (mmcPCB*)sIterator->mObj;
        sPCB->getFixCount(aStatSQL,
                          &sFixCount);
        if(sFixCount == 0)
        {
            sParentPCO = sPCB->getParentPCO();
            sParentPCO->latchPrepareAsExclusive(aStatSQL);
            // 다시 fix-count를 확인.
            sPCB->getFixCount(aStatSQL,&sFixCount);
            if(sFixCount == 0)
            {
                // rebuild또는 plan validation에 바로 child PCO를
                // 해제할수 있다.
                if(sPCB->getChildPCO() != NULL)
                {
                    sChildPCO = sPCB->getChildPCO();

                    /* BUG-46158 */
                    if ((sParentPCO->getPlanCacheKeep() == MMC_PCO_PLAN_CACHE_KEEP) &&
                        (sChildPCO->getPlanState() != MMC_CHILD_PCO_PLAN_IS_UNUSED))
                    {
                        sParentPCO->releasePrepareLatch();
                        continue;
                    }

                    sPCOSize = sChildPCO->getSize();
                    *aVictimsSize += sPCOSize;
                    //parent PCO에서 삭제.
                    sParentPCO->deletePCBOfChild(sPCB);
                    //child PCO 해제 
                    sChildPCO->finalize();
                    mmcPlanCache::freePCO(sChildPCO);
                    if(sPCOSize > 0)
                    {    
                        *aCacheOutCnt += 1;
                    }
                }
                else
                {
                    sParentPCO->deletePCBOfChild(sPCB);
                }
                // LRU List에서 삭제.
                IDU_LIST_REMOVE(sPCB->getLRuListNode());
                //PCB해제
                sPCB->finalize();
                mmcPlanCache::freePCB(sPCB);
                //parent prepare-latch를 풀기전에 child 갯수를 저장.
                sChildCnt = sParentPCO->getChildCnt();
                sParentPCO->releasePrepareLatch();
                if(sChildCnt == 0)
                {
                    mmcPlanCache::tryFreeParentPCO(aStatSQL,
                                                   sParentPCO);
                }
            }
            else
            {
                sParentPCO->releasePrepareLatch();
                //goto next PCB
                continue;
            }
        }
        else
        {
            //nothing to do.    
        }//else        
    }//IDU_LIST_ITERATE_SAFE
}                                                
