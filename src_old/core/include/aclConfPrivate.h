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
 * $Id: aclConfPrivate.h 4823 2009-03-12 03:09:01Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACL_CONF_PRIVATE_H_)
#define _O_ACL_CONF_PRIVATE_H_

#include <acpStr.h>


ACP_EXTERN_C_BEGIN


#define ACL_CONF_DEPTH_MAX 10


#define ACL_CONF_TOKEN_INVALID -1
#define ACL_CONF_TOKEN_EOF      0


typedef enum aclConfErrorType
{
    ACL_CONF_ERR_SYNTAX_ERROR,      /* Syntax Error */
    ACL_CONF_ERR_INVALID_TOKEN,     /* Token used not properly */
    ACL_CONF_ERR_UNEXPECTED_TOKEN,  /* Token must not be here */
    ACL_CONF_ERR_PARSE_FAILURE,     /* Cannot parse */
    ACL_CONF_ERR_UNKNOWN_KEY,       /* Key is invalid */
    ACL_CONF_ERR_DUPLICATED_KEY,    /* Key occured more than once */
    ACL_CONF_ERR_UNKNOWN_TYPE,      /* Cannot determine type */
    ACL_CONF_ERR_TYPE_MISMATCH,     /* Mismatch */
    ACL_CONF_ERR_TOO_MANY_VALUE,    /* Number of Values exceeds */
    ACL_CONF_ERR_VALUE_OVERFLOW,    /* Overflow */
    ACL_CONF_ERR_DEPTH_OVERFLOW     /* Depth too deep */
} aclConfErrorType;


typedef struct aclConfToken
{
    const acp_char_t *mPtr;
    acp_sint32_t      mLen;
    acp_sint32_t      mLine;
    acp_bool_t        mIsQuotedString;
} aclConfToken;

typedef struct aclConfContext
{
    acp_str_t               *mFilePath;

    acp_char_t              *mBuffer;
    acp_size_t               mBufferSize;

    acp_sint32_t             mTokenID;
    acp_sint32_t             mLine;

    const acp_char_t        *mCursor;
    const acp_char_t        *mLimit;
    const acp_char_t        *mToken;
    const acp_char_t        *mCtxMarker;
    const acp_char_t        *mMarker;

    acl_conf_set_callback_t *mDefaultCallback;
    acl_conf_err_callback_t *mErrorCallback;
    void                    *mUserContext;

    acp_rc_t                 mRC;
    acp_sint32_t             mCallbackResult;

    acl_conf_def_t          *mBaseDef[ACL_CONF_DEPTH_MAX];
    acl_conf_def_t          *mCurrentDef[ACL_CONF_DEPTH_MAX];

    acl_conf_key_t           mKey[ACL_CONF_DEPTH_MAX];
    acp_sint32_t             mDepth;
    acp_sint32_t             mIgnoreDepth;
} aclConfContext;


/*
 * lexer interface
 */
acp_sint32_t aclConfGetToken(aclConfContext *aContext);

/*
 * parser interface
 */
void *aclConfParseAlloc(void);
void  aclConfParseFree(void *aParser);
void  aclConfParse(void           *aParser,
                   acp_sint32_t    aTokenID,
                   aclConfToken    aToken,
                   aclConfContext *aContext);

/*
 * function for parser
 */
void aclConfAssignKey(aclConfContext *aContext, aclConfToken *aToken);
void aclConfAssignNull(aclConfContext *aContext, aclConfToken *aToken);
void aclConfAssignValue(aclConfContext *aContext, aclConfToken *aToken);
void aclConfAssignDefault(aclConfContext *aContext, acl_conf_def_t *aDef);

void aclConfError(aclConfContext   *aContext,
                  aclConfToken     *aToken,
                  aclConfErrorType  aError);

/*
 * Initialize aDef
 */
ACP_INLINE void aclConfInitDef(acl_conf_def_t *aDef)
{
    acp_sint32_t i;

    if (aDef != NULL)
    {
        for (i = 0; aDef[i].mType != ACL_CONF_TYPE_NONE; i++)
        {
            /*
             * Key가 NULL인 (Key가 지정되지 않은) CONTAINER는
             * UNNAMED_CONTAINER로 취급함
             */
            if ((aDef[i].mKey == NULL) &&
                (aDef[i].mType == ACL_CONF_TYPE_CONTAINER))
            {
                aDef[i].mType = ACL_CONF_TYPE_UNNAMED_CONTAINER;
            }
            else
            {
                /* do nothing */
            }

            aDef[i].mLineFound = 0;
        }
    }
    else
    {
        /* do nothing */
    }
}

/*
 * finalize aDef
 */
ACP_INLINE void aclConfFinalDef(aclConfContext *aContext, acl_conf_def_t *aDef)
{
    acp_sint32_t i;

    if ((aDef != NULL) && ACP_RC_IS_SUCCESS(aContext->mRC))
    {
        for (i = 0; aDef[i].mType != ACL_CONF_TYPE_NONE; i++)
        {
            if (aDef[i].mLineFound == 0)
            {
                aclConfAssignDefault(aContext, &aDef[i]);

                if (ACP_RC_NOT_SUCCESS(aContext->mRC))
                {
                    break;
                }
                else
                {
                    /* continue */
                }
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        /* do nothing */
    }
}


ACP_EXTERN_C_END


#endif
