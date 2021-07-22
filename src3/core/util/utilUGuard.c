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
 * $Id: utilUGuard.c 4817 2009-03-12 00:20:05Z sjkim $
 ******************************************************************************/

#include <acp.h>
#include <aceError.h>

acp_sint32_t utilGetCurrentTimeout(void);

#define UTIL_DEFAULT_TIMEOUT               300   /* one minute */

#define UTIL_NORMAL_EXIT        0
#define UTIL_ERROR_RUNNING      1
#define UTIL_TIMEOUT            2
#define UTIL_CHILD_ERROR        3

#define UTIL_TIMEOUT_ENV_NAME  "CORE_UTIL_GUARD_TIMEOUT"

/*
 * help title
 */
#define UTILUGUARD_HELP_TITLE                           \
    "Usage: utilUGuard [exec arg1 arg2 ...]\n"          \
    "\tCurrent TIMEOUT is %d sec. \n"                   \
    "\tchange)export "UTIL_TIMEOUT_ENV_NAME"=120 \n"    \
    "\tExit code;\n"                                    \
    "\t\tNormal run            : 0\n"                   \
    "\t\tError on running exec : 1\n"                   \
    "\t\tTimeout               : 2\n"

    

#define UTILUGUARD_EXIT_IF_FAIL(aExpr)              \
    do                                          \
    {                                           \
        acp_rc_t sExprRC_MACRO_LOCAL_VAR = aExpr;               \
                                                \
        if (ACP_RC_NOT_SUCCESS(sExprRC_MACRO_LOCAL_VAR))        \
        {                                       \
            (void)acpPrintf("%s:%d %s\n", __FILE__, __LINE__, #aExpr);  \
                                                                        \
            utilUGuardPrintErrorAndExit(sExprRC_MACRO_LOCAL_VAR);   \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while (0)

ACP_INLINE void utilUGuardPrintErrorAndExit(acp_rc_t aRC)
{
    acp_char_t sBuffer[1024];

    acpErrorString(aRC, sBuffer, sizeof(sBuffer));

    (void)acpPrintf("error(%d): %s\n", aRC, sBuffer);

    acpProcExit(UTIL_ERROR_RUNNING);
}



/*
 * print help
 */
ACP_INLINE void utilUGuardPrintHelpAndExit(void)
{
    (void)acpPrintf(UTILUGUARD_HELP_TITLE, utilGetCurrentTimeout());

    /* Set exit code as normal */
    acpProcExit(UTIL_NORMAL_EXIT);
}

/* ------------------------------------------------
 *  Execute 
 * ----------------------------------------------*/

acp_sint32_t utilDoExecChild(acp_sint32_t aArgc,
                             acp_char_t **aArgv,
                             acp_sint32_t aTimeOut)
{
    acp_proc_t      sChild = ACP_PROC_T_INITIALIZER;
    acp_proc_wait_t sWait;
    acp_char_t     *sArray[256] = { NULL , };
    acp_sint32_t    i;
    acp_sint32_t    sRet;

    for (i = 2; i < aArgc ; i++)
    {
        sArray[i - 2] = aArgv[i];
    }

    UTILUGUARD_EXIT_IF_FAIL(acpProcForkExec(&sChild,
                                            aArgv[1],
                                            sArray,
                                            ACP_TRUE));
    
    sRet = 0;

    /* Calculate timeout */
    for (i = 0; i < aTimeOut * 10; i++)
    {
        acp_rc_t   sRC;
        
        sRC = acpProcWait(&sChild,
                          &sWait,
                          ACP_FALSE);
        
        if (sRC == ACP_RC_SUCCESS && sWait.mState == ACP_PROC_EXITED)
        {
            break;
        }
        else
        {
            /* continue running */
            acpSleepMsec(100);
        }
        
    }

    /* ------------------------------------------------
     * Kill the child in timeout
     * ----------------------------------------------*/
    
    if (i == aTimeOut * 10)
    {
        (void)acpProcKill(&sChild);
        
        (void)acpFprintf(
            ACP_STD_OUT,
            "\tkilled by timeout(%d seconds).\n",
            aTimeOut);
        (void)acpStdFlush(ACP_STD_OUT);

        /* Set exit code as timeout */
        sRet = UTIL_TIMEOUT;
    }
    else
    {
        /* 
         * process finished in time, 
         * but returning the child process' error code 
         */
        /* sRet = UTIL_NORMAL_EXIT; */
        sRet = (sWait.mExitCode == 0)?
            UTIL_NORMAL_EXIT :
            UTIL_CHILD_ERROR;
    }
    
    return sRet;
}


/* ------------------------------------------------
 *  Main
 * ----------------------------------------------*/

acp_sint32_t utilGetCurrentTimeout(void)
{
    acp_rc_t           sRC;
    acp_sint32_t       sTimeOut;
    acp_char_t        *sEnvVal = NULL;
    
    /* ------------------------------------------------
     *  Getting Timeout Value from Env.
     * ----------------------------------------------*/
    
    sRC = acpEnvGet(UTIL_TIMEOUT_ENV_NAME, &sEnvVal);
    
    if (sRC == ACP_RC_SUCCESS)
    {
        acp_uint32_t sVal;
        acp_sint32_t sSign;

        if (sEnvVal != NULL)
        {
            UTILUGUARD_EXIT_IF_FAIL(acpCStrToInt32(sEnvVal,
                                                   128,
                                                   &sSign,
                                                   &sVal,
                                                   10,
                                                   NULL));
            sTimeOut = sSign * (acp_sint32_t)sVal;
        }
        else
        {
            /* set but NULL : use default */
            sTimeOut = UTIL_DEFAULT_TIMEOUT;
        }
    }
    else
    {
        /* use default */
        sTimeOut = UTIL_DEFAULT_TIMEOUT;
    }

    return sTimeOut;
}


acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_sint32_t       sTimeOut;
    
    if (aArgc < 2)
    {
        utilUGuardPrintHelpAndExit();
    }
    else
    {
        /* argc specified */
    }
    

    sTimeOut = utilGetCurrentTimeout();
    

    if (sTimeOut < 1)
    {
        sTimeOut = 1; /* set minimum */
    }
    else
    {
        /* nothing to do */
    }

    /* To return error code */
    return utilDoExecChild(aArgc, aArgv, sTimeOut);
}
