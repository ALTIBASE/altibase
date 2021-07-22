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
#include <ulnGetStmtAttr.h>
#include <ulnSetStmtAttr.h>

/*
 * ULN_SFID_36
 * SQLGetStmtAttr(), STMT, S1, S2-S3, S4, S5
 *      -- [1]
 *      (24000) [2]
 *  where
 *      [1] The statement attribute was not SQL_ATTR_ROW_NUMBER.
 *      [2] The statement attribute was SQL_ATTR_ROW_NUMBER.
 */
ACI_RC ulnSFID_36(ulnFnContext *aFnContext)
{
    acp_sint32_t sAttribute;

    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        sAttribute = *(acp_sint32_t *)(aFnContext->mArgs);

        ACI_TEST_RAISE( sAttribute == SQL_ATTR_ROW_NUMBER, LABEL_INVALID_CURSOR_STATE );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_STATE)
    {
        /* [2] : 24000 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_CURSOR_STATE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_37
 * SQLGetStmtAttr(), STMT, S6
 *      --[1] or ([v] and [2])
 *      24000 [b] and [2]
 *      HY109 [i] and [2]
 *  where
 *      [1] The statement attribute was not SQL_ATTR_ROW_NUMBER.
 *      [2] The statement attribute was SQL_ATTR_ROW_NUMBER.
 *      [b] Before or after.
 *          The cursor was positioned before the start of the result set or
 *          after the end of the result set.
 *      [v] Valid row.
 *          The cursor was positioned on a row in the result set,
 *          and the row had been successfully inserted, successfully updated,
 *          or another operation on the row had been successfully completed.
 *          If the row status array existed, the value in the row status array for the row was
 *          SQL_ROW_ADDED, SQL_ROW_SUCCESS, or SQL_ROW_UPDATED. (The row status array
 *          is pointed to by the SQL_ATTR_ROW_STATUS_PTR statement attribute.)
 */
ACI_RC ulnSFID_37(ulnFnContext *aFnContext)
{
    ACP_UNUSED(aFnContext);

    /*
     * BUGBUG: TODO Must be implemented!!!
     */

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_38
 * SQLGetStmtAttr(), STMT, S7
 *      -- [i] or ([v] and [2])
 *      24000 [b] and [2]
 *      HY109[1] and [2]
 *  where
 *      [1] The statement attribute was not SQL_ATTR_ROW_NUMBER.
 *      [2] The statement attribute was SQL_ATTR_ROW_NUMBER.
 *      [i] Invalid row.
 *          The cursor was positioned on a row in the result set and either the row
 *          had been deleted or an error had occurred in an operation on the row.
 *          If the row status array existed, the value in the row status array for the row
 *          was SQL_ROW_DELETED or SQL_ROW_ERROR. (The row status array is pointed to
 *          by the SQL_ATTR_ROW_STATUS_PTR statement attribute.)
 *      [v] Valid row.
 *          The cursor was positioned on a row in the result set,
 *          and the row had been successfully inserted, successfully updated,
 *          or another operation on the row had been successfully completed.
 *          If the row status array existed, the value in the row status array for the row was
 *          SQL_ROW_ADDED, SQL_ROW_SUCCESS, or SQL_ROW_UPDATED. (The row status array
 *          is pointed to by the SQL_ATTR_ROW_STATUS_PTR statement attribute.)
 */
ACI_RC ulnSFID_38(ulnFnContext *aFnContext)
{
    ACP_UNUSED(aFnContext);

    /*
     * BUGBUG: TODO Must be implemented!!!
     */

    return ACI_SUCCESS;
}

static void ulnGetStmtAttrReturnLengthToUser(acp_sint32_t *aStringLengthPtr, acp_sint32_t aLengthToReturn)
{
    if (aStringLengthPtr != NULL)
    {
        *aStringLengthPtr = aLengthToReturn;
    }
}

/*
 * Note : When the Attribute parameter has one of the following values,
 *        a 64-bit value is returned in *ValuePtr:
 *
 *          SQL_ATTR_APP_PARAM_DESC  --------+
 *          SQL_ATTR_APP_ROW_DESC            |
 *          SQL_ATTR_IMP_PARAM_DESC          |
 *          SQL_ATTR_IMP_ROW_DESC            +--> 포인터 타입. 신경쓰지 않아도 됨.
 *          SQL_ATTR_PARAM_BIND_OFFSET_PTR   |
 *          SQL_ATTR_ROW_BIND_OFFSET_PTR     |
 *          SQL_ATTR_ROWS_FETCHED_PTR -------+
 *
 *          SQL_ATTR_MAX_LENGTH  : 지원하지 않음
 *          SQL_ATTR_KEYSET_SIZE : 지원하지 않음
 *          SQL_ATTR_MAX_ROWS    : 지원하지 않음
 *
 *          SQL_ATTR_ROW_ARRAY_SIZE
 *          SQL_ATTR_ROW_NUMBER
 */

SQLRETURN ulnGetStmtAttr(ulnStmt      *aStmt,
                         acp_sint32_t  aAttribute,
                         void         *aValuePtr,
                         acp_sint32_t  aBufferLength,
                         acp_sint32_t *aStringLengthPtr)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    ulnFnContext  sFnContext;
    ulnDesc      *sDesc = NULL;
    ulnDbc       *sSrcDbc  = NULL;

    ACP_UNUSED(aBufferLength);

    // bug-20729
    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETSTMTATTR, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&aAttribute)) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    /*
     * Optional Feature Not Implemented 를 리턴할 속성들의 목록을 먼저 체크.
     */
    ACI_TEST(ulnStmtAttrCheckUnsupportedAttr(&sFnContext, aAttribute) != ACI_SUCCESS);

    /*
     * BUGBUG: TODO enter state transition
     */

    switch(aAttribute)
    {
        case SQL_ATTR_APP_PARAM_DESC:
            sDesc = ulnStmtGetApd(aStmt);
            ACI_TEST_RAISE(sDesc == NULL, LABEL_MEM_MAN_ERR);
            *(ulnDesc **)aValuePtr = sDesc;

            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(void *));
            break;

        case SQL_ATTR_APP_ROW_DESC:
            sDesc = ulnStmtGetArd(aStmt);
            ACI_TEST_RAISE(sDesc == NULL, LABEL_MEM_MAN_ERR);
            *(ulnDesc **)aValuePtr = sDesc;

            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(void *));
            break;

        case SQL_ATTR_IMP_PARAM_DESC:
            sDesc = ulnStmtGetIpd(aStmt);
            ACI_TEST_RAISE(sDesc == NULL, LABEL_MEM_MAN_ERR);
            /*
             * PROJ-1697: SQLSetDescField or SQLSetDescRec에서 DescRec(PRECISION)을 수정할 경우,
             * 이를 stmt에 반영하기 위함
             */
            sDesc->mStmt = (void*)aStmt;
            *(ulnDesc **)aValuePtr = sDesc;

            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(void *));
            break;

        case SQL_ATTR_IMP_ROW_DESC:
            sDesc = ulnStmtGetIrd(aStmt);
            ACI_TEST_RAISE(sDesc == NULL, LABEL_MEM_MAN_ERR);
            *(ulnDesc **)aValuePtr = sDesc;

            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(void *));
            break;

            /*
             * Note: 아래의 네 속성 (SQL_ATTR_CONCURRENCY, SQL_ATTR_CURSOR_SCROLLABLE,
             *       SQL_ATTR_CURSOR_SENSITIVITY, SQL_ATTR_CURSOR_TYPE) 은 하나를 손대면
             *       다른 것도 consistency 유지를 위해서 함께 바뀌어 줘야 한다.
             *       필요하다면 다른 속성도 손댈 수 있다. consistency 유지를 위해서
             *        -- refer to M$DN ODBC Cursor Characteristics and Cursor Type section
             */
        case SQL_ATTR_CONCURRENCY:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrConcurrency(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_CURSOR_SCROLLABLE:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrCursorScrollable(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_CURSOR_SENSITIVITY:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrCursorSensitivity(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_CURSOR_TYPE:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrCursorType(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
            *(ulvULen **)aValuePtr = ulnStmtGetAttrParamBindOffsetPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulvULen *));
            break;

        case SQL_ATTR_PARAM_BIND_TYPE:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrParamBindType(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_PARAM_OPERATION_PTR:
            *(acp_uint16_t **)aValuePtr = ulnStmtGetAttrParamOperationPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint16_t *));
            break;

        case SQL_ATTR_PARAM_STATUS_PTR:
            *(acp_uint16_t **)aValuePtr = ulnStmtGetAttrParamStatusPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint16_t *));
            break;

        case SQL_ATTR_PARAMS_PROCESSED_PTR:
            *(ulvULen **)aValuePtr = ulnStmtGetAttrParamsProcessedPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulvULen *));
            break;

        case SQL_ATTR_PARAMSET_SIZE:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrParamsetSize(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_QUERY_TIMEOUT:
            ulnStmtGetAttrQueryTimeout(aStmt, (acp_uint32_t *)aValuePtr);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_RETRIEVE_DATA:
            ulnStmtGetAttrRetrieveData(aStmt, (acp_uint32_t *)aValuePtr);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_ROW_ARRAY_SIZE:
            /*
             * Note : 64비트 odbc 에 따르면 필드가 64비트를 지원해야 하는데,
             *        어떤 -_-;; 사람이 20억개 이상의 array 를 써서 바인딩 하겠누 -_-;;
             *
             *        그냥 내부에서는 acp_uint32_t 로 하고, SQLSet/GetStmtAttr 함수들에서 캐스팅만 하자.
             */
            *(ulvULen *)aValuePtr = (ulvULen)ulnStmtGetAttrRowArraySize(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulvULen));
            break;

        case SQL_ATTR_ROW_BIND_OFFSET_PTR:
            *(ulvULen **)aValuePtr = ulnStmtGetAttrRowBindOffsetPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulvULen *));
            break;

        case SQL_ATTR_ROW_BIND_TYPE:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrRowBindType(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case SQL_ATTR_ROW_NUMBER:
            /*
             * 24000: Invalid cursor state.
             * BUGBUG: must check the following condition :
             *         the cursor was positioned before the start of the result set
             *         or after the end of the result set
             */
                
            ACI_TEST_RAISE( ulnStmtIsCursorOpen(aStmt) == ACP_FALSE, LABEL_INVALID_CURSOR_STATE );

            /*
             * BUGBUG: must check HY019
             */

            /*
             * ulnStmt[Set|Get]AttrRowNumber.
             *
             * SQL_ATTR_ROW_NUMBER.
             * The number of the current row in the entire result set.
             * If the number of the current row cannot be determined or there's no current row,
             * it should be 0.
             */
            *(ulvULen *)aValuePtr = ULN_vULEN(0);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulvULen));
            break;

        case SQL_ATTR_ROW_OPERATION_PTR:
            *(acp_uint16_t **)aValuePtr = ulnStmtGetAttrRowOperationPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint16_t *));
            break;

        case SQL_ATTR_ROW_STATUS_PTR:
            *(acp_uint16_t **)aValuePtr = ulnStmtGetAttrRowStatusPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint16_t *));
            break;

        case SQL_ATTR_ROWS_FETCHED_PTR:
            *(ulvULen **)aValuePtr = ulnStmtGetAttrRowsFetchedPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulvULen *));
            break;

        case SQL_ATTR_INPUT_NTS:
            if (ulnStmtGetAttrInputNTS(aStmt) == ACP_TRUE)
            {
                *(acp_uint32_t *)aValuePtr = SQL_TRUE;
            }
            else
            {
                *(acp_uint32_t *)aValuePtr = SQL_FALSE;
            }
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;
        // bug-20730
        case SQL_ROWSET_SIZE:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetRowSetSize(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;
        /* bug-18246 */
        case ALTIBASE_STMT_ATTR_TOTAL_AFFECTED_ROWCOUNT:
            *(acp_uint64_t *)aValuePtr = aStmt->mTotalAffectedRowCount;
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint64_t));
            break;
        // PROJ-1518
        case ALTIBASE_STMT_ATTR_ATOMIC_ARRAY:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAtomicArray(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        /* PROJ-1381 Fetch Across Commits */
        case SQL_ATTR_CURSOR_HOLD:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrCursorHold(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        /* PROJ-1789 Updatable Scrollable Cursor */

        case SQL_ATTR_FETCH_BOOKMARK_PTR:
            *(acp_uint8_t **)aValuePtr = ulnStmtGetAttrFetchBookmarkPtr(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint8_t *));
            break;

        case SQL_ATTR_USE_BOOKMARKS:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrUseBookMarks(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
        case SQL_ATTR_PREFETCH_ROWS:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchRows( aStmt );
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        case SQL_ATTR_PREFETCH_BLOCKS:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchBlocks( aStmt );
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        case SQL_ATTR_PREFETCH_MEMORY:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchMemory( aStmt );
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
        case ALTIBASE_PREFETCH_ASYNC:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchAsync(aStmt);
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        case ALTIBASE_PREFETCH_AUTO_TUNING:
            *(acp_uint32_t *)aValuePtr = (acp_uint32_t)ulnStmtGetAttrPrefetchAutoTuning(aStmt);
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        case ALTIBASE_PREFETCH_AUTO_TUNING_INIT:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchAutoTuningInit(aStmt);
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        case ALTIBASE_PREFETCH_AUTO_TUNING_MIN:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchAutoTuningMin(aStmt);
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        case ALTIBASE_PREFETCH_AUTO_TUNING_MAX:
            *(acp_uint32_t *)aValuePtr = ulnStmtGetAttrPrefetchAutoTuningMax(aStmt);
            ulnGetStmtAttrReturnLengthToUser( aStringLengthPtr, ACI_SIZEOF( acp_uint32_t ) );
            break;

        /* BUG-44858 */
        case ALTIBASE_PREPARE_WITH_DESCRIBEPARAM:
            *(acp_uint32_t *)aValuePtr = (acp_uint32_t)ulnStmtGetAttrPrepareWithDescribeParam(aStmt);
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(acp_uint32_t));
            break;

        case ALTIBASE_UNUSED_05: /* ALTIBASE_SHARD_DIST_TX_INFO for NATC */
            /* BUG-49065 */
            if (aStmt->mParentDbc->mShardDbcCxt.mParentDbc != NULL)
            {
                sSrcDbc = aStmt->mParentDbc->mShardDbcCxt.mParentDbc;
            }
            else
            {
                sSrcDbc = aStmt->mParentDbc;
            }
            ((ulnShardDistTxInfoStruct *)aValuePtr)->mEnvSCN          = sSrcDbc->mParentEnv->mSCN;
            ((ulnShardDistTxInfoStruct *)aValuePtr)->mSCN             = sSrcDbc->mSCN;
            ((ulnShardDistTxInfoStruct *)aValuePtr)->mTxFirstStmtSCN  = sSrcDbc->mTxFirstStmtSCN;
            ((ulnShardDistTxInfoStruct *)aValuePtr)->mTxFirstStmtTime = sSrcDbc->mTxFirstStmtTime;
            ((ulnShardDistTxInfoStruct *)aValuePtr)->mDistLevel       = sSrcDbc->mDistLevel;
            ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, ACI_SIZEOF(ulnShardDistTxInfoStruct));
            break;

        default:
            ACI_RAISE(LABEL_INVALID_ATTR_OPTION);
            break;
    }

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "GetStmtAttr");
    }
    ACI_EXCEPTION(LABEL_INVALID_CURSOR_STATE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_CURSOR_STATE);
    }
    ACI_EXCEPTION(LABEL_INVALID_ATTR_OPTION)
    {
        /*
         * HY092 : Invalid attribute/option identifier
         */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aAttribute);
        ulnGetStmtAttrReturnLengthToUser(aStringLengthPtr, 0);
    }
    ACI_EXCEPTION_END;

    if (sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}


