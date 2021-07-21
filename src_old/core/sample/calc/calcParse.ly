/*******************************************************************************
 * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $Id: calcParse.ly 1414 2007-12-07 08:14:12Z shsuh $
 ******************************************************************************/

%include
{
#include <calc.h>
}

%name           calcParse
%token_prefix   CALC_TOKEN_
%token_type     { calcToken }
%default_type   { calcToken }

%extra_argument { calcContext *aContext }

%syntax_error
{
    (void)acpPrintf("syntax error\n");
    aContext->mError = ACP_TRUE;
}


%type expr { acp_sint32_t }


%left PLUS MINUS.
%left TIMES DIV MOD.


start ::= stmts.

stmts ::= stmts stmt.
stmts ::= .

stmt ::= VAR(B) ASSIGN expr(C) NEWLINE.
{
    calcSetVar(aContext, &B, C);
}
stmt ::= expr(B) NEWLINE.
{
    (void)acpPrintf("%d\n", B);
}
stmt ::= QUIT NEWLINE.
{
    acpProcExit(0);
}

expr(A) ::= expr(B) PLUS expr(C).
{
    A = B + C;
}
expr(A) ::= expr(B) MINUS expr(C).
{
    A = B - C;
}
expr(A) ::= expr(B) TIMES expr(C).
{
    A = B * C;
}
expr(A) ::= expr(B) DIV expr(C).
{
    if (C == 0)
    {
        (void)acpPrintf("divide by zero\n");
        aContext->mError = ACP_TRUE;
    }
    else
    {
        A = B / C;
    }
}
expr(A) ::= expr(B) MOD expr(C).
{
    if (C == 0)
    {
        (void)acpPrintf("divide by zero\n");
        aContext->mError = ACP_TRUE;
    }
    else
    {
        A = B % C;
    }
}
expr(A) ::= MINUS expr(B).
{
    A = -B;
}
expr(A) ::= PAREN_OPEN expr(B) PAREN_CLOSE.
{
    A = B;
}
expr(A) ::= VALUE(B).
{
    acp_uint64_t sValue;
    acp_sint32_t sSign;

    ACP_STR_DECLARE_CONST(sStr);
    ACP_STR_INIT_CONST(sStr);

    acpStrSetConstCStringWithLen(&sStr, B.mPtr, B.mLen);

    (void)acpStrToInteger(&sStr, &sSign, &sValue, NULL, 0, 0);

    A = sSign * sValue;
}
expr(A) ::= VAR(B).
{
    A = calcGetVar(aContext, &B);
}
