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
 * $Id: genmsg.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_GENMSG_H_)
#define _O_GENMSG_H_

#include <acp.h>
#include <aceError.h>


ACP_EXTERN_C_BEGIN


/*
 * constants
 */
#define GENMSG_ARRAY_EXTEND_SIZE 32


/*
 * error handling
 */
#define GENMSG_EXIT_IF_FAIL(aExpr)                              \
    do                                                          \
    {                                                           \
        acp_rc_t sExprRC_MACRO_LOCAL_VAR = (aExpr);             \
                                                                \
        if (ACP_RC_NOT_SUCCESS(sExprRC_MACRO_LOCAL_VAR))        \
        {                                                       \
            genmsgPrintErrorAndExit(sExprRC_MACRO_LOCAL_VAR);   \
        }                                                       \
        else                                                    \
        {                                                       \
        }                                                       \
    } while (0)

ACP_INLINE void genmsgPrintErrorAndExit(acp_rc_t aRC)
{
    acp_char_t sBuffer[1024];

    acpErrorString(aRC, sBuffer, sizeof(sBuffer));

    (void)acpPrintf("error: %s\n", sBuffer);

    acpProcExit(1);
}


/*
 * special token id
 */
#define GENMSG_TOKEN_INVALID  -1
#define GENMSG_TOKEN_EOF       0


/*
 * program option flags
 */
#define GENMSG_FLAG_WARNING 1
#define GENMSG_FLAG_VERBOSE 2


/*
 * parser token; data type for terminal symbols
 */
typedef struct genmsgToken
{
    const acp_char_t *mPtr;
    acp_sint32_t      mLen;
    acp_sint32_t      mLineNo;
    acp_sint32_t      mColNo;
} genmsgToken;

/*
 * data type for non-terminal symbols
 */
typedef enum genmsgMessageFieldType
{
    GENMSG_MESSAGE_FIELD_ERRID = 1,
    GENMSG_MESSAGE_FIELD_KEY,
    GENMSG_MESSAGE_FIELD_MSG
} genmsgMessageFieldType;

typedef struct genmsgMessageField
{
    genmsgToken            mToken;
    genmsgMessageFieldType mType;

    union
    {
        acp_sint32_t mErrID;
        acp_str_t    mKey;
        struct
        {
            acp_sint32_t mLang;
            acp_str_t    mMsg;
        } mMsg;
    } mField;
} genmsgMessageField;

typedef struct genmsgMessage
{
    acp_sint32_t  mErrID;
    acp_str_t     mKey;
    acp_str_t    *mMsg;
} genmsgMessage;

/*
 * parser context
 */
typedef enum genmsgSection
{
    GENMSG_SECTION_FILE,
    GENMSG_SECTION_ERROR,
    GENMSG_SECTION_LOG
} genmsgSection;

typedef struct genmsgParseContext
{
    acp_str_t         mFilePath;

    acp_sint32_t      mFlag;

    acp_char_t       *mBuffer;
    acp_size_t        mBufferSize;
    acp_sint32_t      mLineNo;

    const acp_char_t *mCursor;
    const acp_char_t *mLimit;
    const acp_char_t *mLine;
    const acp_char_t *mToken;
    const acp_char_t *mMarker;

    acp_sint32_t      mProductID;
    acp_str_t         mProductName;
    acp_str_t         mProductAbbr;

    acp_sint32_t      mLangCount;
    acp_sint32_t      mLangSize;
    acp_str_t        *mLang;

    acp_sint32_t      mErrMsgCount;
    acp_sint32_t      mErrMsgSize;
    genmsgMessage   **mErrMsg;

    acp_sint32_t      mLogMsgCount;
    acp_sint32_t      mLogMsgSize;
    genmsgMessage   **mLogMsg;

    genmsgSection     mSection;
} genmsgParseContext;

/*
 * parser help functions
 */
ACP_INLINE void genmsgParseError(genmsgParseContext *aParseContext,
                                 genmsgToken        *aToken,
                                 const acp_char_t   *aMessage)
{
    if (aToken != NULL)
    {
        (void)acpPrintf("%S:%d:%d: %s\n",
                        &aParseContext->mFilePath,
                        aToken->mLineNo,
                        aToken->mColNo,
                        aMessage);
    }
    else
    {
        (void)acpPrintf("%S:%d: %s\n",
                        &aParseContext->mFilePath,
                        aParseContext->mLineNo,
                        aMessage);
    }

    acpProcExit(1);
}

ACP_INLINE void genmsgParseWarning(genmsgParseContext *aParseContext,
                                   genmsgToken        *aToken,
                                   const acp_char_t   *aMessage)
{
    if ((aParseContext->mFlag & GENMSG_FLAG_WARNING) != 0)
    {
        if (aToken != NULL)
        {
            (void)acpPrintf("%S:%d:%d: warning: %s\n",
                            &aParseContext->mFilePath,
                            aToken->mLineNo,
                            aToken->mColNo,
                            aMessage);
        }
        else
        {
            (void)acpPrintf("%S:%d: warning: %s\n",
                            &aParseContext->mFilePath,
                            aParseContext->mLineNo,
                            aMessage);
        }
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void genmsgCopyString(acp_str_t *aTarget, acp_str_t *aSource)
{
    acpMemCpy(aTarget, aSource, sizeof(acp_str_t));
}

ACP_INLINE acp_sint32_t genmsgTokenToInt(genmsgParseContext *aParseContext,
                                         genmsgToken        *aToken)
{
    acp_uint64_t sValue;
    acp_sint32_t sSign;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    acpStrSetConstCStringWithLen(&sStr, aToken->mPtr, aToken->mLen);

    GENMSG_EXIT_IF_FAIL(acpStrToInteger(&sStr, &sSign, &sValue, NULL, 0, 0));

    if (sValue > ACP_SINT32_MAX)
    {
        genmsgParseError(aParseContext, aToken, "numeric value overflow.");
    }
    else
    {
        /* do nothing */
    }

    return (acp_sint32_t)sValue * sSign;
}

ACP_INLINE void genmsgExtendArray(void         *aArray,
                                  acp_sint32_t *aCurrentSize,
                                  acp_sint32_t  aRequestSize,
                                  acp_size_t    aItemSize)
{
    acp_char_t **sPtr = aArray;

    if (*aCurrentSize < aRequestSize)
    {
        aRequestSize = ACP_ALIGN_ALL(aRequestSize, GENMSG_ARRAY_EXTEND_SIZE);

        GENMSG_EXIT_IF_FAIL(acpMemRealloc((void **)sPtr,
                                          aItemSize * aRequestSize));

        acpMemSet(*sPtr + *aCurrentSize * aItemSize,
                  0,
                  aItemSize * aRequestSize - aItemSize * *aCurrentSize);

        *aCurrentSize = aRequestSize;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void genmsgAddLanguage(genmsgParseContext *aParseContext,
                                  genmsgToken        *aToken)
{
    acp_sint32_t i;

    ACP_STR_DECLARE_CONST(sLang);
    ACP_STR_INIT_CONST(sLang);

    for (i = 0; i < aToken->mLen; i++)
    {
        if (acpCharIsUpper(aToken->mPtr[i]) != ACP_TRUE)
        {
            genmsgParseError(aParseContext,
                             aToken,
                             "invalid language name format.");
        }
        else
        {
            /* continue */
        }
    }

    acpStrSetConstCStringWithLen(&sLang, aToken->mPtr, aToken->mLen);

    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        if (acpStrEqual(&aParseContext->mLang[i], &sLang, 0) == ACP_TRUE)
        {
            genmsgParseWarning(aParseContext,
                               aToken,
                               "duplicated name of language. ignored.");
            return;
        }
        else
        {
            /* continue */
        }
    }

    i = aParseContext->mLangSize;

    genmsgExtendArray(&aParseContext->mLang,
                      &aParseContext->mLangSize,
                      aParseContext->mLangCount + 1,
                      sizeof(acp_str_t));

    for (; i < aParseContext->mLangSize; i++)
    {
        ACP_STR_INIT_CONST(aParseContext->mLang[i]);
    }

    acpStrSetConstCStringWithLen(&aParseContext->mLang
                                 [aParseContext->mLangCount],
                                 aToken->mPtr,
                                 aToken->mLen);

    aParseContext->mLangCount++;
}

ACP_INLINE acp_sint32_t genmsgGetLanguage(genmsgParseContext *aParseContext,
                                          genmsgToken        *aToken)
{
    acp_sint32_t sID;

    ACP_STR_DECLARE_CONST(sLang);
    ACP_STR_INIT_CONST(sLang);

    acpStrSetConstCStringWithLen(&sLang, aToken->mPtr, aToken->mLen);

    for (sID = 0; sID < aParseContext->mLangCount; sID++)
    {
        if (acpStrEqual(&aParseContext->mLang[sID], &sLang, 0) == ACP_TRUE)
        {
            return sID;
        }
        else
        {
            /* contiune */
        }
    }

    return -1;
}

ACP_INLINE genmsgMessage *genmsgAllocMessage(genmsgParseContext *aParseContext)
{
    genmsgMessage *sMessage;
    acp_sint32_t   i;

    GENMSG_EXIT_IF_FAIL(acpMemAlloc((void **)&sMessage, sizeof(genmsgMessage)));
    GENMSG_EXIT_IF_FAIL(acpMemAlloc((void **)&sMessage->mMsg,
                                    sizeof(acp_str_t) *
                                    aParseContext->mLangCount));

    sMessage->mErrID = 0;

    ACP_STR_INIT_CONST(sMessage->mKey);

    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        ACP_STR_INIT_CONST(sMessage->mMsg[i]);
    }

    return sMessage;
}

ACP_INLINE void genmsgFreeMessage(genmsgMessage *aMessage)
{
    acpMemFree(aMessage->mMsg);
    acpMemFree(aMessage);
}

ACP_INLINE void genmsgSetMessageField(genmsgParseContext *aParseContext,
                                      genmsgMessage      *aMessage,
                                      genmsgMessageField *aField)
{
    switch (aField->mType)
    {
        case GENMSG_MESSAGE_FIELD_ERRID:
            if (aMessage->mErrID > 0)
            {
                genmsgParseError(aParseContext,
                                 &aField->mToken,
                                 "ERR_ID is already defined in the message.");
            }
            else
            {
                aMessage->mErrID = aField->mField.mErrID;
            }
            break;
        case GENMSG_MESSAGE_FIELD_KEY:
            if (acpStrGetLength(&aMessage->mKey) > 0)
            {
                genmsgParseError(aParseContext,
                                 &aField->mToken,
                                 "KEY is already defined in the message.");
            }
            else
            {
                genmsgCopyString(&aMessage->mKey, &aField->mField.mKey);
            }
            break;
        case GENMSG_MESSAGE_FIELD_MSG:
            if (acpStrGetLength(&aMessage->mMsg[aField->mField.mMsg.mLang]) > 0)
            {
                genmsgParseError(aParseContext,
                                 &aField->mToken,
                                 "MSG for specified language "
                                 "is already defined in the message.");
            }
            else
            {
                genmsgCopyString(&aMessage->mMsg[aField->mField.mMsg.mLang],
                                 &aField->mField.mMsg.mMsg);
            }
            break;
    }
}

ACP_INLINE void genmsgAddErrorMessage(genmsgParseContext *aParseContext,
                                      genmsgMessage      *aMessage,
                                      genmsgToken        *aToken)
{
    acp_char_t   sWarning[64];
    acp_sint32_t sMsgCount = 0;
    acp_sint32_t i;

    /*
     * check whether ERR_ID is defined
     */
    if (aMessage->mErrID > 0)
    {
        /* do nothing */
    }
    else
    {
        genmsgParseError(aParseContext, aToken, "ERR_ID is not defined.");
    }

    /*
     * check whether KEY is defined
     */
    if (acpStrGetLength(&aMessage->mKey) > 0)
    {
        /* do nothing */
    }
    else
    {
        genmsgParseError(aParseContext, aToken, "KEY is not defined.");
    }

    /*
     * check whether MSG_* is defined
     */
    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        if (acpStrGetLength(&aMessage->mMsg[i]) > 0)
        {
            sMsgCount++;
        }
        else
        {
            (void)acpSnprintf(sWarning,
                              sizeof(sWarning),
                              "MSG_%S is not defined.",
                              &aParseContext->mLang[i]);
            genmsgParseWarning(aParseContext, aToken, sWarning);
        }
    }

    if (sMsgCount == 0)
    {
        genmsgParseError(aParseContext, aToken, "no MSG_* is defined.");
    }
    else
    {
        /* do nothing */
    }

    /*
     * extend message array
     */
    if (aParseContext->mErrMsgCount <= aMessage->mErrID)
    {
        aParseContext->mErrMsgCount = aMessage->mErrID + 1;

        genmsgExtendArray(&aParseContext->mErrMsg,
                          &aParseContext->mErrMsgSize,
                          aParseContext->mErrMsgCount,
                          sizeof(genmsgMessage *));
    }
    else
    {
        /* do nothing */
    }

    /*
     * add message
     */
    aParseContext->mErrMsg[aMessage->mErrID] = aMessage;
}

ACP_INLINE void genmsgAddLogMessage(genmsgParseContext *aParseContext,
                                    genmsgMessage      *aMessage,
                                    genmsgToken        *aToken)
{
    acp_char_t   sWarning[64];
    acp_sint32_t sMsgCount = 0;
    acp_sint32_t i;

    /*
     * check whether KEY is defined
     */
    if (acpStrGetLength(&aMessage->mKey) > 0)
    {
        /* do nothing */
    }
    else
    {
        genmsgParseError(aParseContext, aToken, "KEY is not defined.");
    }

    /*
     * check whether MSG_* is defined
     */
    for (i = 0; i < aParseContext->mLangCount; i++)
    {
        if (acpStrGetLength(&aMessage->mMsg[i]) > 0)
        {
            sMsgCount++;
        }
        else
        {
            (void)acpSnprintf(sWarning,
                              sizeof(sWarning),
                              "MSG_%S is not defined.",
                              &aParseContext->mLang[i]);
            genmsgParseWarning(aParseContext, aToken, sWarning);
        }
    }

    if (sMsgCount == 0)
    {
        genmsgParseError(aParseContext, aToken, "no MSG_* is defined.");
    }
    else
    {
        /* do nothing */
    }

    /*
     * set log id
     */
    if (aParseContext->mLogMsgCount == 0)
    {
        aMessage->mErrID = 1;
    }
    else
    {
        aMessage->mErrID = aParseContext->mLogMsgCount;
    }

    /*
     * extend message array
     */
    genmsgExtendArray(&aParseContext->mLogMsg,
                      &aParseContext->mLogMsgSize,
                      aMessage->mErrID + 1,
                      sizeof(genmsgMessage *));

    /*
     * add message
     */
    aParseContext->mLogMsg[aMessage->mErrID] = aMessage;
    aParseContext->mLogMsgCount              = aMessage->mErrID + 1;
}

ACP_INLINE genmsgMessage *genmsgFindMessage(genmsgParseContext *aParseContext,
                                            acp_str_t          *aKey)
{
    acp_sint32_t i;

    for (i = 0; i < aParseContext->mErrMsgCount; i++)
    {
        if (aParseContext->mErrMsg[i] != NULL)
        {
            if (acpStrEqual(&aParseContext->mErrMsg[i]->mKey,
                            aKey,
                            0) == ACP_TRUE)
            {
                return aParseContext->mErrMsg[i];
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* continue */
        }
    }

    for (i = 0; i < aParseContext->mLogMsgCount; i++)
    {
        if (aParseContext->mLogMsg[i] != NULL)
        {
            if (acpStrEqual(&aParseContext->mLogMsg[i]->mKey,
                            aKey,
                            0) == ACP_TRUE)
            {
                return aParseContext->mLogMsg[i];
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* continue */
        }
    }

    return NULL;
}


/*
 * lexer interface
 */
acp_sint32_t genmsgGetToken(genmsgParseContext *aParseContext);

/*
 * parser interface
 */
void *genmsgParseAlloc(void);
void  genmsgParseFree(void *aParser);
void  genmsgParse(void               *aParser,
                  acp_sint32_t        aTokenID,
                  genmsgToken         aToken,
                  genmsgParseContext *aParseContext);

/*
 * generator interface
 */
void genmsgGenerateCode(genmsgParseContext *aParseContext);
void genmsgGenerateManual(genmsgParseContext *aParseContext);


ACP_EXTERN_C_END


#endif
