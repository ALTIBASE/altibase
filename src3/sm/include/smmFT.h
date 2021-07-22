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
 * $Id: smmFT.h 37235 2009-12-09 01:56:06Z cgkim $
 **********************************************************************/

#ifndef _O_SMM_FT_H_
#define _O_SMM_FT_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smmDef.h>

// X$MEMBASE
typedef struct smmMemBaseFT
{
    scSpaceID  mSpaceID;
    smmMemBase mMemBase;
} smmMemBaseFT;

// X$MEMBASEMGR
typedef struct smmTBSStatistics
{
    UInt                  mSpaceID;
    smmDBRestoreType    * mRestoreType;
    SInt                * m_currentDB;
    smSCN               * m_lstSystemSCN;
    ULong               * mDBMaxPageCount;
    ULong               * mHighLimitFile;
    ULong                 mPageCountPerFile;
    UInt                * m_nDBPageCountInDisk;
    UInt                  mMaxAccessFileSize;
    UInt                  mPageSize;
}smmTBSStatistics;

// X$MEM_TABLESPACE_FREE_PAGE_LIST
typedef struct smmPerfMemTBSFreePageList
{
    UInt       mSpaceID;
    UInt       mResourceGroupID ;
    scPageID   mFirstFreePageID ;   // 첫번째 Free Page 의 ID
    ULong      mFreePageCount ;     // Free Page 수
    UInt       mReservedPageCount ; // BUG-31881 예약한 Page수
} smmPerfMemTBSFreePageList ;

/*----------------------------------------
 * D$MEM_TBS_PCH
 *----------------------------------------- */

/* TASK-4007 [SM] PBT를 위한 기능 추가
 * PCH를 Dump할 수 있는 기능 추가 */

typedef struct smmMemTBSPCHDump
{
    scSpaceID  mSpaceID;
    scPageID   mMyPageID;
    vULong     mPage;
    SChar      mDirty;
    UInt       mDirtyStat;
    scPageID   mNxtScanPID;
    scPageID   mPrvScanPID;
    ULong      mModifySeqForScan;
} smmMemTBSPCHDump;

/* ------------------------------------------------
 *  Fixed Table Define for X$MEM_TABLESPACE_DESC
 * ----------------------------------------------*/

typedef struct smmPerfTBSDesc
{
    UInt       mSpaceID;
    SChar      mSpaceName[SMI_MAX_TABLESPACE_NAME_LEN + 1];
    UInt       mSpaceStatus;
    UInt       mSpaceShmKey; 
    UInt       mAutoExtendMode;
    ULong      mAutoExtendNextSize;
    ULong      mMaxSize;
    ULong      mCurrentSize;
} smmPerfTBSDesc ;

/* ------------------------------------------------
 *  Fixed Table Define for X$MEM_TABLESPACE_CHECKPOINT_PATHS
 * ----------------------------------------------*/

typedef struct smmPerfCheckpointPath
{
    UInt       mSpaceID;
    SChar      mCheckpointPath[SMI_MAX_CHKPT_PATH_NAME_LEN + 1];
} smmPerfCheckpointPath ;

/* -----------------------------------------------------
 *  Fixed Table Define for X$MEM_TABLESPACE_STATUS_DESC
 * -----------------------------------------------------*/

#define SMM_MAX_TBS_STATUS_DESC_LEN (64)
typedef struct smmPerfTBSStatusDesc
{
    UInt       mStatus;
    SChar      mStatusDesc[SMM_MAX_TBS_STATUS_DESC_LEN+1];
} smmPerfTBSStatusDesc ;


typedef struct smmTBSStatusDesc
{
    UInt       mStatus;
    SChar    * mStatusDesc;
} smmTBSStatusDesc ;

#endif // _O_SMM_FT_H_

