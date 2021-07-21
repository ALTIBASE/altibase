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
 * $Id$ sdlSqlConnAttrType.h 78722 2017-01-23 16:30:000 swhors $
 **********************************************************************/

#ifndef _O_SDL_SQL_CONN_ATTR_TYPE_H_
#define _O_SDL_SQL_CONN_ATTR_TYPE_H_

/* connection attributes */
#define SDL_SQL_ATTR_QUERY_TIMEOUT                          0
#define SDL_SQL_ATTR_MAX_ROWS                               1
#define SDL_SQL_ATTR_NOSCAN                                 2
#define SDL_SQL_ATTR_MAX_LENGTH                             3
#define SDL_SQL_ATTR_ASYNC_ENABLE                           4
#define SDL_SQL_ATTR_ROW_BIND_TYPE                          5
#define SDL_SQL_ATTR_CURSOR_TYPE                            6
#define SDL_SQL_ATTR_CONCURRENCY                            7
#define SDL_SQL_ATTR_KEYSET_SIZE                            8
#define SDL_SQL_ATTR_SIMULATE_CURSOR                        10
#define SDL_SQL_ATTR_RETRIEVE_DATA                          11
#define SDL_SQL_ATTR_USE_BOOKMARKS                          12
#define SDL_SQL_ATTR_ROW_NUMBER                             14
#define SDL_SQL_ATTR_ENABLE_AUTO_IPD                        15
#define SDL_SQL_ATTR_FETCH_BOOKMARK_PTR                     16
#define SDL_SQL_ATTR_PARAM_BIND_OFFSET_PTR                  17
#define SDL_SQL_ATTR_PARAM_BIND_TYPE                        18
#define SDL_SQL_ATTR_PARAM_OPERATION_PTR                    19
#define SDL_SQL_ATTR_PARAM_STATUS_PTR                       20
#define SDL_SQL_ATTR_PARAMS_PROCESSED_PTR                   21
#define SDL_SQL_ATTR_PARAMSET_SIZE                          22
#define SDL_SQL_ATTR_ROW_BIND_OFFSET_PTR                    23
#define SDL_SQL_ATTR_ROW_OPERATION_PTR                      24
#define SDL_SQL_ATTR_ROW_STATUS_PTR                         25
#define SDL_SQL_ATTR_ROWS_FETCHED_PTR                       26
#define SDL_SQL_ATTR_ROW_ARRAY_SIZE                         27
#define SDL_SQL_ATTR_ACCESS_MODE                            101
#define SDL_SQL_ATTR_AUTOCOMMIT                             102
#define SDL_SQL_ATTR_LOGIN_TIMEOUT                          103
#define SDL_SQL_ATTR_TRACE                                  104
#define SDL_SQL_ATTR_TRACEFILE                              105
#define SDL_SQL_ATTR_TRANSLATE_LIB                          106
#define SDL_SQL_ATTR_TRANSLATE_OPTION                       107
#define SDL_SQL_ATTR_TXN_ISOLATION                          108
#define SDL_SQL_ATTR_CURRENT_CATALOG                        109
#define SDL_SQL_ATTR_ODBC_CURSORS                           110
#define SDL_SQL_ATTR_QUIET_MODE                             111
#define SDL_SQL_ATTR_PACKET_SIZE                            112
#define SDL_SQL_ATTR_CONNECTION_TIMEOUT                     113
#define SDL_SQL_ATTR_DISCONNECT_BEHAVIOR                    114
#define SDL_SQL_ATTR_ODBC_VERSION                           200
#define SDL_SQL_ATTR_CONNECTION_POOLING                     201
#define SDL_SQL_ATTR_ENLIST_IN_DTC                          1207
#define SDL_SQL_ATTR_ENLIST_IN_XA                           1208
#define SDL_SQL_ATTR_CONNECTION_DEAD                        1209
#define SDL_ALTIBASE_DATE_FORMAT                            2003
#define SDL_SQL_ATTR_FAILOVER                               2004
#define SDL_ALTIBASE_NLS_USE                                2005
#define SDL_ALTIBASE_EXPLAIN_PLAN                           2006
#define SDL_SQL_ATTR_PORT                                   2007
#define SDL_ALTIBASE_APP_INFO                               2008
#define SDL_ALTIBASE_STACK_SIZE                             2009
#define SDL_ALTIBASE_OPTIMIZER_MODE                         2010
#define SDL_ALTIBASE_UTRANS_TIMEOUT                         2011
#define SDL_ALTIBASE_FETCH_TIMEOUT                          2012
#define SDL_ALTIBASE_IDLE_TIMEOUT                           2013
#define SDL_ALTIBASE_HEADER_DISPLAY_MODE                    2014
#define SDL_SQL_ATTR_LONGDATA_COMPAT                        2015
#define SDL_SQL_ATTR_XA_RMID                                2022
#define SDL_ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT      2023
#define SDL_ALTIBASE_CONN_ATTR_UNIXDOMAIN_FILEPATH          2024
#define SDL_ALTIBASE_XA_NAME                                2025
#define SDL_ALTIBASE_XA_LOG_DIR                             2026
#define SDL_ALTIBASE_NLS_NCHAR_LITERAL_REPLACE              2028
#define SDL_ALTIBASE_NLS_CHARACTERSET                       2029
#define SDL_ALTIBASE_NLS_NCHAR_CHARACTERSET                 2030
#define SDL_ALTIBASE_NLS_CHARACTERSET_VALIDATION            2031
#define SDL_ALTIBASE_ALTERNATE_SERVERS                      2033
#define SDL_ALTIBASE_LOAD_BALANCE                           2034
#define SDL_ALTIBASE_CONNECTION_RETRY_COUNT                 2035
#define SDL_ALTIBASE_CONNECTION_RETRY_DELAY                 2036
#define SDL_ALTIBASE_SESSION_FAILOVER                       2037
#define SDL_ALTIBASE_HEALTH_CHECK_DURATION                  2038
#define SDL_ALTIBASE_PREFER_IPV6                            2041
#define SDL_ALTIBASE_MAX_STATEMENTS_PER_SESSION             2042
#define SDL_ALTIBASE_TRACELOG                               2043
#define SDL_ALTIBASE_DDL_TIMEOUT                            2044
#define SDL_ALTIBASE_TIME_ZONE                              2046
#define SDL_ALTIBASE_LOB_CACHE_THRESHOLD                    2048
#define SDL_ALTIBASE_ODBC_COMPATIBILITY                     2049
#define SDL_ALTIBASE_FORCE_UNLOCK                           2050
#define SDL_ALTIBASE_ENLIST_AS_TXCOOKIE                     2052
#define SDL_ALTIBASE_SESSION_ID                             2059
#define SDL_ALTIBASE_MESSAGE_CALLBACK                       2071  /* BUG-46019 */
#define SDL_ALTIBASE_GLOBAL_TRANSACTION_LEVEL               2072

// PROJ-2727 add connect attr same sqlcli.h
#define SDL_ALTIBASE_COMMIT_WRITE_WAIT_MODE                 2073
#define SDL_ALTIBASE_ST_OBJECT_BUFFER_SIZE                  2074
#define SDL_ALTIBASE_TRX_UPDATE_MAX_LOGSIZE                 2075
#define SDL_ALTIBASE_PARALLEL_DML_MODE                      2076
#define SDL_ALTIBASE_NLS_NCHAR_CONV_EXCP                    2077
#define SDL_ALTIBASE_AUTO_REMOTE_EXEC                       2078
#define SDL_ALTIBASE_TRCLOG_DETAIL_PREDICATE                2079
#define SDL_ALTIBASE_OPTIMIZER_DISK_INDEX_COST_ADJ          2080
#define SDL_ALTIBASE_OPTIMIZER_MEMORY_INDEX_COST_ADJ        2081
#define SDL_ALTIBASE_NLS_TERRITORY                          2082
#define SDL_ALTIBASE_NLS_ISO_CURRENCY                       2083
#define SDL_ALTIBASE_NLS_CURRENCY                           2084
#define SDL_ALTIBASE_NLS_NUMERIC_CHARACTERS                 2085
#define SDL_ALTIBASE_QUERY_REWRITE_ENABLE                   2086
#define SDL_ALTIBASE_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT     2087
#define SDL_ALTIBASE_RECYCLEBIN_ENABLE                      2088
#define SDL_ALTIBASE___USE_OLD_SORT                         2089
#define SDL_ALTIBASE_ARITHMETIC_OPERATION_MODE              2090
#define SDL_ALTIBASE_RESULT_CACHE_ENABLE                    2091
#define SDL_ALTIBASE_TOP_RESULT_CACHE_MODE                  2092
#define SDL_ALTIBASE_OPTIMIZER_AUTO_STATS                   2093
#define SDL_ALTIBASE___OPTIMIZER_TRANSITIVITY_OLD_RULE      2094
#define SDL_ALTIBASE_OPTIMIZER_PERFORMANCE_VIEW             2095
#define SDL_ALTIBASE_REPLICATION_DDL_SYNC                   2096
#define SDL_ALTIBASE_REPLICATION_DDL_SYNC_TIMEOUT           2097
#define SDL_ALTIBASE___PRINT_OUT_ENABLE                     2098
#define SDL_ALTIBASE_TRCLOG_DETAIL_SHARD                    2099
#define SDL_ALTIBASE_SERIAL_EXECUTE_MODE                    2100
#define SDL_ALTIBASE_TRCLOG_DETAIL_INFORMATION              2101
#define SDL_ALTIBASE___OPTIMIZER_DEFAULT_TEMP_TBS_TYPE      2102    
#define SDL_ALTIBASE_NORMALFORM_MAXIMUM                     2103
#define SDL_ALTIBASE___REDUCE_PARTITION_PREPARE_MEMORY      2104

/* PROJ-2735 DDL Transaction */
#define SDL_ALTIBASE_TRANSACTIONAL_DDL                      2105
#define SDL_ALTIBASE_GLOBAL_DDL                             2106

#define SDL_ALTIBASE_SHARD_STATEMENT_RETRY                  2107
#define SDL_ALTIBASE_INDOUBT_FETCH_TIMEOUT                  2108
#define SDL_ALTIBASE_INDOUBT_FETCH_METHOD                   2109

#define SDL_ALTIBASE___OPTIMIZER_PLAN_HASH_OR_SORT_METHOD   (2110) /* BUG-48132 */
#define SDL_ALTIBASE___OPTIMIZER_BUCKET_COUNT_MAX           (2111) /* BUG-48161 */

#define SDL_ALTIBASE_DDL_LOCK_TIMEOUT                       2112

#define SDL_ALTIBASE___OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION (2113) /* BUG-48348 */

#define SDL_ALTIBASE_SHARD_PROTO_VER                        5000
#define SDL_ALTIBASE_SHARD_LINKER_TYPE                      5001
#define SDL_ALTIBASE_SHARD_NODE_NAME                        5002
#define SDL_ALTIBASE_SHARD_PIN                              5003
#define SDL_ALTIBASE_SHARD_INTERNAL_LOCAL_OPERATION         5005  /* ALTIBASE_UNUSED_02 */
#define SDL_ALTIBASE_INVOKE_USER                            5006  /* ALTIBASE_UNUSED_03 */
#define SDL_ALTIBASE_SHARD_SESSION_TYPE                     5007  /* ALTIBASE_UNUSED_04 */
#define SDL_ALTIBASE_SHARD_DIST_TX_INFO                     5008  /* ALTIBASE_UNUSED_05 */
#define SDL_ALTIBASE_SHARD_COORD_FIX_CTRL_CALLBACK          5009  /* ALTIBASE_UNUSED_06 */
#define SDL_ALTIBASE_SHARD_META_NUMBER                      5010  /* ALTIBASE_UNUSED_07 */
#define SDL_ALTIBASE_REBUILD_SHARD_META_NUMBER              5011  /* ALTIBASE_UNUSED_08 */

#define SDL_SQL_ATTR_METADATA_ID                            10014
#define SDL_SQL_ATTR_AUTO_IPD                               10001

#endif /* _O_SDL_SQL_CONN_ATTR_TYPE_H_ */
