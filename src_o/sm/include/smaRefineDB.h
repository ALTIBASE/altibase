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
 * $Id: smaRefineDB.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMA_REFINE_DB_H_
#define _O_SMA_REFINE_DB_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smx.h>
#include <smc.h>
#include <smn.h>
#include <smiDef.h>

// It is added because of parallel refine table
typedef struct smapRebuildJobItem
{
    struct smapRebuildJobItem  *mNext;
    struct smapRebuildJobItem  *mPrev;

    smnIndexHeader*      mRebuildIndexList[SMC_INDEX_TYPE_COUNT];

    idBool               mFinished;
    idBool               mFirst;
    UInt                 mJobProcessCnt;
    smcTableHeader      *mTable;

    UInt                 mIdx;
    smpPageListEntry    *mTargetPageList;
    scPageID             mPidLstAllocPage;

    smpPageListEntry     mFixed;
    smpPageListEntry     mVar[SM_VAR_PAGE_LIST_COUNT];
} smapRebuildJobItem;


class smaRefineDB
{
public:
    static IDE_RC refineCatalogTableVarPage( smxTrans       * aTrans,
                                             smcTableHeader * aHeader );

    static IDE_RC refineCatalogTableFixedPage( smxTrans       * aTrans,
                                               smcTableHeader * aHeader );

    static IDE_RC refineTempCatalogTable(smxTrans       * aTrans,
                                         smcTableHeader * aHeader);

    //For Parallel Refine Table
    static IDE_RC refineTable(smxTrans               * aTrans,
                              smapRebuildJobItem      * aJobItem);

    static IDE_RC freeTableHdr( smxTrans       * aTrans,
                                smcTableHeader * aTableHdr,
                                ULong            aSlotFlag,
                                SChar          * aNxtPtr );

    /*
     * BUG-24518 [MDB] Shutdown Phase에서 메모리 테이블 Compaction이 필요합니다.
     */
    static IDE_RC compactTables( );

    /* PROJ-1594 Volatile TBS */
    /* 모든 volatile table들을 초기화한다. */
    static IDE_RC initAllVolatileTables();

private:

    // Catalog Table의  Slot을 Free하고 Free Slot List에 매단다.
    static IDE_RC freeCatalogSlot( smxTrans  * aTrans,
                                   SChar     * aSlotPtr );

};

#endif
