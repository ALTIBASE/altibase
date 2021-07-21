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
 * $Id: rp.h 90612 2021-04-15 08:16:38Z donghyun1 $
 **********************************************************************/

#ifndef _O_RP_H_
#define _O_RP_H_ 1

#include <smDef.h>
#include <iduMutex.h>
#include <rpError.h>

#define RP_IN
#define RP_OUT
#define RP_INOUT

#define REPLICATION_MAJOR_VERSION       (7)
#define REPLICATION_MINOR_VERSION       (4)
#define REPLICATION_FIX_VERSION         (8)

#ifdef ENDIAN_IS_BIG_ENDIAN

#ifdef COMPILE_64BIT
#define REPLICATION_ENDIAN_64BIT        (1) // BIG 64bit
#else
#define REPLICATION_ENDIAN_64BIT        (2) // BIG 32bit
#endif /* COMPILE_64BIT */

#else

#ifdef COMPILE_64BIT
#define REPLICATION_ENDIAN_64BIT        (3) // LITTLE 64bit
#else
#define REPLICATION_ENDIAN_64BIT        (4) // LITTLE 32bit
#endif /* COMPILE_64BIT */

#endif /* ENDIAN_IS_BIG_ENDIAN */

#define RP_MAKE_VERSION( major, minor, fix, endian )   \
            ( (((ULong)((UShort)(major)  & 0xFFFF)) << 48) | \
			  (((ULong)((UShort)(minor)  & 0xFFFF)) << 32) | \
			  (((ULong)((UShort)(fix)    & 0xFFFF)) << 16) | \
			  (((ULong)((UShort)(endian) & 0xFFFF)))  )

#define RP_GET_MAJOR_VERSION(a) (UInt)(((ULong)(a) >> 48) & 0xFFFF)
#define RP_GET_MINOR_VERSION(a) (UInt)(((ULong)(a) >> 32) & 0xFFFF)
#define RP_GET_FIX_VERSION(a)   (UInt)(((ULong)(a) >> 16)  & 0xFFFF)
#define RP_GET_ENDIAN_64BIT(a)  (UInt)((ULong)(a) & 0xFFFF)

/* Base protocol*/
#define RP_7_4_1_VERSION ( RP_MAKE_VERSION( 7, 4, 1, 0 ) )
/* mDictTableCount protocol for handshaking is added */
#define RP_7_4_2_VERSION ( RP_MAKE_VERSION( 7, 4, 2, 0 ) )
/* PROJ-2737 Internal replication - start/sync conditional */
#define RP_CONDITIONAL_START_VERSION ( RP_MAKE_VERSION( 7, 4, 7, 0 ) )

#define RP_CURRENT_VERSION ( RP_MAKE_VERSION( REPLICATION_MAJOR_VERSION, \
                                                REPLICATION_MINOR_VERSION, \
                                                REPLICATION_FIX_VERSION, \
                                                REPLICATION_ENDIAN_64BIT ) )

#define RP_DBG_LV_ -999

#if RP_DBG_LV_ > 0
#define RP_DBG_PRINTLINE()\
    printf("%s:%d:DEBUG!!\n",__FILE__, __LINE__); fflush(stdout);
#else
#define RP_DBG_PRINTLINE()
#endif

/* BUG-31545 RP 성능 통계 정보를 수집*/
/* RP_CREATE_FLAG_VARIABLE 매크로는 변수 선언부에 둔다. */
#define RP_CREATE_FLAG_VARIABLE( __Idx__ ) idBool sFlag_ ## __Idx__  = ID_FALSE;
#define RP_OPTIMIZE_TIME_BEGIN( __idvBasePtr__, __Idx__ )  \
{                                                          \
    IDV_SQL_OPTIME_BEGIN( __idvBasePtr__, __Idx__ );       \
    sFlag_ ## __Idx__ = ID_TRUE;                           \
}
#define RP_OPTIMIZE_TIME_END( __idvBasePtr__, __Idx__ )    \
{                                                          \
    if ( sFlag_ ## __Idx__ == ID_TRUE )                    \
    {                                                      \
        IDV_SQL_OPTIME_END( __idvBasePtr__, __Idx__ );     \
        sFlag_ ## __Idx__ = ID_FALSE;                      \
    }                                                      \
}

//fix BUG-9260
#define RP_MAX_PARALLEL_FACTOR          (idlVA::getProcessorCount() * 2)
#define RP_INIT_SAVEPOINT_COUNT_PER_TX  (10)
// PROJ-1557 32k memory varchar
#define RP_BUFFER_SIZE                  (SM_PAGE_SIZE * 4)
#define RP_MAX_MSG_LEN                  (1024)
#define RP_ACK_MSG_LEN                  (1024)
#define RP_SAVEPOINT_NAME_LEN           QCI_MAX_OBJECT_NAME_LEN
#define RP_MAX_SID_LEN                  (12)
//PROJ-1538 IPv6
#define RP_IP_ADDR_LEN                  (IDL_IP_ADDR_MAX_LEN)
#define RP_PORT_LEN                     (IDL_IP_PORT_MAX_LEN)

#define RP_REPL_OFF                     (0)
#define RP_REPL_ON                      (1)
#define RP_REPL_WAKEUP_PEER_SENDER      (2)
#define RP_REPL_START_SYNC_APPLY        (3)

//PROJ-1529
#define RP_LAZY_MODE                    (0)
/* BUG-32208 	[rp-sender] Replication acked mode should be deleted.
 *#define RP_ACKED_MODE                 (1)
 */
#define RP_EAGER_MODE                   (2)
#define RP_USELESS_MODE                 (3)
#define RP_DEFAULT_MODE                 (4)
#define RP_NOWAIT_MODE                  (10)

#define RP_CONSISTENT_MODE              (12)

// PROJ-1537
#define RP_SOCKET_UNIX_DOMAIN_STR       (SChar *)"UNIX_DOMAIN"
#define RP_SOCKET_UNIX_DOMAIN_LEN       (11)
#define RP_SOCKET_FILE_LEN              (250 + 1)

#define RP_SOCKET_TCP_STR               (SChar *)"TCP"
#define RP_SOCKET_TCP_LEN               (3)

#define RP_SOCKET_IB_STR                (SChar *)"IB"
#define RP_SOCKET_IB_LEN                (2)

// PROJ-1442 Replication Online 중 DDL 허용
#define RP_INVALID_COLUMN_ID            (ID_UINT_MAX)

/* PROJ-1969 Various Options */
#define RP_OPTION_RECOVERY              (0)
#define RP_OPTION_OFFLINE               (1)
#define RP_OPTION_GAPLESS               (2)
#define RP_OPTION_PARALLEL              (3)
#define RP_OPTION_GROUPING              (4)
#define RP_OPTION_LOCAL                 (5)
#define RP_OPTION_DDL_REPLICATE         (6)
#define RP_OPTION_V6_PROTOCOL           (7)

#define RP_OPTION_ALL_FLAGS_MASK        (0xFFFFFFFF)

//PROJ-1608 recovery from replication
#define RP_OPTION_RECOVERY_MASK         (0x00000001)
#define RP_OPTION_RECOVERY_SET          (0x00000001)
#define RP_OPTION_RECOVERY_UNSET        (0x00000000)

/*PROJ-1915 off-line replicator */
#define RP_OPTION_OFFLINE_MASK          (0x00000002)
#define RP_OPTION_OFFLINE_SET           (0x00000002)
#define RP_OPTION_OFFLINE_UNSET         (0x00000000)

/* PROJ-1969 GAPLESS */
#define RP_OPTION_GAPLESS_MASK          (0x00000004)
#define RP_OPTION_GAPLESS_SET           (0x00000004)
#define RP_OPTION_GAPLESS_UNSET         (0x00000000)

/* PROJ-1969 Parallel Applier */
#define RP_OPTION_PARALLEL_RECEIVER_APPLY_MASK      (0x00000008)
#define RP_OPTION_PARALLEL_RECEIVER_APPLY_SET       (0x00000008)
#define RP_OPTION_PARALLEL_RECEIVER_APPLY_UNSET     (0x00000000)

/* PROJ-1969 Replicated Transaction Group */
#define RP_OPTION_GROUPING_MASK         (0x00000010)
#define RP_OPTION_GROUPING_SET          (0x00000010)
#define RP_OPTION_GROUPING_UNSET        (0x00000000)

/* BUG-45236 Local Replication 지원 */
#define RP_OPTION_LOCAL_MASK            (0x00000020)
#define RP_OPTION_LOCAL_SET             (0x00000020)
#define RP_OPTION_LOCAL_UNSET           (0x00000000)

/* BUG-46252 DDL Synchronization Lazy */
#define RP_OPTION_DDL_REPLICATE_MASK    (0x00000040)
#define RP_OPTION_DDL_REPLICATE_SET     (0x00000040)
#define RP_OPTION_DDL_REPLICATE_UNSET   (0x00000000)

/* BUG-46528 Apply __REPLICATION_USE_V6_PROTOCOL to each replication  */
#define RP_OPTION_V6_PROTOCOL_MASK         (0x00000080)
#define RP_OPTION_V6_PROTOCOL_SET          (0x00000080)
#define RP_OPTION_V6_PROTOCOL_UNSET        (0x00000000)

//for Invalid_recovery
#define RP_CAN_RECOVERY                 (0)
#define RP_CANNOT_RECOVERY              (1)

#define RP_LIST_IDX_NULL                ( RPU_REPLICATION_MAX_COUNT + 1 )

#define RP_PARALLEL_PARENT_ID           (0)
#define RP_DEFAULT_PARALLEL_ID          (RP_PARALLEL_PARENT_ID)

#define RP_DEFAULT_DATE_FORMAT          (SChar *)"YYYY-MM-DD HH:MI:SS.SSSSSS"
#define RP_DEFAULT_DATE_FORMAT_LEN      (26)

#define RP_CONFLICT_RESOLUTION_BEGIN_SVP_NAME (SChar *)"$$CONFLICT_RESOLUTION_BEGIN"
#define RP_CONFLICT_RESOLUTION_SVP_NAME (SChar *)"$$CONFLICT_RESOLUTION"
#define RP_IMPLICIT_SVP_FOR_PROPAGATION_POSTFIX        (SChar *)"FORPROPAGATION"
#define RP_IMPLICIT_SVP_FOR_PROPAGATION_POSTFIX_SIZE   (14)

#define RP_TID_NULL                     (SM_NULL_TID)

#define RP_EAGER_FLUSH_TIMEOUT          (10)
#define RP_FAILBACK_MASTER_SYNC_TIMEOUT (10)
#define RP_MEMPOOL_SIZE                 ( 1024 )

/* PROJ-2336 */
#define RP_TABLE_UNIT                   ( (SChar *)"T" )
#define RP_PARTITION_UNIT               ( (SChar *)"P" )

/* PROJ-1969 PARALLEL APPLIER */
#define RP_PARALLEL_APPLIER_MAX_COUNT ( 512 )

#define RP_KB_SIZE ( (ULong)1024 )
#define RP_MB_SIZE ( (ULong)(RP_KB_SIZE)*(1024) )
#define RP_GB_SIZE ( (ULong)(RP_MB_SIZE)*(1024) )
#define RP_TB_SIZE ( (ULong)(RP_GB_SIZE)*(1024) )

/* Base MB Max 1 TB */
#define RP_APPLIER_BUFFER_MAX_SIZE ( (ULong)RP_TB_SIZE )

#define RP_LABEL(a) a:

#define RP_BLOB_VALUSE_HEADER_SIZE_FOR_CM   ( 1 + 40 )
#define RP_BLOB_VALUSE_PIECES_SIZE_FOR_CM ( CMB_BLOCK_DEFAULT_SIZE - \
                                            CMP_HEADER_SIZE - \
                                            RP_BLOB_VALUSE_HEADER_SIZE_FOR_CM )

#define RP_GET_BYTE_GAP(aDestLSN, aSrcLSN) \
    ( ( ( (ULong)( (aDestLSN.mFileNo) - (aSrcLSN.mFileNo) ) * (ULong)(smuProperty::getLogFileSize()) ) + \
      (ULong)(aDestLSN.mOffset) ) - (ULong)(aSrcLSN.mOffset) )

#define RP_IS_CORRUPTED_PAGE() ( (ideGetErrorCode() == smERR_ABORT_PageCorrupted) || \
                                 (ideGetErrorCode() == smERR_ABORT_INCONSISTENT_PAGE) \
                                 ? ID_TRUE : ID_FALSE )

#define RPC_TEMPORARY_REP_NAME  (SChar *)"TEMPSYNCREPL"

#define RP_INIT_RECEIVER_TEMP_READCONTEXT( aReadContext, aProtocolContext ) \
do \
{\
    (aReadContext)->mCMContext = aProtocolContext; \
    (aReadContext)->mXLogfileContext = NULL;\
    (aReadContext)->mCurrentMode = RPX_RECEIVER_READ_NETWORK; \
}while(0)

typedef enum RP_INTR_LEVEL
{
    RP_INTR_NONE = 0,
    RP_INTR_RETRY,
    RP_INTR_FAULT,
    RP_INTR_EXIT
} RP_INTR_LEVEL;

typedef enum RP_CONFLICT_RESOLUTION
{
    RP_CONFLICT_RESOLUTION_NONE      = 0,
    RP_CONFLICT_RESOLUTION_MASTER    = 1,
    RP_CONFLICT_RESOLUTION_SLAVE     = 2
    /* Timestamp Conflict Resolution은 Table 단위로 적용되므로, 제외한다. */
} RP_CONFLICT_RESOLUTION;

typedef enum RP_ROLE
{
    RP_ROLE_REPLICATION          = 0,
    RP_ROLE_ANALYSIS             = 1,
    RP_ROLE_PROPAGABLE_LOGGING   = 2,
    RP_ROLE_PROPAGATION          = 3,
    RP_ROLE_ANALYSIS_PROPAGATION = 4,
    RP_ROLE_IGNORE
} RP_ROLE;

typedef enum RP_SOCKET_TYPE
{
    RP_SOCKET_TYPE_TCP  = 0,
    RP_SOCKET_TYPE_UNIX = 1,
    RP_SOCKET_TYPE_IB   = 2
} RP_SOCKET_TYPE;

typedef enum rpIBLatency
{
    RP_IB_LATENCY_0  = 0,
    RP_IB_LATENCY_1,
    RP_IB_LATENCY_NONE
} rpIBLatency;


typedef enum rpLinkImpl
{
    RP_LINK_IMPL_TCP       = 0,
    RP_LINK_IMPL_IB        = 1,
    RP_LINK_IMPL_INVALID   = 2,
    RP_LINK_IMPL_BASE      = RP_LINK_IMPL_TCP,
    RP_LINK_IMPL_MAX       = RP_LINK_IMPL_INVALID
} rpLinkImpl;

typedef enum rpDispatcherImpl
{
    RP_DISPATCHER_IMPL_SOCK      = 0,
    RP_DISPATCHER_IMPL_IB        = 1,
    RP_DISPATCHER_IMPL_INVALID   = 2,
    RP_DISPATCHER_IMPL_BASE      = RP_DISPATCHER_IMPL_SOCK,
    RP_DISPATCHER_IMPL_MAX       = RP_DISPATCHER_IMPL_INVALID
} rpDispatcherImpl;

typedef enum RP_SENDER_TYPE
{
    RP_NORMAL = 0,
    RP_QUICK,
    RP_SYNC,
    RP_SYNC_ONLY,
    RP_RECOVERY,  //PROJ-1608 recovery from replication
    RP_OFFLINE,   //PROJ-1915 off-line replicator
    RP_PARALLEL,
    RP_START_CONDITIONAL,
    RP_SYNC_CONDITIONAL,
    RP_XLOGFILE_FAILBACK_MASTER,
    RP_XLOGFILE_FAILBACK_SLAVE
} RP_SENDER_TYPE;

typedef enum RP_MESSENGER_CHECK_VERSION
{
    RP_MESSENGER_NONE        = 0,
    RP_MESSENGER_CONDITIONAL = 1
} RP_MESSENGER_CHECK_VERSION;

//BUG-25960 : V$REPOFFLINE_STATUS
typedef enum RP_OFFLINE_STATUS
{
    RP_OFFLINE_NOT_START = 0,
    RP_OFFLINE_STARTED,
    RP_OFFLINE_END,
    RP_OFFLINE_FAILED
} RP_OFFLINE_STATUS;

// BUG-18714
typedef enum RP_START_OPTION
{
    RP_START_OPTION_NONE = 0,
    RP_START_OPTION_RETRY,
    RP_START_OPTION_SN,
    RP_START_OPTION_CONDITIONAL
} RP_START_OPTION;

/* Eager Replication에서 Sender의 상태는 아래와 같은 다양한 전이 과정에 의해서 변경될 수 있다.
 * O라고 표시한 단계는 건너뛸 수 있다.
 * stop->sync(O)->failback->run->retry(O)->stop
 *                   |--<-------------| 
 */
typedef enum RP_SENDER_STATUS
{
    RP_SENDER_STOP = 0,
    RP_SENDER_SYNC,
    RP_SENDER_FAILBACK_NORMAL,
    RP_SENDER_FAILBACK_MASTER,
    RP_SENDER_FAILBACK_SLAVE,
    RP_SENDER_RUN,
    RP_SENDER_FAILBACK_EAGER, /* 여기부터는 Eager replication이 동작하는 status를 기술*/
    RP_SENDER_FLUSH_FAILBACK,
    RP_SENDER_IDLE,
    RP_SENDER_RETRY,
    RP_SENDER_CONSISTENT_FAILBACK
} RP_SENDER_STATUS;

typedef enum RP_FAILBACK_STATUS
{
    RP_FAILBACK_NONE = 0,
    RP_FAILBACK_NORMAL,
    RP_FAILBACK_MASTER,
    RP_FAILBACK_SLAVE
} RP_FAILBACK_STATUS;

// message return from receiver to sender
typedef enum rpMsgReturn
{
    RP_MSG_OK                   = 0, /* Replication Protocol Version, Meta 정보가 같음 */
    RP_MSG_DISCONNECT           = 1, /* Network 단절 등으로 인해, 결과를 알 수 없음 */
    RP_MSG_DENY                 = 2, /* 해당 Recovery/Service Phase에서 접근 거부 */
    RP_MSG_NOK                  = 3, /* Receiver 할당 실패 (Receiver Threads, Recovery Items) */
    RP_MSG_PROTOCOL_DIFF        = 4, /* Replication Protocol Version이 다름 */
    RP_MSG_META_DIFF            = 5, /* Meta 정보가 다름 */
    RP_MSG_RECOVERY_OK          = 6, /* Recovery 가능 */
    RP_MSG_RECOVERY_NOK         = 7  /* Recovery 필요없거나, 불가능 */
} rpMsgReturn;

// XLog Message Type
typedef enum rpXLogType
{
    RP_X_NONE  = 0,
    RP_X_BEGIN = 1,         // Transaction Begin
    RP_X_COMMIT,            // Transaction Commit
    RP_X_ABORT,             // Transaction rollback
    RP_X_INSERT,            // DML: Insert
    RP_X_UPDATE,            // DML: Update
    RP_X_DELETE,            // DML: Delete
    RP_X_IMPL_SP_SET,       // Implicit Savepoint set
    RP_X_SP_SET,            // Savepoint Set
    RP_X_SP_ABORT,          // Abort to savepoint
    RP_X_STMT_BEGIN = 10,   // Statement Begin
    RP_X_STMT_END,          // Statement End
    RP_X_CURSOR_OPEN,       // Cursor Open
    RP_X_CURSOR_CLOSE,      // Cursor Close
    RP_X_LOB_CURSOR_OPEN,   // LOB Cursor open
    RP_X_LOB_CURSOR_CLOSE,  // LOB Cursor close
    RP_X_LOB_PREPARE4WRITE, // LOB Prepare for write
    RP_X_LOB_PARTIAL_WRITE, // LOB Partial write
    RP_X_LOB_FINISH2WRITE,  // LOB Finish to write
    RP_X_KEEP_ALIVE,        // Keep Alive
    RP_X_ACK = 20,          // ACK
    RP_X_REPL_STOP,         // Replication Stop
    RP_X_FLUSH,             // Replication Flush
    RP_X_FLUSH_ACK,         // Replication Flush ack
    RP_X_STOP_ACK,          // Stop Ack
    RP_X_HANDSHAKE,         // Handshake
    RP_X_HANDSHAKE_ACK,     // Handshake Ack
    RP_X_SYNC_PK_BEGIN,     // Incremental Sync Primary Key Begin
    RP_X_SYNC_PK,           // Incremental Sync Primary Key
    RP_X_SYNC_PK_END,       // Incremental Sync Primary Key End
    RP_X_FAILBACK_END = 30, // Failback End
    RP_X_SYNC_START,        /* Sync Start */
    RP_X_SYNC_END,          /* Sync End */
    RP_X_SYNC_INSERT,       /* Sync Insert */
    RP_X_SYNC_END_ACK, /* Ack End Rebuild index for RP Sync */
    RP_X_LOB_TRIM,          // LOB Trim
    RP_X_ACK_ON_DML,        // Ack on DML
    RP_X_ACK_WITH_TID,         // Ack Eater
    RP_X_DDL_REPLICATE_HANDSHAKE,     // DDL REPLICATE_HANDSHAKE
    RP_X_DDL_REPLICATE_HANDSHAKE_ACK, // DDL REPLICATE_HANDSHAKEACK
    RP_X_DDL_REPLICATE_EXECUTE = 40,  // DDL REPLICATE_EXECUTE
    RP_X_DDL_REPLICATE_EXECUTE_ACK,   // DDL REPLICATE_EXECUTE_ACK
    RP_X_TRUNCATE,                    //  Truncate
    RP_X_TRUNCATE_ACK,                //  Truncate Ack
    RP_X_XA_START_REQ,                // PROJ-2747 Global Tx Consistent
    RP_X_XA_START_REQ_ACK,            
    RP_X_XA_PREPARE_REQ,
    RP_X_XA_PREPARE,
    RP_X_XA_COMMIT,                   // Global Transaction Commit
    RP_X_XA_END,                      // PROJ-2747
    RP_X_MAX = 50
} rpXLogType;

// Incremental Sync PK Type
typedef enum rpSyncPKType
{
    RP_SYNC_PK_BEGIN = 0,
    RP_SYNC_PK,
    RP_SYNC_PK_END
} rpSyncPKType;

/* TASK-6548 unique key duplication */
typedef enum rpApplyFailType
{
    RP_APPLY_FAIL_NONE = 0,
    RP_APPLY_FAIL_INTERNAL,                  /* INTERNAL FAIL */
    RP_APPLY_FAIL_BY_CONFLICT,               /* FAIL BY CONFLICT */
    RP_APPLY_FAIL_BY_CONFLICT_RESOLUTION_TX, /* FAIL BY CONFLICT RESOLUTION TX */
    RP_APPLY_FAIL_BY_CORRUPTED_PAGE,         /* FAIL BY CORRUPTED PAGE */
    RP_APPLY_FAIL_SKIP                       /* FAIL BY SKIP */
} rpApplyFailType;

typedef struct
{
    smTID mTID;
    smSN  mSN;
} rpTxAck;

typedef struct
{
    UInt           mAckType;
    UInt           mAbortTxCount;
    UInt           mClearTxCount;
    smSN           mRestartSN;
    smSN           mLastCommitSN;
    smSN           mLastArrivedSN;
    smSN           mLastProcessedSN;
    smSN           mFlushSN;
    rpTxAck       *mAbortTxList;
    rpTxAck       *mClearTxList;
    smTID          mTID;
} rpXLogAck;

// To Fix PR-10590
// Replication Flush Option
typedef enum rpFlushType
{
    RP_FLUSH_NONE = 0,
    RP_FLUSH_FLUSH,    // FLUSH
    RP_FLUSH_WAIT,     // FLUSH WAIT second
    RP_FLUSH_ALL,      // FLUSH ALL
    RP_FLUSH_ALL_WAIT, // FLUSH ALL WAIT second
    RP_FLUSH_XLOGFILE  // FLUSH FROM XLOGFILE
} rpFlushType;

typedef struct rpFlushOption
{
    rpFlushType flushType;
    SLong       waitSecond;
} rpFlushOption;

//proj-1608 recovery from replication
typedef enum rpReceiverStartMode
{
    RP_RECEIVER_NORMAL = 0,
    RP_RECEIVER_USING_TRANSFER,
    RP_RECEIVER_SYNC,
    RP_RECEIVER_RECOVERY, //recovery sender counterpart
    RP_RECEIVER_OFFLINE,  //PROJ-1915 off-line replicator receiver
    RP_RECEIVER_PARALLEL,  //같은 repl이름을 허용하는 receiver (for child sender)
    RP_RECEIVER_SYNC_CONDITIONAL,
    RP_RECEIVER_XLOGFILE_RECOVERY,
    RP_RECEIVER_XLOGFILE_FAILBACK_MASTER,
    RP_RECEIVER_FAILOVER_USING_XLOGFILE
} rpReceiverStartMode;

// PROJ-1442 Replication Online 중 DDL 허용
typedef enum RP_META_BUILD_TYPE
{
    RP_META_BUILD_LAST = 0,
    RP_META_BUILD_OLD,
    RP_META_BUILD_AUTO,
    RP_META_NO_BUILD
} RP_META_BUILD_TYPE;

/* proj-1608 recovery status transition
 * recovery giveup이 발생하는 경우 1,2
 * 1. recovery_null --> recovery_support_receiver_run --> recovery_null
 * 2. recovery_null --> recovery_support_receiver_run --> recovery_wait --> recovery_null
 * recovery가 정상적으로 진행되는 경우 3
 * 3. recovery_null --> recovery_support_receiver_run --> recovery_wait --> recovery_sender_run --> recovery_null
 */
typedef enum rpRecoveryStatus
{
    RP_RECOVERY_NULL = 0,
    RP_RECOVERY_SUPPORT_RECEIVER_RUN, //RECOVERY support receiver
    RP_RECOVERY_WAIT,
    RP_RECOVERY_SENDER_RUN
} rpRecoveryStatus;

typedef enum rpSavepointType
{
    RP_SAVEPOINT_IMPLICIT = 0,
    RP_SAVEPOINT_EXPLICIT,
    RP_SAVEPOINT_PSM
} rpSavepointType;

//gap이 없을 때 취해야하는 동작
typedef enum RP_ACTION_ON_NOGAP
{
    RP_WAIT_ON_NOGAP  = 0,
    RP_RETURN_ON_NOGAP
} RP_ACTION_ON_NOGAP;

// XLog를 추가할 때의 동작
typedef enum RP_ACTION_ON_ADD_XLOG
{
    RP_SEND_XLOG_ON_ADD_XLOG  = 0,
    RP_COLLECT_BEGIN_SN_ON_ADD_XLOG,
    RP_SEND_SYNC_PK_ON_ADD_XLOG
} RP_ACTION_ON_ADD_XLOG;

/* PROJ-2336 */
typedef enum rpReplicationUnit
{
    RP_REPLICATION_TABLE_UNIT = 0,
    RP_REPLICATION_PARTITION_UNIT
} rpReplicationUnit;

typedef struct rpxSNEntry
{
    smSN            mSN;
    iduListNode     mNode;
} rpxSNEntry;


#endif /* _O_RP_H_ */
