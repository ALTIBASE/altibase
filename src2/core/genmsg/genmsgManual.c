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
 * $Id: genmsgManual.c 10912 2010-04-23 01:44:15Z djin $
 ******************************************************************************/

#include <acp.h>
#include <genmsg.h>


#define GENMSG_FILE_HEADER                                                     \
    "****************************************\n"                               \
    " * $%s$\n"                                                                \
    " * Generated from %S by genmsg.\n"                                        \
    " ***************************************"                                 \
    "***************************************/\n"                               \
    "\n"


#define GENMSG_SRC_TAIL                         \
    ""



#define GENMSG_MSG_FORMAT_HEADER                        \
    "\n"                                                \
    "LANGUAGE : %S \n"       

#define GENMSG_MSG_FORMAT_TAIL                  \
    "\n"



ACP_INLINE void genmsgFileBase(acp_str_t          *aFilePath,
                               genmsgParseContext *aParseContext)
{
    acp_sint32_t sIndexSlash = ACP_STR_INDEX_INITIALIZER;
    acp_sint32_t sIndexDot   = ACP_STR_INDEX_INITIALIZER;
    acp_rc_t     sRC;

    sRC = acpStrFindChar(&aParseContext->mFilePath,
                         '/',
                         &sIndexSlash,
                         sIndexSlash,
                         ACP_STR_SEARCH_BACKWARD);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        sIndexSlash = -1;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpStrFindChar(&aParseContext->mFilePath,
                         '.',
                         &sIndexDot,
                         sIndexDot,
                         ACP_STR_SEARCH_BACKWARD);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        sIndexDot = -1;
    }
    else
    {
        /* do nothing */
    }

    if (sIndexDot > sIndexSlash)
    {
        GENMSG_EXIT_IF_FAIL(
            acpStrCpyBuffer(aFilePath,
                            acpStrGetBuffer(&aParseContext->mFilePath),
                            sIndexDot));
    }
    else
    {
        GENMSG_EXIT_IF_FAIL(acpStrCpy(aFilePath, &aParseContext->mFilePath));
    }
}

ACP_INLINE void genmsgFileDir(acp_str_t          *aFilePath,
                              genmsgParseContext *aParseContext)
{
    acp_sint32_t sIndex = ACP_STR_INDEX_INITIALIZER;
    acp_rc_t     sRC;

    sRC = acpStrFindChar(&aParseContext->mFilePath,
                         '/',
                         &sIndex,
                         sIndex,
                         ACP_STR_SEARCH_BACKWARD);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        GENMSG_EXIT_IF_FAIL(acpStrCpyBuffer(
                                aFilePath,
                                acpStrGetBuffer(&aParseContext->mFilePath),
                                sIndex + 1));
    }
    else
    {
        acpStrClear(aFilePath);
    }
}


ACP_INLINE void genmsgPrintMessageAsMacro(acp_std_file_t *aFile,
                                          acp_str_t      *aMessage)
{
    const acp_char_t *sPtr      = acpStrGetBuffer(aMessage);
    acp_sint32_t      sLen      = (acp_sint32_t)acpStrGetLength(aMessage);
    acp_sint32_t      sOldIndex = 0;
    acp_sint32_t      sIndex    = ACP_STR_INDEX_INITIALIZER;
    acp_rc_t          sRC;

    while (1)
    {
        sRC = acpStrFindChar(aMessage, '\n', &sIndex, sIndex, 0);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            if (sIndex > sLen)
            {
                (void)acpFprintf(aFile,
                                 "%.*s\n",
                                 sLen - sOldIndex,
                                 sPtr + sOldIndex);
                break;
            }
            else
            {
                (void)acpFprintf(aFile,
                                 "%-70.*s \\\n",
                                 sIndex - sOldIndex,
                                 sPtr + sOldIndex);
                sIndex++;
                sOldIndex = sIndex;
            }
        }
        else if (ACP_RC_IS_ENOENT(sRC))
        {
            (void)acpFprintf(aFile,
                             "%.*s\n",
                             sLen - sOldIndex,
                             sPtr + sOldIndex);
            break;
        }
        else
        {
            genmsgPrintErrorAndExit(sRC);
        }
    }
}


static void genmsgGenerateMsgFormatArray(acp_std_file_t     *aFile,
                                         genmsgParseContext *aParseContext,
                                         const acp_char_t   *aType,
                                         acp_sint32_t        aLangCount,
                                         acp_sint32_t        aLang,
                                         acp_sint32_t        aMsgCount,
                                         genmsgMessage     **aMsg)
{
    acp_str_t    *sMsg = NULL;
    acp_sint32_t  i;
    acp_sint32_t  j;

    ACP_UNUSED(aType);
    
    /* language */
    (void)acpFprintf(aFile,
                     GENMSG_MSG_FORMAT_HEADER,
                     &aParseContext->mLang[aLang]);

    for (i = 0; i < aMsgCount; i++)
    {
        if (aMsg[i] == NULL)
        {
            /* skip */
        }
        else
        {
            sMsg = &aMsg[i]->mMsg[aLang];

            if (acpStrGetLength(&aMsg[i]->mMsg[aLang]) > 0)
            {
                sMsg = &aMsg[i]->mMsg[aLang];
            }
            else
            {
                for (j = 0; j < aLangCount; j++)
                {
                    if (acpStrGetLength(&aMsg[i]->mMsg[j]) > 0)
                    {
                        sMsg = &aMsg[i]->mMsg[j];
                        break;
                    }
                    else
                    {
                        /* continue */
                    }
                }
            }

            (void)acpFprintf(aFile, "[%.*s-%05x] ",
                             acpStrGetLength(&aParseContext->mProductAbbr) - 2,
                             acpStrGetBuffer(&aParseContext->mProductAbbr) + 1,
                             aMsg[i]->mErrID);
            (void)acpFprintf(aFile, " %S\n", sMsg);
        }
    }

    (void)acpFprintf(aFile, GENMSG_MSG_FORMAT_TAIL);
}

static void genmsgGenerateManualForLanguage(
    genmsgParseContext *aParseContext,
    acp_sint32_t        aLang)
{
    acp_std_file_t sFile;

    ACP_STR_DECLARE_DYNAMIC(sFileName);
    ACP_STR_INIT_DYNAMIC(sFileName, 0, 32);

    /*
     * file open
     */
    genmsgFileDir(&sFileName, aParseContext);
    GENMSG_EXIT_IF_FAIL(acpStrCatFormat(&sFileName,
                                        "msg%S%d.manual",
                                        &aParseContext->mLang[aLang],
                                        aParseContext->mProductID));

    if (acpStdOpen(&sFile, acpStrGetBuffer(&sFileName), "w+") != ACP_RC_SUCCESS)
    {
        (void)acpPrintf("fopen failed (%S).\n", &sFileName);
        acpProcExit(1);
    }
    else
    {
        if ((aParseContext->mFlag & GENMSG_FLAG_VERBOSE) != 0)
        {
            (void)acpPrintf("writing file: %S\n", &sFileName);
        }
        else
        {
            /* do nothing */
        }
    }

    ACP_STR_FINAL(sFileName);

    /*
     * file header
     */
    (void)acpFprintf(&sFile,
                     GENMSG_FILE_HEADER,
                     "Id",
                     &aParseContext->mFilePath);


    /*
     * error message format array
     */
    genmsgGenerateMsgFormatArray(&sFile,
                                 aParseContext,
                                 "Err",
                                 aParseContext->mLangCount,
                                 aLang,
                                 aParseContext->mErrMsgCount,
                                 aParseContext->mErrMsg);


    /*
     * file tail
     */
    (void)acpFprintf(&sFile, GENMSG_SRC_TAIL);

    /*
     * file close
     */
    (void)acpStdClose(&sFile);
}

void genmsgGenerateManual(genmsgParseContext *aParseContext)
{
    acp_sint32_t i;

    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        genmsgGenerateManualForLanguage(aParseContext, i);
    }
}
