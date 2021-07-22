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

/***********************************************************************
 *  This file defines Error Log.
 *
 *   !!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!
 *   Don't Use IDE_ASSERT() in this file.
 **********************************************************************/
#include <uln.h>
#include <ulxMsgLog.h>
#include <aciVa.h>

#define ULX_MSG_FORMAT "%d:\n%s     : %s : [ERR-%05"ACI_XINT32_FMT"] %s\n\n"

void ulxMsgLogInitialize(ulxMsgLog *aMsgLog)
{
    acpMemSet(aMsgLog->mXaName, 0, ULX_XA_NAME_LEN);
    acpMemSet(aMsgLog->mLogDir, 0, ULX_LOG_DIR_LEN);
}

/**********************************************************
 *    < xa trace file Name >
 *
 *    altibase_xa_[XA_NAME][YYYYMMDD].log
 **********************************************************/
void ulxMsgLogGetLogFileName(ulxMsgLog    *aMsgLog,
                             acp_char_t   *aBuf,
                             acp_uint32_t  aBufSize)
{
    acp_time_t     sTimevalue;
    acp_time_exp_t sLocalTime;

    sTimevalue = acpTimeNow();
    acpTimeGetLocalTime(sTimevalue, &sLocalTime);

    acpSnprintf(aBuf, aBufSize,
                "%s%saltibase_xa_%s"
                "%4"ACI_UINT32_FMT
                "%02"ACI_UINT32_FMT
                "%02"ACI_UINT32_FMT".log",
                aMsgLog->mLogDir,
                ACI_DIRECTORY_SEPARATOR_STR_A,
                aMsgLog->mXaName,
                sLocalTime.mYear,
                sLocalTime.mMonth,
                sLocalTime.mDay);
}

void ulxMsgLogParseOpenStr(ulxMsgLog   *aMsgLog,
                           acp_char_t  *aOpenStr)
{
    acp_char_t   *sNameField       = NULL;
    acp_char_t   *sValueFieldStart = NULL;
    acp_char_t   *sValueFieldEnd   = NULL;
    acp_sint32_t  sIndex;
    acp_char_t   *sHome;

    if (ACP_RC_IS_ENOENT(
            acpCStrFindCStr(aOpenStr,
                            "XA_NAME=",
                            &sIndex,
                            0,
                            0)))
    {
        acpCStrCpy(aMsgLog->mXaName,
                   ACI_SIZEOF(aMsgLog->mXaName),
                   "NULL",
                   acpCStrLen("NULL", ACP_SINT32_MAX));
    }
    else
    {
        sNameField = aOpenStr + sIndex;
        sValueFieldStart = sNameField + acpCStrLen("XA_NAME=", ACP_SINT32_MAX);

        if (ACP_RC_IS_ENOENT(
                acpCStrFindChar(sNameField,
                                ';',
                                &sIndex,
                                0,
                                0)))
        {
            acpCStrCpy(aMsgLog->mXaName,
                       ACI_SIZEOF(aMsgLog->mXaName),
                       sValueFieldStart,
                       acpCStrLen(sValueFieldStart, ACP_SINT32_MAX));
        }
        else
        {
            sValueFieldEnd = sNameField + sIndex;

            acpCStrCpy(aMsgLog->mXaName,
                       ACI_SIZEOF(aMsgLog->mXaName),
                       sValueFieldStart,
                       sValueFieldEnd - sValueFieldStart);

            aMsgLog->mXaName[sValueFieldEnd - sValueFieldStart] = '\0';
        }
    }

    if (ACP_RC_IS_ENOENT(
            acpCStrFindCStr(aOpenStr,
                            "XA_LOG_DIR=",
                            &sIndex,
                            0,
                            0)))
    {  // XA_LOG_DIR 필드가 없으면 $ALTIBASE_HOME/trc 디렉토리에 로깅
        if (acpEnvGet("ALTIBASE_HOME", &sHome) == ACP_RC_SUCCESS)
        {
            acpSnprintf(aMsgLog->mLogDir,
                        ACI_SIZEOF(aMsgLog->mLogDir),
                        "%s%s%s",
                        sHome,
                        ACI_DIRECTORY_SEPARATOR_STR_A,
                        "trc");
        }
        // XA_LOG_DIR 필드가 없고  $ALTIBASE_HOME환경변수도 없으면 현재 디렉토리에 로깅
        else
        {
            acpSnprintf(aMsgLog->mLogDir,
                        ACI_SIZEOF(aMsgLog->mLogDir),
                        ".");
        }
    }
    else
    {
        sNameField = aOpenStr + sIndex;
        sValueFieldStart = sNameField + acpCStrLen("XA_LOG_DIR=", ACP_SINT32_MAX);

        if (ACP_RC_IS_ENOENT(
                acpCStrFindChar(sNameField,
                                ';',
                                &sIndex,
                                0,
                                0)))
        {
            acpCStrCpy(aMsgLog->mLogDir,
                       ACI_SIZEOF(aMsgLog->mLogDir),
                       sValueFieldStart,
                       acpCStrLen(sValueFieldStart, ACP_SINT32_MAX));
        }
        else
        {
            sValueFieldEnd = sNameField + sIndex;

            acpCStrCpy(aMsgLog->mLogDir,
                       ACI_SIZEOF(aMsgLog->mLogDir),
                       sValueFieldStart,
                       sValueFieldEnd - sValueFieldStart);

            aMsgLog->mLogDir[sValueFieldEnd - sValueFieldStart] = '\0';
        }
    }
}

ACI_RC ulxLogTrace(ulxMsgLog *aLogObj, ... )
{
    acp_char_t      sFileName[1024];
    acp_std_file_t  sFP;
    acp_time_t      sTimevalue;
    acp_time_exp_t  sLocalTime;
    acp_uint32_t    sPid;

    va_list     ap;
    va_start (ap, aLogObj);

    sPid = acpProcGetSelfID();

    sTimevalue = acpTimeNow();
    acpTimeGetLocalTime(sTimevalue, &sLocalTime);

    ulxMsgLogGetLogFileName(aLogObj, sFileName, ACI_SIZEOF(sFileName));

    ACI_TEST_RAISE(acpStdOpen(&sFP,
                              sFileName,
                              ACP_STD_OPEN_APPEND_TEXT) != ACP_RC_SUCCESS,
                   open_error);

    acpFprintf(&sFP,
               "%02"ACI_UINT32_FMT
               "%02"ACI_UINT32_FMT
               "%02"ACI_UINT32_FMT".%"ACI_UINT64_FMT".",
               sLocalTime.mHour,
               sLocalTime.mMin,
               sLocalTime.mSec,
               sPid);

    acpVfprintf (&sFP, ULX_MSG_FORMAT, ap);

    va_end (ap);

    acpStdFlush(&sFP);
    acpStdClose(&sFP);

    return ACI_SUCCESS;

    ACI_EXCEPTION(open_error);
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_char_t *ulxMsgLogGetXaName(ulxMsgLog *aMsgLog)
{
    return aMsgLog->mXaName;
}

void ulxMsgLogSetXaName(ulxMsgLog *aMsgLog,
                        acp_char_t     *aXaName)
{
    if ( aXaName == NULL )
    {
        (void)acpCStrCpy(aMsgLog->mXaName,
                         ULX_XA_NAME_LEN,
                         "NULL",
                         4);
    }
    else
    {
        (void)acpCStrCpy(aMsgLog->mXaName,
                         ULX_XA_NAME_LEN,
                         (const acp_char_t*)aXaName,
                         acpCStrLen(aXaName,ULX_XA_NAME_LEN));
    }
}
