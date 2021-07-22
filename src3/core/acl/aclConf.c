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
 * $Id: aclConf.c 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

/**
 * @example sampleAclConf.c
 */

#include <acpFile.h>
#include <aceAssert.h>
#include <aclConf.h>
#include <aclConfPrivate.h>


static const acp_char_t *gAclConfTypeName[ACL_CONF_TYPE_MAX] =
{
    "NONE",
    "BOOLEAN",
    "SIGNED INTEGER",
    "UNSIGNED INTEGER",
    "SIGNED LONG INTEGER",
    "UNSIGNED LONG INTEGER",
    "STRING",
    "CONTAINER",
    "UNNAMED CONTAINER"
};


ACP_INLINE void aclConfPrintKey(aclConfContext *aContext,
                                acp_str_t      *aStr,
                                acp_sint32_t    aCountAdjust)
{
    acp_sint32_t i;

    ACP_STR_DECLARE_STATIC(sKey, 64);
    ACP_STR_INIT_STATIC(sKey);

    for (i = 0; i < aContext->mDepth + aCountAdjust; i++)
    {
        (void)acpStrCatFormat(&sKey, "%s%S",
                              ((i == 0) ? "" : "."),
                              &aContext->mKey[i].mKey);
    }

    (void)acpStrUpper(&sKey);
    (void)acpStrCatFormat(aStr, "%S", &sKey);
}

void aclConfError(aclConfContext   *aContext,
                  aclConfToken     *aToken,
                  aclConfErrorType  aError)
{
    acl_conf_def_t *sDef = NULL;
    acp_sint32_t    sLine;

    ACP_STR_DECLARE_STATIC(sError, 256);
    ACP_STR_INIT_STATIC(sError);

    switch (aError)
    {
        case ACL_CONF_ERR_UNKNOWN_KEY:
        case ACL_CONF_ERR_DUPLICATED_KEY:
        case ACL_CONF_ERR_UNKNOWN_TYPE:
        case ACL_CONF_ERR_TYPE_MISMATCH:
        case ACL_CONF_ERR_TOO_MANY_VALUE:
        case ACL_CONF_ERR_VALUE_OVERFLOW:
            break;
        default:
            aContext->mRC = ACP_RC_EINVAL;
            break;
    }

    if (aContext->mErrorCallback != NULL)
    {
        sLine = (aToken != NULL) ? aToken->mLine : aContext->mLine;

        (void)acpStrCpyFormat(&sError,
                              "%S:%d: ",
                              aContext->mFilePath,
                              sLine);

        switch (aError)
        {
            case ACL_CONF_ERR_SYNTAX_ERROR:
                (void)acpStrCatFormat(&sError, "syntax error");
                break;
            case ACL_CONF_ERR_INVALID_TOKEN:
                (void)acpStrCatFormat(&sError, "invalid token");
                break;
            case ACL_CONF_ERR_UNEXPECTED_TOKEN:
                (void)acpStrCatFormat(&sError, "unexpected token");
                break;
            case ACL_CONF_ERR_PARSE_FAILURE:
                (void)acpStrCatFormat(&sError, "parse failure");
                break;
            case ACL_CONF_ERR_UNKNOWN_KEY:
                aclConfPrintKey(aContext, &sError, 1);
                (void)acpStrCatFormat(&sError, " unknown");
                break;
            case ACL_CONF_ERR_DUPLICATED_KEY:
                sDef = aContext->mCurrentDef[aContext->mDepth];
                aclConfPrintKey(aContext, &sError, 1);
                (void)acpStrCatFormat(&sError,
                                      " duplicated (line %d)",
                                      sDef->mLineFound);
                break;
            case ACL_CONF_ERR_UNKNOWN_TYPE:
                aclConfPrintKey(aContext, &sError, 0);
                (void)acpStrCatFormat(&sError, " unknown type");
                break;
            case ACL_CONF_ERR_TYPE_MISMATCH:
                sDef = aContext->mCurrentDef[aContext->mDepth - 1];
                aclConfPrintKey(aContext, &sError, 0);
                (void)acpStrCatFormat(&sError,
                                      " type mismatch (%s)",
                                      gAclConfTypeName[sDef->mType]);
                break;
            case ACL_CONF_ERR_TOO_MANY_VALUE:
                aclConfPrintKey(aContext, &sError, 0);
                (void)acpStrCatFormat(&sError, " too many value");
                break;
            case ACL_CONF_ERR_VALUE_OVERFLOW:
                sDef = aContext->mCurrentDef[aContext->mDepth - 1];
                aclConfPrintKey(aContext, &sError, 0);
                (void)acpStrCatFormat(&sError, " value overflow");
                break;
            case ACL_CONF_ERR_DEPTH_OVERFLOW:
                (void)acpStrCatFormat(&sError, "depth overflow");
                break;
            default:
                (void)acpStrCatFormat(&sError, "error");
                break;
        }

        aContext->mCallbackResult = (*aContext->mErrorCallback)
            (aContext->mFilePath,
             sLine,
             &sError,
             aContext->mUserContext);

        if (aContext->mCallbackResult != 0)
        {
            aContext->mRC = ACP_RC_ECANCELED;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE acp_rc_t aclConfAllocBufferAndReadFile(acp_str_t      *aPath,
                                                  aclConfContext *aContext)
{
    acp_file_t    sFile;
    acp_offset_t  sFileSize = 0;
    acp_size_t    sReadSize = 0;
    acp_rc_t      sRC;

    /*
     * aPath must not be null
     */
    if (aPath == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    /*
     * open the file
     */
    sRC = acpFileOpen(&sFile, acpStrGetBuffer(aPath), ACP_O_RDONLY, 0);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * get size of the file
     */
    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_END, &sFileSize);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpFileClose(&sFile);
        return sRC;
    }
    else
    {
        if (sFileSize > (acp_offset_t)ACP_SINT32_MAX)
        {
            (void)acpFileClose(&sFile);
            return ACP_RC_ENOMEM;
        }
        else
        {
            if (sFileSize == 0) /* file has no data */
            {
                (void)acpFileClose(&sFile);
                return ACP_RC_EOF;
            }
            else
            {
                /* do nothing */
            }
        }
    }

    sRC = acpFileSeek(&sFile, 0, ACP_SEEK_SET, NULL);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpFileClose(&sFile);
        return sRC;
    }
    else
    {
        
        /* do nothing */
    }

    /*
     * allocate buffer
     */
    aContext->mBufferSize = (acp_size_t)sFileSize;

    sRC = acpMemAlloc((void **)&aContext->mBuffer, aContext->mBufferSize + 1);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpFileClose(&sFile);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * read the file
     */
    sRC = acpFileRead(&sFile,
                      aContext->mBuffer,
                      (acp_size_t)sFileSize,
                      &sReadSize);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)acpFileClose(&sFile);
        return sRC;
    }
    else
    {
        if (sReadSize != (acp_size_t)sFileSize)
        {
            (void)acpFileClose(&sFile);
            return ACP_RC_EINTR;
        }
        else
        {
            /* do nothing */
        }
    }

    /*
     * make sure that the buffer ends with newline
     */
    if (aContext->mBuffer[sReadSize - 1] != '\n')
    {
        aContext->mBuffer[sReadSize] = '\n';
        aContext->mBufferSize++;
    }
    else
    {
        /* do nothing */
    }

    /*
     * close the file
     */
    (void)acpFileClose(&sFile);

    return ACP_RC_SUCCESS;
}

ACP_INLINE void aclConfFreeBuffer(aclConfContext *aContext)
{
    acpMemFree(aContext->mBuffer);
}


/**
 * loads a configuration file
 * @param aPath path of the configuration to load
 * @param aDef array of configuration key definitions
 * @param aDefaultCallback default callback function to assign key/value
 * @param aErrorCallback callback function for report error
 * @param aCallbackResult pointer to the variable
 * to store the return value of callback function
 * @param aContext opaque data passed to the callback function
 * @return result code
 *
 * if the result code is not #ACP_RC_SUCCESS,
 * the configuration file has unrecoverable error.
 */
ACP_EXPORT acp_rc_t aclConfLoad(acp_str_t               *aPath,
                                acl_conf_def_t          *aDef,
                                acl_conf_set_callback_t *aDefaultCallback,
                                acl_conf_err_callback_t *aErrorCallback,
                                acp_sint32_t            *aCallbackResult,
                                void                    *aContext)
{
    void           *sParser = NULL;
    aclConfContext  sContext;
    aclConfToken    sToken;
    acp_sint32_t    i;

    /*
     * aDef must not be null
     */
    if (aDef == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    /*
     * alloc buffer and read the configuration file
     */
    sContext.mRC = aclConfAllocBufferAndReadFile(aPath, &sContext);

    if (ACP_RC_NOT_SUCCESS(sContext.mRC))
    {
        return sContext.mRC;
    }
    else
    {
        /* do nothing */
    }

    /*
     * setup parse context
     */
    sContext.mFilePath        = aPath;
    sContext.mLine            = 1;

    sContext.mCursor          = sContext.mBuffer;
    sContext.mLimit           = sContext.mBuffer + sContext.mBufferSize - 1;

    sContext.mDefaultCallback = aDefaultCallback;
    sContext.mErrorCallback   = aErrorCallback;
    sContext.mUserContext     = aContext;

    sContext.mCallbackResult  = 0;

    sContext.mBaseDef[0]      = aDef;
    sContext.mCurrentDef[0]   = NULL;

    sContext.mDepth           = 0;
    sContext.mIgnoreDepth     = 0;

    for (i = 0; i < ACL_CONF_DEPTH_MAX; i++)
    {
        ACP_STR_INIT_CONST(sContext.mKey[i].mKey);
    }

    /*
     * init conf def
     */
    aclConfInitDef(aDef);

    /*
     * alloc parser
     */
    sParser = aclConfParseAlloc();

    ACP_TEST(NULL == sParser);

    /*
     * parse the configuration file
     */
    do
    {
        sContext.mTokenID = aclConfGetToken(&sContext);

        sToken.mPtr  = sContext.mToken;
        sToken.mLen  = (acp_sint32_t)(sContext.mCursor - sContext.mToken);
        sToken.mLine = sContext.mLine;

        if (sContext.mTokenID == ACL_CONF_TOKEN_INVALID)
        {
            aclConfError(&sContext, &sToken, ACL_CONF_ERR_INVALID_TOKEN);
        }
        else
        {
            aclConfParse(sParser, sContext.mTokenID, sToken, &sContext);
        }
    } while ((sContext.mTokenID != ACL_CONF_TOKEN_EOF) &&
             ACP_RC_IS_SUCCESS(sContext.mRC));

    /*
     * finalize conf def; set default value for unspecified keys
     */
    aclConfFinalDef(&sContext, aDef);

    /*
     * free parser
     */
    aclConfParseFree(sParser);

    /*
     * free buffer
     */
    aclConfFreeBuffer(&sContext);

    /*
     * return result code
     */
    if (aCallbackResult != NULL)
    {
        *aCallbackResult = sContext.mCallbackResult;
    }
    else
    {
        /* do nothing */
    }

    return sContext.mRC;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();
}
