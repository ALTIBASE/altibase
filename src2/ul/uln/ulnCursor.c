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

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>

#define ULN_ABS(x) ( (acp_uint32_t) (((x) < 0) ? (-(x)) : (x)) )

void ulnCursorInitialize(ulnCursor *aCursor, ulnStmt *aParentStmt)
{
    aCursor->mParentStmt            = aParentStmt;
    aCursor->mPosition              = ULN_CURSOR_POS_BEFORE_START;
    aCursor->mState                 = ULN_CURSOR_STATE_CLOSED;
    aCursor->mServerCursorState     = ULN_CURSOR_STATE_CLOSED;

    /*
     * BUGBUG : ODBC 스펙에서 지정하는 디폴트 값은 SQL_UNSPECIFIED 이지만,
     *          현재 ul 과 cm 및 mm 에서 sensitive 한 커서를 지원하지 못하기 때문에
     *          INSENSITIVE 를 기본으로 했다
     *
     *          과연.. INSENSITIVE 가 맞는지, 아니면 UNSPECIFIED 가 맞는지 모르겠다.
     */
    aCursor->mAttrCursorSensitivity = SQL_INSENSITIVE;

    /*
     * note: M$DN ODBC30 에 디폴트 값이 안나온다. 내맘대로 NON_UNIQUE 로 함
     */
    aCursor->mAttrSimulateCursor    = SQL_SC_NON_UNIQUE;

    aCursor->mAttrCursorType        = SQL_CURSOR_FORWARD_ONLY;
    aCursor->mAttrCursorScrollable  = SQL_NONSCROLLABLE;
    aCursor->mAttrConcurrency       = SQL_CONCUR_READ_ONLY;

    /* PROJ-1789 Updatable Scrollable Cursor */
    aCursor->mAttrHoldability       = SQL_CURSOR_HOLD_DEFAULT;
    aCursor->mRowsetPos             = 1;
}

acp_uint32_t ulnCursorGetSize(ulnCursor *aCursor)
{
    return ulnStmtGetAttrRowArraySize(aCursor->mParentStmt);
}

void ulnCursorSetConcurrency(ulnCursor *aCursor, acp_uint32_t aConcurrency)
{
    aCursor->mAttrConcurrency = aConcurrency;
}

acp_uint32_t ulnCursorGetConcurrency(ulnCursor *aCursor)
{
    return aCursor->mAttrConcurrency;
}

void ulnCursorSetSensitivity(ulnCursor *aCursor, acp_uint32_t aSensitivity)
{
    aCursor->mAttrCursorSensitivity = aSensitivity;
}

acp_uint32_t ulnCursorGetSensitivity(ulnCursor *aCursor)
{
    return aCursor->mAttrCursorSensitivity;
}

/*
 * ==========================================
 * Cursor OPEN / CLOSE
 * ==========================================
 */

void ulnCursorOpen(ulnCursor *aCursor)
{
    ulnCursorSetState(aCursor, ULN_CURSOR_STATE_OPEN);
    ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
}

ACI_RC ulnCursorClose(ulnFnContext *aFnContext, ulnCursor *aCursor)
{
    ULN_FLAG(sNeedFinPtContext);

    ulnDbc       *sDbc;
    ulnStmt      *sStmt;
    ulnStmt      *sRowsetStmt;
    ulnCache     *sCache;
    ulnKeyset    *sKeyset;
    acp_uint16_t  sResultSetCount;
    acp_uint16_t  sCurrentResultSetID;
    ulnFnContext  sTmpFnContext;

    sStmt = aFnContext->mHandle.mStmt;

    // To Fix BUG-18358
    // Cursor Close시 이와 관련된 Statement 자료 구조인
    // GD(GetData) Column Number도 초기화하여야 한다.
    sStmt->mGDColumnNumber = ULN_GD_COLUMN_NUMBER_INIT_VALUE;

    /*
     * uln 의 커서를 닫는다.
     */
    ulnCursorSetState(aCursor, ULN_CURSOR_STATE_CLOSED);

    sDbc = sStmt->mParentDbc;
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    sResultSetCount = ulnStmtGetResultSetCount(sStmt);
    sCurrentResultSetID = ulnStmtGetCurrentResultSetID(sStmt);

    /*
     * 서버의 커서가 아직 열려 있고, 여전히 연결된 상태일 때에만 서버로 close cursor 전송
     */
    if (ulnDbcIsConnected(sDbc) == ACP_TRUE)
    {
        // BUG-17514
        // ResultSetID는 0부터 시작하므로
        // 마지막 ResultSetID는 전체 ResultSet의 갯수보다 1 작다.
        if (ulnCursorGetServerCursorState(aCursor) == ULN_CURSOR_STATE_OPEN ||
            sCurrentResultSetID < (sResultSetCount - 1))
        {
            /*
             * 서버로 close cursor 명령 전송
             *
             * Note : 서버의 커서가 닫혀 있는 상태이거나 열러있지 않은데도 CLOSE CURSOR REQ 를
             *        서버로 전송하더라도, 서버는 그냥 무시한다.
             *
             *        단지 이것을 체크하는 이유는, I/O transaction 을 한번이라도 줄여볼까
             *        해서이다.
             */
            if (sStmt->mIsSimpleQuerySelectExecuted != ACP_TRUE)
            {
                /* PROJ-2616 */
                /* IPCDA-SimpleQuery-Execute는 커서를 사용하지 않음.
                 * 따라서, IPCDA-SimpleQuery-Execute가 아닌 경우에만 호출 함.*/
                ACI_TEST(ulnFreeHandleStmtSendFreeREQ(aFnContext, sDbc, CMP_DB_FREE_CLOSE)
                         != ACI_SUCCESS);
            }

            /* PROJ-1381, BUG-32932 FAC : Close 상태 기억 */
            ulnCursorSetServerCursorState(aCursor, ULN_CURSOR_STATE_CLOSED);
        }
    }

    /* PROJ-2616 */
    /* SimpleQuery-Select-Execute 는 lob을 지원하지 않으며,
     * cache를 사용하지 않는다.
     * SHM의 데이터를 바로 읽어서 사용자의 버퍼에 넣는다.
     * 따라서, 이 기능은 무시하기로 한다.
     */
    ACI_TEST_RAISE(sStmt->mIsSimpleQuerySelectExecuted == ACP_TRUE,
                   ContCursorClose);

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    if (ulnDbcIsAsyncPrefetchStmt(sDbc, sStmt) == ACP_TRUE)
    {
        ACI_TEST(ulnFetchEndAsync(aFnContext, &sDbc->mPtContext) != ACI_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    /*
     * 캐쉬 메모리가 존재할 경우 최초에 만들어진 상태로 초기화시켜버린다.
     * 단, 메모리 해제는 하지 않는다.
     */
    sCache = ulnStmtGetCache(sStmt);

    if (sCache != NULL)
    {
        /*
         * row 에 lob 컬럼이 있을 경우 lob column 을 찾아서 lob 들을 close 해 준다.
         */

        if (ulnCacheHasLob(sCache) == ACP_TRUE)
        {
            //fix BUG-17722
            ACI_TEST(ulnInitializeProtocolContext(aFnContext,
                                                  &(sDbc->mPtContext),
                                                  &(sDbc->mSession)) != ACI_SUCCESS);
            ULN_FLAG_UP(sNeedFinPtContext);
        }

        /*
         * BUGBUG : 잘못하면 죽겠다...
         */
        ACI_TEST(ulnCacheCloseLobInCurrentContents(aFnContext,
                                                   &(sDbc->mPtContext),
                                                   sCache)
                 != ACI_SUCCESS);

        ULN_IS_FLAG_UP(sNeedFinPtContext)
        {
            ULN_FLAG_DOWN(sNeedFinPtContext);
            //fix BUG-17722
            ACI_TEST(ulnFinalizeProtocolContext(aFnContext,&(sDbc->mPtContext) ) != ACI_SUCCESS);
        }

        /*
         * Note : CloseCursor() 는 더 이상 fetch 를 하지 않겠다는 이야기이다.
         *  fix BUG-18260  row-cache를 초기화 한다.
         */
        ACI_TEST( ulnCacheInitialize(sCache) != ACI_SUCCESS );
    }

    ACI_EXCEPTION_CONT(ContCursorClose);

    sKeyset = ulnStmtGetKeyset(sStmt);
    if (sKeyset!= NULL)
    {
        ACI_TEST_RAISE(ulnKeysetInitialize(sKeyset) != ACI_SUCCESS,
                       LABEL_MEM_MAN_ERR);
    }

    /* PROJ-2177: Fetch가 끝나면 초기화 */
    ulnStmtResetLastFetchFuncID(sStmt);

    /* PROJ-1789 Updatable Scrollable Cursor
     * Rowset Cache를 위한 Stmt Close */
    sRowsetStmt = sStmt->mRowsetStmt;
    if ( (ulnCursorGetType(&sStmt->mCursor) == SQL_CURSOR_KEYSET_DRIVEN) &&  
         (sRowsetStmt != SQL_NULL_HSTMT) &&  
         (sStmt->mIsSimpleQuerySelectExecuted != ACP_TRUE) )
    {
        ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_CLOSECURSOR,
                                  sRowsetStmt, ULN_OBJ_TYPE_STMT);

        ACI_TEST_RAISE(ulnCursorClose(&sTmpFnContext,
                                      &(sRowsetStmt->mCursor))
                       != ACI_SUCCESS, LABEL_ERRS_IN_ROWSETSTMT);
    }

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    if (sStmt->mLobCache != NULL)
    {
        ACI_TEST(ulnLobCacheReInitialize(sStmt->mLobCache) != ACI_SUCCESS);
    }
    else
    {
        /* Nothing */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnWriteProtocol : object type is neither DBC nor STMT.");
    }
    ACI_EXCEPTION(LABEL_ERRS_IN_ROWSETSTMT)
    {
        if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
        {
            ulnDiagRecMoveAll( &(sStmt->mObj), &(sRowsetStmt->mObj) );
            ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
        }
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(aFnContext, &(sDbc->mPtContext));
    }

    return ACI_FAILURE;
}

void ulnCursorSetPosition(ulnCursor *aCursor, acp_sint64_t aPosition)
{
    acp_sint64_t sResultSetSize;

    switch (aPosition)
    {
        case ULN_CURSOR_POS_BEFORE_START:
        case ULN_CURSOR_POS_AFTER_END:
            aCursor->mPosition = aPosition;
            break;

        default:
            ACE_ASSERT(aPosition > 0);

            sResultSetSize = ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache);

            if (aCursor->mPosition > sResultSetSize)
            {
                aCursor->mPosition = ULN_CURSOR_POS_AFTER_END;
            }
            else
            {
                aCursor->mPosition = aPosition;
            }
            break;
    }
}

/*
 * 몇개의 row 를 사용자 버퍼로 복사해야 하는지 계산하는 함수
 */
acp_uint32_t ulnCursorCalcRowCountToCopyToUser(ulnCursor *aCursor)
{
    acp_sint64_t sCursorPosition;
    acp_sint64_t sResultSetSize;
    acp_sint32_t sRowCountToCopyToUser;
    acp_uint32_t sCursorSize;

    sCursorPosition = ulnCursorGetPosition(aCursor);

    ACE_ASSERT(sCursorPosition != 0);

    if (sCursorPosition < 0)
    {
        /*
         * ULN_CURSOR_POS_AFTER_END or ULN_CURSOR_POS_BEFORE_START
         */
        sRowCountToCopyToUser = 0;
    }
    else
    {

        /*
         * BUGBUG : ulnCacheGetResultSetSize() 가 항상 양수라는 것을 검증하고 보장해야 한다.
         */

        sResultSetSize = ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache);
        sCursorSize    = ulnCursorGetSize(aCursor);

        /*
         * Note : row number 는 1 부터 시작한다는 것을 명심한다.
         *        물론 cursor 의 mPosition 도 1 이 첫번째 row 를 가리킨다.
         */

        if (sCursorPosition + sCursorSize - 1 > sResultSetSize)
        {
            sRowCountToCopyToUser = sResultSetSize - sCursorPosition + 1;

            /*
             * 커서 position 은 20, 서버로 페치를 시도해 보니 result set 이 16 밖에 없더라.
             * 그럴 경우 위의 식을 계산하면 음수가 나온다.
             * 사용자에게 복사해 줄 row 의 갯수는 0
             */
            if (sRowCountToCopyToUser < 0)
            {
                sRowCountToCopyToUser = 0;
            }
        }
        else
        {
            sRowCountToCopyToUser = sCursorSize;
        }
    }

    return (acp_uint32_t)sRowCountToCopyToUser;
}

/*
 * Note : 아래의 ulnCursorMoveXXX 함수들은 모두 SQLFetchScroll 의
 *        fetch orientation 의 옵션 하나씩에 해당하는 함수들이다.
 *        rowset 단위로 커서를 움직이는 함수들이다.
 */

void ulnCursorMoveAbsolute(ulnFnContext *aFnContext, ulnCursor *aCursor, acp_sint32_t aOffset)
{
    acp_uint32_t sRowsetSize;
    acp_sint64_t sResultSetSize;

    sRowsetSize    = ulnCursorGetSize(aCursor);
    sResultSetSize = ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache);

    if (aOffset < 0)
    {
        if (aOffset * (-1) <= sResultSetSize)
        {
            /*
             * FetchOffset < 0 AND | FetchOffset | <= LastResultRow
             */
            ulnCursorSetPosition(aCursor, sResultSetSize + aOffset + 1);
        }
        else
        {
            if ((acp_uint32_t)(aOffset * (-1)) > sRowsetSize)
            {
                /*
                 * FetchOffset < 0 AND
                 *      | FetchOffset | > LastResultRow AND
                 *      | FetchOffset | > RowsetSize
                 */
                ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
            }
            else
            {
                /*
                 * FetchOffset < 0 AND
                 *      | FetchOffset | > LastResultRow AND
                 *      | FetchOffset | <= RowsetSize
                 */
                ulnCursorSetPosition(aCursor, 1);

                ulnError(aFnContext, ulERR_IGNORE_FETCH_BEFORE_RESULTSET);
            }
        }
    }
    else if (aOffset == 0)
    {
        ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
    }
    else
    {
        if (aOffset <= sResultSetSize)
        {
            /*
             * 1 <= FetchOffset <= LastResultRow
             */
            ulnCursorSetPosition(aCursor, aOffset);
        }
        else
        {
            /*
             * FetchOffset > LastResultRow
             */
            ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_AFTER_END);
        }
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_NEXT);
}

void ulnCursorMoveRelative(ulnFnContext *aFnContext, ulnCursor *aCursor, acp_sint32_t aOffset)
{
    acp_sint64_t sCurrentPosition;
    acp_uint32_t sRowSetSize;
    acp_sint64_t sResultSetSize;

    sRowSetSize      = ulnCursorGetSize(aCursor);
    sCurrentPosition = ulnCursorGetPosition(aCursor);
    sResultSetSize   = ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache);

    switch(sCurrentPosition)
    {
        case ULN_CURSOR_POS_BEFORE_START:
            if (aOffset > 0)
            {
                /*
                 * (Before start AND FetchOffset > 0) OR (After end AND FetchOffset < 0)
                 */
                ulnCursorMoveAbsolute(aFnContext, aCursor, aOffset);
            }
            else
            {
                /*
                 * BeforeStart AND FetchOffset <= 0
                 */
                ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
            }
            break;

        case ULN_CURSOR_POS_AFTER_END:
            if (aOffset < 0)
            {
                /*
                 * (Before start AND FetchOffset > 0) OR (After end AND FetchOffset < 0)
                 */
                ulnCursorMoveAbsolute(aFnContext, aCursor, aOffset);
            }
            else
            {
                /*
                 * After end AND FetchOffset >= 0
                 */
                ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_AFTER_END);
            }
            break;

        default:
            ACE_ASSERT(sCurrentPosition > 0);

            if (1 <= sCurrentPosition + aOffset)
            {
                if (sCurrentPosition + aOffset <= sResultSetSize)
                {
                    /*
                     * 1 <= CurrRowsetStart + FetchOffset <= LastResultRow (M$ODBC 표의 6번째 줄)
                     */
                    ulnCursorSetPosition(aCursor, sCurrentPosition + aOffset);
                }
                else
                {
                    /*
                     * CurrRowsetStart + FetchOffset > LastResultRow (M$ ODBC 표의 7번째 줄)
                     */
                    ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_AFTER_END);
                }
            }
            else
            {
                if (sCurrentPosition == 1 && aOffset < 0)
                {
                    /*
                     * CurrRowsetStart = 1 AND FetchOffset < 0 (M$ ODBC 표의 3번째 줄)
                     */
                    ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
                }
                else
                {
                    if (1 > sCurrentPosition + aOffset)
                    {
                        if (ULN_ABS(aOffset) > sRowSetSize)
                        {
                            /*
                             * CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
                             *      | FetchOffset | > RowsetSize
                             * (M$ ODBC 표의 4번째 줄)
                             */
                            ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
                        }
                        else
                        {
                            /*
                             * CurrRowsetStart > 1 AND CurrRowsetStart + FetchOffset < 1 AND
                             *      | FetchOffset | <= RowsetSize[3]
                             * (M$ ODBC 표의 5번재 줄)
                             */
                            ulnCursorSetPosition(aCursor, 1);

                            ulnError(aFnContext, ulERR_IGNORE_FETCH_BEFORE_RESULTSET);
                        }
                    }
                    else
                    {
                        /*
                         * BUGBUG : 여기가 M$ ODBC SQLFetchScroll() 함수 설명에서 적어둔
                         *          Cursor positioning rule 에 있는 SQL_FETCH_RELATIVE 에 있는
                         *          표 상에서의 논리상의 구멍인데,
                         *          정말 구멍일까? 머리아프다. 그냥 두자. 죽을까 그냥?
                         *
                         *          구멍같아 보이지만, 실상 구멍이 아니다.
                         *              if (1 <= sCurrentPosition + aOffset) {}
                         *              else
                         *              .... if (1 > sCurrentPosition + aOffset)
                         */
                        ACE_ASSERT(0);
                    }
                }
            }
            break;
    } /* switch(sCurrentPosition) */

    /* PROJ-1789 Updatable Scrollable Cursor */
    if (aOffset < 0)
    {
        ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_PREV);
    }
    else if (aOffset > 0)
    {
        ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_NEXT);
    }
    else /* aOffset == 0 */
    {
        /* do nothing. keep prev direction. */
    }
}

void ulnCursorMoveNext(ulnFnContext *aFnContext, ulnCursor *aCursor)
{

#if 0
    /*
     * BUGBUG : 과연 똑같은가?
     */
    ulnCursorMoveRelative(aFnContext, aCursor, ulnCursorGetSize(aCursor));

#else

    acp_sint64_t sCurrentPosition;
    acp_uint32_t sRowSetSize;
    ulnStmt*     sStmt = aCursor->mParentStmt;

    ACP_UNUSED(aFnContext);

    sRowSetSize      = ulnCursorGetSize(aCursor);
    // bug-35198: row array(rowset) size가 변경된후 fetch하는 경우
    // 이전의 size를 한번은 사용해서 cursor를 움직여야 한다
    // MovePrior의 경우 고려할 필요 없다 (msdn에 나와 있음)
    if (sStmt->mPrevRowSetSize != 0)
    {
        sRowSetSize = sStmt->mPrevRowSetSize;
        sStmt->mPrevRowSetSize = 0; // 한번 사용했으면 clear
    }
    sCurrentPosition = ulnCursorGetPosition(aCursor);

    switch(sCurrentPosition)
    {
        case ULN_CURSOR_POS_BEFORE_START:
            ulnCursorSetPosition(aCursor, 1);
            break;

        case ULN_CURSOR_POS_AFTER_END:
            ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_AFTER_END);
            break;

        default:

            /*
             * BUGBUG : ulnCacheGetResultSetSize() 가 항상 양수라는 것을 검증하고 보장해야 한다.
             */

            if ( (sCurrentPosition + sRowSetSize) >
                 ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache) )
            {
                /*
                 * CurrRowsetStart + RowsetSize > LastResultRow : AFTER END
                 */
                ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_AFTER_END);
            }
            else
            {
                /*
                 * CurrRowsetStart + RowsetSize <= LastResultRow : CurrRowsetStart + RowsetSize
                 */
                ulnCursorSetPosition(aCursor, sCurrentPosition + sRowSetSize);
            }
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_NEXT);
#endif
}

void ulnCursorMovePrior(ulnFnContext *aFnContext, ulnCursor *aCursor)
{
#if 0
    /*
     * BUGBUG : 과연 똑같은가?
     */
    ulnCursorMoveRelative(aFnContext, aCursor, ulnCursorGetSize(aCursor) * (-1));

#else

    acp_sint64_t sCurrentPosition;
    acp_uint32_t sRowSetSize;

    sRowSetSize      = ulnCursorGetSize(aCursor);
    sCurrentPosition = ulnCursorGetPosition(aCursor);

    switch(sCurrentPosition)
    {
        case ULN_CURSOR_POS_BEFORE_START:
        case 1:
            ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
            break;

        case ULN_CURSOR_POS_AFTER_END:

            /*
             * BUGBUG : ulnCacheGetResultSetSize() 가 항상 양수라는 것을 검증하고 보장해야 한다.
             */

            if (sRowSetSize > ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache))
            {
                /*
                 * LastResultRow < RowsetSize : 1
                 */
                ulnCursorSetPosition(aCursor, 1);

                ulnError(aFnContext, ulERR_IGNORE_FETCH_BEFORE_RESULTSET);
            }
            else
            {
                /*
                 * LastResultRow >= RowsetSize : LastResultRow - RowsetSize + 1
                 */
                ulnCursorSetPosition(aCursor,
                                     ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache) -
                                     sRowSetSize +
                                     1);
            }
            break;

        default:
            ACE_ASSERT(sCurrentPosition > 1);

            if ((acp_uint32_t)sCurrentPosition <= sRowSetSize)
            {
                /*
                 * 1 < CurrRowsetStart <= RowsetSize : 1
                 */
                ulnCursorSetPosition(aCursor, 1);

                ulnError(aFnContext, ulERR_IGNORE_FETCH_BEFORE_RESULTSET);
            }
            else
            {
                /*
                 * CurrRowsetStart > RowsetSize : CurrRowsetStart - RowsetSize
                 */
                ulnCursorSetPosition(aCursor, sCurrentPosition - sRowSetSize);
            }
            break;
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_PREV);
#endif
}

void ulnCursorMoveFirst(ulnCursor *aCursor)
{
    ulnCursorSetPosition(aCursor, 1);

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_NEXT);
}

void ulnCursorMoveLast(ulnCursor *aCursor)
{
    acp_uint32_t sRowSetSize;
    acp_sint64_t sResultSetSize;

    sRowSetSize    = ulnCursorGetSize(aCursor);
    sResultSetSize = ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache);


    /*
     * BUGBUG : ulnCacheGetResultSetSize() 가 항상 양수라는 것을 검증하고 보장해야 한다.
     */

    if (sRowSetSize > sResultSetSize)
    {
        ulnCursorSetPosition(aCursor, 1);
    }
    else
    {
        ulnCursorSetPosition(aCursor, sResultSetSize - sRowSetSize + 1);
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_PREV);
}

/* PROJ-1789 Updatable Scrollable Cursor */

/**
 * Bookmark를 기준으로 커서를 움직인다.
 *
 * @param[in] aFnContext function context
 * @param[in] aCursor    커서
 * @param[in] aOffset    Bookmark row로 부터의 상대 위치
 */
void ulnCursorMoveByBookmark(ulnFnContext *aFnContext, ulnCursor *aCursor, acp_sint32_t aOffset)
{
    acp_sint64_t sResultSetSize;
    acp_sint64_t sBookmarkPosition;
    acp_sint64_t sTargetPosition;

    ACP_UNUSED(aFnContext);

    sResultSetSize = ulnCacheGetResultSetSize(aCursor->mParentStmt->mCache);
    sBookmarkPosition = ulnStmtGetAttrFetchBookmarkVal(aCursor->mParentStmt);
    sTargetPosition = sBookmarkPosition + aOffset;

    if (sTargetPosition < 1)
    {
        /* BookmarkRow + FetchOffset < 1 */
        ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_BEFORE_START);
    }
    else if (sTargetPosition > sResultSetSize)
    {
        /* BookmarkRow + FetchOffset > LastResultRow  */
        ulnCursorSetPosition(aCursor, ULN_CURSOR_POS_AFTER_END);
    }
    else /* if ((1 <= sTargetPosition) && (sTargetPosition <= sResultSetSize)) */
    {
        /* 1 <= BookmarkRow + FetchOffset <= LastResultRow  */
        ulnCursorSetPosition(aCursor, sTargetPosition);
    }

    /* PROJ-1789 Updatable Scrollable Cursor */
    ulnCursorSetDirection(aCursor, ULN_CURSOR_DIR_NEXT);
}

/**
 * 커서 방향을 얻는다.
 *
 * @param[in] aCursor cursor object
 *
 * @return 커서 방향
 */
ulnCursorDir ulnCursorGetDirection(ulnCursor *aCursor)
{
    return aCursor->mDirection;
}

/**
 * 커서 방향을 설정한다.
 *
 * @param[in] aCursor    cursor object
 * @param[in] aDirection 커서 방향
 */
void ulnCursorSetDirection(ulnCursor *aCursor, ulnCursorDir aDirection)
{
    aCursor->mDirection = aDirection;
}

acp_bool_t ulnCursorHasNoData(ulnCursor *aCursor)
{
    ulnStmt    * sStmt   = aCursor->mParentStmt;
    acp_bool_t   sResult = ACP_FALSE;

    /* SELECT가 아닌 경우, Result Set Count가 0 이다.
     * SQLCloseCursor()를 수행한 경우, ulnCursorGetState()가 ULN_CURSOR_STATE_CLOSED를 반환한다.
     */
    if ( ( ulnStmtGetResultSetCount( sStmt ) == 0 ) ||
         ( ulnCursorGetState( aCursor ) == ULN_CURSOR_STATE_CLOSED ) )
    {
        sResult = ACP_TRUE;
    }
    else
    {
        /* Result Set이 여러 개인 경우, 마지막 Result Set인지 확인한다. (SQLMoreResults)
         * 현재 Result Set을 모두 사용했는지 확인한다. (SQLFetch)
         */
        if ( ( ulnStmtGetCurrentResultSetID( sStmt ) >= ( ulnStmtGetResultSetCount( sStmt ) - 1 ) ) &&
             ( ulnCursorGetServerCursorState( aCursor ) == ULN_CURSOR_STATE_CLOSED ) )
        {
            sResult = ACP_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sResult;
}
