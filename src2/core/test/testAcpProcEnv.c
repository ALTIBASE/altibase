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
#include <acpCStr.h>
#include <acpEnv.h>
#include <acpProc.h>

#define TEST_ENVVAR_NO      (9)
#define TEST_ENVIN          "TestEnvIn"
#define TEST_ENVNOIN        "TestEnvNoIn"
#define TEST_NOENV          "TestNoEnv"
#define TEST_ENVIN_LEN      (9)
#define TEST_ENVNOIN_LEN    (11)
#define TEST_NOENV_LEN      (9)

#define TEST_ENV_VARNAME    "NOBODYNOBODYBUTYOU"
#define TEST_ENV_VARVALUE   "IWANNANOBODYNOBODYBUTYOU"
#define TEST_ENV_VAR_LEN    (128)

static void testNoEnv(void)
{
    acp_uint32_t    i;
    acp_rc_t        sRC;
    acp_char_t*     sValue;

    acp_char_t      sEnvName[TEST_ENV_VAR_LEN];

    for(i = 0; i < TEST_ENVVAR_NO; i++)
    {
        (void)acpSnprintf(sEnvName, TEST_ENV_VAR_LEN, "%s%d",
                          TEST_ENV_VARNAME, i);
        sRC = acpEnvGet(sEnvName, &sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }
}

static void testEnv(acp_bool_t aCheckPath)
{
    acp_uint32_t    i;
    acp_rc_t        sRC;
    acp_char_t*     sValue;

    acp_char_t      sEnvName[TEST_ENV_VAR_LEN];
    acp_char_t      sEnvValue[TEST_ENV_VAR_LEN];

    for(i = 0; i < TEST_ENVVAR_NO; i++)
    {
        (void)acpSnprintf(sEnvName, TEST_ENV_VAR_LEN, "%s%d",
                          TEST_ENV_VARNAME, i);
        (void)acpSnprintf(sEnvValue, TEST_ENV_VAR_LEN, "%s%d",
                          TEST_ENV_VARVALUE, i);
        sRC = acpEnvGet(sEnvName, &sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK_DESC(0 == acpCStrCmp(sEnvValue, sValue, TEST_ENV_VAR_LEN),
                       ("%s must be %s but %s\n", sEnvName, sEnvValue, sValue)
                       );
    }

    sRC = acpEnvGet("PATH", &sValue);
    if(aCheckPath)
    {
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }
    else
    {
        ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));
    }
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACT_TEST_BEGIN();

    if (aArgc > 1)
    {
        if(0 == acpCStrCmp(aArgv[1], TEST_ENVIN, TEST_ENVIN_LEN))
        {
            testEnv(ACP_TRUE);
        }
        else if(0 == acpCStrCmp(aArgv[1], TEST_ENVNOIN, TEST_ENVNOIN_LEN))
        {
            testEnv(ACP_FALSE);
        }
        else if(0 == acpCStrCmp(aArgv[1], TEST_NOENV, TEST_NOENV_LEN))
        {
            testNoEnv();
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        static acp_char_t   sEnvHome[TEST_ENV_VAR_LEN];
        static acp_char_t   sEnvVars[9][TEST_ENV_VAR_LEN];
        static acp_char_t*  sArgs[2] = {NULL, NULL};
        static acp_char_t*  sEnvs[11];

        acp_char_t*     sHomeValue;
        acp_proc_t      sProc;
        acp_uint32_t    i;
        acp_rc_t        sRC;

        sArgs[0] = NULL;
        sEnvs[0] = NULL;
        sRC = acpProcForkExecEnv(
            &sProc, "ls",
            sArgs, sEnvs, ACP_TRUE, ACP_FALSE);
        ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

        sRC = acpProcForkExecEnv(
            &sProc, "veritasluxmea",
            sArgs, sEnvs, ACP_TRUE, ACP_TRUE);
        ACT_CHECK(ACP_RC_NOT_SUCCESS(sRC));

        /*
         * A test that last element of PATH works.
         * Commenting out; specific to my local box.
        sRC = acpProcForkExecEnv(
            &sProc, "atfrun",
            sArgs, sEnvs, ACP_TRUE, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpProcForkExecEnv(
            &sProc, "./folderA/folderB/ls",
            sArgs, sEnvs, ACP_TRUE, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        sRC = acpProcForkExecEnv(
            &sProc, "./folderA/folderB/ls",
            sArgs, sEnvs, ACP_TRUE, ACP_FALSE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        */

        sEnvs[0] = sEnvHome;
        sEnvs[1] = NULL;
        sArgs[0] = TEST_NOENV;
        sArgs[1] = NULL;

        sRC = acpEnvGet("HOME", &sHomeValue);
        if(ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpSnprintf(sEnvHome, TEST_ENV_VAR_LEN,
                              "HOME=%s", sHomeValue);
        }
        else
        {
            (void)acpCStrCpy(sEnvHome, TEST_ENV_VAR_LEN, "HOME=.", 6);
        }

        sRC = acpProcForkExecEnv(
            &sProc, aArgv[0], sArgs, sEnvs, ACP_FALSE, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Launching %s failed!", aArgv[0])
                       );
        sRC = acpProcWait(&sProc, NULL, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpEnvSet(TEST_ENV_VARNAME "0", TEST_ENV_VARVALUE "9", ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        for(i = 0; i < 9; i++)
        {
            (void)acpSnprintf(
                sEnvVars[i], TEST_ENV_VAR_LEN, "%s%d=%s%d",
                TEST_ENV_VARNAME , i,
                TEST_ENV_VARVALUE, i);
            sEnvs[i + 1] = sEnvVars[i];
        }
        sEnvs[10] = NULL;
        sArgs[0] = TEST_ENVIN;
        sArgs[1] = NULL;
        sRC = acpProcForkExecEnv(
            &sProc, aArgv[0], sArgs, sEnvs, ACP_TRUE, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Launching %s failed!", aArgv[0])
                       );
        sRC = acpProcWait(&sProc, NULL, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        sRC = acpProcForkExecEnv(
            &sProc, aArgv[0], sArgs, sEnvs, ACP_TRUE, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        sRC = acpProcWait(&sProc, NULL, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sArgs[0] = TEST_ENVNOIN;
        sArgs[1] = NULL;
        sRC = acpProcForkExecEnv(
            &sProc, aArgv[0], sArgs, sEnvs, ACP_FALSE, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Launching %s failed!", aArgv[0])
                       );
        sRC = acpProcWait(&sProc, NULL, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        sRC = acpProcForkExecEnv(
            &sProc, aArgv[0], sArgs, sEnvs, ACP_FALSE, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        sRC = acpProcWait(&sProc, NULL, ACP_TRUE);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    ACT_TEST_END();

    return 0;
}
