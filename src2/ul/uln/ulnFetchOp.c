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

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>
#include <ulnSemiAsyncPrefetch.h>

/*
 * FETCH 를 하기 위한 일련의 순차적인 함수들
 *      ulnFetchRequestFetch
 *      ulnFetchReceiveFetchResult
 *      ulnFetchUpdateAfterFetch
 *
 * Note : 이 함수들은 항상 세트로 불려야 하기 때문에 혹시라도 실수가 있을 수 있다.
 *        그래서 다른 곳에서 호출하지 못하도록 static 으로 잡았다.
 *        다른 곳에서 호출하려면 반드시 ulnFetchOperationSet() 함수를 이용해야 한다.
 *
 * Note : 예외
 *        SELECT 성능 향상을 위해서 execute 시에 fetch request 까지 전송한다.
 *        이때만은
 *
 *              ulnFetchRequestFetch()
 *
 *        만 따로 호출한다.
 */

ACI_RC ulnFetchRequestFetch(ulnFnContext *aFnContext,
                            ulnPtContext *aPtContext,
                            acp_uint32_t  aNumberOfRowsToFetch,
                            acp_uint16_t  aColumnNumberToStartFrom,
                            acp_uint16_t  aColumnNumberToFetchUntil)
{
    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    ulnFetchInitForFetchResult(aFnContext);

    /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    ACI_TEST(ulnWriteFetchREQ(aFnContext,
                              aPtContext,
                              aNumberOfRowsToFetch,
                              aColumnNumberToStartFrom,
                              aColumnNumberToFetchUntil) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
void ulnFetchInitForFetchResult(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    ulnStmtSetFetchedRowCountFromServer(sStmt, 0);
    ulnStmtSetFetchedBytesFromServer(sStmt, 0);
    //fix BUG-17820
    sStmt->mFetchedColCnt = 0;
}

static ACI_RC ulnFetchReceiveFetchResult(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnDbc  *sDbc;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    if (cmiGetLinkImpl(&(aPtContext->mCmiPtContext)) == CMI_LINK_IMPL_IPCDA)
    {
        ACI_TEST( ulnReadProtocolIPCDA(aFnContext,
                                       aPtContext,
                                       sDbc->mConnTimeoutValue) != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST(ulnReadProtocol(aFnContext,
                                 aPtContext,
                                 sDbc->mConnTimeoutValue) != ACI_SUCCESS);
    }

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
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

ACI_RC ulnFetchUpdateAfterFetch(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    /*
     * Note : IMPORTANT
     *        성능 향상을 위해 prepare exec fetch 를 한번에 전송한다.
     *        따라서, 아래의 상황에서도 이 함수로 진입할 수 있다.
     *
     *          1. cache 가 만들어지지 않은 상황
     *          2. cursor 가 open 되지 않은 상황
     */

    if (ulnCursorGetState(&sStmt->mCursor) == ULN_CURSOR_STATE_OPEN)
    {
        /*
         * Cursor 의 위치를 괜히 한번 세팅해 줌으로써, 현재 위치가
         * ULN_CURSOR_POS_AFTER_END 인지를 체크해서 cursor->mPosition 을 갱신한다.
         */

        ulnCursorSetPosition(&sStmt->mCursor,
                             ulnCursorGetPosition(&sStmt->mCursor));
    }

    if (ulnStmtGetFetchedRowCountFromServer(sStmt) == 0)
    {
        if (ulnStmtGetAttrCursorType(sStmt) == SQL_CURSOR_KEYSET_DRIVEN)
        {
            ulnKeysetSetFullFilled(ulnStmtGetKeyset(sStmt), ACP_TRUE);
        }
        else
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NO_DATA);
        }
    }

    return ACI_SUCCESS;
}

static ACI_RC ulnFetchOperationSet(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   acp_uint32_t  aNumberOfRecordsToFetch,
                                   acp_uint16_t  aColumnNumberToStartFrom,
                                   acp_uint16_t  aColumnNumberToFetchUntil)
{
    ACI_TEST(ulnFetchRequestFetch(aFnContext,
                                  aPtContext,
                                  aNumberOfRecordsToFetch,
                                  aColumnNumberToStartFrom,
                                  aColumnNumberToFetchUntil) != ACI_SUCCESS);

    ACI_TEST(ulnFetchReceiveFetchResult(aFnContext, aPtContext) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ===================================================================
 *
 * 외부로 export 되는 세개의 함수
 *
 *      > 서버에서 fetch 해 오는 함수 두가지
 *        ulnFetchMoreFromServer()
 *        ulnFetchAllFromServer()
 *
 *      > 캐쉬로부터 fetch 해 오는 함수
 *        ulnFetchFromCache()
 *
 * ===================================================================
 */

ACI_RC ulnFetchMoreFromServer(ulnFnContext *aFnContext,
                              ulnPtContext *aPtContext,
                              acp_uint32_t  aNumberOfRowsToGet,
                              acp_uint32_t  aNumberOfPrefetchRows)
{
    acp_uint32_t  sTotalNumberOfRows = 0;
    acp_uint32_t  sNumberOfPrefetchRows;
    ulnStmt      *sStmt;
    ulnCache     *sCache;
    SQLRETURN     sOriginalRC;

    sStmt   = aFnContext->mHandle.mStmt;
    sCache  = ulnStmtGetCache(sStmt);

    /*
     * Note : sNumberOfPrefetchRows이 통신상 최대 수신 ROW의 개수(ACP_SINT32_MAX)보다
     *        클수 있기 때문에, 반복해서 수행되어야 한다.
     */
    sNumberOfPrefetchRows = aNumberOfPrefetchRows;

    if (sNumberOfPrefetchRows > ACP_SINT32_MAX)
    {
        sNumberOfPrefetchRows = ACP_SINT32_MAX;
    }

    while (sTotalNumberOfRows < aNumberOfRowsToGet)
    {
        sOriginalRC = ULN_FNCONTEXT_GET_RC(aFnContext);

        /* BUG-27119
         * 새로 Cache 할 데이터의 결과를 SQL_SUCCESS로 초기화 한다.
         * 그렇지 않을 경우에는 기존에 세팅된 aFnContext->mSqlReturn 값을 계속 사용하게 된다.
         */
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);

        ACI_TEST(ulnFetchOperationSet(aFnContext,
                                      aPtContext,
                                      sNumberOfPrefetchRows,
                                      1,
                                      ulnStmtGetColumnCount(sStmt))
                 != ACI_SUCCESS);

        sTotalNumberOfRows += ulnStmtGetFetchedRowCountFromServer(sStmt);

        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
        {
            /* 하나라도 Fetch해 왔다면 SQL_NO_DATA가 아니다. */
            if (sTotalNumberOfRows > 0)
            {
                ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
            }
            break;
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

        if (sCache->mServerError != NULL)
        {
            break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnFetchAllFromServer(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    acp_uint32_t sTotalNumberOfRows = 0;
    acp_uint16_t sNumberOfRowsToGet = ACP_UINT16_MAX;

    while (sTotalNumberOfRows < ACP_SINT32_MAX) // 안전장치
    {
        ACI_TEST(ulnFetchMoreFromServer(aFnContext,
                                        aPtContext,
                                        sNumberOfRowsToGet,
                                        sNumberOfRowsToGet)
                 != ACI_SUCCESS);

        /* BUG-37642 Improve performance to fetch. */
        sTotalNumberOfRows +=
            ulnStmtGetFetchedRowCountFromServer(aFnContext->mHandle.mStmt);

        if(ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA) // 데이터 다 가져옴.
        {
            break;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
void ulnFetchCalcPrefetchRows(ulnCache     *aCache,
                              ulnCursor    *aCursor,
                              acp_uint32_t *aNumberOfPrefetchRows,
                              acp_sint32_t *aNumberOfRowsToGet)
{
    acp_uint32_t sBlockSizeOfOneFetch;

    ACE_DASSERT(aCache != NULL);
    ACE_DASSERT(aCursor != NULL);
    ACE_DASSERT(aNumberOfPrefetchRows != NULL);
    ACE_DASSERT(aNumberOfRowsToGet != NULL);

    sBlockSizeOfOneFetch   = ulnCacheCalcBlockSizeOfOneFetch(aCache, aCursor);
    *aNumberOfPrefetchRows = ulnCacheCalcPrefetchRowSize(aCache, aCursor);

    *aNumberOfRowsToGet    = ulnCacheCalcNumberOfRowsToGet(aCache,
                                                           aCursor,
                                                           sBlockSizeOfOneFetch);

    /*
     * To fix BUG-20372
     * 사용자가 PREFETCH ATTR을 설정했다면, 설정한 값으로 PREFETCH
     * 해야 하고, 그렇지 않은 경우는 OPTIMAL 값으로 FETCH 해야 한다.
     */
    if (*aNumberOfPrefetchRows != ULN_CACHE_OPTIMAL_PREFETCH_ROWS &&
        *aNumberOfRowsToGet > 0)
    {
        *aNumberOfPrefetchRows = *aNumberOfRowsToGet;
    }
    else
    {
        /* nothing to do */
    }
}

ACI_RC ulnFetchFromCache(ulnFnContext *aFnContext,
                         ulnPtContext *aPtContext,
                         ulnStmt      *aStmt,
                         acp_uint32_t *aFetchedRowCount)
{
    acp_uint32_t  i;
    acp_sint32_t  sNumberOfRowsToGet = 0;
    acp_uint32_t  sNumberOfPrefetchRows = 0;

    acp_sint64_t  sNextFetchStart;
    acp_uint32_t  sFetchedRowCount = 0;

    ulnCursor    *sCursor     = ulnStmtGetCursor(aStmt);
    ulnCache     *sCache      = ulnStmtGetCache(aStmt);
    acp_uint32_t  sCursorSize = ulnStmtGetAttrRowArraySize(sCursor->mParentStmt);
    ulnRow       *sRow;

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    acp_bool_t    sIsSemiAsyncPrefetch = ulnDbcIsAsyncPrefetchStmt(aStmt->mParentDbc, aStmt);
    acp_bool_t    sIsAutoTuning        = ulnCanSemiAsyncPrefetchAutoTuning(aFnContext);

    sNextFetchStart = ulnCursorGetPosition(sCursor);

    for (i = 0; i < sCursorSize; i++ )
    {
        if( (sNextFetchStart < 0) ||  // AFTER_END or BEFORE_START
            ((sNextFetchStart + i) > ulnCacheGetResultSetSize(sCache)) )
        {
            /*
             * Cursor 의 위치가 AFTER END 이거나 BEFORE START 인 경우
             * Cursor + i 의 위치가 result set size 보다 클 경우 루프를 취소한다.
             */

            if (sCache->mServerError != NULL)
            {
                /*
                 * FetchResult 수신시 펜딩된 에러가 있다면 무조건 stmt 에 에러를 단다.
                 * fetch 시 Error 가 발생하면 서버에서는 더 이상 result 를 보내지 않고,
                 * fetch end 를 보내므로 result set size 가 정해진다.
                 *
                 * 즉, error 가 나는 바로 그 row 가 마지막이 되는 것이다.
                 */

                sCache->mServerError->mRowNumber = i + 1;

                ulnDiagHeaderAddDiagRec(&aStmt->mObj.mDiagHeader, sCache->mServerError);
                ULN_FNCONTEXT_SET_RC(aFnContext,
                                     ulnErrorDecideSqlReturnCode(sCache->mServerError->mSQLSTATE));

                sCache->mServerError = NULL;
                ACI_TEST(!(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext))));
            }

            break;
        }

        sRow = ulnCacheGetCachedRow(sCache, sNextFetchStart + i);

        if(sRow == NULL)
        {
            /*
             * 캐쉬 MISS
             */

            if(ulnCursorGetType(sCursor) == SQL_CURSOR_FORWARD_ONLY)
            {
                /*
                 * scrollable cursor 가 아니면,
                 *      1. 캐쉬 위치를 1 로 다시 초기화시킨다.
                 *      2. lob 이 캐쉬되어 있으면 close 한다.
                 *      3. 캐쉬의 mRowCount 를 초기화시킨다.
                 */

                // To Fix BUG-20481
                // FORWARD_ONLY이고 사용자 Buffer에 복사되었음이 보장되므로
                // Cache 영역을 무조건 삭제할 수 있다.

                // To Fix BUG-20409
                // Cursor의 Logical Position을 변경시키지 않는다.
                // ulnCursorSetPosition(sCursor, 1);

                ACI_TEST(ulnCacheCloseLobInCurrentContents(aFnContext,
                                                           aPtContext,
                                                           sCache)
                         != ACI_SUCCESS);

                /* 제거된 Cache Row의 개수만큼 StartPosition 변경 */
                ulnCacheAdjustStartPosition( sCache );

                ulnCacheInitRowCount(sCache);

                // BUG-21746 메모리를 재사용하게 한다.
                ACI_TEST(ulnCacheBackChunkToMark( sCache ) != ACI_SUCCESS);
                // 기존에 사용하고 있는 RPA를 재구성한다.
                ACI_TEST(ulnCacheReBuildRPA( sCache ) != ACI_SUCCESS);

                /* PROJ-2047 Strengthening LOB - LOBCACHE */
                ACI_TEST(ulnLobCacheReInitialize(aStmt->mLobCache)
                         != ACI_SUCCESS);
            }

            /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
            ulnFetchCalcPrefetchRows(sCache,
                                     sCursor,
                                     &sNumberOfPrefetchRows,
                                     &sNumberOfRowsToGet);

            ACI_TEST_RAISE(sNumberOfRowsToGet < 0, LABEL_MEM_MAN_ERR);

            if (sIsSemiAsyncPrefetch == ACP_FALSE)
            {
                ACI_TEST(ulnFetchMoreFromServer(aFnContext,
                                                aPtContext,
                                                (acp_uint32_t)sNumberOfRowsToGet,
                                                sNumberOfPrefetchRows)
                         != ACI_SUCCESS);
            }
            else
            {
                if (sIsAutoTuning == ACP_FALSE)
                {
                    ACI_TEST(ulnFetchMoreFromServerAsync(
                                aFnContext,
                                aPtContext,
                                (acp_uint32_t)sNumberOfRowsToGet,
                                sNumberOfPrefetchRows) != ACI_SUCCESS);
                }
                else
                {
                    ACI_TEST(ulnFetchMoreFromServerAsyncWithAutoTuning(
                                aFnContext,
                                aPtContext,
                                (acp_uint32_t)sNumberOfRowsToGet,
                                sNumberOfPrefetchRows) != ACI_SUCCESS);
                }
            }

            /*
             * RETRY
             */
            sRow = ulnCacheGetCachedRow(aStmt->mCache, sNextFetchStart + i);
        }

        /*
         * 서버에서 결과를 더 가져왔는데도 불구하고, 캐쉬 MISS
         */
        if(sRow == NULL)
        {
            /*
             * 서버에서도 더 이상 가져올 것이 없는 경우 되겠다.
             * ulnFetchUpdateAfterFetch() 함수에서 result cache 에다가 ResultsetSize 세팅했다.
             *
             * 이곳에 오는 경우는 cursor size 가 result set size 의 약수로 이루어진 경우에
             * 이미 result set size 만큼 fetch 를 다 해 온 후 (아직 SQL_NO_DATA 는 리턴 안됨)
             * 한번 더 fetch 를 하게 되면 여기 이곳에 걸리게 된다.
             *
             * SQL_NO_DATA 를 리턴해 주기 위해서 세팅을 해야 하지만,
             * 이미 ulnFetchUpdateAfterFetch() 함수에서 세팅해버린 경우이다.
             *
             * 만약 cursor size 와 result set 의 마지막 경계가 서로 겹쳐서, 예를 들어, 커서 사이즈
             * 10 인데, 6 record 만 페치되어 왔다면, 이미 ulnFetchUpdateAfterFetch() 에서
             * ulnCache 의 mResultsetSize 가 세팅이 끝났으며, 이 다음번 루프에서
             * break 되어 튀어 나가게 된다.
             */
        }
        else
        {
            /*
             * 캐쉬 HIT
             */
            ACI_TEST(ulnCacheRowCopyToUserBuffer(aFnContext,
                                                 aPtContext,
                                                 sRow,
                                                 sCache,
                                                 aStmt,
                                                 i + 1)
                     != ACI_SUCCESS);

            aStmt->mGDTargetPosition = sNextFetchStart + i;

            sFetchedRowCount++;
        }
    } /* for (cursor size) */

    *aFetchedRowCount = sFetchedRowCount;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "Number of rows to get is subzero.");
    }

    ACI_EXCEPTION_END;

    *aFetchedRowCount = sFetchedRowCount;

    return ACI_FAILURE;
}

/* PROJ-1789 Updatable Scrollable Cursor */

/**
 * 지정한 Position까지 채워지도록 서버에서 Key를 더 가져와 Keyset을 채운다.
 *
 * @param[in] aFnContext function context
 * @param[in] aPtContext protocol context
 * @param[in] aTargetPos target position
 *
 * @return ACP_SUCCESS if successful, or ACP_FAILURE otherwise
 */
ACI_RC ulnFetchMoreFromServerForKeyset(ulnFnContext *aFnContext,
                                       ulnPtContext *aPtContext)
{
    ulnStmt      *sStmt;
    ulnKeyset    *sKeyset;
    SQLRETURN     sOriginalRC;

    sStmt   = aFnContext->mHandle.mStmt;
    sKeyset = ulnStmtGetKeyset(sStmt);

    ACE_DASSERT(ulnKeysetIsFullFilled(sKeyset) != ACP_TRUE);

    sOriginalRC = ULN_FNCONTEXT_GET_RC(aFnContext);
    ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);

    ACI_TEST( ulnFetchOperationSet(aFnContext,
                                   aPtContext,
                                   0, /* 통신버퍼에 최대한 채워서 얻는다. */
                                   1,
                                   ulnStmtGetColumnCount(sStmt))
              != ACI_SUCCESS );

    if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
    {
        ulnKeysetSetFullFilled(sKeyset, ACP_TRUE);
    }
    else if (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_SUCCESS)
    {
        /* Error occured. RC 값으로 처리하므로 함수는 ACI_SUCCESS 반환. */
    }
    else
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Keyset-driven을 위한 Prefetch를 수행한다.
 *
 * @param[in] aFnContext     function context
 * @param[in] aPtContext     protocol context
 * @param[in] aStartPosition Fetch를 시작할 Position (inclusive)
 * @param[in] aFetchCount    Fetch 할 Row 개수
 * @param[in] aCacheMode     Cache를 어떻게 쌓을건지
 *
 * @return ACP_SUCCESS if successful, or ACP_FAILURE otherwise
 */
ACI_RC ulnFetchFromServerForSensitive(ulnFnContext     *aFnContext,
                                      ulnPtContext     *aPtContext,
                                      acp_sint64_t      aStartPosition,
                                      acp_uint32_t      aFetchCount,
                                      ulnStmtFetchMode  aFetchMode)
{
    ulnFnContext  sTmpFnContext;
    ulnStmt      *sKeysetStmt;
    ulnStmt      *sRowsetStmt;
    ulnKeyset    *sKeyset;
    acp_uint8_t  *sKeyValue;
    acp_uint32_t  i;
    acp_uint32_t  sActFetchCount = aFetchCount;

    ulnDescRec   *sDescRecArd;
    acp_sint64_t  sBookmark;

    sKeysetStmt  = aFnContext->mHandle.mStmt;
    sKeyset      = ulnStmtGetKeyset(sKeysetStmt);
    sRowsetStmt  = sKeysetStmt->mRowsetStmt;

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnFreeStmtClose(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);

    /* Bind */

    if (aFetchMode == ULN_STMT_FETCHMODE_BOOKMARK)
    {
        ACE_DASSERT(aStartPosition == 1); /* 내부에서만 쓰므로 항상 1 */

        sDescRecArd = ulnStmtGetArdRec(sKeysetStmt, 0);
        ACE_ASSERT(sDescRecArd != NULL); /* 위에서 확인하고 들어온다. */

        for (i = 0; i < sActFetchCount; i++)
        {
            if (ulnStmtGetAttrUseBookMarks(sKeysetStmt) == SQL_UB_VARIABLE)
            {
                sBookmark = *((acp_sint64_t *) ulnBindCalcUserDataAddr(sDescRecArd, i));
            }
            else
            {
                sBookmark = *((acp_sint32_t *) ulnBindCalcUserDataAddr(sDescRecArd, i));
            }
            sKeyValue = ulnKeysetGetKeyValue(sKeyset, sBookmark);
            ACI_TEST_RAISE(sKeyValue == NULL, INVALID_BOOKMARK_VALUE_EXCEPTION);
            ACI_TEST(ulnBindParamBody(&sTmpFnContext,
                                      i + 1,
                                      NULL,
                                      SQL_PARAM_INPUT,
                                      SQL_C_SBIGINT,
                                      SQL_BIGINT,
                                      0,
                                      0,
                                      sKeyValue,
                                      ULN_KEYSET_MAX_KEY_SIZE,
                                      NULL) != ACI_SUCCESS);
        }
    }
    else /* is ULN_STMT_FETCHMODE_ROWSET */
    {
        if ((aStartPosition + sActFetchCount - 1) > ulnKeysetGetKeyCount(sKeyset))
        {
            sActFetchCount = ulnKeysetGetKeyCount(sKeyset) - aStartPosition + 1;
        }

        for (i = 0; i < sActFetchCount; i++)
        {
            sKeyValue = ulnKeysetGetKeyValue(sKeyset, aStartPosition + i);
            ACE_ASSERT(sKeyValue != NULL);
            ACI_TEST(ulnBindParamBody(&sTmpFnContext,
                                      i + 1,
                                      NULL,
                                      SQL_PARAM_INPUT,
                                      SQL_C_SBIGINT,
                                      SQL_BIGINT,
                                      0,
                                      0,
                                      sKeyValue,
                                      ULN_KEYSET_MAX_KEY_SIZE,
                                      NULL) != ACI_SUCCESS);
        }
    }

    /* ExecDirect */

    ACI_TEST(ulnPrepBuildSelectForRowset(aFnContext, sActFetchCount)
             != ACI_SUCCESS);

    ACI_TEST(ulnPrepareCore(&sTmpFnContext, aPtContext,
                            sKeysetStmt->mQstrForRowset,
                            sKeysetStmt->mQstrForRowsetLen,
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    ACI_TEST(ulnExecuteCore(&sTmpFnContext, aPtContext) != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(&sTmpFnContext, aPtContext,
                                     sKeysetStmt->mParentDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    /* Fetch */

    ulnStmtSetFetchMode(sRowsetStmt, aFetchMode);

    /* 순서대로 쌓는게 아니기 때문에 Cache 시작점을 알아야 한다. */
    sRowsetStmt->mRowsetCacheStartPosition = aStartPosition;

    ACI_TEST(ulnFetchAllFromServer(&sTmpFnContext, aPtContext) != ACI_SUCCESS);

    if ((ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
     && (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_NO_DATA))
    {
        ulnDiagRecMoveAll( &(sKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_BOOKMARK_VALUE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BOOKMARK_VALUE);
    }
    ACI_EXCEPTION_END;

    if ((ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
     && (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_NO_DATA))
    {
        ulnDiagRecMoveAll( &(sKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));

        ulnCacheInvalidate(sRowsetStmt->mCache);
    }

    return ACI_FAILURE;
}

ACI_RC ulnFetchFromCacheForKeyset(ulnFnContext *aFnContext,
                                  ulnPtContext *aPtContext,
                                  ulnStmt      *aStmt,
                                  acp_sint64_t  aTargetPos)
{
    ulnRow       *sRow;
    acp_uint8_t   sKeyValueBuf[ULN_KEYSET_MAX_KEY_SIZE] = {0, };
    acp_uint8_t  *sKeyValuePtr = sKeyValueBuf;
    acp_uint32_t  i;
    acp_uint32_t  sFetchedKeyCount = 0;

    ulnKeyset    *sKeyset = ulnStmtGetKeyset(aStmt);
    ulnCache     *sCache  = ulnStmtGetCache(aStmt);
    acp_sint64_t  sNextKeysetStart = ulnKeysetGetKeyCount(sKeyset) + 1;

    ACI_TEST_RAISE((aTargetPos != ULN_KEYSET_FILL_ALL) &&
                   (aTargetPos < sNextKeysetStart),
                   LABEL_ALREADY_EXISTS_CONT);

    for (i = 0; (sNextKeysetStart + i) <= aTargetPos; i++)
    {
        if ( (sNextKeysetStart + i) > ulnCacheGetResultSetSize(sCache) )
        {
            if (sCache->mServerError != NULL)
            {
                sCache->mServerError->mRowNumber = i + 1;

                ulnDiagHeaderAddDiagRec(&aStmt->mObj.mDiagHeader, sCache->mServerError);
                ULN_FNCONTEXT_SET_RC(aFnContext,
                                     ulnErrorDecideSqlReturnCode(sCache->mServerError->mSQLSTATE));

                sCache->mServerError = NULL;
                ACI_TEST(!(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext))));
            }

            break;
        }

        sRow = ulnCacheGetCachedRow(sCache, sNextKeysetStart + i);
        if (sRow == NULL)
        {
            ACI_TEST(ulnFetchMoreFromServerForKeyset(aFnContext, aPtContext)
                     != ACI_SUCCESS);
            sRow = ulnCacheGetCachedRow(sCache, sNextKeysetStart + i);
        }
        ACE_DASSERT(sRow != NULL);

        // only PROWID
        CM_ENDIAN_ASSIGN8(sKeyValuePtr, sRow->mRow);

        ACI_TEST( ulnKeysetAddNewKey(sKeyset, sKeyValuePtr)
                  != ACI_SUCCESS );

        sFetchedKeyCount++;
    }

    if (sFetchedKeyCount == 0)
    {
        ulnKeysetSetFullFilled(sKeyset, ACP_TRUE);
    }

    ACI_EXCEPTION_CONT(LABEL_ALREADY_EXISTS_CONT);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Cache에서 데이타를 Fetch한다.
 * Cache가 부족하면 Cache를 채운 후 Fetch한다.
 *
 * @param[in] aFnContext       function context
 * @param[in] aPtContext       protocol context
 * @param[in] aStmt            statement handle
 * @param[in] aFetchedRowCount Fetch한 row 개수
 *
 * @return ACP_SUCCESS if successful, or ACP_FAILURE otherwise
 */
ACI_RC ulnFetchFromCacheForSensitive(ulnFnContext *aFnContext,
                                     ulnPtContext *aPtContext,
                                     ulnStmt      *aKeysetStmt,
                                     acp_uint32_t *aFetchedRowCount)
{
    ULN_FLAG(sHoleExistFlag);

    acp_uint32_t  i;
    acp_sint64_t  sNextFetchStart;
    acp_sint64_t  sNextFetchEnd;
    acp_sint64_t  sNextPrefetchStart;
    acp_sint64_t  sNextPrefetchEnd;
    acp_uint32_t  sPrefetchSize;
    acp_sint64_t  sRowCount = ULN_ROWCOUNT_UNKNOWN;

    ulnCursor    *sCursor       = ulnStmtGetCursor(aKeysetStmt);
    ulnKeyset    *sKeyset       = ulnStmtGetKeyset(aKeysetStmt);
    ulnStmt      *sRowsetStmt   = aKeysetStmt->mRowsetStmt;
    ulnCache     *sRowsetCache  = ulnStmtGetCache(sRowsetStmt);
    ulnRow       *sRow;
    acp_uint16_t *sRowStatusPtr = ulnStmtGetAttrRowStatusPtr(aKeysetStmt);

    acp_uint32_t  sFetchedRowCount = 0;

    sNextFetchStart = ulnCursorGetPosition(sCursor);
    if (sNextFetchStart < 0) /* AFTER_END or BEFORE_START */
    {
        ACI_RAISE(LABEL_CONT_NO_DATA_FOUND);
    }

    sNextFetchEnd   = sNextFetchStart + ulnCursorGetSize(sCursor) - 1;

    if (ulnKeysetIsFullFilled(sKeyset) == ACP_TRUE)
    {
        sRowCount = ulnKeysetGetKeyCount(sKeyset);
    }

    /* PROJ-1789 Updatable Scrollable Cursor
     * Sensitive일 때는 Cache를 쌓는 방법이 조금 다르다.
     * Hole을 감지해야하므로 Cach MISS가 났을 때 Prefetch를 다시하면 안되고,
     * 미리 Cache를 초기화한 후 Row 위치에 맞는곳에 값을 넣어야 한다. */
    if ((ulnCacheIsInvalidated(sRowsetCache) == ACP_TRUE)
     || (ulnCacheCheckRowsCached(sRowsetCache, sNextFetchStart, sNextFetchEnd) != ACP_TRUE))
    {
        sPrefetchSize = ulnCacheCalcBlockSizeOfOneFetch(ulnStmtGetCache(aKeysetStmt),
                                                        ulnStmtGetCursor(aKeysetStmt));
        // NextPrefetchRange 계산
        if (ulnCursorGetDirection(sCursor) == ULN_CURSOR_DIR_NEXT)
        {
            sNextPrefetchStart = sNextFetchStart;
            sNextPrefetchEnd   = sNextPrefetchStart + sPrefetchSize - 1;
        }
        else /* is ULN_CURSOR_DIR_PREV */
        {
            sNextPrefetchEnd   = sNextFetchEnd;
            sNextPrefetchStart = sNextPrefetchEnd - sPrefetchSize + 1;
        }

        if (sNextPrefetchStart > sRowCount)
        {
            ACI_RAISE(LABEL_CONT_NO_DATA_FOUND);
        }

        if (ulnKeysetIsFullFilled(sKeyset) != ACP_TRUE)
        {
            ACI_TEST(ulnFetchFromCacheForKeyset(aFnContext, aPtContext,
                                                aKeysetStmt, sNextPrefetchEnd)
                     != ACI_SUCCESS);
            sRowCount = ulnKeysetGetKeyCount(sKeyset);
        }

        /* PrefetchSize 설정 복사 */
        ulnStmtSetAttrPrefetchRows(sRowsetStmt, ulnStmtGetAttrPrefetchRows(aKeysetStmt));
        ulnStmtSetAttrPrefetchBlocks(sRowsetStmt, ulnStmtGetAttrPrefetchBlocks(aKeysetStmt));
        ulnStmtSetAttrPrefetchMemory(sRowsetStmt, ulnStmtGetAttrPrefetchMemory(aKeysetStmt));

        ACI_TEST(ulnFetchFromServerForSensitive(aFnContext, aPtContext,
                                                sNextPrefetchStart,
                                                sPrefetchSize,
                                                ULN_STMT_FETCHMODE_ROWSET)
                 != ACI_SUCCESS);
    }

    for (i = 0; i < ulnCursorGetSize(sCursor); i++ )
    {
        if( (sNextFetchStart < 0) ||  /* AFTER_END or BEFORE_START */
            ((sNextFetchStart + i) > sRowCount) )
        {
            if (sRowsetCache->mServerError != NULL)
            {
                /*
                 * FetchResult 수신시 펜딩된 에러가 있다면 무조건 stmt 에 에러를 단다.
                 * fetch 시 Error 가 발생하면 서버에서는 더 이상 result 를 보내지 않고,
                 * fetch end 를 보내므로 result set size 가 정해진다.
                 *
                 * 즉, error 가 나는 바로 그 row 가 마지막이 되는 것이다.
                 */

                sRowsetCache->mServerError->mRowNumber = i + 1;

                ulnDiagHeaderAddDiagRec(&aKeysetStmt->mObj.mDiagHeader, sRowsetCache->mServerError);
                ULN_FNCONTEXT_SET_RC(aFnContext,
                                     ulnErrorDecideSqlReturnCode(sRowsetCache->mServerError->mSQLSTATE));

                sRowsetCache->mServerError = NULL;
                ACI_TEST(!(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext))));
            }

            break;
        }

        sRow = ulnCacheGetCachedRow(sRowsetCache, sNextFetchStart + i);
        /* PROJ-1789 Updatable Scrollable Cursor */
        /* RPA를 미리 만드므로 NULL이면 안될거 같지만, CachedRow를 얻을 때
         * RowNumber가 RowCount보다 크면 NULL을 주도록 되어있으므로,
         * Rowset의 마지막 Row가 HOLE이라면 sRow 자체가 NULL이 된다. */
        if ((sRow == NULL) || (sRow->mRow == NULL))
        {
            if ((sNextFetchStart + i) <= sRowCount)
            {
                if (sRowStatusPtr != NULL)
                {
                    ulnStmtSetAttrRowStatusValue(aKeysetStmt,
                                                 i, SQL_ROW_DELETED);
                }

                /* row-status indicators가 설정되지 않았어도,
                 * 유효한 row 데이타는 사용자 버퍼에 복사해줄거다.
                 * 그를 위해, 여기서는 Hole 발생 여부만 기억해둔다. */
                ULN_FLAG_UP(sHoleExistFlag);
            }

            /*  no continue. Hole도 Fetch된 Row의 하나로 센다. */
        }
        else
        {
            ACI_TEST(ulnCacheRowCopyToUserBuffer(aFnContext, aPtContext,
                                                 sRow, sRowsetCache,
                                                 sRowsetStmt, i + 1)
                     != ACI_SUCCESS);

            aKeysetStmt->mGDTargetPosition = sNextFetchStart + i;
        }

        sFetchedRowCount++;
    } /* for (cursor size) */

    ULN_IS_FLAG_UP(sHoleExistFlag)
    {
        if (sRowStatusPtr == NULL)
        {
            ulnError(aFnContext, ulERR_IGNORE_HOLE_DETECTED_BUT_NO_INDICATOR);
        }
#if defined(USE_HOLE_DETECTED_WARNING)
        else
        {
            ulnError(aFnContext, ulERR_IGNORE_HOLE_DETECTED);
        }
#endif
    }

    ACI_EXCEPTION_CONT(LABEL_CONT_NO_DATA_FOUND);

    *aFetchedRowCount = sFetchedRowCount;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    *aFetchedRowCount = sFetchedRowCount;

    return ACI_FAILURE;
}
