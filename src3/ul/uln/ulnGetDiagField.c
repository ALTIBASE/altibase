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

/*  BUGBUG
    //SQL_DIAG_DYNAMIC_FUNCTION   SQL_DIAG_DYNAMIC_FUNCTION_CODE
    {
        "ALTER DOMAIN"          SQL_DIAG_ALTER_DOMAIN
        "ALTER TABLE"           SQL_DIAG_ALTER_TABLE
        "CREATE ASSERTION"      SQL_DIAG_CREATE_ASSERTION
        "CREATE CHARACTER SET"  SQL_DIAG_CREATE_CHARACTER_SET
        "CREATE COLLATION"      SQL_DIAG_CREATE_COLLATION
        "CREATE INDEX"          SQL_DIAG_CREATE_INDEX
        "CREATE TABLE"          SQL_DIAG_CREATE_TABLE
        "CREATE VIEW"           SQL_DIAG_CREATE_VIEW
        "CREATE DOMAIN"         SQL_DIAG_CREATE_DOMAIN
        "DROP ASSERTION"        SQL_DIAG_DROP_ASSERTION
        "DROP CHARACTER SET"    SQL_DIAG_DROP_CHARACTER_SET
        "DROP COLLATION"        SQL_DIAG_DROP_COLLATION
        "DROP DOMAIN"           SQL_DIAG_DROP_DOMAIN
        "DROP INDEX"            SQL_DIAG_DROP_INDEX
        "DROP SCHEMA"           SQL_DIAG_DROP_SCHEMA
        "DROP TABLE"            SQL_DIAG_DROP_TABLE
        "DROP TRANSLATION"      SQL_DIAG_DROP_TRANSLATION
        "DROP VIEW"             SQL_DIAG_DROP_VIEW
        "GRANT"                 SQL_DIAG_GRANT
        "CALL"                  SQL_DIAG_ CALL
        "REVOKE"                SQL_DIAG_REVOKE
        "CREATE SCHEMA"         SQL_DIAG_CREATE_SCHEMA
        "CREATE TRANSLATION"    SQL_DIAG_CREATE_TRANSLATION
        "DYNAMIC UPDATE CURSOR" SQL_DIAG_DYNAMIC_UPDATE_CURSOR
        "DYNAMIC DELETE CURSOR" SQL_DIAG_DYNAMIC_DELETE_CURSOR
        case QCP_STMT_DELETE:   "DELETE WHERE"      SQL_DIAG_DELETE_WHERE   ; break;
        case QCP_STMT_INSERT:   "INSERT"            SQL_DIAG_INSERT         ; break;
        case QCP_STMT_SELECT:   "SELECT CURSOR"     SQL_DIAG_SELECT_CURSOR  ; break;
        case QCP_STMT_UPDATE:   "UPDATE WHERE"      SQL_DIAG_UPDATE_WHERE   ; break;
        ""                      SQL_DIAG_UNKNOWN_STATEMENT
    }
*/

static void ulnGetDiagFieldReturnStringLen(acp_sint16_t *aStringLengthPtr, acp_sint16_t aLength)
{
    if (aStringLengthPtr != NULL)
    {
        *aStringLengthPtr = aLength;
    }
}

static ACI_RC ulnGetDiagFieldCheckError(ulnObject    *aObject,
                                        acp_sint16_t  aRecNumber,
                                        acp_sint16_t  aDiagIdentifier,
                                        void         *aDiagInfoPtr,
                                        SQLRETURN    *aSqlReturnCode)
{
    SQLRETURN                sSqlReturnCode = SQL_SUCCESS;
    ulnDiagIdentifierClass   sIdentifierClass;

    /*
     * 포인터들이 NULL 인지 체크.
     * ODBC 스펙에는 정해져 있지 않지만, NULL 이면 그냥 에러 리턴하자.
     */

    ACI_TEST_RAISE(aDiagInfoPtr == NULL, Error);

    /*
     * Object 가 SQL_HANDLE_STMT 이고, 상태가 S4 보다 작거나 S7 보다 크면 에러
     *  <-- State Transition Table 에 명시됨.
     */
    if (aDiagIdentifier           == SQL_DIAG_ROW_COUNT &&
        ULN_OBJ_GET_TYPE(aObject) == ULN_OBJ_TYPE_STMT)
    {
        ACI_TEST_RAISE(ULN_OBJ_GET_STATE(aObject) < ULN_S_S4 ||
                       ULN_OBJ_GET_STATE(aObject) > ULN_S_S7,
                       Error);
    }

    /*
     * SQL_ERROR : The DiagIdentifier argument was SQL_DIAG_CURSOR_ROW_COUNT,
     * SQL_DIAG_DYNAMIC_FUNCTION, SQL_DIAG_DYNAMIC_FUNCTION_CODE, or SQL_DIAG_ROW_COUNT,
     * but Handle was not a statement handle. (The Driver Manager returns this diagnostic.)
     */
    if (aDiagIdentifier == SQL_DIAG_ROW_COUNT ||
        aDiagIdentifier == SQL_DIAG_CURSOR_ROW_COUNT ||
        aDiagIdentifier == SQL_DIAG_DYNAMIC_FUNCTION ||
        aDiagIdentifier == SQL_DIAG_DYNAMIC_FUNCTION_CODE)
    {
        ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(aObject) != ULN_OBJ_TYPE_STMT, Error);
    }

    /*
     * aDiagIdentifier 의 유효성 검사
     *
     * SQL_ERROR : The RecNumber argument was negative or 0 when DiagIdentifier indicated
     * a field from a diagnostic record. RecNumber is ignored for header fields.
     */
    ACI_TEST_RAISE(
        ulnDiagGetDiagIdentifierClass(aDiagIdentifier, &sIdentifierClass) != ACI_SUCCESS,
        Error);

    switch (sIdentifierClass)
    {
        case ULN_DIAG_IDENTIFIER_CLASS_HEADER:
            break;

        case ULN_DIAG_IDENTIFIER_CLASS_RECORD:
            ACI_TEST_RAISE(aRecNumber <= 0, Error);
            break;

        default:
            ACI_RAISE(Error);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Error)
    {
        sSqlReturnCode = SQL_ERROR;
    }

    ACI_EXCEPTION_END;

    *aSqlReturnCode = sSqlReturnCode;

    return ACI_FAILURE;
}

/*
 * Destination 이 가리키는 메모리 영역에
 * Source 가 가리키는 메모리 영역에 있는 내용을 출력한다.
 * aBufferLength 는 Destination 의 크기이며,
 * aActualLength 는 실제로 Destination 에 쓰여질 문자열의 크기를 리턴할 포인터이다.
 */
static ACI_RC ulnGetDiagFieldSetCharPtr(acp_char_t         *aDestination,
                                        const acp_char_t   *aSource,
                                        const acp_sint16_t  aBufferLength,
                                        acp_sint16_t       *aActualLength,
                                        acp_bool_t         *aIsTruncated)
{
    acp_sint16_t sLength;

    ACI_TEST(aDestination == NULL);

    /*
     * char string 을 요구했으면서 buffer length 로 0 이나 음수를 주면 SQL_ERORR 를 리턴해야 한다.
     */
    ACI_TEST(aBufferLength <= 0);

    if (aSource == NULL)
    {
        sLength = 0;
    }
    else
    {
        sLength = acpCStrLen(aSource, ACP_SINT32_MAX);
    }

    if (aActualLength != NULL)
    {
        *aActualLength = sLength;
    }

    if (sLength >= aBufferLength)
    {
        *aIsTruncated = ACP_TRUE;
    }
    else
    {
        *aIsTruncated = ACP_FALSE;
    }

    /*
     * snprintf 는 쓰기를 수행할 버퍼의 크기안에서
     * 버퍼에 쓴 문자열은 항상 \0 으로 끝나는 것을 보장하는 방식으로 동작한다.
     * 그래서 아래의 문장을 수행하면,
     *      snprintf(buffer, 4, "%s", "this");
     * buffer 에는
     *      t h i \0
     * 가 들어간다.
     */
    if (aSource != NULL)
    {
        acpSnprintf(aDestination, aBufferLength, "%s", aSource);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static SQLRETURN ulnGetDiagFieldReocrd(ulnObject    *aObject,
                                       acp_sint16_t  aRecNumber,
                                       acp_sint16_t  aDiagIdentifier,
                                       void         *aDiagInfoPtr,
                                       acp_sint16_t  aBufferLength,
                                       acp_sint16_t *aStringLengthPtr)
{
    acp_sint16_t  sStringLength  = 0;
    acp_bool_t    sIsTruncated   = ACP_FALSE;

    ulnDiagRec   *sDiagRecord;
    SQLRETURN     sSqlReturnCode = SQL_SUCCESS;

    acp_char_t   *sSourceString  = NULL;

    ACI_TEST_RAISE(aRecNumber <= 0, LABEL_INVALID_RECORD_NO);

    /*
     * Diagnostic Record 를 가리키는 포인터를 얻는다.
     */
    ACI_TEST_RAISE(ulnGetDiagRecFromObject(aObject, &sDiagRecord, aRecNumber) != ACI_SUCCESS,
                   LABEL_NO_DATA);

    switch (aDiagIdentifier)
    {
        case SQL_DIAG_CLASS_ORIGIN: /* SQLCHAR * */
            ulnDiagGetClassOrigin(sDiagRecord, &sSourceString);
            ACI_TEST_RAISE(ulnGetDiagFieldSetCharPtr((acp_char_t *)aDiagInfoPtr,
                                                     sSourceString,
                                                     aBufferLength,
                                                     aStringLengthPtr,
                                                     &sIsTruncated) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);
            break;

        case SQL_DIAG_COLUMN_NUMBER: /* SQLINTEGER */
            *(acp_sint32_t *)aDiagInfoPtr = ulnDiagRecGetColumnNumber(sDiagRecord);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(acp_sint32_t));
            break;

        case SQL_DIAG_CONNECTION_NAME: /* SQLCHAR * */
            ulnDiagGetConnectionName(sDiagRecord, &sSourceString);
            ACI_TEST_RAISE(ulnGetDiagFieldSetCharPtr((acp_char_t *)aDiagInfoPtr,
                                                     sSourceString,
                                                     aBufferLength,
                                                     aStringLengthPtr,
                                                     &sIsTruncated) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);
            break;

        case SQL_DIAG_MESSAGE_TEXT: /* SQLCHAR * */
            sStringLength = aBufferLength;

            ACI_TEST_RAISE(ulnDiagRecGetMessageText(sDiagRecord,
                                                    (acp_char_t*)aDiagInfoPtr,
                                                    aBufferLength,
                                                    &sStringLength) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);

            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, sStringLength);
            ACI_TEST_RAISE(sStringLength >= aBufferLength, LABEL_TRUNCATED);
            break;

        case SQL_DIAG_NATIVE: /* SQLINTEGER */
            ulnDiagGetNative(sDiagRecord, (acp_sint32_t *)aDiagInfoPtr);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(acp_sint32_t));
            break;

        case SQL_DIAG_ROW_NUMBER: /* SQLLEN */
            *(ulvSLen *)aDiagInfoPtr = (ulvSLen)ulnDiagRecGetRowNumber(sDiagRecord);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(acp_sint32_t));
            break;

        case SQL_DIAG_SERVER_NAME: /* SQLCHAR * */
            ulnDiagGetServerName(sDiagRecord, &sSourceString);
            ACI_TEST_RAISE(ulnGetDiagFieldSetCharPtr((acp_char_t *)aDiagInfoPtr,
                                                     sSourceString,
                                                     aBufferLength,
                                                     aStringLengthPtr,
                                                     &sIsTruncated) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);
            break;

        case SQL_DIAG_SQLSTATE: /* SQLCHAR * */
            ulnDiagRecGetSqlState(sDiagRecord, &sSourceString);
            ACI_TEST_RAISE(ulnGetDiagFieldSetCharPtr((acp_char_t *)aDiagInfoPtr,
                                                     sSourceString,
                                                     aBufferLength,
                                                     aStringLengthPtr,
                                                     &sIsTruncated) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);
            break;

        case SQL_DIAG_SUBCLASS_ORIGIN: /* SQLCHAR * */
            ulnDiagGetSubClassOrigin(sDiagRecord, &sSourceString);
            ACI_TEST_RAISE(ulnGetDiagFieldSetCharPtr((acp_char_t *)aDiagInfoPtr,
                                                     sSourceString,
                                                     aBufferLength,
                                                     aStringLengthPtr,
                                                     &sIsTruncated) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);
            break;

        default:
            sSqlReturnCode = SQL_ERROR;
            break;
    }

    if (sSqlReturnCode == SQL_SUCCESS && sIsTruncated == ACP_TRUE)
    {
        sSqlReturnCode = SQL_SUCCESS_WITH_INFO;
    }

    return sSqlReturnCode;

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN)
    {
        sSqlReturnCode = SQL_ERROR;
    }

    ACI_EXCEPTION(LABEL_TRUNCATED)
    {
        sSqlReturnCode = SQL_SUCCESS_WITH_INFO;
    }

    ACI_EXCEPTION(LABEL_NO_DATA)
    {
        sSqlReturnCode = SQL_NO_DATA;
    }

    ACI_EXCEPTION(LABEL_INVALID_RECORD_NO)
    {
        sSqlReturnCode = SQL_ERROR;
    }

    ACI_EXCEPTION_END;

    return sSqlReturnCode;
}

SQLRETURN ulnGetDiagField(acp_sint16_t  aHandleType,
                          ulnObject    *aObject,
                          acp_sint16_t  aRecNumber,
                          acp_sint16_t  aDiagIdentifier,
                          void         *aDiagInfoPtr,
                          acp_sint16_t  aBufferLength,
                          acp_sint16_t *aStringLengthPtr)
{
    ULN_FLAG(sNeedUnlock);

    acp_bool_t        sIsTruncated   = ACP_FALSE;
    const acp_char_t *sSourceString  = NULL;

    SQLRETURN         sSqlReturnCode = SQL_SUCCESS;
    ulnDiagHeader    *sDiagHeader;
    ulnObject        *sObject;

    ACI_TEST_RAISE(aObject == NULL, LABEL_INVALID_HANDLE);

    sObject = aObject;

    /*
     * 핸들 타입과 객체의 실제 타입이 일치하는지 체크
     */
    switch (aHandleType)
    {
        case SQL_HANDLE_STMT:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_STMT, LABEL_INVALID_HANDLE);
            break;

        case SQL_HANDLE_DBC:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_DBC, LABEL_INVALID_HANDLE);
            sObject = (ulnObject *) ulnDbcSwitch((ulnDbc*) sObject);

            break;

        case SQL_HANDLE_ENV:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_ENV, LABEL_INVALID_HANDLE);
            break;

        case SQL_HANDLE_DESC:
            ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sObject) != ULN_OBJ_TYPE_DESC, LABEL_INVALID_HANDLE);
            ACI_TEST_RAISE(ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_ARD &&
                           ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_APD &&
                           ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_IRD &&
                           ULN_OBJ_GET_DESC_TYPE(sObject) != ULN_DESC_TYPE_IPD,
                           LABEL_INVALID_HANDLE);
            break;

        default:
            ACI_RAISE(LABEL_INVALID_HANDLE);
            break;
    }

    ULN_OBJECT_LOCK(sObject, ULN_FID_GETDIAGFIELD);
    ULN_FLAG_UP(sNeedUnlock);

    /*
     * ======================================
     * Function BEGIN
     * ======================================
     */

    /*
     * 에러 체킹
     */
    ACI_TEST(ulnGetDiagFieldCheckError(sObject,
                                       aRecNumber,
                                       aDiagIdentifier,
                                       aDiagInfoPtr,
                                       &sSqlReturnCode) != ACI_SUCCESS);

    sDiagHeader = ulnGetDiagHeaderFromObject(sObject);

    /*
     * Note : 64비트 0dbc 와 관련한 주의사항 :
     *
     *        When the DiagIdentifier parameter has one of the following values,
     *        a 64-bit value is returned in *DiagInfoPtr:
     *
     *          SQL_DIAG_CURSOR_ROW_COUNT
     *          SQL_DIAG_ROW_COUNT
     *          SQL_DIAG_ROW_NUMBER
     */

    switch (aDiagIdentifier)
    {
        /*
         * 헤더 필드들
         */
        case SQL_DIAG_CURSOR_ROW_COUNT: /* SQLLEN */
            /*
             * BUGBUG : 구현안되어 있다. 그냥 0 을 리턴하자.
             */
            if (aDiagInfoPtr != NULL)
            {
                *(ulvSLen *)aDiagInfoPtr = ULN_vLEN(0);
            }
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(ulvSLen));
            break;

        case SQL_DIAG_DYNAMIC_FUNCTION: /* SQLCHAR * */
            ulnDiagGetDynamicFunction(sDiagHeader, &sSourceString);
            ACI_TEST_RAISE(ulnGetDiagFieldSetCharPtr((acp_char_t *)aDiagInfoPtr,
                                                     sSourceString,
                                                     aBufferLength,
                                                     aStringLengthPtr,
                                                     &sIsTruncated) != ACI_SUCCESS,
                           LABEL_INVALID_BUFFER_LEN);
            ACI_TEST_RAISE(sIsTruncated == ACP_TRUE, LABEL_TRUNCATED);
            break;

        case SQL_DIAG_DYNAMIC_FUNCTION_CODE: /* SQLINTEGER */
            ulnDiagGetDynamicFunctionCode(sDiagHeader, (acp_sint32_t *)aDiagInfoPtr);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(acp_sint32_t));
            break;

        case SQL_DIAG_NUMBER: /* SQLINTEGER */
            ulnDiagGetNumber(sDiagHeader, (acp_sint32_t *)aDiagInfoPtr);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(acp_sint32_t));
            break;

        case SQL_DIAG_RETURNCODE: /* SQLRETURN */
            ulnDiagGetReturnCode(sDiagHeader, (SQLRETURN *)aDiagInfoPtr);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(SQLRETURN));
            break;

        case SQL_DIAG_ROW_COUNT: /* SQLLEN */
            *(ulvSLen *)aDiagInfoPtr = (ulvSLen)ulnDiagGetRowCount(sDiagHeader);
            ulnGetDiagFieldReturnStringLen(aStringLengthPtr, ACI_SIZEOF(ulvSLen));
            break;

        default:
            sSqlReturnCode = ulnGetDiagFieldReocrd(sObject,
                                                   aRecNumber,
                                                   aDiagIdentifier,
                                                   aDiagInfoPtr,
                                                   aBufferLength,
                                                   aStringLengthPtr);
            break;
    }

    /*
     * ======================================
     * Function END
     * ======================================
     */

    ULN_FLAG_DOWN(sNeedUnlock);
    ULN_OBJECT_UNLOCK(sObject, ULN_FID_GETDIAGFIELD);

    return sSqlReturnCode;

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        sSqlReturnCode = SQL_INVALID_HANDLE;
    }

    ACI_EXCEPTION(LABEL_TRUNCATED)
    {
        sSqlReturnCode = SQL_SUCCESS_WITH_INFO;
    }

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN)
    {
        sSqlReturnCode = SQL_ERROR;
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedUnlock)
    {
        ULN_OBJECT_UNLOCK(sObject, ULN_FID_GETDIAGFIELD);
    }

    return sSqlReturnCode;
}

