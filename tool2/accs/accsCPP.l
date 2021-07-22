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
 * $Id: accsCPP.l 82075 2018-01-17 06:39:52Z jina.kim $
 ****************************************************************************/

/* *****************************************************************************
 * BEGIN OF DEF SECTION 
 * ****************************************************************************/

#include <accs.h>    
#undef VOID // from idTypes.h
#include <accsCPP_yacc.cpp.h> /* YACC generated definitions based on C++ grammar */

//YYSTYPE CPP_lval;

//#define INFO_DEBUG

#ifdef INFO_DEBUG
#define LEX_DUMP(msg)  accsg_mgr.dump(msg)
#else
#define LEX_DUMP  
#endif

#define WHITE_RETURN(x) { CPP_lval.text = yytext; accsg_mgr.addColumn(yyleng); LEX_DUMP(""); }
//#define NEW_LINE_RETURN() { accsg_mgr.addLine(1); LEX_DUMP("NEW LINE"); }

#define RETURN_VAL(x) { CPP_lval.text = yytext;  accsg_mgr.addColumn(yyleng); LEX_DUMP(""); return(x); }

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


//#define NAMED_PPOP_RETURN(x) /* error: PreProcessor ONLY operator;  Do nothing */


int isaTYPE(char * string);
/* *****************************************************************************
 * END OF DEF SECTION 
 * ****************************************************************************/
%}

%option noyywrap

%x COMMENT_CC
%x COMMENT_CXX
%x PREPROCESSOR
%x STRING_LITERAL

identifier [a-zA-Z_][0-9a-zA-Z_]*

exponent_part [eE][-+]?[0-9]+
fractional_constant ([0-9]*"."[0-9]+)|([0-9]+".")
floating_constant (({fractional_constant}{exponent_part}?)|([0-9]+{exponent_part}))[FfLl]?

integer_suffix_opt ([uU]?[lL]?[lL]?)
decimal_constant [1-9][0-9]*{integer_suffix_opt}
octal_constant "0"[0-7]*{integer_suffix_opt}
hex_constant "0"[xX][0-9a-fA-F]+{integer_suffix_opt}

simple_escape [abfnrtv'"?\\]
octal_escape  [0-7]{1,3}
hex_escape "x"[0-9a-fA-F]+

escape_sequence [\\]({simple_escape}|{octal_escape}|{hex_escape})
c_char [^'\\\n]|{escape_sequence}
s_char [^"\\\n]|{escape_sequence}


h_tab [\011]
form_feed [\014]
v_tab [\013]
c_return [\015]

horizontal_white [ ]|{h_tab}



%%

"/*" {
   /*=================== C COMMENT ==============================*/
   BEGIN COMMENT_CC;
   accsg_mgr.addColumn(yyleng);
   LEX_DUMP("COMMENT_CC BEGIN");
}
   
<COMMENT_CC>\n {
    accsg_mgr.addLine(1);
    LEX_DUMP("COMMENT_CC \\n");
}

<COMMENT_CC>"*/" {
    BEGIN 0;
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("COMMENT_CC END");
}

<COMMENT_CC>. {
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("COMMENT_CC Line");
}


"\/\/" {
   /*=================== C++ COMMENT ==============================*/
    BEGIN COMMENT_CXX;
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("COMMENT_CXX BEGIN");
}

<COMMENT_CXX>\n {
    BEGIN 0;
    accsg_mgr.addLine(1);
    LEX_DUMP("COMMENT_CXX END with \\n");
}

<COMMENT_CXX>. {
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("COMMENT_CXX Line");
}

"#" {
   /*=================== PREPROCESSOR ==============================*/
    BEGIN PREPROCESSOR;
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("PREPROCESSOR BEGIN");
}

<PREPROCESSOR>{horizontal_white} {
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("PREPROCESSOR Line");
}

<PREPROCESSOR>\\[ \t]*[\n] {
    // 이어진 라인 
    accsg_mgr.addLine(1);
    LEX_DUMP("PREPROCESSOR CONTINUE with \\");
}

<PREPROCESSOR>\n {
    BEGIN 0;
    accsg_mgr.addLine(1);
    LEX_DUMP("PREPROCESSOR END with \\n");
}

<PREPROCESSOR>. {
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("PREPROCESSOR Line");
}

"L"?["]     {
   /*=================== STRING ==============================*/
    BEGIN STRING_LITERAL;
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("STRING BEGIN");
}

<STRING_LITERAL>\\[ \t]*[\n] {
    accsg_mgr.addLine(1);
    LEX_DUMP("STRING CONTINUE with \\");
}

<STRING_LITERAL>["] {
    BEGIN 0;
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("STRING END with \"");
    LITERAL_RETURN(STRINGliteral);
}

<STRING_LITERAL>. {
    accsg_mgr.addColumn(yyleng);
    LEX_DUMP("STRING Line");
}

<STRING_LITERAL>\n {
    accsg_mgr.addLine(1);
    LEX_DUMP("STRING Line with \\n");
}



{horizontal_white}+     {
			WHITE_RETURN(' ');
			}

({v_tab}|{c_return}|{form_feed})+   {
			WHITE_RETURN(' ');
			}


({horizontal_white}|{v_tab}|{c_return}|{form_feed})*"\n"   {
    accsg_mgr.addLine(1);
			}

auto                {PA_KEYWORD_RETURN(AUTO);}
break               {PA_KEYWORD_RETURN(BREAK);}
case                {PA_KEYWORD_RETURN(CASE);}
char                {PA_KEYWORD_RETURN(CHAR);}
const               {PA_KEYWORD_RETURN(CONST);}
continue            {PA_KEYWORD_RETURN(CONTINUE);}
default             {PA_KEYWORD_RETURN(DEFAULT);}
define             {PP_KEYWORD_RETURN(DEFINE);}
defined            {PP_KEYWORD_RETURN(OPDEFINED);}
do                  {PA_KEYWORD_RETURN(DO);}
double              {PA_KEYWORD_RETURN(DOUBLE);}
elif                {PP_KEYWORD_RETURN(ELIF);}
else              {PPPA_KEYWORD_RETURN(ELSE);}
endif              {PP_KEYWORD_RETURN(ENDIF);}
enum                {PA_KEYWORD_RETURN(ENUM);}
error              {PP_KEYWORD_RETURN(ERROR);}
extern              {PA_KEYWORD_RETURN(EXTERN);}
float               {PA_KEYWORD_RETURN(FLOAT);}
for                 {PA_KEYWORD_RETURN(FOR);}
goto                {PA_KEYWORD_RETURN(GOTO);}
if                {PPPA_KEYWORD_RETURN(IF);}
ifdef              {PP_KEYWORD_RETURN(IFDEF);}
ifndef             {PP_KEYWORD_RETURN(IFNDEF);}
include            {PP_KEYWORD_RETURN(INCLUDE); }
int                 {PA_KEYWORD_RETURN(INT);}
line               {PP_KEYWORD_RETURN(LINE);}
long                {PA_KEYWORD_RETURN(LONG);}
pragma             {PP_KEYWORD_RETURN(PRAGMA);}
register            {PA_KEYWORD_RETURN(REGISTER);}
return              {PA_KEYWORD_RETURN(RETURN);}
short               {PA_KEYWORD_RETURN(SHORT);}
signed              {PA_KEYWORD_RETURN(SIGNED);}
sizeof              {PA_KEYWORD_RETURN(SIZEOF);}
static              {PA_KEYWORD_RETURN(STATIC);}
struct              {PA_KEYWORD_RETURN(STRUCT);}
switch              {PA_KEYWORD_RETURN(SWITCH);}
typedef             {PA_KEYWORD_RETURN(TYPEDEF);}
undef              {PP_KEYWORD_RETURN(UNDEF);}
union               {PA_KEYWORD_RETURN(UNION);}
unsigned            {PA_KEYWORD_RETURN(UNSIGNED);}
void                {PA_KEYWORD_RETURN(VOID);}
volatile            {PA_KEYWORD_RETURN(VOLATILE);}
while               {PA_KEYWORD_RETURN(WHILE);}


class               {CPP_KEYWORD_RETURN(CLASS);}
delete              {CPP_KEYWORD_RETURN(DELETE);}
friend              {CPP_KEYWORD_RETURN(FRIEND);}
inline              {CPP_KEYWORD_RETURN(INLINE);}
new                 {CPP_KEYWORD_RETURN(NEW);}
operator            {CPP_KEYWORD_RETURN(OPERATOR);}
overload            {CPP_KEYWORD_RETURN(OVERLOAD);}
protected           {CPP_KEYWORD_RETURN(PROTECTED);}
private             {CPP_KEYWORD_RETURN(PRIVATE);}
public              {CPP_KEYWORD_RETURN(PUBLIC);}
this                {CPP_KEYWORD_RETURN(THIS);}
virtual             {CPP_KEYWORD_RETURN(VIRTUAL);}

{identifier}          {IDENTIFIER_RETURN();}

{decimal_constant}  {NUMERICAL_RETURN(INTEGERconstant);}
{octal_constant}    {NUMERICAL_RETURN(OCTALconstant);}
{hex_constant}      {NUMERICAL_RETURN(HEXconstant);}
{floating_constant} {NUMERICAL_RETURN(FLOATINGconstant);}


"L"?[']{c_char}+[']     {
			NUMERICAL_RETURN(CHARACTERconstant);
			}


"("                  {PPOP_RETURN(LP);}
")"                  {PPOP_RETURN(RP);}
","                  {PPOP_RETURN(COMMA);}
"{"                  {ASCIIOP_RETURN(LC);}
"}"                  {ASCIIOP_RETURN(RC);}
"["                  {ASCIIOP_RETURN(LB);}
"]"                  {ASCIIOP_RETURN(RB);}
"."                  {ASCIIOP_RETURN(DOT);}
"&"                  {ASCIIOP_RETURN(AND);}
"*"                  {ASCIIOP_RETURN(STAR);}
"+"                  {ASCIIOP_RETURN(PLUS);}
"-"                  {ASCIIOP_RETURN(MINUS);}
"~"                  {ASCIIOP_RETURN(NEGATE);}
"!"                  {ASCIIOP_RETURN(NOT);}
"/"                  {ASCIIOP_RETURN(DIV);}
"%"                  {ASCIIOP_RETURN(MOD);}
"<"                  {ASCIIOP_RETURN(LT);}
">"                  {ASCIIOP_RETURN(GT);}
"^"                  {ASCIIOP_RETURN(XOR);}
"|"                  {ASCIIOP_RETURN(PIPE);}
"?"                  {ASCIIOP_RETURN(QUESTION);}
":"                  {ASCIIOP_RETURN(COLON);}
";"                  {ASCIIOP_RETURN(SEMICOLON);}
"="                  {ASCIIOP_RETURN(ASSIGN);}

".*"                 {NAMEDOP_RETURN(DOTstar);}
"::"                 {NAMEDOP_RETURN(CLCL);}
"->"                 {NAMEDOP_RETURN(ARROW);}
"->*"                {NAMEDOP_RETURN(ARROWstar);}
"++"                 {NAMEDOP_RETURN(ICR);}
"--"                 {NAMEDOP_RETURN(DECR);}
"<<"                 {NAMEDOP_RETURN(LS);}
">>"                 {NAMEDOP_RETURN(RS);}
"<="                 {NAMEDOP_RETURN(LE);}
">="                 {NAMEDOP_RETURN(GE);}
"=="                 {NAMEDOP_RETURN(EQ);}
"!="                 {NAMEDOP_RETURN(NE);}
"&&"                 {NAMEDOP_RETURN(ANDAND);}
"||"                 {NAMEDOP_RETURN(OROR);}
"*="                 {NAMEDOP_RETURN(MULTassign);}
"/="                 {NAMEDOP_RETURN(DIVassign);}
"%="                 {NAMEDOP_RETURN(MODassign);}
"+="                 {NAMEDOP_RETURN(PLUSassign);}
"-="                 {NAMEDOP_RETURN(MINUSassign);}
"<<="                {NAMEDOP_RETURN(LSassign);}
">>="                {NAMEDOP_RETURN(RSassign);}
"&="                 {NAMEDOP_RETURN(ANDassign);}
"^="                 {NAMEDOP_RETURN(ERassign);}
"|="                 {NAMEDOP_RETURN(ORassign);}
"..."                {NAMEDOP_RETURN(ELLIPSIS);}



%%

/* I won't bother to provide any error recovery. I won't  even  handle
unknown characters */
//"##"                 {NAMED_PPOP_RETURN(POUNDPOUND);}

/*******************************************************************/
int isaTYPE(char * string)
{
    /*  We  should  really  be  maintaining  a  symbol  table,  and be
    carefully keeping track of what the current scope is  (or  in  the
    case  of  "rescoped"  stuff,  what  scope  to  look in). Since the
    grammar is not annotated with  actions  to  track  transitions  to
    various  scopes,  and  there  is no symbol table, we will supply a
    hack to allow folks to test  the  grammar  out.   THIS  IS  NOT  A
    COMPLETE IMPLEMENTATION!!!! */
//     printf("string is [%s]\n", string);
//     return (strncmp(string, "Int", 3) == 0);
    return ('A' <= string[0] && 'Z' >= string[0]);
}

void accsMgr::setLexStdin(FILE *fd)
{
	yy_switch_to_buffer(yy_create_buffer(fd, YY_BUF_SIZE));
}
    
