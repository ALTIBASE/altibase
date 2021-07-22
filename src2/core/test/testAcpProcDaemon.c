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
 * $Id: testAcpProcDaemon.c 11093 2010-05-19 07:12:58Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aclLog.h>
#include <acpProc.h>
#include <acpSleep.h>
#include <acpShm.h>
#include <acpCStr.h>

#define TEST_STRING_PARK "Park Tae-hwan (born September 27, 1989) is "         \
    "a South Korean swimmer. He is a member of the South Korean national "     \
    "swimming team, based in Taereung, Seoul. He won a gold medal "            \
    "in the 400 meter freestyle and a silver in the 200 meter freestyle "      \
    "events at the 2008 Summer Olympics. He is the first Asian swimmer "       \
    "to win a gold medal in Men's 400 metre freestyle event, and "             \
    "the first Korean to win a medal in swimming."
#define TEST_STRING_PARK_LENGTH acpCStrLen(TEST_STRING_PARK, MAX_SHM_SIZE)

#define TEST_STRING_SOSI "I LOVE TAE-YOUN!"
#define TEST_STRING_SOSI_LENGTH acpCStrLen(TEST_STRING_SOSI, MAX_SHM_SIZE)

#define MAX_SHM_SIZE       8192

#define TEST_PERM_RWRWRW (      \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

#define SHM_TRY_COUNT 10

#define TEST_STRING_LONGPATH "D:\\0123456789\\0123456789\\0123456789\\"  \
    "0123456789\\0123456789\\0123456789\\0123456789\\0123456789"         \
    "0123456789\\0123456789\\0123456789\\0123456789\\0123456789"         \
    "0123456789\\0123456789\\0123456789\\0123456789\\0123456789"         \
    "0123456789\\0123456789\\0123456789\\0123456789\\0123456789"         \
    "0123456789\\0123456789\\0123456789.exe"

static acp_str_t    gTestFilePath = ACP_STR_CONST("testAcpProcDaemon.log");

void testMain(acp_sint32_t aShmKey)
{
    acl_log_t    sLog;
    acp_rc_t     sRC;
    acp_sint32_t sCmp;
    acp_shm_t    sShm;

    /*
     * Daemon should not use console.
     */
    sRC = acpProcDetachConsole(NULL, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclLogOpen(&sLog,
                     &gTestFilePath,
                     ACL_LOG_TIMESTAMP,
                     ACL_LOG_LIMIT_DEFAULT);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    aclLogSetBackupSize(&sLog, 1000);

    aclLogDebugWrite(&sLog, "acpProcDaemonize test is started");

    /*
     * Daemon reads a message in the shared memory
     * and compare it with parent's message.
     */

    sRC = acpShmAttach(&sShm, aShmKey);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("Fails to attach shared memory"));

    sCmp = acpMemCmp(sShm.mAddr, TEST_STRING_PARK, TEST_STRING_PARK_LENGTH);
    ACT_CHECK_DESC(0 == sCmp,
                   ("Fails to read data from parent"));


    /* daemon write another message and parent will read it
     * to check that daemon is executed correctly. */
    acpMemCpy(sShm.mAddr, TEST_STRING_SOSI, TEST_STRING_SOSI_LENGTH);

    sRC = acpShmDetach(&sShm);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    aclLogDebugWrite(&sLog, "acpProcDaemonize test is finished");

    aclLogClose(&sLog);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_char_t     *sArgs[3];
    acp_proc_t      sProc;
    acp_rc_t        sRC;
    acp_sint32_t    sShmKey;
    acp_char_t      sKeyStr[16];
    acp_char_t      sLongParam[] = "--opt=012345678901234567890123456789";
    acp_char_t     *sManyArgs[11] = {sKeyStr,    sLongParam, sLongParam,
                                     sLongParam, sLongParam, sLongParam,
                                     sLongParam, sLongParam, sLongParam,
                                     NULL};

    ACT_TEST_BEGIN();
    
    if (aArgc > 1)
    {
        /*
         * This is the Daemon doing!
         */
        acp_sint32_t sSign;
        acp_uint32_t sResult;
        
        sRC = acpCStrToInt32(aArgv[1], 16, &sSign, &sResult, 10, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sShmKey = sSign * sResult;
        testMain(sShmKey);
    }
    else
    {
        acp_shm_t    sShm;
        acp_sint32_t i;
        acp_sint32_t sCmp;
        
        /*
         * Parent Does HERE!
         */

        /* use parent process id as a shm_key */
        sShmKey = (acp_sint32_t)acpProcGetSelfID();
        (void)acpSnprintf(sKeyStr, 15, "%d", sShmKey);

        /*
         * Parent passes a message to the daemon through shared memory.
         */

        for (i = 0; i < SHM_TRY_COUNT; i++)
        {
            sRC = acpShmCreate(sShmKey, MAX_SHM_SIZE, TEST_PERM_RWRWRW);
            if (ACP_RC_IS_SUCCESS(sRC))
            {
                break;
            }
            else
            {
                /* try another shared-memory key */
                sShmKey++;
            }
        }

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpShmAttach(&sShm, sShmKey);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        acpMemCpy(sShm.mAddr, TEST_STRING_PARK, TEST_STRING_PARK_LENGTH);

        /*
         * Launch daemon!
         */

        sArgs[0] = sKeyStr;
        sArgs[1] = NULL;

        /*
         * BUG - 30120 acpProcLaunchDaemon fails to execute daemon with
         * a large parameters set.
         * Try to start daemon with path longer then MAX_PATH on Windows
         */
#if defined(ALTI_CFG_OS_WINDOWS)
        sRC = acpProcLaunchDaemon(&sProc, TEST_STRING_LONGPATH,
                                  sArgs, ACP_TRUE);
        ACT_CHECK_DESC(ACP_RC_IS_ENAMETOOLONG(sRC),
                       ("Could run the daemon with "
                        "too long path to executable"));
#endif

        sRC = acpProcLaunchDaemon(&sProc, aArgv[0], sManyArgs, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Cannot launch a daemon with error %d",
                        sRC));

        /* read a message from the daemon,
         * it can take some time */
        for (i = 0; i < SHM_TRY_COUNT; i++)
        {
            sCmp = acpMemCmp(sShm.mAddr, TEST_STRING_SOSI,
                             TEST_STRING_SOSI_LENGTH);
            if (sCmp == 0)
            {
                break;
            }
            else
            {
                acpSleepSec(1);
            }
        }

        ACT_CHECK_DESC(0 == sCmp,
                   ("Fails to read data from daemon"));


#if defined(ALTI_CFG_OS_WINDOWS)
        /* remove "exe" extension and try to execute */
        aArgv[0][acpCStrLen(aArgv[0], 128) - 4] = '\0';
        sArgs[0] = sKeyStr;
        sArgs[1] = NULL;

        sRC = acpProcLaunchDaemon(&sProc, aArgv[0], sArgs, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Cannot launch a daemon which has no \".exe\""
                        "extension with error %d",
                        sRC));
#endif

        sRC = acpShmDetach(&sShm);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = acpShmDestroy(sShmKey);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    return 0;
}
