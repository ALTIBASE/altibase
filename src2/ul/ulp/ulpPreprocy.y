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


%pure_parser

%union
{
    char *strval;
}

%initial-action
{
    // parse option에 따라 초기 상태를 지정해줌.
    switch ( gUlpProgOption.mOptParseInfo )
    {
        case PARSE_NONE :
            gUlpPPStartCond = PP_ST_INIT_SKIP;
            break;
        case PARSE_PARTIAL :
        case PARSE_FULL :
            gUlpPPStartCond = PP_ST_MACRO;
            break;
        default :
            break;
    }
};


%{

#include <ulpPreproc.h>


#undef YY_READ_BUF_SIZE
#undef YY_BUF_SIZE
#define YY_READ_BUF_SIZE (16384)
#define YY_BUF_SIZE (YY_READ_BUF_SIZE * 2) /* size of default input buffer */

//============== global variables for PPparse ================//

// parser의 시작 상태를 지정하는 변수.
SInt            gUlpPPStartCond = PP_ST_NONE;
// parser가 이전 상태로 복귀하기위한 변수.
SInt            gUlpPPPrevCond  = PP_ST_NONE;

// PPIF parser에서 사용되는 버퍼 시작/끝 pointer
SChar *gUlpPPIFbufptr = NULL;
SChar *gUlpPPIFbuflim = NULL;

// 아래의 macro if 처리 class를 ulpPPLexer 클래스 안으로 넣고 싶지만,
// sun장비에서 initial 되지 않는 알수없는 문제가있어 밖으로 다뺌.
ulpPPifstackMgr *gUlpPPifstackMgr[MAX_HEADER_FILE_NUM];
// gUlpPPifstackMgr index
SInt             gUlpPPifstackInd = -1;

/* externs of PPLexer */
// include header 파싱중인지 여부.
extern idBool        gUlpPPisCInc;
extern idBool        gUlpPPisSQLInc;

/* externs of ulpMain.h */
extern ulpProgOption gUlpProgOption;
extern ulpCodeGen    gUlpCodeGen;
extern iduMemory    *gUlpMem;
// Macro table
extern ulpMacroTable gUlpMacroT;
// preprocessor parsing 중 발생한 에러에대한 코드정보.
extern SChar        *gUlpPPErrCode;

//============================================================//


//============ Function declarations for PPparse =============//

// Macro if 구문 처리를 위한 parse 함수
extern SInt PPIFparse ( void *aBuf, SInt *aRes );
extern int  PPlex  ( YYSTYPE *lvalp );
extern void PPerror( const SChar* aMsg );

extern void ulpFinalizeError(void);

//============================================================//

%}

/*** EmSQL tokens ***/
%token EM_OPTION_INC
%token EM_INCLUDE
%token EM_SEMI
%token EM_EQUAL
%token EM_COMMA
%token EM_LPAREN
%token EM_RPAREN
%token EM_LBRAC
%token EM_RBRAC
%token EM_DQUOTE
%token EM_PATHNAME
%token EM_FILENAME

/*** MACRO tokens ***/
%token M_INCLUDE
%token M_DEFINE
%token M_IFDEF
%token M_IFNDEF
%token M_UNDEF
%token M_ENDIF
%token M_IF
%token M_ELIF
%token M_ELSE
%token M_SKIP_MACRO
%token M_LBRAC
%token M_RBRAC
%token M_DQUOTE
%token M_FILENAME
%token M_DEFINED
%token M_NUMBER
%token M_CHARACTER
%token M_BACKSLASHNEWLINE
%token M_IDENTIFIER
%token M_FUNCTION
%token M_STRING_LITERAL
%token M_RIGHT_OP
%token M_LEFT_OP
%token M_OR_OP
%token M_LE_OP
%token M_GE_OP
%token M_LPAREN
%token M_RPAREN
%token M_AND_OP
%token M_EQ_OP
%token M_NE_OP
%token M_NEWLINE
%token M_CONSTANT
%%

preprocessor
    :
    | preprocessor Emsql_grammar
    {
        /* 이전 상태로 복귀 SKIP_NONE 상태 혹은 MACRO상태 에서 emsql구문을 파싱할 수 있기때문 */
        gUlpPPStartCond = gUlpPPPrevCond;
    }
    | preprocessor Macro_grammar
    {
        /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
         * 전이 된다. 나머지 경우는 MACRO상태로 전이됨. */
    }
    ;

Emsql_grammar
    : Emsql_include
    {
        /* EXEC SQL INCLUDE ... */
        SChar sStrtmp[MAX_FILE_PATH_LEN];

        // 내장될 헤더파일의 시작을 알린다. my girlfriend
        idlOS::snprintf( sStrtmp, MAX_FILE_PATH_LEN, "@$LOVELY.K.J.H$ (%s)\n",
                         gUlpProgOption.ulpGetIncList() );
        WRITESTR2BUFPP( sStrtmp );

        /* BUG-27683 : iostream 사용 제거 */
        // 0. flex 버퍼 상태 저장.
        ulpPPSaveBufferState();

        gUlpPPisSQLInc = ID_TRUE;

        // 1. doPPparse()를 재호출한다.
        doPPparse( gUlpProgOption.ulpGetIncList() );

        gUlpPPisSQLInc = ID_FALSE;

        // 내장될 헤더파일의 끝을 알린다.
        WRITESTR2BUFPP((SChar *)"#$LOVELY.K.J.H$\n");

        // 2. precompiler를 실행한 directory를 current path로 재setting
        idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
    }
    | Emsql_include_option
    {
        /* EXEC SQL OPTION( INCLUDE = ... ) */
        // 하위 노드에서 처리함.
    }
    ;

Emsql_include
    : EM_INCLUDE EM_LBRAC EM_FILENAME EM_RBRAC EM_SEMI
    {
        // Emsql_include 문법은 제일 뒤에 반드시 ';' 가 와야함.
        // check exist header file in include paths
        if ( gUlpProgOption.ulpLookupHeader( $<strval>3, ID_FALSE )
             == IDE_FAILURE )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_FILE_NOT_FOUND,
                             $<strval>3 );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

    }
    | EM_INCLUDE EM_DQUOTE EM_FILENAME EM_DQUOTE EM_SEMI
    {
        // check exist header file in include paths
        if ( gUlpProgOption.ulpLookupHeader( $<strval>3, ID_FALSE )
             == IDE_FAILURE )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_FILE_NOT_FOUND,
                             $<strval>3 );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

    }
    | EM_INCLUDE EM_FILENAME EM_SEMI
    {
        // check exist header file in include paths
        if ( gUlpProgOption.ulpLookupHeader( $<strval>2, ID_FALSE )
             == IDE_FAILURE )
        {
            ulpErrorMgr sErrorMgr;
            ulpSetErrorCode( &sErrorMgr,
                             ulpERR_ABORT_FILE_NOT_FOUND,
                             $<strval>2 );
            gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
            PPerror( ulpGetErrorMSG(&sErrorMgr) );
        }

    }
    ;

Emsql_include_option
    : EM_OPTION_INC EM_EQUAL Emsql_include_path_list EM_RPAREN EM_SEMI
    ;

Emsql_include_path_list
    : EM_FILENAME
    {

        SChar sPath[MAX_FILE_PATH_LEN];

        // path name 길이 에러 체크 필요.
        idlOS::strncpy( sPath, $<strval>1, MAX_FILE_PATH_LEN-1 );

        // include path가 추가되면 g_ProgOption.path에 저장한다.
        if ( sPath[0] == IDL_FILE_SEPARATOR )
        {
            // 절대경로인 경우
            idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                            sPath );
        }
        else
        {
            // 상대경로인 경우
              idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                              "%s%c%s", gUlpProgOption.mCurrentPath, IDL_FILE_SEPARATOR, sPath);
        }
        gUlpProgOption.mIncludePathCnt++;

    }
    | Emsql_include_path_list EM_COMMA EM_FILENAME
    {

        SChar sPath[MAX_FILE_PATH_LEN];

        idlOS::strncpy( sPath, $<strval>3, MAX_FILE_PATH_LEN-1 );

        // include path가 추가되면 g_ProgOption.path에 저장한다.
        if ( sPath[0] == IDL_FILE_SEPARATOR )
        {
            // 절대경로인 경우
            idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                            sPath );
        }
        else
        {
            // 상대경로인 경우
              idlOS::sprintf( gUlpProgOption.mIncludePathList[gUlpProgOption.mIncludePathCnt],
                              "%s%c%s", gUlpProgOption.mCurrentPath, IDL_FILE_SEPARATOR, sPath);
        }
        gUlpProgOption.mIncludePathCnt++;

    }
    ;


/**********************************************************
 *                                                        *
 *                      MACRO                             *
 *                                                        *
 **********************************************************/

/*** MACRO tokens ****
M_INCLUDE
M_DEFINE
M_IFDEF
M_UNDEF
M_IFNDEF
M_ENDIF
M_ELIF
M_DEFINED
M_INCLUDE_FILE
M_ELSE
M_IF
**********************/

Macro_grammar
        : Macro_include
        {
            gUlpPPStartCond = PP_ST_MACRO;
        }
        | Macro_define
        {
            gUlpPPStartCond = PP_ST_MACRO;
        }
        | Macro_undef
        {
            gUlpPPStartCond = PP_ST_MACRO;
        }
        | Macro_ifdef
        {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_ifndef
        {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_if
        {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_elif
        {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_else
        {
            /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
             * 전이 된다. */
        }
        | Macro_endif
        ;

Macro_include
        : M_INCLUDE M_LBRAC M_FILENAME M_RBRAC
        {
            /* #include <...> */

            // 1. check exist header file in include paths
            if ( gUlpProgOption.ulpLookupHeader( $<strval>3, ID_TRUE )
                 == IDE_FAILURE )
            {
                //do nothing
            }
            else
            {

                // 현재 #include 처리다.
                gUlpPPisCInc = ID_TRUE;

                /* BUG-27683 : iostream 사용 제거 */
                // 2. flex 버퍼 상태 저장.
                ulpPPSaveBufferState();
                // 3. doPPparse()를 재호출한다.
                doPPparse( gUlpProgOption.ulpGetIncList() );
                // 전에 #inlcude 처리중이었나? 확인함
                gUlpPPisCInc = gUlpProgOption.ulpIsHeaderCInclude();

                // 4. precompiler를 실행한 directory를 current path로 재setting
                idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
            }

        }
        | M_INCLUDE M_DQUOTE M_FILENAME M_DQUOTE
        {

            // 1. check exist header file in include paths
            if ( gUlpProgOption.ulpLookupHeader( $<strval>3, ID_TRUE )
                 == IDE_FAILURE )
            {
                // do nothing
            }
            else
            {

                // 현재 #include 처리다.
                gUlpPPisCInc = ID_TRUE;
                /* BUG-27683 : iostream 사용 제거 */
                // 2. flex 버퍼 상태 저장.
                ulpPPSaveBufferState();
                // 3. doPPparse()를 재호출한다.
                doPPparse( gUlpProgOption.ulpGetIncList() );
                // 전에 #inlcude 처리중이었나? 확인함
                gUlpPPisCInc = gUlpProgOption.ulpIsHeaderCInclude();

                // 4. precompiler를 실행한 directory를 current path로 재setting
                idlOS::strcpy( gUlpProgOption.mCurrentPath, gUlpProgOption.mStartPath );
            }

        }
        ;

Macro_define
        : M_DEFINE M_IDENTIFIER
        {
            SChar sTmpDEFtext[ MAX_MACRO_DEFINE_CONTENT_LEN ];

            ulpPPEraseBN4MacroText( sTmpDEFtext, ID_FALSE );

            //printf("ID=[%s], TEXT=[%s]\n",$<strval>2,sTmpDEFtext);
            if ( sTmpDEFtext[0] == '\0' )
            {
                // macro symbol table에 추가함.
                if( gUlpMacroT.ulpMDefine( $<strval>2, NULL, ID_FALSE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }
            else
            {
                // macro symbol table에 추가함.
                if( gUlpMacroT.ulpMDefine( $<strval>2, sTmpDEFtext, ID_FALSE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }

        }
        | M_DEFINE M_FUNCTION
        {
            // function macro의경우 인자 정보는 따로 저장되지 않는다.
            SChar sTmpDEFtext[ MAX_MACRO_DEFINE_CONTENT_LEN ];

            ulpPPEraseBN4MacroText( sTmpDEFtext, ID_FALSE );

            // #define A() {...} 이면, macro id는 A이다.
            if ( sTmpDEFtext[0] == '\0' )
            {
                // macro symbol table에 추가함.
                if ( gUlpMacroT.ulpMDefine( $<strval>2, NULL, ID_TRUE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }
            else
            {
                // macro symbol table에 추가함.
                if ( gUlpMacroT.ulpMDefine( $<strval>2, sTmpDEFtext, ID_TRUE ) == IDE_FAILURE )
                {
                    ulpErrorMgr sErrorMgr;

                    ulpSetErrorCode( &sErrorMgr,
                                     ulpERR_ABORT_Memory_Alloc_Error );
                    gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                    PPerror( ulpGetErrorMSG(&sErrorMgr) );
                }
            }

        }
        ;

Macro_undef
        : M_UNDEF M_IDENTIFIER
        {
            // $<strval>2 를 macro symbol table에서 삭제 한다.
            gUlpMacroT.ulpMUndef( $<strval>2 );
        }
        ;

Macro_if
        : M_IF
        {
            SInt  sVal;
            SChar sTmpExpBuf[MAX_MACRO_IF_EXPR_LEN];

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // #if expression 을 복사해온다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );

                    gUlpPPIFbufptr = sTmpExpBuf;
                    gUlpPPIFbuflim = sTmpExpBuf + idlOS::strlen(sTmpExpBuf);
                    if ( PPIFparse( sTmpExpBuf, &sVal ) != 0 )
                    {
                        //error 처리
                        ulpErrorMgr sErrorMgr;

                        ulpSetErrorCode( &sErrorMgr,
                                         ulpERR_ABORT_COMP_IF_Macro_Syntax_Error );
                        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                        PPerror( ulpGetErrorMSG(&sErrorMgr) );
                    }

                    //idlOS::printf("## PPIF result value=%d\n",sVal);
                    /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
                    * 전이 된다. */
                    if ( sVal != 0 )
                    {
                        // true
                        gUlpPPStartCond = PP_ST_MACRO;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_TRUE );
                    }
                    else
                    {
                        // false
                        gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_FALSE );
                    }
                    break;

                case PP_IF_FALSE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_elif
        : M_ELIF
        {
            SInt  sVal;
            SChar sTmpExpBuf[MAX_MACRO_IF_EXPR_LEN];

            // #elif 순서 문법 검사.
            if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfCheckGrammar( PP_ELIF )
                 == ID_FALSE )
            {
                //error 처리
                ulpErrorMgr sErrorMgr;

                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_COMP_ELIF_Macro_Sequence_Error );
                gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                PPerror( ulpGetErrorMSG(&sErrorMgr) );
            }

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // 단순히 token만 소모하는 역할이다. PPIFparse 호출하지 않는다.
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_IGNORE );
                    break;

                case PP_IF_FALSE :
                    ulpPPEraseBN4MacroText( sTmpExpBuf, ID_TRUE );

                    gUlpPPIFbufptr = sTmpExpBuf;
                    gUlpPPIFbuflim = sTmpExpBuf + idlOS::strlen(sTmpExpBuf);
                    //idlOS::printf("## start PPELIF parser text:[%s]\n",sTmpExpBuf);
                    if ( PPIFparse( sTmpExpBuf, &sVal ) != 0 )
                    {
                        //error 처리
                        ulpErrorMgr sErrorMgr;

                        ulpSetErrorCode( &sErrorMgr,
                                         ulpERR_ABORT_COMP_ELIF_Macro_Syntax_Error );
                        gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                        PPerror( ulpGetErrorMSG(&sErrorMgr) );
                    }
                    //idlOS::printf("## PPELIF result value=%d\n",sVal);

                    /* macro 조건문의 경우 참이면 MACRO상태, 거짓이면 MACRO_IFSKIP 상태로
                     * 전이 된다. */
                    if ( sVal != 0 )
                    {
                        gUlpPPStartCond = PP_ST_MACRO;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_TRUE );
                    }
                    else
                    {
                        gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELIF, PP_IF_FALSE );
                    }
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_ifdef
        : M_IFDEF M_IDENTIFIER
        {

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    /* BUG-28162 : SESC_DECLARE 부활  */
                    // pare가 full 이아니고, SESC_DECLARE가 오고, #include처리가 아닐때
                    // #ifdef SESC_DECLARE 예전처럼 처리됨.
                    if( (gUlpProgOption.mOptParseInfo != PARSE_FULL) &&
                        (idlOS::strcmp( $<strval>2, "SESC_DECLARE" ) == 0 ) &&
                        (gUlpPPisCInc != ID_TRUE) )
                    {
                        gUlpPPStartCond = PP_ST_MACRO;
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_SESC_DEC );
                        WRITESTR2BUFPP((SChar *)"EXEC SQL BEGIN DECLARE SECTION;");
                    }
                    else
                    {
                        // $<strval>2 를 macro symbol table에 존재하는지 확인한다.
                        if ( gUlpMacroT.ulpMLookup($<strval>2) != NULL )
                        {
                            // 존재한다
                            gUlpPPStartCond = PP_ST_MACRO;
                            // if stack manager 에 결과 정보 push
                            gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_TRUE );
                        }
                        else
                        {
                            // 존재안한다
                            gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                            // if stack manager 에 결과 정보 push
                            gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_FALSE );
                        }
                    }
                    break;

                case PP_IF_FALSE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFDEF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_ifndef
        : M_IFNDEF M_IDENTIFIER
        {
            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_IGNORE );
                    break;

                case PP_IF_TRUE :
                    // $<strval>2 를 macro symbol table에 존재하는지 확인한다.
                    if ( gUlpMacroT.ulpMLookup($<strval>2) != NULL )
                    {
                        // 존재한다
                        gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_FALSE );
                    }
                    else
                    {
                        // 존재안한다
                        gUlpPPStartCond = PP_ST_MACRO;
                        // if stack manager 에 결과 정보 push
                        gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_TRUE );
                    }
                    break;

                case PP_IF_FALSE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_IFNDEF, PP_IF_IGNORE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_else
        : M_ELSE
        {
            // #else 순서 문법 검사.
            if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfCheckGrammar( PP_ELSE )
                 == ID_FALSE )
            {
                // error 처리
                ulpErrorMgr sErrorMgr;

                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_COMP_ELSE_Macro_Sequence_Error );
                gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                PPerror( ulpGetErrorMSG(&sErrorMgr) );
            }

            switch( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpPrevIfStatus() )
            {
                case PP_IF_IGNORE :
                case PP_IF_TRUE :
                    gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELSE, PP_IF_IGNORE );
                    break;

                case PP_IF_FALSE :
                    gUlpPPStartCond = PP_ST_MACRO;
                    // if stack manager 에 결과 정보 push
                    gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpush( PP_ELSE, PP_IF_TRUE );
                    break;

                default:
                    IDE_ASSERT(0);
            }
        }
        ;

Macro_endif
        : M_ENDIF
        {
            idBool sSescDEC;

            // #endif 순서 문법 검사.
            if ( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfCheckGrammar( PP_ENDIF )
                 == ID_FALSE )
            {
                // error 처리
                ulpErrorMgr sErrorMgr;

                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_COMP_ENDIF_Macro_Sequence_Error );
                gUlpPPErrCode = ulpGetErrorSTATE( &sErrorMgr );
                PPerror( ulpGetErrorMSG(&sErrorMgr) );
            }

            /* BUG-28162 : SESC_DECLARE 부활  */
            // if stack 을 이전 조건문 까지 pop 한다.
            sSescDEC = gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfpop4endif();

            if( sSescDEC == ID_TRUE )
            {
                //#endif 를 아래 string으로 바꿔준다.
                WRITESTR2BUFPP((SChar *)"EXEC SQL END DECLARE SECTION;");
            }

            /* BUG-27961 : preprocessor의 중첩 #if처리시 #endif 다음소스 무조건 출력하는 버그  */
            if( gUlpPPifstackMgr[gUlpPPifstackInd]->ulpIfneedSkip4Endif() == ID_TRUE )
            {
                // 출력 하지마라.
                gUlpPPStartCond = PP_ST_MACRO_IF_SKIP;
            }
            else
            {
                // 출력 해라.
                gUlpPPStartCond = PP_ST_MACRO;
            }
        }
        ;


%%


int doPPparse( SChar *aFilename )
{
/***********************************************************************
 *
 * Description :
 *      The initial function of PPparser.
 *
 ***********************************************************************/
    int sRes;

    ulpPPInitialize( aFilename );

    gUlpPPifstackMgr[++gUlpPPifstackInd] = new ulpPPifstackMgr();

    sRes = PPparse();

    gUlpCodeGen.ulpGenWriteFile();

    delete gUlpPPifstackMgr[gUlpPPifstackInd];

    gUlpPPifstackInd--;

    ulpPPFinalize();

    return sRes;
}

