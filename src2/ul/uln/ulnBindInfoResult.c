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
#include <ulnBindParamDataIn.h>

typedef void (*ulnDescRecSetNameFunc)(ulnDescRec *aRecord, acp_char_t *aName, acp_size_t aNameLen);

/**
 * cm을 이용해 Name 속성(ColumnName, TableName 등)을 설정한다.
 *
 * @param[in] aProtocolContext    CM Protocol Context
 * @param[in] aDescRecIrd         IRD Record
 * @param[in] aSetNameFunc        Name 속성을 설정할 함수
 * @param[in] aFnContext          Function Context
 *
 * @return 성공하면 ACI_SUCCESS, 아니면 ACI_FAILURE
 */
static ACI_RC ulnBindSetDescRecName( cmiProtocolContext    *aProtocolContext,
                                     ulnDescRec            *aDescRecIrd,
                                     ulnDescRecSetNameFunc  aSetNameFunc,
                                     ulnFnContext          *aFnContext)
{

    ulnDbc       *sDbc;
    acp_char_t    sNameBefore[256]; // at least 51 bytes for DisplayName
    acp_uint8_t   sNameBeforeLen;
    ulnCharSet    sCharSet;
    acp_sint32_t  sState = 0;

    acp_char_t   *sNameAfter;
    acp_sint32_t  sNameAfterLen;


    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    CMI_RD1(aProtocolContext, sNameBeforeLen);
    CMI_RCP(aProtocolContext, sNameBefore, sNameBeforeLen );
    sNameBefore[sNameBeforeLen] = 0;

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    /* bug-37434 charset conversion for korean column name.
       DB charset column name을 client charset으로 변환.
       data flow: cmBlock -> sNameBefore -> sNameAfter */
    ACI_TEST(ulnCharSetConvert(&sCharSet,
                aFnContext,
                NULL,
                (const mtlModule*)sDbc->mCharsetLangModule,
                sDbc->mClientCharsetLangModule,
                (void*)sNameBefore,
                (acp_sint32_t)sNameBeforeLen,
                CONV_DATA_IN)
            != ACI_SUCCESS);


    sNameAfter = (acp_char_t*)ulnCharSetGetConvertedText(&sCharSet);
    sNameAfterLen = ulnCharSetGetConvertedTextLen(&sCharSet);

    aSetNameFunc(aDescRecIrd, sNameAfter, sNameAfterLen);

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

ACI_RC ulnCallbackColumnInfoGetResult(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aServiceSession,
                                      void               *aUserContext)
{
    ulnDbc        *sDbc;
    ulnStmt       *sStmt;
    ulnDescRec    *sDescRecIrd;
    ulnFnContext  *sFnContext;
    ulnMTypeID     sMTYPE;
    acp_uint16_t   sOrgCursor;

    acp_uint32_t   sStatementID;
    acp_uint16_t   sResultSetID;
    acp_uint16_t   sColumnNumber;
    acp_uint32_t   sDataType;
    acp_uint32_t   sLanguage;
    acp_uint8_t    sArguments;
    acp_uint32_t   sPrecision;
    acp_uint32_t   sScale;
    acp_uint8_t    sFlag;
    acp_uint8_t   sNameLen;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    sFnContext = (ulnFnContext *)aUserContext;
    sStmt      = sFnContext->mHandle.mStmt;

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sResultSetID);
    CMI_RD2(aProtocolContext, &sColumnNumber);
    CMI_RD4(aProtocolContext, &sDataType);
    CMI_RD4(aProtocolContext, &sLanguage);
    CMI_RD1(aProtocolContext, sArguments);
    CMI_RD4(aProtocolContext, &sPrecision);
    CMI_RD4(aProtocolContext, &sScale);
    CMI_RD1(aProtocolContext, sFlag);

    ACP_UNUSED(sArguments);

    sOrgCursor = aProtocolContext->mReadBlock->mCursor;

    ACI_TEST(ULN_OBJ_GET_TYPE(sStmt) != ULN_OBJ_TYPE_STMT);

    ACI_TEST(sStmt->mStatementID != sStatementID);
    ACI_TEST(sStmt->mCurrentResultSetID != sResultSetID);

    /*
     * IRD Record 준비
     */
    ACI_TEST_RAISE(ulnBindArrangeNewDescRec(sStmt->mAttrIrd,
                                            sColumnNumber,
                                            &sDescRecIrd) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    /*
     * 타입 정보 세팅
     */
    sMTYPE = ulnTypeMap_MTD_MTYPE(sDataType);

    ACI_TEST_RAISE(sMTYPE == ULN_MTYPE_MAX, LABEL_UNKNOWN_TYPEID);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        ACI_TEST_RAISE(((sMTYPE==ULN_MTYPE_BLOB)||(sMTYPE==ULN_MTYPE_CLOB)),
                       LABEL_UNKNOWN_TYPEID);
    }
    else
    {
        /*do nothing.*/
    }

    ulnMetaBuild4Ird(sDbc,
                     &sDescRecIrd->mMeta,
                     sMTYPE,
                     sPrecision,
                     sScale,
                     sFlag);

    ulnMetaSetLanguage(&sDescRecIrd->mMeta, sLanguage);

    ulnDescRecSetSearchable(sDescRecIrd, ulnTypeGetInfoSearchable(sMTYPE));
    ulnDescRecSetLiteralPrefix(sDescRecIrd, ulnTypeGetInfoLiteralPrefix(sMTYPE));
    ulnDescRecSetLiteralSuffix(sDescRecIrd, ulnTypeGetInfoLiteralSuffix(sMTYPE));
    ulnDescRecSetTypeName(sDescRecIrd, ulnTypeGetInfoName(sMTYPE));

    /* DisplayName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetDisplayName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /* ColumnName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetColumnName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /* BaseColumnName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetBaseColumnName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /* TableName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetTableName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /* BaseTableName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetBaseTableName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /* SchemaName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetSchemaName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /* CatalogName */
    ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                           sDescRecIrd,
                                           ulnDescRecSetCatalogName,
                                           sFnContext )
                    != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

    /*
     * Display Size 세팅
     */
    ulnDescRecSetDisplaySize(sDescRecIrd, ulnTypeGetDisplaySize(sMTYPE, &sDescRecIrd->mMeta));

    /* PROJ-2616 */
    /* Get Max-Byte-Size in qciBindColumn. */
    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        CMI_RD4(aProtocolContext, &sDescRecIrd->mMaxByteSize);
    }
    else
    {
        /* do nothing */
    }

    /*
     * IRD record 를 IRD 에 추가한다.
     */
    ACI_TEST_RAISE(ulnDescAddDescRec(sStmt->mAttrIrd, sDescRecIrd) != ACI_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_UNKNOWN_TYPEID)
    {
        ulnError(sFnContext, ulERR_ABORT_UNHANDLED_MT_TYPE, sMTYPE);
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnCallbackColumnInfoGetResult");
    }

    ACI_EXCEPTION_END;

    aProtocolContext->mReadBlock->mCursor = sOrgCursor;

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

    CMI_RD1(aProtocolContext, sNameLen);
    CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

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

ACI_RC ulnCallbackColumnInfoGetListResult(cmiProtocolContext *aProtocolContext,
                                          cmiProtocol        *aProtocol,
                                          void               *aServiceSession,
                                          void               *aUserContext)
{
    ulnDbc        *sDbc;
    ulnStmt       *sStmt;
    ulnDescRec    *sDescRecIrd;
    ulnFnContext  *sFnContext;
    acp_uint32_t   i = 0;
    ulnMTypeID     sMTYPE;
    acp_uint16_t   sRemainSize;
    acp_uint16_t   sOrgCursor;

    acp_uint32_t   sStatementID;
    acp_uint16_t   sResultSetID;
    acp_uint16_t   sColumnCount;
    acp_uint32_t   sDataType;
    acp_uint32_t   sLanguage;
    acp_uint8_t    sArguments;
    acp_uint32_t   sPrecision;
    acp_uint32_t   sScale;
    acp_uint8_t    sFlag;
    acp_uint8_t   sNameLen;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    sFnContext = (ulnFnContext *)aUserContext;
    sStmt      = sFnContext->mHandle.mStmt;

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sResultSetID);
    CMI_RD2(aProtocolContext, &sColumnCount);

    ACI_TEST(ULN_OBJ_GET_TYPE(sStmt) != ULN_OBJ_TYPE_STMT);

    ACI_TEST(sStmt->mStatementID != sStatementID);
    ACI_TEST(sStmt->mCurrentResultSetID != sResultSetID);

    /* PROJ-1789 Updatable Scrollable Cursor */
    if (sStmt->mParentStmt != SQL_NULL_HSTMT) /* is RowsetStmt ? */
    {
        ACI_TEST_RAISE(ulnBindArrangeNewDescRec(sStmt->mAttrIrd, 0, &sDescRecIrd)
                       != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        ACI_TEST_RAISE(ulnDescAddDescRec(sStmt->mAttrIrd, sDescRecIrd)
                       != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);
    }

    for( i = 1; i <= sColumnCount; i++ )
    {
        sRemainSize = aProtocolContext->mReadBlock->mDataSize -
                      aProtocolContext->mReadBlock->mCursor;

        /* IPCDA의 데이터는 cmiProtocolContext안의 mReadBlock안에서 전송됩니다.
         * 이 버퍼는 Split_Read 되지 않으며, 버퍼의 최대 크기내에서 한 번에
         * WRITE 됩니다. 따라서, cmiRecvNext는 호출하지 않습니다.
         */
        ACI_TEST_RAISE(cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA,
                       SkipRecvNext);

        if( sRemainSize == 0 )
        {
            ACI_TEST_RAISE( cmiRecvNext( aProtocolContext,
                                         sDbc->mConnTimeoutValue )
                            != ACI_SUCCESS, cm_error );
        }

        ACI_EXCEPTION_CONT(SkipRecvNext);
        sOrgCursor = aProtocolContext->mReadBlock->mCursor;

        CMI_RD4(aProtocolContext, &sDataType);
        CMI_RD4(aProtocolContext, &sLanguage);
        CMI_RD1(aProtocolContext, sArguments);
        CMI_RD4(aProtocolContext, &sPrecision);
        CMI_RD4(aProtocolContext, &sScale);
        CMI_RD1(aProtocolContext, sFlag);

        ACP_UNUSED(sArguments);

        /*
         * IRD Record 준비
         */
        ACI_TEST_RAISE(ulnBindArrangeNewDescRec(sStmt->mAttrIrd,
                                                i,
                                                &sDescRecIrd) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);

        /* DisplayName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetDisplayName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /* ColumnName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetColumnName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /* BaseColumnName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetBaseColumnName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /* TableName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetTableName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /* BaseTableName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetBaseTableName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /* SchemaName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetSchemaName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /* CatalogName */
        ACI_TEST_RAISE( ulnBindSetDescRecName( aProtocolContext,
                                               sDescRecIrd,
                                               ulnDescRecSetCatalogName,
                                               sFnContext )
                        != ACI_SUCCESS, LABEL_NOT_ENOUGH_MEM);

        /*
         * 타입 정보 세팅
         */
        sMTYPE = ulnTypeMap_MTD_MTYPE(sDataType);

        ACI_TEST_RAISE(sMTYPE == ULN_MTYPE_MAX, LABEL_UNKNOWN_TYPEID);

        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            ACI_TEST_RAISE(((sMTYPE==ULN_MTYPE_BLOB)||(sMTYPE==ULN_MTYPE_CLOB)), LABEL_UNKNOWN_TYPEID);
        }

        ulnMetaBuild4Ird(sDbc,
                         &sDescRecIrd->mMeta,
                         sMTYPE,
                         sPrecision,
                         sScale,
                         sFlag);

        ulnMetaSetLanguage(&sDescRecIrd->mMeta, sLanguage);

        ulnDescRecSetSearchable(sDescRecIrd, ulnTypeGetInfoSearchable(sMTYPE));
        ulnDescRecSetLiteralPrefix(sDescRecIrd, ulnTypeGetInfoLiteralPrefix(sMTYPE));
        ulnDescRecSetLiteralSuffix(sDescRecIrd, ulnTypeGetInfoLiteralSuffix(sMTYPE));
        ulnDescRecSetTypeName(sDescRecIrd, ulnTypeGetInfoName(sMTYPE));

        /*
         * Display Size 세팅
         */
        ulnDescRecSetDisplaySize(sDescRecIrd, ulnTypeGetDisplaySize(sMTYPE, &sDescRecIrd->mMeta));

        /* PROJ-2616 */
        /* Get Max-Byte-Size in qciBindColumn. */
        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            CMI_RD4(aProtocolContext, &sDescRecIrd->mMaxByteSize);
        }
        else
        {
            /* do nothing */
        }


        /*
         * IRD record 를 IRD 에 추가한다.
         */
        ACI_TEST_RAISE(ulnDescAddDescRec(sStmt->mAttrIrd, sDescRecIrd) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_UNKNOWN_TYPEID)
    {
        ulnError(sFnContext, ulERR_ABORT_UNHANDLED_MT_TYPE, sMTYPE);
    }
    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnCallbackColumnInfoGetResult");
    }
    ACI_EXCEPTION(cm_error)
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    if( i == 0 )
    {
        i = 1;
    }
    else
    {
        aProtocolContext->mReadBlock->mCursor = sOrgCursor;
    }

    for( ; i <= sColumnCount; i++ )
    {
        sRemainSize = aProtocolContext->mReadBlock->mDataSize -
                      aProtocolContext->mReadBlock->mCursor;

        /* PROJ-2616 */
        if(cmiGetLinkImpl(aProtocolContext) != CMI_LINK_IMPL_IPCDA)
        {
            if( sRemainSize == 0 )
            {
                ACI_TEST_RAISE( cmiRecvNext( aProtocolContext,
                                             sDbc->mConnTimeoutValue )
                                != ACI_SUCCESS, cm_error );
            }
        }

        CMI_SKIP_READ_BLOCK(aProtocolContext, 18);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        CMI_RD1(aProtocolContext, sNameLen);
        CMI_SKIP_READ_BLOCK(aProtocolContext, sNameLen);

        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            CMI_SKIP_READ_BLOCK(aProtocolContext, 4);
        }
        else
        {
            /* do nothing */
        }
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

ACI_RC ulnCallbackParamInfoGetResult(cmiProtocolContext *aProtocolContext,
                                     cmiProtocol        *aProtocol,
                                     void               *aServiceSession,
                                     void               *aUserContext)
{
    ulnFnContext  *sFnContext = (ulnFnContext *)aUserContext;
    ulnStmt       *sStmt      = sFnContext->mHandle.mStmt;

    ulnDesc       *sDescIpd    = NULL;
    ulnDescRec    *sDescRecIpd = NULL;

    ulnMTypeID     sMTYPE;

    acp_uint32_t        sStatementID;
    acp_uint16_t        sParamNumber;
    acp_uint32_t        sDataType;
    acp_uint32_t        sLanguage;
    acp_uint8_t         sArguments;
    acp_sint32_t        sPrecision;
    acp_sint32_t        sScale;
    acp_uint8_t         sNullableFlag;
    acp_uint8_t         sCmInOutType;
    ulnParamInOutType   sUlnInOutType;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    ACI_TEST(ULN_OBJ_GET_TYPE(sStmt) != ULN_OBJ_TYPE_STMT);

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sParamNumber);
    CMI_RD4(aProtocolContext, &sDataType);
    CMI_RD4(aProtocolContext, &sLanguage);
    CMI_RD1(aProtocolContext, sArguments);
    CMI_RD4(aProtocolContext, (acp_uint32_t*)&sPrecision);
    CMI_RD4(aProtocolContext, (acp_uint32_t*)&sScale);
    CMI_RD1(aProtocolContext, sCmInOutType);
    CMI_RD1(aProtocolContext, sNullableFlag);

    ACP_UNUSED(sArguments);
    ACP_UNUSED(sNullableFlag);

    ACI_TEST(sStmt->mStatementID != sStatementID);

    /*
     * IPD Record 준비
     */
    sDescIpd    = ulnStmtGetIpd(sStmt);
    ACI_TEST_RAISE(sDescIpd == NULL, LABEL_MEM_MANAGE_ERR);

    sDescRecIpd = ulnStmtGetIpdRec(sStmt, sParamNumber);

    if (sDescRecIpd == NULL)
    {
        /*
         * 사용자가 바인드하지 않은 파라미터에 대해서만 IPD record 생성 및 정보 세팅
         */
        ACI_TEST_RAISE(ulnDescRecCreate(sDescIpd,
                                        &sDescRecIpd,
                                        sParamNumber) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);

        ulnDescRecInitialize(sDescIpd, sDescRecIpd, sParamNumber);
        ulnDescRecInitOutParamBuffer(sDescRecIpd);
        ulnDescRecInitLobArray(sDescRecIpd);
        ulnBindInfoInitialize(&sDescRecIpd->mBindInfo);

        /*
         * 타입 정보
         */
        sMTYPE = ulnTypeMap_MTD_MTYPE(sDataType);
        ACI_TEST_RAISE(sMTYPE == ULN_MTYPE_MAX, LABEL_UNKNOWN_TYPEID);

        if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
        {
            ACI_TEST_RAISE(((sMTYPE==ULN_MTYPE_BLOB)||(sMTYPE==ULN_MTYPE_CLOB)), LABEL_UNKNOWN_TYPEID);
        }

        /*
         * ulnMeta 초기화 및 빌드
         */
        ulnMetaInitialize(&sDescRecIpd->mMeta);

        /*
         * BUGBUG : 아래처럼, columnsize 에 mPrecision 을, decimal digits 에 mScale 을
         *          넣으면 큰 문제가 없이 대부분 적용되겠지만,
         *          DATE 관련일 때 의미가 바뀌는 일이 있다.
         *
         *          그런데, 실제, DATE 관련 타입은 precision, scale 개념이 알티베이스에는
         *          없다.
         *
         *          일단 이대로 가자.
         */
        ulnMetaBuild4IpdByMeta(&sDescRecIpd->mMeta,
                               sMTYPE,
                               ulnTypeMap_MTYPE_SQL(sMTYPE),
                               sPrecision,
                               sScale);

        ulnMetaSetLanguage(&sDescRecIpd->mMeta, sLanguage);

        ulnDescRecSetSearchable(sDescRecIpd, ulnTypeGetInfoSearchable(sMTYPE));
        ulnDescRecSetLiteralPrefix(sDescRecIpd, ulnTypeGetInfoLiteralPrefix(sMTYPE));
        ulnDescRecSetLiteralSuffix(sDescRecIpd, ulnTypeGetInfoLiteralSuffix(sMTYPE));
        ulnDescRecSetTypeName(sDescRecIpd, ulnTypeGetInfoName(sMTYPE));

        /* BUG-42521 */
        sUlnInOutType = ulnBindInfoConvUlnParamInOutType(sCmInOutType);
        (void) ulnDescRecSetParamInOut(sDescRecIpd, sUlnInOutType);

        /*
         * Display Size 세팅
         */
        ulnDescRecSetDisplaySize(sDescRecIpd, ulnTypeGetDisplaySize(sMTYPE, &sDescRecIpd->mMeta));

        /*
         * IPD record 를 IPD 에 매달아준다.
         */
        ACI_TEST_RAISE(ulnDescAddDescRec(sDescIpd, sDescRecIpd) != ACI_SUCCESS,
                       LABEL_NOT_ENOUGH_MEM);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        /*
         * 할당했던 메모리 해제는 생각하지 말자. 어차피 메모리 꼬였다.
         */
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "ulnCallbackParamInfoGetResult");
    }

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnCallbackParamInfoGetResult");
    }

    ACI_EXCEPTION(LABEL_UNKNOWN_TYPEID)
    {
        ulnError(sFnContext, ulERR_ABORT_UNHANDLED_MT_TYPE, sDataType);
    }

    ACI_EXCEPTION_END;

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

ACI_RC ulnCallbackParamInfoSetListResult(cmiProtocolContext *aProtocolContext,
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
