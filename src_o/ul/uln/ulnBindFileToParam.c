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

/*
 * 07006 제한된 데이터 타입 속성 위반 | sqlType 인자에 명시된 값이 유효한 값이 아님
 * 07009 유효하지 않은 번호           | 명시된 par의 값이 1보다 작음
 * 08S01 통신 회선 장애               | ODBC와 데이터베이스 간에 함수 처리가 완료되기
 *                                    | 전에 통신 회선 실패
 * HY000 일반 오류                    |
 * HY001 메모리 할당 오류             | 이 함수를 수행하기 위해 필요한 메모리 할당 실패
 * HY009 유효하지 않은 인자 사용      | fileName, fileOptions, ind 인자가 null pointer
 *       (null pointer)
 * HY090 유효하지 않은 문자열 길이    | maxFileNameLength 인자가 0보다 작음
 */

static ACI_RC ulnBindFileToParamCheckArgs(ulnFnContext  *aContext,
                                          acp_uint16_t   aParamNumber,
                                          acp_sint16_t   aSqlType,
                                          acp_char_t   **aFileNameArray,
                                          acp_uint32_t  *aFileOptionPtr,
                                          ulvSLen        aMaxFileNameLength,
                                          ulvSLen       *aIndicator)
{
    ACI_TEST_RAISE( aParamNumber < 1, LABEL_INVALID_PARAMNUM );

    ACI_TEST_RAISE( aSqlType != SQL_BINARY && aSqlType != SQL_BLOB && aSqlType != SQL_CLOB, LABEL_INVALID_CONVERSION );

    ACI_TEST_RAISE( aFileNameArray == NULL || aFileOptionPtr == NULL || aIndicator == NULL, LABEL_INVALID_USE_OF_NULL );

    ACI_TEST_RAISE( aMaxFileNameLength < ULN_vLEN(0), LABEL_INVALID_BUFFER_LEN );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_PARAMNUM)
    {
        /*
         * Invalid descriptor index
         * 07009
         */
        ulnError(aContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aParamNumber);
    }
    ACI_EXCEPTION(LABEL_INVALID_CONVERSION)
    {
        /*
         * Restricted data type attribute violation
         * 07006
         */
        ulnError(aContext,
                 ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION,
                 SQL_C_FILE,
                 aSqlType);
    }
    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        /*
         * HY009
         */
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN)
    {
        /*
         * Invalid string or buffer length
         */
        ulnError(aContext, ulERR_ABORT_INVALID_BUFFER_LEN, aMaxFileNameLength);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * filename 과 type, 등에 관한 것들은 모두다 APD record 에 저장한다.
 *
 * 차후 execute 시에 해당 파일들을 open 해서 read 하여 데이터를 전송한다.
 *
 * BUGBUG : aIndicator 의 의미를 다시한번 생각해 보도록 하자.
 */
SQLRETURN ulnBindFileToParam(ulnStmt       *aStmt,
                             acp_uint16_t   aParamNumber,
                             acp_sint16_t   aSqlType,
                             acp_char_t   **aFileNameArray,
                             ulvSLen       *aFileNameLengthArray,
                             acp_uint32_t  *aFileOptionPtr,
                             ulvSLen        aMaxFileNameLength,
                             ulvSLen       *aIndicator)
{
    ulnFnContext  sFnContext;
    ulnPtContext *sPtContext = NULL;
    ulnDescRec   *sDescRecApd;

    acp_bool_t    sNeedExit = ACP_FALSE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_BINDFILETOPARAM, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    sPtContext = &(aStmt->mParentDbc->mPtContext);
    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    ACI_TEST_RAISE(cmiGetLinkImpl(&sPtContext->mCmiPtContext) == CMI_LINK_IMPL_IPCDA,
                   IPCDANotSupport);
    /*
     * arguments checking
     */
    ACI_TEST(ulnBindFileToParamCheckArgs(&sFnContext,
                                         aParamNumber,
                                         aSqlType,
                                         aFileNameArray,
                                         aFileOptionPtr,
                                         aMaxFileNameLength,
                                         aIndicator) != ACI_SUCCESS);

    /*
     * ulnBindParameter() 를 호출하여 기본적인 APD, IPD 세팅을 한다.
     *
     * Note : aStrLenOrIndPtr 에 aIndicator 가 아니라
     *        aFileNameLengthArray 를 저장함으로써
     *        ulnDescRec 의 mOctetLengthPtr 에 aFileNameLengthArray 가 들어가도록 한다.
     */
    ACI_TEST(ulnBindParamBody(&sFnContext,
                              aParamNumber,
                              NULL,
                              SQL_PARAM_INPUT,
                              SQL_C_FILE,
                              aSqlType,
                              0, // column size : BUGBUG
                              0, // decimal digits : BUGBUG
                              (void *)aFileNameArray,
                              aMaxFileNameLength,
                              aFileNameLengthArray) != ACI_SUCCESS);

    sDescRecApd = ulnStmtGetApdRec(aStmt, aParamNumber);

    ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_MEM_MAN_ERR);

    /*
     * file option 배열과 indicator 배열을 할당한다.
     *
     * BUGBUG : 좀 더 우아하게 통합하는 방법을 생각해야 한다. 고민좀 하자.
     */
    sDescRecApd->mFileOptionsPtr = aFileOptionPtr;
    ulnDescRecSetIndicatorPtr(sDescRecApd, aIndicator);

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "BindFileToParam");
    }
    ACI_EXCEPTION(IPCDANotSupport)
    {
        ulnError(&sFnContext, ulERR_ABORT_IPCDA_UNSUPPORTED_FUNCTION);
    }
    ACI_EXCEPTION_END;

    if (sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
