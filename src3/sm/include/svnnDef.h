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
 
#ifndef _O_SVNN_DEF_H_
# define _O_SVNN_DEF_H_ 1

# include <idl.h>
# include <idTypes.h>

# include <smc.h>

/* svnnIterator.rows                                      */
# define SVNN_ROWS_CACHE (SM_PAGE_SIZE/ID_SIZEOF(smpSlotHeader))

typedef struct svnnIterator
{
    smSCN               SCN;
    smSCN               infinite;
    void              * trans;
    smcTableHeader    * table;
    SChar             * curRecPtr;
    SChar             * lstFetchRecPtr;
    scGRID              mRowGRID;
    smTID               tid;
    UInt                flag;

    smiCursorProperties  * mProperties;
    smiStatement         * mStatement;
    /* smiIterator 공통 변수 끝 */

    idBool             least;
    idBool             highest;
    const smiCallBack* filter;
    
    scPageID           page;

    /*
     * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
     */
    scPageID           mScanBackPID;
    ULong              mScanBackModifySeq;
    ULong              mModifySeqForScan;
     
    SChar**            slot;
    SChar**            lowFence;
    SChar**            highFence;
    SChar*             rows[SVNN_ROWS_CACHE];
} svnnIterator;

#endif /* _O_SVNN_DEF_H_ */
