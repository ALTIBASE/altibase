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
 * $Id: testAcpProc.c 6943 2009-08-14 01:25:07Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpList.h>
#include <acpProc.h>
#include <acpProcMutex.h>
#include <acpSignal.h>
#include <acpSleep.h>
#include <acpStr.h>
#include <acpTime.h>

#define TEST_WAITNULL_COUNT 5
#define TEST_MUTEX_KEY_ONE (acp_key_t)acpProcGetSelfID()
#define TEST_MUTEX_KEY_TWO (acp_key_t)(acpProcGetSelfID() * 2)

#define TEST_EXITCODE 25
#define TEST_MAXPATH 255
#define TEST_FILENAME "testAcpProc.log"
#define TEST_MESSAGE "System Test of testAcpProc!"

#define TEST_PIPE_CMDLINE_ARG     "PIPE"
#define TEST_PIPE_CMDLINE_ARG_LEN sizeof(TEST_PIPE_CMDLINE_ARG)
#define TEST_PIPE_CMDLINE_CONTENT "PIPECONTENT"

#define TEST_PATHSEP_CMDLINE_ARG     "PATHSEP"
#define TEST_PATHSEP_CMDLINE_ARG_LEN sizeof(TEST_PATHSEP_CMDLINE_ARG)
#define TEST_PATHSEP_CMDLINE_CONTENT "/testswitch"
#define TEST_PATHSEP_CMDLINE_CONTENT_LEN sizeof(TEST_PATHSEP_CMDLINE_CONTENT)

typedef struct acpProcBucket
{
    acp_list_t       mProcList;
    acp_uint32_t     mProcCount;
} acpProcBucket;

typedef struct acpProcNode
{
    acp_list_node_t mListNode;
    acp_uint32_t    mPID;
    acp_sint32_t    mIdx;
} acpProcNode;

typedef struct acpProcTestCase
{
    acp_uint32_t mPID;
    acp_sint32_t mAppearance;
} acpProcTestCase;

static acp_proc_wait_t gAcpProcWaitResult[] =
{
    /* testcase 0 - exit with 0 */
    {
        1,
        ACP_PROC_EXITED,
        0
    },
    /* testcase 1 - exit with 99 */
    {
        1,
        ACP_PROC_EXITED,
        ACP_PROC_EXITCODE(99)
    },
    /* testcase 2 - abort */
    {
        1,
        ACP_PROC_EXITED,
        ACP_PROC_SIGNALCODE(SIGABRT)
    },
    /* testcase 3 - segmentation fault */
    {
        1,
        ACP_PROC_EXITED,
#if defined(ALTI_CFG_CPU_PARISC) && !defined(__GNUC__)
        ACP_PROC_SIGNALCODE(SIGBUS)
#else
        ACP_PROC_SIGNALCODE(SIGSEGV)
#endif
    },
    /* testcase 4 - kill */
    {
        1,
        ACP_PROC_EXITED,
        ACP_PROC_SIGNALCODE(SIGKILL)
    },
    {
        0,
        0,
        0
    },
    {
        0,
        0,
        0
    }
};

static void testMain(acp_char_t *aExecPath, acp_sint32_t aCase)
{
    /* NULL does not generate SIGSEGV on 32bit */
    volatile acp_sint32_t* sPtr = NULL;
    volatile acp_sint32_t  sArray[1];
    acp_sint32_t  i;

    ACP_UNUSED(aExecPath);

    switch (aCase)
    {
        case 0:
            break;
        case 1:
            acpProcExit(99);
            break;
        case 2:
            acpProcAbort();
            break;
        case 3:
            *sPtr = 10;
            *((acp_sint32_t*)NULL) = 10;
            /* try segmentation fault */
            for(i = 0; i < 1048576; i++)
            {
                sArray[i] = i;
            }
#if defined(ALTI_CFG_OS_SOLARIS) && defined(ALTI_CFG_CPU_X86)
            /* SIGSEGV signal cannot be generated in SunOS 5.10
             * only if the process is executing in unit-test process.
             * This force the SEGSEGV generated.
             */
            /* ALINT:C17-DISABLE */
            (void)kill(getpid(), SIGSEGV);
            /* ALINT:C17-ENABLE */
#endif
            (void)acpPrintf("This line must not appear :"
                            " [%p] = %d\n", sPtr, *sPtr);
            break;
        case 4:
            /* Intentional Infinite loop */
            while(ACP_TRUE)
            {
                acpSleepSec(1);
            }
        default:
            break;
    }
}

static void testAcpProcError(void)
{
    acp_proc_t  sProc;
    acp_char_t *sArgs[2];
    acp_char_t *sFile = "testAcpProc.not_exist";
    acp_rc_t    sRC;

    sArgs[0] = NULL;

    sRC = acpProcForkExec(&sProc, sFile, sArgs, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
}

static void testAcpProcFunc(acp_char_t *aArgv[])
{
    acp_char_t       sArg[10];
    acp_char_t      *sArgs[3];
    acp_proc_wait_t  sWaitResult;
    acp_proc_t       sProc;
    acp_rc_t         sRC   = ACP_RC_EINVAL;
    acp_sint32_t     i;
    acp_proc_t       sWaitProc;

    sArgs[0] = sArg;
    sArgs[1] = NULL;

    for (i = 0; gAcpProcWaitResult[i].mPID != 0; i++)
    {
        (void)acpSnprintf(sArg, sizeof(sArg), "%d", i);

        sRC = acpProcForkExec(&sProc, aArgv[0], sArgs, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcForkExec should return SUCCESS but %d "
                        "in test case (%d)",
                        sRC,
                        i));

        if (gAcpProcWaitResult[i].mExitCode == ACP_PROC_SIGNALCODE(SIGKILL))
        {
            sRC = acpProcKill(&sProc);
            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                           ("acpProcKill should return SUCCESS but %d "
                            "in test case (%d)",
                            sRC,
                            i));
        }
        else
        {
        }

        sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sWaitProc = ACP_PROC_WR_PID(sWaitResult);
        ACT_CHECK_DESC(acpProcGetID(&sWaitProc)  == acpProcGetID(&sProc),
                       ("acpProcWait child process pid should be %u but %u "
                        "in test case (%d)",
                        acpProcGetID(&sProc),
                        (acp_uint32_t)ACP_PROC_WR_PID(sWaitResult),
                        i));
        ACT_CHECK_DESC(ACP_PROC_WR_STATE(sWaitResult) ==
                       gAcpProcWaitResult[i].mState,
                       ("acpProcWait child process state should be %d "
                        "but %d in test case (%d)",
                        gAcpProcWaitResult[i].mState,
                        ACP_PROC_WR_STATE(sWaitResult),
                        i));

        switch (ACP_PROC_WR_STATE(sWaitResult))
        {
            case ACP_PROC_EXITED:
                ACT_CHECK_DESC(
                    (ACP_PROC_WR_RETURNCODE(sWaitResult) ==
                     gAcpProcWaitResult[i].mExitCode)
                     ,
                     ("child process return code should be %d "
                      "but %d(%X) in test case (%d)",
                      gAcpProcWaitResult[i].mExitCode,
                      ACP_PROC_WR_RETURNCODE(sWaitResult),
                      sWaitResult.mExitCode,
                      i));
                break;
            default:
                ACT_ERROR(("acpProcWait child process wait state must be "
                           "exited but %d in testcase (%d)",
                           ACP_PROC_WR_STATE(sWaitResult), i));
                break;
        }
    }
}

static void testAcpProcFuncWithWaitNull(acp_char_t *aArgv[])
{
    acp_char_t      *sArgs[3];
    acp_proc_t       sProc;
    acp_proc_wait_t  sWaitResult;
    acp_rc_t         sRC   = ACP_RC_EINVAL;
    acp_sint32_t     i;
    acp_sint32_t     j;
    acpProcTestCase  sTest[TEST_WAITNULL_COUNT];

    sArgs[0] = "0";
    sArgs[1] = NULL;

    for (i = 0; i < TEST_WAITNULL_COUNT; i++)
    {
        sRC = acpProcForkExec(&sProc, aArgv[0], sArgs, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcForkExec should return SUCCESS but %d "
                        "in test case (%d)",
                        sRC,
                        i));

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sTest[i].mPID = acpProcGetID(&sProc);
            sTest[i].mAppearance = 0;
        }
        else
        {
        }
    }

    for (i = 0; gAcpProcWaitResult[i].mPID != 0; i++)
    {
        sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcWait should be SUCCESS, but %d "
                        "in test case (%d)",
                        sRC,
                        i));

        for (j = 0; j < TEST_WAITNULL_COUNT; j++)
        {
            if (ACP_PROC_WR_PID(sWaitResult) == (acp_proc_t)sTest[j].mPID)
            {
                ACT_CHECK(sTest[j].mAppearance == 0);
                sTest[j].mAppearance++;
            }
            else
            {
            }
        }
    }

    /* check test-result whether it has the value we hope  */
    for (i = 0; i < TEST_WAITNULL_COUNT; i++)
    {
        ACT_CHECK_DESC(sTest[i].mAppearance == 1,
                       ("There should be a unique process id, but "
                        "there is the %d number of adundant process id "
                        "in test case (%d)",
                        sTest[i].mAppearance,
                        i));
    }

    sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_ECHILD(sRC),
                   ("acpProcWait should be ACP_RC_ECHILD, but %d ", sRC));
}

static void testMainToCheckProcIsAlive(acp_sint32_t aCase,
                                       acp_key_t    aMutexKey1,
                                       acp_key_t    aMutexKey2)
{
    acp_bool_t       sIsAlive;
    acp_proc_t       sProc;
    acp_proc_mutex_t sMutex1;
    acp_proc_mutex_t sMutex2;
    acp_uint32_t     sPID = acpProcGetSelfID();
    acp_rc_t         sRC;

    switch (aCase)
    {
        case 1:
            sRC = acpProcMutexOpen(&sMutex1, aMutexKey1);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            sRC = acpProcMutexOpen(&sMutex2, aMutexKey2);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            sRC = acpProcMutexLock(&sMutex2);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            sRC = acpProcMutexLock(&sMutex1);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            sRC = acpProcFromPID(&sProc, sPID);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            sRC = acpProcIsAlive(&sProc, &sIsAlive);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);
            ACT_CHECK(sIsAlive == ACP_TRUE);

            sRC = acpProcIsAliveByPID(sPID, &sIsAlive);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);
            ACT_CHECK(sIsAlive == ACP_TRUE);

            acpProcExit(0);
            break;
        case 2:
            sRC = acpProcFromPID(&sProc, sPID);
            ACT_CHECK(sRC == ACP_RC_SUCCESS);

            sRC = acpProcKill(&sProc);
            ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                           ("acpProcKill should return SUCCESS but %d ",
                            sRC));
            break;
        default:
            ACT_ERROR(("This child proces shouldn't have this test case (%d)",
                       aCase));
            break;
    }
}

static void testAcpProcIsAlive(acp_char_t *aArgv[])
{
    acp_bool_t        sIsAlive;
    acp_char_t        sArg[10];
    acp_char_t        sMutexKey1[20];
    acp_char_t        sMutexKey2[20];
    acp_char_t       *sArgs[5];
    acp_proc_t        sProcChild;
    acp_proc_t        sProcSelf;
    acp_proc_t        sProcClon;
    acp_proc_mutex_t  sMutex1;
    acp_proc_mutex_t  sMutex2;
    acp_proc_wait_t   sWaitResult;
    acp_rc_t          sRC;
    acp_uint32_t      sPID  = acpProcGetSelfID();

    sRC = acpProcMutexCreate(TEST_MUTEX_KEY_ONE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcMutexCreate(TEST_MUTEX_KEY_TWO);
    if (ACP_RC_IS_EEXIST(sRC))
    {
        sRC = acpProcMutexDestroy(TEST_MUTEX_KEY_TWO);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcMutexDestroy should return SUCCESS but %d",
                        sRC));

        sRC = acpProcMutexCreate(TEST_MUTEX_KEY_TWO);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcMutexCreate should return SUCCESS but %d",
                        sRC));
    }
    else
    {
        /* do nothing */
    }

    (void)acpSnprintf(sMutexKey1,
                      sizeof(sMutexKey1),
                      "%u",
                      (acp_uint32_t)TEST_MUTEX_KEY_ONE);

    (void)acpSnprintf(sMutexKey2,
                      sizeof(sMutexKey2),
                      "%u",
                      (acp_uint32_t)TEST_MUTEX_KEY_TWO);

    sArgs[0] = sArg;
    sArgs[1] = sMutexKey1;
    sArgs[2] = sMutexKey2;
    sArgs[3] = NULL;

    sRC = acpProcMutexOpen(&sMutex1, TEST_MUTEX_KEY_ONE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcMutexOpen(&sMutex2, TEST_MUTEX_KEY_TWO);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcMutexLock(&sMutex1);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    (void)acpSnprintf(sArg, sizeof(sArg), "%d", 1);

    sRC = acpProcForkExec(&sProcChild, aArgv[0], sArgs, ACP_FALSE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    /* Test process liveness for child process */
    sRC = acpProcIsAlive(&sProcChild, &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_TRUE);

    sRC = acpProcIsAliveByPID(acpProcGetID(&sProcChild), &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_TRUE);

    sRC = acpProcFromPID(&sProcClon, acpProcGetID(&sProcChild));
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcIsAlive(&sProcClon, &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_TRUE);

    sRC = acpProcIsAliveByPID(acpProcGetID(&sProcClon), &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_TRUE);

    /* Test process liveness for self process */
    sRC = acpProcFromPID(&sProcSelf, sPID);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcIsAlive(&sProcSelf, &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_TRUE);

    sRC = acpProcIsAliveByPID(sPID, &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_TRUE);

    sRC = acpProcMutexUnlock(&sMutex1);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcWait(&sProcChild, &sWaitResult, ACP_TRUE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    /* Confirm the child-process was dead after waiting the process */
    sRC = acpProcIsAlive(&sProcChild, &sIsAlive);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
    ACT_CHECK(sIsAlive == ACP_FALSE);

    /* test acpProcKill when the function gets the handle
     * by using acpProcFromPID
     */
    (void)acpSnprintf(sArg, sizeof(sArg), "%d", 2);

    sRC = acpProcForkExec(&sProcChild, aArgv[0], sArgs, ACP_FALSE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcWait(&sProcChild, &sWaitResult, ACP_TRUE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    /*  Close  & Destroy Mutex */
    sRC = acpProcMutexClose(&sMutex1);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcMutexClose(&sMutex2);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcMutexDestroy(TEST_MUTEX_KEY_ONE);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);

    sRC = acpProcMutexDestroy(TEST_MUTEX_KEY_TWO);
    ACT_CHECK(sRC == ACP_RC_SUCCESS);
}

static void testAcpProcNoCldWait(acp_char_t *aArgv[])
{
    acp_bool_t        sIsAlive;
    acp_char_t       *sArgs[3];
    acp_proc_t        sProc;
    acp_proc_wait_t   sWaitResult;
    acp_rc_t          sRC;
    acp_sint32_t      i;

    sArgs[0] = "0";
    sArgs[1] = NULL;

    sRC = acpProcNoCldWait();
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /* test single child process */
    sRC = acpProcForkExec(&sProc, aArgv[0], sArgs, ACP_FALSE);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcForkExec should return SUCCESS but %d ", sRC));

    do
    {
        sRC = acpProcIsAlive(&sProc, &sIsAlive);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    while (sIsAlive == ACP_TRUE);

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_ECHILD(sRC),
                   ("acpProcWait should be ACP_RC_ECHILD, but %d ", sRC));

    /* test multiple child processes */
    for (i = 0; i < TEST_WAITNULL_COUNT; i++)
    {
        sRC = acpProcForkExec(&sProc, aArgv[0], sArgs, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpProcForkExec should return SUCCESS but %d ", sRC));

        do
        {
            sRC = acpProcIsAlive(&sProc, &sIsAlive);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        while (sIsAlive == ACP_TRUE);
    }

    sRC = acpProcWait(NULL, &sWaitResult, ACP_TRUE);
    ACT_CHECK_DESC(ACP_RC_IS_ECHILD(sRC),
                   ("acpProcWait should be ACP_RC_ECHILD, but %d ", sRC));
}

void testAcpProcSystem(acp_char_t* aArgv0)
{
    acp_rc_t sRC;
    acp_proc_wait_t sWR;
    acp_char_t sCommand[TEST_MAXPATH];
    acp_char_t sLine[TEST_MAXPATH];
    acp_size_t sRead;
    acp_file_t sFile;

    (void)acpCStrCpy(sCommand, TEST_MAXPATH, aArgv0, TEST_MAXPATH);
    (void)acpCStrCat(sCommand, TEST_MAXPATH,
                     " 1 2 3 4 5 6 7 8 9 > " TEST_FILENAME ,
                     TEST_MAXPATH);

    (void)acpMemSet(sLine, 0, TEST_MAXPATH);
    sRC = acpProcSystem(sCommand, &sWR);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(TEST_EXITCODE == ACP_PROC_WR_EXITCODE(sWR));

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpFileOpen(&sFile, TEST_FILENAME, ACP_O_RDONLY, 0)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            acpFileRead(&sFile, sLine, TEST_MAXPATH, &sRead)));
    ACT_CHECK(0 == acpCStrCmp(sLine, TEST_MESSAGE, TEST_MAXPATH));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sFile)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileRemove(TEST_FILENAME)));
}

void testAcpProcPipe(acp_char_t* aArgv0)
{
    acp_rc_t               sRC;
    acp_proc_t             sProc;
    acp_char_t             *sArgs[3];
    acp_char_t             sArg[]        = TEST_PIPE_CMDLINE_ARG;
    acp_char_t             sArgContent[] = TEST_PIPE_CMDLINE_CONTENT;
    acp_proc_std_handles_t sStdHandles;
    acp_file_t             sInPipe[2];
    acp_file_t             sOutPipe[2];
    acp_file_t             sErrPipe[2];
    acp_char_t             sBuffer[1024];
    acp_size_t             sSize;

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFilePipe(sInPipe)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFilePipe(sOutPipe)));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFilePipe(sErrPipe)));

    sStdHandles.mStdIn = &sInPipe[ACP_PIPE_READ];
    sStdHandles.mStdOut = &sOutPipe[ACP_PIPE_WRITE];
    sStdHandles.mStdErr = &sErrPipe[ACP_PIPE_WRITE];

    sArgs[0] = sArg;
    sArgs[1] = sArgContent;
    sArgs[2] = NULL;

    sRC = acpProcForkExecEnvPipe(&sProc,
                                 aArgv0,
                                 sArgs,
                                 NULL,
                                 ACP_FALSE,
                                 ACP_FALSE,
                                 &sStdHandles);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileRead(&sOutPipe[ACP_PIPE_READ], &sBuffer, 1024, &sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    ACT_CHECK(0 == acpCStrCmp(sBuffer, sArgContent, sSize));

    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sInPipe[ACP_PIPE_READ])));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sInPipe[ACP_PIPE_WRITE])));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sOutPipe[ACP_PIPE_READ])));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sOutPipe[ACP_PIPE_WRITE])));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sErrPipe[ACP_PIPE_READ])));
    ACT_CHECK(ACP_RC_IS_SUCCESS(acpFileClose(&sErrPipe[ACP_PIPE_WRITE])));
}


void testAcpProcPathSeperator(acp_char_t* aArgv0)
{
    acp_char_t            *sArgs[3];
    acp_char_t             sArgContent[] = TEST_PATHSEP_CMDLINE_CONTENT;
    acp_char_t             sArg[]        = TEST_PATHSEP_CMDLINE_ARG;
    acp_char_t             sBuf[512];
    acp_proc_t             sProc;
    acp_proc_wait_t        sWaitResult;
    acp_rc_t               sRC;

    sArgs[0] = sArg;
    sArgs[1] = sArgContent;
    sArgs[2] = NULL;

    sRC = acpProcForkExecEnvPipeOpt(&sProc,
                                    aArgv0,
                                    sArgs,
                                    NULL,
                                    ACP_FALSE,
                                    ACP_FALSE,
                                    NULL,
                                    ACP_PROC_FORKEXEC_PRESERVE_PATHSEP);

    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    acpErrorString(sRC, sBuf, 512);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcWait returned %d (expected: %d - SUCCESS) - %s",
                    sRC,
                    ACP_RC_SUCCESS,
                    sBuf));
    ACT_CHECK_DESC(ACP_PROC_WR_STATE(sWaitResult) == ACP_PROC_EXITED,
                   ("child process state is %d (expected: %d - EXITED)",
                    ACP_PROC_WR_STATE(sWaitResult),
                    ACP_PROC_EXITED));
    ACT_CHECK_DESC(ACP_PROC_WR_EXITCODE(sWaitResult) == 0,
                   ("Path seperator in the argument was changed (expected: unchanged)"));

#ifdef ALTI_CFG_OS_WINDOWS
    sRC = acpProcForkExec(&sProc,
                          aArgv0,
                          sArgs,
                          ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpProcWait(&sProc, &sWaitResult, ACP_TRUE);
    acpErrorString(sRC, sBuf, 512);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpProcWait returned %d (expected: %d - SUCCESS) - %s",
                    sRC,
                    ACP_RC_SUCCESS,
                    sBuf));
    ACT_CHECK_DESC(ACP_PROC_WR_STATE(sWaitResult) == ACP_PROC_EXITED,
                   ("child process state is %d (expected: %d - EXITED)",
                    ACP_PROC_WR_STATE(sWaitResult),
                    ACP_PROC_EXITED));
    ACT_CHECK_DESC(ACP_PROC_WR_EXITCODE(sWaitResult) == 1,
                   ("Path seperator in the argument was unchanged (expected: changed)"));
#endif
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACT_TEST_BEGIN();

    if (aArgc > 1)
    {
        acp_sint32_t sSign;
        acp_uint64_t sVal;
        acp_str_t    sArg1 = ACP_STR_CONST(aArgv[1]);
        acp_rc_t     sRC;

        switch(aArgc)
        {
        case 3:
            if (acpCStrCmp(aArgv[1], TEST_PIPE_CMDLINE_ARG,
                           TEST_PIPE_CMDLINE_ARG_LEN) == 0)
            {
                (void)acpPrintf("%s", aArgv[2]);
            }
            else if (acpCStrCmp(aArgv[1], TEST_PATHSEP_CMDLINE_ARG,
                                TEST_PATHSEP_CMDLINE_ARG_LEN) == 0)
            {
                if (acpCStrCmp(aArgv[2], TEST_PATHSEP_CMDLINE_CONTENT,
                               TEST_PATHSEP_CMDLINE_CONTENT_LEN) == 0)
                {
                    /* path separator unaltered */
                    acpProcExit(0);
                }
                else
                {
                    /* path separator updated to Windows-style */
                    acpProcExit(1);
                }
            }
            else
            {
                /* case not handled */
                ACE_ASSERT(ACP_FALSE);
            }

            break;
        case 4:
            {
                acp_str_t    sArg2 = ACP_STR_CONST(aArgv[2]);
                acp_str_t    sArg3 = ACP_STR_CONST(aArgv[3]);
                acp_key_t    sMutexKey1;
                acp_key_t    sMutexKey2;
                acp_sint32_t sCase;

                sRC = acpStrToInteger(&sArg1, &sSign, &sVal, NULL, 0, 0);
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

                sCase = (acp_sint32_t)sVal * sSign;

                sRC = acpStrToInteger(&sArg2, &sSign, &sVal, NULL, 0, 0);
                ACT_CHECK(sRC == ACP_RC_SUCCESS);

                sMutexKey1 = (acp_key_t)((acp_sint32_t)sVal * sSign);

                sRC = acpStrToInteger(&sArg3, &sSign, &sVal, NULL, 0, 0);
                ACT_CHECK(sRC == ACP_RC_SUCCESS);

                sMutexKey2 = (acp_key_t)((acp_sint32_t)sVal * sSign);

                testMainToCheckProcIsAlive(sCase, sMutexKey1, sMutexKey2);
                break;
            }

        case 10:
            (void)acpPrintf(TEST_MESSAGE);
            acpSleepSec(1);
            acpProcExit(TEST_EXITCODE);
            break;

        default:
            (void)acpSignalSetExceptionHandler(NULL);

            sRC = acpStrToInteger(&sArg1, &sSign, &sVal, NULL, 0, 0);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

            testMain(aArgv[0], (acp_sint32_t)sVal * sSign);
            break;
        }
    }
    else
    {
        testAcpProcError();

        testAcpProcFunc(aArgv);

        testAcpProcFuncWithWaitNull(aArgv);

        testAcpProcIsAlive(aArgv);

        /*
         * Must come before testAcpProcNoCldWait() as the Windows
         * implementation of acpProcNoCldWait() toggles a global flag
         * that causes acpProcWait() to always return ACP_RC_ECHILD.
         */
        testAcpProcPathSeperator(aArgv[0]);

        testAcpProcNoCldWait(aArgv);

        testAcpProcSystem(aArgv[0]);

        testAcpProcPipe(aArgv[0]);
    }

    ACT_TEST_END();

    return 0;
}
