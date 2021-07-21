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
 * $Id: genmsgCode.c 10912 2010-04-23 01:44:15Z djin $
 ******************************************************************************/

#include <acp.h>
#include <genmsg.h>

#if defined(ALTI_CFG_OS_WINDOWS)
#define ALTI_CFG_OS_DIR_DELIMETER '\\'
#else
#define ALTI_CFG_OS_DIR_DELIMETER '/'
#endif

#define GENMSG_FILE_HEADER                                                     \
    "/***************************************"                                 \
    "****************************************\n"                               \
    " * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.\n"      \
    " * All rights reserved.\n"                                                \
    " ***************************************"                                 \
    "***************************************/\n"                               \
    "\n"                                                                       \
    "/***************************************"                                 \
    "****************************************\n"                               \
    " * $%s$\n"                                                                \
    " * Generated from %S by genmsg.\n"                                        \
    " ***************************************"                                 \
    "***************************************/\n"                               \
    "\n"

#define GENMSG_HDR_HEADER                       \
    "#if !defined(_O_%1$S_H_)\n"                      \
    "#define _O_%1$S_H_\n"                      \
    "\n"                                        \
    "#include <acpStr.h>\n"                     \
    "/* ALINT:S9-SKIP */\n"                     \
    "\n"

#define GENMSG_HDR_TAIL                                         \
    "\n"                                                        \
    "\n"                                                        \
    "ACP_EXPORT acp_rc_t %1$sLoad(\n"                           \
    "    acp_str_t *aDirPath,\n"                                \
    "    acp_str_t *aCustomCodePrefix,\n"                       \
    "    acp_str_t *aLangCode,\n"                               \
    "    acp_str_t *aCustomCodeSuffix);\n"                      \
    "\n"                                                        \
    "ACP_EXPORT void     %1$sUnload(void);\n"                   \
    "\n"                                                        \
    "\n"                                                        \
    "#endif\n"

#define GENMSG_SRC_HEADER                       \
    "#include <aceMsgTable.h>\n"

#define GENMSG_SRC_TAIL                         \
    ""

#define GENMSG_ERR_MACRO                        \
    "#define %-60S \\\n            ACP_UINT64_LITERAL(%#018llx)\n\n"

#define GENMSG_ERR_MACRO_FOR_ALINT              \
    "#define %-70S \\\n"

#define GENMSG_MSG_TABLE_ENTRY_FORMAT                                   \
    "\n"                                                                \
    "\n"                                                                \
    "static ace_msg_desc_t gMsgDesc =\n"                                \
    "{\n"                                                               \
    "    %1$d,\n"                                                       \
    "    %2$d,\n"                                                       \
    "    %3$d,\n"                                                       \
    "    ACP_STR_CONST(%4$S),\n"                                        \
    "    ACP_STR_CONST(%5$S)\n"                                         \
    "};\n"                                                              \
    "\n"                                                                \
    "\n"                                                                \
    "static ace_msg_table_t gMsgTable;\n"                               \
    "\n"                                                                \
    "\n"                                                                \
    "ACP_EXPORT acp_rc_t %6$sLoad(\n"                                   \
    "    acp_str_t *aDirPath,\n"                                        \
    "    acp_str_t *aCustomCodePrefix,\n"                               \
    "    acp_str_t *aLangCode,\n"                                       \
    "    acp_str_t *aCustomCodeSuffix)\n"                               \
    "{\n"                                                               \
    "    gMsgTable.mDesc = gMsgDesc;\n"                                 \
    "\n"                                                                \
    "    return aceMsgTableLoad(&gMsgTable,\n"                          \
    "                           aDirPath,\n"                            \
    "                           aCustomCodePrefix,\n"                   \
    "                           aLangCode,\n"                           \
    "                           aCustomCodeSuffix);"                    \
    "}\n"                                                               \
    "\n"                                                                \
    "ACP_EXPORT void %6$sUnload(void)\n"                                \
    "{\n"                                                               \
    "    aceMsgTableUnload(&gMsgTable);\n"                              \
    "}\n"

#define GENMSG_MAKERULE_FORMAT                  \
    "$(eval $(call msglib_template,msg%S%d))\n"

#define GENMSG_MSG_DESC_FORMAT                  \
    "\n"                                        \
    "\n"                                        \
    "/* ALINT:S9-SKIP */\n"                     \
    "static ace_msg_desc_t gMsgDesc =\n"        \
    "{\n"                                       \
    "    %d,\n"                                 \
    "    %d,\n"                                 \
    "    %d,\n"                                 \
    "    ACP_STR_CONST(%S),\n"                  \
    "    ACP_STR_CONST(%S)\n"                   \
    "};\n"

#define GENMSG_MSG_FORMAT_HEADER                        \
    "\n"                                                \
    "static const acp_char_t *gMsg%sFormat[] =\n"       \
    "{\n"
#define GENMSG_MSG_FORMAT_TAIL                  \
    "};\n"

#define GENMSG_GET_MSG_FUNC_FORMAT                                      \
    "\n"                                                                \
    "\n"                                                                \
    "ACP_EXPORT ace_msg_desc_t *msgGetDesc%1$S%2$d(void)\n"             \
    "{\n"                                                               \
    "    return &gMsgDesc;\n"                                           \
    "}\n"                                                               \
    "\n"                                                                \
    "ACP_EXPORT const acp_char_t **msgGetErrMsgs%1$S%2$d(void)\n"       \
    "{\n"                                                               \
    "    return gMsgErrFormat;\n"                                       \
    "}\n"                                                               \
    "\n"                                                                \
    "ACP_EXPORT const acp_char_t **msgGetLogMsgs%1$S%2$d(void)\n"       \
    "{\n"                                                               \
    "    return gMsgLogFormat;\n"                                       \
    "}\n"                                                               


ACP_INLINE void genmsgFileName(acp_str_t          *aFilePath,
                               genmsgParseContext *aParseContext)
{
    acp_sint32_t sIndexSlash = ACP_STR_INDEX_INITIALIZER;
    acp_sint32_t sIndexDot   = ACP_STR_INDEX_INITIALIZER;
    acp_size_t   sPathLength;
    acp_rc_t     sRC;
    acp_char_t   *sFileName = NULL;

    sRC = acpStrFindChar(&aParseContext->mFilePath,
                         ALTI_CFG_OS_DIR_DELIMETER,
                         &sIndexSlash,
                         sIndexSlash,
                         ACP_STR_SEARCH_BACKWARD);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        sIndexSlash = 0;
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
        sIndexDot = acpStrGetLength(&aParseContext->mFilePath);
    }
    else
    {
        if (sIndexDot > sIndexSlash)
        {
            sFileName = acpStrGetBuffer(&aParseContext->mFilePath);

            if(sIndexSlash == 0)
            {
                /* No delimeter in the path */
                sPathLength = sIndexDot - sIndexSlash;
            }
            else
            {
                sFileName += sIndexSlash + 1;
                sPathLength = sIndexDot - sIndexSlash - 1;
            }
            GENMSG_EXIT_IF_FAIL(
                acpStrCpyBuffer(aFilePath,
                                sFileName,
                                sPathLength));
        }
        else
        {
            GENMSG_EXIT_IF_FAIL(
                acpStrCpy(aFilePath, &aParseContext->mFilePath)
                );
        }
    }
}

ACP_INLINE void genmsgFileBase(acp_str_t          *aFilePath,
                               genmsgParseContext *aParseContext)
{
    acp_sint32_t sIndexSlash = ACP_STR_INDEX_INITIALIZER;
    acp_sint32_t sIndexDot   = ACP_STR_INDEX_INITIALIZER;
    acp_rc_t     sRC;

    sRC = acpStrFindChar(&aParseContext->mFilePath,
                         ALTI_CFG_OS_DIR_DELIMETER,
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
        GENMSG_EXIT_IF_FAIL(acpStrCpy(aFilePath, &aParseContext->mFilePath));
    }
    else
    {
        if (sIndexDot > sIndexSlash)
        {
            GENMSG_EXIT_IF_FAIL(
                acpStrCpyBuffer(aFilePath,
                                acpStrGetBuffer(&aParseContext->mFilePath),
                                sIndexDot));
        }
        else
        {
            GENMSG_EXIT_IF_FAIL(
                acpStrCpy(aFilePath, &aParseContext->mFilePath)
                );
        }
    }
}

ACP_INLINE void genmsgFileDir(acp_str_t          *aFilePath,
                              genmsgParseContext *aParseContext)
{
    acp_sint32_t sIndex = ACP_STR_INDEX_INITIALIZER;
    acp_rc_t     sRC;

    sRC = acpStrFindChar(&aParseContext->mFilePath,
                         ALTI_CFG_OS_DIR_DELIMETER,
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

static void genmsgGenerateHeader(genmsgParseContext *aParseContext)
{
    acp_rc_t sRC;
    acp_path_pool_t sPool;
    acp_std_file_t  sFile;
    acp_char_t*     sPath = NULL;
    acp_uint32_t    sProductCode;
    acp_sint32_t    i;
    acp_uint64_t    j;

    ACP_STR_DECLARE_DYNAMIC(sFileBase);
    ACP_STR_DECLARE_DYNAMIC(sFileName);
    ACP_STR_DECLARE_DYNAMIC(sDefName);

    ACP_STR_INIT_DYNAMIC(sFileBase, 0, 32);
    ACP_STR_INIT_DYNAMIC(sFileName, 0, 32);
    ACP_STR_INIT_DYNAMIC(sDefName, 0, 32);

    sProductCode = (acp_uint32_t)aParseContext->mProductID << 20;

    genmsgFileBase(&sFileBase, aParseContext);
    genmsgFileName(&sDefName, aParseContext);
    GENMSG_EXIT_IF_FAIL(acpStrCpyFormat(&sFileName, "%S.h", &sFileBase));
/*    GENMSG_EXIT_IF_FAIL(acpStrCpy(&sDefName, &sFileBase)); */

    /*
     * file open
     */
    

    if ((sRC = acpStdOpen(&sFile, acpStrGetBuffer(&sFileName), "w+")) != ACP_RC_SUCCESS)
    {
        (void)acpPrintf("fopen failed (%S)(%d).\n", &sFileName, sRC);
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

    /*
     * file header
     */
    (void)acpStrUpper(&sDefName);
    (void)acpFprintf(&sFile,
                     GENMSG_FILE_HEADER,
                     "Id",
                     &aParseContext->mFilePath);
    (void)acpFprintf(&sFile, GENMSG_HDR_HEADER, &sDefName);


    (void)acpFprintf(&sFile, "\n/*\n * product id\n */\n");
    (void)acpFprintf(&sFile,
                     "#define ACE_PRODUCT_%.*s %d\n\n",
                     (acp_sint32_t)
                     acpStrGetLength(&aParseContext->mProductAbbr) - 2,
                     acpStrGetBuffer(&aParseContext->mProductAbbr) + 1,
                     aParseContext->mProductID);

    /*
     * error code macros
     */
    if (aParseContext->mErrMsgCount > 0)
    {
        (void)acpFprintf(&sFile, "\n/*\n * error codes\n */\n");
    }
    else
    {
        /* do nothing */
    }

    for (i = 0, j = 1;
         i < aParseContext->mErrMsgCount;
         i++)
    {
        if (aParseContext->mErrMsg[i] != NULL)
        {
            (void)acpFprintf(&sFile, GENMSG_ERR_MACRO,
                             &aParseContext->mErrMsg[i]->mKey,/* macro string*/

                             /* error code */
                             (acp_uint64_t)(sProductCode) |
                             (acp_uint64_t)(aParseContext->mErrMsg[i]->mErrID) |
                             (j << ACE_INDEX_OFFSET)
                             );
            j++;
            
        }
        else
        {
            /* do nothing */
        }
    }

    /*
     * message code macros
     */
    if (aParseContext->mLogMsgCount > 0)
    {
        (void)acpFprintf(&sFile, "\n/*\n * message codes\n */\n");
    }
    else
    {
        /* do nothing */
    }

    for (i = 0, j = 1;
         i < aParseContext->mLogMsgCount; i++)
    {
        if (aParseContext->mLogMsg[i] != NULL)
        {
            (void)acpFprintf(&sFile, GENMSG_ERR_MACRO,
                             &aParseContext->mLogMsg[i]->mKey,
                             
                             /* error code */
                             (acp_uint64_t)(sProductCode + i) |
                             (j << ACE_INDEX_OFFSET));
            j++;
        }
        else
        {
            /* do nothing */
        }
    }


    /*
     * file tail
     */
    acpPathPoolInit(&sPool);
    sPath = acpPathLast(acpStrGetBuffer(&sFileBase), &sPool);

    if(NULL == sPath)
    {
        GENMSG_EXIT_IF_FAIL(ACP_RC_GET_OS_ERROR());
    }
    else
    {
        (void)acpFprintf(&sFile, GENMSG_HDR_TAIL, sPath);
    }

    acpPathPoolFinal(&sPool);

    /*
     * file close
     */
    ACP_STR_FINAL(sDefName);
    ACP_STR_FINAL(sFileBase);
    ACP_STR_FINAL(sFileName);
    (void)acpStdClose(&sFile);
}

static void genmsgGenerateStub(genmsgParseContext *aParseContext)
{
    acp_rc_t sRC;
    acp_path_pool_t sPool;
    acp_std_file_t  sFile;
    acp_char_t*     sPath = NULL;

    ACP_STR_DECLARE_DYNAMIC(sFileBase);
    ACP_STR_DECLARE_DYNAMIC(sFileName);

    ACP_STR_INIT_DYNAMIC(sFileBase, 0, 32);
    ACP_STR_INIT_DYNAMIC(sFileName, 0, 32);

    /*
     * file open
     */
    genmsgFileBase(&sFileBase, aParseContext);
    GENMSG_EXIT_IF_FAIL(acpStrCpyFormat(&sFileName, "%S.c", &sFileBase));

    if ((sRC = acpStdOpen(&sFile, acpStrGetBuffer(&sFileName), "w+")) != ACP_RC_SUCCESS)
    {
        (void)acpPrintf("fopen failed (%S)(%d).\n", &sFileName, sRC);
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

    /*
     * file header
     */
    (void)acpFprintf(&sFile,
                     GENMSG_FILE_HEADER,
                     "Id",
                     &aParseContext->mFilePath);
    (void)acpFprintf(&sFile, GENMSG_SRC_HEADER);

    /*
     * message table
     */
    acpPathPoolInit(&sPool);
    sPath = acpPathLast(acpStrGetBuffer(&sFileBase), &sPool);

    if(NULL == sPath)
    {
        GENMSG_EXIT_IF_FAIL(ACP_RC_GET_OS_ERROR());
    }
    else
    {
        (void)acpFprintf(&sFile,
                         GENMSG_MSG_TABLE_ENTRY_FORMAT,
                         aParseContext->mProductID,
                         aParseContext->mErrMsgCount,
                         aParseContext->mLogMsgCount,
                         &aParseContext->mProductName,
                         &aParseContext->mProductAbbr,
                         sPath);
    }

    acpPathPoolFinal(&sPool);

    /*
     * file tail
     */
    (void)acpFprintf(&sFile, GENMSG_SRC_TAIL);

    /*
     * file close
     */
    ACP_STR_FINAL(sFileBase);
    ACP_STR_FINAL(sFileName);
    (void)acpStdClose(&sFile);
}

static void genmsgGenerateMakefile(genmsgParseContext *aParseContext)
{
    acp_std_file_t  sFile;
    acp_sint32_t    i;
    acp_rc_t        sRC;

    ACP_STR_DECLARE_DYNAMIC(sFileBase);
    ACP_STR_DECLARE_DYNAMIC(sFileName);

    ACP_STR_INIT_DYNAMIC(sFileBase, 0, 32);
    ACP_STR_INIT_DYNAMIC(sFileName, 0, 32);

    /*
     * file open
     */
    genmsgFileBase(&sFileBase, aParseContext);
    GENMSG_EXIT_IF_FAIL(acpStrCpyFormat(&sFileName, "%S.mk", &sFileBase));

    if ((sRC = acpStdOpen(&sFile, acpStrGetBuffer(&sFileName), "w+")) != ACP_RC_SUCCESS)
    {
        (void)acpPrintf("fopen failed (%S)(%d).\n", &sFileName, sRC);
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

    /*
     * message library rule
     */
    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        (void)acpFprintf(&sFile,
                         GENMSG_MAKERULE_FORMAT,
                         &aParseContext->mLang[i],
                         aParseContext->mProductID);
    }

    /*
     * file close
     */
    ACP_STR_FINAL(sFileBase);
    ACP_STR_FINAL(sFileName);
    (void)acpStdClose(&sFile);
}

static void genmsgGenerateMsgFormatArray(acp_std_file_t    *aFile,
                                         const acp_char_t  *aType,
                                         acp_sint32_t       aLangCount,
                                         acp_sint32_t       aLang,
                                         acp_sint32_t       aMsgCount,
                                         genmsgMessage    **aMsg)
{
    acp_str_t    *sMsg = NULL;
    acp_sint32_t  i;
    acp_sint32_t  j;

    (void)acpFprintf(aFile, GENMSG_MSG_FORMAT_HEADER, aType);

    if (aMsgCount == 0)
    {
        (void)acpFprintf(aFile, "    NULL\n");
    }
    else
    {
        /* do nothing */
    }

    /* 0th array */
    (void)acpFprintf(aFile, "    NULL,\n"); 
    for (i = 0; i < aMsgCount; i++)
    {
        if (aMsg[i] == NULL)
        {
            /* do nothing */
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

            (void)acpFprintf(aFile, "    %S,\n", sMsg);
        }
    }

    (void)acpFprintf(aFile, GENMSG_MSG_FORMAT_TAIL);
}

static void genmsgGenerateCodeForLanguage(genmsgParseContext *aParseContext,
                                          acp_sint32_t        aLang)
{
    acp_std_file_t  sFile;
    acp_rc_t        sRC;

    ACP_STR_DECLARE_DYNAMIC(sFileName);
    ACP_STR_INIT_DYNAMIC(sFileName, 0, 32);

    /*
     * file open
     */
    genmsgFileDir(&sFileName, aParseContext);
    GENMSG_EXIT_IF_FAIL(acpStrCatFormat(&sFileName,
                                        "msg%S%d.c",
                                        &aParseContext->mLang[aLang],
                                        aParseContext->mProductID));

    if ((sRC = acpStdOpen(&sFile, acpStrGetBuffer(&sFileName), "w+")) != ACP_RC_SUCCESS)
    {
        (void)acpPrintf("fopen failed (%S)(%d).\n", &sFileName, sRC);
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
    (void)acpFprintf(&sFile, GENMSG_SRC_HEADER);

    /*
     * message table description
     */
    (void)acpFprintf(&sFile,
                     GENMSG_MSG_DESC_FORMAT,
                     aParseContext->mProductID,
                     aParseContext->mErrMsgCount,
                     aParseContext->mLogMsgCount,
                     &aParseContext->mProductName,
                     &aParseContext->mProductAbbr);

    /*
     * error message format array
     */
    genmsgGenerateMsgFormatArray(&sFile,
                                 "Err",
                                 aParseContext->mLangCount,
                                 aLang,
                                 aParseContext->mErrMsgCount,
                                 aParseContext->mErrMsg);

    /*
     * error message format array
     */
    genmsgGenerateMsgFormatArray(&sFile,
                                 "Log",
                                 aParseContext->mLangCount,
                                 aLang,
                                 aParseContext->mLogMsgCount,
                                 aParseContext->mLogMsg);

    /*
     * get functions
     */
    (void)acpFprintf(&sFile,
                     GENMSG_GET_MSG_FUNC_FORMAT,
                     &aParseContext->mLang[aLang],
                     aParseContext->mProductID);

    /*
     * file tail
     */
    (void)acpFprintf(&sFile, GENMSG_SRC_TAIL);

    /*
     * file close
     */
    (void)acpStdClose(&sFile);
}

void genmsgGenerateCode(genmsgParseContext *aParseContext)
{
    acp_sint32_t i;

    genmsgGenerateHeader(aParseContext);
    genmsgGenerateStub(aParseContext);
    genmsgGenerateMakefile(aParseContext);

    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        genmsgGenerateCodeForLanguage(aParseContext, i);
    }
}
