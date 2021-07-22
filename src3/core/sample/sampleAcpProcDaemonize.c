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
 * $Id: sampleAcpProcDaemonize.c 9116 2009-12-10 02:03:59Z gurugio $
 ******************************************************************************/


#include <acp.h>

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_rc_t     sRC;
    acp_char_t *sArgs[2];
    acp_proc_t sProcChild;
    acp_path_pool_t sPath;


    sRC = acpInitialize();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);

    if (aArgc > 1 && aArgv[1][0] == 'c')
    {
        (void)acpPrintf("CHILD GO!!\n");

        /*
         * no message must be printing when ACP_TRUE
         * otherwise messages are display
         */
        sRC = acpProcDetachConsole(NULL, ACP_TRUE);

        if (sRC == ACP_RC_SUCCESS)
        {
            (void)acpPrintf("daemonized\n");
        }
        else
        {
            (void)acpPrintf("error to daemonize\n");
        }
        while (1)
        {
            (void)acpPrintf("#\n");
            acpSleepSec(1);
        }
        return 0;

    }
    else if (aArgc == 1)
    {
        (void)acpPrintf("PARENT GO!\n");
    }
    else
    {
        (void)acpPrintf("option error\n");
        return 1;
    }



    sArgs[0] = "c";
    sArgs[1] = NULL;

    acpPathPoolInit(&sPath);

    sRC = acpProcLaunchDaemon(&sProcChild,
                          acpPathMakeExe("sampleAcpProcDaemonize", &sPath),
                          sArgs,
                          ACP_FALSE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROC_FAIL);

    acpPathPoolFinal(&sPath);

    (void)acpPrintf("child created..");
    (void)acpPrintf("check sampleAcpProcDaemonize running\n");

    /* new process is create but no wait here!!! */

    ACP_EXCEPTION(PROG_FAIL)
    {
    }
    
    ACP_EXCEPTION(PROC_FAIL)
    {
        acpPathPoolFinal(&sPath);
        (void)acpPrintf("fail to fork\n");
    }

    ACP_EXCEPTION_END;
        (void)acpFinalize();
    return 0;
}
