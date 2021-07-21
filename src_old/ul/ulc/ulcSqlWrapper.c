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
#include <ulo.h>
#include <ulnCharSet.h>

#include <sqlcli.h>

#include <ulnwCPool.h>

#ifndef SQL_API
#define SQL_API
#endif

mtlModule* gWcharModule  = NULL;

// fix BUG-25172
// 한글 DSN 및 한글 데이터가 올 수 있으므로 ASCII 대신 CLIENT NLS로 캐릭터셋을 설정한다.
mtlModule* gClientModule = NULL;

acp_sint32_t getWcharLength(SQLWCHAR* aWchar)
{
    SQLWCHAR     *sTemp;
    acp_sint32_t  sLen;

    if (aWchar != NULL)
    {
        sTemp = aWchar;

        while(*sTemp != 0)
        {
            sTemp++;
        }
        sLen = (SQLCHAR*)sTemp - (SQLCHAR*)aWchar;
    }
    else
    {
        sLen = 0;
    }

    return sLen;
}

/*
 * =============================
 * Alloc Handle
 * =============================
 */
#if (ODBCVER >= 0x0300)
SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
                                 SQLHANDLE   InputHandle,
                                 SQLHANDLE  *OutputHandle)
{
    SQLRETURN   sRet;
    acp_char_t *sClientNLS = NULL;
    acp_bool_t  sNeedASCII = ACP_TRUE;

    ULN_TRACE(SQLAllocHandle);

    sRet = ulnAllocHandle((acp_sint16_t)HandleType,
                          InputHandle,
                          OutputHandle);

    if (HandleType == SQL_HANDLE_ENV)
    {
        if (gWcharModule == NULL)
        {
#ifdef SQL_WCHART_CONVERT
            ACI_TEST(mtlModuleByName((const mtlModule **)&gWcharModule,
                                     "UTF32",
                                     5)
                     != ACI_SUCCESS);
#else
            ACI_TEST(mtlModuleByName((const mtlModule **)&gWcharModule,
                                     "UTF16",
                                     5)
                     != ACI_SUCCESS);
#endif
        }

        // fix BUG-25172
        // 한글 DSN 및 한글 데이터가 올 수 있으므로 ASCII 대신 CLIENT NLS로 캐릭터셋을 설정한다.
        if (gClientModule == NULL)
        {
            if (acpEnvGet("ALTIBASE_NLS_USE", &sClientNLS) == ACP_RC_SUCCESS)
            {
                if (mtlModuleByName((const mtlModule **)&gClientModule,
                                    sClientNLS,
                                    acpCStrLen(sClientNLS, ACP_SINT32_MAX)) == ACI_SUCCESS)
                {
                    sNeedASCII = ACP_FALSE;
                }
            }

            // 환경변수의 ALTIBASE_NLS_USE가 없거나 잘못되어 있을 경우
            // 기본값인 ASCII로 설정하도록 한다.
            if (sNeedASCII == ACP_TRUE)
            {
                ACI_TEST(mtlModuleByName((const mtlModule **)&gClientModule,
                                         "US7ASCII",
                                         8)
                         != ACI_SUCCESS);
            }
        }
    }

    return sRet;

    ACI_EXCEPTION_END;
    {
        //BUGBUG-TODO What Description Record ??
    }

    return SQL_ERROR;
}

SQLRETURN SQL_API SQLAllocHandleStd(SQLSMALLINT HandleType,
                                    SQLHANDLE   InputHandle,
                                    SQLHANDLE  *OutputHandle)
{
    ULN_TRACE(SQLAllocHandleStd);
    return ulnAllocHandleStd((acp_sint16_t)HandleType,
                            InputHandle,
                            OutputHandle);
}
#endif

SQLRETURN SQL_API SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    SQLRETURN   sRet;
    acp_char_t *sClientNLS = NULL;
    acp_bool_t  sNeedASCII = ACP_TRUE;

    ULN_TRACE(SQLAllocEnv);

    sRet = ulnAllocHandle(SQL_HANDLE_ENV,
                          NULL,
                          EnvironmentHandle);

    if (gWcharModule == NULL)
    {
#ifdef SQL_WCHART_CONVERT
        ACI_TEST(mtlModuleByName((const mtlModule **)&gWcharModule,
                                 "UTF32",
                                 5)
                 != ACI_SUCCESS);
#else
        ACI_TEST(mtlModuleByName((const mtlModule **)&gWcharModule,
                                 "UTF16",
                                 5)
                 != ACI_SUCCESS);
#endif
    }

    // fix BUG-25172
    // 한글 DSN 및 한글 데이터가 올 수 있으므로 ASCII 대신 CLIENT NLS로 캐릭터셋을 설정한다.
    if (gClientModule == NULL)
    {
        if (acpEnvGet("ALTIBASE_NLS_USE", &sClientNLS) == ACP_RC_SUCCESS)
        {
            if (mtlModuleByName((const mtlModule **)&gClientModule,
                                sClientNLS,
                                acpCStrLen(sClientNLS, ACP_SINT32_MAX)) == ACI_SUCCESS)
            {
                sNeedASCII = ACP_FALSE;
            }
        }

        // 환경변수의 ALTIBASE_NLS_USE가 없거나 잘못되어 있을 경우
        // 기본값인 ASCII로 설정하도록 한다.
        if (sNeedASCII == ACP_TRUE)
        {
            ACI_TEST(mtlModuleByName((const mtlModule **)&gClientModule,
                                     "US7ASCII",
                                     8)
                     != ACI_SUCCESS);
        }
    }

    return sRet;

    ACI_EXCEPTION_END;
    {
        //BUGBUG-TODO What Description Record ??
    }

    return SQL_ERROR;
}

SQLRETURN SQL_API SQLAllocConnect(SQLHENV EnvironmentHandle,
                                  SQLHDBC *ConnectionHandle)
{
    ULN_TRACE(SQLAllocConnect);
    return ulnAllocHandle(SQL_HANDLE_DBC,
                          EnvironmentHandle,
                          ConnectionHandle);
}

SQLRETURN SQL_API SQLAllocStmt(SQLHDBC   ConnectionHandle,
                               SQLHSTMT *StatementHandle)
{
    ULN_TRACE(SQLAllocStmt);
    return ulnAllocHandle(SQL_HANDLE_STMT,
                          ConnectionHandle,
                          StatementHandle);
}

/*
 * =============================
 * Free Handle
 * =============================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    ULN_TRACE(SQLFreeHandle);
    return ulnFreeHandle((acp_sint16_t)HandleType, (ulnObject *)Handle);
}
#endif

SQLRETURN  SQL_API SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    ULN_TRACE(SQLFreeEnv);
    return ulnFreeHandle(SQL_HANDLE_ENV, (ulnObject *)EnvironmentHandle);
}

SQLRETURN  SQL_API SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    ULN_TRACE(SQLFreeConnect);
    return ulnFreeHandle(SQL_HANDLE_DBC, (ulnObject *)ConnectionHandle);
}

SQLRETURN  SQL_API SQLFreeStmt(SQLHSTMT StatementHandle,
                               SQLUSMALLINT Option)
{
    ULN_TRACE(SQLFreeStmt);
    return ulnFreeStmt((ulnStmt *)StatementHandle, (acp_uint16_t)Option);
}

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLCloseCursor(SQLHSTMT StatementHandle)
{
    ULN_TRACE(SQLCloseCursor);
    return ulnCloseCursor((ulnStmt *)StatementHandle);
}
#endif

/*
 * =============================
 * Env Attr
 * =============================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLSetEnvAttr(SQLHENV EnvironmentHandle,
                                 SQLINTEGER Attribute,
                                 SQLPOINTER Value,
                                 SQLINTEGER StringLength)
{
    ULN_TRACE(SQLSetEnvAttr);
    return ulnSetEnvAttr((ulnEnv *)EnvironmentHandle,
                         (acp_sint32_t)Attribute,
                         (void *)Value,
                         (acp_sint32_t)StringLength);
}

SQLRETURN  SQL_API SQLGetEnvAttr(SQLHENV     EnvironmentHandle,
                                 SQLINTEGER  Attribute,
                                 SQLPOINTER  Value,
                                 SQLINTEGER  BufferLength,
                                 SQLINTEGER *StringLength)
{
    ULN_TRACE(SQLGetEnvAttr);
    return ulnGetEnvAttr((ulnEnv *)EnvironmentHandle,
                         (acp_sint32_t)Attribute,
                         (void *)Value,
                         (acp_sint32_t)BufferLength,
                         (acp_sint32_t *)StringLength);
}
#endif

/*
 * =============================
 * Connection Attr
 * =============================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLSetConnectAttr(SQLHDBC    ConnectionHandle,
                                     SQLINTEGER Attribute,
                                     SQLPOINTER Value,
                                     SQLINTEGER StringLength)
{
    ULN_TRACE(SQLSetConnectAttr);

    return ulnSetConnectAttr((ulnDbc *)ConnectionHandle,
                             (acp_sint32_t)Attribute,
                             (void *)Value,
                             (acp_sint32_t)StringLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLSetConnectAttrW(SQLHDBC    ConnectionHandle,
                                      SQLINTEGER Attribute,
                                      SQLPOINTER Value,
                                      SQLINTEGER StringLength)
{
    /*
     * StringLength
     *     [Input] If Attribute is an ODBC-defined attribute and ValuePtr points
     *     to a character string or a binary buffer, this argument should be the
     *     length of *ValuePtr. For character string data, this argument should
     *     contain the number of bytes in the string.
     *     If Attribute is an ODBC-defined attribute and ValuePtr is an integer,
     *     StringLength is ignored.
     *     If Attribute is a driver-defined attribute, the application indicates
     *     the nature of the attribute to the Driver Manager by setting the
     *     StringLength argument. StringLength can have the following values:
     *     * If ValuePtr is a pointer to a character string, then StringLength is
     *     the length of the string or SQL_NTS.
     *     * If ValuePtr is a pointer to a binary buffer, then the application
     *     places the result of the SQL_LEN_BINARY_ATTR(length) macro in
     *     StringLength. This places a negative value in StringLength.
     *     * If ValuePtr is a pointer to a value other than a character string
     *     or a binary string, then StringLength should have the value SQL_IS_POINTER.
     *     * If ValuePtr contains a fixed-length value, then StringLength is either
     *     SQL_IS_INTEGER or SQL_IS_UINTEGER, as appropriate.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp = NULL;
    acp_sint32_t  sStep = 0;
    acp_sint32_t  sState = 0;

    ULN_TRACE(SQLSetConnectAttrW);

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    switch(Attribute)
    {
        case ALTIBASE_APP_INFO:
        case ALTIBASE_DATE_FORMAT:
        case ALTIBASE_NLS_CHARACTERSET:
        case ALTIBASE_NLS_NCHAR_CHARACTERSET:
        case ALTIBASE_NLS_USE:
        case ALTIBASE_XA_NAME:
        case SQL_ATTR_CURRENT_CATALOG:
            if (Value != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, StringLength * 2) != ACP_RC_SUCCESS);
                sStep = 1;

                ACI_TEST(ulnCharSetConvertUseBuffer(&sCharSet,
                                                    NULL,
                                                    ConnectionHandle,
                                                    (const mtlModule *)gWcharModule,
                                                    (const mtlModule *)gClientModule,
                                                    Value,
                                                    StringLength,
                                                    (void*)sTemp,
                                                    StringLength * 2,
                                                    CONV_DATA_IN)
                         != ACI_SUCCESS);

                Value        = (void*)ulnCharSetGetConvertedText(&sCharSet);
                StringLength = ulnCharSetGetConvertedTextLen(&sCharSet);
            }
            break;

        default:
            break;
    }

    sRet = ulnSetConnectAttr((ulnDbc *)ConnectionHandle,
                             (acp_sint32_t)Attribute,
                             (void *)Value,
                             (acp_sint32_t)StringLength);

    if (sStep == 1)
    {
        acpMemFree(sTemp);
    }

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;
    {
        if (sStep == 1)
        {
            acpMemFree(sTemp);
            sRet = SQL_ERROR;
        }

        if (sState == 1)
        {
            ulnCharSetFinalize(&sCharSet);
        }
    }

    return sRet;
}
#endif

SQLRETURN  SQL_API SQLGetConnectAttr(SQLHDBC     ConnectionHandle,
                                     SQLINTEGER  Attribute,
                                     SQLPOINTER  Value,
                                     SQLINTEGER  BufferLength,
                                     SQLINTEGER *StringLength)
{
    ULN_TRACE(SQLGetConnectAttr);
    return ulnGetConnectAttr((ulnDbc *)ConnectionHandle,
                             (acp_sint32_t)Attribute,
                             (void *)Value,
                             (acp_sint32_t)BufferLength,
                             (acp_sint32_t *)StringLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetConnectAttrW(SQLHDBC     ConnectionHandle,
                                      SQLINTEGER  Attribute,
                                      SQLPOINTER  Value,
                                      SQLINTEGER  BufferLength,
                                      SQLINTEGER *StringLength)
{
    /*
     * BufferLength
     *     [Input] If Attribute is an ODBC-defined attribute and ValuePtr points to
     *     a character string or a binary buffer, this argument should be the length
     *     of *ValuePtr. If Attribute is an ODBC-defined attribute and *ValuePtr is
     *     an integer, BufferLength is ignored. If the value in *ValuePtr is a
     *     Unicode string (when calling SQLGetConnectAttrW), the BufferLength argument
     *     must be an even number.
     *     If Attribute is a driver-defined attribute, the application indicates the
     *     nature of the attribute to the Driver Manager by setting the BufferLength
     *     argument. BufferLength can have the following values:
     *     * If *ValuePtr is a pointer to a character string, BufferLength is the length of the string.
     *     * If *ValuePtr is a pointer to a binary buffer, the application places the
     *     result of the SQL_LEN_BINARY_ATTR(length) macro in BufferLength. This places
     *     a negative value in BufferLength.
     *     * If *ValuePtr is a pointer to a value other than a character string or
     *     binary string, BufferLength should have the value SQL_IS_POINTER.
     *     * If *ValuePtr contains a fixed-length data type, BufferLength is either
     *     SQL_IS_INTEGER or SQL_IS_UINTEGER, as appropriate.
     *
     * StringLengthPtr
     *     [Output] A pointer to a buffer in which to return the total number of bytes
     *     (excluding the null-termination character) available to return in *ValuePtr.
     *     If *ValuePtr is a null pointer, no length is returned. If the attribute value
     *     is a character string and the number of bytes available to return is greater
     *     than BufferLength minus the length of the null-termination character, the data
     *     in *ValuePtr is truncated to BufferLength minus the length of the null-termination
     *     character and is null-terminated by the driver.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint32_t  sLength = 0;

    ULN_TRACE(SQLGetConnectAttrW);

    ulnCharSetInitialize(&sCharSet);

    sRet =  ulnGetConnectAttr((ulnDbc *)ConnectionHandle,
                              (acp_sint32_t)Attribute,
                              (void *)Value,
                              (acp_sint32_t)BufferLength,
                              (acp_sint32_t *)&sLength);

    if (StringLength != NULL)
    {
        *StringLength = sLength;
    }

    switch(Attribute)
    {
        case ALTIBASE_APP_INFO:
        case ALTIBASE_DATE_FORMAT:
        case ALTIBASE_NLS_CHARACTERSET:
        case ALTIBASE_NLS_NCHAR_CHARACTERSET:
        case ALTIBASE_NLS_USE:
        case ALTIBASE_XA_NAME:
        case SQL_ATTR_CURRENT_CATALOG:
            if (Value != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLength + 1,
                           (acp_char_t*)Value,
                           acpCStrLen((acp_char_t*)Value, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sCharSet,
                                               NULL,
                                               ConnectionHandle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLength,
                                               Value,
                                               BufferLength - ACI_SIZEOF(ulWChar),
                                               CONV_DATA_OUT)
                    != ACI_SUCCESS)
                {
                    sRet = SQL_SUCCESS_WITH_INFO;
                    sLength = BufferLength;
                    ((SQLWCHAR*)Value)[(BufferLength / ACI_SIZEOF(ulWChar)) - 1] = 0;
                }
                else
                {
                    sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                    ((SQLWCHAR*)Value)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                }

                if (StringLength != NULL)
                {
                    *StringLength = sLength;
                }

                acpMemFree(sTemp);
            }
            break;

        default:
            break;
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif
#endif

SQLRETURN  SQL_API SQLSetConnectOption(SQLHDBC      ConnectionHandle,
                                       SQLUSMALLINT Option,
                                       SQLULEN      Value)
{
    SQLRETURN sRet;

    ULN_TRACE(SQLSetConnectOption);

    /*
     * MSDN 에 따르면, Option 이 SQL_ATTR_QUIET_MODE 일 때 Value 에는 64비트 값이
     * 넘어온다고 한다.
     *
     * 그 외에는 언급이 없으므로 그냥 32 비트로 캐스팅해서 넘긴다.
     */

    switch (Option)
    {
            /*
             * 32bit value 를 받는 attributes
             */
        case ALTIBASE_MESSAGE_CALLBACK:
        case ALTIBASE_EXPLAIN_PLAN:
        case SQL_ATTR_FAILOVER:
        case SQL_ATTR_ACCESS_MODE:
        case SQL_ATTR_AUTOCOMMIT:
        case SQL_ATTR_CONNECTION_TIMEOUT :
        case SQL_ATTR_QUERY_TIMEOUT:
        case SQL_ATTR_DISCONNECT_BEHAVIOR:
        case SQL_ATTR_LOGIN_TIMEOUT:
        case SQL_ATTR_ODBC_CURSORS:
        case SQL_ATTR_PACKET_SIZE:
        case SQL_ATTR_TXN_ISOLATION:
        case SQL_ATTR_ODBC_VERSION:
        case SQL_ATTR_CONNECTION_POOLING:
            sRet = ulnSetConnectAttr((ulnDbc *)ConnectionHandle,
                                     (acp_sint32_t)Option,
                                     (void *)((acp_slong_t)Value),
                                     0);
            break;

            /*
             * string data 를 받는 attributes
             */
        case ALTIBASE_DATE_FORMAT:
        case ALTIBASE_NLS_USE:
            sRet = ulnSetConnectAttr((ulnDbc *)ConnectionHandle,
                                     (acp_sint32_t)Option,
                                     (void *)((acp_slong_t)Value),
                                     SQL_NTS);
            break;

            /*
             * 아직 구현 안되었거나 누락된 것들
             */
        case SQL_ATTR_ENLIST_IN_XA:
        case SQL_ATTR_ENLIST_IN_DTC:
        case SQL_ATTR_QUIET_MODE:
        case SQL_ATTR_TRACE:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
        case SQL_ATTR_TRANSLATE_OPTION:
        case SQL_ATTR_CURRENT_CATALOG:
        default:
            sRet = ulnSetConnectAttr((ulnDbc *)ConnectionHandle,
                                     (acp_sint32_t)Option,
                                     (void *)((acp_slong_t)Value),
                                     0);
            break;
    }

    return sRet;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLSetConnectOptionW(SQLHDBC      ConnectionHandle,
                                        SQLUSMALLINT Option,
                                        SQLULEN      Value)
{
    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp = NULL;
    acp_sint32_t  sStep = 0;
    acp_sint32_t  sLength = 0;
    acp_sint32_t  sState = 0;

    // BUGBUG
    // SQLULEN을 포인터로 사용하는 것은 문제가 있다.
    acp_ulong_t   sValue;

    ULN_TRACE(SQLSetConnectOptionW);


    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    sValue = Value;

    switch(Option)
    {
        case ALTIBASE_APP_INFO:
        case ALTIBASE_DATE_FORMAT:
        case ALTIBASE_NLS_CHARACTERSET:
        case ALTIBASE_NLS_NCHAR_CHARACTERSET:
        case ALTIBASE_NLS_USE:
        case ALTIBASE_XA_NAME:
        case SQL_ATTR_CURRENT_CATALOG:
            sLength = getWcharLength((SQLWCHAR*)((void*)sValue));
            ACI_TEST(acpMemAlloc((void**)&sTemp, sLength * 2) != ACP_RC_SUCCESS);
            sStep = 1;

            ACI_TEST(ulnCharSetConvertUseBuffer(&sCharSet,
                                                NULL,
                                                ConnectionHandle,
                                                (const mtlModule *)gWcharModule,
                                                (const mtlModule *)gClientModule,
                                                (void*)sValue,
                                                sLength,
                                                (void*)sTemp,
                                                sLength * 2,
                                                CONV_DATA_IN)
                     != ACI_SUCCESS);

            sValue = (acp_ulong_t)ulnCharSetGetConvertedText(&sCharSet);
            sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
            break;

        default:
            break;
    }

    sRet = ulnSetConnectAttr((ulnDbc *)ConnectionHandle,
                             (acp_sint32_t)Option,
                             (void *)sValue,
                             (acp_sint32_t)sLength);

    if (sStep == 1)
    {
        acpMemFree(sTemp);
    }

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;
    {
        if (sStep == 1)
        {
            acpMemFree(sTemp);
            sRet = SQL_ERROR;
        }

        if (sState == 1)
        {
            ulnCharSetFinalize(&sCharSet);
        }
    }

    return sRet;
}
#endif

SQLRETURN  SQL_API SQLGetConnectOption(SQLHDBC      ConnectionHandle,
                                       SQLUSMALLINT Option,
                                       SQLPOINTER   Value)
{
    ULN_TRACE(SQLGetConnectOption);

    /*
     * Note : M$ ODBC 에서는 string 이냐, 32bit integer 냐에 따라서 구분하라고 했지만,
     *        어차피, 내부에서는 구분하지 않기 때문에 그냥 하나로 매핑한다
     *
     * Note : Option 이 SQL_ATTR_QUIET_MODE 일 때는 64비트 값이 되어야 하는데,
     *        지원 안하는 속성이므로 상관 없다.
     */
    return ulnGetConnectAttr((ulnDbc *)ConnectionHandle,
                             (acp_sint32_t)Option,
                             (void *)Value,
                             SQL_MAX_OPTION_STRING_LENGTH,
                             NULL);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetConnectOptionW(SQLHDBC      ConnectionHandle,
                                        SQLUSMALLINT Option,
                                        SQLPOINTER   Value)
{
    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint32_t  sLength = 0;

    ULN_TRACE(SQLGetConnectOptionW);

    ulnCharSetInitialize(&sCharSet);

    sRet =  ulnGetConnectAttr((ulnDbc *)ConnectionHandle,
                             (acp_sint32_t)Option,
                             (void *)Value,
                             SQL_MAX_OPTION_STRING_LENGTH,
                             (acp_sint32_t *)&sLength);

    switch(Option)
    {
        case ALTIBASE_APP_INFO:
        case ALTIBASE_DATE_FORMAT:
        case ALTIBASE_NLS_CHARACTERSET:
        case ALTIBASE_NLS_NCHAR_CHARACTERSET:
        case ALTIBASE_NLS_USE:
        case ALTIBASE_XA_NAME:
        case SQL_ATTR_CURRENT_CATALOG:
            if (Value != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLength + 1,
                           (acp_char_t*)Value,
                           acpCStrLen((acp_char_t*)Value, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sCharSet,
                                               NULL,
                                               ConnectionHandle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLength,
                                               Value,
                                               SQL_MAX_OPTION_STRING_LENGTH,
                                               CONV_DATA_OUT)
                    != ACI_SUCCESS)
                {
                    sRet = SQL_SUCCESS_WITH_INFO;
                    ((SQLWCHAR*)Value)[(SQL_MAX_OPTION_STRING_LENGTH / ACI_SIZEOF(ulWChar)) - 1] = 0;
                }
                else
                {
                    sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                    ((SQLWCHAR*)Value)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                }

                acpMemFree(sTemp);
            }
            break;

        default:
            break;
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

/*
 * =============================
 * Statement Attr
 * =============================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLSetStmtAttr(SQLHSTMT   StatementHandle,
                                  SQLINTEGER Attribute,
                                  SQLPOINTER Value,
                                  SQLINTEGER StringLength)
{
    ULN_TRACE(SQLSetStmtAttr);

    return ulnSetStmtAttr((ulnStmt *)StatementHandle,
                          (acp_sint32_t)Attribute,
                          (void *)Value,
                          (acp_sint32_t)StringLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLSetStmtAttrW(SQLHSTMT   StatementHandle,
                                   SQLINTEGER Attribute,
                                   SQLPOINTER Value,
                                   SQLINTEGER StringLength)
{
    /*
     * StringLength
     *     [Input] If Attribute is an ODBC-defined attribute and ValuePtr points
     *     to a character string or a binary buffer, this argument should be the
     *     length of *ValuePtr. If Attribute is an ODBC-defined attribute and
     *     ValuePtr is an integer, StringLength is ignored.
     *     If Attribute is a driver-defined attribute, the application indicates
     *     the nature of the attribute to the Driver Manager by setting the
     *     StringLength argument. StringLength can have the following values:
     *     * If ValuePtr is a pointer to a character string, then StringLength
     *     is the length of the string or SQL_NTS.
     *     * If ValuePtr is a pointer to a binary buffer, then the application
     *     places the result of the SQL_LEN_BINARY_ATTR(length) macro in StringLength.
     *     This places a negative value in StringLength.
     *     * If ValuePtr is a pointer to a value other than a character string or
     *     a binary string, then StringLength should have the value SQL_IS_POINTER.
     *     * If ValuePtr contains a fixed-length value, then StringLength is either
     *     SQL_IS_INTEGER or SQL_IS_UINTEGER, as appropriate.
     */

    ULN_TRACE(SQLSetStmtAttrW);

    return ulnSetStmtAttr((ulnStmt *)StatementHandle,
                          (acp_sint32_t)Attribute,
                          (void *)Value,
                          (acp_sint32_t)StringLength);
}
#endif

SQLRETURN  SQL_API SQLGetStmtAttr(SQLHSTMT    StatementHandle,
                                  SQLINTEGER  Attribute,
                                  SQLPOINTER  Value,
                                  SQLINTEGER  BufferLength,
                                  SQLINTEGER *StringLength)
{
    ULN_TRACE(SQLGetStmtAttr);

    return ulnGetStmtAttr((ulnStmt *)StatementHandle,
                          (acp_sint32_t)Attribute,
                          (void *)Value,
                          (acp_sint32_t)BufferLength,
                          (acp_sint32_t *)StringLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetStmtAttrW(SQLHSTMT    StatementHandle,
                                   SQLINTEGER  Attribute,
                                   SQLPOINTER  Value,
                                   SQLINTEGER  BufferLength,
                                   SQLINTEGER *StringLength)
{
    /*
     * BufferLength
     *     [Input] If Attribute is an ODBC-defined attribute and ValuePtr points
     *     to a character string or a binary buffer, this argument should be the
     *     length of *ValuePtr. If Attribute is an ODBC-defined attribute and
     *     *ValuePtr is an integer, BufferLength is ignored. If the value returned
     *     in *ValuePtr is a Unicode string (when calling SQLGetStmtAttrW),
     *     the BufferLength argument must be an even number.
     *     If Attribute is a driver-defined attribute, the application indicates
     *     the nature of the attribute to the Driver Manager by setting the
     *     BufferLength argument. BufferLength can have the following values:
     *     * If *ValuePtr is a pointer to a character string, then BufferLength is
     *     the length of the string or SQL_NTS.
     *     * If *ValuePtr is a pointer to a binary buffer, then the application
     *     places the result of the SQL_LEN_BINARY_ATTR(length) macro in BufferLength.
     *     This places a negative value in BufferLength.
     *     * If *ValuePtr is a pointer to a value other than a character string or
     *     binary string, then BufferLength should have the value SQL_IS_POINTER.
     *     * If *ValuePtr is contains a fixed-length data type, then BufferLength is
     *     either SQL_IS_INTEGER or SQL_IS_UINTEGER, as appropriate.
     *
     * StringLengthPtr
     *     [Output] A pointer to a buffer in which to return the total number of bytes
     *     (excluding the null-termination character) available to return in *ValuePtr.
     *     If ValuePtr is a null pointer, no length is returned. If the attribute value
     *     is a character string, and the number of bytes available to return is greater
     *     than or equal to BufferLength, the data in *ValuePtr is truncated to
     *     BufferLength minus the length of a null-termination character and is
     *     null-terminated by the driver.
     */

    ULN_TRACE(SQLGetStmtAttrW);

    return ulnGetStmtAttr((ulnStmt *)StatementHandle,
                          (acp_sint32_t)Attribute,
                          (void *)Value,
                          (acp_sint32_t)BufferLength,
                          (acp_sint32_t *)StringLength);
}
#endif
#endif  /* ODBCVER >= 0x0300 */

SQLRETURN  SQL_API SQLSetStmtOption(SQLHSTMT     StatementHandle,
                                    SQLUSMALLINT Option,
                                    SQLROWCOUNT  Value)
{
    SQLSMALLINT sOptionInternal = (SQLSMALLINT)Option;

    ULN_TRACE(SQLSetStmtOption);

    switch (sOptionInternal)
    {
            /*
             * 32bit value 를 받는 attribute 들
             */
        case SQL_ATTR_APP_PARAM_DESC:
        case SQL_ATTR_APP_ROW_DESC:
        case SQL_ATTR_IMP_PARAM_DESC:
        case SQL_ATTR_IMP_ROW_DESC:
        case SQL_ATTR_CONCURRENCY:
        case SQL_ATTR_CURSOR_SCROLLABLE:
        case SQL_ATTR_CURSOR_SENSITIVITY:
        case SQL_ATTR_CURSOR_TYPE:
        case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
        case SQL_ATTR_PARAM_BIND_TYPE:
        case SQL_ATTR_PARAM_OPERATION_PTR:
        case SQL_ATTR_PARAM_STATUS_PTR:
        case SQL_ATTR_PARAMS_PROCESSED_PTR:
        case SQL_ATTR_PARAMSET_SIZE:
        case SQL_ATTR_QUERY_TIMEOUT:
        case SQL_ATTR_RETRIEVE_DATA:
        case SQL_ATTR_ROW_BIND_OFFSET_PTR:
        case SQL_ATTR_ROW_BIND_TYPE:
        case SQL_ATTR_ROW_NUMBER:
        case SQL_ATTR_ROW_OPERATION_PTR:
        case SQL_ATTR_ROW_STATUS_PTR:
        case SQL_ATTR_ROWS_FETCHED_PTR:

            /*
             * 64bit value 가 SQLGetStmtOption 시에 리턴된다고 되어 있는 속성들.
             * Set 할 때에는 구분할 필요가 없겠다.
             */
        case SQL_ATTR_MAX_ROWS:
        case SQL_ATTR_ROW_ARRAY_SIZE:
        case SQL_ATTR_KEYSET_SIZE:
            return ulnSetStmtAttr((ulnStmt *)StatementHandle,
                                  (acp_sint32_t)Option,
                                  (void *)((acp_ulong_t)Value),
                                  0);

            break;

            /*
             * string 을 받는 attribute 들
             */
        case SQL_ATTR_METADATA_ID:
            return ulnSetStmtAttr((ulnStmt *)StatementHandle,
                                  (acp_sint32_t)Option,
                                  (void *)((acp_ulong_t)Value),
                                  SQL_NTS);
            break;

        default:
            /*
             * BUGBUG : 드라이버 defined Attributes. 그리고, 위에서 커버하지 못한 나머지.
             */
            return ulnSetStmtAttr((ulnStmt *)StatementHandle,
                                  (acp_sint32_t)Option,
                                  (void *)((acp_ulong_t)Value),
                                  0);
            break;
    }
}

SQLRETURN  SQL_API SQLGetStmtOption(SQLHSTMT     StatementHandle,
                                    SQLUSMALLINT Option,
                                    SQLPOINTER   Value)
{
    ULN_TRACE(SQLGetStmtOption);

    /*
     * Note : M$ ODBC 에서는 string 이냐, 32bit integer 냐에 따라서 구분하라고 했지만,
     *        어차피, 내부에서는 구분하지 않기 때문에 그냥 하나로 매핑한다
     */
    return ulnGetStmtAttr((ulnStmt *)StatementHandle,
                          (acp_sint32_t)Option,
                          (void *)Value,
                          SQL_MAX_OPTION_STRING_LENGTH,
                          NULL);
}

SQLRETURN SQL_API SQLParamOptions(SQLHSTMT     StatementHandle,
                                  SQLULEN      ParamsetSize,
                                  SQLULEN     *ParamsProcessedPtr)
{
    SQLRETURN sReturn;

    ULN_TRACE(SQLParamOptions);

    sReturn = SQLSetStmtAttr((ulnStmt *)StatementHandle,
                             SQL_ATTR_PARAMSET_SIZE,
                             (void *)(acp_ulong_t)ParamsetSize,
                             0);

    if (sReturn == SQL_SUCCESS)
    {
        sReturn = SQLSetStmtAttr((ulnStmt *)StatementHandle,
                                 SQL_ATTR_PARAMS_PROCESSED_PTR,
                                 (void *)ParamsProcessedPtr,
                                 0);
    }

    return sReturn;
}

SQLRETURN SQL_API SQLSetScrollOptions(SQLHSTMT     StatementHandle,
                                      SQLUSMALLINT Concurrency,
                                      SQLLEN       KeySetSize,
                                      SQLUSMALLINT RowSetSize)
{
    ULN_TRACE(ulnSetScrollOptions);

    return ulnSetScrollOptions((ulnStmt*)StatementHandle,
                               (acp_uint16_t)  Concurrency,
                               (ulvSLen) KeySetSize,
                               (acp_uint16_t)  RowSetSize);
}

/*
 * =========================
 * Descriptors
 * =========================
 */

#if (ODBCVER >= 0x0300)

SQLRETURN  SQL_API SQLSetDescField(SQLHDESC    DescriptorHandle,
                                   SQLSMALLINT RecNumber,
                                   SQLSMALLINT FieldIdentifier,
                                   SQLPOINTER  Value,
                                   SQLINTEGER  BufferLength)
{
    ULN_TRACE(SQLSetDescField);

    return ulnSetDescField((ulnDesc *)DescriptorHandle,
                           (acp_sint16_t)RecNumber,
                           (acp_sint16_t)FieldIdentifier,
                           (void *)Value,
                           (acp_sint32_t)BufferLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLSetDescFieldW(SQLHDESC    DescriptorHandle,
                                    SQLSMALLINT RecNumber,
                                    SQLSMALLINT FieldIdentifier,
                                    SQLPOINTER  Value,
                                    SQLINTEGER  BufferLength)
{
    /*
     * BufferLength
     *     [Input] If FieldIdentifier is an ODBC-defined field and ValuePtr points
     *     to a character string or a binary buffer, this argument should be the
     *     length of *ValuePtr. For character string data, this argument should
     *     contain the number of bytes in the string.
     *     If FieldIdentifier is an ODBC-defined field and ValuePtr is an integer,
     *     BufferLength is ignored.
     *     If FieldIdentifier is a driver-defined field, the application indicates
     *     the nature of the field to the Driver Manager by setting the BufferLength
     *     argument. BufferLength can have the following values:
     *     * If ValuePtr is a pointer to a character string, then BufferLength is
     *     the length of the string or SQL_NTS.
     *     * If ValuePtr is a pointer to a binary buffer, then the application places
     *     the result of the SQL_LEN_BINARY_ATTR(length) macro in BufferLength. This
     *     places a negative value in BufferLength.
     *     * If ValuePtr is a pointer to a value other than a character string or a
     *     binary string, then BufferLength should have the value SQL_IS_POINTER.
     *     If ValuePtr contains a fixed-length value, then BufferLength is either
     *     SQL_IS_INTEGER, SQL_IS_UINTEGER, SQL_IS_SMALLINT, or SQL_IS_USMALLINT,
     *     as appropriate.
     */

    SQLRETURN     sRet = SQL_ERROR; /* BUG-46052 codesonar Uninitialized Variable */
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp = NULL;
    acp_sint32_t  sStep = 0;
    acp_sint32_t  sState = 0;

    ULN_TRACE(SQLSetDescFieldW);

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    switch(FieldIdentifier)
    {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
            if (Value != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, BufferLength * 2) != ACP_RC_SUCCESS);
                sStep = 1;

                ACI_TEST(ulnCharSetConvertUseBuffer(&sCharSet,
                                                    NULL,
                                                    DescriptorHandle,
                                                    (const mtlModule *)gWcharModule,
                                                    (const mtlModule *)gClientModule,
                                                    Value,
                                                    BufferLength,
                                                    (void*)sTemp,
                                                    BufferLength * 2,
                                                    CONV_DATA_IN)
                         != ACI_SUCCESS);

                Value = (void*)ulnCharSetGetConvertedText(&sCharSet);
                BufferLength = ulnCharSetGetConvertedTextLen(&sCharSet);
            }
            break;

        default:
            break;
    }

    sRet = ulnSetDescField((ulnDesc *)DescriptorHandle,
                           (acp_sint16_t)RecNumber,
                           (acp_sint16_t)FieldIdentifier,
                           (void *)Value,
                           (acp_sint32_t)BufferLength);

    if (sStep == 1)
    {
        acpMemFree(sTemp);
    }

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;
    {
        if (sStep == 1)
        {
            acpMemFree(sTemp);
            sRet = SQL_ERROR;
        }

        if (sState == 1)
        {
            ulnCharSetFinalize(&sCharSet);
        }
    }

    return sRet;
}
#endif

SQLRETURN SQL_API SQLGetDescRec(SQLHDESC     DescriptorHandle,
                                SQLSMALLINT  RecNumber,
                                SQLCHAR     *Name,
                                SQLSMALLINT  BufferLength,
                                SQLSMALLINT *StringLength,
                                SQLSMALLINT *Type,
                                SQLSMALLINT *SubType,
                                SQLLEN      *LengthPtr,
                                SQLSMALLINT *Precision,
                                SQLSMALLINT *Scale,
                                SQLSMALLINT *Nullable)
{
    ULN_TRACE(SQLGetDescRec);

    return ulnGetDescRec((ulnDesc *)DescriptorHandle,
                         (acp_sint16_t  )RecNumber,
                         (acp_char_t * )Name,
                         (acp_sint16_t  )BufferLength,
                         (acp_sint16_t *)StringLength,
                         (acp_sint16_t *)Type,
                         (acp_sint16_t *)SubType,
                         (ulvSLen *)LengthPtr,
                         (acp_sint16_t *)Precision,
                         (acp_sint16_t *)Scale,
                         (acp_sint16_t *)Nullable);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLGetDescRecW(SQLHDESC     DescriptorHandle,
                                 SQLSMALLINT  RecNumber,
                                 SQLWCHAR    *Name,
                                 SQLSMALLINT  BufferLength,
                                 SQLSMALLINT *StringLength,
                                 SQLSMALLINT *Type,
                                 SQLSMALLINT *SubType,
                                 SQLLEN      *LengthPtr,
                                 SQLSMALLINT *Precision,
                                 SQLSMALLINT *Scale,
                                 SQLSMALLINT *Nullable)
{
    /*
     * BufferLength
     *     [Input] Length of the *Name buffer, in characters.
     *
     * StringLengthPtr
     *     [Output] A pointer to a buffer in which to return the number of characters
     *     of data available to return in the *Name buffer, excluding the null-termination
     *     character. If the number of characters was greater than or equal to BufferLength,
     *     the data in *Name is truncated to BufferLength minus the length of a null-termination
     *     character, and is null-terminated by the driver.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint16_t  sLength = 0;

    ULN_TRACE(SQLGetDescRecW);

    ulnCharSetInitialize(&sCharSet);

    sRet = ulnGetDescRec((ulnDesc *)DescriptorHandle,
                         (acp_sint16_t  )RecNumber,
                         (acp_char_t * )Name,
                         (acp_sint16_t  )BufferLength,
                         (acp_sint16_t *)&sLength,
                         (acp_sint16_t *)Type,
                         (acp_sint16_t *)SubType,
                         (ulvSLen *)LengthPtr,
                         (acp_sint16_t *)Precision,
                         (acp_sint16_t *)Scale,
                         (acp_sint16_t *)Nullable);

    if (StringLength != NULL)
    {
        *StringLength = (SQLSMALLINT)sLength;
    }

    if (Name != NULL)
    {
        // fix BUG-24693
        // 입력받은 버퍼 크기만큼 변환을 한다.
        ACI_TEST(acpMemAlloc((void**)&sTemp, BufferLength + 1) != ACP_RC_SUCCESS);
        acpCStrCpy(sTemp,
                   BufferLength + 1,
                   (acp_char_t*)Name,
                   acpCStrLen((acp_char_t*)Name, ACP_SINT32_MAX));

        if (ulnCharSetConvertUseBuffer(&sCharSet,
                                       NULL,
                                       DescriptorHandle,
                                       (const mtlModule *)gClientModule,
                                       (const mtlModule *)gWcharModule,
                                       (void*)sTemp,
                                       acpCStrLen(sTemp, ACP_SINT32_MAX),
                                       Name,
                                       (BufferLength * ACI_SIZEOF(ulWChar)),
                                       CONV_DATA_OUT) != ACI_SUCCESS)
        {
            sLength = BufferLength - 1;
        }
        else
        {
            sLength = ulnCharSetGetConvertedTextLen(&sCharSet) / ACI_SIZEOF(ulWChar);
        }

        Name[sLength] = 0;

        acpMemFree(sTemp);
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLGetDescField(SQLHDESC    DescriptorHandle,
                                  SQLSMALLINT RecNumber,
                                  SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER  ValuePtr,
                                  SQLINTEGER  BufferLength,
                                  SQLINTEGER *StringLengthPtr)
{
    ULN_TRACE(SQLGetDescField);

    return ulnGetDescField((ulnDesc *)DescriptorHandle,
                           (acp_sint16_t)RecNumber,
                           (acp_sint16_t)FieldIdentifier,
                           (void *)ValuePtr,
                           (acp_sint32_t)BufferLength,
                           (acp_sint32_t *)StringLengthPtr);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLGetDescFieldW(SQLHDESC    DescriptorHandle,
                                   SQLSMALLINT RecNumber,
                                   SQLSMALLINT FieldIdentifier,
                                   SQLPOINTER  ValuePtr,
                                   SQLINTEGER  BufferLength,
                                   SQLINTEGER *StringLengthPtr)
{
    /*
     * BufferLength
     *     [Input] If FieldIdentifier is an ODBC-defined field and ValuePtr points to
     *     a character string or a binary buffer, this argument should be the length
     *     of *ValuePtr. If FieldIdentifier is an ODBC-defined field and *ValuePtr is
     *     an integer, BufferLength is ignored. If the value in *ValuePtr is of a
     *     Unicode data type (when calling SQLGetDescFieldW), the BufferLength argument
     *     must be an even number.
     *     If FieldIdentifier is a driver-defined field, the application indicates the
     *     nature of the field to the Driver Manager by setting the BufferLength argument.
     *     BufferLength can have the following values:
     *     * If *ValuePtr is a pointer to a character string, then BufferLength is the length
     *     of the string or SQL_NTS.
     *     * If *ValuePtr is a pointer to a binary buffer, then the application places
     *     the result of the SQL_LEN_BINARY_ATTR(length) macro in BufferLength. This places
     *     a negative value in BufferLength.
     *     * If *ValuePtr is a pointer to a value other than a character string or binary string,
     *     then BufferLength should have the value SQL_IS_POINTER.
     *     * If *ValuePtr is contains a fixed-length data type, then BufferLength is either
     *     SQL_IS_INTEGER, SQL_IS_UINTEGER, SQL_IS_SMALLINT, or SQL_IS_USMALLINT, as appropriate.
     *
     * StringLengthPtr
     *     [Output] Pointer to the buffer in which to return the total number of bytes
     *     (excluding the number of bytes required for the null-termination character)
     *     available to return in *ValuePtr.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint32_t  sLength = 0;

    ULN_TRACE(SQLGetDescFieldW);

    ulnCharSetInitialize(&sCharSet);

    sRet =  ulnGetDescField((ulnDesc *)DescriptorHandle,
                            (acp_sint16_t)RecNumber,
                            (acp_sint16_t)FieldIdentifier,
                            (void *)ValuePtr,
                            (acp_sint32_t)BufferLength,
                            (acp_sint32_t *)&sLength);

    if (StringLengthPtr != NULL)
    {
        *StringLengthPtr = sLength;
    }

    // fix BUG-24969
    // 위의 ulnGetDescField() 후 데이터에 변경이 없을 경우
    // 문자열 변환시 메모리를 덮어 쓸 수 있음.
    if ((sRet == SQL_SUCCESS) || (sRet == SQL_SUCCESS_WITH_INFO))
    {
        switch(FieldIdentifier)
        {
            case SQL_DESC_BASE_COLUMN_NAME:
            case SQL_DESC_BASE_TABLE_NAME:
            case SQL_DESC_CATALOG_NAME:
            case SQL_DESC_LABEL:
            case SQL_DESC_LITERAL_PREFIX:
            case SQL_DESC_LITERAL_SUFFIX:
            case SQL_DESC_LOCAL_TYPE_NAME:
            case SQL_DESC_NAME:
            case SQL_DESC_SCHEMA_NAME:
            case SQL_DESC_TABLE_NAME:
            case SQL_DESC_TYPE_NAME:
                if (ValuePtr != NULL)
                {
                    ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                    acpCStrCpy(sTemp,
                               sLength + 1,
                               (acp_char_t*)ValuePtr,
                               acpCStrLen((acp_char_t*)ValuePtr, ACP_SINT32_MAX));

                    if (ulnCharSetConvertUseBuffer(&sCharSet,
                                                   NULL,
                                                   DescriptorHandle,
                                                   (const mtlModule *)gClientModule,
                                                   (const mtlModule *)gWcharModule,
                                                   (void*)sTemp,
                                                   sLength,
                                                   ValuePtr,
                                                   BufferLength - ACI_SIZEOF(ulWChar),
                                                   CONV_DATA_OUT)
                        != ACI_SUCCESS)
                    {
                        sRet = SQL_SUCCESS_WITH_INFO;
                        sLength = BufferLength;
                        ((SQLWCHAR*)ValuePtr)[(BufferLength / ACI_SIZEOF(ulWChar)) - 1] = 0;
                    }
                    else
                    {
                        sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                        ((SQLWCHAR*)ValuePtr)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                    }

                    if (StringLengthPtr != NULL)
                    {
                        *StringLengthPtr = sLength;
                    }

                    acpMemFree(sTemp);
                }
                break;
            default:
                break;
        }
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

#endif /* ODBCVER >= 0x0300 */

/*
 * ============================
 * Bind Column
 * ============================
 */

/*
 * Note : unix-odbc 의 sqltypes.h 의 SQLLEN 에 대한 노트
 *
 *        I (Nick) have made these changes, to cope with the new 3.52 MS
 *        changes for 64 bit ODBC, but looking at MS's spec they havn't
 *        finished it themself. For example, SQLBindCol now expects the
 *        indicator variable to be a SQLLEN which then is a pointer to
 *        a 64 bit value. However the online book that comes with the
 *        headers, then goes on to describe the indicator_ptr in the
 *        descriptor record (which is set by SQLBindCol) as a pointer
 *        to a SQLINTEGER (32 bit). So I don't think its ready for the
 *        big time yet. Thats not to mention all the ODBC apps on 64 bit
 *        platforms that this would break...
 *
 *        I have just discovered that on win64 ACI_SIZEOF(long) == 4, so its
 *        all smoke and mirrors...
 */

SQLRETURN SQL_API SQLBindCol(SQLHSTMT     StatementHandle,
                             SQLUSMALLINT ColumnNumber,
                             SQLSMALLINT  TargetType,
                             SQLPOINTER   TargetValue,
                             SQLLEN       BufferLength,
                             SQLLEN      *StrLen_or_Ind)
{
    ULN_TRACE(SQLBindCol);

    return ulnBindCol((ulnStmt *)StatementHandle,
                      (acp_uint16_t  )ColumnNumber,
                      (acp_sint16_t  )TargetType,
                      (void *  )TargetValue,
                      (ulvSLen  )BufferLength,
                      (ulvSLen *)StrLen_or_Ind);
}

/*
 * ============================
 * Bind Parameter
 * ============================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT     StatementHandle,
                                   SQLUSMALLINT ParameterNumber,
                                   SQLSMALLINT  InputOutputType,
                                   SQLSMALLINT  ValueType,
                                   SQLSMALLINT  ParameterType,
                                   SQLULEN      ColumnSize,
                                   SQLSMALLINT  DecimalDigits,
                                   SQLPOINTER   ParameterValuePtr,
                                   SQLLEN       BufferLength,
                                   SQLLEN      *StrLen_or_IndPtr)
{
    ULN_TRACE(SQLBindParameter);

    return ulnBindParameter((ulnStmt *)StatementHandle,
                            (acp_uint16_t  )ParameterNumber,
                            (acp_char_t   *)NULL,
                            (acp_sint16_t  )InputOutputType,
                            (acp_sint16_t  )ValueType,
                            (acp_sint16_t  )ParameterType,
                            (ulvULen  )ColumnSize,
                            (acp_sint16_t  )DecimalDigits,
                            (void *  )ParameterValuePtr,
                            (ulvSLen  )BufferLength,
                            (ulvSLen *)StrLen_or_IndPtr);
}
#endif

SQLRETURN  SQL_API SQLSetParam(SQLHSTMT     StatementHandle,
                               SQLUSMALLINT ParameterNumber,
                               SQLSMALLINT  ValueType,
                               SQLSMALLINT  ParameterType,
                               SQLULEN      LengthPrecision,
                               SQLSMALLINT  ParameterScale,
                               SQLPOINTER   ParameterValue,
                               SQLLEN      *StrLen_or_Ind)
{
    ULN_TRACE(SQLSetParam);

    /*
     * BUGBUG : 제대로 매핑했나..;;
     */
    return ulnBindParameter((ulnStmt *)StatementHandle,
                            (acp_uint16_t  )ParameterNumber,
                            (acp_char_t   *)NULL,
                            SQL_PARAM_INPUT_OUTPUT,
                            (acp_sint16_t  )ValueType,
                            (acp_sint16_t  )ParameterType,
                            (ulvULen  )LengthPrecision,
                            (acp_sint16_t  )ParameterScale, // DecimalDigits
                            (void *  )ParameterValue,       // ParameterValuePtr
                            SQL_SETPARAM_VALUE_MAX,         // BufferLength
                            (ulvSLen *)StrLen_or_Ind);
}

SQLRETURN SQL_API SQLBindParam(SQLHSTMT     StatementHandle,
                               SQLUSMALLINT ParameterNumber,
                               SQLSMALLINT  ValueType,
                               SQLSMALLINT  ParameterType,
                               SQLULEN      LengthPrecision,
                               SQLSMALLINT  ParameterScale,
                               SQLPOINTER   ParameterValue,
                               SQLLEN      *StrLen_or_Ind)
{
    ULN_TRACE(SQLBindParam);

    /*
     * BUGBUG
     */
    return ulnBindParameter((ulnStmt *)StatementHandle,
                            (acp_uint16_t  )ParameterNumber,
                            (acp_char_t   *)NULL,
                            SQL_PARAM_INPUT,
                            (acp_sint16_t  )ValueType,
                            (acp_sint16_t  )ParameterType,
                            (ulvULen  )LengthPrecision,
                            (acp_sint16_t  )ParameterScale, // DecimalDigits
                            (void *  )ParameterValue,       // ParameterValuePtr
                            ULN_vLEN(0),                    // BufferLength
                            (ulvSLen *)StrLen_or_Ind);
}

/*
 * =================================
 * Describe Parameter and Column
 * =================================
 */

SQLRETURN SQL_API SQLDescribeCol(SQLHSTMT      StatementHandle,
                                 SQLUSMALLINT  ColumnNumber,
                                 SQLCHAR      *ColumnName,
                                 SQLSMALLINT   BufferLength,
                                 SQLSMALLINT  *NameLength,
                                 SQLSMALLINT  *DataType,
                                 SQLULEN      *ColumnSize,
                                 SQLSMALLINT  *DecimalDigits,
                                 SQLSMALLINT  *Nullable)
{
    ULN_TRACE(SQLDescribeCol);

    return ulnDescribeCol((ulnStmt *)StatementHandle,
                          (acp_uint16_t  )ColumnNumber,
                          (acp_char_t * )ColumnName,
                          (acp_sint16_t  )BufferLength,
                          (acp_sint16_t *)NameLength,
                          (acp_sint16_t *)DataType,
                          (ulvULen *)ColumnSize,
                          (acp_sint16_t *)DecimalDigits,
                          (acp_sint16_t *)Nullable);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLDescribeColW(SQLHSTMT      StatementHandle,
                                  SQLUSMALLINT  ColumnNumber,
                                  SQLWCHAR     *ColumnName,
                                  SQLSMALLINT   BufferLength,
                                  SQLSMALLINT  *NameLength,
                                  SQLSMALLINT  *DataType,
                                  SQLULEN      *ColumnSize,
                                  SQLSMALLINT  *DecimalDigits,
                                  SQLSMALLINT  *Nullable)
{
    /*
     * BufferLength
     *      [Input] Length of the *ColumnName buffer, in characters.
     *
     * NameLengthPtr
     *      [Output] Pointer to a buffer in which to return the total number of characters
     *      (excluding the null termination) available to return in *ColumnName.
     *      If the number of characters available to return is greater than or equal to BufferLength,
     *      the column name in *ColumnName is truncated to BufferLength minus the length of
     *      a null-termination character.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint16_t  sLength = 0;

    ULN_TRACE(SQLDescribeColW);

    ulnCharSetInitialize(&sCharSet);

    sRet = ulnDescribeCol((ulnStmt *)StatementHandle,
                          (acp_uint16_t  )ColumnNumber,
                          (acp_char_t * )ColumnName,
                          (acp_sint16_t  )BufferLength,
                          (acp_sint16_t *)&sLength,
                          (acp_sint16_t *)DataType,
                          (ulvULen *)ColumnSize,
                          (acp_sint16_t *)DecimalDigits,
                          (acp_sint16_t *)Nullable);

    if (NameLength != NULL)
    {
        *NameLength = (SQLSMALLINT)sLength;
    }

    if (ColumnName != NULL)
    {
        // fix BUG-24694
        // 입력받은 버퍼 크기만큼 변환을 한다.

        ACI_TEST(acpMemAlloc((void**)&sTemp, BufferLength + 1) != ACP_RC_SUCCESS);
        acpCStrCpy(sTemp,
                   BufferLength + 1,
                   (acp_char_t*)ColumnName,
                   acpCStrLen((acp_char_t*)ColumnName, ACP_SINT32_MAX));

        if (ulnCharSetConvertUseBuffer(&sCharSet,
                                       NULL,
                                       StatementHandle,
                                       (const mtlModule *)gClientModule,
                                       (const mtlModule *)gWcharModule,
                                       (void*)sTemp,
                                       acpCStrLen(sTemp, ACP_SINT32_MAX),
                                       ColumnName,
                                       (BufferLength * ACI_SIZEOF(ulWChar)),
                                       CONV_DATA_OUT) != ACI_SUCCESS)
        {
            sLength = BufferLength - 1;
        }
        else
        {
            sLength = ulnCharSetGetConvertedTextLen(&sCharSet) / ACI_SIZEOF(ulWChar);
        }

        ColumnName[sLength] = 0;

        acpMemFree(sTemp);
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLDescribeParam(SQLHSTMT      StatementHandle,
                                   SQLUSMALLINT  ParameterNumber,
                                   SQLSMALLINT  *DataTypePtr,
                                   SQLULEN      *ParameterSizePtr,
                                   SQLSMALLINT  *DecimalDigitsPtr,
                                   SQLSMALLINT  *NullablePtr)
{
    ULN_TRACE(SQLDescribeParam);

    return ulnDescribeParam((ulnStmt *)StatementHandle,
                            (acp_uint16_t)  ParameterNumber,
                            (acp_sint16_t *)DataTypePtr,
                            (ulvULen *)ParameterSizePtr,
                            (acp_sint16_t *)DecimalDigitsPtr,
                            (acp_sint16_t *)NullablePtr);
}

/*
 * ===================================
 * Transactions
 * ===================================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLEndTran(SQLSMALLINT HandleType,
                              SQLHANDLE   Handle,
                              SQLSMALLINT CompletionType)
{
    ULN_TRACE(SQLEndTran);
    return ulnEndTran((acp_sint16_t)HandleType, (ulnObject *)Handle, (acp_sint16_t)CompletionType);
}
#endif

SQLRETURN  SQL_API SQLTransact(SQLHENV EnvironmentHandle,
                               SQLHDBC ConnectionHandle,
                               SQLUSMALLINT CompletionType)
{
    ULN_TRACE(SQLTransact);
    if(ConnectionHandle != SQL_NULL_HANDLE)
    {
        return ulnEndTran(SQL_HANDLE_DBC,
                          (ulnObject *)ConnectionHandle,
                          (acp_sint16_t)CompletionType);
    }
    else if(EnvironmentHandle != SQL_NULL_HANDLE)
    {
        return ulnEndTran(SQL_HANDLE_ENV,
                          (ulnObject *)EnvironmentHandle,
                          (acp_sint16_t)CompletionType);
    }

    return SQL_INVALID_HANDLE;
}

/*
 * =======================================
 * Execution
 * =======================================
 */

SQLRETURN  SQL_API SQLPrepare(SQLHSTMT   StatementHandle,
                              SQLCHAR   *StatementText,
                              SQLINTEGER TextLength)
{
    ULN_TRACE(SQLPrepare);
    return ulnPrepare((ulnStmt *)StatementHandle,
                      (acp_char_t *)StatementText,
                      (acp_sint32_t)TextLength,
                      (acp_char_t *)NULL);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLPrepareW(SQLHSTMT   StatementHandle,
                               SQLWCHAR   *StatementText,
                               SQLINTEGER TextLength)
{
    /*
     * TextLength
     *     [Input] Length of *StatementText in characters.
     */

    ulnCharSet   sCharSet;
    acp_sint32_t sState = 0;
    SQLRETURN    sRet   = SQL_ERROR;

    ULN_TRACE(SQLPrepareW);

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    if (TextLength == SQL_NTS)
    {
        TextLength = getWcharLength(StatementText);
    }
    else
    {
        TextLength = TextLength * ACI_SIZEOF(ulWChar);
    }

    // BUG-24831 유니코드 드라이버에서 mtl::defaultModule() 을 호출하면 안됩니다.
    // 이곳에서 클라이언트 캐릭터셋을 얻을수 없다.
    // 인자로 NULL 을 넘겨주고 함수내부에서 구하는 방식

    ACI_TEST(ulnCharSetConvert(&sCharSet,
                               NULL,
                               StatementHandle,
                               (const mtlModule *)gWcharModule,
                               NULL,
                               (void*)StatementText,
                               TextLength,
                               CONV_DATA_IN)
             != ACI_SUCCESS);

    sRet = ulnPrepare((ulnStmt *)StatementHandle,
                      (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet),
                      (acp_sint32_t)ulnCharSetGetConvertedTextLen(&sCharSet),
                      (acp_char_t *)NULL);

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLExecDirect(SQLHSTMT   StatementHandle,
                                 SQLCHAR   *StatementText,
                                 SQLINTEGER TextLength)
{
    ULN_TRACE(SQLExecDirect);
    return ulnExecDirect((ulnStmt *)StatementHandle,
                         (acp_char_t *)StatementText,
                         (acp_sint32_t)TextLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLExecDirectW(SQLHSTMT   StatementHandle,
                                  SQLWCHAR  *StatementText,
                                  SQLINTEGER TextLength)
{
    /*
     * TextLength
     *      [Input] Length of *StatementText in characters.
     */

    ulnCharSet   sCharSet;
    acp_sint32_t sState = 0;
    SQLRETURN    sRet = SQL_ERROR;

    ULN_TRACE(SQLExecDirectW);

    ulnCharSetInitialize(&sCharSet);
    sState = 1;

    if (TextLength == SQL_NTS)
    {
        TextLength = getWcharLength(StatementText);
    }
    else
    {
        TextLength = TextLength * ACI_SIZEOF(ulWChar);
    }

    // BUG-24831 유니코드 드라이버에서 mtl::defaultModule() 을 호출하면 안됩니다.
    // 이곳에서 클라이언트 캐릭터셋을 얻을수 없다.
    // 인자로 NULL 을 넘겨주고 함수내부에서 구하는 방식
    ACI_TEST(ulnCharSetConvert(&sCharSet,
                               NULL,
                               StatementHandle,
                               (const mtlModule *)gWcharModule,
                               NULL,
                               (void*)StatementText,
                               TextLength,
                               CONV_DATA_IN)
             != ACI_SUCCESS);

    sRet = ulnExecDirect((ulnStmt *)StatementHandle,
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet),
                         (acp_sint32_t)ulnCharSetGetConvertedTextLen(&sCharSet));

    sState = 0;
    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLExecute(SQLHSTMT StatementHandle)
{
    ULN_TRACE(SQLExecute);
    return ulnExecute((ulnStmt *)StatementHandle);
}

SQLRETURN  SQL_API SQLNativeSql(SQLHDBC     ConnectionHandle,
                                SQLCHAR    *InStatementText,
                                SQLINTEGER  TextLength1,
                                SQLCHAR    *OutStatementText,
                                SQLINTEGER  BufferLength,
                                SQLINTEGER *TextLength2Ptr)
{
    ULN_TRACE(SQLNativeSql);
    return ulnNativeSql((ulnDbc *)ConnectionHandle,
                        (acp_char_t *) InStatementText,
                        (acp_sint32_t)    TextLength1,
                        (acp_char_t *) OutStatementText,
                        (acp_sint32_t)    BufferLength,
                        (acp_sint32_t *)  TextLength2Ptr);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLNativeSqlW(SQLHDBC     ConnectionHandle,
                                 SQLWCHAR   *InStatementText,
                                 SQLINTEGER  TextLength1,
                                 SQLWCHAR   *OutStatementText,
                                 SQLINTEGER  BufferLength,
                                 SQLINTEGER *TextLength2Ptr)
{
    /*
     * TextLength1
     *     [Input] Length in characters of *InStatementText text string.
     *
     * BufferLength
     *     [Input] Number of characters in the *OutStatementText buffer.
     *     If the value returned in *InStatementText is a Unicode string
     *     (when calling SQLNativeSqlW), the BufferLength argument must be
     *     an even number.
     *
     * TextLength2Ptr
     *     [Output] Pointer to a buffer in which to return the total number
     *     of characters (excluding null-termination) available to return in
     *     *OutStatementText. If the number of characters available to return
     *     is greater than or equal to BufferLength, the translated SQL string
     *     in *OutStatementText is truncated to BufferLength minus the length
     *     of a null-termination character.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSetIn;
    ulnCharSet    sCharSetOut;
    acp_char_t   *sTemp;
    acp_sint32_t  sLength = 0;
    acp_sint32_t  sState = 0;

    ULN_TRACE(SQLNativeSqlW);

    ulnCharSetInitialize(&sCharSetIn);
    ulnCharSetInitialize(&sCharSetOut);
    sState = 1;

    if (InStatementText != NULL)
    {
        if (TextLength1 == SQL_NTS)
        {
            TextLength1 = getWcharLength(InStatementText);
        }
        else
        {
            TextLength1 = TextLength1 * ACI_SIZEOF(ulWChar);
        }
    }

    // BUG-24831 유니코드 드라이버에서 mtl::defaultModule() 을 호출하면 안됩니다.
    // 이곳에서 클라이언트 캐릭터셋을 얻을수 없다.
    // 인자로 NULL 을 넘겨주고 함수내부에서 구하는 방식
    ACI_TEST(ulnCharSetConvert(&sCharSetIn,
                               NULL,
                               ConnectionHandle,
                               (const mtlModule *)gWcharModule,
                               NULL,
                               (void*)InStatementText,
                               TextLength1,
                               CONV_DATA_IN)
             != ACI_SUCCESS);

    sRet = ulnNativeSql((ulnDbc *)ConnectionHandle,
                        (acp_char_t *) ulnCharSetGetConvertedText(&sCharSetIn),
                        (acp_sint32_t)    ulnCharSetGetConvertedTextLen(&sCharSetIn),
                        (acp_char_t *) OutStatementText,
                        (acp_sint32_t)    BufferLength,
                        (acp_sint32_t *)  &sLength);

    if (TextLength2Ptr != NULL)
    {
        *TextLength2Ptr = sLength;
    }

    if (OutStatementText != NULL)
    {
        // fix BUG-24693
        // 입력받은 버퍼 크기만큼 변환을 한다.
        ACI_TEST(acpMemAlloc((void**)&sTemp, BufferLength + 1) != ACP_RC_SUCCESS);
        acpCStrCpy(sTemp,
                   BufferLength + 1,
                   (acp_char_t*)OutStatementText,
                   acpCStrLen((acp_char_t*)OutStatementText, ACP_SINT32_MAX));

        // BUG-24831 유니코드 드라이버에서 mtl::defaultModule() 을 호출하면 안됩니다.
        // 이곳에서 클라이언트 캐릭터셋을 얻을수 없다.
        // 인자로 NULL 을 넘겨주고 함수내부에서 구하는 방식
        if (ulnCharSetConvertUseBuffer(&sCharSetOut,
                                       NULL,
                                       ConnectionHandle,
                                       NULL,
                                       (const mtlModule *)gWcharModule,
                                       (void*)sTemp,
                                       acpCStrLen(sTemp, ACP_SINT32_MAX),
                                       OutStatementText,
                                       (BufferLength * ACI_SIZEOF(ulWChar)),
                                       CONV_DATA_OUT) != ACI_SUCCESS)
        {
            sLength = BufferLength - 1;
        }
        else
        {
            sLength = ulnCharSetGetConvertedTextLen(&sCharSetOut) / ACI_SIZEOF(ulWChar);
        }

        OutStatementText[sLength] = 0;

        acpMemFree(sTemp);
    }

    sState = 0;
    ulnCharSetFinalize(&sCharSetIn);
    ulnCharSetFinalize(&sCharSetOut);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSetIn);
        ulnCharSetFinalize(&sCharSetOut);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    ULN_TRACE(SQLNumResultCols);
    return ulnNumResultCols((ulnStmt *)StatementHandle, (acp_sint16_t *)ColumnCount);
}

SQLRETURN  SQL_API SQLNumParams(SQLHSTMT StatementHandle, SQLSMALLINT *ParamCount)
{
    ULN_TRACE(SQLNumParams);
    return ulnNumParams((ulnStmt *)StatementHandle, (acp_sint16_t *)ParamCount);
}

SQLRETURN SQL_API SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    ULN_TRACE(SQLRowCount);

    return ulnRowCount((ulnStmt *)StatementHandle, (ulvSLen *)RowCount);
}

/* BUG-44572 hidden function */
SQLRETURN SQL_API SQLNumRows(SQLHSTMT StatementHandle, SQLLEN *NumRows)
{
    ULN_TRACE(SQLNumRows);

    return ulnNumRows((ulnStmt *)StatementHandle, (ulvSLen *)NumRows);
}

SQLRETURN  SQL_API SQLMoreResults(SQLHSTMT StatementHandle)
{
    ULN_TRACE(SQLMoreResults);
    return ulnMoreResults((ulnStmt *)StatementHandle);
}

/*
 * =========================================
 * Fetch
 * =========================================
 */
SQLRETURN  SQL_API SQLFetch(SQLHSTMT StatementHandle)
{
    ULN_TRACE(SQLFetch);

    return ulnFetch((ulnStmt *)StatementHandle);
}

#if (ODBCVER >= 0x0300)
SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT     StatementHandle,
                                 SQLSMALLINT  FetchOrientation,
                                 SQLROWOFFSET FetchOffset)
{
    ULN_TRACE(SQLFetchScroll);

    return ulnFetchScroll((ulnStmt *)StatementHandle,
                          (acp_sint16_t)FetchOrientation,
                          (ulvSLen)FetchOffset);
}

#endif

SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT       hstmt,
                                   SQLUSMALLINT   fFetchType,
                                   SQLROWOFFSET   irow,
                                   SQLROWSETSIZE *pcrow,
                                   SQLUSMALLINT  *rgfRowStatus)
{
    ULN_TRACE(SQLExtendedFetch);

    /*
     * Note : 64bit odbc 에서 SQLROWSETSIZE 는 SQLUINTEGER 이다. 즉 32비트 정수이다.
     *        ExtendedFetch 의 4번째 parameter 는 64비트가 아니라 32비트이다.
     */

    return ulnExtendedFetch((ulnStmt *)hstmt,
                            (acp_uint16_t  )fFetchType,
                            (ulvSLen  )irow,
                            (acp_uint32_t   *)pcrow,
                            (acp_uint16_t *)rgfRowStatus);
}

/*
 * ================================
 * Get / Put Data
 * ================================
 */

SQLRETURN SQL_API SQLGetData(SQLHSTMT      StatementHandle,
                             SQLUSMALLINT  ColumnNumber,
                             SQLSMALLINT   TargetType,
                             SQLPOINTER    TargetValue,
                             SQLLEN        BufferLength,
                             SQLLEN       *StrLen_or_Ind)
{
    ULN_TRACE(SQLGetData);

    return ulnGetData((ulnStmt *)StatementHandle,
                      (acp_uint16_t  )ColumnNumber,
                      (acp_sint16_t  )TargetType,
                      (void *  )TargetValue,
                      (ulvSLen  )BufferLength,
                      (ulvSLen *)StrLen_or_Ind);
}

SQLRETURN SQL_API SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    ULN_TRACE(SQLPutData);

    return ulnPutData((ulnStmt *)StatementHandle, Data, (ulvSLen)StrLen_or_Ind);
}

SQLRETURN  SQL_API SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    ULN_TRACE(SQLParamData);

    return ulnParamData((ulnStmt *)StatementHandle, (void **)Value);
}

/*
 * =========================================
 * Diagnostic
 * =========================================
 */

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLGetDiagField(SQLSMALLINT  HandleType,
                                   SQLHANDLE    Handle,
                                   SQLSMALLINT  RecNumber,
                                   SQLSMALLINT  DiagIdentifier,
                                   SQLPOINTER   DiagInfo,
                                   SQLSMALLINT  BufferLength,
                                   SQLSMALLINT *StringLength)
{
    ULN_TRACE(SQLGetDiagField);
    return ulnGetDiagField((acp_sint16_t)HandleType,
                           (ulnObject *)Handle,
                           (acp_sint16_t)RecNumber,
                           (acp_sint16_t)DiagIdentifier,
                           (void *)DiagInfo,
                           (acp_sint16_t)BufferLength,
                           (acp_sint16_t *)StringLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetDiagFieldW(SQLSMALLINT  HandleType,
                                    SQLHANDLE    Handle,
                                    SQLSMALLINT  RecNumber,
                                    SQLSMALLINT  DiagIdentifier,
                                    SQLPOINTER   DiagInfo,
                                    SQLSMALLINT  BufferLength,
                                    SQLSMALLINT *StringLength)
{
    /*
     * BufferLength
     *     [Input] If DiagIdentifier is an ODBC-defined diagnostic and DiagInfoPtr
     *     points to a character string or a binary buffer, this argument should be
     *     the length of *DiagInfoPtr. If DiagIdentifier is an ODBC-defined field
     *     and *DiagInfoPtr is an integer, BufferLength is ignored. If the value
     *     in *DiagInfoPtr is a Unicode string (when calling SQLGetDiagFieldW),
     *     the BufferLength argument must be an even number.
     *     If DiagIdentifier is a driver-defined field, the application indicates
     *     the nature of the field to the Driver Manager by setting the BufferLength
     *     argument. BufferLength can have the following values:
     *     * If DiagInfoPtr is a pointer to a character string, BufferLength is the
     *     length of the string or SQL_NTS.
     *     * If DiagInfoPtr is a pointer to a binary buffer, the application places
     *     the result of the SQL_LEN_BINARY_ATTR(length) macro in BufferLength.
     *     This places a negative value in BufferLength.
     *     * If DiagInfoPtr is a pointer to a value other than a character string or
     *     binary string, BufferLength should have the value SQL_IS_POINTER.
     *     * If *DiagInfoPtr contains a fixed-length data type, BufferLength is
     *     SQL_IS_INTEGER, SQL_IS_UINTEGER, SQL_IS_SMALLINT, or SQL_IS_USMALLINT,
     *     as appropriate.
     *
     * StringLengthPtr
     *     [Output] Pointer to a buffer in which to return the total number of bytes
     *     (excluding the number of bytes required for the null-termination character)
     *     available to return in *DiagInfoPtr, for character data. If the number of
     *     bytes available to return is greater than or equal to BufferLength, the
     *     text in *DiagInfoPtr is truncated to BufferLength minus the length of
     *     a null-termination character.
     */

    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint16_t  sLength = 0;

    ULN_TRACE(SQLGetDiagFieldW);

    ulnCharSetInitialize(&sCharSet);

    if (StringLength != NULL)
    {
        *StringLength = 0;
    }

    sRet = ulnGetDiagField((acp_sint16_t)HandleType,
                           (ulnObject *)Handle,
                           (acp_sint16_t)RecNumber,
                           (acp_sint16_t)DiagIdentifier,
                           (void *)DiagInfo,
                           (acp_sint16_t)BufferLength,
                           (acp_sint16_t *)&sLength);

    if (sRet == SQL_SUCCESS)
    {
        if (StringLength != NULL)
        {
            *StringLength = (SQLSMALLINT)sLength;
        }

        switch(DiagIdentifier)
        {
            case SQL_DIAG_DYNAMIC_FUNCTION:
            case SQL_DIAG_CLASS_ORIGIN:
            case SQL_DIAG_CONNECTION_NAME:
            case SQL_DIAG_MESSAGE_TEXT:
            case SQL_DIAG_SERVER_NAME:
            case SQL_DIAG_SQLSTATE:
            case SQL_DIAG_SUBCLASS_ORIGIN:
                if (DiagInfo != NULL)
                {
                    ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                    acpCStrCpy(sTemp,
                               sLength + 1,
                               (acp_char_t*)DiagInfo,
                               acpCStrLen((acp_char_t*)DiagInfo, ACP_SINT32_MAX));

                    if (ulnCharSetConvertUseBuffer(&sCharSet,
                                                   NULL,
                                                   Handle,
                                                   (const mtlModule *)gClientModule,
                                                   (const mtlModule *)gWcharModule,
                                                   (void*)sTemp,
                                                   sLength,
                                                   DiagInfo,
                                                   BufferLength - ACI_SIZEOF(ulWChar),
                                                   CONV_DATA_OUT)
                        != ACI_SUCCESS)
                    {
                        sRet = SQL_SUCCESS_WITH_INFO;
                        sLength = BufferLength;
                        ((SQLWCHAR*)DiagInfo)[(BufferLength / ACI_SIZEOF(ulWChar)) - 1] = 0;
                    }
                    else
                    {
                        sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                        ((SQLWCHAR*)DiagInfo)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                    }

                    if (StringLength != NULL)
                    {
                        *StringLength = sLength;
                    }

                    acpMemFree(sTemp);
                }
                break;
            default:
                break;
        }
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLGetDiagRec(SQLSMALLINT  HandleType,
                                 SQLHANDLE    Handle,
                                 SQLSMALLINT  RecNumber,
                                 SQLCHAR     *Sqlstate,
                                 SQLINTEGER  *NativeError,
                                 SQLCHAR     *MessageText,
                                 SQLSMALLINT  BufferLength,
                                 SQLSMALLINT *TextLength)
{
    ULN_TRACE(SQLGetDiagRec);
    return ulnGetDiagRec((acp_sint16_t)HandleType,
                         (ulnObject *)Handle,
                         (acp_sint16_t)RecNumber,
                         (acp_char_t *)Sqlstate,
                         (acp_sint32_t *)NativeError,
                         (acp_char_t *)MessageText,
                         (acp_sint16_t)BufferLength,
                         (acp_sint16_t *)TextLength,
                         ACP_FALSE);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetDiagRecW(SQLSMALLINT  HandleType,
                                  SQLHANDLE    Handle,
                                  SQLSMALLINT  RecNumber,
                                  SQLWCHAR    *Sqlstate,
                                  SQLINTEGER  *NativeError,
                                  SQLWCHAR    *MessageText,
                                  SQLSMALLINT  BufferLength,
                                  SQLSMALLINT *TextLength)
{
    /*
     * BufferLength
     *     [Input] Length of the *MessageText buffer in characters. There is no
     *     maximum length of the diagnostic message text.
     *
     * TextLengthPtr
     *     [Output] Pointer to a buffer in which to return the total number of
     *     characters (excluding the number of characters required for the
     *     null-termination character) available to return in *MessageText.
     *     If the number of characters available to return is greater than
     *     BufferLength, the diagnostic message text in *MessageText is truncated
     *     to BufferLength minus the length of a null-termination character.
     */

    ulnCharSet    sMessageText;
    SQLCHAR       sSqlstate[6];
    acp_sint16_t  sLen  = 0;
    acp_char_t   *sTemp;
    SQLRETURN     sRet;

    ULN_TRACE(SQLGetDiagRecW);

    ulnCharSetInitialize(&sMessageText);

    sRet =  ulnGetDiagRec((acp_sint16_t)HandleType,
                          (ulnObject *)Handle,
                          (acp_sint16_t)RecNumber,
                          (acp_char_t *)sSqlstate,
                          (acp_sint32_t *)NativeError,
                          (acp_char_t *)MessageText,
                          (acp_sint16_t)BufferLength,
                          (acp_sint16_t *)&sLen,
                          ACP_FALSE);

    switch(sRet)
    {
        case SQL_NO_DATA:
            if (Sqlstate != NULL)
            {
                *Sqlstate = 0;
            }

            if ((MessageText != NULL) && (BufferLength > 0))
            {
                *MessageText = 0;
            }

            if (TextLength != NULL)
            {
                *TextLength = 0;
            }
            break;

        case SQL_SUCCESS:
            // BUG-22887 메시지 버퍼가 모자르면 SQL_SUCCESS_WITH_INFO 가 발생한다.
        case SQL_SUCCESS_WITH_INFO:
            // ASCII 와 WCHAR 은 다음과 같이 처리해도 무방하다.
            if(Sqlstate != NULL)
            {
                Sqlstate[0] = sSqlstate[0];
                Sqlstate[1] = sSqlstate[1];
                Sqlstate[2] = sSqlstate[2];
                Sqlstate[3] = sSqlstate[3];
                Sqlstate[4] = sSqlstate[4];
                Sqlstate[5] = 0;
            }

            if( (sLen > 0)          &&
                (BufferLength > 0 ) &&
                (MessageText != NULL) )
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLen + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLen + 1,
                           (acp_char_t*)MessageText,
                           acpCStrLen((acp_char_t*)MessageText, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sMessageText,
                                               NULL,
                                               Handle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLen,
                                               (void*)MessageText,
                                               (BufferLength - 1) * ACI_SIZEOF(ulWChar),
                                               CONV_DATA_OUT)
                    != ACI_SUCCESS)
                {
                    sRet = SQL_SUCCESS_WITH_INFO;
                    MessageText[BufferLength - 1] = 0;
                }
                else
                {
                    sLen = ulnCharSetGetConvertedTextLen(&sMessageText) / ACI_SIZEOF(ulWChar);
                    MessageText[sLen] = 0;
                }

                acpMemFree(sTemp);
            }

            if (TextLength != NULL)
            {
                *TextLength = sLen;
            }
            break;

        default:
            break;
    }

    ulnCharSetFinalize(&sMessageText);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif
#endif  /* ODBCVER >= 0x0300 */

SQLRETURN  SQL_API SQLError(SQLHENV      EnvironmentHandle,
                            SQLHDBC      ConnectionHandle,
                            SQLHSTMT     StatementHandle,
                            SQLCHAR     *Sqlstate,
                            SQLINTEGER  *NativeError,
                            SQLCHAR     *MessageText,
                            SQLSMALLINT  BufferLength,
                            SQLSMALLINT *TextLength)
{
    acp_sint16_t  sHandleType;
    acp_sint16_t  sRecNumber;
    ulnObject    *sHandle;
    SQLRETURN     sRetCode;

    ULN_TRACE(SQLError);

    if(StatementHandle != NULL)
    {
        sHandleType = SQL_HANDLE_STMT;
        sHandle     = (ulnObject *)StatementHandle;
    }
    else if(ConnectionHandle != NULL)
    {
        sHandleType = SQL_HANDLE_DBC;
        sHandle     = (ulnObject *)ConnectionHandle;
    }
    else if(EnvironmentHandle != NULL)
    {
        sHandleType = SQL_HANDLE_ENV;
        sHandle     = (ulnObject *)EnvironmentHandle;
    }
    else
    {
        return SQL_INVALID_HANDLE;
    }

    sRecNumber = ulnObjectGetSqlErrorRecordNumber(sHandle);

    sRetCode = ulnGetDiagRec(sHandleType,
                             sHandle,
                             sRecNumber,
                             (acp_char_t *)Sqlstate,
                             (acp_sint32_t *)NativeError,
                             (acp_char_t *)MessageText,
                             (acp_sint16_t)BufferLength,
                             (acp_sint16_t *)TextLength,
                             ACP_TRUE);

    sRecNumber = ulnObjectGetSqlErrorRecordNumber(sHandle);

    if(sRetCode == SQL_SUCCESS)
    {
        ulnObjectSetSqlErrorRecordNumber(sHandle, sRecNumber + 1);
    }

    return sRetCode;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLErrorW(SQLHENV      EnvironmentHandle,
                             SQLHDBC      ConnectionHandle,
                             SQLHSTMT     StatementHandle,
                             SQLWCHAR    *Sqlstate,
                             SQLINTEGER  *NativeError,
                             SQLWCHAR    *MessageText,
                             SQLSMALLINT  BufferLength,
                             SQLSMALLINT *TextLength)
{
    acp_sint16_t  sHandleType;
    acp_sint16_t  sRecNumber;
    ulnObject    *sHandle;
    SQLRETURN     sRetCode;

    ulnCharSet    sMessageText;
    SQLCHAR       sSqlstate[6];
    acp_sint16_t  sLen =  0;
    acp_char_t   *sTemp;

    ULN_TRACE(SQLErrorW);

    ulnCharSetInitialize(&sMessageText);

    if(StatementHandle != NULL)
    {
        sHandleType = SQL_HANDLE_STMT;
        sHandle     = (ulnObject *)StatementHandle;
    }
    else if(ConnectionHandle != NULL)
    {
        sHandleType = SQL_HANDLE_DBC;
        sHandle     = (ulnObject *)ConnectionHandle;
    }
    else if(EnvironmentHandle != NULL)
    {
        sHandleType = SQL_HANDLE_ENV;
        sHandle     = (ulnObject *)EnvironmentHandle;
    }
    else
    {
        return SQL_INVALID_HANDLE;
    }

    sRecNumber = ulnObjectGetSqlErrorRecordNumber(sHandle);

    sRetCode = ulnGetDiagRec(sHandleType,
                             sHandle,
                             sRecNumber,
                             (acp_char_t *)sSqlstate,
                             (acp_sint32_t *)NativeError,
                             (acp_char_t *)MessageText,
                             (acp_sint16_t)BufferLength,
                             (acp_sint16_t *)&sLen,
                             ACP_TRUE);

    switch(sRetCode)
    {
        case SQL_NO_DATA:
            if (Sqlstate != NULL)
            {
                *Sqlstate = 0;
            }

            if ((MessageText != NULL) && (BufferLength > 0))
            {
                *MessageText = 0;
            }

            if (TextLength != NULL)
            {
                *TextLength = 0;
            }
            break;

        case SQL_SUCCESS:
            // BUG-22887 메시지 버퍼가 모자르면 SQL_SUCCESS_WITH_INFO 가 발생한다.
        case SQL_SUCCESS_WITH_INFO:
            sRecNumber = ulnObjectGetSqlErrorRecordNumber(sHandle);
            ulnObjectSetSqlErrorRecordNumber(sHandle, sRecNumber + 1);

            // BUG-22887
            if(Sqlstate != NULL)
            {
                Sqlstate[0] = sSqlstate[0];
                Sqlstate[1] = sSqlstate[1];
                Sqlstate[2] = sSqlstate[2];
                Sqlstate[3] = sSqlstate[3];
                Sqlstate[4] = sSqlstate[4];
                Sqlstate[5] = 0;
            }

            if( (sLen > 0)          &&
                (BufferLength > 0 ) &&
                (MessageText != NULL) )
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLen + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLen + 1,
                           (acp_char_t*)MessageText,
                           acpCStrLen((acp_char_t*)MessageText, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sMessageText,
                                               NULL,
                                               sHandle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLen,
                                               (void*)MessageText,
                                               (BufferLength - 1) * ACI_SIZEOF(ulWChar),
                                               CONV_DATA_OUT) != ACI_SUCCESS)
                {
                    sRetCode = SQL_SUCCESS_WITH_INFO;
                    MessageText[BufferLength - 1] = 0;
                }
                else
                {
                    sLen = ulnCharSetGetConvertedTextLen(&sMessageText) / ACI_SIZEOF(ulWChar);
                    MessageText[sLen] = 0;
                }

                acpMemFree(sTemp);
            }

            if (TextLength != NULL)
            {
                // fix BUG-21731
                *TextLength = sLen * ACI_SIZEOF(ulWChar);
            }
            break;

        default:
            break;
    }

    ulnCharSetFinalize(&sMessageText);
    return sRetCode;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

/*
 * ===========================
 * Information
 * ===========================
 */

SQLRETURN  SQL_API SQLGetFunctions(SQLHDBC       ConnectionHandle,
                                   SQLUSMALLINT  FunctionId,
                                   SQLUSMALLINT *Supported)
{
    ULN_TRACE(SQLGetFunctions);
    return ulnGetFunctions((ulnDbc *)ConnectionHandle,
                           (acp_uint16_t)FunctionId,
                           (acp_uint16_t *)Supported);
}

SQLRETURN  SQL_API SQLGetInfo(SQLHDBC        ConnectionHandle,
                              SQLUSMALLINT   InfoType,
                              SQLPOINTER     InfoValue,
                              SQLSMALLINT    BufferLength,
                              SQLSMALLINT   *StringLength)
{
    ULN_TRACE(SQLGetInfo);
    return ulnGetInfo((ulnDbc *)ConnectionHandle,
                      (acp_uint16_t)InfoType,
                      (void *)InfoValue,
                      (acp_sint16_t)BufferLength,
                      (acp_sint16_t *)StringLength);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetInfoW(SQLHDBC        ConnectionHandle,
                               SQLUSMALLINT   InfoType,
                               SQLPOINTER     InfoValue,
                               SQLSMALLINT    BufferLength,
                               SQLSMALLINT   *StringLength)
{
    /*
     * BufferLength
     *     [Input] Length of the *InfoValuePtr buffer. If the value in *InfoValuePtr
     *     is not a character string or if InfoValuePtr is a null pointer, the
     *     BufferLength argument is ignored. The driver assumes that the size of
     *     *InfoValuePtr is SQLUSMALLINT or SQLUINTEGER, based on the InfoType. If
     *     *InfoValuePtr is a Unicode string (when calling SQLGetInfoW), the
     *     BufferLength argument must be an even number; if not, SQLSTATE HY090
     *     (Invalid string or buffer length) is returned.
     *
     * StringLengthPtr
     *     [Output] Pointer to a buffer in which to return the total number of bytes
     *     (excluding the null-termination character for character data) available
     *     to return in *InfoValuePtr.
     *     For character data, if the number of bytes available to return is greater
     *     than or equal to BufferLength, the information in *InfoValuePtr is
     *     truncated to BufferLength bytes minus the length of a null-termination
     *     character and is null-terminated by the driver.
     *     For all other types of data, the value of BufferLength is ignored and the
     *     driver assumes the size of *InfoValuePtr is SQLUSMALLINT or SQLUINTEGER,
     *     depending on the InfoType.
     */
    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint16_t  sLength = 0;

    ULN_TRACE(SQLGetInfoW);

    ulnCharSetInitialize(&sCharSet);

    sRet = ulnGetInfo((ulnDbc *)ConnectionHandle,
                      (acp_uint16_t)InfoType,
                      (void *)InfoValue,
                      (acp_sint16_t)BufferLength,
                      (acp_sint16_t *)&sLength);

    if (StringLength != NULL)
    {
        *StringLength = (SQLSMALLINT)sLength;
    }

    switch(InfoType)
    {
        case SQL_ACCESSIBLE_PROCEDURES:
        case SQL_ACCESSIBLE_TABLES:
        case SQL_CATALOG_NAME:
        case SQL_CATALOG_NAME_SEPARATOR:
        case SQL_CATALOG_TERM:
        case SQL_COLLATION_SEQ:
        case SQL_COLUMN_ALIAS:
        case SQL_DATA_SOURCE_NAME:
        case SQL_DATA_SOURCE_READ_ONLY:
        case SQL_DATABASE_NAME:
        case SQL_DBMS_NAME:
        case SQL_DBMS_VER:
        case SQL_DESCRIBE_PARAMETER:
        case SQL_DM_VER:
        case SQL_DRIVER_NAME:
        case SQL_DRIVER_ODBC_VER:
        case SQL_DRIVER_VER:
        case SQL_EXPRESSIONS_IN_ORDERBY:
        case SQL_IDENTIFIER_QUOTE_CHAR:
        case SQL_INTEGRITY:
        case SQL_KEYWORDS:
        case SQL_LIKE_ESCAPE_CLAUSE:
        case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
        case SQL_MULT_RESULT_SETS:
        case SQL_MULTIPLE_ACTIVE_TXN:
        case SQL_NEED_LONG_DATA_LEN:
        case SQL_ODBC_VER:
        case SQL_OUTER_JOINS:
        case SQL_ORDER_BY_COLUMNS_IN_SELECT:
        case SQL_PROCEDURE_TERM:
        case SQL_PROCEDURES:
        case SQL_ROW_UPDATES:
        case SQL_SCHEMA_TERM:
        case SQL_SEARCH_PATTERN_ESCAPE:
        case SQL_SERVER_NAME:
        case SQL_SPECIAL_CHARACTERS:
        case SQL_TABLE_TERM:
        case SQL_USER_NAME:
            if (InfoValue != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLength + 1,
                           (acp_char_t*)InfoValue,
                           acpCStrLen((acp_char_t*)InfoValue, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sCharSet,
                                               NULL,
                                               ConnectionHandle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLength,
                                               InfoValue,
                                               BufferLength - ACI_SIZEOF(ulWChar),
                                               CONV_DATA_OUT) != ACI_SUCCESS)
                {
                    sRet = SQL_SUCCESS_WITH_INFO;
                    sLength = BufferLength;
                    ((SQLWCHAR*)InfoValue)[(BufferLength / ACI_SIZEOF(ulWChar)) - 1] = 0;
                }
                else
                {
                    sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                    ((SQLWCHAR*)InfoValue)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                }


                if (StringLength != NULL)
                {
                    *StringLength = sLength;
                }

                acpMemFree(sTemp);
            }
            else
            {
                if (StringLength != NULL)
                {
                    *StringLength = sLength * ACI_SIZEOF(ulWChar);
                }
            }
            break;
        default:
            break;
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    ULN_TRACE(SQLGetTypeInfo);
    return ulnGetTypeInfo((ulnStmt *)StatementHandle, (acp_sint16_t)DataType);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    ULN_TRACE(SQLGetTypeInfoW);
    return ulnGetTypeInfo((ulnStmt *)StatementHandle, (acp_sint16_t)DataType);
}
#endif

/*
 * ===================================
 * Catalog Functions
 * ===================================
 */

SQLRETURN SQL_API SQLTables(SQLHSTMT     StatementHandle,
                            SQLCHAR     *CatalogName,
                            SQLSMALLINT  NameLength1,
                            SQLCHAR     *SchemaName,
                            SQLSMALLINT  NameLength2,
                            SQLCHAR     *TableName,
                            SQLSMALLINT  NameLength3,
                            SQLCHAR     *TableType,
                            SQLSMALLINT  NameLength4)
{
    ULN_TRACE(SQLTables);
    return ulnTables((ulnStmt *)StatementHandle,
                     (acp_char_t *)CatalogName,
                     (acp_sint16_t)NameLength1,
                     (acp_char_t *)SchemaName,
                     (acp_sint16_t)NameLength2,
                     (acp_char_t *)TableName,
                     (acp_sint16_t)NameLength3,
                     (acp_char_t *)TableType,
                     (acp_sint16_t)NameLength4);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLTablesW(SQLHSTMT     StatementHandle,
                            SQLWCHAR    *CatalogName,
                            SQLSMALLINT  NameLength1,
                            SQLWCHAR    *SchemaName,
                            SQLSMALLINT  NameLength2,
                            SQLWCHAR    *TableName,
                            SQLSMALLINT  NameLength3,
                            SQLWCHAR    *TableType,
                            SQLSMALLINT  NameLength4)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *TableName.
     *
     * NameLength4
     *     [Input] Length in characters of *TableType.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    ulnCharSet   sCharSet4;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLTablesW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    ulnCharSetInitialize(&sCharSet4);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(TableName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableType != NULL)
    {
        if (NameLength4 == SQL_NTS)
        {
            NameLength4 = getWcharLength(TableType);
        }
        else
        {
            NameLength4 =  NameLength4 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet4,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableType,
                                   NameLength4,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnTables((ulnStmt *)StatementHandle,
                     (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                     (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                     (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                     (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                     (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                     (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3),
                     (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet4),
                     (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet4));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);
    ulnCharSetFinalize(&sCharSet4);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
        ulnCharSetFinalize(&sCharSet4);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLColumns(SQLHSTMT       StatementHandle,
                              SQLCHAR       *CatalogName,
                              SQLSMALLINT    NameLength1,
                              SQLCHAR       *SchemaName,
                              SQLSMALLINT    NameLength2,
                              SQLCHAR       *TableName,
                              SQLSMALLINT    NameLength3,
                              SQLCHAR       *ColumnName,
                              SQLSMALLINT    NameLength4)
{
    ULN_TRACE(SQLColumns);
    return ulnColumns((ulnStmt *)StatementHandle,
                      (acp_char_t *)CatalogName,
                      (acp_sint16_t)NameLength1,
                      (acp_char_t *)SchemaName,
                      (acp_sint16_t)NameLength2,
                      (acp_char_t *)TableName,
                      (acp_sint16_t)NameLength3,
                      (acp_char_t *)ColumnName,
                      (acp_sint16_t)NameLength4);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLColumnsW(SQLHSTMT       StatementHandle,
                               SQLWCHAR      *CatalogName,
                               SQLSMALLINT    NameLength1,
                               SQLWCHAR      *SchemaName,
                               SQLSMALLINT    NameLength2,
                               SQLWCHAR      *TableName,
                               SQLSMALLINT    NameLength3,
                               SQLWCHAR      *ColumnName,
                               SQLSMALLINT    NameLength4)
{
    /*
     * NameLength1
     *      [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *      [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *      [Input] Length in characters of *TableName.
     *
     * NameLength4
     *      [Input] Length in characters of *ColumnName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    ulnCharSet   sCharSet4;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLColumnsW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    ulnCharSetInitialize(&sCharSet4);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(TableName);
        }
        else
        {
            NameLength3 = NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (ColumnName != NULL)
    {
        if (NameLength4 == SQL_NTS)
        {
            NameLength4 = getWcharLength(ColumnName);
        }
        else
        {
            NameLength4 = NameLength4 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet4,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)ColumnName,
                                   NameLength4,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet =  ulnColumns((ulnStmt *)StatementHandle,
                       (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                       (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                       (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                       (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                       (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                       (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3),
                       (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet4),
                       (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet4));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);
    ulnCharSetFinalize(&sCharSet4);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
        ulnCharSetFinalize(&sCharSet4);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLSpecialColumns(SQLHSTMT       StatementHandle,
                                     SQLUSMALLINT   IdentifierType,
                                     SQLCHAR       *CatalogName,
                                     SQLSMALLINT    NameLength1,
                                     SQLCHAR       *SchemaName,
                                     SQLSMALLINT    NameLength2,
                                     SQLCHAR       *TableName,
                                     SQLSMALLINT    NameLength3,
                                     SQLUSMALLINT   Scope,
                                     SQLUSMALLINT   Nullable)
{
    ULN_TRACE(SQLSpecialColumns);
    return ulnSpecialColumns((ulnStmt *)StatementHandle,
                             (acp_uint16_t )IdentifierType,
                             (acp_char_t *)CatalogName,
                             (acp_sint16_t )NameLength1,
                             (acp_char_t *)SchemaName,
                             (acp_sint16_t )NameLength2,
                             (acp_char_t *)TableName,
                             (acp_sint16_t )NameLength3,
                             (acp_uint16_t )Scope,
                             (acp_uint16_t )Nullable);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLSpecialColumnsW(SQLHSTMT       StatementHandle,
                                      SQLUSMALLINT   IdentifierType,
                                      SQLWCHAR      *CatalogName,
                                      SQLSMALLINT    NameLength1,
                                      SQLWCHAR      *SchemaName,
                                      SQLSMALLINT    NameLength2,
                                      SQLWCHAR      *TableName,
                                      SQLSMALLINT    NameLength3,
                                      SQLUSMALLINT   Scope,
                                      SQLUSMALLINT   Nullable)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *TableName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLSpecialColumnsW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(TableName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnSpecialColumns((ulnStmt *)StatementHandle,
                             (acp_uint16_t )IdentifierType,
                             (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                             (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                             (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                             (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                             (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                             (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3),
                             (acp_uint16_t )Scope,
                             (acp_uint16_t )Nullable);

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLStatistics(SQLHSTMT     StatementHandle,
                                 SQLCHAR     *CatalogName,
                                 SQLSMALLINT  NameLength1,
                                 SQLCHAR     *SchemaName,
                                 SQLSMALLINT  NameLength2,
                                 SQLCHAR     *TableName,
                                 SQLSMALLINT  NameLength3,
                                 SQLUSMALLINT Unique,
                                 SQLUSMALLINT Reserved)
{
    ULN_TRACE(SQLStatistics);
    return ulnStatistics((ulnStmt *)StatementHandle,
                         (acp_char_t *)CatalogName,
                         (acp_sint16_t)NameLength1,
                         (acp_char_t *)SchemaName,
                         (acp_sint16_t)NameLength2,
                         (acp_char_t *)TableName,
                         (acp_sint16_t)NameLength3,
                         (acp_uint16_t)Unique,
                         (acp_uint16_t)Reserved);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLStatisticsW(SQLHSTMT     StatementHandle,
                                  SQLWCHAR    *CatalogName,
                                  SQLSMALLINT  NameLength1,
                                  SQLWCHAR    *SchemaName,
                                  SQLSMALLINT  NameLength2,
                                  SQLWCHAR    *TableName,
                                  SQLSMALLINT  NameLength3,
                                  SQLUSMALLINT Unique,
                                  SQLUSMALLINT Reserved)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *TableName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLStatisticsW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(TableName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnStatistics((ulnStmt *)StatementHandle,
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                         (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                         (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                         (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3),
                         (acp_uint16_t)Unique,
                         (acp_uint16_t)Reserved);

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLProcedureColumns(SQLHSTMT       StatementHandle,
                                      SQLCHAR       *CatalogName,
                                      SQLSMALLINT    NameLength1,
                                      SQLCHAR       *SchemaName,
                                      SQLSMALLINT    NameLength2,
                                      SQLCHAR       *ProcName,
                                      SQLSMALLINT    NameLength3,
                                      SQLCHAR       *ColumnName,
                                      SQLSMALLINT    NameLength4)
{
    ULN_TRACE(SQLProcedureColumns);
    return ulnProcedureColumns((ulnStmt *)StatementHandle,
                               (acp_char_t *)CatalogName,
                               (acp_sint16_t )NameLength1,
                               (acp_char_t *)SchemaName,
                               (acp_sint16_t )NameLength2,
                               (acp_char_t *)ProcName,
                               (acp_sint16_t )NameLength3,
                               (acp_char_t *)ColumnName,
                               (acp_sint16_t )NameLength4,
                               ACP_FALSE); // BUG-23209 orderByPos option
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLProcedureColumnsW(SQLHSTMT       StatementHandle,
                                       SQLWCHAR      *CatalogName,
                                       SQLSMALLINT    NameLength1,
                                       SQLWCHAR      *SchemaName,
                                       SQLSMALLINT    NameLength2,
                                       SQLWCHAR      *ProcName,
                                       SQLSMALLINT    NameLength3,
                                       SQLWCHAR      *ColumnName,
                                       SQLSMALLINT    NameLength4)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *ProcName.
     *
     * NameLength4
     *     [Input] Length in characters of *ColumnName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    ulnCharSet   sCharSet4;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLProcedureColumnsW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    ulnCharSetInitialize(&sCharSet4);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (ProcName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(ProcName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)ProcName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (ColumnName != NULL)
    {
        if (NameLength4 == SQL_NTS)
        {
            NameLength4 = getWcharLength(ProcName);
        }
        else
        {
            NameLength4 =  NameLength4 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet4,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)ColumnName,
                                   NameLength4,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnProcedureColumns((ulnStmt *)StatementHandle,
                               (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                               (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                               (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                               (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                               (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                               (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3),
                               (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet4),
                               (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet4),
                               ACP_FALSE); // orderByPos option, BUG-23209
    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);
    ulnCharSetFinalize(&sCharSet4);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
        ulnCharSetFinalize(&sCharSet4);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLProcedures(SQLHSTMT     StatementHandle,
                                SQLCHAR     *CatalogName,
                                SQLSMALLINT  NameLength1,
                                SQLCHAR     *SchemaName,
                                SQLSMALLINT  NameLength2,
                                SQLCHAR     *ProcName,
                                SQLSMALLINT  NameLength3)
{
    ULN_TRACE(SQLProcedures);
    return ulnProcedures((ulnStmt *)StatementHandle,
                         (acp_char_t *)CatalogName,
                         (acp_sint16_t )NameLength1,
                         (acp_char_t *)SchemaName,
                         (acp_sint16_t )NameLength2,
                         (acp_char_t *)ProcName,
                         (acp_sint16_t )NameLength3);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLProceduresW(SQLHSTMT     StatementHandle,
                                 SQLWCHAR    *CatalogName,
                                 SQLSMALLINT  NameLength1,
                                 SQLWCHAR    *SchemaName,
                                 SQLSMALLINT  NameLength2,
                                 SQLWCHAR    *ProcName,
                                 SQLSMALLINT  NameLength3)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *ProcName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLProceduresW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (ProcName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(ProcName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)ProcName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnProcedures((ulnStmt *)StatementHandle,
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                         (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                         (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                         (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                         (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLForeignKeys(SQLHSTMT     StatementHandle,
                                 SQLCHAR     *PKCatalogName,
                                 SQLSMALLINT  NameLength1,
                                 SQLCHAR     *PKSchemaName,
                                 SQLSMALLINT  NameLength2,
                                 SQLCHAR     *PKTableName,
                                 SQLSMALLINT  NameLength3,
                                 SQLCHAR     *FKCatalogName,
                                 SQLSMALLINT  NameLength4,
                                 SQLCHAR     *FKSchemaName,
                                 SQLSMALLINT  NameLength5,
                                 SQLCHAR     *FKTableName,
                                 SQLSMALLINT  NameLength6)
{
    ULN_TRACE(SQLForeignKeys);
    return ulnForeignKeys((ulnStmt *)StatementHandle,
                          (acp_char_t *)PKCatalogName,   /* unused */
                          (acp_sint16_t )NameLength1,     /* unused */
                          (acp_char_t *)PKSchemaName,
                          (acp_sint16_t )NameLength2,
                          (acp_char_t *)PKTableName,
                          (acp_sint16_t )NameLength3,
                          (acp_char_t *)FKCatalogName,   /* unused */
                          (acp_sint16_t )NameLength4,     /* unused */
                          (acp_char_t *)FKSchemaName,
                          (acp_sint16_t )NameLength5,
                          (acp_char_t *)FKTableName,
                          (acp_sint16_t )NameLength6);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLForeignKeysW(SQLHSTMT     StatementHandle,
                                  SQLWCHAR    *PKCatalogName,
                                  SQLSMALLINT  NameLength1,
                                  SQLWCHAR    *PKSchemaName,
                                  SQLSMALLINT  NameLength2,
                                  SQLWCHAR    *PKTableName,
                                  SQLSMALLINT  NameLength3,
                                  SQLWCHAR    *FKCatalogName,
                                  SQLSMALLINT  NameLength4,
                                  SQLWCHAR    *FKSchemaName,
                                  SQLSMALLINT  NameLength5,
                                  SQLWCHAR    *FKTableName,
                                  SQLSMALLINT  NameLength6)
{
    /*
     * NameLength1
     *     [Input] Length of *PKCatalogName, in characters.
     *
     * NameLength2
     *     [Input] Length of *PKSchemaName, in characters.
     *
     * NameLength3
     *     [Input] Length of *PKTableName, in characters.
     *
     * NameLength4
     *     [Input] Length of *FKCatalogName, in characters.
     *
     * NameLength5
     *     [Input] Length of *FKSchemaName, in characters.
     *
     * NameLength6
     *     [Input] Length of *FKTableName, in characters.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    ulnCharSet   sCharSet4;
    ulnCharSet   sCharSet5;
    ulnCharSet   sCharSet6;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLForeignKeysW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    ulnCharSetInitialize(&sCharSet4);
    ulnCharSetInitialize(&sCharSet5);
    ulnCharSetInitialize(&sCharSet6);
    sState = 1;

    if (PKCatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(PKCatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)PKCatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (PKSchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(PKSchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)PKSchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (PKTableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(PKTableName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)PKTableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (FKCatalogName != NULL)
    {
        if (NameLength4 == SQL_NTS)
        {
            NameLength4 = getWcharLength(FKCatalogName);
        }
        else
        {
            NameLength4 = NameLength4 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet4,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)FKCatalogName,
                                   NameLength4,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (FKSchemaName != NULL)
    {
        if (NameLength5 == SQL_NTS)
        {
            NameLength5 = getWcharLength(FKSchemaName);
        }
        else
        {
            NameLength5 = NameLength5 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet5,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)FKSchemaName,
                                   NameLength5,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (FKTableName != NULL)
    {
        if (NameLength6 == SQL_NTS)
        {
            NameLength6 = getWcharLength(FKTableName);
        }
        else
        {
            NameLength6 =  NameLength6 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet6,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)FKTableName,
                                   NameLength6,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet =  ulnForeignKeys((ulnStmt *)StatementHandle,
                           (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                           (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                           (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                           (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                           (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                           (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3),
                           (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet4),
                           (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet4),
                           (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet5),
                           (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet5),
                           (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet6),
                           (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet6));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);
    ulnCharSetFinalize(&sCharSet4);
    ulnCharSetFinalize(&sCharSet5);
    ulnCharSetFinalize(&sCharSet6);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
        ulnCharSetFinalize(&sCharSet4);
        ulnCharSetFinalize(&sCharSet5);
        ulnCharSetFinalize(&sCharSet6);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLPrimaryKeys(SQLHSTMT     StatementHandle,
                                 SQLCHAR     *CatalogName,
                                 SQLSMALLINT  NameLength1,
                                 SQLCHAR     *SchemaName,
                                 SQLSMALLINT  NameLength2,
                                 SQLCHAR     *TableName,
                                 SQLSMALLINT  NameLength3)
{
    ULN_TRACE(SQLPrimaryKeys);
    return ulnPrimaryKeys((ulnStmt *)StatementHandle,
                          (acp_char_t *)CatalogName,
                          (acp_sint16_t )NameLength1,
                          (acp_char_t *)SchemaName,
                          (acp_sint16_t )NameLength2,
                          (acp_char_t *)TableName,
                          (acp_sint16_t )NameLength3);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLPrimaryKeysW(SQLHSTMT     StatementHandle,
                                  SQLWCHAR    *CatalogName,
                                  SQLSMALLINT  NameLength1,
                                  SQLWCHAR    *SchemaName,
                                  SQLSMALLINT  NameLength2,
                                  SQLWCHAR    *TableName,
                                  SQLSMALLINT  NameLength3)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *TableName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLPrimaryKeysW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(TableName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnPrimaryKeys((ulnStmt *)StatementHandle,
                          (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                          (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                          (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                          (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                          (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                          (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
    }

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLTablePrivileges(SQLHSTMT     StatementHandle,
                                     SQLCHAR     *CatalogName,
                                     SQLSMALLINT  NameLength1,
                                     SQLCHAR     *SchemaName,
                                     SQLSMALLINT  NameLength2,
                                     SQLCHAR     *TableName,
                                     SQLSMALLINT  NameLength3)
{
    ULN_TRACE(SQLTablePrivileges);
    return ulnTablePrivileges((ulnStmt *)StatementHandle,
                              (acp_char_t *)CatalogName,  /* unused */
                              (acp_sint16_t )NameLength1,  /* unused */
                              (acp_char_t *)SchemaName,
                              (acp_sint16_t )NameLength2,
                              (acp_char_t *)TableName,
                              (acp_sint16_t )NameLength3);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLTablePrivilegesW(SQLHSTMT     StatementHandle,
                                      SQLWCHAR    *CatalogName,
                                      SQLSMALLINT  NameLength1,
                                      SQLWCHAR    *SchemaName,
                                      SQLSMALLINT  NameLength2,
                                      SQLWCHAR    *TableName,
                                      SQLSMALLINT  NameLength3)
{
    /*
     * NameLength1
     *     [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *     [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *     [Input] Length in characters of *TableName.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLTablePrivilegesW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    sState = 1;

    if (CatalogName != NULL)
    {
        if (NameLength1 == SQL_NTS)
        {
            NameLength1 = getWcharLength(CatalogName);
        }
        else
        {
            NameLength1 = NameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)CatalogName,
                                   NameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (SchemaName != NULL)
    {
        if (NameLength2 == SQL_NTS)
        {
            NameLength2 = getWcharLength(SchemaName);
        }
        else
        {
            NameLength2 = NameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)SchemaName,
                                   NameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (TableName != NULL)
    {
        if (NameLength3 == SQL_NTS)
        {
            NameLength3 = getWcharLength(TableName);
        }
        else
        {
            NameLength3 =  NameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   StatementHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)TableName,
                                   NameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = ulnTablePrivileges((ulnStmt *)StatementHandle,
                              (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet1),
                              (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                              (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet2),
                              (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                              (acp_char_t *)ulnCharSetGetConvertedText(&sCharSet3),
                              (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSet1);
        ulnCharSetFinalize(&sCharSet2);
        ulnCharSetFinalize(&sCharSet3);
    }

    return SQL_ERROR;
}
#endif

/* PROJ-2177 User Interface - Cancel */
SQLRETURN  SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
    ULN_TRACE(SQLCancel);

    return ulnCancel((ulnStmt *)StatementHandle);
}

/*
 * ====================================
 * BUGBUG : 구현해야 하는 함수들
 * ====================================
 */

SQLSMALLINT ColAttributesMap(SQLUSMALLINT aFieldIdentifier)
{
    SQLUSMALLINT sFieldIdentifier;

    switch (aFieldIdentifier)
    {
        case SQL_COLUMN_COUNT:
            sFieldIdentifier = SQL_DESC_COUNT;
            break;
        case SQL_COLUMN_NAME:
            sFieldIdentifier = SQL_DESC_NAME;
            break;
        case SQL_COLUMN_TYPE:
            sFieldIdentifier = SQL_DESC_CONCISE_TYPE;
            break;
        case SQL_COLUMN_LENGTH:
            sFieldIdentifier = SQL_DESC_LENGTH;
            break;
        case SQL_COLUMN_PRECISION:
            sFieldIdentifier = SQL_DESC_PRECISION; /////////////
            break;
        case SQL_COLUMN_SCALE:
            sFieldIdentifier = SQL_DESC_SCALE; ////////////////
            break;
        case SQL_COLUMN_DISPLAY_SIZE:
            sFieldIdentifier = SQL_DESC_DISPLAY_SIZE;
            break;
        case SQL_COLUMN_NULLABLE:
            sFieldIdentifier = SQL_DESC_NULLABLE;
            break;
        case SQL_COLUMN_UNSIGNED:
            sFieldIdentifier = SQL_DESC_UNSIGNED;
            break;
        case SQL_COLUMN_MONEY:  /* BUGBUG : MSDN 에 안나옴 */
            sFieldIdentifier = SQL_DESC_FIXED_PREC_SCALE;
            break;
        case SQL_COLUMN_LABEL:
            sFieldIdentifier = SQL_DESC_LABEL;
            break;
        case SQL_COLUMN_UPDATABLE:
            sFieldIdentifier = SQL_DESC_UPDATABLE;
            break;
        case SQL_COLUMN_AUTO_INCREMENT:
            sFieldIdentifier = SQL_DESC_AUTO_UNIQUE_VALUE;
            break;
        case SQL_COLUMN_CASE_SENSITIVE:
            sFieldIdentifier = SQL_DESC_CASE_SENSITIVE;
            break;
        case SQL_COLUMN_SEARCHABLE:
            sFieldIdentifier = SQL_DESC_SEARCHABLE;
            break;
        case SQL_COLUMN_TYPE_NAME:
            sFieldIdentifier = SQL_DESC_TYPE_NAME;
            break;
        case SQL_COLUMN_TABLE_NAME:
            sFieldIdentifier = SQL_DESC_TABLE_NAME;
            break;
        case SQL_COLUMN_OWNER_NAME:
            sFieldIdentifier = SQL_DESC_SCHEMA_NAME;
            break;
        case SQL_COLUMN_QUALIFIER_NAME:
            sFieldIdentifier = SQL_DESC_CATALOG_NAME;
            break;
        default:
            sFieldIdentifier = aFieldIdentifier;
            break;
    }

    return sFieldIdentifier;
}

#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLColAttribute(SQLHSTMT     StatementHandle,
                                   SQLUSMALLINT ColumnNumber,
                                   SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER   CharacterAttribute,
                                   SQLSMALLINT  BufferLength,
                                   SQLSMALLINT *StringLength,
                                   SQLPOINTER   NumericAttribute)
{
    ULN_TRACE(SQLColAttribute);

    // fix BUG-30358
    // SQLColAttributes의 속성값을 SQLColAttribute 속성값으로 맵핑
    FieldIdentifier = ColAttributesMap(FieldIdentifier);

    return ulnColAttribute((ulnStmt *)StatementHandle,
                           (acp_uint16_t)   ColumnNumber,
                           (acp_uint16_t)   FieldIdentifier,
                           (void *)   CharacterAttribute,
                           (acp_sint16_t)   BufferLength,
                           (acp_sint16_t *) StringLength,
                           (void *)   NumericAttribute);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLColAttributeW(SQLHSTMT     StatementHandle,
                                    SQLUSMALLINT ColumnNumber,
                                    SQLUSMALLINT FieldIdentifier,
                                    SQLPOINTER   CharacterAttribute,
                                    SQLSMALLINT  BufferLength,
                                    SQLSMALLINT *StringLength,
                                    SQLPOINTER   NumericAttribute)
{
    /*
     * BufferLength
     *     [Input] If FieldIdentifier is an ODBC-defined field and CharacterAttributePtr
     *     points to a character string or binary buffer, this argument should be the
     *     length of *CharacterAttributePtr. If FieldIdentifier is an ODBC-defined field
     *     and *CharacterAttributePtr is an integer, this field is ignored. If the
     *     *CharacterAttributePtr is a Unicode string (when calling SQLColAttributeW),
     *     the BufferLength argument must be an even number. If FieldIdentifier is a
     *     driver-defined field, the application indicates the nature of the field to
     *     the Driver Manager by setting the BufferLength argument.
     *     BufferLength can have the following values:
     *
     *     * If CharacterAttributePtr is a pointer to a pointer, BufferLength should have
     *       the value SQL_IS_POINTER.
     *     * If CharacterAttributePtr is a pointer to a character string, the BufferLength
     *       is the length of the buffer.
     *     * If CharacterAttributePtr is a pointer to a binary buffer, the application
     *       places the result of the SQL_LEN_BINARY_ATTR(length) macro in BufferLength.
     *       This places a negative value in BufferLength.
     *       If CharacterAttributePtr is a pointer to a fixed-length data type, BufferLength
     *       must be one of the following: SQL_IS_INTEGER, SQL_IS_UNINTEGER, SQL_SMALLINT, or SQLUSMALLINT.
     *
     * StringLengthPtr
     *      [Output] Pointer to a buffer in which to return the total number of bytes
     *      (excluding the null-termination byte for character data) available to return in
     *      *CharacterAttributePtr.
     *      For character data, if the number of bytes available to return is greater than
     *      or equal to BufferLength, the descriptor information in *CharacterAttributePtr
     *      is truncated to BufferLength minus the length of a null-termination character
     *      and is null-terminated by the driver.
     *      For all other types of data, the value of BufferLength is ignored and the driver
     *      assumes the size of *CharacterAttributePtr is 32 bits.
     */

    SQLRETURN    sRet;
    ulnCharSet   sCharSet;
    acp_char_t  *sTemp;
    acp_sint16_t sLength = 0;

    ULN_TRACE(SQLColAttributeW);

    ulnCharSetInitialize(&sCharSet);

    // fix BUG-30358
    // SQLColAttributes의 속성값을 SQLColAttribute 속성값으로 맵핑
    FieldIdentifier = ColAttributesMap(FieldIdentifier);

    sRet = ulnColAttribute((ulnStmt *)StatementHandle,
                           (acp_uint16_t)   ColumnNumber,
                           (acp_uint16_t)   FieldIdentifier,
                           (void *)   CharacterAttribute,
                           (acp_sint16_t)   BufferLength,
                           (acp_sint16_t *) &sLength,
                           (void *)   NumericAttribute);

    if (StringLength != NULL)
    {
        *StringLength = (SQLSMALLINT)sLength;
    }

    switch(FieldIdentifier)
    {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
        case SQL_COLUMN_NAME:
            if (CharacterAttribute != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLength + 1,
                           (acp_char_t*)CharacterAttribute,
                           acpCStrLen((acp_char_t*)CharacterAttribute, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sCharSet,
                                               NULL,
                                               StatementHandle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLength,
                                               CharacterAttribute,
                                               BufferLength - ACI_SIZEOF(ulWChar),
                                               CONV_DATA_OUT) != ACI_SUCCESS)
                {
                    sRet = SQL_SUCCESS_WITH_INFO;
                    sLength = BufferLength;
                    ((SQLWCHAR*)CharacterAttribute)[(BufferLength / ACI_SIZEOF(ulWChar)) - 1] = 0;
                }
                else
                {
                    sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                    ((SQLWCHAR*)CharacterAttribute)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                }

                if (StringLength != NULL)
                {
                    *StringLength = sLength;
                }

                acpMemFree(sTemp);
            }
            break;
        default:
            break;
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif
#endif /* ODBCVER >= 0300 */

/*----------------------------------------------------------------*
 *
 * Description:
 *
 * Implementation:
 *
 *    // To Fix BUG-17521
 *    SQLAttributes() 함수는 SQLAttribute() 함수에 1:1 대응된다.
 *
 *---------------------------------------------------------------*/

// To Fix BUG-18286
// SQLColAttributes() 함수는 SQLColAttribute() 와 동일하나,
// 함수 Prototype에 있어 Platform 구별이 없다.
SQLRETURN SQL_API SQLColAttributes(SQLHSTMT     StatementHandle,
                                   SQLUSMALLINT ColumnNumber,
                                   SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER   CharacterAttribute,
                                   SQLSMALLINT  BufferLength,
                                   SQLSMALLINT *StringLength,
                                   SQLLEN      *NumericAttribute)
{
    ULN_TRACE(SQLColAttributes);

    // fix BUG-30358
    // SQLColAttributes의 속성값을 SQLColAttribute 속성값으로 맵핑
    FieldIdentifier = ColAttributesMap(FieldIdentifier);

    return ulnColAttribute((ulnStmt *)StatementHandle,
                           (acp_uint16_t)   ColumnNumber,
                           (acp_uint16_t)   FieldIdentifier,
                           (void *)   CharacterAttribute,
                           (acp_sint16_t)   BufferLength,
                           (acp_sint16_t *) StringLength,
                           (void *)   NumericAttribute);
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLColAttributesW(SQLHSTMT     StatementHandle,
                                   SQLUSMALLINT ColumnNumber,
                                   SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER   CharacterAttribute,
                                   SQLSMALLINT  BufferLength,
                                   SQLSMALLINT *StringLength,
                                   SQLLEN      *NumericAttribute)
{
    SQLRETURN     sRet;
    ulnCharSet    sCharSet;
    acp_char_t   *sTemp;
    acp_sint16_t  sLength = 0;

    ULN_TRACE(SQLColAttributesW);

    ulnCharSetInitialize(&sCharSet);

    // fix BUG-30358
    // SQLColAttributes의 속성값을 SQLColAttribute 속성값으로 맵핑
    FieldIdentifier = ColAttributesMap(FieldIdentifier);

    sRet = ulnColAttribute((ulnStmt *)StatementHandle,
                           (acp_uint16_t)   ColumnNumber,
                           (acp_uint16_t)   FieldIdentifier,
                           (void *)   CharacterAttribute,
                           (acp_sint16_t)   BufferLength,
                           (acp_sint16_t *) &sLength,
                           (void *)   NumericAttribute);

    if (StringLength != NULL)
    {
        *StringLength = (SQLSMALLINT)sLength;
    }

    switch(FieldIdentifier)
    {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
        case SQL_COLUMN_NAME:
            if (CharacterAttribute != NULL)
            {
                ACI_TEST(acpMemAlloc((void**)&sTemp, sLength + 1) != ACP_RC_SUCCESS);
                acpCStrCpy(sTemp,
                           sLength + 1,
                           (acp_char_t*)CharacterAttribute,
                           acpCStrLen((acp_char_t*)CharacterAttribute, ACP_SINT32_MAX));

                if (ulnCharSetConvertUseBuffer(&sCharSet,
                                               NULL,
                                               StatementHandle,
                                               (const mtlModule *)gClientModule,
                                               (const mtlModule *)gWcharModule,
                                               (void*)sTemp,
                                               sLength,
                                               CharacterAttribute,
                                               BufferLength - ACI_SIZEOF(ulWChar),
                                               CONV_DATA_OUT) != ACI_SUCCESS)
                {
                    sRet = SQL_SUCCESS_WITH_INFO;
                    sLength = BufferLength;
                    ((SQLWCHAR*)CharacterAttribute)[(BufferLength / ACI_SIZEOF(ulWChar)) - 1] = 0;
                }
                else
                {
                    sLength = ulnCharSetGetConvertedTextLen(&sCharSet);
                    ((SQLWCHAR*)CharacterAttribute)[sLength / ACI_SIZEOF(ulWChar)] = 0;
                }

                if (StringLength != NULL)
                {
                    *StringLength = sLength;
                }

                acpMemFree(sTemp);
            }
            break;
        default:
            break;
    }

    ulnCharSetFinalize(&sCharSet);

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLDataSources(SQLHENV        EnvironmentHandle,
                                  SQLUSMALLINT   Direction,
                                  SQLCHAR      * ServerName,
                                  SQLSMALLINT    BufferLength1,
                                  SQLSMALLINT  * NameLength1,
                                  SQLCHAR      * Description,
                                  SQLSMALLINT    BufferLength2,
                                  SQLSMALLINT  * NameLength2)
{
    ULN_TRACE(SQLDataSources);

    ACP_UNUSED(EnvironmentHandle);
    ACP_UNUSED(Direction);
    ACP_UNUSED(ServerName);
    ACP_UNUSED(BufferLength1);
    ACP_UNUSED(NameLength1);
    ACP_UNUSED(Description);
    ACP_UNUSED(BufferLength2);
    ACP_UNUSED(NameLength2);

    /*
     * BUGBUG : 함수 구현 : old sqlcli 에도 그냥 SQL_ERROR 리턴함.
     */
    return SQL_ERROR;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLDataSourcesW(SQLHENV        EnvironmentHandle,
                                   SQLUSMALLINT   Direction,
                                   SQLWCHAR     * ServerName,
                                   SQLSMALLINT    BufferLength1,
                                   SQLSMALLINT  * NameLength1,
                                   SQLWCHAR     * Description,
                                   SQLSMALLINT    BufferLength2,
                                   SQLSMALLINT  * NameLength2)
{
    /*
     * BufferLength1
     *      [Input] Length of the *ServerName buffer, in characters; this does not
     *      need to be longer than SQL_MAX_DSN_LENGTH plus the null-termination character.
     *
     * NameLength1Ptr
     *      [Output] Pointer to a buffer in which to return the total number of characters
     *      (excluding the null-termination character) available to return in *ServerName.
     *      If the number of characters available to return is greater than or equal to
     *      BufferLength1, the data source name in *ServerName is truncated to BufferLength1
     *      minus the length of a null-termination character.
     *
     * BufferLength2
     *      [Input] Length in characters of the *Description buffer.
     *
     * NameLength2Ptr
     *      [Output] Pointer to a buffer in which to return the total number of characters
     *      (excluding the null-termination character) available to return in *Description.
     *      If the number of characters available to return is greater than or equal to
     *      BufferLength2, the driver description in *Description is truncated to BufferLength2
     *      minus the length of a null-termination character.
     */
    ULN_TRACE(SQLDataSourcesW);

    ACP_UNUSED(EnvironmentHandle);
    ACP_UNUSED(Direction);
    ACP_UNUSED(ServerName);
    ACP_UNUSED(BufferLength1);
    ACP_UNUSED(NameLength1);
    ACP_UNUSED(Description);
    ACP_UNUSED(BufferLength2);
    ACP_UNUSED(NameLength2);

    /*
     * BUGBUG : 함수 구현 : old sqlcli 에도 그냥 SQL_ERROR 리턴함.
     */
    return SQL_ERROR;
}
#endif

/*
 * Note : CursorName 은 Positioned update, delete 를 사용할 때에만 필요하다.
 *        그것을 지원하지 않는 지금은, 구현할 필요가 없으나,
 *        차후에 구현을 해야 한다.
 */
SQLRETURN  SQL_API SQLSetCursorName(SQLHSTMT     StatementHandle,
                                    SQLCHAR     * CursorName,
                                    SQLSMALLINT  NameLength)
{
    ULN_TRACE(SQLSetCursorName);

    ACP_UNUSED(StatementHandle);
    ACP_UNUSED(CursorName);
    ACP_UNUSED(NameLength);

    /*
     * BUGBUG : old sqlcli 에 있다. 구현필요!
     */

    return SQL_ERROR;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLSetCursorNameW(SQLHSTMT     StatementHandle,
                                     SQLWCHAR    *CursorName,
                                     SQLSMALLINT  NameLength)
{
    /*
     * NameLength
     *     [Input] Length in characters of *CursorName.
     */

    ULN_TRACE(SQLSetCursorNameW);

    ACP_UNUSED(StatementHandle);
    ACP_UNUSED(CursorName);
    ACP_UNUSED(NameLength);

    /*
     * BUGBUG : old sqlcli 에 있다. 구현필요!
     */

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLGetCursorName(SQLHSTMT     StatementHandle,
                                    SQLCHAR     *CursorName,
                                    SQLSMALLINT  BufferLength,
                                    SQLSMALLINT *NameLength)
{
    ULN_TRACE(SQLGetCursorName);

    ACP_UNUSED(StatementHandle);
    ACP_UNUSED(CursorName);
    ACP_UNUSED(BufferLength);
    ACP_UNUSED(NameLength);

    /*
     * BUGBUG : old sqlcli 에 있다. 구현필요!
     */

    return SQL_ERROR;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN  SQL_API SQLGetCursorNameW(SQLHSTMT     StatementHandle,
                                     SQLWCHAR    *CursorName,
                                     SQLSMALLINT  BufferLength,
                                     SQLSMALLINT *NameLength)
{
    /*
     * BufferLength
     *     [Input] Length of *CursorName, in characters. If the value in *CursorName
     *     is a Unicode string (when calling SQLGetCursorNameW), the BufferLength argument
     *     must be an even number.
     *
     * NameLengthPtr
     *     [Output] Pointer to memory in which to return the total number of characters
     *     (excluding the null-termination character) available to return in *CursorName.
     *     If the number of characters available to return is greater than or equal to
     *     BufferLength, the cursor name in *CursorName is truncated to BufferLength minus
     *     the length of a null-termination character.
     */

    ULN_TRACE(SQLGetCursorNameW);

    ACP_UNUSED(StatementHandle);
    ACP_UNUSED(CursorName);
    ACP_UNUSED(BufferLength);
    ACP_UNUSED(NameLength);

    /*
     * BUGBUG : old sqlcli 에 있다. 구현필요!
     */

    return SQL_ERROR;
}
#endif

SQLRETURN SQL_API SQLColumnPrivileges(SQLHSTMT     StatementHandle,
                                      SQLCHAR    * CatalogName,
                                      SQLSMALLINT  NameLength1,
                                      SQLCHAR    * SchemaName,
                                      SQLSMALLINT  NameLength2,
                                      SQLCHAR    * TableName,
                                      SQLSMALLINT  NameLength3,
                                      SQLCHAR    * ColumnName,
                                      SQLSMALLINT  NameLength4)
{
    ULN_TRACE(SQLColumnPrivileges);

    ACP_UNUSED(StatementHandle);
    ACP_UNUSED(CatalogName);
    ACP_UNUSED(NameLength1);
    ACP_UNUSED(SchemaName);
    ACP_UNUSED(NameLength2);
    ACP_UNUSED(TableName);
    ACP_UNUSED(NameLength3);
    ACP_UNUSED(ColumnName);
    ACP_UNUSED(NameLength4);

    /*
     * BUGBUG : 구현 해야 한다. 그러나, old cli2 에도 없었으므로 일단 넘어가자.
     */

    return SQL_ERROR;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLColumnPrivilegesW(SQLHSTMT     StatementHandle,
                                       SQLWCHAR   * CatalogName,
                                       SQLSMALLINT  NameLength1,
                                       SQLWCHAR   * SchemaName,
                                       SQLSMALLINT  NameLength2,
                                       SQLWCHAR   * TableName,
                                       SQLSMALLINT  NameLength3,
                                       SQLWCHAR   * ColumnName,
                                       SQLSMALLINT  NameLength4)
{
    /*
     * NameLength1
     *      [Input] Length in characters of *CatalogName.
     *
     * NameLength2
     *      [Input] Length in characters of *SchemaName.
     *
     * NameLength3
     *      [Input] Length in characters of *TableName.
     *
     * NameLength4
     *      [Input] Length in characters of *ColumnName.
     */

    ULN_TRACE(SQLColumnPrivilegesW);

    ACP_UNUSED(StatementHandle);
    ACP_UNUSED(CatalogName);
    ACP_UNUSED(NameLength1);
    ACP_UNUSED(SchemaName);
    ACP_UNUSED(NameLength2);
    ACP_UNUSED(TableName);
    ACP_UNUSED(NameLength3);
    ACP_UNUSED(ColumnName);
    ACP_UNUSED(NameLength4);

    /*
     * BUGBUG : 구현 해야 한다. 그러나, old cli2 에도 없었으므로 일단 넘어가자.
     */

    return SQL_ERROR;
}
#endif

SQLRETURN  SQL_API SQLDisconnect(SQLHDBC ConnectionHandle)
{
    ULN_TRACE(SQLDisconnect);
    /*
     * BUGBUG : 이걸 여기다 둬도 될래나 -_-;;
     */
    return ulnDisconnect((ulnDbc *)ConnectionHandle);
}

#if (ODBCVER >= 0x0300)
/*
 * Note : 일단 아래의 Descriptor 관련된 함수들은 급하게 필요한 것이 아니므로
 *        일단 그냥 두고, 천천히 만들도록 하자.
 *        우선은 당장 필요한 SetDescField 만 필요한 attribute 에 대해서만 구현해 두었음.
 */
SQLRETURN  SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle,
                               SQLHDESC TargetDescHandle)
{
    ULN_TRACE(SQLCopyDesc);

    ACP_UNUSED(SourceDescHandle);
    ACP_UNUSED(TargetDescHandle);

    /*
     * BUGBUG : 함수구현
     */

    return SQL_ERROR;
}

SQLRETURN  SQL_API SQLSetDescRec(SQLHDESC     DescriptorHandle,
                                 SQLSMALLINT  RecNumber,
                                 SQLSMALLINT  Type,
                                 SQLSMALLINT  SubType,
                                 SQLLEN       Length,
                                 SQLSMALLINT  Precision,
                                 SQLSMALLINT  Scale,
                                 SQLPOINTER   Data,
                                 SQLLEN      *StringLength,
                                 SQLLEN      *Indicator)
{
    ULN_TRACE(SQLSetDescRec);

    ACP_UNUSED(DescriptorHandle);
    ACP_UNUSED(RecNumber);
    ACP_UNUSED(Type);
    ACP_UNUSED(SubType);
    ACP_UNUSED(Length);
    ACP_UNUSED(Precision);
    ACP_UNUSED(Scale);
    ACP_UNUSED(Data);
    ACP_UNUSED(StringLength);
    ACP_UNUSED(Indicator);

    /*
     * BUGBUG : 함수 구현
     */

    return SQL_ERROR;
}


SQLRETURN SQL_API SQLSetPos(SQLHSTMT      hstmt,
                            SQLSETPOSIROW irow,
                            SQLUSMALLINT  fOption,
                            SQLUSMALLINT  fLock)
{
    ULN_TRACE(SQLSetPos);

    return ulnSetPos((ulnStmt *) hstmt,
                     (acp_uint16_t) irow,
                     (acp_uint16_t) fOption,
                     (acp_uint16_t) fLock);
}

SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT    aStatementHandle,
                                    SQLSMALLINT aOperation)
{
    ULN_TRACE(SQLBulkOperations);

    return ulnBulkOperations((ulnStmt *) aStatementHandle,
                             (acp_sint16_t) aOperation);
}

#endif

/* ------------------------------------------------
 *  SQLConnect
 * ----------------------------------------------*/

extern ulnSQLConnectFrameWork gSQLConnectModule;

SQLRETURN SQL_API SQLConnect(SQLHDBC      aConnectionHandle,
                             SQLCHAR     *aServerName,
                             SQLSMALLINT  aNameLength1,
                             SQLCHAR     *aUserName,
                             SQLSMALLINT  aNameLength2,
                             SQLCHAR     *aAuthentication,
                             SQLSMALLINT  aNameLength3)
{
    SQLRETURN sRet;

    ULN_TRACE(SQLConnect);

    sRet = SQL_SUCCESS;


    // Getting private Function
    ACI_TEST(gSQLConnectModule.mSetup((ulnDbc*)aConnectionHandle)
             != ACI_SUCCESS);

    sRet = gSQLConnectModule.mConnect(
                      (ulnDbc *)aConnectionHandle,
                      (acp_char_t*)aServerName,
                      aNameLength1,
                      (acp_char_t*)aUserName,
                      aNameLength2,
                      (acp_char_t*)aAuthentication,
                      aNameLength3);
    return sRet;

    ACI_EXCEPTION_END;
    {
        //BUGBUG-TODO What Description Record ??
    }

    return SQL_ERROR;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLConnectW(SQLHDBC      aConnectionHandle,
                              SQLWCHAR    *aServerName,
                              SQLSMALLINT  aNameLength1,
                              SQLWCHAR    *aUserName,
                              SQLSMALLINT  aNameLength2,
                              SQLWCHAR    *aAuthentication,
                              SQLSMALLINT  aNameLength3)
{
    /*
     * NameLength1
     *      [Input] Length of *ServerName in characters.
     *
     * NameLength2
     *      [Input] Length of *UserName in characters.
     *
     * NameLength3
     *      [Input] Length of *Authentication in characters.
     */
    SQLRETURN    sRet;
    ulnCharSet   sCharSet1;
    ulnCharSet   sCharSet2;
    ulnCharSet   sCharSet3;
    acp_sint32_t sState = 0;

    ULN_TRACE(SQLConnectW);

    ulnCharSetInitialize(&sCharSet1);
    ulnCharSetInitialize(&sCharSet2);
    ulnCharSetInitialize(&sCharSet3);
    sState = 1;

    sRet = SQL_SUCCESS;


    // Getting private Function
    ACI_TEST(gSQLConnectModule.mSetup((ulnDbc*)aConnectionHandle)
             != ACI_SUCCESS);

    if (aServerName != NULL)
    {
        if (aNameLength1 == SQL_NTS)
        {
            aNameLength1 = getWcharLength(aServerName);
        }
        else
        {
            aNameLength1 = aNameLength1 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet1,
                                   NULL,
                                   aConnectionHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)aServerName,
                                   aNameLength1,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (aUserName != NULL)
    {
        if (aNameLength2 == SQL_NTS)
        {
            aNameLength2 = getWcharLength(aUserName);
        }
        else
        {
            aNameLength2 = aNameLength2 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet2,
                                   NULL,
                                   aConnectionHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)aUserName,
                                   aNameLength2,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    if (aAuthentication != NULL)
    {
        if (aNameLength3 == SQL_NTS)
        {
            aNameLength3 = getWcharLength(aAuthentication);
        }
        else
        {
            aNameLength3 = aNameLength3 * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSet3,
                                   NULL,
                                   aConnectionHandle,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)aAuthentication,
                                   aNameLength3,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sRet = gSQLConnectModule.mConnect(
                      (ulnDbc *)aConnectionHandle,
                      (acp_char_t*)ulnCharSetGetConvertedText(&sCharSet1),
                      (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet1),
                      (acp_char_t*)ulnCharSetGetConvertedText(&sCharSet2),
                      (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet2),
                      (acp_char_t*)ulnCharSetGetConvertedText(&sCharSet3),
                      (acp_sint16_t)ulnCharSetGetConvertedTextLen(&sCharSet3));

    sState = 0;
    ulnCharSetFinalize(&sCharSet1);
    ulnCharSetFinalize(&sCharSet2);
    ulnCharSetFinalize(&sCharSet3);

    return sRet;

    ACI_EXCEPTION_END;
    {
        //BUGBUG-TODO What Description Record ??
        if (sState == 1)
        {
            ulnCharSetFinalize(&sCharSet1);
            ulnCharSetFinalize(&sCharSet2);
            ulnCharSetFinalize(&sCharSet3);
        }
    }

    return SQL_ERROR;
}
#endif

/* ------------------------------------------------
 *  SQLDriverConnect
 * ----------------------------------------------*/

extern ulnSQLDriverConnectFrameWork gSQLDriverConnectModule;

SQLRETURN SQL_API SQLDriverConnect(SQLHDBC       hdbc,
                                   SQLHWND       hwnd,
                                   SQLCHAR      *szConnStrIn,
                                   SQLSMALLINT   cbConnStrIn,
                                   SQLCHAR      *szConnStrOut,
                                   SQLSMALLINT   cbConnStrOutMax,
                                   SQLSMALLINT  *pcbConnStrOut,
                                   SQLUSMALLINT  fDriverCompletion)
{
    acp_char_t   sNewConnString[1024]; // Dialog Box Set
    acp_char_t  *sCurrConnString = (acp_char_t *)szConnStrIn;
    SQLSMALLINT  sConnStringLen  = cbConnStrIn;
    acp_bool_t   sIsPrompt;

    ULN_TRACE(SQLDriverConnect);

    // Getting private Function
    ACI_TEST(gSQLDriverConnectModule.mSetup((ulnDbc*)hdbc)
             != ACI_SUCCESS);

    // need prompt?
    ACI_TEST(gSQLDriverConnectModule.mCheckPrompt((acp_char_t *)szConnStrIn,
                                                  fDriverCompletion,
                                                  &sIsPrompt)
             != ACI_SUCCESS);

    if (sIsPrompt == ACP_TRUE)
    {
        // do prompt
        if (gSQLDriverConnectModule.mOpenDialog(hwnd,
                                                (acp_char_t *)szConnStrIn,
                                                sNewConnString,
                                                ACI_SIZEOF(sNewConnString))
            == ACI_SUCCESS)
        {
            sCurrConnString = sNewConnString;
            sConnStringLen  = ACI_SIZEOF(sNewConnString);
        }
    }

    return gSQLDriverConnectModule.mConnect((ulnDbc*)hdbc,
                                            (acp_char_t*)sCurrConnString,
                                            sConnStringLen,
                                            (acp_char_t*)szConnStrOut,
                                            cbConnStrOutMax,
                                            pcbConnStrOut);

    ACI_EXCEPTION_END;
    {
        //BUGBUG-TODO What Description Record ??
    }

    return SQL_ERROR;
}

// fix BUG-26703 ODBC 유니코드 함수는 유닉스 ODBC에서는 제외
#if !defined(ALTIBASE_ODBC)
SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC       hdbc,
                                    SQLHWND       hwnd,
                                    SQLWCHAR     *szConnStrIn,
                                    SQLSMALLINT   cbConnStrIn,
                                    SQLWCHAR     *szConnStrOut,
                                    SQLSMALLINT   cbConnStrOutMax,
                                    SQLSMALLINT  *pcbConnStrOut,
                                    SQLUSMALLINT  fDriverCompletion)
{
    /*
     * StringLength1
     *     [Input] Length of *InConnectionString, in characters if the string is Unicode,
     *     or bytes if string is ANSI or DBCS.
     *
     * BufferLength
     *     [Input] Length of the *OutConnectionString buffer, in characters.
     *
     * StringLength2Ptr
     *     [Output] Pointer to a buffer in which to return the total number of characters
     *     (excluding the null-termination character) available to return in *OutConnectionString.
     *     If the number of characters available to return is greater than or equal to BufferLength,
     *     the completed connection string in *OutConnectionString is truncated to BufferLength minus
     *     the length of a null-termination character.
     */

    acp_char_t   sNewConnString[1024]; // Dialog Box Set
    acp_char_t  *sCurrConnString;
    SQLSMALLINT  sConnStringLen;
    acp_bool_t   sIsPrompt;

    ulnCharSet   sCharSetIn;
    ulnCharSet   sCharSetOut;
    acp_sint32_t sLen = 0;
    acp_sint32_t sState = 0;

    SQLRETURN    sRet;
    SQLSMALLINT  sConnStrOutLen;
    acp_char_t  *sTemp = NULL;

    ULN_TRACE(SQLDriverConnectW);

    ulnCharSetInitialize(&sCharSetIn);
    ulnCharSetInitialize(&sCharSetOut);
    sState = 1;

    if(szConnStrIn != NULL)
    {
        if (cbConnStrIn == SQL_NTS)
        {
            sLen = getWcharLength(szConnStrIn);
        }
        else
        {
            sLen = cbConnStrIn * ACI_SIZEOF(ulWChar);
        }

        ACI_TEST(ulnCharSetConvert(&sCharSetIn,
                                   NULL,
                                   hdbc,
                                   (const mtlModule *)gWcharModule,
                                   (const mtlModule *)gClientModule,
                                   (void*)szConnStrIn,
                                   sLen,
                                   CONV_DATA_IN)
                 != ACI_SUCCESS);
    }

    sCurrConnString  = (acp_char_t *)ulnCharSetGetConvertedText(&sCharSetIn);
    sConnStringLen   = ulnCharSetGetConvertedTextLen(&sCharSetIn);

    // Getting private Function
    ACI_TEST(gSQLDriverConnectModule.mSetup((ulnDbc*)hdbc)
             != ACI_SUCCESS);

    // need prompt?
    ACI_TEST(gSQLDriverConnectModule.mCheckPrompt(sCurrConnString,
                                                  fDriverCompletion,
                                                  &sIsPrompt)
             != ACI_SUCCESS);

    if (sIsPrompt == ACP_TRUE)
    {
        // do prompt
        if (gSQLDriverConnectModule.mOpenDialog(hwnd,
                                                sCurrConnString,
                                                sNewConnString,
                                                ACI_SIZEOF(sNewConnString))
            == ACI_SUCCESS)
        {
            sCurrConnString = sNewConnString;
            sConnStringLen  = ACI_SIZEOF(sNewConnString);
        }
    }

    sRet = gSQLDriverConnectModule.mConnect((ulnDbc*)hdbc,
                                            (acp_char_t*)sCurrConnString,
                                            sConnStringLen,
                                            (acp_char_t*)szConnStrOut,
                                            cbConnStrOutMax,
                                            &sConnStrOutLen);
    if(szConnStrOut != NULL)
    {
        ACI_TEST(acpMemAlloc((void**)&sTemp, sConnStrOutLen + 1) != ACP_RC_SUCCESS);
        acpCStrCpy(sTemp,
                   sConnStrOutLen + 1,
                   (acp_char_t*)szConnStrOut,
                   acpCStrLen((acp_char_t*)szConnStrOut, ACP_SINT32_MAX));

        if (ulnCharSetConvertUseBuffer(&sCharSetOut,
                                       NULL,
                                       hdbc,
                                       (const mtlModule *)gClientModule,
                                       (const mtlModule *)gWcharModule,
                                       (void*)sTemp,
                                       sConnStrOutLen,
                                       (void*)szConnStrOut,
                                       (cbConnStrOutMax - 1) * ACI_SIZEOF(ulWChar),
                                       CONV_DATA_OUT) != ACI_SUCCESS)
        {
            sRet = SQL_SUCCESS_WITH_INFO;
            sConnStrOutLen = cbConnStrOutMax;
            szConnStrOut[cbConnStrOutMax - 1] = 0;
        }
        else
        {
            sConnStrOutLen = ulnCharSetGetConvertedTextLen(&sCharSetOut) / ACI_SIZEOF(ulWChar);
            szConnStrOut[sConnStrOutLen] = 0;
        }

        if (pcbConnStrOut != NULL)
        {
            *pcbConnStrOut = sConnStrOutLen;
        }

        acpMemFree(sTemp);
    }

    sState = 0;
    ulnCharSetFinalize(&sCharSetIn);
    ulnCharSetFinalize(&sCharSetOut);

    return sRet;

    ACI_EXCEPTION_END;

    if (sState == 1)
    {
        ulnCharSetFinalize(&sCharSetIn);
        ulnCharSetFinalize(&sCharSetOut);
    }

    return SQL_ERROR;
}
#endif

/*
 * Altibase Connection Pool CLI API
 */


SQLRETURN SQL_API SQLCPoolAllocHandle(SQLHDBCP *aConnectionPoolHandle)
{
    ULN_TRACE(SQLCPoolAllocHandle);

    return ulnwCPoolAllocHandle((ulnwCPool **)aConnectionPoolHandle);
}


SQLRETURN SQL_API SQLCPoolFreeHandle(SQLHDBCP aConnectionPoolHandle)
{
    ULN_TRACE(SQLCPoolFreeHandle);

    return ulnwCPoolFreeHandle((ulnwCPool *)aConnectionPoolHandle);
}


SQLRETURN SQL_API SQLCPoolSetAttr(SQLHDBCP aConnectionPoolHandle,
        SQLINTEGER      aAttribute,
        SQLPOINTER      aValue,
        SQLINTEGER      aStringLength)
{
    ULN_TRACE(SQLCPoolSetAttr);

    return ulnwCPoolSetAttr((ulnwCPool *)aConnectionPoolHandle, aAttribute, aValue, aStringLength);
}


SQLRETURN SQL_API SQLCPoolGetAttr(SQLHDBCP aConnectionPoolHandle,
        SQLINTEGER      aAttribute,
        SQLPOINTER      aValue,
        SQLINTEGER      aBufferLength,
        SQLINTEGER      *aStringLength)
{
    ULN_TRACE(SQLCPoolGetAttr);

    return ulnwCPoolGetAttr((ulnwCPool *)aConnectionPoolHandle, aAttribute, aValue, aBufferLength, aStringLength);
}


SQLRETURN SQL_API SQLCPoolInitialize(SQLHDBCP aConnectionPoolHandle)
{
    ULN_TRACE(SQLCPoolInitialize);

    return ulnwCPoolInitialize((ulnwCPool *)aConnectionPoolHandle);
}


SQLRETURN SQL_API SQLCPoolFinalize(SQLHDBCP aConnectionPoolHandle)
{
    ULN_TRACE(SQLCPoolFinalize);

    return ulnwCPoolFinalize((ulnwCPool *)aConnectionPoolHandle);
}


SQLRETURN SQL_API SQLCPoolGetConnection(SQLHDBCP aConnectionPoolHandle,
        SQLHDBC         *aConnectionHandle)
{
    ULN_TRACE(SQLCPoolGetConnection);

    return ulnwCPoolGetConnection((ulnwCPool *)aConnectionPoolHandle, (ulnDbc **)aConnectionHandle);
}


SQLRETURN SQL_API SQLCPoolReturnConnection(SQLHDBCP aConnectionPoolHandle,
        SQLHDBC         aConnectionHandle)
{
    ULN_TRACE(SQLCPoolReturnConnection);

    return ulnwCPoolReturnConnection((ulnwCPool *)aConnectionPoolHandle, (ulnDbc *)aConnectionHandle);
}
