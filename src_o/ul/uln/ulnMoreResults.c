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

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>

ACI_RC ulnSFID_39_40(ulnFnContext *aFnContext, ulnStmtState aBackPreparedState)
{
    ulnResult *sResult;

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
            ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO)
        {
            /* [s]
             *
             * Note : MoreResult 함수 본체에서 next result 를
             *        current result 로 세팅했으므로
             *        ulnStmtGetNextResult() 가 아닌
             *        ulnStmtGetCurrentResult() 를 호출해야
             *        next result 가 나온다.
             */
            sResult = ulnStmtGetCurrentResult(aFnContext->mHandle.mStmt);
            ACE_ASSERT(sResult != NULL); /* NULL이면 NO_DATA 쪽 코드를 타야함 */

            if (ulnStmtHasResultSet(aFnContext->mHandle.mStmt) == ACP_TRUE)
            {
                /* [3] The next result is a result set */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
            }
            else if (ulnStmtHasRowCount(aFnContext->mHandle.mStmt) == ACP_TRUE)
            {
                /* [2] The next result is a row count */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
            }
            else
            {
                /* BUGBUG (2014-04-18) #MultiStmt
                 * 결과가 없으면 SQL_NO_DATA가 떨어지므로 여기를 타면 안된다.
                 * multi-stmt에서는 RowCount나 ResultSet을 만들지 않으면
                 * MoreResults로 반환할 유효한 result로 보지 않기 때문. (MsSql-like)
                 * 현재는 그런 처리가 없긴 하다만, multi-stmt는 비공개므로 그냥 ASSERT 처리한다. */
                ACE_ASSERT(0);
            }
        }
        else
        {
            if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
            {
                /* [nf] */
                if (ulnStmtIsLastResult(aFnContext->mHandle.mStmt) == ACP_TRUE)
                {
                    /* [4] The current result is the last result */
                    if (ulnStmtIsPrepared(aFnContext->mHandle.mStmt) == ACP_TRUE)
                    {
                        /* [p] */
                        ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, aBackPreparedState);
                    }
                    else
                    {
                        /* [np] */
                        ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
                    }
                }
            }
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_39
 * SQLGetStmtAttr(), STMT, S1, S2-S3, S4, S5
 *      -- [s] and [2]
 *      S5 [s] and [3]
 *
 *      S1 [nf], [np], and [4]
 *      S2 [nf], [p], and [4]
 *
 *      S11 [x]
 *  where
 *      [1] The function always returns SQL_NO_DATA in this state.
 *      [2] The next result is a row count.
 *      [3] The next result is a result set.
 *      [4] The current result is the last result.
 */
ACI_RC ulnSFID_39(ulnFnContext *aFnContext)
{
    return ulnSFID_39_40(aFnContext, ULN_S_S2);
}

/*
 * ULN_SFID_40
 * SQLGetStmtAttr(), STMT, S1, S2-S3, S4, S5
 *      S1 [nf], [np], and [4]
 *      S3 [nf], [p] and [4]
 *
 *      S4 [s] and [2]
 *      S5 [s] and [3]
 *
 *      S11 [x]
 *  where
 *      [1] The function always returns SQL_NO_DATA in this state.
 *      [2] The next result is a row count.
 *      [3] The next result is a result set.
 *      [4] The current result is the last result.
 */
ACI_RC ulnSFID_40(ulnFnContext *aFnContext)
{
    return ulnSFID_39_40(aFnContext, ULN_S_S3);
}

ACI_RC ulnMoreResultsCore(ulnFnContext *aFnContext,
                          ulnPtContext *aPtContext)
{
    ulnStmt      *sStmt;
    acp_uint16_t  sResultSetCount;
    acp_uint16_t  sCurrentResultSetID;
    ulnCursor    *sCursor;
    ulnResult    *sResult;
    ulnDesc      *sDescIrd;

    acp_uint32_t  sPrefetchCnt;

    sStmt = aFnContext->mHandle.mStmt;

    /* PROJ-1381, BUG-32902, BUG-33037 FAC
     * FetchEnd 때 서버에서 Close하며 다음 결과를 받을 때 다시
     * OPEN 상태로 바꾸므로 여기서 FetchEnd를 고려해 CLOSED 상태로 바꿔준다.
     * 하지만, Holdable Statement는 CloseCursor 전까지 임의로 끝내지 않으므로
     * 상태를 바꾸지 않아야 한다. */
    if (ulnStmtGetAttrCursorHold(sStmt) == SQL_CURSOR_HOLD_OFF)
    {
        sCursor = ulnStmtGetCursor(sStmt);
        ulnCursorSetServerCursorState(sCursor, ULN_CURSOR_STATE_CLOSED);
    }

    sResult = ulnStmtGetNextResult(sStmt);
    if (sResult != NULL)
    {
        /*
         * Array of Parameter의 다음 결과가 존재
         */
        ulnStmtSetCurrentResult(sStmt, sResult);
        ulnDiagSetRowCount(&sStmt->mObj.mDiagHeader, sResult->mAffectedRowCount);
    }
    else
    {
        sCurrentResultSetID = ulnStmtGetCurrentResultSetID(sStmt);
        sResultSetCount = ulnStmtGetResultSetCount(sStmt);

        if (sCurrentResultSetID < sResultSetCount - 1)
        {
            /*
             * 다음 Result Set이 존재
             */

            // CurrentResultSetID 증가
            sCurrentResultSetID++;
            ulnStmtSetCurrentResultSetID(sStmt, sCurrentResultSetID);

            // ResultSet 초기화
            sResult = ulnStmtGetCurrentResult(sStmt);
            ulnResultSetType(sResult, ULN_RESULT_TYPE_RESULTSET);

            // IRD 초기화
            sDescIrd = ulnStmtGetIrd(sStmt);
            ACI_TEST_RAISE(sDescIrd == NULL, LABEL_MEM_MAN_ERR);
            ACI_TEST_RAISE(ulnDescRollBackToInitial(sDescIrd) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
            ACI_TEST_RAISE(ulnDescInitialize(sDescIrd, (ulnObject *)sStmt) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEM);

            // 캐시 초기화
            ACI_TEST( ulnCacheInitialize(sStmt->mCache) != ACI_SUCCESS );

            if (sStmt->mKeyset != NULL)
            {
                ACI_TEST(ulnKeysetInitialize(sStmt->mKeyset) != ACI_SUCCESS );
            }

            // 커서 초기화
            ulnCursorSetPosition(&sStmt->mCursor, ULN_CURSOR_POS_BEFORE_START);

            // fix BUG-21737
            sStmt->mGDColumnNumber = ULN_GD_COLUMN_NUMBER_INIT_VALUE;

            // ColumnInfoGet RES
            ACI_TEST(ulnWriteColumnInfoGetREQ(aFnContext, aPtContext, 0) != ACI_SUCCESS);

            // Fetch RES
            // To Fix BUG-20390
            // Prefetch Cnt를 계산하여 요청함.
            sPrefetchCnt = ulnCacheCalcPrefetchRowSize( sStmt->mCache,
                                                        & sStmt->mCursor );

            ACI_TEST(ulnFetchRequestFetch(aFnContext,
                                          aPtContext,
                                          sPrefetchCnt,
                                          0,
                                          0) != ACI_SUCCESS);

            ACI_TEST(ulnFlushAndReadProtocol(aFnContext,
                                             aPtContext,
                                             sStmt->mParentDbc->mConnTimeoutValue) != ACI_SUCCESS);

            /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
            if (ulnDbcIsAsyncPrefetchStmt(sStmt->mParentDbc, sStmt) == ACP_TRUE)
            {
                ulnFetchNextAsync(aFnContext, aPtContext);
            }
            else
            {
                /* synchronous prefetch */
            }
        }
        else
        {
            /*
             * 더 이상 Result Set이 없음
             */
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NO_DATA);
        }
    }

    /* PROJ-1789: 다른 결과셋으로 넘어가므로 초기화해주어야 한다. */
    ulnStmtResetQstrForDelete(sStmt);
    ulnStmtResetTableNameForUpdate(sStmt);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnMoreResultsCore");
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnMoreResultsCore");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * Multiple Result 를 구현하면서 주의해야 할 포인트들
 *
 *      1. PREPARE RESULT 수신시
 *         explicit batch 구현시 이 포인트를 고려해야 할 필요가 있음.
 *
 *      2. SQLExecute() / SQLExecDirect() 함수 진입시
 *         ulnStmtFreeAllResult(sStmt);
 *         sStmt->mTotalAffectedRowCount = ID_ULONG(0); <-- SQL_NO_DATA 리턴을 위해 필요함.
 *
 *      3. EXECUTE RESULT 수신시
 *         Add new result
 *
 *      4. SQLExecute() / SQLExecDirect() 함수 탈출시
 *         Current result 를 처음 result 로 세팅.
 *         Diag header 의 row count 를 세팅
 *
 *      5. SQLCloseCursor() 호출시 (SQLFreeStmt(SQL_CLOSE))
 *         ulnStmtFreeAllResult(sStmt);
 */

SQLRETURN ulnMoreResults(ulnStmt *aStmt)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_MORERESULTS, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &aStmt->mParentDbc->mSession) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnMoreResultsCore(&sFnContext,
                               &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

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
