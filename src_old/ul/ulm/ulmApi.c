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

#include <ulmApiCore.h>

#define ULM_INIT( aQueryType )                                      \
        gResourceManagerPtr = &( gResourceManager[aQueryType] );    \
        gFetchedRowCount = 0;                                       \
        gErrCode = ULM_INIT_ERR_CODE

// BUG-38917 [mm-altibaseMonitor] The stmt handle related to error should be free.
#define ULM_FREE_HSTMT( aHStmt );                                   \
        if ( aHStmt != SQL_NULL_HSTMT )                             \
        {                                                           \
            (void)SQLFreeHandle( SQL_HANDLE_STMT, aHStmt );         \
            aHStmt = SQL_NULL_HSTMT;                                \
        }

/******************************
 *     Extern declaration     *
 ******************************/

extern SQLHANDLE           gHEnv;
extern SQLHANDLE           gHDbc;
extern ulmResourceManager  gResourceManager[];
extern ulmResourceManager *gResourceManagerPtr;
extern acp_uint32_t        gFetchedRowCount;
extern ulmErrCode          gErrCode;
// BUG-33946 The maxgauge library should provide the API that sets the user and the password.
extern ulmProperties       gProperties[];
extern acp_std_file_t      gLogFile;


/*******************************************
 *     Implementation of API functions     *
 *******************************************/

// BUG-33946 The maxgauge library should provide the API that sets the user and the password.
int ABISetProperty( ABIPropType aPropType, const char *aPropValue )
{
    if( aPropValue != NULL )
    {
        ACI_TEST_RAISE( ( acpCStrLen( aPropValue, ACP_UINT32_MAX ) >= ULM_SIZE_2048 ), ERR_SET_TOO_LONG_PROPERTY_VALUE );
        acpMemCpy( gProperties[aPropType].mPropValue, aPropValue, ULM_SIZE_2048 );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_SET_TOO_LONG_PROPERTY_VALUE );
    {
        ULM_SET_ERR_CODE( ULM_ERR_SET_TOO_LONG_PROPERTY_VALUE );
    }
    ACI_EXCEPTION_END;

    return gErrCode;
}

int ABIInitialize( void )
{
    acp_char_t     sConnectionURL[ULM_SIZE_1024];
    static ulmFlag sIsBlockedSigPipe = ULM_FLAG_FREE;

    if( sIsBlockedSigPipe != ULM_FLAG_SET )
    {
        acpSignalBlock( ACP_SIGNAL_SET_PIPE );
        sIsBlockedSigPipe = ULM_FLAG_SET;
    }

    // Open a log file.
    ACI_TEST_RAISE( acpStdOpen( &gLogFile,
                                gProperties[ABI_LOGFILE].mPropValue,
                                ACP_STD_OPEN_APPEND_TEXT ) != ACP_RC_SUCCESS,
                    ERR_OPEN_LOGFILE );

    // Allocate an environment handle.
    ACI_TEST_RAISE( ( SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &gHEnv ) != SQL_SUCCESS ), ERR_ALLOC_HANDLE_ENV );

    // Allocate a database connection handle.
    ACI_TEST_RAISE( ( SQLAllocHandle( SQL_HANDLE_DBC, gHEnv, &gHDbc ) != SQL_SUCCESS ), ERR_ALLOC_HANDLE_DBC );

    /*
       IF the CONNTYPE sets to
       1 - TCP_IP, 2 - UNIX_DOMAIN, 3 - IPC.
     */
    acpSnprintf( sConnectionURL,
                 ACI_SIZEOF( sConnectionURL ),
                 "CONNTYPE=%d;UID=%s;PWD=%s",
                 2,
                 gProperties[ABI_USER].mPropValue,
                 gProperties[ABI_PASSWD].mPropValue );

    // Connect to the Altibase server.
    ACI_TEST_RAISE( ( SQLDriverConnect( gHDbc, NULL, (SQLCHAR *)sConnectionURL, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT )
                      != SQL_SUCCESS ), ERR_CONNECT_DB );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_OPEN_LOGFILE );
    {
        ULM_SET_ERR_CODE( ULM_ERR_OPEN_LOGFILE );
    }
    ACI_EXCEPTION( ERR_ALLOC_HANDLE_ENV );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_HANDLE_ENV );
    }
    ACI_EXCEPTION( ERR_ALLOC_HANDLE_DBC );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_HANDLE_DBC );
    }
    ACI_EXCEPTION( ERR_CONNECT_DB );
    {
        ULM_SET_ERR_CODE( ULM_ERR_CONNECT_DB );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIInitialize" );

    return gErrCode;
}

int ABIFinalize( void )
{
    // Clean up the resource.
    ACI_TEST( ulmCleanUpResource() != ACI_SUCCESS );

    // Disconnect from the Altibase server.
    ACI_TEST_RAISE( ( SQLDisconnect( gHDbc ) != SQL_SUCCESS ), ERR_DISCONNECT_DB );

    // Free the database handle.
    ACI_TEST_RAISE( ( SQLFreeHandle( SQL_HANDLE_DBC, gHDbc ) != SQL_SUCCESS ), ERR_FREE_HANDLE_DBC );
    gHDbc = SQL_NULL_HDBC;

    // Free the environment handle.
    ACI_TEST_RAISE( ( SQLFreeHandle( SQL_HANDLE_ENV, gHEnv ) != SQL_SUCCESS ), ERR_FREE_HANDLE_ENV );
    gHEnv = SQL_NULL_HENV;

    // Close the log file.
    acpStdFlush( &gLogFile );
    ACI_TEST_RAISE( acpStdClose( &gLogFile ) != ACP_RC_SUCCESS, ERR_CLOSE_LOGFILE );

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    gErrCode = ULM_INIT_ERR_CODE;

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_DISCONNECT_DB );
    {
        ULM_SET_ERR_CODE( ULM_ERR_DISCONNECT_DB );
    }
    ACI_EXCEPTION( ERR_FREE_HANDLE_DBC );
    {
        ULM_SET_ERR_CODE( ULM_ERR_FREE_HANDLE_DBC );
    }
    ACI_EXCEPTION( ERR_FREE_HANDLE_ENV );
    {
        ULM_SET_ERR_CODE( ULM_ERR_FREE_HANDLE_ENV );
    }
    ACI_EXCEPTION( ERR_CLOSE_LOGFILE );
    {
        ULM_SET_ERR_CODE( ULM_ERR_CLOSE_LOGFILE );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIFinalize" );

    return gErrCode;
}

int ABICheckConnection( void )
{
    SQLINTEGER sIsConnection = 0;

    /*
       IF the sIsConnection value sets to
       SQL_CD_FALSE - The connection is still active.
       SQL_CD_TRUE  - The connection is terminated.
     */
    ACI_TEST( SQLGetConnectAttr( gHDbc, SQL_ATTR_CONNECTION_DEAD, &sIsConnection, 0, NULL ) != SQL_SUCCESS );
    ACI_TEST( sIsConnection != SQL_CD_FALSE );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABICheckConnection" );

    return ACI_FAILURE;
}

int ABIGetVSession( ABIVSession **aHandle, unsigned int aExecutingOnly )
{
    acp_uint32_t sRowCount = 0;

    /*
       IF the aExecutingOnly value is
       ULM_FLAG_SET  - Return executing sessions only.
       ULM_FLAG_FREE - Return all sessions.
     */
    if( aExecutingOnly != ULM_FLAG_FREE )
    {
        ULM_INIT( ULM_V_SESSION_EXECUTING_ONLY );
    }
    else
    {
        ULM_INIT( ULM_V_SESSION );
    }

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSession ), ACI_SIZEOF( ABIVSession ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSession ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSession *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSession" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSessionBySID( ABIVSession **aHandle, int aSessionID )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SESSION_BY_SID );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSession ), ACI_SIZEOF( ABIVSession ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST_RAISE( ( SQLBindParameter( gResourceManagerPtr->mHStmt,
                                        1,
                                        SQL_PARAM_INPUT,
                                        SQL_C_SLONG,
                                        SQL_INTEGER,
                                        0,
                                        0,
                                        (SQLPOINTER)&aSessionID,
                                        0,
                                        NULL ) != SQL_SUCCESS ), ERR_BIND_PARAMETER );

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSession ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSession *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION( ERR_BIND_PARAMETER );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_PARAMETER );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSessionBySID" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSysstat( ABIVSysstat **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SYSSTAT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSysstat ), ACI_SIZEOF( ABIVSysstat ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSysstat ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSysstat *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSysstat" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

// BUG-34728: cpu is too high: 2번째 인자 추가
int ABIGetVSesstat( ABIVSesstat **aHandle, unsigned int aExecutingOnly)
{
    acp_uint32_t sRowCount = 0;

    // BUG-34728: cpu is too high
    // 인자값이 1인경우 active 세션의 stat만 조회 (join)
    // 인자값이 0인 경우 전체 세션 조회
    /*
       IF the aExecutingOnly value is
       ULM_FLAG_SET  - Return executing sessions' stat only.
       ULM_FLAG_FREE - Return all sessions' stat.
     */
    if( aExecutingOnly != ULM_FLAG_FREE )
    {
        ULM_INIT( ULM_V_SESSTAT_EXECUTING_ONLY );
    }
    else
    {
        ULM_INIT( ULM_V_SESSTAT );
    }

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSesstat ), ACI_SIZEOF( ABIVSesstat ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSesstat ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSesstat *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSesstat" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSesstatBySID( ABIVSesstat **aHandle, int aSessionID )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SESSTAT_BY_SID );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSesstat ), ACI_SIZEOF( ABIVSesstat ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST_RAISE( ( SQLBindParameter( gResourceManagerPtr->mHStmt,
                                        1,
                                        SQL_PARAM_INPUT,
                                        SQL_C_SLONG,
                                        SQL_INTEGER,
                                        0,
                                        0,
                                        (SQLPOINTER)&aSessionID,
                                        0,
                                        NULL ) != SQL_SUCCESS ), ERR_BIND_PARAMETER );

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSesstat ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSesstat *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION( ERR_BIND_PARAMETER );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_PARAMETER );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSesstatBySID" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetStatName( ABIStatName **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_STAT_NAME );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmStatName ), ACI_SIZEOF( ABIStatName ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIStatName ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIStatName *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetStatName" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSystemEvent( ABIVSystemEvent **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SYSTEM_EVENT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSystemEvent ), ACI_SIZEOF( ABIVSystemEvent ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSystemEvent ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSystemEvent *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSystemEvent" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSessionEvent( ABIVSessionEvent **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SESSION_EVENT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSessionEvent ), ACI_SIZEOF( ABIVSessionEvent ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSessionEvent ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSessionEvent *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSessionEvent" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return ACI_FAILURE;
}

int ABIGetVSessionEventBySID( ABIVSessionEvent **aHandle, int aSessionID )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SESSION_EVENT_BY_SID );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSessionEvent ), ACI_SIZEOF( ABIVSessionEvent ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST_RAISE( ( SQLBindParameter( gResourceManagerPtr->mHStmt,
                                        1,
                                        SQL_PARAM_INPUT,
                                        SQL_C_SLONG,
                                        SQL_INTEGER,
                                        0,
                                        0,
                                        (SQLPOINTER)&aSessionID,
                                        0,
                                        NULL ) != SQL_SUCCESS ), ERR_BIND_PARAMETER );

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSessionEvent ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSessionEvent *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION( ERR_BIND_PARAMETER );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_PARAMETER );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSessionEventBySID" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetEventName( ABIEventName **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_EVENT_NAME );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmEventName ), ACI_SIZEOF( ABIEventName ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIEventName ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIEventName *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetEventName" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSessionWait( ABIVSessionWait **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SESSION_WAIT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSessionWait ), ACI_SIZEOF( ABIVSessionWait ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSessionWait ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSessionWait *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSessionWait" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetVSessionWaitBySID( ABIVSessionWait **aHandle, int aSessionID )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_V_SESSION_WAIT_BY_SID );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmVSessionWait ), ACI_SIZEOF( ABIVSessionWait ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST_RAISE( ( SQLBindParameter( gResourceManagerPtr->mHStmt,
                                        1,
                                        SQL_PARAM_INPUT,
                                        SQL_C_SLONG,
                                        SQL_INTEGER,
                                        0,
                                        0,
                                        (SQLPOINTER)&aSessionID,
                                        0,
                                        NULL ) != SQL_SUCCESS ), ERR_BIND_PARAMETER );

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIVSessionWait ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIVSessionWait *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION( ERR_BIND_PARAMETER );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_PARAMETER );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetVSessionWaitBySID" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetSqlText( ABISqlText **aHandle, int aStmtID )
{
    acp_uint32_t  sRowCount = 0;
    ABISqlText   *sSqlText;

    // BUG-34728: cpu is too high
    // stmtID 인자로 0이 들어오면, active 세션들의 sql text를 조회 (새로추가됨)
    // stmtID 인자로 0이 아니면  , 해당 stmtID의 sql text를 조회
    if (aStmtID == 0)
    {
        ULM_INIT( ULM_SQL_TEXT );
    }
    else
    {
        ULM_INIT( ULM_SQL_TEXT_BY_STMT_ID );
    }

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmSqlText ), ACI_SIZEOF( ABISqlText ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    // BUG-34728: cpu is too high
    // 특정 sql text를 조회하는 경우만 binding하면 됨
    if (aStmtID != 0)
    {
        ACI_TEST_RAISE( ( SQLBindParameter( gResourceManagerPtr->mHStmt,
                                            1,
                                            SQL_PARAM_INPUT,
                                            SQL_C_SLONG,
                                            SQL_INTEGER,
                                            0,
                                            0,
                                            (SQLPOINTER)&aStmtID,
                                            0,
                                            NULL ) != SQL_SUCCESS ),
                        ERR_BIND_PARAMETER );
    }

    // BUGBUG: 상위 레이어에서 return 값 검사만 잘해줘도 되는데, 그렇지 않은 경우를 위한 방어 코드.
    sSqlText = (ABISqlText *)gResourceManagerPtr->mResultArray;
    sSqlText->mSqlText[0] = 0x00;
    sSqlText->mTextLength = 0;
    /* BUG-41825 */
    sSqlText->mQueryStartTime = 0;
    sSqlText->mExecuteFlag = 0;
    sSqlText->mSqlCacheTextID[0] = '\0';
    /* BUG-46436 */
    sSqlText->mParseTime = 0;
    sSqlText->mSoftPrepareTime = 0;
    sSqlText->mLastQueryStartTime = 0;
    sSqlText->mExecuteTime = 0;
    sSqlText->mFetchTime = 0;
    sSqlText->mFetchStartTime = 0;
    sSqlText->mTotalTime = 0;
    sSqlText->mValidateTime = 0;
    sSqlText->mOptimizeTime = 0;

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABISqlText ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABISqlText *)gResourceManagerPtr->mResultArray;

    return sRowCount;

    ACI_EXCEPTION( ERR_BIND_PARAMETER );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_PARAMETER );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetSqlText" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetLockPairBetweenSessions( ABILockPair **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_LOCK_PAIR_BETWEEN_SESSIONS );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmLockPair ), ACI_SIZEOF( ABILockPair ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABILockPair ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABILockPair *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetLockPairBetweenSessions" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetDBInfo( ABIDBInfo **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_DB_INFO );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmDBInfo ), ACI_SIZEOF( ABIDBInfo ), 2 ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIDBInfo ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIDBInfo *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetDBInfo" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetReadCount( ABIReadCount **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_READ_COUNT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmReadCount ), ACI_SIZEOF( ABIReadCount ), 2 ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIReadCount ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIReadCount *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetReadCount" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetSessionCount( unsigned int aExecutingOnly )
{
    SQLLEN       sIndicator = 0;
    acp_uint32_t sSessionCount = 0;

    /*
       IF the aExecutingOnly value is
       ULM_FLAG_SET  - Return executing session count.
       ULM_FLAG_FREE - Return all session count.
     */
    if( aExecutingOnly != ULM_FLAG_FREE )
    {
        ULM_INIT( ULM_SESSION_COUNT_EXECUTING_ONLY );
    }
    else
    {
        ULM_INIT( ULM_SESSION_COUNT );
    }

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        // Allocate a statement handle.
        ACI_TEST_RAISE( ( SQLAllocHandle( SQL_HANDLE_STMT, gHDbc, &( gResourceManagerPtr->mHStmt ) ) != SQL_SUCCESS ),
                        ERR_ALLOC_HANDLE_STMT );

        // Prepare the statement.
        ACI_TEST_RAISE( ( SQLPrepare( gResourceManagerPtr->mHStmt, (SQLCHAR *)gResourceManagerPtr->mQuery, SQL_NTS ) != SQL_SUCCESS ),
                        ERR_PREPARE_STMT );
    }

    // Execute the statement.
    ACI_TEST_RAISE( ( SQLExecute( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_EXECUTE_STMT );

    // Bind a column.
    ACI_TEST_RAISE( ( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_ULONG, &sSessionCount, ACI_SIZEOF( sSessionCount ), &sIndicator )
                      != SQL_SUCCESS ), ERR_BIND_COLUMN );

    // Fetch a row.
    ACI_TEST_RAISE( ( SQLFetch( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_FETCH_RESULT );

    // Close the cursor.
    ACI_TEST_RAISE( ( SQLCloseCursor( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_CLOSE_CURSOR );

    return sSessionCount;

    ACI_EXCEPTION( ERR_ALLOC_HANDLE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_HANDLE_STMT );
    }
    ACI_EXCEPTION( ERR_PREPARE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_PREPARE_STMT );
    }
    ACI_EXCEPTION( ERR_EXECUTE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_EXECUTE_STMT );
    }
    ACI_EXCEPTION( ERR_BIND_COLUMN );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_COLUMN );
    }
    ACI_EXCEPTION( ERR_FETCH_RESULT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_FETCH_RESULT );
    }
    ACI_EXCEPTION( ERR_CLOSE_CURSOR );
    {
        ULM_SET_ERR_CODE( ULM_ERR_CLOSE_CURSOR );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetSessionCount" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetMaxClientCount( void )
{
    SQLLEN       sIndicator = 0;
    acp_uint32_t sMaxClientCount = 0;

    ULM_INIT( ULM_MAX_CLIENT_COUNT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        // Allocate a statement handle.
        ACI_TEST_RAISE( ( SQLAllocHandle( SQL_HANDLE_STMT, gHDbc, &( gResourceManagerPtr->mHStmt ) ) != SQL_SUCCESS ),
                        ERR_ALLOC_HANDLE_STMT );

        // Prepare the statement.
        ACI_TEST_RAISE( ( SQLPrepare( gResourceManagerPtr->mHStmt, (SQLCHAR *)gResourceManagerPtr->mQuery, SQL_NTS ) != SQL_SUCCESS ),
                        ERR_PREPARE_STMT );
    }

    // Execute the statement.
    ACI_TEST_RAISE( ( SQLExecute( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_EXECUTE_STMT );

    // Bind a column.
    ACI_TEST_RAISE( ( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_ULONG, &sMaxClientCount, ACI_SIZEOF( sMaxClientCount ), &sIndicator )
                      != SQL_SUCCESS ), ERR_BIND_COLUMN );

    // Fetch a row.
    ACI_TEST_RAISE( ( SQLFetch( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_FETCH_RESULT );

    // Close the cursor.
    ACI_TEST_RAISE( ( SQLCloseCursor( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_CLOSE_CURSOR );

    return sMaxClientCount;

    ACI_EXCEPTION( ERR_ALLOC_HANDLE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_HANDLE_STMT );
    }
    ACI_EXCEPTION( ERR_PREPARE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_PREPARE_STMT );
    }
    ACI_EXCEPTION( ERR_EXECUTE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_EXECUTE_STMT );
    }
    ACI_EXCEPTION( ERR_BIND_COLUMN );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_COLUMN );
    }
    ACI_EXCEPTION( ERR_FETCH_RESULT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_FETCH_RESULT );
    }
    ACI_EXCEPTION( ERR_CLOSE_CURSOR );
    {
        ULM_SET_ERR_CODE( ULM_ERR_CLOSE_CURSOR );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetMaxClientCount" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

int ABIGetLockWaitSessionCount( void )
{
    SQLLEN       sIndicator = 0;
    acp_uint32_t sLockWaitSessionCount = 0;

    ULM_INIT( ULM_LOCK_WAIT_SESSION_COUNT );

    // Prepare the statement.
    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        // Allocate a statement handle.
        ACI_TEST_RAISE( ( SQLAllocHandle( SQL_HANDLE_STMT, gHDbc, &( gResourceManagerPtr->mHStmt ) ) != SQL_SUCCESS ),
                        ERR_ALLOC_HANDLE_STMT );

        // Prepare the statement.
        ACI_TEST_RAISE( ( SQLPrepare( gResourceManagerPtr->mHStmt, (SQLCHAR *)gResourceManagerPtr->mQuery, SQL_NTS ) != SQL_SUCCESS ),
                        ERR_PREPARE_STMT );
    }

    // Execute the statement.
    ACI_TEST_RAISE( ( SQLExecute( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_EXECUTE_STMT );

    // Bind a column.
    ACI_TEST_RAISE( ( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_ULONG, &sLockWaitSessionCount, ACI_SIZEOF( sLockWaitSessionCount ), &sIndicator )
                      != SQL_SUCCESS ), ERR_BIND_COLUMN );

    // Fetch a row.
    ACI_TEST_RAISE( ( SQLFetch( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_FETCH_RESULT );

    // Close the cursor.
    ACI_TEST_RAISE( ( SQLCloseCursor( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_CLOSE_CURSOR );

    return sLockWaitSessionCount;

    ACI_EXCEPTION( ERR_ALLOC_HANDLE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_HANDLE_STMT );
    }
    ACI_EXCEPTION( ERR_PREPARE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_PREPARE_STMT );
    }
    ACI_EXCEPTION( ERR_EXECUTE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_EXECUTE_STMT );
    }
    ACI_EXCEPTION( ERR_BIND_COLUMN );
    {
        ULM_SET_ERR_CODE( ULM_ERR_BIND_COLUMN );
    }
    ACI_EXCEPTION( ERR_FETCH_RESULT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_FETCH_RESULT );
    }
    ACI_EXCEPTION( ERR_CLOSE_CURSOR );
    {
        ULM_SET_ERR_CODE( ULM_ERR_CLOSE_CURSOR );
    }
    ACI_EXCEPTION_END;

    // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
    ulmRecordErrorLog( "ABIGetLockWaitSessionCount" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

/* BUG-40397 The API for replication should be implemented at altiMonitor */
int ABIGetRepGap( ABIRepGap **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_REP_GAP );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmRepGap ), ACI_SIZEOF( ABIRepGap ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIRepGap ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIRepGap *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    ulmRecordErrorLog( "ABIGetRepGap" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

/* BUG-40397 The API for replication should be implemented at altiMonitor */
int ABIGetRepSentLogCount( ABIRepSentLogCount **aHandle )
{
    acp_uint32_t sRowCount = 0;

    ULM_INIT( ULM_REP_SENT_LOG_COUNT );

    if( gResourceManagerPtr->mHStmt == SQL_NULL_HSTMT )
    {
        ACI_TEST( ulmPrepare( ACI_SIZEOF( ulmRepSentLogCount ), ACI_SIZEOF( ABIRepSentLogCount ), ULM_EXTENT_SIZE ) != ACI_SUCCESS );
    }

    ACI_TEST( ulmExecute( ACI_SIZEOF( ABIRepSentLogCount ), &sRowCount ) != ACI_SUCCESS );

    *aHandle = (ABIRepSentLogCount *)gResourceManagerPtr->mResultArray;
    
    return sRowCount;

    ACI_EXCEPTION_END;

    ulmRecordErrorLog( "ABIGetRepSentLogCount" );

    ULM_FREE_HSTMT( gResourceManagerPtr->mHStmt );

    return gErrCode;
}

void ABIGetErrorMessage( int aErrCode, const char **aErrMsg )
{
    static acp_char_t *sErrMsg[] =
    {
        // BUG-33946 The maxgauge library should provide the API that sets the user and the password.
        "ERROR : Invalid socket.",
        "ERROR : Property value too long.",
        "ERROR : Failed to open log file",
        "ERROR : Failed to allocate environment handle.",
        "ERROR : Failed to allocate database connection handle.",
        "ERROR : Failed to connect to the Altibase server.",
        /* ulmPrepare */
        "ERROR : Failed to allocate statement handle.",
        "ERROR : Failed to set required statement attributes.",
        "ERROR : Failed to prepare statement.",
        "ERROR : Failed to allocate heap memory.",
        /* End */
        "ERROR : Failed to bind parameter in SQL statement.",
        /* ulmExecute */
        "ERROR : Statement execution failed.",
        "ERROR : Failed to bind columns.",
        "ERROR : Failed to fetch rows.",
        "ERROR : Failed to close cursor.",
        /* End */
        "ERROR : Could not free statement handle.",
        "ERROR : Failed to disconnect from the Altibase server.",
        "ERROR : Could not free database handle.",
        "ERROR : Could not free environment handle.",
        "ERROR : Failed to close log file."
    };
    acp_sint32_t sIdx = ~(acp_sint32_t)ULM_INIT_ERR_CODE;

    sIdx += aErrCode;

    *aErrMsg = sErrMsg[sIdx];
}
