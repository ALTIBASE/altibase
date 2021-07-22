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
 * $Id: ulaLog.c 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <ulaLog.h>

/*
 * -----------------------------------------------------------------------------
 *  ulaLog :
 *
 *      ideMsgLog 의 구현을 그대로 가져옴.
 *
 *      원래는 ideMsgLog 클래스의 인스턴스로 ulaLog 를 생성했으나
 *      PROJ-1000 진행하면서 id 를 없애는 통에
 *      ideMsgLog 를 그대로 가져오게 됨.
 * -----------------------------------------------------------------------------
 */

/*
 *  TRACE Logging Format
 *  {
 *      [2006/02/25 03:33:11] [Thread-19223, SessID-01] [Level-32]
 *      Message
 *  }

 *  ^^^^ <= padding
 *  {}   <= begin/end block
 *  []   <= sub begin/end block
 *
 */

#define ULA_MSGLOG_HEADER_NUMBER "* ALTIBASE LOGFILE-"

#define ULA_MSGLOG_CONTENT_PAD     ""

struct ulaLogMgr
{
    acp_std_file_t  *mFP;             // Normal Error Log File Pointer
    acp_thr_mutex_t  mMutex;          // Error Log File Mutex
    acp_sint32_t     mDoLogFlag;      // Message Logging ?
    acp_char_t       mFileName[1024]; // File Name
    acp_offset_t     mSize;           // file size
    acp_uint32_t     mMaxNumber;      // loop file number
    acp_uint32_t     mCurNumber;      // Replace될 화일번호
    acp_uint32_t     mInitialized;    // 초기화 유무 : acp_bool_t을 쓰지 않은
                                      // 것은 정적영역으로 0 초기화으로
                                      // 되기 때문에 의미가 틀려짐.
};

static ulaLogMgr gUlaLogMgr;


static ACI_RC ulaLogMgrInitialize(ulaLogMgr        *aLogMgr,
                                  acp_sint32_t      aDoFlag,
                                  const acp_char_t *aLogFile,
                                  acp_uint32_t      aFileSize,
                                  acp_uint32_t      aMaxFileNumber)
{
    acp_rc_t sRc;

    sRc = acpThrMutexCreate(&aLogMgr->mMutex, ACP_THR_MUTEX_DEFAULT);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), mutex_init_error);

    (void)acpCStrCpy(aLogMgr->mFileName,
                     ACI_SIZEOF(aLogMgr->mFileName),
                     aLogFile,
                     1024);

    aLogMgr->mFP          = NULL;
    aLogMgr->mDoLogFlag   = aDoFlag;
    aLogMgr->mSize        = aFileSize;
    aLogMgr->mMaxNumber   = aMaxFileNumber;
    aLogMgr->mCurNumber   = 1;
    aLogMgr->mInitialized = 1;

    sRc = acpMemAlloc( (void **)&aLogMgr->mFP, ACI_SIZEOF( acp_std_file_t ) );
    ACE_ASSERT( ACP_RC_IS_SUCCESS( sRc ) );

    return ACI_SUCCESS;

    ACI_EXCEPTION(mutex_init_error);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaLogMgrDestroy(ulaLogMgr *aLogMgr)
{
    acp_rc_t sRc;

    if (aLogMgr->mInitialized != 0)
    {
        aLogMgr->mInitialized = 0;

        if ( aLogMgr->mFP != NULL )
        {
            acpMemFree( aLogMgr->mFP );

            aLogMgr->mFP = NULL;
        }
        else
        {
            /* nothing to do */
        }

        sRc = acpThrMutexDestroy(&aLogMgr->mMutex);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), mutex_destroy_error);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(mutex_destroy_error);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaLogMgrLock(ulaLogMgr *aLogMgr)
{
    acp_rc_t sRc;

    if (aLogMgr->mInitialized != 0)
    {
        sRc = acpThrMutexLock(&aLogMgr->mMutex);

        if (ACP_RC_IS_SUCCESS(sRc))
        {
            return ACI_SUCCESS;
        }
        else
        {
            return ACI_FAILURE;
        }
    }
    else
    {
        return ACI_SUCCESS;
    }
}

static ACI_RC ulaLogMgrUnlock(ulaLogMgr *aLogMgr)
{
    acp_rc_t sRc;

    if (aLogMgr->mInitialized != 0)
    {
        sRc = acpThrMutexUnlock(&aLogMgr->mMutex);

        if (ACP_RC_IS_SUCCESS(sRc))
        {
            return ACI_SUCCESS;
        }
        else
        {
            return ACI_FAILURE;
        }
    }
    else
    {
        return ACI_SUCCESS;
    }
}


ACI_RC ulaLogMgrCreateFileAndHeader(ulaLogMgr *aLogMgr)
{
    acp_rc_t     sRc;

    ACE_DASSERT(aLogMgr->mInitialized == 1);

    sRc = acpStdOpen(aLogMgr->mFP, aLogMgr->mFileName, "a");
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), open_error);

    /*
     * PROJ-1000 Client C Porting
     *           id 를 없애면서, iduVersionString, iduGetSystemInfoString(),
     *           iduGetProductionTimeString() 이 많은 것을 물고 들어오므로
     *           없앰.
     *
     *           단순한 로그의 헤더이기 때문에
     *           큰 문제 없다고 판단.
     *
     *           로그 헤더를 읽을 때에도 ULA_MSGLOG_HEADER_NUMBER 뒤의
     *           숫자까지만 체크하기 때문에 큰 문제 없어 보임.
     */
    
    (void)acpFprintf(aLogMgr->mFP,
                    ULA_MSGLOG_HEADER_NUMBER"%u \n",
                    aLogMgr->mCurNumber);

    return ACI_SUCCESS;

    ACI_EXCEPTION(open_error);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaLogMgrCheckExist(ulaLogMgr *aLogMgr)
{
#if defined(WRS_VXWORKS) && defined(USE_RAW_DEVICE)
    return ACI_SUCCESS;
#else
    acp_rc_t   sRc;
    acp_stat_t sStat;

    /*
     * BUGBUG : acp 에 access() 함수를 대체할 수 있는 함수가 없음.
     *          우선은 본 함수 ulaLogMgrCheckExist() 의 이름처럼
     *          파일 의 존재 여부만 체크하도록 구현함.
     *
     *          BUG-29613 으로 등록함.
     */
    sRc = acpFileStatAtPath(aLogMgr->mFileName, &sStat, ACP_TRUE);
    if (ACP_RC_IS_ENOENT(sRc))
    {
        return ACI_FAILURE;
    }
    else
    {
        if (ACP_RC_IS_SUCCESS(sRc))
        {
            return ACI_SUCCESS;
        }
        else
        {
            ACE_ASSERT(0);
        }
    }

#if 0
    if (idlOS::access(mFileName, R_OK | W_OK | F_OK) == 0)
    {
        return ACI_SUCCESS;
    }
#endif

    return ACI_FAILURE;
#endif
}

static ACI_RC ulaLogMgrReadFileNumber(ulaLogMgr    *aLogMgr,
                                      acp_sint32_t *filenumber)
{
    acp_sint32_t   i;
    acp_sint32_t   sLength;
    acp_char_t     sBuffer[1024] = {0,};
    acp_sint32_t   sBufferLen;
    acp_std_file_t sFP;
    acp_rc_t       sRc;

    sLength = acpCStrLen(ULA_MSGLOG_HEADER_NUMBER, 20);

    sRc = acpStdOpen(&sFP, aLogMgr->mFileName, "r");
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), open_error);

    sRc = acpStdGetCString(&sFP, sBuffer, 1024);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), open_error);
    sBuffer[1023] = 0; // for klocwork error

    ACI_TEST_RAISE(acpCStrCmp(sBuffer, ULA_MSGLOG_HEADER_NUMBER, sLength) != 0,
                   find_token_error);

    sBufferLen = (acp_sint32_t)acpCStrLen(sBuffer, ACI_SIZEOF(sBuffer));

    for (i = 0; i <= sBufferLen; i++) // 공백 제거
    {
        if (sBuffer[i] == 0) break;
        if (sBuffer[i] == ' ')
        {
            sBuffer[i] = 0;
        }
    }

    {
        acp_sint32_t sSign;
        acp_uint32_t sConvertedValue;

        sRc = acpCStrToInt32(sBuffer + sLength,
                             ACI_SIZEOF(sBuffer) - sLength,
                             &sSign,
                             &sConvertedValue,
                             10,    /* base */
                             NULL);
        ACE_ASSERT(ACP_RC_IS_SUCCESS(sRc));
        ACE_ASSERT(sSign > 0);
        ACE_ASSERT(sConvertedValue <= (acp_uint32_t)ACP_SINT32_MAX);

        *filenumber = (acp_sint32_t)sConvertedValue;
    }

    (void)acpStdClose(&sFP);

    return ACI_SUCCESS;

    ACI_EXCEPTION(find_token_error)
    {
        (void)acpStdClose(&sFP);
    }

    ACI_EXCEPTION(open_error);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaLogMgrErase(ulaLogMgr *aLogMgr)
{
    acp_rc_t sRc;

    sRc = acpFileRemove(aLogMgr->mFileName);

    if (ACP_RC_IS_SUCCESS(sRc))
    {
        return ACI_SUCCESS;
    }
    else
    {
        return ACI_FAILURE;
    }
}

static ACI_RC ulaLogMgrOpen(ulaLogMgr *aLogMgr)
{
#if defined(WRS_VXWORKS) && defined(USE_RAW_DEVICE)
    aLogMgr->mFP = ACP_STD_OUT;

    return ACI_SUCCESS;
#else

    acp_rc_t     sRc;
    acp_sint32_t sCurNumber;

    ACE_DASSERT( aLogMgr->mInitialized == 1 );

    if (ulaLogMgrCheckExist(aLogMgr) == ACI_SUCCESS) // 화일이 존재함.
    {
        if (ulaLogMgrReadFileNumber(aLogMgr, &sCurNumber) != ACI_SUCCESS)
        {
            // 화일의 내용이 비정상임 => 지운후에 1번으로 재생성
            (void)acpStdClose( aLogMgr->mFP );
            (void)ulaLogMgrErase(aLogMgr);

            aLogMgr->mCurNumber = 1;
            ACI_TEST(ulaLogMgrCreateFileAndHeader(aLogMgr) != ACI_SUCCESS);
        }
        else
        {
            aLogMgr->mCurNumber = sCurNumber;

            sRc = acpStdOpen(aLogMgr->mFP, aLogMgr->mFileName, "a");
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), open_error);
        }
    }
    else // 화일이 없음
    {
        aLogMgr->mCurNumber = 1;
        ACI_TEST_RAISE(ulaLogMgrCreateFileAndHeader(aLogMgr) != ACI_SUCCESS,
                       create_error);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(open_error);

    ACI_EXCEPTION(create_error);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
#endif
}


static ACI_RC ulaLogMgrCheckSizeAndRename(ulaLogMgr *aLogMgr)
{
#if defined(WRS_VXWORKS) && defined(USE_RAW_DEVICE)
    return ACI_SUCCESS;
#else
    acp_rc_t     sRc;
    acp_offset_t sSize = 0;

    // 화일 Size Check and Cutting
    if (aLogMgr->mSize != 0) // 화일 Size 명시되면 Cutting.
    {
        sRc = acpStdTell(aLogMgr->mFP, &sSize); 

        /* 화일 크기를 넘음 : 자르기 
         * acpStdTell 에 실패함 : 해당 로그 파일을 복사본으로 넘기고
         * 새 로그 파일 씀. */
        if ( ( sRc != ACP_RC_SUCCESS ) ||              
             ( sSize >= aLogMgr->mSize ) ) 
        {
            acp_char_t newFileName[1024] = {0,};

            /*
             * - 1. close
             * - 2. 현재 화일을 rename
             * - 3. 새롭게 생성
             */
            (void)acpStdClose( aLogMgr->mFP ); 

            (void)acpSnprintf(newFileName,
                              ACI_SIZEOF(newFileName),
                              "%s-%u",
                              aLogMgr->mFileName,
                              aLogMgr->mCurNumber);

            (void)acpFileRename(aLogMgr->mFileName, newFileName);

            if (aLogMgr->mCurNumber == aLogMgr->mMaxNumber)
            {
                aLogMgr->mCurNumber = 1;
            }
            else
            {
                aLogMgr->mCurNumber++;
            }

            ACI_TEST(ulaLogMgrCreateFileAndHeader(aLogMgr) != ACI_SUCCESS);
        }
        else
        {
            // proceeding
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
#endif
}

static ACI_RC ulaLogMgrClose(ulaLogMgr *aLogMgr)
{
#if defined(WRS_VXWORKS) && defined(USE_RAW_DEVICE)
    aLogMgr->mFP = NULL;
    return ACI_SUCCESS;
#else
    ACI_RC   sReturnValue = ACI_SUCCESS;
    acp_rc_t sRc;

    if (aLogMgr->mFP != NULL)
    {
        sRc = acpStdClose(aLogMgr->mFP);
        if (ACP_RC_IS_SUCCESS(sRc))
        {
            sReturnValue = ACI_SUCCESS;
        }
        else
        {
            sReturnValue = ACI_FAILURE;
        }
    }

    return sReturnValue;
#endif
}

static ACI_RC ulaLogMgrLogBody(ulaLogMgr        *aLogMgr,
                               const acp_char_t *format,
                               ...)
{
    if (aLogMgr->mFP != NULL)
    {
        va_list     ap;

        ACI_TEST(ulaLogMgrCheckSizeAndRename(aLogMgr) != ACI_SUCCESS);

        va_start (ap, format);

        (void)acpFprintf(aLogMgr->mFP, ULA_MSGLOG_CONTENT_PAD);
        (void)acpVfprintf(aLogMgr->mFP, format, ap);

        va_end (ap);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// fix BUG-28453
// 오버로딩된 logBody()를 분리
static ACI_RC ulaLogMgrLogBodyV(ulaLogMgr        *aLogMgr,
                                const acp_char_t *format,
                                va_list           aList)
{
    if (aLogMgr->mFP)
    {
        ACI_TEST(ulaLogMgrCheckSizeAndRename(aLogMgr) != ACI_SUCCESS);

        (void)acpFprintf(aLogMgr->mFP, ULA_MSGLOG_CONTENT_PAD);
        (void)acpVfprintf(aLogMgr->mFP, format, aList);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulaLogMgrFlush(ulaLogMgr *aLogMgr)
{
    acp_rc_t sRc;

    if (aLogMgr->mFP)
    {
        if (ulaLogMgrCheckExist(aLogMgr) != ACI_SUCCESS) // 화일이 지워짐
        {
            (void)ulaLogMgrClose(aLogMgr);

            ACI_TEST(ulaLogMgrCreateFileAndHeader(aLogMgr) != ACI_SUCCESS);
        }

        sRc = acpStdFlush(aLogMgr->mFP);

        if (ACP_RC_IS_SUCCESS(sRc))
        {
            return ACI_SUCCESS;
        }
        else
        {
            return ACI_FAILURE;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 *  ulaLog
 * -----------------------------------------------------------------------------
 */
ACI_RC ulaEnableLogging(const acp_char_t *aLogDirectory,
                        const acp_char_t *aLogFileName,
                        acp_uint32_t      aFileSize,
                        acp_uint32_t      aMaxFileNumber,
                        ulaErrorMgr      *aOutErrorMgr)
{
    acp_char_t sTargetFileName[ULA_LOG_FILENAME_LEN];

    ACI_TEST_RAISE(aLogFileName == NULL, ERR_PARAMETER_NULL);

    /*
     * Note : unix / windows 구분해서 / 혹은 \ 를 넣어야 하는데,
     *        acp 에서는 필요 없다.
     */
    (void)acpSnprintf(sTargetFileName, ACI_SIZEOF(sTargetFileName), "%s%c%s",
                      aLogDirectory, '/', aLogFileName);

    ACI_TEST_RAISE(ulaLogMgrInitialize(&gUlaLogMgr,
                                       1,
                                       sTargetFileName,
                                       aFileSize,
                                       aMaxFileNumber)
                   != ACI_SUCCESS, ERR_INITIALIZE);
    ACI_TEST_RAISE(ulaLogMgrOpen(&gUlaLogMgr) != ACI_SUCCESS, ERR_OPEN);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_PARAMETER_NULL)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_PARAMETER_NULL,
                        "Logging Activate");
    }

    ACI_EXCEPTION(ERR_INITIALIZE)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LOG_MGR_INITIALIZE);
    }

    ACI_EXCEPTION(ERR_OPEN)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LOG_FILE_OPEN);
    
        (void)ulaLogMgrDestroy( &gUlaLogMgr );
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaDisableLogging(ulaErrorMgr *aOutErrorMgr)
{
    ACP_UNUSED(aOutErrorMgr);

    (void)ulaLogMgrClose( &gUlaLogMgr );
    (void)ulaLogMgrDestroy( &gUlaLogMgr );

    return ACI_SUCCESS;
}

static void ulaGetTimestampString(acp_char_t *aBuf, acp_uint32_t aBufSize)
{
    acp_time_t      sStdTime;
    acp_time_exp_t  sLocalTime;

    sStdTime = acpTimeNow();
    acpTimeGetLocalTime(sStdTime, &sLocalTime);

    (void)acpSnprintf(aBuf, aBufSize,
                      "%4u/%02u/%02u %02u:%02u:%02u",
                      sLocalTime.mYear,
                      sLocalTime.mMonth,
                      sLocalTime.mDay,
                      sLocalTime.mHour,
                      sLocalTime.mMin,
                      sLocalTime.mSec);
}

ACI_RC ulaLogTrace(ulaErrorMgr      *aOutErrorMgr,
                   const acp_char_t *aFormat,
                   ...)
{
#if defined(ITRON)

    ACP_UNUSED(aOutErrorMgr);
    ACP_UNUSED(aFormat);
    return ACI_SUCCESS;

#else

    va_list      sArgs;
    acp_char_t   sTimeBuf[128];

    /* LOCK */
    ACI_TEST_RAISE(ulaLogMgrLock(&gUlaLogMgr) != ACI_SUCCESS, ERR_LOCK)

    /* MESSAGE */
    ulaGetTimestampString(sTimeBuf, ACI_SIZEOF(sTimeBuf));
    (void)ulaLogMgrLogBody(&gUlaLogMgr,
                           "[%s] [Thread-%llu]\n",
                           sTimeBuf,
                           acpThrGetSelfID());

    va_start(sArgs, aFormat);

    // fix BUG-28453
    // 오버로딩된 logBody()를 분리
    (void)ulaLogMgrLogBodyV(&gUlaLogMgr, aFormat, sArgs);

    va_end(sArgs);

    (void)ulaLogMgrLogBody(&gUlaLogMgr, "\n\n");

    (void)ulaLogMgrFlush(&gUlaLogMgr);

    /* UNLOCK */
    ACI_TEST_RAISE(ulaLogMgrUnlock(&gUlaLogMgr) != ACI_SUCCESS, ERR_UNLOCK)

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LOG_MGR_LOCK);
    }
    ACI_EXCEPTION(ERR_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LOG_MGR_UNLOCK);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;

#endif
}

ACI_RC ulaLogError(ulaErrorMgr *aOutErrorMgr)
{
    acp_char_t   sTimeBuf[128];

    /* LOCK */
    ACI_TEST_RAISE(ulaLogMgrLock(&gUlaLogMgr) != ACI_SUCCESS, ERR_LOCK)

    /* MESSAGE */
    ulaGetTimestampString(sTimeBuf, ACI_SIZEOF(sTimeBuf));

    (void)ulaLogMgrLogBody(&gUlaLogMgr,
                           "[%s] [Thread-%llu]\n",
                           sTimeBuf,
                           acpThrGetSelfID());

    (void)ulaLogMgrLogBody(&gUlaLogMgr,
                           "ERR-%05x(errno=%u) %s\n\n",
                           ACI_E_ERROR_CODE(ulaGetErrorCode(aOutErrorMgr)),
                           errno,
                           ulaGetErrorMessage(aOutErrorMgr));

    (void)ulaLogMgrFlush(&gUlaLogMgr);

    /* UNLOCK */
    ACI_TEST_RAISE(ulaLogMgrUnlock(&gUlaLogMgr) != ACI_SUCCESS, ERR_UNLOCK)

    return ACI_SUCCESS;

    ACI_EXCEPTION(ERR_LOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LOG_MGR_LOCK);
    }
    ACI_EXCEPTION(ERR_UNLOCK)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_IGNORE_LOG_MGR_UNLOCK);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
