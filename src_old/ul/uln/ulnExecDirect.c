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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnExecDirect.h>

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>
#include <ulnSemiAsyncPrefetch.h>

/*
 * ULN_SFID_16
 * SQLExecDirect(), STMT, S1
 *      S4 [s] and [nr]
 *      S5 [s] and [r]
 *      S8 [d]
 *      S11 [x]
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 */
ACI_RC ulnSFID_16(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        switch(ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            case SQL_NO_DATA:
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                if(ulnStateCheckR(aFnContext) == ACP_TRUE)
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                    ulnCursorOpen(&sStmt->mCursor);
                }
                else
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
                }
                break;
            case SQL_NEED_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                break;
            default:
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_17
 * SQLExecDirect(), STMT, S2-S3
 *      -- [e] and [1]
 *      S1 [e] and [2]
 *      S4 [s] and [nr]
 *      S5 [s] and [r]
 *      S8 [d]
 *      S11 [x]
 *  where
 *      [1]  The error was returned by the Driver Manager.
 *      [2]  The error was not returned by the Driver Manager.
 *
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [e]  Error. The function returned SQL_ERROR.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 *
 *      BUGBUG: Since we do support Driver Manager's functionality, [2] is always true.???
 */
ACI_RC ulnSFID_17(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if(aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        switch(ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            case SQL_ERROR:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                break;

            case SQL_NO_DATA:
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                if(ulnStateCheckR(aFnContext) == ACP_TRUE)
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                    ulnCursorOpen(&sStmt->mCursor);
                }
                else
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
                }
                break;
            case SQL_NEED_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                break;
            default:
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SF_18
 * SQLExecDirect(), STMT, S4
 *      -- [e], [1], and [3]
 *      S1 [e], [2], and [3]
 *      S4 [s], [nr], and [3]
 *      S5 [s], [r], and [3]
 *      S8 [d] and [3]
 *      S11 [x] and [3]
 *      24000 [4]
 *  where
 *      [3] The current result is the last or only result, or there are no current results.
 *      [4] The current result is not the last result.
 *
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [e]  Error. The function returned SQL_ERROR.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 */
ACI_RC ulnSFID_18(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if(aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        ACI_TEST_RAISE( ulnStateCheckLastResult(aFnContext) == ACP_FALSE, LABEL_INVALID_CURSOR_STATE );
    }
    else
    {
        /*
         * EXIT POINT
         */
        if(ulnStateCheckLastResult(aFnContext) == ACP_TRUE)
        {
            /* [3] */
            switch(ULN_FNCONTEXT_GET_RC(aFnContext))
            {
                case SQL_NO_DATA:
                case SQL_SUCCESS:
                case SQL_SUCCESS_WITH_INFO:
                    if(ulnStateCheckR(aFnContext) == ACP_TRUE)
                    {
                        /* [r] */
                        ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                        ulnCursorOpen(&sStmt->mCursor);
                    }
                    else
                    {
                        /* [nr] */
                        ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
                    }
                    break;
                case SQL_ERROR:
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                    break;
                case SQL_NEED_DATA:
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                    break;
                default:
                    break;
            }
        }
        else
        {
            ACI_RAISE(LABEL_INVALID_CURSOR_STATE);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_STATE)
    {
        /* [4] : 24000 */
        ulnError(aFnContext,
                 ulERR_ABORT_INVALID_CURSOR_STATE,
                 "Cursor is still open. SQLCloseCursor() needs to be called.");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnExecDirectCheckArgs(ulnFnContext *aFnContext,
                                     acp_char_t   *aStatementText,
                                     acp_sint32_t  aTextLength)
{
    ACI_TEST(ulnPrepCheckArgs(aFnContext, aStatementText, aTextLength) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnExecDirect(ulnStmt *aStmt, acp_char_t *aStatementText, acp_sint32_t aTextLength)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;

    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    ulnDbc       *sDbc;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_EXECDIRECT, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST( ulnExecDirectCheckArgs(&sFnContext,
                                     aStatementText,
                                     aTextLength)
              != ACI_SUCCESS );

    //fix BUG-17722
    ACI_TEST( ulnInitializeProtocolContext(&sFnContext,
                                           &(aStmt->mParentDbc->mPtContext),
                                           &(aStmt->mParentDbc->mSession))
              != ACI_SUCCESS );
    ULN_FLAG_UP(sNeedFinPtContext);

    //fix BUG-17722
    ACI_TEST( ulnPrepareCore(&sFnContext,
                             &(aStmt->mParentDbc->mPtContext),
                             aStatementText,
                             aTextLength,
                             CMP_DB_PREPARE_MODE_EXEC_DIRECT)
              != ACI_SUCCESS );

    /* PROJ-2177 User Interface - Cancel
     * 이후에 DataAtExec를 검사해 NEED DATA를 설정하므로 성공했을때를 위해 초기화해둔다. */
    ulnStmtResetNeedDataFuncID(aStmt);

    ulnDescSetRowsProcessedPtrValue(aStmt->mAttrIpd, 0);
    //fix BUG-17722
    ACI_TEST( ulnExecuteCore(&sFnContext,
                             &(aStmt->mParentDbc->mPtContext))
              != ACI_SUCCESS );

    /*
     * Note : SELECT 성능을 높이기 위해서 prepare -> execute -> fetch 를 한번에 전송
     * Note : Direct Execution 일 때에는 column count 를 모르므로 무조건 fetch
     */

    // fix BUG-17715
    // RecordCount를 Prefetch Row크기만큼 설정해 레코드를 받아온다.
    ACI_TEST( ulnFetchRequestFetch(&sFnContext,
                                   &(aStmt->mParentDbc->mPtContext),
                                   ulnStmtGetAttrPrefetchRows(aStmt),
                                   0,
                                   0)
              != ACI_SUCCESS );

    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    if ( ulnFlushAndReadProtocol( &sFnContext,
                                  &(aStmt->mParentDbc->mPtContext),
                                  aStmt->mParentDbc->mConnTimeoutValue ) != ACI_SUCCESS )
    {
        ACI_TEST( ULN_FNCONTEXT_GET_RC( &sFnContext ) != SQL_NO_DATA );
        ULN_FNCONTEXT_GET_DBC( &sFnContext, sDbc );
        /* BUG-46052 codesonar Null Pointer Dereference */
        ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

        if ( sDbc->mAttrOdbcCompatibility == 2 )
        {
            ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    ACI_TEST( ulnExecuteLob(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext))
              != ACI_SUCCESS );

    ulnStmtSetCurrentResult(aStmt, (ulnResult *)(aStmt->mResultList.mNext));
    ulnStmtSetCurrentResultRowNumber(aStmt, 1);
    ulnDiagSetRowCount(&aStmt->mObj.mDiagHeader, aStmt->mCurrentResult->mAffectedRowCount);

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    ACI_TEST(ulnExecuteBeginFetchAsync(&sFnContext,
                                       &aStmt->mParentDbc->mPtContext)
             != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST( ulnFinalizeProtocolContext(&sFnContext,
                                         &(aStmt->mParentDbc->mPtContext))
              != ACI_SUCCESS );

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| %"ACI_INT32_FMT"\n [%s]",
                  "ulnExecDirect", sFnContext.mSqlReturn, aStatementText);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail\n [%s]", "ulnExecDirect", aStatementText);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
