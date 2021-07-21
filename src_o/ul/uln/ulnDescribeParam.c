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
#include <ulnDescribeParam.h>

/*
 * ==============
 * IMPORTANT Note
 * ==============
 *
 * SQLDecribeParam() 함수를 호출해서 사용자가 얻게 되는 정보는 IPD 레코드에 있는 정보가
 * 되어야 한다.
 *
 * 어려운 문제는,
 *  1. 사용자가 이미 바인드한 파라미터에 SQLDescribeParam() 을 호출할 경우 어떻게 할것인가?
 *
 * 인데, 아래와 같이 정책을 결정하였다 :
 *
 *  1. 이미 바인드한 파라미터라면 IPD record 에 있는 정보를 읽어서 그대로 사용자에게 넘겨준다.
 *
 *  2. 아직 바인드되지 않은 파라미터라면 서버에서 보내온 정보를 이용해 만든 IPD record 를
 *     IPD 에 매달아버린다. 이렇게 만들어진 IPD record 는 사용자가 바인드를 할 때 덮어씌여진다.
 *     덮어씌여질 IPD record 를 IPD 에 매다는 이유는, 사용자가 SQLGetDescField() 함수를 호출할
 *     경우 정보를 재사용하기 위해서이다.
 *
 *  3. SQLGetDescField() 함수를 이용해서 IPD record 의 필드들을 사용자가 얻어오고자 했을 경우,
 *     해당 IPD record 가 존재하지 않으면 SQLDescribeParam() 의 루틴들을 이용해서 IPD record 를
 *     생성하도록 한다.
 *
 *  4. 사용자가 SQLBindParameter() 를 하게 되면 IPD record 의 정보를 갱신하도록 한다.
 *
 * 위의 정책은, SQLDescribeParam() 함수가 "서버에서 필요로 하는" 파라미터의 메타라고 생각할 때
 * 약간 이상할 수 있다.
 * 그러나, SQLBindParameter() 함수의 주요 역할 중에 하나가 IPD record 를 "갱신" 하는 것이라는
 * 사실을 생각해 볼 때 전혀 이상하지 않다.
 *
 * SQLGetDescField() 를 통해 얻는 IRD record 의 정보와
 * SQLDescribeCol() 을 통해 얻는 정보는 일치해야 하며, 현재 일치하도록 구현되어 있다.
 *
 * 마찬가지로,
 * SQLGetDescField() 를 통해 얻는 IPD record 의 정보와
 * SQLDescribeParam() 을 통해 얻는 정보는 일치해야 한다.
 * ---> SQLDescribeParam() 시 IPD record 가 존재함에도 불구하고 서버에서 정보를
 * 얻어와서 IPD record 의 내용과 다른 내용을 리턴해서는 안된다.
 */

ACI_RC ulnDescribeParamGetIpdInfoFromServer(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnDbc       *sDbc;
    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST(ulnWriteParamInfoGetREQ(aFnContext, aPtContext, 0) != ACI_SUCCESS);

    /* bug-31989: SQLDescribeParam() ignores connection_timeout. */
    ACI_TEST(ulnFlushAndReadProtocol(aFnContext,
                                     aPtContext,
                                     sDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnDescribeParamCheckArgs(ulnFnContext *aFnContext, acp_uint16_t aParamNumber)
{
    ACI_TEST_RAISE(aParamNumber == 0, LABEL_INVALID_DESC_INDEX);

    ACI_TEST_RAISE(ulnStmtGetParamCount(aFnContext->mHandle.mStmt) < aParamNumber,
                   LABEL_INVALID_DESC_INDEX);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_DESC_INDEX)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aParamNumber);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnDescribeParamCore(ulnFnContext *aFnContext,
                                   ulnPtContext *aPtContext,
                                   ulnStmt      *aStmt,
                                   acp_uint16_t  aParamNumber,
                                   acp_sint16_t *aDataTypePtr,
                                   ulvULen      *aParamSizePtr,
                                   acp_sint16_t *aDecimalDigitsPtr,
                                   acp_sint16_t *aNullablePtr)
{
    acp_sint16_t  sSQLTYPE;
    ulnDescRec   *sDescRecIpd = NULL;
    ulnMTypeID    sMTYPE;

    sDescRecIpd = ulnStmtGetIpdRec(aStmt, aParamNumber);

    /* BUG-44296 ipd가 하나라도 없으면 서버에서 정보를 가져와 없는 ipd를 만든다. */
    if (sDescRecIpd == NULL)
    {
        ACI_TEST(ulnDescribeParamGetIpdInfoFromServer(aFnContext, aPtContext) != ACI_SUCCESS);

        sDescRecIpd = ulnStmtGetIpdRec(aStmt, aParamNumber);

        /*
         * 파라미터의 갯수보다 큰 ParamNumber 를 넘겨줬기 때문에 IPD record 가 발견되지 않을
         * 가능성도 있다. 그러나, SQLDescribeParam() 함수는 Prepared 상태(S2, S3)에서만
         * 유일하게 호출될 수 있다. 따라서, 이 함수가 호출될 시점에서는,
         * 필요한 파라미터의 갯수를 이미 알고 있는 상태이며, 함수 진입시에 파라미터 넘버를
         * 체크해서 범위에 들어올 때에만 통과시킨다.
         * 그러므로 지금 서버에서 parameter 정보들을 가져 왔는데, IPD record 가 없다는 것은
         * 메모리 corruption 으로 단정지어도 좋은 상황이다.
         */
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MAN_ERR);
    }

    if (aDataTypePtr != NULL)
    {
        sSQLTYPE = ulnMetaGetOdbcConciseType(&sDescRecIpd->mMeta);
        sSQLTYPE = ulnTypeMap_LOB_SQLTYPE(sSQLTYPE,
                                          ulnDbcGetLongDataCompat(aStmt->mParentDbc));

        *aDataTypePtr = sSQLTYPE;
    }

    if (aParamSizePtr != NULL)
    {
        /*
         * Note : ulnMeta 의 octet length 에는 항상 양수가 들어간다.
         */
        *aParamSizePtr = (ulvULen)ulnMetaGetOctetLength(&sDescRecIpd->mMeta);
    }

    if (aDecimalDigitsPtr != NULL)
    {
        sMTYPE = ulnMetaGetMTYPE(&sDescRecIpd->mMeta);

        if (sMTYPE == ULN_MTYPE_NUMBER || sMTYPE == ULN_MTYPE_NUMERIC)
        {
            *aDecimalDigitsPtr = ulnMetaGetScale(&sDescRecIpd->mMeta);
        }
        else
        {
            *aDecimalDigitsPtr = 0;
        }
    }

    if (aNullablePtr != NULL)
    {
        *aNullablePtr = ulnMetaGetNullable(&sDescRecIpd->mMeta);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnDescribeParamCore");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnDescribeParam(ulnStmt      *aStmt,
                           acp_uint16_t  aParamNumber,
                           acp_sint16_t *aDataTypePtr,
                           ulvULen      *aParamSizePtr,
                           acp_sint16_t *aDecimalDigitsPtr,
                           acp_sint16_t *aNullablePtr)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_DESCRIBEPARAM, aStmt, ULN_OBJ_TYPE_STMT);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /* PROJ-1891, BUG-46011 If deferred prepare is exists, process it first */
    if (ulnStmtIsSetDeferredQstr(aStmt) == ACP_TRUE)
    {
        ACI_TEST( ulnPrepareDeferComplete(&sFnContext, ACP_TRUE) );
    }

    /*
     * 넘겨받은 인자들 체크
     */
    ACI_TEST(ulnDescribeParamCheckArgs(&sFnContext, aParamNumber) != ACI_SUCCESS);

    /*
     * Protocol Context 초기화
     */
    // fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(aStmt->mParentDbc->mPtContext),
                                          &(aStmt->mParentDbc->mSession))
             != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);
    // fix BUG-17722
    ACI_TEST(ulnDescribeParamCore(&sFnContext,
                                  &(aStmt->mParentDbc->mPtContext),
                                  aStmt,
                                  aParamNumber,
                                  aDataTypePtr,
                                  aParamSizePtr,
                                  aDecimalDigitsPtr,
                                  aNullablePtr) != ACI_SUCCESS);

    /*
     * Protocol Context 정리
     */
    ULN_FLAG_DOWN(sNeedFinPtContext);
    // fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                        &(aStmt->mParentDbc->mPtContext))
                != ACI_SUCCESS);

    /*
     * Exit
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" stypePtr: %p]",
            "ulnDescribeParam", aParamNumber, aDataTypePtr);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        // fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext,
                                   &(aStmt->mParentDbc->mPtContext) );
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" stypePtr: %p] fail",
            "ulnDescribeParam", aParamNumber, aDataTypePtr);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
