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
 * $Id: rpi.h 90266 2021-03-19 05:23:09Z returns $
 **********************************************************************/

#ifndef _O_RPI_H_
#define _O_RPI_H_ 1

#include <rp.h>
#include <qci.h>
#include <rpuProperty.h>

// PROJ-1726 performance view definition
// rp/rpi/rpi.cpp 와 qp/qcm/qcmPerformanceView.cpp 두 군데에서
// 사용되므로 한 코드로 관리하기 위하여 #define 으로 정의함.
#define RP_PERFORMANCE_VIEWS \
    (SChar*)"CREATE VIEW V$REPEXEC "\
                "( PORT, MAX_SENDER_COUNT, MAX_RECEIVER_COUNT ) "\
            "AS SELECT "\
                "PORT, MAX_SENDER_COUNT, MAX_RECEIVER_COUNT "\
            "FROM X$REPEXEC",\
\
    (SChar*)"CREATE VIEW V$REPGAP "\
                "( REP_NAME, "\
                "START_FLAG, "\
                "REP_LAST_SN, REP_SN, REP_GAP, REP_GAP_SIZE, "\
                "READ_LFG_ID, READ_FILE_NO, READ_OFFSET ) "\
            "AS SELECT "\
                "REP_NAME, "\
                "CAST(DECODE(CURRENT_TYPE, 0, 0, 1, 1, 2, 2, 3, 3, 4, 6, 5, 7, 6, 8, -1) AS BIGINT) START_FLAG, "\
                "REP_LAST_SN, REP_SN, REP_GAP, REP_GAP_SIZE, "\
                "READ_LFG_ID, READ_FILE_NO, READ_OFFSET "\
            "FROM X$REPGAP "\
            "WHERE PARALLEL_ID = 0",\
\
    (SChar*)"CREATE VIEW V$REPRECEIVER "\
                "( REP_NAME, MY_IP, MY_PORT, PEER_IP, PEER_PORT, "\
                "APPLY_XSN, INSERT_SUCCESS_COUNT, INSERT_FAILURE_COUNT, "\
                "UPDATE_SUCCESS_COUNT, UPDATE_FAILURE_COUNT, "\
                "DELETE_SUCCESS_COUNT, DELETE_FAILURE_COUNT, "\
                "PARALLEL_ID, SQL_APPLY_TABLE_COUNT, "\
                "APPLIER_INIT_BUFFER_USAGE ) "\
            "AS SELECT "\
                "REP_NAME, MY_IP, MY_PORT, PEER_IP, PEER_PORT, "\
                "APPLY_XSN, INSERT_SUCCESS_COUNT, INSERT_FAILURE_COUNT, "\
                "UPDATE_SUCCESS_COUNT, UPDATE_FAILURE_COUNT, "\
                "DELETE_SUCCESS_COUNT, DELETE_FAILURE_COUNT, "\
                "PARALLEL_ID,  SQL_APPLY_TABLE_COUNT, "\
                "APPLIER_INIT_BUFFER_USAGE "\
            "FROM X$REPRECEIVER "\
            "WHERE PARALLEL_ID = 0",\
\
    (SChar*)"CREATE VIEW V$REPRECEIVER_PARALLEL_APPLY "\
                "( REP_NAME, PARALLEL_APPLIER_INDEX, APPLY_XSN, "\
                "INSERT_SUCCESS_COUNT, INSERT_FAILURE_COUNT, "\
                "UPDATE_SUCCESS_COUNT, UPDATE_FAILURE_COUNT, "\
                "DELETE_SUCCESS_COUNT, DELETE_FAILURE_COUNT, "\
                "STATUS ) "    \
            "AS SELECT "\
                "REP_NAME, PARALLEL_APPLIER_INDEX, APPLY_XSN, "\
                "INSERT_SUCCESS_COUNT, INSERT_FAILURE_COUNT, "\
                "UPDATE_SUCCESS_COUNT, UPDATE_FAILURE_COUNT, "\
                "DELETE_SUCCESS_COUNT, DELETE_FAILURE_COUNT, "\
                "DECODE( STATUS, 0, 'INITIALIZE', 1, 'WORKING', 2, 'DEQUEUEING', 3, 'WAITING', 4, 'STOP', 'N/A' ) "\
            "FROM X$REPRECEIVER_PARALLEL_APPLY ",\
\
    (SChar*)"CREATE VIEW V$REPSENDER "\
                "( REP_NAME, "\
                "START_FLAG, "\
                "NET_ERROR_FLAG, XSN, COMMIT_XSN, "\
                "STATUS, "\
                "SENDER_IP, PEER_IP, SENDER_PORT, PEER_PORT, "\
                "READ_LOG_COUNT, SEND_LOG_COUNT, "\
                "REPL_MODE, "\
                "ACT_REPL_MODE ) "\
            "AS SELECT "\
                "REP_NAME, "\
                "CAST(DECODE(CURRENT_TYPE, 0, 0, 1, 1, 2, 2, 3, 3, 4, 6, 5, 7, 6, 8, -1) AS BIGINT) START_FLAG, "\
                "NET_ERROR_FLAG, XSN, COMMIT_XSN, "\
                "CAST(DECODE(STATUS, 0,0, 1,6, 2,3, 3,4, 4,5, 5,1, 6,7, 7,8, 8,9, 9,2, 10,3 -1) AS BIGINT) STATUS, "\
                "SENDER_IP, PEER_IP, SENDER_PORT, PEER_PORT, "\
                "READ_LOG_COUNT, SEND_LOG_COUNT, "\
                "DECODE(REPL_MODE, 0, 'LAZY', 2, 'EAGER', 3, 'USELESS', 10, 'NOWAIT', 'UNKNOWN' ) REPL_MODE, "\
                "DECODE(ACT_REPL_MODE, 0, 'LAZY', 2, 'EAGER', 3, 'USELESS', 10, 'NOWAIT', 'UNKNOWN' ) ACT_REPL_MODE "\
            "FROM X$REPSENDER "\
            "WHERE PARALLEL_ID = 0",\
\
    (SChar*)"CREATE VIEW V$REPSYNC "\
                "( REP_NAME, SYNC_TABLE, SYNC_PARTITION, SYNC_RECORD_COUNT, SYNC_SN ) "\
            "AS SELECT "\
                "REP_NAME, SYNC_TABLE, SYNC_PARTITION, SYNC_RECORD_COUNT, SYNC_SN "\
            "FROM X$REPSYNC ",\
\
    /* PROJ-1915 V$REPSENDER_TRANSTBL에 start_flag 추가 */\
    (SChar*)"CREATE VIEW V$REPSENDER_TRANSTBL "\
                "( REP_NAME, "\
                "START_FLAG, "\
                "LOCAL_TID, REMOTE_TID, BEGIN_FLAG, BEGIN_SN ) "\
            "AS SELECT "\
                "REP_NAME, "\
                "CAST(DECODE(CURRENT_TYPE, 0, 0, 1, 1, 2, 2, 3, 3, 4, 6, 5, 7, 6, 8, -1) AS BIGINT) START_FLAG, "\
                "CAST(DECODE(LOCAL_TRANS_ID, 4294967295, -1, LOCAL_TRANS_ID) AS BIGINT) LOCAL_TID, CAST(DECODE(REMOTE_TRANS_ID, 4294967295, -1, REMOTE_TRANS_ID) AS BIGINT) REMOTE_TID, BEGIN_FLAG, BEGIN_SN "\
            "FROM X$REPSENDER_TRANSTBL "\
            "WHERE PARALLEL_ID = 0",\
\
    (SChar*)"CREATE VIEW V$REPRECEIVER_TRANSTBL " \
                "( REP_NAME, LOCAL_TID, REMOTE_TID, BEGIN_FLAG, BEGIN_SN, PARALLEL_ID, PARALLEL_APPLIER_INDEX ) " \
            "AS SELECT " \
                "REP_NAME, CAST(DECODE(LOCAL_TRANS_ID, 4294967295, -1, LOCAL_TRANS_ID) AS BIGINT) " \
                "LOCAL_TID, CAST(DECODE(REMOTE_TRANS_ID, 4294967295, -1, REMOTE_TRANS_ID) AS BIGINT) " \
                "REMOTE_TID, BEGIN_FLAG, BEGIN_SN, PARALLEL_ID, PARALLEL_APPLIER_INDEX " \
            "FROM X$REPRECEIVER_TRANSTBL "\
            "WHERE PARALLEL_ID = 0",\
\
    /*proj-1608*/\
    (SChar*)"CREATE VIEW V$REPRECOVERY "\
                "(REP_NAME, STATUS, START_XSN, XSN, END_XSN, "\
                "RECOVERY_SENDER_IP, PEER_IP, RECOVERY_SENDER_PORT, PEER_PORT) "\
            "AS SELECT "\
                "REP_NAME, STATUS, START_XSN, XSN, END_XSN, "\
                "RECOVERY_SENDER_IP, PEER_IP, RECOVERY_SENDER_PORT, PEER_PORT "\
            "FROM X$REPRECOVERY ",\
\
    (SChar*)"CREATE VIEW V$REPRECEIVER_COLUMN " \
                "( REP_NAME, USER_NAME, TABLE_NAME, PARTITION_NAME, COLUMN_NAME, APPLY_MODE ) " \
            "AS SELECT " \
                "REP_NAME, USER_NAME, TABLE_NAME, PARTITION_NAME, COLUMN_NAME, APPLY_MODE " \
            "FROM X$REPRECEIVER_COLUMN", \
\
    (SChar*)"CREATE VIEW V$REPLOGBUFFER "\
                "(REP_NAME, BUFFER_MIN_SN, READ_SN, BUFFER_MAX_SN) "\
            "AS SELECT "\
                "REP_NAME, BUFFER_MIN_SN, READ_SN, BUFFER_MAX_SN "\
            "FROM X$REPLOGBUFFER ",\
\
    (SChar*)"CREATE VIEW V$REPOFFLINE_STATUS "\
                "(REP_NAME, STATUS, SUCCESS_TIME) "\
            "AS SELECT " \
                "REP_NAME, STATUS, SUCCESS_TIME " \
            "FROM X$REPOFFLINE_STATUS",\
\
    (SChar*)"CREATE VIEW V$REPGAP_PARALLEL "\
                "( REP_NAME, "\
                "CURRENT_TYPE, "\
                "REP_LAST_SN, REP_SN, REP_GAP, REP_GAP_SIZE, "\
                "READ_LFG_ID, READ_FILE_NO, READ_OFFSET, PARALLEL_ID ) "\
            "AS SELECT "\
                "REP_NAME, "\
                "DECODE(CURRENT_TYPE, 0, 'NORMAL', 1, 'QUICK', 2, 'SYNC', 3, 'SYNC_ONLY', 4, 'RECOVERY', 5, 'OFFLINE', 6, 'PARALLEL', 'UNKNOWN') CURRENT_TYPE, "\
                "REP_LAST_SN, REP_SN, REP_GAP, REP_GAP_SIZE, "\
                "READ_LFG_ID, READ_FILE_NO, READ_OFFSET, PARALLEL_ID "\
            "FROM X$REPGAP",\
\
    (SChar*)"CREATE VIEW V$REPRECEIVER_PARALLEL "\
                "( REP_NAME, MY_IP, MY_PORT, PEER_IP, PEER_PORT, "\
                "APPLY_XSN, INSERT_SUCCESS_COUNT, INSERT_FAILURE_COUNT, "\
                "UPDATE_SUCCESS_COUNT, UPDATE_FAILURE_COUNT, "\
                "DELETE_SUCCESS_COUNT, DELETE_FAILURE_COUNT, "\
                "PARALLEL_ID ) "\
            "AS SELECT "\
                "REP_NAME, MY_IP, MY_PORT, PEER_IP, PEER_PORT, "\
                "APPLY_XSN, INSERT_SUCCESS_COUNT, INSERT_FAILURE_COUNT, "\
                "UPDATE_SUCCESS_COUNT, UPDATE_FAILURE_COUNT, "\
                "DELETE_SUCCESS_COUNT, DELETE_FAILURE_COUNT, "\
                "PARALLEL_ID "\
            "FROM X$REPRECEIVER",\
\
    (SChar*)"CREATE VIEW V$REPSENDER_PARALLEL "\
                "( REP_NAME, "\
                "CURRENT_TYPE, "\
                "NET_ERROR_FLAG, XSN, COMMIT_XSN, "\
                "STATUS, SENDER_IP, PEER_IP, SENDER_PORT, PEER_PORT, "\
                "READ_LOG_COUNT, SEND_LOG_COUNT, "\
                "REPL_MODE, "\
                "PARALLEL_ID) "\
            "AS SELECT "\
                "REP_NAME, "\
                "DECODE(CURRENT_TYPE, 0, 'NORMAL', 1, 'QUICK', 2, 'SYNC', 3, 'SYNC_ONLY', 4, 'RECOVERY', 5, 'OFFLINE', 6, 'PARALLEL', 'UNKNOWN') CURRENT_TYPE, "\
                "NET_ERROR_FLAG, XSN, COMMIT_XSN, "\
                "DECODE(STATUS, 0, 'STOP', 1, 'SYNC', 2, 'FAILBACK_NORMAL', 3, 'FAILBACK_MASTER', "\
                "               4, 'FAILBACK_SLAVE',  5, 'RUN',             6, 'FAILBACK_EAGER', " \
                "               7, 'FAILBACK_FLUSH',  8, 'IDLE',            9, 'RETRY', 'UNKNOWN' ) STATUS, "\
                "SENDER_IP, PEER_IP, SENDER_PORT, PEER_PORT, "\
                "READ_LOG_COUNT, SEND_LOG_COUNT, "\
                "DECODE(REPL_MODE, 0, 'LAZY', 2, 'EAGER', 3, 'USELESS', 'UNKNOWN' ) REPL_MODE, "\
                "PARALLEL_ID "\
            "FROM X$REPSENDER ",\
\
    (SChar*)"CREATE VIEW V$REPSENDER_TRANSTBL_PARALLEL "\
                "( REP_NAME, "\
                "CURRENT_TYPE, "\
                "LOCAL_TID, REMOTE_TID, BEGIN_FLAG, BEGIN_SN, PARALLEL_ID ) "\
            "AS SELECT "\
                "REP_NAME, "\
                "DECODE(CURRENT_TYPE, 0, 'NORMAL', 1, 'QUICK', 2, 'SYNC', 3, 'SYNC_ONLY', 4, 'RECOVERY', 5, 'OFFLINE', 6, 'PARALLEL', 'UNKNOWN') CURRENT_TYPE, "\
                "CAST(DECODE(LOCAL_TRANS_ID, 4294967295, -1, LOCAL_TRANS_ID) AS BIGINT) LOCAL_TID, CAST(DECODE(REMOTE_TRANS_ID, 4294967295, -1, REMOTE_TRANS_ID) AS BIGINT) REMOTE_TID, BEGIN_FLAG, BEGIN_SN, PARALLEL_ID "\
            "FROM X$REPSENDER_TRANSTBL",\
\
    (SChar*)"CREATE VIEW V$REPRECEIVER_TRANSTBL_PARALLEL " \
                "( REP_NAME, LOCAL_TID, REMOTE_TID, BEGIN_FLAG, BEGIN_SN, PARALLEL_ID ) " \
            "AS SELECT " \
                "REP_NAME, CAST(DECODE(LOCAL_TRANS_ID, 4294967295, -1, LOCAL_TRANS_ID) AS BIGINT) LOCAL_TID, CAST(DECODE(REMOTE_TRANS_ID, 4294967295, -1, REMOTE_TRANS_ID) AS BIGINT) REMOTE_TID, BEGIN_FLAG, BEGIN_SN, PARALLEL_ID " \
            "FROM X$REPRECEIVER_TRANSTBL",                  \
\
    /* BUG-31545 repsender_statistics performance view */\
    (SChar*)"CREATE VIEW V$REPSENDER_STATISTICS "\
                "( REP_NAME, PARALLEL_ID, "\
                "WAIT_NEW_LOG, READ_LOG_FROM_REPLBUFFER, READ_LOG_FROM_FILE, "\
                "CHECK_USEFUL_LOG, ANALYZE_LOG, SEND_XLOG, RECV_ACK, SET_ACKEDVALUE ) "\
            "AS SELECT "\
                "REP_NAME, PARALLEL_ID, "\
                "WAIT_NEW_LOG, READ_LOG_FROM_REPLBUFFER, READ_LOG_FROM_FILE, "\
                "CHECK_USEFUL_LOG, ANALYZE_LOG, SEND_XLOG, RECV_ACK, SET_ACKEDVALUE "\
            "FROM X$REPSENDER_STATISTICS",\
\
    /* BUG-31545 repreceiver_statistics performance view */\
    (SChar*)"CREATE VIEW V$REPRECEIVER_STATISTICS "\
                "( REP_NAME, PARALLEL_ID,"\
                "RECV_XLOG, CONVERT_ENDIAN, BEGIN_TRANSACTION, COMMIT_TRANSACTION, ABORT_TRANSACTION, "\
                "OPEN_TABLE_CURSOR, CLOSE_TABLE_CURSOR, INSERT_ROW, UPDATE_ROW, DELETE_ROW, "\
                "OPEN_LOB_CURSOR, PREPARE_LOB_WRITING, WRITE_LOB_PIECE, FINISH_LOB_WRITE, CLOSE_LOB_CURSOR, COMPARE_IMAGE, SEND_ACK ) "\
            "AS SELECT "\
                "REP_NAME, PARALLEL_ID, "\
                "RECV_XLOG, CONVERT_ENDIAN, BEGIN_TRANSACTION, COMMIT_TRANSACTION, ABORT_TRANSACTION, "\
                "OPEN_TABLE_CURSOR, CLOSE_TABLE_CURSOR, INSERT_ROW, UPDATE_ROW, DELETE_ROW, "\
                "OPEN_LOB_CURSOR, PREPARE_LOB_WRITING, WRITE_LOB_PIECE, FINISH_LOB_WRITE, CLOSE_LOB_CURSOR, COMPARE_IMAGE, SEND_ACK "\
            "FROM X$REPRECEIVER_STATISTICS",\
    /* BUG-38448 */\
    (SChar*)"CREATE VIEW V$REPSENDER_SENT_LOG_COUNT "\
                "( REP_NAME, CURRENT_TYPE, TABLE_OID,"\
                "INSERT_LOG_COUNT, DELETE_LOG_COUNT, UPDATE_LOG_COUNT, LOB_LOG_COUNT ) "\
            "AS SELECT "\
                " REP_NAME, " \
                "DECODE(CURRENT_TYPE, 0, 'NORMAL', 1, 'QUICK', 2, 'SYNC', 3, 'SYNC_ONLY', 4, 'RECOVERY', 5, 'OFFLINE', 6, 'PARALLEL', 'UNKNOWN') CURRENT_TYPE, "\
                "TABLE_OID, INSERT_LOG_COUNT, DELETE_LOG_COUNT, UPDATE_LOG_COUNT, LOB_LOG_COUNT "\
            "FROM X$REPSENDER_SENT_LOG_COUNT "\
            "WHERE PARALLEL_ID = 0",\
\
    (SChar*)"CREATE VIEW V$REPSENDER_SENT_LOG_COUNT_PARALLEL "\
                "( REP_NAME, CURRENT_TYPE, PARALLEL_ID, TABLE_OID,"\
                "INSERT_LOG_COUNT, DELETE_LOG_COUNT, UPDATE_LOG_COUNT, LOB_LOG_COUNT ) "\
            "AS SELECT "\
                " REP_NAME, " \
                "DECODE(CURRENT_TYPE, 0, 'NORMAL', 1, 'QUICK', 2, 'SYNC', 3, 'SYNC_ONLY', 4, 'RECOVERY', 5, 'OFFLINE', 6, 'PARALLEL', 'UNKNOWN') CURRENT_TYPE, PARALLEL_ID, "\
                "TABLE_OID, INSERT_LOG_COUNT, DELETE_LOG_COUNT, UPDATE_LOG_COUNT, LOB_LOG_COUNT "\
            "FROM X$REPSENDER_SENT_LOG_COUNT"\

// 주의 : 마지막 performance view 에는 ',' 를 생략할 것!

class rpi
{
public:
    static IDE_RC   initRPProperty();
    static IDE_RC   initREPLICATION           ();
    static IDE_RC   finalREPLICATION          ();

    static IDE_RC   createReplication         ( void        * aQcStatement );
    static IDE_RC   alterReplicationFlush     ( smiStatement  * aSmiStmt,
                                                SChar         * aReplName,
                                                rpFlushOption * aFlushOption,
                                                idvSQL        * aStatistics );
    static IDE_RC   alterReplicationAddTable  ( void        * aQcStatement );
    static IDE_RC   alterReplicationDropTable ( void        * aQcStatement );
    static IDE_RC   alterReplicationAddHost   ( void        * aQcStatement );
    static IDE_RC   alterReplicationDropHost  ( void        * aQcStatement );
    static IDE_RC   alterReplicationSetHost   ( void        * aQcStatement );
    static IDE_RC   dropReplication           ( void        * aQcStatement );

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // aStatistics  통계 정보 파라메터를 추가 합니다.
    //startSenderThread(), stopSenderThread(), resetReplication(), stopReceiverThreads()
    static IDE_RC   startSenderThread( idvSQL               * aStatistics,
                                       iduVarMemList        * aMemory,
                                       smiStatement         * aSmiStmt,
                                       SChar                * aReplName,
                                       RP_SENDER_TYPE         astartType,
                                       idBool                 aTryHandshakeOnce,
                                       smSN                   aStartSN,
                                       qciSyncItems         * aSyncItemList,
                                       SInt                   aParallelFactor,
                                       void                 * aLockTable );

    static IDE_RC   startTempSync( void * aQcStatement );

    static IDE_RC   stopSenderThread( smiStatement * aSmiStmt,
                                      SChar        * aReplName,
                                      idvSQL       * aStatistics,
                                      idBool         aIsImmediate );
    static IDE_RC   resetReplication(smiStatement * aSmiStmt,
                                     SChar        * aReplName,
                                     idvSQL       * aStatistics);

    static IDE_RC   alterReplicationSetRecovery( void        * aQcStatement );

    /* PROJ-1915 */
    static IDE_RC alterReplicationSetOfflineEnable(void        * aQcStatement);
    static IDE_RC alterReplicationSetOfflineDisable(void        * aQcStatement);

    /* PROJ-1969 */
    static IDE_RC alterReplicationSetGapless( void * aQcStatement );
    static IDE_RC alterReplicationSetParallel( void * aQcStatement );
    static IDE_RC alterReplicationSetGrouping( void * aQcStatement );

    static IDE_RC alterReplicationSetDDLReplicate( void * aQcStatement );

    // PROJ-1442 Replication Online 중 DDL 허용
    static IDE_RC   stopReceiverThreads(smiStatement * aSmiStmt,
                                        smOID          aTableOID,
                                        idvSQL       * aStatistics);
    
    static IDE_RC   isRunningEagerSenderByTableOID(smiStatement * aSmiStmt,
                                                   smOID          aTableOID,
                                                   idvSQL       * aStatistics,
                                                   idBool       * aIsExist);
    static IDE_RC   isRunningEagerReceiverByTableOID(smiStatement * aSmiStmt,
                                                     smOID          aTableOID,
                                                     idvSQL       * aStatistics,
                                                     idBool       * aIsExist);

    static IDE_RC   writeTableMetaLog(void        * aQcStatement,
                                      smOID         aOldTableOID,
                                      smOID         aNewTableOID);

    static IDE_RC   initSystemTables();

    static void applyStatisticsForSystem();
    
    static qciValidateReplicationCallback getReplicationValidateCallback( );

    static qciExecuteReplicationCallback getReplicationExecuteCallback( );

    static qciCatalogReplicationCallback getReplicationCatalogCallback( );

    static qciManageReplicationCallback getReplicationManageCallback( );

    static IDE_RC ddlSyncBegin( qciStatement  * aQciStatement, 
                                smiStatement  * aSmiStatement );

    static IDE_RC ddlSyncEnd( qciStatement * aQciStatement, 
                              smiStatement * aSmiStatement );

    static void   ddlSyncException( qciStatement * aQciStatement,
                                    smiStatement * aSmiStatement ); 

    static void setReplicationDDLSync( UInt aValue )
    {
        rpuProperty::setReplicationDDLSync( aValue );
    }

    static void setReplicationDDLSyncTimeout( UInt aValue )
    {    
        rpuProperty::setReplicationDDLSyncTimeout( aValue );
    }

};

#endif /* _O_RPI_H_ */
