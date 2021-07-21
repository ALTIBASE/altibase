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
%option noyywrap
%option prefix = "PPIF"

%{

#include <idl.h>
#include <ide.h>
#include <ulpGenCode.h>
#include <ulpMacroTable.h>
#include <ulpMacro.h>
#include <ulpPreprocify.hpp>

#undef  YY_INPUT
#define YY_INPUT(aBuf, aRtn, aMax) { aRtn = PPIF_yyinput(aBuf, aMax); }

int  PPIFlex( void *aBuf );
void PPIFerror( void *aBuf, int *aRes, const SChar* aMsg);
int  PPIF_yyinput( char * aBuf, int   aMax );
IDE_RC ulpPPIFCommentC(void);
void   ulpPPIFCommentCplus(void);

extern SChar *gUlpPPIFbufptr;
extern SChar *gUlpPPIFbuflim;
extern ulpCodeGen    gUlpCodeGen;
/* extern of ulpMain.h */
extern ulpMacroTable  gUlpMacroT;

// defined 처리 중인지 알려줌.
idBool gUlpIsDefined = ID_FALSE;
// #if 조건문 안에 defined ID가 올경우 해당 id를 expansion 하기위해 필요
struct ulpPPIFBufState
{
    YY_BUFFER_STATE         mBufState;
    SChar                   mExpandText[MAX_MACRO_DEFINE_CONTENT_LEN];
} gUlpPPIFBufState[MAX_ID_EXPANSION];

// gUlpIDExpandStack의 index 값
SInt gUlpExpandIndex = 0;

%}

/*** for MACRO IF ***/
D           [0-9]
L           [a-zA-Z_$]
H           [a-fA-F0-9]
FS          (l|L)
IS          (u|U|l|L)*

%%


{D}+{FS}?           {
    PPIFlval.intval = atoi(yytext);
    return(CONSTANT);
}
0[xX]{H}+{IS}?      {
    PPIFlval.intval = atoi(yytext);
    return(CONSTANT);
}
0{D}+{IS}?          {
    PPIFlval.intval = atoi(yytext);
    return(CONSTANT);
}
{D}+{IS}?           {
    PPIFlval.intval = atoi(yytext);
    return(CONSTANT);
}
'(\\.|[^\\'])+'     {

    SInt sTmp;
    sTmp = 0;

    if ( yyleng == 2)
    {
        // empty literal('')일경우 cpp에서도 error를 발생해준다.
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_PREPROC_If_Empty_Char_Constant_Error );
        ulpPrintfErrorCode( stderr,
                            &sErrorMgr);
        // 프로그램 종료함.
    }
    else
    {
        if( yyleng == 3 )
        {
            if( yytext[1] == '\\' )
            {
                // '\' is error, '\\' is correct.
                ulpErrorMgr sErrorMgr;
                ulpSetErrorCode( &sErrorMgr,
                                 ulpERR_ABORT_PREPROC_If_Empty_Char_Constant_Error );
                ulpPrintfErrorCode( stderr,
                                    &sErrorMgr);
                // 프로그램 종료함.
            }
            else
            {
                sTmp = (SInt) yytext[1];
            }
        }
        else //yyleng 가 4이상일 경우
        {
            if( yytext[1] == '\\' )
            {
                switch( yytext[2] )
                {
                    case 'b' :
                        sTmp = (SInt) '\b';
                        break;
                    case 't' :
                        sTmp = (SInt) '\t';
                        break;
                    case 'n' :
                        sTmp = (SInt) '\n';
                        break;
                    case 'v' :
                        sTmp = (SInt) '\v';
                        break;
                    case 'f' :
                        sTmp = (SInt) '\f';
                        break;
                    case 'r' :
                        sTmp = (SInt) '\r';
                        break;
                    case '\\' :
                        sTmp = (SInt) '\\';
                        break;
                    case '\'' :
                        sTmp = (SInt) '\'';
                        break;
                    case '\"' :
                        sTmp = (SInt) '\"';
                        break;
                    case '0' :
                        sTmp = (SInt) '\0';
                        break;
                    case 'a' :
                        sTmp = (SInt) '\a';
                        break;
                    case '?' :
                        sTmp = (SInt) '\?';
                    default :
                        sTmp = (SInt) yytext[2];
                        break;
                }
                if ( yyleng > 4 )
                {
                    // 'a' 와 'aa' 를 다른 값으로 처리하기 위해, 편법이지만 임의의 숫자 999를 더함.
                    // 문제가 발생할 것 같지는 않다.
                    sTmp += 999;
                }
            }
            else
            {
                // 'a' 와 'aa' 를 다른 값으로 처리하기 위해, 편법이지만 999를 더함.
                // 문제가 발생할 것 같지는 않다.
                sTmp = (SInt) yytext[1] + 999;
            }
        }
    }
    PPIFlval.intval = sTmp;
    return(CHARACTER);
}

"defined"           { gUlpIsDefined = ID_TRUE; return(DEFINED);  }

{L}({L}|{D})*       {
    SChar sCh;
    SChar sYYTEXT[MAX_MACRO_DEFINE_NAME_LEN];
    ulpMacroNode *sMacroN;

    idlOS::snprintf( sYYTEXT, MAX_MACRO_DEFINE_NAME_LEN,
                     "%s", yytext);

    // remove spaces
    do
    {
        sCh = yyinput();
    }
    while (sCh == ' ' || sCh == '\t');

    if (sCh == '(')
    {
        do
        {
            sCh = yyinput();
        }
        while (( sCh != ')' ) && ( sCh != (SChar) EOF ));

        if( sCh == (SChar) EOF )
        {
            unput(sCh);
        }

        // function
        // bugbug : function을 실제 처리하지 않고 있으면 1 없으면 0값이 설정됨.
        if( gUlpMacroT.ulpMLookup(sYYTEXT) != NULL )
        {
            PPIFlval.intval = 1;
        }
        else
        {
            PPIFlval.intval = 0;
        }

        return(IDENTIFIER_FUNC);
    }
    else
    {
        // id
        unput(sCh);

        // defined 처리일 경우
        // ID를 macro테이블에서 찾아본다.
        if( gUlpIsDefined != ID_TRUE )
        {
            if( (sMacroN = gUlpMacroT.ulpMLookup(sYYTEXT)) != NULL )
            {
                idlOS::memset( gUlpPPIFBufState[gUlpExpandIndex].mExpandText,
                               0,
                               MAX_MACRO_DEFINE_CONTENT_LEN);

                idlOS::snprintf( gUlpPPIFBufState[gUlpExpandIndex].mExpandText,
                                 MAX_MACRO_DEFINE_CONTENT_LEN,
                                 "%s",
                                 sMacroN->mText);

                // 현재 buffer 정보 구조체 저장. (YY_CURRENT_BUFFER)
                gUlpPPIFBufState[ gUlpExpandIndex ].mBufState = YY_CURRENT_BUFFER;
                // 새로 처리될 buffer 정보 구조체로 switch. (return value of yy_scan_buffer)
                // The last two bytes of mText must be ASCII NUL.
                yy_switch_to_buffer(
                                     yy_scan_buffer(
                                        gUlpPPIFBufState[gUlpExpandIndex].mExpandText,
                                        idlOS::strlen(gUlpPPIFBufState[gUlpExpandIndex].mExpandText)+2
                                                   )
                                   );

                gUlpExpandIndex++;
            }
            else
            {
                PPIFlval.intval = 0;
                return(IDENTIFIER);
            }
        }
        else
        {
            if( gUlpMacroT.ulpMLookup(sYYTEXT) != NULL )
            {
                PPIFlval.intval = 1;
            }
            else
            {
                PPIFlval.intval = 0;
            }

            return(IDENTIFIER);
        }
    }
}

">>"            { return(RIGHT_OP); }
"<<"            { return(LEFT_OP); }
"||"            { return(OR_OP); }
"<="            { return(LE_OP); }
">="            { return(GE_OP); }
"=="            { return(EQ_OP); }
"!="            { return(NE_OP); }
"&&"            { return(AND_OP);}
"("             { return('('); }
")"             { return(')'); }
"!"             { return('!'); }
"~"             { return('~'); }
"-"             { return('-'); }
"+"             { return('+'); }
"*"             { return('*'); }
"/"             { return('/'); }
"%"             { return('%'); }
"<"             { return('<'); }
">"             { return('>'); }
"^"             { return('^'); }
"|"             { return('|'); }
"?"             { return('?'); }
"&"             { return('&'); }
":"             { return(':'); }

.               { }

<<EOF>>         {
    // Expand stack에 처리중이었던 buffer가 없으면 처리를 마치고, 있었으면 이어서 lexing 처리 한다.
    if( gUlpExpandIndex  <= 0 )
    {
        //printf("\n<EOF:%d>\n", gUlpExpandIndex);
        yyterminate();
    }
    else
    {
        //printf("\n<Expand stack end:%d>\n", gUlpExpandIndex);
        gUlpExpandIndex--;
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer( gUlpPPIFBufState[ gUlpExpandIndex ].mBufState );
    }
}

%%
// bugbug - think about these regural expr.
// "/*"            { ulpPPIFCommentC(); }
// "//"            { ulpPPIFCommentCplus();}

int PPIFlex( void * )
{
    int sRes;

    sRes =yylex();

    return sRes;
}


void PPIFerror( void *, int *,const SChar* )
{
    /*
    do nothing
    */
}

int PPIF_yyinput( char * aBuf,
                  int   aMax )
{
    int sI = ( ( aMax < (gUlpPPIFbuflim - gUlpPPIFbufptr) )
                 ? aMax : gUlpPPIFbuflim - gUlpPPIFbufptr );

    if ( sI > 0)
    {
        memcpy(aBuf, gUlpPPIFbufptr, sI);
        gUlpPPIFbufptr += sI;
        aBuf[sI]=0;
    }

    return sI;
}


IDE_RC ulpPPIFCommentC()
{
    SChar sCh1;
    SChar sCh2;

CommentC:
    sCh1 = yyinput();

    while (( sCh1 != '*' ) && ( sCh1 != (SChar) EOF ))
    {
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
        ulpPrintfErrorCode( stderr,
                            &sErrorMgr);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void ulpPPIFCommentCplus()
{
    SChar sCh;

    sCh = yyinput();
    while ( (sCh != '\n') && (sCh != (SChar) EOF) )
    {
        sCh = yyinput();
    }

    if ( sCh != (SChar) EOF )
    {
        //do nothing
    }
    else
    {
        unput(sCh);
    }
}

