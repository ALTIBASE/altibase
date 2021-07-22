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

static ACI_RC ulnGetDescHeaderField(ulnFnContext *aFnContext,
                                    acp_sint16_t  aFieldIdentifier,
                                    void         *aValuePtr,
                                    acp_sint32_t  aBufferLength,
                                    acp_sint32_t *aStringLengthptr)
{
    ulnDesc          *sDesc  = aFnContext->mHandle.mDesc;
    ulnDescAllocType  sAllocType;

    ACP_UNUSED(aBufferLength);
    ACP_UNUSED(aStringLengthptr);

    switch(aFieldIdentifier)
    {
        case SQL_DESC_BIND_TYPE:
            *(acp_uint32_t *)aValuePtr = ulnDescGetBindType(sDesc);
            break;

        case SQL_DESC_ALLOC_TYPE:
            sAllocType = ulnDescGetAllocType(sDesc);
            switch (sAllocType)
            {
                case ULN_DESC_ALLOCTYPE_IMPLICIT:
                    *(acp_sint16_t *)aValuePtr = SQL_DESC_ALLOC_AUTO;
                    break;

                case ULN_DESC_ALLOCTYPE_EXPLICIT:
                    *(acp_sint16_t *)aValuePtr = SQL_DESC_ALLOC_USER;
                    break;

                default:
                    ACE_ASSERT(0);
                    break;
            }
            break;

        case SQL_DESC_ARRAY_SIZE:
            /*
             * Note : When the FieldIdentifier parameter has one of the following values,
             *        a 64-bit value is returned in *ValuePtr:
             *
             *          SQL_DESC_ARRAY_SIZE
             *
             * 그러나 누가 20억개 이상의 원소가 있는 배열을 array 로 바인드 하겠는가.
             * 매핑만 시키고 내부에서는 단순히 acp_uint32_t 를 사용하도록 한다.
             *
             * ulnStmtSetAttrRowArraySize() 함수의 주석 참조
             */
            *(ulvULen *)aValuePtr = (ulvULen)ulnDescGetArraySize(sDesc);
            break;

        case SQL_DESC_ARRAY_STATUS_PTR:
            *(acp_uint16_t **)aValuePtr = ulnDescGetArrayStatusPtr(sDesc);
            break;

        case SQL_DESC_BIND_OFFSET_PTR:
            *(ulvULen **)aValuePtr = ulnDescGetBindOffsetPtr(sDesc);
            break;

        case SQL_DESC_COUNT:
            *(acp_uint16_t *)aValuePtr = ulnDescGetHighestBoundIndex(sDesc);
            break;

        case SQL_DESC_ROWS_PROCESSED_PTR:
            *(ulvULen **)aValuePtr = ulnDescGetRowsProcessedPtr(sDesc);
            break;

        default:
            ACI_RAISE(LABEL_MEM_MAN_ERR);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnGetDescHeaderField");
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulnGetDescRecordField(ulnFnContext *aFnContext,
                                    acp_uint16_t  aRecordNumber,
                                    acp_sint16_t  aFieldIdentifier,
                                    void         *aValuePtr,
                                    acp_sint32_t  aBufferLength,
                                    acp_sint32_t *aStringLengthPtr,
                                    acp_bool_t    aLongDataCompat)
{
    ulnDesc    *sDesc    = aFnContext->mHandle.mDesc;
    ulnDescRec *sDescRec = ulnDescGetDescRec(sDesc, aRecordNumber);

    acp_uint32_t  sLength;
    acp_sint16_t  sSQLTYPE;
    acp_char_t   *sSourceBuffer = NULL;
    acp_sint16_t  sStringLength;

    ulnParamInOutType sParamType;

    /*
     * BUGBUG : ulnColAttribute 와 중복된 코드가 상당수 존재한다.
     *          물론 SQLColAttribute() 함수의 경우에는 모든 값을 32비트 integer 로
     *          캐스팅 해서 사용자에게 돌려주어야 한다는 차이가 있지만,
     *          적어도 공통된 루틴을 통해서 값을 가져온 다음에
     *          그 값을 캐스팅하도록 해야 할 것이다.
     */

    ACI_TEST_RAISE(sDescRec == NULL, LABEL_INVALID_DESC_INDEX);

    switch(aFieldIdentifier)
    {
        case SQL_DESC_DATA_PTR:
            *(SQLPOINTER *)aValuePtr = ulnDescRecGetDataPtr(sDescRec);
            break;

        case SQL_DESC_CONCISE_TYPE:
            /*
             * Note : 이처럼 ulnTypes.cpp 에서 근본적으로 수정하지 않고,
             *        사용자에게 타입을 돌려주는 함수마다 그때그때 long type 을 매핑하는
             *        함수를 호출하는 것은 버그의 소지도 있고 위험한 짓이지만,
             *        ulnTypes.cpp 에 function context, dbc, stmt 등의 지저분한 다른
             *        것들을 받는 함수를 만들고 싶지 않아서 굳이 이와같이 했다.
             */

            sSQLTYPE = ulnMetaGetOdbcConciseType(&sDescRec->mMeta);
            sSQLTYPE = ulnTypeMap_LOB_SQLTYPE(sSQLTYPE, aLongDataCompat);

            *(acp_sint16_t *)aValuePtr = sSQLTYPE;
            break;

        case SQL_DESC_TYPE:
            sSQLTYPE = ulnMetaGetOdbcType(&sDescRec->mMeta);
            /* PROJ-2638 shard native linker */
            if ( sSQLTYPE >= ULSD_INPUT_RAW_MTYPE_NULL )
            {
                sSQLTYPE = ulnTypeMap_MTYPE_SQL( sSQLTYPE - ULSD_INPUT_RAW_MTYPE_NULL );
            }
            else
            {
                /* Do Nothing. */
            }

            sSQLTYPE = ulnTypeMap_LOB_SQLTYPE(sSQLTYPE, aLongDataCompat);
            *(acp_sint16_t *)aValuePtr = sSQLTYPE;
            break;

        case SQL_DESC_DATETIME_INTERVAL_CODE:
            *(acp_sint16_t *)aValuePtr = ulnMetaGetOdbcDatetimeIntCode(&sDescRec->mMeta);
            break;

        case SQL_DESC_DATETIME_INTERVAL_PRECISION:
            *(acp_sint32_t *)aValuePtr = ulnMetaGetOdbcDatetimeIntPrecision(&sDescRec->mMeta);
            break;

        case SQL_DESC_DISPLAY_SIZE:
            *(acp_sint32_t *)aValuePtr = ulnDescRecGetDisplaySize(sDescRec);
            break;

        case SQL_DESC_INDICATOR_PTR:
            *(ulvSLen **)aValuePtr = sDescRec->mIndicatorPtr;
            break;

        case SQL_DESC_LABEL:
        case SQL_DESC_NAME:
            /*
             * BUG-28980 [CodeSonar]Ignored Return Value
             */
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);

            /* BUGBUG (BUG-33625) */
            sSourceBuffer = ulnDescRecGetDisplayName(sDescRec);

            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_LENGTH:
            *(acp_sint32_t *)aValuePtr = ulnMetaGetOdbcLength(&sDescRec->mMeta);
            break;

        case SQL_DESC_LITERAL_PREFIX:
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);    //BUG-28623 [CodeSonar]Ignored Return Value

            sSourceBuffer = ulnDescRecGetLiteralPrefix(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_LITERAL_SUFFIX:
            /*
             * BUG-28980 [CodeSonar]Ignored Return Value
             */
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);
            sSourceBuffer = ulnDescRecGetLiteralSuffix(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_TYPE_NAME:
            /*
             * BUG-28980 [CodeSonar]Ignored Return Value
             */
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);
            sSourceBuffer = ulnDescRecGetTypeName(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_NULLABLE:
            *(acp_sint16_t *)aValuePtr = ulnMetaGetNullable(&sDescRec->mMeta);
            break;

        case SQL_DESC_OCTET_LENGTH:
            *(acp_sint32_t *)aValuePtr = ulnMetaGetOctetLength(&sDescRec->mMeta);
            break;

        case SQL_DESC_OCTET_LENGTH_PTR:
            *(ulvSLen **)aValuePtr = sDescRec->mOctetLengthPtr;
            break;

        case SQL_DESC_PARAMETER_TYPE:
            sParamType = ulnDescRecGetParamInOut(sDescRec);
            switch (sParamType)
            {
                // fix BUG-19411
                case ULN_PARAM_INOUT_TYPE_INIT:
                    *(acp_sint16_t *)aValuePtr = SQL_PARAM_TYPE_UNKNOWN;
                    break;

                case ULN_PARAM_INOUT_TYPE_MAX:
                case ULN_PARAM_INOUT_TYPE_INPUT:
                    *(acp_sint16_t *)aValuePtr = SQL_PARAM_INPUT;
                    break;

                case ULN_PARAM_INOUT_TYPE_IN_OUT:
                    *(acp_sint16_t *)aValuePtr = SQL_PARAM_INPUT_OUTPUT; /* BUG-42521 */
                    break;

                case ULN_PARAM_INOUT_TYPE_OUTPUT:
                    *(acp_sint16_t *)aValuePtr = SQL_PARAM_OUTPUT;
                    break;
            }

            break;

        case SQL_DESC_PRECISION:
            *(acp_sint16_t *)aValuePtr = ulnMetaGetPrecision(&sDescRec->mMeta);
            break;

        case SQL_DESC_SCALE:
            *(acp_sint16_t *)aValuePtr = ulnMetaGetScale(&sDescRec->mMeta);
            break;

        case SQL_DESC_SEARCHABLE:
            *(acp_sint16_t *)aValuePtr = ulnDescRecGetSearchable(sDescRec);
            break;

        case SQL_DESC_UNNAMED:
            *(acp_sint16_t *)aValuePtr = ulnDescRecGetUnnamed(sDescRec);
            break;

        case SQL_DESC_UNSIGNED:
            *(acp_sint16_t *)aValuePtr = ulnDescRecGetUnsigned(sDescRec);
            break;

            //BUG-18607
        case SQL_DESC_FIXED_PREC_SCALE:
            *(acp_sint16_t *)aValuePtr = ulnMetaGetFixedPrecScale(&sDescRec->mMeta);
            break;

        /* PROJ-1789 Updatable Scrollable Cursor */

        case SQL_DESC_UPDATABLE:
            /* BUG-23997 */
            *(acp_sint16_t *)aValuePtr = SQL_ATTR_READWRITE_UNKNOWN;
            break;

        case SQL_DESC_BASE_COLUMN_NAME:
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);

            sSourceBuffer = ulnDescRecGetBaseColumnName(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;

        case SQL_DESC_TABLE_NAME:
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);

            sSourceBuffer = ulnDescRecGetTableName(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_BASE_TABLE_NAME:
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);

            sSourceBuffer = ulnDescRecGetBaseTableName(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_SCHEMA_NAME:
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);

            sSourceBuffer = ulnDescRecGetSchemaName(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_CATALOG_NAME:
            ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);

            sSourceBuffer = ulnDescRecGetCatalogName(sDescRec);
            if(sSourceBuffer == NULL)
            {
                sLength = 0;
            }
            else
            {
                sLength = acpCStrLen(sSourceBuffer, ACP_SINT32_MAX);
            }

            ulnDataWriteStringToUserBuffer(aFnContext,
                                           sSourceBuffer,
                                           sLength,
                                           (acp_char_t *)aValuePtr,
                                           aBufferLength,
                                           &sStringLength);
            *aStringLengthPtr = sStringLength;
            break;

        case SQL_DESC_ROWVER:
        case SQL_DESC_NUM_PREC_RADIX:
        case SQL_DESC_CASE_SENSITIVE:
        case SQL_DESC_AUTO_UNIQUE_VALUE:
            ACI_RAISE(LABEL_NOT_IMPLEMENTED);
            break;

        default:
            ACI_RAISE(LABEL_MEM_MAN_ERR);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUFFERSIZE)
    {
        /*
         * HY090 : 얻고자 하는 정보가 문자 타입인데, aBufferLength 에 음수를 주었을 경우
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aBufferLength);
    }

    ACI_EXCEPTION(LABEL_NOT_IMPLEMENTED)
    {
        ulnError(aFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnGetDescRecordField");
    }

    ACI_EXCEPTION(LABEL_INVALID_DESC_INDEX)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnGetDescField(ulnDesc      *aDesc,
                          acp_sint16_t  aRecordNumber,
                          acp_sint16_t  aFieldIdentifier,
                          void         *aValuePtr,
                          acp_sint32_t  aBufferLength,
                          acp_sint32_t *aStringLengthPtr)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;
    ulnDescType   sDescType;

    ulnStmt      *sParentStmt = NULL;
    ulnDbc       *sParentDbc  = NULL;

    acp_bool_t    sLongDataCompat = ACP_FALSE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETDESCFIELD, aDesc, ULN_OBJ_TYPE_DESC);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * ====================================
     * Function Body BEGIN
     * ====================================
     */

    sDescType = ULN_OBJ_GET_DESC_TYPE(aDesc);

    ACI_TEST_RAISE(sDescType != ULN_DESC_TYPE_ARD &&
                   sDescType != ULN_DESC_TYPE_APD &&
                   sDescType != ULN_DESC_TYPE_IRD &&
                   sDescType != ULN_DESC_TYPE_IPD,
                   LABEL_INVALID_HANDLE);

    switch (ULN_OBJ_GET_TYPE(aDesc->mParentObject))
    {
        case ULN_OBJ_TYPE_STMT:
            sParentStmt = (ulnStmt *)aDesc->mParentObject;

            if (sDescType == ULN_DESC_TYPE_IRD)
            {
                ACI_TEST_RAISE(ulnCursorGetState(&sParentStmt->mCursor) == ULN_CURSOR_STATE_CLOSED,
                               LABEL_NO_DATA);
            }

            sLongDataCompat = ulnDbcGetLongDataCompat(sParentStmt->mParentDbc);

            break;

        case ULN_OBJ_TYPE_DBC:
            sParentDbc      = (ulnDbc *)aDesc->mParentObject;
            sLongDataCompat = ulnDbcGetLongDataCompat(sParentDbc);
            break;

        default:
            ACI_RAISE(LABEL_INVALID_HANDLE);
            break;
    }

    ACI_TEST_RAISE(aRecordNumber > ulnDescGetHighestBoundIndex(aDesc), LABEL_NO_DATA);

    switch(aFieldIdentifier)
    {
        case SQL_DESC_ALLOC_TYPE:
        case SQL_DESC_ARRAY_SIZE:
        case SQL_DESC_ARRAY_STATUS_PTR:
        case SQL_DESC_BIND_OFFSET_PTR:
        case SQL_DESC_BIND_TYPE:
        case SQL_DESC_COUNT:
        case SQL_DESC_ROWS_PROCESSED_PTR:
            ACI_TEST_RAISE(aRecordNumber != 0, LABEL_INVALID_INDEX);
            ACI_TEST(ulnGetDescHeaderField(&sFnContext,
                                           aFieldIdentifier,
                                           aValuePtr,
                                           aBufferLength,
                                           aStringLengthPtr) != ACI_SUCCESS);
            break;

        default:
            ACI_TEST_RAISE(aRecordNumber <= 0, LABEL_INVALID_INDEX);
            ACI_TEST(ulnGetDescRecordField(&sFnContext,
                                           aRecordNumber,
                                           aFieldIdentifier,
                                           aValuePtr,
                                           aBufferLength,
                                           aStringLengthPtr,
                                           sLongDataCompat) != ACI_SUCCESS);
            break;
    }

    /*
     * ====================================
     * Function Body END
     * ====================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_NO_DATA)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_NO_DATA);
    }

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }

    ACI_EXCEPTION(LABEL_INVALID_INDEX)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aRecordNumber);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
