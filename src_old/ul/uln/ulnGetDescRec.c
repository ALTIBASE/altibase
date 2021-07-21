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
 * BUGBUG : 상위 핸들간의 상태 전이에 관련된 부분이 구현이 되지 않은 관계로
 *          Get/Set DescRec/DescField 함수들의 상태전이는 구현이 불가능하다.
 */

SQLRETURN ulnGetDescRec(ulnDesc      *aDesc,
                        acp_sint16_t  aRecordNumber,
                        acp_char_t   *aName,
                        acp_sint16_t  aBufferLength,
                        acp_sint16_t *aStringLengthPtr,
                        acp_sint16_t *aTypePtr,
                        acp_sint16_t *aSubTypePtr,
                        ulvSLen      *aLengthPtr,
                        acp_sint16_t *aPrecisionPtr,
                        acp_sint16_t *aScalePtr,
                        acp_sint16_t *aNullablePtr)
{
    ULN_FLAG(sNeedExit);
    ulnFnContext  sFnContext;

    ulnDescRec   *sDescRec    = NULL;

    ulnStmt      *sParentStmt = NULL;
    ulnDbc       *sParentDbc  = NULL;

    acp_uint32_t  sLength;
    acp_char_t   *sSourceBuffer;
    acp_sint16_t  sSQLTYPE    = 0;

    acp_bool_t    sLongDataCompat = ACP_FALSE;

    ulnDescType   sDescType;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETDESCREC, aDesc, ULN_OBJ_TYPE_DESC);

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
    ACI_TEST_RAISE(aRecordNumber <= 0, LABEL_INVALID_INDEX);

    sDescRec = ulnDescGetDescRec(aDesc, aRecordNumber);
    ACI_TEST_RAISE(sDescRec == NULL, LABEL_MEM_MAN_ERR2);

    /*
     * --------------------------------------------------------------
     *  Name             : SQL_DESC_NAME 필드의 값 리턴
     *  aBufferLength    : aName 이 가리키는 버퍼의 size
     *  aStringLengthPtr : 실제 SQL_DESC_NAME 필드에 있는 이름의 길이
     * --------------------------------------------------------------
     */

    if (aName != NULL)
    {
        ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFFERSIZE);        //BUG-28623 [CodeSonar]Ignored Return Value
        /*
         * BUGBUG : ulnColAttribute() 의 SQL_DESC_NAME 부분과 중복된 코드
         */
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

        ulnDataWriteStringToUserBuffer(&sFnContext,
                                       sSourceBuffer,
                                       sLength,
                                       aName,
                                       aBufferLength,
                                       aStringLengthPtr);
    }

    /*
     * --------------------------------------------------------------
     *  aTypePtr : SQL_DESC_TYPE 필드의 값 리턴
     * --------------------------------------------------------------
     */

    if (aTypePtr != NULL)
    {
        /*
         * BUGBUG : ulnColAttribute() 의 SQL_DESC_NAME 부분과 중복된 코드
         *
         * verbose data type 을 리턴해야 한다.
         * ulnMeta 의 odbc type 에는 verbose type 이 들어간다.
         */

        /*
         * Note : ulnTypeMap_LOB_SQLTYPE :
         *        이처럼 ulnTypes.cpp 에서 근본적으로 수정하지 않고,
         *        사용자에게 타입을 돌려주는 함수마다 그때그때 long type 을 매핑하는
         *        함수를 호출하는 것은 버그의 소지도 있고 위험한 짓이지만,
         *        ulnTypes.cpp 에 function context, dbc, stmt 등의 지저분한 다른
         *        것들을 받는 함수를 만들고 싶지 않아서 굳이 이와같이 했다.
         */

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
        sSQLTYPE = ulnTypeMap_LOB_SQLTYPE(sSQLTYPE, sLongDataCompat);
        // BUG-21730: wrong type(SInt) casting
        *aTypePtr = sSQLTYPE;
    }

    /*
     * ----------------------------------------------------------------------------
     *  aSubTypePtr : type 이 SQL_DATETIME 이나 SQL_INTERVAL 일 때
     *                SQL_DESC_DATETIME_INTERVAL_CODE 를 리턴한다.
     *
     *  msdn 의 설명을 읽어보면, type 이 SQL_DATETIME 일 때에만 리턴하는 것 같다.
     *                           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
     * ----------------------------------------------------------------------------
     */

    // To Fix BUG-20651
    if (aSubTypePtr != NULL)
    {
        if ( (sSQLTYPE == SQL_DATETIME) || (sSQLTYPE == SQL_INTERVAL) )
        {
            *aSubTypePtr = ulnMetaGetOdbcDatetimeIntCode(&sDescRec->mMeta);
        }
        else
        {
            *aSubTypePtr = 0;
        }
    }

    /*
     * ----------------------------------------------------------------------------
     *  aLengthPtr : SQL_DESC_OCTET_LENGTH 를 리턴
     * ----------------------------------------------------------------------------
     */

    if (aLengthPtr != NULL)
    {
        /*
         * BUGBUG : ulnColAttribute() 의 SQL_DESC_NAME 부분과 중복된 코드
         *
         * IRD 의 octet length 는 null 종료자를 제외한 길이가 들어간다.
         * 그러나 SQLColAttribute 함수의 설명에서 이 field id 로 리턴되는 값에는
         * null 종료자의 길이까지 포함한다고 되어 있다.
         */
        *aLengthPtr = ulnMetaGetOctetLength(&sDescRec->mMeta) +
                                                ULN_SIZE_OF_NULLTERMINATION_CHARACTER;
    }

    /*
     * ----------------------------------------------------------------------------
     *  aPrecisionPtr : SQL_DESC_PRECISION 을 리턴
     * ----------------------------------------------------------------------------
     */

    if (aPrecisionPtr != NULL)
    {
        *aPrecisionPtr = ulnMetaGetPrecision(&sDescRec->mMeta);
    }

    /*
     * ----------------------------------------------------------------------------
     *  aScalePtr : SQL_DESC_SCALE 을 리턴
     * ----------------------------------------------------------------------------
     */

    if (aScalePtr != NULL)
    {
        *aScalePtr = ulnMetaGetScale(&sDescRec->mMeta);
    }

    /*
     * ----------------------------------------------------------------------------
     *  aNullablePtr : SQL_DESC_NULLABLE 을 리턴
     * ----------------------------------------------------------------------------
     */

    if (aNullablePtr != NULL)
    {
        *aNullablePtr = ulnMetaGetNullable(&sDescRec->mMeta);
    }

    /*
     * ====================================
     * Function Body END
     * ====================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_BUFFERSIZE)
    {
        /*
         * HY090 : 얻고자 하는 정보가 문자 타입인데, aBufferLength 에 음수를 주었을 경우
         */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aBufferLength);
    }

    ACI_EXCEPTION(LABEL_NO_DATA)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_NO_DATA);
    }

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR2)
    {
        /*
         * BUGBUG : 이와같은 경우에는 디폴트값을 리턴해야 하지만
         *          여유가 없는 관계로 일단 메모리 매니지 에러를 리턴한다.
         */
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulnColAttribute2");
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

