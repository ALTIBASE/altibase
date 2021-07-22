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

#include <qci.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmuFixedTable.h>
#include <mmuProperty.h>
#include <mmtAuditManager.h>

iduMemPool         mmcStatementManager::mPlanBufferPool;
iduMemPool         mmcStatementManager::mStmtPool;
/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
iduMemPool         mmcStatementManager::mStmtPageTablePool;
mmcStmtPageTable** mmcStatementManager::mStmtPageTableArr;
UInt               mmcStatementManager::mStmtPageTableArrSize;

idvSQL             mmcStatementManager::mStatistics;
idvSession         mmcStatementManager::mCurrSess;
idvSession         mmcStatementManager::mOldSess;

IDE_RC mmcStatementManager::initialize()
{
    UInt   i;
    UInt   sNumPreAlloc;

    /*
     * Pool 초기화
     */

    IDE_TEST(mPlanBufferPool.initialize(IDU_MEM_MMC,
                                        (SChar *)"MMC_STMT_PLAN_BUFFER_POOL",
                                        ID_SCALABILITY_SYS,
                                        MMC_PLAN_BUFFER_SIZE,
                                        4,
                                        IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                        ID_TRUE,							/* UseMutex */
                                        IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                        ID_FALSE,							/* ForcePooling */
                                        ID_TRUE,							/* GarbageCollection */
                                        ID_TRUE,                          /* HWCacheLine */
                                        IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
            != IDE_SUCCESS);			

    IDE_TEST(mStmtPool.initialize(IDU_MEM_MMC,
                                  (SChar *)"MMC_STMT_POOL",
                                  ID_SCALABILITY_SYS,
                                  ID_SIZEOF(mmcStatement),
                                  4,
                                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                  ID_TRUE,							/* UseMutex */
                                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                  ID_FALSE,							/* ForcePooling */
                                  ID_TRUE,							/* GarbageCollection */
                                  ID_TRUE,                          /* HWCacheLine */
                                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mStmtPageTablePool.initialize(IDU_MEM_MMC,
                                           (SChar *)"MMC_STMTPAGETABLE_POOL",
                                           ID_SCALABILITY_SYS,
                                           ID_SIZEOF(mmcStmtPageTable),
                                           mmuProperty::getMmcStmtpagetableMempoolSize(),
                                           IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                           ID_TRUE,							/* UseMutex */
                                           IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                           ID_FALSE,						/* ForcePooling */
                                           ID_TRUE,							/* GarbageCollection */
                                           ID_TRUE,                         /* HWCacheLine */
                                           IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/) 
             != IDE_SUCCESS);			

    /*
     * PageTableArr 할당
     */
    /* +1 for sysdba mode */
    /* Sysdba mode session always can be created. */
    /* BUG-39700 생성될수 있는 stmt의 수는 max_client + job_thread + 1이다. */
    /* PROJ-2451 Concurrent Execute Package
     * add qciMisc::getConcExecDegreeMax */
    mStmtPageTableArrSize = mmuProperty::getJobThreadCount() +
        mmuProperty::getMaxClient() +
        qciMisc::getConcExecDegreeMax() + 1;

    IDU_FIT_POINT_RAISE( "mmcStatementManager::initialize::malloc::StmtPageTableArr",
                          InsufficientMemory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMM,
                                     ID_SIZEOF(mmcStmtPageTable*) * mStmtPageTableArrSize,
                                     (void **)&(mStmtPageTableArr),
                                     IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

    idlOS::memset( mStmtPageTableArr, 0, ID_SIZEOF(mmcStmtPageTable*) * mStmtPageTableArrSize);

    /*
     * StmtPageTable을 STMTPAGETABLE_PREALLOC_RATIO만큼 미리 할당.
     */
    sNumPreAlloc = (UInt) ( mStmtPageTableArrSize *
            mmuProperty::getStmtpagetablePreallocRatio() / 100 );
    /* sNumPreAlloc must bigger than 0 */
    sNumPreAlloc = (sNumPreAlloc > 1) ? sNumPreAlloc : 1;

    for( i = 1 ; ( i <= sNumPreAlloc ) && ( i <= mStmtPageTableArrSize ) ; i++ )
    {
        /* i = session ID */
        IDE_TEST( allocStmtPageTable( i ) != IDE_SUCCESS );
    }

    /*
     * StatementID = 0 에는 Statement를 할당할 수 없음
     */

    mStmtPageTableArr[0]->mPage[0].mSlotUseCount    = 1;
    mStmtPageTableArr[0]->mPage[0].mFirstFreeSlotID = 1;

    // fix BUG-30731
    idvManager::initSQL( &mStatistics,
                         &mCurrSess,
                         NULL,
                         NULL,
                         NULL,
                         NULL );

    return IDE_SUCCESS;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::finalize()
{
    UShort i;
    UShort sPageID;
    UShort sPageCnt;
    mmcStmtPage *sPage = NULL;

    /*
     * 각 StmtPageTable과 그 안의 Page들에 매달린 Slot들에 대한 메모리를 해제
     */

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    for(i = 0 ; i < mStmtPageTableArrSize ; i++ )
    {
        if( mStmtPageTableArr[i] != NULL )
        {
            for (sPageID = 0, sPageCnt = mStmtPageTableArr[i]->mAllocPchCnt
                 ; sPageID < sPageCnt
                 ; sPageID++)
            {
                sPage = &(mStmtPageTableArr[i]->mPage[sPageID]);

                if (sPage->mSlot != NULL)
                {
                    // fix BUG-28267 [codesonar] Ignored Return Value
                    IDE_ASSERT(iduMemMgr::free(sPage->mSlot)
                               == IDE_SUCCESS);
                    sPage->mSlot = NULL;
                }
                /* fix BUG-28669 statement slot검색을 latch-free하게할수 있어야 한다. */
                // PROJ-2408
                IDE_ASSERT( sPage->mPageLatch.destroy() == IDE_SUCCESS );
            }

            IDE_TEST(mStmtPageTablePool.memfree(mStmtPageTableArr[i]) != IDE_SUCCESS);
        }
    }

    /*
     * Pool 해제
     */

    IDE_TEST(mStmtPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mPlanBufferPool.destroy() != IDE_SUCCESS);
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mStmtPageTablePool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void mmcStatementManager::applyStatisticsForSystem()
{
    idvManager::applyStatisticsToSystem(&mCurrSess,
                                        &mOldSess );
}

IDE_RC mmcStatementManager::allocStatement(mmcStatement ** aStatement,
                                           mmcSession    * aSession,
                                           mmcStatement  * aParentStmt)
{
    mmcStatement *sStatement = NULL;
    mmcStmtPage  *sPage      = NULL;
    mmcStmtSlot  *sSlot      = NULL;
    mmcStmtID     sStmtID;

    /* BUG-31144 */
    UInt          sNumberOfStatementsInSession;
    UInt          sMaxStatementsPerSession;

    /* fix BUG-28669 statement slot검색을 latch-free하게할수 있어야 한다.
       freeStmtSlot의 latch duration을 조금이나 줄이자.
     */
    UShort        sPageID;
    UShort        sSlotID;
    mmcSessID     sSessionID;

    /* PROJ-2109, BUG-40763 */
    sSessionID = aSession->getSessionID();
    IDE_TEST_RAISE( sSessionID > mStmtPageTableArrSize, InvalidSessionID );

    /* BUG-31144 */
    sNumberOfStatementsInSession= aSession->getNumberOfStatementsInSession();
    sMaxStatementsPerSession = aSession->getMaxStatementsPerSession();
    IDE_TEST_RAISE(((sNumberOfStatementsInSession + 1) > sMaxStatementsPerSession), TooManyStatementsInSession);

    IDE_TEST(allocStmtSlot(&sPage, &sSlot, &sStmtID, &sSessionID) != IDE_SUCCESS);

    IDU_FIT_POINT( "mmcStatementManager::allocStatement::alloc::Statement" );

    IDE_TEST(mStmtPool.alloc((void **)&sStatement) != IDE_SUCCESS);

    IDE_TEST(sStatement->initialize(sStmtID, aSession, aParentStmt) != IDE_SUCCESS);

    sSlot->mStatement = sStatement;

    *aStatement = sStatement;

    /* BUG-31144 */
    sNumberOfStatementsInSession++;
    aSession->setNumberOfStatementsInSession(sNumberOfStatementsInSession);

    return IDE_SUCCESS;

    /* BUG-31144 */
    IDE_EXCEPTION(TooManyStatementsInSession);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_MANY_STATEMENTS_IN_THE_SESSION));
    }
    /* PROJ-2109, BUG-40763 */
    IDE_EXCEPTION(InvalidSessionID);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_ID, sSessionID));
    }

    IDE_EXCEPTION_END;
    {
        if (sStatement != NULL)
        {
            mStmtPool.memfree(sStatement);
        }
        if (sSlot != NULL)
        {
            /* fix BUG-28669 statement slot검색을 latch-free하게할수 있어야 한다.
             * freeStmtSlot의 latch duration을 조금이나 줄이자.
             */
             sPageID = MMC_STMT_ID_PAGE(sStmtID);
             sSlotID = MMC_STMT_ID_SLOT(sStmtID);
             /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
             freeStmtSlot( sPage, sSlot, sSessionID, sPageID, sSlotID );
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::freeStatement(mmcStatement *aStatement)
{
    /* BUG-31144 */
    mmcSession *sSession = aStatement->getSession();
    UInt        sNumberOfStatementsInSession;

    /* BUG-36203 PSM Optimize */
    IDE_TEST( aStatement->freeChildStmt( ID_TRUE,
                                         ID_TRUE ) != IDE_SUCCESS );

    /* PROJ-2177 User Interface - Cancel */
    if (aStatement->getStmtCID() != MMC_STMT_CID_NONE)
    {
        (void) sSession->removeStmtIDFromMap(aStatement->getStmtCID());
    }

    IDE_TEST(freeStmtSlot(aStatement) != IDE_SUCCESS);

    IDE_TEST(aStatement->finalize() != IDE_SUCCESS);

    IDE_TEST(mStmtPool.memfree(aStatement) != IDE_SUCCESS);

    /* BUG-38614 The number of statements in session is calculated incorrectly by mmcStatementManager. */
    sNumberOfStatementsInSession = sSession->getNumberOfStatementsInSession();
    sNumberOfStatementsInSession--;
    sSession->setNumberOfStatementsInSession(sNumberOfStatementsInSession);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::findStatement(mmcStatement **aStatement,
                                          mmcSession    *aSession,
                                          mmcStmtID      aStatementID)
{
    mmcStmtPage *sPage = NULL;
    mmcStmtSlot *sSlot = NULL;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    UShort       sSessionID = MMC_STMT_ID_SESSION(aStatementID);
    UShort       sPageID = MMC_STMT_ID_PAGE(aStatementID);
    UShort       sSlotID = MMC_STMT_ID_SLOT(aStatementID);

    /*
     * Page 및 Slot 검색
     */

    IDE_TEST(findStmtSlot(&sPage, &sSlot, sSessionID, sPageID, sSlotID) != IDE_SUCCESS);

    /*
     * Slot에 Statement가 존재하는지 검사
     */

    IDE_TEST(sSlot->mStatement == NULL);

    /*
     * Session 소속이 맞는지 검사
     */
    /* PROJ-2177: Cancel 명령은 다른 Session으로 받으므로 NULL을 허용한다. */
    if (aSession != NULL)
    {
        IDE_TEST(sSlot->mStatement->getSession() != aSession);
    }

    /*
     * Slot의 Statement를 돌려줌
     */

    *aStatement = sSlot->mStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_STATEMENT_NOT_FOUND));
    }

    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::allocStmtPage(mmcStmtPage *aPage)
{
    /*
     * Page에 Slot Array가 할당되어 있지 않으면 할당하고 초기화
     */

    if (aPage->mSlot == NULL)
    {
        UShort sSlotID;
        
        /*
         * Slot Array 할당
         */

        IDU_FIT_POINT_RAISE( "mmcStatementManager::allocStmtPage::malloc::Slot", 
                              InsufficientMemory );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_CMM,
                                         ID_SIZEOF(mmcStmtSlot) * MMC_STMT_ID_SLOT_MAX,
                                         (void **)&(aPage->mSlot),
                                         IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        /*
         * 각 Slot 초기화
         */

        for (sSlotID = 0; sSlotID < MMC_STMT_ID_SLOT_MAX; sSlotID++)
        {
            aPage->mSlot[sSlotID].mSlotID         = sSlotID;
            aPage->mSlot[sSlotID].mNextFreeSlotID = sSlotID + 1;
            aPage->mSlot[sSlotID].mStatement        = NULL;
        }

        /*
         * Page의 Slot 속성값 초기화
         */

        aPage->mSlotUseCount    = 0;
        aPage->mFirstFreeSlotID = 0;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::freeStmtPage(mmcStmtPage *aPage)
{
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    mmcStmtSlot *sStmtSlot = NULL;

    // PROJ-2408
    IDE_ASSERT( aPage->mPageLatch.lockWrite(NULL, NULL) == IDE_SUCCESS );

    sStmtSlot = aPage->mSlot;

    /*
     * Page에 Slot이 하나도 사용되고 있지 않으면 Slot Array를 해제
     */
    if ( (sStmtSlot != NULL) && (aPage->mSlotUseCount == 0) )
    {
        aPage->mFirstFreeSlotID = 0;
        aPage->mSlot            = NULL;

        IDE_ASSERT( aPage->mPageLatch.unlock() == IDE_SUCCESS );

        IDE_TEST(iduMemMgr::free(sStmtSlot) != IDE_SUCCESS);
    }
    else
    {
        IDE_ASSERT( aPage->mPageLatch.unlock() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::allocStmtSlot(mmcStmtPage **aPage,
                                          mmcStmtSlot **aSlot,
                                          mmcStmtID    *aStmtID,
                                          /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                                          mmcSessID    *aSessionID)
{
    UShort       sPageID;
    UShort       sSlotID;

    mmcStmtPageTable *sStmtPageTable;
    mmcStmtPage      *sPage;
    mmcStmtSlot      *sSlot;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    sStmtPageTable = mStmtPageTableArr[*aSessionID - 1];

    /*
     * 할당가능한 Page가 없으면 실패
     */

    IDE_TEST_RAISE(sStmtPageTable->mFirstFreePageID == MMC_STMT_ID_PAGE_MAX, TooManyStatement);

    /*
     * 할당가능한 Page 검색
     */

    sPageID = sStmtPageTable->mFirstFreePageID;
    sPage   = &(sStmtPageTable->mPage[sPageID]);

    /*
     * Page 메모리 할당
     */

    IDE_TEST(allocStmtPage(sPage) != IDE_SUCCESS);

    /*
     * Page내의 빈 Slot 검색
     */

    sSlotID = sPage->mFirstFreeSlotID;
    sSlot   = &(sPage->mSlot[sSlotID]);

    /*
     * Slot이 비어있는지 확인
     */

    IDE_ASSERT(sSlot->mStatement == NULL);

    /*
     * 할당된 Page와 Slot, Statement ID 반환
     */

    *aPage   = sPage;
    *aSlot   = sSlot;
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    *aStmtID = MMC_STMT_ID(*aSessionID, sPageID, sSlotID);

    /*
     * UsedSlotCount 증가
     */

    sPage->mSlotUseCount++;

    /*
     * FreeSlot List 갱신
     */

    sPage->mFirstFreeSlotID = sSlot->mNextFreeSlotID;
    sSlot->mNextFreeSlotID  = MMC_STMT_ID_SLOT_MAX;

    /*
     * Page가 full이면 FreePage List 갱신
     */

    if (sPage->mFirstFreeSlotID == MMC_STMT_ID_SLOT_MAX)
    {
        sStmtPageTable->mFirstFreePageID = sPage->mNextFreePageID;
        sPage->mNextFreePageID      = MMC_STMT_ID_PAGE_MAX;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(TooManyStatement);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_MANY_STATEMENT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::freeStmtSlot(mmcStmtPage *aPage,
                                         mmcStmtSlot *aSlot,
                                         /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                                         UShort aSessionID,
                                         UShort aPageID,
                                         UShort aSlotID )
{
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT( aPage->mPageLatch.lockWrite(NULL, /* idvSQL* */
                                   NULL /* idvWeArgs* */ ) == IDE_SUCCESS);

    /*
     * Statement 삭제
     */

    aSlot->mStatement = NULL;

    /*
     * UsedSlotCount 감소
     */

    aPage->mSlotUseCount--;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT( aPage->mPageLatch.unlock() == IDE_SUCCESS);

    /*
     * FreeSlot List 갱신
     */

    aSlot->mNextFreeSlotID  = aPage->mFirstFreeSlotID;
    aPage->mFirstFreeSlotID = aSlotID;

    /*
     * Page가 full이었으면 FreePage List 갱신
     */

    if (aPage->mSlotUseCount == (MMC_STMT_ID_SLOT_MAX - 1))
    {
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        aPage->mNextFreePageID      = mStmtPageTableArr[aSessionID-1]->mFirstFreePageID;
        mStmtPageTableArr[aSessionID-1]->mFirstFreePageID = aPageID;

    }

    return IDE_SUCCESS;
}

IDE_RC mmcStatementManager::freeStmtSlot(mmcStatement *aStatement)
{
    mmcStmtPage *sPage = NULL;
    mmcStmtSlot *sSlot = NULL;

    mmcStmtID    sStatementID = aStatement->getInfo()->mStatementID;
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    UShort       sSessionID = MMC_STMT_ID_SESSION(sStatementID);
    UShort       sPageID = MMC_STMT_ID_PAGE(sStatementID);
    UShort       sSlotID = MMC_STMT_ID_SLOT(sStatementID);

    /*
     * Page 및 Slot 검색
     */

    IDE_TEST_RAISE(findStmtSlot(&sPage, &sSlot, sSessionID, sPageID, sSlotID) != IDE_SUCCESS, StatementNotFound);

    /*
     * Slot에 저장된 Statement가 일치하는지 검사
     */

    IDE_TEST(sSlot->mStatement != aStatement);

    /*
     * Slot 할당 해제
     */
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(freeStmtSlot(sPage, sSlot, sSessionID, sPageID, sSlotID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(StatementNotFound);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_STATEMENT_NOT_FOUND));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcStatementManager::findStmtSlot(mmcStmtPage **aPage,
                                         mmcStmtSlot **aSlot,
                                         /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                                         UShort aSessionID,
                                         UShort aPageID,
                                         UShort aSlotID)
{
    /*
     * Page 검색
     */
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /*SessionID must bigger than 0*/
    IDE_TEST( aSessionID <= 0 );

    /* bug-37476: protection from a invalid stmt-id */
    IDE_TEST(aSessionID > mStmtPageTableArrSize);
    IDE_TEST(mStmtPageTableArr[aSessionID-1] == NULL);

    *aPage = &(mStmtPageTableArr[aSessionID-1]->mPage[aPageID]);

    /*
     * Page에 Slot Array가 할당되어 있는지 검사
     */

    IDE_TEST((*aPage)->mSlot == NULL);

    /*
     * Slot 검색
     */

    *aSlot = &((*aPage)->mSlot[aSlotID]);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/*
 * Fixed Table Definition for STATEMENT
 */

static UInt callbackForBuildQuery(void *aBaseObj, void * /*aMember*/, UChar *aBuf, UInt aBufSize)
{
    mmcStatementInfo *sStmtInfo = (mmcStatementInfo *)aBaseObj;
    SInt              sLen      = 0;
    SChar            *sQuery;

    sStmtInfo->mStmt->lockQuery();

    sQuery = sStmtInfo->mStmt->getQueryString();

    if (sQuery != NULL)
    {
        sLen = idlOS::snprintf((SChar *)aBuf, aBufSize, "%s", sQuery);

        if (sLen < 0)
        {
            sLen = 0;
        }
    }

    sStmtInfo->mStmt->unlockQuery();

    // fix BUG-20125
    return (UInt)IDL_MIN((UInt)sLen, aBufSize - 1);
}

static UInt callbackForGetSafeWaitTime(void *aBaseObj,
                                       void *aMember,
                                       UChar *aBuf,
                                       UInt /*aBufSize*/)
{
    mmcStatementInfo *sStmtInfo = (mmcStatementInfo *)aBaseObj;
    idvSQL           *sStat     = &sStmtInfo->mStatistics;
    UInt              sOffset   = (UInt)((vULong)aMember - (vULong)aBaseObj);
    idvTime           sBegin;
    idvTime           sEnd;

    switch (sOffset)
    {
        case offsetof(mmcStatementInfo, mStatistics) +
            offsetof(idvSQL, mTimedWait):
            {
                if ( IDV_TIMEBOX_GET_TIME_SWITCH( &(sStat->mTimedWait) )
                     == IDV_TIME_SWITCH_ON )
                {
                    sBegin =
                        IDV_TIMEBOX_GET_BEGIN_TIME( &(sStat->mTimedWait) );
                    IDV_TIME_GET(&sEnd);
                    
                    *((ULong *)aBuf) =
                        IDV_TIME_DIFF_MICRO(&sBegin, &sEnd);
                }
                else
                {
                    *((ULong *)aBuf) = 0;
                }
            }
            break;
        default:
            IDE_CALLBACK_FATAL("not support type");
            break;
    }

    return ID_SIZEOF(ULong);
}

static UInt callbackForGetSafeWaitTimeInSec(void *aBaseObj,
                                       void *aMember,
                                       UChar *aBuf,
                                       UInt   aBufSize)
{
    (void)callbackForGetSafeWaitTime( aBaseObj, aMember, aBuf, aBufSize );
    *((ULong*)aBuf) = *((ULong*)aBuf)/1000000; // in second

    return ID_SIZEOF(ULong);
}

static UInt callbackForTotalTime(void *aBaseObj, void */*aMember*/, UChar *aBuf, UInt /*aBufSize*/)
{
    mmcStatementInfo *sStmtInfo  = (mmcStatementInfo *)aBaseObj;
    idvSQL           *sStat      = &sStmtInfo->mStatistics;
    ULong             sTotalTime = 0;

    idvTime           sBegin;
    idvTime           sEnd;
    UInt              i = 0;

    idvOperTimeIndex sOptmOfQuery[] =
    {
        IDV_OPTM_INDEX_QUERY_PARSE,
        IDV_OPTM_INDEX_QUERY_VALIDATE,
        IDV_OPTM_INDEX_QUERY_OPTIMIZE,
        IDV_OPTM_INDEX_QUERY_EXECUTE,
        IDV_OPTM_INDEX_QUERY_FETCH,
        IDV_OPTM_INDEX_QUERY_SOFT_PREPARE
    };

    /* BUG-45553 */
    for (i = 0; i < ID_ARR_ELEM_CNT(sOptmOfQuery); i++)
    {
        sBegin = IDV_TIMEBOX_GET_BEGIN_TIME(IDV_SQL_OPTIME_DIRECT(sStat, sOptmOfQuery[i]));
        sEnd   = IDV_TIMEBOX_GET_END_TIME  (IDV_SQL_OPTIME_DIRECT(sStat, sOptmOfQuery[i]));

        sTotalTime += IDV_TIME_DIFF_MICRO_SAFE(&sBegin, &sEnd);
    }

    *((ULong*)aBuf) = sTotalTime;

    return ID_SIZEOF(ULong);
}

static UInt callbackForConvertTime(void *aBaseObj, void *aMember, UChar *aBuf, UInt /*aBufSize*/)
{
    /*
     * native 타입으로 저장된 데이타를 usec 형태로 변환한다.
     */
    mmcStatementInfo *sStmtInfo = (mmcStatementInfo *)aBaseObj;
    idvSQL           *sStat     = &sStmtInfo->mStatistics;
    UInt              sOffset   = (UInt)((vULong)aMember - (vULong)aBaseObj);
    idvTime           sBegin;
    idvTime           sEnd;
   
    // fix BUG-18765 [win32] fix compile error 
    if ( sOffset == offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_EXECUTE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_EXECUTE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_EXECUTE ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) + 
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_FETCH) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                       IDV_OPTM_INDEX_QUERY_FETCH ));
       sEnd   =
           IDV_TIMEBOX_GET_END_TIME(
                   IDV_SQL_OPTIME_DIRECT( 
                       sStat, 
                       IDV_OPTM_INDEX_QUERY_FETCH ));
    }
    else if ( sOffset == 
              offsetof(mmcStatementInfo, mStatistics) +
              IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_PARSE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_PARSE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_PARSE ));
    }
    //PROJ-1436.
    else if ( sOffset == 
              offsetof(mmcStatementInfo, mStatistics) +
              IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_SOFT_PREPARE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_SOFT_PREPARE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_SOFT_PREPARE ));
    }
    else if ( sOffset == 
              offsetof(mmcStatementInfo, mStatistics) + 
              IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_VALIDATE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_VALIDATE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_VALIDATE ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_OPTIMIZE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_OPTIMIZE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_QUERY_OPTIMIZE ));
    }
    else if (sOffset == offsetof(mmcStatementInfo, mStatistics) +
             IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES ));
    }
    else if ( sOffset == 
       offsetof(mmcStatementInfo, mStatistics) +
       IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT ));
    }
    else if (sOffset == 
         offsetof(mmcStatementInfo, mStatistics) +
         IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) +
         IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER ));
    }
    else if ( sOffset == offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING ) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING ));
    }
    else if ( offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING ) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING ));
    }
    else if ( offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE ) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE ));
    }
    else if ( offsetof(mmcStatementInfo, mStatistics) +
            IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE ) )
    {
        sBegin =
            IDV_TIMEBOX_GET_BEGIN_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE ));
        sEnd   =
            IDV_TIMEBOX_GET_END_TIME(
                    IDV_SQL_OPTIME_DIRECT( 
                        sStat, 
                        IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE ));
    }
    else 
    {
        IDE_CALLBACK_FATAL("not support type");
    }

    // BUG-21093 : V$STATEMENT의 시간 단위를 Micro sec 단위로 원복
    // milli-second 단위를 바꾸려면 mmuFixedTable::buildConvertTime() 대신
    // mmuFixedTable::buildConvertTimeMSEC()을 사용하면 됨
    return mmuFixedTable::buildConvertTime(&sBegin, &sEnd, aBuf);   
}

static UInt callbackForGetParentStmtId(void *aBaseObj,
                                       void * /*aMember*/,
                                       UChar *aBuf,
                                       UInt   /*aBufSize*/)
{
/***********************************************************************
 *
 *  Description : PROJ-1386 Dynamic-SQL
 *
 *  Implementation : parent statement id를 구하는 FT callback
 *
 ***********************************************************************/
    mmcStatementInfo* sStmtInfo;

    sStmtInfo = (mmcStatementInfo*)aBaseObj;
    
    
    if( sStmtInfo->mParentStmt == NULL )
    {
        // PROJ-1386 Dynamic-SQL
        // root인 경우 parent는 자기 자신
        *((UInt*)aBuf) = (UInt)sStmtInfo->mStatementID;
    }
    else
    {
        *((UInt*)aBuf) = (UInt)(sStmtInfo->mParentStmt->getStmtID());
    }
    

    return ID_SIZEOF(UInt);
}


static iduFixedTableColDesc gSTATEMENTColDesc[] =
{
    {
        (SChar *)"ID",
        offsetof(mmcStatementInfo, mStatementID),
        IDU_FT_SIZEOF(mmcStatementInfo, mStatementID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PARENT_ID",
        offsetof(mmcStatementInfo, mParentStmt),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        callbackForGetParentStmtId,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURSOR_TYPE",
        offsetof(mmcStatementInfo, mCursorFlag),
        IDU_FT_SIZEOF(mmcStatementInfo, mCursorFlag),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SESSION_ID",
        offsetof(mmcStatementInfo, mSessionID),
        IDU_FT_SIZEOF(mmcStatementInfo, mSessionID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TX_ID",
        offsetof(mmcStatementInfo, mTransID),
        IDU_FT_SIZEOF(mmcStatementInfo, mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY",
        offsetof(mmcStatementInfo, mQueryString),
        MMC_STMT_QUERY_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        callbackForBuildQuery,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LAST_QUERY_START_TIME",
        offsetof(mmcStatementInfo, mLastQueryStartTime),
        IDU_FT_SIZEOF(mmcStatementInfo, mLastQueryStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY_START_TIME",
        offsetof(mmcStatementInfo, mQueryStartTime),
        IDU_FT_SIZEOF(mmcStatementInfo, mQueryStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_START_TIME",
        offsetof(mmcStatementInfo, mFetchStartTime),
        IDU_FT_SIZEOF(mmcStatementInfo, mFetchStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STATE",
        offsetof(mmcStatementInfo, mStmtState),
        IDU_FT_SIZEOF(mmcStatementInfo, mStmtState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {   
        /* BUG-34068 */
        (SChar *)"FETCH_STATE",
        offsetof(mmcStatementInfo, mFetchFlag),
        IDU_FT_SIZEOF(mmcStatementInfo, mFetchFlag),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ARRAY_FLAG",
        offsetof(mmcStatementInfo, mIsArray),
        IDU_FT_SIZEOF(mmcStatementInfo, mIsArray),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ROW_NUMBER",
        offsetof(mmcStatementInfo, mRowNumber),
        IDU_FT_SIZEOF(mmcStatementInfo, mRowNumber),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TOTAL_ROW_NUMBER",
        offsetof(mmcStatementInfo, mTotalRowNumber),
        IDU_FT_SIZEOF(mmcStatementInfo, mTotalRowNumber),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"EXECUTE_FLAG",
        offsetof(mmcStatementInfo, mExecuteFlag),
        IDU_FT_SIZEOF(mmcStatementInfo, mExecuteFlag),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"BEGIN_FLAG",
        offsetof(mmcStatementInfo, mIsStmtBegin),
        IDU_FT_SIZEOF(mmcStatementInfo, mIsStmtBegin),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TOTAL_TIME",
        0,
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForTotalTime,
        0, 0, NULL // for internal use
    },
    //PROJ-1436.
    {
        (SChar *)"SOFT_PREPARE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET(IDV_OPTM_INDEX_QUERY_SOFT_PREPARE),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PARSE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_QUERY_PARSE ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"VALIDATE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_QUERY_VALIDATE ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPTIMIZE_TIME", 
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_QUERY_OPTIMIZE ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"EXECUTE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_QUERY_EXECUTE ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_QUERY_FETCH ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    //PROJ-1436
    {
        (SChar *)"SQL_CACHE_TEXT_ID",
        offsetof(mmcStatementInfo, mSQLPlanCacheTextId),
        MMC_SQL_CACHE_TEXT_ID_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SQL_CACHE_PCO_ID",
        offsetof(mmcStatementInfo,mSQLPlanCachePCOId),
        IDU_FT_SIZEOF(mmcStatementInfo,mSQLPlanCachePCOId ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"DRDB_DML_ANALYZE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_DML_RECORD_LOCK_VALIDATE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_DML_ALLOCATE_SLOT_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_DML_WRITE_UNDO_RECORD_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_DML_ALLOCATE_TSS_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_DML_ALLOCATE_UNDO_PAGE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_DML_INDEX_OPER_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_TRANS_LOGICAL_AGING_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DRDB_TRANS_PHYSICAL_AGING_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PLAN_CACHE_REPLACE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PLAN_CACHE_VICTIM_FREE_IN_REPLACE_TIME",
        offsetof(mmcStatementInfo, mStatistics) + 
        IDV_SQL_OPTIME_OFFSET( IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPTIMIZER",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mOptimizer),
        IDU_FT_SIZEOF(idvSQL, mOptimizer),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"COST",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mCost),
        IDU_FT_SIZEOF(idvSQL, mCost),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"USED_MEMORY",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mUseMemory),
        IDU_FT_SIZEOF(idvSQL, mUseMemory),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"READ_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mReadPageCount),
        IDU_FT_SIZEOF(idvSQL, mReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"WRITE_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mWritePageCount),
        IDU_FT_SIZEOF(idvSQL, mWritePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"GET_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mGetPageCount),
        IDU_FT_SIZEOF(idvSQL, mGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CREATE_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mCreatePageCount),
        IDU_FT_SIZEOF(idvSQL, mCreatePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UNDO_READ_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mUndoReadPageCount),
        IDU_FT_SIZEOF(idvSQL, mUndoReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UNDO_WRITE_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mUndoWritePageCount),
        IDU_FT_SIZEOF(idvSQL, mUndoWritePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UNDO_GET_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mUndoGetPageCount),
        IDU_FT_SIZEOF(idvSQL, mUndoGetPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UNDO_CREATE_PAGE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mUndoCreatePageCount),
        IDU_FT_SIZEOF(idvSQL, mUndoCreatePageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MEM_CURSOR_FULL_SCAN",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mMemCursorSeqScan),
        IDU_FT_SIZEOF(idvSQL, mMemCursorSeqScan),
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MEM_CURSOR_INDEX_SCAN",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mMemCursorIndexScan),
        IDU_FT_SIZEOF(idvSQL, mMemCursorIndexScan),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DISK_CURSOR_FULL_SCAN",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mDiskCursorSeqScan),
        IDU_FT_SIZEOF(idvSQL, mDiskCursorSeqScan),
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DISK_CURSOR_INDEX_SCAN",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mDiskCursorIndexScan),
        IDU_FT_SIZEOF(idvSQL, mDiskCursorIndexScan),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"EXECUTE_SUCCESS",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mExecuteSuccessCount),
        IDU_FT_SIZEOF(idvSQL, mExecuteSuccessCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"EXECUTE_FAILURE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mExecuteFailureCount),
        IDU_FT_SIZEOF(idvSQL, mExecuteFailureCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_SUCCESS",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mFetchSuccessCount),
        IDU_FT_SIZEOF(idvSQL, mFetchSuccessCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_FAILURE",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mFetchFailureCount),
        IDU_FT_SIZEOF(idvSQL, mFetchFailureCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PROCESS_ROW",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mProcessRow),
        IDU_FT_SIZEOF(idvSQL, mProcessRow),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MEMORY_TABLE_ACCESS_COUNT",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mMemoryTableAccessCount),
        IDU_FT_SIZEOF(idvSQL, mMemoryTableAccessCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SEQNUM",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mWeArgs) +
        offsetof(idvWeArgs, mWaitEventID),
        IDU_FT_SIZEOF(idvWeArgs, mWaitEventID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"P1",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mWeArgs ) +
        IDV_SQL_WAIT_PARAM_OFFSET( IDV_WAIT_PARAM_1 ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"P2",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mWeArgs ) +
        IDV_SQL_WAIT_PARAM_OFFSET( IDV_WAIT_PARAM_2 ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"P3",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mWeArgs ) +
        IDV_SQL_WAIT_PARAM_OFFSET( IDV_WAIT_PARAM_3 ),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"WAIT_TIME",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mTimedWait),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForGetSafeWaitTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SECOND_IN_TIME",
        offsetof(mmcStatementInfo, mStatistics) +
        offsetof(idvSQL, mTimedWait),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForGetSafeWaitTimeInSec,
        0, 0, NULL // for internal use
    },
    {  /*PROJ-2616*/
        (SChar *)"SIMPLE_QUERY",
        offsetof(mmcStatementInfo, mIsSimpleQuery),
        IDU_FT_SIZEOF(mmcStatementInfo, mIsSimpleQuery),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {   /* BUG-45823 */     
        (SChar *)"SHARD_PIN",
        offsetof(mmcStatementInfo, mShardPinStr),
        IDU_FT_SIZEOF(mmcStatementInfo, mShardPinStr) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SHARD_SESSION_TYPE",
        offsetof(mmcStatementInfo, mShardSessionType),
        IDU_FT_SIZEOF(mmcStatementInfo, mShardSessionType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SHARD_QUERY_TYPE",
        offsetof(mmcStatementInfo, mShardQueryType),
        IDU_FT_SIZEOF(mmcStatementInfo, mShardQueryType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    /* BUG-46892 */
    {
        (SChar *)"MATHEMATICS_TEMP_MEMORY",
        offsetof( mmcStatementInfo, mStatistics ) +
        offsetof( idvSQL, mMathTempMem ),
        IDU_FT_SIZEOF( idvSQL, mMathTempMem ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"GCTX_REQUEST_SCN",
        offsetof( mmcStatementInfo, mGCTxStmtInfo.mRequestSCN ),
        IDU_FT_SIZEOF( mmcStatementInfo, mGCTxStmtInfo.mRequestSCN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"DISTRIBUTION_FIRST_STMT_SCN",
        offsetof( mmcStatementInfo, mGCTxStmtInfo.mShareBuffer.mTxFirstStmtSCN ),
        IDU_FT_SIZEOF( mmcStatementInfo, mGCTxStmtInfo.mShareBuffer.mTxFirstStmtSCN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"DISTRIBUTION_FIRST_STMT_TIME",
        offsetof( mmcStatementInfo, mGCTxStmtInfo.mShareBuffer.mTxFirstStmtTime ),
        IDU_FT_SIZEOF( mmcStatementInfo, mGCTxStmtInfo.mShareBuffer.mTxFirstStmtTime ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"DISTRIBUTION_LEVEL",
        offsetof( mmcStatementInfo, mGCTxStmtInfo.mShareBuffer.mDistLevel ),
        IDU_FT_SIZEOF( mmcStatementInfo, mGCTxStmtInfo.mShareBuffer.mDistLevel ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"REBUILD_COUNT",
        offsetof( mmcStatementInfo, mRebuildCount ),
        IDU_FT_SIZEOF( mmcStatementInfo, mRebuildCount ),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SHARD_PARTIAL_EXEC_TYPE",
        offsetof( mmcStatementInfo, mShardPartialExecType ),
        IDU_FT_SIZEOF( mmcStatementInfo, mShardPartialExecType ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
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


IDE_RC mmcStatementManager::buildRecordForSTATEMENT(idvSQL              * /*aStatistics*/,
                                                    void *aHeader,
                                                    void * /* aDumpObj */,
                                                    iduFixedTableMemory *aMemory)
{
    UInt          i, j, k;
    UInt          sState = 0;
    mmcStatement *sStmt = NULL;
    mmcStmtPage  *sPage = NULL;
    mmcStatementInfo * sInfo = NULL;
    void             * sIndexValues[4];

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Time complexity O(nml) : n=MAX_CLIENT, m=MMC_STMT_ID_PAGE_MAX, l=MMC_STMT_ID_SLOT_MAX */
    for( i=0 ; i < mStmtPageTableArrSize ; i++ )
    {   /*Each of the StmtPageTables*/
        if( mStmtPageTableArr[i] != NULL )
        {
            for( j=0 ;
                 (( mStmtPageTableArr[i]->mPage[j].mSlot != NULL) && (j < MMC_STMT_ID_PAGE_MAX));
                 j++ )
            {   /*Each of the Pages*/
                sPage = &(mStmtPageTableArr[i]->mPage[j]);

                if ( sPage->mSlotUseCount == 0 )
                {   /*There is no statement in this StmtPage*/
                    continue;
                }

                IDE_ASSERT( sPage->mPageLatch.lockRead(NULL, NULL) == IDE_SUCCESS );
                sState++;

                if( (sPage->mSlot) == NULL )
                {   /*low possibility*/
                    sState--;
                    IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
                    break;
                }
                else
                {
                    for( k=0 ; k < MMC_STMT_ID_SLOT_MAX ; k++ )
                    {   /*Each of the Slots*/
                        sStmt = sPage->mSlot[k].mStatement;

                        if( sStmt != NULL )
                        {
                            sInfo = sStmt->getInfo();

                            /* BUG-43006 FixedTable Indexing Filter
                             * Column Index 를 사용해서 전체 Record를 생성하지않고
                             * 부분만 생성해 Filtering 한다.
                             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
                             * 해당하는 값을 순서대로 넣어주어야 한다.
                             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
                             * 어 주어야한다.
                             */
                            sIndexValues[0] = &sInfo->mSessionID;
                            sIndexValues[1] = &sInfo->mExecuteFlag;
                            sIndexValues[2] = &sInfo->mStatistics.mMemCursorSeqScan;
                            sIndexValues[3] = &sInfo->mStatistics.mDiskCursorSeqScan;
                            if ( iduFixedTable::checkKeyRange( aMemory,
                                                               gSTATEMENTColDesc,
                                                               sIndexValues )
                                 == ID_FALSE )
                            {
                                continue; /* IDU_LIST_ITERATE(&mInternalSessionList, sIterator) */
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                            IDE_TEST(iduFixedTable::buildRecord(aHeader, aMemory, (void *)sInfo) != IDE_SUCCESS);
                        }
                    }

                    sState--;
                    IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
                }
            }
        }/*mStmtPageTableArr[i] != NULL*/
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if ( sState > 0 )
        {
            IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
        }
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gSTATEMENTTableDesc =
{
    (SChar *)"X$STATEMENT",
    mmcStatementManager::buildRecordForSTATEMENT,
    gSTATEMENTColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*
 * Fixed Table Definition of SQLTEXT
 */

typedef struct mmcSqlText
{
    mmcSessID  mSID;
    UInt       mStmtID;
    UInt       mPiece;
    SChar     *mText;
} mmcSqlText;


static iduFixedTableColDesc gSQLTEXTColDesc[] =
{
    {
        (SChar *)"SID",
        offsetof(mmcSqlText, mSID),
        IDU_FT_SIZEOF(mmcSqlText, mSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STMT_ID",
        offsetof(mmcSqlText, mStmtID),
        IDU_FT_SIZEOF(mmcSqlText, mStmtID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PIECE",
        offsetof(mmcSqlText, mPiece),
        IDU_FT_SIZEOF(mmcSqlText, mPiece),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TEXT",
        offsetof(mmcSqlText, mText),
        MMC_STMT_SQL_TEXT_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
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


IDE_RC mmcStatementManager::buildRecordForSQLTEXT(idvSQL              * /*aStatistics*/,
                                                  void *aHeader,
                                                  void * /* aDumpObj */,
                                                  iduFixedTableMemory *aMemory)
{
    UInt          i, j, k;
    mmcSqlText    sSqlText;
    SChar        *sSqlTextString = NULL;
    UInt          sSqlLen = 0;
    UInt          sState = 0;
    mmcStatement *sStmt  = NULL;
    mmcStmtPage  *sPage  = NULL;
    idBool        sQueryLocked = ID_FALSE;
    SChar       * sIndex;
    SChar       * sStartIndex;
    SChar       * sPrevIndex;
    SInt          sCurrPos = 0;
    SInt          sCurrLen = 0;
    SInt          sSeqNo   = 0;
    SChar         sParseStr [ MMC_STMT_SQL_TEXT_LEN * 2 + 2 ] = { 0, };

    const mtlModule* sModule;
    
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Time complexity O(nml) : n=MAX_CLIENT, m=MMC_STMT_ID_PAGE_MAX, l=MMC_STMT_ID_SLOT_MAX */
    for( i=0 ; i < mStmtPageTableArrSize ; i++ )
    {   /*Each of the PageTables*/
        if( mStmtPageTableArr[i] != NULL )
        {
            for( j=0;
                 ((mStmtPageTableArr[i]->mPage[j].mSlot != NULL) && (j < MMC_STMT_ID_PAGE_MAX));
                 j++ )
            {   /*Each of the Pages*/
                sPage = &(mStmtPageTableArr[i]->mPage[j]);

                if ( sPage->mSlotUseCount == 0 )
                {   /*There is no statement in this StmtPage*/
                    continue;
                }

                IDE_ASSERT( sPage->mPageLatch.lockRead(NULL, NULL) == IDE_SUCCESS );
                sState++;

                if( (sPage->mSlot) == NULL )
                {   /*low possibility*/
                    sState--;
                    IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
                    break;
                }
                else
                {
                    for( k=0 ; k < MMC_STMT_ID_SLOT_MAX ; k++ )
                    {   /*Each of the Slots*/
                        sStmt = sPage->mSlot[k].mStatement;

                        if( sStmt != NULL )
                        {   /*job for buildRecord*/
                            sStmt->lockQuery();
                            sQueryLocked = ID_TRUE;

                            sSqlTextString   = sStmt->getQueryString();
                            sSqlLen          = sStmt->getQueryLen();
                            sSqlText.mSID    = sStmt->getSessionID();
                            sSqlText.mStmtID = sStmt->getStmtID();

                            // BUG-44978
                            sModule          = mtl::mDBCharSet;
                            sStartIndex      = sSqlTextString;
                            sIndex           = sStartIndex;

                            if ( sSqlTextString != NULL )
                            {
                                while(1)
                                {
                                    sPrevIndex = sIndex;

                                    (void)sModule->nextCharPtr( (UChar**) &sIndex,
                                                                (UChar*) ( sSqlTextString + sSqlLen ));

                                    if (( sSqlTextString + sSqlLen ) <= sIndex )
                                    {
                                        // 끝까지 간 경우.
                                        // 기록을 한 후 break.
                                        sSeqNo++;

                                        sCurrPos = sStartIndex - sSqlTextString;
                                        sCurrLen = sIndex - sStartIndex;

                                        prsCopyStrDupAppo( sParseStr,
                                                           sSqlTextString + sCurrPos,
                                                           sCurrLen );
                
                                        sSqlText.mPiece  = sSeqNo;
                                        sSqlText.mText   = sParseStr;

                                        IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                                                             aMemory,
                                                                             (void *)&sSqlText)
                                                 != IDE_SUCCESS);
                                        break;
                                    }
                                    else
                                    {
                                        if( sIndex - sStartIndex >= MMC_STMT_SQL_TEXT_LEN )
                                        {
                                            // 아직 끝가지 안 갔고, 읽다보니 64바이트 또는 초과한 값이
                                            // 되었을 때 잘라서 기록
                                            sCurrPos = sStartIndex - sSqlTextString;
                
                                            if( sIndex - sStartIndex == MMC_STMT_SQL_TEXT_LEN )
                                            {
                                                // 딱 떨어지는 경우
                                                sCurrLen = MMC_STMT_SQL_TEXT_LEN;
                                                sStartIndex = sIndex;
                                            }
                                            else
                                            {
                                                // 삐져나간 경우 그 이전 캐릭터 위치까지 기록
                                                sCurrLen = sPrevIndex - sStartIndex;
                                                sStartIndex = sPrevIndex;
                                            }

                                            sSeqNo++;

                                            prsCopyStrDupAppo( sParseStr,
                                                               sSqlTextString + sCurrPos,
                                                               sCurrLen );

                                            sSqlText.mPiece  = sSeqNo;
                                            sSqlText.mText   = sParseStr;

                                            IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                                                                 aMemory,
                                                                                 (void *)&sSqlText)
                                                     != IDE_SUCCESS);
                                        }
                                        else
                                        {
                                            // Nothing to do.
                                        }
                                    }
                                }

                            }

                            sStmt->unlockQuery();
                            sQueryLocked = ID_FALSE;
                        }/*sStmt != NULL*/
                    }

                    sState--;
                    IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
                }
            }
        }/*mStmtPageTableArr[i] != NULL*/
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sQueryLocked == ID_TRUE)
        {
            sStmt->unlockQuery();
        }

        if( sState > 0 )
        {
            IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
        }
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gSQLTEXTTableDesc =
{
    (SChar *)"X$SQLTEXT",
    mmcStatementManager::buildRecordForSQLTEXT,
    gSQLTEXTColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*
 * Fixed Table Definition for PLANTEXT
 */

typedef struct mmcPlanText
{
    mmcSessID  mSID;
    UInt       mStmtID;
    UInt       mPiece;
    SChar     *mText;
} mmcPlanText;


static iduFixedTableColDesc gPLANTEXTColDesc[] =
{
    {
        (SChar *)"SID",
        offsetof(mmcPlanText, mSID),
        IDU_FT_SIZEOF(mmcPlanText, mSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STMT_ID",
        offsetof(mmcPlanText, mStmtID),
        IDU_FT_SIZEOF(mmcPlanText, mStmtID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PIECE",
        offsetof(mmcPlanText, mPiece),
        IDU_FT_SIZEOF(mmcPlanText, mPiece),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TEXT",
        offsetof(mmcPlanText, mText),
        MMC_STMT_PLAN_TEXT_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
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


IDE_RC mmcStatementManager::buildRecordForPLANTEXT(idvSQL              * /*aStatistics*/,
                                                   void *aHeader,
                                                   void * /* aDumpObj */,
                                                   iduFixedTableMemory *aMemory)
{
    UInt               i, j, k;
    mmcPlanText        sPlanText;
    iduListNode       *sPieceIterator;
    iduVarStringPiece *sPiece;
    iduVarString       sPlanString;
    UInt               sPieceLen;
    mmcStatement      *sStmt  = NULL;
    mmcStmtPage       *sPage = NULL;
    UInt               sState = 0;

    IDE_TEST(iduVarStringInitialize(&sPlanString,
                                    mmcStatementManager::getPlanBufferPool(),
                                    MMC_PLAN_BUFFER_SIZE) != IDE_SUCCESS);
    sState++;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Time complexity O(nml) : n=MAX_CLIENT, m=MMC_STMT_ID_PAGE_MAX, l=MMC_STMT_ID_SLOT_MAX */
    for( i=0 ; i < mStmtPageTableArrSize ; i++ )
    {   /*Each of the StmtPageTables*/
        if( mStmtPageTableArr[i] != NULL )
        {
            for( j=0;
                 ((mStmtPageTableArr[i]->mPage[j].mSlot != NULL) && (j < MMC_STMT_ID_PAGE_MAX));
                 j++ )
            {   /*Each of the Pages*/
                sPage = &(mStmtPageTableArr[i]->mPage[j]);

                if ( sPage->mSlotUseCount == 0 )
                {   /*There is no statement in this StmtPage*/
                    continue;
                }

                IDE_ASSERT( sPage->mPageLatch.lockRead(NULL, NULL) == IDE_SUCCESS );
                sState++;

                if( (sPage->mSlot) == NULL )
                {   /*low possibility*/
                    sState--;
                    IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
                    break;
                }
                else
                {
                    for( k=0 ; k < MMC_STMT_ID_SLOT_MAX ; k++ )
                    {   /*Each of the Slots*/
                        sStmt = sPage->mSlot[k].mStatement;

                        if( sStmt != NULL )
                        {   /*job for buildRecord*/

                            // BUG-38920
                            if ( sStmt->getStmtState() != MMC_STMT_STATE_ALLOC )
                            {
                                IDE_TEST(iduVarStringTruncate(&sPlanString, ID_TRUE) != IDE_SUCCESS);

                                /* BUG-48224 */
                                if (qci::getPlanTreeTextForFixedTable(sStmt->getQciStmt(), &sPlanString, ID_TRUE) == IDE_SUCCESS)
                                {
                                    if ( sPlanString.mLength != 0 )
                                    {
                                        sPlanText.mSID    = sStmt->getSessionID();
                                        sPlanText.mStmtID = sStmt->getStmtID();
                                        sPlanText.mPiece  = 0;

                                        IDU_LIST_ITERATE(&sPlanString.mPieceList, sPieceIterator)
                                        {
                                            sPiece          = (iduVarStringPiece *)sPieceIterator->mObj;
                                            sPieceLen       = sPiece->mLength;
                                            sPlanText.mText = sPiece->mData;

                                            while (1)
                                            {
                                                IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                                                    aMemory,
                                                                                    (void *)&sPlanText)
                                                         != IDE_SUCCESS);

                                                sPlanText.mPiece++;

                                                if (sPieceLen < MMC_STMT_PLAN_TEXT_LEN)
                                                {
                                                    break;
                                                }

                                                sPieceLen       -= MMC_STMT_PLAN_TEXT_LEN;
                                                sPlanText.mText += MMC_STMT_PLAN_TEXT_LEN;
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // mInfo.mStmtStat : MMC_STMT_STATE_ALLOC
                                // Nothing to do.
                            }
                        }/*sStmt != NULL*/
                    }

                    sState--;
                    IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );
                }
            }
        }/*mStmtPageTableArr[i] != NULL*/
    }

    sState--;
    IDE_TEST(iduVarStringFinalize(&sPlanString) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sState)
        {
            case 2:
                IDE_ASSERT( sPage->mPageLatch.unlock() == IDE_SUCCESS );

            case 1:
                IDE_ASSERT(iduVarStringFinalize(&sPlanString) == IDE_SUCCESS);

            default:
                break;
        }
    }

    return IDE_FAILURE;
}

iduFixedTableDesc gPLANTEXTTableDesc =
{
    (SChar *)"X$PLANTEXT",
    mmcStatementManager::buildRecordForPLANTEXT,
    gPLANTEXTColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
/* A Fuction that allocates/initializes a StmtPageTable related to a specific session. */
IDE_RC mmcStatementManager::allocStmtPageTable( mmcSessID aSessionID )
{
    UInt   i;
    SChar  sLatchName[IDU_MUTEX_NAME_LEN];
    mmcStmtPageTable *sStmtPageTable = NULL;

    if ( mStmtPageTableArr[ aSessionID - 1 ] == NULL )
    {
        IDU_FIT_POINT_RAISE("mmcStatementManager::allocStmtPageTable::alloc::stmtPageTable",
                            InsufficientMemory);
        /*malloc a new mmcStmtPageTable*/
        IDE_TEST_RAISE(mStmtPageTablePool.alloc((void **)&sStmtPageTable) != IDE_SUCCESS,
                       InsufficientMemory);

        /*Initialize the StmtPageTable*/
        sStmtPageTable->mFirstFreePageID = 0;
        sStmtPageTable->mAllocPchCnt = 0;

        /*Initialize each StmtPage*/
        for ( i = 0; i < MMC_STMT_ID_PAGE_MAX; i++, sStmtPageTable->mAllocPchCnt++ )
        {
            idlOS::snprintf(sLatchName,
                            IDU_MUTEX_NAME_LEN,
                            "STMT_PAGE_LATCH_%"ID_UINT32_FMT,
                            i);
            IDE_TEST_RAISE( sStmtPageTable->mPage[i].mPageLatch.initialize(sLatchName)
                            != IDE_SUCCESS, LatchInitFailed );
            sStmtPageTable->mPage[i].mPageID          = i;
            sStmtPageTable->mPage[i].mSlotUseCount    = 0;
            sStmtPageTable->mPage[i].mFirstFreeSlotID = 0;
            sStmtPageTable->mPage[i].mNextFreePageID  = i + 1;
            sStmtPageTable->mPage[i].mSlot            = NULL;
        }

        /*Allocate the first StmtSlot*/
        IDE_TEST(allocStmtPage(&(sStmtPageTable->mPage[0])) != IDE_SUCCESS);

        mStmtPageTableArr[ aSessionID - 1 ] = sStmtPageTable;
    }
    else
    {
        if( mStmtPageTableArr[ aSessionID - 1 ]->mPage[0].mSlot == NULL )
        {   /*Only the first StmtSlot does not exist*/
            /*Allocate the first StmtSlot*/
            IDE_TEST(allocStmtPage(&(mStmtPageTableArr[ aSessionID - 1 ]->mPage[0])) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION(LatchInitFailed)
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_LATCH_INIT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
IDE_RC mmcStatementManager::freeAllStmtSlotExceptFirstOne( mmcSessID aSessionID )
{
    UShort i;
    mmcStmtPageTable* sStmtPageTable = NULL;

    if ( (sStmtPageTable = mStmtPageTableArr[ aSessionID - 1 ]) != NULL )
    {
        /* StmtPage 내의 첫번째를 제외한 나머지 i번째 StmtSlot들에 대해서 free함 */
        for( i = 1 ; (sStmtPageTable->mPage[i].mSlot != NULL) && (i < MMC_STMT_ID_PAGE_MAX) ; i++ )
        {
            IDE_TEST( freeStmtPage( &(sStmtPageTable->mPage[i]) ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void mmcStatementManager::prsCopyStrDupAppo( SChar  * aDest,
                                             SChar  * aSrc,
                                             UInt     aLength )
{
    while ( aLength-- > 0 )
    {
        *aDest++ = *aSrc;
        aSrc++;
    }
    
    *aDest = '\0';
}
