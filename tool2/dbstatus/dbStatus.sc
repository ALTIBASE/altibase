EXEC SQL OPTION (INCLUDE=.);
EXEC SQL INCLUDE dbStatus.h;

int gPortNo = -1;
char gNLS[1024];

int main( int argc, char ** argv )
{
    EXEC SQL BEGIN DECLARE SECTION;
    char usr[20];
    char pwd[20];
    char conn_opt[100];
    EXEC SQL END DECLARE SECTION;

    int  sMode;
    int  idx;

    char server_ip[64];

    if ( argc < 2 )
    {
        showUsage();
        exit(-1);
    }

    sMode = checkMode(argv[1]);
    if ( sMode < 0 )
    {
        printf( "ERR - Unknown mode [%s]\n", argv[1] );
        showUsage();
        exit(-1);
    }

    sprintf( usr,      "SYS" );
    sprintf( pwd,      "MANAGER" );
    sprintf( server_ip,"127.0.0.1" );

    getProperty();

    if( argc > 2)
    {
        for(idx = 0;idx < argc;idx++)
        {
            if( strcasecmp(argv[idx], "-p" ) == 0)
            {
                sprintf(pwd, argv[idx+1]);
            }

            if( strcasecmp(argv[idx], "-s" ) == 0)
            {
                sprintf(server_ip, argv[idx+1]);
            }

            if( strcasecmp(argv[idx], "-port" ) == 0)
            {
                gPortNo = atoi(argv[idx+1]);
            }
        }
    }
    
    sprintf( conn_opt, "DSN=%s;PORT_NO=%d;NLS_USE=%s", server_ip, gPortNo, gNLS );

    EXEC SQL CONNECT :usr IDENTIFIED BY :pwd USING :conn_opt;  
    if ( sqlca.sqlcode == SQL_ERROR ) 
    {
        printf("ERR - [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        exit(1);
    }

    switch (sMode)
    {
    case MODE_DB :
        printf( "MODE - DB\n" );
        statusDB();
        break;
    case MODE_MEMORY :
        printf( "MODE - MEMORY\n" );
        statusMemory();
        break;
    case MODE_SESSION :
        printf( "MODE - SESSION\n" );
        statusSession();
        break;
    case MODE_REPL :
        printf( "MODE - REPLICATION\n" );
        statusRepl();
        break;
    }

    EXEC SQL DISCONNECT;
}

void getProperty()
{
    FILE *fd;
    char *sAltiHomePath;
    char sPropFile[512];
    char sBuff[1024];
    char *sName = NULL;
    char *sValue = NULL;

    sAltiHomePath = getenv("ALTIBASE_HOME");
    if ( sAltiHomePath == NULL )
    {
        printf( "ERR - Can't find ALTIBASE_HOME environment\n" );
        return;
    }

    sprintf( sPropFile, "%s/conf/altibase.properties", sAltiHomePath );
    fd = fopen( sPropFile, "r" );

    if ( fd == NULL )
    {
        printf( "ERR - Can't open property file in [%s]\n", sPropFile );
        return;
    }

    while( !feof(fd) )
    {
        memset( sBuff, 0, 1024 );
        if ( fgets(sBuff, 1024, fd) == NULL )
        {
            break;
        }

        if ( parseBuffer(sBuff, &sName, &sValue) == SQL_ERROR )
        {
            fclose(fd);
            return;
        }

        if (sName)
        {
            if ( strcasecmp( sName, "PORT_NO" ) == 0 )
            {
                if (sValue)
                {
                    gPortNo = atoi(sValue);
                }
            }
            else if ( strcasecmp( sName, "NLS_USE" ) == 0 )
            {
                if (sValue)
                {
                    strcpy(gNLS,sValue);
                }
            }
        }
    }

    fclose(fd);
}

void eraseWhiteSpace(char *buffer)
{       
    int i, j; 
    int len = strlen(buffer);
                
    for (i = 0; i < len && buffer[i]; i++)
    {           
        if (buffer[i] == '#')
        {           
            buffer[i]= 0;
            return;
        }   
        if (isspace(buffer[i]))
        {
            for (j = i;  buffer[j]; j++)
            {
                buffer[j] = buffer[j + 1];
            }
            i--;
        }
    }
}

SQLRETURN parseBuffer( char *buffer,
                       char **Name,
                       char **Value)
{
    int i;
    int len;

    eraseWhiteSpace(buffer);

    len = strlen(buffer);
    if (len == 0 || buffer[0] == '#')
    {
        return SQL_SUCCESS;
    }

    *Name = buffer;

    for (i = 0; i < len; i++)
    {
        if (buffer[i] == '=')
        {
            buffer[i] = 0;

            if (buffer[i + 1])
            {
                *Value = &buffer[i + 1];

                if (strlen(&buffer[i + 1]) > 256)
                {
                    return SQL_ERROR;
                }
            }
            
            return SQL_SUCCESS;
        }
    }
    return SQL_SUCCESS;
}

void showUsage()
{
    printf( "[USAGE] dbStatus {MODE} [options]\n" );
    printf( "        * MODE : db          - DB Information \n" );
    printf( "                 memory      - Memory Information \n" );
    printf( "                 session     - Session Information \n" );
    printf( "                 replication - Replication Information \n" );
    printf( "        * options  : -s      - DB Server IP \n" );
    printf( "                     -port   - DB Server Port\n" );
    printf( "                     -p      - 'SYS' user's Password\n" );
}

int checkMode(char * aModeStr)
{
    if      ( strcasecmp( aModeStr, "db" ) == 0 )
    {
        return MODE_DB;
    }
    else if ( strcasecmp( aModeStr, "memory" ) == 0 )
    {
        return MODE_MEMORY;
    }
    else if ( strcasecmp( aModeStr, "session" ) == 0 )
    {
        return MODE_SESSION;
    }
    else if ( strcasecmp( aModeStr, "replication" ) == 0 )
    {
        return MODE_REPL;
    }
    else
    {
        return -1;
    }
}

void statusDB()
{
    EXEC SQL BEGIN DECLARE SECTION;
    DatabaseInfo hv_database_info;
    MemTblInfo   hv_memtbl_info;
    DiskTblInfo  hv_disktbl_info;
    EXEC SQL END DECLARE SECTION;

    /*** Database Info & Database Usage ***/
    sprintf( query, "SELECT "
                    "    trim(DB_NAME)          , "
                    "    trim(PRODUCT_SIGNATURE), "
                    "    trim(DB_SIGNATURE)     , "
                    "    VERSION_ID             , "
                    "    COMPILE_BIT            , "
                    "    ENDIAN                 , "
                    "    LOGFILE_SIZE           , "
                    "    MEM_DBFILE_SIZE        , "
                    "    TX_TBL_SIZE            , "
                    "    MEM_DBFILE_COUNT_0     , "
                    "    MEM_DBFILE_COUNT_1     , "
                    "    trim(MEM_TIMESTAMP)    , "
                    "    MEM_ALLOC_PAGE_COUNT   , "
                    "    MEM_FREE_PAGE_COUNT    , "
                    "    MEM_FIRST_FREE_PAGE    , "
                    "    DURABLE_SYSTEM_SCN       "
                    "FROM V$DATABASE" );
    
    EXEC SQL PREPARE DI FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get database information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE database_info CURSOR FOR DI;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get database information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN database_info;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get database information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    while(1)
    {
        EXEC SQL FETCH database_info
        INTO :hv_database_info ;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get database information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }
        
        showDatabaseInfo( &hv_database_info );
    }
    printf("\n");

    /*** Memory Table Usage ***/
    sprintf( query, "SELECT "
                    "trim(c.USER_NAME)  , "
                    "trim(b.TABLE_NAME) , "
                    "a.MEM_PAGE_CNT     , "
                    "a.MEM_VAR_PAGE_CNT , "
                    "a.MEMSLOT_PERPAGE  , "
                    "a.MEM_SLOT_SIZE    , "
                    "a.MEM_FIRST_PAGEID   "
                    "FROM V$MEMTBL_INFO a, SYSTEM_.SYS_TABLES_ b, SYSTEM_.SYS_USERS_ c "
                    "WHERE a.TABLE_OID = b.TABLE_OID and b.USER_ID = c.USER_ID "
                    "  and c.USER_NAME <> 'SYSTEM_'" );
    
    EXEC SQL PREPARE MT FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get memory table information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE memtbl_info CURSOR FOR MT;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get memory table information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN memtbl_info;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get memory table information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    printf( "<< Memory Table Usage >>\n" );
    while(1)
    {
        EXEC SQL FETCH memtbl_info
        INTO :hv_memtbl_info ;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get memory table information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }
        
        showMemTblInfo( &hv_memtbl_info );
    }
    printf("\n");

    /*** Disk Table Usage ***/
    sprintf( query, "SELECT "
                    "trim(c.USER_NAME)   , "
                    "trim(b.TABLE_NAME)  , "
                    "a.DISK_PAGE_CNT     , "
                    "a.INSERT_HIGH_LIMIT , "
                    "a.INSERT_LOW_LIMIT   "
                    "FROM V$DISKTBL_INFO a, SYSTEM_.SYS_TABLES_ b, SYSTEM_.SYS_USERS_ c "
                    "WHERE a.TABLE_OID = b.TABLE_OID and b.USER_ID = c.USER_ID "
                    "  and c.USER_NAME <> 'SYSTEM_'" );
    
    EXEC SQL PREPARE DT FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get disk table information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE disktbl_info CURSOR FOR DT;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get disk table information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN disktbl_info;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get disk table information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    printf( "<< Disk Table Usage >>\n" );
    while(1)
    {
        EXEC SQL FETCH disktbl_info
        INTO :hv_disktbl_info ;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get disk table information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }
        
        showDiskTblInfo( &hv_disktbl_info );
    }
}

void statusMemory()
{
    EXEC SQL BEGIN DECLARE SECTION;
    char        hv_name[129];
    int         hv_name_ind;
    long long   hv_size;
    int         hv_size_ind;
    EXEC SQL END DECLARE SECTION;

    sprintf( query, "SELECT trim(NAME), ALLOC_SIZE/1024/1024 FROM X$MEMSTAT" );

    EXEC SQL PREPARE Q FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get memory information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE memstat_cur CURSOR FOR Q;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get memory information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN memstat_cur;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get memory information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    while(1)
    {
        EXEC SQL FETCH memstat_cur
        INTO :hv_name :hv_name_ind, :hv_size :hv_size_ind ;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get memory information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }

        printf( "%25s : %10lld Mb\n", hv_name, hv_size );
    }
}

void statusSession()
{
    EXEC SQL BEGIN DECLARE SECTION;
    SessionInfo hv_session_info;
    EXEC SQL END DECLARE SECTION;

    sprintf( query, "SELECT "
                    "ID PID, "
                    "CLIENT_ID CID, "
                    "trim(COMM_NAME), "
                    "decode(XA_SESSION_FLAG, 0, 'Non XA Session', 1, 'XA Session'), "
                    "QUERY_TIME_LIMIT, "
                    "FETCH_TIME_LIMIT, "
                    "UTRANS_TIME_LIMIT, "
                    "IDLE_TIME_LIMIT, "
                    "decode(IDLE_START_TIME, 0, 0, b.BASE_TIME - IDLE_START_TIME), "
                    "decode(ACTIVE_FLAG, 0, 'Tx Inactivated', 1, 'Tx Activated'), "
                    "OPENED_STMT_COUNT, "
                    "trim(CLIENT_CONSTR), "
                    "trim(CLIENT_IDN), "
                    "trim(DB_USERNAME), "
                    "CLIENT_PID, "
                    "decode(SYSDBA_FLAG, 0, 'General', 1, 'SYSDBA' ), "
                    "decode(AUTOCOMMIT_FLAG, 0, 'Non Autocommit Mode', 1, 'Autocommit Mode'), "
                    "decode(SESSION_STATE, 0, 'Initializing', "
                    "                      1, 'Connected for Auth', "
                    "                      2, 'Autherization Init', "
                    "                      3, 'Autherization Process', "
                    "                      4, 'Autherization Complete', "
                    "                      5, 'On Serviece', "
                    "                      6, 'Disconnecting', "
                    "                      7, 'Rollback & Disconnecting'), "
                    "decode(PROTOCOL_SESSION, 1397051990, 'Handshake for sysdba dispatch', "
                    "                         1094995278, 'Handshake for users dispatch', "
                    "                         1129270862, 'Connect', "
                    "                         1145652046, 'Disconnect', "
                    "                         1229870668, 'Invalid', "
                    "                         1280460613, 'Large', "
                    "                         1414352724, 'Timeout', "
                    "                         1163022162, 'Error', "
                    "                         1347568976, 'Prepare', "
                    "                         1163412803, 'Execute', "
                    "                         1162103122, 'ExecDirect', "
                    "                         1179927368, 'Fetch', "
                    "                         1179796805, 'Free', "
                    "                         1229931858, 'I/O Error', "
                    "                         1161905998, 'EAGIN Error', "
                    "                         1480672077, 'XA Commands', "
                    "                         1094929998, 'Acknowledge', "
                    "                         1195725908, 'Get' ), "
                    "ISOLATION_LEVEL, "
                    "OPTIMIZER_MODE, "
                    "CURRENT_STMT_ID, "
                    "CONTINUOUS_FAILURE_COUNT, "
                    "trim(DEFAULT_DATE_FORMAT) "
                    "FROM X$SESSION, X$SESSIONMGR b" );

    sprintf( query_2, "SELECT "
                      "a.ID , "
                      "a.CURSOR_TYPE , "
                      "a.TX_ID , "
                      "trim(a.QUERY) , "
                      "decode(a.QUERY_START_TIME, 0, 0, b.BASE_TIME - a.QUERY_START_TIME), "
                      "decode(a.FETCH_START_TIME, 0, 0, b.BASE_TIME - a.FETCH_START_TIME), "
                      "decode(STATE, 0, 'State Init', "
                      "              1, 'Prepared', "
                      "              2, 'Fetch Ready', "
                      "              3, 'Fetch Proceeding' ), "
                      "decode(BEGIN_FLAG, 0, 'Wait', 1, 'Begin') "
                      "FROM X$STATEMENT a, X$SESSIONMGR b " );
    
    EXEC SQL PREPARE SQ FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get session information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE session_stat_cur CURSOR FOR SQ;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get session information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN session_stat_cur;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get session information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    while(1)
    {
        EXEC SQL FETCH session_stat_cur
        INTO :hv_session_info ;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get session information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }
        
        showSessionInfo( &hv_session_info );
        statusStmt( hv_session_info.ID );
        printf( "\n" );
    }
}

void statusStmt(long long aSid)
{
    EXEC SQL BEGIN DECLARE SECTION;
    StmtInfo hv_stmt_info;
    EXEC SQL END DECLARE SECTION;

    sprintf( query_2, "SELECT "
                      "a.ID , "
                      "decode(a.CURSOR_TYPE ,2, 'Memory Cursor', 4, 'Disk Cursor', 6, 'M+D Cursor') , "
                      "a.TX_ID , "
                      "trim(a.QUERY) , "
                      "decode(a.QUERY_START_TIME, 0, 0, b.BASE_TIME - a.QUERY_START_TIME), "
                      "decode(a.FETCH_START_TIME, 0, 0, b.BASE_TIME - a.FETCH_START_TIME), "
                      "decode(STATE, 0, 'Init', "
                      "              1, 'Prepared', "
                      "              2, 'Fetch Ready', "
                      "              3, 'Fetch Proceeding' ), "
                      "decode(BEGIN_FLAG, 0, 'Wait', 1, 'Begin') "
                      "FROM X$STATEMENT a, X$SESSIONMGR b " 
                      "WHERE SESSION_ID = %lld ", aSid );
    
    EXEC SQL PREPARE SMQ FROM :query_2;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get statement information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE stmt_stat_cur CURSOR FOR SMQ;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get statement information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN stmt_stat_cur;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get statement information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    while(1)
    {
        EXEC SQL FETCH stmt_stat_cur
        INTO :hv_stmt_info ;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get statement information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }
        
        showStmtInfo( &hv_stmt_info );
    }
}

void statusRepl()
{
    EXEC SQL BEGIN DECLARE SECTION;
    ReplSenderInfo   hv_repl_sender_info;
    ReplReceiverInfo hv_repl_receiver_info;
    EXEC SQL END DECLARE SECTION;

    /*** Sender ***/
    printf( "=== Replication Sender List ===\n" );
    sprintf( query, "SELECT "
                    "trim(REP_NAME), "
                    "decode(START_FLAG, 0, 'Normal', "
                    "                   1, 'Quick', "
                    "                   2, 'Sync', "
                    "                   3, 'Sync Only'), "
                    "decode(NET_ERROR_FLAG, 0, 'Connected',1, 'Disconnected'), "
                    "XLSN_FINENO, "
                    "XLSN_OFFSET, "
                    "COMMIT_LSN_FINENO, "
                    "COMMIT_LSN_OFFSET, "
                    "decode(STATUS, 0, 'Stop', 1, 'Run', 2, 'Retry'), "
                    "trim(SENDER_IP), "
                    "trim(PEER_IP), "
                    "SENDER_PORT, "
                    "PEER_PORT "
                    "FROM X$REPSENDER " );
    
    EXEC SQL PREPARE RS FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get replication information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE repsend_cur CURSOR FOR RS;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get replication information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN repsend_cur;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get replication information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    while(1)
    {
        EXEC SQL FETCH repsend_cur
        INTO :hv_repl_sender_info;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get replication information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }

        showReplSenderInfo(&hv_repl_sender_info);
    }

    printf( "\n" );
    
    /*** Receiver ***/
    printf( "=== Replication Receiver List ===\n" );
    sprintf( query, "SELECT "
                    "trim(REP_NAME), "
                    "trim(MY_IP), "
                    "trim(PEER_IP), "
                    "MY_PORT, "
                    "PEER_PORT "
                    "FROM X$REPRECEIVER " );
    
    EXEC SQL PREPARE RR FROM :query;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get replication information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL DECLARE reprecv_cur CURSOR FOR RR;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get replication information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    EXEC SQL OPEN reprecv_cur;
    if ( sqlca.sqlcode == SQL_ERROR )
    {
        printf("ERR - Can't get replication information\n");
        printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
        return;
    }

    while(1)
    {
        EXEC SQL FETCH reprecv_cur
        INTO :hv_repl_receiver_info;
        if ( sqlca.sqlcode == SQL_ERROR )
        {
            printf("ERR - Can't get replication information\n");
            printf("      [%d] %s\n\n", SQLCODE, sqlca.sqlerrm.sqlerrmc);
            return;
        }
        else if ( sqlca.sqlcode == SQL_NO_DATA )
        {
            break;
        }

        showReplReceiverInfo(&hv_repl_receiver_info);
    }
}

void showDatabaseInfo( DatabaseInfo * aDatabaseInfo )
{
    printf( "<< Database Info >>\n" );
    printf( "Version Info   -- %d\n", aDatabaseInfo->VERSION_ID );
    printf( "Database Name  -- %s\n", aDatabaseInfo->DB_NAME );
    printf( "Product Info   -- %s\n", aDatabaseInfo->PRODUCT_SIGNATURE );
    printf( "Storage Info   -- %s\n", aDatabaseInfo->DB_SIGNATURE );
    printf( "Compiler Bit   -- %d\n", aDatabaseInfo->COMPILE_BIT );
    printf( "Endian Info    -- %lld\n", aDatabaseInfo->ENDIAN );
    printf( "Tx Table Size  -- %d\n", aDatabaseInfo->TX_TBL_SIZE );
    printf( "Timestamp      -- %s\n", aDatabaseInfo->TIMESTAMP );
    printf( "System SCN     -- %lld\n", aDatabaseInfo->DURABLE_SYSTEM_SCN );
    printf( "Log File Size  -- %lld\n", aDatabaseInfo->LOGFILE_SIZE );
    printf( "DB File Size   -- %lld\n", aDatabaseInfo->DBFILE_SIZE );
    printf( "DB File Count  -- (First)  = %d\n", aDatabaseInfo->DBFILE_COUNT_0 );
    printf( "DB File Count  -- (Second) = %d\n", aDatabaseInfo->DBFILE_COUNT_1 );
    printf( "\n" );
    printf( "<< Database Usage >>\n" );
    printf( "Allocate Page  -- %lld (Current Size = %.3lfM)\n",
            aDatabaseInfo->ALLOC_PAGE_COUNT, (double)(aDatabaseInfo->ALLOC_PAGE_COUNT)*8/1024 );
    printf( "Used Page      -- %lld (Current Size = %.3lfM)\n",
            (aDatabaseInfo->ALLOC_PAGE_COUNT-aDatabaseInfo->FREE_PAGE_COUNT),
            (double)(aDatabaseInfo->ALLOC_PAGE_COUNT-aDatabaseInfo->FREE_PAGE_COUNT)*8/1024 );
    printf( "Free Page      -- %lld (Current Size = %.3lfM)\n",
            aDatabaseInfo->FREE_PAGE_COUNT, (double)(aDatabaseInfo->FREE_PAGE_COUNT)*8/1024 );
    printf( "\n" );
}

void showMemTblInfo( MemTblInfo * aMemTblInfo )
{
    printf( "User.Table : %s.%s\n", aMemTblInfo->USER_NAME, aMemTblInfo->TABLE_NAME );
    printf( "             Slot Size : %lld (1 page = %d slots)\n", 
            aMemTblInfo->MEM_SLOT_SIZE, aMemTblInfo->MEM_SLOT_CNT );
    printf( "             Alloc Mem = Total:%.3lfM, Fixed:%.3lfM, Variable:%.3lfM\n", 
            ((double)(aMemTblInfo->MEM_PAGE_CNT+aMemTblInfo->MEM_VAR_PAGE_CNT)*8/1024),
             (double)(aMemTblInfo->MEM_PAGE_CNT)*8/1024,
             (double)(aMemTblInfo->MEM_VAR_PAGE_CNT)*8/1024 );
}

void showDiskTblInfo( DiskTblInfo * aDiskTblInfo )
{
    printf( "User.Table : %s.%s\n", aDiskTblInfo->USER_NAME, aDiskTblInfo->TABLE_NAME );
    printf( "             Attribute : High = %d, Low = %d\n", 
            aDiskTblInfo->INSERT_HIGH_LIMIT, aDiskTblInfo->INSERT_LOW_LIMIT );
    printf( "             Alloc Mem = %.3lfM\n", (double)(aDiskTblInfo->DISK_PAGE_CNT)*8/1024 );
}

void showSessionInfo( SessionInfo * aSessionInfo )
{
    printf( "Session [%d] <Type : %s, CID : %d> <From : %s,%lld>\n",
            aSessionInfo->ID, 
            aSessionInfo->CLIENT_CONSTR,
            aSessionInfo->CLIENT_ID, 
            aSessionInfo->COMM_NAME,
            aSessionInfo->CLIENT_PID );
    printf( "          State           : %s\n", aSessionInfo->SESSION_STATE );
    printf( "          Attribute       : <%s> <%s>\n", aSessionInfo->AUTOCOMMIT_FLAG,
                                                 aSessionInfo->ACTIVE_FLAG );
    printf( "          Isolation Level : %d\n", aSessionInfo->ISOLATION_LEVEL);
    printf( "          Timeout Setting : <Q : %d> <F : %d> <U : %d>\n",  
            aSessionInfo->QUERY_TIME_LIMIT, 
            aSessionInfo->FETCH_TIME_LIMIT,
            aSessionInfo->UTRANS_TIME_LIMIT );
    printf( "          Connection Attr : <User : %s> <NLS : %s> <%s>\n", 
            aSessionInfo->DB_USERNAME, aSessionInfo->CLIENT_IDN, aSessionInfo->SYSDBA_FLAG);
    printf( "          Is XA Session   : %s\n", aSessionInfo->XA_SESSION_FLAG);
    printf( "          Open Stmt Count : %d\tCurrent stmt ID : %lld\n", 
            aSessionInfo->OPENED_STMT_COUNT, aSessionInfo->CURRENT_STMT_ID);
    printf( "          Protocol        : [%s]\n", aSessionInfo->PROTOCOL_SESSION);
    printf( "          Continuous Fail : %lld\n", aSessionInfo->CONTINUOUS_FAILURE_COUNT);
    printf( "          Default Dateform: [%s]\n", aSessionInfo->DEFAULT_DATE_FORMAT);
    printf( "          Idle Time       : %d / %d\n", aSessionInfo->IDLE_START_TIME,
                                                     aSessionInfo->IDLE_TIME_LIMIT );
}

void showStmtInfo( StmtInfo * aStmtInfo )
{
    printf( "          Statement [%lld] : ", aStmtInfo->ID );
    printf( "State = %s (%s) : ", aStmtInfo->STATE, aStmtInfo->BEGIN_FLAG );
    printf( "Cursor Type = %s : ", aStmtInfo->CURSOR_TYPE );
    printf( "TxID=[%lld] : ", aStmtInfo->TX_ID );
    printf( "Time [Q=>%d, F=>%d]\n",  aStmtInfo->QUERY_START_TIME, aStmtInfo->FETCH_START_TIME );
    printf( "             => \"%s\"\n", aStmtInfo->QUERY );
}

void showReplSenderInfo(ReplSenderInfo * aReplSenderInfo)
{
    printf( "    %s(%s,%d->%s,%d) Sender Status : %s, CommitLSN : (%d,%d), XLSN : (%d,%d)\n",
            aReplSenderInfo->REP_NAME,
            aReplSenderInfo->SENDER_IP,
            aReplSenderInfo->SENDER_PORT,
            aReplSenderInfo->PEER_IP,
            aReplSenderInfo->PEER_PORT,
            aReplSenderInfo->STATUS,
            aReplSenderInfo->COMMIT_LSN_FINENO,
            aReplSenderInfo->COMMIT_LSN_OFFSET,
            aReplSenderInfo->XLSN_FINENO,
            aReplSenderInfo->XLSN_OFFSET );

}

void showReplReceiverInfo(ReplReceiverInfo * aReplReceiverInfo)
{
    printf( "    %s(%s,%d->%s,%d) \n",
            aReplReceiverInfo->REP_NAME,
            aReplReceiverInfo->SENDER_IP,
            aReplReceiverInfo->SENDER_PORT,
            aReplReceiverInfo->PEER_IP,
            aReplReceiverInfo->PEER_PORT );
}
