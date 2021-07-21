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

/*
 * ======================================
 * Fetch 되어 온 데이터를 처리하는 함수들
 * ======================================
 */

ACI_RC ulnCallbackFetchResult(cmiProtocolContext *aProtocolContext,
                              cmiProtocol        *aProtocol,
                              void               *aServiceSession,
                              void               *aUserContext)
{
    ulnFnContext  *sFnContext = (ulnFnContext*)aUserContext;
    ulnDbc        *sDbc;
    ulnStmt       *sStmt      = sFnContext->mHandle.mStmt;
    ulnCache      *sCache     = ulnStmtGetCache(sStmt);
    acp_uint8_t   *sRow;
    acp_uint32_t   sRowSize;
    acp_sint64_t   sPosition;
    acp_sint64_t   sPRowID;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    CMI_RD4(aProtocolContext, &sRowSize);

    ACI_TEST_RAISE(sCache == NULL, LABEL_MEM_MANAGE_ERR);

    ACI_TEST_RAISE( ulnCacheAllocChunk( sCache,
                                        sRowSize,
                                        &sRow )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM );

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        ACI_TEST_RAISE( cmiSplitReadIPCDA(aProtocolContext,
                                          sRowSize,
                                          &sRow,
                                          sRow) != ACI_SUCCESS,
                        cm_error );
        sStmt->mCacheIPCDA.mRemainingLength = sRowSize;
    }
    else
    {
        ACI_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                      sRowSize,
                                      sRow,
                                      sDbc->mConnTimeoutValue )
                        != ACI_SUCCESS, cm_error );
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    if (sStmt->mParentStmt != NULL)
    {
        /* _PROWID를 기준으로 쌓으므로 Cache 시작점을 알아야 한다. */
        ulnCacheSetStartPosition( sCache, sStmt->mRowsetCacheStartPosition );

        // only read _PROWID
        CM_ENDIAN_ASSIGN8(&sPRowID, sRow);

        sPosition = ulnKeysetGetSeq( sStmt->mParentStmt->mKeyset,
                                     (acp_uint8_t *) &sPRowID);
        ACE_ASSERT(sPosition > 0);

        ACI_TEST(ulnCacheSetRPByPosition( sCache,
                                          sRow + ACI_SIZEOF(acp_sint64_t),
                                          sPosition )
                 != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST_RAISE( ulnCacheAddNewRP(sCache, sRow)
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM );
    }

    sStmt->mFetchedRowCountFromServer += 1;

    /**
     * PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning
     *
     *   5 = OpID(1) + row size(4)
     *
     *   +---------+-------------+---------------------------------+
     *   | OpID(1) | row size(4) | row data(row size)              |
     *   +---------+-------------+---------------------------------+
     *
     *   Caution : mFetchedBytesFromServer isn't contained CM header (16).
     */
    sStmt->mFetchedBytesFromServer += sRowSize + 5;

    ulnCacheInitInvalidated(sCache);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnCallbackFetchResult : Cache is NULL");
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnCallbackFetchResult");
    }
    ACI_EXCEPTION(cm_error)
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        ACI_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          sDbc->mConnTimeoutValue )
                        != ACI_SUCCESS, cm_error );
    }

    /*
     * Note : ACI_SUCCESS 를 리턴하는 것은 버그가 아니다.
     *        cm 의 콜백함수가 ACI_FAILURE 를 리턴하면 communication error 로
     *        취급되어 버리기 때문에 언제나 ACI_SUCCESS 를 리턴해야 한다.
     *
     *        어찌되었던 간에, Function Context 의 멤버인 mSqlReturn 에 함수 리턴값이
     *        저장되게 될 것이며, uln 의 cmi 매핑 함수인 ulnReadProtocol() 함수 안에서
     *        Function Context 의 mSqlReturn 을 체크해서 적절한 조치를 취하게 될 것이다.
     */
    return ACI_SUCCESS;
}

ACI_RC ulnCallbackFetchBeginResult(cmiProtocolContext *aProtocolContext,
                                   cmiProtocol        *aProtocol,
                                   void               *aServiceSession,
                                   void               *aUserContext)
{
    ulnFnContext  *sFnContext = (ulnFnContext*)aUserContext;
    ulnStmt       *sStmt      = sFnContext->mHandle.mStmt;
    ulnCache      *sCache     = ulnStmtGetCache(sStmt);
    ulnKeyset     *sKeyset    = ulnStmtGetKeyset(sStmt);
    ulnDescRec    *sDescRecIrd;

    acp_uint16_t   sColumnNumber;
    acp_sint16_t   sColumnNumberAdjust;
    acp_uint16_t   sColumnCount    = ulnStmtGetColumnCount(sStmt);
    acp_uint32_t   sColumnPosition;
    acp_uint32_t   sColumnDataSize = 0;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_SKIP_READ_BLOCK( aProtocolContext, 6);

    /*
     * Note : 이 콜백은 서버에서 완전히 새로운 fetch 가 시작되었을 때에만 호출된다.
     *        따라서, 이미 할당한 메모리만 그대로 두고 캐쉬를 완전히 초기화 해버려야 한다.
     *
     *        여태까지의 fetch 된 결과는 다 버린다.
     *
     *        캐쉬에 매달린 ulnLob 들은 CloseCursor() 하거나, 캐쉬 미스가 발생하면 free 하므로
     *        여기서 신경쓰지 않아도 된다.
     */
    ACI_TEST( ulnCacheInitialize(sCache) != ACI_SUCCESS );

    if (sKeyset != NULL)
    {
        ACI_TEST(ulnKeysetInitialize(sKeyset) != ACI_SUCCESS );
    }

    if (sStmt->mParentStmt != NULL)
    {
        ACI_TEST(ulnCacheRebuildRPAForSensitive(sStmt, sCache)
                 != ACI_SUCCESS);
    }

    /*
     * column info 를 유지할 배열을 준비한다.
     */
    ACI_TEST_RAISE(ulnCachePrepareColumnInfoArray(sCache, sColumnCount)
                   != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    /*
     * 캐쉬의 컬럼 형상과 블록 할당 정보를 갱신한다.
     */

    sColumnPosition = 0;

    /* PROJ-1789 Updatable Scrollable Cursor
     * Bookmark 처리를 위해 0번째 컬럼 정보를 초기화 해야한다.
     * 여기서 MType은 USE_BOOKMARK에 따라 바뀔 수 있는데, 일단 BIGINT로 해둔다.
     * (@seealso ulnDataBuildColumnZero) */
    ulnCacheSetColumnInfo(sCache, 0, ULN_MTYPE_BIGINT, 0);

    /* PROJ-1789 Updatable Scrollable Cursor
     * Keyset-driven이면 첫번째 컬럼으로 _PROWID가 추가된다. 이걸 떼어내야한다. */
    if (sStmt->mParentStmt != SQL_NULL_HSTMT) /* is RowsetStmt ? */
    {
        sColumnNumber       = 2;
        sColumnNumberAdjust = -1;
    }
    else
    {
        sColumnNumber       = 1;
        sColumnNumberAdjust = 0;
    }
    for (; sColumnNumber <= sColumnCount; sColumnNumber++)
    {
        /*
         * BUG-28623 [CodeSonar]Null Pointer Dereference
         * sDescRecIrd에 대한 NULL 검사가 없음
         * 따라서 ACI_TEST 검사 추가
         */
        sDescRecIrd = ulnDescGetDescRec(sStmt->mAttrIrd, sColumnNumber);
        ACI_TEST( sDescRecIrd == NULL );

        ulnCacheSetColumnInfo(sCache, sColumnNumber + sColumnNumberAdjust,
                              ulnMetaGetMTYPE(&sDescRecIrd->mMeta),
                              sColumnPosition);

        sColumnDataSize = sDescRecIrd->mMeta.mOctetLength;

        sColumnPosition += ACP_ALIGN8(ACI_SIZEOF(ulnColumn)) + ACP_ALIGN8(sColumnDataSize);
    }

    ulnCacheSetSingleRowSize(sCache,
                             ACP_ALIGN8(ACI_SIZEOF(ulnRow)) + ACP_ALIGN8(sColumnPosition));

    /*
     * 커서 상태 변경
     */
    ulnCursorSetServerCursorState(&sStmt->mCursor, ULN_CURSOR_STATE_OPEN);

    ulnCacheInitInvalidated(sCache);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "CallbackFetchBeginResult");
    }

    ACI_EXCEPTION_END;

    /*
     * Note : ACI_SUCCESS 를 리턴하는 것은 버그가 아니다.
     *        cm 의 콜백함수가 ACI_FAILURE 를 리턴하면 communication error 로 취급되어 버리기
     *        때문이다.
     *
     *        어찌되었던 간에, Function Context 의 멤버인 mSqlReturn 에 함수 리턴값이
     *        저장되게 될 것이며, uln 의 cmi 매핑 함수인 ulnReadProtocol() 함수 안에서
     *        Function Context 의 mSqlReturn 을 체크해서 적절한 조치를 취하게 될 것이다.
     */
    return ACI_SUCCESS;
}

ACI_RC ulnCallbackFetchEndResult(cmiProtocolContext *aProtocolContext,
                                 cmiProtocol        *aProtocol,
                                 void               *aServiceSession,
                                 void               *aUserContext)
{
    ulnFnContext  *sFnContext = (ulnFnContext*)aUserContext;
    ulnStmt       *sStmt      = sFnContext->mHandle.mStmt;
    ulnCache      *sCache     = ulnStmtGetCache(sStmt);
    ulnCursor     *sCursor    = ulnStmtGetCursor(sStmt);

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_SKIP_READ_BLOCK( aProtocolContext, 6);

    /* PROJ-1381, BUG-32902 FAC
     * Holdable Statement는 CloseCursor 전까지 임의로 끝내지 않는다. */
    /* PROJ-1789 Updatable Scrollable Cursor
     * Keyset-driven도 CloseCursor 전까지 임의로 끝내지 않는다. */
    if ((ulnCursorGetHoldability(sCursor) != SQL_CURSOR_HOLD_ON)
     && (ulnCursorGetType(sCursor) != SQL_CURSOR_KEYSET_DRIVEN))
    {
        ulnCursorSetServerCursorState(sCursor, ULN_CURSOR_STATE_CLOSED);
    }

    if (sCache != NULL)
    {
        // To Fix BUG-20409
        ulnCacheSetResultSetSize(sCache, ulnCacheGetTotalRowCnt(sCache));
    }

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackFetchMoveResult(cmiProtocolContext *aProtocolContext,
                                  cmiProtocol        *aProtocol,
                                  void               *aServiceSession,
                                  void               *aUserContext )
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

