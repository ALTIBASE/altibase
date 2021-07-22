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

#include <iduMemMgr.h>
#include <iduMemPool.h>
#include <iduHashUtil.h>
#include <mmuProperty.h>
#include <mmcStatement.h>
#include <mmcPlanCache.h>
#include <mmcSQLTextHash.h>
#include <mmcPCB.h>
#include <mmcParentPCO.h>
#include <mmcChildPCO.h>
#include <mmcPlanCacheLRUList.h>

iduMemPool             mmcPlanCache::mPCBMemPool;
iduMemPool             mmcPlanCache::mParentPCOMemPool;
iduMemPool             mmcPlanCache::mChildPCOMemPool;
mmcPlanCacheSystemInfo mmcPlanCache::mPlanCacheSystemInfo;
iduLatch               mmcPlanCache::mPerfViewLatch;   // PROJ-2408

const SChar  *const  mmcPlanCache::mCacheAbleText[] =
{
    "SELECT",
    "INSERT",
    "DELETE",
    "UPDATE",
    "MOVE",
    "ENQUEUE",
    "DEQUEUE",
    "WITH",     // PROJ-2206 with clause
    "MERGE",    // PROJ-1988 MERGE query
    (SChar*) NULL
};


IDE_RC mmcPlanCache::initialize()
{
    // property load.
    
    IDE_TEST(mmcSQLTextHash::initialize(mmuProperty::getSqlPlanCacheBucketCnt())
             != IDE_SUCCESS);
    
    mmcPlanCacheLRUList::initialize();
    
    IDE_TEST(mPCBMemPool.initialize(IDU_MEM_MMPLAN_CACHE_CONTROL,
                                    (SChar*)"SQL_PLAN_CACHE_PCB_POOL",
                                    ID_SCALABILITY_SYS,
                                    ID_SIZEOF(mmcPCB),
                                    mmuProperty::getSqlPlanCacheInitPCBCnt(),
                                    IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                    ID_TRUE,							/* UseMutex */
                                    IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                    ID_FALSE,							/* ForcePooling */
                                    ID_TRUE,							/* GarabageCollection */
                                    ID_TRUE,                            /* HWCacheLine */
                                    IDU_MEMPOOL_TYPE_LEGACY             /* mempool type*/) 
             != IDE_SUCCESS);			
    
    IDE_TEST(mParentPCOMemPool.initialize(IDU_MEM_MMPLAN_CACHE_CONTROL,
                                          (SChar*)"SQL_PLAN_CACHE_PARENT_PCO_POOL",
                                          ID_SCALABILITY_SYS,
                                          ID_SIZEOF(mmcParentPCO),
                                          mmuProperty::getSqlPlanCacheInitParentPCOCnt(),
                                          IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                          ID_TRUE,							/* UseMutex */
                                          IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                          ID_FALSE,							/* ForcePooling */
                                          ID_TRUE,							/* GarbageCollection */
                                          ID_TRUE,                          /* HWCacheLine */
                                          IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

    IDE_TEST(mChildPCOMemPool.initialize(IDU_MEM_MMPLAN_CACHE_CONTROL,
                                         (SChar*)"SQL_PLAN_CACHE__PCO_POOL",
                                         ID_SCALABILITY_SYS,
                                         ID_SIZEOF(mmcChildPCO),
                                         mmuProperty::getSqlPlanCacheInitChildPCOCnt(),
                                         IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                         ID_TRUE,							/* UseMutex */
                                         IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                         ID_FALSE,							/* ForcePooling */
                                         ID_TRUE,							/* GarbageCollection */
                                         ID_TRUE,                          /* HWCacheLine */
                                         IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			
    idlOS::memset(&mPlanCacheSystemInfo,0x00, ID_SIZEOF(mPlanCacheSystemInfo));
    // PROJ-2408
    IDE_ASSERT ( mPerfViewLatch.initialize( (SChar *)"SQL_PLAN_CACHE_PERF_VIEW_LATCH" )
               == IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC mmcPlanCache::finalize()
{
    mmcPlanCacheLRUList::finalize();
    
    IDE_TEST(mmcSQLTextHash::finalize() != IDE_SUCCESS);
    
    IDE_TEST(mPCBMemPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mParentPCOMemPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mChildPCOMemPool.destroy() != IDE_SUCCESS);
    // PROJ-2408
    IDE_ASSERT(mPerfViewLatch.destroy() == IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**********************************************************************
 * Description: statement의 SQL문장을 가지고 SQL Plan cache에서 SQL문장과
 * 동일한 문장을 가지고 있는 Parent PCO Object를 찾는다.
 * Parent PCO를 찾으면,  해당 Parent PCO에 있는 child PCO리스트를 순회
 * 하면서  environment와 매치되는 chold PCO를 찾고 child PCO의 handle인
 * PCB를 assign한다.
 *
 * Action: SQL문자의 hash key value를 계산하고 , 또한 bucket을 정한다.
 *         bucket latch에 s-latch를 건다.
 *         해당 bucket chain를  따라가면서 HashKey value과 문자열이 같은
 *         Parent PCO를 탐색하며,찾은 parent PCO에서 대하여 prepare-latch를
 *         s-latch를 건다.
 *         Parent PCO를 찾았거나 탐색이 완료되면  bucketLatch를 푼다.
 *         Parent PCO의 used child list에서  environment와 match되는
 *         child PCO를 검색한다.
 *       
 ***********************************************************************/
void  mmcPlanCache::searchSQLText(mmcStatement     *aStatement,
                                  mmcParentPCO     **aFoundedParentPCO,
                                  vULong           *aHashKeyVal,
                                  mmcPCOLatchState *aPCOLatchState )
{

    UInt          sBucket;
    iduListNode * sDummy;
    UInt          sQueryLen;
    UInt          sQueryHashMax;

    sQueryLen     = aStatement->getQueryLen();
    sQueryHashMax = iduProperty::getQueryHashStringLengthMax();

    // BUG-36203
    if( sQueryLen > sQueryHashMax )
    {
        sQueryLen = sQueryHashMax;
    }
    else
    {
        // Nothing to do.
    }

    *aHashKeyVal = iduHashUtil::hashString(IDU_HASHUTIL_INITVALUE,
                                           (const UChar*)aStatement->getQueryString(),
                                           sQueryLen );
    sBucket = mmcSQLTextHash::hash(*aHashKeyVal);
    //해당 bucket에 대하여 s-latch를 건다.
    mmcSQLTextHash::latchBucketAsShared(aStatement->getStatistics(),
                                        sBucket);
    //bucket chain을 따라가면서 SQLText에 해당하는 Parent PCO를 찾고,
    // 찾은 Parent PCO에 대하여  prepare-latch를 s-latch로 걸고 빠져나온다.
    mmcSQLTextHash::searchParentPCO(aStatement->getStatistics(),
                                    sBucket,
                                    aStatement->getQueryString(),
                                    *aHashKeyVal,
                                    aStatement->getQueryLen(),
                                    aFoundedParentPCO,
                                    &sDummy,
                                    aPCOLatchState);
    mmcSQLTextHash::releaseBucketLatch(sBucket);
    
}

//SQL Text에 해당하는 Parent PCO  insert시도를한다.
// 최조로 insert 에 성공하는 statement는
// Parent PCO, child PCO, PCB를 생성하고,
// child PCO의 prepare-latch를 x-latch로 걸고
// chain에 Parent PCO를 추가한다.
IDE_RC  mmcPlanCache::tryInsertSQLText(idvSQL         *aStatistics,
                                       SChar          *aSQLText,
                                       UInt            aSQLTextLen,
                                       vULong          aHashKeyVal,
                                       mmcParentPCO    **aFoundedParentPCO,
                                       mmcPCB          **aPCB,
                                       idBool          *aSuccess,
                                       mmcPCOLatchState *aPCOLatchState)
{
    UInt            sBucket;
    mmcParentPCO   *sParentPCO;
    mmcChildPCO    *sChildPCO;
    iduListNode    *sInsertAfterNode;
    UChar           sState = 0;
    
    sBucket = mmcSQLTextHash::hash(aHashKeyVal);
    mmcSQLTextHash::latchBucketAsExeclusive(aStatistics,
                                            sBucket);
    sState =1;
    //bucket chain을 따라가면서 SQLText에 해당하는 Parent PCO를 찾고,
    // 찾은 Parent PCO에 대하여  prepare-latch를 s-latch로 걸고 빠져나온다.
    mmcSQLTextHash::searchParentPCO(aStatistics,
                                    sBucket,
                                    aSQLText,
                                    aHashKeyVal,
                                    aSQLTextLen,
                                    aFoundedParentPCO,
                                    &sInsertAfterNode,
                                    aPCOLatchState);
    if((*aFoundedParentPCO) != NULL)
    {
        //SQL Text에 해당하는 Parent PCO가 있는 경우.
        *aSuccess = ID_FALSE;
    }
    else
    {
        *aSuccess = ID_TRUE;

        IDU_FIT_POINT( "mmcPlanCache::tryInsertSQLText::alloc::ParentPCO" );

        // 최초로 SQLText에 해당하는 Parent PCO를 insert하는 statement.
        // parent PCO를 할당한다.
        IDE_TEST(mParentPCOMemPool.alloc((void**)&sParentPCO) != IDE_SUCCESS);
        sState = 2;

        IDU_FIT_POINT( "mmcPlanCache::tryInsertSQLText::alloc::PCB" );

        // PCB와 child PCO를 할당받는다.
        IDE_TEST( mPCBMemPool.alloc((void**)aPCB) != IDE_SUCCESS);
        sState = 3;

        IDU_FIT_POINT( "mmcPlanCache::tryInsertSQLText::alloc::ChildPCO" );

        IDE_TEST( mChildPCOMemPool.alloc((void**)&sChildPCO) != IDE_SUCCESS);
        sState = 4;

        //parent PCO를 초기화한다.
        /* BUG-29865
         * 메모리 한계상황에서 ParentPCO->initialize가 실패할 수 있습니다.
         */
        IDE_TEST(sParentPCO->initialize(mmcSQLTextHash::getSQLTextId(sBucket),
                                       aSQLText,
                                       aSQLTextLen,
                                       aHashKeyVal,
                                       sBucket) != IDE_SUCCESS);

        mmcSQLTextHash::incSQLTextId(sBucket);
        
        sState = 5;
        // PCB와 child PCO를 초기화 한다.
        //fix BUG-21429
        (*aPCB)->initialize((SChar*)(sParentPCO->mSQLTextId),
                            0);
        sState = 6;        
        sChildPCO->initialize((SChar*)(sParentPCO->mSQLTextId),
                              0,
                              MMC_CHILD_PCO_IS_CACHE_MISS,
                              0,
                              (*aPCB)->getFrequencyPtr());
        sState = 7;
        (*aPCB)->assignPCO(aStatistics,
                        sParentPCO,
                        sChildPCO);
        // PVO를 위하여  child PCO의 prepare-latch를 x-mode로 건다.
        sChildPCO->latchPrepareAsExclusive(aStatistics);
        // parent PCO의 used child list의  child PCO를  append한다.
        sParentPCO->incChildCreateCnt();
        sParentPCO->addPCBOfChild(*aPCB);
        // last에 달리는 node , max hash value update.
        mmcSQLTextHash::tryUpdateBucketMaxHashVal(sBucket,
                                                  sInsertAfterNode,
                                                  aHashKeyVal);
        
        // bucket chain에 add.
        IDU_LIST_ADD_AFTER(sInsertAfterNode,sParentPCO->getBucketChainNode());
    }
    
    mmcSQLTextHash::releaseBucketLatch(sBucket);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch(sState)
        {
            case 7:
                sChildPCO->finalize();
            case 6:
                (*aPCB)->finalize();
            case 5:
                sParentPCO->finalize();
            case 4:
                mChildPCOMemPool.memfree(sChildPCO);
            case 3:
                mPCBMemPool.memfree(*aPCB);
            case 2:
                mParentPCOMemPool.memfree(sParentPCO);
            case 1:
                mmcSQLTextHash::releaseBucketLatch(sBucket);
        }
    }
    return IDE_FAILURE;
}

                                     
IDE_RC mmcPlanCache::freePCB(mmcPCB* aPCB)
{
    return mPCBMemPool.memfree(aPCB);
}

IDE_RC mmcPlanCache::freePCO(mmcParentPCO* aPCO)
{
    return mParentPCOMemPool.memfree(aPCO);
}

IDE_RC mmcPlanCache::freePCO(mmcChildPCO* aPCO)
{
    return mChildPCOMemPool.memfree(aPCO);
}

// BUG-23144 인라인 함수에서 동시성 제어를 하면 간혹 죽습니다.
// 인라인을 처리하지 않습니다.
void mmcPlanCache::incCacheMissCnt(idvSQL  *aStatSQL)
{
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.lockWrite(aStatSQL, NULL) == IDE_SUCCESS );
    mPlanCacheSystemInfo.mCacheMissCnt++;    

    IDE_ASSERT( mPerfViewLatch.unlock() == IDE_SUCCESS );
}

void mmcPlanCache::incCacheInFailCnt(idvSQL  *aStatSQL)
{
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.lockWrite(aStatSQL, (idvWeArgs*)NULL) == IDE_SUCCESS );
    mPlanCacheSystemInfo.mCacheInFailCnt++;

    IDE_ASSERT( mPerfViewLatch.unlock() == IDE_SUCCESS );
}

void mmcPlanCache::incNoneCacheSQLTryCnt(idvSQL  *aStatSQL)
{
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.lockWrite(aStatSQL, (idvWeArgs*)NULL) == IDE_SUCCESS );
    mPlanCacheSystemInfo.mNoneCacheSQLTryCnt++;

    IDE_ASSERT( mPerfViewLatch.unlock() == IDE_SUCCESS );
}

void mmcPlanCache::incCacheInsertCnt(idvSQL  *aStatSQL)
{
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.lockWrite(aStatSQL, (idvWeArgs*)NULL) == IDE_SUCCESS );
    mPlanCacheSystemInfo.mCacheInsertedCnt++;
    mPlanCacheSystemInfo.mCurrentCacheObjCnt++;

    IDE_ASSERT( mPerfViewLatch.unlock() == IDE_SUCCESS );
}

void mmcPlanCache::updateCacheOutCntAndSize(idvSQL  *aStatSQL,
                                            UInt     aCacheOutCnt,
                                            ULong    aReducedSize)
{
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.lockWrite(aStatSQL, (idvWeArgs*)NULL) == IDE_SUCCESS );

    mPlanCacheSystemInfo.mCacheOutCnt += aCacheOutCnt;
    mPlanCacheSystemInfo.mCurrentCacheSize -= aReducedSize;
    mPlanCacheSystemInfo.mCurrentCacheObjCnt-= aCacheOutCnt;
    IDE_ASSERT( mPerfViewLatch.unlock() == IDE_SUCCESS );
}

void mmcPlanCache::incCacheSize(ULong    aAddedSize)
{
    mPlanCacheSystemInfo.mCurrentCacheSize += aAddedSize;
}

//parent PCO는 plan cache에 있지만
//environment가 맞는 child PCO가 없는 경우에
//child PCO의 중복생성을 막기위한 함수.
IDE_RC  mmcPlanCache::preventDupPlan(idvSQL               *aStatistics,
                                     mmcParentPCO         *aParentPCO,
                                     mmcPCB               *aSafeGuardPCB,
                                     mmcPCB               **aNewPCB,
                                     mmcPCOLatchState     *aPCOLatchState)
{
    mmcPCB        *sNewPCB;
    mmcChildPCO   *sNewChildPCO;
    UInt           sPrevChildCreateCnt;
    UInt           sCurChildCreateCnt;
    UChar          sState = 0;

    IDE_DASSERT(*aPCOLatchState == MMC_PCO_LOCK_ACQUIRED_SHARED);
    //새로운 child PCO생성시도.
    aParentPCO->getChildCreateCnt(&sPrevChildCreateCnt);
    // parent PCO의 s-latch로 잡혔던 prepare-latch를 푼다.
    // parent PCO prepare-latch를 release 하기 전에 safeguard PCB를 plan-Fix를 함.
    // 그렇지 않으면 parent PCO가 victim이 될 수 있다.
    aSafeGuardPCB->planFix(aStatistics);
    aParentPCO->releasePrepareLatch();
    *aPCOLatchState = MMC_PCO_LOCK_RELEASED;
    aParentPCO->getChildCreateCnt(&sCurChildCreateCnt);
    if(sCurChildCreateCnt != sPrevChildCreateCnt)
    {
        //이미 다른 statment가 recompile한경우이다.
        *aNewPCB = (mmcPCB*)NULL;
        aSafeGuardPCB->planUnFix(aStatistics);
    }
    else
    {        
        // parent PCO의 prepare-latch를 x-mode로 건다.
        aParentPCO->latchPrepareAsExclusive(aStatistics);
        aSafeGuardPCB->planUnFix(aStatistics);
        *aPCOLatchState = MMC_PCO_LOCK_ACQUIRED_EXECL;
        aParentPCO->getChildCreateCnt(&sCurChildCreateCnt);
        if( sCurChildCreateCnt == sPrevChildCreateCnt)
        {
            aParentPCO->incChildCreateCnt();
            /* fix BUG-31232  Reducing x-latch duration of parent PCO
               while perform soft prepare .
            */
            IDE_TEST(allocPlanCacheObjs(aStatistics,
                                        aParentPCO,
                                        MMC_CHILD_PCO_IS_CACHE_MISS,
                                        sCurChildCreateCnt,
                                        0,
                                        (qciPlanProperty *)NULL,
                                        (mmcStatement*)NULL,
                                        &sNewPCB,
                                        &sNewChildPCO,
                                        &sState)
                 != IDE_SUCCESS);
            // PVO를 위하여  child PCO의 prepare-latch를 x-mode로 건다.
            sNewChildPCO->latchPrepareAsExclusive(aStatistics);
            // parent PCO의 used child list의  child PCO를  append한다.
            aParentPCO->addPCBOfChild(sNewPCB);
            // parent PCO의 x-latch로 잡혔던 prepare-latch를 푼다.
            aParentPCO->releasePrepareLatch();
            *aPCOLatchState = MMC_PCO_LOCK_RELEASED;
            *aNewPCB = sNewPCB;
        }
        else
        {
            // 해당 Parent PCO부터 soft-prepare를 하기 위하여 latch를 푼다.
            // parent PCO의 x-latch로 잡혔던 prepare-latch를 푼다.
            aParentPCO->releasePrepareLatch();
            *aPCOLatchState = MMC_PCO_LOCK_RELEASED;
            *aNewPCB = (mmcPCB*)NULL;
        }

    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch(sState )
        {
            /* fix BUG-31232  Reducing x-latch duration of parent PCO
           while perform soft prepare.  */
            case 4:
                sNewChildPCO->finalize();
            case 3:
                sNewPCB->finalize();
            case 2:
                IDE_ASSERT(mChildPCOMemPool.memfree(sNewChildPCO) == IDE_SUCCESS);
            case 1:
                //fix BUG-27340 Code-Sonar UMR
                IDE_ASSERT(mPCBMemPool.memfree(sNewPCB) == IDE_SUCCESS);
            default:
                break;
        }
        *aNewPCB = (mmcPCB*)NULL;
    }
    if(*aPCOLatchState !=  MMC_PCO_LOCK_RELEASED)
    {    
        aParentPCO->releasePrepareLatch();
        *aPCOLatchState = MMC_PCO_LOCK_RELEASED;
    }
    
    return IDE_FAILURE;
}

//environement가 맞는 child PCO가 있지만,
//plan이 valid하지 않아,새로운 child PCO의
//중복생성을 막기위한 함수.
IDE_RC  mmcPlanCache::preventDupPlan(idvSQL                 *aStatistics,
                                     mmcStatement           *aStatement,
                                     mmcParentPCO           *aParentPCO,
                                     mmcPCB                 *aUnUsedPCB,
                                     mmcPCB                 **aNewPCB,
                                     mmcPCOLatchState       *aPCOLatchState,
                                     mmcChildPCOCR          aRecompileReason,
                                     UInt                   aPrevChildCreateCnt)
{
    mmcChildPCO                  *sNewChildPCO;
    mmcChildPCO                  *sUnusedChildPCO;
    UInt                         sCurChildCreateCnt;
    UInt                         sChildPCOSize;
    /*fix BUG-31403,
    It would be better to free an old plan cache object immediately
    which is only referenced by a statement while the statement do soft-prepare. */
    UInt                         sFixCount;
    UChar                        sState = 0;
    idBool                       sWinner;
    mmcChildPCOPlanState         sPlanState;
    
    if( aRecompileReason ==  MMC_PLAN_RECOMPILE_BY_SOFT_PREPARE )
    {
        
        sUnusedChildPCO = aUnUsedPCB->getChildPCO();
        sChildPCOSize =  sUnusedChildPCO->getSize();
        aParentPCO->getChildCreateCnt(&sCurChildCreateCnt);
        if(sCurChildCreateCnt != aPrevChildCreateCnt)
        {
            //이미 다른 statment가 recompile한경우이다.
            *aNewPCB = (mmcPCB*)NULL;
            IDE_CONT(You_Are_Looser);
        }
        else
        {
            // parent PCO의 prepare-latch를 x-mode로 건다.
            aParentPCO->latchPrepareAsExclusive(aStatistics);
            *aPCOLatchState = MMC_PCO_LOCK_ACQUIRED_EXECL;
            aParentPCO->getChildCreateCnt(&sCurChildCreateCnt);
            sWinner = (sCurChildCreateCnt == aPrevChildCreateCnt ) ? ID_TRUE: ID_FALSE;
        }
    }
    else
    {
        // MMC_PLAN_RECOMPILE_BY_EXECUTION
        IDE_DASSERT( *aPCOLatchState == MMC_PCO_LOCK_RELEASED);
        sUnusedChildPCO = aUnUsedPCB->getChildPCO();
        sChildPCOSize =  sUnusedChildPCO->getSize();
        sUnusedChildPCO->getPlanState(&sPlanState);
        if(sPlanState == MMC_CHILD_PCO_PLAN_IS_UNUSED)
        {
            //이미 다른 statment가 recompile한경우이다.
            *aNewPCB = (mmcPCB*)NULL;
            IDE_CONT(You_Are_Looser);
        }
        else
        {
            // parent PCO의 prepare-latch를 x-mode로 건다.
            aParentPCO->latchPrepareAsExclusive(aStatistics);
            //fix BUG-22570 [valgrind] mmcPCB::initialze에서 UMR
            aParentPCO->getChildCreateCnt(&sCurChildCreateCnt);
            *aPCOLatchState = MMC_PCO_LOCK_ACQUIRED_EXECL;
            sUnusedChildPCO->getPlanState(&sPlanState);
            sWinner = (sPlanState != MMC_CHILD_PCO_PLAN_IS_UNUSED) ? ID_TRUE: ID_FALSE;
        }
        
    }
    
    if( sWinner == ID_TRUE)
    {
        aParentPCO->incChildCreateCnt();
        // PCB와 child PCO를 할당받는다.
        /* fix BUG-31232  Reducing x-latch duration of parent PCO
           while perform soft prepare .
        */
        IDE_TEST(allocPlanCacheObjs(aStatistics,
                                    aParentPCO,
                                    aRecompileReason,
                                    sCurChildCreateCnt,
                                    sUnusedChildPCO->getRebuildCnt()+1,
                                    sUnusedChildPCO->getEnvironment(),
                                    aStatement,
                                    aNewPCB,
                                    &sNewChildPCO,
                                    &sState)
                 != IDE_SUCCESS);

        // PVO를 위하여  child PCO의 prepare-latch를 x-mode로 건다.
        sNewChildPCO->latchPrepareAsExclusive(aStatistics);
        // parent PCO의 used child list의  child PCO를  append한다.
        aParentPCO->addPCBOfChild(*aNewPCB);
        aUnUsedPCB->getFixCount(aStatistics,
                                &sFixCount);
        aParentPCO->movePCBOfChildToUnUsedLst(aUnUsedPCB);
        // fix BUG-27360, mmcStatement::softPrepare에서 preventDupPlan fix하면서
        // 같이 수정합니다.
        sUnusedChildPCO->updatePlanState(MMC_CHILD_PCO_PLAN_IS_UNUSED);
        /*fix BUG-31403,
          It would be better to free an old plan cache object immediately
          which is only referenced by a statement while the statement do soft-prepare.
         old  PCO에 대하여 caller가 plan-Fix하여 call하기 때문에
         fix count가 1인 case는  나 이외에 참조하는 statement가 없는것이다.. */
        if(sFixCount <= 1)
        {
            //child PCO null로 update.
            aUnUsedPCB->assignPCO(aStatistics,
                                  aParentPCO,
                                  NULL);
            aParentPCO->releasePrepareLatch();
            /* fix BUG-31232  Reducing x-latch duration of parent PCO
               while perform soft prepare .
            */            
            *aPCOLatchState = MMC_PCO_LOCK_RELEASED;

            //child PCO 해제
            sUnusedChildPCO->finalize();
            mmcPlanCache::freePCO(sUnusedChildPCO);

            /* BUG-35673 the instruction ordering serialization */
            IDL_MEM_BARRIER;

            //PCB해제는 fixCount및 동시성문제로
            //여기서 바로 할수 없고, LRU replace에 의하여 행하여 진다.
            updateCacheSize(aStatistics,
                            sChildPCOSize,
                            aUnUsedPCB);
        }
        else
        {
            /* BUG-35673 the instruction ordering serialization */
            //child PCO를 바로 지울수 없음.
            ID_SERIAL_BEGIN(aParentPCO->releasePrepareLatch());
            ID_SERIAL_EXEC(*aPCOLatchState = MMC_PCO_LOCK_RELEASED, 1);
            // unused PCB 를parent에서 unused child list으로 move.
            ID_SERIAL_END(mmcPlanCacheLRUList::moveToColdRegionTail(aStatistics,
                                                                    aUnUsedPCB,
                                                                    NULL)); 

        }
    }
    else
    {
        // 해당 Parent PCO부터 soft-prepare를 하기 위하여 latch를 푼다.
        // parent PCO의 x-latch로 잡혔던 prepare-latch를 푼다.
        aParentPCO->releasePrepareLatch();
        *aNewPCB = (mmcPCB*)NULL;

        *aPCOLatchState = MMC_PCO_LOCK_RELEASED;

    }

    IDE_EXCEPTION_CONT(You_Are_Looser);
    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch(sState )
        {
            /* fix BUG-31232  Reducing x-latch duration of parent PCO
           while perform soft prepare .*/
            case 4:
                sNewChildPCO->finalize();
            case 3:
               (*aNewPCB)->finalize();
            case 2:
                IDE_ASSERT(mChildPCOMemPool.memfree(sNewChildPCO) == IDE_SUCCESS);
            case 1:
                //fix BUG-27340 Code-Sonar UMR
                IDE_ASSERT(mPCBMemPool.memfree(*aNewPCB) == IDE_SUCCESS);
            default:
                break;
        }
        if(*aPCOLatchState !=  MMC_PCO_LOCK_RELEASED)
        {    
            aParentPCO->releasePrepareLatch();
            *aPCOLatchState = MMC_PCO_LOCK_RELEASED;
        }
        *aNewPCB = (mmcPCB*)NULL;

    }
    return IDE_FAILURE;
}

/* fix BUG-31232  Reducing x-latch duration of parent PCO
   while perform soft prepare .
   pre-alloc PCB, PCO
*/
IDE_RC mmcPlanCache::allocPlanCacheObjs(idvSQL          *aStatistics,
                                        mmcParentPCO    *aParentPCO,
                                        mmcChildPCOCR   aRecompileReason,
                                        const UInt      aPcoId,
                                        const UInt      aRebuildCount,
                                        qciPlanProperty *aEnv,
                                        mmcStatement    *aStatement,
                                        mmcPCB          **aNewPCB,
                                        mmcChildPCO     **aNewChildPCO,
                                        UChar           *aState)
{

    IDU_FIT_POINT( "mmcPlanCache::allocPlanCacheObjs::alloc::NewPCB" );

    // PCB와 child PCO를 할당받는다.
    IDE_TEST( mPCBMemPool.alloc((void**)aNewPCB) != IDE_SUCCESS);
    (*aState)++;

    IDU_FIT_POINT( "mmcPlanCache::allocPlanCacheObjs::alloc::NewChildPCO" );

    IDE_TEST( mChildPCOMemPool.alloc((void**)aNewChildPCO) != IDE_SUCCESS);
    (*aState)++;

    // PCB와 child PCO를 초기화 한다.
    // fix BUG-21429
    // fix BUG-29116,There is needless +1 operation in numbering child PCO.
    (*aNewPCB)->initialize((SChar*)(aParentPCO->mSQLTextId),
                           aPcoId);
    (*aState)++;
    // fix BUG-29116,There is needless +1 operation in numbering child PCO.
    (*aNewChildPCO)->initialize((SChar*)(aParentPCO->mSQLTextId),
                                aPcoId,
                                aRecompileReason,
                                aRebuildCount,
                                (*aNewPCB)->getFrequencyPtr());
    (*aState)++;
    // PCB에서 parent ,child PCO를 assign한다.
    (*aNewPCB)->assignPCO(aStatistics,
                          aParentPCO,
                          *aNewChildPCO);

    // PROJ-2163
    if( aEnv != NULL )
    {
        // BUG-36956
        (*aNewChildPCO)->assignEnv(aEnv, (qciPlanBindInfo *)NULL);
        IDE_TEST( qci::rebuildEnvironment(aStatement->getQciStmt() ,
                                          (*aNewChildPCO)->getEnvironment())
                  != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

/* fix BUG-31232  Reducing x-latch duration of parent PCO
   while perform soft prepare .
   free PCB, PCO
*/
void  mmcPlanCache::freePlanCacheObjs(mmcPCB       *aNewPCB,
                                      mmcChildPCO  *aNewChildPCO,
                                      UChar        *aState)
{
    (*aState)--;
    aNewChildPCO->finalize();
    (*aState)--;
    aNewPCB->finalize();
    (*aState)--;
    IDE_ASSERT(mChildPCOMemPool.memfree(aNewChildPCO) == IDE_SUCCESS);
    (*aState)--;
    //fix BUG-27340 Code-Sonar UMR
    IDE_ASSERT(mPCBMemPool.memfree(aNewPCB) == IDE_SUCCESS);

}


void mmcPlanCache::checkIn(idvSQL                 *aStatSQL,
                           mmcPCB                 *aPCB,
                           qciSQLPlanCacheContext *aPlanCacheContext,
                           idBool                 *aSuccess)
{
    mmcPlanCacheLRUList::checkIn(aStatSQL,
                                 aPCB,
                                 aPlanCacheContext,
                                 aSuccess);                             
}

void mmcPlanCache::register4GC(idvSQL                  *aStatSQL,
                               mmcPCB                  *aPCB)
{
    mmcPCBLRURegion      sPCBLruRegion;
    
    aPCB->getLRURegion(aStatSQL,
                       &sPCBLruRegion);
    IDE_ASSERT(sPCBLruRegion == MMC_PCB_IN_NONE_LRU_REGION);
    


    mmcPlanCacheLRUList::register4GC(aStatSQL,
                                         aPCB);
    
    
}


void mmcPlanCache::tryFreeParentPCO(idvSQL  *aStatSQL,
                                    mmcParentPCO *aParentPCO)
{
    UInt  sBucket;
    
    sBucket = aParentPCO->getBucket();
    
    // bucket에 x-latch를 건다.
    mmcSQLTextHash::latchBucketAsExeclusive(aStatSQL,
                                            sBucket);
    
    aParentPCO->latchPrepareAsExclusive(aStatSQL);
    if(aParentPCO->getChildCnt() == 0)
    {
        
        mmcSQLTextHash::tryUpdateBucketMaxHashVal(sBucket,
                                                  aParentPCO->getBucketChainNode());
        //bucket chain에서 제거.
        IDU_LIST_REMOVE(aParentPCO->getBucketChainNode());
        aParentPCO->releasePrepareLatch();
        mmcSQLTextHash::releaseBucketLatch(sBucket);
        //parent PCO 자원 해제.
        aParentPCO->finalize();
        IDE_ASSERT(mParentPCOMemPool.memfree(aParentPCO) == IDE_SUCCESS);
    }
    else
    {
        aParentPCO->releasePrepareLatch();
        mmcSQLTextHash::releaseBucketLatch(sBucket);
    }

}

idBool  mmcPlanCache::isCacheAbleSQLText(SChar        *aSQLText)
{
    UShort  i;
    SChar   *sTokenPtr;
    SChar   *sTemp;
    idBool  sRetVal;
    SChar   sToken[mmcPlanCache::CACHE_KEYWORD_MAX_LEN+1];
    UInt    sTokenLen;

    sTokenPtr = aSQLText;
    sRetVal = ID_FALSE;

    while (1)
    {
        // 공백, tab, new line 제거
        //fix BUG-21427
        for ( ;
              ( ( *sTokenPtr == ' ' )  || ( *sTokenPtr == '\t' ) ||
                ( *sTokenPtr == '\r' ) || ( *sTokenPtr == '\n' ) );
              sTokenPtr++ )
            ;

        // BUG-43885 remove comment
        if ( *sTokenPtr == '/' )
        {
            sTokenPtr++;
            if ( *sTokenPtr == '*' )
            {
                sTokenPtr++;
                sTemp = idlOS::strstr( sTokenPtr, "*/" );
                if ( sTemp != NULL )
                {
                    sTokenPtr = sTemp + 2;
                }
                else
                {
                    break;
                }
            }
            else if ( *sTokenPtr == '/' )
            {
                sTokenPtr++;
                sTemp = idlOS::strstr( sTokenPtr, "\n" );
                if ( sTemp != NULL )
                {
                    sTokenPtr = sTemp + 1;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else if ( *sTokenPtr == '-' )
        {
            sTokenPtr++;
            if ( *sTokenPtr == '-' )
            {
                sTokenPtr++;
                sTemp = idlOS::strstr( sTokenPtr, "\n" );
                /* BUG-44126 */
                if ( sTemp != NULL )
                {
                    sTokenPtr = sTemp + 1;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    // token 구성.
    for(sTokenLen =0;((*sTokenPtr != ' ' ) && (*sTokenPtr != '\t' ) && (*sTokenPtr != '\r') && (*sTokenPtr != '\n' ) &&(*sTokenPtr !='\0' ));
        sTokenPtr++)
    {
        sToken[sTokenLen] = *sTokenPtr;
        sTokenLen++;
        if(sTokenLen  == mmcPlanCache::CACHE_KEYWORD_MAX_LEN )
        {
            break;
        }
    }
    sToken[sTokenLen] = '\0';
    for(i = 0; mCacheAbleText[i] != NULL; i++)
    {
        if(idlOS::strcasecmp(sToken, mCacheAbleText[i]) == 0)
        {
            sRetVal =  ID_TRUE;
            break;
        }
    }//for i
    return sRetVal;
}
void mmcPlanCache::updateCacheSize(idvSQL                 *aStatSQL,
                                   UInt                    aChildPCOSize,
                                   mmcPCB                 *aUnUsedPCB)
{
    /*fix BUG-31245	The size of hot or cold LRU region can be incorrect
      when old PCO is freed directly while statement perform rebuilding
      a plan for old PCO.*/
    mmcPlanCacheLRUList::updateCacheSize(aStatSQL,
                                         aChildPCOSize,
                                         aUnUsedPCB);
                                         
}

IDE_RC mmcPlanCache::compact(void     *aStatement)
{
    mmcStatement * sStatement = (mmcStatement*) aStatement;
    
    mmcPlanCacheLRUList::compact(sStatement->getStatistics());
    return IDE_SUCCESS;                                     
}

// reset := compact + reset statistics
IDE_RC mmcPlanCache::reset(void     *aStatement)
{

    mmcStatement * sStatement = (mmcStatement*) aStatement;
    
    mmcPlanCacheLRUList::compact(sStatement->getStatistics());
    
    mPlanCacheSystemInfo.mCacheHitCnt      = 0;
    mPlanCacheSystemInfo.mCacheMissCnt     = 0;
    mPlanCacheSystemInfo.mCacheInFailCnt   = 0;
    mPlanCacheSystemInfo.mCacheOutCnt      = 0;
    mPlanCacheSystemInfo.mCacheInsertedCnt = 0;
    mPlanCacheSystemInfo.mNoneCacheSQLTryCnt = 0;
    
    
    return IDE_SUCCESS;                  
}


void mmcPlanCache::latchShared4PerfV(idvSQL  *aStatSQL)
{
    idvWeArgs sWeArgs;
    
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.lockRead(aStatSQL, &sWeArgs) == IDE_SUCCESS );
}

void mmcPlanCache::releaseLatchShared4PerfV()
{
    // PROJ-2408
    IDE_ASSERT( mPerfViewLatch.unlock() == IDE_SUCCESS);  
}


IDE_RC mmcPlanCache::buildRecordForSQLPlanCache(idvSQL              * /*aStatistics*/,
                                                void                 *aHeader,
                                                void                 */*aDummyObj*/,
                                                iduFixedTableMemory  *aMemory)
{
    latchShared4PerfV(NULL);
    
    mmcSQLTextHash::getCacheHitCount(&mPlanCacheSystemInfo.mCacheHitCnt);
    mPlanCacheSystemInfo.mMaxCacheSize  = mmuProperty::getSqlPlanCacheSize();
    mPlanCacheSystemInfo.mColdLruSize  = mmcPlanCacheLRUList::mColdRegionList.mCurMemSize;
    mPlanCacheSystemInfo.mHotLruSize   = mmcPlanCacheLRUList::mHotRegionList.mCurMemSize;
    IDE_TEST( iduFixedTable::buildRecord(aHeader,aMemory,(void*)&mPlanCacheSystemInfo)
              != IDE_SUCCESS);
    
    releaseLatchShared4PerfV();
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    {
        releaseLatchShared4PerfV();
    }
    return IDE_FAILURE;

}

static iduFixedTableColDesc gSQLPLANCACHEColDesc[]=
{
    {
        (SChar *)"CACHE_HIT_COUNT",
        offsetof(mmcPlanCacheSystemInfo,mCacheHitCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCacheHitCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CACHE_MISS_COUNT",
        offsetof(mmcPlanCacheSystemInfo,mCacheMissCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCacheMissCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CACHE_IN_FAIL_COUNT",
        offsetof(mmcPlanCacheSystemInfo,mCacheInFailCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCacheInFailCnt ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CACHE_OUT_COUNT",
        offsetof(mmcPlanCacheSystemInfo,mCacheOutCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCacheOutCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"CACHE_INSERTED_COUNT",
        offsetof(mmcPlanCacheSystemInfo,mCacheInsertedCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCacheInsertedCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"CURRENT_CACHE_OBJ_CNT",
        offsetof(mmcPlanCacheSystemInfo,mCurrentCacheObjCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCurrentCacheObjCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    
    
    {
        (SChar *)"NONE_CACHE_SQL_TRY_COUNT",
        offsetof(mmcPlanCacheSystemInfo,mNoneCacheSQLTryCnt),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mNoneCacheSQLTryCnt),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"CURRENT_CACHE_SIZE",
        offsetof(mmcPlanCacheSystemInfo,mCurrentCacheSize),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mCurrentCacheSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
   
    {
        (SChar *)"MAX_CACHE_SIZE",
        offsetof(mmcPlanCacheSystemInfo,mMaxCacheSize),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mMaxCacheSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
 
    {
        (SChar *)"CURRENT_HOT_LRU_SIZE",
        offsetof(mmcPlanCacheSystemInfo,mHotLruSize),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mHotLruSize),
        IDU_FT_TYPE_UBIGINT ,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURRENT_COLD_LRU_SIZE",
        offsetof(mmcPlanCacheSystemInfo,mColdLruSize),
        IDU_FT_SIZEOF(mmcPlanCacheSystemInfo,mColdLruSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    
     {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

iduFixedTableDesc   gSQLPLANCACHETableDesc =
{
    (SChar*)"X$SQL_PLAN_CACHE",
    mmcPlanCache::buildRecordForSQLPlanCache,
    gSQLPLANCACHEColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC mmcPlanCache::movePCBOfChildToUnUsedLst(idvSQL *aStatistics,
                                               mmcPCB *aUnUsedPCB) 
{
/***********************************************************************
 *
 * Description : PROJ-2163
 *      Plan 을 plan cache 에 등록할 수 없게 됐을 때 PCB 와 child PCO 를
 *      안전하게 제거하는 함수이다.
 *
 *      Plan cache 에 등록하기 위해 plan 을 만들 때에는 child PCO 에
 *      X latch 를 잡아서 다른 세션이 같은 plan 을 중복으로 생성할 수
 *      없게 한다.
 *      하지만 생성한 plan 이 plan cache 에 등록할 수 없다면
 *      다른 세션들을 위해 PCB 와 child PCO 를 안전하게 제거해야 한다.
 *
 *      이 함수에서는 PCB 가 안전하게 제거될 수 있도록 parent PCO 의
 *      unused list 로 이동하고 LRU list 의 cold region 뒤로 이동시킨다.
 *      Child PCO 는 NEED_HARD_PREPARE 상태로 변경하여 대기중인 세션은
 *      바로 hard prepare 를 수행하게 한다.
 *      만약 대기중인 세션이 없다면 child PCO 를 바로 삭제한다.
 *
 * Implementation :
 *      1. Parent PCO 에 X latch 걸고 PCB 를 unused list 로 이동
 *      2. Child PCO 를 NEED_HARD_PREPARE 로 변경
 *      3. PCB 의 fix 카운트(대기중인 세션 수)로 분기
 *        a. (fix count == 0; 대기중인 세션이 없음)
 *           PCB 의 child PCO 연결을 끊음 (null 로 세팅)
 *           Parent PCO 의 latch 를 해제
 *           PCB 를 cold region 끝으로 이동
 *           Child PCO 를 삭제
 *        b. (fix count > 0; 대기중인 세션이 있음)
 *           Parent PCO 의 latch 를 해제
 *           PCB 를 cold region 끝으로 이동
 *           Child PCO 의 latch 를 해제
 *
 ***********************************************************************/

    UInt               sFixCount;
    mmcChildPCO      * sUnusedChildPCO = aUnUsedPCB->getChildPCO();
    mmcPCBLRURegion    sPCBLruRegion;
 
    // parent PCO의 prepare-latch를 x-mode로 건다.
    aUnUsedPCB->getParentPCO()->latchPrepareAsExclusive(aStatistics);
    
    aUnUsedPCB->getParentPCO()->movePCBOfChildToUnUsedLst(aUnUsedPCB);

    // fix BUG-27360, mmcStatement::softPrepare에서 preventDupPlan fix하면서
    // 같이 수정합니다.
    sUnusedChildPCO->updatePlanState(MMC_CHILD_PCO_PLAN_NEED_HARD_PREPARE);

    /*fix BUG-31403,
      It would be better to free an old plan cache object immediately
      which is only referenced by a statement while the statement do soft-prepare.
      old  PCO에 대하여 caller가 plan-Fix하여 call하기 때문에
     fix count가 1인 case는  나 이외에 참조하는 statement가 없는것이다.. */
    aUnUsedPCB->getFixCount(aStatistics,
                            &sFixCount);
      
    if(sFixCount <= 0)
    {
        //child PCO null로 update.
        aUnUsedPCB->assignPCO(aStatistics,
                              aUnUsedPCB->getParentPCO(),
                              NULL);

        /* BUG-35673 the instruction ordering serialization */
        ID_SERIAL_BEGIN(aUnUsedPCB->getParentPCO()->releasePrepareLatch());

        /* BUG-47813 pcb가 victim으로 선정되기 전에 PCBLruRegion값을 가져와야 함. */
        // unused PCB 를parent에서 unused child list으로 move.
        ID_SERIAL_EXEC(mmcPlanCacheLRUList::moveToColdRegionTail(aStatistics,
                                                                 aUnUsedPCB,
                                                                 &sPCBLruRegion), 1);
        //child PCO 해제
        /* BUG-38955 Destroying a Latch Held as Exclusive */
        ID_SERIAL_EXEC(sPCBLruRegion == MMC_PCB_IN_NONE_LRU_REGION ?
                       sUnusedChildPCO->releasePrepareLatch() : (void)0, 2);
        ID_SERIAL_EXEC(sUnusedChildPCO->finalize(), 3);
        ID_SERIAL_END(mmcPlanCache::freePCO(sUnusedChildPCO));
    }
    else
    {
        /* BUG-35673 the instruction ordering serialization */
        //child PCO를 바로 지울수 없음.
        ID_SERIAL_BEGIN(aUnUsedPCB->getParentPCO()->releasePrepareLatch());
 
        // unused PCB 를parent에서 unused child list으로 move.
        ID_SERIAL_EXEC(mmcPlanCacheLRUList::moveToColdRegionTail(aStatistics,
                                                                 aUnUsedPCB,
                                                                 NULL), 1);

        ID_SERIAL_END(sUnusedChildPCO->releasePrepareLatch());
    }
    
    return IDE_SUCCESS;
}

/* BUG-46158 PLAN_CACHE_KEEP */
IDE_RC mmcPlanCache::planCacheKeep(void *aMmStatement, SChar *aSQLTextID, idBool aIsKeep)
{
    mmcStatement *sStatement = (mmcStatement *)aMmStatement;

    IDE_TEST(mmcSQLTextHash::planCacheKeep(sStatement->getStatistics(),
                                           aSQLTextID,
                                           (aIsKeep == ID_TRUE) ?
                                           MMC_PCO_PLAN_CACHE_KEEP : MMC_PCO_PLAN_CACHE_UNKEEP)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
