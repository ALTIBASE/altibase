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

#ifndef _O_ALTIBASE_SQL_CLI_H_
#define _O_ALTIBASE_SQL_CLI_H_ 1

#define HAVE_LONG_LONG

#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -------------------------
 * Altibase specific types
 * -------------------------
 */

#define SQL_BLOB_LOCATOR        31
#define SQL_CLOB_LOCATOR        41
#define SQL_C_BLOB_LOCATOR      SQL_BLOB_LOCATOR
#define SQL_C_CLOB_LOCATOR      SQL_CLOB_LOCATOR

/*
 * Note : There is no type such as SQL_C_BLOB or SQL_C_CLOB.
 *        One shall make use of SQL_C_BINARY and SQL_C_CHAR instead.
 */

#define SQL_BLOB                30      /* SQL_LONGVARBINARY */
#define SQL_CLOB                40      /* SQL_LONGVARCHAR   */
#define SQL_BYTE                20001
#define SQL_VARBYTE             20003
#define SQL_NIBBLE              20002
#define SQL_GEOMETRY            10003
#define SQL_VARBIT              (-100)
#define SQL_NATIVE_TIMESTAMP    3010

#define SQL_BYTES               SQL_BYTE /* Deprecated !!! use SQL_BYTE instead */

/*
 * -----------------------------
 * Altibase specific attributes
 * -----------------------------
 */

#define ALTIBASE_DATE_FORMAT            2003
#define SQL_ATTR_FAILOVER               2004
#define ALTIBASE_NLS_USE                2005
#define ALTIBASE_EXPLAIN_PLAN           2006
#define SQL_ATTR_IDN_LANG               ALTIBASE_NLS_USE
#define SQL_ATTR_PORT		            2007

#define ALTIBASE_APP_INFO               2008
#define ALTIBASE_STACK_SIZE             2009

#define ALTIBASE_OPTIMIZER_MODE         2010

#define ALTIBASE_OPTIMIZER_MODE_UNKNOWN (0)
#define ALTIBASE_OPTIMIZER_MODE_COST    (1)
#define ALTIBASE_OPTIMIZER_MODE_RULE    (2)

#define ALTIBASE_UTRANS_TIMEOUT         2011
#define ALTIBASE_FETCH_TIMEOUT          2012
#define ALTIBASE_IDLE_TIMEOUT           2013
#define ALTIBASE_DDL_TIMEOUT            2044
#define ALTIBASE_HEADER_DISPLAY_MODE    2014

#define SQL_ATTR_LONGDATA_COMPAT        2015

#define ALTIBASE_CONN_ATTR_GPKIWORKDIR  2016
#define ALTIBASE_CONN_ATTR_GPKICONFDIR  2017
#define ALTIBASE_CONN_ATTR_USERCERT     2018
#define ALTIBASE_CONN_ATTR_USERKEY      2019
#define ALTIBASE_CONN_ATTR_USERAID      2020
#define ALTIBASE_CONN_ATTR_USERPASSWD   2021

#define ALTIBASE_XA_RMID                2022

#define ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT   2023
#define ALTIBASE_CONN_ATTR_UNIXDOMAIN_FILEPATH       2024


#define ALTIBASE_XA_NAME                2025
#define ALTIBASE_XA_LOG_DIR             2026


#define ALTIBASE_STMT_ATTR_ATOMIC_ARRAY 2027


#define ALTIBASE_NLS_NCHAR_LITERAL_REPLACE           2028
#define ALTIBASE_NLS_CHARACTERSET                    2029
#define ALTIBASE_NLS_NCHAR_CHARACTERSET              2030
#define ALTIBASE_NLS_CHARACTERSET_VALIDATION         2031


/* ALTIBASE UL FAIL-OVER */

#define ALTIBASE_ALTERNATE_SERVERS                   2033
#define ALTIBASE_LOAD_BALANCE                        2034
#define ALTIBASE_CONNECTION_RETRY_COUNT              2035
#define ALTIBASE_CONNECTION_RETRY_DELAY              2036
#define ALTIBASE_SESSION_FAILOVER                    2037
#define ALTIBASE_FAILOVER_CALLBACK                   2039

#define ALTIBASE_PROTO_VER                           2040
#define ALTIBASE_PREFER_IPV6                         2041

#define ALTIBASE_MAX_STATEMENTS_PER_SESSION          2042
#define ALTIBASE_TRACELOG                            2043

#define ALTIBASE_DDL_TIMEOUT                         2044

#define SQL_CURSOR_HOLD                              2045
#define SQL_ATTR_CURSOR_HOLD                         SQL_CURSOR_HOLD

#define ALTIBASE_TIME_ZONE                           2046

#define SQL_ATTR_DEFERRED_PREPARE                    2047

#define ALTIBASE_LOB_CACHE_THRESHOLD                 2048

#define ALTIBASE_ODBC_COMPATIBILITY                  2049

#define ALTIBASE_FORCE_UNLOCK                        2050

/* SSL/TLS */
#define ALTIBASE_SSL_CA                              2053
#define ALTIBASE_SSL_CAPATH                          2054
#define ALTIBASE_SSL_CERT                            2055
#define ALTIBASE_SSL_KEY                             2056
#define ALTIBASE_SSL_VERIFY                          2057
#define ALTIBASE_SSL_CIPHER                          2058

#define ALTIBASE_SESSION_ID                          2059

#define ALTIBASE_CONN_ATTR_IPC_FILEPATH              2032
#define ALTIBASE_CONN_ATTR_IPCDA_FILEPATH            2060
#define ALTIBASE_CONN_ATTR_IPCDA_CLIENT_SLEEP_TIME   2061
#define ALTIBASE_CONN_ATTR_IPCDA_CLIENT_EXPIRE_COUNT 2062

#define ALTIBASE_SOCK_RCVBUF_BLOCK_RATIO             2063

#define ALTIBASE_SOCK_BIND_ADDR                      2064

/* altibase shard */
#define ALTIBASE_SHARD_VER                           2065
#define ALTIBASE_DEPRECATED_01                       2066   /* ALTIBASE_SHARD_SINGLE_NODE_TRANSACTION   */
#define ALTIBASE_DEPRECATED_02                       2067   /* ALTIBASE_SHARD_MULTIPLE_NODE_TRANSACTION */
#define ALTIBASE_DEPRECATED_03                       2069   /* ALTIBASE_SHARD_GLOBAL_TRANSACTION        */

#define ALTIBASE_PREPARE_WITH_DESCRIBEPARAM          2068

/* InfiniBand(IB) */    
#define ALTIBASE_IB_LATENCY                          2069
#define ALTIBASE_IB_CONCHKSPIN                       2070

#define ALTIBASE_MESSAGE_CALLBACK                    2071

#define ALTIBASE_GLOBAL_TRANSACTION_LEVEL            2072

/* Connection Attributes */
#define ALTIBASE_COMMIT_WRITE_WAIT_MODE              2073
#define ALTIBASE_ST_OBJECT_BUFFER_SIZE               2074
#define ALTIBASE_TRX_UPDATE_MAX_LOGSIZE              2075
#define ALTIBASE_PARALLEL_DML_MODE                   2076
#define ALTIBASE_NLS_NCHAR_CONV_EXCP                 2077
#define ALTIBASE_AUTO_REMOTE_EXEC                    2078
#define ALTIBASE_TRCLOG_DETAIL_PREDICATE             2079
#define ALTIBASE_OPTIMIZER_DISK_INDEX_COST_ADJ       2080
#define ALTIBASE_OPTIMIZER_MEMORY_INDEX_COST_ADJ     2081
#define ALTIBASE_NLS_TERRITORY                       2082
#define ALTIBASE_NLS_ISO_CURRENCY                    2083
#define ALTIBASE_NLS_CURRENCY                        2084
#define ALTIBASE_NLS_NUMERIC_CHARACTERS              2085
#define ALTIBASE_QUERY_REWRITE_ENABLE                2086
#define ALTIBASE_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT  2087
#define ALTIBASE_RECYCLEBIN_ENABLE                   2088
#define ALTIBASE___USE_OLD_SORT                      2089
#define ALTIBASE_ARITHMETIC_OPERATION_MODE           2090
#define ALTIBASE_RESULT_CACHE_ENABLE                 2091
#define ALTIBASE_TOP_RESULT_CACHE_MODE               2092
#define ALTIBASE_OPTIMIZER_AUTO_STATS                2093
#define ALTIBASE___OPTIMIZER_TRANSITIVITY_OLD_RULE   2094
#define ALTIBASE_OPTIMIZER_PERFORMANCE_VIEW          2095
#define ALTIBASE_REPLICATION_DDL_SYNC                2096
#define ALTIBASE_REPLICATION_DDL_SYNC_TIMEOUT        2097
#define ALTIBASE___PRINT_OUT_ENABLE                  2098
#define ALTIBASE_TRCLOG_DETAIL_SHARD                 2099
#define ALTIBASE_SERIAL_EXECUTE_MODE                 2100
#define ALTIBASE_TRCLOG_DETAIL_INFORMATION           2101
#define ALTIBASE___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE   2102
#define ALTIBASE_NORMALFORM_MAXIMUM                  2103
#define ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY   2104
#define ALTIBASE_TRANSACTIONAL_DDL                   2105
#define ALTIBASE_GLOBAL_DDL                          2106
#define ALTIBASE_SHARD_STATEMENT_RETRY               2107
#define ALTIBASE_INDOUBT_FETCH_TIMEOUT               2108
#define ALTIBASE_INDOUBT_FETCH_METHOD                2109
#define ALTIBASE___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD (2110) /* BUG-48132 */
#define ALTIBASE___OPTIMIZER_BUCKET_COUNT_MAX         (2111) /* BUG-48161 */
#define ALTIBASE_DDL_LOCK_TIMEOUT                    2112
#define ALTIBASE___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION (2113) /* BUG-48348 */

#define ALTIBASE_UNUSED_02                           5005  /* ALTIBASE_SHARD_INTERNAL_LOCAL_OPERATION */
#define ALTIBASE_UNUSED_03                           5006  /* ALTIBASE_INVOKE_USER */
#define ALTIBASE_UNUSED_04                           5007  /* ALTIBASE_SHARD_SESSION_TYPE */
#define ALTIBASE_UNUSED_05                           5008  /* ALTIBASE_SHARD_DIST_TX_INFO */
#define ALTIBASE_UNUSED_06                           5009  /* ALTIBASE_SHARD_COORD_FIX_CTRL_CALLBACK */
#define ALTIBASE_UNUSED_07                           5010  /* ALTIBASE_SHARD_META_NUMBER */
#define ALTIBASE_UNUSED_08                           5011  /* ALTIBASE_REBUILD_SHARD_SHARD_META_NUMBER */

/* Below Ver 6 */
#define ALTIBASE_UNUSED_01                           6001  /* ALTIBASE_NUMERIC_DOUBLE_MODE */

/*
 * STMT and DBC attributes that decide how many rows to prefetch
 * or how much memory to reserve as prefetch cache memory
 */
#define ALTIBASE_PREFETCH_ROWS                       12001
#define ALTIBASE_PREFETCH_BLOCKS                     12002
#define ALTIBASE_PREFETCH_MEMORY                     12003

#define SQL_ATTR_PREFETCH_ROWS                       ALTIBASE_PREFETCH_ROWS
#define SQL_ATTR_PREFETCH_BLOCKS                     ALTIBASE_PREFETCH_BLOCKS
#define SQL_ATTR_PREFETCH_MEMORY                     ALTIBASE_PREFETCH_MEMORY

#define ALTIBASE_PREFETCH_ASYNC                      12004
#define ALTIBASE_PREFETCH_AUTO_TUNING                12005
#define ALTIBASE_PREFETCH_AUTO_TUNING_INIT           12006
#define ALTIBASE_PREFETCH_AUTO_TUNING_MIN            12007
#define ALTIBASE_PREFETCH_AUTO_TUNING_MAX            12008

#define SQL_ATTR_PARAMS_ROW_COUNTS_PTR               13001
#define SQL_ATTR_PARAMS_SET_ROW_COUNTS               13002

#define ALTIBASE_PDO_DEFER_PROTOCOLS                 14001

/*
 * File option constants.
 * Used in a call to SQLBindFileToCol() or SQLBindFileToParam().
 */
#define SQL_FILE_CREATE     1
#define SQL_FILE_OVERWRITE  2
#define SQL_FILE_APPEND     3
#define SQL_FILE_READ       4

/*
 * Constants for indicator property of external procedure.
 */
#define ALTIBASE_EXTPROC_IND_NOTNULL    0
#define ALTIBASE_EXTPROC_IND_NULL       1

/*
 * ----------------------------
 * FailOver  Event Types.
 * ----------------------------
 */
#define ALTIBASE_FO_BEGIN               0
#define ALTIBASE_FO_END                 1
#define ALTIBASE_FO_ABORT               2
#define ALTIBASE_FO_GO                  3
#define ALTIBASE_FO_QUIT                4

/*
 * ----------------------------
 * FailOver  Success Error Code.
 * ----------------------------
 */

#define ALTIBASE_FAILOVER_SUCCESS             0x51190
#define EMBEDED_ALTIBASE_FAILOVER_SUCCESS   (-ALTIBASE_FAILOVER_SUCCESS)

/* Data Node  Failover is not available Error Code */
#define ALTIBASE_SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE    0x51216

/* Data Node  Invalid Touch Error Code */
#define ALTIBASE_SHARD_NODE_INVALID_TOUCH    0x51217

/* Shard meta information is changed */
#define ALITBASE_SHARD_META_INFO_CHANGED     0x5121f

#define ALTIBASE_DISTRIBUTED_DEADLOCK_DETECTED 0x111b6

/* Options for SQL_CURSOR_HOLD */
#define SQL_CURSOR_HOLD_ON        1
#define SQL_CURSOR_HOLD_OFF       0
#define SQL_CURSOR_HOLD_DEFAULT   SQL_CURSOR_HOLD_OFF

/* Options for ALTIBASE_EXPLAIN_PLAN */
#define ALTIBASE_EXPLAIN_PLAN_OFF   0
#define ALTIBASE_EXPLAIN_PLAN_ON    1
#define ALTIBASE_EXPLAIN_PLAN_ONLY  2

/* Options for ALTIBASE_PREFETCH_ASYNC */
#define ALTIBASE_PREFETCH_ASYNC_OFF       0
#define ALTIBASE_PREFETCH_ASYNC_PREFERRED 1

/* Options for ALTIBASE_PREFETCH_AUTO_TUNING */
#define ALTIBASE_PREFETCH_AUTO_TUNING_OFF 0
#define ALTIBASE_PREFETCH_AUTO_TUNING_ON  1

/* Options for ALTIBASE_PREPARE_WITH_DESCRIBEPARAM */
#define ALTIBASE_PREPARE_WITH_DESCRIBEPARAM_OFF 0
#define ALTIBASE_PREPARE_WITH_DESCRIBEPARAM_ON  1

/* Options for ALTIBASE_IB_LATENCY */
#define ALTIBASE_IB_LATENCY_NORMAL     0
#define ALTIBASE_IB_LATENCY_PREFERRED  1

#define ALTIBASE_IB_CONCHKSPIN_MIN     0
#define ALTIBASE_IB_CONCHKSPIN_MAX     2147483
#define ALTIBASE_IB_CONCHKSPIN_DEFAULT 0

/* Options for SQL_ATTR_PARAMS_SET_ROW_COUNTS */
#define SQL_ROW_COUNTS_ON  1
#define SQL_ROW_COUNTS_OFF 0
#define SQL_USHRT_MAX      65535

/*
 * ----------------------------
 * Functions for handling LOB
 * ----------------------------
 */

#ifndef SQL_API
#   define SQL_API
#endif

SQLRETURN SQL_API SQLBindFileToCol(SQLHSTMT     aHandle,
                                   SQLSMALLINT  aColumnNumber,
                                   SQLCHAR     *aFileNameArray,
                                   SQLLEN      *aFileNameLengthArray,
                                   SQLUINTEGER *aFileOptionsArray,
                                   SQLLEN       aMaxFileNameLength,
                                   SQLLEN      *aLenOrIndPtr);

SQLRETURN SQL_API SQLBindFileToParam(SQLHSTMT     aHandle,
                                     SQLSMALLINT  aParameterNumber,
                                     SQLSMALLINT  aSqlType,
                                     SQLCHAR     *aFileNameArray,
                                     SQLLEN      *aFileNameLengthArray,
                                     SQLUINTEGER *aFileOptionsArray,
                                     SQLLEN       aMaxFileNameLength,
                                     SQLLEN      *aLenOrIndPtr);

SQLRETURN SQL_API SQLGetLobLength(SQLHSTMT     aHandle,
                                  SQLUBIGINT   aLocator,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUINTEGER *aLobLengthPtr);

SQLRETURN SQL_API SQLPutLob(SQLHSTMT     aHandle,
                            SQLSMALLINT  aLocatorCType,
                            SQLUBIGINT   aLocator,
                            SQLUINTEGER  aStartOffset,
                            SQLUINTEGER  aSizeToBeUpdated,
                            SQLSMALLINT  aSourceCType,
                            SQLPOINTER   aDataToPut,
                            SQLUINTEGER  aSizeDataToPut);

SQLRETURN SQL_API SQLGetLob(SQLHSTMT     aHandle,
                            SQLSMALLINT  aLocatorCType,
                            SQLUBIGINT   aLocator,
                            SQLUINTEGER  aStartOffset,
                            SQLUINTEGER  aSizeToGet,
                            SQLSMALLINT  aTargetCType,
                            SQLPOINTER   aBufferToStoreData,
                            SQLUINTEGER  aSizeBuffer,
                            SQLUINTEGER *aSizeReadPtr);

SQLRETURN SQL_API SQLFreeLob(SQLHSTMT aHandle, SQLUBIGINT aLocator);

SQLRETURN SQL_API SQLTrimLob(SQLHSTMT     aHandle,
                             SQLSMALLINT  aLocatorCType,
                             SQLUBIGINT   aLocator,
                             SQLUINTEGER  aStartOffset);


/*
 * ----------------------------------
 * Altibase Specific CLI Functions
 * ----------------------------------
 */

SQLRETURN SQL_API SQLGetPlan(SQLHSTMT aHandle, SQLCHAR **aPlan);


/*  Altibase UL Fail-Over, ConfigFile Loading Test function */
void  SQL_API   SQLDumpDataSources();
void  SQL_API   SQLDumpDataSourceFromName(SQLCHAR *aDataSourceName);


/* Altibase UL Fail-Over  */
typedef  SQLUINTEGER  (*SQLFailOverCallbackFunc)(SQLHDBC                       aDBC,
                                                 void                         *aAppContext,
                                                 SQLUINTEGER                   aFailOverEvent);
typedef   struct SQLFailOverCallbackContext
{
    SQLHDBC                  mDBC;
    void                    *mAppContext;
    SQLFailOverCallbackFunc  mFailOverCallbackFunc;
}SQLFailOverCallbackContext;


SQLRETURN SQL_API SQLInitializeShardKeyContext( SQLShardKeyContext * ShardKeyContext,
                                                SQLHDBC              ConnectionHandle,
                                                SQLCHAR            * TableName,
                                                SQLINTEGER           TableNameLen,
                                                SQLCHAR            * KeyColumnName,
                                                SQLINTEGER           KeyColumnNameLen,
                                                SQLSMALLINT          KeyColumnValueType,
                                                SQLPOINTER           KeyColumnValueBuffer,
                                                SQLLEN               KeyColumnValueBufferLen,
                                                SQLLEN             * KeyColumnStrLenOrIndPtr,
                                                SQLCHAR              ShardDistType[1] );

SQLRETURN SQL_API SQLGetNodeByShardKeyValue( SQLShardKeyContext     ShardKeyContext,
                                             const SQLCHAR       ** DstNodeName );
                                             //SQLINTEGER         * DstNodeNameLen );

SQLRETURN SQL_API SQLFinalizeShardKeyContext( SQLShardKeyContext ShardKeyContext );

/*
 * ----------------------------------
 * Altibase Connection Pool CLI Functions
 * ----------------------------------
 */
typedef SQLHANDLE SQLHDBCP;

SQLRETURN SQL_API SQLCPoolAllocHandle(SQLHDBCP *aConnectionPoolHandle);

SQLRETURN SQL_API SQLCPoolFreeHandle(SQLHDBCP aConnectionPoolHandle);

enum ALTIBASE_CPOOL_ATTR
{
    ALTIBASE_CPOOL_ATTR_MIN_POOL,
    ALTIBASE_CPOOL_ATTR_MAX_POOL,
    ALTIBASE_CPOOL_ATTR_IDLE_TIMEOUT,
    ALTIBASE_CPOOL_ATTR_CONNECTION_TIMEOUT,
    ALTIBASE_CPOOL_ATTR_DSN,
    ALTIBASE_CPOOL_ATTR_UID,
    ALTIBASE_CPOOL_ATTR_PWD,
    ALTIBASE_CPOOL_ATTR_TRACELOG,
    ALTIBASE_CPOOL_ATTR_VALIDATE_QUERY_TEXT,
    ALTIBASE_CPOOL_ATTR_VALIDATE_QUERY_RESULT,

    /* read-only */
    ALTIBASE_CPOOL_ATTR_TOTAL_CONNECTION_COUNT,
    ALTIBASE_CPOOL_ATTR_CONNECTED_CONNECTION_COUNT,
    ALTIBASE_CPOOL_ATTR_INUSE_CONNECTION_COUNT
};

enum ALTIBASE_TRACELOG_VALUE
{
    ALTIBASE_TRACELOG_NONE  = 0,
    ALTIBASE_TRACELOG_ERROR = 2,
    ALTIBASE_TRACELOG_FULL  = 4
};

enum ALTIBASE_GLOBAL_TRANSACTION_LEVEL_VALUE
{
    ALTIBASE_SINGLE_NODE_TRANSACTION        = 0,
    ALTIBASE_MULTIPLE_NODE_TRANSACTION      = 1,
    ALTIBASE_GLOBAL_TRANSACTION             = 2,
    ALTIBASE_GLOBAL_CONSISTENT_TRANSACTION  = 3
};

SQLRETURN SQL_API SQLCPoolSetAttr(SQLHDBCP aConnectionPoolHandle,
        SQLINTEGER      aAttribute,
        SQLPOINTER      aValue,
        SQLINTEGER      aStringLength);

SQLRETURN SQL_API SQLCPoolGetAttr(SQLHDBCP aConnectionPoolHandle,
        SQLINTEGER      aAttribute,
        SQLPOINTER      aValue,
        SQLINTEGER      aBufferLength,
        SQLINTEGER      *aStringLength);

SQLRETURN SQL_API SQLCPoolInitialize(SQLHDBCP aConnectionPoolHandle);

SQLRETURN SQL_API SQLCPoolFinalize(SQLHDBCP aConnectionPoolHandle);

SQLRETURN SQL_API SQLCPoolGetConnection(SQLHDBCP aConnectionPoolHandle,
        SQLHDBC         *aConnectionHandle);

SQLRETURN SQL_API SQLCPoolReturnConnection(SQLHDBCP aConnectionPoolHandle,
        SQLHDBC         aConnectionHandle);


/* Altibase Message Callback */
typedef void (*SQLMessageCallback)(SQLCHAR     *aMessage,
                                   SQLUINTEGER  aLength,
                                   void        *aUserData);

typedef struct SQLMessageCallbackStruct
{
    SQLMessageCallback  mFunction;
    void               *mUserData;
} SQLMessageCallbackStruct;

#ifdef __cplusplus
}
#endif
#endif

