/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id$
 **********************************************************************/
#ifndef _O_IDV_TYPE_H_
#define _O_IDV_TYPE_H_ 1

#include <idvTime.h>
/* ------------------------------------------------
 *  경과시간 측정할 연산 타입
 * ----------------------------------------------*/
typedef enum idvOperTimeIndex
{
    IDV_OPTM_INDEX_BEGIN = 0,

    IDV_OPTM_INDEX_QUERY_PARSE = IDV_OPTM_INDEX_BEGIN,
    IDV_OPTM_INDEX_QUERY_VALIDATE,
    IDV_OPTM_INDEX_QUERY_OPTIMIZE,
    IDV_OPTM_INDEX_QUERY_EXECUTE,
    IDV_OPTM_INDEX_QUERY_FETCH,
    //PROJ-1436
    IDV_OPTM_INDEX_QUERY_SOFT_PREPARE,
    IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES,
    IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE,
    IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT,
    IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD,
    IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS,
    IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE,
    IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER,
    IDV_OPTM_INDEX_DRDB_CHAIN_INDEX_TSS,
    IDV_OPTM_INDEX_DRDB_CREATE_PAGE,
    IDV_OPTM_INDEX_DRDB_GET_PAGE,
    IDV_OPTM_INDEX_DRDB_FIX_PAGE,
    IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING,
    IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING,
    IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE,
    IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE,
    /*fix BUG-30855 It needs to describe soft prepare time in detail
      for problem tracking . */
    IDV_OPTM_INDEX_PLAN_HARD_REBUILD,
    IDV_OPTM_INDEX_SOFT_REBUILD,
    IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_PREPARE,
    IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_REBUILD,
    IDV_OPTM_INDEX_PLANCACHE_SEARCH_PPCO,
    IDV_OPTM_INDEX_PLANCACHE_CREATE_PPCO,
    IDV_OPTM_INDEX_PLANCACHE_SEARCH_CHILD_PCO,
    IDV_OPTM_INDEX_PLANCACHE_CREATE_CHILD_PCO,
    IDV_OPTM_INDEX_VALIDATE_PCO,
    IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC,
    IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE,
    IDV_OPTM_INDEX_HARD_PREPARE,
    IDV_OPTM_INDEX_MATCHING_CHILD_PCO,
    IDV_OPTM_INDEX_WAITING_HARD_PREPARE,
    IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT,
    IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO,
    IDV_OPTM_INDEX_CHECK_PRIVILEGE_PCO,
    /* fix BUG-31545 replication statistics */
    IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER,
    IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG,
    IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER,
    IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE,
    IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG,
    IDV_OPTM_INDEX_RP_S_LOG_ANALYZE,
    IDV_OPTM_INDEX_RP_S_SEND_XLOG,
    IDV_OPTM_INDEX_RP_S_RECV_ACK,
    IDV_OPTM_INDEX_RP_S_SET_ACKEDVALUE,
    IDV_OPTM_INDEX_RP_R_RECV_XLOG,
    IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN,
    IDV_OPTM_INDEX_RP_R_TX_BEGIN,
    IDV_OPTM_INDEX_RP_R_TX_COMMIT,
    IDV_OPTM_INDEX_RP_R_TX_ABORT,
    IDV_OPTM_INDEX_RP_R_TABLE_CURSOR_OPEN,
    IDV_OPTM_INDEX_RP_R_TABLE_CURSOR_CLOSE,
    IDV_OPTM_INDEX_RP_R_INSERT_ROW,
    IDV_OPTM_INDEX_RP_R_UPDATE_ROW,
    IDV_OPTM_INDEX_RP_R_DELETE_ROW,
    IDV_OPTM_INDEX_RP_R_OPEN_LOB_CURSOR,
    IDV_OPTM_INDEX_RP_R_PREPARE_LOB_WRITE,
    IDV_OPTM_INDEX_RP_R_WRITE_LOB_PIECE,
    IDV_OPTM_INDEX_RP_R_FINISH_LOB_WRITE,
    IDV_OPTM_INDEX_RP_R_CLOSE_LOB_CURSOR,
    IDV_OPTM_INDEX_RP_R_COMPARE_IMAGE,
    IDV_OPTM_INDEX_RP_R_SEND_ACK,
    IDV_OPTM_INDEX_RP_R_TRIM_LOB,
    IDV_OPTM_INDEX_TASK_SCHEDULE, /* bug-35395 */
    IDV_OPTM_INDEX_TASK_SCHEDULE_MAX,

    IDV_OPTM_INDEX_MAX,
    IDV_OPTM_INDEX_NULL = IDV_OPTM_INDEX_MAX

} idvOperTimeIndex;


/* ------------------------------------------------
 *  Wait Event의 Wait Class
 * ----------------------------------------------*/
typedef enum idvWaitClassIndex
{
    IDV_WCLASS_INDEX_BEGIN = 0,

    IDV_WCLASS_INDEX_OTHER = IDV_WCLASS_INDEX_BEGIN,
    IDV_WCLASS_INDEX_ADMINISTRATIVE,
    IDV_WCLASS_INDEX_CONFIGURATION,
    IDV_WCLASS_INDEX_CONCURRENCY,
    IDV_WCLASS_INDEX_COMMIT,
    IDV_WCLASS_INDEX_IDLE,
    IDV_WCLASS_INDEX_USER_IO,
    IDV_WCLASS_INDEX_SYSTEM_IO,
    IDV_WCLASS_INDEX_RP,
    IDV_WCLASS_INDEX_ID_SYSTEM,
    IDV_WCLASS_INDEX_MAX

} idvWaitClassIndex;


/* ------------------------------------------------
 *  하나의 Session에 대한 통계 정보
 * ----------------------------------------------*/

typedef enum idvStatIndex
{
    IDV_STAT_INDEX_BEGIN = 0,
    IDV_STAT_INDEX_LOGON_CURR = IDV_STAT_INDEX_BEGIN,

    IDV_STAT_INDEX_LOGON_CUMUL,
    IDV_STAT_INDEX_READ_PAGE,
    IDV_STAT_INDEX_WRITE_PAGE,
    IDV_STAT_INDEX_GET_PAGE,
    IDV_STAT_INDEX_FIX_PAGE,
    IDV_STAT_INDEX_CREATE_PAGE,
    IDV_STAT_INDEX_UNDO_READ_PAGE,
    IDV_STAT_INDEX_UNDO_WRITE_PAGE,
    IDV_STAT_INDEX_UNDO_GET_PAGE,
    IDV_STAT_INDEX_UNDO_FIX_PAGE,
    IDV_STAT_INDEX_UNDO_CREATE_PAGE,

    IDV_STAT_INDEX_DETECTOR_BASE_TIME,

    IDV_STAT_INDEX_QUERY_TIMEOUT_COUNT,
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    IDV_STAT_INDEX_DDL_TIMEOUT_COUNT,
    IDV_STAT_INDEX_IDLE_TIMEOUT_COUNT,
    IDV_STAT_INDEX_FETCH_TIMEOUT_COUNT,
    IDV_STAT_INDEX_UTRANS_TIMEOUT_COUNT,
    IDV_STAT_INDEX_SESSION_TERMINATED_COUNT,

    /* PROJ-2677 */
    IDV_STAT_INDEX_DDL_SYNC_TIMEOUT_COUNT,


    /* BUG-24151: [SC] Update Retry, Delete Retry, Statement Rebuild Count를
     *            AWI로 추가해야 합니다.*/
    IDV_STAT_INDEX_STMT_REBUILD_COUNT,
    IDV_STAT_INDEX_UNIQUE_VIOLATION_COUNT,
    IDV_STAT_INDEX_UPDATE_RETRY_COUNT,
    IDV_STAT_INDEX_DELETE_RETRY_COUNT,
    IDV_STAT_INDEX_LOCKROW_RETRY_COUNT,

    IDV_STAT_INDEX_COMMIT_COUNT,
    IDV_STAT_INDEX_ROLLBACK_COUNT,
    IDV_STAT_INDEX_FETCH_SUCCESS_COUNT,
    IDV_STAT_INDEX_FETCH_FAILURE_COUNT,
    IDV_STAT_INDEX_EXECUTE_SUCCESS_COUNT,

    /* BUG-39352 Add each execute success count for select, insert, delete,
     * update into V$SYSSTAT */
    IDV_STAT_INDEX_EXECUTE_INSERT_SUCCESS_COUNT,
    IDV_STAT_INDEX_EXECUTE_UPDATE_SUCCESS_COUNT,
    IDV_STAT_INDEX_EXECUTE_DELETE_SUCCESS_COUNT,
    IDV_STAT_INDEX_EXECUTE_SELECT_SUCCESS_COUNT,
    IDV_STAT_INDEX_EXECUTE_REPL_INSERT_SUCCESS_COUNT,
    IDV_STAT_INDEX_EXECUTE_REPL_UPDATE_SUCCESS_COUNT,
    IDV_STAT_INDEX_EXECUTE_REPL_DELETE_SUCCESS_COUNT,

    IDV_STAT_INDEX_EXECUTE_FAILURE_COUNT,
    IDV_STAT_INDEX_PREPARE_SUCCESS_COUNT,
    IDV_STAT_INDEX_PREPARE_FAILURE_COUNT,
    IDV_STAT_INDEX_REBUILD_COUNT,
    IDV_STAT_INDEX_REDOLOG_COUNT,
    IDV_STAT_INDEX_REDOLOG_SIZE,

    IDV_STAT_INDEX_RECV_SOCKET_COUNT,
    IDV_STAT_INDEX_SEND_SOCKET_COUNT,
    
    IDV_STAT_INDEX_RECV_TCP_BYTE,
    IDV_STAT_INDEX_SEND_TCP_BYTE,

    IDV_STAT_INDEX_RECV_UNIX_BYTE,
    IDV_STAT_INDEX_SEND_UNIX_BYTE,

    IDV_STAT_INDEX_RECV_IPC_BLOCK_COUNT,
    IDV_STAT_INDEX_SEND_IPC_BLOCK_COUNT,

    IDV_STAT_INDEX_MEM_CURSOR_SEQ_SCAN,
    IDV_STAT_INDEX_MEM_CURSOR_IDX_SCAN,
    IDV_STAT_INDEX_MEM_CURSOR_GRID_SCAN,
    IDV_STAT_INDEX_DISK_CURSOR_SEQ_SCAN,
    IDV_STAT_INDEX_DISK_CURSOR_IDX_SCAN,
    IDV_STAT_INDEX_DISK_CURSOR_GRID_SCAN,

    IDV_STAT_INDEX_LOCK_ACQUIRED,
    IDV_STAT_INDEX_LOCK_RELEASED,

    IDV_STAT_INDEX_SERVICE_THREAD_CREATED,

    IDV_STAT_INDEX_MEMORY_TABLE_ACCESS_COUNT,

    IDV_STAT_INDEX_PLAN_CACHE_PPCO_MISS_X_TRY_LATCH_COUNT, /* BUG-35631 */

    /* PROJ-2681 */
    IDV_STAT_INDEX_RECV_IB_COUNT,
    IDV_STAT_INDEX_SEND_IB_COUNT,

    IDV_STAT_INDEX_RECV_IB_BYTE,
    IDV_STAT_INDEX_SEND_IB_BYTE,

    // stmt에서 session 으로 누적되는 통계정보 추가
    // 물론, system 통계정보에도 누적된다.
    IDV_STAT_INDEX_BEGIN_STMT_TO_SESS,

    IDV_STAT_INDEX_OPTM_QUERY_PARSE = IDV_STAT_INDEX_BEGIN_STMT_TO_SESS,
    IDV_STAT_INDEX_OPTM_QUERY_VALIDATE,
    IDV_STAT_INDEX_OPTM_QUERY_OPTIMIZE,
    IDV_STAT_INDEX_OPTM_QUERY_EXECUTE,
    IDV_STAT_INDEX_OPTM_QUERY_FETCH,
    //PROJ-1436
    IDV_STAT_INDEX_QUERY_SOFT_PREPARE,
    IDV_STAT_INDEX_OPTM_DRDB_DML_ANALYZE_VALUES,
    IDV_STAT_INDEX_OPTM_DRDB_DML_RECORD_LOCK_VALIDATE,
    IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_SLOT,
    IDV_STAT_INDEX_OPTM_DRDB_DML_WRITE_UNDO_RECORD,
    IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_TSS,
    IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_UNDO_PAGE,
    IDV_STAT_INDEX_OPTM_DRDB_DML_INDEX_OPER,
    IDV_STAT_INDEX_OPTM_DRDB_CREATE_PAGE,
    IDV_STAT_INDEX_OPTM_DRDB_GET_PAGE,
    IDV_STAT_INDEX_OPTM_DRDB_FIX_PAGE,
    IDV_STAT_INDEX_OPTM_DRDB_TRANS_LOGICAL_AGING,
    IDV_STAT_INDEX_OPTM_DRDB_TRANS_PHYSICAL_AGING,

    IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE,
    IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_VICTIM_FREE,
    /*fix BUG-30855 It needs to describe soft prepare time in detail
      for problem tracking . */
    IDV_STAT_INDEX_OPTM_PLAN_HARD_REBUILD,
    IDV_STAT_INDEX_OPTM_SOFT_REBUILD,
    IDV_STAT_INDEX_OPTM_PLANCACHE_CHECK_IN_BY_HARD_PREPARE,
    IDV_STAT_INDEX_OPTM_PLANCACHE_CHECK_IN_BY_HARD_REBUILD,
    IDV_STAT_INDEX_OPTM_PLANCACHE_SEARCH_PPCO,
    IDV_STAT_INDEX_OPTM_PLANCACHE_CREATE_PPCO,
    IDV_STAT_INDEX_OPTM_PLANCACHE_SEARCH_CHILD_PCO,
    IDV_STAT_INDEX_OPTM_PLANCACHE_CREATE_CHILD_PCO,
    IDV_STAT_INDEX_OPTM_VALIDATE_PCO,
    IDV_STAT_INDEX_OPTM_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC,
    IDV_STAT_INDEX_OPTM_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE,
    IDV_STAT_INDEX_OPTM_HARD_PREPARE,
    IDV_STAT_INDEX_OPTM_MATCHING_CHILD_PCO,
    IDV_STAT_INDEX_OPTM_WAITING_HARD_PREPARE,
    IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT,
    IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO,
    IDV_STAT_INDEX_OPTM_INDEX_CHECK_PRIVILEGE_PCO,

    /* fix BUG-31545 replication statistics */
    IDV_STAT_INDEX_OPTM_RP_S_COPY_LOG_TO_REPLBUFFER,
    IDV_STAT_INDEX_OPTM_RP_S_WAIT_NEW_LOG,
    IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_REPLBUFFER,
    IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_FILE,
    IDV_STAT_INDEX_OPTM_RP_S_CHECK_USEFUL_LOG,
    IDV_STAT_INDEX_OPTM_RP_S_LOG_ANALYZE,
    IDV_STAT_INDEX_OPTM_RP_S_SEND_XLOG,
    IDV_STAT_INDEX_OPTM_RP_S_RECV_ACK,
    IDV_STAT_INDEX_OPTM_RP_S_SET_ACKEDVALUE,
    IDV_STAT_INDEX_OPTM_RP_R_RECV_XLOG,
    IDV_STAT_INDEX_OPTM_RP_R_CONVERT_ENDIAN,
    IDV_STAT_INDEX_OPTM_RP_R_TX_BEGIN,
    IDV_STAT_INDEX_OPTM_RP_R_TX_COMMIT,
    IDV_STAT_INDEX_OPTM_RP_R_TX_ABORT,
    IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_OPEN,
    IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_CLOSE,
    IDV_STAT_INDEX_OPTM_RP_R_INSERT_ROW,
    IDV_STAT_INDEX_OPTM_RP_R_UPDATE_ROW,
    IDV_STAT_INDEX_OPTM_RP_R_DELETE_ROW,
    IDV_STAT_INDEX_OPTM_RP_R_OPEN_LOB_CURSOR,
    IDV_STAT_INDEX_OPTM_RP_R_PREPARE_LOB_WRITE,
    IDV_STAT_INDEX_OPTM_RP_R_WRITE_LOB_PIECE,
    IDV_STAT_INDEX_OPTM_RP_R_FINISH_LOB_WRITE,
    IDV_STAT_INDEX_OPTM_RP_R_CLOSE_LOB_CURSOR,
    IDV_STAT_INDEX_OPTM_RP_R_COMPARE_IMAGE,
    IDV_STAT_INDEX_OPTM_RP_R_SEND_ACK,
    IDV_STAT_INDEX_OPTM_RP_R_TRIM_LOB,  /* PROJ-2047 Strengthening LOB */
    IDV_STAT_INDEX_OPTM_TASK_SCHEDULE, /* bug-35395 */
    IDV_STAT_INDEX_OPTM_TASK_SCHEDULE_MAX,

    IDV_STAT_INDEX_MAX

} idvStatIndex;


/* ------------------------------------------------
 *  Wait Event 정의
 * ----------------------------------------------*/
typedef enum idvWaitIndex
{
    IDV_WAIT_INDEX_BEGIN = 0,

    IDV_WAIT_INDEX_LATCH_BUFFER_BUSY_WAITS = IDV_WAIT_INDEX_BEGIN,
    IDV_WAIT_INDEX_LATCH_DRDB_BTREE_INDEX_SMO,
    IDV_WAIT_INDEX_LATCH_DRDB_BTREE_INDEX_SMO_BY_OTHER_SESSION,
    IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
    IDV_WAIT_INDEX_DB_FILE_MULTI_PAGE_READ,
    IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_READ,
    IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_WRITE,

    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_MULTI_PAGE_READ,
    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_READ,
    IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_WRITE,

    IDV_WAIT_INDEX_ENQ_DATA_ROW_LOCK_CONTENTION,
    IDV_WAIT_INDEX_ENQ_ALLOCATE_TXSEG_ENTRY,

    IDV_WAIT_INDEX_LATCH_FREE_DRDB_FILEIO,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_TBS_LIST,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_TBS_CREATION,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_PAGE_LIST_ENTRY,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_TXSEG_FREELIST,

    IDV_WAIT_INDEX_LATCH_FREE_DRDB_LRU_LIST,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_PREPARE_LIST,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_PREPARE_LIST_WAIT,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_FLUSH_LIST,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_CHECKPOINT_LIST,

    IDB_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_FLUSHER_MIN_RECOVERY_LSN,

    IDB_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_FLUSH_MANAGER_REQJOB,

    IDV_WAIT_INDEX_LATCH_FREE_DRDB_BCB_MUTEX,

    IDV_WAIT_INDEX_LATCH_FREE_DRDB_BCB_READ_IO_MUTEX,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_MANAGER_EXPAND_MUTEX,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_HASH_MUTEX,

    IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FILEIO,   
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_CHECKPOINT_LIST,
    IDB_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FLUSHER_MIN_RECOVERY_LSN,
    IDB_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FLUSH_MANAGER_REQJOB,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BCB_MUTEX,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_READIO_MUTEX,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_EXTENT_MUTEX,
    IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_EXTENT_WAIT,

    IDV_WAIT_INDEX_LATCH_FREE_PLAN_CACHE_LRU_LIST_MUTEX,
    IDV_WAIT_INDEX_LATCH_FREE_STATEMENT_LIST_MUTEX,

    IDV_WAIT_INDEX_LATCH_FREE_OTHERS,
    
    IDV_WAIT_INDEX_RP_BEFORE_COMMIT,
    IDV_WAIT_INDEX_RP_AFTER_COMMIT,

    IDV_WAIT_ID_SYSTEM,

    IDV_WAIT_INDEX_MAX,
    IDV_WAIT_INDEX_NULL = IDV_WAIT_INDEX_MAX

} idvWaitIndex;

/* 연산 소요시간 측정을 위한 Switch */
typedef enum idvTimeSwitch
{
    IDV_TIME_SWITCH_OFF = 0,

    IDV_TIME_SWITCH_ON,

    IDV_TIME_SWITCH_ACCUM

} idvTimeSwitch;

/* 연산 소요시간 측정을 위한 Switch */
typedef enum idvOwner
{
    IDV_OWNER_UNKNOWN = 0,
    IDV_OWNER_TRANSACTION,
    IDV_OWNER_REPL_TRANSACTION,
    IDV_OWNER_DISK_GC,
    IDV_OWNER_DISK_DEL_THR,
    IDV_OWNER_FLUSHER,
    IDV_OWNER_CHECKPOINTER,
    IDV_OWNER_MAX
} idvOwner;

/* ------------------------------------------------
 *  Elapsed Time Structure
 * ----------------------------------------------*/
typedef struct idvAccumTime
{
    /* 경과시간 ( 누적치 ) */
    ULong            mAccumTime;
    ULong            mElaTime;
    /* 측정시작/완료/반영 여부 */
    idvTimeSwitch    mTimeSwitch;

} idvAccumTime;


/* ------------------------------------------------
 *  Elapsed Time Structure
 * ----------------------------------------------*/
typedef struct idvTimeBox
{
    /* 측정시작시각 */
    idvTime          mBegin;
    /* 측정완료시각 */
    idvTime          mEnd;

    idvAccumTime     mATD;

} idvTimeBox;

/* ------------------------------------------------
 *  Wait Event 별 통계데이타
 * ----------------------------------------------*/
typedef struct idvWaitEvent
{
    /* Wait Event ID */
    idvWaitIndex mEventID;

    /* 총 대기 횟수 ( session or sessions )*/
    ULong        mTotalWaits;

    /* 지정된 대기시간 이후에도 요청한 리소스 획득 실패 횟수  */
    ULong        mTotalTimeOuts;

    /* 총 대기시간 (mili sec. 단위) */
//    ULong        mTimeWaited;

    /* 평균대기시간 */
//    ULong        mAverageWait;

    /* 최대대기시간 */
    ULong        mMaxWait;

    /* 총대기시간 (micro sec. 단위) */
    ULong        mTimeWaitedMicro;

    /* 측정시작/완료/반영 여부 */
    idvTimeSwitch mTimeSwitch;

} idvWaitEvent;

typedef struct idvStatEvent
{
    idvStatIndex  mEventID;
    ULong         mValue;

} idvStatEvent;


/* ------------------------------------------------
 * Wait Event의 Wait Parameters
 * ----------------------------------------------*/

typedef enum idvWaitParamType
{
    IDV_WAIT_PARAM_1  = 0,
    IDV_WAIT_PARAM_2,
    IDV_WAIT_PARAM_3,
    IDV_WAIT_PARAM_COUNT
} idvWaitParamType;

typedef struct idvWeArgs
{
    /* statmenet의 현재 대기중인 Wait Event ID */
    idvWaitIndex   mWaitEventID;
    /* 현재 대기중인 Wait Event와 Wait Parameter */
    ULong          mWaitParam[ IDV_WAIT_PARAM_COUNT ];
} idvWeArgs;

typedef struct idvSQL
{
    struct idvSession *mSess; /* 속한 세션 */

    ULong    *mSessionEvent;  /* Session Event */
    UInt     *mCurrStmtID;    /* Current Statement ID for TIMEOUT Event */

    void     *mLink;          /* Session CM Link */
    UInt     *mLinkCheckTime; /* Session Link Check Time */

    ULong     mSessionID; // useless

    idvOwner  mOwner;

    UInt      mCommandType;

    /* ------------------------------------------------
     *  QP Plan Infos
     * ----------------------------------------------*/
    ULong     mOptimizer;
    ULong     mCost;

    ULong     mUseMemory;

    /* ------------------------------------------------
     *  Disk Operation
     * ----------------------------------------------*/

    ULong  mReadPageCount;
    ULong  mWritePageCount;
    ULong  mGetPageCount;
    ULong  mFixPageCount;
    ULong  mCreatePageCount;

    ULong  mUndoReadPageCount;
    ULong  mUndoWritePageCount;
    ULong  mUndoGetPageCount;
    ULong  mUndoFixPageCount;
    ULong  mUndoCreatePageCount;

    // CASE-8623
    ULong  mLastIOWaitTimeUS;
    ULong  mMaxIOWaitTimeUS;
    ULong  mTotalIOWaitTimeUS;

    /* ------------------------------------------------
     *  Cursor Infos
     *  BUGBUG : Temp Table Info Needed!!
     * ----------------------------------------------*/
    ULong  mMemCursorSeqScan;
    ULong  mMemCursorIndexScan;
    ULong  mMemCursorGRIDScan;

    ULong  mDiskCursorSeqScan;
    ULong  mDiskCursorIndexScan;
    ULong  mDiskCursorGRIDScan;

    ULong  mExecuteSuccessCount;

    /* BUG-39352 Add each execute success count for select, insert, delete,
     * update into V$SYSSTAT */
    ULong  mExecuteInsertSuccessCount;
    ULong  mExecuteUpdateSuccessCount;
    ULong  mExecuteDeleteSuccessCount;
    ULong  mExecuteSelectSuccessCount;
    ULong  mExecuteReplInsertSuccessCount;
    ULong  mExecuteReplUpdateSuccessCount;
    ULong  mExecuteReplDeleteSuccessCount;

    ULong  mExecuteFailureCount;
    ULong  mFetchSuccessCount;
    ULong  mFetchFailureCount;
    ULong  mProcessRow;

    ULong  mMemoryTableAccessCount;

    /* ------------------------------------------------
     *  Time Infos
     * ----------------------------------------------*/
    idvTimeBox     mOpTime[ IDV_OPTM_INDEX_MAX ];


    /* 현재 대기중인 이벤트에 대해서 Wait Time 측정할때 사용
     * 측정완료시 바로 Session의 해당 Event에 반영된다. */
    idvTimeBox     mTimedWait;

    /* WaitEvent 기술자 ( id 및 parameters ) */
    idvWeArgs      mWeArgs;

    /* Process Monitoring */
    void         * mProcInfo; /* Process Thread Info */
    void         * mThrInfo;  /* Thread Info */
    
    ULong mMathTempMem; /* BUG-46892 */
}idvSQL;


/* ------------------------------------------------
 * 통계 정보에서 아래는 하나의 멤버에 대한 속성
 * ----------------------------------------------*/

#define IDV_ATTR_ACCUM   1 // 세션으로 부터 통계 수집함.
#define IDV_ATTR_SETUP   2 // 메모리 구조로 부터 직접 시스템으로 설정됨.

 // SM에서의 Page 타입 개수와 일치하여야 정확한 통계를 구할수 있다.
#define IDV_SM_PAGE_TYPE_MAX  26  

// X$SYSTEM_CONFLICT_PAGE을 위한 출력 구조체 
typedef struct idvSysConflictPageFT
{
    UInt     mPageType;
    ULong    mMissCnt;
    ULong    mMissTime;
} idvSysConflictPageFT;

/* ------------------------------------------------
 * 세션별 통계데이타
 * ----------------------------------------------*/
typedef struct idvSession
{
    // 세션 ID
    UInt mSID;
    // 세션 Stat Event 정보
    idvStatEvent mStatEvent[ IDV_STAT_INDEX_MAX ];
    // 세션 Wait Event 정보
    idvWaitEvent mWaitEvent[ IDV_WAIT_INDEX_MAX ];

    // 세션의 Page Miss Count 누적
    ULong        mMissCnt[ IDV_SM_PAGE_TYPE_MAX ];
    ULong        mMissTime[ IDV_SM_PAGE_TYPE_MAX ];

    /* BUG-19080: Old Version의 양이 일정이상이 되면 해당
     * Transaction을 Abort하는 기능이 필요합니다.
     *
     * MM의 Session의 mmcSession을 가리키고 있다.
     * SM에서 MM의 Callback Function을 이용해서
     * 현재 Session의 Pointer를 가진다.
     **/
    void*     mSession;
} idvSession;

// idvSystem
typedef struct idvSession idvSystem;


// For Fixed Table x$statname
typedef struct idvStatName
{
    UInt         mSeqNum;
    const SChar *mName;
    UInt         mAttr;
    UInt         mWaitClassID; /* Wait Name에서만 사용한다. */

} idvStatName;

// also used for x$sysstat
typedef struct idvSessStatFT
{
    UInt          mSID;
    idvStatEvent  mStatEvent;

} idvSessStatFT;

// also used for x$syswait
typedef struct idvSessWaitFT
{
    UInt          mSID;
    idvWaitEvent  mWaitEvent;

} idvSessWaitFT;

// For Fixed Table x$waitname
typedef struct idvStatName idvWaitEventName;

// For Fixed Table x$waitclassname
typedef struct idvStatName idvWaitClassName;

#endif
