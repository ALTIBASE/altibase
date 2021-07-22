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
 * $Id: sdiPerformanceView.h 83512 2018-07-18 00:47:26Z hykim $
 **********************************************************************/

#ifndef _O_SDI_PERFORMANCE_VIEW_H_
#define _O_SDI_PERFORMANCE_VIEW_H_ 1

/* Shard performance view
 * 마지막 performance view 에는 ',' 를 생략해야 한다.
 */

#define SDI_PERFORMANCE_VIEWS \
    (SChar*)"CREATE VIEW S$CONNECTION_INFO "\
               "(NODE_ID, NODE_NAME, COMM_NAME, TOUCH_COUNT, LINK_FAILURE) "\
            "AS SELECT "\
               "NODE_ID, NODE_NAME, COMM_NAME, TOUCH_COUNT, LINK_FAILURE "\
            "FROM X$SHARD_CONNECTION_INFO"\
,\
    (SChar*)"CREATE VIEW S$PROPERTY( "\
               "NAME, STOREDCOUNT, ATTR, MIN, MAX, "\
               "VALUE1, VALUE2, VALUE3, VALUE4, VALUE5, VALUE6, VALUE7, VALUE8, "\
               "NODE_NAME, D_STOREDCOUNT, D_ATTR, D_MIN, D_MAX, "\
               "D_VALUE1, D_VALUE2, D_VALUE3, D_VALUE4, D_VALUE5, D_VALUE6, D_VALUE7, D_VALUE8 ) "\
            "AS SELECT /*+ USE_HASH(D,M) */ "\
               "DECODE( M.NAME, null, D.NAME, M.NAME ), M.STOREDCOUNT, M.ATTR, M.MIN, M.MAX, "\
               "M.VALUE1, M.VALUE2, M.VALUE3, M.VALUE4, M.VALUE5, M.VALUE6, M.VALUE7, M.VALUE8, "\
               "D.NODE_NAME, D.STOREDCOUNT, D.ATTR, D.MIN, D.MAX, "\
               "D.VALUE1, D.VALUE2, D.VALUE3, D.VALUE4, D.VALUE5, D.VALUE6, D.VALUE7, D.VALUE8 "\
            "FROM V$PROPERTY M FULL OUTER JOIN "\
               "NODE[DATA] (SELECT SHARD_NODE_NAME() NODE_NAME, NAME, STOREDCOUNT, ATTR, MIN, MAX, "\
                                  "VALUE1, VALUE2, VALUE3, VALUE4, VALUE5, VALUE6, VALUE7, VALUE8 "\
                           "FROM V$PROPERTY) D "\
            "ON M.NAME = D.NAME"\
,\
    (SChar*)"CREATE VIEW S$SESSION( "\
            "ID, SESSION_ID, SHARD_CLIENT, "\
               "TRANS_ID, TASK_STATE, COMM_NAME, XA_SESSION_FLAG, XA_ASSOCIATE_FLAG, QUERY_TIME_LIMIT, "\
               "DDL_TIME_LIMIT, FETCH_TIME_LIMIT, UTRANS_TIME_LIMIT, IDLE_TIME_LIMIT, IDLE_START_TIME, "\
               "ACTIVE_FLAG, OPENED_STMT_COUNT, CLIENT_PACKAGE_VERSION, CLIENT_PROTOCOL_VERSION, "\
               "CLIENT_PID, CLIENT_TYPE, CLIENT_APP_INFO, CLIENT_NLS, DB_USERNAME, DB_USERID, "\
               "DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG, AUTOCOMMIT_FLAG, SESSION_STATE, "\
               "ISOLATION_LEVEL, REPLICATION_MODE, TRANSACTION_MODE, COMMIT_WRITE_WAIT_MODE, "\
               "OPTIMIZER_MODE, HEADER_DISPLAY_MODE, CURRENT_STMT_ID, STACK_SIZE, DEFAULT_DATE_FORMAT, "\
               "TRX_UPDATE_MAX_LOGSIZE, PARALLEL_DML_MODE, LOGIN_TIME, FAILOVER_SOURCE, NLS_TERRITORY, "\
               "NLS_ISO_CURRENCY, NLS_CURRENCY, NLS_NUMERIC_CHARACTERS, TIME_ZONE, LOB_CACHE_THRESHOLD, "\
               "QUERY_REWRITE_ENABLE, DBLINK_GLOBAL_TRANSACTION_LEVEL, DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "\
               "MAX_STATEMENTS_PER_SESSION, SSL_CIPHER, SSL_CERTIFICATE_SUBJECT, SSL_CERTIFICATE_ISSUER, "\
               "MODULE, ACTION, REPLICATION_DDL_SYNC, REPLICATION_DDL_SYNC_TIMELIMIT, "\
            "NODE_NAME, D_SESSION_ID, D_SHARD_CLIENT, D_SESSION_TYPE, "\
               "D_TRANS_ID, D_TASK_STATE, D_COMM_NAME, D_XA_SESSION_FLAG, D_XA_ASSOCIATE_FLAG, D_QUERY_TIME_LIMIT, "\
               "D_DDL_TIME_LIMIT, D_FETCH_TIME_LIMIT, D_UTRANS_TIME_LIMIT, D_IDLE_TIME_LIMIT, D_IDLE_START_TIME, "\
               "D_ACTIVE_FLAG, D_OPENED_STMT_COUNT, D_CLIENT_PACKAGE_VERSION, D_CLIENT_PROTOCOL_VERSION, "\
               "D_CLIENT_PID, D_CLIENT_TYPE, D_CLIENT_APP_INFO, D_CLIENT_NLS, D_DB_USERNAME, D_DB_USERID, "\
               "D_DEFAULT_TBSID, D_DEFAULT_TEMP_TBSID, D_SYSDBA_FLAG, D_AUTOCOMMIT_FLAG, D_SESSION_STATE, "\
               "D_ISOLATION_LEVEL, D_REPLICATION_MODE, D_TRANSACTION_MODE, D_COMMIT_WRITE_WAIT_MODE, "\
               "D_OPTIMIZER_MODE, D_HEADER_DISPLAY_MODE, D_CURRENT_STMT_ID, D_STACK_SIZE, D_DEFAULT_DATE_FORMAT, "\
               "D_TRX_UPDATE_MAX_LOGSIZE, D_PARALLEL_DML_MODE, D_LOGIN_TIME, D_FAILOVER_SOURCE, D_NLS_TERRITORY, "\
               "D_NLS_ISO_CURRENCY, D_NLS_CURRENCY, D_NLS_NUMERIC_CHARACTERS, D_TIME_ZONE, D_LOB_CACHE_THRESHOLD, "\
               "D_QUERY_REWRITE_ENABLE, D_DBLINK_GLOBAL_TRANSACTION_LEVEL, D_DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "\
               "D_MAX_STATEMENTS_PER_SESSION, D_SSL_CIPHER, D_SSL_CERTIFICATE_SUBJECT, D_SSL_CERTIFICATE_ISSUER, "\
               "D_MODULE, D_ACTION, D_REPLICATION_DDL_SYNC, D_REPLICATION_DDL_SYNC_TIMELIMIT ) "\
            "AS SELECT "\
            "M.SHARD_PIN ID, M.ID SESSION_ID, DECODE(M.SHARD_CLIENT, 0, 'N', 'Y') SHARD_CLIENT, "\
               "M.TRANS_ID, "\
               "DECODE(M.TASK_STATE, 0, 'WAITING', "\
                                    "1, 'READY', "\
                                    "2, 'EXECUTING', "\
                                    "3, 'QUEUE WAIT', "\
                                    "4, 'QUEUE READY', "\
                                    "'UNKNOWN'), "\
               "M.COMM_NAME, M.XA_SESSION_FLAG, M.XA_ASSOCIATE_FLAG, M.QUERY_TIME_LIMIT, "\
               "M.DDL_TIME_LIMIT, M.FETCH_TIME_LIMIT, M.UTRANS_TIME_LIMIT, M.IDLE_TIME_LIMIT, M.IDLE_START_TIME, "\
               "M.ACTIVE_FLAG, M.OPENED_STMT_COUNT, M.CLIENT_PACKAGE_VERSION, M.CLIENT_PROTOCOL_VERSION, "\
               "M.CLIENT_PID, M.CLIENT_TYPE, M.CLIENT_APP_INFO, M.CLIENT_NLS, M.DB_USERNAME, M.DB_USERID, "\
               "M.DEFAULT_TBSID, M.DEFAULT_TEMP_TBSID, M.SYSDBA_FLAG, M.AUTOCOMMIT_FLAG, "\
               "DECODE(M.SESSION_STATE, 0, 'INIT', "\
                                       "1, 'AUTH', "\
                                       "2, 'SERVICE READY', "\
                                       "3, 'SERVICE', "\
                                       "4, 'END', "\
                                       "5, 'ROLLBACK', "\
                                       "'UNKNOWN'), "\
               "M.ISOLATION_LEVEL, M.REPLICATION_MODE, M.TRANSACTION_MODE, M.COMMIT_WRITE_WAIT_MODE, "\
               "M.OPTIMIZER_MODE, M.HEADER_DISPLAY_MODE, M.CURRENT_STMT_ID, M.STACK_SIZE, M.DEFAULT_DATE_FORMAT, "\
               "M.TRX_UPDATE_MAX_LOGSIZE, M.PARALLEL_DML_MODE, M.LOGIN_TIME, M.FAILOVER_SOURCE, M.NLS_TERRITORY, "\
               "M.NLS_ISO_CURRENCY, M.NLS_CURRENCY, M.NLS_NUMERIC_CHARACTERS, M.TIME_ZONE, M.LOB_CACHE_THRESHOLD, "\
               "DECODE(M.QUERY_REWRITE_ENABLE, 0, 'FALSE', "\
                                              "1, 'TRUE', "\
                                              "'UNKNOWN'), "\
               "M.DBLINK_GLOBAL_TRANSACTION_LEVEL, M.DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "\
               "M.MAX_STATEMENTS_PER_SESSION, M.SSL_CIPHER, M.SSL_CERTIFICATE_SUBJECT, M.SSL_CERTIFICATE_ISSUER, "\
               "M.MODULE, M.ACTION, M.REPLICATION_DDL_SYNC, M.REPLICATION_DDL_SYNC_TIMELIMIT, "\
            "D.NODE_NAME, D.ID, "\
               "DECODE(D.SHARD_CLIENT, 0, 'N', 1, 'Y', null), "\
               "DECODE(D.SHARD_SESSION_TYPE, 0, 'E', 1, 'I', null), "\
               "D.TRANS_ID, "\
               "DECODE(D.TASK_STATE, 0, 'WAITING', "\
                                    "1, 'READY', "\
                                    "2, 'EXECUTING', "\
                                    "3, 'QUEUE WAIT', "\
                                    "4, 'QUEUE READY', "\
                                    "'UNKNOWN'), "\
               "D.COMM_NAME, D.XA_SESSION_FLAG, D.XA_ASSOCIATE_FLAG, D.QUERY_TIME_LIMIT, "\
               "D.DDL_TIME_LIMIT, D.FETCH_TIME_LIMIT, D.UTRANS_TIME_LIMIT, D.IDLE_TIME_LIMIT, D.IDLE_START_TIME, "\
               "D.ACTIVE_FLAG, D.OPENED_STMT_COUNT, D.CLIENT_PACKAGE_VERSION, D.CLIENT_PROTOCOL_VERSION, "\
               "D.CLIENT_PID, D.CLIENT_TYPE, D.CLIENT_APP_INFO, D.CLIENT_NLS, D.DB_USERNAME, D.DB_USERID, "\
               "D.DEFAULT_TBSID, D.DEFAULT_TEMP_TBSID, D.SYSDBA_FLAG, D.AUTOCOMMIT_FLAG, "\
               "DECODE(D.SESSION_STATE, 0, 'INIT', "\
                                       "1, 'AUTH', "\
                                       "2, 'SERVICE READY', "\
                                       "3, 'SERVICE', "\
                                       "4, 'END', "\
                                       "5, 'ROLLBACK', "\
                                       "'UNKNOWN'), "\
               "D.ISOLATION_LEVEL, D.REPLICATION_MODE, D.TRANSACTION_MODE, D.COMMIT_WRITE_WAIT_MODE, "\
               "D.OPTIMIZER_MODE, D.HEADER_DISPLAY_MODE, D.CURRENT_STMT_ID, D.STACK_SIZE, D.DEFAULT_DATE_FORMAT, "\
               "D.TRX_UPDATE_MAX_LOGSIZE, D.PARALLEL_DML_MODE, D.LOGIN_TIME, D.FAILOVER_SOURCE, D.NLS_TERRITORY, "\
               "D.NLS_ISO_CURRENCY, D.NLS_CURRENCY, D.NLS_NUMERIC_CHARACTERS, D.TIME_ZONE, D.LOB_CACHE_THRESHOLD, "\
               "DECODE(D.QUERY_REWRITE_ENABLE, 0, 'FALSE', "\
                                              "1, 'TRUE', "\
                                              "'UNKNOWN'), "\
               "D.DBLINK_GLOBAL_TRANSACTION_LEVEL, D.DBLINK_REMOTE_STATEMENT_AUTOCOMMIT, "\
               "D.MAX_STATEMENTS_PER_SESSION, D.SSL_CIPHER, D.SSL_CERTIFICATE_SUBJECT, D.SSL_CERTIFICATE_ISSUER, "\
               "D.MODULE, D.ACTION, D.REPLICATION_DDL_SYNC, D.REPLICATION_DDL_SYNC_TIMELIMIT "\
            "FROM X$SESSION M LEFT OUTER JOIN "\
               "NODE[DATA] (SELECT SHARD_NODE_NAME() NODE_NAME, * "\
                           "FROM X$SESSION "\
                           "WHERE SHARD_PIN != '0-0-0') D "\
               "ON M.SHARD_PIN = D.SHARD_PIN "\
            "WHERE M.SHARD_PIN != '0-0-0'"\
,\
    (SChar*)"CREATE VIEW S$STATEMENT( "\
               "SHARD_SESSION_ID, NODE_NAME, SESSION_ID, "\
               "SHARD_SESSION_TYPE, STATEMENT_ID, QUERY_TYPE, "\
               "PARENT_ID, CURSOR_TYPE, TX_ID, QUERY, LAST_QUERY_START_TIME, QUERY_START_TIME, "\
               "FETCH_START_TIME, EXECUTE_STATE, FETCH_STATE, ARRAY_FLAG, ROW_NUMBER, EXECUTE_FLAG, "\
               "BEGIN_FLAG, TOTAL_TIME, PARSE_TIME, VALIDATE_TIME, OPTIMIZE_TIME, EXECUTE_TIME, "\
               "FETCH_TIME, SOFT_PREPARE_TIME, SQL_CACHE_TEXT_ID, SQL_CACHE_PCO_ID, OPTIMIZER, COST, "\
               "USED_MEMORY, READ_PAGE, WRITE_PAGE, GET_PAGE, CREATE_PAGE, UNDO_READ_PAGE, "\
               "UNDO_WRITE_PAGE, UNDO_GET_PAGE, UNDO_CREATE_PAGE, MEM_CURSOR_FULL_SCAN, "\
               "MEM_CURSOR_INDEX_SCAN, DISK_CURSOR_FULL_SCAN, DISK_CURSOR_INDEX_SCAN, EXECUTE_SUCCESS, "\
               "EXECUTE_FAILURE, FETCH_SUCCESS, FETCH_FAILURE, PROCESS_ROW, MEMORY_TABLE_ACCESS_COUNT, "\
               "SEQNUM, EVENT, P1, P2, P3, WAIT_TIME, SECOND_IN_TIME, SIMPLE_QUERY ) "\
            "AS ( "\
            "SELECT /*+ USE_HASH( A, B ) */ "\
               "M.SHARD_PIN, 'META', M.SESSION_ID, "\
               "DECODE(M.SHARD_SESSION_TYPE, 0, 'E', 'I'), M.ID, "\
               "DECODE(M.SHARD_QUERY_TYPE, 1, 'S', 2, 'N', '-'), "\
               "M.PARENT_ID, M.CURSOR_TYPE, M.TX_ID, M.QUERY, M.LAST_QUERY_START_TIME, M.QUERY_START_TIME, "\
               "M.FETCH_START_TIME, "\
               "DECODE(M.STATE, 0, 'ALLOC', "\
                               "1, 'PREPARED', "\
                               "2, 'EXECUTED', "\
                               "'UNKNOWN'), "\
               "DECODE(M.FETCH_STATE, 0, 'PROCEED', "\
                                     "1, 'CLOSE', "\
                                     "2, 'NO RESULTSET', "\
                                     "3, 'INVALIDATED', "\
                                     "'UNKNOWN'), "\
               "M.ARRAY_FLAG, M.ROW_NUMBER, M.EXECUTE_FLAG, "\
               "M.BEGIN_FLAG, M.TOTAL_TIME, M.PARSE_TIME, M.VALIDATE_TIME, M.OPTIMIZE_TIME, M.EXECUTE_TIME, "\
               "M.FETCH_TIME, M.SOFT_PREPARE_TIME, M.SQL_CACHE_TEXT_ID, M.SQL_CACHE_PCO_ID, M.OPTIMIZER, M.COST, "\
               "M.USED_MEMORY, M.READ_PAGE, M.WRITE_PAGE, M.GET_PAGE, M.CREATE_PAGE, M.UNDO_READ_PAGE, "\
               "M.UNDO_WRITE_PAGE, M.UNDO_GET_PAGE, M.UNDO_CREATE_PAGE, M.MEM_CURSOR_FULL_SCAN, "\
               "M.MEM_CURSOR_INDEX_SCAN, M.DISK_CURSOR_FULL_SCAN, M.DISK_CURSOR_INDEX_SCAN, M.EXECUTE_SUCCESS, "\
               "M.EXECUTE_FAILURE, M.FETCH_SUCCESS, M.FETCH_FAILURE, M.PROCESS_ROW, M.MEMORY_TABLE_ACCESS_COUNT, "\
               "M.SEQNUM, B.EVENT, M.P1, M.P2, M.P3, M.WAIT_TIME, M.SECOND_IN_TIME, M.SIMPLE_QUERY "\
            "FROM X$STATEMENT M, X$WAIT_EVENT_NAME B "\
            "WHERE M.SEQNUM = B.EVENT_ID AND M.SHARD_PIN != '0-0-0' "\
            "UNION ALL "\
            "SELECT "\
               "D.SHARD_PIN, D.NODE_NAME, D.SESSION_ID, "\
               "D.SHARD_SESSION_TYPE, D.ID, D.SHARD_QUERY_TYPE, "\
               "D.PARENT_ID, D.CURSOR_TYPE, D.TX_ID, D.QUERY, D.LAST_QUERY_START_TIME, D.QUERY_START_TIME, "\
               "D.FETCH_START_TIME, D.EXECUTE_STATE, D.FETCH_STATE, D.ARRAY_FLAG, D.ROW_NUMBER, D.EXECUTE_FLAG, "\
               "D.BEGIN_FLAG, D.TOTAL_TIME, D.PARSE_TIME, D.VALIDATE_TIME, D.OPTIMIZE_TIME, D.EXECUTE_TIME, "\
               "D.FETCH_TIME, D.SOFT_PREPARE_TIME, D.SQL_CACHE_TEXT_ID, D.SQL_CACHE_PCO_ID, D.OPTIMIZER, D.COST, "\
               "D.USED_MEMORY, D.READ_PAGE, D.WRITE_PAGE, D.GET_PAGE, D.CREATE_PAGE, D.UNDO_READ_PAGE, "\
               "D.UNDO_WRITE_PAGE, D.UNDO_GET_PAGE, D.UNDO_CREATE_PAGE, D.MEM_CURSOR_FULL_SCAN, "\
               "D.MEM_CURSOR_INDEX_SCAN, D.DISK_CURSOR_FULL_SCAN, D.DISK_CURSOR_INDEX_SCAN, D.EXECUTE_SUCCESS, "\
               "D.EXECUTE_FAILURE, D.FETCH_SUCCESS, D.FETCH_FAILURE, D.PROCESS_ROW, D.MEMORY_TABLE_ACCESS_COUNT, "\
               "D.SEQNUM, D.EVENT, D.P1, D.P2, D.P3, D.WAIT_TIME, D.SECOND_IN_TIME, D.SIMPLE_QUERY "\
            "FROM NODE[DATA]( "\
               "SELECT /*+ USE_HASH( A, B ) */ "\
               "A.SHARD_PIN SHARD_PIN, SHARD_NODE_NAME() NODE_NAME, A.SESSION_ID SESSION_ID, "\
                   "DECODE(A.SHARD_SESSION_TYPE, 0, 'E', 'I') SHARD_SESSION_TYPE, A.ID ID, "\
                   "DECODE(A.SHARD_SESSION_TYPE, 0, 'S', '-') SHARD_QUERY_TYPE, "\
                   "A.PARENT_ID PARENT_ID, A.CURSOR_TYPE CURSOR_TYPE, A.TX_ID TX_ID, A.QUERY QUERY, "\
                   "A.LAST_QUERY_START_TIME LAST_QUERY_START_TIME, A.QUERY_START_TIME QUERY_START_TIME, "\
                   "A.FETCH_START_TIME FETCH_START_TIME, "\
                   "DECODE(A.STATE, 0, 'ALLOC', "\
                                   "1, 'PREPARED', "\
                                   "2, 'EXECUTED', "\
                                   "'UNKNOWN') EXECUTE_STATE, "\
                   "DECODE(A.FETCH_STATE, 0, 'PROCEED', "\
                                         "1, 'CLOSE', "\
                                         "2, 'NO RESULTSET', "\
                                         "3, 'INVALIDATED', "\
                                         "'UNKNOWN') FETCH_STATE, "\
                   "A.ARRAY_FLAG ARRAY_FLAG, A.ROW_NUMBER ROW_NUMBER, A.EXECUTE_FLAG EXECUTE_FLAG, "\
                   "A.BEGIN_FLAG BEGIN_FLAG, A.TOTAL_TIME TOTAL_TIME, A.PARSE_TIME PARSE_TIME, "\
                   "A.VALIDATE_TIME VALIDATE_TIME, A.OPTIMIZE_TIME OPTIMIZE_TIME, "\
                   "A.EXECUTE_TIME EXECUTE_TIME, A.FETCH_TIME FETCH_TIME, A.SOFT_PREPARE_TIME SOFT_PREPARE_TIME, "\
                   "A.SQL_CACHE_TEXT_ID SQL_CACHE_TEXT_ID, A.SQL_CACHE_PCO_ID SQL_CACHE_PCO_ID, "\
                   "A.OPTIMIZER OPTIMIZER, A.COST COST, A.USED_MEMORY USED_MEMORY, A.READ_PAGE READ_PAGE, "\
                   "A.WRITE_PAGE WRITE_PAGE, A.GET_PAGE GET_PAGE, A.CREATE_PAGE CREATE_PAGE, "\
                   "A.UNDO_READ_PAGE UNDO_READ_PAGE, A.UNDO_WRITE_PAGE UNDO_WRITE_PAGE, "\
                   "A.UNDO_GET_PAGE UNDO_GET_PAGE, A.UNDO_CREATE_PAGE UNDO_CREATE_PAGE, "\
                   "A.MEM_CURSOR_FULL_SCAN MEM_CURSOR_FULL_SCAN, A.MEM_CURSOR_INDEX_SCAN MEM_CURSOR_INDEX_SCAN, "\
                   "A.DISK_CURSOR_FULL_SCAN DISK_CURSOR_FULL_SCAN, A.DISK_CURSOR_INDEX_SCAN DISK_CURSOR_INDEX_SCAN, "\
                   "A.EXECUTE_SUCCESS EXECUTE_SUCCESS, A.EXECUTE_FAILURE EXECUTE_FAILURE, "\
                   "A.FETCH_SUCCESS FETCH_SUCCESS, A.FETCH_FAILURE FETCH_FAILURE, A.PROCESS_ROW PROCESS_ROW, "\
                   "A.MEMORY_TABLE_ACCESS_COUNT MEMORY_TABLE_ACCESS_COUNT, A.SEQNUM SEQNUM, B.EVENT EVENT, "\
                   "A.P1 P1, A.P2 P2, A.P3 P3, A.WAIT_TIME WAIT_TIME, A.SECOND_IN_TIME SECOND_IN_TIME, "\
                   "A.SIMPLE_QUERY SIMPLE_QUERY "\
               "FROM X$STATEMENT A, X$WAIT_EVENT_NAME B "\
               "WHERE A.SEQNUM = B.EVENT_ID AND A.SHARD_PIN != '0-0-0') D "\
            "WHERE D.SHARD_PIN IN (SELECT SHARD_PIN FROM X$STATEMENT WHERE SHARD_PIN != '0-0-0'))"\

#endif /* _O_SDI_PERFORMANCE_VIEW_H_ */
