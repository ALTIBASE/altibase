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
 
/*******************************************************************************
 * $Id: testAcpSignal.c 9236 2009-12-17 02:57:25Z djin $
 ******************************************************************************/

#include <act.h>
#include <acp.h>
#include <acpInit.h>
#include <acpMem.h>
#include <acpProc.h>
#include <acpSignal.h>
#include <acpSock.h>
#include <acpStr.h>

#if !defined(ALINT) && defined(ALTI_CFG_OS_TRU64)
#include <sys/sysinfo.h>
#include <machine/hal_sysinfo.h>
#include <sys/proc.h>
#endif


static acp_sock_t gSockClient;


static void setupForSpecificOS(void)
{
#if !defined(ALINT) && defined(ALTI_CFG_OS_TRU64)
    acp_sint32_t sBuf[2];

    sBuf[0] = SSIN_UACPROC;
    sBuf[1] = UAC_SIGBUS | UAC_NOPRINT;

    (void)setsysinfo(SSI_NVPAIRS, sBuf, 1, 0, 0);
#endif
}


static acp_sint32_t openPipeThread(void *aArg)
{
    struct sockaddr_in sAddr;
    acp_rc_t           sRC;

    sAddr.sin_family = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_LOOPBACK);
    sAddr.sin_port        = (acp_uint16_t)((acp_ulong_t)aArg);

    sRC = acpSockOpen(&gSockClient, ACP_AF_INET, ACP_SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockConnect(&gSockClient,
                         (acp_sock_addr_t *)&sAddr,
                         (acp_sock_len_t)sizeof(sAddr));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    return 0;
}

static acp_rc_t openPipe(void)
{
    acp_char_t         sBuffer[10000];
    struct sockaddr_in sAddr;
    acp_sock_len_t     sAddrLen;
    acp_sock_t         sSockListen;
    acp_sock_t         sSockServer;
    acp_rc_t           sRC;

    acpMemSet(sBuffer, 1, sizeof(sBuffer));

    sAddr.sin_family      = ACP_AF_INET;
    sAddr.sin_addr.s_addr = ACP_TON_BYTE4_ARG(INADDR_ANY);
    sAddr.sin_port        = ACP_TON_BYTE2_ARG(45768);

    sRC = acpSockOpen(&sSockListen, ACP_AF_INET, SOCK_STREAM, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockBind(&sSockListen,
                      (acp_sock_addr_t *)&sAddr,
                      (acp_sock_len_t)sizeof(sAddr),
                      ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockListen(&sSockListen, 1);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sAddrLen = (acp_sock_len_t)sizeof(sAddr);
    sRC = acpSockGetName(&sSockListen, (acp_sock_addr_t *)&sAddr, &sAddrLen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    (void)openPipeThread((void *)((acp_ulong_t)sAddr.sin_port));

    sRC = acpSockAccept(&sSockServer, &sSockListen, NULL, 0);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSockServer);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockClose(&sSockListen);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    (void)acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);

    return ACP_RC_SUCCESS;
}

static void exceptionHandler(acp_sint32_t aSignal, acp_callstack_t *aCallstack)
{
    ACP_UNUSED(aCallstack);

    acpProcExit(aSignal);
}

void testSIGABRT(void)
{
    acpProcAbort();
}

void testSIGFPE(void)
{
    acp_sint32_t sValue1 = 1;
    acp_sint32_t sValue2 = 0;
    acp_sint32_t sValue;

    sValue = sValue1 / sValue2;
}

void testSIGSEGV(void)
{
#if defined(ALTI_CFG_CPU_PARISC) && !defined(__GNUC__)
            /* PA-RISC cpu does raise SIGBUS instead of SIGSEGV 
             * when writing to an invalid page
             * This force the SEGSEGV generated.
             */
            /* ALINT:C17-DISABLE */
            (void)kill(getpid(), SIGSEGV);
            /* ALINT:C17-ENABLE */
#elif defined(ALTI_CFG_OS_SOLARIS) && defined(ALTI_CFG_CPU_X86)
            /* SIGSEGV signal cannot be generated in SunOS 5.10
             * only if the process is executing in unit-test process.
             * This force the SEGSEGV generated.
             */
            /* ALINT:C17-DISABLE */
            (void)kill(getpid(), SIGSEGV);
            /* ALINT:C17-ENABLE */
#else
    volatile acp_sint32_t* sNull = NULL;
    volatile acp_sint32_t  sArray[1];
    acp_sint32_t  i;

    *sNull = 100;
    for(i = 0; i < 1048476; i++)
    {
        sArray[i] = i;
    }
#endif
    ACT_ERROR(("This line must not appear!\n"));
}

void testSIGBUS(void)
{
    union
    {
        acp_char_t   *mChar;
        acp_sint64_t *mLong;
    } sPtr;

    acp_sint64_t sValue = 0;

    sPtr.mLong = &sValue;

    sPtr.mChar++;

    if (*sPtr.mLong == 0)
    {
        /* sValue = 1; */
        *(sPtr.mLong) = 1;
    }
    else
    {
        /* sValue = 2; */
        *(sPtr.mLong) = 2;
    }
}

void testSIGPIPE1(void)
{
    acp_char_t sBuffer[10000];
    acp_rc_t   sRC;

    acpMemSet(sBuffer, 1, sizeof(sBuffer));

    sRC = openPipe();
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    (void)acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);
    (void)acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);
    (void)acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);
    (void)acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);
    sRC = acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);
#if defined (ALTI_CFG_OS_WINDOWS)
    ACT_CHECK_DESC(ACP_RC_IS_ECONNABORTED(sRC),
                   ("acpSockSend should return ECONNABORTED but %d", sRC));
#else
    ACT_CHECK_DESC(ACP_RC_IS_EPIPE(sRC),
                   ("acpSockSend should return EPIPE but %d", sRC));
#endif

    sRC = acpSockClose(&gSockClient);
#if defined(ALTI_CFG_OS_TRU64)
    ACT_CHECK(ACP_RC_IS_ECONNRESET(sRC));
#else
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
#endif
}

void testSIGPIPE2(void)
{
    acp_char_t sBuffer[10];
    acp_rc_t   sRC;

    acpMemSet(sBuffer, 1, sizeof(sBuffer));

    sRC = acpSignalBlockDefault();
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = openPipe();
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSockSend(&gSockClient, sBuffer, sizeof(sBuffer), NULL, 0);
    ACT_CHECK_DESC(ACP_RC_IS_EPIPE(sRC),
                   ("acpSockSend should return EPIPE but %d", sRC));

    sRC = acpSockClose(&gSockClient);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}


struct
{
    acp_char_t        *mDescription;
    void             (*mFunc)(void);
    acp_proc_state_t   mExitState;
    acp_sint32_t       mExitValue;
} gTestSignal[] =
{
#if 0
    /*
     * No SIGPIPE on Windows 
     */
    {
        "SIGPIPE1",
        testSIGPIPE1,
        ACP_PROC_EXITED,
        ACP_PROC_EXITCODE(SIGPIPE)
    },
#endif
#if 0
    /*
     * Exit code is strance on Windows
     */
    {
        "SIGABRT",
        testSIGABRT,
        ACP_PROC_EXITED,
        ACP_PROC_EXITCODE(SIGABRT)
    },
#endif
    {
        "SIGSEGV",
        testSIGSEGV,
        ACP_PROC_EXITED,
        ACP_PROC_EXITCODE(SIGSEGV)
    },
#if 0
    /*
     * testSIGFPE is eliminated with optimization. :(
     */
    {
        "SIGFPE",
        testSIGFPE,
        ACP_PROC_EXITED,
        ACP_PROC_EXITCODE(SIGFPE)
    },
#endif
#if 0
    /*
     * Intel CPU supports unaligned access :p
     */
    {
        "SIGBUS",
        testSIGBUS,
        ACP_PROC_EXITED,
        ACP_PROC_EXITCODE(SIGBUS)
    },
#endif
    {
        NULL,
        NULL,
        ACP_PROC_EXITED,
        0
    }
};

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_proc_t       sProc;
    acp_proc_wait_t  sWaitResult;
    acp_char_t      *sArgs[3];
    acp_char_t       sArg[10];
    acp_rc_t         sRC;
    acp_sint32_t     i;

    ACT_TEST_BEGIN();

    setupForSpecificOS();

    (void)acpInitialize();

    if (aArgc > 1)
    {
        acp_sint32_t sSign;
        acp_uint64_t sVal;
        acp_str_t    sArg1 = ACP_STR_CONST(aArgv[1]);

        sRC = acpStrToInteger(&sArg1, &sSign, &sVal, NULL, 0, 0);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

#if 0
        /*
         * This disables all signal except fatal.
         * Commenting out to test.
         */
        sRC = acpSignalBlockAll();
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
#endif

        sRC = acpSignalSetExceptionHandler(exceptionHandler);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        (*gTestSignal[(acp_sint32_t)sVal * sSign].mFunc)();
    }
    else
    {
        sArgs[0] = sArg;
        sArgs[1] = NULL;

        for (i = 0; gTestSignal[i].mFunc != NULL; i++)
        {
            (void)acpSnprintf(sArg, sizeof(sArg), "%d", i);

            sRC = acpProcForkExec(&sProc, aArgv[0], sArgs, ACP_FALSE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            ACT_CHECK_DESC(
                ACP_PROC_WR_STATE(sWaitResult) == gTestSignal[i].mExitState,
                ("child process exit state should be %d but %d "
                 "for testing %s in test case (%d)",
                 gTestSignal[i].mExitState,
                 ACP_PROC_WR_STATE(sWaitResult),
                 gTestSignal[i].mDescription,
                 i));

            if (ACP_PROC_WR_STATE(sWaitResult) == ACP_PROC_EXITED)
            {
                ACT_CHECK_DESC(
                    ACP_PROC_WR_RETURNCODE(sWaitResult) ==
                    gTestSignal[i].mExitValue,
                    ("child process exit code should be %d but %d "
                     "for testing %s in test case (%d)",
                     gTestSignal[i].mExitValue,
                     sWaitResult.mExitCode,
                     gTestSignal[i].mDescription,
                     i));
            }
            else
            {
                ACT_ERROR(("child process exited with unexpected state = %d "
                           "for testing %s in test case (%d)",
                           ACP_PROC_WR_STATE(sWaitResult),
                           gTestSignal[i].mDescription,
                           i));
            }
        }
    }

    ACT_TEST_END();

    return 0;
}
