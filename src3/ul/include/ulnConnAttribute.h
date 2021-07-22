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

#ifndef _O_ULN_CONN_ATTRIBUTE_H_
#define _O_ULN_CONN_ATTRIBUTE_H_ 1

/* Stack Size */
#define ULN_STACK_SIZE_MIN          (32)
#define ULN_STACK_SIZE_MAX          (65536)
#define ULN_STACK_SIZE_DEFAULT      (128)

/* Packet size */
#define ULN_PACKET_SIZE_MIN         (1024)
#define ULN_PACKET_SIZE_MAX         (1024 * 128)
#define ULN_PACKET_SIZE_DEFAULT     (1024 * 8)

/* common timeouts limits */
#define ULN_TMOUT_MIN             0
#define ULN_TMOUT_MAX             ACP_UINT32_MAX
#define ULN_DDL_LOCK_TMOUT_UNDEF (ACP_UINT32_MAX -1)
#define ULN_QUERY_TMOUT_DEFAULT   60
/* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
#define ULN_DDL_TMOUT_DEFAULT     0
#define ULN_LOGIN_TMOUT_DEFAULT   60
#define ULN_CONN_TMOUT_DEFAULT    0  // forever wait

/* PROJ-2047 Strengthening LOB - LOBCACHE */
#define ULN_LOB_CACHE_THRESHOLD_MIN        (0)
#define ULN_LOB_CACHE_THRESHOLD_MAX        (1024 * 512)  /* BUG-46411 */
#define ULN_LOB_CACHE_THRESHOLD_DEFAULT    (1024 * 8)

/* PROJ-1645 UL Failover   */
#define ULN_CONN_RETRY_COUNT_DEFAULT        1
#define ULN_CONN_RETRY_COUNT_MIN            0
#define ULN_CONN_RETRY_COUNT_MAX            1024
#define ULN_CONN_RETRY_DELAY_DEFAULT        1
#define ULN_CONN_RETRY_DELAY_MIN            0
#define ULN_CONN_RETRY_DELAY_MAX            3600

/* PROJ-1891 Deferred Prepare */
#define ULN_CONN_DEFERRED_PREPARE_OFF              0UL
#define ULN_CONN_DEFERRED_PREPARE_ON               1UL
#define ULN_CONN_DEFERRED_PREPARE_DEFAULT          ULN_CONN_DEFERRED_PREPARE_OFF

/* PROJ-2474 SSL/TLS */
#define ULN_CONN_SSL_VERIFY_OFF                    0UL
#define ULN_CONN_SSL_VERIFY_ON                     1UL
#define ULN_CONN_SSL_VERIFY_DEFAULT                ULN_CONN_SSL_VERIFY_OFF

#define ULN_CONN_ATTR_INVALID_VALUE (-1)

/* BUG-39817 */
#define ULN_ISOLATION_LEVEL_DEFAULT (0)

typedef enum ulnConnAttrType
{
    ULN_CONN_ATTR_TYPE_INT,
    ULN_CONN_ATTR_TYPE_DISCRETE_INT,
    ULN_CONN_ATTR_TYPE_STRING,
    ULN_CONN_ATTR_TYPE_UPPERCASE_STRING,
    ULN_CONN_ATTR_TYPE_CALLBACK,
    ULN_CONN_ATTR_TYPE_POINTER,
    ULN_CONN_ATTR_TYPE_MAX

} ulnConnAttrType;

typedef enum ulnConnAttrID
{
    ULN_CONN_ATTR_DSN,
    ULN_CONN_ATTR_UID,
    ULN_CONN_ATTR_PWD,
    ULN_CONN_ATTR_URL,
    ULN_CONN_ATTR_NLS_NCHAR_LITERAL_REPLACE,
    ULN_CONN_ATTR_NLS_CHARACTERSET_VALIDATION,
    ULN_CONN_ATTR_NLS_USE,
    ULN_CONN_ATTR_DATE_FORMAT,
    ULN_CONN_ATTR_QUERY_TIMEOUT,
    ULN_CONN_ATTR_AUTOCOMMIT,
    ULN_CONN_ATTR_LOGIN_TIMEOUT,
    ULN_CONN_ATTR_CONNECTION_TIMEOUT,
    ULN_CONN_ATTR_PACKET_SIZE,
    ULN_CONN_ATTR_TXN_ISOLATION,
    ULN_CONN_ATTR_ODBC_VERSION,
    ULN_CONN_ATTR_DISCONNECT_BEHAVIOR,
    ULN_CONN_ATTR_CONNECTION_POOLING,

    ULN_CONN_ATTR_CONNTYPE,
    ULN_CONN_ATTR_PORT_NO,

    ULN_CONN_ATTR_ASYNC_ENABLE,
    ULN_CONN_ATTR_METADATA_ID,

    ULN_CONN_ATTR_EXPLAIN_PLAN,
    ULN_CONN_ATTR_STACK_SIZE,
    ULN_CONN_ATTR_ACCESS_MODE,
    ULN_CONN_ATTR_OPTIMIZER_MODE,

    ULN_CONN_ATTR_UTRANS_TIMEOUT,
    ULN_CONN_ATTR_FETCH_TIMEOUT,
    ULN_CONN_ATTR_IDLE_TIMEOUT,
    ULN_CONN_ATTR_HEADER_DISPLAY_MODE,

    ULN_CONN_ATTR_APP_INFO,
    ULN_CONN_ATTR_LONGDATA_COMPAT,

    ULN_CONN_ATTR_HOSTNAME,
    ULN_CONN_ATTR_MESSAGE_CALLBACK,
    ULN_CONN_ATTR_CURRENT_CATALOG,
    ULN_CONN_ATTR_ODBC_CURSORS,
    ULN_CONN_ATTR_ENLIST_IN_DTC,
    ULN_CONN_ATTR_ENLIST_IN_XA,
    ULN_CONN_ATTR_TRACEFILE,
    ULN_CONN_ATTR_TRANSLATE_LIB,
    ULN_CONN_ATTR_TRANSLATE_OPTION,
    ULN_CONN_ATTR_TRACE,
    ULN_CONN_ATTR_QUIET_MODE,

    ULN_CONN_ATTR_GPKICONFDIR,
    ULN_CONN_ATTR_GPKIWORKDIR,
    ULN_CONN_ATTR_USERCERT,
    ULN_CONN_ATTR_USERKEY,
    ULN_CONN_ATTR_USERAID,
    ULN_CONN_ATTR_USERPASSWD,
    /*PROJ-1573 XA*/
    ULN_CONN_ATTR_XA_RMID,
    ULN_CONN_ATTR_XA_NAME,
    ULN_CONN_ATTR_XA_LOG_DIR,

    ULN_CONN_ATTR_UNIXDOMAIN_FILEPATH,
    ULN_CONN_ATTR_IPC_FILEPATH,

    /* PROJ-1579 NCHAR */
    ULN_CONN_ATTR_NLS_CHARACTERSET,
    ULN_CONN_ATTR_NLS_NCHAR_CHARACTERSET,

    ULN_CONN_ATTR_ANSI_APP,
    // bug-19279 remote sysdba enable
    ULN_CONN_ATTR_PRIVILEGE,

    /* PROJ-1645 UL Failover */
    ULN_CONN_ATTR_ALTERNATE_SERVERS,
    ULN_CONN_ATTR_LOAD_BALANCE,
    ULN_CONN_ATTR_CONNECTION_RETRY_COUNT,
    ULN_CONN_ATTR_CONNECTION_RETRY_DELAY,
    ULN_CONN_ATTR_SESSION_FAILOVER,
    ULN_CONN_ATTR_FAILOVER_CALLBACK,

    ULN_CONN_ATTR_PREFER_IPV6, /* proj-1538 ipv6 */

    ULN_CONN_ATTR_MAX_STATEMENTS_PER_SESSION, /* BUG-31144 */
    ULN_CONN_ATTR_TRACELOG,
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    ULN_CONN_ATTR_DDL_TIMEOUT,
    ULN_CONN_ATTR_TIME_ZONE, /* PROJ-2209 DBTIMEZONE */

    /* PROJ-1891 Deferred prepare */
    ULN_CONN_ATTR_DEFERRED_PREPARE,

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    ULN_CONN_ATTR_LOB_CACHE_THRESHOLD,
    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    ULN_CONN_ATTR_ODBC_COMPATIBILITY,
    /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
    ULN_CONN_ATTR_FORCE_UNLOCK,

    /* PROJ-2474 SSL/TLS */
    ULN_CONN_ATTR_SSL_CA, 
    ULN_CONN_ATTR_SSL_CAPATH,
    ULN_CONN_ATTR_SSL_CERT, 
    ULN_CONN_ATTR_SSL_KEY, 
    ULN_CONN_ATTR_SSL_VERIFY, 
    ULN_CONN_ATTR_SSL_CIPHER, 

    /* PROJ-2616 */
    ULN_CONN_ATTR_IPCDA_FILEPATH,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    ULN_CONN_ATTR_SOCK_RCVBUF_BLOCK_RATIO,

    /* BUG-44271 */
    ULN_CONN_ATTR_SOCK_BIND_ADDR,

    /* PROJ-2638 shard native linker */
    ULN_CONN_ATTR_SHARD_LINKER_TYPE,
    ULN_CONN_ATTR_SHARD_NODE_NAME,
    ULN_CONN_ATTR_SHARD_PIN,

    /* BUG-46090 Meta Node SMN ÀüÆÄ */
    ULN_CONN_ATTR_SHARD_META_NUMBER,

    ULN_CONN_ATTR_PDO_DEFER_PROTOCOLS,  /* BUG-45286 */

    /* PROJ-2681 */
    ULN_CONN_ATTR_IB_LATENCY,
    ULN_CONN_ATTR_IB_CONCHKSPIN,

    ULN_CONN_ATTR_SHARD_CONNTYPE,

    /* BUG-45707 */
    ULN_CONN_ATTR_SHARD_CLIENT,
    ULN_CONN_ATTR_SHARD_SESSION_TYPE,

    /* BUG-46092
     * ULN_CONN_ATTR_SHARD_CLIENT_CONNECTION_REPORT
     * Only send property by shardcli.
     * User can not set attibute value. */
    /* BUG-46785 Deprecated */
    ULN_CONN_ATTR_SHARD_CLIENT_CONNECTION_REPORT,

    /* BUG-47257 */
    ULN_CONN_ATTR_GLOBAL_TRANSACTION_LEVEL,
    // PROJ-2727 add connect attr
    ULN_CONN_ATTR_COMMIT_WRITE_WAIT_MODE,
    ULN_CONN_ATTR_ST_OBJECT_BUFFER_SIZE,
    ULN_CONN_ATTR_TRX_UPDATE_MAX_LOGSIZE,
    ULN_CONN_ATTR_PARALLEL_DML_MODE,
    ULN_CONN_ATTR_NLS_NCHAR_CONV_EXCP,
    ULN_CONN_ATTR_AUTO_REMOTE_EXEC,
    ULN_CONN_ATTR_TRCLOG_DETAIL_PREDICATE,
    ULN_CONN_ATTR_OPTIMIZER_DISK_INDEX_COST_ADJ,
    ULN_CONN_ATTR_OPTIMIZER_MEMORY_INDEX_COST_ADJ,
    ULN_CONN_ATTR_NLS_TERRITORY,
    ULN_CONN_ATTR_NLS_ISO_CURRENCY,
    ULN_CONN_ATTR_NLS_CURRENCY,
    ULN_CONN_ATTR_NLS_NUMERIC_CHARACTERS,
    ULN_CONN_ATTR_QUERY_REWRITE_ENABLE,
    ULN_CONN_ATTR_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT,
    ULN_CONN_ATTR_RECYCLEBIN_ENABLE,
    ULN_CONN_ATTR___USE_OLD_SORT,
    ULN_CONN_ATTR_ARITHMETIC_OPERATION_MODE,
    ULN_CONN_ATTR_RESULT_CACHE_ENABLE,
    ULN_CONN_ATTR_TOP_RESULT_CACHE_MODE,
    ULN_CONN_ATTR_OPTIMIZER_AUTO_STATS,
    ULN_CONN_ATTR___OPTIMIZER_TRANSITIVITY_OLD_RULE,
    ULN_CONN_ATTR_OPTIMIZER_PERFORMANCE_VIEW,
    ULN_CONN_ATTR_REPLICATION_DDL_SYNC,
    ULN_CONN_ATTR_REPLICATION_DDL_SYNC_TIMEOUT,
    ULN_CONN_ATTR___PRINT_OUT_ENABLE,
    ULN_CONN_ATTR_TRCLOG_DETAIL_SHARD,
    ULN_CONN_ATTR_SERIAL_EXECUTE_MODE,
    ULN_CONN_ATTR_TRCLOG_DETAIL_INFORMATION,
    ULN_CONN_ATTR___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE,
    ULN_CONN_ATTR_NORMALFORM_MAXIMUM,    
    ULN_CONN_ATTR___REDUCE_PARTITION_PREPARE_MEMORY,    
    ULN_CONN_ATTR_TRANSACTIONAL_DDL,
    /* shard internal management flag for ha*/
    ULN_CONN_ATTR_SHARD_INTERNAL_LOCAL_OPERATION,
    ULN_CONN_ATTR_INVOKE_USER,
    ULN_CONN_ATTR_GLOBAL_DDL,
    ULN_CONN_ATTR_SHARD_STATEMENT_RETRY,
    ULN_CONN_ATTR_INDOUBT_FETCH_TIMEOUT,
    ULN_CONN_ATTR_INDOUBT_FETCH_METHOD,
    ULN_CONN_ATTR_SHARD_COORD_FIX_CTRL_CALLBACK,
    ULN_CONN_ATTR___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD,  /* BUG-48132 */
    ULN_CONN_ATTR___OPTIMIZER_BUCKET_COUNT_MAX,          /* BUG-48161 */
    ULN_CONN_ATTR_DDL_LOCK_TIMEOUT,
    ULN_CONN_ATTR___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION, /* BUG-48348 */
    ULN_CONN_ATTR_REBUILD_SHARD_META_NUMBER,
    ULN_CONN_ATTR_MAX
} ulnConnAttrID;

/*
 * ULN_CONN_ATTR_TYPE_INT
 */
typedef struct ulnMetricPrefixInt
{
    const acp_char_t *mKey;
    acp_sint32_t      mValue;
} ulnMetricPrefixInt;

/*
 * ULN_CONN_ATTR_TYPE_DISCRETE_INT
 */
typedef struct ulnDiscreteInt
{
    const acp_char_t *mKey;
    acp_sint32_t      mValue;
} ulnDiscreteInt;

/*
 * ULN_CONN_ATTR_TYPE_STRING
 */
typedef struct ulnReservedString
{
    const acp_char_t *mKey;
    const acp_char_t *mValue;
} ulnReservedString;

typedef struct ulnKeyLinkImpl
{
    const acp_char_t *mKey;
    cmiLinkImpl       mValue;
} ulnKeyLinkImpl;

typedef struct ulnAttrNameIdPair
{
    const acp_char_t *mKey;
    ulnConnAttrID     mConnAttrID;
} ulnAttrNameIdPair;

extern const ulnKeyLinkImpl      gULN_CM_PROTOCOL[];

extern const ulnMetricPrefixInt  gULN_MUL[];
extern const ulnMetricPrefixInt  gULN_MUL_TIME[];

extern const ulnDiscreteInt      gULN_BOOL[];
extern const ulnDiscreteInt      gULN_OPTIMIZER_MODE[];
extern const ulnDiscreteInt      gULN_CONNTYPE[];
extern const ulnDiscreteInt      gULN_SHARD_CONNTYPE[];
extern const ulnDiscreteInt      gULN_POOL[];
extern const ulnDiscreteInt      gULN_SQL_TXN[];
extern const ulnDiscreteInt      gSQL_OV_ODBC[];
extern const ulnDiscreteInt      gULN_ACCESS_MODE[];

extern const ulnReservedString   gSQL_DATE[];
extern const ulnReservedString   gNULL_LIST[];

typedef struct ulnConnAttribute
{
    ulnConnAttrID     mConnAttrID;
    const acp_char_t *mKey;              /* POSTFIX NAME of SQL_OPT_xxx or SQL_ATTR_xxx */

    acp_sint32_t      mSQL_ATTR_ID;
    ulnConnAttrType   mAttrType;

    acp_sint64_t      mMinValue;
    acp_sint64_t      mDefValue;
    acp_sint64_t      mMaxValue;

    const void       *mCheck;

    /* BUG-47272 */
    acp_bool_t        mIsSetShardUserConn;
    acp_bool_t        mIsSetShardLibConn;

} ulnConnAttribute;

extern const ulnConnAttribute  gUlnConnAttrTable[];
extern const ulnAttrNameIdPair gUlnConnAttrMap_PROFILE[];
extern const ulnAttrNameIdPair gUlnConnAttrMap_KEYWORD[];

ulnConnAttrID ulnGetConnAttrIDfromKEYWORD(const acp_char_t *aName, acp_sint32_t aNameLen);
ulnConnAttrID ulnGetConnAttrIDfromSQL_ATTR_ID(acp_sint32_t aSQL_ATTR_ID);
acp_char_t   *ulnGetConnAttrKEYWORDfromConnAttrId(ulnConnAttrID aConnAttrId);

/*
 * BUG-36256 Improve property's communication
 */
typedef struct ulnConnAttrArr
{
    ulnConnAttrID *mArrId;
    acp_sint32_t   mArrIndex;
    acp_sint32_t   mArrSize;
} ulnConnAttrArr;

void   ulnConnAttrArrInit  (ulnConnAttrArr *aUnsupportedConnAttr);
void   ulnConnAttrArrReInit(ulnConnAttrArr *aUnsupportedConnAttr);
void   ulnConnAttrArrFinal (ulnConnAttrArr *aUnsupportedConnAttr);
ACI_RC ulnConnAttrArrAddId (ulnConnAttrArr *aUnsupportedConnAttr,
                            ulnConnAttrID   aId);
ACI_RC ulnConnAttrArrHasId (ulnConnAttrArr *aUnsupportedConnAttr,
                            ulnConnAttrID   aId);

/* ULN_CONN_ATTR_SHARD_STATEMENT_RETRY */
typedef enum ulnShardStatmentRetryConst
{
    ULN_SHARD_STATEMENT_RETRY_DEFAULT = 1,      /* IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_DEFAULT */
    ULN_SHARD_STATEMENT_RETRY_MIN     = 0,      /* IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_MIN     */
    ULN_SHARD_STATEMENT_RETRY_MAX     = 65535,  /* IDP_SHARD_PROPERTY_SHARD_STATEMENT_RETRY_MAX     */
} ulnShardStatmentRetryConst;

#endif /* _O_ULN_CONN_ATTRIBUTE_H_ */

