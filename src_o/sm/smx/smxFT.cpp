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
 * $Id: smxTransMgr.cpp 37671 2010-01-08 02:28:34Z linkedlist $
 **********************************************************************/

#include <idl.h>
#include <ida.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smr.h>
#include <smxTransMgr.h>
#include <smxTrans.h>
#include <smxTransFreeList.h>
#include <sdc.h>
#include <smxReq.h>
#include <smxFT.h>

/***********************************************************************
 *
 * Description :
 *
 *  smxTransMgr는 모두 static 멤버를 가지기 때문에 FixedTable로
 *  직접 변환할 수 없다. 따라서, 변환을 위한 구조체를 미리
 *  초기화 시킨다.
 *
 **********************************************************************/
typedef struct smxTransMgrStatistics
{
    smxTrans          **mArrTrans;
    UInt               *mTransCnt;
    UInt               *mTransFreeListCnt;
    UInt               *mCurAllocTransFreeList;
    idBool             *mEnabledTransBegin;
    UInt               *mActiveTransCnt;
    smSCN               mSysMinDskViewSCN;
    ULong              *mTransTableFullCount;     // BUG-47655
    ULong              *mAllocRetryTransCount;    // BUG-47655
} smxTransMgrStatistics;

static smxTransMgrStatistics gSmxTransMgrStatistics;

void smxFT::initializeFixedTableArea()
{
    gSmxTransMgrStatistics.mTransCnt              = &smxTransMgr::mTransCnt;
    gSmxTransMgrStatistics.mTransFreeListCnt      = &smxTransMgr::mTransFreeListCnt;
    gSmxTransMgrStatistics.mCurAllocTransFreeList = &smxTransMgr::mCurAllocTransFreeList;
    gSmxTransMgrStatistics.mEnabledTransBegin     = &smxTransMgr::mEnabledTransBegin;
    gSmxTransMgrStatistics.mActiveTransCnt        = &smxTransMgr::mActiveTransCnt;
    gSmxTransMgrStatistics.mTransTableFullCount   = &smxTransMgr::mTransTableFullCount;  // BUG-47655
    gSmxTransMgrStatistics.mAllocRetryTransCount  = &smxTransMgr::mAllocRetryTransCount; // BUG-47655
    SM_INIT_SCN( &gSmxTransMgrStatistics.mSysMinDskViewSCN );
}

/* ------------------------------------------------
 *  Fixed Table Define for smxTransMgr
 * ----------------------------------------------*/
static iduFixedTableColDesc gTxMgrTableColDesc[] =
{
    {
        (SChar*)"TOTAL_COUNT",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mTransCnt),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"FREE_LIST_COUNT",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mTransFreeListCnt),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },


    {
        (SChar*)"BEGIN_ENABLE",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mEnabledTransBegin),
        ID_SIZEOF(smxTransMgr::mEnabledTransBegin),
        IDU_FT_TYPE_UBIGINT | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"ACTIVE_COUNT",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mActiveTransCnt),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"SYS_MIN_DISK_VIEWSCN",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mSysMinDskViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },

    {   /* BUG-47655 Transaction 할당 재시도 총 횟수 */
        (SChar*)"ALLOC_TRANSACTION_RETRY_COUNT",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mTransTableFullCount),
        ID_SIZEOF(smxTransMgr::mTransTableFullCount),
        IDU_FT_TYPE_UBIGINT | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },

    {   /* BUG-47655 Transaction 할당 재시도한 Transaction 갯수 */
        (SChar*)"ALLOC_RETRY_TRANSACTION_COUNT",
        IDU_FT_OFFSETOF(smxTransMgrStatistics, mAllocRetryTransCount),
        ID_SIZEOF(smxTransMgr::mAllocRetryTransCount),
        IDU_FT_TYPE_UBIGINT | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
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


IDE_RC smxFT::buildRecordForTxMgr( idvSQL              * /*aStatistics*/,
                                   void                *aHeader,
                                   void                * /* aDumpObj */,
                                   iduFixedTableMemory * aMemory)
{
    UInt     i;
    UInt     sTotalFreeCount = 0;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    // Calculate Active Tx Count
    for ( i = 0 ; i < (UInt)smxTransMgr::mTransFreeListCnt ; i++ )
    {
        sTotalFreeCount += smxTransMgr::mArrTransFreeList[i].mCurFreeTransCnt;
    }

    smxTransMgr::mActiveTransCnt = smxTransMgr::mTransCnt - sTotalFreeCount;
    SMX_GET_MIN_DISK_VIEW( &gSmxTransMgrStatistics.mSysMinDskViewSCN );

    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                         aMemory,
                                         (void *) &gSmxTransMgrStatistics )
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gTxMgrTableDesc =
{
    (SChar *)"X$TRANSACTION_MANAGER",
    smxFT::buildRecordForTxMgr,
    gTxMgrTableColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  Fixed Table Define for X$TRANSACTIONS 
 * ----------------------------------------------*/

static iduFixedTableColDesc gTxListTableColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mTransID),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    {
        (SChar*)"SESSION_ID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mSessionID),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mSessionID),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_MEM_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mMscn),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_MEM_VIEW_SCN_FOR_LOB",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mMinMemViewSCNwithLOB),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_DISK_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mDscn),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_DISK_VIEW_SCN_FOR_LOB",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mMinDskViewSCNwithLOB),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_MIN_DISK_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mFstDskViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_REQUEST_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLastRequestSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREPARE_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mPrepareSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COMMIT_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mCommitSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATUS",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mStatus),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mStatus),
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UPDATE_STATUS",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mIsUpdate),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mIsUpdate),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOG_TYPE",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLogTypeFlag),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mLogTypeFlag),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
#ifdef NOTDEF
    {
        (SChar*)"XA_ID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mXaTransID),
        256,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
#endif
    {
        (SChar*)"XA_COMMIT_STATUS",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mCommitState),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mCommitState),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"XA_PREPARED_TIME",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mPreparedTime),
        64,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertTIMESTAMP,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_UNDO_NEXT_LSN_FILENO",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mFstUndoNxtLSN) + IDU_FT_OFFSETOF(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_UNDO_NEXT_LSN_OFFSET",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mFstUndoNxtLSN) + IDU_FT_OFFSETOF(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN, mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CURRENT_UNDO_NEXT_SN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mCurUndoNxtSN),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mCurUndoNxtSN),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CURRENT_UNDO_NEXT_LSN_FILENO",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mCurUndoNxtLSN) + IDU_FT_OFFSETOF(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CURRENT_UNDO_NEXT_LSN_OFFSET",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mCurUndoNxtLSN) + IDU_FT_OFFSETOF(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN, mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_UNDO_NEXT_LSN_FILENO",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLstUndoNxtLSN) + IDU_FT_OFFSETOF(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_UNDO_NEXT_LSN_OFFSET",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLstUndoNxtLSN) + IDU_FT_OFFSETOF(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN, mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LAST_UNDO_NEXT_SN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mCurUndoNxtSN),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mCurUndoNxtSN),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SLOT_NO",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mSlotN),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mSlotN),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"UPDATE_SIZE",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mUpdateSize),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mUpdateSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ENABLE_ROLLBACK",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mAbleToRollback),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mAbleToRollback),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_UPDATE_TIME",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mFstUpdateTime),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mFstUpdateTime),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PROCESSED_UNDO_TIME",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mProcessedUndoTime),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mProcessedUndoTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ESTIMATED_TOTAL_UNDO_TIME",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mEstimatedTotalUndoTime),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mEstimatedTotalUndoTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_LOG_COUNT",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mTotalLogCount),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mTotalLogCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PROCESSED_UNDO_LOG_COUNT",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mProcessedUndoLogCount),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mProcessedUndoLogCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOG_BUF_SIZE",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLogBufferSize),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mLogBufferSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOG_OFFSET",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLogOffset),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mLogOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SKIP_CHECK_FLAG",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mDoSkipCheck),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mDoSkipCheck),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SKIP_CHECK_SCN_FLAG",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mDoSkipCheckSCN),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mDoSkipCheckSCN),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DDL_FLAG",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mIsDDL),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mIsDDL),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TSS_RID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mTSSlotSID),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mTSSlotSID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"RESOURCE_GROUP_ID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mRSGroupID),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mRSGroupID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MEM_LOB_CURSOR_COUNT",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mMemLobCursorCount ),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mMemLobCursorCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"DISK_LOB_CURSOR_COUNT",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mDiskLobCursorCount ),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mDiskLobCursorCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LEGACY_TRANS_COUNT",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mLegacyTransCount ),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mLegacyTransCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"ISOLATION_LEVEL",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mIsolationLevel ),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mIsolationLevel ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"GLOBAL_CONSISTENCY",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mIsGCTx ),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mIsGCTx ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar *)"SHARD_PIN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mShardPin ),
        SMI_MAX_SHARD_PIN_STR_LEN,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertShardPinToString,
        0, 0, NULL
    },
    {
        (SChar*)"DISTRIBUTION_FIRST_STMT_TIME",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mGCTxFirstStmtTime.tv_ ),
        64,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertAlignedTIMESTAMP,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"DISTRIBUTION_FIRST_STMT_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mGCTxFirstStmtViewSCN ),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"DISTRIBUTION_LEVEL",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mDistLevel ),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mDistLevel ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"DISTRIBUTION_DEADLOCK_DETECTION",
        IDU_FT_OFFSETOF( smxTransInfo4Perf, mDetectionStr ),
        IDU_FT_SIZEOF( smxTransInfo4Perf, mDetectionStr ),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"DISTRIBUTION_DEADLOCK_WAIT_TIME",
        IDU_FT_OFFSETOF( smxTransInfo4Perf, mDieWaitTime ),
        IDU_FT_SIZEOF( smxTransInfo4Perf, mDieWaitTime ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"DISTRIBUTION_DEADLOCK_ELAPSED_TIME",
        IDU_FT_OFFSETOF( smxTransInfo4Perf, mElapsedTime ),
        IDU_FT_SIZEOF( smxTransInfo4Perf, mElapsedTime ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL /* for internal use */
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

/* smxDistDeadlockDetection to String */
SChar sDetectionString[][64] = { "NONE", "VIEWSCN", "TIME", "SHARD_PIN_SEQ", "SHARD_PIN_NODE_ID", "ALL_EQUAL" };

IDE_RC smxFT::buildRecordForTxList( idvSQL              * /*aStatistics*/,
                                    void                * aHeader,
                                    void                * /* aDumpObj */,
                                    iduFixedTableMemory * aMemory )
{
    ULong              sNeedRecCount;
    ULong              sRedoLogCnt;
    ULong              sUndoLogCnt;
    UInt               i;
    smxTrans         * sTrans;
    smxTransInfo4Perf  sTransInfo;
    void             * sIndexValues[3];

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sNeedRecCount = smxTransMgr::mTransCnt;

    for ( i = 0 ; i < sNeedRecCount ; i++ )
    {
        sTrans = &smxTransMgr::mArrTrans[i];

        /* BUG-23314 V$Transaction에서 Status가 End인 Transaction이 나오면
         * 안됩니다. Status가 END인것은 건너뛰도록 하였습니다. */
        /* BUG-43198 valgrind 경고 수정 */
        if ( sTrans->mStatus4FT != SMX_TX_END )
        {
            /* BUG-43006 FixedTable Indexing Filter
             * Column Index 를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sTrans->mSessionID;
            sIndexValues[1] = &sTrans->mStatus4FT;
            sIndexValues[2] = &sTrans->mFstUpdateTime;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gTxListTableColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            sTransInfo.mTransID        = sTrans->mTransID;
            //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
            //그들간의 관계를 정확히 유지해야 함.
            sTransInfo.mSessionID             = sTrans->mSessionID;
            sTransInfo.mMscn                  = sTrans->mMinMemViewSCN;
            sTransInfo.mDscn                  = sTrans->mMinDskViewSCN;
            sTransInfo.mFstDskViewSCN         = sTrans->mFstDskViewSCN;
            sTransInfo.mLastRequestSCN        = sTrans->mLastRequestSCN;
            sTransInfo.mPrepareSCN            = sTrans->mPrepareSCN;
            sTransInfo.mCommitSCN             = sTrans->mCommitSCN;
            sTransInfo.mStatus                = sTrans->mStatus4FT;
            sTransInfo.mIsUpdate              = sTrans->mIsUpdate;
            sTransInfo.mLogTypeFlag           = sTrans->mLogTypeFlag;
            sTransInfo.mXaTransID             = sTrans->mXaTransID;
            sTransInfo.mCommitState           = sTrans->mCommitState;
            sTransInfo.mPreparedTime          = sTrans->mPreparedTime;
            sTransInfo.mFstUndoNxtLSN         = sTrans->mFstUndoNxtLSN;
            sTransInfo.mCurUndoNxtLSN         = sTrans->mCurUndoNxtLSN;
            sTransInfo.mLstUndoNxtLSN         = sTrans->mLstUndoNxtLSN;
            sTransInfo.mCurUndoNxtSN          = SM_MAKE_SN( sTrans->mCurUndoNxtLSN );
            sTransInfo.mSlotN                 = sTrans->mSlotN;
            sTransInfo.mUpdateSize            = sTrans->mUpdateSize;
            sTransInfo.mAbleToRollback        = ID_TRUE; // dummy
            sTransInfo.mFstUpdateTime         = sTrans->mFstUpdateTime;
            if ( sTrans->mUndoBeginTime == 0 )
            {
                sTransInfo.mProcessedUndoTime = 0;
            }
            else
            {
                sTransInfo.mProcessedUndoTime = smLayerCallback::smiGetCurrentTime()
                                                - sTrans->mUndoBeginTime;
            }
            sTransInfo.mTotalLogCount         = sTrans->mTotalLogCount;
            sTransInfo.mProcessedUndoLogCount = sTrans->mProcessedUndoLogCount;
            /* PROCEED_UNDO_TIME * REDO_CNT / UNDO_CNT */
            sRedoLogCnt = sTrans->mTotalLogCount 
                          - sTrans->mProcessedUndoLogCount ;
            sUndoLogCnt = sTrans->mProcessedUndoLogCount;
            if ( sUndoLogCnt > 0 )
            {
                sTransInfo.mEstimatedTotalUndoTime = 
                             (UInt)( sTransInfo.mProcessedUndoTime * sRedoLogCnt / sUndoLogCnt );
            }
            else
            {
                sTransInfo.mEstimatedTotalUndoTime = 0;
            }
            sTransInfo.mLogBufferSize      = sTrans->mLogBufferSize;
            sTransInfo.mLogOffset          = sTrans->mLogOffset;
            sTransInfo.mDoSkipCheck        = sTrans->mDoSkipCheck;
            sTransInfo.mDoSkipCheckSCN     = sTrans->mDoSkipCheckSCN;
            sTransInfo.mIsDDL              = sTrans->mIsDDL;
            sTransInfo.mTSSlotSID          = smxTrans::getTSSlotSID( sTrans );
            sTransInfo.mRSGroupID          = sTrans->mRSGroupID;
            sTransInfo.mMemLobCursorCount  = sTrans->mMemLCL.getLobCursorCnt( 0, NULL );
            sTransInfo.mDiskLobCursorCount = sTrans->mDiskLCL.getLobCursorCnt( 0, NULL );
            sTransInfo.mLegacyTransCount   = sTrans->mLegacyTransCnt;

            sTrans->getMinMemViewSCNwithLOB( &sTransInfo.mMinMemViewSCNwithLOB );
            sTrans->getMinDskViewSCNwithLOB( &sTransInfo.mMinDskViewSCNwithLOB );

            sTransInfo.mIsolationLevel     = (sTrans->mFlag & SMI_ISOLATION_MASK);

            /* PROJ-2734 */
            sTransInfo.mGCTxFirstStmtTime    = sTrans->mDistTxInfo.mFirstStmtTime;
            sTransInfo.mGCTxFirstStmtViewSCN = sTrans->mDistTxInfo.mFirstStmtViewSCN;
            sTransInfo.mDistLevel            = sTrans->mDistTxInfo.mDistLevel;
            sTransInfo.mShardPin             = sTrans->mDistTxInfo.mShardPin;
            
            sTransInfo.mIsGCTx = sTrans->mIsGCTx;

            idlOS::snprintf( sTransInfo.mDetectionStr,
                             ID_SIZEOF( sTransInfo.mDetectionStr ),
                             "%s",
                             sDetectionString[sTrans->mDistDeadlock4FT.mDetection] );

            if ( sTrans->mDistDeadlock4FT.mDetection != SMX_DIST_DEADLOCK_DETECTION_NONE )
            {
                sTransInfo.mDieWaitTime = sTrans->mDistDeadlock4FT.mDieWaitTime;
                sTransInfo.mElapsedTime = sTrans->mDistDeadlock4FT.mElapsedTime;
            }
            else
            {
                sTransInfo.mDieWaitTime = 0;
                sTransInfo.mElapsedTime = 0;
            }

            IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                                 aMemory,
                                                 (void *) &sTransInfo )
                     != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gTxListTableDesc =
{
    (SChar *)"X$TRANSACTIONS",
    smxFT::buildRecordForTxList,
    gTxListTableColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  Fixed Table Define for DBA_2PC_PENDING
 * ----------------------------------------------*/

static iduFixedTableColDesc gTxPendingTableColDesc[] =
{
    {
        (SChar*)"LOCAL_TRAN_ID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mTransID),
        IDU_FT_SIZEOF(smxTransInfo4Perf, mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"GLOBAL_TX_ID",
        IDU_FT_OFFSETOF(smxTransInfo4Perf, mXaTransID),
        256,
        IDU_FT_TYPE_VARCHAR,
        idaXaConvertXIDToString,
        0, 0,NULL // for internal use
    },
/* prepared 상태의 transaction 만 보여줌
    {
        (SChar*)"STATUS",
        IDU_FT_OFFSETOF(smxTrans, mCommitState),
        IDU_FT_SIZEOF(smxTrans, mCommitState),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
*/
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

IDE_RC smxFT::buildRecordForTxPending( idvSQL              * /*aStatistics*/,
                                       void                * aHeader,
                                       void                * /* aDumpObj */,
                                       iduFixedTableMemory * aMemory )
{
    ULong                sNeedRecCount;
    UInt                 i;
    smxTransInfo4Perf    sTransInfo;
    smxTrans            *sTrans;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sNeedRecCount = smxTransMgr::mTransCnt;

    for (i = 0; i < sNeedRecCount; i++)
    {
        sTrans = &smxTransMgr::mArrTrans[i];

        if ( sTrans->isPrepared() == ID_TRUE )
        {
            sTransInfo.mTransID   = sTrans->mTransID;
            sTransInfo.mXaTransID = sTrans->mXaTransID;

            IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                                 aMemory,
                                                 (void *) &sTransInfo )
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gTxPendingTableDesc =
{
    (SChar *)"X$TXPENDING",
    smxFT::buildRecordForTxPending,
    gTxPendingTableColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  Fixed Table Define for X$ACTIVE_TXSEGS
 * ----------------------------------------------*/
static iduFixedTableColDesc gActiveTXSEGSTableColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf, mEntryID),
        IDU_FT_SIZEOF(smxTXSeg4Perf, mEntryID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TRANS_ID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf, mTransID),
        IDU_FT_SIZEOF(smxTXSeg4Perf, mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_DISK_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTXSeg4Perf, mMinDskViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COMMIT_SCN",
        IDU_FT_OFFSETOF(smxTXSeg4Perf, mCommitSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_DISK_VIEW_SCN",
        IDU_FT_OFFSETOF(smxTXSeg4Perf, mFstDskViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TSSLOT_SID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mTSSlotSID),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mTSSlotSID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TSSEG_EXTDESC_RID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mExtRID4TSS),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mExtRID4TSS),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_UDSEG_EXTENT_RID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mFstExtRID4UDS),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mFstExtRID4UDS),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LST_UDSEG_EXTENT_RID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mLstExtRID4UDS),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mLstExtRID4UDS),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_UNDO_PAGEID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mFstUndoPID),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mFstUndoPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FST_UNDO_SLOTNUM",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mFstUndoSlotNum),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mFstUndoSlotNum),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LST_UNDO_PAGEID",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mLstUndoPID),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mLstUndoPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LST_UNDO_SLOTNUM",
        IDU_FT_OFFSETOF(smxTXSeg4Perf,mLstUndoSlotNum),
        IDU_FT_SIZEOF(smxTXSeg4Perf,mLstUndoSlotNum),
        IDU_FT_TYPE_USMALLINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc gActiveTXSEGSTableDesc =
{
    (SChar *)"X$ACTIVE_TXSEGS",
    smxFT::buildRecordForActiveTXSEGS,
    gActiveTXSEGSTableColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC smxFT::buildRecordForActiveTXSEGS( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * /* aDumpObj */,
                                          iduFixedTableMemory * aMemory)
{
    UInt             i;
    smxTrans       * sTrans;
    smxTXSeg4Perf    sTXSegInfo;
    sdcTXSegEntry  * sTXSegEntry;
    ULong            sNeedRecCount = smxTransMgr::mTransCnt;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    for (i = 0; i < sNeedRecCount; i++)
    {
        sTrans      = &smxTransMgr::mArrTrans[i];
        sTXSegEntry = sTrans->getTXSegEntry();

        if ( sTXSegEntry == NULL )
        {
            continue;
        }

        sTXSegInfo.mEntryID        = sTXSegEntry->mEntryIdx;
        sTXSegInfo.mTransID        = sTrans->mTransID;
        sTXSegInfo.mMinDskViewSCN  = sTrans->mMinDskViewSCN;
        sTXSegInfo.mCommitSCN      = sTrans->mCommitSCN;
        sTXSegInfo.mFstDskViewSCN  = sTrans->mFstDskViewSCN;
        sTXSegInfo.mTSSlotSID      = sTXSegEntry->mTSSlotSID;
        sTXSegInfo.mExtRID4TSS     = sTXSegEntry->mExtRID4TSS;
        sTXSegInfo.mFstExtRID4UDS  = sTXSegEntry->mFstExtRID4UDS;
        sTXSegInfo.mLstExtRID4UDS  = sTXSegEntry->mLstExtRID4UDS;
        sTXSegInfo.mFstUndoPID     = sTXSegEntry->mFstUndoPID;
        sTXSegInfo.mFstUndoSlotNum = sTXSegEntry->mFstUndoSlotNum;
        sTXSegInfo.mLstUndoPID     = sTXSegEntry->mLstUndoPID;
        sTXSegInfo.mLstUndoSlotNum = sTXSegEntry->mLstUndoSlotNum;

        IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                             aMemory,
                                             (void *) &sTXSegInfo )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Fixed Table Define for X$LEGACY_TRANSACTIONS
 * ----------------------------------------------*/

IDE_RC smxFT::buildRecordForLegacyTxList( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * /* aDumpObj */,
                                          iduFixedTableMemory * aMemory )
{
    smxLegacyTrans * sLegacyTrans;
    smuList        * sCurLegacyTransNode;

    smxLegacyTransMgr::lockLTL();

    SMU_LIST_ITERATE( &(smxLegacyTransMgr::mHeadLegacyTrans), sCurLegacyTransNode )
    {
        sLegacyTrans = (smxLegacyTrans*)sCurLegacyTransNode->mData;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)sLegacyTrans )
                  != IDE_SUCCESS);
    }

    smxLegacyTransMgr::unlockLTL();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc gLegacyTxListTableColDesc[] =
{
    {
        (SChar*)"TransID",
        IDU_FT_OFFSETOF(smxLegacyTrans, mTransID),
        IDU_FT_SIZEOF(smxLegacyTrans, mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COMMIT_END_LSN_FILENO",
        IDU_FT_OFFSETOF(smxLegacyTrans, mCommitEndLSN) + IDU_FT_OFFSETOF(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COMMIT_END_LSN_OFFSET",
        IDU_FT_OFFSETOF(smxLegacyTrans, mCommitEndLSN) + IDU_FT_OFFSETOF(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN, mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"COMMIT_SCN",
        IDU_FT_OFFSETOF(smxLegacyTrans, mCommitSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_MEM_VIEW_SCN",
        IDU_FT_OFFSETOF(smxLegacyTrans, mMinMemViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"MIN_DISK_VIEW_SCN",
        IDU_FT_OFFSETOF(smxLegacyTrans, mMinDskViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FIRST_DISK_VIEW_SCN",
        IDU_FT_OFFSETOF(smxLegacyTrans, mFstDskViewSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"IS_MADE_BY_COMMIT/ROLLBACK",
        IDU_FT_OFFSETOF( smxLegacyTrans, mMadeType ),
        IDU_FT_SIZEOF( smxLegacyTrans, mMadeType ),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


iduFixedTableDesc gLegacyTxListTableDesc =
{
    (SChar *)"X$LEGACY_TRANSACTIONS",
    smxFT::buildRecordForLegacyTxList,
    gLegacyTxListTableColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  Fixed Table Define for X$TIME_SCN 
 * ----------------------------------------------*/

typedef struct smxTimeSCN4Perf
{
    timeval mTime;
    smSCN   mSystemSCN;
    SChar   mBase;
} smxTimeSCN4Perf;

static iduFixedTableColDesc gTimeSCNListTableColDesc[] =
{
    {
        (SChar*)"TIME",
        IDU_FT_OFFSETOF(smxTimeSCN4Perf, mTime ),
        32,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertAlignedTIMESTAMP,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"SYSTEM_SCN",
        IDU_FT_OFFSETOF(smxTimeSCN4Perf, mSystemSCN),
        29,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertSCN,
        0, 0,NULL /* for internal use */
    },
    {
        (SChar*)"BASE",
        IDU_FT_OFFSETOF(smxTimeSCN4Perf, mBase),
        IDU_FT_SIZEOF(smxTimeSCN4Perf, mBase),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL /* for internal use */
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL /* for internal use */
    }
};

iduFixedTableDesc gTimeSCNListTableDesc =
{
    (SChar *)"X$TIME_SCN",
    smxFT::buildRecordForTimeSCNList,
    gTimeSCNListTableColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC smxFT::buildRecordForTimeSCNList( idvSQL              * /*aStatistics*/,
                                         void                * aHeader,
                                         void                * /* aDumpObj */,
                                         iduFixedTableMemory * aMemory )
{
    smxTimeSCN4Perf  sTimeSCNInfo;
    SInt             i;
    SInt             sBaseIdx;
    SInt             sLastIdx;
    smxTimeSCNNode * sTimeSCNList = NULL;
 
    if ( smxTransMgr::isActiveVersioningMinTime() == ID_FALSE )
    {
        IDE_CONT( SKIP_TIME_SCN_LIST );
    }

    sBaseIdx = smxTransMgr::getTimeSCNBaseIdx();
    sLastIdx = smxTransMgr::getTimeSCNLastIdx();
    
    if ( ( sBaseIdx < 0 ) ||
         ( sLastIdx < 0 ) )
    {
        /* 아직 TIME-LIST가 준비되지 않았다. SKIP 한다. */
        IDE_CONT( SKIP_TIME_SCN_LIST );
    }

    IDE_DASSERT( sBaseIdx < smxTransMgr::getTimeSCNMaxCnt() );
    IDE_DASSERT( sLastIdx < smxTransMgr::getTimeSCNMaxCnt() );

    smxTransMgr::getTimeSCNList( &sTimeSCNList );

    i = sBaseIdx;

    while ( 1 )
    {
        sTimeSCNInfo.mTime       = (timeval)(sTimeSCNList[i].mTime);
        sTimeSCNInfo.mSystemSCN  = sTimeSCNList[i].mSCN;

        if ( i == sBaseIdx )
        {
            sTimeSCNInfo.mBase = 'Y';
        }
        else
        {
            sTimeSCNInfo.mBase = 'N';
        }

        if ( SM_SCN_IS_INIT( sTimeSCNList[i].mSCN ) )
        {
            /* 조회중에 TIME-SCN LIST가 클리어된 경우이다.
               탐색 중지한다. */
            break;
        }

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sTimeSCNInfo )
                  != IDE_SUCCESS );

        if ( i == sLastIdx )
        {
            break;
        }

        i = ( (i + 1) %  smxTransMgr::getTimeSCNMaxCnt() );
    }

    IDE_EXCEPTION_CONT( SKIP_TIME_SCN_LIST );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Fixed Table Define for X$PENDING_WAIT
 * ----------------------------------------------*/

typedef struct smxPendingWait4Perf
{
    smTID       mWaitingTID;         
    smiShardPin mWaitingTxShardPin;
    smTID       mPendingTID;
    ID_XID      mPendingXID;
    smiShardPin mPendingTxShardPin;

} smlLockWaitStat;


static iduFixedTableColDesc   gPendingWaitColDesc[]=
{

    {
        (SChar*)"TRANS_ID",
        offsetof(smxPendingWait4Perf,mWaitingTID),
        IDU_FT_SIZEOF(smxPendingWait4Perf,mWaitingTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SHARD_PIN",
        IDU_FT_OFFSETOF(smxPendingWait4Perf, mWaitingTxShardPin),
        SMI_MAX_SHARD_PIN_STR_LEN,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertShardPinToString,
        0, 0, NULL
    },
    {
        (SChar*)"WAIT_FOR_TRANS_ID",
        offsetof(smxPendingWait4Perf,mPendingTID),
        IDU_FT_SIZEOF(smxPendingWait4Perf,mPendingTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"WAIT_FOR_XID",
        IDU_FT_OFFSETOF(smxPendingWait4Perf, mPendingXID),
        SMI_XID_STRING_LEN,
        IDU_FT_TYPE_VARCHAR,
        idaXaConvertXIDToString,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"WAIT_FOR_SHARD_PIN",
        IDU_FT_OFFSETOF(smxPendingWait4Perf, mPendingTxShardPin),
        SMI_MAX_SHARD_PIN_STR_LEN,
        IDU_FT_TYPE_VARCHAR,
        smiFixedTable::convertShardPinToString,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc gPendingWaitTableDesc =
{
    (SChar *)"X$PENDING_WAIT",
    smxFT::buildRecordForPendingWait,
    gPendingWaitColDesc,
    IDU_STARTUP_SERVICE,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC smxFT::buildRecordForPendingWait( idvSQL              * /*aStatistics*/,
                                         void                * aHeader,
                                         void                * /* aDumpObj */,
                                         iduFixedTableMemory * aMemory )
{
    SInt       i;
    SInt       j;
    smTID      sWaitingTID;         
    smTID      sPendingTID;
    smxTrans * sPendingTrans = NULL;
    smxTrans * sWaitingTrans = NULL;

    smxPendingWait4Perf sTxInfo;

    SInt   sTransCnt = smxTransMgr::getCurTransCnt();

    for ( j = 0; j < sTransCnt; j++ )
    {
        if ( smxTransMgr::isActiveBySID(j) == ID_TRUE )
        {
            for ( i = 0; i < sTransCnt; i++ )
            {
                if ( smxTransMgr::mPendingWait[j][i] == 1 )
                {
                    sPendingTID         = smxTransMgr::getTIDBySID(i);
                    sTxInfo.mPendingTID = sPendingTID;
                    sPendingTrans       = smxTransMgr::getTransByTID(sPendingTID);
                    sTxInfo.mPendingXID = sPendingTrans->mXaTransID;
                    sTxInfo.mPendingTxShardPin = sPendingTrans->mDistTxInfo.mShardPin;

                    sWaitingTID         = smxTransMgr::getTIDBySID(j);
                    sTxInfo.mWaitingTID = sWaitingTID;
                    sWaitingTrans       = smxTransMgr::getTransByTID(sWaitingTID);
                    sTxInfo.mWaitingTxShardPin = sWaitingTrans->mDistTxInfo.mShardPin;

                    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                          aMemory,
                                                          (void *)&sTxInfo )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
