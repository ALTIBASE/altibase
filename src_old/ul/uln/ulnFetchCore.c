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
#include <ulnCursor.h>
#include <ulnFetch.h>
#include <ulnGetData.h>

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnSemiAsyncPrefetch.h>

static ACI_RC ulnFetchCoreRetrieveAllRowsFromServer(ulnFnContext *aFnContext,
                                                    ulnPtContext *aPtContext)
{
    SQLRETURN  sOriginalRC;

    /*
     * Trick ; 비록 무식-_- 하지만,
     * 일단 서버에서 모든 데이터를 다 받은 후
     */

    sOriginalRC = ULN_FNCONTEXT_GET_RC(aFnContext);

    ACI_TEST(ulnFetchAllFromServer(aFnContext, aPtContext) != ACI_SUCCESS);

    /*
     * ulnFetchAllFromServer() 함수는 일단 다 페치해 오면 SQL_NO_DATA 를 이용해서
     * 모두 페치해 왔는지의 여부를 판단하므로
     * 페치해 온 결과가 있어도 SQL_NO_DATA 로 세팅되어 있다.
     * 원상복구해 줘야 한다.
     */

    if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_NO_DATA)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, sOriginalRC);
    }

    /*
     * 이 함수 호출 후 커서의 위치를 재조정 해 줘야 한다.
     * 즉, 한번 더 LAST 혹은 ABSOLUTE(x) 로 커서를 옮긴다. 그리고서 아래쪽의
     * ulnFetchFromCache() 함수로 캐쉬된 record 들을 돌려준다.
     *
     * BUGBUG : 불행히도 result set 의 크기가 ACP_SINT32_MAX 인 경우,
     *          버그의 소지가 있으나 테스트 해 보지 못했다. 다시한번 생각해 보면 버그가
     *          없을 것 같기도 하고... rowset size 만큼 더해서 서버로 request
     *          하므로...
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnFetchCoreForIPCDASimpleQuery(ulnFnContext *aFnContext,
                                       ulnStmt      *aStmt,
                                       acp_uint32_t *aFetchedRowCount)
{
    ulnDescRec         *sDescRecArd  = NULL;
    ulnDescRec         *sDescRecIrd  = NULL;
    acp_uint8_t        *sSrc         = NULL;

    acp_uint16_t        sColumnId    = 0;
    acp_uint32_t        sDataRowSize = 0;
    acp_uint32_t        sNextDataPos = 0;

    *aFetchedRowCount = 0;

    ACI_TEST(aStmt->mCacheIPCDA.mFetchBuffer == NULL);

    sSrc = aStmt->mCacheIPCDA.mFetchBuffer;

    if (aStmt->mCacheIPCDA.mRemainingLength == 0)
    {
        /*더 이상 읽을 데이터가 없음*/
    }
    else
    {
        sSrc += aStmt->mCacheIPCDA.mReadLength;

        // 로우의 전체 길이
        sDataRowSize = *((acp_uint32_t*)sSrc);
        sSrc +=8;
        aStmt->mCacheIPCDA.mReadLength      += 8;
        aStmt->mCacheIPCDA.mRemainingLength -= 8;

        // 다음 데이터의 위치
        sNextDataPos = aStmt->mCacheIPCDA.mReadLength + sDataRowSize - 8;

        while (aStmt->mCacheIPCDA.mReadLength < sNextDataPos)
        {
            /* read ColumnId */
            sColumnId = *((acp_uint16_t*)sSrc);
            sSrc += 8;

            sDescRecArd = ulnDescGetDescRec(aStmt->mAttrArd, sColumnId);
            sDescRecIrd = ulnDescGetDescRec(aStmt->mAttrIrd, sColumnId);
            ACI_TEST(sDescRecIrd == NULL);

            ACI_TEST(ulnCopyToUserBufferForSimpleQuery(aFnContext,
                                                       aStmt,
                                                       sSrc,
                                                       sDescRecArd,
                                                       sDescRecIrd)
                     != ACI_SUCCESS );

            sSrc += sDescRecIrd->mMaxByteSize;
            aStmt->mCacheIPCDA.mReadLength      += (sDescRecIrd->mMaxByteSize + 8);
            aStmt->mCacheIPCDA.mRemainingLength -= (sDescRecIrd->mMaxByteSize + 8);
        }
        *aFetchedRowCount = 1;
    }

    if (*aFetchedRowCount == 0)
    {
        /*
         * BUGBUG : ulnFetchFromCache() 함수 주석을 보면 아래와 같이 적혀 있다 :
         *
         * SQL_NO_DATA 를 리턴해 주기 위해서 세팅을 해야 하지만,
         * 이미 ulnFetchUpdateAfterFetch() 함수에서 세팅해버린 경우이다.
         */
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NO_DATA);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_FNCONTEXT_SET_RC(aFnContext, SQL_ERROR);

    return ACI_FAILURE;
}

ACI_RC ulnCheckFetchOrientation(ulnFnContext *aFnContext,
                                acp_sint16_t  aFetchOrientation)
{
    ulnStmt     *sStmt = aFnContext->mHandle.mStmt;

    /*
     * FORWARD ONLY 커서일 때 FETCH_NEXT 말고 다른것으로 호출하면 에러.
     */
    if ( (ulnCursorGetType(&sStmt->mCursor) == SQL_CURSOR_FORWARD_ONLY)
      || (ulnCursorGetScrollable(&sStmt->mCursor) == SQL_NONSCROLLABLE) )
    {
        ACI_TEST_RAISE(aFetchOrientation != SQL_FETCH_NEXT,
                       LABEL_FETCH_TYPE_OUT_OF_RANGE);
    }

    /* PROJ-1789 Updatable Scrollable Cursor
     * SQL_FETCH_BOOKMARK일 때, 관련 속성을 제대로 설정했나 확인 */
    if (aFetchOrientation == SQL_FETCH_BOOKMARK)
    {
        ACI_TEST_RAISE(ulnStmtGetAttrUseBookMarks(sStmt) == SQL_UB_OFF,
                       LABEL_FETCH_TYPE_OUT_OF_RANGE);

        ACI_TEST_RAISE( ulnStmtGetAttrFetchBookmarkVal(sStmt) <= 0,
                        LABEL_INVALID_BOOKMARK_VALUE );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FETCH_TYPE_OUT_OF_RANGE)
    {
        /*
         * HY106 : Fetch type out of range
         */
        ulnError(aFnContext, ulERR_ABORT_WRONG_FETCH_TYPE);
    }
    ACI_EXCEPTION(LABEL_INVALID_BOOKMARK_VALUE)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BOOKMARK_VALUE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnFetchCore(ulnFnContext *aFnContext,
                    ulnPtContext *aPtContext,
                    acp_sint16_t  aFetchOrientation,
                    ulvSLen       aFetchOffset,
                    acp_uint32_t *aNumberOfRowsFetched)
{
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    ulnDescRec   *sDescRecArd;
    acp_sint16_t  sOdbcConciseType;
    acp_uint32_t  sAttrUseBookMarks;
    acp_sint32_t  sFetchOffset;

    sDescRecArd = ulnStmtGetArdRec(sStmt, 0);
    if (sDescRecArd != NULL)
    {
        sOdbcConciseType = ulnMetaGetOdbcConciseType(&sDescRecArd->mMeta);
        sAttrUseBookMarks = ulnStmtGetAttrUseBookMarks(sStmt);

        ACI_TEST_RAISE(sAttrUseBookMarks == SQL_UB_OFF,
                       LABEL_INVALID_DESCRIPTOR_INDEX_0);

        ACI_TEST_RAISE((sOdbcConciseType == SQL_C_BOOKMARK) &&
                       (sAttrUseBookMarks == SQL_UB_VARIABLE),
                       LABEL_RESTRICTED_DATATYPE_VIOLATION_BOOKMARK_TYPE);

        ACI_TEST_RAISE((sOdbcConciseType == SQL_C_VARBOOKMARK) &&
                       (sAttrUseBookMarks != SQL_UB_VARIABLE),
                       LABEL_RESTRICTED_DATATYPE_VIOLATION_BOOKMARK_TYPE);
    }

    ACI_TEST_RAISE(aFetchOffset > ACP_SINT32_MAX, LABEL_FETCH_OFFSET_OUT_OF_RANGE);
    ACI_TEST_RAISE(aFetchOffset < ACP_SINT32_MIN, LABEL_FETCH_OFFSET_OUT_OF_RANGE);

    sFetchOffset = aFetchOffset;

    /*
     * GetData() 를 위한 초기화 : Fetch 를 부를때마다 일어나야 한다.
     *
     * 만약 사용자가 GetData() 로 데이터를 조금만 가져가고
     * 다음 fetch 를 부르고, 그 후 getdata() 를 하는 경우를 대비함.
     */

    if (sStmt->mGDColumnNumber != ULN_GD_COLUMN_NUMBER_INIT_VALUE)
    {
        ACI_TEST(ulnGDInitColumn(aFnContext, sStmt->mGDColumnNumber) != ACI_SUCCESS);
        sStmt->mGDColumnNumber = ULN_GD_COLUMN_NUMBER_INIT_VALUE;
    }
    sStmt->mGDTargetPosition = ULN_GD_TARGET_POSITION_INIT_VALUE;

    /*
     * ===============================================
     * step 1. 커서를 사용자가 지정한 위치로 옮긴다.
     * ===============================================
     */

    switch(aFetchOrientation)
    {
        case SQL_FETCH_NEXT:
            ulnCursorMoveNext(aFnContext, &sStmt->mCursor);
            break;

        case SQL_FETCH_PRIOR:
            ulnCursorMovePrior(aFnContext, &sStmt->mCursor);
            break;

        case SQL_FETCH_RELATIVE:
            ulnCursorMoveRelative(aFnContext, &sStmt->mCursor, sFetchOffset);
            break;

        case SQL_FETCH_ABSOLUTE:
            /* PROJ-1789 Updatable Scrollable Cursor */
            if (ulnCursorGetType(&sStmt->mCursor) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                if (sFetchOffset < 0)
                {
                    ACI_TEST(ulnFetchFromCacheForKeyset(aFnContext, aPtContext,
                                                        sStmt,
                                                        ULN_KEYSET_FILL_ALL)
                             != ACI_SUCCESS);
                }
                else if (sFetchOffset > 0)
                {
                    ACI_TEST(ulnFetchFromCacheForKeyset(aFnContext, aPtContext,
                                                        sStmt,
                                                        sFetchOffset)
                             != ACI_SUCCESS);
                }
                else /* sFetchOffset == 0 */
                {
                    /* do nothing */
                }
            }
            else /* is STATIC */
            {
                // fix BUG-17715
                if (sFetchOffset < 0)
                {
                    ACI_TEST(ulnFetchCoreRetrieveAllRowsFromServer(aFnContext,
                                                                   aPtContext)
                             != ACI_SUCCESS);
                }
            }

            ulnCursorMoveAbsolute(aFnContext, &sStmt->mCursor, sFetchOffset);
            break;

        case SQL_FETCH_FIRST:
            ulnCursorMoveFirst(&sStmt->mCursor);
            break;

        case SQL_FETCH_LAST:
            //ulnCursorMoveLast(&sStmt->mCursor);

            /* PROJ-1789 Updatable Scrollable Cursor */
            if (ulnCursorGetType(&sStmt->mCursor) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                ACI_TEST(ulnFetchFromCacheForKeyset(aFnContext, aPtContext,
                                                    sStmt,
                                                    ULN_KEYSET_FILL_ALL)
                         != ACI_SUCCESS);
            }
            else /* is STATIC */
            {
                ACI_TEST(ulnFetchCoreRetrieveAllRowsFromServer(aFnContext,
                                                               aPtContext)
                         != ACI_SUCCESS);
            }

            ulnCursorMoveLast(&sStmt->mCursor);
            break;

        case SQL_FETCH_BOOKMARK:
            ulnCursorMoveByBookmark(aFnContext, &sStmt->mCursor, sFetchOffset);
            break;

        default:
            ACI_RAISE(LABEL_FETCH_TYPE_OUTOF_RANGE);
            break;
    }

    // bug-35198: row array(rowset) size가 변경된후 fetch하는 경우.
    // cursor가 옮겨졌다면 prev rowset size는 더이상 사용해서는 안됨.
    // ex) FETCH_LAST 한 후에 FETCH_NEXT하는 경우 중간에 clear 해야함
    sStmt->mPrevRowSetSize = 0;

    /*
     * ===============================================
         * step 2. 캐쉬에서 fetch해 온다.
     * ===============================================
     */

    ulnDescSetRowsProcessedPtrValue(sStmt->mAttrIrd, 0);

    if (ulnCursorGetType(&sStmt->mCursor) == SQL_CURSOR_KEYSET_DRIVEN)
    {
        ACI_TEST(ulnFetchFromCacheForSensitive(aFnContext, aPtContext,
                                               sStmt, aNumberOfRowsFetched)
                 != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnFetchFromCache(aFnContext, aPtContext,
                                   sStmt, aNumberOfRowsFetched)
                 != ACI_SUCCESS);
    }

    if (*aNumberOfRowsFetched == 0)
    {
        /*
         * BUGBUG : ulnFetchFromCache() 함수 주석을 보면 아래와 같이 적혀 있다 :
         *
         *          SQL_NO_DATA 를 리턴해 주기 위해서 세팅을 해야 하지만,
         *          이미 ulnFetchUpdateAfterFetch() 함수에서 세팅해버린 경우이다.
         */
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NO_DATA);
    }

    /* PROJ-2177: Cancel 후 상태전이를 위해, 마지막으로 Fetch한 함수를 기억해둔다. */
    ulnStmtSetLastFetchFuncID(sStmt, aFnContext->mFuncID);

    /* PROJ-1789 Rowset Position은 Fetch하면 초기화된다. */
    ulnCursorSetRowsetPosition(ulnStmtGetCursor(sStmt), 1);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_DESCRIPTOR_INDEX_0)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, 0);
    }
    ACI_EXCEPTION(LABEL_RESTRICTED_DATATYPE_VIOLATION_BOOKMARK_TYPE)
    {
        ulnError(aFnContext, ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION_BOOKMARK_TYPE);
    }
    ACI_EXCEPTION(LABEL_FETCH_TYPE_OUTOF_RANGE)
    {
        /* HY106 */
        ulnError(aFnContext, ulERR_ABORT_WRONG_FETCH_TYPE);
    }
    ACI_EXCEPTION(LABEL_FETCH_OFFSET_OUT_OF_RANGE)
    {
        ulnError(aFnContext, ulERR_ABORT_FETCH_OFFSET_OUT_OF_RANGE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
