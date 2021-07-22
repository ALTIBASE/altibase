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
 
#define MODE_DB      (0)
#define MODE_MEMORY  (1)
#define MODE_SESSION (2)
#define MODE_REPL    (3)

EXEC SQL BEGIN DECLARE SECTION;
typedef struct SessionInfo_
{
    int         ID;
    long long   CLIENT_ID;
    char        COMM_NAME[129];
    char        XA_SESSION_FLAG[65];
    int         QUERY_TIME_LIMIT;
    int         FETCH_TIME_LIMIT;
    int         UTRANS_TIME_LIMIT;
    int         IDLE_TIME_LIMIT;
    int         IDLE_START_TIME;
    char        ACTIVE_FLAG[65];
    int         OPENED_STMT_COUNT;
    char        CLIENT_CONSTR[257];
    char        CLIENT_IDN[257];
    char        DB_USERNAME[65];
    long long   CLIENT_PID;
    char        SYSDBA_FLAG[65];
    char        AUTOCOMMIT_FLAG[65];
    char        SESSION_STATE[65];
    char        PROTOCOL_SESSION[65];
    int         ISOLATION_LEVEL;
    long long   CURRENT_STMT_ID;
    long long   CONTINUOUS_FAILURE_COUNT;
    char        DEFAULT_DATE_FORMAT[65];
} SessionInfo;

typedef struct StmtInfo_
{
    long long   ID;
    char        CURSOR_TYPE[65];
    long long   TX_ID;
    char        QUERY[1025];
    int         QUERY_START_TIME;
    int         FETCH_START_TIME;
    char        STATE[65];
    char        BEGIN_FLAG[65];
} StmtInfo;

typedef struct ReplSenderInfo_
{
    char        REP_NAME[43];
    char        START_FLAG[65];
    char        NET_ERROR_FLAG[65];
    int         XLSN_FINENO;
    int         XLSN_OFFSET;
    int         COMMIT_LSN_FINENO;
    int         COMMIT_LSN_OFFSET;
    char        STATUS[65];
    char        SENDER_IP[51];
    char        PEER_IP[51];
    int         SENDER_PORT;
    int         PEER_PORT;
} ReplSenderInfo;

typedef struct ReplReceiverInfo_
{
    char        REP_NAME[43];
    char        SENDER_IP[51];
    char        PEER_IP[51];
    int         SENDER_PORT;
    int         PEER_PORT;
} ReplReceiverInfo;

typedef struct DatabaseInfo_
{
    char        DB_NAME[129];
    char        PRODUCT_SIGNATURE[257];
    char        DB_SIGNATURE[257];
    int         VERSION_ID;
    int         COMPILE_BIT;
    long long   ENDIAN;
    long long   LOGFILE_SIZE;
    long long   DBFILE_SIZE;
    int         TX_TBL_SIZE;
    int         DBFILE_COUNT_0;
    int         DBFILE_COUNT_1;
    char        TIMESTAMP[65];
    long long   ALLOC_PAGE_COUNT;
    long long   FREE_PAGE_COUNT;
    long long   FIRST_FREE_PAGE;
    long long   DURABLE_SYSTEM_SCN;
} DatabaseInfo;

typedef struct MemTblInfo_
{
    char        USER_NAME[41];
    char        TABLE_NAME[41];
    long long   MEM_PAGE_CNT;
    int         MEM_VAR_PAGE_CNT;
    int         MEM_SLOT_CNT;
    long long   MEM_SLOT_SIZE;
    long long   MEM_FIRST_PAGEID;
} MemTblInfo;

typedef struct DiskTblInfo_
{
    char        USER_NAME[41];
    char        TABLE_NAME[41];
    long long   DISK_PAGE_CNT;
    short       INSERT_HIGH_LIMIT;
    short       INSERT_LOW_LIMIT;
} DiskTblInfo;

char query[32768];
char query_2[32768];
EXEC SQL END DECLARE SECTION;

void getProperty();
void eraseWhiteSpace(char *buffer);
SQLRETURN parseBuffer(char *buffer, char **Name, char **Value);
void showUsage();
void statusDB();
void statusMemory();
void statusSession();
void statusRepl();
void statusStmt(long long aSid);
void showDatabaseInfo( DatabaseInfo * aDatabaseInfo );
void showMemTblInfo( MemTblInfo * aMemTblInfo );
void showDiskTblInfo( DiskTblInfo * aDiskTblInfo );
void showSessionInfo( SessionInfo * aSessionInfo );
void showStmtInfo( StmtInfo * aStmtInfo );
void showReplSenderInfo(ReplSenderInfo * aRepSenderInfo);
void showReplReceiverInfo(ReplReceiverInfo * aReplReceiverInfo);
