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

#ifndef _O_MMC_STATEMENT_MANAGER_H_
#define _O_MMC_STATEMENT_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <mmcDef.h>


#define MMC_PLAN_BUFFER_SIZE  1024

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
/* Session ID,the size of 16bit, is inserted in the StatementID. */
#define MMC_STMT_ID_SIZE_BIT  32
#define MMC_STMT_ID_SESSION_BIT 16
#define MMC_STMT_ID_PAGE_BIT  6
#define MMC_STMT_ID_SLOT_BIT  (MMC_STMT_ID_SIZE_BIT - MMC_STMT_ID_SESSION_BIT - MMC_STMT_ID_PAGE_BIT)

#define MMC_STMT_ID_SESSION_MAX  (1 << MMC_STMT_ID_SESSION_BIT)
#define MMC_STMT_ID_PAGE_MAX  (1 << MMC_STMT_ID_PAGE_BIT)
#define MMC_STMT_ID_SLOT_MAX  (1 << MMC_STMT_ID_SLOT_BIT)

#define MMC_STMT_ID_SESSION_MASK ((MMC_STMT_ID_SESSION_MAX - 1) << (MMC_STMT_ID_PAGE_BIT + MMC_STMT_ID_SLOT_BIT))
#define MMC_STMT_ID_PAGE_MASK ((MMC_STMT_ID_PAGE_MAX - 1) << MMC_STMT_ID_SLOT_BIT)
#define MMC_STMT_ID_SLOT_MASK (MMC_STMT_ID_SLOT_MAX - 1)

#define MMC_STMT_ID_SESSION(aStmtID) (((aStmtID) >> (MMC_STMT_ID_PAGE_BIT + MMC_STMT_ID_SLOT_BIT)) & (MMC_STMT_ID_SESSION_MAX - 1))
#define MMC_STMT_ID_PAGE(aStmtID) (((aStmtID) >> MMC_STMT_ID_SLOT_BIT) & (MMC_STMT_ID_PAGE_MAX - 1))
#define MMC_STMT_ID_SLOT(aStmtID) ((aStmtID) & MMC_STMT_ID_SLOT_MASK)

#define MMC_STMT_ID(aSession, aPage, aSlot) ( ((aSession) << (MMC_STMT_ID_PAGE_BIT + MMC_STMT_ID_SLOT_BIT)) | (((aPage) << MMC_STMT_ID_SLOT_BIT) & MMC_STMT_ID_PAGE_MASK) | ((aSlot) & MMC_STMT_ID_SLOT_MASK) )

/* PROJ-2177 User Interface - Cancel
 * Note. ULN_STMT_CID_* 와 값을 맞출 것 */

#define MMC_STMT_CID_SIZE_BIT           32
#define MMC_STMT_CID_SESSION_BIT        16
#define MMC_STMT_CID_SEQ_BIT            (MMC_STMT_CID_SIZE_BIT - MMC_STMT_CID_SESSION_BIT)

#define MMC_STMT_CID_SESSION_MAX        (1 << MMC_STMT_CID_SESSION_BIT)

#define MMC_STMT_CID_SESSION(aStmtCID)  (((aStmtCID) >> MMC_STMT_CID_SEQ_BIT) & (MMC_STMT_CID_SESSION_MAX - 1))


class mmcSession;
class mmcStatement;


typedef struct mmcStmtSlot
{
    UShort        mSlotID;
    UShort        mNextFreeSlotID;
    mmcStatement *mStatement;
} mmcStmtSlot;

typedef struct mmcStmtPage
{
    /* fix BUG-28669 statement slot검색을 latch-free하게할수 있어야 한다.
       
       WAS에서 statement caching을 on시킨상태에서 중간에 Altibaes가 restart
       되었다면,과거의 statement-id 로 검색하는 경우에, Statement-id가 속한
       Page Body가 아직 할당되지않는 상태일수 있거나 할당과정을 처리중 일때
       안전하게 statement slot검색을 하 게한다.
       
     */
    // PROJ-2408
    iduLatch     mPageLatch; 
    UShort       mPageID;
    UShort       mSlotUseCount;
    UShort       mFirstFreeSlotID;
    UShort       mNextFreePageID;
    mmcStmtSlot *mSlot;
} mmcStmtPage;

typedef struct mmcStmtPageTable
{
    UShort      mFirstFreePageID;
    /* fix BUG-28669 statement slot검색을 latch-free하게할수 있어야 한다.
       PCH(Page Control Header) allocate 에 성공한 Page갯수   */
    UInt        mAllocPchCnt;
    mmcStmtPage mPage[MMC_STMT_ID_PAGE_MAX];
} mmcStmtPageTable;


class mmcStatementManager
{
private:
    static iduMemPool       mPlanBufferPool;

    static iduMemPool       mStmtPool;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* MemPool for allocating mmcStmtPageTable */
    static iduMemPool         mStmtPageTablePool;
    static UInt               mStmtPageTableArrSize;
    /* This array contains pointers of mmcStmtPageTable which are used for
       connecting a specific session to a specific StmtPageTable. 
       NOK ref. = http://nok.altibase.com/x/Fyp9 */
    static mmcStmtPageTable **mStmtPageTableArr;

    // fix BUG-30731
    // mmcStatementManager의 Mutex 정보를 출력한다.
    static idvSQL           mStatistics;
    static idvSession       mCurrSess;
    static idvSession       mOldSess;

public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static iduMemPool *getPlanBufferPool();

    static IDE_RC allocStatement(mmcStatement ** aStatement,
                                 mmcSession    * aSession,
                                 mmcStatement  * aParentStmt);
    
    static IDE_RC freeStatement(mmcStatement *aStatement);

    /* fix BUG-28669 statement slot검색을 latch-free하게할수 있어야 한다.
       freeStmtSlot의 latch duration을 조금이나 줄이자.
     */
    static IDE_RC findStatement(mmcStatement **aStatement,
                                mmcSession    *aSession,
                                mmcStmtID      aStatementID);

    // fix BUG-30731
    // mmtSessionManager에서 주기적으로 통계정보 반영
    static void   applyStatisticsForSystem();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* behave just like the name of the function */
    static IDE_RC freeAllStmtSlotExceptFirstOne( mmcSessID aSessionID );
    static IDE_RC allocStmtPageTable( mmcSessID aSessionID );

    static void prsCopyStrDupAppo( SChar  * aDest,
                                   SChar  * aSrc,
                                   UInt     aLength );
    
private:
    static IDE_RC allocStmtPage(mmcStmtPage *aPage);
    static IDE_RC freeStmtPage(mmcStmtPage *aPage);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    static IDE_RC allocStmtSlot(mmcStmtPage **aPage, mmcStmtSlot **aSlot, mmcStmtID *aStmtID, mmcSessID *aSessionID);

    static IDE_RC freeStmtSlot(mmcStmtPage *aPage,
                               mmcStmtSlot *aSlot,
                               /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                               /* Session ID,the size of 16bit, is inserted in the StatementID. */
                               UShort aSessionID,
                               UShort aPageID,
                               UShort aSlotID);

    static IDE_RC freeStmtSlot(mmcStatement *aStatement);

    static IDE_RC findStmtSlot(mmcStmtPage **aPage,
                               mmcStmtSlot **aSlot,
                               /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
                               UShort aSessionID,
                               UShort aPageID,
                               UShort aSlotID);

public:
    /*
     * Fixed Table Builder
     */

    static IDE_RC buildRecordForSTATEMENT(idvSQL              * /*aStatistics*/,
                                          void *aHeader,
                                          void *aDumpObj,
                                          iduFixedTableMemory *aMemory);
    static IDE_RC buildRecordForSQLTEXT(idvSQL              * /*aStatistics*/,
                                        void *aHeader,
                                        void *aDumpObj,
                                        iduFixedTableMemory *aMemory);
    static IDE_RC buildRecordForPLANTEXT(idvSQL              * /*aStatistics*/,
                                         void *aHeader,
                                         void *aDumpObj,
                                         iduFixedTableMemory *aMemory);
};


inline iduMemPool *mmcStatementManager::getPlanBufferPool()
{
    return &mPlanBufferPool;
}


#endif
