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
 * $Id: smrFT.h 32652 2009-05-13 02:59:22Z bskim $
 *
 * Description :
 *
 * Backup 관련 Dump
 *
 * X$ARCHIVE
 * X$STABLE_MEM_DATAFILES
 * X$LFG
 * X$LOG
 *
 **********************************************************************/

#ifndef _O_SMR_FT_H_
#define _O_SMR_FT_H_ 1

#include <idu.h>
#include <idp.h>
#include <smrDef.h>
#include <sctTableSpaceMgr.h>

// fixed table related def.
typedef struct smrArchiveInfo
{
    smiArchiveMode mArchiveMode;
    idBool         mArchThrRunning;
    const SChar*   mArchDest;
    UInt           mNextLogFile2Archive;
    /* Archive Directory에서 가장 작은 Logfile No */
    UInt           mOldestActiveLogFile;
    /* 현재 사용중인 Logfile No */
    UInt           mCurrentLogFile;
} smrArchiveInfo;

//added for performance view
typedef struct smrStableMemDataFile
{
    UInt           mSpaceID;
    const SChar*   mDataFileName;
    smuList        mDataFileNameLst;
}smrStableMemDataFile;

//added for LFG Fixed Table
typedef struct smrLFGInfo
{
    // 현재 log를 기록하기 위해 사용하는 logfile No
    UInt          mCurWriteLFNo;

    // 현재 logfile에서 다음 log가 기록될 위치
    UInt          mCurOffset;

    // 현재 Open된 LogFile의 갯수
    UInt          mLFOpenCnt;

    // Log Prepare Count
    UInt          mLFPrepareCnt;

    // log switch 발생시 wait event가 발생한 횟수
    UInt          mLFPrepareWaitCnt;

    // 마지막으로 prepare된 logfile No
    UInt          mLstPrepareLFNo;

    smLSN         mEndLSN;
    UInt          mFstDeleteFileNo;
    UInt          mLstDeleteFileNo;
    smLSN         mResetLSN;

    // Update Transaction의 수
    UInt          mUpdateTxCount;

    // Group Commit 통계치
    // Commit하려는 트랜잭션들이 Log가 Flush되기를 기다린 횟수
    UInt          mGCWaitCount;

    // Commit하려는 트랜잭션들이 Flush하려는 Log의 위치가
    // 이미 Log가 Flush된 것으로 판명되어 빠져나간 횟수
    UInt          mGCAlreadySyncCount;

    // Commit하려는 트랜잭션들이 실제로 Log 를 Flush한 횟수
    UInt          mGCRealSyncCount;
} smrLFGInfo;

class smrFT
{
public:
    // X$ARCHIVE
    static IDE_RC buildRecordForArchiveInfo(idvSQL              * /*aStatistics*/,
                                            void        *aHeader,
                                            void        *aDumpObj,
                                            iduFixedTableMemory *aMemory);

    // X$STABLE_MEM_DATAFILES
    static IDE_RC buildRecordForStableMemDataFiles(idvSQL              * /*aStatistics*/,
                                                   void        *aHeader,
                                                   void        *aDumpObj,
                                                   iduFixedTableMemory *aMemory);

    // X$LFG
    static IDE_RC buildRecordOfLFGForFixedTable(idvSQL              * /*aStatistics*/,
                                                void*  aHeader,
                                                void*  aDumpObj,
                                                iduFixedTableMemory *aMemory);

    // X$LOG
    static IDE_RC buildRecordForLogAnchor(idvSQL              * /*aStatistics*/,
                                          void*  aHeader,
                                          void*  aDumpObj,
                                          iduFixedTableMemory *aMemory);

    // X$RECOVERY_FAIL_OBJ
    static IDE_RC buildRecordForRecvFailObj(idvSQL              * /*aStatistics*/,
                                            void*  aHeader,
                                            void*  /* aDumpObj */,
                                            iduFixedTableMemory *aMemory);
};



#endif /* _O_SMR_FT_H_ */

