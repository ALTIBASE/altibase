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
 
#ifndef _O_SVM_FT_H_
#define _O_SVM_FT_H_ 1

#include <smDef.h>
#include <svmDef.h>

/*----------------------------------------
 * D$VOL_TBS_PCH
 *----------------------------------------- */

/* TASK-4007 [SM] PBT를 위한 기능 추가
 * PCH를 Dump할 수 있는 기능 추가 */

typedef struct svmVolTBSPCHDump
{
    scSpaceID  mSpaceID;
    scPageID   mMyPageID;
    vULong     mPage;
    scPageID   mNxtScanPID;
    scPageID   mPrvScanPID;
    ULong      mModifySeqForScan;
} svmVolTBSPCHDump;

/* ------------------------------------------------
 *  Fixed Table Define for X$VOL_TABLESPACE_DESC
 * ----------------------------------------------*/

typedef struct svmPerfTBSDesc
{
    UInt       mSpaceID;
    SChar      mSpaceName[SMI_MAX_TABLESPACE_NAME_LEN + 1];
    UInt       mSpaceStatus;
    UInt       mAutoExtendMode;
    ULong      mInitSize;
    ULong      mNextSize;
    ULong      mMaxSize;
    ULong      mCurrentSize;
} svmPerfTBSDesc ;


/* X$VOL_TABLESPACE_FREE_PAGE_LIST */
typedef struct svmPerfVolTBSFreePageList
{
    UInt       mSpaceID;
    UInt       mResourceGroupID ;
    scPageID   mFirstFreePageID ;   // 첫번째 Free Page 의 ID
    ULong      mFreePageCount ;     // Free Page 수
    UInt       mReservedPageCount ; // BUG-31881 예약한 Page수
} svmPerfVolTBSFreePageList ;


#endif // _O_SVM_FT_H_
