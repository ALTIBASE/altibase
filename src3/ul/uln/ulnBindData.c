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
#include <ulnBindData.h>
#include <ulnPDContext.h>
#include <ulnConv.h>
#include <ulnCharSet.h>
#include <ulsdnBindData.h>
#include <ulsdDistTxInfo.h>

typedef ACI_RC ulnParamProcessFunc(ulnFnContext      *aFnContext,
                                   ulnPtContext      *aPtContext,
                                   ulnDescRec        *aDescRecApd,
                                   ulnDescRec        *aDescRecIpd,
                                   void              *aUserDataPtr,
                                   ulnIndLenPtrPair  *aUserIndLenPair,
                                   acp_uint32_t       aRowNumber);

static ACI_RC ulnParamProcess_DEFAULT(ulnFnContext      *aFnContext,
                                      ulnPtContext      *aPtContext,
                                      ulnDescRec        *aDescRecApd,
                                      ulnDescRec        *aDescRecIpd,
                                      void              *aUserDataPtr,
                                      ulnIndLenPtrPair  *aUserIndLenPair,
                                      acp_uint32_t       aRowNumber)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aPtContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aUserDataPtr);
    ACP_UNUSED(aUserIndLenPair);
    ACP_UNUSED(aRowNumber);

    return ACI_SUCCESS;
}

static ACI_RC ulnParamProcess_DATA(ulnFnContext      *aFnContext,
                                   ulnPtContext      *aPtContext,
                                   ulnDescRec        *aDescRecApd,
                                   ulnDescRec        *aDescRecIpd,
                                   void              *aUserDataPtr,
                                   ulnIndLenPtrPair  *aUserIndLenPair,
                                   acp_uint32_t       aRowNumber)
{
    ulnStmt     *sStmt    = aFnContext->mHandle.mStmt;
    ulnCharSet   sCharSet;
    acp_sint32_t sUserOctetLength;
    acp_sint32_t sState = 0;

    ACP_UNUSED(aRowNumber);

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    if (aUserIndLenPair->mLengthPtr == NULL)
    {
        /*
         * ODBC spec. SQLBindParameter() StrLen_or_IndPtr argument 의 설명 :
         * 이게 NULL 이면 모든 input parameter 가 non-null 이고,
         * char, binary 데이터는 null-terminated 된 것이라고 가정해야 한다.
         *
         * BUG-13704 SES 의 -n 옵션을 위한 처리. 자세한 내용은
         *           SQL_ATTR_INPUT_NTS 가 선언된 헤더파일의 주석을 참고할 것.
         */
        if (ulnStmtGetAttrInputNTS(sStmt) == ACP_TRUE)
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
        sUserOctetLength = ulnBindGetUserIndLenValue(aUserIndLenPair);
    }

    /*
     * 이 함수는 output parameter 에 대해서는 절대로 호출되지 않는다.
     *      --> 따라서 in/out 타입 체크 할 필요 없이 안심하고 PARAM DATA IN 해도 된다.
     */

    ACI_TEST( aDescRecApd->mBindInfo.mParamDataInBuildAnyFunc( aFnContext,
                                                               aDescRecApd,
                                                               aDescRecIpd,
                                                               aUserDataPtr,
                                                               sUserOctetLength,
                                                               aRowNumber,
                                                               NULL,
                                                               &sCharSet) != ACI_SUCCESS );

    ACI_TEST(ulnWriteParamDataInREQ(aFnContext,
                                    aPtContext,
                                    aDescRecApd->mIndex,
                                    aUserDataPtr,
                                    sUserOctetLength,
                                    &aDescRecApd->mMeta,
                                    &aDescRecIpd->mMeta) != ACI_SUCCESS);

    ulnStmtChunkReset( sStmt );
    // BUGBUG : 이거 없애도 되나?
    // aDescRecApd->mState = ULN_DR_ST_INITIAL;
    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

static ACI_RC ulnParamProcess_DATA_AT_EXEC(ulnFnContext      *aFnContext,
                                           ulnPtContext      *aPtContext,
                                           ulnDescRec        *aDescRecApd,
                                           ulnDescRec        *aDescRecIpd,
                                           void              *aUserDataPtr,
                                           ulnIndLenPtrPair  *aUserIndLenPair,
                                           acp_uint32_t       aRowNumber)
{
    ulvSLen           sAccumulatedDataLength;
    ulnIndLenPtrPair  sUserIndLenPair;
    ulnPDContext     *sPDContext = &aDescRecApd->mPDContext;

    ACP_UNUSED(aUserDataPtr);
    ACP_UNUSED(aUserIndLenPair);

    // idlOS::printf("#### normal pd state = %d\n", ulnPDContextGetState(sPDContext));

    switch (ulnPDContextGetState(sPDContext))
    {
        case ULN_PD_ST_INITIAL:
            ACI_RAISE(LABEL_INVALID_STATE);
            break;

        case ULN_PD_ST_CREATED:     /* SQLExecute() 에서 들어옴 */
        case ULN_PD_ST_NEED_DATA:   /* SQLParamData() 에서 들어옴 */
            /*
             * 최초의 SQLPutData() 호출시 아래의 상태전이 발생 :
             *      ULN_PD_ST_NEED_DATA --> ULN_PD_ST_ACCUMULATING_DATA
             */
            ulnPDContextSetState(sPDContext, ULN_PD_ST_NEED_DATA);
            ACI_RAISE(LABEL_NEED_DATA);
            break;

        case ULN_PD_ST_ACCUMULATING_DATA:

            /*
             * SQLPutData() 를 이용해서 데이터를 쌓고 있는 state 에
             * SQLParamData() 가 호출되었으므로 이곳으로 들어옴.
             * SQLParamData() 호출은 데이터 적재가 끝났다는 의미이므로 적재된 데이터 전송.
             */

            sAccumulatedDataLength        = ulnPDContextGetDataLength(sPDContext);
            sUserIndLenPair.mLengthPtr    = &sAccumulatedDataLength;
            sUserIndLenPair.mIndicatorPtr = &sAccumulatedDataLength;

            ACI_TEST_RAISE(ulnParamProcess_DATA(aFnContext,
                                                aPtContext,
                                                aDescRecApd,
                                                aDescRecIpd,
                                                ulnPDContextGetBuffer(sPDContext),
                                                &sUserIndLenPair,
                                                aRowNumber) != ACI_SUCCESS,
                           LABEL_NEED_PD_FIN);

            sPDContext->mOp->mFinalize(sPDContext);

            ulnDescRemovePDContext(aDescRecApd->mParentDesc, sPDContext);

            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_STATE)
    {
        ulnErrorExtended(aFnContext,
                         SQL_ROW_NUMBER_UNKNOWN,
                         aDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnParamProcess_DATA_AT_EXEC");
    }

    ACI_EXCEPTION(LABEL_NEED_DATA)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_NEED_DATA);
    }

    ACI_EXCEPTION(LABEL_NEED_PD_FIN)
    {
        sPDContext->mOp->mFinalize(sPDContext);

        ulnDescRemovePDContext(aDescRecApd->mParentDesc, sPDContext);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnParamProcess_ERROR(ulnFnContext      *aFnContext,
                                    ulnPtContext      *aPtContext,
                                    ulnDescRec        *aDescRecApd,
                                    ulnDescRec        *aDescRecIpd,
                                    void              *aUserDataPtr,
                                    ulnIndLenPtrPair  *aUserIndLenPair,
                                    acp_uint32_t       aRowNumber)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aPtContext);
    ACP_UNUSED(aDescRecApd);
    ACP_UNUSED(aDescRecIpd);
    ACP_UNUSED(aUserDataPtr);
    ACP_UNUSED(aUserIndLenPair);
    ACP_UNUSED(aRowNumber);

    /*
     * 이 함수는 DATA AT EXEC 파라미터를 output 으로 바인딩 했을 때
     * 호출되는 함수이다. 무조건 에러를 리턴하고 쫑낸다.
     */

    /*
     * Note : OUTPUT 파라미터일 때에는 StrLenOrIndPtr 을 무시하므로
     *        DATA AT EXEC 인지 뭔지 알 도리가 없다.
     *
     *        그냥 리턴.
     */

    return ACI_SUCCESS;
}

static ulnParamProcessFunc * const gUlnParamProcessorTable[ULN_PARAM_TYPE_MAX] =
{
    /* Mem    DATA   User       Build &            */
    /* Bound   AT    IN/OUT     Send      Send     */
    /* LOB    EXEC   Type       BindInfo  BindData */

    /* FALSE  FALSE   IN    */  ulnParamProcess_DATA,
    /* FALSE  FALSE   OUT   */  ulnParamProcess_DEFAULT,

    /* FALSE  TRUE    IN    */  ulnParamProcess_DATA_AT_EXEC,
    /* FALSE  TRUE    OUT   */  ulnParamProcess_ERROR,

    /* TRUE   FALSE   IN    */  ulnParamProcess_DEFAULT,
    /* TRUE   FALSE   OUT   */  ulnParamProcess_DEFAULT,

    /* TRUE   TRUE    IN    */  ulnParamProcess_DEFAULT,
    /* TRUE   TRUE    OUT   */  ulnParamProcess_ERROR
};

ACI_RC ulnBindDataWriteRow(ulnFnContext *aFnContext,
                           ulnPtContext *aPtContext,
                           ulnStmt      *aStmt,
                           acp_uint32_t  aRowNumber)  /* 0 베이스 */
{
    acp_uint32_t sParamNumber;
    acp_uint16_t sNumberOfParams;      /* 파라미터의 갯수 */

    ulnDescRec  *sDescRecIpd;
    ulnDescRec  *sDescRecApd;

    void             *sUserDataPtr;
    ulnIndLenPtrPair  sUserIndLenPair;
    ulnParamInOutType sInOutType;

    acp_uint8_t       sParamType = 0;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(aStmt);

    for (sParamNumber = aStmt->mProcessingParamNumber;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        aStmt->mProcessingParamNumber = sParamNumber;

        /*
         * APD, IPD 획득
         */
        sDescRecApd = ulnStmtGetApdRec(aStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NOT_BOUND);

        sDescRecIpd = ulnStmtGetIpdRec(aStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        /*
         * sParamType 결정
         */

        sParamType   = 0;
        sInOutType   = ulnDescRecGetParamInOut(sDescRecIpd);
        sUserDataPtr = ulnBindCalcUserDataAddr(sDescRecApd, aRowNumber);
        ulnBindCalcUserIndLenPair(sDescRecApd, aRowNumber, &sUserIndLenPair);

        if (ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecIpd->mMeta),
                                 ulnMetaGetCTYPE(&sDescRecApd->mMeta)) == ACP_TRUE)
        {
            sParamType |= ULN_PARAM_TYPE_MEM_BOUND_LOB;
            aStmt->mHasMemBoundLobParam = ACP_TRUE;
        }

        if (ulnDescRecIsDataAtExecParam(sDescRecApd, aRowNumber) == ACP_TRUE)
        {
            /*
             * BUG-16491 output param 의 경우 data at exec 라는 것의 의미가 없다.
             *           StrLenOrInd 포인터가 가리키는 곳의 값은 출력 전용이기 때문이다.
             */

            if (sInOutType != ULN_PARAM_INOUT_TYPE_OUTPUT)
            {
                sParamType |= ULN_PARAM_TYPE_DATA_AT_EXEC;
                aStmt->mHasDataAtExecParam |= ULN_HAS_DATA_AT_EXEC_PARAM;

                if ((sParamType & ULN_PARAM_TYPE_MEM_BOUND_LOB) == ULN_PARAM_TYPE_MEM_BOUND_LOB)
                {
                    aStmt->mHasDataAtExecParam |= ULN_HAS_LOB_DATA_AT_EXEC_PARAM;
                }
            }
        }

        if (sInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT)
        {
            sParamType |= ULN_PARAM_TYPE_OUT_PARAM;
        }

        /*
         * sParamType 에 따른 파라미터 처리 함수 호출
         */

        ACI_TEST(gUlnParamProcessorTable[sParamType](aFnContext,
                                                     aPtContext,
                                                     sDescRecApd,
                                                     sDescRecIpd,
                                                     sUserDataPtr,
                                                     &sUserIndLenPair,
                                                     aRowNumber) != ACI_SUCCESS);

        /*
         * OUTPUT PARAMETER 인 경우 마무리
         */

        if (sInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT ||
            sInOutType == ULN_PARAM_INOUT_TYPE_IN_OUT)
        {
            /*
             * Note : 만약, 사용자가 지정한 OUTPUT 혹은 in/out 파라미터에 대해서
             *        서버가 BINDDATA OUT 을 주지 않으면 StrLen_or_IndPtr 가
             *        가리키는 곳에 SQL_NULL_DATA 를 넣어주어야 한다.
             *        일일이 BINDDATA OUT 을 체크하려면 복잡하므로 일단
             *        BINDDATA IN 을 한 후에 미리 SQL_NULL_DATA 로 초기화 시켜둔다.
             *        좀 있다가 BINDDATA OUT 이 오면 해당 메모리에 길이를 세팅할 것이다.
             */
            ulnBindSetUserIndLenValue(&sUserIndLenPair, SQL_NULL_DATA);
        }

    } /* for (sParamNumber = ....) */

    // if (sParamNumber > sNumberOfParams)
    {
        aStmt->mProcessingParamNumber = 1;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnBindDataWriteRow");
    }

    ACI_EXCEPTION(LABEL_NOT_BOUND)
    {
        /*
         * BUGBUG: 해당 파라미터에 대한 바인드 정보가 없다.
         * 어떤 에러를 해야 할지 정의되지 않아서 general error 로 했다.
         */
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ==============================================
 *
 * BIND PARAM DATA OUT 처리
 *
 * ==============================================
 */

static ACI_RC ulnBindStoreLobLocator(ulnFnContext *aFnContext,
                                     ulnDescRec   *aDescRec,
                                     acp_uint16_t  aRowNumber,    /* 0 base */
                                     acp_uint8_t  *aData)
{
    ulnLob       *sLob = NULL;
    acp_uint64_t  sLobLocatorID;
    /* BUG-18945 */
    acp_uint64_t  sLobSize = 0;
    acp_uint8_t  *sSrc  = aData;
    acp_uint8_t  *sDest = (acp_uint8_t*)(&sLobLocatorID);
    /*
     * 사용자가 memory 혹은 file 로 바인드한 lob column or parameter
     * 전달되어 온 lob locator 는 IRD 내부에 할당해 둔 lob array 의 ulnLob 구조체에
     * 세팅된다.
     *
     * 이 함수에서는 ulnLob 을 초기화하고, locator 를 세팅한다.
     *
     * ulnExecSendLobParamData() 함수에서 사용자가 바인드한 데이터를
     * 서버로 전송한다.
     */
    sLob = ulnDescRecGetLobElement(aDescRec, aRowNumber);
    ACI_TEST_RAISE(sLob == NULL, LABEL_MEM_MANAGE_ERR);


    /*
     * Initialize lob
     */
    ACI_TEST_RAISE(ulnLobInitialize(sLob, ulnMetaGetMTYPE(&aDescRec->mMeta)) != ACI_SUCCESS,
                   LABEL_INVALID_LOB_TYPE);

    /*
     * locator 얻기
     */
    // proj_2160 cm_type removal
    CM_ENDIAN_ASSIGN8(sDest, sSrc);

    if(sLobLocatorID == MTD_LOCATOR_NULL )
    {
        sLobSize = 0;
    }
    else
    {
        sSrc  = aData + ACI_SIZEOF(acp_uint64_t);
        sDest = (acp_uint8_t*)(&sLobSize);

        /* PROJ-2047 Strengthening LOB - LOBCACHE */
        CM_ENDIAN_ASSIGN8(sDest, sSrc);
    }

    /*
     * locator 세팅
     */
    ACI_TEST_RAISE(sLob->mOp->mSetLocator(aFnContext, sLob, sLobLocatorID) != ACI_SUCCESS,
                   LABEL_INVALID_LOB_STATE);
    sLob->mSize = sLobSize;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LOB_TYPE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aDescRec->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnBindStoreLobLocator : IPD record is not lob type");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aDescRec->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnBindStoreLobLocator : Cannot find LOB locator");
    }

    ACI_EXCEPTION(LABEL_INVALID_LOB_STATE)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         aDescRec->mIndex,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnBindStoreLobLocator : ulnLob is not in a state where "
                         "LOB locator can be set");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnBindProcssOutParamData(ulnFnContext   *aFnContext,
                                        ulnDescRec     *aDescRecApd,
                                        ulnDescRec     *aDescRecIpd,
                                        acp_uint16_t    aRowNumber,
                                        acp_uint16_t    aParamNumber,
                                        acp_uint8_t    *aData)
{
    ulnColumn        *sColumn;
    ulnConvFunction  *sFilter         = NULL;
    ulnLengthPair     sLengthPair     = {ULN_vLEN(0), ULN_vLEN(0)};

    ulnAppBuffer      sAppBuffer;
    ulnIndLenPtrPair  sUserIndLenPair;

    ulnStmt          *sStmt = aFnContext->mHandle.mStmt;

#ifdef COMPILE_SHARDCLI
    ulsdLobLocator   *sInBindLocator = NULL;
#endif

    /*
     * 사용자 버퍼 주소 값 세팅
     */
    sAppBuffer.mCTYPE        = ulnMetaGetCTYPE(&aDescRecApd->mMeta);
    sAppBuffer.mBuffer       = (acp_uint8_t *)ulnBindCalcUserDataAddr(aDescRecApd,
                                                                      aRowNumber - 1);
    sAppBuffer.mBufferSize   = ulnMetaGetOctetLength(&aDescRecApd->mMeta);
    sAppBuffer.mColumnStatus = ULN_ROW_SUCCESS;

    ulnBindCalcUserIndLenPair(aDescRecApd, aRowNumber - 1, &sUserIndLenPair);

    /*
     * --------------------------------------
     * 임시 ulnColumn 에 수신한 데이터 복사
     * --------------------------------------
     */
    sColumn = aDescRecIpd->mOutParamBuffer;
    ACI_TEST_RAISE(sColumn == NULL, LABEL_MEM_MANAGE_ERR);

    sColumn->mGDState       = 0;
    sColumn->mGDPosition    = 0;
    sColumn->mRemainTextLen = 0;
    sColumn->mBuffer        = (acp_uint8_t*)(sColumn + 1);

    if ( sStmt->mShardStmtCxt.mIsMtDataFetch == ACP_TRUE )
    {
        /* BUG-45392 MT type 그대로 복사 */
        ACI_TEST( ulsdDataCopyFromMT(aFnContext,
                                     aData,
                                     sAppBuffer.mBuffer,
                                     sAppBuffer.mBufferSize,
                                     sColumn,
                                     sAppBuffer.mBufferSize )
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST(ulnDataBuildColumnFromMT(aFnContext,
                                          aData,
                                          sColumn) != ACI_SUCCESS);
    }

    /*
     * --------------------------------------
     * conversion 하여 사용자버퍼에 복사
     * --------------------------------------
     */

    /*
     * convert it!
     */
    if (sColumn->mDataLength == SQL_NULL_DATA)
    {
        if (ulnBindSetUserIndLenValue(&sUserIndLenPair, SQL_NULL_DATA) != ACI_SUCCESS)
        {
            /*
             * 22002 :
             *
             * NULL 이 컬럼에 fetch 되어 와서, SQL_NULL_DATA 를 사용자가 지정한
             * StrLen_or_IndPtr 에 써 줘야 하는데, 이녀석이 NULL 포인터이다.
             * 그럴때에 발생시켜 주는 에러.
             */
            ulnErrorExtended(aFnContext,
                             aRowNumber,
                             aParamNumber,
                             ulERR_ABORT_INDICATOR_REQUIRED_BUT_NOT_SUPPLIED_ERR);
        }
    }
    else
    {
        if ( sStmt->mShardStmtCxt.mIsMtDataFetch == ACP_FALSE )
        {
            /*
             * conversion 함수 구하기
             */
            sFilter = ulnConvGetFilter(aDescRecApd->mMeta.mCTYPE,
                                       (ulnMTypeID)sColumn->mMtype);
            ACI_TEST_RAISE(sFilter == NULL, LABEL_CONV_NOT_APPLICABLE);

#ifdef COMPILE_SHARDCLI
            /*
             * PROJ-2739 Client-side Sharding LOB
             *   For Locator InBind,
             *   아래 sFilter 함수에서 사용자 버퍼를 덮어쓰기 전에
             *   InBind locator 값을 보관해 둔다.
             */
            if ( ulsdTypeIsLocInBoundLob(sAppBuffer.mCTYPE,
                                         ulnDescRecGetParamInOut(aDescRecIpd))
                 == ACP_TRUE )
            {
                sInBindLocator = (ulsdLobLocator *)(*(acp_uint64_t *)sAppBuffer.mBuffer);
            }
#endif

            if (sFilter(aFnContext,
                        &sAppBuffer,
                        sColumn,
                        &sLengthPair,
                        aRowNumber) == ACI_SUCCESS)
            {
                ulnBindSetUserIndLenValue(&sUserIndLenPair, sLengthPair.mNeeded);
            }
            else
            {
                // BUGBUG : param status ptr 은?
                // sAppBuffer.mColumnStatus = ULN_ROW_ERROR;
            }

#ifdef COMPILE_SHARDCLI
            /* PROJ-2739 Client-side Sharding LOB */
            if ( ulsdTypeIsLocatorCType(sAppBuffer.mCTYPE) == ACP_TRUE )
            {
                ACI_TEST_RAISE( ulsdLobAddOutBindLocator(
                                        sStmt,
                                        aParamNumber,
                                        ulnDescRecGetParamInOut(aDescRecIpd),
                                        sInBindLocator,
                                        (acp_uint64_t *)sAppBuffer.mBuffer )
                                != ACI_SUCCESS, LABEL_MEM_MANAGE_ERR );

            }
            else
            {
                /* Nothing to do */
            }
#endif
        }
        else
        {
            /* Nothing to do */
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_CONV_NOT_APPLICABLE)
    {
        /* 07006 : Restricted data type attribute violation */
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aParamNumber,
                         ulERR_ABORT_INVALID_CONVERSION);
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber,
                         aParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnBindProcssOutParamData");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;

}

// bug-28259: ipc needs paramDataInResult
ACI_RC ulnCallbackParamDataInResult(cmiProtocolContext *aProtocolContext,
                                    cmiProtocol        *aProtocol,
                                    void               *aServiceSession,
                                    void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);
    ACP_UNUSED(aUserContext);

    return ACI_SUCCESS;
}

ACI_RC ulnCallbackParamDataOutList(cmiProtocolContext *aPtContext,
                                   cmiProtocol        *aProtocol,
                                   void               *aServiceSession,
                                   void               *aUserContext)
{
    ulnFnContext  *sFnContext = (ulnFnContext *)aUserContext;
    ulnDbc        *sDbc;
    ulnStmt       *sStmt      = sFnContext->mHandle.mStmt;
    acp_uint16_t   sNumberOfParams;
    // BUG-25315 [CodeSonar] 초기화되지 않는 변수 사용
    acp_uint16_t   i = 0;
    acp_uint32_t   sInOutType;
    acp_bool_t     sIsLob;
    acp_bool_t     sIsLocInBoundLob = ACP_FALSE;

    ulnDescRec    *sDescRecApd;
    ulnDescRec    *sDescRecIpd;

    acp_uint32_t        sStatementID;
    acp_uint32_t        sRowNumber;
    acp_uint32_t        sRowSize;
    acp_uint8_t        *sRow;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException); 

    CMI_RD4(aPtContext, &sStatementID);
    CMI_RD4(aPtContext, &sRowNumber);
    CMI_RD4(aPtContext, &sRowSize);

    ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sStmt) != ULN_OBJ_TYPE_STMT,
                   LABEL_OBJECT_TYPE_MISMATCH);

    ACI_TEST_RAISE(sStmt->mStatementID != sStatementID, LABEL_MEM_MANAGE_ERR);

    if (cmiGetLinkImpl(aPtContext) == CMI_LINK_IMPL_IPCDA)
    {
        /* PROJ-2616 메모리에 바로 접근하여 데이터를 읽도록 한다. */
        ACI_TEST_RAISE( cmiSplitReadIPCDA(aPtContext,
                                          sRowSize,
                                          &sRow,
                                          NULL)
                        != ACI_SUCCESS, cm_error );

    }
    else
    {
        ACI_TEST( ulnStmtAllocChunk( sStmt,
                                     sRowSize,
                                     &sRow)
                  != ACI_SUCCESS );

        ACI_TEST_RAISE( cmiSplitRead( aPtContext,
                                      sRowSize,
                                      sRow,
                                      sDbc->mConnTimeoutValue )
                        != ACI_SUCCESS, cm_error );
    }
    sRowSize = 0;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(sStmt);

    for( i = 1; i <= sNumberOfParams; i++ )
    {
        sDescRecApd = ulnStmtGetApdRec(sStmt, i);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_MEM_MANAGE_ERR);

        sDescRecIpd = ulnStmtGetIpdRec(sStmt, i);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        sInOutType = ulnDescRecGetParamInOut(sDescRecIpd);
        sIsLob     = ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecIpd->mMeta),
                                          ulnMetaGetCTYPE(&sDescRecApd->mMeta));
#ifdef COMPILE_SHARDCLI
        /*
         * PROJ-2739 Client-side Sharding LOB
         *   In -> Out으로 바뀌어 바인딩된 경우에도
         *   ulnBindProcssOutParamData에서 처리되어야 한다.
         */
        sIsLocInBoundLob = ( ulsdTypeIsLocInBoundLob(
                               ulnMetaGetCTYPE(&sDescRecApd->mMeta),
                               sInOutType) == ACP_TRUE ) &&
                           (sDescRecApd->mBindInfo.mInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT);
#endif

        if( (sInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT) ||
            (sInOutType == ULN_PARAM_INOUT_TYPE_IN_OUT) ||
            (sIsLob == ACP_TRUE) ||
            (sIsLocInBoundLob == ACP_TRUE) )
        {
            if (sIsLob == ACP_TRUE)
            {
                /*
                 * ------------------------------------
                 * LOB locator 수신 (mem bound lob)
                 * ------------------------------------
                 *
                 * BindInfo 를 전송하면서 할당한 lob array 에서 ulnLob 을 하나 가져와서
                 * Initialize 하고, locator 세팅.
                 */
                ACI_TEST(ulnBindStoreLobLocator(sFnContext,
                                                sDescRecIpd,
                                                sRowNumber - 1,
                                                sRow)
                         != ACI_SUCCESS);

                /* PROJ-2047 Strengthening LOB - LOBCACHE */
                sRow += ACI_SIZEOF(acp_uint64_t) +
                        ACI_SIZEOF(acp_uint64_t) +
                        ACI_SIZEOF(acp_uint8_t);
            }
            else
            {
                /*
                 * ------------------------------------------------------------
                 * LOB 아닌, 일반적인 output parameter : locator binding 포함
                 * ------------------------------------------------------------
                 */

                ACI_TEST(ulnBindProcssOutParamData(sFnContext,
                                                   sDescRecApd,
                                                   sDescRecIpd,
                                                   sRowNumber,
                                                   i,
                                                   sRow)
                         != ACI_SUCCESS);

                sRow += (sDescRecIpd->mOutParamBuffer->mMTLength);
            }
        }
    }

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_OBJECT_TYPE_MISMATCH)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnCallbackParamDataOutList : Object type does not match");
    }

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(sFnContext,
                         sRowNumber,
                         i,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnCallbackParamDataOutList");
    }
    ACI_EXCEPTION(cm_error)
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        ACI_TEST_RAISE( cmiSplitSkipRead( aPtContext,
                                          sRowSize,
                                          sDbc->mConnTimeoutValue )
                        != ACI_SUCCESS, cm_error );
    }

    /*
     * Note : ACI_SUCCESS 를 리턴하는 것은 버그가 아니다.
     *        cm 의 콜백함수가 ACI_FAILURE 를 리턴하면 communication error 로 취급되어 버리기
     *        때문이다.
     *
     *        어찌되었던 간에, Function Context 의 멤버인 mSqlReturn 에 함수 리턴값이
     *        저장되게 될 것이며, uln 의 cmi 매핑 함수인 ulnReadProtocol() 함수 안에서
     *        Function Context 의 mSqlReturn 을 체크해서 적절한 조치를 취하게 될 것이다.
     */
    return ACI_SUCCESS;
}

ACI_RC ulnCallbackParamDataInListResult(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *aProtocol,
                                        void               *aServiceSession,
                                        void               *aUserContext)
{
    ulnFnContext       *sFnContext  = (ulnFnContext *)aUserContext;
    ulnStmt            *sStmt       = sFnContext->mHandle.mStmt;
    ulnResult          *sResult     = NULL;
    acp_uint32_t        i;

    acp_uint32_t        sStatementID;
    acp_uint32_t        sFromRowNumber;
    acp_uint32_t        sToRowNumber;
    acp_uint16_t        sResultSetCount;
    acp_sint64_t        sAffectedRowCount;
    acp_sint64_t        sFetchedRowCount;
    ulnResultType       sResultType = ULN_RESULT_TYPE_UNKNOWN;
    acp_uint64_t        sSCN = 0;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD4(aProtocolContext, &sFromRowNumber);
    CMI_RD4(aProtocolContext, &sToRowNumber);
    CMI_RD2(aProtocolContext, &sResultSetCount);
    CMI_RD8(aProtocolContext, (acp_uint64_t *)&sAffectedRowCount);
    CMI_RD8(aProtocolContext, (acp_uint64_t *)&sFetchedRowCount);
    CMI_RD8(aProtocolContext, &sSCN);  /* PROJ-2733-Protocol */

    if (sSCN > 0)
    {
        ulsdUpdateSCN(sStmt->mParentDbc, &sSCN);  /* PROJ-2733-DistTxInfo */
    }

    /*
     * -----------------------
     * ResultSet Count 설정
     * -----------------------
     */

    ulnStmtSetResultSetCount(sStmt, sResultSetCount);

    /*
     * -----------------------
     * new result 생성, 매달기
     * -----------------------
     */

    sResult = ulnStmtGetNewResult(sStmt);
    ACI_TEST_RAISE(sResult == NULL, LABEL_MEM_MAN_ERR);

    sResult->mAffectedRowCount = sAffectedRowCount;
    sResult->mFetchedRowCount  = sFetchedRowCount;
    sResult->mFromRowNumber    = sFromRowNumber - 1;
    sResult->mToRowNumber      = sToRowNumber - 1;

    // BUG-38649
    if (sAffectedRowCount != ULN_DEFAULT_ROWCOUNT)
    {
        sResultType |= ULN_RESULT_TYPE_ROWCOUNT;
    }
    else
    {
        /* do nothing */
    }
    if ( sResultSetCount > 0 )
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

    for( i = sFromRowNumber; i <= sToRowNumber; i++ )
    {
        if (sAffectedRowCount == 0)
        {
            ulnStmtUpdateAttrParamsRowCountsValue(sStmt, SQL_NO_DATA);

            if (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON)
            {
                ulnStmtSetAttrParamStatusValue(sStmt, i - 1, 0);
            }
            else if (ulnStmtGetStatementType(sStmt) == ULN_STMT_UPDATE ||
                     ulnStmtGetStatementType(sStmt) == ULN_STMT_DELETE)
            {
                ulnStmtSetAttrParamStatusValue(sStmt, i-1, SQL_NO_DATA);
                if (ULN_FNCONTEXT_GET_RC((sFnContext)) != SQL_ERROR)
                {
                    ULN_FNCONTEXT_SET_RC((sFnContext), SQL_NO_DATA);

                    ULN_TRACE_LOG(sFnContext, ULN_TRACELOG_LOW, NULL, 0,
                            "%-18s| [rows from %"ACI_UINT32_FMT
                            " to %"ACI_UINT32_FMT"] SQL_NO_DATA",
                            "ulnCBDataInListRes",
                            sFromRowNumber, sToRowNumber);
                }
            }
        }
        else if (sAffectedRowCount > 0)
        {
            if (ulnStmtGetAttrParamsSetRowCounts(sStmt) == SQL_ROW_COUNTS_ON)
            {
                ulnStmtSetAttrParamStatusValue(sStmt, i - 1, ACP_MIN(sAffectedRowCount, SQL_USHRT_MAX - 1));
            }
            sStmt->mTotalAffectedRowCount += sAffectedRowCount;
        }
        else
        {
            /* do nothing */
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnCallbackParamDataInListResult");
    }

    ACI_EXCEPTION_END;

    /*
     * Note : callback 이므로 ACI_SUCCESS 를 리턴해야 한다.
     */

    return ACI_SUCCESS;
}

ACI_RC ulnParamProcess_INFOs(ulnFnContext *aFnContext,
                             ulnPtContext *aPtContext,
                             acp_uint32_t  aRowNumber ) /* 0 베이스 */
{
    ulnDbc       *sDbc               = NULL;
    ulnStmt      *sStmt              = aFnContext->mHandle.mStmt;
    acp_bool_t    sIsBindInfoChanged = ACP_FALSE;
    acp_bool_t    sIsWriteParamInfo  = ACP_FALSE;
    ulnMTypeID    sMTYPE;
    acp_uint32_t  sBufferSize;
    acp_uint32_t  sParamNumber;
    acp_uint32_t  sParamMaxSize = 0;
    acp_uint8_t   *sTemp;
    acp_uint16_t  sNumberOfParams;      /* 파라미터의 갯수 */
    ulnDescRec   *sDescRecIpd;
    ulnDescRec   *sDescRecApd;

    ulnParamInOutType sInOutType;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(sStmt);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

#ifdef COMPILE_SHARDCLI
    ulsdLobResetLocatorInBoundParam(sStmt);
#endif

    for (sParamNumber = 1;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NOT_BOUND);

        sDescRecIpd = ulnStmtGetIpdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        sInOutType = ulnDescRecGetParamInOut(sDescRecIpd);

        ACI_TEST_RAISE(ulnBindInfoBuild4Param(aFnContext,
                                              aRowNumber,
                                              sDescRecApd,
                                              sDescRecIpd,
                                              sInOutType,
                                              &sIsBindInfoChanged) != ACI_SUCCESS,
                       LABEL_BINDINFO_BUILD_ERR);

        /* BUG-35016*/
        sParamMaxSize += ulnTypeGetMaxMtSize(sDbc, &(sDescRecIpd->mMeta));

        if( sIsBindInfoChanged == ACP_TRUE )
        {
            sIsWriteParamInfo = ACP_TRUE;
        }
    }

    /* BUG-42096 BindInfo가 변경되지 않아도 ParamSetSize 변경을 체크해야 한다. */
    ACI_TEST( ulnStmtAllocChunk( sStmt,
                                 sParamMaxSize * ulnStmtGetAttrParamsetSize(sStmt),
                                 &sTemp )
              != ACI_SUCCESS );

    ACI_TEST_RAISE( sIsWriteParamInfo == ACP_FALSE, SKIP_WRITE_PARAM_INFO );

    for (sParamNumber = 1;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NOT_BOUND);

        sDescRecIpd = ulnStmtGetIpdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        sInOutType = sDescRecApd->mBindInfo.mInOutType;

        if (ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecIpd->mMeta),
                                 ulnMetaGetCTYPE(&sDescRecApd->mMeta)) == ACP_TRUE)
        {
            ACI_TEST_RAISE(ulnDescRecArrangeLobArray(sDescRecIpd,
                                                     ulnStmtGetAttrParamsetSize(sStmt))
                           != ACI_SUCCESS,
                           LABEL_NOT_ENOUGH_MEM);

            sStmt->mHasMemBoundLobParam = ACP_TRUE;
        }
        else
        {
            /*
             * output parameter 의 경우 param data out 이 수신될 때 수신된 data 를 임시로
             * 저장하기 위한 ulnColumn 을 위한 공간을 미리 할당해 둔다.
             */
            if (sInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT ||
                sInOutType == ULN_PARAM_INOUT_TYPE_IN_OUT)
            {
                sMTYPE = ulnBindInfoGetType(&sDescRecApd->mBindInfo);

                if (ulnTypeIsFixedMType(sMTYPE) == ACP_TRUE)
                {
                    sBufferSize = ulnTypeGetSizeOfFixedType(sMTYPE) + 1;
                }
                else
                {
                    sBufferSize = sDescRecApd->mBindInfo.mPrecision + 1;
                }

                ACI_TEST_RAISE(ulnDescRecPrepareOutParamBuffer(sDescRecIpd,
                                                               sBufferSize,
                                                               sMTYPE) != ACI_SUCCESS,
                               LABEL_NOT_ENOUGH_MEM2);
            }
        }
    }

    ACI_TEST(ulnWriteParamInfoSetListREQ(aFnContext,
                                         aPtContext,
                                         sNumberOfParams)
             != ACI_SUCCESS);

    ulnStmtChunkReset( sStmt );

    ACI_EXCEPTION_CONT( SKIP_WRITE_PARAM_INFO );

    ulnStmtSetBuildBindInfo( sStmt, ACP_FALSE );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnParamProcess_INFOs");
    }

    ACI_EXCEPTION(LABEL_NOT_BOUND)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }

    ACI_EXCEPTION(LABEL_BINDINFO_BUILD_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sDescRecApd->mIndex,
                         ulERR_ABORT_UNHANDLED_MT_TYPE_BINDINFO,
                         "ulnParamProcess_INFOs",
                         ulnMetaGetCTYPE(&sDescRecApd->mMeta),
                         ulnMetaGetMTYPE(&sDescRecIpd->mMeta));
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_ALLOC_ERROR,
                         "ulnParamProcess_INFOs");
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM2)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sDescRecApd->mIndex,
                         ulERR_FATAL_MEMORY_ALLOC_ERROR,
                         "ulnParamProcess_INFOs : output column buffer prep fail");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*********************************************************************
 * ulnParamProcess_IPCDA_DATAs
 *  : 사용자의 입력 데이터를 공유 메모리에 기록하는 함수.
 *    사용자의 데이터를 확인하여 NONE-ENDIAN 타입으로
 *    공유 메모리에 쓴다.
 *
 * aFnContext [IN] - ulnFnContext
 * aPtContext [IN] - ulnPtContext
 * aRowNumber [IN] - acp_uint32_t, Row 번호
 * aDataSize  [OUT] - acp_uint64_t, ROW를 구성하는 데이터의 전체 크기.
 *********************************************************************/
ACI_RC ulnParamProcess_IPCDA_DATAs(ulnFnContext       *aFnContext,
                                   ulnPtContext       *aPtContext,
                                   acp_uint32_t        aRowNumber,
                                   acp_uint64_t       *aDataSize) /* 0 베이스 */
{
    ulnStmt          *sStmt = aFnContext->mHandle.mStmt;
    acp_uint32_t      sParamNumber;
    acp_uint16_t      sNumberOfParams;      /* 파라미터의 갯수 */
    ulnDescRec       *sDescRecIpd;
    ulnDescRec       *sDescRecApd;
    acp_uint16_t      sUserDataLength = 0;
    acp_uint32_t      sOrgChunkCursor = 0;
    acp_sint32_t      sNullIndicator = -1;
    acp_sint32_t      sIndicator = 0;

    cmiProtocolContext * sCtx = &aPtContext->mCmiPtContext;

    acp_uint32_t      sInitPos = sCtx->mWriteBlock->mCursor;

    *aDataSize = 0;

    ACP_UNUSED(aPtContext);

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(sStmt);

    ACE_DASSERT( sStmt->mProcessingParamNumber == 1 );

    sOrgChunkCursor = sStmt->mChunk.mCursor;
    for (sParamNumber = 1;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NOT_BOUND);

        sDescRecIpd = ulnStmtGetIpdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        if (sDescRecApd->mIndicatorPtr != NULL)
        {
            sIndicator = *(sDescRecApd->mIndicatorPtr);
        }
        else
        {
            sIndicator = 0;
        }

        if ((sDescRecApd->mDataPtr != NULL) && (sIndicator != sNullIndicator))
        {
            /* For Indicator */
            CMI_WRITE_ALIGN8(sCtx);
            CMI_NE_WR4(sCtx, (acp_uint32_t*)&sIndicator);

            switch(sDescRecApd->mMeta.mCTYPE)
            {
                case ULN_CTYPE_CHAR: /* SQL_C_CHAR */
                    if (sDescRecApd->mDataPtr != NULL)
                    {
                        sUserDataLength = (acp_uint16_t)acpCStrLen((acp_char_t*)sDescRecApd->mDataPtr, ACP_UINT16_MAX);
                        sUserDataLength = (sUserDataLength<(acp_uint16_t)(sDescRecIpd->mMeta.mLength)?
                                           sUserDataLength:(acp_uint16_t)(sDescRecIpd->mMeta.mLength));
                    }
                    else
                    {
                        sUserDataLength = 0;
                    }
                    CMI_WRITE_ALIGN8(sCtx);
                    CMI_NE_WR2(sCtx, &sUserDataLength);

                    if (sUserDataLength > 0)
                    {
                        CMI_WCP(sCtx, sDescRecApd->mDataPtr, sUserDataLength);
                    }
                    else
                    {
                        /* do nothing. */
                    }
                    break;
                case ULN_CTYPE_USHORT: /* SQL_C_SHORT */
                case ULN_CTYPE_SSHORT: /* SQL_C_SSHORT */
                    CMI_WRITE_ALIGN8(sCtx);
                    CMI_NE_WR2(sCtx, sDescRecApd->mDataPtr);
                    break;
                case ULN_CTYPE_ULONG: /* SQL_C_LONG */
                case ULN_CTYPE_SLONG: /* SQL_C_SLONG */
                    CMI_WRITE_ALIGN8(sCtx);
                    CMI_NE_WR4(sCtx, sDescRecApd->mDataPtr);
                    break;
                case ULN_CTYPE_SBIGINT: /* SQL_C_SBIGINT */
                case ULN_CTYPE_DOUBLE: /* SQL_C_DOUBLE */
                    CMI_WRITE_ALIGN8(sCtx);
                    CMI_NE_WR8(sCtx, sDescRecApd->mDataPtr);
                    break;
                case ULN_CTYPE_TIMESTAMP:/* SQL_C_TYPE_TIMESTAMP */
                    CMI_WRITE_ALIGN8(sCtx);
                    CMI_WCP(sCtx, sDescRecApd->mDataPtr, ACI_SIZEOF(SQL_TIMESTAMP_STRUCT));
                    break;
                default:
                    ACI_RAISE(LABEL_INVALID_SIMPLEQUERY_TYPE);
                    break;
            }
        }
        else
        {
            CMI_WRITE_ALIGN8(sCtx);
            CMI_NE_WR4(sCtx, (acp_uint32_t*)&sNullIndicator);
        }
    }

    *aDataSize = ((acp_uint64_t)sCtx->mWriteBlock->mCursor - sInitPos);

    ulnStmtChunkIncRowCount( sStmt );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnParamProcess_IPCDA_DATAs");
    }

    ACI_EXCEPTION(LABEL_NOT_BOUND)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }
    ACI_EXCEPTION(LABEL_INVALID_SIMPLEQUERY_TYPE)
    {
        ulnErrorExtended(aFnContext,
                         1,
                         sParamNumber,
                         ulERR_ABORT_ACS_INVALID_DATA_TYPE);
    }

    ACI_EXCEPTION_END;

    /* proj_2160 cm_type removal */
    /* parameter 한개에서 에러가 나면, row 단위로 처리해야 하므로 */
    /* cursor를 이전 row 위치(혹은 파라미터 없는 상태)로 돌린다. */
    sStmt->mChunk.mCursor = sOrgChunkCursor;

    return ACI_FAILURE;
}

ACI_RC ulnParamProcess_DATAs(ulnFnContext *aFnContext,
                             ulnPtContext *aPtContext,
                             acp_uint32_t  aRowNumber ) /* 0 베이스 */
{
    ulnStmt               *sStmt = aFnContext->mHandle.mStmt;
    acp_uint32_t           sParamNumber;
    acp_uint16_t           sNumberOfParams;      /* 파라미터의 갯수 */
    ulnDescRec            *sDescRecIpd;
    ulnDescRec            *sDescRecApd;

    void                  *sUserDataPtr;
    ulnIndLenPtrPair       sUserIndLenPair;
    ulnParamInOutType      sInOutType;
    acp_sint32_t           sUserOctetLength;

    /*
     * PROJ-1697 : SQL_C_BIT는 내부적으로 클라이언트에서 변환(SQL_*)이
     * 이루어지기 때문에, 변환에 필요한 별도의 공간이 필요하다.
     */
    acp_uint8_t            sConversionBuffer;
    ulnCharSet             sCharSet;
    acp_sint32_t           sState = 0;
    acp_uint32_t           sOrgChunkCursor = 0;

    ACP_UNUSED(aPtContext);

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(sStmt);

    ACE_DASSERT( sStmt->mProcessingParamNumber == 1 );

    sOrgChunkCursor = sStmt->mChunk.mCursor;
    for (sParamNumber = 1;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecApd == NULL, LABEL_NOT_BOUND);

        sDescRecIpd = ulnStmtGetIpdRec(sStmt, sParamNumber);
        ACI_TEST_RAISE(sDescRecIpd == NULL, LABEL_MEM_MANAGE_ERR);

        sInOutType   = ulnDescRecGetParamInOut(sDescRecIpd);

        if ( (ulnTypeIsMemBoundLob(ulnMetaGetMTYPE(&sDescRecIpd->mMeta),
                                   ulnMetaGetCTYPE(&sDescRecApd->mMeta)) == ACP_TRUE) ||
             (sInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT)
#ifdef COMPILE_SHARDCLI
             /*
              * PROJ-2739 Client-side Sharding LOB
              * 사용자가 InBind 했지만 src, dest 노드가 달라서 OUT으로 바뀐 경우를 고려한다.
              * sDescRecApd->mBindInfo.mInOutType == OUTPUT 조건만 써도 될 것 같지만,
              * 혹시 모르니 모든 조건 명시
              */
             || 
             ((ulsdTypeIsLocInBoundLob(
                  ulnMetaGetCTYPE(&sDescRecApd->mMeta),
                  sInOutType) == ACP_TRUE) &&
              (sDescRecApd->mBindInfo.mInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT))
#endif
           )
        {
            continue;
        }

        sUserDataPtr = ulnBindCalcUserDataAddr(sDescRecApd, aRowNumber);
        ulnBindCalcUserIndLenPair(sDescRecApd, aRowNumber, &sUserIndLenPair);

        if (sUserIndLenPair.mLengthPtr == NULL)
        {
            if (ulnStmtGetAttrInputNTS(sStmt) == ACP_TRUE)
            {
                sUserOctetLength = SQL_NTS;
            }
            else
            {
                /* BUGBUG : ulvSLen -> acp_sint32_t */
                sUserOctetLength = (acp_sint32_t)ulnMetaGetOctetLength(&sDescRecApd->mMeta);
            }
        }
        else
        {
            sUserOctetLength = ulnBindGetUserIndLenValue(&sUserIndLenPair);
        }

        if ( ulsdIsInputMTData(sDescRecApd->mMeta.mOdbcType) == ACP_FALSE )
        {
            ACI_TEST( sDescRecApd->mBindInfo.mParamDataInBuildAnyFunc( aFnContext,
                                                                       sDescRecApd,
                                                                       sDescRecIpd,
                                                                       sUserDataPtr,
                                                                       sUserOctetLength,
                                                                       aRowNumber,
                                                                       &sConversionBuffer,
                                                                       &sCharSet)
                      != ACI_SUCCESS );
        }
        else
        {
            /*PROJ-2638 shard native linker */
            ACI_TEST( ulsdParamProcess_DATAs_ShardCore( aFnContext,
                                                        sStmt,
                                                        sDescRecApd,
                                                        sDescRecIpd,
                                                        sParamNumber,
                                                        aRowNumber,
                                                        sUserDataPtr )
                      != ACI_SUCCESS );
        }
    }

    ulnStmtChunkIncRowCount( sStmt );

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                         "ulnParamProcess_DATAs");
    }

    ACI_EXCEPTION(LABEL_NOT_BOUND)
    {
        ulnErrorExtended(aFnContext,
                         aRowNumber + 1,
                         sParamNumber,
                         ulERR_ABORT_PARAMETER_NOT_BOUND);
    }

    ACI_EXCEPTION_END;

    // proj_2160 cm_type removal
    // parameter 한개에서 에러가 나면, row 단위로 처리해야 하므로
    // cursor를 이전 row 위치(혹은 파라미터 없는 상태)로 돌린다.
    sStmt->mChunk.mCursor = sOrgChunkCursor;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return ACI_FAILURE;
}

ACI_RC ulnBindDataSetUserIndLenValue(ulnStmt      *aStmt,
                                     acp_uint32_t  aRowNumber)
{
    acp_uint32_t sParamNumber;
    acp_uint16_t sNumberOfParams;      /* 파라미터의 갯수 */

    ulnDescRec *sDescRecIpd;
    ulnDescRec *sDescRecApd;

    ulnIndLenPtrPair  sUserIndLenPair;
    ulnParamInOutType sInOutType;

    sNumberOfParams = ulnBindCalcNumberOfParamsToSend(aStmt);

    for (sParamNumber = aStmt->mProcessingParamNumber;
         sParamNumber <= sNumberOfParams;
         sParamNumber++)
    {
        sDescRecApd = ulnStmtGetApdRec(aStmt, sParamNumber);
        sDescRecIpd = ulnStmtGetIpdRec(aStmt, sParamNumber);

        ACI_TEST( sDescRecApd == NULL );        //BUG-28623 [CodeSonar]Null Pointer Dereference

        /*
         * BUG-28623 [CodeSonar]Null Pointer Dereference
         * sDescRecIpd도 NULL이 될 수 있음
         * 따라서 ACI_TEST로 검사
         */
        ACI_TEST( sDescRecIpd == NULL );

        sInOutType   = ulnDescRecGetParamInOut(sDescRecIpd);
        ulnBindCalcUserIndLenPair(sDescRecApd, aRowNumber, &sUserIndLenPair);

        if (sInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT ||
            sInOutType == ULN_PARAM_INOUT_TYPE_IN_OUT)
        {
            ulnBindSetUserIndLenValue(&sUserIndLenPair, SQL_NULL_DATA);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

