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
 * ULN_SFID_02
 *
 * SQLBulkOperations(), STMT, S5-S6
 *
 * related state :
 *      -- [s]
 *      S8 [d]
 *      S11 [x]
 *
 * where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *      [x]  Executing. The function returned SQL_STILL_EXECUTING.
 *
 * @param[in] aFnContext funnction context
 *
 * @return always ACI_SUCCESS
 */
ACI_RC ulnSFID_02(ulnFnContext *aFnContext)
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
 * 0번째 BindCol에 지정한 행을 FETCH 한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnBulkFetchByBookmark(ulnFnContext *aFnContext,
                              ulnPtContext *aPtContext,
                              ulnStmt      *aKeysetStmt)
{
    ULN_FLAG(sHoleExistFlag);

    ulnDescRec     *sDescRecArd;
    acp_sint32_t    sFetchCount;
    acp_sint32_t    i;

    ulnStmt        *sRowsetStmt;
    ulnCache       *sRowsetCache;
    ulnRow         *sRow;
    acp_uint16_t   *sRowStatusPtr = ulnStmtGetAttrRowStatusPtr(aKeysetStmt);
    acp_sint64_t    sBookmark;
    acp_sint64_t    sMaxBookmark;

    sFetchCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);

    /* Sensitive이면 새 데이타를 Fetch, 아니면 Cache에 있는걸 반환 */
    if (ulnStmtGetAttrCursorSensitivity(aKeysetStmt) == SQL_SENSITIVE)
    {
        sRowsetStmt = aKeysetStmt->mRowsetStmt;
        sRowsetCache = ulnStmtGetCache(sRowsetStmt);
        sMaxBookmark = ulnKeysetGetKeyCount(ulnStmtGetKeyset(aKeysetStmt));

        ulnStmtSetAttrPrefetchRows(sRowsetStmt, sFetchCount);

        ACI_TEST(ulnFetchFromServerForSensitive(aFnContext, aPtContext,
                                                1,
                                                sFetchCount,
                                                ULN_STMT_FETCHMODE_BOOKMARK)
                 != ACI_SUCCESS);
    }
    else
    {
        sRowsetStmt = aKeysetStmt;
        sRowsetCache = ulnStmtGetCache(sRowsetStmt);
        sMaxBookmark = ulnCacheGetTotalRowCnt(sRowsetCache);
    }

    sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, 0);
    ACE_ASSERT(sDescRecArd != NULL); /* 위에서 확인하고 들어온다. */

    for (i = 0; i < sFetchCount; i++)
    {
        if (ulnStmtGetAttrUseBookMarks(aKeysetStmt) == SQL_UB_VARIABLE)
        {
            sBookmark = *((acp_sint64_t *) ulnBindCalcUserDataAddr(sDescRecArd, i));
        }
        else
        {
            sBookmark = *((acp_sint32_t *) ulnBindCalcUserDataAddr(sDescRecArd, i));
        }
        ACI_TEST_RAISE((sBookmark <= 0) || (sBookmark > sMaxBookmark),
                       INVALID_BOOKMARK_VALUE_EXCEPTION);

        sRow = ulnCacheGetCachedRow(sRowsetCache, sBookmark);
        if ((sRow == NULL) || (sRow->mRow == NULL))
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, i, SQL_ROW_DELETED);

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
                                                 1 + i)
                     != ACI_SUCCESS);

            aKeysetStmt->mGDTargetPosition = sBookmark;
        }
    } /* for (RowCouunt) */

    if ((aKeysetStmt != sRowsetStmt)
     && (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_SUCCESS))
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

    ACI_EXCEPTION(INVALID_BOOKMARK_VALUE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BOOKMARK_VALUE);
    }
    ACI_EXCEPTION_END;

    if ((aKeysetStmt != sRowsetStmt)
     && (ULN_FNCONTEXT_GET_RC(aFnContext) != SQL_SUCCESS))
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
    }

    return ACI_FAILURE;
}

/* BUGBUG: KEYSET_DRIVEN일 때만 정상 동작한다.
 *         FORWARD_ONLY/STATIC으로는 UPDATABLE을 지원하지 않기 때문. */
/**
 * 0번째 BindCol에 지정한 행을 UPDATE 한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnBulkUpdateByBookmark(ulnFnContext *aFnContext,
                               ulnPtContext *aPtContext,
                               ulnStmt      *aKeysetStmt)
{
    ULN_FLAG(sNeedRestoreParamSetAttrs);

    ulnFnContext         sTmpFnContext;
    ulnDescRec          *sDescRecArd;
    ulnDescRec          *sDescRecIpd;
    acp_sint32_t         sRowActCount;
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
    acp_sint32_t         i;

    acp_sint16_t         sSQL_C_TYPE;
    acp_sint16_t         sSQL_TYPE;
    ulvSLen             *sBindColIndPtr;
    ulnIndLenPtrPair     sUserIndLenPair;
    ulvSLen             *sIndPtr;
    acp_uint8_t         *sCurrRowPtr;
    acp_uint8_t         *sCurrColPtr;
    ulvSLen             *sCurrIndPtr;
    ulvSLen              sBufLen;

    ulnKeyset           *sKeyset         = ulnStmtGetKeyset(aKeysetStmt);
    ulnStmt             *sRowsetStmt     = aKeysetStmt->mRowsetStmt;
    acp_sint64_t         sBookmark;
    acp_sint64_t         sMaxBookmark;
    acp_uint8_t         *sKeyValue;

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    sRowActCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
    ulnStmtSetAttrPrefetchRows(sRowsetStmt, sRowActCount);

    ACI_TEST_RAISE( sRowActCount == 0, NO_UPDATE_ROWS_EXCEPTION_CONT );
    ACI_TEST_RAISE( sRowActCount > ACP_UINT16_MAX,
                    INVALID_PARAMSET_SIZE_EXCEPTION );

    /* check updatable column count */
    sColTotCount = ulnStmtGetColumnCount(aKeysetStmt);
    ACI_TEST_RAISE(ulnStmtEnsureAllocColumnIgnoreFlagsBuf(aKeysetStmt,
                                                          ACI_SIZEOF(ulnColumnIgnoreFlag) * sColTotCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sColumnIgnoreFlags = ulnStmtGetColumnIgnoreFlagsBuf(aKeysetStmt);

    if (ulnStmtGetAttrCursorType(aKeysetStmt) == SQL_CURSOR_KEYSET_DRIVEN)
    {
        sMaxBookmark = ulnKeysetGetKeyCount(ulnStmtGetKeyset(aKeysetStmt));
    }
    else
    {
        sMaxBookmark = ulnCacheGetTotalRowCnt(ulnStmtGetCache(sRowsetStmt));
    }

    sColActCount = 0;
    sMaxRowSize = ACP_ALIGN8(ULN_KEYSET_MAX_KEY_SIZE);
    for (i = 1; i <= sColTotCount; i++)
    {
        /* BUG-42492 [mm] diff /Interface/sample/sampleTest.sql */
        sColumnIgnoreFlags[i - 1] = ULN_COLUMN_PROCEED;

        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, i);
        if (sDescRecArd == NULL)
        {
            sColumnIgnoreFlags[i-1] = ULN_COLUMN_IGNORE;
            continue;
        }
        if (ulnDescRecGetIndicatorPtr(sDescRecArd) != NULL)
        {
            for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
            {
                ulnBindCalcUserIndLenPair(sDescRecArd, sRowIdx,
                                          &sUserIndLenPair);
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
                                                          ACI_SIZEOF(acp_uint16_t) * sRowActCount)
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
        for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
        {
            acpMemCpy(sCurrColPtr,
                      ulnBindCalcUserDataAddr(sDescRecArd, sRowIdx),
                      sBufLen);

            if (sBindColIndPtr != NULL)
            {
                ulnBindCalcUserIndLenPair(sDescRecArd, sRowIdx,
                                          &sUserIndLenPair);
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

    /* _PROWID */
    {
        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, 0);
        ACE_ASSERT(sDescRecArd != NULL); /* 위에서 확인하고 들어온다. */

        sCurrRowPtr = ACP_ALIGN8_PTR(sCurrRowPtr);
        sCurrColPtr = sCurrRowPtr;
        for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
        {
            if (ulnStmtGetAttrUseBookMarks(aKeysetStmt) == SQL_UB_VARIABLE)
            {
                sBookmark = *((acp_sint64_t *) ulnBindCalcUserDataAddr(sDescRecArd, sRowIdx));
            }
            else
            {
                sBookmark = *((acp_sint32_t *) ulnBindCalcUserDataAddr(sDescRecArd, sRowIdx));
            }
            ACI_TEST_RAISE((sBookmark <= 0) || (sBookmark > sMaxBookmark),
                           INVALID_BOOKMARK_VALUE_EXCEPTION);

            sKeyValue = ulnKeysetGetKeyValue(sKeyset, sBookmark);
            ACI_TEST_RAISE(sKeyValue == NULL, INVALID_BOOKMARK_VALUE_EXCEPTION);

            acpMemCpy(sCurrColPtr, sKeyValue, ULN_KEYSET_MAX_KEY_SIZE);
            sCurrColPtr += ULN_KEYSET_MAX_KEY_SIZE;
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

    for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
    {
        if (sParamStatusBuf[sRowIdx] == SQL_PARAM_SUCCESS)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowIdx, SQL_ROW_UPDATED);
        }
        else
        {
            /* BUGBUG: 이런 일이 생길 수 있는 상황은 뭘까? */
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowIdx, SQL_ROW_ERROR);
        }
    }

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

    ACI_EXCEPTION_CONT(NO_UPDATE_ROWS_EXCEPTION_CONT);

    return ACI_SUCCESS;

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
                 "ulnBulkUpdate");
    }
    ACI_EXCEPTION(INVALID_BOOKMARK_VALUE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BOOKMARK_VALUE);
    }
    ACI_EXCEPTION_END;

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
 * 0번째 BindCol에 지정한 행을 DELETE 한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnBulkDeleteByBookmark(ulnFnContext *aFnContext,
                               ulnPtContext *aPtContext,
                               ulnStmt      *aKeysetStmt)
{
    ULN_FLAG(sNeedRestoreParamSetAttrs);

    ulnFnContext         sTmpFnContext;
    ulnDescRec          *sDescRecArd;
    acp_sint32_t         sRowActCount;
    acp_uint32_t         sParamBindTypeOld;
    acp_uint32_t         sParamSetSizeOld;
    acp_uint16_t        *sParamStatusBuf;
    acp_uint16_t        *sParamStatusPtrOld;
    ulvULen              sParamsProcessed;
    ulvULen             *sParamsProcessedPtrOld;
    acp_sint32_t         i;

    acp_uint8_t         *sCurrColPtr;

    ulnKeyset           *sKeyset         = ulnStmtGetKeyset(aKeysetStmt);
    ulnStmt             *sRowsetStmt     = aKeysetStmt->mRowsetStmt;
    acp_uint8_t         *sKeyValue;
    acp_sint64_t         sBookmark;
    acp_sint64_t         sMaxBookmark;

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    sRowActCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
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
                                                          ACI_SIZEOF(acp_uint16_t) * sRowActCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sParamStatusBuf = ulnStmtGetRowsetParamStatusBuf(sRowsetStmt);

    /* Param 정보 생성 (_PROWID) */
    if (ulnStmtGetAttrCursorType(aKeysetStmt) == SQL_CURSOR_KEYSET_DRIVEN)
    {
        sMaxBookmark = ulnKeysetGetKeyCount(ulnStmtGetKeyset(aKeysetStmt));
    }
    else
    {
        sMaxBookmark = ulnCacheGetTotalRowCnt(ulnStmtGetCache(sRowsetStmt));
    }
    sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, 0);
    ACE_ASSERT(sDescRecArd != NULL); /* 위에서 확인하고 들어온다. */
    sCurrColPtr = ulnStmtGetRowsetParamBuf(sRowsetStmt);
    for (i = 0; i < sRowActCount; i++)
    {
        if (ulnStmtGetAttrUseBookMarks(aKeysetStmt) == SQL_UB_VARIABLE)
        {
            sBookmark = *((acp_sint64_t *) ulnBindCalcUserDataAddr(sDescRecArd, i));
        }
        else
        {
            sBookmark = *((acp_sint32_t *) ulnBindCalcUserDataAddr(sDescRecArd, i));
        }
        ACI_TEST_RAISE((sBookmark <= 0) || (sBookmark > sMaxBookmark),
                       INVALID_BOOKMARK_VALUE_EXCEPTION);

        sKeyValue = ulnKeysetGetKeyValue(sKeyset, sBookmark);
        ACI_TEST_RAISE(sKeyValue == NULL, INVALID_BOOKMARK_VALUE_EXCEPTION);

        acpMemCpy(sCurrColPtr, sKeyValue, ULN_KEYSET_MAX_KEY_SIZE);
        sCurrColPtr += ULN_KEYSET_MAX_KEY_SIZE;
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

    for (i = 0; i < sRowActCount; i++)
    {
        if (sParamStatusBuf[i] == SQL_PARAM_SUCCESS)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, i, SQL_ROW_DELETED);
        }
        else
        {
            /* BUGBUG: 이런 일이 생길 수 있는 상황은 뭘까? */
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, i, SQL_ROW_ERROR);
        }
    }

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

    ACI_EXCEPTION_CONT(NO_DELETE_ROWS_EXCEPTION_CONT);

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_PARAMSET_SIZE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ARRAY_SIZE);
    }
    ACI_EXCEPTION(NOT_ENOUGH_MEM_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR,
                 "ulnSetPosDelete");
    }
    ACI_EXCEPTION(INVALID_BOOKMARK_VALUE_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BOOKMARK_VALUE);
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
 * BindCol한 데이타를 추가한다.
 *
 * @param[in] aFnContext  function context
 * @param[in] aPtContext  protocol context
 * @param[in] aKeysetStmt statement handle
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnBulkInsert(ulnFnContext *aFnContext,
                     ulnPtContext *aPtContext,
                     ulnStmt      *aKeysetStmt)
{
    ULN_FLAG(sNeedRestoreParamSetAttrs);

    ulnFnContext         sTmpFnContext;
    ulnDescRec          *sDescRecArd;
    ulnDescRec          *sDescRecIpd;
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
    acp_sint32_t         i;

    acp_sint16_t         sSQL_C_TYPE;
    acp_sint16_t         sSQL_TYPE;
    ulvSLen             *sBindColIndPtr;
    ulnIndLenPtrPair     sUserIndLenPair;
    ulvSLen             *sIndPtr;
    acp_uint8_t         *sCurrRowPtr;
    acp_uint8_t         *sCurrColPtr;
    ulvSLen             *sCurrIndPtr;
    ulvSLen              sBufLen;

    ulnStmt             *sRowsetStmt     = aKeysetStmt->mRowsetStmt;

    ULN_INIT_FUNCTION_CONTEXT(sTmpFnContext, ULN_FID_EXECDIRECT, sRowsetStmt, ULN_OBJ_TYPE_STMT);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    sRowActCount = ulnStmtGetAttrRowArraySize(aKeysetStmt);
    ulnStmtSetAttrPrefetchRows(sRowsetStmt, sRowActCount);

    ACI_TEST_RAISE( sRowActCount == 0, NO_UPDATE_ROWS_EXCEPTION_CONT );
    ACI_TEST_RAISE( sRowActCount > ACP_UINT16_MAX,
                    INVALID_PARAMSET_SIZE_EXCEPTION );

    /* check updatable column count */
    sColTotCount = ulnStmtGetColumnCount(aKeysetStmt);
    ACI_TEST_RAISE(ulnStmtEnsureAllocColumnIgnoreFlagsBuf(aKeysetStmt,
                                                          ACI_SIZEOF(ulnColumnIgnoreFlag) * sColTotCount)
                   != ACI_SUCCESS, NOT_ENOUGH_MEM_EXCEPTION);
    sColumnIgnoreFlags = ulnStmtGetColumnIgnoreFlagsBuf(aKeysetStmt);

    sColActCount = 0;
    sMaxRowSize = 0;
    for (i = 1; i <= sColTotCount; i++)
    {
        /* BUG-42492 [mm] diff /Interface/sample/sampleTest.sql */
        sColumnIgnoreFlags[i - 1] = ULN_COLUMN_PROCEED;

        sDescRecArd = ulnStmtGetArdRec(aKeysetStmt, i);
        if (sDescRecArd == NULL)
        {
            sColumnIgnoreFlags[i-1] = ULN_COLUMN_IGNORE;
            continue;
        }
        if (ulnDescRecGetIndicatorPtr(sDescRecArd) != NULL)
        {
            for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
            {
                ulnBindCalcUserIndLenPair(sDescRecArd, sRowIdx,
                                          &sUserIndLenPair);
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
                                                          ACI_SIZEOF(acp_uint16_t) * sRowActCount)
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
        for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
        {
            acpMemCpy(sCurrColPtr,
                      ulnBindCalcUserDataAddr(sDescRecArd, sRowIdx),
                      sBufLen);

            if (sBindColIndPtr != NULL)
            {
                ulnBindCalcUserIndLenPair(sDescRecArd, sRowIdx,
                                          &sUserIndLenPair);
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
    for (sRowIdx = 0; sRowIdx < sRowActCount; sRowIdx++)
    {
        if (sParamStatusBuf[sRowIdx] == SQL_PARAM_SUCCESS)
        {
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowIdx, SQL_ROW_ADDED);
        }
        else
        {
            /* BUGBUG: 이런 일이 생길 수 있는 상황은 뭘까? */
            ulnStmtSetAttrRowStatusValue(aKeysetStmt, sRowIdx, SQL_ROW_ERROR);
            sRowErrCount++;
        }
    }

    if (ULN_FNCONTEXT_GET_RC(&sTmpFnContext) != SQL_SUCCESS)
    {
        ulnDiagRecMoveAll( &(aKeysetStmt->mObj), &(sRowsetStmt->mObj) );
        ULN_FNCONTEXT_SET_RC(aFnContext, ULN_FNCONTEXT_GET_RC(&sTmpFnContext));
    }

#if (ODBCVER < 0x0300)
    if (sRowErrCount > 0)
    {
        ulnError(aFnContext, ulERR_IGNORE_ERROR_IN_ROW);
    }
#endif

    ACI_EXCEPTION_CONT(NO_UPDATE_ROWS_EXCEPTION_CONT);

    return ACI_SUCCESS;

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
                 "ulnBulkUpdate");
    }
    ACI_EXCEPTION_END;

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
 * BulkOperations 관련 속성과 인자가 유효한지 확인한다.
 *
 * @param[in] aFnContext function context
 * @param[in] aOperation 수행할 작업:
 *                       SQL_ADD
 *                       SQL_UPDATE_BY_BOOKMARK
 *                       SQL_DELETE_BY_BOOKMARK
 *                       SQL_FETCH_BY_BOOKMARK
 *
 * @return 유효하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
ACI_RC ulnBulkOperationsCheckArgs(ulnFnContext *aFnContext,
                                  acp_uint16_t  aOperation)
{
    ulnStmt        *sStmt = aFnContext->mHandle.mStmt;
    acp_uint32_t    sColCount;
    acp_uint32_t    sBindCount;

    switch (aOperation)
    {
        case SQL_ADD:
            ACI_TEST_RAISE(ulnStmtGetAttrConcurrency(sStmt)
                           == SQL_CONCUR_READ_ONLY, INVALID_ATTR_ID_EXCEPTION);

            sColCount = ulnStmtGetColumnCount(sStmt);
            sBindCount = ulnDescGetHighestBoundIndex(sStmt->mAttrArd);
            ACI_TEST_RAISE(sColCount < sBindCount,
                           INVALID_DESC_INDEX_EXCEPTION);
            break;

        case SQL_UPDATE_BY_BOOKMARK:
        case SQL_DELETE_BY_BOOKMARK:
            ACI_TEST_RAISE((ulnStmtGetAttrUseBookMarks(sStmt) == SQL_UB_OFF) ||
                           (ulnStmtGetArdRec(sStmt, 0) == NULL) ||
                           (ulnStmtGetAttrCursorScrollable(sStmt) != SQL_SCROLLABLE) ||
                           (ulnStmtGetAttrConcurrency(sStmt) == SQL_CONCUR_READ_ONLY),
                           INVALID_ATTR_ID_EXCEPTION);
            break;

        case SQL_FETCH_BY_BOOKMARK:
            /* static도 fetch는 가능하다. */
            ACI_TEST_RAISE((ulnStmtGetAttrUseBookMarks(sStmt) == SQL_UB_OFF) ||
                           (ulnStmtGetArdRec(sStmt, 0) == NULL) ||
                           (ulnStmtGetAttrCursorScrollable(sStmt) != SQL_SCROLLABLE),
                           INVALID_ATTR_ID_EXCEPTION);
            break;

        default:
            ACI_RAISE(INVALID_ATTR_ID_EXCEPTION);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_ATTR_ID_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aOperation);
    }
    ACI_EXCEPTION(INVALID_DESC_INDEX_EXCEPTION)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX,
                 sColCount + 1);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * BOOKMARK로 지정한 Row에 특정 작업을 수행한다.
 *
 * @param[in] aStmt      statement handle
 * @param[in] aOperation 수행할 작업:
 *                       SQL_ADD
 *                       SQL_UPDATE_BY_BOOKMARK
 *                       SQL_DELETE_BY_BOOKMARK
 *                       SQL_FETCH_BY_BOOKMARK
 *
 * @return SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_ERROR, or SQL_INVALID_HANDLE.
 */
SQLRETURN ulnBulkOperations(ulnStmt      *aStmt,
                            acp_sint16_t  aOperation)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnDbc         *sParentDbc = NULL;
    ulnFnContext    sFnContext;
    ulnPtContext   *sPtContext = NULL;

    ACP_UNUSED(aOperation);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_BULKOPERATIONS, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    sParentDbc = aStmt->mParentDbc;  /* BUG-46885 */
    sPtContext = &(sParentDbc->mPtContext);

    ACI_TEST(ulnInitializeProtocolContext(&sFnContext, sPtContext,
                                          &(sParentDbc->mSession))
             != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * ===========================================
     * Check
     * ===========================================
     */

    ACI_TEST_RAISE(ulnStmtGetResultSetCount(aStmt) == 0,
                   INVALID_CURSOR_STATE_EXCEPTION);

    ACI_TEST(ulnBulkOperationsCheckArgs(&sFnContext, aOperation)
             != ACI_SUCCESS);

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */

    switch (aOperation)
    {
        case SQL_ADD:
            ACI_TEST(ulnBulkInsert(&sFnContext, sPtContext, aStmt)
                     != ACI_SUCCESS);
            break;

        case SQL_UPDATE_BY_BOOKMARK:
            if (ulnStmtGetAttrCursorType(aStmt) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                ulnCacheInvalidate(aStmt->mRowsetStmt->mCache);
            }
            ACI_TEST(ulnBulkUpdateByBookmark(&sFnContext, sPtContext, aStmt)
                     != ACI_SUCCESS);
            break;

        case SQL_DELETE_BY_BOOKMARK:
            if (ulnStmtGetAttrCursorType(aStmt) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                ulnCacheInvalidate(aStmt->mRowsetStmt->mCache);
            }
            ACI_TEST(ulnBulkDeleteByBookmark(&sFnContext, sPtContext, aStmt)
                     != ACI_SUCCESS);
            break;

        case SQL_FETCH_BY_BOOKMARK:
            if (ulnStmtGetAttrCursorType(aStmt) == SQL_CURSOR_KEYSET_DRIVEN)
            {
                ulnCacheInvalidate(aStmt->mRowsetStmt->mCache);
            }
            ACI_TEST(ulnBulkFetchByBookmark(&sFnContext, sPtContext, aStmt)
                     != ACI_SUCCESS);
            break;

        default:
            /* 앞에서 확인하고 들어왔으므로 이런일이 생겨서는 안된다. */
            ACE_ASSERT(ACP_FALSE);
            break;
    }

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext, sPtContext)
             != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(INVALID_CURSOR_STATE_EXCEPTION)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_CURSOR_STATE);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext(&sFnContext, sPtContext);
    }
    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
