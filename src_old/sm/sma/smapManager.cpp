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
 

/***********************************************************************
 * $Id: smapManager.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <ide.h>
#include <smErrorCode.h>

#include <smtPJChild.h>
#include <smtPJMgr.h>
#include <smm.h>
#include <smc.h>
#include <smp.h>
#include <smapManager.h>
#include <smaRefineDB.h>
#include <sctTableSpaceMgr.h>

/* ------------------------------------------------
 *  Child RefineDB
 * ----------------------------------------------*/

class smapChild : public smtPJChild
{
    smcTableHeader* mTable;
    smapRebuildJobItem*    mJob;

public:
    smapChild() {}
    void setTableToBeRefine( smcTableHeader  * aTable,
                             smapRebuildJobItem     * aJob)
        {
            mTable = aTable;
            mJob   = aJob;
        }
    IDE_RC doJob();
};

IDE_RC smapChild::doJob()
{
    smxTrans*     sTrans=NULL;
    UInt          sState = 0;

    IDE_TEST( smxTransMgr::alloc( (smxTrans**)&sTrans ) != IDE_SUCCESS );
    IDE_ASSERT( sTrans->begin( NULL,
                               ( SMI_TRANSACTION_REPL_NONE |
                                 SMI_COMMIT_WRITE_NOWAIT ),
                               SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    //refine table
    IDE_TEST(smaRefineDB::refineTable( sTrans,
                                       mJob )
             != IDE_SUCCESS);

    IDE_TEST(smapManager::lock() != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(smapManager::removeJob(mJob) != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(smapManager::unlock() != IDE_SUCCESS);

    IDE_TEST( sTrans->commit() != IDE_SUCCESS );
    IDE_TEST( smxTransMgr::freeTrans( sTrans )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sState == 1)
    {
        (void)smapManager::unlock();
    }
    IDE_POP();
    if(sTrans != NULL)
    {
        sTrans->abort( ID_FALSE, /* aIsLegacyTrans */
                       NULL      /* aLegacyTrans */ );
        smxTransMgr::freeTrans( sTrans );
    }
    return IDE_FAILURE;

}

/* ------------------------------------------------
 *  RefineDB Manager
 * ----------------------------------------------*/

class smapChildMgr : public smtPJMgr
{
    SInt              mThreadCount;
    smapChild       **mChildArray;
    smcTableHeader   *mBaseTable;
    SChar            *mCurTablePtr;
    idBool            mSuccess;

public:
    smapChildMgr() {}
    IDE_RC initialize(SInt aThreadCount);
    IDE_RC assignJob(SInt    aReqChild,
                     idBool *aJobAssigned);
    IDE_RC destroy();
};

IDE_RC smapChildMgr::initialize( SInt aThreadCount)
{
    SInt i;

    mThreadCount = aThreadCount;
    mBaseTable   = (smcTableHeader *)smmManager::m_catTableHeader;
    mCurTablePtr = NULL;

    /* smapChildMgr_initialize_calloc_ChildArray.tc */
    IDU_FIT_POINT("smapChildMgr::initialize::calloc::ChildArray");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_SM_SMA,
                               aThreadCount,
                               ID_SIZEOF(smapChild *),
                               (void**)&mChildArray)
             != IDE_SUCCESS);

    for (i = 0; i < aThreadCount; i++)
    {
        /* TC/FIT/Limit/sm/sma/smapChildMgr_initialize_malloc.sql */
        IDU_FIT_POINT_RAISE( "smapChildMgr::initialize::malloc",
                              insufficient_memory );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMA,
                                   ID_SIZEOF(smapChild),
                                   (void**)&(mChildArray[i])) != IDE_SUCCESS,
                       insufficient_memory );

        mChildArray[i] = new (mChildArray[i])smapChild();
    }

    // 매니저 초기화
    IDE_TEST(smtPJMgr::initialize(aThreadCount,
                                  (smtPJChild **)mChildArray,
                                  &mSuccess)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smapChildMgr::destroy()
{
    SInt i;

    IDE_ASSERT(smtPJMgr::destroy() == IDE_SUCCESS);

    for (i = 0; i < mThreadCount; i++)
    {
        IDE_TEST(mChildArray[i]->destroy() != IDE_SUCCESS);

        IDE_TEST(iduMemMgr::free(mChildArray[i])
                 != IDE_SUCCESS);
    }

    IDE_TEST(iduMemMgr::free(mChildArray)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smapChildMgr::assignJob(SInt aReqChild, idBool *aJobAssigned)
{
    SChar          *sNxtTablePtr;
    smcTableHeader *sCurTable;
    smpSlotHeader  *sSlot;
    smSCN           sSCN;
    smapRebuildJobItem    *sJob;
    UInt            sState = 0;

    while (ID_TRUE)
    {
        IDE_TEST( smcRecord::nextOIDall( mBaseTable,
                                         mCurTablePtr,
                                         &sNxtTablePtr )
                  != IDE_SUCCESS);

        if (sNxtTablePtr != NULL)
        {
            sSlot  = (smpSlotHeader *)sNxtTablePtr;
            sCurTable = (smcTableHeader *)(sSlot + 1);
            SM_GET_SCN( &sSCN, &(sSlot->mCreateSCN) );

            mCurTablePtr  = sNxtTablePtr;

            /* BUG-14975: Delete Row Alloc후 곧바로 Undo시 ASSERT발생.
               IDE_ASSERT(SM_SCN_IS_NOT_INFINITE(sSCN) ||
               (sSlot->mDropFlag == SMP_SLOT_DROP_TRUE));
            */

            /* BUG-35568 - When refining a table, XDB acceses a uninitialized
             *             table header.
             * DROP_FALSE이고 delete bit가 설정된 경우.
             * 테이블 생성하다가 실패한 경우에는 DROP_FALSE이고 delete bit가
             * 설정되어 있을 수 있다.
             * 이 경우 catalog refine시 slot만 free하기 때문에 그냥 skip한다. */
            if( SMP_SLOT_IS_NOT_DROP( sSlot ) &&
                SM_SCN_IS_DELETED( sSCN ) )
            {
                continue;
            }

            // added for A4
            if( SMI_TABLE_TYPE_IS_DISK( sCurTable ) == ID_TRUE )
            {
                /* DISK table은 smrRecoveryMgr::restart() 에서 DRDB Table
                 * refine 과정을 통해 lock item을 초기화 하고 runtime item들을
                 * 설정하므로 여기서는 건너뛴다. */

                continue;
            }

            // Refine SKIP할 Tablespace인지 체크 ( DICARD/OFFLINE TBS)
            if ( sctTableSpaceMgr::hasState( sCurTable->mSpaceID,
                                             SCT_SS_SKIP_REFINE ) == ID_TRUE )
            {
                // Table header의 Lock Item을 할당,초기화 하고
                // Runtime Item들은 NULL로 설정한다.
                IDE_TEST( smcTable::initLockAndSetRuntimeNull( sCurTable )
                          != IDE_SUCCESS );

                continue;
            }

            /*
               1. normal table
               DROP_FALSE
               2. droped table
               DROP_TRUE
               3. create table by abort ( NTA로그까지 찍었지만 Abort하여 Logical Undo )
               DROP_TRUE deletebit
               4. create table by abort ( allocslot 까지 되고 NTA 로그 못찍고 Physical Undo)
               DROP_FALSE deletebit

               case 1,2,3  -> initLockAndRuntimeItem
                 -  1번  => 사용중인 Table이므로 초기화해야함
                 -  2번, => catalog table refine시에
                            drop Table pending이 호출됨
                    3번    ( drop table pending수행을 위해서 초기화 필요)
               case 4      -> skip
                 -  4번  => catalog table refine시 catalog table row만 지워짐
            */
            /*
               if  (sSlot->mDropFlag == SMP_SLOT_DROP_TRUE)  ||
               (((sSlot->mDropFlag == SMP_SLOT_DROP_FALSE) && !(SM_SCN_IS_DELETED(sSCN))) ||
               ((sSlot->mDropFlag == SMP_SLOT_DROP_TRUE) && (SM_SCN_IS_DELETED(sSCN))))
            */
            if( SMP_SLOT_IS_DROP( sSlot ) ||
                SM_SCN_IS_NOT_DELETED(sSCN) )
            {
                IDE_TEST( smcTable::initLockAndRuntimeItem( sCurTable )
                          != IDE_SUCCESS );
            }

            if( SMP_SLOT_IS_DROP( sSlot ) )
            {
                continue;
            }

            if( SM_SCN_IS_DELETED( sSCN ) )
            {
                continue;
            }

            /* PROJ-1594 Volatile TBS */
            /* Volatile table에 대한 refine 작업은
               Service 단계에서 이루어진다. 따라서 여기서는 skip 한다.
               하지만 위 drop flag에 대한 처리는 메모리 테이블과
               공통으로 처리해야 한다. */
            if( SMI_TABLE_TYPE_IS_VOLATILE( sCurTable ) == ID_TRUE )
            {
                continue;
            }

            IDE_TEST(smapManager::lock() != IDE_SUCCESS);
            sState = 1;

            IDE_TEST(smapManager::addJob(&sJob, sCurTable) != IDE_SUCCESS);

            mChildArray[aReqChild]->setTableToBeRefine(sCurTable, sJob);
            *aJobAssigned = ID_TRUE;

            sState = 0;
            IDE_TEST(smapManager::unlock() != IDE_SUCCESS);

            break;
        }
        else
        {
            *aJobAssigned = ID_FALSE;
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sState == 1 )
    {
        IDE_ASSERT(smapManager::unlock() == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}


/* ------------------------------------------------
 *  Do RefineDB Routine
 * ----------------------------------------------*/
iduMemPool  smapManager::mJobPool;
iduMutex    smapManager::mMutex;
smapRebuildJobItem smapManager::mJobItemHeader;

IDE_RC smapManager::doIt(SInt aLoaderCnt)
{
    /* ------------------------------------------------
     *  [1] Normal Table에 대해 RefineDB를 수행
     * ----------------------------------------------*/
    smapChildMgr  * sLoadMgr    = NULL;
    smxTrans      * sTrans      = NULL;
    UInt            sState      = 0;

    /* Init Member */
    mJobItemHeader.mNext = &mJobItemHeader;
    mJobItemHeader.mPrev = &mJobItemHeader;

    ideLog::log(IDE_SERVER_0, "     BEGIN DATABASE REFINING\n");
    IDE_CALLBACK_SEND_SYM("  [SM] Refine Memory Table : ");

    /* Initialize Job Pool */
    IDE_TEST(mMutex.initialize((SChar*)"Index Job List Mutext",
                               IDU_MUTEX_KIND_NATIVE,
                               IDV_WAIT_INDEX_NULL )
             != IDE_SUCCESS);

    IDE_TEST(mJobPool.initialize(IDU_MEM_SM_SMA,
                                  (SChar*)"Job List Pool",
                                  1,
                                  ID_SIZEOF(smapRebuildJobItem),
                                  256,
                                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                  ID_TRUE,							/* UseMutex */
                                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                  ID_FALSE,							/* ForcePooling */
                                  ID_TRUE,							/* GarbageCollection */
                                  ID_TRUE,                          /* HWCacheLine */
                                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

    /* TC/FIT/Limit/sm/sma/smapManager_doIt_malloc.sql */
    IDU_FIT_POINT_RAISE( "smapManager::doIt::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SMA,
                               ID_SIZEOF(smapChildMgr),
                               (void**)&sLoadMgr) != IDE_SUCCESS,
                   insufficient_memory );
    sState = 1;

    sLoadMgr = new (sLoadMgr) smapChildMgr();

    IDE_TEST(sLoadMgr->initialize(aLoaderCnt) != IDE_SUCCESS);
    IDE_TEST(sLoadMgr->start() != IDE_SUCCESS);
    IDE_TEST(sLoadMgr->waitToStart() != IDE_SUCCESS);
    IDE_TEST_RAISE(sLoadMgr->join() != IDE_SUCCESS, thr_join_error);
    IDE_TEST(sLoadMgr->destroy() != IDE_SUCCESS);
    /* BUG-40933 thread 한계상황에서 FATAL에러 내지 않도록 수정
     * thread를 하나도 생성하지 못하여 ABORT된 경우에
     * smapChildMgr thread join 후 그 결과를 확인하여
     * ABORT에러를 낼 수 있도록 한다. */
    IDE_TEST(sLoadMgr->getResult() == ID_FALSE);

    IDE_TEST(iduMemMgr::free(sLoadMgr)
             != IDE_SUCCESS);

    sLoadMgr = NULL;

    /* ------------------------------------------------
     *  [2] Catalog Table에 대해 RefineDB를 수행
     * ----------------------------------------------*/
    IDE_TEST( smxTransMgr::alloc( (smxTrans**)&sTrans ) != IDE_SUCCESS );
    IDE_ASSERT( sTrans->begin( NULL,
                               ( SMI_TRANSACTION_REPL_NONE |
                                 SMI_COMMIT_WRITE_NOWAIT ),
                               SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    IDE_TEST( smaRefineDB::refineTempCatalogTable(sTrans,
                          (smcTableHeader *)smmManager::m_catTempTableHeader)
              != IDE_SUCCESS);

    IDE_TEST( smaRefineDB::refineCatalogTableVarPage(
                                 sTrans,
                                 (smcTableHeader *)smmManager::m_catTableHeader )
              != IDE_SUCCESS );

    IDE_TEST( smaRefineDB::refineCatalogTableFixedPage(
                                 sTrans,
                                 (smcTableHeader *)smmManager::m_catTableHeader )
              != IDE_SUCCESS );

    IDE_TEST( sTrans->commit() != IDE_SUCCESS );
    IDE_TEST( smxTransMgr::freeTrans( sTrans )
              != IDE_SUCCESS );

    /* Destroy Job Pool */
    // To Fix BUG-13209
    IDE_TEST(mJobPool.destroy(ID_TRUE) != IDE_SUCCESS);

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    IDE_CALLBACK_SEND_MSG(" [SUCCESS]");

    ideLog::log(IDE_SERVER_0, "     END DATABASE REFINING\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION(thr_join_error);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_Systhrjoin));
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    if(sTrans != NULL)
    {
        sTrans->abort( ID_FALSE, /* aIsLegacyTrans */
                       NULL      /* aLegacyTrans */ );
        smxTransMgr::freeTrans( sTrans);
    }

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sLoadMgr ) == IDE_SUCCESS );
            sLoadMgr = NULL;
        default:
            break;
    }

    return IDE_FAILURE;

}

IDE_RC smapManager::addJob(smapRebuildJobItem   **aJobItem,
                           smcTableHeader *aTable)
{
    smapRebuildJobItem *sJobItem;

    /* smapManager_addJob_alloc_JobItem.tc */
    IDU_FIT_POINT("smapManager::addJob::alloc::JobItem");
    IDE_TEST(mJobPool.alloc((void**)&sJobItem) != IDE_SUCCESS);

    IDE_TEST(initializeJobInfo(sJobItem, aTable) != IDE_SUCCESS);

    /* Add Job Item To Job List */
    sJobItem->mPrev = mJobItemHeader.mPrev;
    sJobItem->mNext = &mJobItemHeader;

    mJobItemHeader.mPrev->mNext = sJobItem;
    mJobItemHeader.mPrev = sJobItem;

    *aJobItem = sJobItem;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aJobItem = NULL;

    return IDE_FAILURE;
}

IDE_RC smapManager::removeJob(smapRebuildJobItem* aJobItem)
{
    aJobItem->mPrev->mNext = aJobItem->mNext;
    aJobItem->mNext->mPrev = aJobItem->mPrev;

    aJobItem->mPrev = NULL;
    aJobItem->mNext = NULL;

    // To Fix BUG-13209
    IDE_TEST(destroyJobInfo(aJobItem) != IDE_SUCCESS);
    IDE_TEST(mJobPool.memfree(aJobItem) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smapManager::initializeJobInfo( smapRebuildJobItem  * apJobItem,
                                       smcTableHeader      * apTable )
{
    apJobItem->mNext = NULL;
    apJobItem->mPrev = NULL;
    apJobItem->mJobProcessCnt = 0;
    apJobItem->mFinished = ID_FALSE;
    apJobItem->mFirst = ID_TRUE;
    apJobItem->mTargetPageList = NULL;
    apJobItem->mTable = apTable;
    apJobItem->mPidLstAllocPage =
        smpManager::getFirstAllocPageID(&(apTable->mFixed.mMRDB));
    apJobItem->mTargetPageList = &(apTable->mFixed.mMRDB);

    apJobItem->mIdx = 0;

    return IDE_SUCCESS;
}

IDE_RC smapManager::destroyJobInfo(smapRebuildJobItem* /*aJobItem*/)
{
    return IDE_SUCCESS;
}


