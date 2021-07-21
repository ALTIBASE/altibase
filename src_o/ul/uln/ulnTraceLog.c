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

/*
  PROJ-1645 UL Fail-Over
  Trace Logging 기능.
 */

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnTraceLog.h>


ulnTraceLogFile   gTraceLogFile;


void ulnInitTraceLog()
{
    acp_char_t  *sEnvStr;

    /* bug-35142 cli trace log
       trace 파일명을 지정한다. 지정하지 않으면 default 로
       현재 디렉토리 밑에 altibase_cli_mmddhh.log 파일을 생성 */
    if (acpEnvGet("ALTIBASE_CLI_TRCLOG_FILE", &sEnvStr) == ACP_RC_SUCCESS)
    {
        acpSnprintf(gTraceLogFile.mCurrentFileName,
                    ULN_TRACE_LOG_PATH_LEN, "%s", sEnvStr);
    }
    else
    {
        //fix BUG-25972 Trace Log에서 HOME Directory는
        //제외하자!!
        //current directory
        acpSnprintf(gTraceLogFile.mCurrentFileName,
                    ULN_TRACE_LOG_PATH_LEN, "%s", ULN_TRACE_LOG_FILE_NAME);
    }//else

    /* bug-35142 cli trace log
     trace level은 최초 한번 지정하며 전역적으로 사용됨.
     level은  off, low, mid, high로 구분된다. 
     off - default, 출력 안함
     low - 최소 출력 ex) connect, prepare, execute, error
     mid - 좀더 출력 ex) allocHandle, bindParameter, bindCol
     high- 상세 출력 ex) fetch, fetched data */
    if (acpEnvGet("ALTIBASE_CLI_TRCLOG_LEVEL", &sEnvStr) == ACP_RC_SUCCESS)
    {
        if (acpCStrCaseCmp(sEnvStr, "low", 3) == 0)
        {
            gTraceLogFile.mUlnTraceLevel = ULN_TRACELOG_LOW;
        }
        else if (acpCStrCaseCmp(sEnvStr, "mid", 3) == 0)
        {
            gTraceLogFile.mUlnTraceLevel = ULN_TRACELOG_MID;
        }
        else if (acpCStrCaseCmp(sEnvStr, "high", 4) == 0)
        {
            gTraceLogFile.mUlnTraceLevel = ULN_TRACELOG_HIGH;
        }
        else
        {
            gTraceLogFile.mUlnTraceLevel = ULN_TRACELOG_OFF;
        }
    }
    else
    {
        gTraceLogFile.mUlnTraceLevel = ULN_TRACELOG_OFF;
    }

    gTraceLogFile.mCurrentLogLineCnt = 0;
    gTraceLogFile.mCurrentLogFileNo  = 0;
    ACE_ASSERT(acpThrRwlockCreate(&gTraceLogFile.mFileLatch, ACP_THR_RWLOCK_DEFAULT)
               == ACP_RC_SUCCESS);
}

void ulnFinalTraceLog()
{
    ACE_ASSERT(acpThrRwlockDestroy(&gTraceLogFile.mFileLatch) == ACP_RC_SUCCESS);
}

/* bug-36098: compile error on linux: inline 을 사용하지 않기로 함 */
void ULN_TRACE_LOG(ulnFnContext     *aCtx,
                   acp_sint32_t      aLevel,
                   void             *aData,
                   acp_sint32_t      aDataLen,
                   const acp_char_t *aFormat,
                   ...)
{
    va_list sArgs;

    va_start(sArgs, aFormat);

    ULN_TRACE_LOG_V(aCtx, aLevel, aData, aDataLen, aFormat, sArgs);

    va_end(sArgs);
}

void ULN_TRACE_LOG_V(ulnFnContext     *aCtx,
                     acp_sint32_t      aLevel,
                     void             *aData,
                     acp_sint32_t      aDataLen,
                     const acp_char_t *aFormat,
                     va_list           aArgs)
{
    acp_time_t      sTimevalue;
    acp_time_exp_t  sLocalTime;
    acp_std_file_t  sFP;
    acp_uint32_t    sState =0;
    acp_char_t      sNewLogFileName[ULN_TRACE_LOG_PATH_LEN];

    acp_sint32_t    sPid;
    acp_uint64_t    sThrId;
    acp_char_t      sDbcStmtStr[80];
    acp_char_t*     sData = (acp_char_t*)aData;
    acp_sint32_t    sDataLen;
    acp_sint32_t    sDataIdx;

    ACI_TEST_RAISE(aLevel > gTraceLogFile.mUlnTraceLevel, skipTrace);

    sPid = acpProcGetSelfID();
    sThrId = acpThrGetSelfID();

    /* bug-35142 cli trace log
       connection 과 stmt handle을 출력 */
    if (aCtx != NULL)
    {
        if (aCtx->mHandle.mObj != NULL)
        {
            if (aCtx->mObjType == ULN_OBJ_TYPE_DBC)
            {
                acpSnprintf(sDbcStmtStr, ACI_SIZEOF(sDbcStmtStr),
                        "%p unknown  ", aCtx->mHandle.mDbc);
            }
            else if (aCtx->mObjType == ULN_OBJ_TYPE_STMT)
            {
                acpSnprintf(sDbcStmtStr, ACI_SIZEOF(sDbcStmtStr), "%p %p",
                        aCtx->mHandle.mStmt->mParentDbc, aCtx->mHandle.mStmt);
            }
            else
            {
                acpSnprintf(sDbcStmtStr, ACI_SIZEOF(sDbcStmtStr), "--------- ---------");
            }
        }
        else
        {
            acpSnprintf(sDbcStmtStr, ACI_SIZEOF(sDbcStmtStr), "context object null");
        }
    }
    else
    {
        acpSnprintf(sDbcStmtStr, ACI_SIZEOF(sDbcStmtStr), "--------- ---------");
    }


    sTimevalue = acpTimeNow();
    acpTimeGetLocalTime(sTimevalue, &sLocalTime);
    /* ex) altibase_cli_mmddhh.log */
    acpSnprintf(sNewLogFileName, ACI_SIZEOF(sNewLogFileName),
            "%s_%02"ACI_INT32_FMT
            "%02"ACI_INT32_FMT
            "%02"ACI_INT32_FMT".log",
            gTraceLogFile.mCurrentFileName,
            sLocalTime.mMonth, sLocalTime.mDay, sLocalTime.mHour);

    ACE_ASSERT(acpThrRwlockLockWrite(&gTraceLogFile.mFileLatch) == ACP_RC_SUCCESS);
    sState =1;

    ACI_TEST(acpStdOpen(&sFP,
                        sNewLogFileName,
                        ACP_STD_OPEN_APPEND_TEXT) != ACP_RC_SUCCESS);

    /* bug-35142 cli trace log
       ex1)[hh:mm:ss:ms| pid| thr-id| conn| stmt| function| [args] return
       ex2)[14:21:35:297|   627| 140082760103712|
       0x1070420 0x10745d8] ulnFetchScroll    | [orient: 1] 100 */
    acpFprintf(&sFP,
               "[%02"ACI_INT32_FMT ":"
               "%02"ACI_INT32_FMT ":"
               "%02"ACI_INT32_FMT ":"
               "%03"ACI_INT32_FMT "| "
               "%5"ACI_INT32_FMT  "| "
               "%"ACI_UINT64_FMT "| %21s] ",
               sLocalTime.mHour,
               sLocalTime.mMin,
               sLocalTime.mSec,
               sLocalTime.mUsec/1000,
               sPid, sThrId, sDbcStmtStr);

    acpVfprintf(&sFP, aFormat, aArgs);

    /* bug-35142 cli trace log
       data를 hex dump 할 때 사용. dump 양 관계로 30bytes 로 제한.
       현재는 fetch data를 볼때만 (ulnCharSetConvertUseBuffer) 사용 */
    if (sData != NULL && aDataLen > 0)
    {
        sDataLen = (aDataLen > ULN_TRACE_LOG_MAX_DATA_LEN)?
            ULN_TRACE_LOG_MAX_DATA_LEN: aDataLen;
        acpFprintf(&sFP,"\n  ");
        for (sDataIdx = 0; sDataIdx < sDataLen; sDataIdx++)
        {
            acpFprintf(&sFP,"%02x ", sData[sDataIdx]);
        }
    }
    (void)acpFprintf(&sFP,"\n");

    acpStdFlush(&sFP);

    sState = 1;
    acpStdClose(&sFP);

    sState = 0;
    ACE_ASSERT(acpThrRwlockUnlock(&gTraceLogFile.mFileLatch) == ACP_RC_SUCCESS);

    ACI_EXCEPTION_CONT(skipTrace);
    return;

    ACI_EXCEPTION_END;
    switch(sState)
    {
        case 1:
            ACE_ASSERT(acpThrRwlockUnlock(&gTraceLogFile.mFileLatch) == ACP_RC_SUCCESS);
            break;
    }
    return;
}

