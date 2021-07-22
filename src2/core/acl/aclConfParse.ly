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
 * $Id: aclConfParse.ly 2824 2008-07-01 08:15:07Z shsuh $
 ******************************************************************************/

%include
{
#include <aclConf.h>
#include <aclConfPrivate.h>
}

%name           aclConfParse
%token_prefix   ACL_CONF_TOKEN_
%token_type     { aclConfToken }
%default_type   { aclConfToken }

%extra_argument { aclConfContext *aContext }

%syntax_error
{
    aclConfError(aContext, &TOKEN, ACL_CONF_ERR_SYNTAX_ERROR);
}

%parse_accept
{
    if (aContext->mTokenID != ACL_CONF_TOKEN_EOF)
    {
        aclConfError(aContext, NULL, ACL_CONF_ERR_UNEXPECTED_TOKEN);
    }
    else
    {
        /* do nothing */
    }
}

%parse_failure
{
    if (ACP_RC_IS_SUCCESS(aContext->mRC))
    {
        aclConfError(aContext, NULL, ACL_CONF_ERR_PARSE_FAILURE);
    }
    else
    {
        /* do nothing */
    }
}


conf_file ::= conf_list.

conf_list ::= conf_list conf_def.
conf_list ::= .

conf_def ::= conf_key conf_assign.
{
    if (aContext->mIgnoreDepth > 0)
    {
        aContext->mIgnoreDepth--;
    }
    else
    {
        aContext->mDepth--;
    }
}

conf_assign ::= conf_assign_collection PAREN_CLOSE.
conf_assign ::= conf_assign_collection conf_array_of_col PAREN_CLOSE.
conf_assign ::= conf_assign_collection conf_array_of_val PAREN_CLOSE.
conf_assign ::= conf_assign_collection conf_list_of_def PAREN_CLOSE.
conf_assign ::= conf_assign_null.
conf_assign ::= conf_assign_value.

conf_key ::= KEY(B).
{
    if (aContext->mIgnoreDepth > 0)
    {
        aContext->mIgnoreDepth++;
    }
    else
    {
        if (aContext->mDepth < ACL_CONF_DEPTH_MAX)
        {
            aclConfAssignKey(aContext, &B);
        }
        else
        {
            aclConfError(aContext, &B, ACL_CONF_ERR_DEPTH_OVERFLOW);
        }
    }
}

conf_assign_collection ::= ASSIGN_COLLECTION PAREN_OPEN.
conf_assign_null ::= ASSIGN_NULL(B).
{
    if (aContext->mIgnoreDepth > 0)
    {
        /* do nothing */
    }
    else
    {
        aclConfAssignNull(aContext, &B);
    }
}
conf_assign_value ::= ASSIGN_VALUE value(B).
{
    if (aContext->mIgnoreDepth > 0)
    {
        /* do nothing */
    }
    else
    {
        aclConfAssignValue(aContext, &B);
    }
}

conf_array_of_col ::= conf_array_of_col COMMA conf_collection.
conf_array_of_col ::= conf_collection.

conf_collection ::= PAREN_OPEN(B) conf_list_of_def PAREN_CLOSE.
{
    if (aContext->mIgnoreDepth > 0)
    {
        /* do nothing */
    }
    else
    {
        acl_conf_def_t *sDef   = aContext->mCurrentDef[aContext->mDepth - 1];
        acp_sint32_t    sIndex = aContext->mKey[aContext->mDepth - 1].mIndex;

        if ((sDef->mMultiplicity > 0) && (sDef->mMultiplicity <= sIndex))
        {
            aclConfError(aContext, &B, ACL_CONF_ERR_TOO_MANY_VALUE);
            aContext->mIgnoreDepth++;
        }
        else
        {
            if (aContext->mDepth < ACL_CONF_DEPTH_MAX)
            {
                aclConfFinalDef(aContext, aContext->mBaseDef[aContext->mDepth]);
                aclConfInitDef(aContext->mBaseDef[aContext->mDepth]);
            }
            else
            {
                /* do nothing */
            }

            aContext->mKey[aContext->mDepth - 1].mIndex++;
        }
    }
}

conf_array_of_val ::= conf_array_of_val COMMA value(B).
{
    if (aContext->mIgnoreDepth > 0)
    {
        /* do nothing */
    }
    else
    {
        aclConfAssignValue(aContext, &B);
    }
}
conf_array_of_val ::= value(B).
{
    if (aContext->mIgnoreDepth > 0)
    {
        /* do nothing */
    }
    else
    {
        aclConfAssignValue(aContext, &B);
    }
}

conf_list_of_def ::= conf_list_of_def conf_def.
conf_list_of_def ::= conf_def.

value(A) ::= VALUE(B).
{
    A = B;

    A.mIsQuotedString = ACP_FALSE;
}
value(A) ::= QVALUE(B).
{
    A = B;

    A.mLen            -= 2;
    A.mPtr            += 1;
    A.mIsQuotedString  = ACP_TRUE;
}
