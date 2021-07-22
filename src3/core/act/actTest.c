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
 * $Id: actTest.c 6394 2009-07-17 01:16:24Z gurugio $
 ******************************************************************************/

#include <acpInit.h>
#include <actTest.h>


enum
{
    ACT_TEST_EXIT_SUCCESS = 0,
    ACT_TEST_EXIT_FAILURE,
    ACT_TEST_EXIT_SIGNALED,
    ACT_TEST_EXIT_UNINITIALIZED,
    ACT_TEST_EXIT_UNEXPECTED
};


static acp_sint32_t gExitCode = ACT_TEST_EXIT_UNINITIALIZED;

/*
 * Test Exception Handler
 * Catches every signal and trace the call stack
 */
static void actExceptionHandler(acp_sint32_t     aSignal,
                                acp_callstack_t *aCallstack)
{
    acp_rc_t sRC;
    const acp_char_t* sSignalName;

    switch(aSignal)
    {
    case SIGINT:
        sSignalName = "SIGINT";
        break;
    case SIGILL:
        sSignalName = "SIGILL";
        break;
    case SIGABRT:
        sSignalName = "SIGABRT";
        break;
    case SIGFPE:
        sSignalName = "SIGFPE";
        break;
    case SIGSEGV:
        sSignalName = "SIGSEGV";
        break;
    case SIGTRAP:
        sSignalName = "SIGTRAP";
        break;
    case SIGBUS:
        sSignalName = "SIGBUS";
        break;
    default:
        sSignalName = "Unknown";
        break;
    }

    (void)acpPrintf("Unhandled Exception SIGNAL=%s[%d]\n",
                    sSignalName, aSignal);

    sRC = acpCallstackDumpToStdFile(aCallstack, ACP_STD_OUT);

    if (ACP_RC_IS_EFAULT(sRC))
    {
        (void)acpPrintf("* acpCallstackDump() interrupted by signal *\n");
    }
    else if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpPrintf("* acpCallstackDump() returned error (%d) *\n", sRC);
    }
    else
    {
        /* do nothing */
    }


    /* In debug mode, ABORT signal is sent again. */
# if defined (ACP_CFG_DEBUG)

    /* Remove custom signal handler,
     * then ABORT signal can be handled by system default handler.
     */
    sRC = acpSignalSetExceptionHandler(NULL);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        /* If it is failed to change signal handler,
         * exit process.
         */
        acp_char_t sMsg[100];
        acpErrorString(sRC, sMsg, sizeof(sMsg));

        acpProcExit(ACT_TEST_EXIT_UNEXPECTED);
    }
    else
    {
        /* System default signal handler is applied */
        
        /* send abort signal to itself.
         * Core dump generated in UNIX.
         * In Windows, abnormal process terminating happens.
         */
        acpProcAbort();
    }

    /* never reach here */

# else

    acpProcExit(ACT_TEST_EXIT_SIGNALED);
#endif
    
}


/*
 * Prepare for test environment
 */
ACP_EXPORT void actTestBegin(void)
{
    acp_char_t sMsg[100];
    acp_rc_t   sRC;

    sRC = acpInitialize();

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        acpErrorString(sRC, sMsg, sizeof(sMsg));

        (void)acpPrintf("FAIL acpInitialize(): [%d] %s\n", sRC, sMsg);

        acpProcExit(ACT_TEST_EXIT_UNEXPECTED);
    }
    else
    {
        /* do nothing */
    }

    sRC = acpSignalSetExceptionHandler(actExceptionHandler);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        acpErrorString(sRC, sMsg, sizeof(sMsg));

        (void)acpPrintf("FAIL acpSignalSetExceptionHandler(): [%d] %s\n",
                        sRC,
                        sMsg);

        acpProcExit(ACT_TEST_EXIT_UNEXPECTED);
    }
    else
    {
        /* do nothing */
    }

    gExitCode = ACT_TEST_EXIT_SUCCESS;
}

/*
 * Finalize test environment
 */
ACP_EXPORT void actTestEnd(void)
{
    acp_char_t sMsg[100];
    acp_rc_t   sRC;

    sRC = acpFinalize();

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        acpErrorString(sRC, sMsg, sizeof(sMsg));

        (void)acpPrintf("FAIL acpFinalize(): [%d] %s\n", sRC, sMsg);

        acpProcExit(ACT_TEST_EXIT_UNEXPECTED);
    }
    else
    {
        acpProcExit(gExitCode);
    }
}

/*
 * Print error to stdout
 */
ACP_EXPORT void actTestError(const acp_char_t *aFileName, acp_sint32_t aLine)
{
    (void)acpPrintf("%s:%d: ", aFileName, aLine);

    gExitCode = ACT_TEST_EXIT_FAILURE;
}
