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
#include <ulnBindParameter.h>
#include <ulsdnBindData.h>

static ACI_RC ulnBindParamCheckArgs(ulnFnContext *aFnContext,
                                    acp_uint16_t  aParamNumber,
                                    acp_sint16_t  aInputOutputType,
                                    acp_sint16_t  aValueType,
                                    void         *aParamValuePtr,
                                    ulvSLen       aBufferLength,
                                    ulvSLen      *aStrLenOrIndPtr)
{
    /*
     * 파라미터 넘버 유효성 체크
     */
    ACI_TEST_RAISE(aParamNumber < 1, LABEL_INVALID_PARAMNUM);

    /*
     * InputOutput 타입 인자의 유효성 체크
     */
    ACI_TEST_RAISE((aInputOutputType != SQL_PARAM_INPUT &&
                    aInputOutputType != SQL_PARAM_OUTPUT &&
                    aInputOutputType != SQL_PARAM_INPUT_OUTPUT),
                    LABEL_INVALID_IN_OUT_TYPE);

    /*
     * Buffer Length 유효성 체크
     */
    ACI_TEST_RAISE(aBufferLength < ULN_vLEN(0), LABEL_INVALID_BUFFER_LEN);

    /*
     * NULL 포인터의 적법한 사용 체크
     */
    ACI_TEST_RAISE(aStrLenOrIndPtr  == NULL &&
                   aInputOutputType != SQL_PARAM_OUTPUT &&
                   aParamValuePtr   == NULL,
                   LABEL_INVALID_USE_OF_NULL);

    ACI_TEST_RAISE((aParamValuePtr == NULL && aInputOutputType != SQL_PARAM_OUTPUT) &&
                   (*aStrLenOrIndPtr != SQL_NULL_DATA && *aStrLenOrIndPtr != SQL_DATA_AT_EXEC),
                   LABEL_INVALID_USE_OF_NULL);

    ACI_TEST_RAISE(aBufferLength    >  ULN_vLEN(0) &&
                   aInputOutputType == SQL_PARAM_OUTPUT &&
                   aParamValuePtr   == NULL &&
                   (aValueType == SQL_C_CHAR || /*aValueType == SQL_C_WCHAR ||*/
                    aValueType == SQL_C_BINARY),
                   LABEL_INVALID_USE_OF_NULL);

    /*
     * BUGBUG : HYC00 을 꼭 만들어야 하는지... 좀 애매하다. 다음 기회에 만들자.
     */

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_IN_OUT_TYPE)
    {
        /* HY105 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_PARAM_TYPE, aInputOutputType);
    }

    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        /* HY009 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN)
    {
        /* HY090 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aBufferLength);
    }

    ACI_EXCEPTION(LABEL_INVALID_PARAMNUM)
    {
        /* 07009 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aParamNumber);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnBindParamIpdPart(ulnFnContext      *aFnContext,
                                  ulnDescRec       **aDescRecIpd,
                                  acp_uint16_t       aParamNumber,
                                  acp_sint16_t       aSQL_TYPE,
                                  ulnMTypeID         aMTYPE,
                                  ulvULen            aColumnSize,
                                  acp_sint16_t       aDecimalDigits,
                                  ulnParamInOutType  aInputOutputType)
{
    ulnStmt    *sStmt       = aFnContext->mHandle.mStmt;
    ulnDescRec *sDescRecIpd = NULL;
    ulnMeta    *sIpdMeta    = NULL;

    /*
     * IPD record 준비
     */
    ACI_TEST_RAISE(ulnBindArrangeNewDescRec(sStmt->mAttrIpd, aParamNumber, &sDescRecIpd)
                   != ACI_SUCCESS,
                   LABEL_OUTOF_MEM);

    sIpdMeta = &sDescRecIpd->mMeta;

    /*
     * ulnMeta 초기화
     */
    ulnMetaInitialize(sIpdMeta);

    ulnMetaBuild4IpdByMeta(sIpdMeta,
                           aMTYPE,
                           aSQL_TYPE,
                           aColumnSize,
                           aDecimalDigits);

    /*
     * IPD record 의 in/out 타입 세팅
     */
    ulnDescRecSetParamInOut(sDescRecIpd, aInputOutputType);

    /*
     * IPD record 를 IPD 에 매달기
     */
    ACI_TEST_RAISE(ulnDescAddDescRec(sStmt->mAttrIpd, sDescRecIpd) != ACI_SUCCESS,
                   LABEL_OUTOF_MEM);

    *aDescRecIpd = sDescRecIpd;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUTOF_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "BindParamIpdPart");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * BUGBUG : APD 와 ARD 의 meta information 을 만드는 것이 너무나도 비슷하다.
 *          공통되는 하나의 함수로 분리 추출 가능하다. 생각해 보도록 하자.
 */
ACI_RC ulnBindParamApdPart(ulnFnContext      *aFnContext,
                           ulnDescRec       **aDescRecApd,
                           acp_uint16_t       aParamNumber,
                           acp_sint16_t       aSQL_C_TYPE,
                           ulnParamInOutType  aInputOutputType,
                           void              *aParamValuePtr,
                           ulvSLen            aBufferLength,
                           ulvSLen           *aStrLenOrIndPtr)
{
    ulnStmt    * sStmt       = aFnContext->mHandle.mStmt;
    ulnDescRec * sDescRecApd = NULL;
    ulnMTypeID   sMTYPE      = 0;
    ulnCTypeID   sCTYPE      = 0;

    /*PROJ-2638 shard native linker*/
    if ( aSQL_C_TYPE >= ULSD_INPUT_RAW_MTYPE_NULL )
    {
        sMTYPE = aSQL_C_TYPE - ULSD_INPUT_RAW_MTYPE_NULL;
        sCTYPE = ulsdTypeMap_MTYPE_CTYPE( sMTYPE );
    }
    else
    {
        sCTYPE = ulnTypeMap_SQLC_CTYPE(aSQL_C_TYPE);
    }

    /*
     * APD record 준비
     */
    ACI_TEST_RAISE(ulnBindArrangeNewDescRec(sStmt->mAttrApd, aParamNumber, &sDescRecApd)
                   != ACI_SUCCESS,
                   LABEL_OUTOF_MEM);

    ulnMetaInitialize(&sDescRecApd->mMeta);

    /*
     * 사용자 변수의 타입을 묘사하는 meta 구조체 작업
     */

    ulnMetaBuild4ArdApd(&sDescRecApd->mMeta,
                        sCTYPE,
                        aSQL_C_TYPE,
                        aBufferLength);

    /*
     * Note : APD 의 in/out type 은 사용자가 지정한 in/out 타입을 그대로 둔다.
     *        LOB 컬럼인 경우에 서버로 보내는 BINDINFO SET REQ 에서는 그때그때
     *        체크해서 in/out type 조정.
     */
    ulnDescRecSetParamInOut(sDescRecApd, aInputOutputType);

    /*
     * 사용자 변수의 메모리를 묘사하는 desc rec 멤버 세팅
     */
    sDescRecApd->mDataPtr = aParamValuePtr;

    ulnDescRecSetOctetLengthPtr(sDescRecApd, aStrLenOrIndPtr);
    ulnDescRecSetIndicatorPtr(sDescRecApd, aStrLenOrIndPtr);

    /*
     * APD record 를 APD 에 매달기
     */
    ACI_TEST_RAISE(ulnDescAddDescRec(sStmt->mAttrApd, sDescRecApd) != ACI_SUCCESS,
                   LABEL_OUTOF_MEM);

    *aDescRecApd = sDescRecApd;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_OUTOF_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "BindParamApdPart");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnBindParamBody(ulnFnContext *aFnContext,
                        acp_uint16_t  aParamNumber,
                        acp_char_t   *aParamName,
                        acp_sint16_t  aInputOutputType,
                        acp_sint16_t  aValueType,
                        acp_sint16_t  aParamType,
                        ulvULen       aColumnSize,
                        acp_sint16_t  aDecimalDigits,
                        void         *aParamValuePtr,
                        ulvSLen       aBufferLength,
                        ulvSLen      *aStrLenOrIndPtr)
{
    ulnDbc           *sDbc;
    ulnDescRec       *sDescRecApd;
    ulnDescRec       *sDescRecIpd;
    ulnMTypeID        sMTYPE;
    ulnCTypeID        sCTYPE;
    ulnParamInOutType sInputOutputType = ULN_PARAM_INOUT_TYPE_MAX;

    /* PROJ-1721 Name-based Binding */
    ulnStmt          *sStmt        = NULL;
    acp_uint16_t     *sParamPosArr = NULL;
    acp_uint16_t      sParamPosCnt = 0;
    acp_uint16_t      i;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /*
     * MTYPE 과 CTYPE 을 우선 구한 후, 서로 적법한 짝인지 체크
     */
    if ( ulsdIsInputMTData( aParamType ) == ACP_FALSE )
    {
        sMTYPE = ulnTypeMap_SQL_MTYPE( aParamType );
        sCTYPE = ulnTypeMap_SQLC_CTYPE( aValueType );
    }
    else
    {
        /*PROJ-2638 shard native linker*/
        sMTYPE = aParamType - ULSD_INPUT_RAW_MTYPE_NULL;
        sCTYPE = ulsdTypeMap_MTYPE_CTYPE(sMTYPE);
    }

    ACI_TEST_RAISE(sMTYPE == ULN_MTYPE_MAX, LABEL_INVALID_SQL_TYPE);
    ACI_TEST_RAISE(sCTYPE == ULN_CTYPE_MAX, LABEL_INVALID_C_TYPE);

    ACI_TEST_RAISE(ulnBindInfoGetMTYPEtoSet(sCTYPE, sMTYPE) == ULN_MTYPE_MAX,
                   LABEL_INVALID_CONVERSION);

    /*
     * Note : Altibase 에는 TIME 과 DATE 의 구분이 없으므로 이렇게 손으로 구분해
     *        주어야 한다.
     *
     * BUGBUG : ctype 을 보고 구분해야 하나, mtype 을 보고 해야 하나
     */
    ACI_TEST_RAISE(sCTYPE == ULN_CTYPE_DATE && aParamType == SQL_TYPE_TIME,
                   LABEL_INVALID_CONVERSION);

    ACI_TEST_RAISE(sCTYPE == ULN_CTYPE_TIME && aParamType == SQL_TYPE_DATE,
                   LABEL_INVALID_CONVERSION);

    switch (aInputOutputType)
    {
        case SQL_PARAM_INPUT:
            sInputOutputType = ULN_PARAM_INOUT_TYPE_INPUT;
            break;

        case SQL_PARAM_OUTPUT:
            sInputOutputType = ULN_PARAM_INOUT_TYPE_OUTPUT;
            break;

        case SQL_PARAM_INPUT_OUTPUT:
            sInputOutputType = ULN_PARAM_INOUT_TYPE_IN_OUT;
            break;

        default:
            ACE_ASSERT(0);
    }

    /* PROJ-1721 Name-based Binding */
    sStmt = aFnContext->mHandle.mStmt;

    if (aParamName != NULL)
    {
        /*
         * sParamPosArr이 NULL로 리턴되는 조건
         *
         * 1. aParamName이 Statement에 없는 경우
         * 2. Statement에 ?, :name이 혼용된 경우
         */
        ulnAnalyzeStmtGetPosArr(sStmt->mAnalyzeStmt,
                                aParamName,
                                &sParamPosArr,
                                &sParamPosCnt);
    }
    else
    {
        /* Nothing */
    }

    if (sParamPosArr == NULL)
    {
        sParamPosArr = &aParamNumber;
        sParamPosCnt = 1;
    }
    else
    {
        /* Nothing */
    }

    for (i = 0; i < sParamPosCnt; i++)
    {
        /*
         * BUGBUG : IPD 의 의미를 확실하게 정립시켜야 한다.
         *          특히나 LOB, LOB_LOCATOR 와 관련한 부분에서.
         *
         * IPD 관련 처리
         */
        ACI_TEST(ulnBindParamIpdPart(aFnContext,
                                     &sDescRecIpd,
                                     sParamPosArr[i],
                                     aParamType,
                                     sMTYPE,
                                     aColumnSize,
                                     aDecimalDigits,
                                     sInputOutputType) != ACI_SUCCESS);

        ulnMetaAdjustIpdByMeta( sDbc,
                                &sDescRecIpd->mMeta,
                                aColumnSize,
                                sMTYPE );

        /*
         * APD 관련 처리
         */
        ACI_TEST(ulnBindParamApdPart(aFnContext,
                                     &sDescRecApd,
                                     sParamPosArr[i],
                                     aValueType,
                                     sInputOutputType,
                                     aParamValuePtr,
                                     aBufferLength,
                                     aStrLenOrIndPtr) != ACI_SUCCESS);

        ACI_TEST( ulnBindAdjustStmtInfo( aFnContext->mHandle.mStmt ) != ACI_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_SQL_TYPE)
    {
        /* HY004 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_SQL_TYPE, aParamType);
    }

    ACI_EXCEPTION(LABEL_INVALID_C_TYPE)
    {
        /*
         * Invalid Application Buffer Type
         * BUGBUG: 스펙의 에러코드 리스트에는 aValueType 이 SQL_C_DEFAULT 일때도 HY003 인데,
         * 또 ValueType 설명할때 보면 SQL_C_DEFAULT 일 때에는 어떻게 어떻게 하라고
         * 설명해 두고 있다.
         * 어느 장단에 춤을 출꼬 -_-;;;;
         * 일단, SQL_C_DEFAULT 는 valid 한 것으로 하고 넘어간다.
         *
         * HY003
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aValueType);
    }

    ACI_EXCEPTION(LABEL_INVALID_CONVERSION)
    {
        /*
         * 07006 : Restricted data type attribute violation
         *         The data type identified by the ValueType argument cannot be converted
         *         to the data type identified by the ParameterType argument.
         *         Note that this error may be returned by SQLExecDirect, SQLExecute, or
         *         SQLPutData at execution time, instead of by SQLBindParameter.
         */
        ulnError(aFnContext, ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION, aValueType, aParamType);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnBindParameter(ulnStmt      *aStmt,
                           acp_uint16_t  aParamNumber,
                           acp_char_t   *aParamName,
                           acp_sint16_t  aInputOutputType,
                           acp_sint16_t  aValueType,
                           acp_sint16_t  aParamType,
                           ulvULen       aColumnSize,
                           acp_sint16_t  aDecimalDigits,
                           void         *aParamValuePtr,
                           ulvSLen       aBufferLength,
                           ulvSLen      *aStrLenOrIndPtr)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;
    ulnPtContext *sPtContext = NULL;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_BINDPARAMETER, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    sPtContext = &(aStmt->mParentDbc->mPtContext);

    ACI_TEST_RAISE( (aParamType == SQL_CLOB           ||
                     aParamType == SQL_BLOB           ||
                     aParamType == SQL_C_BLOB_LOCATOR ||
                     aParamType == SQL_C_CLOB_LOCATOR ) &&
                    (cmiGetLinkImpl(&sPtContext->mCmiPtContext) == CMI_LINK_IMPL_IPCDA),
                    IPCDANotSupportParamType );

    if ( (aStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_ON)
        && (aStmt->mParamCount < aParamNumber) )
    {
        ulnStmtSetParamCount(aStmt, aParamNumber);
    }

    ACI_TEST(ulnBindParamCheckArgs(&sFnContext,
                                   aParamNumber,
                                   aInputOutputType,
                                   aValueType,
                                   aParamValuePtr,
                                   aBufferLength,
                                   aStrLenOrIndPtr) != ACI_SUCCESS);

    ACI_TEST(ulnBindParamBody(&sFnContext,
                              aParamNumber,
                              aParamName,
                              aInputOutputType,
                              aValueType,
                              aParamType,
                              aColumnSize,
                              aDecimalDigits,
                              aParamValuePtr,
                              aBufferLength,
                              aStrLenOrIndPtr) != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" io: %"ACI_INT32_FMT
            " ctype: %3"ACI_INT32_FMT" stype: %2"ACI_INT32_FMT
            " size: %3"ACI_UINT32_FMT" buf: %p "
            " len: %3"ACI_INT32_FMT"]", "ulnBindParameter",
            aParamNumber, aInputOutputType, aValueType, aParamType,
            aColumnSize, aParamValuePtr, (acp_sint32_t)aBufferLength);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(IPCDANotSupportParamType)
    {
        ulnError(&sFnContext, ulERR_ABORT_IPCDA_UNSUPPORTED_SQL_DATA_TYPE, aParamType);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" io: %"ACI_INT32_FMT
            " ctype: %3"ACI_INT32_FMT" stype: %2"ACI_INT32_FMT
            " size: %3"ACI_UINT32_FMT" buf: %p "
            " len: %3"ACI_INT32_FMT"] fail", "ulnBindParameter",
            aParamNumber, aInputOutputType, aValueType, aParamType,
            aColumnSize, aParamValuePtr, (acp_sint32_t)aBufferLength);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
