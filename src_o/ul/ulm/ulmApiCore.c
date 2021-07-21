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

#define ULM_ALLOC_BIND_MEM( aBindRowSize, aCount );                                                 \
        {                                                                                           \
            acp_rc_t sRc;                                                                           \
            sRc = acpMemAlloc( &( gResourceManagerPtr->mBindArray ), aBindRowSize * ( aCount ) );   \
            if( sRc == ACP_RC_ENOMEM )                                                              \
            {                                                                                       \
                acpMemFree( gResourceManagerPtr->mBindArray );                                      \
                goto ERR_ALLOC_MEM;                                                                 \
            }                                                                                       \
        }

#define ULM_REALLOC_RESULT_MEM( aResultRowSize, aCount );                                               \
        {                                                                                               \
            acp_rc_t sRc;                                                                               \
            sRc = acpMemRealloc( &( gResourceManagerPtr->mResultArray ), aResultRowSize * ( aCount ) ); \
            if( sRc == ACP_RC_ENOMEM )                                                                  \
            {                                                                                           \
                acpMemFree( gResourceManagerPtr->mResultArray );                                        \
                gResourceManagerPtr->mResultArraySize = 0;                                              \
                goto ERR_ALLOC_MEM;                                                                     \
            }                                                                                           \
            else                                                                                        \
            {                                                                                           \
                gResourceManagerPtr->mResultArraySize = aCount;                                         \
            }                                                                                           \
        }


/*******************************************
 *     Declaration of global variables     *
 *******************************************/

SQLHANDLE           gHEnv = SQL_NULL_HENV;
SQLHANDLE           gHDbc = SQL_NULL_HDBC;

ulmResourceManager  gResourceManager[ULM_MAX_QUERY_COUNT] =
{
    // ULM_V_SESSION
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT * "
      "FROM V$SESSION",
      ulmBindColOfVSession, ulmCopyOfVSession
    },

    // ULM_V_SESSION_EXECUTING_ONLY 
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT * "
      "FROM V$SESSION "
      "WHERE TASK_STATE = \'EXECUTING\'",
      ulmBindColOfVSession, ulmCopyOfVSession
    },

    // ULM_V_SESSION_BY_SID
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT * "
      "FROM V$SESSION "
      "WHERE ID = ?",
      ulmBindColOfVSession, ulmCopyOfVSession
    },

    // ULM_V_SYSSTAT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT VALUE "
      "FROM V$SYSSTAT "
      "ORDER BY SEQNUM",
      ulmBindColOfVSysstat, ulmCopyOfVSysstat
    },

    // ULM_V_SESSTAT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SID, VALUE "
      "FROM V$SESSTAT "
      "ORDER BY SID, SEQNUM",
      ulmBindColOfVSesstat, ulmCopyOfVSesstat
    },

    // BUG-34728: cpu is too high
    // cpu를 낮추기 위해 join query 새로 추가.
    // v$view 보다 x$table을 직접 조회하는 것이 cpu 적게 사용
    // ULM_V_SESSTAT_EXECUTING_ONLY
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT B.SID, B.VALUE "
      "FROM X$SESSION A, X$SESSTAT B "
      "WHERE A.TASK_STATE = 2 AND A.ID = B.SID "
      "ORDER BY B.SID, B.SEQNUM",
      ulmBindColOfVSesstat, ulmCopyOfVSesstat
    },

    // ULM_V_SESSTAT_BY_SID
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SID, VALUE "
      "FROM V$SESSTAT "
      "WHERE SID = ? "
      "ORDER BY SEQNUM",
      ulmBindColOfVSesstat, ulmCopyOfVSesstat
    },

    // ULM_STAT_NAME
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SEQNUM, NAME "
      "FROM V$STATNAME "
      "ORDER BY SEQNUM",
      ulmBindColOfStatName, ulmCopyOfStatName
    },

    // ULM_V_SYSTEM_EVENT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT TOTAL_WAITS, TOTAL_TIMEOUTS, TIME_WAITED, AVERAGE_WAIT, TIME_WAITED_MICRO "
      "FROM V$SYSTEM_EVENT "
      "ORDER BY EVENT_ID",
      ulmBindColOfVSystemEvent, ulmCopyOfVSystemEvent
    },

    // ULM_V_SESSION_EVENT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SID, TOTAL_WAITS, TOTAL_TIMEOUTS, TIME_WAITED, AVERAGE_WAIT, MAX_WAIT, TIME_WAITED_MICRO "
      "FROM V$SESSION_EVENT "
      "ORDER BY SID, EVENT_ID",
      ulmBindColOfVSessionEvent, ulmCopyOfVSessionEvent
    },

    // ULM_V_SESSION_EVENT_BY_SID
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SID, TOTAL_WAITS, TOTAL_TIMEOUTS, TIME_WAITED, AVERAGE_WAIT, MAX_WAIT, TIME_WAITED_MICRO "
      "FROM V$SESSION_EVENT "
      "WHERE SID = ? "
      "ORDER BY EVENT_ID",
      ulmBindColOfVSessionEvent, ulmCopyOfVSessionEvent
    },

    // ULM_EVENT_NAME
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT EVENT_ID, NAME, WAIT_CLASS_ID, WAIT_CLASS "
      "FROM V$EVENT_NAME "
      /* BUG-39222 이 함수  결과 중 no wait event 제외는 초기 개발당시 협의사항 */
      "WHERE NAME!='no wait event' "
      "ORDER BY EVENT_ID",
      ulmBindColOfEventName, ulmCopyOfEventName
    },

    // ULM_V_SESSION_WAIT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SID, SEQNUM, P1, P2, P3, WAIT_CLASS_ID, WAIT_TIME, SECOND_IN_TIME "
      "FROM V$SESSION_WAIT "
      "ORDER BY SID, SEQNUM",
      ulmBindColOfVSessionWait, ulmCopyOfVSessionWait
    },

    // ULM_V_SESSION_WAIT_BY_SID
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SID, SEQNUM, P1, P2, P3, WAIT_CLASS_ID, WAIT_TIME, SECOND_IN_TIME "
      "FROM V$SESSION_WAIT "
      "WHERE SID = ? "
      "ORDER BY SEQNUM",
      ulmBindColOfVSessionWait, ulmCopyOfVSessionWait
    },

    // BUG-34728: cpu is too high
    // v$sqltext 조회를 x$statement join query로 변경
    // active 세션들의 current sql text 들을 조회
    // ULM_SQL_TEXT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT A.ID, B.ID, B.QUERY, B.QUERY_START_TIME, B.EXECUTE_FLAG, B.SQL_CACHE_TEXT_ID, "
      "B.PARSE_TIME, B.SOFT_PREPARE_TIME, B.LAST_QUERY_START_TIME, B.EXECUTE_TIME, B.FETCH_TIME, "
      "B.FETCH_START_TIME, B.TOTAL_TIME, B.VALIDATE_TIME, B.OPTIMIZE_TIME "
      "FROM X$SESSION A, X$STATEMENT B "
      "WHERE A.TASK_STATE = 2 AND A.ID = B.SESSION_ID "
      "AND A.CURRENT_STMT_ID = B.ID",
      ulmBindColOfSqlText, ulmCopyOfSqlText
    },

    // BUG-34728: cpu is too high
    // 기존의 단일 sql text 조회 기능을 유지하기 위해 새로 추가
    // ULM_SQL_TEXT_BY_STMT_ID
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT SESSION_ID, ID, QUERY, QUERY_START_TIME, EXECUTE_FLAG, SQL_CACHE_TEXT_ID, "
      "PARSE_TIME, SOFT_PREPARE_TIME, LAST_QUERY_START_TIME, EXECUTE_TIME, FETCH_TIME, "
      "FETCH_START_TIME, TOTAL_TIME, VALIDATE_TIME, OPTIMIZE_TIME "
      "FROM X$STATEMENT "
      "WHERE ID = ?",
      ulmBindColOfSqlText, ulmCopyOfSqlText
    },

    // ULM_LOCK_PAIR_BETWEEN_SESSIONS
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT DISTINCT WAITER.SESSION_ID, HOLDER.SESSION_ID, HOLDER.LOCK_DESC "
      "FROM "
      "( "
           "SELECT A.TRANS_ID, A.WAIT_FOR_TRANS_ID, B.SESSION_ID "
           "FROM V$LOCK_WAIT A, V$TRANSACTION B "
           "WHERE A.TRANS_ID = B.ID "
      ") WAITER, "
      "( "
           "SELECT A.TRANS_ID, A.WAIT_FOR_TRANS_ID, B.SESSION_ID, C.LOCK_DESC "
           "FROM V$LOCK_WAIT A, V$TRANSACTION B, V$LOCK C "
           "WHERE A.WAIT_FOR_TRANS_ID = B.ID AND A.WAIT_FOR_TRANS_ID = C.TRANS_ID "
      ") HOLDER "
      "WHERE WAITER.WAIT_FOR_TRANS_ID = HOLDER.WAIT_FOR_TRANS_ID "
      "ORDER BY 1, 2",
      ulmBindColOfLockPair, ulmCopyOfLockPair
    },

    // ULM_DB_INFO
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT DB_NAME, PRODUCT_VERSION "
      "FROM V$DATABASE, V$VERSION",
      ulmBindColOfDBInfo, ulmCopyOfDBInfo
    },

    // ULM_READ_COUNT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      // Logical read count : FIX_PAGES+GET_PAGES-READ_PAGES,
      // Physical read count : READ_PAGES
      "SELECT FIX_PAGES+GET_PAGES-READ_PAGES, READ_PAGES "
      "FROM V$BUFFPOOL_STAT",
      ulmBindColOfReadCount, ulmCopyOfReadCount
    },

    // ULM_SESSION_COUNT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT TASK_COUNT "
      "FROM V$SESSIONMGR",
      NULL, NULL
    },

    // ULM_SESSION_COUNT_EXECUTING_ONLY
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT COUNT(*) "
      "FROM V$SESSION "
      "WHERE TASK_STATE = \'EXECUTING\'",
      NULL, NULL
    },

    // ULM_MAX_CLIENT_COUNT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT VALUE1 "
      "FROM V$PROPERTY "
      "WHERE NAME = \'MAX_CLIENT\'",
      NULL, NULL
    },

    // ULM_LOCK_WAIT_SESSION_COUNT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT COUNT( DISTINCT TRANS_ID ) "
      "FROM V$LOCK_WAIT",
      NULL, NULL
    },

    // ULM_REP_GAP
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT REP_NAME, REP_GAP "
      "FROM V$REPGAP",
      ulmBindColOfRepGap, ulmCopyOfRepGap
    },

    // ULM_REP_SENT_LOG_COUNT
    { SQL_NULL_HSTMT, NULL, NULL, 0,
      "SELECT T1.REP_NAME, T2.TABLE_NAME, T1.INSERT_LOG_COUNT, T1.DELETE_LOG_COUNT, T1.UPDATE_LOG_COUNT "
      "FROM V$REPSENDER_SENT_LOG_COUNT T1, SYSTEM_.SYS_TABLES_ T2 "
      "WHERE T1.TABLE_OID = T2.TABLE_OID "
      "ORDER BY 1, 2", 
      ulmBindColOfRepSentLogCount, ulmCopyOfRepSentLogCount
    }
};

ulmResourceManager *gResourceManagerPtr = NULL;

acp_uint32_t        gFetchedRowCount = 0;

ulmErrCode          gErrCode = ULM_INIT_ERR_CODE;

// BUG-33946 The maxgauge library should provide the API that sets the user and the password.
ulmProperties       gProperties[] =
{
    // ABI_USER
    { "SYS" },

    // ABI_PASSWD
    { "MANAGER" },

    // ABI_LOGFILE
    { "./altibaseMonitor.log" }
};

// BUG-33946 The maxgauge library should provide the API that sets the user and the password.
acp_std_file_t      gLogFile;


/********************************************
 *     Implementation of core functions     *
 ********************************************/

acp_rc_t ulmPrepare( acp_size_t   aBindRowSize,
                     acp_size_t   aResultRowSize,
                     acp_uint32_t aCount )
{
    // Allocate a statement handle.
    ACI_TEST_RAISE( ( SQLAllocHandle( SQL_HANDLE_STMT, gHDbc, &( gResourceManagerPtr->mHStmt ) ) != SQL_SUCCESS ),
                    ERR_ALLOC_HANDLE_STMT );

    // Set the required statement attributes.
    ACI_TEST_RAISE( ( SQLSetStmtAttr( gResourceManagerPtr->mHStmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)aBindRowSize, 0 )
                      != SQL_SUCCESS ), ERR_SET_STMT_ATTR );
    ACI_TEST_RAISE( ( SQLSetStmtAttr( gResourceManagerPtr->mHStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)(acp_size_t)aCount, 1 )
                      != SQL_SUCCESS ), ERR_SET_STMT_ATTR );
    ACI_TEST_RAISE( ( SQLSetStmtAttr( gResourceManagerPtr->mHStmt, SQL_ATTR_ROWS_FETCHED_PTR, (SQLPOINTER)&gFetchedRowCount, 0 )
                      != SQL_SUCCESS ), ERR_SET_STMT_ATTR );

    // Prepare the statement.
    ACI_TEST_RAISE( ( SQLPrepare( gResourceManagerPtr->mHStmt, (SQLCHAR *)gResourceManagerPtr->mQuery, SQL_NTS ) != SQL_SUCCESS ),
                    ERR_PREPARE_STMT );

    ULM_ALLOC_BIND_MEM( aBindRowSize, aCount );
    ULM_REALLOC_RESULT_MEM( aResultRowSize, aCount );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_ALLOC_HANDLE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_HANDLE_STMT );
    }
    ACI_EXCEPTION( ERR_SET_STMT_ATTR );
    {
        ULM_SET_ERR_CODE( ULM_ERR_SET_STMT_ATTR );
    }
    ACI_EXCEPTION( ERR_PREPARE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_PREPARE_STMT );
    }
    ACI_EXCEPTION( ERR_ALLOC_MEM );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_MEM );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmExecute( acp_size_t    aResultRowSize,
                     acp_uint32_t *aRowCount )
{
    SQLRETURN    sRet = SQL_SUCCESS;
    acp_uint32_t sRowCount = 0;
    acp_uint32_t sPos = 0;

    // Execute the statement.
    ACI_TEST_RAISE( ( SQLExecute( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_EXECUTE_STMT );

    // Bind the columns.
    ACI_TEST_RAISE( ( gResourceManagerPtr->mBindCol() != ACI_SUCCESS ), ERR_BIND_COLUMN );

    // Fetch the rows.
    while( ( sRet = SQLFetch( gResourceManagerPtr->mHStmt ) ) != SQL_NO_DATA )
    {
        ACI_TEST_RAISE( ( sRet != SQL_SUCCESS ), ERR_FETCH_RESULT );

        gResourceManagerPtr->mCopyBindToResult( sPos++ );

        sRowCount += gFetchedRowCount;

        gFetchedRowCount = 0;

        if( sRowCount == gResourceManagerPtr->mResultArraySize )
        {
            // BUG-34088  It should be fixed that the memory allocation is incorrect in the maxgauge library.
            ULM_REALLOC_RESULT_MEM( aResultRowSize, sRowCount + ULM_EXTENT_SIZE ); 
        }
    }

    // Close the cursor.
    ACI_TEST_RAISE( ( SQLCloseCursor( gResourceManagerPtr->mHStmt ) != SQL_SUCCESS ), ERR_CLOSE_CURSOR );
    
    *aRowCount = sRowCount;

    return ACI_SUCCESS;

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
    ACI_EXCEPTION( ERR_ALLOC_MEM );
    {
        ULM_SET_ERR_CODE( ULM_ERR_ALLOC_MEM );
    }
    ACI_EXCEPTION( ERR_CLOSE_CURSOR );
    {
        ULM_SET_ERR_CODE( ULM_ERR_CLOSE_CURSOR );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfVSession( void )
{
    ulmVSession  *sPtr = NULL;
    acp_uint32_t  sI = 1, sJ = 0;

    sPtr = (ulmVSession *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mID ), ACI_SIZEOF( sPtr->mID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SBIGINT, &( sPtr->mTransID ), ACI_SIZEOF( sPtr->mTransID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mTaskState, ACI_SIZEOF( sPtr->mTaskState ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mCommName, ACI_SIZEOF( sPtr->mCommName ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mXASessionFlag ), ACI_SIZEOF( sPtr->mXASessionFlag ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mXAAssociateFlag ), ACI_SIZEOF( sPtr->mXAAssociateFlag ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mQueryTimeLimit ), ACI_SIZEOF( sPtr->mQueryTimeLimit ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mDdlTimeLimit ), ACI_SIZEOF( sPtr->mDdlTimeLimit ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mFetchTimeLimit ), ACI_SIZEOF( sPtr->mFetchTimeLimit ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mUTransTimeLimit ), ACI_SIZEOF( sPtr->mUTransTimeLimit ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mIdleTimeLimit ), ACI_SIZEOF( sPtr->mIdleTimeLimit ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mIdleStartTime ), ACI_SIZEOF( sPtr->mIdleStartTime ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mActiveFlag ), ACI_SIZEOF( sPtr->mActiveFlag ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mOpenedStmtCount ), ACI_SIZEOF( sPtr->mOpenedStmtCount ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mClientPackageVersion, ACI_SIZEOF( sPtr->mClientPackageVersion ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mClientProtocolVersion, ACI_SIZEOF( sPtr->mClientProtocolVersion ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SBIGINT, &( sPtr->mClientPID ), ACI_SIZEOF( sPtr->mClientPID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mClientType, ACI_SIZEOF( sPtr->mClientType ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mClientAppInfo, ACI_SIZEOF( sPtr->mClientAppInfo ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mClientNls, ACI_SIZEOF( sPtr->mClientNls ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mDBUserName, ACI_SIZEOF( sPtr->mDBUserName ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mDBUserID ), ACI_SIZEOF( sPtr->mDBUserID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SBIGINT, &( sPtr->mDefaultTbsID ), ACI_SIZEOF( sPtr->mDefaultTbsID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SBIGINT, &( sPtr->mDefaultTempTbsID ), ACI_SIZEOF( sPtr->mDefaultTempTbsID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mSysDbaFlag ), ACI_SIZEOF( sPtr->mSysDbaFlag ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mAutoCommitFlag ), ACI_SIZEOF( sPtr->mAutoCommitFlag ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mSessionState, ACI_SIZEOF( sPtr->mSessionState ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mIsolationLevel ), ACI_SIZEOF( sPtr->mIsolationLevel ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mReplicationMode ), ACI_SIZEOF( sPtr->mReplicationMode ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mTransactionMode ), ACI_SIZEOF( sPtr->mTransactionMode ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mCommitWriteWaitMode ), ACI_SIZEOF( sPtr->mCommitWriteWaitMode ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mOptimizerMode ), ACI_SIZEOF( sPtr->mOptimizerMode ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mHeaderDisplayMode ), ACI_SIZEOF( sPtr->mHeaderDisplayMode ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mCurrentStmtID ), ACI_SIZEOF( sPtr->mCurrentStmtID ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mStackSize ), ACI_SIZEOF( sPtr->mStackSize ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mDefaultDateFormat, ACI_SIZEOF( sPtr->mDefaultDateFormat ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SBIGINT, &( sPtr->mTrxUpdateMaxLogSize ), ACI_SIZEOF( sPtr->mTrxUpdateMaxLogSize ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mParallelDmlMode ), ACI_SIZEOF( sPtr->mParallelDmlMode ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mLoginTime ), ACI_SIZEOF( sPtr->mLoginTime ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI, SQL_C_CHAR, sPtr->mFailoverSource, ACI_SIZEOF( sPtr->mFailoverSource ), &( sPtr->mLengthOrInd[sJ] ) )
              != SQL_SUCCESS );
 
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfVSysstat( void )
{
    ulmVSysstat *sPtr = NULL;

    sPtr = (ulmVSysstat *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SBIGINT, &( sPtr->mValue ), ACI_SIZEOF( sPtr->mValue ), &( sPtr->mLengthOrInd ) )
              != SQL_SUCCESS );
  
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfVSesstat( void )
{
    ulmVSesstat  *sPtr = NULL;
    acp_uint32_t  sI = 0;

    sPtr = (ulmVSesstat *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mSID ), ACI_SIZEOF( sPtr->mSID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SBIGINT, &( sPtr->mValue ), ACI_SIZEOF( sPtr->mValue ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );
  
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfStatName( void )
{
    ulmStatName  *sPtr = NULL;
    acp_uint32_t  sI = 0;

    sPtr = (ulmStatName *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mSeqNum ), ACI_SIZEOF( sPtr->mSeqNum ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_CHAR, sPtr->mName, ACI_SIZEOF( sPtr->mName ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfVSystemEvent( void )
{
    ulmVSystemEvent *sPtr = NULL;
    acp_uint32_t     sI = 0;

    sPtr = (ulmVSystemEvent *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SBIGINT, &( sPtr->mTotalWaits ), ACI_SIZEOF( sPtr->mTotalWaits ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SBIGINT, &( sPtr->mTotalTimeOuts ), ACI_SIZEOF( sPtr->mTotalTimeOuts ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 3, SQL_C_SBIGINT, &( sPtr->mTimeWaited ), ACI_SIZEOF( sPtr->mTimeWaited ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 4, SQL_C_SBIGINT, &( sPtr->mAverageWait ), ACI_SIZEOF( sPtr->mAverageWait ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 5, SQL_C_SBIGINT, &( sPtr->mTimeWaitedMicro ), ACI_SIZEOF( sPtr->mTimeWaitedMicro ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );
  
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfVSessionEvent( void )
{
    ulmVSessionEvent *sPtr = NULL;
    acp_uint32_t      sI = 0;

    sPtr = (ulmVSessionEvent *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mSID ), ACI_SIZEOF( sPtr->mSID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SBIGINT, &( sPtr->mTotalWaits ), ACI_SIZEOF( sPtr->mTotalWaits ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 3, SQL_C_SBIGINT, &( sPtr->mTotalTimeOuts ), ACI_SIZEOF( sPtr->mTotalTimeOuts ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 4, SQL_C_SBIGINT, &( sPtr->mTimeWaited ), ACI_SIZEOF( sPtr->mTimeWaited ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 5, SQL_C_SBIGINT, &( sPtr->mAverageWait ), ACI_SIZEOF( sPtr->mAverageWait ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 6, SQL_C_SBIGINT, &( sPtr->mMaxWait ), ACI_SIZEOF( sPtr->mMaxWait ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 7, SQL_C_SBIGINT, &( sPtr->mTimeWaitedMicro ), ACI_SIZEOF( sPtr->mTimeWaitedMicro ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfEventName( void )
{
    ulmEventName *sPtr = NULL;
    acp_uint32_t  sI = 0;

    sPtr = (ulmEventName *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mEventID ), ACI_SIZEOF( sPtr->mEventID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_CHAR, sPtr->mEvent, ACI_SIZEOF( sPtr->mEvent ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 3, SQL_C_SLONG, &( sPtr->mWaitClassID ), ACI_SIZEOF( sPtr->mWaitClassID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 4, SQL_C_CHAR, sPtr->mWaitClass, ACI_SIZEOF( sPtr->mWaitClass ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfVSessionWait( void )
{
    ulmVSessionWait *sPtr = NULL;
    acp_uint32_t     sI = 0;

    sPtr = (ulmVSessionWait *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mSID ), ACI_SIZEOF( sPtr->mSID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SLONG, &( sPtr->mSeqNum ), ACI_SIZEOF( sPtr->mSeqNum ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 3, SQL_C_SBIGINT, &( sPtr->mP1 ), ACI_SIZEOF( sPtr->mP1 ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 4, SQL_C_SBIGINT, &( sPtr->mP2 ), ACI_SIZEOF( sPtr->mP2 ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 5, SQL_C_SBIGINT, &( sPtr->mP3 ), ACI_SIZEOF( sPtr->mP3 ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 6, SQL_C_SLONG, &( sPtr->mWaitClassID ), ACI_SIZEOF( sPtr->mWaitClassID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 7, SQL_C_SBIGINT, &( sPtr->mWaitTime ), ACI_SIZEOF( sPtr->mWaitTime ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 8, SQL_C_SBIGINT, &( sPtr->mSecondInTime ), ACI_SIZEOF( sPtr->mSecondInTime ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );
    
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfSqlText( void )
{
    ulmSqlText *sPtr = NULL;

    sPtr = (ulmSqlText *)gResourceManagerPtr->mBindArray;

    /* BUG-41825: 필드 추가, Length 받는 필드 조절 */

    // BUG-34728: cpu is too high
    // 기존에는 sqltext만 조회했는데, 수정후 sessionID, stmtID, sqltext를 조회한다
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &sPtr->mSessID, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SLONG, &sPtr->mStmtID, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 3, SQL_C_CHAR, sPtr->mSqlText,
                          ACI_SIZEOF( sPtr->mSqlText ), &( sPtr->mSqlTextLength ) )
              != SQL_SUCCESS );
	
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 4, SQL_C_ULONG, &sPtr->mQueryStartTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 5, SQL_C_ULONG, &sPtr->mExecuteFlag, 0, NULL )
              != SQL_SUCCESS );

    /* BUG-45924 */
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 6, SQL_C_CHAR, sPtr->mSqlCacheTextID,
                          ACI_SIZEOF( sPtr->mSqlCacheTextID ), &( sPtr->mLengthOrInd[0] ) )
              != SQL_SUCCESS );

    /* BUG-46436 */
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 7, SQL_C_UBIGINT, &sPtr->mParseTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 8, SQL_C_UBIGINT, &sPtr->mSoftPrepareTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 9, SQL_C_ULONG, &sPtr->mLastQueryStartTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 10, SQL_C_UBIGINT, &sPtr->mExecuteTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 11, SQL_C_UBIGINT, &sPtr->mFetchTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 12, SQL_C_ULONG, &sPtr->mFetchStartTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 13, SQL_C_UBIGINT, &sPtr->mTotalTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 14, SQL_C_UBIGINT, &sPtr->mValidateTime, 0, NULL )
              != SQL_SUCCESS );

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 15, SQL_C_UBIGINT, &sPtr->mOptimizeTime, 0, NULL )
              != SQL_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfLockPair( void )
{
    ulmLockPair  *sPtr = NULL;
    acp_uint32_t  sI = 0;

    sPtr = (ulmLockPair *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mWaiterSID ), ACI_SIZEOF( sPtr->mWaiterSID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SLONG, &( sPtr->mHolderSID ), ACI_SIZEOF( sPtr->mHolderSID ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 3, SQL_C_CHAR, sPtr->mLockDesc, ACI_SIZEOF( sPtr->mLockDesc ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfDBInfo( void )
{
    ulmDBInfo    *sPtr = NULL;
    acp_uint32_t  sI = 0;

    sPtr = (ulmDBInfo *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_CHAR, sPtr->mDBName, ACI_SIZEOF( sPtr->mDBName ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_CHAR, sPtr->mDBVersion, ACI_SIZEOF( sPtr->mDBVersion ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfReadCount( void )
{
    ulmReadCount *sPtr = NULL;
    acp_uint32_t  sI = 0;

    sPtr = (ulmReadCount *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 1, SQL_C_SLONG, &( sPtr->mLogicalReadCount ), ACI_SIZEOF( sPtr->mLogicalReadCount ), &( sPtr->mLengthOrInd[sI++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, 2, SQL_C_SLONG, &( sPtr->mPhysicalReadCount ), ACI_SIZEOF( sPtr->mPhysicalReadCount ), &( sPtr->mLengthOrInd[sI] ) )
              != SQL_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* BUG-40397 The API for replication should be implemented at altiMonitor */
acp_rc_t ulmBindColOfRepGap( void )
{
    ulmRepGap    *sPtr = NULL;
    acp_uint32_t  sI = 1, sJ = 0;

    sPtr = (ulmRepGap *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mRepName, ACI_SIZEOF( sPtr->mRepName ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SBIGINT, &( sPtr->mRepGap ), ACI_SIZEOF( sPtr->mRepGap ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
 
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_rc_t ulmBindColOfRepSentLogCount( void )
{
    ulmRepSentLogCount *sPtr = NULL;
    acp_uint32_t        sI = 1, sJ = 0;

    sPtr = (ulmRepSentLogCount *)gResourceManagerPtr->mBindArray;

    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mRepName, ACI_SIZEOF( sPtr->mRepName ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_CHAR, sPtr->mTableName, ACI_SIZEOF( sPtr->mTableName ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mInsertLogCount ), ACI_SIZEOF( sPtr->mInsertLogCount ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mDeleteLogCount ), ACI_SIZEOF( sPtr->mDeleteLogCount ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
    ACI_TEST( SQLBindCol( gResourceManagerPtr->mHStmt, sI++, SQL_C_SLONG, &( sPtr->mUpdateLogCount ), ACI_SIZEOF( sPtr->mUpdateLogCount ), &( sPtr->mLengthOrInd[sJ++] ) )
              != SQL_SUCCESS );
 
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulmCopyOfVSession( acp_uint32_t aPos )
{
    ulmVSession  *sSrc = (ulmVSession *)gResourceManagerPtr->mBindArray;
    ABIVSession  *sDest = ( (ABIVSession *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABIVSession );
    acp_uint32_t  sI;

    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        // For CLIENT_APP_INFO
        if ( sSrc->mLengthOrInd[18] == SQL_NULL_DATA )
        {
            sDest->mClientAppInfo[0] = 0x00;
        }
        // For FAILOVER_SOURCE
        if ( sSrc->mLengthOrInd[39] == SQL_NULL_DATA )
        {
            sDest->mFailoverSource[0] = 0x00;
        }

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfVSysstat( acp_uint32_t aPos )
{
    ulmVSysstat  *sSrc = (ulmVSysstat *)gResourceManagerPtr->mBindArray;
    ABIVSysstat  *sDest = ( (ABIVSysstat *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABIVSysstat );
    acp_uint32_t  sI;

    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfVSesstat( acp_uint32_t aPos )
{
    ulmVSesstat  *sSrc = (ulmVSesstat *)gResourceManagerPtr->mBindArray;
    ABIVSesstat  *sDest = ( (ABIVSesstat *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABIVSesstat );
    acp_uint32_t  sI;

    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfStatName( acp_uint32_t aPos )
{
    ulmStatName  *sSrc = (ulmStatName *)gResourceManagerPtr->mBindArray;
    ABIStatName  *sDest = ( (ABIStatName *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABIStatName );
    acp_uint32_t  sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfVSystemEvent( acp_uint32_t aPos )
{
    ulmVSystemEvent *sSrc = (ulmVSystemEvent *)gResourceManagerPtr->mBindArray;
    ABIVSystemEvent *sDest = ( (ABIVSystemEvent *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t     sSize = ACI_SIZEOF( ABIVSystemEvent );
    acp_uint32_t     sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfVSessionEvent( acp_uint32_t aPos )
{
    ulmVSessionEvent *sSrc = (ulmVSessionEvent *)gResourceManagerPtr->mBindArray;
    ABIVSessionEvent *sDest = ( (ABIVSessionEvent *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos ); 
    acp_uint32_t      sSize = ACI_SIZEOF( ABIVSessionEvent );
    acp_uint32_t      sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfEventName( acp_uint32_t aPos )
{
    ulmEventName *sSrc = (ulmEventName *)gResourceManagerPtr->mBindArray;
    ABIEventName *sDest = ( (ABIEventName *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos ); 
    acp_uint32_t  sSize = ACI_SIZEOF( ABIEventName );
    acp_uint32_t  sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfVSessionWait( acp_uint32_t aPos )
{
    ulmVSessionWait *sSrc = (ulmVSessionWait *)gResourceManagerPtr->mBindArray;
    ABIVSessionWait *sDest = ( (ABIVSessionWait *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos ); 
    acp_uint32_t     sSize = ACI_SIZEOF( ABIVSessionWait );
    acp_uint32_t     sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfSqlText( acp_uint32_t aPos )
{  
    ulmSqlText   *sSrc = (ulmSqlText *)gResourceManagerPtr->mBindArray;
    ABISqlText   *sDest = ( (ABISqlText *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABISqlText );
    acp_uint32_t  sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        // BUG-34728: cpu is too high
        // indicator[2] 는 3번째 조회컬럼, 즉 query의 null 여부확인
        if( sSrc->mSqlTextLength != SQL_NULL_DATA ) /* BUG-41825 */
        {
            acpMemCpy( sDest, sSrc, sSize );

            /* BUG-45924 */
            if (sSrc->mLengthOrInd[0] == SQL_NULL_DATA)
            {
                sDest->mSqlCacheTextID[0] = '\0';
            }
        }
        // if query is null. null인 경우가 있는지는 모르겠음
        else
        {
            sDest->mSessID = sSrc->mSessID;
            sDest->mStmtID = sSrc->mStmtID;
            sDest->mSqlText[0] = 0;
            sDest->mTextLength = 0;
            /* BUG-41825 */
            sDest->mQueryStartTime = sSrc->mQueryStartTime;
            sDest->mExecuteFlag = sSrc->mExecuteFlag;
            sDest->mSqlCacheTextID[0] = '\0';
            /* BUG-46436 */
            sDest->mParseTime = sSrc->mParseTime;
            sDest->mSoftPrepareTime = sSrc->mSoftPrepareTime;
            sDest->mLastQueryStartTime = sSrc->mLastQueryStartTime;
            sDest->mExecuteTime = sSrc->mExecuteTime;
            sDest->mFetchTime = sSrc->mFetchTime;
            sDest->mFetchStartTime = sSrc->mFetchStartTime;
            sDest->mTotalTime = sSrc->mTotalTime;
            sDest->mValidateTime = sSrc->mValidateTime;
            sDest->mOptimizeTime = sSrc->mOptimizeTime;
        }

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfLockPair( acp_uint32_t aPos )
{  
    ulmLockPair  *sSrc = (ulmLockPair *)gResourceManagerPtr->mBindArray;
    ABILockPair  *sDest = ( (ABILockPair *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABILockPair );
    acp_uint32_t  sI;
    
    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfDBInfo( acp_uint32_t aPos /* Not used */ )
{  
    ulmDBInfo    *sSrc = (ulmDBInfo *)gResourceManagerPtr->mBindArray;
    ABIDBInfo    *sDest = (ABIDBInfo *)gResourceManagerPtr->mResultArray;
    acp_uint32_t  sSize = ACI_SIZEOF( ABIDBInfo );

    ACP_UNUSED( aPos );

    acpMemCpy( sDest, sSrc, sSize );
}

void ulmCopyOfReadCount( acp_uint32_t aPos /* Not used */ )
{  
    ulmReadCount *sSrc = (ulmReadCount *)gResourceManagerPtr->mBindArray;
    ABIReadCount *sDest = (ABIReadCount *)gResourceManagerPtr->mResultArray;
    acp_uint32_t  sSize = ACI_SIZEOF( ABIReadCount );

    ACP_UNUSED( aPos );

    acpMemCpy( sDest, sSrc, sSize );
}

/* BUG-40397 The API for replication should be implemented at altiMonitor */
void ulmCopyOfRepGap( acp_uint32_t aPos )
{
    ulmRepGap    *sSrc = (ulmRepGap *)gResourceManagerPtr->mBindArray;
    ABIRepGap    *sDest = ( (ABIRepGap *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t  sSize = ACI_SIZEOF( ABIRepGap );
    acp_uint32_t  sI;

    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

void ulmCopyOfRepSentLogCount( acp_uint32_t aPos )
{
    ulmRepSentLogCount *sSrc = (ulmRepSentLogCount *)gResourceManagerPtr->mBindArray;
    ABIRepSentLogCount *sDest = ( (ABIRepSentLogCount *)gResourceManagerPtr->mResultArray ) + ( ULM_EXTENT_SIZE * aPos );
    acp_uint32_t        sSize = ACI_SIZEOF( ABIRepSentLogCount );
    acp_uint32_t        sI;

    for( sI = 0; sI < gFetchedRowCount; sI++ )
    {
        acpMemCpy( sDest, sSrc, sSize );

        sSrc++;
        sDest++;
    }
}

acp_rc_t ulmCleanUpResource( void )
{
    acp_uint32_t sI;

    for( sI = 0; sI < ULM_MAX_QUERY_COUNT; sI++ )
    {
        if( gResourceManager[sI].mHStmt != SQL_NULL_HSTMT )
        {
            // Free the statement handle.
            ACI_TEST_RAISE( ( SQLFreeHandle( SQL_HANDLE_STMT, gResourceManager[sI].mHStmt ) != SQL_SUCCESS ), ERR_FREE_HANDLE_STMT );
            gResourceManager[sI].mHStmt = SQL_NULL_HSTMT;

            // Free the heap memory.
            acpMemFree( gResourceManager[sI].mBindArray );
            gResourceManager[sI].mBindArray = NULL;

            acpMemFree( gResourceManager[sI].mResultArray );
            gResourceManager[sI].mResultArray = NULL;
            gResourceManager[sI].mResultArraySize = 0;
        }
    }

    gResourceManagerPtr = NULL;

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_FREE_HANDLE_STMT );
    {
        ULM_SET_ERR_CODE( ULM_ERR_FREE_HANDLE_STMT );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// BUG-33946 The maxgauge library should provide the API that sets the user and the password.
void ulmRecordErrorLog( const acp_char_t *aFunctionName )
{
    SQLSMALLINT sHandleType;
    SQLHANDLE   sHandle;
    SQLSMALLINT sRecordNo = 1;
    SQLCHAR     sSQLState[6];
    SQLINTEGER  sNativeError;
    SQLCHAR     sMessage[ULM_SIZE_2048];
    SQLSMALLINT sMessageLength;
    SQLRETURN   sRc;
    acp_char_t  sErrorLog[ULM_SIZE_4096];
    acp_size_t  sWritten;

    ACI_TEST( gErrCode == ULM_ERR_OPEN_LOGFILE || gErrCode == ULM_ERR_CLOSE_LOGFILE );

    if( gResourceManagerPtr != NULL )
    {
        sHandleType = SQL_HANDLE_STMT;
        sHandle = gResourceManagerPtr->mHStmt;
    }
    else if( gHDbc != SQL_NULL_HDBC )
    {
        sHandleType = SQL_HANDLE_DBC;
        sHandle = gHDbc;
    }
    else
    {
        sHandleType = SQL_HANDLE_DBC;
        sHandle = gHEnv;
    }

    acpSnprintf( sErrorLog, ACI_SIZEOF( sErrorLog ), "Error : %s\n", aFunctionName );
    acpStdPutCString( &gLogFile, sErrorLog, ACI_SIZEOF( sErrorLog ), &sWritten );

    while( ( sRc = SQLGetDiagRec( sHandleType,
                                  sHandle,
                                  sRecordNo,
                                  sSQLState,
                                  &sNativeError,
                                  sMessage,
                                  ACI_SIZEOF( sMessage ),
                                  &sMessageLength ) ) != SQL_NO_DATA )
    {
        if( sRc != SQL_SUCCESS && sRc != SQL_SUCCESS_WITH_INFO )
        {
            break;
        }

        acpSnprintf( sErrorLog,
                     ACI_SIZEOF( sErrorLog ),
                     "Diagnostic Record %d\n     SQLState     : %s\n     Message      : %s\n     Native error : 0x%X\n\n",
                     sRecordNo,
                     sSQLState,
                     sMessage,
                     sNativeError );

        acpStdPutCString( &gLogFile, sErrorLog, ACI_SIZEOF( sErrorLog ), &sWritten );

        sRecordNo ++;
    }

    ACI_EXCEPTION_END;
}
