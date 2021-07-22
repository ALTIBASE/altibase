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

%option 8bit
%option yylineno
%option noyywrap
%option prefix  = "PP"

%{

#include <ulpPreproc.h>
#include <ulpPreprocy.hpp>

#undef YY_READ_BUF_SIZE
#undef YY_BUF_SIZE
#define YY_READ_BUF_SIZE (16384)
#define YY_BUF_SIZE (YY_READ_BUF_SIZE * 2) /* size of default yyinput buffer */

// column 위치를 계산하기위한 macro
#define YY_USER_ACTION if(YY_AT_BOL()) { yycolumnP = 0; }\
                       gFirstcolumnP = yycolumnP; gLastcolumnP = yycolumnP+yyleng-1; \
                       yycolumnP += yyleng;

/* lex main function */
int  PPlex   ( YYSTYPE *lvalp );
void PPerror ( const SChar *aErrMsg);

//============== global variables for PPlexer ================//


/* externs of ulpMain.h */
extern ulpProgOption gUlpProgOption;
extern ulpCodeGen    gUlpCodeGen;
extern iduMemory    *gUlpMem;
// Macro table
extern ulpMacroTable gUlpMacroT;

/* extern of parser */
extern SInt             gUlpPPStartCond;
extern SInt             gUlpPPPrevCond;
extern ulpPPifstackMgr *gUlpPPifstackMgr[MAX_HEADER_FILE_NUM];
extern SInt             gUlpPPifstackInd;

/* BUG-27683 : iostream 사용 제거 */
// include를 처리하기위한 buffer state 배열.
struct ulpPPBufState
{
    YY_BUFFER_STATE         mPPIncludeStack;
    SInt                    mPPLinenum;
} gUlpPPIncludeStack[MAX_HEADER_FILE_NUM];
// gUlpPPIncludeStack의 index 값
SInt                 gUlpPPIncludeIndex = 0;

// 현제 #include file 처리 중인지를 알려줌.
idBool               gUlpPPisCInc   = ID_FALSE;
// 현제 exec sql include file 처리 중인지를 알려줌.
idBool               gUlpPPisSQLInc = ID_FALSE;
// preprocessor parsing 중 발생한 에러에대한 코드정보.
SChar               *gUlpPPErrCode  = NULL;
// column 위치를 계산하기위한 변수들.
SInt                 gFirstcolumnP  = 0;
SInt                 gLastcolumnP   = 0;
SInt                 yycolumnP      = 0;

// declare section error 처리 하기위한 변수.
SInt                 gPPERRCheck4DECL = 0;
/* BUG-28162 : SESC_DECLARE 부활  */
// -parse none 일경우 #ifdef SESC_DELCLARE 에대한 #endif를 찾기위한 변수.
SInt                 gUlpPPSescDec  = 0;

// JIS 처리를 위한 변수.
idnCharFeature       gUlpPPCharType4JIS = IDN_CF_UNKNOWN;
//============================================================//


%}

/*** for JIS ***/
NEG_TRAIL_CH     [\x80-\xFF]
POS_TRAIL_CH     [\x81-\xFE][\x40-\x7E\x80-\xFE]
SJIS_HWKATA      [\xA0-\xDF]
SJIS_JIS97       [\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]
SJIS_ALL         ({SJIS_HWKATA})|({SJIS_JIS97})

%x inc_opt_neg_trail
%x inc_opt_pos_trail
%x inc_opt_sjis
%x inc_neg_trail
%x inc_pos_trail
%x inc_sjis

/*** for C ***/
D           [0-9]
L           [a-zA-Z_$]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+
FS          (f|F|l|L)
IS          (u|U|l|L)*

/*** for EmSQL***/
BEGIN_DECL   (?i:"exec")[ \t\n]+(?i:"sql")[ \t\n]+(?i:"begin")[ \t\n]+(?i:"declare")[ \t\n]+(?i:"section")[ \t\n]*;
END_DECL     (?i:"exec")[ \t\n]+(?i:"sql")[ \t\n]+(?i:"end")[ \t\n]+(?i:"declare")[ \t\n]+(?i:"section")[ \t\n]*;
OPTION_INC   (?i:"exec")[ \t\n]+(?i:"sql")[ \t\n]+(?i:"option")[ \t\n]*"("[ \t\n]*(?i:"include")
INCLUDE      (?i:"exec")[ \t\n]+(?i:"sql")[ \t\n]+(?i:"include")
BEGIN_ARG    (?i:"exec")[ \t\n]+(?i:"sql")[ \t\n]+(?i:"begin")[ \t\n]+(?i:"argument")[ \t\n]+(?i:"section")[ \t\n]*;
END_ARG      (?i:"exec")[ \t\n]+(?i:"sql")[ \t\n]+(?i:"end")[ \t\n]+(?i:"argument")[ \t\n]+(?i:"section")[ \t\n]*;


/*** for MACRO ***/
%x INIT_SKIP
%x MACRO
%x MACRO_IF_SKIP
%x MACRO_INC
%x MACRO_DEF
%x MACRO_IFDEF
%x MACRO_IFNDEF_APRE
%x MACRO_SESC_DEC

%%

%{

switch( gUlpPPStartCond )
{
    case PP_ST_NONE :
        break;
    case PP_ST_INIT_SKIP :
        gUlpPPStartCond = PP_ST_NONE;
        BEGIN(INIT_SKIP);
        break;
    case PP_ST_MACRO :
        gUlpPPStartCond = PP_ST_NONE;
        BEGIN(MACRO);
        break;
    case PP_ST_MACRO_IF_SKIP :
        gUlpPPStartCond = PP_ST_NONE;
        BEGIN(MACRO_IF_SKIP);
        break;
    default :
        /* 특정 상태로 복귀 */
        BEGIN(gUlpPPStartCond);
        gUlpPPStartCond = PP_ST_NONE;
        break;
}
%}

<INIT_SKIP,MACRO>^([ \t]*{BEGIN_DECL})  {

    if( gUlpPPisCInc == ID_TRUE )
    {
        // do nothing
    }
    else
    {
        if ( gPPERRCheck4DECL == 0 )
        {
            gPPERRCheck4DECL = 1;
        }
        else
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_No_End_Declare_Section_Error );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }

    // WriteBuf에 그대로 쓴다
    WRITESTR2BUFPP(yytext);

    BEGIN(MACRO);
}
<INIT_SKIP,MACRO>(;[ \t]*{BEGIN_DECL})  {

    if( gUlpPPisCInc == ID_TRUE )
    {
        // do nothing
    }
    else
    {

        if ( gPPERRCheck4DECL == 0 )
        {
            gPPERRCheck4DECL = 1;
        }
        else
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_No_End_Declare_Section_Error );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }
    // WriteBuf에 그대로 쓴다
    WRITESTR2BUFPP(yytext);

    BEGIN(MACRO);
}
<INIT_SKIP,MACRO>^([ \t]*{END_DECL})   {

    if( gUlpPPisCInc == ID_TRUE )
    {
        // do nothing
    }
    else
    {

        if ( gPPERRCheck4DECL == 1 )
        {
            gPPERRCheck4DECL = 0;
        }
        else
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                            ulpERR_ABORT_COMP_No_Begin_Declare_Section_Error );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }

    // WriteBuf에 그대로 쓴다
    WRITESTR2BUFPP(yytext);

    ulpPPRestoreCond();
}
<INIT_SKIP,MACRO>(;[ \t]*{END_DECL})   {

    if( gUlpPPisCInc == ID_TRUE )
    {
        // do nothing
    }
    else
    {

        if ( gPPERRCheck4DECL == 1 )
        {
            gPPERRCheck4DECL = 0;
        }
        else
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_No_Begin_Declare_Section_Error );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }

    // WriteBuf에 그대로 쓴다
    WRITESTR2BUFPP(yytext);

    ulpPPRestoreCond();
}
<INIT_SKIP,MACRO>^([ \t]*{OPTION_INC}) {
    // #include header file 처리 중일 경우에는 error를 발생시킨다.
    if( gUlpPPisCInc == ID_TRUE )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Emsqlstmt_Exist_in_CIncludefile_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpPPPrevCond = YYSTATE;

    if( gUlpPPCharType4JIS == IDN_CF_SJIS )
    {
        BEGIN inc_opt_sjis;
    }
    else if ( gUlpPPCharType4JIS == IDN_CF_POS_TRAIL )
    {
        BEGIN inc_opt_pos_trail;
    }
    else
    {
        BEGIN inc_opt_neg_trail;
    }

    // option INCLUDE 의경우 에는 WriteBuf에 flush할 필요 없다.
    return EM_OPTION_INC;

}
<INIT_SKIP,MACRO>(;[ \t]*{OPTION_INC}) {
    // #include header file 처리 중일 경우에는 error를 발생시킨다.
    if( gUlpPPisCInc == ID_TRUE )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Emsqlstmt_Exist_in_CIncludefile_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpPPPrevCond = YYSTATE;

    if( gUlpPPCharType4JIS == IDN_CF_SJIS )
    {
        BEGIN inc_opt_sjis;
    }
    else if ( gUlpPPCharType4JIS == IDN_CF_POS_TRAIL )
    {
        BEGIN inc_opt_pos_trail;
    }
    else
    {
        BEGIN inc_opt_neg_trail;
    }

    // option INCLUDE 의경우 에는 WriteBuf에 flush할 필요 없다.
    return EM_OPTION_INC;

}

<inc_opt_neg_trail>([^ ;()=,*?<>|\t\n\"\r]|({NEG_TRAIL_CH}))+ {
    return EM_FILENAME;
}
<inc_opt_pos_trail>([^ ;()=,*?<>|\t\n\"\r]|({POS_TRAIL_CH}))+ {
    return EM_FILENAME;
}
<inc_opt_sjis>([^ ;()=,*?<>|\t\n\"\r]|({SJIS_ALL}))+ {
    return EM_FILENAME;
}
<inc_opt_neg_trail,inc_opt_pos_trail,inc_opt_sjis>{
[=]                     { return EM_EQUAL;  }
[(]                     { return EM_LPAREN; }
[)]                     { return EM_RPAREN; }
[,]                     { return EM_COMMA;  }
[;]                     { return EM_SEMI;   }
[<]                     { return EM_LBRAC;  }
[>]                     { return EM_RBRAC;  }
[\"]                    { return EM_DQUOTE; }
}

<inc_opt_neg_trail,inc_opt_pos_trail,inc_opt_sjis>"\n"|"\n\r" {
    WRITESTR2BUFPP(yytext);
}


<INIT_SKIP,MACRO>^([ \t]*{INCLUDE}[ \t\n]+(?i:"sqlca")(.h)?[ \t\n]*;)   {
    gUlpCodeGen.ulpGenComment( yytext );
}
<INIT_SKIP,MACRO>(;[ \t]*{INCLUDE}[ \t\n]+(?i:"sqlca")(.h)?[ \t\n]*;)   {
    gUlpCodeGen.ulpGenComment( yytext );
}
<INIT_SKIP,MACRO>^([ \t]*{INCLUDE})  {
    // #include header file 처리 중일 경우에는 error를 발생시킨다.
    if( gUlpPPisCInc == ID_TRUE )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Emsqlstmt_Exist_in_CIncludefile_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpPPPrevCond = YYSTATE;

    if( gUlpPPCharType4JIS == IDN_CF_SJIS )
    {
        BEGIN inc_sjis;
    }
    else if ( gUlpPPCharType4JIS == IDN_CF_POS_TRAIL )
    {
        BEGIN inc_pos_trail;
    }
    else
    {
        BEGIN inc_neg_trail;
    }

    // option INCLUDE 의경우 에는 WriteBuf에 flush할 필요 없다.
    // 단지 header file을 embedded 시킬 뿐이다.
    return EM_INCLUDE;

}
<INIT_SKIP,MACRO>(;[ \t]*{INCLUDE})  {
    // #include header file 처리 중일 경우에는 error를 발생시킨다.
    if( gUlpPPisCInc == ID_TRUE )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Emsqlstmt_Exist_in_CIncludefile_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpPPPrevCond = YYSTATE;

    if( gUlpPPCharType4JIS == IDN_CF_SJIS )
    {
        BEGIN inc_sjis;
    }
    else if ( gUlpPPCharType4JIS == IDN_CF_POS_TRAIL )
    {
        BEGIN inc_pos_trail;
    }
    else
    {
        BEGIN inc_neg_trail;
    }

    // option INCLUDE 의경우 에는 WriteBuf에 flush할 필요 없다.
    // 단지 header file을 embedded 시킬 뿐이다.
    return EM_INCLUDE;
}

<inc_neg_trail>([^ ,*?\"<>|\t\n\;\r]|({NEG_TRAIL_CH}))+  {
    return EM_FILENAME;
}
<inc_pos_trail>([^ ,*?\"<>|\t\n\;\r]|({POS_TRAIL_CH}))+  {
    return EM_FILENAME;
}
<inc_sjis>([^ ,*?\"<>|\t\n\;\r]|({SJIS_ALL}))+  {
    return EM_FILENAME;
}
<inc_neg_trail,inc_pos_trail,inc_sjis>{
[<]                    { return EM_LBRAC; }
[>]                    { return EM_RBRAC; }
[\"]                   { return EM_DQUOTE; }
[;]                    { return EM_SEMI; }
}
<inc_neg_trail,inc_pos_trail,inc_sjis>"\n"|"\n\r" {
    WRITESTR2BUFPP(yytext);
}

<INIT_SKIP>"\n"|"\n\r"                           { WRITESTR2BUFPP(yytext); }
<INIT_SKIP>.                                     { WRITESTR2BUFPP(yytext); }

<MACRO>^([ \t]*"#"[ \t]*"include") |
       (\n[ \t]*"#"[ \t]*"include")                {
    if ( gUlpProgOption.mOptParseInfo == PARSE_NONE )
    {
        /* ignore */
        WRITESTR2BUFPP(yytext);
    }
    else
    {
        WRITESTR2BUFPP(yytext);
        BEGIN(MACRO_INC);
        return(M_INCLUDE);
    }
}
<MACRO_INC>"<"                                   { WRITESTR2BUFPP(yytext); return(M_LBRAC); }
<MACRO_INC>">"                                   { WRITESTR2BUFPP(yytext); return(M_RBRAC); }
<MACRO_INC>"\""                                  { WRITESTR2BUFPP(yytext); return(M_DQUOTE); }
<MACRO_INC>[^ <>\t\n\"\r]+                       { WRITESTR2BUFPP(yytext); return(M_FILENAME); }
<MACRO_INC>.                                     { WRITESTR2BUFPP(yytext); }
<INIT_SKIP,MACRO>^([ \t]*"#"[ \t]*"include"[ \t]*"<"[ \t]*"ses.h"[ \t]*">") |
                  (\n[ \t]*"#"[ \t]*"include"[ \t]*"<"[ \t]*"ses.h"[ \t]*">")     {
    WRITESTR2BUFPP((SChar*)"/* #include <ses.h> */");
}
<INIT_SKIP,MACRO>^([ \t]*"#"[ \t]*"include"[ \t]*"\""[ \t]*"ses.h"[ \t]*"\"") |
                  (\n[ \t]*"#"[ \t]*"include"[ \t]*"\""[ \t]*"ses.h"[ \t]*"\"")   {
    WRITESTR2BUFPP((SChar*)"/* #include \"ses.h\" */");
}
<INIT_SKIP,MACRO>^([ \t]*"#"[ \t]*"include"[ \t]*"<"[ \t]*"sqlca.h"[ \t]*">") |
                  (\n[ \t]*"#"[ \t]*"include"[ \t]*"<"[ \t]*"sqlca.h"[ \t]*">")     {
    WRITESTR2BUFPP((SChar*)"/* #include <sqlca.h> */");
}
<INIT_SKIP,MACRO>^([ \t]*"#"[ \t]*"include"[ \t]*"\""[ \t]*"sqlca.h"[ \t]*"\"") |
                  (\n[ \t]*"#"[ \t]*"include"[ \t]*"\""[ \t]*"sqlca.h"[ \t]*"\"")   {
    WRITESTR2BUFPP((SChar*)"/* #include \"sqlca.h\" */");
}

<MACRO>^([ \t]*"#"[ \t]*"define") |
        (\n[ \t]*"#"[ \t]*"define")              {
    WRITESTR2BUFPP(yytext);
    BEGIN(MACRO_DEF);
    return(M_DEFINE);
}
<MACRO>^([ \t]*"#"[ \t]*"undef") |
        (\n[ \t]*"#"[ \t]*"undef")               {
    WRITESTR2BUFPP(yytext);
    BEGIN(MACRO_DEF);
    return(M_UNDEF);
}
<MACRO_DEF>{L}({L}|{D})*                         {
    SChar sCh;

    sCh  = yyinput();

    // 함수인지 check
    if (sCh == '(')
    {
        WRITESTR2BUFPP(yytext);
        WRITECH2BUFPP(sCh);
        do
        {
            sCh = ulpPPYYinput();
        }
        while (( sCh != ')' ) && ( sCh != '\n' ) && ( sCh != (SChar) EOF ));

        if( (sCh == (SChar) EOF) || (sCh == '\n') )
        {
            // error
            ulpErrorMgr  sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_DEFINE_FUNC_NOBRACE_Error );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

        return(M_FUNCTION);
    }
    else
    {
        unput(sCh);
        yytext[yyleng] = '\0';
        WRITESTR2BUFPP(yytext);
        return(M_IDENTIFIER);
    }
}
<MACRO_DEF>.                                     {
    WRITESTR2BUFPP(yytext);
}

<MACRO>^([ \t]*"#"[ \t]*"if") |
        (\n[ \t]*"#"[ \t]*"if")                  { return(M_IF);   }
<MACRO>^([ \t]*"#"[ \t]*"elif") |
        (\n[ \t]*"#"[ \t]*"elif")                { return(M_ELIF); }
<MACRO>^([ \t]*"#"[ \t]*"else") |
        (\n[ \t]*"#"[ \t]*"else")                { return(M_ELSE); }
<MACRO>^([ \t]*"#"[ \t]*"endif") |
        (\n[ \t]*"#"[ \t]*"endif")               { return(M_ENDIF);}


<MACRO>^([ \t]*"#"[ \t]*"ifdef") |
        (\n[ \t]*"#"[ \t]*"ifdef")               {
    BEGIN(MACRO_IFDEF);
    return(M_IFDEF);
}
<MACRO>^([ \t]*"#"[ \t]*"ifndef") |
        (\n[ \t]*"#"[ \t]*"ifndef")              {
    BEGIN(MACRO_IFDEF);
    return(M_IFNDEF);
}
<MACRO>^([ \t]*"#"[ \t]*"ifndef"[ \t]*"ALTIBASE_APRE") |
        (\n[ \t]*"#"[ \t]*"ifndef"[ \t]*"ALTIBASE_APRE")     {
    WRITESTR2BUFPP(yytext);
    BEGIN(MACRO_IFNDEF_APRE);
}
<MACRO_IFNDEF_APRE>^([ \t]*"#"[ \t]*"endif")  {
    WRITESTR2BUFPP(yytext);
    BEGIN(MACRO);
}
<MACRO_IFNDEF_APRE>(\n[ \t]*"#"[ \t]*"endif") {
    WRITESTR2BUFPP(yytext);
    BEGIN(MACRO);
}
<MACRO_IFNDEF_APRE>"\n"|"\n\r"            { WRITESTR2BUFPP(yytext); }
<MACRO_IFNDEF_APRE>.                      { WRITESTR2BUFPP(yytext); }

<MACRO_IFDEF>(?-i:0[xX]{H}+{IS}?)         { return(M_CONSTANT);   }
<MACRO_IFDEF>(?-i:0{D}+{IS}?)             { return(M_CONSTANT);   }
<MACRO_IFDEF>(?-i:{D}+{IS}?)              { return(M_CONSTANT);   }
<MACRO_IFDEF>(?-i:{D}+{E}{FS}?)           { return(M_CONSTANT);   }
<MACRO_IFDEF>'(\\.|[^\\'])+'              { return(M_CONSTANT);   }
<MACRO_IFDEF>(?-i:{D}*"."{D}+({E})?{FS}?) { return(M_CONSTANT);   }
<MACRO_IFDEF>(?-i:{D}+"."{D}*({E})?{FS}?) { return(M_CONSTANT);   }
<MACRO_IFDEF>{L}({L}|{D})*                { return(M_IDENTIFIER); }
<MACRO_IFDEF>{L}({L}|{D})*"("[^\n]*       {
    SChar *sStr;
    // 함수이름일경우 '(' 서부터의 토큰들은 무시된다.
    sStr = idlOS::strstr( yytext, "(" );
    if( sStr != NULL )
    {
        *sStr = '\0';
    }
    return(M_IDENTIFIER);
}

<MACRO_IFDEF>.                            {  }

<MACRO>^([ \t]*"#"[ \t]*"assert") |
        (\n[ \t]*"#"[ \t]*"assert")              {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"error") |
        (\n[ \t]*"#"[ \t]*"error")               {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();

}
<MACRO>^([ \t]*"#"[ \t]*"ident") |
        (\n[ \t]*"#"[ \t]*"ident")               {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();

}
<MACRO>^([ \t]*"#"[ \t]*"import") |
        (\n[ \t]*"#"[ \t]*"import")              {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"include_next") |
        (\n[ \t]*"#"[ \t]*"include_next")        {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"line") |
        (\n[ \t]*"#"[ \t]*"line")                {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"pragma") |
        (\n[ \t]*"#"[ \t]*"pragma")              {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"sccs") |
        (\n[ \t]*"#"[ \t]*"sccs")                {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"unassert") |
        (\n[ \t]*"#"[ \t]*"unassert")            {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>^([ \t]*"#"[ \t]*"warning") |
        (\n[ \t]*"#"[ \t]*"warning")             {
    WRITESTR2BUFPP(yytext);
    ulpPPSkipUnknownMacro();
}
<MACRO>[ \t\v\n\f]|"\n\r"                        { WRITESTR2BUFPP(yytext); }
<MACRO>.                                         { WRITESTR2BUFPP(yytext); }


<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"if")      {
    // 아래 regular expr. (\n[ \t]*"#"[ \t]*"if")와 합치면 flex compile시 에러 발생. 이유모름.
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"if")     {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"ifdef")      {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"ifdef")     {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"ifndef")      {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"ifndef")     {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"endif")      {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"endif")     {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"elif")      {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"elif")       {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"else")       {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"else")       {
    BEGIN(MACRO);
    yyless(0);
}

<MACRO_IF_SKIP>"\n"|"\n\r"                {  /* don't ignore newline */ WRITESTR2BUFPP(yytext); }
<MACRO_IF_SKIP>.                          {  /* ignore */ }

<INIT_SKIP>"#"[ \t]*"ifdef"[ \t]+"SESC_DECLARE"  {
    WRITESTR2BUFPP((SChar*)"EXEC SQL BEGIN DECLARE SECTION;");
    gUlpPPSescDec = 0;
    BEGIN(MACRO_SESC_DEC);
}

<MACRO_SESC_DEC>"#"[ \t]*"if"         {
    WRITESTR2BUFPP(yytext);
    gUlpPPSescDec++;
}
<MACRO_SESC_DEC>"#"[ \t]*"ifdef"      {
    WRITESTR2BUFPP(yytext);
    gUlpPPSescDec++;
}
<MACRO_SESC_DEC>"#"[ \t]*"ifndef"     {
    WRITESTR2BUFPP(yytext);
    gUlpPPSescDec++;
}
<MACRO_SESC_DEC>"#"[ \t]*"endif"      {
    /* BUG-28162 : SESC_DECLARE 부활  */
    if ( gUlpPPSescDec > 0 )
    {
        WRITESTR2BUFPP(yytext);
        gUlpPPSescDec--;
    }
    else
    {
        if ( gUlpPPSescDec == 0 )
        {
            WRITESTR2BUFPP((SChar*)"EXEC SQL END DECLARE SECTION;");
            gUlpPPSescDec = 0;
            BEGIN(INIT_SKIP);
        }
        else // < 0
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_NO_ENDIF_Macro_Error );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }
}
<MACRO_SESC_DEC>"\n"|"\n\r"                      { WRITESTR2BUFPP(yytext); }
<MACRO_SESC_DEC>.                                { WRITESTR2BUFPP(yytext); }

<INIT_SKIP,MACRO,MACRO_INC,MACRO_DEF>"/*" {
    WRITESTR2BUFPP(yytext);
    ulpPPCommentC();
}
<INIT_SKIP,MACRO,MACRO_INC,MACRO_DEF>"//" { WRITESTR2BUFPP(yytext);  ulpPPCommentCplus(); }

<inc_opt_neg_trail,inc_opt_pos_trail,inc_opt_sjis><<EOF>> {
    ulpErrorMgr sErrorMgr;
    ulpSetErrorCode( &sErrorMgr,
                      ulpERR_ABORT_COMP_Unterminated_EMString_Error );
    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    PPerror( ulpGetErrorMSG(&sErrorMgr) );
}
<inc_neg_trail,inc_pos_trail,inc_sjis><<EOF>> {
    ulpErrorMgr sErrorMgr;
    ulpSetErrorCode( &sErrorMgr,
                      ulpERR_ABORT_COMP_Unterminated_EMString_Error );
    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    PPerror( ulpGetErrorMSG(&sErrorMgr) );
}
<INIT_SKIP,MACRO,MACRO_IF_SKIP,MACRO_INC,MACRO_DEF,MACRO_IFDEF><<EOF>> {
    if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfgetIndex() > -1 )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_NO_ENDIF_Macro_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    if ( gPPERRCheck4DECL == 1 )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_No_End_Declare_Section_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    yyterminate();
}
<MACRO_IFNDEF_APRE,MACRO_SESC_DEC><<EOF>> {
    ulpErrorMgr sErrorMgr;
    ulpSetErrorCode( &sErrorMgr,
                     ulpERR_ABORT_COMP_NO_ENDIF_Macro_Error );
    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    PPerror( ulpGetErrorMSG(&sErrorMgr) );
}

%%


IDE_RC ulpPPInitialize( SChar *aFileName )
{
    PDL_stat   sFileInfo;
    PDL_HANDLE sOutfilePtr;

    sOutfilePtr = idlOS::open(aFileName, O_RDONLY);
    IDE_TEST_RAISE( sOutfilePtr == IDL_INVALID_HANDLE, ERR_FILE_OPEN );

    IDE_TEST_RAISE( idlOS::fstat(sOutfilePtr, &sFileInfo) == -1, ERR_FILE_OPEN );

    IDE_TEST_RAISE( sFileInfo.st_size == 0, ERR_FILE_OPEN_INVALID_SIZE );

    idlOS::close(sOutfilePtr);

    // yyin setting
    IDE_TEST_RAISE( (yyin = idlOS::fopen(aFileName, "r")) == NULL,
                    ERR_FILE_OPEN)

    /* BUG-27683 : iostream 사용 제거 */
    // include 처리일 경우 새 버퍼를 생성하고, switch시켜준다.
    if ( gUlpPPIncludeIndex > 0 )
    {
        yy_switch_to_buffer ( yy_create_buffer( yyin, YY_BUF_SIZE) );
        // 새file 열때마다 line정보 초기화.
        yylineno = 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_FILE_OPEN);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                          ulpERR_ABORT_FILE_OPEN_ERROR,
                          aFileName, errno );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );

    }
    IDE_EXCEPTION (ERR_FILE_OPEN_INVALID_SIZE);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Invaild_File_Size,
                         aFileName );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void ulpPPFinalize()
{
    /* 하나의 include file처리가 끝났으므로 include index 1 감소 */
    gUlpProgOption.ulpPopIncList();

    /* 처리가 끝난 file close */
    if( yyin != NULL )
    {
        idlOS::fclose(yyin);
        yyin = NULL;
    }

    /* BUG-27683 : iostream 사용 제거 */
    // include 처리를 마친 경우 현제 버퍼를 지우고, 이전 버퍼로 switch시켜준다.
    if ( gUlpPPIncludeIndex > 0 )
    {
        gUlpPPIncludeIndex--;
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( gUlpPPIncludeStack[ gUlpPPIncludeIndex ].mPPIncludeStack );
        yylineno = gUlpPPIncludeStack[ gUlpPPIncludeIndex ].mPPLinenum;
    }
}

/* BUG-31626 : BOF occurs in the middle of parsing a long C style comment */
IDE_RC ulpPPCommentC()
{
/***********************************************************************
 *
 * Description :
 *      C comment 처리 함수. comment의 마지막인 " * / " 가 올때까지 읽어들여 write 버퍼에 쓴다.
 *
 ***********************************************************************/
    SChar sCh1;
    SChar sCh2;
    SInt  sIndex;
    SChar sTmpStr[MAX_COMMENTSTR_LEN]; //1024 bytes

    for( sIndex = 0 ; ( sCh1 = yyinput() ) != (SChar) EOF ; )
    {
        /* stack 이 꽉찼으면, write buf에 flush 하고 sIndex초기화후 다시 읽는다. */
        if ( ( MAX_COMMENTSTR_LEN - 6 ) < sIndex )
        {
            unput(sCh1);
            sTmpStr[sIndex]   = '\0';
            WRITESTR2BUFPP(sTmpStr);
            sIndex = 0;
            continue;
        }
        else
        {
            sTmpStr[sIndex++] = sCh1;
        }

        if( sCh1 == '*' )
        {
            sCh2 = yyinput();
            if( sCh2 == '/' )
            {   /* 주석의 마지막이 올경우 */
                sTmpStr[sIndex++] = sCh2;
                sTmpStr[sIndex]   = '\0';
                WRITESTR2BUFPP(sTmpStr);
                break;
            }
            else
            {
                unput(sCh2);
                continue;
            }
        }
    }

    /* BUG-27854 : AIX에서 comment처리중 hang걸림. */
    IDE_TEST_RAISE ( sCh1 == (SChar) EOF, ERR_COMMENT_NO_CLOSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_COMMENT_NO_CLOSE);
    {
        // unput EOF
        unput(sCh1);
        sTmpStr[sIndex] = '\0';
        WRITESTR2BUFPP(sTmpStr);

        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_C_Comment_No_Close_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC ulpPPCommentC4IF()
{
/***********************************************************************
 *
 * Description :
 *      #if 안의 C comment 처리 함수 (newline만 file에 write한다.)
 *
 ***********************************************************************/
    SChar sCh1;
    SChar sCh2;

CommentC:
    sCh1 = yyinput();

    while (( sCh1 != '*' ) && ( sCh1 != (SChar) EOF ))
    {
        if( sCh1 == '\n' )
        {
            // newline만은 write 한다. (line number 씽크를 맞추기위함)
            WRITECH2BUFPP('\n');
        }
        sCh1 = yyinput();
    }

    IDE_TEST_RAISE ( sCh1 == (SChar) EOF, ERR_COMMENT_NO_CLOSE );

    sCh2 = yyinput();
    if ( sCh2 != '/' )
    {
        unput(sCh2);
        goto CommentC;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_COMMENT_NO_CLOSE);
    {
        unput(sCh1);

        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_C_Comment_No_Close_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void ulpPPCommentCplus()
{
/***********************************************************************
 *
 * Description :
 *      C++ comment 처리 함수
 *
 ***********************************************************************/
    SChar sCh;
    SChar sTmpStr[MAX_COMMENTSTR_LEN]; //1024 bytes
    SInt  sIndex;

    sIndex = 0;

    sCh = yyinput();
    while ( (sCh != '\n') && (sCh != (SChar) EOF) )
    {
        sTmpStr[sIndex++] = sCh;
        sCh = yyinput();
    }
    // unput '\n' or EOF
    unput(sCh);

    sTmpStr[sIndex] = '\0';
    WRITESTR2BUFPP(sTmpStr);
}


void ulpPPCommentCplus4IF()
{
/***********************************************************************
 *
 * Description :
 *      #if 구문안에서의 C++ comment 처리 함수
 *
 ***********************************************************************/
    SChar sCh;

    sCh = yyinput();
    while ( (sCh != '\n') && (sCh != (SChar) EOF) )
    {
        sCh = yyinput();
    }
    // unput '\n' or EOF
    unput(sCh);
}


void ulpPPEraseBN4MacroText( SChar *aTmpDefinedStr,
                             idBool aIsIf)
{
/***********************************************************************
 *
 * Description :
 *      #if <expr>, #define ID <text> 구문 처리시 <expr>와 <text> 안에 포함되어 있는
 *      backspace newline 문자제거와 주석제거 처리를 하여  aTmpDefinedStr 버퍼에 복사해주는
 *      함수.
 *
 ***********************************************************************/
    SChar  sCh;
    SInt   sNumSpace;
    SInt   sIndex;
    idBool sBreak1;
    idBool sBreak2;

    sIndex     = 0;
    sNumSpace  = 0;
    sBreak1    = ID_FALSE;
    sBreak2    = ID_FALSE;

    // text 앞부분 처리.
    while( sBreak1 != ID_TRUE )
    {
        switch ( (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput() )
        {
            // 처음 공백 제거
            case '\t':
            case ' ' :
                do
                {
                    (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput();
                } while( (sCh == ' ') || (sCh == '\t') );
                sBreak1    = ID_TRUE;
                break;
            // 처음에 그냥 newline이 온다고?
            case '\n':
                sBreak1    = ID_TRUE;
                break;
            // 처음 주석이 오는경우 처리.
            case '/':
                (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput();
                if ( sCh == '/')
                {
                    (aIsIf == ID_TRUE)? ulpPPCommentCplus4IF() : ulpPPCommentCplus();
                }
                else
                {
                    if( sCh == '*' )
                    {
                        (aIsIf == ID_TRUE)? ulpPPCommentC4IF() : ulpPPCommentC();
                    }
                    else
                    {
                        (aIsIf == ID_TRUE)? unput(sCh) : ulpPPYYunput(sCh);
                        sBreak1    = ID_TRUE;
                    }
                }
                break;
            default:
                sBreak1    = ID_TRUE;
                break;
        }
    }

    (aIsIf == ID_TRUE)? unput(sCh) : ulpPPYYunput(sCh);

    while ( ((aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput()) != (SChar) EOF )
    {
        /* BUG-30233 : #define 내용이 5K이상이면 apre segv발생함. */
        IDE_TEST_RAISE( sIndex >= MAX_MACRO_DEFINE_CONTENT_LEN-1,
                        ERR_DEFINE_MACRO_OVERFLOW );
        switch(sCh)
        {
            case '\\':
                (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput();
                if ( sCh == '\n' )
                {
                    // newline만은 write 한다. (line number 씽크를 맞추기위함)
                    //WRITECH2BUFPP('\n');
                }
                else
                {
                    if( (sCh == ' ') || (sCh == '\t') )
                    {
                        sNumSpace++;
                        while ( ((aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput())
                                != (SChar) EOF )
                        {
                            if( sCh == ' ' )
                            {
                                sNumSpace++;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    if ( (sCh == '\n') || (sCh == (SChar) EOF) )
                    {
                        if (sCh == (SChar) EOF)
                        {
                            sBreak2 = ID_TRUE;
                        }
                        else
                        {
                            //WRITECH2BUFPP('\n');
                        }
                    }
                    else
                    {
                        aTmpDefinedStr[sIndex++] = '\\';
                        for(; sNumSpace > 0 ; sNumSpace-- )
                        {
                            IDE_TEST_RAISE( sIndex >= MAX_MACRO_DEFINE_CONTENT_LEN-1,
                                            ERR_DEFINE_MACRO_OVERFLOW );
                            aTmpDefinedStr[sIndex++] = ' ';
                        }
                        (aIsIf == ID_TRUE)? unput(sCh) : ulpPPYYunput(sCh);
                    }
                }
                break;

            case '\n':
                sBreak2 = ID_TRUE;
                aTmpDefinedStr[sIndex] = '\0';
                break;

            // 중간에 주석이 오는경우 처리.
            case '/':
                (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpPPYYinput();
                if ( sCh == '/')
                {
                    (aIsIf == ID_TRUE)? ulpPPCommentCplus4IF() : ulpPPCommentCplus();
                    sBreak2 = ID_TRUE;
                }
                else
                {
                    if( sCh == '*' )
                    {
                        (aIsIf == ID_TRUE)? ulpPPCommentC4IF() : ulpPPCommentC();
                        // text 중간에 /*~*/ 이오면 그냥 공백하나로 간주된다.
                        aTmpDefinedStr[sIndex++] = ' ';
                    }
                    else
                    {
                        aTmpDefinedStr[sIndex++] = '/';
                        IDE_TEST_RAISE( sIndex >= MAX_MACRO_DEFINE_CONTENT_LEN-1,
                                        ERR_DEFINE_MACRO_OVERFLOW );
                        aTmpDefinedStr[sIndex++] = sCh;
                    }
                }
                break;

            default:
                aTmpDefinedStr[sIndex++] = sCh;
                break;
        }
        if ( sBreak2 == ID_TRUE )
        {
            break;
        }
    }

    if ( sCh == (SChar) EOF )
    {
        aTmpDefinedStr[sIndex] = '\0';
    }
    else
    {
        if( sCh == '\n' )
        {
            (aIsIf == ID_TRUE)? unput(sCh) : ulpPPYYunput(sCh);
        }
    }

    return;

    /* BUG-30233 : #define 내용이 5K이상이면 apre segv발생함. */
    IDE_EXCEPTION (ERR_DEFINE_MACRO_OVERFLOW);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Define_Macro_Overflow_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    IDE_EXCEPTION_END;

    return;
}


IDE_RC ulpPPSkipUnknownMacro()
{
/***********************************************************************
 *
 * Description :
 *      처리 할필요 없는 macro 구문을 skip해주는 함수.
 *
 ***********************************************************************/
    SInt  sIndex;
    SChar sTmpStr[ MAX_SKIP_MACRO_LEN ];
    SChar sCh1;
    SChar sCh2;

    sIndex = 0;

    sIndex = idlOS::snprintf( sTmpStr, MAX_SKIP_MACRO_LEN, yytext );
    IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN, ERR_SKIP_MACRO_OVERFLOW );

SkipUnknownMacro:
    while (( (sCh1 = ulpPPYYinput()) != '\\' ) &&
           ( sCh1 != '\n' && sCh1 != (SChar) EOF ))
    {
        IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
        sTmpStr[sIndex++] = sCh1;
    }

    if ( sCh1 == '\\' )
    {
        IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
        sTmpStr[sIndex++] = sCh1;
        while ( (sCh2 = ulpPPYYinput()) != (SChar) EOF )
        {
            if( sCh2 == ' ' )
            {
                IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
                sTmpStr[sIndex++] = sCh2;
                continue;
            }
            else
            {
                break;
            }
        }
        if ( sCh2 == '\n' )
        {
            IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
            sTmpStr[sIndex++] = sCh2;
        }
        else
        {
            ulpPPYYunput(sCh2);
        }
        goto SkipUnknownMacro;
    }
    else
    {
        IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
        sTmpStr[sIndex++] = sCh1;
    }

    sTmpStr[sIndex] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_SKIP_MACRO_OVERFLOW);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                          ulpERR_ABORT_COMP_Unknown_Macro_Overflow_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void ulpPPRestoreCond(void)
{
/***********************************************************************
 *
 * Description :
 *      lexer의 start condition을 원복해주는 함수.
 *
 ***********************************************************************/
    switch ( gUlpProgOption.mOptParseInfo )
    {
      case PARSE_NONE :
        BEGIN(INIT_SKIP);
        break;
      case PARSE_PARTIAL :
        BEGIN(MACRO);
        break;
      case PARSE_FULL :
        BEGIN(MACRO);
        break;
      default :
        break;
    }
}


SChar ulpPPYYinput(void)
{
/***********************************************************************
 *
 * Description :
 *      yyinput() 하여 문자 하나를 읽어온후 file buffer에 쓴다.
 *
 ***********************************************************************/
    SChar sCh;

    sCh = yyinput();
    WRITECH2BUFPP(sCh);

    return sCh;
}


void ulpPPYYunput( SChar aCh )
{
/***********************************************************************
 *
 * Description :
 *      unput() 하여 문자 하나를 내뱉은후 file buffer에서 제거한다.
 *
 ***********************************************************************/
    unput(aCh);
    WRITEUNCH2BUFPP();
}

/* BUG-27683 : iostream 사용 제거 */
void ulpPPSaveBufferState()
{
/***********************************************************************
 *
 * Description :
 *      include 처리를 하기위한 현재 flex버퍼 상태를 저장하는 함수.
 *
 ***********************************************************************/

    if ( gUlpPPIncludeIndex >= MAX_HEADER_FILE_NUM )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Include_Depth_Too_Large_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        PPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    gUlpPPIncludeStack[ gUlpPPIncludeIndex ].mPPIncludeStack = YY_CURRENT_BUFFER;
    gUlpPPIncludeStack[ gUlpPPIncludeIndex ].mPPLinenum      = yylineno;
    gUlpPPIncludeIndex++;
}


int PPlex( YYSTYPE *lvalp )
{
/***********************************************************************
 *
 * Description :
 *      PPparser에서 token을 읽어 오기위해 호출되는 함수.
 *
 ***********************************************************************/
    SInt   sRc;
    UInt   sMytextlen;
    SChar *sNlsUse;
    SChar  sDefaultNLS[] = "US7ASCII";
    ulpErrorMgr sErrorMgr;

    if( gUlpPPCharType4JIS == IDN_CF_UNKNOWN )
    {
        sNlsUse = idlOS::getenv("ALTIBASE_NLS_USE");
        if( sNlsUse == NULL )
        {
            sNlsUse = sDefaultNLS;
        }

        if( ( idlOS::strncmp( sNlsUse, (SChar*)"SHIFTJIS", 8 ) == 0 ) ||
            ( idlOS::strncmp( sNlsUse, (SChar*)"JAPANESE", 8 ) == 0 ) ||
            /* PROJ-2590 [기능성] CP932 database character set 지원 */
            ( idlOS::strncmp( sNlsUse, (SChar*)"MS932", 5 ) == 0 ) ||
            ( idlOS::strncmp( sNlsUse, (SChar*)"CP932", 5 ) == 0 ) ||
            ( idlOS::strncmp( sNlsUse, (SChar*)"WINDOWS932", 10 ) == 0 ) )
        {
            gUlpPPCharType4JIS = IDN_CF_SJIS;
        }
        else if( ( idlOS::strncmp( sNlsUse, (SChar*)"GB231280", 8 ) == 0 )   ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"ZHS16CGB231280", 14 )== 0) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"CHINESE", 7 ) == 0 ) ||
                 /* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
                 ( idlOS::strncmp( sNlsUse, (SChar*)"MS936", 5 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"CP936", 5 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"GBK", 3 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"ZHS16GBK", 8 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"WINDOWS936", 10 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"BIG5", 4 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"ZHT16BIG5", 9 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"TAIWAN", 6 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"MS949", 5 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"CP949", 5 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"WINDOWS949", 10 ) == 0 ) )
        {
            gUlpPPCharType4JIS = IDN_CF_POS_TRAIL;
        }
        else
        {
            gUlpPPCharType4JIS = IDN_CF_NEG_TRAIL;
        }
    }

    sRc = yylex();

    sMytextlen     = (UInt)idlOS::strlen(yytext);

    IDE_TEST_RAISE(gUlpMem->alloc( sMytextlen+3, (void **) &(lvalp->strval) )
                   != IDE_SUCCESS, MAllocError);

    idlOS::memcpy(lvalp->strval, yytext, sMytextlen);
    (lvalp->strval)[sMytextlen] = '\0';
    fflush(stdout);

    return sRc;

    IDE_EXCEPTION(MAllocError);
    {
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Memory_Alloc_Error );
        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    }
    IDE_EXCEPTION_END;

    PPerror( ulpGetErrorMSG(&sErrorMgr) );

    return 0; // PPerror()에서 강제 종료 되므로 unreachable
}


void PPerror( const SChar* aErrMsg)
{
/***********************************************************************
 *
 * Description :
 *      PPlexer/parser의 error처리 함수.
 *
 ***********************************************************************/
    idBool sSkip;
    sSkip = ID_FALSE;

    sSkip = (gUlpPPErrCode==NULL)?ID_FALSE:
            (idBool) ((idlOS::strcmp(gUlpPPErrCode,"101H0") == 0) ||
            (idlOS::strcmp(gUlpPPErrCode,"102H0") == 0));

    if( idlOS::strncmp(aErrMsg,"syntax error", 12) == 0 )
    {
        idlOS::printf("\n[ERR-%.4s : %s",
                      (gUlpPPErrCode!=NULL)?gUlpPPErrCode:"000H",
                      "parse error");
        if (idlOS::strlen(aErrMsg) > 12)
        {
            printf(aErrMsg+12);
        }
        idlOS::printf("]\n");
    }
    else
    {
        idlOS::printf("\n[ERR-%.4s : %s]\n",
                      (gUlpPPErrCode!=NULL)?gUlpPPErrCode:"000H",
                      aErrMsg);
    }

    // file open error, file size error는 skip
    if( sSkip == ID_FALSE )
    {
        if( (gUlpPPisCInc == ID_TRUE) || (gUlpPPisSQLInc == ID_TRUE) )
        {
            idlOS::printf( "File  : %s\nLine  : %d\nOffset: %d-%d\nError_token:%s\n\n",
                       gUlpProgOption.mIncludeFileList[gUlpProgOption.mIncludeFileIndex].mFileName,
                       yylineno, gFirstcolumnP, gLastcolumnP,
                       // 아래와 같이 안하면 sun장비에서 segmetation fault
                       yytext);
        }
        else
        {
            idlOS::printf( "File  : %s\nLine  : %d\nOffset: %d-%d\nError_token:%s\n\n",
                       gUlpProgOption.mInFile,
                       yylineno, gFirstcolumnP, gLastcolumnP,
                       yytext);
        }
    }

    gUlpPPErrCode = NULL;

    idlOS::fflush(stdout);

    ulpFinalizeError();
}

