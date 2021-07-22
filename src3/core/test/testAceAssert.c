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
 * $Id: testAcpCStr.c 4486 2009-02-10 07:37:20Z jykim $
 ******************************************************************************/

/*******************************************************************************
 * A Skeleton Code for Testcases
*******************************************************************************/

#include <acp.h>
#include <act.h>
#include <aceAssert.h>


static void testAceExceptionHandler(acp_sint32_t     aSignal,
                                    acp_callstack_t
                                    *aCallstack)
{
    /**********************************************************************
     * This is originally to ensure that callstack is printed properly
     * If any platform needed to be checked with callstack,
     * uncomment the lines.
     **********************************************************************

    acp_rc_t sRC;
    (void)acpPrintf("Handling SIGNAL %d. Printing Callstack.\n", aSignal);
    sRC = acpCallstackDumpToStdFile(aCallstack, ACP_STD_OUT);
    if(ACP_RC_IS_EFAULT(sRC))
    {
        (void)acpPrintf("* acpCallstackDump() interrupted by signal *\n");
    }
    else if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpPrintf("* acpCallstackDump() returned error (%d) *\n", sRC);
    }
    else
    {
    }
     ***********************************************************************
     * to test callstack, uncomment lines so far.
     **********************************************************************/

    ACP_UNUSED(aSignal);
    ACP_UNUSED(aCallstack);

    /* To use later */
    /* ACE_ASSERT(NULL == "Assertion in Signal Handler"); */
    acpProcExit(0);
}

void testAceAssertAnotherCallback(const acp_char_t *aExpr,
                       const acp_char_t *aFile,
                        acp_sint32_t aLine)
{
    ACP_UNUSED(aExpr);
    ACP_UNUSED(aFile);
     ACP_UNUSED(aLine);

    /* To use later */
    /* ACE_ASSERT(NULL == "Infinite Assertion on Purpose"); */
    acpProcExit(0);
}

void testAceAssertCallback(const acp_char_t *aExpr,
                       const acp_char_t *aFile,
                       acp_sint32_t aLine)
{
    testAceAssertAnotherCallback(aExpr, aFile, aLine);
}

void testAceAssertMakeAssert(void)
{
    /* Make ACE_ASSERT */
    ACE_ASSERT(0);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    ACP_UNUSED(aArgv);

    if(1 == aArgc) /* Parent Process */
    {
        /* Make a child process */
        acp_char_t* sParam[] = {"1", NULL};
        acp_proc_t sProc;
        acp_proc_wait_t sWait;
        acp_sint32_t sReturnCode;

        ACT_TEST_BEGIN();

        ACT_CHECK(ACP_RC_IS_SUCCESS(
                acpProcForkExec(&sProc, aArgv[0], sParam, ACP_FALSE)
                ));
        ACT_CHECK(ACP_RC_IS_SUCCESS(
                acpProcWait(&sProc, &sWait, ACP_TRUE)
                ));
        
        sReturnCode = ACP_PROC_WR_RETURNCODE(sWait);
        /* To test Abort Signal, Uncomment this line
        ACT_CHECK_DESC(
            (SIGABRT == sReturnCode),
            ("Signal code with ASSERT must be %d but %d returned",
             SIGABRT, sReturnCode)
            );
            */
    
        ACT_TEST_END();
    }
    else
    {
        /* Child Process */
        /* Make an infinite assertion */
        acp_rc_t sRC;

        sRC = acpInitialize();
        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            acpProcAbort();
        }
        else
        {
            sRC = acpSignalSetExceptionHandler(
                testAceExceptionHandler);

            aceSetAssertCallback(testAceAssertCallback);
            testAceAssertMakeAssert();
        }

        sRC = acpFinalize();
    }

    return 0;
}
