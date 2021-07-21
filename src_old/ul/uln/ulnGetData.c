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
#include <ulnGetData.h>
#include <ulnConv.h>

/*
 * ULN_SFID_29
 * SQLGetData(), STMT, S6, S7
 *
 *      -- [s] or [nf]
 *      S11 [x]
 *      24000 [b]
 *      HY109 [i]
 *
 * where
 *      [i]  Invalid row. The cursor was positioned on a row in the result set and
 *           either the row had been deleted or an error had occurred in an operation
 *           on the row. If the row status array existed, the value in the row status array
 *           for the row was SQL_ROW_DELETED or SQL_ROW_ERROR.
 *           (The row status array is pointed to by the SQL_ATTR_ROW_STATUS_PTR
 *           statement attribute.)
 *      [b]  Before or after.
 *           The cursor was positioned before the start of the result set or
 *           after the end of the result set.
 *      [nf] Not found.
 *           The function returned SQL_NO_DATA.
 *           This does not apply when SQLExecDirect, SQLExecute, or SQLParamData returns
 *           SQL_NO_DATA after executing a searched update or delete statement.
 */
ACI_RC ulnSFID_29(ulnFnContext *aContext)
{
    switch (aContext->mWhere)
    {
        case ULN_STATE_ENTRY_POINT:
            break;

        case ULN_STATE_EXIT_POINT:
            break;

        default:
            ACE_ASSERT(0);
    }

    return ACI_SUCCESS;
}

ACI_RC ulnGDInitColumn(ulnFnContext *aFnContext, acp_uint16_t aColumnNumber)
{
    ulnRow         *sRow;
    ulnColumn      *sColumn;
    ulnCursor      *sCursor;
    ulnCache       *sCache;
    ulnStmt        *sStmt = aFnContext->mHandle.mStmt;

    sCursor = ulnStmtGetCursor(sStmt);
    ACI_TEST_RAISE(ulnCursorGetPosition(sCursor) < 0, LABEL_INVALID_CURSOR_POSITION);

    /* PROJ-1789 Updatable Scrollable Cursor : 데이타는 RowsetStmt에 있다. */
    if (ulnStmtGetAttrCursorType(sStmt) == SQL_CURSOR_KEYSET_DRIVEN)
    {
        sCache = ulnStmtGetCache(sStmt->mRowsetStmt);
    }
    else
    {
        sCache = ulnStmtGetCache(sStmt);
    }
    ACI_TEST_RAISE(sCache == NULL, LABEL_MEM_MANAGE_ERR_CACHE);

    /* Keyset-Driven이면 Hole, Invalidate로 인해 Cache가 비어있을 수 있다. */
    if (ulnStmtGetAttrCursorType(sStmt) != SQL_CURSOR_KEYSET_DRIVEN)
    {
        sRow = ulnCacheGetCachedRow(sCache, ulnCursorGetPosition(sCursor) + ulnCursorGetRowsetPosition(sCursor) - 1);
        ACI_TEST_RAISE(sRow == NULL, LABEL_MEM_MANAGE_ERR_ROW);
    }

    sColumn = ulnCacheGetColumn(sCache, aColumnNumber);
    ACI_TEST_RAISE(sColumn == NULL, LABEL_MEM_MANAGE_ERR_COLUMN);

    sColumn->mGDState      = ULN_GD_ST_INITIAL;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_POSITION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_CURSOR_POSITION_GD);
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_CACHE)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGDInitColumn : ulnCache was NULL");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_ROW)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGDInitColumn : ulnRow was NULL");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_COLUMN)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGDInitColumn : ulnColumn was NULL");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnGDProcessOnTheFly(ulnFnContext     *aFnContext,
                                   ulnColumn        *aColumn,
                                   ulnAppBuffer     *aAppBuffer,
                                   ulnIndLenPtrPair *aUserIndLenPair)
{
    ULN_FLAG(sNeedFinPtContext);
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    void         *sBackupArg;

    /*
     * Note : 이곳의 protocol context 초기화는 없애서는 안된다.
     *        LOB 을 GetData() 로 받아올 때 conversion 루틴 안에서 
     *        사용하기 때문이다.
     *
     *        LOB 루틴에 protocol context 를 심은 후에
     *        이곳의 불필요한 protocol context 초기화를 없애야 한다.
     */
    //fix BUG-17722 
    ACI_TEST(ulnInitializeProtocolContext(aFnContext,
                                          &(sStmt->mParentDbc->mPtContext),
                                          &(sStmt->mParentDbc->mSession)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedFinPtContext);

    sBackupArg = aFnContext->mArgs;
    aFnContext->mArgs =  &(sStmt->mParentDbc->mPtContext);

    ACI_TEST(ulnConvert(aFnContext,
                        aAppBuffer,
                        aColumn,
                        1, /* aUserRowNumber, */
                        aUserIndLenPair) != ACI_SUCCESS);

    aFnContext->mArgs = sBackupArg;

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(aFnContext,
                  &(sStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(aFnContext, &(sStmt->mParentDbc->mPtContext));
    }

    return ACI_FAILURE;
}

static ACI_RC ulnGetDataMain(ulnFnContext *aFnContext,
                             acp_uint16_t  aColumnNumber,
                             acp_sint16_t  aTargetType,
                             void         *aTargetValuePtr,
                             ulvSLen       aBufferLength,
                             ulvSLen      *aIndPtr)
{
    ulnStmt         *sStmt       = aFnContext->mHandle.mStmt;
    ulnDescRec      *sDescRecIrd = NULL;
    ulnDescRec      *sDescRecArd = NULL;

    ulnCursor       *sCursor     = NULL;
    ulnCache        *sCache      = NULL;
    ulnRow          *sRow        = NULL;
    ulnColumn       *sColumn     = NULL;
    acp_uint8_t     *sSrc        = NULL;
    ulnAppBuffer     sAppBuffer;
    ulnIndLenPtrPair sUserIndLenPair;
    acp_sint64_t     sTargetPosition;
    acp_sint32_t     sAdjustColCnt;
    acp_uint32_t     sColumnCount;
    acp_uint32_t     i;

    sCursor = ulnStmtGetCursor(sStmt);
    sTargetPosition = ulnCursorGetPosition(sCursor);
    ACI_TEST_RAISE(sTargetPosition < 0, LABEL_INVALID_CURSOR_POSITION_AE_OR_BS);

    /* PROJ-1789 Updatable Scrollable Cursor: SQLSetPos()로 설정한 값 참조 */
    sTargetPosition = sTargetPosition + ulnCursorGetRowsetPosition(sCursor) - 1;

    /* PROJ-1789 Updatable Scrollable Cursor: 데이타는 RowsetStmt에 있다. */
    if (ulnStmtGetAttrCursorType(sStmt) == SQL_CURSOR_KEYSET_DRIVEN)
    {
        sCache = ulnStmtGetCache(sStmt->mRowsetStmt);
    }
    else
    {
        sCache = ulnStmtGetCache(sStmt);
    }
    ACI_TEST_RAISE(sCache == NULL, LABEL_MEM_MANAGE_ERR_CACHE);

    if (ulnStmtGetAttrCursorScrollable(sStmt) == SQL_SCROLLABLE)
    {
        sAdjustColCnt = -1;
    }
    else
    {
        sAdjustColCnt = 0;
    }

    ACI_TEST_RAISE(ulnCacheIsInvalidated(sCache) == ACP_TRUE,
                   LABEL_INVALID_CURSOR_POSITION);

    sRow = ulnCacheGetCachedRow(sCache, sTargetPosition);
    if ((sRow == NULL) || (sRow->mRow == NULL))
    {
        if (ulnStmtGetAttrCursorType(sStmt) == SQL_CURSOR_KEYSET_DRIVEN)
        {
            /* PROJ-1789 Updatable Scrollable Cursor
             * Hole인 Row의 데이타를 얻으려고 하면 에러 */
            ACI_RAISE(LABEL_INVALID_CURSOR_POSITION);
        }
        else
        {
            ACI_RAISE(LABEL_MEM_MANAGE_ERR_ROW);
        }
    }

    /*
     * ColumnNumber 의 범위는 이미 체크된 상태임.
     */

    /* PROJ-1789 Rowset Position이 바꼈다면, Column를 다시 만들어야 한다. */
    if (sTargetPosition != sStmt->mGDTargetPosition)
    {
        sSrc = sRow->mRow;
        sColumnCount = ulnStmtGetColumnCount(sStmt);
        for (i = 1 + sAdjustColCnt; i <= sColumnCount; i++)
        {
            sColumn = ulnCacheGetColumn(sCache, i);
            ACI_TEST_RAISE(sColumn == NULL, LABEL_MEM_MANAGE_ERR_COLUMN);

            sColumn->mGDPosition    = 0;
            sColumn->mRemainTextLen = 0;
            sColumn->mGDState       = ULN_GD_ST_INITIAL;
            sColumn->mBuffer        = (acp_uint8_t*)sCache->mColumnBuffer
                                    + (i * ULN_CACHE_MAX_SIZE_FOR_FIXED_TYPE);

            if( sColumn->mColumnNumber ==0 )
            {
                ulnDataBuildColumnZero( aFnContext, sRow, sColumn);
            }
            else
            {
                ACI_TEST(ulnDataBuildColumnFromMT(aFnContext,
                                                  sSrc,
                                                  sColumn)
                         != ACI_SUCCESS );

                sSrc += sColumn->mMTLength;
            }
        }
        sStmt->mGDTargetPosition = sTargetPosition;
    }

    sColumn = ulnCacheGetColumn(sCache, aColumnNumber);

    /* PROJ-1789 Updatable Scrollable Cursor: Bookmark는 Ird가 없다. */
    if (aColumnNumber != 0)
    {
        sDescRecIrd = ulnStmtGetIrdRec(sCache->mParentStmt, aColumnNumber);
        ACI_TEST_RAISE(sDescRecIrd == NULL, LABEL_MEM_MANAGE_ERR_IRD);
    }

    // fix BUG-24381 SQL_ARD_TYPE을 지원해야 합니다.
    if (aTargetType == SQL_ARD_TYPE)
    {
        sDescRecArd = ulnStmtGetArdRec(sStmt, aColumnNumber);
        ACI_TEST_RAISE(sDescRecArd == NULL, LABEL_MEM_MANAGE_ERR_IRD);

        // ARD에 설정된 값으로 세팅
        sAppBuffer.mCTYPE = ulnMetaGetCTYPE(&sDescRecArd->mMeta);
    }
    else
    {
        // 사용자로부터 받은 값으로 세팅
        sAppBuffer.mCTYPE = ulnTypeMap_SQLC_CTYPE(aTargetType);
    }

    ACI_TEST_RAISE(sAppBuffer.mCTYPE == ULN_CTYPE_MAX, LABEL_INVALID_C_TYPE);

    sAppBuffer.mBuffer       = (acp_uint8_t *)aTargetValuePtr;
    sAppBuffer.mBufferSize   = aBufferLength;
    sAppBuffer.mColumnStatus = ULN_ROW_SUCCESS;

    sUserIndLenPair.mIndicatorPtr = aIndPtr;
    sUserIndLenPair.mLengthPtr    = aIndPtr;

    if (sColumn->mGDState == ULN_GD_ST_INITIAL)
    {
        /*
         * 데이터를 다 가져간 컬럼에 대한 재호출
         */

        ACI_TEST_RAISE(aColumnNumber == sStmt->mGDColumnNumber, LABEL_NO_DATA);

        /*
         * 최초의 GD 호출 : ulnColumn 의 GetData State 를 "읽고있음" 으로 바꾼다.
         */

        sStmt->mGDColumnNumber = aColumnNumber;
        sColumn->mGDState      = ULN_GD_ST_RETRIEVING;
    }

    ACI_TEST_RAISE(aColumnNumber != sStmt->mGDColumnNumber, LABEL_MEM_MANAGE_ERR);

    ACI_TEST(ulnGDProcessOnTheFly(aFnContext,
                                  sColumn,
                                  &sAppBuffer,
                                  &sUserIndLenPair) != ACI_SUCCESS);

    /* BUG-32474 */
    if ((sAppBuffer.mCTYPE == ULN_CTYPE_BLOB_LOCATOR)
     || (sAppBuffer.mCTYPE == ULN_CTYPE_CLOB_LOCATOR))
    {
        ACI_TEST_RAISE(ulnCacheAddReadLobLocator(sCache, (acp_uint64_t*)sAppBuffer.mBuffer)
                       != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR);
#ifdef COMPILE_SHARDCLI
            /* PROJ-2739 Client-side Sharding LOB */
            ACI_TEST_RAISE( ulsdLobAddFetchLocator(
                                sStmt,
                                (acp_uint64_t *)sAppBuffer.mBuffer)
                            != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR );
#endif
    }

    if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS)
    {
        /*
         * SQL_SUCCESS 를 리턴한다는 것은 다 읽었다는 뜻이므로
         * ulnColumn 의 GetData State 를 초기상태로 바꾼다. 그래서 다음번에 같은 컬럼에다가
         * 호출하면 SQL_NO_DATA 가 리턴되도록 한다.
         */

        sColumn->mGDState = ULN_GD_ST_INITIAL;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_C_TYPE)
    {
        /* HY003 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aTargetType);
    }

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_POSITION_AE_OR_BS)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_CURSOR_POSITION_GD);
    }

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_POSITION)
    {
        ulnError(aFnContext, ulERR_ABORT_Invalid_Cursor_Position_Error);
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGetDataMain.");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_IRD)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGetDataMain : no IRD record");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_CACHE)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGetDataMain : ulnCache was NULL");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_ROW)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGetDataMain : ulnRow was NULL");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR_COLUMN)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnGetDataMain : ulnColumn was NULL");
    }

    ACI_EXCEPTION(LABEL_NO_DATA)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NO_DATA);
    }

    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

SQLRETURN ulnGetData(ulnStmt      *aStmt,
                     acp_uint16_t  aColumnNumber,
                     acp_sint16_t  aTargetType,
                     void         *aTargetValuePtr,
                     ulvSLen       aBufferLength,
                     ulvSLen      *aIndPtr)
{
    ULN_FLAG(sNeedExit);
    ulnFnContext  sFnContext;
    acp_uint32_t  sAttrUseBookMarks;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETDATA, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Note : ODBC 에 보면 아래와 같은 이야기가 있다 :
     *
     *        SQLGetData does not interact directly with any descriptor fields.
     *
     *        즉, SQLGetData() 는 ARD 를 안만든다는 뜻이다 !!
     *        따라서 SQLBindCol() 을 이용해서 바인드한 컬럼을
     *        SQLGetData() 로 다른 타입으로 fetch 해도
     *        다음번에 호출되는 SQLFetch() 사이클에서는
     *        SQLBindCol() 로 바인드한 타입이 리턴된다는 뜻이다 !!
     */

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedExit);

    /*
     * ------------------
     * Check Args
     * ------------------
     */

    /* PROJ-1789 Updatable Scrollable Cursor */
    if (aColumnNumber == 0)
    {
        sAttrUseBookMarks = ulnStmtGetAttrUseBookMarks(aStmt);
        ACI_TEST_RAISE(sAttrUseBookMarks == SQL_UB_OFF,
                       LABEL_INVALID_DESCRIPTOR_INDEX);

        ACI_TEST_RAISE((aTargetType != SQL_C_BOOKMARK) &&
                       (aTargetType != SQL_C_VARBOOKMARK) &&
                       (aTargetType != SQL_C_DEFAULT) &&
                       (aTargetType != SQL_ARD_TYPE),
                       LABEL_PROGRAM_TYPE_OUT_OF_RANGE);

        ACI_TEST_RAISE((sAttrUseBookMarks == SQL_UB_VARIABLE) &&
                       (aTargetType != SQL_C_VARBOOKMARK),
                       LABEL_PROGRAM_TYPE_OUT_OF_RANGE);
        ACI_TEST_RAISE((sAttrUseBookMarks != SQL_UB_VARIABLE) &&
                       (aTargetType == SQL_C_VARBOOKMARK),
                       LABEL_PROGRAM_TYPE_OUT_OF_RANGE);
    }
    else
    {
        ACI_TEST_RAISE(aColumnNumber > ulnStmtGetColumnCount(aStmt),
                       LABEL_INVALID_DESCRIPTOR_INDEX);
    }

    /*
     * GetData 수행
     */
    ACI_TEST(ulnGetDataMain(&sFnContext,
                            aColumnNumber,
                            aTargetType,
                            aTargetValuePtr,
                            aBufferLength,
                            aIndPtr) != ACI_SUCCESS);

    /*
     * ------------------
     * Exit
     * ------------------
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_HIGH, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" ctype: %3"ACI_INT32_FMT
            " buf: %p len: %3"ACI_INT32_FMT"]", "ulnGetData",
            aColumnNumber, aTargetType,
            aTargetValuePtr, (acp_sint32_t)aBufferLength);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_DESCRIPTOR_INDEX)
    {
        /* 07009 */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aColumnNumber);
    }
    ACI_EXCEPTION(LABEL_PROGRAM_TYPE_OUT_OF_RANGE)
    {
        ulnError(&sFnContext, ulERR_ABORT_PROGRAM_TYPE_OUT_OF_RANGE);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" ctype: %3"ACI_INT32_FMT
            " buf: %p len: %3"ACI_INT32_FMT"] fail", "ulnGetData",
            aColumnNumber, aTargetType,
            aTargetValuePtr, (acp_sint32_t)aBufferLength);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
