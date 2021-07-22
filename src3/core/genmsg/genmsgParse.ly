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
 * $Id: genmsgParse.ly 1414 2007-12-07 08:14:12Z shsuh $
 ******************************************************************************/

%include
{
#include <genmsg.h>
}

%name           genmsgParse
%token_prefix   GENMSG_TOKEN_
%token_type     { genmsgToken }
%default_type   { genmsgToken }

%extra_argument { genmsgParseContext *aParseContext }

%syntax_error
{
    (void)acpPrintf("%S:%d:%d: syntax error: \"%.*s\"\n",
                    &aParseContext->mFilePath,
                    TOKEN.mLineNo,
                    TOKEN.mColNo,
                    TOKEN.mLen,
                    TOKEN.mPtr);
    acpProcExit(1);
}


start ::= sections.

sections ::= sections section.
sections ::= section.

section ::= file_section product_definitions.
section ::= error_section message_definitions.
section ::= log_section message_definitions.

file_section ::= SECTION FILE SEMICOLON.
{
    aParseContext->mSection = GENMSG_SECTION_FILE;
}
error_section ::= SECTION ERROR SEMICOLON.
{
    aParseContext->mSection = GENMSG_SECTION_ERROR;
}
log_section ::= SECTION LOG SEMICOLON.
{
    aParseContext->mSection = GENMSG_SECTION_LOG;
}

product_definitions ::= product_definitions product_definition.
product_definitions ::= product_definition.

message_definitions ::= message_definitions message_definition.
message_definitions ::= message_definition.


product_definition ::= PRODUCT_ID(A) ASSIGN NUMBER(B) SEMICOLON.
{
    if (aParseContext->mProductID >= 0)
    {
        genmsgParseError(aParseContext, &A, "PRODUCT_ID is already defined.");
    }
    else
    {
        aParseContext->mProductID = genmsgTokenToInt(aParseContext, &B);

        if ((aParseContext->mProductID < ACE_PRODUCT_MIN) ||
            (aParseContext->mProductID > ACE_PRODUCT_MAX))
        {
            genmsgParseError(aParseContext, &B, "invalid range PRODUCT_ID.");
        }
        else
        {
            /* do nothing */
        }
    }
}
product_definition ::= PRODUCT_NAME(A) ASSIGN STRING(B) SEMICOLON.
{
    acp_sint32_t i;

    if (acpStrGetLength(&aParseContext->mProductName) > 0)
    {
        genmsgParseError(aParseContext, &A, "PRODUCT_NAME is already defined.");
    }
    else
    {
        for (i = 1; i < (B.mLen - 1); i++)
        {
            if (acpCharIsAlpha(B.mPtr[i]) != ACP_TRUE)
            {
                B.mColNo += i;
                genmsgParseError(aParseContext,
                                 &B,
                                 "PRODUCT_NAME must be alphabet letters");
            }
            else
            {
                /* continue */
            }
        }

        acpStrSetConstCStringWithLen(&aParseContext->mProductName,
                                     B.mPtr,
                                     B.mLen);
    }
}
product_definition ::= PRODUCT_ABBR(A) ASSIGN STRING(B) SEMICOLON.
{
    acp_sint32_t i;

    if (acpStrGetLength(&aParseContext->mProductAbbr) > 0)
    {
        genmsgParseError(aParseContext, &A, "PRODUCT_ABBR is already defined.");
    }
    else
    {
        for (i = 1; i < (B.mLen - 1); i++)
        {
            if (acpCharIsUpper(B.mPtr[i]) != ACP_TRUE)
            {
                B.mColNo += i;
                genmsgParseError(aParseContext,
                                 &B,
                                 "PRODUCT_ABBR must be 3 uppercase letters");
            }
            else
            {
                /* continue */
            }
        }

        if (B.mLen != 5)
        {
            genmsgParseError(aParseContext,
                             &B,
                             "PRODUCT_ABBR must be 3 uppercase letters");
        }
        else
        {
            /* do nothing */
        }

        acpStrSetConstCStringWithLen(&aParseContext->mProductAbbr,
                                     B.mPtr,
                                     B.mLen);
    }
}
product_definition ::= LANGUAGES ASSIGN languages SEMICOLON.

languages ::= languages COMMA IDENTIFIER(A).
{
    genmsgAddLanguage(aParseContext, &A);
}
languages ::= IDENTIFIER(A).
{
    genmsgAddLanguage(aParseContext, &A);
}


%type message_fields { genmsgMessage * }
%type message_field  { genmsgMessageField }

%destructor message_fields
{
    genmsgFreeMessage($$);
}

message_definition ::= BRACE_OPEN message_fields(A) BRACE_CLOSE(B).
{
    if (aParseContext->mSection == GENMSG_SECTION_ERROR)
    {
        genmsgAddErrorMessage(aParseContext, A, &B);
    }
    else if (aParseContext->mSection == GENMSG_SECTION_LOG)
    {
        genmsgAddLogMessage(aParseContext, A, &B);
    }
    else
    {
        genmsgParseError(aParseContext,
                         &B,
                         "message defined in unknown section.");
    }
}

message_fields(A) ::= message_fields(B) message_field(C).
{
    A = B;
    genmsgSetMessageField(aParseContext, A, &C);
}
message_fields(A) ::= message_field(B).
{
    A = genmsgAllocMessage(aParseContext);
    genmsgSetMessageField(aParseContext, A, &B);
}

message_field(A) ::= ERR_ID(B) ASSIGN NUMBER(C) SEMICOLON.
{
    if (aParseContext->mSection == GENMSG_SECTION_ERROR)
    {
        A.mToken        = C;
        A.mType         = GENMSG_MESSAGE_FIELD_ERRID;
        A.mField.mErrID = genmsgTokenToInt(aParseContext, &C);

        if ((A.mField.mErrID < ACE_SUBCODE_MIN) ||
            (A.mField.mErrID > ACE_SUBCODE_MAX))
        {
            genmsgParseError(aParseContext, &C, "invalid range ERR_ID.");
        }
        else
        {
            if ((A.mField.mErrID < aParseContext->mErrMsgCount) &&
                (aParseContext->mErrMsg[A.mField.mErrID] != NULL))
            {
                genmsgParseError(aParseContext, &C, "duplicated ERR_ID.");
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        genmsgParseError(aParseContext,
                         &B,
                         "ERR_ID should be defined only in ERROR section.");
    }
}
message_field(A) ::= KEY ASSIGN IDENTIFIER(B) SEMICOLON.
{
    ACP_STR_INIT_CONST(A.mField.mKey);

    A.mToken = B;
    A.mType  = GENMSG_MESSAGE_FIELD_KEY;

    acpStrSetConstCStringWithLen(&A.mField.mKey, B.mPtr, B.mLen);

    if (genmsgFindMessage(aParseContext, &A.mField.mKey) != NULL)
    {
        genmsgParseError(aParseContext,
                         &B,
                         "duplicated KEY.");
    }
    else
    {
        /* do nothing */
    }
}
message_field(A) ::= MSG(B) ASSIGN strings(C) SEMICOLON.
{
    ACP_STR_INIT_CONST(A.mField.mMsg.mMsg);

    A.mToken            = B;
    A.mType             = GENMSG_MESSAGE_FIELD_MSG;
    A.mField.mMsg.mLang = genmsgGetLanguage(aParseContext, &B);

    acpStrSetConstCStringWithLen(&A.mField.mMsg.mMsg, C.mPtr, C.mLen);

    if (A.mField.mMsg.mLang == -1)
    {
        genmsgParseError(aParseContext, &B, "unknown language.");
    }
    else
    {
        /* do nothing */
    }
}

strings(A) ::= strings(B) STRING(C).
{
    A.mPtr = B.mPtr;
    A.mLen = C.mLen + (acp_sint32_t)(C.mPtr - B.mPtr);
}
strings(A) ::= STRING(B).
{
    A = B;
}
