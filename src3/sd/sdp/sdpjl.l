/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

%option c++
%option 8bit
%option caseless
%option never-interactive
%option noreject
%option noyymore
%option noyywrap
%option nostdinit
%option prefix="sdpj"
%option yyclass="sdpjLexer"

%{
#include <idl.h>
#include <idn.h>
#include <sdpjManager.h>
#include "sdpjx.h"

#if defined(BISON_POSTFIX_HPP)
#include "sdpjy.hpp"
#else  /* BISON_POSTFIX_CPP_H */
#include "sdpjy.cpp.h"
#endif

#include "sdpjl.h"

// To Fix PR-11793 To Remove Warning
// Linux의 경우 /usr/include/bits/termios.h 에 ECHO가 정의되어 있음.'
// Flex가 ECHO를 define하여 사용하므로, undef해야 함.
// 참조) Flex가 생성하는 sdpjl.cpp 에는 다음과 같은 Code가 생성됨.
//       #ifndef ECHO
//       #define ECHO LexerOutput( yytext, yyleng )
//       #endif
#undef ECHO

int  sdpjlex(YYSTYPE * lvalp, void * param );

/* TR_ Reserved Word.    */
/* TK_ Keyword.          */
/* TO_ Oracle Keyword.   */
/* TA_ Altibase Keyword. */
/* TI_ Identifier.       */
/* TL_ Literal.          */
/* TS_ Symbol.           */

%}


%%
TRUE              { return TR_TRUE;              }
FALSE             { return TR_FALSE;             }
NULL              { return TR_NULL;              }


"\""[^'"]*"\"" |
"'"[^']*"'"                           { return TI_QUOTED_IDENTIFIER;    }


[0-9]+                                { return TL_INTEGER;      }

[0-9]+("."[0-9]*)?([Ee][+-]?[0-9]+)?  |
"."[0-9]+([Ee][+-]?[0-9]+)?           { return TL_NUMERIC;      }


"{"   { return TS_OPENING_BRACE;       }
"}"   { return TS_CLOSING_BRACE;       }

"["   { return TS_OPENING_BRACKET;     }
"]"   { return TS_CLOSING_BRACKET;     }

","   { return TS_COMMA;               }
":"   { return TS_COLON;               }

"\n\r" |
"\n"   | 
"\r"     { }

[ \t]+   { }

%%


// If you change the following class descriptions,
// the changes should be reflected into sdpjl.h also.
sdpjLexer::sdpjLexer( SChar* aBuffer,
                      UInt   aBufferLength )
{
    mBuffer       = aBuffer;
    mBufferCursor = mBuffer;
    mBufferLength = aBufferLength;
    mBufferRemain = aBufferLength;
}

SChar * sdpjLexer::getLexLastError( SChar * aMessage )
{
    qcNamePosition sPosition;
    UInt           sLineOffset;
    UInt           sLineNo;
    UInt           sBufferIterator;
    UInt           sMessageIterator;
    UInt           sMessageFence;

    getPosition( &sPosition );

    sLineOffset = 0 ;
    sLineNo     = 1;
    for( sBufferIterator = 0;
         sBufferIterator < (UInt)(sPosition.offset);
         sBufferIterator++ )
    {
        if( mBuffer[sBufferIterator] == '\n' )
        {
            sLineOffset = sBufferIterator + 1;
            sLineNo++;
        }
    }

    sMessageIterator = idlOS::snprintf( mMessage, SDPJ_BUFFER_SIZE,
                                        "\n\nline %d: %s\n",
                                        sLineNo,
                                        aMessage );

    sMessageFence     = sMessageIterator + sPosition.size;

    if( sMessageFence >= SDPJ_BUFFER_SIZE )
    {
        sMessageFence = SDPJ_BUFFER_SIZE - 8;
    }

    for( sBufferIterator  = sLineOffset;
         sMessageIterator < sMessageFence &&
         sBufferIterator  < mBufferLength;
         sBufferIterator++, sMessageIterator++ )
    {
        mMessage[sMessageIterator] = mBuffer[sBufferIterator];
    }

    for( sMessageFence    = SDPJ_BUFFER_SIZE - 8;
         sMessageIterator < sMessageFence &&
         sBufferIterator  < mBufferLength &&
         mBuffer[sBufferIterator] != '\n';
         sBufferIterator++, sMessageIterator++ )
    {
        mMessage[sMessageIterator] = mBuffer[sBufferIterator];
    }

    if( sMessageIterator < sMessageFence )
    {
        mMessage[sMessageIterator] = '\n';
        sMessageIterator++;
    }

    for( sBufferIterator = sLineOffset;
         sMessageIterator < sMessageFence &&
         sBufferIterator  < mBufferLength;
         sBufferIterator++, sMessageIterator++ )
    {
         if( sBufferIterator == (UInt)(sPosition.offset) ||
             sBufferIterator == (UInt)(sPosition.offset + sPosition.size - 1)  )
         {
             mMessage[sMessageIterator] = '^';
         }
         else
         {
             mMessage[sMessageIterator] = ( mBuffer[sBufferIterator] == '\n' )
                                        ? '\n' : ' ' ;
         }

        if( sBufferIterator == (UInt)(sPosition.offset + sPosition.size - 1) )
        {
            sMessageIterator++;
            break;
        }
        else
        {
            // Nothing To Do
        }
    }

    mMessage[sMessageIterator] = '\n';
    sMessageIterator++;
    mMessage[sMessageIterator] = '\n';
    sMessageIterator++;
    mMessage[sMessageIterator] = '\0';

    return mMessage;
}

SInt sdpjLexer::getProgress()
{
    SInt sOffset = (mBufferLast-mBuffer) + (YYText()-mBufferInput);
    SInt sProgress = 100;

    if ( sOffset < (SInt)mBufferLength )
    {
        sProgress = sOffset * 100 / mBufferLength;
    }

    return sProgress;
}

void sdpjLexer::getPosition( qcNamePosition* aPosition )
{
    aPosition->stmtText = mBuffer;
    aPosition->offset   = (mBufferLast-mBuffer)+(YYText()-mBufferInput);
    aPosition->size     = YYLeng();
}

int sdpjLexer::LexerInput( char* aBuffer,
                           int   aMaximum )
{
    aMaximum       =
        ( (int)mBufferRemain < aMaximum ) ? (int)mBufferRemain : aMaximum ;
    aMaximum       = ( aMaximum > 0 ) ? aMaximum : 0 ;
    mBufferRemain -= aMaximum;

    idlOS::memcpy( aBuffer, mBufferCursor, aMaximum );

    mBufferLast    = mBufferCursor;

    mBufferCursor += aMaximum;

    mBufferInput   = aBuffer;

    return aMaximum;
}

int sdpjlex( YYSTYPE* lvalp,
             void*    param )
{
    int        sToken;
    sdpjLexer* sLexer  = ((sdpjx *)param)->mLexer;

    sToken = sLexer->yylex();

    //sLexer->printToken();

    sLexer->getPosition( &lvalp->position );

    return sToken;
}
