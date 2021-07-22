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

#ifndef _O_MMC_DEF_H_
#define _O_MMC_DEF_H_ 1


typedef UInt mmcStmtID;
typedef UInt mmcSessID;

/* PROJ-2177 User Interface - Cancel */
typedef UInt mmcStmtCID;

#define MMC_STMT_ID_NONE                0
#define MMC_STMT_CID_NONE               0

#define MMC_STMT_ID_CACHE_COUNT         10
#define MMC_STMT_ID_MAP_SIZE            10
#define MMC_STMT_ID_POOL_ELEM_COUNT     8

/* BUG-47238 */
# define MMC_STMT_NEED_UNLOCK_MASK        (0x00000001)
# define MMC_STMT_NEED_UNLOCK_FALSE       (0x00000000)
# define MMC_STMT_NEED_UNLOCK_TRUE        (0x00000001)

//fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
//그들간의 관계를 정확히 유지해야 함.
typedef UInt mmcTransID;

#define MMC_QUERY_IP_LEN         512
#define MMC_QUERY_TYPE_COUNT     4
#define MMC_QUERY_TYPE_INSERT    0
#define MMC_QUERY_TYPE_UPDATE    1
#define MMC_QUERY_TYPE_DELETE    2
#define MMC_QUERY_TYPE_SELECT    3
#define MMC_CONV_NUMERIC_BUFFER_SIZE 20

#define MMC_RESULTSET_FIRST      0
#define MMC_RESULTSET_ALL        ID_USHORT_MAX

// BUG-24075 [MM] BIND COL 정보를 조회할 때,
//                DISPLAY_NAME_SIZE를 최대 50으로 잡아야 합니다
#define MMC_MAX_DISPLAY_NAME_SIZE    (42)

//fix BUG-22365 Query Len 1k->16K
#define MMC_STMT_QUERY_LEN     (16 * 1024)
#define MMC_STMT_SQL_TEXT_LEN  64
#define MMC_STMT_PLAN_TEXT_LEN 64
#define MMC_STMT_PLAN_MAX_LEN  (32 * 1024)
#define MMC_SQL_CACHE_TEXT_ID_LEN (64)
#define MMC_SQL_CACHE_TEXT_ID_BUCKET_DIGIT (4)  /* BUG-46158 */
// fix BUG-21429
#define MMC_SQL_PCB_MUTEX_NAME_LEN (MMC_SQL_CACHE_TEXT_ID_LEN+64)
typedef enum
{
    MMC_SERVICE_THREAD_TYPE_SOCKET, // Shared Socket Service Thread : TCP, UNIX
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    MMC_SERVICE_THREAD_TYPE_DEDICATED, // Dedicated Socket Service Thread : TCP, UNIX
    MMC_SERVICE_THREAD_TYPE_IPC,    // Dedicated IPC Service Thread : IPC
    MMC_SERVICE_THREAD_TYPE_DA,     //fix PROJ-1749 
    MMC_SERVICE_THREAD_TYPE_IPCDA, // IPCDA Service Thread : IPCDA
    MMC_SERVICE_THREAD_TYPE_MAX
} mmcServiceThreadType;

typedef enum
{
    MMC_TASK_STATE_WAITING = 0,
    MMC_TASK_STATE_READY,
    MMC_TASK_STATE_EXECUTING,
    MMC_TASK_STATE_QUEUEWAIT,
    MMC_TASK_STATE_QUEUEREADY,

    MMC_TASK_STATE_MAX
} mmcTaskState;


typedef enum
{
    MMC_SESSION_STATE_INIT = 0, // 초기 상태
    MMC_SESSION_STATE_AUTH,     // 인증 상태
    MMC_SESSION_STATE_READY,    // 서비스 준비 상태 (SQL 수행 안됨. 예: xa_start되지 않은 XA 세션)
    MMC_SESSION_STATE_SERVICE,  // 일반 서비스 처리 상태
    MMC_SESSION_STATE_END,      // disconnect에 의해 정상 종료
    MMC_SESSION_STATE_ROLLBACK, // 비정상 종료(Network error..)

    MMC_SESSION_STATE_MAX
} mmcSessionState;


typedef enum
{
    MMC_STMT_EXEC_PREPARED = 0,
    MMC_STMT_EXEC_DIRECT,

    MMC_STMT_EXEC_MODE_MAX
} mmcStmtExecMode;


typedef enum
{
    MMC_STMT_STATE_ALLOC = 0,     // statement 초기시작
    MMC_STMT_STATE_PREPARED,      // prepare가 끝났을 때
    MMC_STMT_STATE_EXECUTED,      // execute가 끝났을 때

    MMC_STMT_STATE_MAX
} mmcStmtState;

/* PROJ-1381, BUG-33121 FAC : MMC_RESULTSET_STATE_FETCH_END 추가 */
typedef enum
{
    MMC_RESULTSET_STATE_ERROR = 0,
    MMC_RESULTSET_STATE_INITIALIZE,    /**< Result Set 초기화 */
    MMC_RESULTSET_STATE_FETCH_READY,   /**< Result Set Fetch 시작 */
    MMC_RESULTSET_STATE_FETCH_PROCEED, /**< Result Set Fetch 진행 */
    MMC_RESULTSET_STATE_FETCH_END,     /**< Result Set Fetch End */
    MMC_RESULTSET_STATE_FETCH_CLOSE,   /**< Result Set Fetch 종료 */

    MMC_RESULTSET_STATE_MAX
} mmcResultSetState;

// PROJ-2256 Communication protocol for efficient query result transmission
typedef struct mmcBaseRow
{
    UChar  *mBaseRow;
    UInt    mBaseRowSize;
    UInt    mBaseColumnPos;
    // PROJ-2331
    UInt    mCompressedSize4CurrentRow;

    idBool  mIsFirstRow;
    idBool  mIsRedundant;
} mmcBaseRow;

typedef struct mmcResultSet
{
    mmcResultSetState  mResultSetState;
    void              *mResultSetStmt;
    idBool             mRecordExist;
    idBool             mInterResultSet;
    mmcBaseRow         mBaseRow;        // PROJ-2256
} mmcResultSet;

typedef enum
{
    MMC_STMT_BIND_NONE = 0, // Bind Param이 안된 상태
    MMC_STMT_BIND_INFO,     // Bind Param Info가 수행된 상태
    MMC_STMT_BIND_DATA      // Bind Param Data가 수행된 상태
} mmcStmtBindState;


typedef enum
{
    MMC_COMMITMODE_NONAUTOCOMMIT = 0,
    MMC_COMMITMODE_AUTOCOMMIT,

    MMC_COMMITMODE_MAX
} mmcCommitMode;


typedef enum
{
    MMC_BYTEORDER_BIG_ENDIAN = 0,
    MMC_BYTEORDER_LITTLE_ENDIAN,

    MMC_BYTEORDER_MAX
} mmcByteOrder;

//PROJ-1677 DEQUEUE
enum
{
    MMC_DID_NOT_PARTIAL_ROLLBACK=0,
    MMC_DID_PARTIAL_ROLLBACK
};
typedef UChar   mmcPartialRollbackFlag;

//PROJ-1436 SQL Plan Cache.
// PCB State
typedef UInt mmcPCBState;


typedef enum
{
    MMC_PCB_IN_NONE_LRU_REGION=0,
    MMC_PCB_IN_COLD_LRU_REGION,
    MMC_PCB_IN_HOT_LRU_REGION
}mmcPCBLRURegion;

typedef enum
{
    MMC_PCO_LOCK_RELEASED = 0,
    MMC_PCO_LOCK_ACQUIRED_SHARED,
    MMC_PCO_LOCK_ACQUIRED_EXECL
}mmcPCOLatchState;

typedef enum
{
    MMC_CHILD_PCO_PLAN_IS_NOT_READY = 0,
    MMC_CHILD_PCO_PLAN_IS_READY,
    MMC_CHILD_PCO_PLAN_NEED_HARD_PREPARE,
    MMC_CHILD_PCO_PLAN_IS_UNUSED
}mmcChildPCOPlanState;

typedef enum
{
    MMC_CHILD_PCO_ENV_IS_NOT_READY = 0,
    MMC_CHILD_PCO_ENV_IS_READY
}mmcChildPCOEnvState;

// child PCO를 생성한 이유.
typedef enum
{
    MMC_CHILD_PCO_IS_CACHE_MISS =0,
    MMC_PLAN_RECOMPILE_BY_SOFT_PREPARE,
    MMC_PLAN_RECOMPILE_BY_EXECUTION
}mmcChildPCOCR;

// softPrepare mode
//fix BUG-22061
typedef enum
{
    MMC_SOFT_PREPARE_FOR_PREPARE=0,
    MMC_SOFT_PREPARE_FOR_REBUILD
}mmcSoftPrepareReason;

typedef enum  /* BUG-46158 */
{
    MMC_PCO_PLAN_CACHE_UNKEEP = 0,
    MMC_PCO_PLAN_CACHE_KEEP   = 1
} mmcPCOPlanCacheKeep;


//fix BUG-30891
typedef enum
{
    MMC_EXECUTION_FLAG_FAILURE = 0,
    MMC_EXECUTION_FLAG_REBUILD,
    MMC_EXECUTION_FLAG_RETRY,
    MMC_EXECUTION_FLAG_QUEUE_EMPTY,
    MMC_EXECUTION_FLAG_SUCCESS
} mmcExecutionFlag;

// fix BUG-30913
typedef enum
{
    MMC_FETCH_FLAG_PROCEED = 0,
    MMC_FETCH_FLAG_CLOSE,
    MMC_FETCH_FLAG_NO_RESULTSET,
    MMC_FETCH_FLAG_INVALIDATED          /**< Fetch invalidated */
} mmcFetchFlag;

//-----------------------------------
// X$SQL_PLAN_CACHE_SQLTEXT 의 구조
//-----------------------------------
typedef struct mmcParentPCOInfo4PerfV
{
    SChar               mSQLTextId[MMC_SQL_CACHE_TEXT_ID_LEN + 1]; // SQL_TEXT_ID
    SChar*              mSQLString4SoftPrepare;       // SQL_TEXT
    UInt                mChildCnt;                    // CHILD_PCO_COUNT
    UInt                mChildCreateCnt;              // CHILD_PCO_CREATE_COUNT
    mmcPCOPlanCacheKeep mPlanCacheKeep;               /* PLAN_CACHE_KEEP */
} mmcParentPCOInfo4PerfV;

//-------------------------------
// X$SQL_PLAN_CACHE_PCO 의 구조
//-------------------------------
typedef struct mmcChildPCOInfo4PerfV
{
    SChar                *mSQLTextId;          // SQL_TEXT_ID
    UInt                  mChildID;            // PCO_ID
    mmcChildPCOCR         mCreateReason;       // CREATE_REASON
    //fix BUG-31169,The LRU region of a PCO has better to be showed in V$SQL_PLAN_CACHE_PCO
    mmcPCBLRURegion       mLruRegion;          // LRU region
    UInt                 *mHitCntPtr;          // HIT_COUNT
    UInt                  mRebuildedCnt;       // REBUILD_COUNT
    mmcChildPCOPlanState  mPlanState;          // PLAN_STATE
    UInt                  mPlanSize;           /* PLAN_SIZE */
    UInt                  mFixCount;           /* PLAN_FIX_COUNT */
    mmcPCOPlanCacheKeep   mPlanCacheKeep;      /* PLAN_CACHE_KEEP */
} mmcChildPCOInfo4PerfV;
/*  fix BUG-31408,[mm-protocols] It need to describe mmERR_ABORT_INVALID_DATA_SIZE error message in detail.*/
typedef struct mmcMtTypeName
{
    UInt          mMtType;
    const SChar  *mTypeName;
}mmcMtTypeName;

/* PROJ-1381 Fetch Across Commits */

/* Cursor Hold */
typedef enum
{
    MMC_STMT_CURSOR_HOLD_ON = 0,    /**< Holdable (default) */
    MMC_STMT_CURSOR_HOLD_OFF,       /**< Non-Holdable */

    MMC_STMT_CURSOR_HOLD_MAX
} mmcStmtCursorHold;

/* Cursor Close Mode */
typedef enum
{
    MMC_CLOSEMODE_NON_COMMITED,     /**< Commit 된 적 없는 모든 Fetch의 커서를 닫는다. */
    MMC_CLOSEMODE_REMAIN_HOLD,      /**< Holdable Fetch는 남기고, Non-Holdable Fetch의 커서만 닫는다. */

    MMC_CLOSEMODE_MAX
} mmcCloseMode;

/* PROJ-1789 Updatable Scrollable Cursor */

typedef enum
{
    MMC_STMT_KEYSETMODE_OFF = 0,    /**< Keyset 모드로 커서를 연다. */
    MMC_STMT_KEYSETMODE_ON,         /**< Non-Keyset 모드로 커서를 연다. */

    MMC_STMT_KEYSETMODE_MAX
} mmcStmtKeysetMode;

/* PROJ-2436 ADO.NET MSDTC */
typedef enum
{
    MMC_TRANS_DO_NOT_ESCALATE = 0,  /**< Do not escalate */
    MMC_TRANS_ESCALATE,             /**< Escalate to a distributed transaction. */

    MMC_TRANS_ESCALATION_MAX
} mmcTransEscalation;

/* PROJ-2626 Snapshot Export */
typedef enum
{
    MMC_CLIENT_APP_INFO_TYPE_NONE = 0,
    MMC_CLIENT_APP_INFO_TYPE_ILOADER,
    MMC_CLIENT_APP_INFO_TYPE_MAX
} mmcClientAppInfoType;

/* BUG-46019 MESSAGE_CALLBACK */
typedef enum
{
    MMC_MESSAGE_CALLBACK_UNREG   = 0,  /* 클라이언트 메시지콜백 미등록 */
    MMC_MESSAGE_CALLBACK_REG     = 1,  /* 클라이언트 메시지콜백 등록 */
    MMC_MESSAGE_CALLBACK_UNKNOWN = 2   /* 초기값 */
} mmcMessageCallback;

#endif
