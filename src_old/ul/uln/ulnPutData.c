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
#include <ulnPDContext.h>
#include <ulnPutData.h>

/*
 * ULN_SFID_46
 * SQLPutData(), STMT, S9
 *
 *      S1 [e] and [1]
 *      S2 [e], [nr], and [2]
 *      S3 [e], [r], and [2]
 *      S5 [e] and [4]
 *      S6 [e] and [5]
 *      S7 [e] and [3]
 *      S10 [s]
 *      S11 [x]
 *
 * where
 *      [1] SQLExecDirect returned SQL_NEED_DATA.
 *      [2] SQLExecute returned SQL_NEED_DATA.
 *      [3] SQLSetPos had been called from state S7 and returned SQL_NEED_DATA.
 *      [4] SQLBulkOperations had been called from state S5 and returned SQL_NEED_DATA.
 *      [5] SQLSetPos or SQLBulkOperations had been called from state S6
 *          and returned SQL_NEED_DATA.
 *      [6] One or more calls to SQLPutData for a single parameter returned SQL_SUCCESS,
 *          and then a call to SQLPutData was made for the same parameter with
 *          StrLen_or_Ind set to SQL_NULL_DATA.
 */
ACI_RC ulnSFID_46(ulnFnContext *aFnContext)
{
    /*
     * BUGBUG : [3] [4] [5] 는 아직 구현하지 않았으므로 일단 무시하도록 한다.
     *          [1] [2] 를 구현해야 한다.
     *          일단은 SQLExecute() 를 했다고 가정한다. 즉, 무조건 [2]
     */

    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
            ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO)
        {
            /* [s] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S10);
        }
        else if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_ERROR)
        {
            /* [e] */
            if (0)   // BUGBUG
            {
                /* [1] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
            }
            else
            {
                /* [2] */
                if (ulnStmtGetColumnCount(aFnContext->mHandle.mStmt) == 0)
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                }
                else
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
                }
            }
        }
        else
        {
            ACE_ASSERT(0);
        }
    }

    return ACI_SUCCESS;
}

/*
 * ULN_SFID_47
 * SQLPutData(), STMT, S10
 *
 *      -- [s]
 *      S1 [e] and [1]
 *      S2 [e], [nr], and [2]
 *      S3 [e], [r], and [2]
 *      S5 [e] and [4]
 *      S6 [e] and [5]
 *      S7 [e] and [3]
 *      S11 [x]
 *      HY011[6]
 *
 * where
 *      [1] SQLExecDirect returned SQL_NEED_DATA.
 *      [2] SQLExecute returned SQL_NEED_DATA.
 *      [3] SQLSetPos had been called from state S7 and returned SQL_NEED_DATA.
 *      [4] SQLBulkOperations had been called from state S5 and returned SQL_NEED_DATA.
 *      [5] SQLSetPos or SQLBulkOperations had been called from state S6
 *          and returned SQL_NEED_DATA.
 *      [6] One or more calls to SQLPutData for a single parameter returned SQL_SUCCESS,
 *          and then a call to SQLPutData was made for the same parameter with
 *          StrLen_or_Ind set to SQL_NULL_DATA.
 */
ACI_RC ulnSFID_47(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        /*
         * S10 상태로 올 수 있는 경우는
         *      1. S9 상태에서 SQLPutData() 함수가 SQL_SUCCESS 를 리턴한 경우,
         *      2. S10 상태에서 SQLPutData() 함수가 SQL_SUCCESS 를 리턴한 경우
         * 밖에는 없다.
         * 즉, [6] 번 조건의
         *      " One or more calls to SQLPutData for a single parameter returned SQL_SUCCESS,
         *        and then a call to SQLPutData was made for the same parameter"
         * 라는 문장이 이 부분이 실행된다는 사실 만으로도 충족이 된다.
         * 체크할 것은 단지, StrLenOrInd set to SQL_NULL_DATA 밖에 없다.
         */
        ACI_TEST_RAISE( *(ulvSLen *)(aFnContext->mArgs) == SQL_NULL_DATA, LABEL_ABORT_SET);
    }
    else
    {
        /*
         * BUGBUG : [3] [4] [5] 는 아직 구현하지 않았으므로 일단 무시하도록 한다.
         *          [1] [2] 를 구현해야 한다.
         *          일단은 SQLExecute() 를 했다고 가정한다. 즉, 무조건 [2]
         */
        if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS ||
           ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_SUCCESS_WITH_INFO)
        {
            /* [s] */
        }
        else if (ULN_FNCONTEXT_GET_RC(aFnContext) == SQL_ERROR)
        {
            /* [e] */
            if (0)   // BUGBUG
            {
                /* [1] */
                ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
            }
            else
            {
                /* [2] */
                if (ulnStmtGetColumnCount(aFnContext->mHandle.mStmt) == 0)
                {
                    /* [nr] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S2);
                }
                else
                {
                    /* [r] */
                    ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
                }
            }
        }
        else
        {
            ACE_ASSERT(0);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_ABORT_SET)
    {
        ulnError(aFnContext, ulERR_ABORT_PD_ATTRIBUTE_CANNOT_BE_SET_NOW);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnPutDataLob(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            ulnDescRec   *aDescRecApd,
                            ulnDescRec   *aDescRecIpd,
                            void         *aDataPtr,
                            ulvSLen       aStrLenOrInd)
{
    ULN_FLAG(sNeedFinPtContext);
    ULN_FLAG(sNeedFinPD);

    ulnDbc       *sDbc;
    ulnLob       *sLob;
    ulnLobBuffer  sLobBuffer;
    ulnCTypeID    sCTYPE;

    acp_uint32_t  sDataLength = 0;

    ulnPDContext *sPDContext = &aDescRecApd->mPDContext;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    /*
     * ulnLob 구조체 얻기
     */

    sLob = ulnDescRecGetLobElement(aDescRecIpd, aStmt->mProcessingLobRowNumber);
    ACI_TEST_RAISE(sLob == NULL, LABEL_MEM_MAN_ERR);

    /*
     * lob open 은 SQLExecute() 함수에서 수행한다.
     */

    ACI_TEST_RAISE(sLob->mOp->mGetState(sLob) != ULN_LOB_ST_OPENED, LABEL_MEM_MAN_ERR);

    switch (aStrLenOrInd)
    {
        case SQL_NTS:
            sDataLength = acpCStrLen((acp_char_t *)aDataPtr, ACP_UINT32_MAX);
            break;

        case SQL_NULL_DATA:
            sDataLength = 0;
            break;

        case SQL_DEFAULT_PARAM:
            /*
             * BUGBUG : 구현 아직 못함. 스펙 읽어 봐야 함.
             */
            ACE_ASSERT(0);
            break;

        default:
            /*
             * BUGBUG : 에러 코드
             */
            ACI_TEST(aStrLenOrInd < ULN_vLEN(0));

            /*
             * BUGBUG : 4기가 이상 되는 사이즈일 때에는 대략 난감 -_-;
             *          에러 발생시켜 줘야 한다.
             */
            sDataLength = (acp_uint32_t)aStrLenOrInd;
            break;
    }

    switch (ulnPDContextGetState(sPDContext))
    {
        /*
         * BUG-28980 [CodeSonar]Ignored Return Value
         * false alarm 처리
         */
        case ULN_PD_ST_CREATED:
        case ULN_PD_ST_INITIAL:
            ACI_RAISE(LABEL_INVALID_STATE);
            break;

        case ULN_PD_ST_NEED_DATA:
            /*
             * 최초의 LOB SQLPutData() : Descriptor state shift
             */
            // idlOS::printf("#### >>>>>> initialize pd context LOB\n");
            ulnPDContextInitialize(sPDContext, ULN_PD_BUFFER_TYPE_USER);

            ULN_FLAG_UP(sNeedFinPD);

            ulnPDContextSetState(sPDContext, ULN_PD_ST_ACCUMULATING_DATA);

        case ULN_PD_ST_ACCUMULATING_DATA:
            /*
             * 연속되는 LOB SQLPutData()
             *
             * BUFFER 준비
             */
            sCTYPE = ulnMetaGetCTYPE(&aDescRecApd->mMeta);

            ACI_TEST_RAISE(ulnLobBufferInitialize(&sLobBuffer,
                                                  sDbc,
                                                  sLob->mType,
                                                  sCTYPE,
                                                  (acp_uint8_t *)aDataPtr,
                                                  sDataLength) != ACI_SUCCESS,
                           LABEL_INVALID_C_TYPE);

            ACI_TEST(sLobBuffer.mOp->mPrepare(aFnContext, &sLobBuffer) != ACI_SUCCESS);

            /*
             * 서버로 전송 (lob open 은 Execute 시에 이루어진다)
             */
            //fix BUG-17722
            ACI_TEST(ulnInitializeProtocolContext(aFnContext,
                                                  &(aStmt->mParentDbc->mPtContext),
                                                  &(aStmt->mParentDbc->mSession)) != ACI_SUCCESS);
            ULN_FLAG_UP(sNeedFinPtContext);

            ACI_TEST(sLob->mOp->mAppend(aFnContext,
                                        &(aStmt->mParentDbc->mPtContext),
                                        sLob,
                                        &sLobBuffer) != ACI_SUCCESS);

            ULN_FLAG_DOWN(sNeedFinPtContext);
            //fix BUG-17722
            ACI_TEST(ulnFinalizeProtocolContext(aFnContext,
                               &(aStmt->mParentDbc->mPtContext)) != ACI_SUCCESS);

            /*
             * BUFFER 정리
             */

            ACI_TEST(sLobBuffer.mOp->mFinalize(aFnContext, &sLobBuffer) != ACI_SUCCESS);

            break;
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnPutDataLob");
    }

    ACI_EXCEPTION(LABEL_INVALID_C_TYPE);
    {
        ulnErrorExtended(aFnContext,
                         aStmt->mProcessingLobRowNumber + 1,
                         aStmt->mProcessingParamNumber,
                         ulERR_ABORT_INVALID_APP_BUFFER_TYPE_LOB,
                         ulnTypeMap_CTYPE_SQLC(ulnMetaGetCTYPE(&aDescRecApd->mMeta)));
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aStmt->mProcessingLobRowNumber + 1,
                         aStmt->mProcessingParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnPutDataLob");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext(aFnContext, &(aStmt->mParentDbc->mPtContext));
    }
    ULN_IS_FLAG_UP(sNeedFinPD)
    {
        sPDContext->mOp->mFinalize(sPDContext);
    }

    return ACI_FAILURE;
}

static ACI_RC ulnPutDataVariable(ulnFnContext *aFnContext,
                                 ulnDescRec   *aDescRecApd,
                                 void         *aDataPtr,
                                 ulvSLen       aStrLenOrInd)
{
    ulnPDContext *sPDContext = &aDescRecApd->mPDContext;
    acp_uint32_t  sDataLength = 0;

    /*
     * CHAR 나 BINARY 데이터를 여러번에 걸쳐서 SQLPutData() 하는 경우임.
     */
    switch (ulnPDContextGetState(sPDContext))
    {
        case ULN_PD_ST_CREATED:
        case ULN_PD_ST_INITIAL:
            ACI_RAISE(LABEL_INVALID_STATE);
            break;

        case ULN_PD_ST_NEED_DATA:
            /*
             * 최초의 SQLPutData() 호출
             *
             * ulnParamProcess_DATA_AT_EXEC() 에서 할당한 메모리 free,
             * 만약 에러 등으로 free 안되었을 때에는 ulnDescDestroy() 에서 Descriptor 의
             * PDContextList 를 따라가면서 free.
             */
            // idlOS::printf("#### >>>>>> initialize pd context variable\n");
            ulnPDContextInitialize(sPDContext, ULN_PD_BUFFER_TYPE_ALLOC);

            ACI_TEST_RAISE(sPDContext->mOp->mPrepare(sPDContext, NULL) != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEM);

            ulnPDContextSetState(sPDContext, ULN_PD_ST_ACCUMULATING_DATA);

            ulnDescAddPDContext(aDescRecApd->mParentDesc, sPDContext);

        case ULN_PD_ST_ACCUMULATING_DATA:
            /*
             * 연속되는 SQLPutData() 호출
             */
            if (aStrLenOrInd == SQL_NTS)
            {
                sDataLength = acpCStrLen((acp_char_t *)aDataPtr, ACP_UINT32_MAX);
            }
            else if (aStrLenOrInd == SQL_NULL_DATA)
            {
                sDataLength = 0;
            }
            else if (aStrLenOrInd == SQL_DEFAULT_PARAM)
            {
                /*
                 * BUGBUG : 구현 아직 못함. 스펙 읽어 봐야 함.
                 */
                ACE_ASSERT(0);
            }
            else
            {
                sDataLength = aStrLenOrInd;
            }

            ACI_TEST_RAISE(sPDContext->mOp->mAccumulate(sPDContext,
                                                        (acp_uint8_t *)aDataPtr,
                                                        sDataLength) != ACI_SUCCESS,
                           LABEL_DATA_TOO_BIG);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnPutDataVariable");
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        /*
         * HY001
         */
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnPutDataVariable");
    }

    ACI_EXCEPTION(LABEL_DATA_TOO_BIG)
    {
        sPDContext->mOp->mFinalize(sPDContext);
        ulnDescRemovePDContext(aDescRecApd->mParentDesc, sPDContext);

        /*
         * HY000 : SQLSTATE 정의 필요
         * BUGBUG : 메모리 realloc 하는 과정이 필요하다. 에러를 내면 안된다.
         */
        ulnError(aFnContext, ulERR_ABORT_PD_DATA_TOO_BIG);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnPutDataFixed(ulnFnContext *aFnContext,
                              ulnDescRec   *aDescRecApd,
                              void         *aDataPtr,
                              ulvSLen       aStrLenOrInd)
{
    ulnPDContext *sPDContext = &aDescRecApd->mPDContext;

    ACP_UNUSED(aStrLenOrInd);

    switch (ulnPDContextGetState(sPDContext))
    {
        case ULN_PD_ST_CREATED:
        case ULN_PD_ST_INITIAL:
            ACI_RAISE(LABEL_INVALID_STATE);
            break;

        case ULN_PD_ST_NEED_DATA:
            // idlOS::printf("#### >>>>>> initialize pd context fixed\n");
            ulnPDContextInitialize(sPDContext, ULN_PD_BUFFER_TYPE_USER);

            sPDContext->mOp->mPrepare(sPDContext, aDataPtr);
            /* BUG-41741 Fixed 데이타에 대한 PutData는 한번만 할 수 있으므로
               Prepare와 Accumulate를 동시에 수행해야한다. */
            sPDContext->mOp->mAccumulate(sPDContext, aDataPtr, ulnMetaGetOctetLength(&aDescRecApd->mMeta));
            ulnPDContextSetState(sPDContext, ULN_PD_ST_ACCUMULATING_DATA);

            break;

        case ULN_PD_ST_ACCUMULATING_DATA:
            /*
             * CHAR 나 BINARY 데이터가 아닌 경우 여러번에 걸쳐서 SQLPutData() 하면 안됨.
             */
            ACI_RAISE(LABEL_NOT_ALLOWED_IN_PIECES);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ALLOWED_IN_PIECES)
    {
        /*
         * HY019
         */
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         aDescRecApd->mIndex,
                         ulERR_ABORT_PD_FIXED_DATA_NOT_ALLOWD_IN_PIECES);
    }

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnPutDataFixed");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnPutDataCore(ulnFnContext *aFnContext,
                         ulnStmt      *aStmt,
                         void         *aDataPtr,
                         ulvSLen       aStrLenOrInd)
{
    ulnDescRec   *sDescRecApd;
    ulnDescRec   *sDescRecIpd;

    sDescRecApd = ulnStmtGetApdRec(aStmt, aStmt->mProcessingParamNumber);
    ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NEED_BINDPARAMETER);

    sDescRecIpd = ulnStmtGetIpdRec(aStmt, aStmt->mProcessingParamNumber);
    ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MAN_ERR);

    ACI_TEST(&sDescRecIpd->mMeta == NULL);           //BUG-28623 [CodeSonar]Null Pointer Dereference

    if (ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecIpd->mMeta),
                             ulnMetaGetCTYPE(&sDescRecApd->mMeta)) == ACP_TRUE)
    {
        ACI_TEST(ulnPutDataLob(aFnContext,
                               aStmt,
                               sDescRecApd,
                               sDescRecIpd,
                               aDataPtr,
                               aStrLenOrInd) != ACI_SUCCESS);
    }
    else
    {
        if (ulnBindIsFixedLengthColumn(sDescRecApd, sDescRecIpd) == ACP_TRUE)
        {
            ACI_TEST(ulnPutDataFixed(aFnContext,
                                     sDescRecApd,
                                     aDataPtr,
                                     aStrLenOrInd) != ACI_SUCCESS);
        }
        else
        {
            ACI_TEST(ulnPutDataVariable(aFnContext,
                                        sDescRecApd,
                                        aDataPtr,
                                        aStrLenOrInd) != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NEED_BINDPARAMETER)
    {
        ulnErrorExtended(aFnContext,
                         aStmt->mProcessingRowNumber + 1,
                         aStmt->mProcessingParamNumber,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }
    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aStmt->mProcessingRowNumber + 1,
                         aStmt->mProcessingParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnPutDataCore");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnPutData(ulnStmt *aStmt, void *aDataPtr, ulvSLen aStrLenOrInd)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PUTDATA, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&aStrLenOrInd)) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    /*
     * BUGBUG : 인자의 유효성 체크
     */

    /*
     * PutData 메인
     */

    /* PROJ-1789 Updatable Scrollable Cursor
     * SetPos, BulkOperations에서 NEED_DATA가 났으면 RowsetStmt로 작업. */
    if ((ulnStmtGetNeedDataFuncID(aStmt) == ULN_FID_SETPOS)
     || (ulnStmtGetNeedDataFuncID(aStmt) == ULN_FID_BULKOPERATIONS))
    {
        sFnContext.mHandle.mStmt = aStmt->mRowsetStmt;
        ACI_TEST_RAISE(ulnPutDataCore(&sFnContext, aStmt->mRowsetStmt,
                                      aDataPtr, aStrLenOrInd)
                       != ACI_SUCCESS, ROWSET_PUTDATA_FAILED_EXCEPTION);
        sFnContext.mHandle.mStmt = aStmt;

        if (ULN_FNCONTEXT_GET_RC(&sFnContext) != SQL_SUCCESS)
        {
            ulnDiagRecMoveAll(&aStmt->mObj, &aStmt->mRowsetStmt->mObj);
        }
    }
    else
    {
        ACI_TEST(ulnPutDataCore(&sFnContext, aStmt, aDataPtr, aStrLenOrInd)
                 != ACI_SUCCESS);
    }

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(ROWSET_PUTDATA_FAILED_EXCEPTION)
    {
        ulnDiagRecMoveAll(&aStmt->mObj, &aStmt->mRowsetStmt->mObj);
    }
    ACI_EXCEPTION_END;

    if (sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
