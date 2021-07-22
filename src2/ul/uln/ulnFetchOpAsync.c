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

#include <ulnFetch.h>
#include <ulnCache.h>
#include <ulnFetchOp.h>
#include <ulnFetchOpAsync.h>
#include <ulnSemiAsyncPrefetch.h>

#define ULN_ASYNC_PREFETCH_ROWS(aNumberOfPrefetchRows, aNumberOfRowsToGet) \
    (aNumberOfRowsToGet > 0) ? ((acp_uint32_t)aNumberOfRowsToGet) : (aNumberOfPrefetchRows)

/**
 * 비동기용 fetch 프로토콜을 송신한다.
 */
static ACI_RC ulnFetchRequestFetchAsync(ulnFnContext *aFnContext,
                                        ulnPtContext *aPtContext,
                                        acp_uint32_t  aNumberOfRowsToFetch,
                                        acp_uint16_t  aColumnNumberToStartFrom,
                                        acp_uint16_t  aColumnNumberToFetchUntil)
{
    /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    ACI_TEST(ulnWriteFetchREQ(aFnContext,
                              aPtContext,
                              aNumberOfRowsToFetch,
                              aColumnNumberToStartFrom,
                              aColumnNumberToFetchUntil) != ACI_SUCCESS);

    /* because receive, if mNeedReadProtocol > 0 in ulnFinalizeProtocolContext() */
    aPtContext->mNeedReadProtocol = 0;

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    ulnSetAsyncSentFetch(aFnContext, ACP_TRUE);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * 비동기용 fetch 프로토콜을 수신한다.
 */
ACI_RC ulnFetchReceiveFetchResultAsync(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnDbc  *sDbc = NULL;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ulnFetchInitForFetchResult(aFnContext);

    ACI_TEST(ulnReadProtocolAsync(aFnContext,
                                  aPtContext,
                                  sDbc->mConnTimeoutValue) != ACI_SUCCESS);

    ulnSetAsyncSentFetch(aFnContext, ACP_FALSE);

    ACI_TEST(ulnFetchUpdateAfterFetch(aFnContext) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * 비동기로 fetch 해 왔는데 cursor size 대비 부족할 경우 부족한 양만큼 다시 요청한다.
 * e.g.) 비동기로 0 으로 요청하였는데 그 양이 cursor size 보다 부족한 경우
 */
static acp_uint32_t ulnFetchGetInsufficientRows(ulnCache  *aCache,
                                                ulnCursor *aCursor)
{
    acp_uint32_t sBlockSizeOfOneFetch;
    acp_sint32_t sNumberOfRowsToGet;

    sBlockSizeOfOneFetch = ulnCacheCalcBlockSizeOfOneFetch(aCache, aCursor);

    sNumberOfRowsToGet   = ulnCacheCalcNumberOfRowsToGet(aCache,
                                                         aCursor,
                                                         sBlockSizeOfOneFetch);

    if (sNumberOfRowsToGet <= 0)
    {
        ACI_RAISE(SUFFICIENT_ROWS);
    }
    else
    {
        /* insufficient rows */
    }

    return (acp_uint32_t)sNumberOfRowsToGet;

    ACI_EXCEPTION_CONT(SUFFICIENT_ROWS);

    return 0;
}

/**
 * Server 로부터 fetch 를 요청하는 비동기용 함수.
 * 만약, 비동기로 send 하지 않았다면 동기로 송수신하고 비동기로 송신해 놓음.
 */
ACI_RC ulnFetchMoreFromServerAsync(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   acp_uint32_t  aNumberOfRowsToGet,
                                   acp_uint32_t  aNumberOfPrefetchRows)
{
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    ulnCache     *sCache = ulnStmtGetCache(sStmt);
    ulnCursor    *sCursor = ulnStmtGetCursor(sStmt);
    acp_uint32_t  sPrefetchRows = aNumberOfPrefetchRows;
    acp_uint32_t  sFetchedRowsAsync = 0;
    acp_uint32_t  sFetchedRowsSync = 0;
    acp_uint32_t  sInsufficientRows = 0;
    acp_bool_t    sIsAsyncPrefetch = ACP_TRUE;
    SQLRETURN     sOriginalRC;

    /* 1. synchronous prefetch, if already read by other protocol */
    if (ulnIsAsyncSentFetch(aFnContext) == ACP_FALSE)
    {
        /* 1-1. synchronous prefetch */
        ACI_TEST(ulnFetchMoreFromServer(aFnContext,
                                        aPtContext,
                                        aNumberOfRowsToGet,
                                        aNumberOfPrefetchRows) != ACI_SUCCESS);

        sIsAsyncPrefetch = ACP_FALSE;
    }
    else
    {
        /* 1-2. asynchronous prefetch */

        /* 1-2-1. read from socket receive buffer */
        sOriginalRC = ULN_FNCONTEXT_GET_RC(aFnContext);

        /* BUG-27119
         * 새로 Cache 할 데이터의 결과를 SQL_SUCCESS로 초기화 한다.
         * 그렇지 않을 경우에는 기존에 세팅된 aFnContext->mSqlReturn 값을 계속 사용하게 된다.
         */
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);

        ACI_TEST(ulnFetchReceiveFetchResultAsync(aFnContext,
                                                 aPtContext) != ACI_SUCCESS);

        sFetchedRowsAsync = ulnStmtGetFetchedRowCountFromServer(sStmt);

        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
        {
            /* 하나라도 Fetch해 왔다면 SQL_NO_DATA가 아니다. */
            if (sFetchedRowsAsync > 0)
            {
                ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
            }

            ACI_RAISE(NORMAL_EXIT);
        }
        else if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS)
        {
            /* BUG-27119 
             * 이 함수에 사용된 aFnContext는 서버에서 새로 받은 결과이다.
             * 미리 Cache되어있던 데이터를 Fetch했었을 수 있기 때문에
             * Original 값으로 변경해야 한다.
             */
            ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
        }

        ACI_TEST_RAISE(sCache->mServerError != NULL, NORMAL_EXIT);

        /* 1-2-2. request fetch protocol synchronous, if fetched rows is insufficient */
        if (ulnCursorGetServerCursorState(sCursor) != ULN_CURSOR_STATE_CLOSED)
        {
            sInsufficientRows = ulnFetchGetInsufficientRows(sCache, sCursor);
            if (sInsufficientRows > 0)
            {
                ACI_TEST(ulnFetchMoreFromServer(aFnContext,
                                                aPtContext,
                                                sInsufficientRows,
                                                sInsufficientRows) != ACI_SUCCESS);

                sFetchedRowsSync = ulnStmtGetFetchedRowCountFromServer(sStmt);
            }
            else
            {
                /* sufficient rows */
            }
        }
        else
        {
            /* nothing to do */
        }
    }

    /* 2. no send CMP_OP_DB_FetchV2, if end of fetch */
    if (ulnCursorGetServerCursorState(sCursor) == ULN_CURSOR_STATE_CLOSED)
    {
        ACI_RAISE(NORMAL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    /* 3. auto-tuning or caclulate prefetch rows, if asynchronous prefetch */
    if (sIsAsyncPrefetch == ACP_TRUE)
    {
#if defined(ALTI_CFG_OS_LINUX)

        if (ACP_LIKELY_FALSE(ulnIsPrefetchAsyncStatTrcLog() == ACP_TRUE))
        {
            struct tcp_info sTcpInfo;

            ACI_TEST(cmiGetLinkInfo(sStmt->mParentDbc->mLink,
                                    (acp_char_t *)&sTcpInfo,
                                    ACI_SIZEOF(struct tcp_info),
                                    CMI_LINK_INFO_TCP_KERNEL_STAT) != ACI_SUCCESS);

            ulnTraceLogPrefetchAsyncStat(aFnContext,
                                         "prefetch rows = %"ACI_UINT32_FMT" (%"ACI_UINT32_FMT" = %"ACI_UINT32_FMT" + %"ACI_UINT32_FMT"), r = %"ACI_INT32_FMT,
                                         sPrefetchRows,
                                         sFetchedRowsAsync + sFetchedRowsSync,
                                         sFetchedRowsAsync,
                                         sFetchedRowsSync,
                                         sTcpInfo.tcpi_last_data_recv);
        }
        else
        {
            /* nothing to do */
        }

#endif /* ALTI_CFG_OS_LINUX */
    }
    else
    {
        /* nothing to do */
    }

    /* 4. send CMP_OP_DB_FetchV2 */
    ACI_TEST(ulnFetchRequestFetchAsync(aFnContext,
                                       aPtContext,
                                       sPrefetchRows,
                                       1,
                                       ulnStmtGetColumnCount(sStmt)) != ACI_SUCCESS);

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Auto-tuning 을 통해 최적의 fetch 양을 조절해 가면서 server 로부터 fetch 를
 * 요청하는 비동기용 함수. 최적의 fetch 양은 network idle time 을 0 에 근사함.
 * 만약, auto-tuning 도중 에러가 발생하면 OFF 시키고 이후부터는 비동기로만
 * 동작됨.
 */
ACI_RC ulnFetchMoreFromServerAsyncWithAutoTuning(ulnFnContext *aFnContext,
                                                 ulnPtContext *aPtContext,
                                                 acp_uint32_t  aNumberOfRowsToGet,
                                                 acp_uint32_t  aNumberOfPrefetchRows)
{
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    ulnCache     *sCache = ulnStmtGetCache(sStmt);
    ulnCursor    *sCursor = ulnStmtGetCursor(sStmt);
    acp_uint32_t  sPrefetchRows = aNumberOfPrefetchRows;
    acp_uint32_t  sFetchedRows = 0;
    acp_bool_t    sIsAsyncPrefetch = ACP_TRUE;
    SQLRETURN     sOriginalRC;

    /* 1. synchronous prefetch, if already read by other protocol */
    if (ulnIsAsyncSentFetch(aFnContext) == ACP_FALSE)
    {
        /* 1-1. synchronous prefetch */
        ACI_TEST(ulnFetchMoreFromServer(aFnContext,
                                        aPtContext,
                                        aNumberOfRowsToGet,
                                        aNumberOfPrefetchRows) != ACI_SUCCESS);

        sIsAsyncPrefetch = ACP_FALSE;
    }
    else
    {
        /* 1-2. asynchronous prefetch */

        ulnFetchAutoTuningBeforeReceive(aFnContext);

        /* 1-2-1. read from socket receive buffer */
        sOriginalRC = ULN_FNCONTEXT_GET_RC(aFnContext);

        /* BUG-27119
         * 새로 Cache 할 데이터의 결과를 SQL_SUCCESS로 초기화 한다.
         * 그렇지 않을 경우에는 기존에 세팅된 aFnContext->mSqlReturn 값을 계속 사용하게 된다.
         */
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);

        ACI_TEST(ulnFetchReceiveFetchResultAsync(aFnContext,
                                                 aPtContext) != ACI_SUCCESS);

        sFetchedRows = ulnStmtGetFetchedRowCountFromServer(sStmt);

        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
        {
            /* 하나라도 Fetch해 왔다면 SQL_NO_DATA가 아니다. */
            if (ulnStmtGetFetchedRowCountFromServer(sStmt) > 0)
            {
                ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
            }

            ACI_RAISE(NORMAL_EXIT);
        }
        else if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS)
        {
            /* BUG-27119 
             * 이 함수에 사용된 aFnContext는 서버에서 새로 받은 결과이다.
             * 미리 Cache되어있던 데이터를 Fetch했었을 수 있기 때문에
             * Original 값으로 변경해야 한다.
             */
            ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
        }

        ACI_TEST_RAISE(sCache->mServerError != NULL, NORMAL_EXIT);
    }

    /* 2. no send CMP_OP_DB_FetchV2, if end of fetch */
    if (ulnCursorGetServerCursorState(sCursor) == ULN_CURSOR_STATE_CLOSED)
    {
        ACI_TEST(ulnFetchEndAutoTuning(aFnContext) != ACI_SUCCESS);

        ACI_RAISE(NORMAL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    /* 3. auto-tuning or caclulate prefetch rows, if asynchronous prefetch */
    if (sIsAsyncPrefetch == ACP_TRUE)
    {
        /* 3-1. auto-tuning and get predicted prefetch rows */
        if (ulnFetchAutoTuning(aFnContext,
                               sFetchedRows,
                               &sPrefetchRows) == ACI_SUCCESS)
        {
            /* auto-tuning success */
        }
        else
        {
            /* end auto-tuning, if auto-tuning was occured error */
            (void)ulnFetchEndAutoTuning(aFnContext);

            /* asynchronous prefetch without auto-tuning from hence */
            sPrefetchRows = ULN_ASYNC_PREFETCH_ROWS(aNumberOfPrefetchRows, aNumberOfRowsToGet);
        }
    }
    else
    {
        /* not meant auto-tuning statistics */
        ulnFetchSkipAutoTuning(aFnContext);

        /* get the last auto-tuned prefetch rows */
        sPrefetchRows = ulnFetchGetAutoTunedPrefetchRows(aFnContext);
    }

    /* 4. send CMP_OP_DB_FetchV2 */
    ACI_TEST(ulnFetchRequestFetchAsync(aFnContext,
                                       aPtContext,
                                       sPrefetchRows,
                                       1,
                                       ulnStmtGetColumnCount(sStmt)) != ACI_SUCCESS);

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Fetch 양을 계산하는 비동기용 함수
 */
void ulnFetchCalcPrefetchRowsAsync(ulnCache     *aCache,
                                   ulnCursor    *aCursor,
                                   acp_uint32_t *aPrefetchRows)
{
    acp_sint32_t sNumberOfRowsToGet;    /* insufficient rows in cache */
    acp_uint32_t sNumberOfPrefetchRows; /* max(cursor size, attr) */

    ACE_DASSERT(aCache != NULL);
    ACE_DASSERT(aCursor != NULL);
    ACE_DASSERT(aPrefetchRows != NULL);

    ulnFetchCalcPrefetchRows(aCache,
                             aCursor,
                             &sNumberOfPrefetchRows,
                             &sNumberOfRowsToGet);

    /**
     * sNumberOfPrefetchRows may be ULN_CACHE_OPTIMAL_PREFETCH_ROWS(0)
     * sNumberOfRowsToGet may be negative number, if sufficient in cache
     */

    *aPrefetchRows = ULN_ASYNC_PREFETCH_ROWS(sNumberOfPrefetchRows, sNumberOfRowsToGet);
}

/**
 * 비동기 fetch 를 시작한다.
 */
ACI_RC ulnFetchBeginAsync(ulnFnContext *aFnContext,
                          ulnPtContext *aPtContext)
{
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    ulnCache     *sCache = ulnStmtGetCache(sStmt);
    ulnCursor    *sCursor = ulnStmtGetCursor(sStmt);
    acp_uint32_t  sPrefetchRows = 0;

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "fetch async begin : semi-async prefetch");

    if (ulnStmtGetAttrPrefetchAutoTuning(sStmt) == ACP_FALSE)
    {
        ulnFetchCalcPrefetchRowsAsync(sCache,
                                      sCursor,
                                      &sPrefetchRows);
    }
    else
    {
        ulnFetchBeginAutoTuning(aFnContext);

        if (ulnFetchAutoTuning(aFnContext,
                               0,
                               &sPrefetchRows) == ACI_SUCCESS)
        {
            /* auto-tuning success */
        }
        else
        {
            /* end auto-tuning, if auto-tuning was occured error */
            (void)ulnFetchEndAutoTuning(aFnContext);

            /* asynchronous prefetch without auto-tuning from hence */
            ulnFetchCalcPrefetchRowsAsync(sCache,
                                          sCursor,
                                          &sPrefetchRows);
        }
    }

    ACI_TEST(ulnFetchRequestFetchAsync(aFnContext,
                                       aPtContext,
                                       sPrefetchRows,
                                       0,
                                       0) != ACI_SUCCESS);

    ulnDbcSetAsyncPrefetchStmt(sStmt->mParentDbc, sStmt);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * SQLMoreResults() 함수 등에서 다음 result set 으로 fetch 를 시작할 경우 호출한다.
 */
ACI_RC ulnFetchNextAsync(ulnFnContext *aFnContext,
                         ulnPtContext *aPtContext)
{
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    ulnCache     *sCache = ulnStmtGetCache(sStmt);
    ulnCursor    *sCursor = ulnStmtGetCursor(sStmt);
    acp_uint32_t  sPrefetchRows = 0;

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "fetch async : next result set");

    if (ulnStmtGetAttrPrefetchAutoTuning(sStmt) == ACP_FALSE)
    {
        ulnFetchCalcPrefetchRowsAsync(sCache,
                                      sCursor,
                                      &sPrefetchRows);
    }
    else
    {
        ulnFetchNextAutoTuning(aFnContext);

        if (ulnFetchAutoTuning(aFnContext,
                               0,
                               &sPrefetchRows) == ACI_SUCCESS)
        {
            /* auto-tuning success */
        }
        else
        {
            /* end auto-tuning, if auto-tuning was occured error */
            (void)ulnFetchEndAutoTuning(aFnContext);

            /* asynchronous prefetch without auto-tuning from hence */
            ulnFetchCalcPrefetchRowsAsync(sCache,
                                          sCursor,
                                          &sPrefetchRows);
        }
    }

    ACI_TEST(ulnFetchRequestFetchAsync(aFnContext,
                                       aPtContext,
                                       sPrefetchRows,
                                       0,
                                       0) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Curosr close 또는 free statement 시 비동기 fetch 를 종료한다.
 */
ACI_RC ulnFetchEndAsync(ulnFnContext *aFnContext,
                        ulnPtContext *aPtContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;
    ULN_FLAG(sNeedFinPtContext);

    if (ulnIsAsyncSentFetch(aFnContext) == ACP_TRUE)
    {
        ACI_TEST(ulnInitializeProtocolContext(aFnContext,
                                              aPtContext,
                                              &sStmt->mParentDbc->mSession)
                != ACI_SUCCESS);

        ULN_FLAG_UP(sNeedFinPtContext);

        ACI_TEST(ulnFetchReceiveFetchResultAsync(aFnContext,
                                                 aPtContext) != ACI_SUCCESS);

        ULN_FLAG_DOWN(sNeedFinPtContext);

        ACI_TEST(ulnFinalizeProtocolContext(aFnContext,
                                            aPtContext)
                != ACI_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    if (ulnStmtGetAttrPrefetchAutoTuning(sStmt) == ACP_TRUE)
    {
        (void)ulnFetchEndAutoTuning(aFnContext);
    }
    else
    {
        /* auto-uning is OFF */
    }

    ulnDbcSetAsyncPrefetchStmt(sStmt->mParentDbc, NULL);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "fetch async end");

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext(aFnContext, aPtContext);
    }

    return ACI_FAILURE;
}

/**
 * 비동기로 프로토콜 송신한 상태에서 다른 프로토콜 송수신하고자 할 경우,
 * 비동기로 송신한 프로토콜을 수신한다.
 */
ACI_RC ulnFetchReceiveFetchResultForSync(ulnFnContext *aFnContext,
                                         ulnPtContext *aPtContext,
                                         ulnStmt      *aAsyncPrefetchStmt)
{
    ulnFnContext sFnContext;

    ACE_DASSERT(aAsyncPrefetchStmt != NULL);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext,
                              aFnContext->mFuncID,
                              aAsyncPrefetchStmt,
                              ULN_OBJ_TYPE_STMT);

    if (ulnIsAsyncSentFetch(&sFnContext) == ACP_TRUE)
    {
        ACI_TEST(ulnFetchReceiveFetchResultAsync(&sFnContext,
                                                 aPtContext) != ACI_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

