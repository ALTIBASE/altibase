/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
%{

/*****************************************************************************
 * $Id
 ****************************************************************************/

#include <accs.h>    
#undef VOID // from idTypes.h 
#include <accsProperty_yacc.cpp.h> /* YACC generated definitions based on C++ grammar */

//YYSTYPE property_lval;

/* #define PROP_DEBUG */

#ifdef PROP_DEBUG
SInt prop_dump(const SChar *msg);
#define PROP_DUMP(msg)  prop_dump(msg)
#else
#define PROP_DUMP  
#endif

%}

%option noyywrap
%x COMMENT_CC
%x COMMENT_CXX
%x STRING_LITERAL

identifier [a-zA-Z_][0-9a-zA-Z_]*

/* for number constants */
exponent_part [eE][-+]?[0-9]+
fractional_constant ([0-9]*"."[0-9]+)|([0-9]+".")
floating_constant (({fractional_constant}{exponent_part}?)|([0-9]+{exponent_part}))[FfLl]?
decimal_constant [-]?[1-9][0-9]*
octal_constant "0"[0-7]*
hex_constant "0"[xX][0-9a-fA-F]+

/* for white space */
h_tab [\011]
form_feed [\014]
v_tab [\013]
c_return [\015]
horizontal_white [ ]|{h_tab}

%%

"/*" {
   /*=================== C COMMENT ==============================*/
   BEGIN COMMENT_CC;
   PROP_DUMP("COMMENT_CC BEGIN");
}
   
<COMMENT_CC>\n {
    PROP_DUMP("COMMENT_CC \\n");
}

<COMMENT_CC>"*/" {
    BEGIN 0;
    PROP_DUMP("COMMENT_CC END");
}

<COMMENT_CC>. {
    PROP_DUMP("COMMENT_CC Line");
}

"\/\/" {
   /*=================== C++ COMMENT ==============================*/
    BEGIN COMMENT_CXX;
    PROP_DUMP("COMMENT_CXX BEGIN");
}

<COMMENT_CXX>\n {
    BEGIN 0;
    PROP_DUMP("COMMENT_CXX END with \\n");
}

<COMMENT_CXX>. {
    PROP_DUMP("COMMENT_CXX Line");
}


["]     {
   /*=================== STRING ==============================*/
    BEGIN STRING_LITERAL;
    PROP_DUMP("STRING BEGIN");
}

<STRING_LITERAL>\\[ \t]*[\n] {
    PROP_DUMP("STRING CONTINUE with \\");
}

<STRING_LITERAL>["] {
    BEGIN 0;
    PROP_DUMP("STRING END with \"");
    return STRING;
}

<STRING_LITERAL>. {
    PROP_DUMP("STRING Line");
}

<STRING_LITERAL>\n {
    PROP_DUMP("STRING Line with \\n");
}

{horizontal_white}+     {
    PROP_DUMP("White space \\n");
}

\n |
= {
    return yytext[0];
}

PRELOAD_LIST            { return PRELOAD_LIST;}
TAB_SIZE                { return TAB_SIZE;    }
LINE_WIDTH                { return LINE_WIDTH;    }
COL_START                { return COL_START;    }
PREFIX_LIST                { return PREFIX_LIST;    }

{identifier}([\.]{identifier})?        {
	idlOS::strcpy(property_lval.name, yytext);
	return IDENTIFIER;
}

{decimal_constant}  { 
	property_lval.number = idlOS::strtol(yytext, 0, 10); 
	return NUMBER;
}
{octal_constant}    {
	property_lval.number = idlOS::strtol(yytext, 0, 8); 
	return NUMBER;
}
{hex_constant}      {
	property_lval.number = idlOS::strtol(yytext, 0, 16); 
	return NUMBER;
}
{floating_constant} {
	property_lval.fnumber = atof(yytext);
	return NUMBER;
}

. {
    //idlOS::printf("invalid token...[%s][%x]\n", yytext, yytext[0] );
	yyterminate();
}
<<EOF>> {
	//idlOS::printf("end of lex \n");
	yyterminate();
}
%%
#ifdef PROP_DEBUG
SInt prop_dump(const SChar *msg)
{
    printf("{lex debug} %s<--%s\n", property_text, msg);
	return 0;
}
#endif

void accsPropertyMgr::setLexStdin(FILE *fd)
{
	yy_switch_to_buffer(yy_create_buffer(fd, YY_BUF_SIZE));
}
    
