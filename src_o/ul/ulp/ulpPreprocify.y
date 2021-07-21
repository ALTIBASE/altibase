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



%parse-param {void *aBuf}
%parse-param {int  *aRes}
%lex-param   {void *aBuf}
%{

#include <idl.h>
#include <ide.h>
#include <ulpMacroTable.h>
#include <ulpGenCode.h>
#include <ulpMacro.h>

#undef YY_READ_BUF_SIZE
#undef YY_BUF_SIZE
#define YY_READ_BUF_SIZE (16384)
#define YY_BUF_SIZE (YY_READ_BUF_SIZE * 2) /* size of default input buffer */

extern int PPIFlex( void *aBuf );
extern void PPIFerror( void *aBuf, int *aRes, const char* aMsg );

/* extern of ulpMain.h */
extern ulpMacroTable  gUlpMacroT;
/* extern of lexer */
extern idBool gUlpIsDefined;

%}

%union
{
    int  intval;
}

/*** MACRO tokens ***/
%token CONSTANT
%token CHARACTER
%token IDENTIFIER
%token IDENTIFIER_FUNC
%token DEFINED
%token DEFINED_LP
%token RIGHT_OP
%token LEFT_OP
%token OR_OP
%token LE_OP
%token GE_OP
%token EQ_OP
%token NE_OP
%token AND_OP

%%

macro_if_expr
    :
    {
        // error for no input string. return 1.
        YYABORT;
    }
    | conditional_expr
    {
        *aRes = $<intval>1;
    }
    ;

conditional_expr
    : logical_or_expr
    | logical_or_expr '?' logical_or_expr ':' conditional_expr
    {
        $<intval>$ = $<intval>1 ? $<intval>3 : $<intval>5;
    }
    ;

logical_or_expr
    : logical_and_expr
    | logical_or_expr OR_OP logical_and_expr
    {
        $<intval>$ = $<intval>1 || $<intval>3;
    }
    ;

logical_and_expr
    : inclusive_or_expr
    | logical_and_expr AND_OP inclusive_or_expr
    {
        $<intval>$ = $<intval>1 && $<intval>3;
    }
    ;

inclusive_or_expr
    : exclusive_or_expr
    | inclusive_or_expr '|' exclusive_or_expr
    {
        $<intval>$ = $<intval>1 | $<intval>3;
    }
    ;

exclusive_or_expr
    : and_expr
    | exclusive_or_expr '^' and_expr
    {
        $<intval>$ = $<intval>1 ^ $<intval>3;
    }
    ;

and_expr
    : equality_expr
    | and_expr '&' equality_expr
    {
        $<intval>$ = $<intval>1 & $<intval>3;
    }
    ;

equality_expr
    : relational_expr
    | equality_expr EQ_OP relational_expr
    {
        $<intval>$ = ( $<intval>1 == $<intval>3 );
    }
    | equality_expr NE_OP relational_expr
    {
        $<intval>$ = ( $<intval>1 != $<intval>3 );
    }
    ;

relational_expr
    : shift_expr
    | relational_expr '<' shift_expr
    {
        $<intval>$ = ( $<intval>1 < $<intval>3 );
    }
    | relational_expr '>' shift_expr
    {
        $<intval>$ = ( $<intval>1 > $<intval>3 );
    }
    | relational_expr LE_OP shift_expr
    {
        $<intval>$ = ( $<intval>1 <= $<intval>3 );
    }
    | relational_expr GE_OP shift_expr
    {
        $<intval>$ = ( $<intval>1 >= $<intval>3 );
    }
    ;

shift_expr
    : additive_expr
    | shift_expr LEFT_OP additive_expr
    {
        $<intval>$ = ( $<intval>1 << $<intval>3 );
    }
    | shift_expr RIGHT_OP additive_expr
    {
        $<intval>$ = ( $<intval>1 >> $<intval>3 );
    }
    ;

additive_expr
    : multiplicative_expr
    | additive_expr '+' multiplicative_expr
    {
        $<intval>$ = $<intval>1 + $<intval>3;
    }
    | additive_expr '-' multiplicative_expr
    {
        $<intval>$ = $<intval>1 - $<intval>3;
    }
    ;

multiplicative_expr
    : unary_expr
    | multiplicative_expr '*' unary_expr
    {
        $<intval>$ = $<intval>1 * $<intval>3;
    }
    | multiplicative_expr '/' unary_expr
    {
        // Division by zero error
        if( $<intval>3 == 0 )
        {
            idlOS::printf("ERR: Division by zero\n");
            YYABORT;
        }
        $<intval>$ = $<intval>1 / $<intval>3;
    }
    | multiplicative_expr '%' unary_expr
    {
        $<intval>$ = $<intval>1 % $<intval>3;
    }
    ;

unary_expr
    : postfix_expr
    | '+' unary_expr
    {
        $<intval>$ = +$<intval>2;
    }
    | '-' unary_expr
    {
        $<intval>$ = -$<intval>2;
    }
    | '~' unary_expr
    {
        $<intval>$ = ~$<intval>2;
    }
    | '!' unary_expr
    {
        $<intval>$ = !$<intval>2;
    }
    | DEFINED id_or_function
    {
        $<intval>$ = $<intval>2;
        gUlpIsDefined = ID_FALSE;
    }
    | DEFINED '(' id_or_function ')'
    {
        $<intval>$ = $<intval>3;
        gUlpIsDefined = ID_FALSE;
    }
    ;

postfix_expr
    : primary_expr
    ;

primary_expr
    : id_or_function
    | CONSTANT
    {
        $<intval>$ = $<intval>1;
    }
    | CHARACTER
    {
        $<intval>$ = $<intval>1;
    }
    | '(' conditional_expr ')'
    {
        $<intval>$ = $<intval>2;
    }
    ;

id_or_function
    : IDENTIFIER
    {
        // parser에서 ID를 심블테이블에서 찾아본다.
        // 숫자값이냐 이름이냐에 따른 복잡한 처리가 필요함
        //$<intval>$ = $<intval>1;
    }
    | IDENTIFIER_FUNC
    ;
%%


