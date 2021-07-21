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
#include <ulnError.h>
#include <ulnErrorDef.h>
#include <ulsdnFailover.h>
#include <aciErrorMgr.h>
#include <ulsdnTrans.h>
#include <idErrorCodeClient.h>
#include <smErrorCodeClient.h>
#include <mtErrorCodeClient.h>
#include <mmErrorCodeClient.h>
#include <cmErrorCodeClient.h>
#include <qcuErrorClient.h>
#include <ulsdDistTxInfo.h>

/*
 * Note: ODBC3 / ODBC2 매핑은 매번 일어나는 것이 아니라
 * 사용자가 GetDiagRec() 를 호출할때만 해도 되겠네
 */

aci_client_error_factory_t gUlnErrorFactory[] =
{
#include "E_UL_US7ASCII.c"
};

static aci_client_error_factory_t *gClientFactory[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gUlnErrorFactory,
    NULL,
    NULL,
    NULL,
    NULL // gUtErrorFactory,
};

/*
 * 함수이름 정말 마음에 안든다.
 * 그렇다고 딱히 뾰족한 이름이 떠오르는 것도 아니고... _-;
 */
acp_char_t *ulnErrorMgrGetSQLSTATE_Server(acp_uint32_t aServerErrorCode)
{
    acp_uint32_t i;

    for (i = 0; gUlnServerErrorSQLSTATEMap[i].mErrorCode != 0; i++)
    {
        if (ACI_E_ERROR_CODE(gUlnServerErrorSQLSTATEMap[i].mErrorCode) ==
            ACI_E_ERROR_CODE(aServerErrorCode))
        {
            return (acp_char_t *)(gUlnServerErrorSQLSTATEMap[i].mSQLSTATE);
        }
    }

    return (acp_char_t *)"HY000";
}

static void ulnErrorMgrSetSQLSTATE(ulnErrorMgr  *aManager, acp_char_t *aSQLSTATE)
{
    acpSnprintf(aManager->mErrorState, SQL_SQLSTATE_SIZE + 1, "%s", aSQLSTATE);
}

static void ulnErrorMgrSetErrorCode(ulnErrorMgr  *aManager, acp_uint32_t aErrorCode)
{
    aManager->mErrorCode = aErrorCode;
}

/*
 * 함수 이름 정말 마음에 안든다 -_-;;
 */
static ACI_RC ulnErrorMgrSetErrorMessage(ulnErrorMgr  *aManager, acp_uint8_t* aVariable, acp_uint32_t aLen)
{
    acp_uint32_t sErrorMsgSize;

    /*
     * NULL terminate
     */
    if (aLen >= ACI_SIZEOF(aManager->mErrorMessage))
    {
        sErrorMsgSize = ACI_SIZEOF(aManager->mErrorMessage) - 1;
    }
    else
    {
        sErrorMsgSize = aLen;
    }

    acpMemCpy(aManager->mErrorMessage, aVariable, sErrorMsgSize);

    aManager->mErrorMessage[sErrorMsgSize] = '\0';

    return ACI_SUCCESS;
}

/*
 * 주의 : 아래 함수 ulnErrorMgrSetUlErrorVA() (함수이름을 뭘로 하나 -_-);;
 *        에서는 에러코드만 가지고
 *        SQLSTATE, ErrorMessage 까지 세팅해 버린다.
 */
void ulnErrorMgrSetUlErrorVA( ulnErrorMgr  *aErrorMgr,
                              acp_uint32_t  aErrorCode,
                              va_list       aArgs )
{
    acp_uint32_t                sSection;
    aci_client_error_factory_t *sCurFactory;

    sSection = (aErrorCode & ACI_E_MODULE_MASK) >> 28;

    sCurFactory = gClientFactory[sSection];

    aciSetClientErrorCode(aErrorMgr,
                          sCurFactory,
                          aErrorCode,
                          aArgs);
}

void ulnErrorMgrSetUlError( ulnErrorMgr *aErrorMgr, acp_uint32_t aErrorCode, ...)
{
    va_list sArgs;

    va_start(sArgs, aErrorCode);
    ulnErrorMgrSetUlErrorVA(aErrorMgr, aErrorCode, sArgs);
    va_end(sArgs);
}

SQLRETURN ulnErrorDecideSqlReturnCode(acp_char_t *aSqlState)
{
    if (aSqlState[0] == '0')
    {
        /* PROJ-1789 Updatable Scrollable Cursor
         * SQL_NO_DATA는 Fetch 결과를 보고 따로 설정해준다.
         * 여기서는 다른 DB들 처럼 '02*'도 SUCCESS_WITH_INFO로 설정한다. */
        switch (aSqlState[1])
        {
            case '0':
                return SQL_SUCCESS;
                break;
            case '1':
            case '2':
                return SQL_SUCCESS_WITH_INFO;
                break;
            default :
                break;
        }
    }

    return SQL_ERROR;
}

static void ulnErrorBuildDiagRec(ulnDiagRec   *aDiagRec,
                                 ulnErrorMgr  *aErrorMgr,
                                 acp_sint32_t  aRowNumber,
                                 acp_sint32_t  aColumnNumber)
{
    /*
     * 메세지 텍스트 복사
     * BUGBUG : 메모리 부족상황 처리
     */
    ulnDiagRecSetMessageText(aDiagRec, ulnErrorMgrGetErrorMessage(aErrorMgr));

    /*
     * SQLSTATE 복사
     */
    ulnDiagRecSetSqlState(aDiagRec, ulnErrorMgrGetSQLSTATE(aErrorMgr));

    /*
     * NativeErrorCode 세팅
     */
    ulnDiagRecSetNativeErrorCode(aDiagRec, ulnErrorMgrGetErrorCode(aErrorMgr));

    /*
     * RowNumber, ColumnNumber 세팅
     */
    ulnDiagRecSetRowNumber(aDiagRec, aRowNumber);
    ulnDiagRecSetColumnNumber(aDiagRec, aColumnNumber);
}

static ACI_RC ulnErrorAddDiagRec(ulnFnContext *aFnContext,
                                 ulnErrorMgr  *aErrorMgr,
                                 acp_sint32_t  aRowNumber,
                                 acp_sint32_t  aColumnNumber)
{
    ulnDiagRec    *sDiagRec    = NULL;
    ulnDiagHeader *sDiagHeader;

    if (aFnContext->mHandle.mObj != NULL)
    {
        if (ulnErrorMgrGetErrorCode(aErrorMgr) == ulERR_ABORT_INVALID_HANDLE)
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
        }
        else
        {
            sDiagHeader = &aFnContext->mHandle.mObj->mDiagHeader;

            /*
             * Diagnostic Record 생성 (메모리 부족하면 static record 사용함)
             */
            ulnDiagRecCreate(sDiagHeader, &sDiagRec);
            /* BUG-36729 Connection attribute will be added to unlock client mutex by force */
            ACI_TEST(sDiagRec == NULL);

            ulnErrorBuildDiagRec(sDiagRec,
                                 aErrorMgr,
                                 aRowNumber,
                                 aColumnNumber);

            /*
             * Diagnostic Record 매달기
             */
            ulnDiagHeaderAddDiagRec(sDiagHeader, sDiagRec);

            /*
             * DiagHeader 에 SQLRETURN 세팅.
             * BUGBUG : 아래의 함수(ulnDiagSetReturnCode) 이상하다 -_-;; 다시 한번 보자.
             */
            // ulnDiagSetReturnCode(sDiagHeader, ulnErrorDecideSqlReturnCode(sDiagRec->mSQLSTATE));
            ULN_FNCONTEXT_SET_RC(aFnContext, ulnErrorDecideSqlReturnCode(sDiagRec->mSQLSTATE));

            /* BUG-46092 */
            if ( ulnErrorMgrGetErrorCode( aErrorMgr )
                 == ACI_E_ERROR_CODE( ulERR_ABORT_FAILOVER_SUCCESS ) )
            {
                aFnContext->mIsFailoverSuccess = ACP_TRUE;
            }

#if COMPILE_SHARDCLI
            /* TASK-7218 Multi-Error Handling 2nd */
            ulsdMultiErrorAdd(aFnContext, sDiagRec);
#endif
        }

        ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                "%-18s| row: %"ACI_INT32_FMT" col: %"ACI_INT32_FMT
                " code: 0x%08x\n msg [%s]",
                "ulnErrorAddDiagRec", aRowNumber, aColumnNumber,
                ulnErrorMgrGetErrorCode(aErrorMgr),
                ulnErrorMgrGetErrorMessage(aErrorMgr));
    }

    ACI_TEST(!(SQL_SUCCEEDED(ULN_FNCONTEXT_GET_RC(aFnContext))));

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnErrorHandleFetchError(ulnFnContext *aFnContext,
                                       ulnErrorMgr  *aErrorMgr,
                                       acp_sint32_t  aColumnNumber)
{
    ulnDiagHeader *sDiagHeader;
    ulnDiagRec    *sDiagRec = NULL;

    ulnCache      *sCache;

    ACI_TEST(aFnContext->mHandle.mObj == NULL);

    sCache = aFnContext->mHandle.mStmt->mCache;
    ACI_TEST(sCache == NULL);

    sDiagHeader = &aFnContext->mHandle.mObj->mDiagHeader;

    /*
     * DiagHeader 에 not enough memory 에러용의 static diag rec 을 이용한다.
     */
    sDiagRec = &sDiagHeader->mStaticDiagRec;

    ulnErrorBuildDiagRec(sDiagRec,
                         aErrorMgr,
                         ulnCacheGetRowCount(sCache) + 1,
                         aColumnNumber);

    sCache->mServerError = sDiagRec;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnError(ulnFnContext *aFnContext, acp_uint32_t aErrorCode, ...)
{
    va_list     sArgs;
    ulnErrorMgr sErrorMgr;

    /*
     * BUGBUG : ulnErrorMgr 에서는 에러 메세지를 위한 공간을
     *          MAX_ERROR_MSG_LEN + 256 바이트만큼
     *          static 으로 가지고 있다. 에러 메세지가 얼마나 커질지 모르는 상태에서 이처럼
     *          고정 사이즈를 가지는 것은 자칫 정보가 잘릴 위험이 있다.
     *          뿐만 아니라, 대부분의 에러 메세지는 짤막짤막한데, 무려 2K 가 넘는 공간을
     *          바로 할당한다는 것은 실로 메모리 낭비가 아닐 수 없다. 비록 스택영역일지라도.
     *
     *          이 부분을 적당히 malloc() 을 하도록 수정해야 한다.
     *          지금은 바쁘고 머리 아프니 그냥 가자 -_-;;
     */
    va_start(sArgs, aErrorCode);
    ulnErrorMgrSetUlErrorVA(&sErrorMgr, aErrorCode, sArgs);
    va_end(sArgs);

    if (aErrorCode != ulERR_IGNORE_NO_ERROR)
    {
        if (aErrorCode == ulERR_ABORT_INVALID_HANDLE)
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
            ACI_RAISE(LABEL_INVALID_HANDLE);
        }
        else
        {
            ACI_TEST(ulnErrorAddDiagRec(aFnContext,
                                        &sErrorMgr,
                                        SQL_NO_ROW_NUMBER,
                                        SQL_NO_COLUMN_NUMBER) != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * BUGBUG : 뒤에다가 extended 를 주니까 꼭 M$ 함수같다 -_-;;;
 *          좀 궁리해 보고 자연스럽게 되는 방법을 생각해 보자. -_-;;
 */
ACI_RC ulnErrorExtended(ulnFnContext *aFnContext,
                        acp_sint32_t  aRowNumber,
                        acp_sint32_t  aColumnNumber,
                        acp_uint32_t  aErrorCode, ...)
{
    va_list     sArgs;
    ulnErrorMgr sErrorMgr;

    /*
     * BUGBUG : ulnErrorMgr 에서는 에러 메세지를 위한 공간을
     *          MAX_ERROR_MSG_LEN + 256 바이트만큼
     *          static 으로 가지고 있다. 에러 메세지가 얼마나 커질지 모르는 상태에서 이처럼
     *          고정 사이즈를 가지는 것은 자칫 정보가 잘릴 위험이 있다.
     *          뿐만 아니라, 대부분의 에러 메세지는 짤막짤막한데, 무려 2K 가 넘는 공간을
     *          바로 할당한다는 것은 실로 메모리 낭비가 아닐 수 없다. 비록 스택영역일지라도.
     *
     *          이 부분을 적당히 malloc() 을 하도록 수정해야 한다.
     *          지금은 바쁘고 머리 아프니 그냥 가자 -_-;;
     */
    va_start(sArgs, aErrorCode);
    ulnErrorMgrSetUlErrorVA(&sErrorMgr, aErrorCode, sArgs);
    va_end(sArgs);

    if (aErrorCode != ulERR_IGNORE_NO_ERROR)
    {
        if (aErrorCode == ulERR_ABORT_INVALID_HANDLE)
        {
            ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
            ACI_RAISE(LABEL_INVALID_HANDLE);
        }
        else
        {
            ACI_TEST(ulnErrorAddDiagRec(aFnContext,
                                        &sErrorMgr,
                                        aRowNumber,
                                        aColumnNumber) != ACI_SUCCESS);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnCallbackErrorResult(cmiProtocolContext *aPtContext,
                              cmiProtocol        *aProtocol,
                              void               *aServiceSession,
                              void               *aUserContext)
{
    ulnFnContext        *sFnContext = (ulnFnContext *)aUserContext;
    ulnStmt             *sStmt;
    ulnErrorMgr          sErrorMgr;

    acp_uint8_t          sOperationID;
    acp_uint32_t         sErrorIndex;
    acp_uint32_t         sErrorCode;
    acp_uint16_t         sErrorMessageLen;
    acp_uint8_t         *sErrorMessage;
    acp_uint64_t         sSCN = 0;
    acp_uint32_t         sNodeId = 0;

    /* BUG-36256 Improve property's communication */
    ulnDbc              *sDbc = NULL;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD1(aPtContext, sOperationID);
    CMI_RD4(aPtContext, &sErrorIndex);
    CMI_RD4(aPtContext, &sErrorCode);
    CMI_RD2(aPtContext, &sErrorMessageLen);

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MANAGE_ERR);

    if (sErrorMessageLen > 0)
    {
        sErrorMessage = aPtContext->mReadBlock->mData + aPtContext->mReadBlock->mCursor;
        aPtContext->mReadBlock->mCursor += sErrorMessageLen;
    }
    else
    {
        sErrorMessage = NULL;
    }

    /* PROJ-2733-Protocol 서버가 Handshake 프로토콜 읽기 전에 Error가 올 수 있나???
                          Error가 올 수 있다면 안전을 위해 처리를 할 수 있어야 한다. */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_ErrorV3Result:
            CMI_RD8(aPtContext, &sSCN);
            CMI_RD4(aPtContext, &sNodeId); // TASK-7218
            break;

        case CMP_OP_DB_ErrorResult:
        default:
            break;
    }

    /* PROJ-2733-DistTxInfo */
    if (sSCN > 0)
    {
        ulsdUpdateSCN(sDbc, &sSCN);
    }

    if (CMI_IS_EXECUTE_GROUP(sOperationID) == ACP_TRUE)
    {
        /* bug-18246 */
        sStmt = sFnContext->mHandle.mStmt;

        if (ulnStmtGetStatementType(sStmt) == ULN_STMT_UPDATE ||
            ulnStmtGetStatementType(sStmt) == ULN_STMT_DELETE)
        {
            ulnStmtSetAttrParamStatusValue(sStmt, sErrorIndex - 1, SQL_PARAM_ERROR);
            ULN_FNCONTEXT_SET_RC((sFnContext), SQL_ERROR);
        }
    }
    /* BUG-36256 Improve property's communication */
    else if ( (CMI_IS_PROPERTY_SET_GROUP(sOperationID) == ACP_TRUE) ||
              (CMI_IS_PROPERTY_GET_GROUP(sOperationID) == ACP_TRUE) )
    {
        if (ACI_E_ERROR_CODE(sErrorCode) ==
            ACI_E_ERROR_CODE(mmERR_IGNORE_UNSUPPORTED_PROPERTY))
        {
            /* 서버에서 지원하지 않는 프로퍼티는 Off 한다. */
            ACI_TEST(ulnSetConnectAttrOff(sFnContext,
                                          sDbc,
                                          sErrorIndex)
                     != ACI_SUCCESS);
        }
        else
        {
            /* Nothing */
        }
    }
    else
    {
        /* Nothing */
    }

    if ((ACI_E_ACTION_MASK & sErrorCode) != ACI_E_ACTION_IGNORE)
    {
        /*
         * ulnErrorMgr 의 에러코드 세팅
         */
        ulnErrorMgrSetErrorCode(&sErrorMgr, sErrorCode);

        /*
         * ulnErrorMgr 의 SQLSTATE 세팅
         */
        ulnErrorMgrSetSQLSTATE(&sErrorMgr,
                               ulnErrorMgrGetSQLSTATE_Server(sErrorCode));

        /*
         * ulnErrorMgr 의 에러 메세지 세팅
         */
        ACI_TEST_RAISE(ulnErrorMgrSetErrorMessage(&sErrorMgr, sErrorMessage, sErrorMessageLen)
                       != ACI_SUCCESS,
                       LABEL_MEM_MANAGE_ERR);

        /*
         * 만약 FETCH REQ 에 대한 에러라면, 일단 펜딩을 시키고, 그렇지 않으면 일반적인 진행.
         * 이렇게 하는 이유는 대략 다음과 같다.
         *
         * 1. ExecDirect는 Prepare, Execute, Fetch를 한번에 보내고 결과도 한번에 받는다.
         *    때문에 펜딩안하면, SELECT를 수행한게 아닐 경우에 Fetch 결과로 에러가 떨어진다.
         * 2. Array Fetch는 에러 처리 방법이 조금 다르다.
         *    에러가 있다고 바로 반환하면 안된다.
         *
         * 단, Fetch out of sequence는 무효화된 Cursor로 Fetch를 해서 난 에러이므로
         * 그냥 에러로 처리한다. (for PROJ-1381 FAC)
         */
        if ( (CMI_IS_FETCH_GROUP(sOperationID) == ACP_TRUE) &&
             (sErrorCode != mmERR_ABORT_FETCH_OUT_OF_SEQ) )
        {
            ACI_TEST_RAISE(ulnErrorHandleFetchError(sFnContext,
                                                    &sErrorMgr,
                                                    SQL_COLUMN_NUMBER_UNKNOWN)
                           != ACI_SUCCESS,
                           LABEL_MEM_MAN_ERR_DIAG);
        }
        else
        {
            /*
             * DiagRec 를 만들어서 object 에 붙이기
             * BUGBUG : 함수 이름, 개념, scope 등이 이상하다.
             */
            ulnErrorAddDiagRec(sFnContext,
                               &sErrorMgr,
                               (sErrorIndex == 0) ? SQL_NO_ROW_NUMBER
                                                             : (acp_sint32_t)sErrorIndex,
                               SQL_COLUMN_NUMBER_UNKNOWN);

            if (ULN_OBJ_GET_TYPE(sFnContext->mHandle.mObj) == ULN_OBJ_TYPE_STMT)
            {
                if (sFnContext->mFuncID == ULN_FID_EXECUTE ||
                    sFnContext->mFuncID == ULN_FID_EXECDIRECT ||
                    sFnContext->mFuncID == ULN_FID_PARAMDATA)
                {
                    // BUG-21255
                    if (sErrorIndex > 0)
                    {
                        ulnExecProcessErrorResult(sFnContext, sErrorIndex);
                    }
                }
            }

#ifdef COMPILE_SHARDCLI /* BUG-46092 */
            if ( ( sErrorCode & ACI_E_MODULE_MASK ) == ACI_E_MODULE_SD )
            {
                ulsdErrorHandleShardingError( sFnContext, sNodeId );
            }
#endif /* COMPILE_SHARDCLI */
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR_DIAG)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnCallbackErrorResult : while registering fetch error");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "CallbackErrorResult");
    }

    ACI_EXCEPTION_END;

    /*
     * BUG 아님. callback 이므로 ACI_SUCCESS 리턴해야 함.
     */
    return ACI_SUCCESS;
}

void ulnErrorMgrSetCmError( ulnDbc       *aDbc,
                            ulnErrorMgr  *aErrorMgr,
                            acp_uint32_t  aCmErrorCode )
{
    switch (aCmErrorCode)
    {
        case cmERR_ABORT_CONNECT_ERROR:
        case cmERR_ABORT_IB_RCONNECT_ERROR: /* PROJ-2681 */
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_FATAL_FAIL_TO_ESTABLISH_CONNECTION,
                                   aciGetErrorMsg(aCmErrorCode) );
            break;

        /* TASK-5894 Permit sysdba via IPC */
        case cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL:
            /*
             * SYSDBA가 접속 실패하면 이미 SYSDBA가 접속중일 수도 있고,
             * Non-SYSDBA의 동시접속들에 의해 접속을 못한 경우일 수도 있다.
             * 채널을 할당받고 Connect 프로토콜을 전송받아야 SYSDBA인지 아닌지
             * 알 수 있기 때문에 정확하게 상황을 알 수 없다.
             * 일반적인 경우라 가정하고 ADMIN_ALREADY_RUNNING 에러를 준다.
             */
            if (ulnDbcGetPrivilege(aDbc) == ULN_PRIVILEGE_SYSDBA)
            {
                ulnErrorMgrSetUlError( aErrorMgr,
                                       ulERR_ABORT_ADMIN_ALREADY_RUNNING );
            }
            else
            {
                ulnErrorMgrSetUlError( aErrorMgr,
                                       ulERR_FATAL_FAIL_TO_ESTABLISH_CONNECTION,
                                       aciGetErrorMsg(aCmErrorCode) );
            }
            break;

        case cmERR_ABORT_TIMED_OUT:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_CONNECTION_TIME_OUT );
            break;

        case cmERR_ABORT_CONNECTION_CLOSED:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_COMMUNICATION_LINK_FAILURE,
                                   aciGetErrorMsg(aCmErrorCode) );
            break;

        case cmERR_ABORT_CALLBACK_DOES_NOT_EXIST:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_CM_CALLBACK_NOT_EXIST );
            break;

        /* bug-30835: support link-local address with zone index. */
        case cmERR_ABORT_GETADDRINFO_ERROR:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_GETADDRINFO_ERROR );
            break;

        case cmERR_ABORT_CONNECT_INVALIDARG:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_CONNECT_INVALIDARG );
            break;

        /* PROJ-2681 */
        case cmERR_ABORT_IB_RCONNECT_INVALIDARG:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_IB_RCONNECT_INVALIDARG );
            break;

        /* BUG-41330 Returning a detailed SSL error to the client */
        /* SQLState: 08S01 - Communications link failure */
        case cmERR_ABORT_SSL_READ:
        case cmERR_ABORT_SSL_WRITE:
        case cmERR_ABORT_SSL_CONNECT:
        case cmERR_ABORT_SSL_HANDSHAKE:
        case cmERR_ABORT_SSL_SHUTDOWN:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_SSL_LINK_FAILURE,
                                   aciGetErrorMsg(aCmErrorCode) );
            break;

        /* SQLState: HY000 - General error */
        case cmERR_ABORT_INVALID_CERTIFICATE:
        case cmERR_ABORT_INVALID_PRIVATE_KEY:
        case cmERR_ABORT_PRIVATE_KEY_VERIFICATION:
        case cmERR_ABORT_INVALID_VERIFY_LOCATION:
        case cmERR_ABORT_SSL_OPERATION:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_SSL_OPERATION_FAILURE,
                                   aciGetErrorMsg(aCmErrorCode) );
            break;

        case cmERR_ABORT_DLOPEN:
        case cmERR_ABORT_DLSYM:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_SSL_LIBRARY_ERROR,
                                   aciGetErrorMsg(aCmErrorCode) );
            break;

#ifdef COMPILE_SHARDCLI
        /* PROJ-2658 altibase sharding */
        case cmERR_ABORT_SHARD_VERSION_MISMATCH:
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_SHARD_VERSION_MISMATCH );
            break;
#endif /* COMPILE_SHARDCLI */

        default:
            /* BUGBUG : 무조건 08S01 로 집어넣을 것이 아니라 좀 더 정교하게 해야 한다. */
            ulnErrorMgrSetUlError( aErrorMgr,
                                   ulERR_ABORT_CM_GENERAL_ERROR,
                                   aciGetErrorMsg(aCmErrorCode) );
            break;
    }
}

ACI_RC ulnErrHandleCmError(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnErrorMgr   sErrorMgr;
    ulnDbc       *sDbc = NULL;
    acp_uint32_t  sCmErrorCode;

    sCmErrorCode = aciGetErrorCode();

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    ACI_TEST_RAISE(sDbc == NULL, LABEL_MEM_MAN_ERR);

    ulnErrorMgrSetCmError( sDbc, &sErrorMgr, sCmErrorCode );

    switch (sCmErrorCode)
    {
        case cmERR_ABORT_CONNECTION_CLOSED:
            if (aPtContext != NULL)
            {
                if (cmiGetLinkImpl(&(aPtContext->mCmiPtContext)) == CMI_LINK_IMPL_IPCDA)
                {
                    (void) cmiRecvIPCDA(&aPtContext->mCmiPtContext,
                                        aFnContext,
                                        ACP_TIME_INFINITE,
                                        sDbc->mIPCDAMicroSleepTime);
                }
                else
                {
                    (void) cmiRecv(&aPtContext->mCmiPtContext, aFnContext, ACP_TIME_INFINITE);
                }
            }
            else
            {
                /* do nothing */
            }

        case cmERR_ABORT_CONNECT_ERROR:
        case cmERR_ABORT_IB_RCONNECT_ERROR:          /* PROJ-2681*/
        case cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL:
        case cmERR_ABORT_TIMED_OUT:
        case cmERR_ABORT_GETADDRINFO_ERROR:
        case cmERR_ABORT_CONNECT_INVALIDARG:
        case cmERR_ABORT_IB_RCONNECT_INVALIDARG:     /* PROJ-2681 */
        case cmERR_ABORT_SSL_READ:
        case cmERR_ABORT_SSL_WRITE:
        case cmERR_ABORT_SSL_CONNECT:
        case cmERR_ABORT_SSL_HANDSHAKE:
        case cmERR_ABORT_SSL_SHUTDOWN:
        case cmERR_ABORT_DLSYM:
        case cmERR_ABORT_DLOPEN:
        case cmERR_ABORT_SELECT_ERROR:               /* BUG-47714 */
            ulnDbcSetIsConnected(sDbc, ACP_FALSE);
            break;

        default:
            /* do nothing */
            break;
    }

    ACI_TEST( ulnErrorAddDiagRec(aFnContext,
                                 &sErrorMgr,
                                 SQL_NO_ROW_NUMBER,
                                 SQL_NO_COLUMN_NUMBER)
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnErrHandleCmError : object type is neither DBC nor STMT.");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

