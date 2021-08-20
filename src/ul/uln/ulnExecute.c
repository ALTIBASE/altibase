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
#include <ulnExecute.h>
#include <ulnPDContext.h>

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
#include <ulnFetchOpAsync.h>
#include <ulnSemiAsyncPrefetch.h>
#include <ulsdDistTxInfo.h>

/*
 * ULN_SFID_70
 * SQLExecute(), SQLExecDirect(), DBC, C5
 */
ACI_RC ulnSFID_70(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        // ulnStmtGetColumnCount(aFnContext->mHandle.mStmt, &sNumOfCols);

        if (aFnContext->mHandle.mDbc->mAttrAutoCommit == SQL_ATTR_AUTOCOMMIT)
        {
            // if (sNumOfCols != 0)
            // BUGBUG
            // BUGBUG
            // BUGBUG
            // BUGBUG
            // if (ulnStmtIsCursorOpen(aFnContext->mHandle.mStmt) == ACP_TRUE)
            // {
            // ULN_OBJ_SET_STATE(aFnContext->mHandle.mDbc, ULN_S_C6);
            // }
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_19
 * SQLExecute(), STMT, S2
 *
 *      S4 [s]
 *      S8 [d]
 *      S11 [x]
 *
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 */
ACI_RC ulnSFID_19(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        switch (ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
            case SQL_NO_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
                break;

            case SQL_NEED_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                break;

            default:
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_20
 * SQLExecute(), STMT, S3
 *
 *      S5 [s]
 *      S8 [d]
 *      X11 [x]
 *
 *  where
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 */
ACI_RC ulnSFID_20(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        switch (ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                ulnCursorOpen(&aFnContext->mHandle.mStmt->mCursor);
                break;

            case SQL_NEED_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                break;
            default:
                break;
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_21
 * SQLExecute(), STMT, S4
 *
 *      S2 [e], [p], and [1]
 *      S4 [s], [p], [nr], and [1]
 *      S5 [s], [p], [r], and [1]
 *      S8 [d], [p], and [1]
 *      S11 [x], [p], and [1]
 *      24000 [p] and [2]
 *      (HY010) [np]
 *
 * where
 *      [1] The current result is the last or only result, or there are no current results.
 *      [2] The current result is not the last result.
 *
 *      [s]  Success. The function returned SQL_SUCCESS_WITH_INFO or SQL_SUCCESS.
 *      [e]  Error. The function returned SQL_ERROR.
 *      [d]  Need data. The function returned SQL_NEED_DATA.
 *      [r]  Results. The statement will or did create a (possibly empty) result set.
 *      [nr] No results. The statement will not or did not create a result set.
 *      [np] Not prepared. The statement was not prepared.
 *      [p]  Prepared. The statement was prepared.
 */
ACI_RC ulnSFID_21(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        ACI_TEST_RAISE( ulnStmtIsPrepared(aFnContext->mHandle.mStmt) != ACP_TRUE, LABEL_ABORT_FUNC_SEQ_ERR );
    }
    else
    {
        /*
         * ULN_STATE_EXIT_POINT
         */
        ACI_TEST_RAISE( ulnStmtIsPrepared(aFnContext->mHandle.mStmt) != ACP_TRUE, LABEL_ABORT_FUNC_SEQ_ERR );

        /*
         * BUGBUG: To check the current result is not the last result.
         *         For now, just assume that the current result is the last or only result.
         *         Or there is no current result.
         */

        /* [1] */
        switch (ULN_FNCONTEXT_GET_RC(aFnContext))
        {
            case SQL_ERROR:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                break;

            case SQL_NO_DATA:
            case SQL_SUCCESS:
            case SQL_SUCCESS_WITH_INFO:
                if (ulnStateCheckR(aFnContext) == ACP_TRUE)
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S5);
                    ulnCursorOpen(&sStmt->mCursor);
                }
                else
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S4);
                }
                break;

            case SQL_NEED_DATA:
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S8);
                break;

            default:
                /*
                 * Something went wrong
                 */
                ACI_RAISE(LABEL_MEM_MANAGE_ERR);
                break;
	}
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_FUNC_SEQ_ERR)
    {
        /*
         * HY010
         */
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "Unknown SQLRETURN value. Stack corruption.");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ULN_SFID_22
 *      24000 [p]
 *      (HY010) [np]
 */
ACI_RC ulnSFID_22(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        if (ulnStmtIsPrepared(aFnContext->mHandle.mStmt) == ACP_TRUE)
        {
            ACI_RAISE(LABEL_INVALID_CURSOR_STATE);
        }
        else
        {
            ACI_RAISE(LABEL_ABORT_FUNC_SEQ_ERR);
        }
    }
    else
    {
        /*
         * Something went seriously wrong. This piece of code must not be reached.
         */
        ACI_RAISE(LABEL_MEM_MANAGE_ERR);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_STATE)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_INVALID_CURSOR_STATE,
                 "Cursor is still open. SQLCloseCursor() needs to be called.");
    }
    ACI_EXCEPTION(LABEL_ABORT_FUNC_SEQ_ERR)
    {
        /*
         * HY010
         */
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "The code which should never be reached has been reached.");
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

/*
 * ULN_SFID_23
 *      (24000) [p], [1]
 *      (HY010) [np]
 *  where
 *      [1] This error is returned by the Driver Manager if SQLFetch or SQLFetchScroll has not
 *          returned SQL_NO_DATA, and is returned by the driver if SQLFetch or SQLFetchScroll has
 *          returned SQL_NO_DATA.
 */
ACI_RC ulnSFID_23(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        if (ulnStmtIsPrepared(aFnContext->mHandle.mStmt) == ACP_TRUE)
        {
            /*
             * BUGBUG : How can I get to know if SQLFetch or SQLFetchScroll has returned
             *          SQL_NO_DATA?
             */
            ACI_RAISE(LABEL_INVALID_CURSOR_STATE);
        }
        else
        {
            ACI_RAISE(LABEL_ABORT_FUNC_SEQ_ERR);
        }
    }
    else
    {
        /*
         * Something went seriously wrong. This piece of code must not be reached.
         */
        ACI_RAISE(LABEL_MEM_MANAGE_ERR);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_CURSOR_STATE)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_INVALID_CURSOR_STATE,
                 "Cursor is still open. SQLCloseCursor() needs to be called.");
    }
    ACI_EXCEPTION(LABEL_ABORT_FUNC_SEQ_ERR)
    {
        /*
         * HY010
         */
        ulnError(aFnContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }
    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "The code which should never be reached has been reached.");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static void ulnExecUpdateSqlReturn(ulnFnContext *aFnContext, acp_uint32_t aFlag)
{
    aFnContext->mArrayExecutionResult |= aFlag;

    switch (aFnContext->mArrayExecutionResult)
    {
        case ULN_ARRAY_EXEC_RES_ERROR:
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_ERROR);
            break;

        case ULN_ARRAY_EXEC_RES_SUCCESS:
            if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS)
            {
                ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS);
            }
            break;

        case ULN_ARRAY_EXEC_RES_SUCCESS_WITH_INFO:
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS_WITH_INFO);
            break;
    }
}

ACI_RC ulnExecProcessErrorResult(ulnFnContext *aFnContext, acp_uint32_t aRealErrorRowNumber)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    ulnStmtUpdateAttrParamsRowCountsValue(sStmt, SQL_ERROR);
    ulnStmtSetAttrParamStatusValue(sStmt,
                                   aRealErrorRowNumber - 1,
                                   (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON) ? SQL_USHRT_MAX : SQL_PARAM_ERROR);

    ulnExecUpdateSqlReturn(aFnContext, ULN_ARRAY_EXEC_RES_ERROR);

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackExecuteResult(cmiProtocolContext *aPtContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext)
{
    ulnFnContext          *sFnContext = (ulnFnContext *)aUserContext;
    ulnStmt               *sStmt      = sFnContext->mHandle.mStmt;

    acp_uint32_t           sStatementID;
    acp_uint32_t           sRowNumber;
    acp_uint16_t           sResultSetCount;
    acp_sint64_t           sAffectedRowCount;
    acp_sint64_t           sFetchedRowCount;
    acp_uint8_t            sIsSimpleSelectExecute = 0;
    acp_uint64_t           sSCN = 0;

    // PROJ-2727
    acp_uint16_t           sAttributeID;
    acp_uint32_t           sAttributeLen;
    acp_char_t             sAttributeStr[ULN_MAX_VALUE_LEN + 1] = { '\0', };

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD4(aPtContext, &sStatementID);
    CMI_RD4(aPtContext, &sRowNumber);
    CMI_RD2(aPtContext, &sResultSetCount);
    CMI_RD8(aPtContext, (acp_uint64_t *)&sAffectedRowCount);
    CMI_RD8(aPtContext, (acp_uint64_t *)&sFetchedRowCount);
    CMI_RD8(aPtContext, &sSCN);  /* PROJ-2733-Protocol */
    // PROJ-2727
    CMI_RD2(aPtContext, &sAttributeID);
    CMI_RD4(aPtContext, &sAttributeLen);
    
    if ( sAttributeLen != 0 )
    {
        CMI_RCP(aPtContext, sAttributeStr, sAttributeLen);
        sAttributeStr[sAttributeLen] = 0;

        sStmt->mAttributeID  = sAttributeID;
        sStmt->mAttributeLen = sAttributeLen;
        if ( sStmt->mAttributeStr != NULL )
        {
            acpMemFree( sStmt->mAttributeStr );
            sStmt->mAttributeStr = NULL;
        }
        ACI_TEST( acpMemAlloc((void**)&sStmt->mAttributeStr, sAttributeLen + 1)
                  != ACP_RC_SUCCESS );
        
        acpMemCpy(sStmt->mAttributeStr, sAttributeStr, sAttributeLen);
        sStmt->mAttributeStr[sAttributeLen]='\0';
    }
    else
    {
        // nothing to do
    }

    if (sSCN > 0)
    {
        ulsdUpdateSCN(sStmt->mParentDbc, &sSCN);  /* PROJ-2733-DistTxInfo */
    }

    /* PROJ-2616 */
    if (cmiGetLinkImpl(aPtContext) == CMI_LINK_IMPL_IPCDA)
    {
        CMI_RD1(aPtContext, sIsSimpleSelectExecute);
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-45967 Data Node의 Shard Session 정리 */
    ACI_TEST( ulnCallbackExecuteResultInternal(aPtContext,
                                               aUserContext,
                                               sStatementID,
                                               sRowNumber,
                                               sResultSetCount,
                                               sAffectedRowCount,
                                               sFetchedRowCount,
                                               sIsSimpleSelectExecute)
              != ACI_SUCCESS );

    // PROJ-2727
    if ( sAttributeLen != 0 )
    {
        ACI_TEST( ulnSetConnAttributeToDbc( sFnContext, sStmt)
                != ACI_SUCCESS );
    }
    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if ( sStmt->mAttributeStr != NULL )
    {
        acpMemFree( sStmt->mAttributeStr );
    }
    else
    {
        /* Nothing to do */
    }
    
    sStmt->mAttributeID  = 0;
    sStmt->mAttributeLen = 0;

    return ACI_FAILURE;
}

ACI_RC ulnCallbackExecuteResultInternal(cmiProtocolContext *aProtocolContext,
                                        void               *aUserContext,
                                        acp_uint32_t        aStatementID,
                                        acp_uint32_t        aRowNumber,
                                        acp_uint16_t        aResultSetCount,
                                        acp_sint64_t        aAffectedRowCount,
                                        acp_sint64_t        aFetchedRowCount,
                                        acp_uint8_t         aIsSimpleSelectExecute)
{
    ulnFnContext          *sFnContext  = (ulnFnContext *)aUserContext;
    ulnStmt               *sStmt       = sFnContext->mHandle.mStmt;
    ulnResult             *sResult     = NULL;

    ulnResultType          sResultType = ULN_RESULT_TYPE_UNKNOWN;

    /* PROJ-2616 */
    acp_uint64_t           sDataLength            = 0;
    acp_uint8_t           *sSimpleQueryFetchBlock = aProtocolContext->mSimpleQueryFetchIPCDAReadBlock.mData;

    ACP_UNUSED(aStatementID);

    if (sStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_ON)
    {
        ulnUpdateDeferredState(sFnContext, sStmt);
    }

    /* PROJ-2616 */
    sStmt->mCacheIPCDA.mRemainingLength = 0;
    sStmt->mCacheIPCDA.mReadLength   = 0;

    /* PROJ-2616 */
    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        sStmt->mIsSimpleQuerySelectExecuted = (aIsSimpleSelectExecute==1) ? ACP_TRUE : ACP_FALSE;

        if (sStmt->mIsSimpleQuerySelectExecuted == ACP_TRUE)
        {
            sDataLength = *((acp_uint64_t*)sSimpleQueryFetchBlock);
            sStmt->mCacheIPCDA.mRemainingLength = (acp_uint32_t)sDataLength;
            sStmt->mCacheIPCDA.mReadLength   = 0;

            if (sStmt->mCacheIPCDA.mIsAllocFetchBuffer == ACP_TRUE)
            {
                ACE_DASSERT(sStmt->mCacheIPCDA.mFetchBuffer != NULL);

                if (sStmt->mCacheIPCDA.mRemainingLength > 0)
                {
                    acpMemCpy(sStmt->mCacheIPCDA.mFetchBuffer,
                              (acp_uint8_t*)(sSimpleQueryFetchBlock + 8),
                              sStmt->mCacheIPCDA.mRemainingLength);
                }
                else
                {
                    /* do nothing. */
                }
            }
            else
            {
                sStmt->mCacheIPCDA.mFetchBuffer = (acp_uint8_t*)(sSimpleQueryFetchBlock + 8);
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    /*
     * -----------------------
     * ResultSet Count 설정
     * -----------------------
     */

    ulnStmtSetResultSetCount(sStmt, aResultSetCount);

    /*
     * -----------------------
     * new result 생성, 매달기
     * -----------------------
     */

    sResult = ulnStmtGetNewResult(sStmt);
    ACI_TEST_RAISE(sResult == NULL, LABEL_MEM_MAN_ERR);

    sResult->mAffectedRowCount = aAffectedRowCount;
    sResult->mFetchedRowCount  = aFetchedRowCount;
    sResult->mFromRowNumber    = aRowNumber - 1;
    sResult->mToRowNumber      = aRowNumber - 1;

    // BUG-38649
    if (aAffectedRowCount != ULN_DEFAULT_ROWCOUNT)
    {
        sResultType |= ULN_RESULT_TYPE_ROWCOUNT;
    }
    else
    {
        /* do nothing */
    }
    if ( aResultSetCount > 0 )
    {
        ulnCursorSetServerCursorState(&sStmt->mCursor, ULN_CURSOR_STATE_OPEN);
        sResultType |= ULN_RESULT_TYPE_RESULTSET;
    }
    else
    {
        /* do nothing */
    }
    ulnResultSetType(sResult, sResultType);

    ulnStmtAddResult(sStmt, sResult);

    if (aAffectedRowCount == 0)
    {
        ulnStmtUpdateAttrParamsRowCountsValue(sStmt, SQL_NO_DATA);

        if (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON)
        {
            ulnStmtSetAttrParamStatusValue(sStmt, aRowNumber - 1, 0);
        }
        else if (ulnStmtGetStatementType(sStmt) == ULN_STMT_UPDATE ||
                 ulnStmtGetStatementType(sStmt) == ULN_STMT_DELETE)
        {
            ulnStmtSetAttrParamStatusValue(sStmt, aRowNumber - 1, SQL_NO_DATA);
            if (ULN_FNCONTEXT_GET_RC((sFnContext)) != SQL_ERROR)
            {
                ULN_FNCONTEXT_SET_RC((sFnContext), SQL_NO_DATA);
            }
        }
    }
    else if (aAffectedRowCount > 0)
    {
        if (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON)
        {
            ulnStmtSetAttrParamStatusValue(sStmt, aRowNumber - 1, ACP_MIN(aAffectedRowCount, SQL_USHRT_MAX - 1));
        }
        sStmt->mTotalAffectedRowCount += aAffectedRowCount;
    }
    else
    {
        /* do nothing */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnCallbackExecuteResultInternal");
    }

    ACI_EXCEPTION_END;

    /*
     * Note : callback 이므로 ACI_SUCCESS 를 리턴해야 한다.
     */

    return ACI_SUCCESS;
}

static ACI_RC ulnExecuteFlushChunkAndWriteREQ( ulnFnContext *aFnContext,
                                               ulnPtContext *aPtContext,
                                               ulnStmt      *aStmt,
                                               acp_uint32_t  aRowNumber,
                                               acp_uint8_t   aExecuteOption)
{
    acp_uint32_t sRowCount = 0;

    if ((aStmt->mIsSimpleQuery == ACP_TRUE) &&
        (cmiGetLinkImpl(&(aPtContext->mCmiPtContext)) == CMI_LINK_IMPL_IPCDA))
    {
        ACI_TEST( ulnWriteParamDataInListREQForSimpleQuery( aFnContext,
                                                            aPtContext,
                                                            aExecuteOption )
                  != ACI_SUCCESS );
        aStmt->mCacheIPCDA.mIsParamDataInList = ACP_TRUE;
    }
    else
    {
        sRowCount = ulnStmtChunkGetRowCount( aStmt );

        if( sRowCount != 0 )
        {
            ACI_TEST( ulnWriteParamDataInListREQ( aFnContext,
                                                  aPtContext,
                                                  aRowNumber - sRowCount + 2,
                                                  aRowNumber + 1,
                                                  aExecuteOption )
                      != ACI_SUCCESS );
        }
    }

    ulnStmtChunkReset( aStmt );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnExecProcessArrayBegin(ulnFnContext *aFnContext,
                                       ulnPtContext *aPtContext,
                                       ulnStmt      *aStmt)
{
    ulnDescRec   *sDescRecApd;
    ulnPDContext *sPDContext;

    if ( aStmt->mProcessingRowNumber == 0 &&
         aStmt->mProcessingLobRowNumber == 0 &&
         aStmt->mProcessingParamNumber == 1 )
    {
        /*
         * PutData 를 이용할 때 여러번 들어올 수 있으므로, 처음 한번만 ARRAY_BEGIN 을 해야 한다.
         */

        sDescRecApd = ulnDescGetDescRec(aStmt->mAttrApd, 1);
        ACI_TEST_RAISE(sDescRecApd  == NULL, LABEL_NOT_BOUND);

        sPDContext = &sDescRecApd->mPDContext;

        if (ulnPDContextGetState(sPDContext) == ULN_PD_ST_INITIAL)
        {
            /*
             * ARRAY EXECUTE BEGIN
             *
             * Note : array exec begin end 의 row number 는 서버에서 무시한다.
             */
            ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                        aPtContext,
                                        aStmt->mStatementID,
                                        0,  // row number
                                        CMP_DB_EXECUTE_ARRAY_BEGIN) != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_BOUND)
    {
        ulnErrorExtended(aFnContext,
                         aStmt->mProcessingRowNumber + 1,
                         aStmt->mProcessingParamNumber,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnExecProcessArrayEnd(ulnFnContext *aFnContext,
                                     ulnPtContext *aPtContext,
                                     ulnStmt      *aStmt)
{
    /*
     * BUGBUG : 이거 버그다!!/
     */

    /*
     * Data at exec Parameter 를 처리하면서 마지막 파라미터를 처리하면
     * mProcessingParamNumber 를 1 로 다시 초기화 시킨다.       <------------- 여기서 1 로
     *                                                                         초기화하는것도 버그
     * SQL_NEED_DATA 를 리턴할 것이면 어쨌든 이곳에는 오지 않는다.
     */

    /*
     * PutData 를 이용할 때 여러번 들어올 수 있으므로, 마지막 한번만 ARRAY_END 을 해야 한다.
     */

    if (aStmt->mProcessingParamNumber == 1) // <------------- 1 을 검사하는 것 버그다.
    {
        /*
         * ARRAY EXECUTE END
         *
         * Note : array exec begin end 의 row number 는 서버에서 무시한다.
         */
        ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                    aPtContext,
                                    aStmt->mStatementID,
                                    0,  // row number
                                    CMP_DB_EXECUTE_ARRAY_END) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * BIND PARAM DATA IN 을 하고, EXECUTE REQ 를 날린다.
 */
ACI_RC ulnExecuteCore(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnStmt      *sStmt            = aFnContext->mHandle.mStmt;
    acp_uint8_t   sExecuteOption   = CMP_DB_EXECUTE_NORMAL_EXECUTE;
    acp_uint32_t  sParamNumber;
    acp_uint16_t  sParamCount      = ulnBindCalcNumberOfParamsToSend(sStmt);
    acp_uint32_t  sParamSetSize    = ulnDescGetArraySize(sStmt->mAttrApd);
    acp_uint32_t  sRowNumber       = 0;
    ulnDescRec   *sDescRecApd;
    ulnDesc      *sDescIrd;

    /* PROJ-2616 */
    cmiProtocolContext *sCtx = &aPtContext->mCmiPtContext;

    /* BUG-46011 If deferred prepare is exists, process it first */
    if (ulnStmtIsSetDeferredQstr(sStmt) == ACP_TRUE)
    {
        ACI_TEST( ulnPrepareDeferComplete(aFnContext, ACP_FALSE) );
    }
    
    /*
     * Note : SQL_NO_DATA 리턴을 위해 총 affected row count 를 유지해야 한다.
     */

    sStmt->mTotalAffectedRowCount = 0;
    ulnStmtFreeAllResult(sStmt);
    ulnStmtChunkReset( sStmt );
    ulnStmtInitAttrParamsRowCounts(sStmt);

    /* PROJ-1789 Updatable Scrollable Cursor
     * Rowset Cache를 위한 Stmt 생성 */
    if ( (ulnCursorGetType(&sStmt->mCursor) == SQL_CURSOR_KEYSET_DRIVEN)
     &&  (sStmt->mRowsetStmt == SQL_NULL_HSTMT) )
    {
        ACI_TEST_RAISE(ulnDbcGetStmtCount(sStmt->mParentDbc)
                       >= ULN_DBC_MAX_STMT, LABEL_TOO_MANY_STMT_EXCEPTION);
        ACI_TEST_RAISE(ulnStmtCreate(sStmt->mParentDbc, &sStmt->mRowsetStmt)
                       != ACI_SUCCESS, LABEL_MEMORY_ALLOC_EXCEPTION);
        ulnStmtInitialize(sStmt->mRowsetStmt);
#ifdef COMPILE_SHARDCLI
        /* BUG-47050 */
        ulsdInitalizeNodeStmt(sStmt->mShardStmtCxt.mParentStmt, sStmt->mRowsetStmt);
#endif
        ulnStmtSetAttrCursorHold(sStmt->mRowsetStmt, SQL_CURSOR_HOLD_OFF);
        ulnDbcAddStmt(sStmt->mParentDbc, sStmt->mRowsetStmt);
        sStmt->mRowsetStmt->mParentStmt = sStmt;

        /* RowsetStmt는 생성할때 Cache도 미리 만들어 둔다. */
        ACI_TEST_RAISE(ulnStmtCreateCache(sStmt->mRowsetStmt)
                       != ACI_SUCCESS, LABEL_MEMORY_ALLOC_EXCEPTION);
    }

    if (sParamCount > 0)
    {
        if (sParamSetSize > 1)
        {
            /*
             * -----------------------------
             * ARRAY EXECUTE BEGIN
             * -----------------------------
             */

            // PROJ-1518
            if(ulnStmtGetAtomicArray(sStmt)   == SQL_TRUE &&
               ulnStmtGetStatementType(sStmt) == ULN_STMT_INSERT)
            {
                    ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                                aPtContext,
                                                sStmt->mStatementID,
                                                0,
                                                CMP_DB_EXECUTE_ATOMIC_BEGIN)
                                != ACI_SUCCESS);

                    sExecuteOption = CMP_DB_EXECUTE_ATOMIC_EXECUTE;
            }
            else
            {
                ACI_TEST(ulnExecProcessArrayBegin(aFnContext,
                                                  aPtContext,
                                                  sStmt) != ACI_SUCCESS);

                sExecuteOption = CMP_DB_EXECUTE_ARRAY_EXECUTE;
            }

        }

        /* 
         * BUG-42096 A user's app with CLI dies when Array insert is executing.
         * 
         * ParamSetSize의 변화에 따라 ulnParamProcess_INFOs()가 필요하다.
         */
        if (sParamSetSize > ulnStmtGetExecutedParamSetMaxSize(sStmt))
        {
            ulnStmtSetExecutedParamSetMaxSize(sStmt, sParamSetSize);
            ulnStmtSetBuildBindInfo(sStmt, ACP_TRUE);
        }
        else
        {
            /* Nothing */
        }

#ifdef COMPILE_SHARDCLI
        /*
         * PROJ-2739 Client-side Sharding LOB
         *   Locator를 IN BIND 한 경우 값에 따라, 즉 src locator의 노드와
         *   execute 되는 노드가 다른 경우, OUT으로 바뀔 수 있으므로
         *   ulnParamProcess_INFOs()가 필요하다.
         */
        if ( ulsdLobHasLocatorInBoundParam(sStmt) == ACP_TRUE)
        {
            ulnStmtSetBuildBindInfo(sStmt, ACP_TRUE);
        }
        else
        {
            /* Nothing */
        }
#endif

        /*
         * -----------------------------
         * BIND INFO SET
         * -----------------------------
         */
        if( ulnStmtGetBuildBindInfo( sStmt ) == ACP_TRUE )
        {
            ACI_TEST( ulnParamProcess_INFOs(aFnContext,
                                            aPtContext,
                                            sRowNumber)
                      != ACI_SUCCESS );
        }

        /*
         * BIND DATA 와 EXECUTE REQ 전송
         */
        for (sRowNumber = sStmt->mProcessingRowNumber;
             sRowNumber < sParamSetSize;
             sRowNumber++)
        {
            sStmt->mProcessingRowNumber = sRowNumber;

            /*
             * Note : APD 의 rows processed ptr : for SQLPutData() 와 SQLParamData()
             */
            ulnDescSetRowsProcessedPtrValue(sStmt->mAttrApd,
                                            sRowNumber);

            if (ulnStmtGetAttrParamOperationValue(sStmt,
                                                  sRowNumber)
                != SQL_PARAM_PROCEED)
            {
                ulnStmtUpdateAttrParamsRowCountsValue(sStmt, SQL_NO_DATA);
                ulnStmtSetAttrParamStatusValue(sStmt,
                                               sRowNumber,
                                               (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON) ? 0 : SQL_PARAM_UNUSED);

                ACI_TEST( ulnExecuteFlushChunkAndWriteREQ( aFnContext,
                                                           aPtContext,
                                                           sStmt,
                                                           sRowNumber - 1,
                                                           sExecuteOption )
                          != ACI_SUCCESS );

                continue;
            }

            /*
             * -----------------------------
             * BIND DATA IN
             * -----------------------------
             */

            /* check DATA_AT_EXEC_PARAM */
            for (sParamNumber = 1; sParamNumber <= sParamCount; sParamNumber++)
            {
                sDescRecApd = ulnStmtGetApdRec(sStmt, sParamNumber);

#ifdef COMPILE_SHARDCLI
                if ( ulsdTypeIsLocatorCType(ulnMetaGetCTYPE(&sDescRecApd->mMeta)) == ACP_TRUE )
                {
                    /* ulsdLobLocator 임시 저장소인 DescRec->mShardLocator를 NULL로 초기화한다 */
                    ulsdDescRecInitShardLobLocator(
                            sStmt,
                            sParamNumber,
                            ulnDescRecGetParamInOut(ulnStmtGetIpdRec(sStmt, sParamNumber)),
                            ulnBindCalcUserDataAddr(sDescRecApd, sRowNumber) );
                }
                else
                {
                    /* Nothing to do. */
                }
#endif

                if( ulnDescRecIsDataAtExecParam(sDescRecApd,
                                                sRowNumber) == ACP_TRUE )
                {
                    sStmt->mHasDataAtExecParam |= ULN_HAS_DATA_AT_EXEC_PARAM;

                    ACI_TEST( ulnExecuteFlushChunkAndWriteREQ( aFnContext,
                                                               aPtContext,
                                                               sStmt,
                                                               sRowNumber - 1,
                                                               sExecuteOption )
                            != ACI_SUCCESS );
                    break;
                }
            }

            if( (sStmt->mHasDataAtExecParam & ULN_HAS_DATA_AT_EXEC_PARAM) ==
                 ULN_HAS_DATA_AT_EXEC_PARAM )
            {
                ACI_TEST(ulnBindDataWriteRow(aFnContext,
                                             aPtContext,
                                             sStmt,
                                             sRowNumber)
                         != ACI_SUCCESS);

                ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                            aPtContext,
                                            sStmt->mStatementID,
                                            sRowNumber,
                                            sExecuteOption)
                         != ACI_SUCCESS);
            }
            else
            {
                if( (sStmt->mInOutType & ULN_PARAM_INOUT_TYPE_INPUT) ==
                    ULN_PARAM_INOUT_TYPE_INPUT )
                {
                    if ((cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA) &&
                        (sStmt->mIsSimpleQuery == ACP_TRUE))
                    {
                        /*PROJ-2616*/
                        /* 이 경우에는 실제 데이터는 전송시에 작성 된다. */
                        /* In ulnExecuteFlushChunkAndWriteREQ, datas will be writed. */
                        ulnStmtChunkIncRowCount( sStmt );
                    }
                    else
                    {
                        if (ulnParamProcess_DATAs(aFnContext,
                                                  aPtContext,
                                                  sRowNumber) != ACI_SUCCESS)
                        {
                            // row 처리수는 오류건 포함하여 정상적으로 증가
                            ulnStmtIncreaseParamsProcessedValue(sStmt);
                            ulnStmtUpdateAttrParamsRowCountsValue(sStmt, SQL_ERROR);
                            ulnStmtSetAttrParamStatusValue(sStmt,
                                                           sRowNumber,
                                                           (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON) ? SQL_USHRT_MAX : SQL_PARAM_ERROR);

                            // array 이면 에러건만 skip 하고 다음건 처리
                            if ((sParamSetSize > 1) &&
                                (ulnStmtGetAtomicArray(sStmt) == SQL_FALSE))
                            {
                                continue;
                            }
                            // array 가 아니면 정상적으로 에러처리
                            else
                            {
                                ACI_TEST(ACP_TRUE);
                            }
                        }
                    }
                }
                else
                {
                    ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                                aPtContext,
                                                sStmt->mStatementID,
                                                sRowNumber,
                                                sExecuteOption)
                             != ACI_SUCCESS);
                }

                if( (sStmt->mInOutTypeWithoutBLOB & ULN_PARAM_INOUT_TYPE_OUTPUT) ==
                    ULN_PARAM_INOUT_TYPE_OUTPUT )
                {
                    ACI_TEST( ulnBindDataSetUserIndLenValue( sStmt,
                                                             sRowNumber)
                              != ACI_SUCCESS );
                }
            }

            /*
            * 처리된 parameter set 의 갯수를 증가시킴
            */
            ulnStmtIncreaseParamsProcessedValue(sStmt);

            /* ExecuteResult를 받으면 그에 따라 다시 써질것. 여기는 초기화. */
            ulnStmtSetAttrParamStatusValue(sStmt,
                                           sRowNumber,
                                           (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON) ? 0 : SQL_PARAM_SUCCESS);
        } // array 수만큼 loop

        ACI_TEST( ulnExecuteFlushChunkAndWriteREQ( aFnContext,
                                                   aPtContext,
                                                   sStmt,
                                                   sRowNumber - 1,
                                                   sExecuteOption )
                  != ACI_SUCCESS );

        if (sParamSetSize > 1)
        {
            /*
             * -----------------------------
             * ARRAY EXECUTE END
             * -----------------------------
             */

            // PROJ-1518
            if(ulnStmtGetAtomicArray(sStmt)   == SQL_TRUE &&
               ulnStmtGetStatementType(sStmt) == ULN_STMT_INSERT)
            {
                    ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                                aPtContext,
                                                sStmt->mStatementID,
                                                0,
                                                CMP_DB_EXECUTE_ATOMIC_END)
                                != ACI_SUCCESS);
            }
            else
            {
                ACI_TEST(ulnExecProcessArrayEnd(aFnContext, aPtContext, sStmt) != ACI_SUCCESS);
            }
        }
    }
    else
    {
        /*
         * parameter 갯수가 0 인 경우에는 그냥 execute
         */
        ACI_TEST(ulnWriteExecuteREQ(aFnContext,
                                    aPtContext,
                                    sStmt->mStatementID,
                                    0,
                                    CMP_DB_EXECUTE_NORMAL_EXECUTE) != ACI_SUCCESS);
    }

    // fix BUG-17807
    // ExecDirect의 경우 SQL타입을 알 수 없으므로 컬럼 정보를 다시 요청해야 한다.

    // BUG-17592
    if ( ( ulnStmtGetStatementType(sStmt) == 0 ) ||
         ( ulnStmtGetStatementType(sStmt) == ULN_STMT_EXEC_PROC ) ||
         ( ulnStmtGetStatementType(sStmt) == ULN_STMT_EXEC_FUNC ) )
    {
        /* BUG-45186 ulnWriteColumnInfoGetREQ 호출전에 sDescIrd를 초기화한다.*/
        sDescIrd = ulnStmtGetIrd(sStmt);
        ACI_TEST_RAISE(sDescIrd == NULL, LABEL_MEM_MAN_ERR);
        ACI_TEST_RAISE(ulnDescRollBackToInitial(sDescIrd) != ACI_SUCCESS, LABEL_MEM_MAN_ERR);
        ACI_TEST_RAISE(ulnDescInitialize(sDescIrd, (ulnObject *)sStmt) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);
        
        /*
         * BINDINFO GET REQ
         *
         * Note : PSM의 경우 execute 하기 전까지는 result set의 정보를 알 수 없기 때문에 전송
         */
        ACI_TEST(ulnWriteColumnInfoGetREQ(aFnContext, aPtContext, 0) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_TOO_MANY_STMT_EXCEPTION);
    {
        ulnError(aFnContext, ulERR_ABORT_TOO_MANY_STATEMENT);
    }
    ACI_EXCEPTION(LABEL_MEMORY_ALLOC_EXCEPTION);
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnExecuteCore");
    }
    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnExecuteCore");
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnExecuteCore");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * aRowNumber 는 0 베이스 인덱스 (BUGBUG : row number 가 0 베이스인지 1 베이스인지 통일해야 함)
 */
static ACI_RC ulnExecProcessFileLobParam(ulnFnContext *aFnContext,
                                         ulnPtContext *aPtContext,
                                         ulnDescRec   *aDescRecApd,
                                         ulnLob       *aLob,
                                         acp_uint32_t  aRowNumber)
{
    acp_uint8_t  *sFileName;
    acp_uint32_t *sFileOption;
    ulnLobBuffer  sLobBuffer;

    /*
     * file 일 경우 putdata 로 전송하는 것을 가정하고 있지 않다.
     */

    /*
     * BUFFER 준비
     */

    sFileName   = (acp_uint8_t *)ulnBindCalcUserDataAddr(aDescRecApd, aRowNumber);
    sFileOption = ulnBindCalcUserFileOptionAddr(aDescRecApd, aRowNumber);
    ACI_TEST_RAISE(sFileOption == NULL, LABEL_MEM_MAN_ERR);
    ACI_TEST_RAISE(sFileName   == NULL, LABEL_MEM_MAN_ERR);

    ACI_TEST_RAISE(*sFileOption != SQL_FILE_READ, LABEL_INVALID_FILE_OPTION);

    ulnLobBufferInitialize(&sLobBuffer,
                           NULL,
                           aLob->mType,
                           ULN_CTYPE_FILE,
                           sFileName,
                           *sFileOption);

    ACI_TEST(sLobBuffer.mOp->mPrepare(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * LOB OPEN
     *
     * ParamDataOut 에서 initialize, set locator 함.
     */

    ACI_TEST(aLob->mOp->mOpen(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * 서버로 전송
     */

    ACI_TEST(aLob->mOp->mOverWrite(aFnContext, aPtContext, aLob, &sLobBuffer) != ACI_SUCCESS);

    /*
     * BUFFER 정리
     */

    ACI_TEST(sLobBuffer.mOp->mFinalize(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * LOB LOCATOR FREE
     */

    ACI_TEST(aLob->mOp->mClose(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_FILE_OPTION)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_INVALID_FILE_OPTION,
                         *sFileOption);
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnExecProcessFileLobParam");
    }

    ACI_EXCEPTION_END;

    (void)aLob->mOp->mClose(aFnContext, aPtContext, aLob);

    return ACI_FAILURE;
}

/*
 * aRowNumber 는 0 베이스 인덱스
 */
static ACI_RC ulnExecProcessMemLobParamDataAtExec(ulnFnContext *aFnContext,
                                                  ulnPtContext *aPtContext,
                                                  ulnDescRec   *aDescRecApd,
                                                  ulnLob       *aLob)
{
    ulnPDContext *sPDContext = &aDescRecApd->mPDContext;

    /*
     * DATA AT EXEC 파라미터. 사용자는 SQLPutData() 를 사용하기를 원한다.
     *
     * 여기서는 lob 은 일단 open 만 해 주고,
     * 거시기,,, SQLPutData() 에서 버퍼를 준비하고, 데이터를 보내면 되겠다.
     */

    // idlOS::printf("#### LOB pd state : %d\n", ulnPDContextGetState(sPDContext));

    switch (ulnPDContextGetState(sPDContext))
    {
        case ULN_PD_ST_INITIAL:
            ACI_RAISE(LABEL_INVALID_STATE);
            break;

        case ULN_PD_ST_CREATED:
            ACI_TEST(aLob->mOp->mOpen(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

        case ULN_PD_ST_NEED_DATA:
            ulnPDContextSetState(sPDContext, ULN_PD_ST_NEED_DATA);
            ACI_RAISE(LABEL_NEED_DATA);
            break;

        case ULN_PD_ST_ACCUMULATING_DATA:
            /*
             * 사용자가 SQLParamData() 를 호출해서 Data put 이 끝났음을 알림
             */
            sPDContext->mOp->mFinalize(sPDContext);

            // ulnPDContextSetState(sPDContext, ULN_PD_ST_INITIAL);
            // ACI_TEST(aLob->mOp->mClose(aFnContext, aPtContext, aLob) != ACI_SUCCESS);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnExecProcessMemLobParamDataAtExec");
    }

    ACI_EXCEPTION(LABEL_NEED_DATA)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NEED_DATA);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * row number 0 베이스
 */
static ACI_RC ulnExecProcessMemLobParamNormal(ulnFnContext *aFnContext,
                                              ulnPtContext *aPtContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnLob       *aLob,
                                              acp_uint32_t  aRowNumber)
{
    ulnDbc      *sDbc;
    void        *sUserDataPtr           = NULL;
    ulvSLen     *sUserOctetLengthPtr    = NULL;
    ulvSLen      sUserOctetLength       = ULN_vLEN(0);
    acp_uint32_t sUserDataLength        = 0;
    ulnCTypeID   sCTYPE;

    ulnCharSet   sCharSet;
    ulnLobBuffer sLobBuffer;
    acp_sint32_t sState = 0;

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    /*
     * 사용자 주소 계산
     */

    sUserDataPtr        = ulnBindCalcUserDataAddr(aDescRecApd, aRowNumber);
    /*
     * BUG-28980 [CodeSonar]Null Pointer Dereference
     */
    ACI_TEST( sUserDataPtr == NULL );
    sUserOctetLengthPtr = ulnBindCalcUserOctetLengthAddr(aDescRecApd, aRowNumber);

    if (sUserOctetLengthPtr == NULL)
    {
        /*
         * ODBC spec. SQLBindParameter() StrLen_or_IndPtr argument 의 설명 :
         * 이게 NULL 이면 모든 input parameter 가 non-null 이고,
         * char, binary 데이터는 null-terminated 된 것이라고 가정해야 한다.
         *
         * BUG-13704. SES 의 -n 옵션을 위한 처리. 자세한 내용은
         * SQL_ATTR_INPUT_NTS 가 선언된 헤더파일의 주석을 참고할 것.
         */
        if (ulnStmtGetAttrInputNTS(aFnContext->mHandle.mStmt) == ACP_TRUE)
        {
            sUserOctetLength = SQL_NTS;
        }
        else
        {
            sUserOctetLength = ulnMetaGetOctetLength(&aDescRecApd->mMeta);
        }
    }
    else
    {
        sUserOctetLength = *sUserOctetLengthPtr;
    }

    sCTYPE = ulnMetaGetCTYPE(&aDescRecApd->mMeta);

    ACI_TEST(ulnBindCalcUserDataLen(sCTYPE,
                                    ulnMetaGetOctetLength(&aDescRecApd->mMeta), // user buf size
                                    (acp_uint8_t *)sUserDataPtr,
                                    sUserOctetLength,
                                    (acp_sint32_t *)&sUserDataLength,
                                    NULL) != ACI_SUCCESS);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * BUFFER 초기화 및 준비
     */

    ACI_TEST_RAISE(ulnLobBufferInitialize(&sLobBuffer,
                                          sDbc,
                                          aLob->mType,
                                          sCTYPE,
                                          (acp_uint8_t *)sUserDataPtr,
                                          sUserDataLength) != ACI_SUCCESS,
                   LABEL_INVALID_BUFFER_TYPE);

    ACI_TEST(sLobBuffer.mOp->mPrepare(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * LOB OPEN
     */

    ACI_TEST(aLob->mOp->mOpen(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    /*
     * 서버로 전송
     */

    ACI_TEST(aLob->mOp->mOverWrite(aFnContext, aPtContext, aLob, &sLobBuffer) != ACI_SUCCESS);

    /*
     * BUFFER 정리 : 사용자 메모리 바인드 lob 은 버퍼 정리, prepare 등이 필요 없지만,
     *               형식상 해 주도록 하자.
     */

    ACI_TEST(sLobBuffer.mOp->mFinalize(aFnContext, &sLobBuffer) != ACI_SUCCESS);

    /*
     * LOB LOCATOR FREE
     */

    ACI_TEST(aLob->mOp->mClose(aFnContext, aPtContext, aLob) != ACI_SUCCESS);

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFFER_TYPE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "Given buffer type cannot be used as a ulnLobBuffer. "
                         "ulnFetchGetLobColumnData");
    }

    ACI_EXCEPTION_END;

    (void)aLob->mOp->mClose(aFnContext, aPtContext, aLob);

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

static ACI_RC ulnExecProcessMemLobParam(ulnFnContext *aFnContext,
                                        ulnPtContext *aPtContext,
                                        ulnDescRec   *aDescRecApd,
                                        ulnLob       *aLob,
                                        acp_uint32_t  aRowNumber)
{
    if (ulnDescRecIsDataAtExecParam(aDescRecApd, aRowNumber) == ACP_TRUE)
    {
        ACI_TEST(ulnExecProcessMemLobParamDataAtExec(aFnContext,
                                                     aPtContext,
                                                     aDescRecApd,
                                                     aLob) != ACI_SUCCESS);
    }
    else
    {
        ACI_TEST(ulnExecProcessMemLobParamNormal(aFnContext,
                                                 aPtContext,
                                                 aDescRecApd,
                                                 aLob,
                                                 aRowNumber) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnExecSendLobParamData(ulnFnContext *aFnContext,
                                      ulnPtContext *aPtContext,
                                      ulnDescRec   *aDescRecApd,
                                      ulnDescRec   *aDescRecIpd)
{
    acp_uint32_t  sRowNumber;
    ulnStmt      *sStmt;
    ulnLob       *sLob;

    sStmt = aFnContext->mHandle.mStmt;

    /*
     * Note : mLobLocatorArrayCount 는 사용자가 지정한 SQL_DESC_ARRAY_SIZE 만큼이다.
     *        즉, 여러개의 파일을 바인드하려면 array binding 기능을 사용하도록 한다.
     */

    for ( sRowNumber = sStmt->mProcessingLobRowNumber;
          sRowNumber < ulnDescGetArraySize(sStmt->mAttrApd);
          sRowNumber++ )
    {
        sStmt->mProcessingLobRowNumber = sRowNumber;

        /*
         * BUGBUG : 사용자가 array binding 을 해서 SQL_DATA_AT_EXEC 파라미터로 주고,
         *          SQLParamData, SQLPutData 를 했을 경우 이 포인터를 NULL 로 두면
         *          몇번 row 의 data 를 줘야 할지 알 길이 없다.
         *
         *          error 나 success_with_info 를 리턴해 줘야 할까,
         *          아니면, 그냥 알아서 하겠지 하고 넘어갈까..
         */
        ulnDescSetRowsProcessedPtrValue(sStmt->mAttrApd, sRowNumber);

        sLob = ulnDescRecGetLobElement(aDescRecIpd, sRowNumber);
        ACI_TEST_RAISE(sLob == NULL, LABEL_MEM_MAN_ERR);

        /*
         * BUGBUG : 지금은 시간도 없고, 루틴이 많이 복잡해서 일단 ulnLobBuffer 추상화는
         *          대충 집어넣어 두자.
         *
         *          꼭!!! 리팩토링 해서 깔끔하게 만들어야 한다.
         *
         *          젠장, ParamData() 하고, PutData(), Execute() 상호 작용하는게 너무 복잡하다.
         *
         * Note : ulnLob 의 할당은 BindParamInfo 시에,
         *          ulnExecDoParamInfoSet() 함수에서,
         *        ulnLob 의 초기화 및 로케이터 세팅은 ParamDataOut 시에
         *          ulnBindStoreLobLocator() 함수에서,
         *        그리고, 실제 데이터의 전송은 이 함수에서 한다.
         */

        if (ulnMetaGetCTYPE(&aDescRecApd->mMeta) == ULN_CTYPE_FILE)
        {
            ACI_TEST(ulnExecProcessFileLobParam(aFnContext,
                                                aPtContext,
                                                aDescRecApd,
                                                sLob,
                                                sRowNumber) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST(ulnExecProcessMemLobParam(aFnContext,
                                               aPtContext,
                                               aDescRecApd,
                                               sLob,
                                               sRowNumber) != ACI_SUCCESS);
        }
    } /* for (sRowNumber = mStmt->mProcessingLobRowNumber; ... */

    /* 
     * BUG-41862 Lob row number needs to be initialized.(refer to BUG-25410)
     */
    if ( sRowNumber >= ulnDescGetArraySize(sStmt->mAttrApd) )
    {
        sStmt->mProcessingLobRowNumber = 0;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         sRowNumber + 1,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnExecSendLobParamData");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnExecLobPhase(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    acp_uint16_t i;
    acp_uint16_t sNumberOfParams;      /* 파라미터의 갯수 */

    ulnDescRec  *sDescRecIpd;
    ulnDescRec  *sDescRecApd;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(aFnContext->mHandle.mStmt);

    for (i = aFnContext->mHandle.mStmt->mProcessingParamNumber; i <= sNumberOfParams; i++)
    {
        aFnContext->mHandle.mStmt->mProcessingParamNumber = i;

        sDescRecApd = ulnStmtGetApdRec(aFnContext->mHandle.mStmt, i);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NEED_BINDPARAMETER_ERR);

        sDescRecIpd = ulnStmtGetIpdRec(aFnContext->mHandle.mStmt, i);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        if (ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecIpd->mMeta),
                                 ulnMetaGetCTYPE(&sDescRecApd->mMeta)) == ACP_TRUE)
        {
            ACI_TEST_RAISE(aFnContext->mHandle.mStmt->mParentDbc->mAttrAutoCommit
                           != SQL_AUTOCOMMIT_OFF,
                           LABEL_AUTOCOMMIT_MODE_LOB);

            /*
             * 사용자가 LOB 컬럼을 메모리 혹은 file로 바인드함.
             */
            ACI_TEST(ulnExecSendLobParamData(aFnContext,
                                             aPtContext,
                                             sDescRecApd,
                                             sDescRecIpd) != ACI_SUCCESS);
        }
    }

    if (i > sNumberOfParams)
    {
        aFnContext->mHandle.mStmt->mProcessingParamNumber = 1;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NEED_BINDPARAMETER_ERR)
    {
        ulnErrorExtended(aFnContext,
                         SQL_NO_ROW_NUMBER,
                         i,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }

    ACI_EXCEPTION(LABEL_AUTOCOMMIT_MODE_LOB)
    {
        ulnErrorExtended(aFnContext,
                         SQL_NO_ROW_NUMBER,
                         i,
                         ulERR_ABORT_LOB_AUTOCOMMIT_MODE_ERR);
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         SQL_NO_ROW_NUMBER,
                         i,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ExecLobPhase");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnExecuteLob(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    if (aFnContext->mHandle.mStmt->mHasMemBoundLobParam == ACP_TRUE)
    {
        ACI_TEST(ulnExecLobPhase(aFnContext, aPtContext) != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * BUGBUG : Array binding 시 서버로 REQ 와 DATA 를 보낼 때 에러가 발생하면 무조건
 *          SQL_ERROR 이다.
 *          원래는 SQL_SUCCESS_WITH_INFO 를 리턴하고 PARAM_STATUS_PTR 이 가리키는 배열의
 *          해당 원소에 SQL_PARAM_ERROR 를 넣어 주어야 한다.
 *          머리아프다 -_-;;
 *          다음에 버그로 처리하도록 하고, 지금은 우선,
 *          무조건 SQL_ERROR 를 리턴하고 튀어나가고,
 *          성공하면 모둔 원소가 SQL_PARAM_SUCCESS 혹은 SQL_PARAM_UNUSED 로 세팅되어 있도록
 *          한다.
 *          혹은 매우 단순무식하고 간단하게 SQL_PARAM_DIAG_UNAVALIABLE 로 전부 세팅해
 *          버릴 수도 있으나, 왠지 기분이 나빠서 그렇게 안했다.
 *
 *          서버에서 ERROR RESULT 가 전송되어 오면
 */

SQLRETURN ulnExecute(ulnStmt *aStmt)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;
    
    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    ulnDbc       *sDbc = NULL;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_EXECUTE, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    //fix BUG-17722
    ACI_TEST( ulnInitializeProtocolContext(&sFnContext,
                                           &(aStmt->mParentDbc->mPtContext),
                                           &aStmt->mParentDbc->mSession)
              != ACI_SUCCESS );
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * -----------------------------------------
     * Protocol Context
     */

    ulnStmtResetPD(aStmt);

#ifdef COMPILE_SHARDCLI
    /* PROJ-2739 Client-side Sharding LOB
     *   mHasLocatorParamToCopy 초기화 */
    ulsdLobResetLocatorParamToCopy(aStmt);
#endif

    /* PROJ-2177 User Interface - Cancel
     * 이후에 DataAtExec를 검사해 NEED DATA를 설정하므로 성공했을때를 위해 초기화해둔다. */
    ulnStmtResetNeedDataFuncID(aStmt);

    ulnDescSetRowsProcessedPtrValue(aStmt->mAttrIpd, 0);

    //fix BUG-17722
    ACI_TEST(ulnExecuteCore(&sFnContext, &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

    // BUG-17592
    if ( (ulnStmtHasResultSet(aStmt) == ACP_TRUE) ||
         ((aStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_ON) &&
          (aStmt->mIsSelect == ACP_TRUE)) )
    {
        /* Note : SELECT 성능을 높이기 위해서 prepare -> execute -> fetch 를 한번에 전송 */

        // fix BUG-17715
        // RecordCount를 Prefetch Row크기만큼 설정해 레코드를 받아온다.
        ACI_TEST( ulnFetchRequestFetch(&sFnContext,
                                       &(aStmt->mParentDbc->mPtContext),
                                       ulnStmtGetAttrPrefetchRows(aStmt),
                                       0,
                                       0)
                  != ACI_SUCCESS );
    }
    else
    {
        /* Do nothing */
    }

    /* BUG-36548 Return code of client functions should be differed by ODBC version */
    if ( ulnFlushAndReadProtocol( &sFnContext,
                                  &(aStmt->mParentDbc->mPtContext),
                                  aStmt->mParentDbc->mConnTimeoutValue ) != ACI_SUCCESS )
    {
        ACI_TEST( ULN_FNCONTEXT_GET_RC(&sFnContext ) != SQL_NO_DATA );
        ACI_TEST( sDbc->mAttrOdbcCompatibility == 3 );
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    ACI_TEST( ulnExecuteLob(&sFnContext,
                            &(aStmt->mParentDbc->mPtContext))
              != ACI_SUCCESS );

    ulnStmtSetCurrentResult(aStmt, (ulnResult *)(aStmt->mResultList.mNext));
    ulnStmtSetCurrentResultRowNumber(aStmt, 1);
    ulnDiagSetRowCount(&aStmt->mObj.mDiagHeader, aStmt->mCurrentResult->mAffectedRowCount);

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    ACI_TEST(ulnExecuteBeginFetchAsync(&sFnContext,
                                       &aStmt->mParentDbc->mPtContext)
             != ACI_SUCCESS);

    /*
     * Protocol Context
     * -----------------------------------------
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST( ulnFinalizeProtocolContext(&sFnContext,
                                         &(aStmt->mParentDbc->mPtContext))
              != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| %"ACI_INT32_FMT,
                  "ulnExecute", sFnContext.mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    // fix BUG-19349
    if (aStmt != NULL)
    {
        // fix BUG-19166
        // Array Update시 SQL_NO_DATA가 발생해도 Execute의 Result가 존재하면
        // CurrentResult로 설정한다.
        if (acpListIsEmpty(&aStmt->mResultList) != ACP_TRUE)
        {
            ulnStmtSetCurrentResult(aStmt, (ulnResult *)(aStmt->mResultList.mNext));
            ulnStmtSetCurrentResultRowNumber(aStmt, 1);
            ulnDiagSetRowCount(&aStmt->mObj.mDiagHeader, aStmt->mCurrentResult->mAffectedRowCount);
        }
    }

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        if ((sDbc->mAttrPDODeferProtocols >= 1) &&
            (ULN_FNCONTEXT_GET_RC(&sFnContext) == SQL_NEED_DATA))
        {
            /* BUG-45286 ParamInfoSetList 전송 지연 */
        }
        else
        {
            //fix BUG-17722
            ulnFinalizeProtocolContext(&sFnContext, &(aStmt->mParentDbc->mPtContext));
        }
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail: %"ACI_INT32_FMT,
            "ulnExecute", sFnContext.mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

/**
 * Execute 성공 후 바로 fetch 를 시작하므로 비동기로 fetch 할 수 조건이라면,
 * 비동기 fetch 를 시작한다.
 * (PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning)
 */
ACI_RC ulnExecuteBeginFetchAsync(ulnFnContext *aFnContext,
                                 ulnPtContext *aPtContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    if (ulnStmtHasResultSet(sStmt) == ACP_TRUE)
    {
        if (ulnCanSemiAsyncPrefetch(aFnContext) == ACP_TRUE)
        {
            if (ulnDbcGetAsyncPrefetchStmt(sStmt->mParentDbc) == NULL)
            {
                /* begin semi-async prefetch */
                ACI_TEST(ulnFetchBeginAsync(aFnContext,
                                            aPtContext)
                        != ACI_SUCCESS);
            }
            else
            {
                /* cannot be prefetched asynchronous and will be prefetched synchronous */
                ulnError(aFnContext, ulERR_IGNORE_CANNOT_BE_PREFETCHED_ASYNC);
                ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS_WITH_INFO);
            }
        }
        else
        {
            if (ulnStmtGetAttrPrefetchAsync(sStmt) == ALTIBASE_PREFETCH_ASYNC_PREFERRED)
            {
                /* cannot be prefetched asynchronous and will be prefetched synchronous */
                ulnError(aFnContext, ulERR_IGNORE_CANNOT_BE_PREFETCHED_ASYNC);
                ULN_FNCONTEXT_SET_RC(aFnContext, SQL_SUCCESS_WITH_INFO);
            }
            else
            {
                /* synchronous prefetch */
            }
        }
    }
    else
    {
        /* don't have result set */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

