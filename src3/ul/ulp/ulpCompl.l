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
%option caseless
%option yylineno
%option noyywrap
%option prefix  = "COMP"

%{

#include <ulpComp.h>
#include <ulpCompy.hpp>

#undef YY_READ_BUF_SIZE
#undef YY_BUF_SIZE
#define YY_READ_BUF_SIZE (16384)
#define YY_BUF_SIZE (YY_READ_BUF_SIZE * 2) /* size of default input buffer */

// column 위치를 계산하기위한 macro
#define YY_USER_ACTION if(YY_AT_BOL()) { yycolumnC = 0; }\
                       gFirstcolumnC = yycolumnC; gLastcolumnC = yycolumnC+yyleng-1; \
                       yycolumnC += yyleng;

#define RTN(name)                    { return name; }
#define RTN_ONLY(name)               { return name; }

//============== global variables for COMPlexer ===============//

/* externs of ulpMain.h */
extern ulpProgOption gUlpProgOption;
extern ulpCodeGen    gUlpCodeGen;
extern iduMemory    *gUlpMem;
// Macro table
extern ulpMacroTable gUlpMacroT;
// Scope table
extern ulpScopeTable gUlpScopeT;
// Struct tabletable
extern ulpStructTable gUlpStructT;

/* extern of parser */
extern SInt          gUlpCurDepth;
extern ulpParseInfo  gUlpParseInfo;
extern SInt          gUlpCOMPStartCond;
extern SInt          gUlpCOMPPrevCond;

extern ulpPPifstackMgr *gUlpCOMPifstackMgr[MAX_HEADER_FILE_NUM];
extern SInt             gUlpCOMPifstackInd;

/* BUG-27683 : iostream 사용 제거 */
// include를 처리하기위한 buffer state 배열.
struct ulpCOMPBufState
{
    YY_BUFFER_STATE         mCOMPIncludeStack;
    SInt                    mCOMPLinenum;
} gUlpCOMPIncludeStack[ MAX_HEADER_FILE_NUM ];
// gUlpPPIncludeStack의 index 값
SInt                    gUlpCOMPIncludeIndex = 0;

/* BUG-28068 : #define 매크로이름 확장안되는 문제 */
/* for macro expansion */
struct ulpCOMPMacroExpState
{
    YY_BUFFER_STATE         mBufState;
    SChar                   mExpandText[MAX_MACRO_DEFINE_CONTENT_LEN];
} gUlpCOMPMacroExpState[MAX_ID_EXPANSION];
// gUlpIDExpandStack의 index 값
SInt                    gUlpCOMPMacroExpIndex = 0;

// file에 쓸것인지를 결정함.
idBool               gDontPrint2file = ID_FALSE;
// parsing 중 발생한 에러에대한 코드정보.
SChar               *gUlpCOMPErrCode = NULL;

// column 위치를 계산하기위한 변수들.
SInt                 gFirstcolumnC = 0;
SInt                 gLastcolumnC  = 0;
SInt                 yycolumnC     = 0;

/* declare section error 처리 하기위한 변수. */
SInt           gUlpCOMPERRCheck4DECL = 0;

/* argument section error 처리 하기위한 변수. */
SInt           gUlpCOMPERRCheck4ARG  = 0;

/* EXEC SQL INCLUDE 헤더 파일의 라인넘버를 계산하기위한 변수. */
SInt           gUlpCOMPHeaderLine    = 0;
// 누적 헤더 라인수.
SInt           gUlpCOMPSumHeaderLine = 0;

/* 헤더파일의 path/name 을 저장하기 위한 변수 */
SChar          gUlpCOMPHeaderPath[MAX_FILE_PATH_LEN] = {0};

/* JIS 처리를 위한 변수 */
idnCharFeature gUlpCOMPCharType4JIS = IDN_CF_UNKNOWN;
//============================================================//


//=========== Function declarations for COMPlexer ============//

int  COMPlex( YYSTYPE *lvalp );
void COMPerror( const SChar *aErrMsg);

extern void ulpFinalizeError();
//============================================================//

%}

/*** for JIS Embedded SQL ***/
NEG_TRAIL_CH     [\x80-\xFF]
POS_TRAIL_CH     [\x81-\xFE][\x40-\x7E\x80-\xFE]
SJIS_HWKATA      [\xA0-\xDF]
SJIS_JIS97       [\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]
SJIS_ALL         ({SJIS_HWKATA})|({SJIS_JIS97})

/*NEG_TRAIL*/
%x EN
/*POS_TRAIL*/
%x EP
/*SJIS*/
%x EJ

/*** for C ***/
/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
/* -parse full */
%x C
/* -parse none */
%x NONE
/* -parse partial */
%x PARTIAL

%x MACRO_INC
%x MACRO_IFDEF
%x MACRO_IF_SKIP
%x MACRO_IFNDEF_APRE
%x MACRO_DEF
%x ARG_SKIP

D           [0-9]
L           [a-zA-Z_$]
H           [a-fA-F0-9]
E           [Ee][+-]?{D}+
FS          (f|F|l|L)
IS          (u|U|l|L)*

/*** for EmSQL***/
BEGIN_DECL       "exec"[ \t\n]+"sql"[ \t\n]+"begin"[ \t\n]+"declare"[ \t\n]+"section"[ \t\n]*;?
END_DECL         "exec"[ \t\n]+"sql"[ \t\n]+"end"[ \t\n]+"declare"[ \t\n]+"section"[ \t\n]*;?
BEGIN_DECL_ARG   "exec"[ \t\n]+"sql"[ \t\n]+"begin"[ \t\n]+"argument"[ \t\n]+"section"[ \t\n]*;?
END_DECL_ARG     "exec"[ \t\n]+"sql"[ \t\n]+"end"[ \t\n]+"argument"[ \t\n]+"section"[ \t\n]*;?

%x EMSQL


%%
%{

// yylex가 호출되면 가장먼저 이부분이 수행되며, gUlpCOMPStartCond 상태로 전이 된다.
switch( gUlpCOMPStartCond )
{
/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
    case CP_ST_VOID :
        break;
    case CP_ST_NONE :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(NONE);
        break;
    case CP_ST_PARTIAL :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(PARTIAL);
        break;
    case CP_ST_EMSQL :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(EMSQL);
        break;
    case CP_ST_EN :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(EN);
        break;
    case CP_ST_EP :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(EP);
        break;
    case CP_ST_EJ :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(EJ);
        break;
    case CP_ST_C :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(C);
        break;
    case CP_ST_MACRO_IF_SKIP :
        gUlpCOMPStartCond = CP_ST_VOID;
        BEGIN(MACRO_IF_SKIP);
        break;
    default :
        /* 특정 상태로 복귀 */
        BEGIN(gUlpCOMPStartCond);
        gUlpCOMPStartCond = CP_ST_VOID;
        break;
}
%}

<NONE,C,PARTIAL>{BEGIN_DECL}  {
    if ( gUlpCOMPERRCheck4DECL == 0 )
    {
        gUlpCOMPERRCheck4DECL = 1;

        /* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */ 
        if (gUlpProgOption.mOptLineMacro == ID_TRUE)
        {
            SChar sLineMacroStr[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = { 0, };

            gUlpCodeGen.ulpGenMakeLineMacroStr( sLineMacroStr );

            WRITESTR2BUFCOMP(sLineMacroStr);
        }
    }
    else
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                        ulpERR_ABORT_COMP_No_End_Declare_Section_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    BEGIN(C);
}

<NONE,C,PARTIAL>{END_DECL}   {
    if ( gUlpCOMPERRCheck4DECL == 1 )
    {
        gUlpCOMPERRCheck4DECL = 0;

        /* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */ 
        if (gUlpProgOption.mOptLineMacro == ID_TRUE)
        {
            SChar sLineMacroStr[MAX_FILE_PATH_LEN] = { 0, };

            gUlpCodeGen.ulpGenMakeLineMacroStr( sLineMacroStr );

            WRITESTR2BUFCOMP(sLineMacroStr);
        }
    }
    else
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_No_Begin_Declare_Section_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    ulpCOMPRestoreCond();
}


<C>{BEGIN_DECL_ARG}  {
    if ( gUlpCOMPERRCheck4ARG == 0 )
    {
        gUlpCOMPERRCheck4ARG = 1;
    }
    else
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_No_End_Arg_Section_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    BEGIN(ARG_SKIP);
}

<ARG_SKIP>{END_DECL_ARG}      {
    // 하위 호환성을 위한 argument sectioin 처리.
    if ( gUlpCOMPERRCheck4ARG == 1 )
    {
        gUlpCOMPERRCheck4ARG = 0;
    }
    else
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_No_Begin_Arg_Section_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    BEGIN(C);
}

<ARG_SKIP>.           { /* ignore */ }

<ARG_SKIP>"\n"|"\n\r" { /* BUG-42357 */ }

<NONE,PARTIAL>{BEGIN_DECL_ARG}  {
    if ( gUlpCOMPERRCheck4ARG == 0 )
    {
        gUlpCOMPERRCheck4ARG = 1;
    }
    else
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_No_End_Arg_Section_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    gDontPrint2file = ID_TRUE;

    BEGIN(C);
}

<C>{END_DECL_ARG}      {
    if ( gUlpCOMPERRCheck4ARG == 1 )
    {
        gUlpCOMPERRCheck4ARG = 0;
    }
    else
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_No_Begin_Arg_Section_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    gDontPrint2file = ID_FALSE;

    BEGIN(NONE);
}

<NONE,C,PARTIAL>"EXEC"[ \t]+"SQL"    {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;

    // EMSQL구문 처리중 SJIS를 처리하기 위한 상태 변화.
    switch ( gUlpCOMPCharType4JIS )
    {
        case IDN_CF_SJIS:
            BEGIN(EJ);
            break;
        case IDN_CF_POS_TRAIL:
            BEGIN(EP);
            break;
        case IDN_CF_NEG_TRAIL:
            BEGIN(EN);
            break;
        default:
            BEGIN(EMSQL);
            break;
    }

    return(EM_SQLSTART);
}

<NONE,C,PARTIAL>"@$LOVELY.K.J.H$" {
    // EXEC SQL INCLUDE 헤더파일의 시작을 알린다.
    // line number 재설정.
    gUlpCOMPHeaderLine = yylineno;
    ulpCOMPSetHeaderName();

    /* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */ 
    if (gUlpProgOption.mOptLineMacro == ID_TRUE)
    {
        SChar sLineMacroStr[MAX_FILE_PATH_LEN] = { 0, };

        gUlpCodeGen.ulpGenSetCurFileInfo( yylineno, 0, gUlpCOMPHeaderPath );

        gUlpCodeGen.ulpGenMakeLineMacroStr( sLineMacroStr );

        WRITESTR2BUFCOMP(sLineMacroStr);
    }
}

<NONE,C,PARTIAL>"#$LOVELY.K.J.H$" {

    /* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */ 
    if (gUlpProgOption.mOptLineMacro == ID_TRUE)
    {
        SChar sLineMacroStr[MAX_FILE_PATH_LEN] = { 0, };

        gUlpCodeGen.ulpGenAddSubHeaderFilesLineCnt();

        gUlpCodeGen.ulpGenResetCurFileInfo();

        gUlpCodeGen.ulpGenMakeLineMacroStr( sLineMacroStr );

        WRITESTR2BUFCOMP(sLineMacroStr);
    }

    // EXEC SQL INCLUDE 헤더파일의 끝을 알린다.
    // line number 재설정.
    gUlpCOMPSumHeaderLine += yylineno - gUlpCOMPHeaderLine + 1;
    gUlpCOMPHeaderLine     = 0;
    gUlpCOMPHeaderPath[0]  = '\0';
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"include") |
            (\n[ \t]*"#"[ \t]*"include")    {

    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;

    if ( gUlpProgOption.mOptParseInfo == PARSE_FULL )
    {
        // -parse full 일경우에만 #include 처리하여 header파일의 변수선언 파싱.
        WRITESTR2BUFCOMP(yytext);
        BEGIN(MACRO_INC);
        return(M_INCLUDE);
    }
    else
    {
        WRITESTR2BUFCOMP(yytext);
    }
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"if") |
            (\n[ \t]*"#"[ \t]*"if")          {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    return(M_IF);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"elif") |
            (\n[ \t]*"#"[ \t]*"elif")        {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    return(M_ELIF);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"else") |
            (\n[ \t]*"#"[ \t]*"else")        {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    return(M_ELSE);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"endif") |
            (\n[ \t]*"#"[ \t]*"endif")       {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    return(M_ENDIF);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"ifdef") |
            (\n[ \t]*"#"[ \t]*"ifdef")       {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    BEGIN(MACRO_IFDEF);
    return(M_IFDEF);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"ifndef") |
            (\n[ \t]*"#"[ \t]*"ifndef")      {
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    BEGIN(MACRO_IFDEF);
    return(M_IFNDEF);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"define") |
            (\n[ \t]*"#"[ \t]*"define")          {
    /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
     *             ulpComp 에서 재구축한다.                       */
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    WRITESTR2BUFCOMP(yytext);
    BEGIN(MACRO_DEF);
    return(M_DEFINE);
}
<C,PARTIAL>^([ \t]*"#"[ \t]*"undef") |
    (\n[ \t]*"#"[ \t]*"undef")                   {
    /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
     *             ulpComp 에서 재구축한다.                       */
    // 현재 상태 저장. (나중에 다시 복귀하기 위함)
    gUlpCOMPPrevCond = YYSTATE;
    WRITESTR2BUFCOMP(yytext);
    BEGIN(MACRO_DEF);
    return(M_UNDEF);
}
<MACRO_DEF>{L}({L}|{D})*                         {
    SChar sCh;

    sCh  = yyinput();

    // 함수인지 check
    if (sCh == '(')
    {
        WRITESTR2BUFCOMP(yytext);
        WRITECH2BUFCOMP(sCh);
        do
        {
            sCh = ulpCOMPYYinput();
        }
        while (( sCh != ')' ) && ( sCh != '\n' ) && ( sCh != (SChar) EOF ));

        if( (sCh == (SChar) EOF) || (sCh == '\n') )
        {
            // error
            ulpErrorMgr  sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_DEFINE_FUNC_NOBRACE_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

        return(M_FUNCTION);
    }
    else
    {
        unput(sCh);
        yytext[yyleng] = '\0';
        WRITESTR2BUFCOMP(yytext);
        return(M_IDENTIFIER);
    }
}
<MACRO_DEF>"/*"                                  {
    WRITESTR2BUFCOMP(yytext);
    ulpCOMPCommentC( ID_FALSE );
}
<MACRO_DEF>"//"                                  {
    WRITESTR2BUFCOMP(yytext);
    ulpCOMPCommentCplus();
}
<MACRO_DEF>.                                     {
    WRITESTR2BUFCOMP(yytext);
}

<C,PARTIAL>^([ \t]*"#"[ \t]*"ifndef"[ \t]*"ALTIBASE_APRE") |
            (\n[ \t]*"#"[ \t]*"ifndef"[ \t]*"ALTIBASE_APRE")     {
    WRITESTR2BUFCOMP(yytext);
    BEGIN(MACRO_IFNDEF_APRE);
}

<MACRO_IFNDEF_APRE>^([ \t]*"#"[ \t]*"endif")     {
    WRITESTR2BUFCOMP(yytext);
    BEGIN(C);
}
<MACRO_IFNDEF_APRE>(\n[ \t]*"#"[ \t]*"endif")    {
    WRITESTR2BUFCOMP(yytext);
    BEGIN(C);
}
<MACRO_IFNDEF_APRE>"\n"|"\n\r"            { WRITESTR2BUFCOMP(yytext); }
<MACRO_IFNDEF_APRE>.                      { WRITESTR2BUFCOMP(yytext); }

<NONE>^([ \t]*"#"[ \t]*"include")         {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"include")        {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"if")              {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"if")             {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"elif")            {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"elif")           {  ulpCOMPSkipMacro(); }
<NONE>^([ \t]*"#"[ \t]*"else")            {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"else")           {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"endif")           {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"endif")          {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"ifdef")           {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"ifdef")          {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"ifndef")          {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"ifndef")         {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"define")          {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"define")         {  ulpCOMPSkipMacro(); }

<NONE>^([ \t]*"#"[ \t]*"undef")           {  ulpCOMPSkipMacro(); }
<NONE>(\n[ \t]*"#"[ \t]*"undef")          {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"assert") |
                   (\n[ \t]*"#"[ \t]*"assert")      {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"error") |
                   (\n[ \t]*"#"[ \t]*"error")       {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"ident") |
                   (\n[ \t]*"#"[ \t]*"ident")       {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"import") |
                   (\n[ \t]*"#"[ \t]*"import")      {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"include_next") |
                   (\n[ \t]*"#"[ \t]*"include_next")    {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"line") |
                   (\n[ \t]*"#"[ \t]*"line")        {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"pragma") |
                   (\n[ \t]*"#"[ \t]*"pragma")      {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"sccs") |
                   (\n[ \t]*"#"[ \t]*"sccs")        {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"unassert") |
                   (\n[ \t]*"#"[ \t]*"unassert")    {  ulpCOMPSkipMacro(); }

<NONE,C,PARTIAL>^([ \t]*"#"[ \t]*"warning") |
                   (\n[ \t]*"#"[ \t]*"warning")     {  ulpCOMPSkipMacro(); }

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
<MACRO_IFDEF>.                            { /* ignore */ }

<NONE,PARTIAL>"{"                  {
    WRITESTR2BUFCOMP(yytext);
    // block을 만나면 current depth를 1증가.
    gUlpCurDepth++;
    /* BUG-44675 {} 중첩이 많으면 apre가 죽음  */
    if (gUlpCurDepth > MAX_SCOPE_DEPTH)
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Invalid_Scope_Depth_Error,
                         gUlpCurDepth);
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
}
<NONE,PARTIAL>"}"                  {
    WRITESTR2BUFCOMP(yytext);


    // -debug symbol 옵션이 켜있으면 block을 빠져 나올때마다 
    // struct/symbol table을 print해준다.
    if( gUlpProgOption.mDebugSymbol == ID_TRUE )
    {
        gUlpStructT.ulpPrintStructT();
        gUlpScopeT.ulpPrintAllSymT();
    }

    // struct 선언 block이 아니면, scope table에서 해당 symbol table 제거
    // struct table에서 해당 scope의 node들도 제거함.
    if( gUlpParseInfo.mStructDeclDepth == 0 )
    {
        /* BUG-44675 {} 중첩이 많으면 apre가 죽음  */
        if (gUlpCurDepth < 0)
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_Syntax_Error);
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
        gUlpScopeT.ulpSDelScope( gUlpCurDepth );
        gUlpStructT.ulpStructDelScope( gUlpCurDepth );
    }

    // current depth 감소.
    gUlpCurDepth--;
    // depth를 보고 whenever 구조체를 reset.
    gUlpCodeGen.ulpGenResetWhenever( gUlpCurDepth );
}
<NONE,PARTIAL>\"(\\.|[^\\"]|\\\n)*\"           { WRITESTR2BUFCOMP(yytext); }
<NONE,PARTIAL>\'(\\.|[^\\']|\\\n)*\'           { WRITESTR2BUFCOMP(yytext); }


<MACRO_INC>"<"                                 { WRITESTR2BUFCOMP(yytext); return(M_LBRAC);    }
<MACRO_INC>">"                                 { WRITESTR2BUFCOMP(yytext); return(M_RBRAC);    }
<MACRO_INC>"\""                                { WRITESTR2BUFCOMP(yytext); return(M_DQUOTE);   }
<MACRO_INC>[^ <>\t\n\"\r]+                     { WRITESTR2BUFCOMP(yytext); return(M_FILENAME); }
<MACRO_INC>.                                   { WRITESTR2BUFCOMP(yytext); }


<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"if")      {
    // 아래 regular expr. (\n[ \t]*"#"[ \t]*"if")와 합치면 flex compile시 에러 발생. 이유모름.
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"if")     {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"ifdef")      {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"ifdef")     {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"ifndef")      {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"ifndef")     {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"endif")      {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"endif")     {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"elif")      {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"elif")       {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>^([ \t]*"#"[ \t]*"else")       {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>(\n[ \t]*"#"[ \t]*"else")       {
    BEGIN(C);
    yyless(0);
}

<MACRO_IF_SKIP>.                          {  /* ignore */ }

<C>(?-i:"auto")         { WRITESTR2BUFCOMP(yytext); return(C_AUTO);       }
<C>(?-i:"break")        { WRITESTR2BUFCOMP(yytext); return(C_BREAK);      }
<C>(?-i:"case")         { WRITESTR2BUFCOMP(yytext); return(C_CASE);       }
<C>(?-i:"char")         { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext);
    return(C_CHAR);
}
<C>("varchar")      {
    // 함수 선언 부분(함수 인자로 varchar type이 사용)이 아닐경우 주석처리함.

    if ( gUlpParseInfo.mFuncDecl != ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
        gUlpParseInfo.mVarcharDecl = ID_TRUE;
    }
    WRITESTR2BUFCOMP(yytext);
    return(C_VARCHAR);
}
<C>(?-i:"const")        { WRITESTR2BUFCOMP(yytext); return(C_CONST);      }
<C>(?-i:"continue")     { WRITESTR2BUFCOMP(yytext); return(C_CONTINUE);   }
<C>(?-i:"default")      { WRITESTR2BUFCOMP(yytext); return(C_DEFAULT);    }
<C>(?-i:"do")           { WRITESTR2BUFCOMP(yytext); return(C_DO);         }
<C>(?-i:"double")       { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_DOUBLE);     }
<C>(?-i:"enum")         { WRITESTR2BUFCOMP(yytext); return(C_ENUM);       }
<C>(?-i:"extern")       { WRITESTR2BUFCOMP(yytext); return(C_EXTERN);     }
<C>(?-i:"float")        { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_FLOAT);      }
<C>(?-i:"for")          { WRITESTR2BUFCOMP(yytext); return(C_FOR);        }
<C>(?-i:"goto")         { WRITESTR2BUFCOMP(yytext); return(C_GOTO);       }
<C>(?-i:"int")          { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_INT);        }
<C>(?-i:"long")         { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_LONG);       }
<C>(?-i:"register")     { WRITESTR2BUFCOMP(yytext); return(C_REGISTER);   }
<C>(?-i:"return")       { WRITESTR2BUFCOMP(yytext); return(C_RETURN);     }
<C>(?-i:"short")        { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_SHORT);      }
<C>(?-i:"signed")       { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_SIGNED);     }
<C>(?-i:"sizeof")       { WRITESTR2BUFCOMP(yytext); return(C_SIZEOF);     }
<C>(?-i:"static")       { WRITESTR2BUFCOMP(yytext); return(C_STATIC);     }
<C>(?-i:"struct")       { WRITESTR2BUFCOMP(yytext); return(C_STRUCT);     }
<C>(?-i:"switch")       { WRITESTR2BUFCOMP(yytext); return(C_SWITCH);     }
<C>(?-i:"typedef")      { WRITESTR2BUFCOMP(yytext); return(C_TYPEDEF);    }
<C>(?-i:"union")        { WRITESTR2BUFCOMP(yytext); return(C_UNION);      }
<C>(?-i:"unsigned")     { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_UNSIGNED);   }
<C>(?-i:"void")         { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_VOID);       }
<C>(?-i:"volatile")     { WRITESTR2BUFCOMP(yytext); return(C_VOLATILE);   }
<C>(?-i:"while")        { WRITESTR2BUFCOMP(yytext); return(C_WHILE);      }
<C>(?-i:"else")         { WRITESTR2BUFCOMP(yytext); return(C_ELSE);       }
<C>(?-i:"if")           { WRITESTR2BUFCOMP(yytext); return(C_IF);         }
<C>(?-i:"SQLFailOverCallback")  { WRITESTR2BUFCOMP(yytext); return(C_FAILOVERCB); }
<C>(?-i:"APRE_BINARY")          { WRITESTR2BUFCOMP(yytext); return(C_APRE_BINARY); }
<C>(?-i:"APRE_BIT")             { WRITESTR2BUFCOMP(yytext); return(C_APRE_BIT); }
<C>(?-i:"APRE_BYTES")           { WRITESTR2BUFCOMP(yytext); return(C_APRE_BYTES); }
<C>(?-i:"APRE_VARBYTES")        { WRITESTR2BUFCOMP(yytext); return(C_APRE_VARBYTES); }
<C>(?-i:"APRE_NIBBLE")          { WRITESTR2BUFCOMP(yytext); return(C_APRE_NIBBLE); }
<C>(?-i:"APRE_INTEGER")         { WRITESTR2BUFCOMP(yytext); return(C_APRE_INTEGER); }
<C>(?-i:"APRE_NUMERIC")         { WRITESTR2BUFCOMP(yytext); return(C_APRE_NUMERIC); }
<C>(?-i:"APRE_BLOB_LOCATOR")    { WRITESTR2BUFCOMP(yytext); return(C_APRE_BLOB_LOCATOR); }
<C>(?-i:"APRE_CLOB_LOCATOR")    { WRITESTR2BUFCOMP(yytext); return(C_APRE_CLOB_LOCATOR); }
<C>(?-i:"APRE_BLOB")            { WRITESTR2BUFCOMP(yytext); return(C_APRE_BINARY); }
<C>(?-i:"APRE_CLOB")            { WRITESTR2BUFCOMP(yytext); return(C_APRE_CLOB); }
<C>(?-i:"APRE_BINARY2")         { WRITESTR2BUFCOMP(yytext); return(C_APRE_BINARY2); }  /* BUG-46418 */
<C>(?-i:"SES_BINARY")           { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_APRE_BINARY); }
<C>(?-i:"SES_BIT")              { WRITESTR2BUFCOMP(yytext); return(C_APRE_BIT); }
<C>(?-i:"SES_BYTES")            { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_APRE_BYTES); }
<C>(?-i:"SES_VARBYTES")          {
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_APRE_VARBYTES); }
<C>(?-i:"SES_NIBBLE")           { 
    /* BUG-35518 Shared pointer should be supported in APRE */
    if ( gUlpParseInfo.mIsSharedPtr == ID_TRUE )
    {
        WRITESTR2BUFCOMP ( (SChar *)"/* " );
    }
    WRITESTR2BUFCOMP(yytext); return(C_APRE_NIBBLE); }
<C>(?-i:"SES_INTEGER")          { WRITESTR2BUFCOMP(yytext); return(C_APRE_INTEGER); }
<C>(?-i:"SES_NUMERIC")          { WRITESTR2BUFCOMP(yytext); return(C_APRE_NUMERIC); }
<C>(?-i:"SES_BLOB_LOCATOR")     { WRITESTR2BUFCOMP(yytext); return(C_APRE_BLOB_LOCATOR); }
<C>(?-i:"SES_CLOB_LOCATOR")     { WRITESTR2BUFCOMP(yytext); return(C_APRE_CLOB_LOCATOR); }
<C>(?-i:"SES_BLOB")             { WRITESTR2BUFCOMP(yytext); return(C_APRE_BLOB); }
<C>(?-i:"SES_CLOB")             { WRITESTR2BUFCOMP(yytext); return(C_APRE_CLOB); }
<C>(?-i:"SQLLEN")               { WRITESTR2BUFCOMP(yytext); return(C_SQLLEN); }
<C>(?-i:"SQL_TIMESTAMP_STRUCT") { WRITESTR2BUFCOMP(yytext); return(C_SQL_TIMESTAMP_STRUCT); }
<C>(?-i:"SQL_TIME_STRUCT")      { WRITESTR2BUFCOMP(yytext); return(C_SQL_TIME_STRUCT); }
<C>(?-i:"SQL_DATE_STRUCT")      { WRITESTR2BUFCOMP(yytext); return(C_SQL_DATE_STRUCT); }
<C>(?-i:"SQL_NUMERIC_STRUCT")   { WRITESTR2BUFCOMP(yytext); return(C_SQL_NUMERIC_STRUCT); } /* BUG-45933 */
<C>(?-i:"SQLDA")                { WRITESTR2BUFCOMP(yytext); return(C_SQL_DA_STRUCT); }
<C>(CHARACTER|character)[ \t]+(SET|set)[ \t]+((IS[ \t]+)|(is[ \t]+))?(NCHAR_CS|nchar_cs) {
    return(C_NCHAR_CS);
}
<C>"..."                { WRITESTR2BUFCOMP(yytext); return(C_ELIPSIS);    }
<C>(?-i:{L}({L}|{D})*)  {
    ulpMacroNode *sMacroN;

    WRITESTR2BUFCOMP(yytext);

    // user defined type인지 검사한다. (typedef)
    gUlpParseInfo.mHostValInfo4Typedef = gUlpScopeT.ulpSLookupAll( yytext, gUlpCurDepth );

    if( ( gUlpParseInfo.mHostValInfo4Typedef != NULL ) &&
        // typedef struct 한뒤 나중에 struct 변수 선언하는 경우 고려.
        /* BUG-27875 : 구조체안의 typedef type인식못함. */
        ( gUlpParseInfo.mSkipTypedef != ID_TRUE ) )
    {

        if( gUlpParseInfo.mHostValInfo4Typedef->mIsTypedef == ID_TRUE )
        {
            return(C_TYPE_NAME);
        }
    }

    /* BUG-28068 : #define 매크로이름 확장안되는 문제 */
    switch ( gUlpProgOption.mOptParseInfo )
    {
        case PARSE_FULL :
        case PARSE_PARTIAL :
            if ( (sMacroN = gUlpMacroT.ulpMLookup( yytext )) != NULL )
            {
                if ( sMacroN -> mIsFunc == ID_TRUE )
                {
                    /* BUG-28118 : system 헤더파일들도 파싱돼야함.     *
                     * 3th. problem : 매크로 함수가 확장되지 않는 문제. */
                    // macro 함수일경우에는 id다음의 (...) token을 소모해줘야한다.
                    (void) ulpCOMPSkipMacroFunc();
                }

                // initilize
                idlOS::memset( gUlpCOMPMacroExpState[gUlpCOMPMacroExpIndex].mExpandText,
                               0,
                               MAX_MACRO_DEFINE_CONTENT_LEN);
                // copy
                idlOS::snprintf( gUlpCOMPMacroExpState[gUlpCOMPMacroExpIndex].mExpandText,
                                 MAX_MACRO_DEFINE_CONTENT_LEN,
                                 "%s",
                                 sMacroN->mText);

                // 현재 buffer 정보 구조체 저장. (YY_CURRENT_BUFFER)
                gUlpCOMPMacroExpState[ gUlpCOMPMacroExpIndex ].mBufState = YY_CURRENT_BUFFER;
                // 새로 처리될 buffer 정보 구조체로 switch. (return value of yy_scan_buffer)
                // The last two bytes of mText must be ASCII NUL.
                yy_switch_to_buffer(
                            yy_scan_buffer(
                               gUlpCOMPMacroExpState[gUlpCOMPMacroExpIndex].mExpandText,
                               idlOS::strlen(gUlpCOMPMacroExpState[gUlpCOMPMacroExpIndex].mExpandText)+2
                                          )
                                   );

                gUlpCOMPMacroExpIndex++;
            }
            else
            {
                return(C_IDENTIFIER);
            }
            break;
        case PARSE_NONE :
            return(C_IDENTIFIER);
            break;
    }
}
<C>(?-i:0[xX]{H}+{IS}?) { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT);   }
<C>(?-i:0{D}+{IS}?)     { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT);   }
<C>(?-i:{D}+{IS}?)      { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT);   }
<C>(?-i:{D}+{E}{FS}?)   { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT);   }
<C>(__attribute__\ *\(\(.*\)\)) { WRITESTR2BUFCOMP(yytext); return(C_ATTRIBUTE); }
<C>\'(\\.|[^\\']|\\\n)*\'       { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT);   }
<C>\"(\\.|[^\\"]|\\\n)*\"       { WRITESTR2BUFCOMP(yytext); return(C_STRING_LITERAL);   }
<C>(?-i:{D}*"."{D}+({E})?{FS}?) { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT); }
<C>(?-i:{D}+"."{D}*({E})?{FS}?) { WRITESTR2BUFCOMP(yytext); return(C_CONSTANT); }

<C>">>="                { WRITESTR2BUFCOMP(yytext); return(C_RIGHT_ASSIGN); }
<C>"<<="                { WRITESTR2BUFCOMP(yytext); return(C_LEFT_ASSIGN);  }
<C>"+="                 { WRITESTR2BUFCOMP(yytext); return(C_ADD_ASSIGN);   }
<C>"-="                 { WRITESTR2BUFCOMP(yytext); return(C_SUB_ASSIGN);   }
<C>"*="                 { WRITESTR2BUFCOMP(yytext); return(C_MUL_ASSIGN);   }
<C>"/="                 { WRITESTR2BUFCOMP(yytext); return(C_DIV_ASSIGN);   }
<C>"%="                 { WRITESTR2BUFCOMP(yytext); return(C_MOD_ASSIGN);   }
<C>"&="                 { WRITESTR2BUFCOMP(yytext); return(C_AND_ASSIGN);   }
<C>"^="                 { WRITESTR2BUFCOMP(yytext); return(C_XOR_ASSIGN);   }
<C>"|="                 { WRITESTR2BUFCOMP(yytext); return(C_OR_ASSIGN);    }
<C>"++"                 { WRITESTR2BUFCOMP(yytext); return(C_INC_OP);       }
<C>"--"                 { WRITESTR2BUFCOMP(yytext); return(C_DEC_OP);       }
<C>"->"                 { WRITESTR2BUFCOMP(yytext); return(C_PTR_OP);       }
<C>"&&"                 { WRITESTR2BUFCOMP(yytext); return(C_AND_OP);       }
<C>"=="                 { WRITESTR2BUFCOMP(yytext); return(C_EQ_OP);        }
<C>"!="                 { WRITESTR2BUFCOMP(yytext); return(C_NE_OP);        }
<C>">>"                 { WRITESTR2BUFCOMP(yytext); return(C_RIGHT_OP);     }
<C>"<<"                 { WRITESTR2BUFCOMP(yytext); return(C_LEFT_OP);      }
<C>"||"                 { WRITESTR2BUFCOMP(yytext); return(C_OR_OP);        }
<C>"<="                 { WRITESTR2BUFCOMP(yytext); return(C_LE_OP);        }
<C>">="                 { WRITESTR2BUFCOMP(yytext); return(C_GE_OP);        }
<C>","                  { WRITESTR2BUFCOMP(yytext); return(',');            }
<C>"="                  { WRITESTR2BUFCOMP(yytext); return('=');            }
<C>"["                  { WRITESTR2BUFCOMP(yytext); return('[');            }
<C>"]"                  { WRITESTR2BUFCOMP(yytext); return(']');            }
<C>"."                  { WRITESTR2BUFCOMP(yytext); return('.');            }
<C>"("                  { WRITESTR2BUFCOMP(yytext); return('(');            }
<C>")"                  { WRITESTR2BUFCOMP(yytext); return(')');            }
<C>"!"                  { WRITESTR2BUFCOMP(yytext); return('!');            }
<C>"-"                  { WRITESTR2BUFCOMP(yytext); return('-');            }
<C>"+"                  { WRITESTR2BUFCOMP(yytext); return('+');            }
<C>"*"                  { WRITESTR2BUFCOMP(yytext); return('*');            }
<C>"/"                  { WRITESTR2BUFCOMP(yytext); return('/');            }
<C>"%"                  { WRITESTR2BUFCOMP(yytext); return('%');            }
<C>"<"                  { WRITESTR2BUFCOMP(yytext); return('<');            }
<C>">"                  { WRITESTR2BUFCOMP(yytext); return('>');            }
<C>"&"                  { WRITESTR2BUFCOMP(yytext); return('&');            }
<C>"~"                  { WRITESTR2BUFCOMP(yytext); return('~');            }
<C>"^"                  { WRITESTR2BUFCOMP(yytext); return('^');            }
<C>"|"                  { WRITESTR2BUFCOMP(yytext); return('|');            }
<C>"?"                  { WRITESTR2BUFCOMP(yytext); return('?');            }
<C>"{"                  {
    WRITESTR2BUFCOMP(yytext);
    /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
     * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
    // struct 선언 중이라면 scope를 늘릴필요 없다.
    if( gUlpParseInfo.mStructDeclDepth == 0 )
    {
        // block을 만나면 current depth를 1증가.
        gUlpCurDepth++;
        /* BUG-44675 {} 중첩이 많으면 apre가 죽음  */
        if (gUlpCurDepth > MAX_SCOPE_DEPTH)
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_Invalid_Scope_Depth_Error,
                             gUlpCurDepth);
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }
    return('{');
}
<C>"}"                  {
    WRITESTR2BUFCOMP(yytext);

    //for -debug symbol option
    if( gUlpProgOption.mDebugSymbol == ID_TRUE )
    {
        gUlpStructT.ulpPrintStructT();
        gUlpScopeT.ulpPrintAllSymT();
    }

    // struct 선언 block이 아니면, scope table에서 해당 symbol table 제거
    // struct table에서 해당 scope의 node들도 제거함.
    /* BUG-28118 : system 헤더파일들도 파싱돼야함.                       *
     * 6th. problem : Nested structure 정의중 scope를 잘못 계산하는 문제 */
    // struct 선언 중이라면 scope를 줄일필요 없다.
    if( gUlpParseInfo.mStructDeclDepth == 0 )
    {
        /* BUG-44675 {} 중첩이 많으면 apre가 죽음  */
        if (gUlpCurDepth < 0)
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_Syntax_Error);
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

        gUlpScopeT.ulpSDelScope( gUlpCurDepth );
        gUlpStructT.ulpStructDelScope( gUlpCurDepth );
        // current depth 감소.
        gUlpCurDepth--;
    }

    // depth를 보고 whenever 구조체를 reset.
    gUlpCodeGen.ulpGenResetWhenever( gUlpCurDepth );

    return('}');
}
<C>";"                  { WRITESTR2BUFCOMP(yytext); return(';');            }
<C>":"                  { WRITESTR2BUFCOMP(yytext); return(':');            }
<C>"\n"|"\n\r"          { WRITESTR2BUFCOMP(yytext); }
<C>.                    { WRITESTR2BUFCOMP(yytext); }


<EMSQL,EJ,EP,EN>ADD               { WRITESTR2QUERYCOMP( yytext ); return TR_ADD;               }
<EMSQL,EJ,EP,EN>AFTER             { WRITESTR2QUERYCOMP( yytext ); return TR_AFTER;             }
<EMSQL,EJ,EP,EN>AGER              { WRITESTR2QUERYCOMP( yytext ); return TR_AGER;              }
<EMSQL,EJ,EP,EN>ALL               { WRITESTR2QUERYCOMP( yytext ); return TR_ALL;               }
<EMSQL,EJ,EP,EN>ALTER             { WRITESTR2QUERYCOMP( yytext ); return TR_ALTER;             }
<EMSQL,EJ,EP,EN>AND               { WRITESTR2QUERYCOMP( yytext ); return TR_AND;               }
<EMSQL,EJ,EP,EN>ANY               { WRITESTR2QUERYCOMP( yytext ); return TR_ANY;               }
<EMSQL,EJ,EP,EN>ARCHIVE           { WRITESTR2QUERYCOMP( yytext ); return TR_ARCHIVE;           }
<EMSQL,EJ,EP,EN>ARCHIVELOG        { WRITESTR2QUERYCOMP( yytext ); return TR_ARCHIVELOG;        }
<EMSQL,EJ,EP,EN>AS                { WRITESTR2QUERYCOMP( yytext ); return TR_AS;                }
<EMSQL,EJ,EP,EN>ASC               { WRITESTR2QUERYCOMP( yytext ); return TR_ASC;               }
<EMSQL,EJ,EP,EN>BACKUP            { WRITESTR2QUERYCOMP( yytext ); return TR_BACKUP;            }
<EMSQL,EJ,EP,EN>BEFORE            { WRITESTR2QUERYCOMP( yytext ); return TR_BEFORE;            }
<EMSQL,EJ,EP,EN>BEGIN             { WRITESTR2QUERYCOMP( yytext ); return TR_BEGIN;             }
<EMSQL,EJ,EP,EN>BY                { WRITESTR2QUERYCOMP( yytext ); return TR_BY;                }
<EMSQL,EJ,EP,EN>BIND              { WRITESTR2QUERYCOMP( yytext ); return TR_BIND;              } 
<EMSQL,EJ,EP,EN>CASCADE           { WRITESTR2QUERYCOMP( yytext ); return TR_CASCADE;           }
<EMSQL,EJ,EP,EN>CASE              { WRITESTR2QUERYCOMP( yytext ); return TR_CASE;              }
<EMSQL,EJ,EP,EN>CAST              { WRITESTR2QUERYCOMP( yytext ); return TR_CAST;              }
<EMSQL,EJ,EP,EN>CHECK[ \t\r\n]*"(" { WRITESTR2QUERYCOMP( yytext ); return TR_CHECK_OPENING_PARENTHESIS; }
<EMSQL,EJ,EP,EN>CLOSE             { WRITESTR2QUERYCOMP( yytext ); return TR_CLOSE;             }
<EMSQL,EJ,EP,EN>COALESCE          { WRITESTR2QUERYCOMP( yytext ); return TR_COALESCE;          } 
<EMSQL,EJ,EP,EN>COLUMN            { WRITESTR2QUERYCOMP( yytext ); return TR_COLUMN;            }
<EMSQL,EJ,EP,EN>COMMENT           { WRITESTR2QUERYCOMP( yytext ); return TR_COMMENT;           }
<EMSQL,EJ,EP,EN>COMPILE           { WRITESTR2QUERYCOMP( yytext ); return TR_COMPILE;           }
<EMSQL,EJ,EP,EN>COMMIT            { WRITESTR2QUERYCOMP( yytext ); return TR_COMMIT;            }
<EMSQL,EJ,EP,EN>CONNECT           { WRITESTR2QUERYCOMP( yytext ); return TR_CONNECT;           }
<EMSQL,EJ,EP,EN>CONSTRAINT        { WRITESTR2QUERYCOMP( yytext ); return TR_CONSTRAINT;        }
<EMSQL,EJ,EP,EN>CONSTRAINTS       { WRITESTR2QUERYCOMP( yytext ); return TR_CONSTRAINTS;       }
<EMSQL,EJ,EP,EN>CONTINUE          { WRITESTR2QUERYCOMP( yytext ); return TR_CONTINUE;          }
<EMSQL,EJ,EP,EN>CREATE            { WRITESTR2QUERYCOMP( yytext ); return TR_CREATE;            }
<EMSQL,EJ,EP,EN>CURSOR            { WRITESTR2QUERYCOMP( yytext ); return TR_CURSOR;            }
<EMSQL,EJ,EP,EN>CYCLE             { WRITESTR2QUERYCOMP( yytext ); return TR_CYCLE;             }
<EMSQL,EJ,EP,EN>DECLARE           { WRITESTR2QUERYCOMP( yytext ); return TR_DECLARE;           }
<EMSQL,EJ,EP,EN>DEFAULT           { WRITESTR2QUERYCOMP( yytext ); return TR_DEFAULT;           }
<EMSQL,EJ,EP,EN>DELETE            { WRITESTR2QUERYCOMP( yytext ); return TR_DELETE;            }
<EMSQL,EJ,EP,EN>DEQUEUE           { WRITESTR2QUERYCOMP( yytext ); return TR_DEQUEUE;           }
<EMSQL,EJ,EP,EN>DESC              { WRITESTR2QUERYCOMP( yytext ); return TR_DESC;              }
<EMSQL,EJ,EP,EN>DISABLE           { WRITESTR2QUERYCOMP( yytext ); return TR_DISABLE;           }
<EMSQL,EJ,EP,EN>DIRECTORY         { WRITESTR2QUERYCOMP( yytext ); return TR_DIRECTORY;         }
<EMSQL,EJ,EP,EN>DISCONNECT        { WRITESTR2QUERYCOMP( yytext ); return TR_DISCONNECT;        }
<EMSQL,EJ,EP,EN>DISTINCT          { WRITESTR2QUERYCOMP( yytext ); return TR_DISTINCT;          }
<EMSQL,EJ,EP,EN>DESCRIBE          { WRITESTR2QUERYCOMP( yytext ); return TR_DESCRIBE;          }
<EMSQL,EJ,EP,EN>DESCRIPTOR        { WRITESTR2QUERYCOMP( yytext ); return TR_DESCRIPTOR;        }
<EMSQL,EJ,EP,EN>DROP              { WRITESTR2QUERYCOMP( yytext ); return TR_DROP;              }
<EMSQL,EJ,EP,EN>EACH              { WRITESTR2QUERYCOMP( yytext ); return TR_EACH;              }
<EMSQL,EJ,EP,EN>ELSE              { WRITESTR2QUERYCOMP( yytext ); return TR_ELSE;              }
<EMSQL,EJ,EP,EN>ELSEIF            { WRITESTR2QUERYCOMP( yytext ); return TR_ELSEIF;            }
<EMSQL,EJ,EP,EN>ENABLE            { WRITESTR2QUERYCOMP( yytext ); return TR_ENABLE;            }
<EMSQL,EJ,EP,EN>END               { WRITESTR2QUERYCOMP( yytext ); return TR_END;               }
<EMSQL,EJ,EP,EN>ENQUEUE           { WRITESTR2QUERYCOMP( yytext ); return TR_ENQUEUE;           }
<EMSQL,EJ,EP,EN>ESCAPE            { WRITESTR2QUERYCOMP( yytext ); return TR_ESCAPE;            }
<EMSQL,EJ,EP,EN>EXCEPTION         { WRITESTR2QUERYCOMP( yytext ); return TR_EXCEPTION;         }
<EMSQL,EJ,EP,EN>EXEC              { WRITESTR2QUERYCOMP( yytext ); return TR_EXEC;              }
<EMSQL,EJ,EP,EN>EXECUTE           { WRITESTR2QUERYCOMP( yytext ); return TR_EXECUTE;           }
<EMSQL,EJ,EP,EN>EXIT              { WRITESTR2QUERYCOMP( yytext ); return TR_EXIT;              }
<EMSQL,EJ,EP,EN>FAIL_OVER_CALLBACK { WRITESTR2QUERYCOMP( yytext ); return TR_FAILOVERCB;       }
<EMSQL,EJ,EP,EN>FALSE             { WRITESTR2QUERYCOMP( yytext ); return TR_FALSE;             }
<EMSQL,EJ,EP,EN>FETCH             { WRITESTR2QUERYCOMP( yytext ); return TR_FETCH;             }
<EMSQL,EJ,EP,EN>FIFO              { WRITESTR2QUERYCOMP( yytext ); return TR_FIFO;              }
<EMSQL,EJ,EP,EN>FLUSH             { WRITESTR2QUERYCOMP( yytext ); return TR_FLUSH;             }
<EMSQL,EJ,EP,EN>FOR               { WRITESTR2QUERYCOMP( yytext ); return TR_FOR;               }
<EMSQL,EJ,EP,EN>FOREIGN           { WRITESTR2QUERYCOMP( yytext ); return TR_FOREIGN;           }
<EMSQL,EJ,EP,EN>FROM              { WRITESTR2QUERYCOMP( yytext ); return TR_FROM;              }
<EMSQL,EJ,EP,EN>FULL              { WRITESTR2QUERYCOMP( yytext ); return TR_FULL;              }
<EMSQL,EJ,EP,EN>FUNCTION          { WRITESTR2QUERYCOMP( yytext ); return TR_FUNCTION;          }
<EMSQL,EJ,EP,EN>GOTO              { WRITESTR2QUERYCOMP( yytext ); return TR_GOTO;              }
<EMSQL,EJ,EP,EN>GRANT             { WRITESTR2QUERYCOMP( yytext ); return TR_GRANT;             }
<EMSQL,EJ,EP,EN>GROUP             { WRITESTR2QUERYCOMP( yytext ); return TR_GROUP;             }
<EMSQL,EJ,EP,EN>HAVING            { WRITESTR2QUERYCOMP( yytext ); return TR_HAVING;            }
<EMSQL,EJ,EP,EN>IF                { WRITESTR2QUERYCOMP( yytext ); return TR_IF;                }
<EMSQL,EJ,EP,EN>IN                { WRITESTR2QUERYCOMP( yytext ); return TR_IN;                }
<EMSQL,EJ,EP,EN>IN/[ \t\r\n]*"("  { WRITESTR2QUERYCOMP( yytext ); return TR_IN_BF_LPAREN;      }
<EMSQL,EJ,EP,EN>INNER             { WRITESTR2QUERYCOMP( yytext ); return TR_INNER;             }
<EMSQL,EJ,EP,EN>INSERT            { WRITESTR2QUERYCOMP( yytext ); return TR_INSERT;            }
<EMSQL,EJ,EP,EN>INTERSECT         { WRITESTR2QUERYCOMP( yytext ); return TR_INTERSECT;         }
<EMSQL,EJ,EP,EN>INTO              { WRITESTR2QUERYCOMP( yytext ); return TR_INTO;              }
<EMSQL,EJ,EP,EN>IS                { WRITESTR2QUERYCOMP( yytext ); return TR_IS;                }
<EMSQL,EJ,EP,EN>ISOLATION         { WRITESTR2QUERYCOMP( yytext ); return TR_ISOLATION;         }
<EMSQL,EJ,EP,EN>JOIN              { WRITESTR2QUERYCOMP( yytext ); return TR_JOIN;              }
<EMSQL,EJ,EP,EN>KEY               { WRITESTR2QUERYCOMP( yytext ); return TR_KEY;               }
<EMSQL,EJ,EP,EN>LEFT              { WRITESTR2QUERYCOMP( yytext ); return TR_LEFT;              }
<EMSQL,EJ,EP,EN>LESS              { WRITESTR2QUERYCOMP( yytext ); return TR_LESS;              } 
<EMSQL,EJ,EP,EN>LEVEL             { WRITESTR2QUERYCOMP( yytext ); return TR_LEVEL;             }
<EMSQL,EJ,EP,EN>LIFO              { WRITESTR2QUERYCOMP( yytext ); return TR_LIFO;              }
<EMSQL,EJ,EP,EN>LIKE              { WRITESTR2QUERYCOMP( yytext ); return TR_LIKE;              }
<EMSQL,EJ,EP,EN>LIMIT             { WRITESTR2QUERYCOMP( yytext ); return TR_LIMIT;             }
<EMSQL,EJ,EP,EN>LOCAL             { WRITESTR2QUERYCOMP( yytext ); return TR_LOCAL;             } 
<EMSQL,EJ,EP,EN>LOGANCHOR         { WRITESTR2QUERYCOMP( yytext ); return TR_LOGANCHOR;         }
<EMSQL,EJ,EP,EN>LOOP              { WRITESTR2QUERYCOMP( yytext ); return TR_LOOP;              }
<EMSQL,EJ,EP,EN>MERGE             { WRITESTR2QUERYCOMP( yytext ); return TR_MERGE;             } 
<EMSQL,EJ,EP,EN>MOVE              { WRITESTR2QUERYCOMP( yytext ); return TR_MOVE;              }
<EMSQL,EJ,EP,EN>MOVEMENT          { WRITESTR2QUERYCOMP( yytext ); return TR_MOVEMENT;          } 
<EMSQL,EJ,EP,EN>NEW               { WRITESTR2QUERYCOMP( yytext ); return TR_NEW;               }
<EMSQL,EJ,EP,EN>NOARCHIVELOG      { WRITESTR2QUERYCOMP( yytext ); return TR_NOARCHIVELOG;      }
<EMSQL,EJ,EP,EN>NOCYCLE           { WRITESTR2QUERYCOMP( yytext ); return TR_NOCYCLE;           }
<EMSQL,EJ,EP,EN>NOT               { WRITESTR2QUERYCOMP( yytext ); return TR_NOT;               }
<EMSQL,EJ,EP,EN>NULL              { WRITESTR2QUERYCOMP( yytext ); return TR_NULL;              }
<EMSQL,EJ,EP,EN>OF                { WRITESTR2QUERYCOMP( yytext ); return TR_OF;                }
<EMSQL,EJ,EP,EN>OFF               { WRITESTR2QUERYCOMP( yytext ); return TR_OFF;               }
<EMSQL,EJ,EP,EN>OLD               { WRITESTR2QUERYCOMP( yytext ); return TR_OLD;               }
<EMSQL,EJ,EP,EN>ON                { WRITESTR2QUERYCOMP( yytext ); return TR_ON;                }
<EMSQL,EJ,EP,EN>OPEN              { WRITESTR2QUERYCOMP( yytext ); return TR_OPEN;              }
<EMSQL,EJ,EP,EN>OR                { WRITESTR2QUERYCOMP( yytext ); return TR_OR;                }
<EMSQL,EJ,EP,EN>ORDER             { WRITESTR2QUERYCOMP( yytext ); return TR_ORDER;             }
<EMSQL,EJ,EP,EN>OUT               { WRITESTR2QUERYCOMP( yytext ); return TR_OUT;               }
<EMSQL,EJ,EP,EN>OUTER             { WRITESTR2QUERYCOMP( yytext ); return TR_OUTER;             }
<EMSQL,EJ,EP,EN>OVER              { WRITESTR2QUERYCOMP( yytext ); return TR_OVER;              }
<EMSQL,EJ,EP,EN>PARTITION         { WRITESTR2QUERYCOMP( yytext ); return TR_PARTITION;         }
<EMSQL,EJ,EP,EN>PARTITIONS        { WRITESTR2QUERYCOMP( yytext ); return TR_PARTITIONS;        }
<EMSQL,EJ,EP,EN>PRIMARY           { WRITESTR2QUERYCOMP( yytext ); return TR_PRIMARY;           }
<EMSQL,EJ,EP,EN>PRIOR             { WRITESTR2QUERYCOMP( yytext ); return TR_PRIOR;             }
<EMSQL,EJ,EP,EN>PRIVILEGES        { WRITESTR2QUERYCOMP( yytext ); return TR_PRIVILEGES;        }
<EMSQL,EJ,EP,EN>PROCEDURE         { WRITESTR2QUERYCOMP( yytext ); return TR_PROCEDURE;         }
<EMSQL,EJ,EP,EN>PUBLIC            { WRITESTR2QUERYCOMP( yytext ); return TR_PUBLIC;            }
<EMSQL,EJ,EP,EN>QUEUE             { WRITESTR2QUERYCOMP( yytext ); return TR_QUEUE;             }
<EMSQL,EJ,EP,EN>READ              { WRITESTR2QUERYCOMP( yytext ); return TR_READ;              }
<EMSQL,EJ,EP,EN>REBUILD           { WRITESTR2QUERYCOMP( yytext ); return TR_REBUILD;           }
<EMSQL,EJ,EP,EN>RECOVER           { WRITESTR2QUERYCOMP( yytext ); return TR_RECOVER;           }
<EMSQL,EJ,EP,EN>REFERENCES        { WRITESTR2QUERYCOMP( yytext ); return TR_REFERENCES;        }
<EMSQL,EJ,EP,EN>REFERENCING       { WRITESTR2QUERYCOMP( yytext ); return TR_REFERENCING;       }
<EMSQL,EJ,EP,EN>REGISTER          { WRITESTR2QUERYCOMP( yytext ); return TR_REGISTER;          }
<EMSQL,EJ,EP,EN>RESTRICT          { WRITESTR2QUERYCOMP( yytext ); return TR_RESTRICT;          }
<EMSQL,EJ,EP,EN>RETURN            { WRITESTR2QUERYCOMP( yytext ); return TR_RETURN;            }
<EMSQL,EJ,EP,EN>REVOKE            { WRITESTR2QUERYCOMP( yytext ); return TR_REVOKE;            }
<EMSQL,EJ,EP,EN>RIGHT             { WRITESTR2QUERYCOMP( yytext ); return TR_RIGHT;             }
<EMSQL,EJ,EP,EN>ROLLBACK          { WRITESTR2QUERYCOMP( yytext ); return TR_ROLLBACK;          }
<EMSQL,EJ,EP,EN>ROW               { WRITESTR2QUERYCOMP( yytext ); return TR_ROW;               }
<EMSQL,EJ,EP,EN>SAVEPOINT         { WRITESTR2QUERYCOMP( yytext ); return TR_SAVEPOINT;         }
<EMSQL,EJ,EP,EN>SELECT            { WRITESTR2QUERYCOMP( yytext ); return TR_SELECT;            }
<EMSQL,EJ,EP,EN>SEQUENCE          { WRITESTR2QUERYCOMP( yytext ); return TR_SEQUENCE;          }
<EMSQL,EJ,EP,EN>SESSION           { WRITESTR2QUERYCOMP( yytext ); return TR_SESSION;           }
<EMSQL,EJ,EP,EN>SET               { WRITESTR2QUERYCOMP( yytext ); return TR_SET;               }
<EMSQL,EJ,EP,EN>SOME              { WRITESTR2QUERYCOMP( yytext ); return TR_SOME;              }
<EMSQL,EJ,EP,EN>START             { WRITESTR2QUERYCOMP( yytext ); return TR_START;             }
<EMSQL,EJ,EP,EN>STATEMENT         { WRITESTR2QUERYCOMP( yytext ); return TR_STATEMENT;         }
<EMSQL,EJ,EP,EN>TABLE             { WRITESTR2QUERYCOMP( yytext ); return TR_TABLE;             }
<EMSQL,EJ,EP,EN>TEMPORARY         { WRITESTR2QUERYCOMP( yytext ); return TR_TEMPORARY;         }
<EMSQL,EJ,EP,EN>THAN              { WRITESTR2QUERYCOMP( yytext ); return TR_THAN;              }
<EMSQL,EJ,EP,EN>THEN              { WRITESTR2QUERYCOMP( yytext ); return TR_THEN;              }
<EMSQL,EJ,EP,EN>TO                { WRITESTR2QUERYCOMP( yytext ); return TR_TO;                }
<EMSQL,EJ,EP,EN>TRUE              { WRITESTR2QUERYCOMP( yytext ); return TR_TRUE;              }
<EMSQL,EJ,EP,EN>TRIGGER           { WRITESTR2QUERYCOMP( yytext ); return TR_TRIGGER;           }
<EMSQL,EJ,EP,EN>TYPE              { WRITESTR2QUERYCOMP( yytext ); return TR_TYPE;              }
<EMSQL,EJ,EP,EN>TYPESET           { WRITESTR2QUERYCOMP( yytext ); return TR_TYPESET;           }
<EMSQL,EJ,EP,EN>UNION             { WRITESTR2QUERYCOMP( yytext ); return TR_UNION;             }
<EMSQL,EJ,EP,EN>UNIQUE            { WRITESTR2QUERYCOMP( yytext ); return TR_UNIQUE;            }
<EMSQL,EJ,EP,EN>UNREGISTER        { WRITESTR2QUERYCOMP( yytext ); return TR_UNREGISTER;        }
<EMSQL,EJ,EP,EN>UNTIL             { WRITESTR2QUERYCOMP( yytext ); return TR_UNTIL;             }
<EMSQL,EJ,EP,EN>UPDATE            { WRITESTR2QUERYCOMP( yytext ); return TR_UPDATE;            }
<EMSQL,EJ,EP,EN>USER              { WRITESTR2QUERYCOMP( yytext ); return TR_USER;              }
<EMSQL,EJ,EP,EN>USING             { WRITESTR2QUERYCOMP( yytext ); return TR_USING;             }
<EMSQL,EJ,EP,EN>VALUES            { WRITESTR2QUERYCOMP( yytext ); return TR_VALUES;            }
<EMSQL,EJ,EP,EN>VARIABLE          { WRITESTR2QUERYCOMP( yytext ); return TR_VARIABLE;          }
<EMSQL,EJ,EP,EN>VARIABLE_LARGE    { WRITESTR2QUERYCOMP( yytext ); return TR_VARIABLE_LARGE;    }           /* BUG-43840 */
<EMSQL,EJ,EP,EN>VARIABLES         { WRITESTR2QUERYCOMP( yytext ); return TR_VARIABLES;         }
<EMSQL,EJ,EP,EN>VIEW              { WRITESTR2QUERYCOMP( yytext ); return TR_VIEW;              }
<EMSQL,EJ,EP,EN>VOLATILE          { WRITESTR2QUERYCOMP( yytext ); return TR_VOLATILE;          }
<EMSQL,EJ,EP,EN>WHEN              { WRITESTR2QUERYCOMP( yytext ); return TR_WHEN;              }
<EMSQL,EJ,EP,EN>WHERE             { WRITESTR2QUERYCOMP( yytext ); return TR_WHERE;             }
<EMSQL,EJ,EP,EN>WHILE             { WRITESTR2QUERYCOMP( yytext ); return TR_WHILE;             }
<EMSQL,EJ,EP,EN>WITH              { WRITESTR2QUERYCOMP( yytext ); return TR_WITH;              }
<EMSQL,EJ,EP,EN>WORK              { WRITESTR2QUERYCOMP( yytext ); return TR_WORK;              }
<EMSQL,EJ,EP,EN>WRITE             { WRITESTR2QUERYCOMP( yytext ); return TR_WRITE;             }

<EMSQL,EJ,EP,EN>BETWEEN           { WRITESTR2QUERYCOMP( yytext ); return TK_BETWEEN;           }
<EMSQL,EJ,EP,EN>EXISTS            { WRITESTR2QUERYCOMP( yytext ); return TK_EXISTS;            }

<EMSQL,EJ,EP,EN>ACCESS            { WRITESTR2QUERYCOMP( yytext ); return TO_ACCESS;            }
<EMSQL,EJ,EP,EN>CONSTANT          { WRITESTR2QUERYCOMP( yytext ); return TO_CONSTANT;          }
<EMSQL,EJ,EP,EN>IDENTIFIED        { WRITESTR2QUERYCOMP( yytext ); return TO_IDENTIFIED;        }
<EMSQL,EJ,EP,EN>INDEX             { WRITESTR2QUERYCOMP( yytext ); return TO_INDEX;             }
<EMSQL,EJ,EP,EN>MINUS             { WRITESTR2QUERYCOMP( yytext ); return TO_MINUS;             }
<EMSQL,EJ,EP,EN>MODE              { WRITESTR2QUERYCOMP( yytext ); return TO_MODE;              }
<EMSQL,EJ,EP,EN>OTHERS            { WRITESTR2QUERYCOMP( yytext ); return TO_OTHERS;            }
<EMSQL,EJ,EP,EN>RAISE             { WRITESTR2QUERYCOMP( yytext ); return TO_RAISE;             }
<EMSQL,EJ,EP,EN>RENAME            { WRITESTR2QUERYCOMP( yytext ); return TO_RENAME;            }
<EMSQL,EJ,EP,EN>REPLACE           { WRITESTR2QUERYCOMP( yytext ); return TO_REPLACE;           }
<EMSQL,EJ,EP,EN>ROWTYPE           { WRITESTR2QUERYCOMP( yytext ); return TO_ROWTYPE;           }
<EMSQL,EJ,EP,EN>SEGMENT           { WRITESTR2QUERYCOMP( yytext ); return TO_SEGMENT;           }
<EMSQL,EJ,EP,EN>WAIT              { WRITESTR2QUERYCOMP( yytext ); return TO_WAIT;              }
<EMSQL,EJ,EP,EN>PIVOT             { WRITESTR2QUERYCOMP( yytext ); return TO_PIVOT;             }
<EMSQL,EJ,EP,EN>UNPIVOT           { WRITESTR2QUERYCOMP( yytext ); return TO_UNPIVOT;           }
<EMSQL,EJ,EP,EN>MATERIALIZED      { WRITESTR2QUERYCOMP( yytext ); return TO_MATERIALIZED;      }
<EMSQL,EJ,EP,EN>CONNECT_BY_ROOT   { WRITESTR2QUERYCOMP( yytext ); return TO_CONNECT_BY_ROOT;   }
<EMSQL,EJ,EP,EN>CONNECT[ \t\r\n]+BY[ \t\r\n]+NOCYCLE { WRITESTR2QUERYCOMP( yytext ); return TO_CONNECT_BY_NOCYCLE; }
<EMSQL,EJ,EP,EN>NULLS             { WRITESTR2QUERYCOMP( yytext ); return TO_NULLS; }
<EMSQL,EJ,EP,EN>VC2COLL           { WRITESTR2QUERYCOMP( yytext ); return TO_VC2COLL; }

<EMSQL,EJ,EP,EN>DATABASE          { WRITESTR2QUERYCOMP( yytext ); return TR_DATABASE;          }
<EMSQL,EJ,EP,EN>ELSIF             { WRITESTR2QUERYCOMP( yytext ); return TA_ELSIF;             }
<EMSQL,EJ,EP,EN>EXTENTSIZE        { WRITESTR2QUERYCOMP( yytext ); return TA_EXTENTSIZE;        }
<EMSQL,EJ,EP,EN>FIXED             { WRITESTR2QUERYCOMP( yytext ); return TA_FIXED;             }
<EMSQL,EJ,EP,EN>LOCK              { WRITESTR2QUERYCOMP( yytext ); return TA_LOCK;              }
<EMSQL,EJ,EP,EN>MAXROWS           { WRITESTR2QUERYCOMP( yytext ); return TA_MAXROWS;           }
<EMSQL,EJ,EP,EN>ONLINE            { WRITESTR2QUERYCOMP( yytext ); return TA_ONLINE;            }
<EMSQL,EJ,EP,EN>OFFLINE           { WRITESTR2QUERYCOMP( yytext ); return TA_OFFLINE;           }
<EMSQL,EJ,EP,EN>REPLICATION       { WRITESTR2QUERYCOMP( yytext ); return TA_REPLICATION;       }
<EMSQL,EJ,EP,EN>REVERSE           { WRITESTR2QUERYCOMP( yytext ); return TA_REVERSE;           }
<EMSQL,EJ,EP,EN>ROWCOUNT          { WRITESTR2QUERYCOMP( yytext ); return TA_ROWCOUNT;          }
<EMSQL,EJ,EP,EN>SQLCODE           { WRITESTR2QUERYCOMP( yytext ); return TA_SQLCODE;           }
<EMSQL,EJ,EP,EN>SQLERRM           { WRITESTR2QUERYCOMP( yytext ); return TA_SQLERRM;           }
<EMSQL,EJ,EP,EN>STEP              { WRITESTR2QUERYCOMP( yytext ); return TA_STEP;              }
<EMSQL,EJ,EP,EN>TABLESPACE        { WRITESTR2QUERYCOMP( yytext ); return TA_TABLESPACE;        }
<EMSQL,EJ,EP,EN>TRUNCATE          { WRITESTR2QUERYCOMP( yytext ); return TA_TRUNCATE;          }
<EMSQL,EJ,EP,EN>LINKER            { WRITESTR2QUERYCOMP( yytext ); return TA_LINKER;            }           /* BUG-37100 */
<EMSQL,EJ,EP,EN>REMOTE_TABLE      { WRITESTR2QUERYCOMP( yytext ); return TA_REMOTE_TABLE;      }           /* BUG-37100 */
<EMSQL,EJ,EP,EN>SHARD             { WRITESTR2QUERYCOMP( yytext ); return TA_SHARD;             }           /* PROJ-2638 */
<EMSQL,EJ,EP,EN>SEC               { WRITESTR2QUERYCOMP( yytext ); return TA_SEC;               }           /* BUG-45502 */
<EMSQL,EJ,EP,EN>MSEC              { WRITESTR2QUERYCOMP( yytext ); return TA_MSEC;              }
<EMSQL,EJ,EP,EN>USEC              { WRITESTR2QUERYCOMP( yytext ); return TA_USEC;              }
<EMSQL,EJ,EP,EN>SECOND            { WRITESTR2QUERYCOMP( yytext ); return TA_SECOND;            }
<EMSQL,EJ,EP,EN>MILLISECOND       { WRITESTR2QUERYCOMP( yytext ); return TA_MILLISECOND;       }
<EMSQL,EJ,EP,EN>MICROSECOND       { WRITESTR2QUERYCOMP( yytext ); return TA_MICROSECOND;       }
<EMSQL,EJ,EP,EN>SYNONYM           { WRITESTR2QUERYCOMP( yytext ); return TR_SYNONYM;           }
<EMSQL,EJ,EP,EN>PARALLEL          { WRITESTR2QUERYCOMP( yytext ); return TR_PARALLEL;          }
<EMSQL,EJ,EP,EN>NOPARALLEL        { WRITESTR2QUERYCOMP( yytext ); return TR_NOPARALLEL;        }
<EMSQL,EJ,EP,EN>LOB               { WRITESTR2QUERYCOMP( yytext ); return TR_LOB;               }
<EMSQL,EJ,EP,EN>STORE             { WRITESTR2QUERYCOMP( yytext ); return TR_STORE;             }
<EMSQL,EJ,EP,EN>SPLIT             { WRITESTR2QUERYCOMP( yytext ); return TR_SPLIT;             }
<EMSQL,EJ,EP,EN>END-EXEC          { WRITESTR2QUERYCOMP( yytext ); return TR_ENDEXEC;           }
<EMSQL,EJ,EP,EN>PRECEDING         { WRITESTR2QUERYCOMP( yytext ); return TR_PRECEDING;         }
<EMSQL,EJ,EP,EN>FOLLOWING         { WRITESTR2QUERYCOMP( yytext ); return TR_FOLLOWING;         }
<EMSQL,EJ,EP,EN>CURRENT[ \t\r\n]+ROW { WRITESTR2QUERYCOMP( yytext ); return TR_CURRENT_ROW;    }
<EMSQL,EJ,EP,EN>LINK              { WRITESTR2QUERYCOMP( yytext ); return TR_LINK;              }           /* BUG-37100 */
<EMSQL,EJ,EP,EN>ROLE              { WRITESTR2QUERYCOMP( yytext ); return TR_ROLE;              }           /* PROJ-1812 ROLE */
<EMSQL,EJ,EP,EN>WITHIN            { WRITESTR2QUERYCOMP( yytext ); return TR_WITHIN;            }           /* PROJ-2527 WITHIN GROUP AGGR */
<EMSQL,EJ,EP,EN>PURGE             { WRITESTR2QUERYCOMP( yytext ); return TO_PURGE;             }           /*PROJ-2441*/
<EMSQL,EJ,EP,EN>FLASHBACK         { WRITESTR2QUERYCOMP( yytext ); return TO_FLASHBACK;         }           /*PROJ-2441*/
<EMSQL,EJ,EP,EN>DISJOIN           { WRITESTR2QUERYCOMP( yytext ); return TA_DISJOIN;           }           /* PROJ-1810 Partition Exchange */
<EMSQL,EJ,EP,EN>CONJOIN           { WRITESTR2QUERYCOMP( yytext ); return TA_CONJOIN;           }           /* BUG-42468 JOIN TO CONJOIN */
<EMSQL,EJ,EP,EN>KEEP              { WRITESTR2QUERYCOMP( yytext ); return TO_KEEP;              }           /* PROJ-2528 */
<EMSQL,EJ,EP,EN>LOGGING           { WRITESTR2QUERYCOMP( yytext ); return TR_LOGGING;           }                /* BUG-46157 */
<EMSQL,EJ,EP,EN>ANALYSIS[ \t\r\n]+PROPAGATION { WRITESTR2QUERYCOMP( yytext ); return TA_ANALYSIS_PROPAGATION; } /* BUG-46157 */

<EMSQL,EJ,EP,EN>ABSOLUTE                 {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ABSOLUTE); }
<EMSQL,EJ,EP,EN>ALLOCATE                 {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ALLOCATE); }
<EMSQL,EJ,EP,EN>ASENSITIVE               {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ASENSITIVE);}
<EMSQL,EJ,EP,EN>AT                       {  WRITESTR2QUERYCOMP( yytext ); return(TR_AT); }
<EMSQL,EJ,EP,EN>AUTOCOMMIT               {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_AUTOCOMMIT); }
<EMSQL,EJ,EP,EN>BATCH                    {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_BATCH); }
<EMSQL,EJ,EP,EN>BLOB_FILE                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_BLOB_FILE); }
<EMSQL,EJ,EP,EN>BREAK                    {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_BREAK); }
<EMSQL,EJ,EP,EN>CLOB_FILE                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_CLOB_FILE); }
<EMSQL,EJ,EP,EN>CUBE                     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_CUBE);  }
<EMSQL,EJ,EP,EN>DEALLOCATE               {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_DEALLOCATE); }
<EMSQL,EJ,EP,EN>DESCRIPTOR               {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_DESCRIPTOR); }
<EMSQL,EJ,EP,EN>DO                       {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_DO); }
<EMSQL,EJ,EP,EN>FIRST                    {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_FIRST); }
<EMSQL,EJ,EP,EN>FOUND                    {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_FOUND); }
<EMSQL,EJ,EP,EN>FREE                     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_FREE);  }
<EMSQL,EJ,EP,EN>HOLD                     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_HOLD); }
<EMSQL,EJ,EP,EN>IMMEDIATE                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_IMMEDIATE); }
<EMSQL,EJ,EP,EN>INDICATOR                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_INDICATOR); }
<EMSQL,EJ,EP,EN>INSENSITIVE              {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_INSENSITIVE); }
<EMSQL,EJ,EP,EN>LAST                     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_LAST);  }
<EMSQL,EJ,EP,EN>CURRENT                  {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_CURRENT);  }
<EMSQL,EJ,EP,EN>NEXT                     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_NEXT);  }
<EMSQL,EJ,EP,EN>ONERR                    {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ONERR); }
<EMSQL,EJ,EP,EN>ONLY                     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ONLY);  }
<EMSQL,EJ,EP,EN>OPTION                   {  WRITESTR2QUERYCOMP( yytext ); return(APRE_V_OPTION);  }
<EMSQL,EJ,EP,EN>PREPARE                  {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_PREPARE);   }
<EMSQL,EJ,EP,EN>RELATIVE                 {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_RELATIVE);  }
<EMSQL,EJ,EP,EN>RELEASE                  {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_RELEASE);   }
<EMSQL,EJ,EP,EN>ROLLUP                   {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ROLLUP);    }
<EMSQL,EJ,EP,EN>SCROLL                   {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_SCROLL);    }
<EMSQL,EJ,EP,EN>SENSITIVE                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_SENSITIVE); }
<EMSQL,EJ,EP,EN>SQLERROR                 {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_SQLERROR);  }
<EMSQL,EJ,EP,EN>THREADS                  {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_THREADS);   }
<EMSQL,EJ,EP,EN>WHENEVER                 {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_WHENEVER);  }
<EMSQL,EJ,EP,EN>GROUPING[ \t\r\n]+SETS   {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_GROUPING_SETS); }
<EMSQL,EJ,EP,EN>POINTER                  {  WRITESTR2QUERYCOMP( yytext ); return(TR_POINTER); }
<EMSQL,EJ,EP,EN>WITH[ \t\r\n]+ROLLUP     {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_WITH_ROLLUP); }

<EMSQL,EJ,EP,EN>GET                      {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_GET); }
<EMSQL,EJ,EP,EN>DIAGNOSTICS              {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_DIAGNOSTICS); }
<EMSQL,EJ,EP,EN>CONDITION                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_CONDITION); }
<EMSQL,EJ,EP,EN>NUMBER                   {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_NUMBER); }
<EMSQL,EJ,EP,EN>ROW_COUNT                {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ROW_COUNT); }
<EMSQL,EJ,EP,EN>RETURNED_SQLCODE         {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_RETURNED_SQLCODE); }
<EMSQL,EJ,EP,EN>RETURNED_SQLSTATE        {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_RETURNED_SQLSTATE); }
<EMSQL,EJ,EP,EN>MESSAGE_TEXT             {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_MESSAGE_TEXT); }
<EMSQL,EJ,EP,EN>ROW_NUMBER               {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_ROW_NUMBER); }
<EMSQL,EJ,EP,EN>COLUMN_NUMBER            {  WRITESTR2QUERYCOMP( yytext ); return(SES_V_COLUMN_NUMBER); }

<EMSQL,EN>([_A-Z$]|({NEG_TRAIL_CH}))([_0-9A-Z$@]|({NEG_TRAIL_CH}))*  {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_IDENTIFIER;
}
<EMSQL,EP>([_A-Z$]|({POS_TRAIL_CH}))([_0-9A-Z$@]|({POS_TRAIL_CH}))*   {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_IDENTIFIER;
}
<EMSQL,EJ>([_A-Z$]|({SJIS_ALL}))([_0-9A-Z$@]|({SJIS_ALL}))*   {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_IDENTIFIER;
}


<EMSQL,EJ,EP,EN>"\""[^'"]+"\""  {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_DQUOTE_LITERAL;
}

<EMSQL>[_A-Z][_0-9A-Z]*"'"("''"*[^']*)*"'"                {
     WRITESTR2QUERYCOMP( yytext ); return SES_V_TYPED_LITERAL;
}
<EN>([_A-Z]|({NEG_TRAIL_CH}))([_0-9A-Z]|({NEG_TRAIL_CH}))*"'"("''"*[^']*)*"'" {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_TYPED_LITERAL;
}
<EP>([_A-Z]|({POS_TRAIL_CH}))([_0-9A-Z]|({POS_TRAIL_CH}))*"'"("''"*[^']*)*"'" {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_TYPED_LITERAL;
}
<EJ>([_A-Z]|({SJIS_ALL}))([_0-9A-Z]|({SJIS_ALL}))*"'"("''"*[^']*)*"'" {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_TYPED_LITERAL;
}


<EMSQL,EJ,EP,EN>"'"("''"*[^']*)*"'"                       {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_LITERAL;
}

<EMSQL,EJ,EP,EN>":"[ \t\n]*[_A-Z][_0-9A-Z]*(([.]|("->"))[_A-Z][_0-9A-Z]*)?("["[_0-9A-Z"*+/%!~()=&^.|"]*"]"([.][_A-Z][_0-9A-Z]*)?)?  {
    /* BUG-28250 ::문자와 호스트변수명 사이에 공백문자 허용. */
    (void) ulpCOMPEraseHostValueSpaces( yytext );
    WRITESTR2QUERYCOMP( yytext );
    return SES_V_HOSTVARIABLE;
}
<EMSQL,EJ,EP,EN>[0-9]+                           {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_INTEGER;
}

<EMSQL,EJ,EP,EN>[0-9]+("."[0-9]*)?([Ee][+-]?[0-9]+)?  |
       "."[0-9]+([Ee][+-]?[0-9]+)?                  {
    WRITESTR2QUERYCOMP( yytext ); return SES_V_NUMERIC;
}

<EMSQL,EJ,EP,EN>"/*+"[^*/]*"*/"                       {
    WRITESTR2QUERYCOMP( yytext ); return TX_HINTS;
}
<EMSQL,EJ,EP,EN>("--"|"//").*                         { }
<EMSQL,EJ,EP,EN>"\\\n"                                { }


<EMSQL,EJ,EP,EN>"||"  { WRITESTR2QUERYCOMP( yytext ); return TS_CONCATENATION_SIGN;  }
<EMSQL,EJ,EP,EN>".."  { WRITESTR2QUERYCOMP( yytext ); return TS_DOUBLE_PERIOD;       }

<EMSQL,EJ,EP,EN>"!"   { WRITESTR2QUERYCOMP( yytext ); return TS_EXCLAMATION_POINT;   }
<EMSQL,EJ,EP,EN>"%"   { WRITESTR2QUERYCOMP( yytext ); return TS_PERCENT_SIGN;        }
<EMSQL,EJ,EP,EN>"("   { WRITESTR2QUERYCOMP( yytext ); return TS_OPENING_PARENTHESIS; }
<EMSQL,EJ,EP,EN>")"   { WRITESTR2QUERYCOMP( yytext ); return TS_CLOSING_PARENTHESIS; }
<EMSQL,EJ,EP,EN>"["   { WRITESTR2QUERYCOMP( yytext ); return TS_OPENING_BRACKET;     }
<EMSQL,EJ,EP,EN>"]"   { WRITESTR2QUERYCOMP( yytext ); return TS_CLOSING_BRACKET;     }
<EMSQL,EJ,EP,EN>"*"   { WRITESTR2QUERYCOMP( yytext ); return TS_ASTERISK;            }
<EMSQL,EJ,EP,EN>"+"   { WRITESTR2QUERYCOMP( yytext ); return TS_PLUS_SIGN;           }
<EMSQL,EJ,EP,EN>","   { WRITESTR2QUERYCOMP( yytext ); return TS_COMMA;               }
<EMSQL,EJ,EP,EN>"-"   { WRITESTR2QUERYCOMP( yytext ); return TS_MINUS_SIGN;          }
<EMSQL,EJ,EP,EN>"."   { WRITESTR2QUERYCOMP( yytext ); return TS_PERIOD;              }
<EMSQL,EJ,EP,EN>"/"   { WRITESTR2QUERYCOMP( yytext ); return TS_SLASH;               }
<EMSQL,EJ,EP,EN>":"   { WRITESTR2QUERYCOMP( yytext ); return TS_COLON;               }
<EMSQL,EJ,EP,EN>";"   { WRITESTR2QUERYCOMP( yytext ); return TS_SEMICOLON;           }
<EMSQL,EJ,EP,EN>"<"   { WRITESTR2QUERYCOMP( yytext ); return TS_LESS_THAN_SIGN;      }
<EMSQL,EJ,EP,EN>"="   { WRITESTR2QUERYCOMP( yytext ); return TS_EQUAL_SIGN;          }
<EMSQL,EJ,EP,EN>">"   { WRITESTR2QUERYCOMP( yytext ); return TS_GREATER_THAN_SIGN;   }
<EMSQL,EJ,EP,EN>"?"   { WRITESTR2QUERYCOMP( yytext ); return TS_QUESTION_MARK;       }
<EMSQL,EJ,EP,EN>"(+)" { WRITESTR2QUERYCOMP( yytext ); return TS_OUTER_JOIN_OPERATOR; } // PROJ-1653 Outer Join Operator (+)
<EMSQL,EJ,EP,EN>"@"   { WRITESTR2QUERYCOMP( yytext ); return TS_AT_SIGN;             } /* BUG-37100 */

<EMSQL,EJ,EP,EN>"/*"                                          {
    ulpCOMPCommentC( ID_TRUE );
}
<NONE,C,PARTIAL>"/*"                                {
    if ( gUlpParseInfo.mVarcharDecl != ID_TRUE )
    {
        WRITESTR2BUFCOMP(yytext);
    }
    else
    {
        // varchar 선언중 주석이 오면 #로 바꿔준다.
        WRITESTR2BUFCOMP((SChar *)"/#");
    }

    ulpCOMPCommentC( ID_FALSE );

}
<NONE,C,PARTIAL>"//"                               {
    WRITESTR2BUFCOMP(yytext);
    ulpCOMPCommentCplus();
}

<EMSQL,EJ,EP,EN>"\n"|"\n\r"      {
    // 공백을 하나 추가해줘야 한다. 아니면 query string이 아래 라인과 붙어 에러발생하는 경우가 있다
    WRITESTR2QUERYCOMP((SChar *)" ");
    WRITESTR2QUERYCOMP( yytext );
}

<EMSQL,EJ,EP,EN>[ \t\r]+         { WRITESTR2QUERYCOMP( yytext );  }

<EMSQL,EJ,EP,EN>'[^'\n]*$        {
    ulpErrorMgr  sErrorMgr;
    ulpSetErrorCode( &sErrorMgr,
                     ulpERR_ABORT_COMP_Unterminated_String_Error );
    gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    COMPerror( ulpGetErrorMSG(&sErrorMgr) );
}
<EMSQL,EJ,EP,EN>.                 { return EM_ERROR;          }
<NONE,PARTIAL>"\n"|"\n\r"         {
    WRITESTR2BUFCOMP(yytext);
}
<NONE,PARTIAL>.                   { WRITESTR2BUFCOMP(yytext); }
<C,NONE,PARTIAL><<EOF>>           {
    /* BUG-28068 : #define 매크로이름 확장안되는 문제 */
    if( gUlpCOMPMacroExpIndex > 0 )
    {
        gUlpCOMPMacroExpIndex--;
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( gUlpCOMPMacroExpState[ gUlpCOMPMacroExpIndex ].mBufState );
    }
    else
    {
        if ( gUlpCurDepth != 0 )
        {
            ulpErrorMgr  sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_Brace_Count_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

        if ( gUlpCOMPifstackMgr[gUlpCOMPifstackInd]->ulpIfgetIndex() > -1 )
        {
            ulpErrorMgr   sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_NO_ENDIF_Macro_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

        return(END_OF_FILE);
    }
}
<MACRO_IFNDEF_APRE><<EOF>> {
    ulpErrorMgr   sErrorMgr;
    ulpSetErrorCode( &sErrorMgr,
                      ulpERR_ABORT_COMP_NO_ENDIF_Macro_Error );
    gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    COMPerror( ulpGetErrorMSG(&sErrorMgr) );
}
%%


IDE_RC ulpCOMPInitialize( SChar *aFileName )
{
/***********************************************************************
 *
 * Description :
 *      COMPLexer 에서 읽어들일 파일을 세팅해주는 함수.
 *
 ***********************************************************************/
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
    if ( gUlpCOMPIncludeIndex > 0 )
    {
        yy_switch_to_buffer ( yy_create_buffer( yyin, YY_BUF_SIZE) );
        // 새file 열때마다 line정보 초기화.
        yylineno = 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_FILE_OPEN);
    {
        ulpErrorMgr   sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                          ulpERR_ABORT_FILE_OPEN_ERROR,
                          aFileName, errno );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    IDE_EXCEPTION (ERR_FILE_OPEN_INVALID_SIZE);
    {
        ulpErrorMgr   sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                          ulpERR_ABORT_COMP_Invaild_File_Size,
                          aFileName );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void ulpCOMPFinalize( )
{
/***********************************************************************
 *
 * Description :
 *      Destructor
 *
 ***********************************************************************/
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
    if ( gUlpCOMPIncludeIndex > 0 )
    {
        gUlpCOMPIncludeIndex--;
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( gUlpCOMPIncludeStack[ gUlpCOMPIncludeIndex ].mCOMPIncludeStack );
        yylineno = gUlpCOMPIncludeStack[ gUlpCOMPIncludeIndex ].mCOMPLinenum;
    }
}

/* BUG-31626 : BOF occurs in the middle of parsing a long C style comment */
IDE_RC ulpCOMPCommentC( idBool aSkip )
{
/***********************************************************************
 *
 * Description :
 *      c-style comment 처리. comment의 마지막인 " * / " 가 올때까지 읽어들여 write 버퍼에 쓴다.
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
            if ( aSkip != ID_TRUE )
            {
                WRITESTR2BUFCOMP(sTmpStr);
            }
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
                if( gUlpParseInfo.mVarcharDecl == ID_TRUE )
                {   /* varchar 선언 사이에 주석이 있을경우 */
                    sTmpStr[sIndex-1] = '#';
                }
                sTmpStr[sIndex++] = sCh2;
                sTmpStr[sIndex]   = '\0';
                if ( aSkip != ID_TRUE )
                {
                    WRITESTR2BUFCOMP(sTmpStr);
                }
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
        if ( aSkip != ID_TRUE )
        {
            WRITESTR2BUFCOMP(sTmpStr);
        }

        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                          ulpERR_ABORT_COMP_C_Comment_No_Close_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void ulpCOMPCommentCplus()
{
/***********************************************************************
 *
 * Description :
 *      c++-style comment 처리.
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
    WRITESTR2BUFCOMP(sTmpStr);
}


/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
void ulpCOMPCommentCplus4IF()
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


IDE_RC ulpCOMPSkipMacro()
{
/***********************************************************************
 *
 * Description :
 *      파싱할 필요없는 macro구문을 스킵해주는 함수.
 *
 ***********************************************************************/
    SInt  sIndex;
    SChar sTmpStr[ MAX_SKIP_MACRO_LEN ];
    SChar sCh1;
    SChar sCh2;

    sIndex = 0;

    sIndex = idlOS::snprintf( sTmpStr, MAX_SKIP_MACRO_LEN, yytext );
    IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN, ERR_SKIP_MACRO_OVERFLOW );

SkipMacro:
    while (( (sCh1 = yyinput()) != '\\' ) &&
           ( sCh1 != '\n' && sCh1 != (SChar) EOF ))
    {
        IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
        sTmpStr[sIndex++] = sCh1;
    }

    if ( sCh1 == '\\' )
    {
        IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
        sTmpStr[sIndex++] = sCh1;
        while ( (sCh2 = yyinput()) != (SChar) EOF )
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
            unput(sCh2);
        }
        goto SkipMacro;
    }
    else
    {
        IDE_TEST_RAISE( sIndex >= MAX_SKIP_MACRO_LEN-1, ERR_SKIP_MACRO_OVERFLOW );
        sTmpStr[sIndex++] = sCh1;
    }

    sTmpStr[sIndex] = '\0';

    WRITESTR2BUFCOMP(sTmpStr);

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_SKIP_MACRO_OVERFLOW);
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Unknown_Macro_Overflow_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
void ulpCOMPEraseBN4MacroText( SChar *aTmpDefinedStr, idBool aIsIf )
{
/***********************************************************************
 *
 * Description :
 *      #if <expr>, #define ID <text> 구문 처리시 <expr>와 <text> 안에 포함되어 있는
 *      backspace newline 문자 처리와 주석처리를 하여  aTmpDefinedStr 버퍼에 폭사해주는 함수.
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
        switch ( (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput() )
        {
            // 처음 공백 제거
            case '\t':
            case ' ' :
                do
                {
                    (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput();
                } while( (sCh == ' ') || (sCh == '\t') );
                sBreak1    = ID_TRUE;
                break;
            // 처음에 그냥 newline이 온다고?
            case '\n':
                sBreak1    = ID_TRUE;
                break;
            // 처음 주석이 오는경우 처리.
            case '/':
                (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput();
                if ( sCh == '/')
                {
                    (aIsIf == ID_TRUE)? ulpCOMPCommentCplus4IF() : ulpCOMPCommentCplus();
                }
                else
                {
                    if( sCh == '*' )
                    {
                        (aIsIf == ID_TRUE)? ulpCOMPCommentC4IF() : ulpCOMPCommentC(ID_FALSE);
                    }
                    else
                    {
                        (aIsIf == ID_TRUE)? unput(sCh) : ulpCOMPYYunput(sCh);
                        sBreak1    = ID_TRUE;
                    }
                }
                break;
            default:
                sBreak1    = ID_TRUE;
                break;
        }
    }

    (aIsIf == ID_TRUE)? unput(sCh) : ulpCOMPYYunput(sCh);

    while ( ((aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput()) != (SChar) EOF )
    {
        /* BUG-30233 : #define 내용이 5K이상이면 apre segv발생함. */
        IDE_TEST_RAISE( sIndex >= MAX_MACRO_DEFINE_CONTENT_LEN-1,
                        ERR_DEFINE_MACRO_OVERFLOW );

        switch(sCh)
        {
            case '\\':
                (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput();
                if ( sCh == '\n' )
                {

                }
                else
                {
                    if( (sCh == ' ') || (sCh == '\t') )
                    {
                        sNumSpace++;
                        while ( ((aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput())
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
                        (aIsIf == ID_TRUE)? unput(sCh) : ulpCOMPYYunput(sCh);
                    }
                }
                break;

            case '\n':
                sBreak2 = ID_TRUE;
                aTmpDefinedStr[sIndex] = '\0';
                break;

            // 중간에 주석이 오는경우 처리.
            case '/':
                (aIsIf == ID_TRUE)? sCh = yyinput() : sCh = ulpCOMPYYinput();
                if ( sCh == '/')
                {
                    (aIsIf == ID_TRUE)? ulpCOMPCommentCplus4IF() : ulpCOMPCommentCplus();
                    sBreak2 = ID_TRUE;
                }
                else
                {
                    if( sCh == '*' )
                    {
                        (aIsIf == ID_TRUE)? ulpCOMPCommentC4IF() : ulpCOMPCommentC(ID_FALSE);
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
            (aIsIf == ID_TRUE)? unput(sCh) : ulpCOMPYYunput(sCh);
        }
    }

    return;

    /* BUG-30233 : #define 내용이 5K이상이면 apre segv발생함. */
    IDE_EXCEPTION (ERR_DEFINE_MACRO_OVERFLOW);
    {
        ulpErrorMgr  sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Define_Macro_Overflow_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }
    IDE_EXCEPTION_END;

    return;
}

/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
SChar ulpCOMPYYinput(void)
{
/***********************************************************************
 *
 * Description :
 *      yyinput() 하여 문자 하나를 읽어온후 file buffer에 쓴다.
 *
 ***********************************************************************/
    SChar sCh;

    sCh = yyinput();
    WRITECH2BUFCOMP(sCh);

    return sCh;
}

/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
void ulpCOMPYYunput( SChar aCh )
{
/***********************************************************************
 *
 * Description :
 *      unput() 하여 문자 하나를 내뱉은후 file buffer에서 제거한다.
 *
 ***********************************************************************/
    unput(aCh);
    WRITEUNCH2BUFCOMP();
}

/* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
 *             ulpComp 에서 재구축한다.                       */
IDE_RC ulpCOMPCommentC4IF()
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
            WRITECH2BUFCOMP('\n');
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
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void ulpCOMPRestoreCond()
{
/***********************************************************************
 *
 * Description :
 *      이전의 lexer 상태로 복귀해주는 함수.
 *
 ***********************************************************************/
    /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
     *             ulpComp 에서 재구축한다.                       */
    switch ( gUlpProgOption.mOptParseInfo )
    {
        case PARSE_PARTIAL :
            BEGIN(PARTIAL);
            break;
        case PARSE_NONE :
            BEGIN(NONE);
            break;
        case PARSE_FULL :
            BEGIN(C);
            break;
        default :
            break;
    }
}


void ulpCOMPWheneverFunc( SChar *aTmpStr )
{
/***********************************************************************
 *
 * Description :
 *      WHENEVER 구문 DO function_name 의 함수이름을 저장하는 함수.
 *
 ***********************************************************************/
    SChar sCh;
    idBool sDoBreak;
    // ( 열린 횟수
    UInt   sParenNum;
    UInt   sLen;
    UInt   sLen2;

    sDoBreak     = ID_FALSE;
    // 일단 ( 이 한번 열린 상태다. 따라서 1.
    sParenNum    = 1;
    sLen2 = sLen = idlOS::strlen( aTmpStr );

    for ( ; sLen < (MAX_EXPR_LEN - 1); sLen++ )
    {
        switch( sCh = yyinput() )
        {
            case ';':
                unput(sCh);
                sDoBreak = ID_TRUE;
                break;
            case ')':
                sParenNum--;
                if( sParenNum <= 0 )
                {
                    sDoBreak = ID_TRUE;
                }
                aTmpStr[sLen] = sCh;
                break;
            case '(':
                sParenNum++;
                aTmpStr[sLen] = sCh;
                break;
            case (SChar) EOF:
                unput(sCh);
                sDoBreak = ID_TRUE;
                break;
            default:
                aTmpStr[sLen] = sCh;
                break;
        }

        if( sDoBreak == ID_TRUE )
        {
            break;
        }
    }

    aTmpStr[++sLen] = '\0';

    gUlpCodeGen.ulpGenQueryString( aTmpStr + sLen2 );
}


void ulpCOMPSetHeaderName( void )
{
/***********************************************************************
 *
 * Description :
 *      EXEC SQL INCLUDE 의 header file 패스+이름을 저장하는 함수.
 *
 ***********************************************************************/

    SChar  sCh;
    SInt   sLen;
    idBool sDoBreak = ID_FALSE;

    while ( yyinput() != '(' ) { }

    for ( sLen = 0 ; sLen < MAX_FILE_PATH_LEN - 1 ; sLen++ )
    {
        switch( sCh = yyinput() )
        {
            case ')':
                sDoBreak = ID_TRUE;
                break;
            default:
                gUlpCOMPHeaderPath[sLen] = sCh;
                break;
        }

        if( sDoBreak == ID_TRUE )
        {
            break;
        }
    }

    gUlpCOMPHeaderPath[sLen] = '\0';
}

void ulpCOMPSaveBufferState( void )
{
/***********************************************************************
 *
 * Description :
 *      include 처리를 하기위한 현재 flex버퍼 상태를 저장하는 함수.
 *
 ***********************************************************************/

    if ( gUlpCOMPIncludeIndex >= MAX_HEADER_FILE_NUM )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_COMP_Include_Depth_Too_Large_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
        COMPerror( ulpGetErrorMSG(&sErrorMgr) );
    }

    gUlpCOMPIncludeStack[ gUlpCOMPIncludeIndex ].mCOMPIncludeStack = YY_CURRENT_BUFFER;
    gUlpCOMPIncludeStack[ gUlpCOMPIncludeIndex ].mCOMPLinenum = yylineno;
    gUlpCOMPIncludeIndex++;
}


/* BUG-28250 : :문자와 호스트변수명 사이에 공백문자 허용. */
void ulpCOMPEraseHostValueSpaces( SChar *aString )
{
/***********************************************************************
 *
 * Description :
 *      : <호스트변수이름> 에서 :와 이름사이의 공백을 제거해주는 함수이다.
 *
 ***********************************************************************/

    SInt i, j;
    SInt sLen;

    if( (aString != NULL) && (aString[0] == ':') )
    {
        sLen = idlOS::strlen( aString );

        // count spaces = i - 1
        for ( i = 1 ; i < sLen ; i++ )
        {
            if ( (aString[i] == ' ') ||
                 (aString[i] == '\t') ||
                 (aString[i] == '\n') )
            {
                // do nothing
            }
            else
            {
                break;
            }
        }
        // shift
        for ( j = i ; j < sLen ; j++ )
        {
            aString[j-i+1] = aString[j];
        }
        aString[j-i+1] = '\0';
    }
    else
    {
        // do nothing
    }
}


/* BUG-28118 : system 헤더파일들도 파싱돼야함.     *
 * 3th. problem : 매크로 함수가 확장되지 않는 문제. */
void ulpCOMPSkipMacroFunc()
{
/***********************************************************************
 *
 * Description :
 *      : Macro 함수가 올경우 id 뒤의 (...) 토큰을 소모해주기 위한 함수.
 *
 ***********************************************************************/
    SChar sCh;
    idBool sDoBreak;
    idBool sIsQuote;
    idBool sIsDQuote;
    idBool sIsFirst;
    // ( 열린 횟수
    UInt   sParenNum;

    sIsFirst     = ID_TRUE;
    sDoBreak     = ID_FALSE;
    sIsQuote     = ID_FALSE;
    sIsDQuote    = ID_FALSE;
    sParenNum    = 0;

    while ( (sCh = yyinput()) != (SChar) EOF )
    {
        WRITECH2BUFCOMP(sCh);
        switch( sCh )
        {
            case ')':
                if ( sIsFirst == ID_TRUE )
                {
                    ulpErrorMgr  sErrorMgr;
                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_COMP_DEFINE_Unknown_Macro_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    COMPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
                if( (sIsQuote  != ID_TRUE) &&
                    (sIsDQuote != ID_TRUE) )
                {
                    sParenNum--;
                    if( sParenNum <= 0 )
                    {
                        sDoBreak = ID_TRUE;
                    }
                }
                break;
            case '(':
                if( sIsFirst == ID_TRUE )
                {
                    sIsFirst = ID_FALSE;
                }
                if( (sIsQuote  != ID_TRUE) &&
                    (sIsDQuote != ID_TRUE) )
                {
                    sParenNum++;
                }
                break;
            case '\'':
                if ( sIsFirst == ID_TRUE )
                {
                    ulpErrorMgr  sErrorMgr;
                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_COMP_DEFINE_Unknown_Macro_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    COMPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
                if( sIsDQuote == ID_FALSE )
                {
                    if( sIsQuote == ID_FALSE )
                    {
                        sIsQuote = ID_TRUE;
                    }
                    else
                    {
                        sIsQuote = ID_FALSE;
                    }
                }
                break;
            case '"':
                if ( sIsFirst == ID_TRUE )
                {
                    ulpErrorMgr  sErrorMgr;
                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_COMP_DEFINE_Unknown_Macro_Error );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    COMPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
                if( sIsQuote == ID_FALSE )
                {
                    if( sIsDQuote == ID_FALSE )
                    {
                        sIsDQuote = ID_TRUE;
                    }
                    else
                    {
                        sIsDQuote = ID_FALSE;
                    }
                }
                break;
            // spaces
            case ' ':
                break;
            case '\t':
                break;
            default:
                if ( sIsFirst == ID_TRUE )
                {
                    ulpErrorMgr  sErrorMgr;
                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_COMP_DEFINE_Unknown_Macro_Error,
                                     yytext
                                   );
                    gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    COMPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
                break;
        }

        if( sDoBreak == ID_TRUE )
        {
            break;
        }
    }

    if( sCh ==  EOF )
    {
        if( ( sIsQuote  == ID_TRUE ) ||
            ( sIsDQuote == ID_TRUE ) )
        {
            ulpErrorMgr  sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_DEFINE_FUNC_Unterminated_String_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
        else
        {
            ulpErrorMgr  sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_COMP_Paren_Count_Error );
            gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            COMPerror( ulpGetErrorMSG(&sErrorMgr) );
        }
    }
}


int COMPlex( YYSTYPE        *lvalp )
{
/***********************************************************************
 *
 * Description :
 *      parser에서 토큰을 읽을 때마다 호출되는 lexer함수. (=yylex)
 *
 ***********************************************************************/
    SInt   sRc;
    UInt   sMytextlen;
    SChar *sNlsUse;
    SChar  sDefaultNLS[] = "US7ASCII";
    ulpErrorMgr sErrorMgr;

    if( gUlpCOMPCharType4JIS == IDN_CF_UNKNOWN )
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
            gUlpCOMPCharType4JIS = IDN_CF_SJIS;
        }
        else if( ( idlOS::strncmp( sNlsUse, (SChar*)"GB231280", 8 ) == 0 ) ||
                 ( idlOS::strncmp( sNlsUse, (SChar*)"ZHS16CGB231280", 14) == 0) ||
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
            gUlpCOMPCharType4JIS = IDN_CF_POS_TRAIL;
        }
        else
        {
            gUlpCOMPCharType4JIS = IDN_CF_NEG_TRAIL;
        }
    }

    sRc = yylex();

    sMytextlen     = (UInt)idlOS::strlen(yytext);

    // array [ expr ] => expr을 mConstantExprStr 에 strcat 함.
    if ( gUlpParseInfo.mArrExpr == ID_TRUE )
    {
        idlOS::strcat(gUlpParseInfo.mConstantExprStr, lvalp->strval);
    }

    IDE_TEST_RAISE(gUlpMem -> alloc( sMytextlen+3, (void **) &(lvalp->strval) )
                   != IDE_SUCCESS, MAllocError);

    idlOS::memcpy(lvalp->strval, yytext, sMytextlen);
    (lvalp->strval)[sMytextlen] = '\0';
    fflush(stdout);

    return sRc;

    IDE_EXCEPTION(MAllocError);
    {
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Memory_Alloc_Error );
        gUlpCOMPErrCode = ulpGetErrorSTATE( &sErrorMgr );
    }
    IDE_EXCEPTION_END;

    COMPerror( ulpGetErrorMSG(&sErrorMgr) );

    return 0; // COMPerror()에서 강제 종료 되므로 unreachable
}


void COMPerror( const SChar* aErrMsg)
{
/***********************************************************************
 *
 * Description :
 *      COMP lexer parser에서 발생하는 모든 에러들을 처리해줌.
 *
 ***********************************************************************/
    idBool sSkip;
    sSkip = ID_FALSE;

    sSkip = (gUlpCOMPErrCode==NULL)?ID_FALSE:
            (idBool) ((idlOS::strcmp(gUlpCOMPErrCode,"101H0") == 0) ||
            (idlOS::strcmp(gUlpCOMPErrCode,"102H0") == 0));

    if( idlOS::strncmp(aErrMsg,"syntax error", 12) == 0 )
    {
        idlOS::printf("\n[ERR-%.4s : %s",
                      (gUlpCOMPErrCode!=NULL)?gUlpCOMPErrCode:"000H",
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
                      (gUlpCOMPErrCode!=NULL)?gUlpCOMPErrCode:"000H",
                      aErrMsg);
    }

    // file open error, file size error는 skip
    if( sSkip == ID_FALSE )
    {
        if( gDontPrint2file == ID_TRUE )
        {
            // in the case of #include file error
            idlOS::printf( "File  : %s\nLine  : %d\nOffset: %d-%d\nError_token:%s\n\n",
                           gUlpProgOption.mIncludeFileList[gUlpProgOption.mIncludeFileIndex].mFileName,
                           yylineno, gFirstcolumnC, gLastcolumnC,
                           yytext);
        }
        else
        {
            if ( gUlpCOMPHeaderPath[0] != '\0' )
            {
                // in the case of EXEC SQL INCLUDE file error
                idlOS::printf( "File  : %s\nLine  : %d\nOffset: %d-%d\nError_token:%s\n\n",
                               gUlpCOMPHeaderPath,
                               yylineno - gUlpCOMPHeaderLine,
                               gFirstcolumnC, gLastcolumnC,
                               yytext );
            }
            else
            {
                idlOS::printf( "File  : %s\nLine  : %d\nOffset: %d-%d\nError_token:%s\n\n",
                               gUlpProgOption.mInFile,
                               yylineno - gUlpCOMPSumHeaderLine,
                               gFirstcolumnC, gLastcolumnC,
                               yytext );
            }
        }
    }

    gUlpCOMPErrCode = NULL;

    idlOS::fflush(stdout);

    ulpFinalizeError();
}
