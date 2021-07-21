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
 * $Id: testAcpOpt.c 5910 2009-06-08 08:20:20Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpOpt.h>


static acp_char_t *gHelp =
    "  start                         start the service\n"
    "  stop                          stop the service\n"
    "\n"
    "Mandatory arguments to long options are mandatory for short options too.\n"
    "  -a, --all                     do not ignore entries starting with .\n"
    "      --author                  with -l, print the author of each file\n"
    "  -C                            list entries by columns\n"
    "  -I, --ignore=PATTERN          do not list implied entries matching "
    "shell\n"
    "                                  PATTERN\n"
    "      --block-size=SIZE         use SIZE-byte blocks\n"
    "      --color[=COLOR]           control whether color is used to "
    "distinguish\n"
    "                                  file types\n"
    "      --preserve[=ATTR_LIST]    preserve the specified attributes\n"
    "                                  (default: mode,ownership,timestamps)\n"
    "  -h, --help                    print this help and exit\n";

enum
{
    TEST_RC_EINVAL = -2,
    TEST_RC_EOF    = -1,
    TEST_NONE      = 0,
    TEST_OPT_ALL,
    TEST_OPT_AUTHOR,
    TEST_OPT_COLUMN,
    TEST_OPT_IGNORE,
    TEST_OPT_BLOCK_SIZE,
    TEST_OPT_USER,
    TEST_OPT_PRESERVE,
    TEST_OPT_COLOR,
    TEST_OPT_BACKUP,
    TEST_OPT_HELP,
    TEST_CMD_START,
    TEST_CMD_STOP
};

static acp_opt_def_t gOptDefError1[] =
{
    {
        TEST_OPT_USER,
        ACP_OPT_ARG_REQUIRED,
        'u',
        NULL,
        NULL,
        "USER",
        "list only the file owned by USER"
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

static acp_opt_def_t gOptDefError2[] =
{
    {
        TEST_OPT_PRESERVE,
        ACP_OPT_ARG_OPTIONAL,
        'p',
        "preserve",
        "mode,ownership,timestamps",
        "ATTR_LIST",
        "preserve the specified attributes"
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

static acp_opt_def_t gOptDefError3[] =
{
    {
        TEST_OPT_BACKUP,
        ACP_OPT_ARG_OPTIONAL,
        'b',
        NULL,
        NULL,
        "CONTROL",
        "make a backup of each existing destination file"
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

static acp_opt_def_t gOptDefErrorDupShort[] =
{
    {
        TEST_OPT_ALL,
        ACP_OPT_ARG_NOTEXIST,
        'a',
        "all",
        NULL,
        NULL,
        "do not ignore entries starting with ."
    },
    {
        TEST_OPT_AUTHOR,
        ACP_OPT_ARG_NOTEXIST,
        'a',
        "author",
        NULL,
        NULL,
        "with -l, print the author of each file"
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

static acp_opt_def_t gOptDefErrorDupLong[] =
{
    {
        TEST_OPT_ALL,
        ACP_OPT_ARG_NOTEXIST,
        'a',
        "author",
        NULL,
        NULL,
        "do not ignore entries starting with ."
    },
    {
        TEST_OPT_COLUMN,
        ACP_OPT_ARG_NOTEXIST,
        'C',
        "column",
        NULL,
        NULL,
        "list entries by columns"
    },
    {
        TEST_OPT_AUTHOR,
        ACP_OPT_ARG_NOTEXIST,
        0,
        "author",
        NULL,
        NULL,
        "with -l, print the author of each file"
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

static acp_opt_def_t gOptDef[] =
{
    {
        TEST_OPT_ALL,
        ACP_OPT_ARG_NOTEXIST,
        'a',
        "all",
        NULL,
        NULL,
        "do not ignore entries starting with ."
    },
    {
        TEST_OPT_AUTHOR,
        ACP_OPT_ARG_NOTEXIST,
        0,
        "author",
        NULL,
        NULL,
        "with -l, print the author of each file"
    },
    {
        TEST_OPT_COLUMN,
        ACP_OPT_ARG_NOTEXIST,
        'C',
        NULL,
        NULL,
        NULL,
        "list entries by columns"
    },
    {
        TEST_OPT_IGNORE,
        ACP_OPT_ARG_REQUIRED,
        'I',
        "ignore",
        NULL,
        "PATTERN",
        "do not list implied entries matching shell"
        ACP_OPT_HELP_NEWLINE
        "PATTERN"
    },
    {
        TEST_OPT_BLOCK_SIZE,
        ACP_OPT_ARG_REQUIRED,
        0,
        "block-size",
        NULL,
        "SIZE",
        "use SIZE-byte blocks"
    },
    {
        TEST_OPT_COLOR,
        ACP_OPT_ARG_OPTIONAL,
        0,
        "color",
        NULL,
        "COLOR",
        "control whether color is used to distinguish"
        ACP_OPT_HELP_NEWLINE
        "file types"
    },
    {
        TEST_OPT_PRESERVE,
        ACP_OPT_ARG_OPTIONAL,
        0,
        "preserve",
        "mode,ownership,timestamps",
        "ATTR_LIST",
        "preserve the specified attributes"
    },
    {
        TEST_OPT_HELP,
        ACP_OPT_ARG_NOTEXIST,
        'h',
        "help",
        NULL,
        NULL,
        "print this help and exit"
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

static acp_opt_cmd_t gOptCmd[] =
{
    {
        TEST_CMD_START,
        "start",
        "start the service"
    },
    {
        TEST_CMD_STOP,
        "stop",
        "stop the service"
    },
    {
        0,
        NULL,
        NULL
    }
};


typedef struct testOptResult
{
    acp_sint32_t mValue;
    acp_str_t    mArg;
} testOptResult;

struct
{
    acp_sint32_t       mArgc;
    acp_char_t * const mArgv[10];
    testOptResult      mResult[10];
} gTestOpt[] =
{
    {
        5,
        {
            "prog", "-I", "1", "-I=2", "-aIC", NULL
        },
        {
            {
                TEST_OPT_IGNORE,
                ACP_STR_CONST("1")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("invalid use of option: -I")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("unknown option: -=")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("unknown option: -2")
            },
            {
                TEST_OPT_ALL,
                ACP_STR_CONST("")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("invalid use of option: -I")
            },
            {
                TEST_OPT_COLUMN,
                ACP_STR_CONST("")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        8,
        {
            "prog", "-I", "1", "-I", "--", "-I", "-", "-I", NULL
        },
        {
            {
                TEST_OPT_IGNORE,
                ACP_STR_CONST("1")
            },
            {
                TEST_OPT_IGNORE,
                ACP_STR_CONST("--")
            },
            {
                TEST_OPT_IGNORE,
                ACP_STR_CONST("-")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("invalid use of option: -I")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        6,
        {
            "prog", "--", "-", "--color", "1", "--color=2", NULL
        },
        {
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("invalid option: --")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("invalid option: -")
            },
            {
                TEST_OPT_COLOR,
                ACP_STR_CONST("")
            },
            {
                0,
                ACP_STR_CONST("1")
            },
            {
                TEST_OPT_COLOR,
                ACP_STR_CONST("2")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        5,
        {
            "prog", "--block-size", "1", "--block-size=2", "--block-size", NULL
        },
        {
            {
                TEST_OPT_BLOCK_SIZE,
                ACP_STR_CONST("1")
            },
            {
                TEST_OPT_BLOCK_SIZE,
                ACP_STR_CONST("2")
            },
            {
                TEST_RC_EINVAL,
                ACP_STR_CONST("invalid use of option: --block-size")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        6,
        {
            "prog", "start", "-a", "--author", "-C", "5", NULL
        },
        {
            {
                TEST_CMD_START,
                ACP_STR_CONST("start")
            },
            {
                TEST_OPT_ALL,
                ACP_STR_CONST("")
            },
            {
                TEST_OPT_AUTHOR,
                ACP_STR_CONST("")
            },
            {
                TEST_OPT_COLUMN,
                ACP_STR_CONST("")
            },
            {
                0,
                ACP_STR_CONST("5")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        3,
        {
            "prog", "start", "-a", NULL
        },
        {
            {
                TEST_CMD_START,
                ACP_STR_CONST("start")
            },
            {
                TEST_OPT_ALL,
                ACP_STR_CONST("")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        2,
        {
            "prog", "start", NULL
        },
        {
            {
                TEST_CMD_START,
                ACP_STR_CONST("start")
            },
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        1,
        {
            "prog", NULL
        },
        {
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    },
    {
        0,
        {
            NULL
        },
        {
            {
                TEST_RC_EOF,
                ACP_STR_CONST("")
            }
        }
    }
};


acp_sint32_t main(acp_sint32_t argc, acp_char_t *argv[])
{
    acp_opt_t    sOpt;
    acp_sint32_t sValue;
    acp_rc_t     sRC;
    acp_sint32_t i;
    acp_sint32_t j;
    acp_char_t  *sArg;
    acp_char_t   sHelp[1024];
    acp_char_t   sError[1024];
    

    ACT_TEST_BEGIN();

    sRC = acpOptInit(&sOpt, argc, argv);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpOptCheckOpts
     */
    sRC = acpOptCheckOpts(gOptDef);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    /*
     * acpOptGet
     */
    sRC = acpOptGet(&sOpt, gOptDef, gOptCmd, &sValue,
                    &sArg, sError, sizeof(sError));
    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    for (i = 0; gTestOpt[i].mArgc > 0; i++)
    {
        sRC = acpOptInit(&sOpt, gTestOpt[i].mArgc, gTestOpt[i].mArgv);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        for (j = 0; ; j++)
        {
            sRC = acpOptGet(&sOpt, gOptDef, gOptCmd, &sValue,
                            &sArg, sError, sizeof(sError));

            if (gTestOpt[i].mResult[j].mValue == TEST_RC_EOF)
            {
                ACT_CHECK_DESC(ACP_RC_IS_EOF(sRC),
                               ("acpGetOpt should return EOF but %d "
                                "at case (%d,%d)",
                                sRC,
                                i,
                                j));

                if (ACP_RC_NOT_EOF(sRC) && ACP_RC_NOT_SUCCESS(sRC))
                {
                    ACT_ERROR(("acpGetOpt returned error: %s", sError));
                }
                else
                {
                    /* do nothing */
                }

                break;
            }
            else if (gTestOpt[i].mResult[j].mValue == TEST_RC_EINVAL)
            {
                ACT_CHECK_DESC(ACP_RC_IS_EINVAL(sRC),
                               ("acpOptGet should return EINVAL but %d "
                                "at case (%d,%d)",
                                sRC,
                                i,
                                j));
                ACT_CHECK_DESC(acpStrCmpCString(&gTestOpt[i].mResult[j].mArg,
                                                sError,
                                                0) == 0,
                               ("acpOptGet should get error '%S' but '%s' "
                                "at case (%d,%d)",
                                &gTestOpt[i].mResult[j].mArg,
                                sError,
                                i,
                                j));
            }
            else
            {
                ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                               ("acpOptGet should return SUCCESS but %d "
                                "at case (%d,%d)",
                                sRC,
                                i,
                                j));
                ACT_CHECK_DESC(gTestOpt[i].mResult[j].mValue == sValue,
                               ("acpOptGet should get value %d but %d "
                                "at case (%d,%d)",
                                gTestOpt[i].mResult[j].mValue,
                                sValue,
                                i,
                                j));
                ACT_CHECK_DESC(acpStrCmpCString(&gTestOpt[i].mResult[j].mArg,
                                                sArg,
                                                0) == 0,
                               ("acpOptGet should get arg '%S' but '%s' "
                                "at case (%d,%d)",
                                &gTestOpt[i].mResult[j].mArg,
                                sArg,
                                i,
                                j));

                if (ACP_RC_IS_EOF(sRC))
                {
                    ACT_ERROR(("acpOptGet returned EOF"));
                }
                else if (ACP_RC_NOT_SUCCESS(sRC))
                {
                    ACT_ERROR(("acpGetOpt returned error: %s", sError));
                }
                else
                {
                    /* do nothing */
                }
            }
        }
    }

    /*
     * acpOptHelp
     */
    sRC = acpOptHelp(gOptDef, gOptCmd, sHelp, sizeof(sHelp));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrCmp(sHelp, gHelp, sizeof(sHelp)) == 0);

    sRC = acpOptHelp(gOptDef, gOptCmd, sHelp, sizeof(sHelp));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpOptHelp(gOptDefError1, NULL, sHelp, sizeof(sHelp));
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpOptHelp(gOptDefError2, NULL, sHelp, sizeof(sHelp));
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpOptHelp(gOptDefError3, NULL, sHelp, sizeof(sHelp));
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpOptCheckOpts(gOptDefErrorDupShort);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpOptCheckOpts(gOptDefErrorDupLong);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    ACT_TEST_END();

    return 0;
}
