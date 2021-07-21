/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idv.cpp 83362 2018-06-27 04:15:04Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>
#include <idu.h>
#include <idvProfile.h>
/* ------------------------------------------------
 *  idvManager
 * ----------------------------------------------*/

extern idvTimeFunc gNoneFunctions;
extern idvTimeFunc gTimeThreadFunctions;
extern idvTimeFunc gLibraryFunctions;
extern idvTimeFunc gClockFunctions;

extern idvHandler  gTimeThreadHandler;
extern idvHandler  gClockThreadHandler;
extern idvHandler  gDefaultHandler;


UInt         idvManager::mType; // 0 : none 1 : timer thread 2:libraray 3:tick
UInt         idvManager::mLinkCheckTime = 0;
ULong        idvManager::mClock;
ULong        idvManager::mTimeSec;
idvResource *idvManager::mResource;
idvHandler  *idvManager::mHandler;
// Time Service사용 가능 여부
idBool       idvManager::mIsServiceAvail = ID_FALSE;

idvTimeFunc *idvManager::mOPArray[] =
{
    &gNoneFunctions,
    &gTimeThreadFunctions,
    &gLibraryFunctions,
    &gClockFunctions
};

idvHandler *gTimerType[] =
{
    &gDefaultHandler,
    &gTimeThreadHandler,
    &gDefaultHandler,
    &gClockThreadHandler
};

idvTimeFunc *idvManager::mOP;

idvLinkCheckCallback idvManager::mLinkCheckCallback;
idvBaseTimeCallback  idvManager::mBaseTimeCallback;


/* ------------------------------------------------
 *                 Name Registration
 *
 *  !!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!
 *
 *    등록되는 순서가 0부터 마지막까지
 *    순차적으로 증가하는 형태로 되어야 한다.
 *    [0, 2, 1] 형태로 등록되면 구동시 에러가 발생한다.
 *
 * ----------------------------------------------*/

idvStatName idvManager::mStatName[IDV_STAT_INDEX_MAX] =
{
    { IDV_STAT_INDEX_LOGON_CURR,               "logon current",
      IDV_ATTR_SETUP, 0 /* unuse */ },
    { IDV_STAT_INDEX_LOGON_CUMUL,              "logon cumulative",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_READ_PAGE,                "data page read",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_WRITE_PAGE,               "data page write",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_GET_PAGE,                 "data page gets",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_FIX_PAGE,                 "data page fix",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_CREATE_PAGE,               "data page create",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_UNDO_READ_PAGE,           "undo page read",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_UNDO_WRITE_PAGE,          "undo page write",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_UNDO_GET_PAGE,            "undo page gets",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_UNDO_FIX_PAGE,            "undo page fix",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_UNDO_CREATE_PAGE,          "undo page create",
      IDV_ATTR_SETUP, 0 },
    { IDV_STAT_INDEX_DETECTOR_BASE_TIME,       "base time in second",
      IDV_ATTR_SETUP, 0 },

    { IDV_STAT_INDEX_QUERY_TIMEOUT_COUNT,      "query timeout",
      IDV_ATTR_ACCUM , 0 },
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    { IDV_STAT_INDEX_DDL_TIMEOUT_COUNT,        "ddl timeout",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_IDLE_TIMEOUT_COUNT,       "idle timeout",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_FETCH_TIMEOUT_COUNT,      "fetch timeout",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_UTRANS_TIMEOUT_COUNT,     "utrans timeout",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SESSION_TERMINATED_COUNT, "session terminated",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_DDL_SYNC_TIMEOUT_COUNT,   "ddl sync timeout",
      IDV_ATTR_ACCUM , 0 },

    /* BUG-24151: [SC] Update Retry, Delete Retry, Statement Rebuild Count를
     *            AWI로 추가해야 합니다.*/
    { IDV_STAT_INDEX_STMT_REBUILD_COUNT,       "statement rebuild count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_UNIQUE_VIOLATION_COUNT,   "unique violation count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_UPDATE_RETRY_COUNT,       "update retry count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_DELETE_RETRY_COUNT,       "delete retry count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_LOCKROW_RETRY_COUNT,      "lock row retry count",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_COMMIT_COUNT,             "session commit",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_ROLLBACK_COUNT,           "session rollback",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_FETCH_SUCCESS_COUNT,      "fetch success count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_FETCH_FAILURE_COUNT,      "fetch failure count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_SUCCESS_COUNT,    "execute success count",
      IDV_ATTR_ACCUM , 0 },

    /* BUG-39352 Add each execute success count for select, insert, delete,
     * update into V$SYSSTAT */
    { IDV_STAT_INDEX_EXECUTE_INSERT_SUCCESS_COUNT,    "execute success count : insert",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_UPDATE_SUCCESS_COUNT,    "execute success count : update",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_DELETE_SUCCESS_COUNT,    "execute success count : delete",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_SELECT_SUCCESS_COUNT,    "execute success count : select",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_REPL_INSERT_SUCCESS_COUNT,    "rep_execute success count : insert",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_REPL_UPDATE_SUCCESS_COUNT,    "rep_execute success count : update",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_EXECUTE_REPL_DELETE_SUCCESS_COUNT,    "rep_execute success count : delete",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_EXECUTE_FAILURE_COUNT,    "execute failure count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_PREPARE_SUCCESS_COUNT,    "prepare success count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_PREPARE_FAILURE_COUNT,    "prepare failure count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_REBUILD_COUNT,            "rebuild count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_REDOLOG_COUNT,            "write redo log count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_REDOLOG_SIZE,             "write redo log bytes",
      IDV_ATTR_ACCUM , 0 },

    // To Fix BUG-21008
    { IDV_STAT_INDEX_RECV_SOCKET_COUNT,        "read socket count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SEND_SOCKET_COUNT,        "write socket count",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_RECV_TCP_BYTE,            "byte received via inet",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SEND_TCP_BYTE,            "byte sent via inet",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_RECV_UNIX_BYTE,           "byte received via unix domain",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SEND_UNIX_BYTE,           "byte sent via unix domain",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_RECV_IPC_BLOCK_COUNT,     "semop count for receiving via ipc ",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SEND_IPC_BLOCK_COUNT,     "semop count for sending via ipc ",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_MEM_CURSOR_SEQ_SCAN,      "memory table cursor full scan count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_MEM_CURSOR_IDX_SCAN,      "memory table cursor index scan count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_MEM_CURSOR_GRID_SCAN,     "memory table cursor GRID scan count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_DISK_CURSOR_SEQ_SCAN,     "disk table cursor full scan count" ,
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_DISK_CURSOR_IDX_SCAN,     "disk table cursor index scan count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_DISK_CURSOR_GRID_SCAN,     "disk table cursor GRID scan count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_LOCK_ACQUIRED,            "lock acquired count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_LOCK_RELEASED,            "lock released count",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_SERVICE_THREAD_CREATED,   "service thread created count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_MEMORY_TABLE_ACCESS_COUNT, "memory table access count",
      IDV_ATTR_ACCUM , 0 },

    /* BUG-35631 Add x-trylatch count into stat */
    { IDV_STAT_INDEX_PLAN_CACHE_PPCO_MISS_X_TRY_LATCH_COUNT,
      "missing ppco x-trylatch count",
      IDV_ATTR_ACCUM, 0 },

    /* PROJ-2681 */
    { IDV_STAT_INDEX_RECV_IB_COUNT,
      "read IB count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SEND_IB_COUNT,
      "write IB count",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_RECV_IB_BYTE,
      "byte received via IB",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_SEND_IB_BYTE,
      "byte sent via IB",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_QUERY_PARSE,
      "elapsed time: query parse",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_QUERY_VALIDATE,
      "elapsed time: query validate",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_QUERY_OPTIMIZE,
      "elapsed time: query optimize",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_QUERY_EXECUTE,
      "elapsed time: query execute",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_QUERY_FETCH,
      "elapsed time: query fetch",
      IDV_ATTR_ACCUM , 0 },
    //PROJ-1436.
    { IDV_STAT_INDEX_QUERY_SOFT_PREPARE,
      "elapsed time: soft prepare",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_DRDB_DML_ANALYZE_VALUES,
      "elapsed time: analyze values in DML(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_DML_RECORD_LOCK_VALIDATE,
      "elapsed time: record lock validation in DML(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_SLOT,
      "elapsed time: allocate data slot in DML(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_DML_WRITE_UNDO_RECORD,
      "elapsed time: write undo record in DML(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_TSS,
      "elapsed time: allocate tss in dml(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_UNDO_PAGE,
      "elapsed time: allocate undopage in dml(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_DML_INDEX_OPER,
      "elapsed time: index operation in dml(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_CREATE_PAGE,
      "elapsed time: create page(disk)",
      IDV_ATTR_ACCUM, 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_GET_PAGE,
      "elapsed time: get page(disk)",
      IDV_ATTR_ACCUM, 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_FIX_PAGE,
      "elapsed time: fix page(disk)",
      IDV_ATTR_ACCUM, 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_TRANS_LOGICAL_AGING,
      "elapsed time: logical aging by tx in dml(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_DRDB_TRANS_PHYSICAL_AGING,
      "elapsed time: phyical aging by tx in dml(disk)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE,
      "elapsed time: replace(plan cache)",
      IDV_ATTR_ACCUM , 0 },
    { IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_VICTIM_FREE,
      "elapsed time: victim free in replace(plan cache)",
      IDV_ATTR_ACCUM , 0 },

    /*fix BUG-30855 It needs to describe soft prepare time in detail
      for problem tracking . */
    { IDV_STAT_INDEX_OPTM_PLAN_HARD_REBUILD,
      "elapsed time: hard rebuild",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_SOFT_REBUILD,
      "elapsed time: soft rebuild",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_PLANCACHE_CHECK_IN_BY_HARD_PREPARE ,
      "elapsed time: add hard-prepared plan to plan cache",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_PLANCACHE_CHECK_IN_BY_HARD_REBUILD ,
      "elapsed time: add hard-rebuild plan to plan cache",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_PLANCACHE_SEARCH_PPCO,
      "elapsed time: search time for parent PCO",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_PLANCACHE_CREATE_PPCO ,
      "elapsed time: creation time for parent PCO",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_PLANCACHE_SEARCH_CHILD_PCO ,
      "elapsed time: search time for child PCO",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_PLANCACHE_CREATE_CHILD_PCO ,
      "elapsed time: creation time for child PCO",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_VALIDATE_PCO ,
      "elapsed time: validation time for child PCO",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC ,
      "elapsed time: creation time for new child PCO by rebuild at execution ",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE ,
      "elapsed time: creation time for new child PCO by rebuild at soft prepare",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_HARD_PREPARE  ,
      "elapsed time: hard prepare time",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_MATCHING_CHILD_PCO,
      "elapsed time: matching time for child PCO",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_WAITING_HARD_PREPARE,
      "elapsed time: waiting time for hard prepare",
      IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT,
      "elapsed time: moving time from cold region to hot region.",
     IDV_ATTR_ACCUM , 0 },
    
    {IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO,
      "elapsed time: waiting time for parent PCO  when choosing plan cache replacement victim.",
     IDV_ATTR_ACCUM , 0 },
    
    {IDV_STAT_INDEX_OPTM_INDEX_CHECK_PRIVILEGE_PCO,
      "elapsed time: privilege checking time during soft prepare.",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_COPY_LOG_TO_REPLBUFFER,
      "elapsed time: copying logs to replication log buffer (sender side)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_WAIT_NEW_LOG,
      "elapsed time: sender(s) waiting for new logs",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_REPLBUFFER,
      "elapsed time: sender(s) reading logs from replication log buffer",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_FILE,
      "elapsed time: sender(s) reading logs from log file(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_CHECK_USEFUL_LOG,
      "elapsed time: sender(s) checking whether logs are useful",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_LOG_ANALYZE,
      "elapsed time: sender(s) analyzing logs",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_SEND_XLOG,
      "elapsed time: sender(s) sending xlogs to receiver(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_RECV_ACK,
      "elapsed time: sender(s) receiving ACK from receiver(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_S_SET_ACKEDVALUE,
      "elapsed time: sender(s) setting ACKed value",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_RECV_XLOG,
      "elapsed time: receiver(s) receiving xlogs from sender(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_CONVERT_ENDIAN,
      "elapsed time: receiver(s) performing endian conversion",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_TX_BEGIN,
      "elapsed time: receiver(s) beginning transaction(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_TX_COMMIT,
      "elapsed time: receiver(s) committing transaction(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_TX_ABORT,
      "elapsed time: receiver(s) aborting transaction(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_OPEN,
      "elapsed time: receiver(s) opening table cursor(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_CLOSE,
      "elapsed time: receiver(s) closing table cursor(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_INSERT_ROW,
      "elapsed time: receiver(s) inserting rows",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_UPDATE_ROW,
      "elapsed time: receiver(s) updating rows",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_DELETE_ROW,
      "elapsed time: receiver(s) deleting rows",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_OPEN_LOB_CURSOR,
      "elapsed time: receiver(s) opening lob cursor(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_PREPARE_LOB_WRITE,
      "elapsed time: receiver(s) preparing to write LOB(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_WRITE_LOB_PIECE,
      "elapsed time: receiver(s) writing LOB piece(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_FINISH_LOB_WRITE,
      "elapsed time: receiver(s) finish writing LOBs",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_CLOSE_LOB_CURSOR,
      "elapsed time: receiver(s) closing LOB cursor(s)",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_COMPARE_IMAGE,
      "elapsed time: receiver(s) comparing images to check for conflicts",
     IDV_ATTR_ACCUM , 0 },

    {IDV_STAT_INDEX_OPTM_RP_R_SEND_ACK,
      "elapsed time: receiver(s) sending ACK",
     IDV_ATTR_ACCUM , 0 },

    /* PROJ-2047 Strengthening LOB */
    {IDV_STAT_INDEX_OPTM_RP_R_TRIM_LOB,
     "elapsed time: receiver(s) trim LOB(s)",
     IDV_ATTR_ACCUM , 0 },

    /* bug-35395: task schedule time 을 sysstat에 추가 */
    { IDV_STAT_INDEX_OPTM_TASK_SCHEDULE,
      "elapsed time: task schedule",
      IDV_ATTR_ACCUM , 0 },

    { IDV_STAT_INDEX_OPTM_TASK_SCHEDULE_MAX,
      "max     time: task schedule",
      IDV_ATTR_SETUP , 0 }
};

/* ------------------------------------------------
 *  세션 Wait Event 종류
 * ----------------------------------------------*/

idvWaitEventName idvManager::mWaitEventName[ IDV_WAIT_INDEX_MAX + 1 ] =
{
    { IDV_WAIT_INDEX_LATCH_BUFFER_BUSY_WAITS,
      "latch: buffer busy waits",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_DRDB_BTREE_INDEX_SMO,
      "latch: drdb B-Tree index SMO",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_DRDB_BTREE_INDEX_SMO_BY_OTHER_SESSION,
      "latch: drdb B-Tree index SMO by other session",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
      "latch: drdb R-Tree index SMO",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_DB_FILE_MULTI_PAGE_READ,
      "db file multi page read",                IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_USER_IO },

    { IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_READ,
      "db file single page read",               IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_USER_IO },

    { IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_WRITE,
      "db file single page write",              IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_SYSTEM_IO },

    { IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_MULTI_PAGE_READ,
      "secondary buffer file multi page read",                
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_USER_IO },

    { IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_READ,
      "secondary buffer file single page read",               
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_USER_IO },

    { IDV_WAIT_INDEX_SECONDARY_BUFFER_FILE_SINGLE_PAGE_WRITE,
      "secondary buffer file single page write",              
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_SYSTEM_IO },

    { IDV_WAIT_INDEX_ENQ_DATA_ROW_LOCK_CONTENTION,
      "enq: TX - row lock contention, data row",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_ENQ_ALLOCATE_TXSEG_ENTRY,
      "enq: TX - allocate TXSEG entry",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_FILEIO,
      "latch free: drdb file io",               IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_TBS_LIST,
      "latch free: drdb tbs list",              IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_TBS_CREATION,
      "latch free: drdb tbs creation",          IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_PAGE_LIST_ENTRY,
      "latch free: drdb page list entry",       IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_TXSEG_FREELIST,
      "latch free: drdb transaction segment freelist",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_LRU_LIST,
      "latch free: drdb LRU list", IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_PREPARE_LIST,
      "latch free: drdb prepare list", IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_PREPARE_LIST_WAIT,
      "latch free: drdb prepare list wait",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_FLUSH_LIST,
      "latch free: drdb flush list", IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_CHECKPOINT_LIST,
      "latch free: drdb checkpoint list",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDB_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_FLUSHER_MIN_RECOVERY_LSN,
      "latch free: drdb buffer flusher min recovery LSN",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDB_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_FLUSH_MANAGER_REQJOB,
      "latch free: drdb buffer flush manager req job",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_BCB_MUTEX,
      "latch free: drdb buffer bcb mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_BCB_READ_IO_MUTEX,
      "latch free: drdb buffer bcb read io mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_MANAGER_EXPAND_MUTEX,
      "latch free: drdb buffer buffer manager expand mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_HASH_MUTEX,
      "latch free: drdb buffer hash mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FILEIO,
      "latch free: drdb secondary buffer io",               
       IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_CHECKPOINT_LIST,
      "latch free: drdb secondary buffer checkpoint list",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDB_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FLUSHER_MIN_RECOVERY_LSN,
      "latch free: drdb secondary buffer flusher min recovery LSN",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDB_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_FLUSH_MANAGER_REQJOB,
      "latch free: drdb secondary buffer flush manager req job",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BCB_MUTEX,
      "latch free: drdb secondary bcb mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_READIO_MUTEX,
      "latch free: drdb secondary read io mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_EXTENT_MUTEX,
      "latch free: drdb secondary buffer flush block mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_DRDB_SECONDARY_BUFFER_EXTENT_WAIT,
      "latch free: drdb secondary buffer block wait",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },


    { IDV_WAIT_INDEX_LATCH_FREE_PLAN_CACHE_LRU_LIST_MUTEX,
      "latch free: plan cache LRU List mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_STATEMENT_LIST_MUTEX,
      "latch free: statement list mutex",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_CONCURRENCY },

    { IDV_WAIT_INDEX_LATCH_FREE_OTHERS,
      "latch free: others",                     IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_OTHER },

    { IDV_WAIT_INDEX_RP_BEFORE_COMMIT,
      "replication before commit",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_RP },

    { IDV_WAIT_INDEX_RP_AFTER_COMMIT,
      "replication after commit",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_RP },

    { IDV_WAIT_ID_SYSTEM,
      "system internal",
      IDV_ATTR_ACCUM,
      IDV_WCLASS_INDEX_ID_SYSTEM },

    { IDV_WAIT_INDEX_NULL,
      "no wait event",
      IDV_ATTR_SETUP,
      IDV_WCLASS_INDEX_IDLE }
};

idvWaitClassName idvManager::mWaitClassName[ IDV_WCLASS_INDEX_MAX ] =
{
    { IDV_WCLASS_INDEX_OTHER,
      "Other",                   IDV_ATTR_ACCUM, 0 /* unuse */ },
    { IDV_WCLASS_INDEX_ADMINISTRATIVE,
      "Administrative",          IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_CONFIGURATION,
      "Configuration",           IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_CONCURRENCY,
      "Concurrency",             IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_COMMIT,
      "Commit",                  IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_IDLE,
      "Idle",                    IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_USER_IO,
      "User I/O",                IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_SYSTEM_IO,
      "System I/O",              IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_RP,
      "Replication",             IDV_ATTR_ACCUM, 0 },
    { IDV_WCLASS_INDEX_ID_SYSTEM,
      "ID System",               IDV_ATTR_ACCUM, 0 }
};



/* ------------------------------------------------
 *  시스템 영역.
 *  동적으로 할당을 받아 포인터를 통해 접근하는 것보다,
 *  전역으로 놓고, 직접 접근하는 것이 성능에 잇점이 있다.
 *  이런 이유로 전역으로 정의한다.
 * ----------------------------------------------*/

idvSystem gSystemInfo;


/* ------------------------------------------------
 *  idv 모듈을 초기화 한다.
 * ----------------------------------------------*/

void idvManager::initSQL(idvSQL     *aSQL,
                         idvSession *aSession,
                         ULong      *aSessionEvent,
                         UInt       *aCurrStmtID,
                         void       *aLink,
                         UInt       *aLinkCheckTime,
                         idvOwner    aOwner)
{
    IDE_DASSERT(aSQL != NULL);

    idlOS::memset(aSQL, 0, ID_SIZEOF(idvSQL));

    aSQL->mSess                = aSession;
    aSQL->mSessionEvent        = aSessionEvent;
    aSQL->mCurrStmtID          = aCurrStmtID;
    aSQL->mLink                = aLink;
    aSQL->mLinkCheckTime       = aLinkCheckTime;
    aSQL->mWeArgs.mWaitEventID = IDV_WAIT_INDEX_NULL;
    aSQL->mOwner         = aOwner;

    if (aSession != NULL)
    {
        aSQL->mSessionID = aSession->mSID;
    }
}

void idvManager::resetSQL(idvSQL *aSQL)
{
    idvSession *sSession       = aSQL->mSess;
    ULong      *sSessionEvent  = aSQL->mSessionEvent;
    UInt       *sCurrStmtID    = aSQL->mCurrStmtID;
    void       *sLink          = aSQL->mLink;
    UInt       *sLinkCheckTime = aSQL->mLinkCheckTime;
    idvOwner    sOwner         = aSQL->mOwner;
    ULong       sSessionID     = aSQL->mSessionID;
    
    idlOS::memset(aSQL, 0, ID_SIZEOF(idvSQL));
    
    aSQL->mSess                = sSession;
    aSQL->mSessionEvent        = sSessionEvent;
    aSQL->mCurrStmtID          = sCurrStmtID;
    aSQL->mLink                = sLink;
    aSQL->mLinkCheckTime       = sLinkCheckTime;
    aSQL->mWeArgs.mWaitEventID = IDV_WAIT_INDEX_NULL;
    aSQL->mOwner               = sOwner;
    aSQL->mSessionID           = sSessionID;
}

/* ------------------------------------------------
 *  idvSQL의 누적시간을 prepare 시 초기화 한다
 * ----------------------------------------------*/

void idvManager::initPrepareAccumTime( idvSQL * aStatSQL )
{
    if ( aStatSQL != NULL )
    {
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_QUERY_PARSE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_QUERY_VALIDATE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_QUERY_OPTIMIZE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_QUERY_SOFT_PREPARE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE ) );
       
        /*fix BUG-30855 It needs to describe soft prepare time in detail
          for problem tracking . */
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_PREPARE));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLANCACHE_SEARCH_PPCO));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLANCACHE_CREATE_PPCO));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLANCACHE_SEARCH_CHILD_PCO));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLANCACHE_CREATE_CHILD_PCO));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_VALIDATE_PCO));
            
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_HARD_PREPARE));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_MATCHING_CHILD_PCO));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_WAITING_HARD_PREPARE));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT));

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_CHECK_PRIVILEGE_PCO));

        
        
    }
}

/* ------------------------------------------------
 *  idvSQL의 누적시간을 beginStmt 시 초기화 한다
 * ----------------------------------------------*/

void idvManager::initBeginStmtAccumTime( idvSQL * aStatSQL )
{
    if ( aStatSQL != NULL )
    {
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_QUERY_EXECUTE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_QUERY_FETCH ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_CREATE_PAGE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_GET_PAGE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_FIX_PAGE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING ) );
        
         /*fix BUG-30855 It needs to describe soft prepare time in detail
          for problem tracking . */
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLAN_HARD_REBUILD));

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_REBUILD));
        
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_SOFT_REBUILD ));

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC));

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL,IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER));
    }
}

void idvManager::initRPSenderAccumTime( idvSQL * aStatSQL )
{
    if ( aStatSQL != NULL )
    {
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_LOG_ANALYZE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_SEND_XLOG ) );
    }
}

void idvManager::initRPSenderApplyAccumTime( idvSQL * aStatSQL )
{
    if ( aStatSQL != NULL )
    {
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_RECV_ACK ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_S_SET_ACKEDVALUE ) );
    }
}

void idvManager::initRPReceiverAccumTime( idvSQL * aStatSQL )
{
    if ( aStatSQL != NULL )
    {
        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_RECV_XLOG ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_TX_BEGIN ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_TX_COMMIT ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_TX_ABORT ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_TABLE_CURSOR_OPEN ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_TABLE_CURSOR_CLOSE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_INSERT_ROW ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_UPDATE_ROW ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_DELETE_ROW ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_OPEN_LOB_CURSOR ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_PREPARE_LOB_WRITE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_WRITE_LOB_PIECE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_FINISH_LOB_WRITE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_CLOSE_LOB_CURSOR ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_COMPARE_IMAGE ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_SEND_ACK ) );

        IDV_TIMEBOX_INIT_ACCUM_TIME(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, IDV_OPTM_INDEX_RP_R_TRIM_LOB ) );
    }
}

void idvManager::initStat4Session( idvSession * aSession )
{
    UInt i;

    IDE_DASSERT( aSession != NULL );

    /* ------------------------------------------------
     *  initialize Stat Event ID
     * ----------------------------------------------*/
    for (i = IDV_STAT_INDEX_BEGIN; i < IDV_STAT_INDEX_MAX; i++)
    {
        aSession->mStatEvent[i].mEventID = (idvStatIndex)i;
        aSession->mStatEvent[i].mValue   = 0;
    }

    /* ------------------------------------------------
     *  initialize Wait Event ID
     * ----------------------------------------------*/
    for (i = IDV_WAIT_INDEX_BEGIN; i < IDV_WAIT_INDEX_MAX; i++)
    {
        aSession->mWaitEvent[i].mEventID = (idvWaitIndex)i;
    }

    /* ------------------------------------------------
     *  latch: buffer busy wait 대기 이벤트의
     *  page 타입별 latch miss 통계정보 초기화
     * ----------------------------------------------*/
    // BUG-21155 : session 초기화
    idlOS::memset(aSession->mMissCnt, 0x00, (ID_SIZEOF(ULong) * IDV_SM_PAGE_TYPE_MAX) );
    idlOS::memset(aSession->mMissTime, 0x00, (ID_SIZEOF(ULong) * IDV_SM_PAGE_TYPE_MAX) );
}

void idvManager::initSession( idvSession *aSession,
                              UInt        aSessionID,
                              void       *aMMSessionPtr )
{
    idlOS::memset(aSession, 0, ID_SIZEOF(idvSession));
    aSession->mSID = aSessionID;
    aSession->mSession = aMMSessionPtr;

    // BUG-21148
    initStat4Session(aSession);
}

IDE_RC idvManager::initializeStatic()
{
    UInt i;

    /* ------------------------------------------------
     *  Timer Handler Setup by Property
     * ----------------------------------------------*/

    IDE_ASSERT(idp::read("TIMER_RUNNING_LEVEL", &mType) == IDE_SUCCESS);

    /* BUG-32878 The startup failure caused by setting TIMER_RUNNING_LEVEL=3 should be handled */
#if defined(SPARC_SOLARIS) || defined(X86_SOLARIS) || defined(IBM_AIX) || \
    defined(IA64_HP_HPUX) || ( defined(HP_HPUX) && !defined(COMPILE_64BIT) )
    if ( mType == IDV_TIMER_LEVEL_CLOCK )
    {
        mType = IDV_TIMER_DEFAULT_LEVEL;
        ideLog::log(IDE_SERVER_0, ID_TRC_TIMER_RUNNING_LEVEL_EXCEPTION, mType);
    }
#endif

    mHandler =  gTimerType[mType];
    mOP      =  mOPArray[mType];

    IDE_TEST( (*mHandler->mInitializeStatic)(&mResource, &mClock, &mTimeSec)
              != IDE_SUCCESS);

    idlOS::memset( &gSystemInfo, 0x00, ID_SIZEOF(idvSystem));

    initStat4Session(&gSystemInfo);

    /* ------------------------------------------------
     *  Validate Wait Class Registration
     * ----------------------------------------------*/
    for (i = 0; i < IDV_WCLASS_INDEX_MAX; i++)
    {
        if (mWaitClassName[i].mSeqNum != i)
        {

            ideLog::log(IDE_SERVER_0, ID_TRC_INVALID_WAIT_SEQ, i);
            idlOS::exit(-1);
        }
    }

    /* ------------------------------------------------
     *  Validate Stat Event Registration
     * ----------------------------------------------*/
    for (i = 0; i < IDV_STAT_INDEX_MAX; i++)
    {
        if (mStatName[i].mSeqNum != i)
        {
            ideLog::log(IDE_SERVER_0, ID_TRC_INVALID_STAT_SEQ, i);
            idlOS::exit(-1);
        }
    }

    /* ------------------------------------------------
     *  Validate Wait Event Registration
     * ----------------------------------------------*/
    for (i = 0; i < IDV_WAIT_INDEX_MAX; i++)
    {
        if (mWaitEventName[i].mSeqNum != i)
        {
            ideLog::log(IDE_SERVER_0, ID_TRC_INVALID_WAIT_SEQ, i);
            idlOS::exit(-1);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC idvManager::startupService()
{
    IDE_TEST( (*mHandler->mStartup)(mResource) != IDE_SUCCESS);
    mIsServiceAvail = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idvManager::shutdownService()
{
    mIsServiceAvail = ID_FALSE;
    IDE_TEST( (*mHandler->mShutdown)(mResource) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idvManager::destroyStatic()
{
    IDE_TEST( (*mHandler->mDestroyStatic)(mResource) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  session 에 있는 통계 정보의 차이를 시스템으로 복사한다.
 *  복사후에는 old의 값을 new로 복사하여, 그 차이를
 *  0으로 만든다.
 *
 *  이렇게 번거로운 작업을 하는 이유는
 *  만일 많은 쓰레드들이 동시에 시스템의 통계정보를
 *  update할 경우에 BUS-Saturation이 발생하여
 *  시스템의 성능을 떨어뜨릴 가능성이 있기 때문에
 *  주기적으로 반영하는 것이다. by sjkim
 * ----------------------------------------------*/

void   idvManager::applyStatisticsToSystem(idvSession *aCurr, idvSession *aOld)
{
    UInt              i;
    idvStatEvent    * sStatCurr;
    idvStatEvent    * sStatOld;
    idvWaitEvent    * sWaitCurr;
    idvWaitEvent    * sWaitOld;
    ULong           * sMissCntCurr;
    ULong           * sMissCntOld;
    ULong           * sMissTimeCurr;
    ULong           * sMissTimeOld;

    for (i = IDV_STAT_INDEX_BEGIN;
         i < IDV_STAT_INDEX_MAX;
         (UInt)i ++)
    {
        if ( (mStatName[i].mAttr  & IDV_ATTR_ACCUM) != 0)
        {
            sStatCurr = &(aCurr->mStatEvent[i]);
            sStatOld  = &(aOld->mStatEvent[i]);

            applyStatEventToSystem( (idvStatIndex)i, sStatCurr, sStatOld );

        }
    }

    // TIMED_STATISTICS가 ON인 경우
    if ( iduProperty::getTimedStatistics() == IDV_TIMED_STATISTICS_ON )
    {
        for (i = IDV_WAIT_INDEX_BEGIN;
                i < IDV_WAIT_INDEX_MAX;
                i ++)
        {
            if ( (mWaitEventName[i].mAttr  & IDV_ATTR_ACCUM) != 0)
            {
                sWaitCurr = &(aCurr->mWaitEvent[i]);
                sWaitOld  = &(aOld->mWaitEvent[i]);

                applyWaitEventToSystem( (idvWaitIndex)i,
                        sWaitCurr,
                        sWaitOld );
            }
        }

        // 'latch: buffer busy wait'에 의해
        // page latch가 miss된 통계 정보 누적
        for (i = 0;
             i < IDV_SM_PAGE_TYPE_MAX;
             i ++)
        {
            sMissCntCurr = &(aCurr->mMissCnt[i]);
            sMissCntOld  = &(aOld->mMissCnt[i]);
            sMissTimeCurr = &(aCurr->mMissTime[i]);
            sMissTimeOld  = &(aOld->mMissTime[i]);

            applyLatchStatToSystem( i,
                                    sMissCntCurr,
                                    sMissCntOld,
                                    sMissTimeCurr,
                                    sMissTimeOld );
        }
    }

    idvProfile::writeSession(aCurr);
}

/*
 * stmt end시에 통계정보를 자신의 Session 경과시간
 * 통계정보에 누적시킨다. 경과시간 외의 다른 몇몇
 * stmt 통계정보는 stmt와 sess에 직접 증감시키도록
 * 구현되어 있다.
 */
void idvManager::applyOpTimeToSession(
                          idvSession  *aCurrSess,
                          idvSQL      *aCurrSQL  )
{
    UInt  i;

    // TIMED_STATISTICS가 ON인 경우
    if ( iduProperty::getTimedStatistics() == IDV_TIMED_STATISTICS_ON )
    {
        for (i = IDV_STAT_INDEX_BEGIN_STMT_TO_SESS;
                i < IDV_STAT_INDEX_MAX;
                (UInt)i ++)
        {
            if ( (mStatName[i].mAttr & IDV_ATTR_ACCUM) != 0)
            {
                addOpTimeToSession( (idvStatIndex)i,
                        aCurrSess,
                        aCurrSQL );
            }
        }
    }
}

void idvManager::addOpTimeToSession(
                        idvStatIndex  aStatIdx,
                        idvSession   *aCurrSess,
                        idvSQL       *aCurrSQL )
{
    idvOperTimeIndex sOetIndex = IDV_OPTM_INDEX_BEGIN;
    idvTimeBox       sOetTime;

    IDE_DASSERT( aCurrSess!= NULL );
    IDE_DASSERT( aCurrSQL != NULL );

    switch( aStatIdx )
    {
        case IDV_STAT_INDEX_OPTM_QUERY_PARSE:
            sOetIndex = IDV_OPTM_INDEX_QUERY_PARSE;
            break;
        case IDV_STAT_INDEX_OPTM_QUERY_VALIDATE:
            sOetIndex = IDV_OPTM_INDEX_QUERY_VALIDATE;
            break;
        case IDV_STAT_INDEX_OPTM_QUERY_OPTIMIZE:
            sOetIndex = IDV_OPTM_INDEX_QUERY_OPTIMIZE;
            break;
        case IDV_STAT_INDEX_OPTM_QUERY_EXECUTE:
            sOetIndex = IDV_OPTM_INDEX_QUERY_EXECUTE;
            break;
        case IDV_STAT_INDEX_OPTM_QUERY_FETCH:
            sOetIndex = IDV_OPTM_INDEX_QUERY_FETCH;
            break;
        case IDV_STAT_INDEX_QUERY_SOFT_PREPARE:
            sOetIndex = IDV_OPTM_INDEX_QUERY_SOFT_PREPARE;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_ANALYZE_VALUES:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_ANALYZE_VALUES;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_RECORD_LOCK_VALIDATE:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_RECORD_LOCK_VALIDATE;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_SLOT:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_ALLOC_SLOT;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_WRITE_UNDO_RECORD:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_TSS:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_ALLOC_UNDO_PAGE:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_DML_INDEX_OPER:
            sOetIndex = IDV_OPTM_INDEX_DRDB_DML_INDEX_OPER;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_CREATE_PAGE:
            sOetIndex = IDV_OPTM_INDEX_DRDB_CREATE_PAGE;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_GET_PAGE:
            sOetIndex = IDV_OPTM_INDEX_DRDB_GET_PAGE;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_FIX_PAGE:
            sOetIndex = IDV_OPTM_INDEX_DRDB_FIX_PAGE;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_TRANS_LOGICAL_AGING:
            sOetIndex = IDV_OPTM_INDEX_DRDB_TRANS_LOGICAL_AGING;
            break;
        case IDV_STAT_INDEX_OPTM_DRDB_TRANS_PHYSICAL_AGING:
            sOetIndex = IDV_OPTM_INDEX_DRDB_TRANS_PHYSICAL_AGING;
            break;
        case IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE:
            sOetIndex = IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE;
            break;
        case IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_VICTIM_FREE:
            sOetIndex = IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_VICTIM_FREE;
            break;
            /*fix BUG-30855 It needs to describe soft prepare time in detail
              for problem tracking . */
        case IDV_STAT_INDEX_OPTM_PLAN_HARD_REBUILD:
            sOetIndex = IDV_OPTM_INDEX_PLAN_HARD_REBUILD;
            break;
            
        case IDV_STAT_INDEX_OPTM_SOFT_REBUILD:
            sOetIndex = IDV_OPTM_INDEX_SOFT_REBUILD;
            break;

        case IDV_STAT_INDEX_OPTM_PLANCACHE_CHECK_IN_BY_HARD_PREPARE:
            sOetIndex = IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_PREPARE;
            break;

        case IDV_STAT_INDEX_OPTM_PLANCACHE_CHECK_IN_BY_HARD_REBUILD:
            sOetIndex = IDV_OPTM_INDEX_PLANCACHE_CHECK_IN_BY_HARD_REBUILD;
            break;
            
        case IDV_STAT_INDEX_OPTM_PLANCACHE_SEARCH_PPCO:
            sOetIndex = IDV_OPTM_INDEX_PLANCACHE_SEARCH_CHILD_PCO;
            break;

        case IDV_STAT_INDEX_OPTM_PLANCACHE_CREATE_PPCO:
            sOetIndex = IDV_OPTM_INDEX_PLANCACHE_CREATE_PPCO;
            break;

        case IDV_STAT_INDEX_OPTM_PLANCACHE_SEARCH_CHILD_PCO:
            sOetIndex = IDV_OPTM_INDEX_PLANCACHE_SEARCH_CHILD_PCO;
            break;
            
        case IDV_STAT_INDEX_OPTM_PLANCACHE_CREATE_CHILD_PCO:
            sOetIndex =  IDV_OPTM_INDEX_PLANCACHE_CREATE_CHILD_PCO;
            break;

        case IDV_STAT_INDEX_OPTM_VALIDATE_PCO:
            sOetIndex = IDV_OPTM_INDEX_VALIDATE_PCO;
            break;

        case IDV_STAT_INDEX_OPTM_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC:
            sOetIndex = IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_EXEC;
            break;

        case IDV_STAT_INDEX_OPTM_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE:
            sOetIndex = IDV_OPTM_INDEX_CREATE_NEW_CHILD_PCO_BY_REBUILD_AT_SOFT_PREPARE;
            break;

        case IDV_STAT_INDEX_OPTM_HARD_PREPARE:
            sOetIndex = IDV_OPTM_INDEX_HARD_PREPARE;
            break;

        case IDV_STAT_INDEX_OPTM_MATCHING_CHILD_PCO:
            sOetIndex =IDV_OPTM_INDEX_MATCHING_CHILD_PCO;
            break;

        case IDV_STAT_INDEX_OPTM_WAITING_HARD_PREPARE:
            sOetIndex = IDV_OPTM_INDEX_WAITING_HARD_PREPARE;
            break;

        case IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT:
            sOetIndex = IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_MOVE_COLD_TO_HOT ;
            break;

        case IDV_STAT_INDEX_OPTM_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO:
            sOetIndex = IDV_OPTM_INDEX_PLAN_CACHE_IN_REPLACE_WAIT_PARENT_PCO;
            break;

        case IDV_STAT_INDEX_OPTM_INDEX_CHECK_PRIVILEGE_PCO:
            sOetIndex = IDV_OPTM_INDEX_CHECK_PRIVILEGE_PCO;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_COPY_LOG_TO_REPLBUFFER:
            sOetIndex = IDV_OPTM_INDEX_RP_S_COPY_LOG_TO_REPLBUFFER;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_WAIT_NEW_LOG:
            sOetIndex = IDV_OPTM_INDEX_RP_S_WAIT_NEW_LOG;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_REPLBUFFER:
            sOetIndex = IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_REPLBUFFER;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_FILE:
            sOetIndex = IDV_OPTM_INDEX_RP_S_READ_LOG_FROM_FILE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_CHECK_USEFUL_LOG:
            sOetIndex = IDV_OPTM_INDEX_RP_S_CHECK_USEFUL_LOG;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_LOG_ANALYZE:
            sOetIndex = IDV_OPTM_INDEX_RP_S_LOG_ANALYZE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_SEND_XLOG:
            sOetIndex = IDV_OPTM_INDEX_RP_S_SEND_XLOG;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_RECV_ACK:
            sOetIndex = IDV_OPTM_INDEX_RP_S_RECV_ACK;
            break;

        case IDV_STAT_INDEX_OPTM_RP_S_SET_ACKEDVALUE:
            sOetIndex = IDV_OPTM_INDEX_RP_S_SET_ACKEDVALUE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_RECV_XLOG:
            sOetIndex = IDV_OPTM_INDEX_RP_R_RECV_XLOG;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_CONVERT_ENDIAN:
            sOetIndex = IDV_OPTM_INDEX_RP_R_CONVERT_ENDIAN;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_TX_BEGIN:
            sOetIndex = IDV_OPTM_INDEX_RP_R_TX_BEGIN;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_TX_COMMIT:
            sOetIndex = IDV_OPTM_INDEX_RP_R_TX_COMMIT;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_TX_ABORT:
            sOetIndex = IDV_OPTM_INDEX_RP_R_TX_ABORT;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_OPEN:
            sOetIndex = IDV_OPTM_INDEX_RP_R_TABLE_CURSOR_OPEN;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_CLOSE:
            sOetIndex = IDV_OPTM_INDEX_RP_R_TABLE_CURSOR_CLOSE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_INSERT_ROW:
            sOetIndex = IDV_OPTM_INDEX_RP_R_INSERT_ROW;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_UPDATE_ROW:
            sOetIndex = IDV_OPTM_INDEX_RP_R_UPDATE_ROW;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_DELETE_ROW:
            sOetIndex = IDV_OPTM_INDEX_RP_R_DELETE_ROW;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_OPEN_LOB_CURSOR:
            sOetIndex = IDV_OPTM_INDEX_RP_R_OPEN_LOB_CURSOR;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_PREPARE_LOB_WRITE:
            sOetIndex = IDV_OPTM_INDEX_RP_R_PREPARE_LOB_WRITE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_WRITE_LOB_PIECE:
            sOetIndex = IDV_OPTM_INDEX_RP_R_WRITE_LOB_PIECE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_FINISH_LOB_WRITE:
            sOetIndex = IDV_OPTM_INDEX_RP_R_FINISH_LOB_WRITE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_CLOSE_LOB_CURSOR:
            sOetIndex = IDV_OPTM_INDEX_RP_R_CLOSE_LOB_CURSOR;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_COMPARE_IMAGE:
            sOetIndex = IDV_OPTM_INDEX_RP_R_COMPARE_IMAGE;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_SEND_ACK:
            sOetIndex = IDV_OPTM_INDEX_RP_R_SEND_ACK;
            break;

        case IDV_STAT_INDEX_OPTM_RP_R_TRIM_LOB:
            sOetIndex = IDV_OPTM_INDEX_RP_R_TRIM_LOB;
            break;

        case IDV_STAT_INDEX_OPTM_TASK_SCHEDULE:
            sOetIndex = IDV_OPTM_INDEX_TASK_SCHEDULE;
            break;

        default:
            IDE_CALLBACK_FATAL(" error invalid statistics index !!" );
            break;
    }

    if ( IDV_TIME_AVAILABLE() )
    {
        sOetTime.mATD.mAccumTime =
            IDV_TIMEBOX_GET_ACCUM_TIME(
                    IDV_SQL_OPTIME_DIRECT(aCurrSQL, sOetIndex));

        if ( (sOetTime.mATD.mAccumTime > 0) &&
                (IDV_TIMEBOX_GET_TIME_SWITCH(
                IDV_SQL_OPTIME_DIRECT(aCurrSQL, sOetIndex))
                 == IDV_TIME_SWITCH_OFF))
        {
            /*
             * 이미 계산되어 있으므로 경과시간을 누적만 하면 된다.
             */
            IDV_SESS_ADD( aCurrSess,
                    aStatIdx,
                    sOetTime.mATD.mAccumTime );

            IDV_TIMEBOX_ACCUMULATED(
                    IDV_SQL_OPTIME_DIRECT(aCurrSQL, sOetIndex) );
        }
    }
}

/*
 * 계속 증가하는 값의 속성을 갖는 통계정보를 누적시킨다.
 * 예를들어, OAread page count, write page count와 같은
 * 통계정보등이 있다.
 * 경과시간에 대한 상대적 증감을 통계정보는 본 함수를
 * 사용해서는 안된다.
 */
void idvManager::applyStatEventToSystem( idvStatIndex  aStatIdx,
                                         idvStatEvent *aCurr,
                                         idvStatEvent *aOld )
{
    IDE_DASSERT( aCurr != NULL );
    IDE_DASSERT( aOld  != NULL );

    IDV_SYS_ADD(aStatIdx, (aCurr->mValue - aOld->mValue));
    aOld->mValue = aCurr->mValue;
}

void idvManager::applyLatchStatToSystem( UInt     aPageTypeIdx,
                                         ULong   *aCurrMissCnt,
                                         ULong   *aOldMissCnt,
                                         ULong   *aCurrMissTime,
                                         ULong   *aOldMissTime )
{
    IDE_DASSERT( aCurrMissCnt != NULL );
    IDE_DASSERT( aOldMissCnt  != NULL );
    IDE_DASSERT( aCurrMissTime != NULL );
    IDE_DASSERT( aOldMissTime  != NULL );

    IDV_SYS_LATCH_STAT_ADD(aPageTypeIdx,
                           (*aCurrMissCnt - *aOldMissCnt),
                           (*aCurrMissTime - *aOldMissTime));
    *aOldMissCnt  = *aCurrMissCnt;
    *aOldMissTime = *aCurrMissTime;
}

void idvManager::applyWaitEventToSystem( idvWaitIndex  aWaitIdx,
                                         idvWaitEvent *aCurr,
                                         idvWaitEvent *aOld )
{
    idvWaitEvent   * sWePtr;
    ULong            sDiff;

    IDE_DASSERT( aCurr != NULL );
    IDE_DASSERT( aOld  != NULL );

    sWePtr = IDV_SYSTEM_WAIT_EVENT( aWaitIdx );

    sDiff = aCurr->mTotalWaits - aOld->mTotalWaits;
    sWePtr->mTotalWaits += sDiff;

    sDiff = aCurr->mTotalTimeOuts - aOld->mTotalTimeOuts;
    sWePtr->mTotalTimeOuts += sDiff;

    sDiff = aCurr->mTimeWaitedMicro - aOld->mTimeWaitedMicro;
    sWePtr->mTimeWaitedMicro += sDiff;

    if ( aCurr->mMaxWait > sWePtr->mMaxWait )
    {
       sWePtr->mMaxWait  = aCurr->mMaxWait;
    }

    memcpy( aOld, aCurr, ID_SIZEOF(idvWaitEvent) );
}

/* ------------------------------------------------
 *  Fixed Table Define for CmDetector
 * ----------------------------------------------*/

static iduFixedTableColDesc gStatNameColDesc[] =
{
    // task
    {
        (SChar *)"SEQNUM",
        offsetof(idvStatName, mSeqNum),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"NAME",
        offsetof(idvStatName, mName),
        128,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER | IDU_FT_COLUMN_INDEX,
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


void idvManager::beginWaitEvent( void      * aStatSQL,
                                 void      * aWeArgs )
{
    idvWaitEvent * sWePtr;
    idvSQL       * sStatSQL = (idvSQL*) aStatSQL;

    IDE_DASSERT( aStatSQL!= NULL );

    IDE_TEST_RAISE( aWeArgs == NULL, cont_finish );
    IDE_TEST_RAISE( iduProperty::getTimedStatistics() == 0, cont_finish );

    IDE_DASSERT( ((idvWeArgs*)aWeArgs)->mWaitEventID != IDV_WAIT_INDEX_NULL );

    if ( sStatSQL->mSess != NULL )
    {
        /*
         * [1] Session 통계정보 처리
         * a. 총 대기회수 1 증가
         */
        IDE_DASSERT( ((idvWeArgs*)aWeArgs)->mWaitEventID
                == *(idvWaitIndex*)aWeArgs );

        sWePtr = IDV_SESSION_WAIT_EVENT( sStatSQL->mSess,
                *(UInt*)aWeArgs);
        sWePtr->mTotalWaits += 1;
    }
    else
    {
        /* system thread와 같이 session이 존재하지 않는 경우에는
         * 본 함수에서 통계정보를 누적시키지 않는다 */
    }
    /*
     * [2] SQL 통계정보 처리
     * a. 대기이벤트 파라미터 설정
     */
    IDV_SQL_SET( sStatSQL, mWeArgs, *(idvWeArgs*)aWeArgs );

    // SQL에 대기이벤트 대기시간 측정시작
    IDV_TIMEBOX_BEGIN(
            IDV_SQL_WAIT_TIME_DIRECT( sStatSQL ));

    IDE_EXCEPTION_CONT( cont_finish );
}


void idvManager::endWaitEvent( void   * aStatSQL,
                               void   * aWeArgs )
{
    UInt           sPageTypeIndex;
    ULong          sElaTime;
    idvWaitEvent * sWePtr;
    idvSQL       * sStatSQL = (idvSQL*) aStatSQL;

    IDE_ASSERT( aStatSQL != NULL );

    IDE_TEST_RAISE( aWeArgs == NULL, cont_finish );
    IDE_TEST_RAISE(((idvWeArgs*)aWeArgs)->mWaitEventID == IDV_WAIT_INDEX_NULL, 
                   cont_finish );
    IDE_TEST_RAISE( iduProperty::getTimedStatistics() == 0, cont_finish );

    // SQL에 대기이벤트 대기시간의 측정을 완료한다.
    IDV_TIMEBOX_END(
            IDV_SQL_WAIT_TIME_DIRECT( sStatSQL ));

    if ( sStatSQL->mSess != NULL )
    {
        sWePtr = IDV_SESSION_WAIT_EVENT( sStatSQL->mSess, 
                                         ((idvWeArgs*)aWeArgs)->mWaitEventID);

        sElaTime =
            IDV_TIMEBOX_GET_ELA_TIME(
                    IDV_SQL_WAIT_TIME_DIRECT( sStatSQL ));

        // 총 대기시간 ( micro sec. 단위 )
        sWePtr->mTimeWaitedMicro += sElaTime;

        // 최대대기시간
        if ( ( sElaTime / 1000 ) > sWePtr->mMaxWait )
        {
            sWePtr->mMaxWait = ( sElaTime / 1000 );
        }

        IDV_TIMEBOX_ACCUMULATED(
                IDV_SQL_WAIT_TIME_DIRECT( sStatSQL ));

        if ( sStatSQL->mWeArgs.mWaitEventID ==
                IDV_WAIT_INDEX_LATCH_BUFFER_BUSY_WAITS )
        {
            sPageTypeIndex =
                sStatSQL->mWeArgs.mWaitParam[IDV_WAIT_PARAM_3];

            /* BUG-24092: [SD] BufferMgr에서 BCB의 Latch Stat을 갱신시 Page Type이
             * Invalid하여 서버가 죽습니다.
             *
             * sPageTypeIndex가 Invalid할 경우 Warning Msg를 찍고 넘어간다. */
            if( sPageTypeIndex < IDV_SM_PAGE_TYPE_MAX  )
            {
                sStatSQL->mSess->mMissCnt[sPageTypeIndex]++;
                sStatSQL->mSess->mMissTime[sPageTypeIndex] += sElaTime;
            }
            else
            {
                ideLog::log( IDE_SERVER_0,
                        ID_TRC_INVALID_LATCH_PAGE_IDX,
                        sPageTypeIndex );
            }
        }

        IDV_WEARGS_INIT( &(sStatSQL->mWeArgs) );
    }
    else
    {
        /* system thread와 같이 session이 존재하지 않는 경우에는
         * 본 함수에서 통계정보를 누적시키지 않는다 */
    }

    IDE_EXCEPTION_CONT( cont_finish );
}

/*
 * 연산의 경과시간을 측정시작한다.
 */
void idvManager::beginOpTime( idvSQL       * aStatSQL,
                              idvOperTimeIndex   aOpIdx )
{
    IDE_DASSERT( aStatSQL != NULL );
    IDE_DASSERT( aOpIdx != IDV_OPTM_INDEX_NULL );

    if ( iduProperty::getTimedStatistics() == 1 )
    {
        IDV_TIMEBOX_BEGIN(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, aOpIdx ));
    }
}

/*
 * 연산의 경과시간을 측정완료한다.
 */
void idvManager::endOpTime( idvSQL              * aStatSQL,
                            idvOperTimeIndex   aOpIdx )
{
    IDE_DASSERT( aStatSQL != NULL );
    IDE_DASSERT( aOpIdx != IDV_OPTM_INDEX_NULL );

    if ( iduProperty::getTimedStatistics() == 1 )
    {
        IDV_TIMEBOX_END(
            IDV_SQL_OPTIME_DIRECT( aStatSQL, aOpIdx ));
    }
}

/* BUG-29005 - Fullscan 성능 문제
 * idvSQL에서 Session ID를 가져오는 함수 추가함.
 * SM에서 callback으로 사용 */
UInt idvManager::getSessionID( idvSQL  *aStatSQL )
{
    UInt sSID = 0;

    if ( (aStatSQL != NULL) && (aStatSQL->mSess != NULL) )
    {
        sSID = aStatSQL->mSess->mSID;
    }

    return sSID;
}


IDE_RC idvManager::buildRecordForStatName(idvSQL              */*aStatistics*/,
                                          void                *aHeader,
                                          void                */* aDumpObj */,
                                          iduFixedTableMemory *aMemory)
{
    ULong    sNeedRecCount;
    UInt     i;
    void   * sIndexValues[1];

    sNeedRecCount = IDV_STAT_INDEX_MAX;

    for (i = 0; i < sNeedRecCount; i++)
    {
        /* BUG-43006 FixedTable Indexing Filter
         * Indexing Filter를 사용해서 전체 Record를 생성하지않고
         * 부분만 생성해 Filtering 한다.
         * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
         * 해당하는 값을 순서대로 넣어주어야 한다.
         * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
         * 어 주어야한다.
         */
        sIndexValues[0] = &mStatName[i].mName;
        if ( iduFixedTable::checkKeyRange( aMemory,
                                           gStatNameColDesc,
                                           sIndexValues )
                == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) &mStatName[i])
                 != IDE_SUCCESS);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gStatNameTableDesc =
{
    (SChar *)"X$STATNAME",
    idvManager::buildRecordForStatName,
    gStatNameColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/* ------------------------------------------------
 *  Fixed Table Define for CmDetector
 * ----------------------------------------------*/

static iduFixedTableColDesc gWaitEventNameColDesc[] =
{
    // task
    {
        (SChar *)"EVENT_ID",
        offsetof(idvWaitEventName, mSeqNum),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"EVENT",
        offsetof(idvWaitEventName, mName),
        128,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"WAIT_CLASS_ID",
        offsetof(idvWaitEventName, mWaitClassID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
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


IDE_RC idvManager::buildRecordForWaitEventName(
    idvSQL              */*aStatistics*/,
    void                *aHeader,
    void                */* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    ULong    sNeedRecCount;
    UInt     i;
    void   * sIndexValues[1];

    sNeedRecCount = IDV_WAIT_INDEX_MAX;

    for (i = 0; i <= sNeedRecCount; i++)
    {
        /* BUG-43006 FixedTable Indexing Filter
         * Indexing Filter를 사용해서 전체 Record를 생성하지않고
         * 부분만 생성해 Filtering 한다.
         * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
         * 해당하는 값을 순서대로 넣어주어야 한다.
         * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
         * 어 주어야한다.
         */
        sIndexValues[0] = &mWaitEventName[i].mName;
        if ( iduFixedTable::checkKeyRange( aMemory,
                                           gWaitEventNameColDesc,
                                           sIndexValues )
                == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) &mWaitEventName[i])
                 != IDE_SUCCESS);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gWaitEventNameTableDesc =
{
    (SChar *)"X$WAIT_EVENT_NAME",
    idvManager::buildRecordForWaitEventName,
    gWaitEventNameColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  Fixed Table Define for CmDetector
 * ----------------------------------------------*/

static iduFixedTableColDesc gWaitClassNameColDesc[] =
{
    // task
    {
        (SChar *)"WAIT_CLASS_ID",
        offsetof(idvWaitClassName, mSeqNum),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar *)"WAIT_CLASS",
        offsetof(idvWaitClassName, mName),
        128,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
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


IDE_RC idvManager::buildRecordForWaitClassName(
    idvSQL              */*aStatistics*/,
    void                *aHeader,
    void                */*aDumpObj*/,
    iduFixedTableMemory *aMemory )
{
    ULong    sNeedRecCount;
    UInt     i;

    sNeedRecCount = IDV_WCLASS_INDEX_MAX;

    for (i = 0; i < sNeedRecCount; i++)
    {
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) &mWaitClassName[i])
                 != IDE_SUCCESS);
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gWaitClassNameTableDesc =
{
    (SChar *)"X$WAIT_CLASS_NAME",
    idvManager::buildRecordForWaitClassName,
    gWaitClassNameColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
