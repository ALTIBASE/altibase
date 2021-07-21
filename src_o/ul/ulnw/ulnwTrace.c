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

#include <acp.h>
#include <sqlcli.h>
#include <ulnwCPool.h>
#include <ulnPrivate.h>

static acp_uint32_t gLogLevel = ALTIBASE_TRACELOG_FULL;

void ulnwTraceSet(acp_uint32_t aCurrentLogLevel)
{
    switch (aCurrentLogLevel)
    {
    case ALTIBASE_TRACELOG_NONE:
        gLogLevel = ALTIBASE_TRACELOG_NONE;
        break;
    case ALTIBASE_TRACELOG_ERROR:
        gLogLevel = ALTIBASE_TRACELOG_ERROR;
        break;
    case ALTIBASE_TRACELOG_FULL:
        gLogLevel = ALTIBASE_TRACELOG_FULL;
        break;

    case ALTIBASE_TRACELOG_DEBUG:
    default:
        gLogLevel = ALTIBASE_TRACELOG_DEBUG;
        break;
    }
}

void ulnwTrace(const acp_char_t *aFuncName, const acp_char_t *aFormat, ...)
{
    va_list     sArgs;
    acp_char_t  sLogLine[ULNW_MAX_STRING_LENGTH*5];
    acp_rc_t    sRc;

    ACI_TEST(gLogLevel == ALTIBASE_TRACELOG_NONE);
    ACI_TEST(gLogLevel == ALTIBASE_TRACELOG_ERROR);

    va_start(sArgs, aFormat);
    sRc = acpVsnprintf(sLogLine, ULNW_MAX_STRING_LENGTH, aFormat, sArgs);
    va_end(sArgs);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRc));

    ULN_TRACE_LOG(NULL, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| %s", aFuncName, sLogLine);
    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwDebug(const acp_char_t *aFuncName, const acp_char_t *aFormat, ...)
{
    va_list     sArgs;
    acp_char_t  sLogLine[ULNW_MAX_STRING_LENGTH*5];
    acp_rc_t    sRc;

    ACI_TEST(gLogLevel != ALTIBASE_TRACELOG_DEBUG);

    va_start(sArgs, aFormat);
    sRc = acpVsnprintf(sLogLine, ULNW_MAX_STRING_LENGTH, aFormat, sArgs);
    va_end(sArgs);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRc));

    ULN_TRACE_LOG(NULL, ULN_TRACELOG_HIGH, NULL, 0,
            "%-18s| DEBUG %s", aFuncName, sLogLine);

    return;

    ACI_EXCEPTION_END;
    return;
}

void ulnwError(const acp_char_t *aFuncName, const acp_char_t *aFormat, ...)
{
    va_list     sArgs;
    acp_char_t  sLogLine[ULNW_MAX_STRING_LENGTH*5];
    acp_rc_t    sRc;

    ACI_TEST(gLogLevel == ALTIBASE_TRACELOG_NONE);

    va_start(sArgs, aFormat);
    sRc = acpVsnprintf(sLogLine, ULNW_MAX_STRING_LENGTH, aFormat, sArgs);
    va_end(sArgs);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRc));

    ULN_TRACE_LOG(NULL, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| ERROR %s", aFuncName, sLogLine);

    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwTraceSqlError(const acp_char_t *aFuncName, const acp_char_t *aSqlFuncName, SQLSMALLINT aHandleType, SQLHANDLE aHandle)
{
    SQLRETURN       sRc             = SQL_SUCCESS;
    SQLSMALLINT     sRecordNo       = 1;
    SQLCHAR         sSQLSTATE[16];
    SQLCHAR         sMessage[2048];
    SQLSMALLINT     sMessageLength  = 0;
    SQLINTEGER      sNativeError    = 0;

    ACI_TEST(gLogLevel == ALTIBASE_TRACELOG_NONE);

    /* BUG-35033 [compile warning] pointer targets in passing argument 4 of ulnGetDiagRec differ in signedness */
    while ( ( sRc = ulnGetDiagRec( aHandleType,
                                   aHandle,
                                   sRecordNo,
                                   (acp_char_t *)sSQLSTATE,
                                   &sNativeError,
                                   (acp_char_t *)sMessage,
                                   ACI_SIZEOF( sMessage ),
                                   &sMessageLength,
                                   ACP_FALSE ) ) != SQL_NO_DATA )
    {
        ulnwDebug(aFuncName, "\"%s\" failed (Rec:%d STATE:%s MSG:\"%s\" MSGLEN:%d NATIVE:0x%X",
                aSqlFuncName, sRecordNo, sSQLSTATE, sMessage, sMessageLength, sNativeError);

        if ((sRc != SQL_SUCCESS) && (sRc != SQL_SUCCESS_WITH_INFO))
        {
            break;
        }

        sRecordNo++;
    }
    return;

    ACI_EXCEPTION_END;

    return;
}
