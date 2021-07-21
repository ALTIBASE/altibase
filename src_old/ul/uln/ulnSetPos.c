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

/**
 * ULN_SFID_49
 *
 * SQLSetPos(), STMT, S6-S7
 *
 * related state :
 *      -- [s]
 *      S8 [d]
 *      S11 [x]
 *      24000 [b]
 *      HY109 [i]
 *
 * where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *      [x]  Executing. The function returned SQL_STILL_EXECUTING.
 *      [b]  Before or after.
 *           The cursor was positioned before the start of the result set or
 *           after the end of the result set.
 *      [i]  Invalid row. The cursor was positioned on a row in the result set and
 *           either the row had been deleted or an error had occurred in an operation
 *           on the row. If the row status array existed, the value in the row status array
 *           for the row was SQL_ROW_DELETED or SQL_ROW_ERROR.
 *           (The row status array is pointed to by the SQL_ATTR_ROW_STATUS_PTR
 *           statement attribute.)
 *
 * @param[in] aFnContext funnction context
 *
 * @return always ACI_SUCCESS
 */
ACI_RC ulnSFID_49(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        /* do nothing */
    }
    else /* is ULN_STATE_EXIT_POINT */
    {
        switch (ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            /* [s] */
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                /* 기존 state 유지 */
                break;

            /* [d] */
            case SQL_NEED_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                break;

            /* [x] */
            case SQL_STILL_EXECUTING:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S11);
                break;

            default:
                /* do nothing */
                break;
        }
    }

    return ACI_SUCCESS;
}

/**
 * 지정한 행을 REFRESH 한다.
 *
 * 만약, RowNumber가 0이면 모든 행을 REFRESH 한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 * @param[in] aRowNumber  작업을 수행할 Rowset Position
 * @param[in] aLockType   Not Used and Ignored
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnSetPosRefresh(ulnFnContext *aFnContext,
                        ulnPtContext *aPtContext,
                        ulnStmt      *aKeysetStmt,
                        acp_uint16_t  aRowNumber,
                        acp_uint16_t  aLockType)
{
    ULN_FLAG(sHoleExistFlag);

    acp_sint64_t    sNextFetchStart;
    acp_uint32_t    sUserRowNumberStart;
    acp_sint32_t    sFetchCount;
    acp_sint32_t    i;

    ulnCursor      *sCursor       = ulnStmtGetCursor(aKeysetStmt);
    ulnStmt        *sRowsetStmt;
    ulnCache       *sRowsetCache;
    ulnRow         *sRow;
    acp_uint16_t   *sRowStatusPtr = ulnStmtGetAttrRowStatusPtr(aKeysetStmt);

    ACP_UNUSED(aLockType);

    if (aRowNumber == 0)
    {
        sNextFetchStart = ulnCursorGetPosition(sCursor);
        sFetchCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
        sUserRowNumberStart = 1;
    }
    else
    {
        sNextFetchStart = ulnCursorGetPosition(sCursor) + aRowNumber - 1;
        sFetchCount = 1;
        sUserRowNumberStart = aRowNumber;
    }

    /* Sensitive이면 새 데이타를 Fetch, 아니면 Cache에 있는걸 반환 */
    if (ulnStmtGetAttrCursorSensitivity(aKeysetStmt) == SQL_SENSITIVE)
    {
        sRowsetStmt = aKeysetStmt->mRowsetStmt;

        ulnStmtSetAttrPrefetchRows(sRowsetStmt, sFetchCount);

        ACI_TEST(ulnFetchFromServerForSensitive(aFnContext, aPtContext,
                                                sNextFetchStart,
                                                sFetchCount,
                                                ULN_STMT_FETCHMODE_ROWSET)
                 != ACI_SUCCESS);
    }
    else
    {
        sRowsetStmt = aKeysetStmt;
    }
    sRowsetCache = ulnStmtGetCache(sRowsetStmt);

    for (i = 0; i < sFetchCount; i++)
    {
        sRow = ulnCacheGetCachedRow(sRowsetCache, sNextFetchStart + i);
        if ((sRow == NULL) || (sRow->mRow == NULL))
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt,
                                         sUserRowNumberStart + i - 1,
                                         SQL_ROW_DELETED);

            /* row-status indicators가 설정되지 않았어도,
             * 유효한 row 데이타는 사용자 버퍼에 복사해줄거다.
             * 그를 위해, 여기서는 Hole 발생 여부만 기억해둔다. */
            ULN_FLAG_UP(sHoleExistFlag);
        }
        else
        {
            ACI_TEST(ulnCacheRowCopyToUserBuffer(aFnContext, aPtContext,
                                                 sRow, sRowsetCache,
                                                 sRowsetStmt,
                                                 sUserRowNumberStart + i)
                     != ACI_SUCCESS);

            aKeysetStmt->mGDTargetPosition = sNextFetchStart + i;
        }
    } /* for (RowCouunt) */

    if ((aKeysetStmt != sRowsetStmt)
     && (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_SUCCESS)
     && (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_NO_DATA))
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
    }

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

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if ((aKeysetStmt != sRowsetStmt)
     && (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_SUCCESS)
     && (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_NO_DATA))
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
    }

    if (sRowStatusPtr != NULL)
    {
        for (i = 0; i < sFetchCount; i++)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sUserRowNumberStart + i - 1, SQL_ROW_ERROR);
        }
    }

    return ACI_FAILURE;
}

/* BUGBUG: KEYSET_DRIVEN일 때만 정상 동작한다.
 *         FORWARD_ONLY/STATIC으로는 UPDATABLE을 지원하지 않기 때문. */
/**
 * 지정한 행을 UPDATE 한다.
 *
 * 만약, RowNumber가 0이면 모든 행을 UPDATE 한다.
 * 단, 다음에 해당하는 행 또는 열은 무시한다.
 * - SQL_ATTR_ROW_OPERATION_PTR에 SQL_ROW_IGNORE로 지정한 행
 * - IndPtr에 SQL_COLUMN_IGNORE로 지정한 열
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 * @param[in] aRowNumber  작업을 수행할 Rowset Position
 * @param[in] aLockType   Not Used and Ignored
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnSetPosUpdate(ulnFnContext *aFnContext,
                       ulnPtContext *aPtContext,
                       ulnStmt      *aKeysetStmt,
                       acp_uint16_t  aRowNumber,
                       acp_uint16_t  aLockType)
{
    ULN_FLAG(sNeedRestoreParamSetAttrs);

    ulnFnContext         sTmpFnContext;
    ulnDescRec          *sDescRecArd;
    ulnDescRec          *sDescRecIpd;
    acp_uint16_t        *sRowStatusPtr;
    acp_uint16_t        *sRowOperationPtr;
    acp_sint32_t         sRowTotCount;
    acp_sint32_t         sRowActCount;
    acp_sint32_t         sRowErrCount;
    acp_sint32_t         sMaxRowSize;
    ulnColumnIgnoreFlag *sColumnIgnoreFlags;
    acp_sint32_t         sColTotCount;
    acp_sint32_t         sColActCount;
    acp_uint32_t         sParamBindTypeOld;
    acp_uint32_t         sParamSetSizeOld;
    acp_uint16_t        *sParamStatusBuf;
    acp_uint16_t        *sParamStatusPtrOld;
    ulvULen              sParamsProcessed;
    ulvULen             *sParamsProcessedPtrOld;
    acp_sint32_t         sColIdx;
    acp_sint32_t         sRowIdx;
    acp_sint32_t         sRowNum;
    acp_sint32_t         i;
    acp_sint32_t         j;

    acp_sint16_t         sSQL_C_TYPE;
    acp_sint16_t         sSQL_TYPE;
    ulvSLen             *sBindColIndPtr;
    ulnIndLenPtrPair     sUserIndLenPair;
    ulvSLen             *sIndPtr;
    acp_uint32_t         sUserRowNumberStart;
    acp_uint8_t         *sCurrRowPtr;
    acp_uint8_t         *sCurrColPtr;
    ulvSLen             *sCurrIndPtr;
    ulvSLen              sBufLen;

    ulnCursor           *sCursor         = ulnStmtGetCursor(aKeysetStmt);
    acp_sint64_t         sCursorPosition = ulnCursorGetPosition(sCursor);
    ulnKeyset           *sKeyset         = ulnStmtGetKeyset(aKeysetStmt);
    ulnStmt             *sRowsetStmt     = aKeysetStmt->mRowsetStmt;

    ACP_UNUSED(aLockType);

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    /* aRowNumber가 0인지 보고 UPDATE 할 개수 결정. */
    if (aRowNumber == 0)
    {
        sRowTotCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
        sUserRowNumberStart = 1;
    }
    else
    {
        sRowTotCount = 1;
        sUserRowNumberStart = aRowNumber;
    }

    sRowStatusPtr    = ulnStmtGetAttrRowStatusPtr(aKeysetStmt);
    sRowOperationPtr = ulnStmtGetAttrRowOperationPtr(aKeysetStmt);

    sRowActCount = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        /* SQL_ATTR_ROW_OPERATION_PTR를 이용해 건너뜀 */
        if ((sRowOperationPtr != NULL)
         && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        if (sRowStatusPtr != NULL)
        {
            /* 이미 작업된 것으로 추정되는 Row는 UPDATE 할 수 없다.
             * 다시 Fetch or Refresh 한 후 시도해야 한다. */
            ACI_TEST_RAISE( sRowStatusPtr[sRowNum] == SQL_ROW_DELETED,
                            INVALID_CURSOR_POSITION_EXCEPTION );
            ACI_TEST_RAISE( sRowStatusPtr[sRowNum] == SQL_ROW_UPDATED,
                            CURSOR_OP_CONFLICT_EXCEPTION );
        }

        sRowActCount++;
    }
    ACI_TEST_RAISE( sRowActCount == 0, NO_UPDATE_ROWS_EXCEPTION_CONT );
    ACI_TEST_RAISE( sRowActCount > ACP_UINT16_MAX,
                    INVALID_PARAMSET_SIZE_EXCEPTION );

    /* check updatable column count */
    sColTotCount = ulnStmtGetColumnCount(aKeysetStmt);
    ACI_TEST_RAISE(ulnStmtEnsureAllocColumnIgnoreFlagsBuf(aKeysetStmt,
                                                          sColTotCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sColumnIgnoreFlags = ulnStmtGetColumnIgnoreFlagsBuf(aKeysetStmt);

    sColActCount = 0;
    sMaxRowSize = ACP_ALIGN8(ULN_KEYSET_MAX_KEY_SIZE);
    for (i = 1; i <= sColTotCount; i++)
    {
        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, i);
        if (sDescRecArd == NULL)
        {
            sColumnIgnoreFlags[i-1] = ULN_COLUMN_IGNORE;
            continue;
        }
        sColumnIgnoreFlags[i-1] = ULN_COLUMN_PROCEED;
        if (ulnDescRecGetIndicatorPtr(sDescRecArd) != NULL)
        {
            for (j = 0; j < sRowTotCount; j++)
            {
                sRowNum = sUserRowNumberStart + j - 1;

                if ((sRowOperationPtr != NULL)
                 && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
                {
                    continue;
                }

                ulnBindCalcUserIndLenPair(sDescRecArd, sRowNum, &sUserIndLenPair);
                ACE_DASSERT(sUserIndLenPair.mIndicatorPtr != NULL);
                if (*sUserIndLenPair.mIndicatorPtr == SQL_COLUMN_IGNORE)
                {
                    sColumnIgnoreFlags[i-1] = ULN_COLUMN_IGNORE;
                    break;
                }
            }
            if (sColumnIgnoreFlags[i-1] == ULN_COLUMN_IGNORE)
            {
                continue;
            }
        }

        sBufLen = ulnMetaGetOctetLength(&sDescRecArd->mMeta);
        sMaxRowSize += ACP_ALIGN8(sBufLen) + ACP_ALIGN8(ACI_SIZEOF(ulvSLen));

        sColActCount++;
    }

    ACI_TEST_RAISE(sColActCount == 0, NO_UPDATE_COLUMNS_EXCEPTION);

    /*
     * ===========================================
     * Prepare
     * ===========================================
     */

    /* init RowsetStmt */
    ACI_TEST(ulnFreeStmtClose(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);
    ACI_TEST(ulnFreeStmtUnbind(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);
    ACI_TEST(ulnFreeStmtResetParams(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);

    /* BUG-44858 */
    ulnStmtSetAttrPrepareWithDescribeParam(sRowsetStmt, ACP_TRUE);

    /* Prepare */
    ACI_TEST(ulnPrepBuildUpdateQstr(aFnContext, sColumnIgnoreFlags)
             != ACI_SUCCESS);
    ACI_TEST(ulnPrepareCore(&sTmpFnContext,
                            aPtContext,
                            aKeysetStmt->mQstrForInsUpd,
                            aKeysetStmt->mQstrForInsUpdLen,
                            CMP_DB_PREPARE_MODE_EXEC_PREPARE) != ACI_SUCCESS);

    /* Param 버퍼 생성 */
    ACI_TEST_RAISE(ulnStmtEnsureAllocRowsetParamBuf(sRowsetStmt,
                                                    sMaxRowSize * sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    ACI_TEST_RAISE(ulnStmtEnsureAllocRowsetParamStatusBuf(sRowsetStmt,
                                                          sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sParamStatusBuf = ulnStmtGetRowsetParamStatusBuf(sRowsetStmt);

    /* Param 정보 생성 */
    sCurrRowPtr = ulnStmtGetRowsetParamBuf(sRowsetStmt);
    sColIdx = 1;
    for (i = 1; i <= sColTotCount; i++)
    {
        if (sColumnIgnoreFlags[i-1] == ULN_COLUMN_IGNORE)
        {
            continue;
        }

        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, i);
        ACE_ASSERT(sDescRecArd != NULL);
        sDescRecIpd = ulnStmtGetIpdRec(sRowsetStmt, sColIdx);
        ACE_ASSERT(sDescRecIpd != NULL);

        sSQL_C_TYPE = ulnMetaGetOdbcConciseType(&sDescRecArd->mMeta);
        sSQL_TYPE = ulnMetaGetOdbcConciseType(&sDescRecIpd->mMeta);
        sBufLen = ulnMetaGetOctetLength(&sDescRecArd->mMeta);

        sCurrRowPtr = ACP_ALIGN8_PTR(sCurrRowPtr);
        sCurrIndPtr = ACP_ALIGN8_PTR(sCurrRowPtr + (sBufLen * sRowActCount));

        /* sCurrBufPtr, sBufLen씩 이동해가며 데이타 복사 */
        sBindColIndPtr = ulnDescRecGetIndicatorPtr(sDescRecArd);
        sCurrColPtr = sCurrRowPtr;
        sRowIdx = 0;
        for (j = 0; j < sRowTotCount; j++)
        {
            sRowNum = sUserRowNumberStart + j - 1;

            if ((sRowOperationPtr != NULL)
             && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
            {
                continue;
            }

            acpMemCpy(sCurrColPtr,
                      ulnBindCalcUserDataAddr(sDescRecArd, sRowNum),
                      sBufLen);

            if (sBindColIndPtr != NULL)
            {
                ulnBindCalcUserIndLenPair(sDescRecArd, sRowNum, &sUserIndLenPair);
                ACE_DASSERT(sUserIndLenPair.mIndicatorPtr != NULL);
                if ((sSQL_C_TYPE == SQL_C_CHAR)
                 && (*sUserIndLenPair.mIndicatorPtr == SQL_NTS))
                {
                    sCurrIndPtr[sRowIdx] = acpCStrLen((acp_char_t*)sCurrColPtr, ACP_SINT32_MAX);
                }
                else
                {
                    sCurrIndPtr[sRowIdx] = *sUserIndLenPair.mIndicatorPtr;
                }
            }
            sCurrColPtr += sBufLen;
            sRowIdx++;
        }

        if (sBindColIndPtr == NULL)
        {
            sIndPtr = NULL;
        }
        else
        {
            sIndPtr = sCurrIndPtr;
        }

        /* BUGBUG: Precision, Scale을 제대로 설정해줘야 될까? */
        ACI_TEST(ulnBindParamBody(&sTmpFnContext,
                                  sColIdx,
                                  NULL,
                                  SQL_PARAM_INPUT,
                                  sSQL_C_TYPE,
                                  sSQL_TYPE,
                                  ULN_DEFAULT_PRECISION,
                                  0,
                                  sCurrRowPtr,
                                  sBufLen,
                                  sIndPtr)
                 != ACI_SUCCESS);

        sCurrRowPtr = (acp_uint8_t *)(sCurrIndPtr + sRowActCount);
        sColIdx++;
    }

    /* bind param for _PROWID */
    {
        sCurrRowPtr = ACP_ALIGN8_PTR(sCurrRowPtr);
        sCurrColPtr = sCurrRowPtr;
        sRowIdx = 0;
        for (j = 0; j < sRowTotCount; j++)
        {
            sRowNum = sUserRowNumberStart + j - 1;

            if ((sRowOperationPtr != NULL)
             && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
            {
                continue;
            }

            acpMemCpy(sCurrColPtr,
                      ulnKeysetGetKeyValue(sKeyset,
                                           sCursorPosition + sRowNum),
                      ULN_KEYSET_MAX_KEY_SIZE);

            sCurrColPtr += ULN_KEYSET_MAX_KEY_SIZE;
            sRowIdx++;
        }

        ACI_TEST(ulnBindParamBody(&sTmpFnContext,
                                  sColActCount + 1,
                                  NULL,
                                  SQL_PARAM_INPUT,
                                  SQL_C_SBIGINT,
                                  SQL_BIGINT,
                                  0,
                                  0,
                                  sCurrRowPtr,
                                  ULN_KEYSET_MAX_KEY_SIZE,
                                  NULL)
                 != ACI_SUCCESS);
    }

    /*
     * ===========================================
     * Run - ExecDirect
     * ===========================================
     */

    sParamBindTypeOld      = ulnStmtGetAttrParamBindType(sRowsetStmt);
    sParamSetSizeOld       = ulnStmtGetAttrParamsetSize(sRowsetStmt);
    sParamStatusPtrOld     = ulnStmtGetAttrParamStatusPtr(sRowsetStmt);
    sParamsProcessedPtrOld = ulnStmtGetAttrParamsProcessedPtr(sRowsetStmt);

    ULN_FLAG_UP(sNeedRestoreParamSetAttrs);
    ulnStmtSetAttrParamBindType(sRowsetStmt, SQL_PARAM_BIND_BY_COLUMN);
    ulnStmtSetAttrParamsetSize(sRowsetStmt, sRowActCount);
    ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusBuf);
    ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, &sParamsProcessed);

    ACI_TEST(ulnExecuteCore(&sTmpFnContext, aPtContext) != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(&sTmpFnContext, aPtContext,
                                     aKeysetStmt->mParentDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedRestoreParamSetAttrs);
    ulnStmtSetAttrParamBindType(sRowsetStmt, sParamBindTypeOld);
    ulnStmtSetAttrParamsetSize(sRowsetStmt, sParamSetSizeOld);
    ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusPtrOld);
    ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, sParamsProcessedPtrOld);

    /*
     * ===========================================
     * Return
     * ===========================================
     */

#if 0
    if (sParamsProcessed < sRowActCouunt)
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
#endif

    sRowErrCount = 0;
    sRowIdx = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        /* SQL_ATTR_ROW_OPERATION_PTR를 이용해 건너뜀 */
        if ((sRowOperationPtr != NULL)
         && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        if (sParamStatusBuf[sRowIdx] == SQL_PARAM_SUCCESS)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowNum, SQL_ROW_UPDATED);
        }
        else
        {
            /* BUGBUG: 이런 일이 생길 수 있는 상황은 뭘까? */
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowNum, SQL_ROW_ERROR);
            sRowErrCount++;
        }

        sRowIdx++;
    }

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

#if (ODBCVER < 0x0300)
    if (sRowErrCount > 0)
    {
        if (sRowErrCount == sRowActCount)
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_ERROR);
        }
        else
        {
            ulnError(aFnContext, ulERR_IGNORE_ERROR_IN_ROW);
        }
    }
#endif

    ACI_EXCEPTION_CONT(NO_UPDATE_ROWS_EXCEPTION_CONT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_CURSOR_POSITION_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_Invalid_Cursor_Position_Error);
    }
    ACI_EXCEPTION(CURSOR_OP_CONFLICT_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_CURSOR_OP_CONFLICT);
    }
    ACI_EXCEPTION(INVALID_PARAMSET_SIZE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ARRAY_SIZE);
    }
    ACI_EXCEPTION(NO_UPDATE_COLUMNS_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_DOES_NOT_MATCH_COLUMN_LIST);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnSetPosUpdate");
    }
    ACI_EXCEPTION_END;

#if 0
    if (sRowStatusPtr != NULL)
    {
        for (i = 0; i < sRowCount; i++)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sUserRowNumberStart + i - 1, SQL_ROW_ERROR);
        }
    }
#endif

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

    ULN_IS_FLAG_UP(sNeedRestoreParamSetAttrs)
    {
        ulnStmtSetAttrParamBindType(sRowsetStmt, sParamBindTypeOld);
        ulnStmtSetAttrParamsetSize(sRowsetStmt, sParamSetSizeOld);
        ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusPtrOld);
        ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, sParamsProcessedPtrOld);
    }

    return ACI_FAILURE;
}

/* BUGBUG: KEYSET_DRIVEN일 때만 정상 동작한다.
 *         FORWARD_ONLY/STATIC으로는 UPDATABLE을 지원하지 않기 때문. */
/**
 * 지정한 행을 DELETE 한다.
 *
 * 만약, RowNumber가 0이면 모든 행을 DELETE 한다.
 * 단, SQL_ATTR_ROW_OPERATION_PTR에 SQL_ROW_IGNORE로 지정한 행을 무시한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 * @param[in] aRowNumber  작업을 수행할 Rowset Position
 * @param[in] aLockType   Not Used and Ignored
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnSetPosDelete(ulnFnContext *aFnContext,
                       ulnPtContext *aPtContext,
                       ulnStmt      *aKeysetStmt,
                       acp_uint16_t  aRowNumber,
                       acp_uint16_t  aLockType)
{
    ULN_FLAG(sNeedRestoreParamSetAttrs);

    ulnFnContext         sTmpFnContext;
    acp_uint16_t        *sRowStatusPtr;
    acp_uint16_t        *sRowOperationPtr;
    acp_sint32_t         sRowTotCount;
    acp_sint32_t         sRowActCount;
    acp_sint32_t         sRowErrCount;
    acp_uint32_t         sParamBindTypeOld;
    acp_uint32_t         sParamSetSizeOld;
    acp_uint16_t        *sParamStatusBuf;
    acp_uint16_t        *sParamStatusPtrOld;
    ulvULen              sParamsProcessed;
    ulvULen             *sParamsProcessedPtrOld;
    acp_sint32_t         sRowIdx;
    acp_sint32_t         sRowNum;
    acp_sint32_t         i;

    acp_uint32_t         sUserRowNumberStart;
    acp_uint8_t         *sCurrColPtr;

    ulnCursor           *sCursor         = ulnStmtGetCursor(aKeysetStmt);
    acp_sint64_t         sCursorPosition = ulnCursorGetPosition(sCursor);
    ulnKeyset           *sKeyset         = ulnStmtGetKeyset(aKeysetStmt);
    ulnStmt             *sRowsetStmt     = aKeysetStmt->mRowsetStmt;

    ACP_UNUSED(aLockType);

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    /* aRowNumber가 0인지 보고 DELETE 할 개수 결정. */
    if (aRowNumber == 0)
    {
        sRowTotCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
        sUserRowNumberStart = 1;
    }
    else
    {
        sRowTotCount = 1;
        sUserRowNumberStart = aRowNumber;
    }

    sRowStatusPtr    = ulnStmtGetAttrRowStatusPtr(aKeysetStmt);
    sRowOperationPtr = ulnStmtGetAttrRowOperationPtr(aKeysetStmt);

    sRowActCount = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        /* SQL_ATTR_ROW_OPERATION_PTR를 이용해 건너뜀 */
        if ((sRowOperationPtr != NULL)
         && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        if (sRowStatusPtr != NULL)
        {
            /* 이미 작업된 것으로 추정되는 Row는 DELETE 할 수 없다.
             * 다시 Fetch or Refresh 한 후 시도해야 한다. */
            ACI_TEST_RAISE( sRowStatusPtr[sRowNum] == SQL_ROW_DELETED,
                            INVALID_CURSOR_POSITION_EXCEPTION );
            ACI_TEST_RAISE( sRowStatusPtr[sRowNum] == SQL_ROW_UPDATED,
                            CURSOR_OP_CONFLICT_EXCEPTION);
        }

        sRowActCount++;
    }
    ACI_TEST_RAISE( sRowActCount == 0, NO_DELETE_ROWS_EXCEPTION_CONT );
    ACI_TEST_RAISE( sRowActCount > ACP_UINT16_MAX,
                    INVALID_PARAMSET_SIZE_EXCEPTION );

    /*
     * ===========================================
     * Prepare
     * ===========================================
     */

    ACI_TEST(ulnPrepBuildDeleteQstr(aFnContext) != ACI_SUCCESS);

    /* init RowsetStmt */
    ACI_TEST(ulnFreeStmtClose(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);
    ACI_TEST(ulnFreeStmtUnbind(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);
    ACI_TEST(ulnFreeStmtResetParams(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);

    /* Param 버퍼 생성 */
    ACI_TEST_RAISE(ulnStmtEnsureAllocRowsetParamBuf(sRowsetStmt,
                                                    ULN_KEYSET_MAX_KEY_SIZE * sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    ACI_TEST_RAISE(ulnStmtEnsureAllocRowsetParamStatusBuf(sRowsetStmt,
                                                          sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sParamStatusBuf = ulnStmtGetRowsetParamStatusBuf(sRowsetStmt);

    /* Param 정보 생성 (_PROWID) */
    sCurrColPtr = ulnStmtGetRowsetParamBuf(sRowsetStmt);
    sRowIdx = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        if ((sRowOperationPtr != NULL)
            && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        acpMemCpy(sCurrColPtr, ulnKeysetGetKeyValue(sKeyset,
                                                    sCursorPosition + sRowNum),
                  ULN_KEYSET_MAX_KEY_SIZE);

        sCurrColPtr += ULN_KEYSET_MAX_KEY_SIZE;
        sRowIdx++;
    }

    ACI_TEST(ulnBindParamBody(&sTmpFnContext,
                              1,
                              NULL,
                              SQL_PARAM_INPUT,
                              SQL_C_SBIGINT,
                              SQL_BIGINT,
                              0,
                              0,
                              ulnStmtGetRowsetParamBuf(sRowsetStmt),
                              ULN_KEYSET_MAX_KEY_SIZE, NULL)
             != ACI_SUCCESS);

    /*
     * ===========================================
     * Run - ExecDirect
     * ===========================================
     */

    sParamBindTypeOld      = ulnStmtGetAttrParamBindType(sRowsetStmt);
    sParamSetSizeOld       = ulnStmtGetAttrParamsetSize(sRowsetStmt);
    sParamStatusPtrOld     = ulnStmtGetAttrParamStatusPtr(sRowsetStmt);
    sParamsProcessedPtrOld = ulnStmtGetAttrParamsProcessedPtr(sRowsetStmt);

    ULN_FLAG_UP(sNeedRestoreParamSetAttrs);
    ulnStmtSetAttrParamBindType(sRowsetStmt, SQL_PARAM_BIND_BY_COLUMN);
    ulnStmtSetAttrParamsetSize(sRowsetStmt, sRowActCount);
    ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusBuf);
    ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, &sParamsProcessed);

    /* DirectExec */
    ACI_TEST(ulnPrepareCore(&sTmpFnContext,
                            aPtContext,
                            aKeysetStmt->mQstrForDelete,
                            aKeysetStmt->mQstrForDeleteLen,
                            CMP_DB_PREPARE_MODE_EXEC_DIRECT) != ACI_SUCCESS);

    ACI_TEST(ulnExecuteCore(&sTmpFnContext, aPtContext) != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(&sTmpFnContext, aPtContext,
                                     aKeysetStmt->mParentDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedRestoreParamSetAttrs);
    ulnStmtSetAttrParamBindType(sRowsetStmt, sParamBindTypeOld);
    ulnStmtSetAttrParamsetSize(sRowsetStmt, sParamSetSizeOld);
    ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusPtrOld);
    ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, sParamsProcessedPtrOld);

    /*
     * ===========================================
     * Return
     * ===========================================
     */

#if 0
    if (sParamsProcessed < sRowActCouunt)
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
#endif

    sRowErrCount = 0;
    sRowIdx = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        /* SQL_ATTR_ROW_OPERATION_PTR를 이용해 건너뜀 */
        if ((sRowOperationPtr != NULL)
         && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        if (sParamStatusBuf[sRowIdx] == SQL_PARAM_SUCCESS)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowNum, SQL_ROW_DELETED);
        }
        else
        {
            /* BUGBUG: 이런 일이 생길 수 있는 상황은 뭘까? */
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowNum, SQL_ROW_ERROR);
            sRowErrCount++;
        }

        sRowIdx++;
    }

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

#if (ODBCVER < 0x0300)
    if (sRowErrCount > 0)
    {
        if (sRowErrCount == sRowActCount)
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_ERROR);
        }
        else
        {
            ulnError(aFnContext, ulERR_IGNORE_ERROR_IN_ROW);
        }
    }
#endif

    ACI_EXCEPTION_CONT(NO_DELETE_ROWS_EXCEPTION_CONT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_CURSOR_POSITION_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_Invalid_Cursor_Position_Error);
    }
    ACI_EXCEPTION(CURSOR_OP_CONFLICT_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_CURSOR_OP_CONFLICT);
    }
    ACI_EXCEPTION(INVALID_PARAMSET_SIZE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ARRAY_SIZE);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnSetPosDelete");
    }
    ACI_EXCEPTION_END;

#if 0
    if (sRowStatusPtr != NULL)
    {
        for (i = 0; i < sRowCount; i++)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sUserRowNumberStart + i - 1, SQL_ROW_ERROR);
        }
    }
#endif

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

    ULN_IS_FLAG_UP(sNeedRestoreParamSetAttrs)
    {
        ulnStmtSetAttrParamBindType(sRowsetStmt, sParamBindTypeOld);
        ulnStmtSetAttrParamsetSize(sRowsetStmt, sParamSetSizeOld);
        ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusPtrOld);
        ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, sParamsProcessedPtrOld);
    }

    return ACI_FAILURE;
}


/* BUGBUG: KEYSET_DRIVEN일 때만 정상 동작한다.
 *         FORWARD_ONLY/STATIC으로는 UPDATABLE을 지원하지 않기 때문. */
/**
 * 지정한 행을 INSERT 한다.
 *
 * 만약, RowNumber가 0이면 모든 행을 INSERT 한다.
 * 단, SQL_ATTR_ROW_OPERATION_PTR에 SQL_ROW_IGNORE로 지정한 행을 무시한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 * @param[in] aRowNumber  작업을 수행할 Rowset Position
 * @param[in] aLockType   Not Used and Ignored
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnSetPosInsert(ulnFnContext *aFnContext,
                       ulnPtContext *aPtContext,
                       ulnStmt      *aKeysetStmt,
                       acp_uint16_t  aRowNumber,
                       acp_uint16_t  aLockType)
{
    ULN_FLAG(sNeedRestoreParamSetAttrs);

    ulnFnContext         sTmpFnContext;
    ulnDescRec          *sDescRecArd;
    ulnDescRec          *sDescRecIpd;
    acp_uint16_t        *sRowOperationPtr;
    acp_sint32_t         sRowTotCount;
    acp_sint32_t         sRowActCount;
    acp_sint32_t         sRowErrCount;
    acp_sint32_t         sMaxRowSize;
    ulnColumnIgnoreFlag *sColumnIgnoreFlags;
    acp_sint32_t         sColTotCount;
    acp_sint32_t         sColActCount;
    acp_uint32_t         sParamBindTypeOld;
    acp_uint32_t         sParamSetSizeOld;
    acp_uint16_t        *sParamStatusBuf;
    acp_uint16_t        *sParamStatusPtrOld;
    ulvULen              sParamsProcessed;
    ulvULen             *sParamsProcessedPtrOld;
    acp_sint32_t         sColIdx;
    acp_sint32_t         sRowIdx;
    acp_sint32_t         sRowNum;
    acp_sint32_t         i;
    acp_sint32_t         j;

    acp_sint16_t         sSQL_C_TYPE;
    acp_sint16_t         sSQL_TYPE;
    ulvSLen             *sBindColIndPtr;
    ulnIndLenPtrPair     sUserIndLenPair;
    ulvSLen             *sIndPtr;
    acp_uint32_t         sUserRowNumberStart;
    acp_uint8_t         *sCurrRowPtr;
    acp_uint8_t         *sCurrColPtr;
    ulvSLen             *sCurrIndPtr;
    ulvSLen              sBufLen;

    ulnStmt             *sRowsetStmt     = aKeysetStmt->mRowsetStmt;

    ACP_UNUSED(aLockType);

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    /* aRowNumber가 0인지 보고 INSERT 할 개수 결정. */
    if (aRowNumber == 0)
    {
        sRowTotCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
        sUserRowNumberStart = 1;
    }
    else
    {
        sRowTotCount = 1;
        sUserRowNumberStart = aRowNumber;
    }

    sRowOperationPtr = ulnStmtGetAttrRowOperationPtr(aKeysetStmt);

    sRowActCount = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        /* SQL_ATTR_ROW_OPERATION_PTR를 이용해 건너뜀 */
        if ((sRowOperationPtr != NULL)
         && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        sRowActCount++;
    }
    ACI_TEST_RAISE( sRowActCount == 0, NO_INSERT_ROWS_EXCEPTION_CONT );
    ACI_TEST_RAISE( sRowActCount > ACP_UINT16_MAX,
                    INVALID_PARAMSET_SIZE_EXCEPTION );

    /* check updatable column count */
    sColTotCount = ulnStmtGetColumnCount(aKeysetStmt);
    ACI_TEST_RAISE(ulnStmtEnsureAllocColumnIgnoreFlagsBuf(aKeysetStmt,
                                                          sColTotCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sColumnIgnoreFlags = ulnStmtGetColumnIgnoreFlagsBuf(aKeysetStmt);

    sColActCount = 0;
    sMaxRowSize = 0;
    for (i = 1; i <= sColTotCount; i++)
    {
        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, i);
        if (sDescRecArd == NULL)
        {
            sColumnIgnoreFlags[i-1] = ULN_COLUMN_IGNORE;
            continue;
        }
        sColumnIgnoreFlags[i-1] = ULN_COLUMN_PROCEED;
        if (ulnDescRecGetIndicatorPtr(sDescRecArd) != NULL)
        {
            for (j = 0; j < sRowTotCount; j++)
            {
                sRowNum = sUserRowNumberStart + j - 1;

                if ((sRowOperationPtr != NULL)
                 && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
                {
                    continue;
                }

                ulnBindCalcUserIndLenPair(sDescRecArd, sRowNum, &sUserIndLenPair);
                ACE_DASSERT(sUserIndLenPair.mIndicatorPtr != NULL);
                if (*sUserIndLenPair.mIndicatorPtr == SQL_COLUMN_IGNORE)
                {
                    sColumnIgnoreFlags[i-1] = ULN_COLUMN_IGNORE;
                    break;
                }
            }
            if (sColumnIgnoreFlags[i-1] == ULN_COLUMN_IGNORE)
            {
                continue;
            }
        }

        sBufLen = ulnMetaGetOctetLength(&sDescRecArd->mMeta);
        sMaxRowSize += ACP_ALIGN8(sBufLen) + ACP_ALIGN8(ACI_SIZEOF(ulvSLen));

        sColActCount++;
    }

    ACI_TEST_RAISE(sColActCount == 0, NO_INSERT_COLUMNS_EXCEPTION);

    /*
     * ===========================================
     * Prepare
     * ===========================================
     */

    /* init RowsetStmt */
    ACI_TEST(ulnFreeStmtClose(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);
    ACI_TEST(ulnFreeStmtUnbind(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);
    ACI_TEST(ulnFreeStmtResetParams(&sTmpFnContext, sRowsetStmt) != ACI_SUCCESS);

    /* BUG-44858 */
    ulnStmtSetAttrPrepareWithDescribeParam(sRowsetStmt, ACP_TRUE);

    /* Prepare */
    ACI_TEST(ulnPrepBuildInsertQstr(aFnContext, sColumnIgnoreFlags)
             != ACI_SUCCESS);
    ACI_TEST(ulnPrepareCore(&sTmpFnContext,
                            aPtContext,
                            aKeysetStmt->mQstrForInsUpd,
                            aKeysetStmt->mQstrForInsUpdLen,
                            CMP_DB_PREPARE_MODE_EXEC_PREPARE) != ACI_SUCCESS);

    /* Param 버퍼 생성 */
    ACI_TEST_RAISE(ulnStmtEnsureAllocRowsetParamBuf(sRowsetStmt,
                                                    sMaxRowSize * sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    ACI_TEST_RAISE(ulnStmtEnsureAllocRowsetParamStatusBuf(sRowsetStmt,
                                                          sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sParamStatusBuf = ulnStmtGetRowsetParamStatusBuf(sRowsetStmt);

    /* Param 정보 생성 */
    sCurrRowPtr = ulnStmtGetRowsetParamBuf(sRowsetStmt);
    sColIdx = 1;
    for (i = 1; i <= sColTotCount; i++)
    {
        if (sColumnIgnoreFlags[i-1] == ULN_COLUMN_IGNORE)
        {
            continue;
        }

        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, i);
        ACE_ASSERT(sDescRecArd != NULL);
        sDescRecIpd = ulnStmtGetIpdRec(sRowsetStmt, sColIdx);
        ACE_ASSERT(sDescRecIpd != NULL);

        sSQL_C_TYPE = ulnMetaGetOdbcConciseType(&sDescRecArd->mMeta);
        sSQL_TYPE = ulnMetaGetOdbcConciseType(&sDescRecIpd->mMeta);
        sBufLen = ulnMetaGetOctetLength(&sDescRecArd->mMeta);

        sCurrRowPtr = ACP_ALIGN8_PTR(sCurrRowPtr);
        sCurrIndPtr = ACP_ALIGN8_PTR(sCurrRowPtr + (sBufLen * sRowActCount));

        /* sCurrBufPtr, sBufLen씩 이동해가며 데이타 복사 */
        sBindColIndPtr = ulnDescRecGetIndicatorPtr(sDescRecArd);
        sCurrColPtr = sCurrRowPtr;
        sRowIdx = 0;
        for (j = 0; j < sRowTotCount; j++)
        {
            sRowNum = sUserRowNumberStart + j - 1;

            if ((sRowOperationPtr != NULL)
             && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
            {
                continue;
            }

            acpMemCpy(sCurrColPtr,
                      ulnBindCalcUserDataAddr(sDescRecArd, sRowNum),
                      sBufLen);

            if (sBindColIndPtr != NULL)
            {
                ulnBindCalcUserIndLenPair(sDescRecArd, sRowNum, &sUserIndLenPair);
                ACE_DASSERT(sUserIndLenPair.mIndicatorPtr != NULL);
                if ((sSQL_C_TYPE == SQL_C_CHAR)
                 && (*sUserIndLenPair.mIndicatorPtr == SQL_NTS))
                {
                    sCurrIndPtr[sRowIdx] = acpCStrLen((acp_char_t*)sCurrColPtr, ACP_SINT32_MAX);
                }
                else
                {
                    sCurrIndPtr[sRowIdx] = *sUserIndLenPair.mIndicatorPtr;
                }
            }
            sCurrColPtr += sBufLen;
            sRowIdx++;
        }

        if (sBindColIndPtr == NULL)
        {
            sIndPtr = NULL;
        }
        else
        {
            sIndPtr = sCurrIndPtr;
        }

        /* BUGBUG: Precision, Scale을 제대로 설정해줘야 될까? */
        ACI_TEST(ulnBindParamBody(&sTmpFnContext,
                                  sColIdx,
                                  NULL,
                                  SQL_PARAM_INPUT,
                                  sSQL_C_TYPE,
                                  sSQL_TYPE,
                                  ULN_DEFAULT_PRECISION,
                                  0,
                                  sCurrRowPtr,
                                  sBufLen,
                                  sIndPtr)
                 != ACI_SUCCESS);

        sCurrRowPtr = (acp_uint8_t *)(sCurrIndPtr + sRowActCount);
        sColIdx++;
    }

    /*
     * ===========================================
     * Run - ExecDirect
     * ===========================================
     */

    sParamBindTypeOld      = ulnStmtGetAttrParamBindType(sRowsetStmt);
    sParamSetSizeOld       = ulnStmtGetAttrParamsetSize(sRowsetStmt);
    sParamStatusPtrOld     = ulnStmtGetAttrParamStatusPtr(sRowsetStmt);
    sParamsProcessedPtrOld = ulnStmtGetAttrParamsProcessedPtr(sRowsetStmt);

    ULN_FLAG_UP(sNeedRestoreParamSetAttrs);
    ulnStmtSetAttrParamBindType(sRowsetStmt, SQL_PARAM_BIND_BY_COLUMN);
    ulnStmtSetAttrParamsetSize(sRowsetStmt, sRowActCount);
    ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusBuf);
    ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, &sParamsProcessed);

    ACI_TEST(ulnExecuteCore(&sTmpFnContext, aPtContext) != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(&sTmpFnContext, aPtContext,
                                     aKeysetStmt->mParentDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedRestoreParamSetAttrs);
    ulnStmtSetAttrParamBindType(sRowsetStmt, sParamBindTypeOld);
    ulnStmtSetAttrParamsetSize(sRowsetStmt, sParamSetSizeOld);
    ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusPtrOld);
    ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, sParamsProcessedPtrOld);

    /*
     * ===========================================
     * Return
     * ===========================================
     */

#if 0
    if (sParamsProcessed < sRowActCouunt)
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
#endif

    sRowErrCount = 0;
    sRowIdx = 0;
    for (i = 0; i < sRowTotCount; i++)
    {
        sRowNum = sUserRowNumberStart + i - 1;

        /* SQL_ATTR_ROW_OPERATION_PTR를 이용해 건너뜀 */
        if ((sRowOperationPtr != NULL)
         && (sRowOperationPtr[sRowNum] == SQL_ROW_IGNORE))
        {
            continue;
        }

        if (sParamStatusBuf[sRowIdx] == SQL_PARAM_SUCCESS)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowNum, SQL_ROW_ADDED);
        }
        else
        {
            /* BUGBUG: 이런 일이 생길 수 있는 상황은 뭘까? */
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowNum, SQL_ROW_ERROR);
            sRowErrCount++;
        }

        sRowIdx++;
    }

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

#if (ODBCVER < 0x0300)
    if (sRowErrCount > 0)
    {
        if (sRowErrCount == sRowActCount)
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_ERROR);
        }
        else
        {
            ulnError(aFnContext, ulERR_IGNORE_ERROR_IN_ROW);
        }
    }
#endif

    ACI_EXCEPTION_CONT(NO_INSERT_ROWS_EXCEPTION_CONT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_PARAMSET_SIZE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ARRAY_SIZE);
    }
    ACI_EXCEPTION(NO_INSERT_COLUMNS_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_DOES_NOT_MATCH_COLUMN_LIST);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnSetPosInsert");
    }
    ACI_EXCEPTION_END;

#if 0
    if (sRowStatusPtr != NULL)
    {
        for (i = 0; i < sRowCount; i++)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sUserRowNumberStart + i - 1, SQL_ROW_ERROR);
        }
    }
#endif

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

    ULN_IS_FLAG_UP(sNeedRestoreParamSetAttrs)
    {
        ulnStmtSetAttrParamBindType(sRowsetStmt, sParamBindTypeOld);
        ulnStmtSetAttrParamsetSize(sRowsetStmt, sParamSetSizeOld);
        ulnStmtSetAttrParamStatusPtr(sRowsetStmt, sParamStatusPtrOld);
        ulnStmtSetAttrParamsProcessedPtr(sRowsetStmt, sParamsProcessedPtrOld);
    }

    return ACI_FAILURE;
}

/**
 * SetPos 관련 속성과 인자가 유효한지 확인한다.
 *
 * @param[in] aFnContext function context
 * @param[in] aRowNumber 작업을 수행할 Rowset Position
 * @param[in] aOperation 수행할 작업:
 *                       SQL_POSITION
 *                       SQL_REFRESH
 *                       SQL_UPDATE
 *                       SQL_DELETE
 *                       SQL_ADD
 *
 * @return 유효하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnSetPosCheckArgs(ulnFnContext *aFnContext,
                          acp_uint16_t  aRowNumber,
                          acp_uint16_t  aOperation)
{
    ulnStmt        *sStmt = aFnContext->mHandle.mStmt;
    acp_uint32_t    sRowsetSize;

    switch (aOperation)
    {
        case SQL_POSITION:
            ACI_TEST_RAISE(ulnStmtGetAttrCursorScrollable(sStmt)
                           != SQL_SCROLLABLE, FUNC_SEQ_EXCEPTION);
            break;

        case SQL_REFRESH:
            /* 꼭 Sensitive일 필요는 없다. 기존 데이타를 주면 되기 때문.
            ACI_TEST_RAISE(ulnStmtGetAttrCursorSensitivity(sStmt)
                           != SQL_SENSITIVE, FUNC_SEQ_EXCEPTION); */
            break;

        case SQL_UPDATE:
        case SQL_DELETE:
        case SQL_ADD:
            switch (ulnStmtGetAttrConcurrency(sStmt))
            {
                case SQL_CONCUR_ROWVER:
                    /* supported */
                    break;

                case SQL_CONCUR_READ_ONLY:
                    ACI_RAISE(FUNC_SEQ_EXCEPTION);
                    break;

                case SQL_CONCUR_LOCK:
                case SQL_CONCUR_VALUES:
                default:
                    /* 지원하지 않는 속성값이므로 뭔가 대단히 잘못된거다. */
                    ACE_ASSERT(ACP_FALSE);
                    break;
            }
            break;

        default:
            ACI_RAISE(INVALID_ATTR_ID_EXCEPTION);
            break;
    }

    /* check [b] */
    ACI_TEST_RAISE(ulnCursorGetPosition(ulnStmtGetCursor(sStmt)) < 0,
                   INVALID_CURSOR_STATE_EXCEPTION);

    /* BUGBUG: SQL_ROWSET_SIZE 설정값은 참고하지 않아도 될까? */
    sRowsetSize = ulnStmtGetAttrRowArraySize(sStmt);
    ACI_TEST_RAISE(aRowNumber > sRowsetSize, ROW_VALUE_OUT_OF_RANGE_EXCEPTION);

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_ATTR_ID_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aOperation);
    }
    ACI_EXCEPTION(FUNC_SEQ_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(INVALID_CURSOR_STATE_EXCEPTION)
    {
        /* [b] */
        ulnError(aFnContext, ulERR_ABORT_INVALID_CURSOR_STATE);
    }
    ACI_EXCEPTION(ROW_VALUE_OUT_OF_RANGE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_ROW_VALUE_OUT_OF_RANGE_ERR);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Rowset Position을 바꾸고, Refresh와 같은 작업을 수행한다.
 *
 * @param[in] aStmt      statement handle
 * @param[in] aRowNumber 작업을 수행할 Rowset Position
 * @param[in] aOperation 수행할 작업:
 *                       SQL_POSITION
 *                       SQL_REFRESH
 *                       SQL_UPDATE
 *                       SQL_DELETE
 * @param[in] aLockType  Not Used and Ignored
 *
 * @return SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_ERROR, or SQL_INVALID_HANDLE.
 */
SQLRETURN ulnSetPos(ulnStmt      *aStmt,
                    acp_uint16_t  aRowNumber,
                    acp_uint16_t  aOperation,
                    acp_uint16_t  aLockType)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;
    ulnDbc       *sParentDbc = NULL;

    ACP_UNUSED(aLockType);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETPOS, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    sParentDbc = aStmt->mParentDbc;  /* BUG-46885 */

    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(sParentDbc->mPtContext),
                                          &(sParentDbc->mSession))
             != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    /* Notes. Project 제약 사항 (스펙이 원래 이런건 아닌 듯. 아마;)
     * - SQL_POSITION은 Scrollable만 쓸 수 있다.
     * - SQL_REFRESH는 Sensitive만 쓸 수 있다.
     * - SQL_UPDATE, SQL_DELETE, SQL_ADD는 Updatable만 쓸 수 있다. */
    ACI_TEST(ulnSetPosCheckArgs(&sFnContext, aRowNumber, aOperation)
             != ACI_SUCCESS);

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */

    switch (aOperation)
    {
        case SQL_POSITION:
            ACI_TEST_RAISE(aRowNumber == 0, INVALID_CURSOR_POSITION_EXCEPTION);
            break;

        case SQL_REFRESH:
            if ((aRowNumber != 0)
             && (ulnStmtGetAttrCursorType(aStmt) == SQL_CURSOR_KEYSET_DRIVEN))
            {
                ulnCacheInvalidate(aStmt->mRowsetStmt->mCache);
            }
            ACI_TEST(ulnSetPosRefresh(&sFnContext,
                                      &(sParentDbc->mPtContext),
                                      aStmt, aRowNumber, aLockType)
                     != ACI_SUCCESS);
            break;

        case SQL_UPDATE:
            if (ulnStmtGetAttrCursorType(aStmt) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                ulnCacheInvalidate(aStmt->mRowsetStmt->mCache);
            }
            ACI_TEST(ulnSetPosUpdate(&sFnContext,
                                     &(sParentDbc->mPtContext),
                                     aStmt, aRowNumber, aLockType)
                     != ACI_SUCCESS);
            break;

        case SQL_DELETE:
            if (ulnStmtGetAttrCursorType(aStmt) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                ulnCacheInvalidate(aStmt->mRowsetStmt->mCache);
            }
            ACI_TEST(ulnSetPosDelete(&sFnContext,
                                     &(sParentDbc->mPtContext),
                                     aStmt, aRowNumber, aLockType)
                     != ACI_SUCCESS);
            break;

        case SQL_ADD:
            ACI_TEST(ulnSetPosInsert(&sFnContext,
                                     &(sParentDbc->mPtContext),
                                     aStmt, aRowNumber, aLockType)
                     != ACI_SUCCESS);
            break;

        default:
            /* 앞에서 확인하고 들어왔으므로 이런일이 생겨서는 안된다. */
             ACE_ASSERT(ACP_FALSE);
            break;
    }

    if (aRowNumber > 0)
    {
        ulnCursorSetRowsetPosition(ulnStmtGetCursor(aStmt), aRowNumber);

        /* NO_DATA가 떨어진 후에도 다시 GetData() 할 수 있도록
         * GetData 관련 변수를 초기화 */
        if (aStmt->mGDColumnNumber != ULN_GD_COLUMN_NUMBER_INIT_VALUE)
        {
            ACI_TEST(ulnGDInitColumn(&sFnContext, aStmt->mGDColumnNumber)
                     != ACI_SUCCESS);
            aStmt->mGDColumnNumber = ULN_GD_COLUMN_NUMBER_INIT_VALUE;
        }
    }

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext, &(sParentDbc->mPtContext))
             != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(INVALID_CURSOR_POSITION_EXCEPTION)
    {
        /* [i] */
        ulnError(&sFnContext, ulERR_ABORT_Invalid_Cursor_Position_Error);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext(&sFnContext, &(sParentDbc->mPtContext));
    }
    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
