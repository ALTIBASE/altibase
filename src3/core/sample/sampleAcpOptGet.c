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
 * $Id: sampleAcpOptGet.c 6343 2009-07-14 04:02:49Z gurugio $
 ******************************************************************************/


#include <acpOpt.h>
#include <acpPrintf.h>
#include <acpProc.h>
#include <acpInit.h>



#define SAMPLE_ARG_LEN 99
#define SAMPLE_OPT_LEN 3

enum
{
    SAMPLE_OPT_UNKNOWN = 0,
    SAMPLE_OPT_USER,
    SAMPLE_OPT_HELP,
    SAMPLE_CMD_START,
    SAMPLE_CMD_STOP
};

static acp_opt_def_t gMyOpt[] =
{
    {
        SAMPLE_OPT_USER,
        ACP_OPT_ARG_REQUIRED,
        'u',
        "user",
        NULL,
        "USER",
        "specify name"
    },
    {
        SAMPLE_OPT_HELP,
        ACP_OPT_ARG_NOTEXIST,
        'h',
        "help",
        NULL,
        NULL,
        "display this help and exit"
    },
    {
        0,
        ACP_OPT_ARG_NOTEXIST,
        0,
        NULL,
        NULL,
        NULL,
        NULL
    }
};

static acp_opt_cmd_t gMyCmd[] =
{
    {
        SAMPLE_CMD_START,
        "start",
        "start the service"
    },
    {
        SAMPLE_CMD_STOP,
        "stop",
        "stop the service"
    },
    {
        0,
        NULL,
        NULL
    }
};



acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_opt_t    sOpt;
    acp_sint32_t sValue;
    acp_sint32_t sCmd;
    acp_rc_t     sRC;

    acp_char_t *sArg;

    acp_char_t sUser[SAMPLE_OPT_LEN + 1] = {0,};
    acp_char_t sName[SAMPLE_OPT_LEN + 1] = {0,};
    acp_char_t sError[SAMPLE_ARG_LEN + 1];
    acp_char_t sHelp[SAMPLE_ARG_LEN + 1];



    sRC = acpInitialize();
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);


    sCmd = 0;

    sRC = acpOptInit(&sOpt, aArgc, aArgv);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), PROG_FAIL);



    while (1)
    {
        sRC = acpOptGet(&sOpt, gMyOpt, gMyCmd, &sValue, 
                        &sArg, sError, SAMPLE_ARG_LEN);


        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else if (ACP_RC_IS_SUCCESS(sRC))
        {
            /*
             * continue
             */
        }
        else
        {
            /*
             * error
             */
            (void)acpPrintf("%s\n", sError);
            break;
        }


        switch (sValue)
        {
            case SAMPLE_OPT_USER:
                /*
                 * user option specified
                 */
                if (acpCStrLen(sUser, SAMPLE_OPT_LEN) != 0)
                {
                    /*
                     * error; already specified
                     */
                }
                else
                {
                    (void)acpCStrCpy(sUser, SAMPLE_OPT_LEN, 
                                     sArg, SAMPLE_ARG_LEN);
                }
                break;
            case SAMPLE_OPT_HELP:
                /*
                 * help option specified
                 */
                (void)acpOptHelp(gMyOpt, gMyCmd, sHelp, SAMPLE_ARG_LEN);
                (void)acpPrintf("%s", sHelp);

                goto PROG_FAIL;

                break;
            case SAMPLE_CMD_START:
                /*
                 * start command specified
                 */
                if (sCmd != 0)
                {
                    /*
                     * error; already specified
                     */
                }
                else
                {
                    sCmd = SAMPLE_CMD_START;
                }
                break;
            case SAMPLE_CMD_STOP:
                /*
                 * stop command specified
                 */
                if (sCmd != 0)
                {
                    /*
                     * error; already specified
                     */
                }
                else
                {
                    sCmd = SAMPLE_CMD_STOP;
                }
                break;
            case 0:
            default:
                /*
                 * undefined option or command
                 */
                if (acpCStrLen(sName, SAMPLE_OPT_LEN) != 0)
                {
                    /*
                     * error; already specified
                     */
                    goto PROG_FAIL;
                }
                else
                {
                    (void)acpCStrCpy(sName, SAMPLE_OPT_LEN, 
                                     sArg, SAMPLE_ARG_LEN);
                }
                break;
        }
    }


    (void)acpPrintf("command = %d\n", sCmd);

    if (acpCStrLen(sUser, SAMPLE_OPT_LEN) != 0)
    {
        (void)acpPrintf("user = %s\n", sUser);
    }
    else
    {
        (void)acpPrintf("user = (undefined)\n");
    }

    if (acpCStrLen(sName, SAMPLE_OPT_LEN) != 0)
    {
        (void)acpPrintf("name = %s\n", sName);
    }
    else
    {
        (void)acpPrintf("name = (undefined)\n");
    }


    (void)acpFinalize();

    return 0;


    ACP_EXCEPTION(PROG_FAIL)


    ACP_EXCEPTION_END;
        (void)acpFinalize();
    return 1;


}
