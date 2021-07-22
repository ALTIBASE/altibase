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
 * $Id: genmsg.c 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#include <acp.h>
#include <aceError.h>
#include <genmsg.h>


/*
 * help title
 */
#define GENMSG_HELP_TITLE                               \
    "Usage: genmsg [OPTION]... -o FORMAT INPUT_FILE\n"  \
    "Generate CODE/TEXT/HTML from MSG file.\n\n"

/*
 * option definition
 */
enum
{
    GENMSG_OPT_OUTPUT = 1,
    GENMSG_OPT_NOWARNING,
    GENMSG_OPT_VERBOSE,
    GENMSG_OPT_HELP
};

static acp_opt_def_t gOptDef[] =
{
    {
        GENMSG_OPT_OUTPUT,
        ACP_OPT_ARG_REQUIRED,
        'o',
        "output",
        "code",
        "FORMAT",
        "specify output format"
        ACP_OPT_HELP_NEWLINE
        "c[ode] -> C source code"
        ACP_OPT_HELP_NEWLINE
        "d[ump] -> dump messages (for debugging)"
        ACP_OPT_HELP_NEWLINE
        "t[ext] -> text manual"
        ACP_OPT_HELP_NEWLINE
        "h[tml] -> html manual"
    },
    {
        GENMSG_OPT_NOWARNING,
        ACP_OPT_ARG_NOTEXIST,
        'w',
        "nowarning",
        NULL,
        NULL,
        "suppress warnings"
    },
    {
        GENMSG_OPT_VERBOSE,
        ACP_OPT_ARG_NOTEXIST,
        'v',
        "verbose",
        NULL,
        NULL,
        "print informational messages"
    },
    {
        GENMSG_OPT_HELP,
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


/*
 * print help
 */
ACP_INLINE void genmsgPrintHelpAndExit(void)
{
    acp_char_t sHelp[1024] = {0, };
    
    GENMSG_EXIT_IF_FAIL(acpOptHelp(gOptDef, NULL, sHelp, sizeof(sHelp) - 1));

    (void)acpPrintf(GENMSG_HELP_TITLE);

    (void)acpPrintf("%s", sHelp);

    acpProcExit(0);
}

/*
 * parse context
 */
ACP_INLINE void genmsgParseContextInit(genmsgParseContext *aParseContext)
{
    acpMemSet(aParseContext, 0, sizeof(genmsgParseContext));

    ACP_STR_INIT_CONST(aParseContext->mProductName);
    ACP_STR_INIT_CONST(aParseContext->mProductAbbr);
    ACP_STR_INIT_DYNAMIC(aParseContext->mFilePath, 0, 64);

    aParseContext->mFlag        = GENMSG_FLAG_WARNING;
    aParseContext->mProductID   = -1;
}

ACP_INLINE void genmsgParseContextFinal(genmsgParseContext *aParseContext)
{
    acp_sint32_t i;

    for (i = 0; i < aParseContext->mErrMsgCount; i++)
    {
        if (aParseContext->mErrMsg[i] != NULL)
        {
            genmsgFreeMessage(aParseContext->mErrMsg[i]);
        }
        else
        {
            /* do nothing */
        }
    }

    for (i = 0; i < aParseContext->mLogMsgCount; i++)
    {
        if (aParseContext->mLogMsg[i] != NULL)
        {
            genmsgFreeMessage(aParseContext->mLogMsg[i]);
        }
        else
        {
            /* do nothing */
        }
    }

    ACP_STR_FINAL(aParseContext->mFilePath);
    acpMemFree(aParseContext->mBuffer);
    acpMemFree(aParseContext->mLang);
    acpMemFree(aParseContext->mErrMsg);
    acpMemFree(aParseContext->mLogMsg);
}

/*
 * parse file
 */
static void genmsgParseFile(genmsgParseContext *aParseContext)
{
    genmsgToken   sToken;
    acp_file_t    sFile;
    acp_stat_t    sStat;
    void         *sParser = NULL;
    acp_sint32_t  sTokenID;

    /*
     * read whole file content
     */
    GENMSG_EXIT_IF_FAIL(acpFileOpen(&sFile,
                                    acpStrGetBuffer(&aParseContext->mFilePath),
                                    ACP_O_RDONLY,
                                    0));

    GENMSG_EXIT_IF_FAIL(acpFileStat(&sFile, &sStat));

    GENMSG_EXIT_IF_FAIL(acpMemAlloc((void **)&aParseContext->mBuffer,
                                    (acp_size_t)sStat.mSize));

    GENMSG_EXIT_IF_FAIL(acpFileRead(&sFile,
                                    aParseContext->mBuffer,
                                    (acp_size_t)sStat.mSize,
                                    &aParseContext->mBufferSize));

    if (aParseContext->mBufferSize != (acp_size_t)sStat.mSize)
    {
        (void)acpPrintf("could not read %S to %zu bytes\n",
                        &aParseContext->mFilePath,
                        (acp_size_t)sStat.mSize);
        acpProcExit(1);
    }
    else
    {
        /* do nothing */
    }

    GENMSG_EXIT_IF_FAIL(acpFileClose(&sFile));

    aParseContext->mCursor   = aParseContext->mBuffer;
    aParseContext->mLimit    = aParseContext->mBuffer +
        aParseContext->mBufferSize - 1;
    aParseContext->mLine     = aParseContext->mBuffer;
    aParseContext->mLineNo   = 1;

    /*
     * parse
     */
    sParser = genmsgParseAlloc();
    ACP_TEST(NULL == sParser);

    while (1)
    {
        sTokenID = genmsgGetToken(aParseContext);

        sToken.mPtr    = aParseContext->mToken;
        sToken.mLen    = (acp_sint32_t)(aParseContext->mCursor -
                                        aParseContext->mToken);
        sToken.mLineNo = aParseContext->mLineNo;
        sToken.mColNo  = (acp_sint32_t)(sToken.mPtr - aParseContext->mLine + 1);

        if (sTokenID == GENMSG_TOKEN_EOF)
        {
            genmsgParse(sParser, sTokenID, sToken, aParseContext);
            break;
        }
        else if (sTokenID == GENMSG_TOKEN_INVALID)
        {
            (void)acpPrintf("%S:%d:%d: invalid token: \"%.*s\"\n",
                            &aParseContext->mFilePath,
                            sToken.mLineNo,
                            sToken.mColNo,
                            sToken.mLen,
                            sToken.mPtr);
            acpProcExit(1);
        }
        else
        {
            genmsgParse(sParser, sTokenID, sToken, aParseContext);
        }
    }

    genmsgParseFree(sParser);
    return;

    ACP_EXCEPTION_END;
    GENMSG_EXIT_IF_FAIL(ACP_RC_GET_OS_ERROR());
}

static void genmsgDump(genmsgParseContext *aParseContext)
{
    genmsgMessage *sMessage = NULL;
    acp_sint32_t   i;
    acp_sint32_t   j;

    (void)acpPrintf("========================================\n");
    (void)acpPrintf("                  DUMP\n");
    (void)acpPrintf("----------------------------------------\n");
    (void)acpPrintf("mProductID   = %d\n", aParseContext->mProductID);
    (void)acpPrintf("mProductName = %S\n", &aParseContext->mProductName);
    (void)acpPrintf("mProductAbbr = %S\n", &aParseContext->mProductAbbr);
    (void)acpPrintf("mLangCount   = %d\n", aParseContext->mLangCount);
    (void)acpPrintf("mLangSize    = %d\n", aParseContext->mLangSize);
    (void)acpPrintf("mErrMsgCount = %d\n", aParseContext->mErrMsgCount);
    (void)acpPrintf("mErrMsgSize  = %d\n", aParseContext->mErrMsgSize);
    (void)acpPrintf("mLogMsgCount = %d\n", aParseContext->mLogMsgCount);
    (void)acpPrintf("mLogMsgSize  = %d\n", aParseContext->mLogMsgSize);
    (void)acpPrintf("----------------------------------------\n");

    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        (void)acpPrintf("mLang[%d] = %S\n", i, &aParseContext->mLang[i]);
    }

    (void)acpPrintf("----------------------------------------\n");

    for (i = 0; i < aParseContext->mErrMsgCount; i++)
    {
        sMessage = aParseContext->mErrMsg[i];

        if (sMessage != NULL)
        {
            (void)acpPrintf("mErrMsg[%d] = (%d, %S)\n",
                            i,
                            sMessage->mErrID,
                            &sMessage->mKey);

            for (j = 0; j < aParseContext->mLangCount; j++)
            {
                (void)acpPrintf("    [%S] -> %S\n",
                                &aParseContext->mLang[j],
                                &sMessage->mMsg[j]);
            }
        }
        else
        {
            (void)acpPrintf("mErrMsg[%d] = (null)\n", i);
        }
    }

    (void)acpPrintf("----------------------------------------\n");

    for (i = 0; i < aParseContext->mLogMsgCount; i++)
    {
        sMessage = aParseContext->mLogMsg[i];

        if (sMessage != NULL)
        {
            (void)acpPrintf("mLogMsg[%d] = (%d, %S)\n",
                            i,
                            sMessage->mErrID,
                            &sMessage->mKey);

            for (j = 0; j < aParseContext->mLangCount; j++)
            {
                (void)acpPrintf("    [%S] -> %S\n",
                                &aParseContext->mLang[j],
                                &sMessage->mMsg[j]);
            }
        }
        else
        {
            (void)acpPrintf("mLogMsg[%d] = (null)\n", i);
        }
    }

    (void)acpPrintf("========================================\n");
}

/*
 * main
 */
acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    genmsgParseContext sParseContext;
    acp_opt_t          sOpt;
    acp_sint32_t       sValue;
    acp_char_t         sOutput = 'c';
    acp_rc_t           sRC;

    acp_char_t        *sArg = NULL;
    acp_char_t         sError[1024] = { 0, };
    
    /*
     * initialize
     */
    genmsgParseContextInit(&sParseContext);

    GENMSG_EXIT_IF_FAIL(acpOptInit(&sOpt, aArgc, aArgv));

    /*
     * get options
     */
    while (1)
    {
        sRC = acpOptGet(&sOpt, gOptDef, NULL, &sValue,
                        &sArg, sError, sizeof(sError) - 1);

        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else if (ACP_RC_IS_SUCCESS(sRC))
        {
            /* continue */
        }
        else
        {
            (void)acpPrintf("%s\n", sError);
            break;
        }

        switch (sValue)
        {
            case GENMSG_OPT_OUTPUT:
                sOutput = sArg[0];
                break;

            case GENMSG_OPT_NOWARNING:
                sParseContext.mFlag &= ~GENMSG_FLAG_WARNING;
                break;

            case GENMSG_OPT_VERBOSE:
                sParseContext.mFlag |= GENMSG_FLAG_VERBOSE;
                break;

            case GENMSG_OPT_HELP:
                genmsgPrintHelpAndExit();
                break;

            case 0:
                if (acpStrGetLength(&sParseContext.mFilePath) > 0)
                {
                    (void)acpPrintf("multiple input files are not allowed\n");
                    acpProcExit(1);
                }
                else
                {
                    GENMSG_EXIT_IF_FAIL(acpStrCpyCString(
                                            &sParseContext.mFilePath, sArg));
                }
                break;
        }
    }

    /*
     * parse
     */
    if (acpStrGetLength(&sParseContext.mFilePath) > 0)
    {
        genmsgParseFile(&sParseContext);
    }
    else
    {
        genmsgPrintHelpAndExit();
    }

    /*
     * generate output
     */
    switch (sOutput)
    {
        case 'c':
            genmsgGenerateCode(&sParseContext);
            break;
        case 'd':
            genmsgDump(&sParseContext);
            break;
        case 't':
            genmsgGenerateManual(&sParseContext);
            break;
        case 'h':
            (void)acpPrintf("the output format is currently not supported.\n");
            acpProcExit(1);
            break;
        default:
            (void)acpPrintf("invalid output format\n");
            acpProcExit(1);
            break;
    }

    /*
     * finalize
     */
    genmsgParseContextFinal(&sParseContext);

    return 0;
}
