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
 * $Id:
 ******************************************************************************/

#include <acp.h>
#include <aceError.h>

/* Default library name */
#define UTIL_LIBRARY_NAME "alticore"

/* Version function names */
#define UTIL_MAJOR_FUNC   "acpVersionMajor"
#define UTIL_MINOR_FUNC   "acpVersionMinor"
#define UTIL_TERM_FUNC    "acpVersionTerm"
#define UTIL_PATCH_FUNC   "acpVersionPatch"

/* Options */
enum
{
    UTIL_NONE = 0,
    UTIL_HELP,
    UTIL_DIR,
    UTIL_LIB,
    UTIL_NOLOGO
};

static acp_opt_def_t gOptDef[] =
{
    {
        UTIL_HELP,
        ACP_OPT_ARG_NOTEXIST,
        'h', "help", NULL, "Help",
        "Prints this help screen."
    },
    {
        UTIL_DIR,
        ACP_OPT_ARG_REQUIRED,
        'p', "path", NULL, "Path",
        "Prints the alticore library version in the directory."
    },
    {
        UTIL_LIB,
        ACP_OPT_ARG_REQUIRED,
        'l', "lib", UTIL_LIBRARY_NAME, "Library",
        "Sets the library file name without prefix and postfix."
    },
    {
        UTIL_NOLOGO,
        ACP_OPT_ARG_NOTEXIST,
        'n', "nologo", NULL, NULL,
        "Do not print logo screen"
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

#define UTIL_HELP_LENGTH (2048)

/*******************************************************************************
 * utilPrintLogo : print logo message
 ******************************************************************************/
void utilPrintLogo(void)
{
    (void)acpPrintf("Alticore version check utility %d.%d.%d.%d",
                    acpVersionMajor(), acpVersionMinor(),
                    acpVersionTerm(), acpVersionPatch());
    (void)acpPrintf(" Built at %s %s\n", __DATE__, __TIME__);
}

/*******************************************************************************
 * utilPrintUsage : print help message
 ******************************************************************************/
void utilPrintUsage(void)
{
    acp_char_t  sHelp[UTIL_HELP_LENGTH];
    (void)acpOptHelp(gOptDef, NULL, sHelp, UTIL_HELP_LENGTH - 1);

    (void)acpPrintf("Usage : utilCoreVersion -p pathname [-l libname] ...\n");
    (void)acpPrintf("%s\n", sHelp);
}

/*******************************************************************************
 * utilPrintVersion : print version of aPath/aLibName.so (, sl, dll, dylib, etc)
 * aPath    : Path of the library
 * aLibName : Name of the library. Default is UTIL_LIBRARY_NAME("alticore")
 ******************************************************************************/
void utilPrintVersion(const acp_char_t* aPath, const acp_char_t* aLibName)
{
    typedef acp_uint32_t    utilVerFunc(void);
    const acp_char_t*       sFuncNames[] = 
    {
        UTIL_MAJOR_FUNC,
        UTIL_MINOR_FUNC,
        UTIL_TERM_FUNC,
        UTIL_PATCH_FUNC,
        NULL
    };

    acp_sint32_t    i;
    acp_rc_t        sRC;
    acp_dl_t        sDl;
    acp_char_t*     sPath = NULL;
    acp_uint32_t    sVersion;
    acp_path_pool_t sPool;

    utilVerFunc*    sFuncs = NULL;
    
    acpPathPoolInit(&sPool);
    sPath = acpPathFull(aPath, &sPool);
    ACP_TEST_RAISE(NULL == sPath, E_PATHFAIL);

    if(NULL == aLibName)
    {
        aLibName = UTIL_LIBRARY_NAME;
    }
    else
    {
        /* Do nothing */
    }

    (void)acpPrintf("Opening [%s]/[%s]\n", aPath, aLibName);
    sRC = acpDlOpen(&sDl, sPath, aLibName, ACP_TRUE);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_OPENFAIL);

    (void)acpPrintf("Opening successful. Now checking version...\n");
    for(i = 0; sFuncNames[i] != NULL; i++)
    {
        /* Iterate every version function */
        sFuncs = acpDlSym(&sDl, sFuncNames[i]);
        if(NULL == sFuncs)
        {
            (void)acpPrintf("\t[%s]/[%s] is not an alticore shared object\n",
                            aPath, aLibName);
            break;
        }
        else
        {
            sVersion = (*sFuncs)();
            (void)acpPrintf("\t%-15s = %d\n", sFuncNames[i], sVersion);
        }
    }

    (void)acpPrintf("All done. Closing shared object.\n");
    sRC = acpDlClose(&sDl);
    if(ACP_RC_IS_SUCCESS(sRC))
    {
        /* Do nothing */
    }
    else
    {
        (void)acpPrintf("Could not close shared object : errno=[%d]\n\t(%s)\n",
                        ACP_RC_GET_OS_ERROR(), acpDlError(&sDl));
    }

    acpPathPoolFinal(&sPool);

    return;

    ACP_EXCEPTION(E_PATHFAIL)
    {
        (void)acpPrintf("Could not resolve path : errno=[%d]\n",
                        ACP_RC_GET_OS_ERROR());
    }

    ACP_EXCEPTION(E_OPENFAIL)
    {
        (void)acpPrintf("Could not open [%s]/[%s] : errno=[%d];\n\t%s\n",
                        sPath, aLibName, sRC, acpDlError(&sDl));
        acpPathPoolFinal(&sPool);
    }

    ACP_EXCEPTION_END;
}

/*******************************************************************************
 * main function
 ******************************************************************************/
acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_rc_t        sRC;

    acp_bool_t      sLogo = ACP_TRUE;
    acp_bool_t      sHelp = ACP_FALSE;
    acp_sint32_t    sValue;
    acp_char_t*     sArg = NULL;

    acp_char_t*     sPath = NULL;
    acp_char_t*     sLib = NULL;

    acp_opt_t       sOpt;
    acp_char_t      sError[1024];

    sRC = acpOptInit(&sOpt, aArgc, aArgv);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_INITFAILED);

    while(1)
    {
        sRC = acpOptGet(&sOpt, gOptDef, NULL, &sValue,
                        &sArg, sError, sizeof(sError));

        if(ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else
        {
            /* Fall through */
        }

        if(ACP_RC_IS_SUCCESS(sRC))
        {
            switch(sValue)
            {
                case UTIL_HELP:
                    sHelp = ACP_TRUE;
                    break;
                case UTIL_DIR:
                    sPath = sArg;
                    break;
                case UTIL_LIB:
                    sLib = sArg;
                    break;
                case UTIL_NOLOGO:
                    sLogo = ACP_FALSE;
                    break;
                default:
                    break;
            }
        }
        else
        {
            ACP_RAISE(E_ERROR);
        }
    }

    if(ACP_TRUE == sLogo)
    {
        utilPrintLogo();
    }
    else
    {
        /* Do nothing */
    }

    if(NULL != sPath)
    {
        utilPrintVersion(sPath, sLib);
    }
    else
    {
        if(ACP_FALSE == sHelp)
        {
            sHelp = ACP_TRUE;
            (void)acpPrintf("Path option should be given!\n");
        }
        else
        {
            /* Do nothing */
        }
    }

    if(ACP_TRUE == sHelp)
    {
        utilPrintUsage();
    }
    else
    {
        /* Do nothing */
    }

    return 0;

    ACP_EXCEPTION(E_INITFAILED)
    {
        (void)acpPrintf("Initialize Failed : %d\n", sRC);
    }

    ACP_EXCEPTION(E_ERROR)
    {
        (void)acpPrintf("Internal Error : %s\n", sError);
    }

    ACP_EXCEPTION_END;

    return 1;
}
