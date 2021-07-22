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

/* *****************************************************************************
 * BEGIN OF DEF SECTION 
 * ****************************************************************************/

#include <accs.h>    
#undef VOID // from idTypes.h 
#include <accs_yacc.cpp.h> /* YACC generated definitions based on C++ grammar */

YYSTYPE accslval;

#define INFO_DEBUG

#ifdef INFO_DEBUG
#define LEX_DUMP(msg)  myAccs.dump(msg)
#else
#define LEX_DUMP  
#endif

#define WHITE_RETURN(x) { accslval.text = yytext; myAccs.addColumn(yyleng); LEX_DUMP(""); }
#define NEW_LINE_RETURN() { myAccs.addLine(1); LEX_DUMP("NEW LINE"); }

#define RETURN_VAL(x) { accslval.text = yytext;  myAccs.addColumn(yyleng); LEX_DUMP(""); return(x); }

#define PPOP_RETURN(x)         RETURN_VAL((int)*yytext) /* PreProcess and Parser operator */
#define ASCIIOP_RETURN(x)      RETURN_VAL((int)*yytext) /* a single character operator */
#define NAMEDOP_RETURN(x)      RETURN_VAL(x)            /* a multichar operator, with a name */
#define NUMERICAL_RETURN(x)    RETURN_VAL(x)            /* some sort of constant */
#define LITERAL_RETURN(x)      RETURN_VAL(x)            /* a string literal */

#define IDENTIFIER_RETURN()    RETURN_VAL(isaTYPE(yytext) ? TYPEDEFname : IDENTIFIER)
#define PP_KEYWORD_RETURN(x)   IDENTIFIER_RETURN()
#define PA_KEYWORD_RETURN(x)   RETURN_VAL(x)  /* standard C PArser Keyword */
#define PPPA_KEYWORD_RETURN(x) RETURN_VAL(x)  /* both PreProce and PArser keyword */
#define CPP_KEYWORD_RETURN(x)  PA_KEYWORD_RETURN(x)  /* C++ keyword */




/* *****************************************************************************
 * END OF DEF SECTION 
 * ****************************************************************************/
%}

%option noyywrap

%x COMMENT_CC
%x COMMENT_CXX
%x STRING_LITERAL

identifier [a-zA-Z_][0-9a-zA-Z_]*

exponent_part [eE][-+]?[0-9]+
fractional_constant ([0-9]*"."[0-9]+)|([0-9]+".")
floating_constant (({fractional_constant}{exponent_part}?)|([0-9]+{exponent_part}))[FfLl]?

decimal_constant [1-9][0-9]*{integer_suffix_opt}
octal_constant "0"[0-7]*{integer_suffix_opt}
hex_constant "0"[xX][0-9a-fA-F]+{integer_suffix_opt}

simple_escape [abfnrtv'"?\\]
octal_escape  [0-7]{1,3}

h_tab [\011]
form_feed [\014]
v_tab [\013]
c_return [\015]

horizontal_white [ ]|{h_tab}



%%

"/*" {
   /*=================== C COMMENT ==============================*/
   BEGIN COMMENT_CC;
   myAccs.addColumn(yyleng);
   LEX_DUMP("COMMENT_CC BEGIN");
}
   
<COMMENT_CC>\n {
    myAccs.addLine(1);
    LEX_DUMP("COMMENT_CC \\n");
}

<COMMENT_CC>"*/" {
    BEGIN 0;
    myAccs.addColumn(yyleng);
    LEX_DUMP("COMMENT_CC END");
}

<COMMENT_CC>. {
    myAccs.addColumn(yyleng);
    LEX_DUMP("COMMENT_CC Line");
}

"\/\/" {
   /*=================== C++ COMMENT ==============================*/
    BEGIN COMMENT_CXX;
    myAccs.addColumn(yyleng);
    LEX_DUMP("COMMENT_CXX BEGIN");
}

<COMMENT_CXX>\n {
    BEGIN 0;
    myAccs.addLine(1);
    LEX_DUMP("COMMENT_CXX END with \\n");
}

<COMMENT_CXX>. {
    myAccs.addColumn(yyleng);
    LEX_DUMP("COMMENT_CXX Line");
}


["]     {
   /*=================== STRING ==============================*/
    BEGIN STRING_LITERAL;
    myAccs.addColumn(yyleng);
    LEX_DUMP("STRING BEGIN");
}

<STRING_LITERAL>\\[ \t]*[\n] {
    myAccs.addLine(1);
    LEX_DUMP("STRING CONTINUE with \\");
}

<STRING_LITERAL>["] {
    BEGIN 0;
    myAccs.addColumn(yyleng);
    LEX_DUMP("STRING END with \"");
    LITERAL_RETURN(STRINGliteral);
}

<STRING_LITERAL>. {
    myAccs.addColumn(yyleng);
    LEX_DUMP("STRING Line");
}

<STRING_LITERAL>\n {
    myAccs.addLine(1);
    LEX_DUMP("STRING Line with \\n");
}



{horizontal_white}+     {
			WHITE_RETURN(' ');
			}

({v_tab}|{c_return}|{form_feed})+   {
			WHITE_RETURN(' ');
			}


({horizontal_white}|{v_tab}|{c_return}|{form_feed})*"\n"   {
    NEW_LINE_RETURN();
			}

keywords                {PA_KEYWORD_RETURN(AUTO);}

{identifier}          {IDENTIFIER_RETURN();}

{decimal_constant}  {NUMERICAL_RETURN(INTEGERconstant);}
{octal_constant}    {NUMERICAL_RETURN(OCTALconstant);}
{hex_constant}      {NUMERICAL_RETURN(HEXconstant);}
{floating_constant} {NUMERICAL_RETURN(FLOATINGconstant);}


%%

/******************************************************************
int isaTYPE(char * string)
{
}

*******************************************************************/
