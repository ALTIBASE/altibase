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
#include <ulnParamData.h>

/*
 * ULN_SFID_41
 * SQLParamData(), STMT, S8
 *
 *      S1 [e] and [1]
 *      S2 [e], [nr], and [2]
 *      S3 [e], [r], and [2]
 *      S5 [e] and [4]
 *      S6 [e] and [5]
 *      S7 [e] and [3]
 *      S9 [d]
 *      S11 [x]
 *
 * where
 *      [1] SQLExecDirect returned SQL_NEED_DATA.
 *      [2] SQLExecute returned SQL_NEED_DATA.
 *      [3] SQLSetPos had been called from state S7 and returned SQL_NEED_DATA.
 *      [4] SQLBulkOperations had been called from state S5 and returned SQL_NEED_DATA.
 *      [5] SQLSetPos or SQLBulkOperations had been called from state S6 and returned
 *          SQL_NEED_DATA.
 *
 *      [e]  Error. The function returned SQL_ERROR.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *
 *      [nr] No results. The statement will not or did not create a result set.
 */
ACI_RC ulnSFID_41(ulnFnContext *aFnContext)
{
    /*
     * BUGBUG : [3] [4] [5] 는 아직 구현하지 않았으므로 일단 무시하도록 한다.
     *          [1] [2] 를 구현해야 한다.
     *          일단은 SQLExecute() 를 했다고 가정한다. 즉, 무조건 [2]
     */

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_ERROR)
        {
            /* [e] */
            if (0)   // BUGBUG
            {
                /* [1] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
            }
            else
            {
                /* [2] */
                if (ulnStmtGetColumnCount(aFnContext->mHandle.mStmt) == 0)
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                }
                else
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
                }
            }
        }
        else if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NEED_DATA)
        {
            /* [d] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S9);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_42
 * SQLParamData(), STMT, S10
 *
 *      S1 [e] and [1]
 *      S2 [e], [nr], and [2]
 *      S3 [e], [r], and [2]
 *      S4 [s], [nr], and ([1] or [2])
 *      S5 [s], [r], and ([1] or [2])
 *      S5 ([s] or [e]) and [4]
 *      S6 ([s] or [e]) and [5]
 *      S7 ([s] or [e]) and [3]
 *      S9 [d]
 *      S11 [x]
 *
 * where
 *      [1] SQLExecDirect returned SQL_NEED_DATA.
 *      [2] SQLExecute returned SQL_NEED_DATA.
 *      [3] SQLSetPos had been called from state S7 and returned SQL_NEED_DATA.
 *      [4] SQLBulkOperations had been called from state S5 and returned SQL_NEED_DATA.
 *      [5] SQLSetPos or SQLBulkOperations had been called from state S6 and returned
 *          SQL_NEED_DATA.
 *
 *      [e]  Error. The function returned SQL_ERROR.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 */
ACI_RC ulnSFID_42(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    /*
     * BUGBUG : [3] [4] [5] 는 아직 구현하지 않았으므로 일단 무시하도록 한다.
     *          [1] [2] 를 구현해야 한다.
     *          일단은 SQLExecute() 를 했다고 가정한다. 즉, 무조건 [2]
     */

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NEED_DATA)
        {
            /* [d] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S9);
        }
        else
        {
            if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_ERROR)
            {
                /* [e] */
                if (0)   // BUGBUG
                {
                    /* [1] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                }
                else
                {
                    /* [2] */
                    if (ulnStmtGetColumnCount(aFnContext->mHandle.mStmt) == 0)
                    {
                        /* [nr] */
                        ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                    }
                    else
                    {
                        /* [r] */
                        ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
                    }
                }
            }
            else if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
                     ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO ||
                     ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA /* BUG-45504 */)
            {
                /* [s] */
                if (ulnStmtGetColumnCount(aFnContext->mHandle.mStmt) == 0)
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
                }
                else
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                    ulnCursorOpen(&sStmt->mCursor);
                }
            }

            /*
             * BUGBUG : [4] [5] [3] 번 조건 검사
             */
        }
    }

    return ACI_SUCCESS;
}

static ACI_RC ulnParamDataReturnUserMarker(ulnFnContext *aFnContext, void **aValuePtr)
{
    ulnStmt             *sStmt = aFnContext->mHandle.mStmt;
    acp_uint16_t         sProcessingParamNumber = sStmt->mProcessingParamNumber;
    ulnDescRec          *sDescRecApp;
    ulnDescRec          *sDescRecImp;
    ulnColumnIgnoreFlag *sColumnIgnoreFlags;
    acp_sint32_t         sColTotCount;
    acp_sint32_t         sActColIdx;
    acp_sint32_t         sActParamNum;

    /* PROJ-1789 Updatable Scrollable Cursor
     * RowsetStmt로 작업할 때는 KeysetStmt의 BindCol 정보를 얻어와야 한다. */
    if (sStmt->mParentStmt != NULL)
    {
        sStmt = sStmt->mParentStmt;
        ACE_DASSERT((ulnStmtGetNeedDataFuncID(sStmt) == ULN_FID_SETPOS) ||
                    (ulnStmtGetNeedDataFuncID(sStmt) == ULN_FID_BULKOPERATIONS));

        /* SetPos, BulkOperations를 호출해서 NEED_DATA가 발생한 것이므로
         * ColumnIgnoreFlags가 생성되고 올바른 값으로 채워졌음을 보장한다. */
        sColumnIgnoreFlags = ulnStmtGetColumnIgnoreFlagsBuf(sStmt);
        ACI_TEST_RAISE(sColumnIgnoreFlags == NULL, LABEL_MEM_MAN_ERR);

        sColTotCount = ulnStmtGetColumnCount(sStmt);
        sActParamNum = 0;
        for (sActColIdx = 1; sActColIdx <= sColTotCount; sActColIdx++)
        {
            if (sColumnIgnoreFlags[sActColIdx-1] == ULN_COLUMN_IGNORE)
            {
                continue;
            }
            sActParamNum ++;

            if (sActParamNum == sProcessingParamNumber)
            {
                break;
            }
        }
        ACI_TEST_RAISE(sActColIdx > sColTotCount, LABEL_MEM_MAN_ERR);

        sDescRecApp = ulnStmtGetArdRec(sStmt, sActColIdx);
        ACI_TEST_RAISE(sDescRecApp == NULL, LABEL_MEM_MAN_ERR);

        sDescRecImp = ulnStmtGetIrdRec(sStmt, sActColIdx);
        ACI_TEST_RAISE(sDescRecImp == NULL, LABEL_MEM_MAN_ERR);
    }
    else
    {
        /*
        * execute 를 하고 나면 현재 처리하는 row, param number 가 달라질수도 있다.
        */
        sDescRecApp = ulnStmtGetApdRec(sStmt, sProcessingParamNumber);
        ACI_TEST_RAISE(sDescRecApp == NULL, LABEL_MEM_MAN_ERR);

        sDescRecImp = ulnStmtGetIpdRec(sStmt, sProcessingParamNumber);
        ACI_TEST_RAISE(sDescRecImp == NULL, LABEL_MEM_MAN_ERR);
    }

    /* BUG-26666 SQLParamData 에서 2번째 인자의 값을 잘못 넘겨주고 있습니다. */
    *aValuePtr = sDescRecApp->mDataPtr;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        /*
         * HY013
         */
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ParamDataReturnUserMarker");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnParamDataCore(ulnFnContext *aFnContext,
                        ulnStmt      *aStmt,
                        void        **aValuePtr)
{
    ulnDescRec   *sDescRecApd;
    ulnDescRec   *sDescRecIpd;

    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    ulnDbc *sDbc;

    sDescRecApd = ulnStmtGetApdRec(aStmt, aStmt->mProcessingParamNumber);
    ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_MEM_MAN_ERR);

    sDescRecIpd = ulnStmtGetIpdRec(aStmt, aStmt->mProcessingParamNumber);
    ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MAN_ERR);

    if (ulnTypeIsMemBoundLob(sDescRecIpd->mMeta.mMTYPE,
                             sDescRecApd->mMeta.mCTYPE) == ACP_TRUE)
    {
        //fix BUG-17722
        if (ulnExecLobPhase(aFnContext, &(aStmt->mParentDbc->mPtContext))
               != ACI_SUCCESS)
        {
            if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NEED_DATA)
            {
                ACI_TEST(ulnParamDataReturnUserMarker(aFnContext,
                                                      aValuePtr) != ACI_SUCCESS);
            }
        }
    }
    else
    {
        //fix BUG-17722
        if (ulnExecuteCore(aFnContext, &(aStmt->mParentDbc->mPtContext))
               != ACI_SUCCESS)
        {
            if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NEED_DATA)
            {
                ACI_TEST(ulnParamDataReturnUserMarker(aFnContext,
                                                      aValuePtr) != ACI_SUCCESS);
            }
        }
        else
        {
            /* BUG-44942 Fetch 프로토콜을 같이 전송해서 통신비용 줄이기 */
            if ((ulnStmtHasResultSet(aStmt) == ACP_TRUE) ||
                ((aStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_ON) &&
                 (aStmt->mIsSelect == ACP_TRUE)))
            {
                ACI_TEST(ulnFetchRequestFetch(aFnContext,
                                              &(aStmt->mParentDbc->mPtContext),
                                              ulnStmtGetAttrPrefetchRows(aStmt),
                                              0,
                                              0) != ACI_SUCCESS);
            }
            else
            {
                /* A obsolete convention */
            }

            //fix BUG-17722
            /* BUG-36548 Return code of client functions should be differed by ODBC version */
            if ( ulnFlushAndReadProtocol( aFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          aStmt->mParentDbc->mConnTimeoutValue ) != ACI_SUCCESS )
            {
                ACI_TEST( ULN_FNCONTEXT_GET_RC( aFnContext ) != SQL_NO_DATA );
                ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );
                /* BUG-46052 codesonar Null Pointer Dereference */
                ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

                ACI_TEST( sDbc->mAttrOdbcCompatibility == 3 );
                ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );
            }
            else
            {
                /* do nothing */
            }

            if (ulnExecuteLob(aFnContext, &(aStmt->mParentDbc->mPtContext))
                  != ACI_SUCCESS)
            {
                if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NEED_DATA)
                {
                    ACI_TEST(ulnParamDataReturnUserMarker(aFnContext,
                                                          aValuePtr) != ACI_SUCCESS);
                }
            }

            // bug-21634 Result set 을 세팅해야 합니다.
            ulnStmtSetCurrentResult(aStmt, (ulnResult *)(aStmt->mResultList.mNext));
            ulnStmtSetCurrentResultRowNumber(aStmt, 1);
            ulnDiagSetRowCount(&aStmt->mObj.mDiagHeader, aStmt->mCurrentResult->mAffectedRowCount);

            /* PROJ-2177: NEES DATA 처리가 끝났으므로 초기화 */
            /* BUG-32618: 계속 NEED_DATA 상태더라도 예외로 빠지지않고 이쪽 코드를 탄다.
             * RC가 SUCCESS일 때만 초기화하도록 해야한다. */
            if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS)
            {
                ulnStmtResetNeedDataFuncID(aStmt);
            }
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        /* HY013 */
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnParamDataCore");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnParamData(ulnStmt *aStmt, void **aValuePtr)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnDbc       *sDbc = NULL;

    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PARAMDATA, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * -----------------------------------------
     * Protocol Context
     */

    /* PROJ-1789 Updatable Scrollable Cursor
     * SetPos, BulkOperations에서 NEED_DATA가 났으면 RowsetStmt로 작업. */
    if ((ulnStmtGetNeedDataFuncID(aStmt) == ULN_FID_SETPOS)
     || (ulnStmtGetNeedDataFuncID(aStmt) == ULN_FID_BULKOPERATIONS))
    {
        sFnContext.mHandle.mStmt = aStmt->mRowsetStmt;
        ACI_TEST_RAISE(ulnParamDataCore(&sFnContext, aStmt->mRowsetStmt, aValuePtr)
                       != ACI_SUCCESS, ROWSET_PARAMDATA_FAILED_EXCEPTION);
        sFnContext.mHandle.mStmt = aStmt;

        if (ULN_FNCONTEXT_GET_RC(&sFnContext) != SQL_SUCCESS)
        {
            ulnDiagRecMoveAll(&aStmt->mObj, &aStmt->mRowsetStmt->mObj);
        }
    }
    else
    {
        ACI_TEST(ulnParamDataCore(&sFnContext, aStmt, aValuePtr)
                 != ACI_SUCCESS);
    }

    /*
     * Protocol Context
     * -----------------------------------------
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);

    if ((sDbc->mAttrPDODeferProtocols >= 1) &&
        (ULN_FNCONTEXT_GET_RC(&sFnContext) == SQL_NEED_DATA))
    {
        /* BUG-45286 ParamDataIn 전송 지연 */
    }
    else
    {
        //fix BUG-17722
        ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                            &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);
    }

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(ROWSET_PARAMDATA_FAILED_EXCEPTION)
    {
        ulnDiagRecMoveAll(&aStmt->mObj, &aStmt->mRowsetStmt->mObj);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext,&(aStmt->mParentDbc->mPtContext));
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}


