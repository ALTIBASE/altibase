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
 

/***********************************************************************
 * $Id: qcphl.l 89424 2020-12-04 00:29:01Z hykim $
 **********************************************************************/

%option c++
%option 8bit
%option caseless
%option never-interactive
%option noreject
%option noyymore
%option noyywrap
%option nostdinit
%option prefix="qcph"
%option yyclass="qcphLexer"

%{
#include <idl.h>
#include <idn.h>
#include <mtuProperty.h>
#include <qcuError.h>
#include <qcphManager.h>
#include "qcphx.h"

#if defined(BISON_POSTFIX_HPP)
#include "qcphy.hpp"
#else  /* BISON_POSTFIX_CPP_H */
#include "qcphy.cpp.h"
#endif

#include "qcphl.h"

// To Fix PR-11793 To Remove Warning
// Linux의 경우 /usr/include/bits/termios.h 에 ECHO가 정의되어 있음.'
// Flex가 ECHO를 define하여 사용하므로, undef해야 함.
// 참조) Flex가 생성하는 qcphl.cpp 에는 다음과 같은 Code가 생성됨.
//       #ifndef ECHO
//       #define ECHO LexerOutput( yytext, yyleng )
//       #endif
#undef ECHO

int  qcphlex(YYSTYPE * lvalp, void * param );

extern idnCharFeature gCharType;

/* TR_ Reserved Word.    */
/* TK_ Keyword.          */
/* TO_ Oracle Keyword.   */
/* TA_ Altibase Keyword. */
/* TI_ Identifier.       */
/* TL_ Literal.          */
/* TS_ Symbol.           */


/* NEG_TRAIL : 2byte character whose leading charater is always negative and
               trailing character is also negative.
               ex) KSC5601, UTF-8, EUC-JP,...    */

/* POS_TRAIL : 2byte character whose leading charater is always negative and
               trailing character can be positive
               ex) BIG5, GB231280, MS949, MS936  */

/* SJIS      : some of negative leading character is 1 byte. and
               trailing character of 2byte character can be positive
               ex) SHIFT-JIS, MS932              */
               

%}



NEG_TRAIL_CH     [\x80-\xFF]

POS_TRAIL_CH     [\x81-\xFE][\x40-\x7E\x80-\xFE]

SJIS_HWKATA      [\xA0-\xDF]
SJIS_JIS97       [\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]
SJIS_ALL         ({SJIS_HWKATA})|({SJIS_JIS97})


%s NEG_TRAIL
%s POS_TRAIL
%s SJIS


%%
ASC               { return TR_ASC;               }
DESC              { return TR_DESC;              }
FULL              { return TR_FULL;              }
NO                { return TR_NO;                }
PARALLEL          { return TR_PARALLEL;          }
NOPARALLEL        { return TR_NOPARALLEL;        }
NO_PARALLEL       { return TR_NO_PARALLEL;       }

INDEX             { return TO_INDEX;             }
NO_INDEX          { return TO_NO_INDEX;          }
INDEX_ASC         { return TO_INDEX_ASC;         }
INDEX_DESC        { return TO_INDEX_DESC;        }

BUCKET            { return TA_BUCKET;            }
COUNT             { return TA_COUNT;             }
PARTIAL           { return TA_PARTIAL;           }
ALTI_PARTIAL_FULL_SCAN { return TA_ALTI_PARTIAL_FULL_SCAN; }
ALTI_PARALLEL          { return TA_ALTI_PARALLEL;          }
ALTI_INDEX             { return TA_ALTI_INDEX;             }
ALTI_INDEX_ASC         { return TA_ALTI_INDEX_ASC;         }
ALTI_INDEX_DESC        { return TA_ALTI_INDEX_DESC;        }
ALTI_NO_INDEX          { return TA_ALTI_NO_INDEX;          }

<NEG_TRAIL>([_A-Z]|({NEG_TRAIL_CH}))([_0-9A-Z$]|({NEG_TRAIL_CH}))*   {
                                         return TI_NONQUOTED_IDENTIFIER; }
<POS_TRAIL>([_A-Z]|({POS_TRAIL_CH}))([_0-9A-Z$]|({POS_TRAIL_CH}))*   {
                                         return TI_NONQUOTED_IDENTIFIER; }
<SJIS>([_A-Z]|({SJIS_ALL}))([_0-9A-Z$]|({SJIS_ALL}))*   {
                                         return TI_NONQUOTED_IDENTIFIER; }


"\""[^'"]+"\""  |
"'"[^']+"'"                            { return TI_QUOTED_IDENTIFIER;    }



[0-9]+                                { return TL_INTEGER;      }

"("   { return TS_OPENING_PARENTHESIS; }
")"   { return TS_CLOSING_PARENTHESIS; }
","   { return TS_COMMA;               }
"."   { return TS_PERIOD;              }

"\n\r" |
"\n"   | 
"\r"     { }

[ \t]+   { }

.        { return E_ERROR; }

%%


// If you change the following class descriptions,
// the changes should be reflected into qcphl.h also.
qcphLexer::qcphLexer( SChar* aBuffer,
                      UInt   aBufferLength )
{
    mBuffer       = aBuffer;
    mBufferCursor = mBuffer;
    mBufferLength = aBufferLength;
    mBufferRemain = aBufferLength;

    if (mtl::mDBCharSet != NULL)
    {
        mMtlModule = mtl::mDBCharSet;
    }
    else
    {
        mMtlModule = mtl::defaultModule();
    }
}

SChar * qcphLexer::getLexLastError( SChar * aMessage )
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

    sMessageIterator = idlOS::snprintf( mMessage, QCPH_BUFFER_SIZE,
                                        "\n\nline %d: %s\n",
                                        sLineNo,
                                        aMessage );

    sMessageFence     = sMessageIterator + sPosition.size;

    if( sMessageFence >= QCPH_BUFFER_SIZE )
    {
        sMessageFence = QCPH_BUFFER_SIZE - 8;
    }

    for( sBufferIterator  = sLineOffset;
         sMessageIterator < sMessageFence &&
         sBufferIterator  < mBufferLength;
         sBufferIterator++, sMessageIterator++ )
    {
        mMessage[sMessageIterator] = mBuffer[sBufferIterator];
    }

    for( sMessageFence    = QCPH_BUFFER_SIZE - 8;
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
         if( sBufferIterator == (UInt)(sPosition.offset)                      ||
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

void qcphLexer::getPosition( qcNamePosition* aPosition )
{
    // mBuffer는 복사된 버퍼이므로 mStatement->stmtText를 저장해야 한다.
    aPosition->stmtText = mStatement->myPlan->stmtText;
    aPosition->offset   = (mBufferLast-mBuffer)+(YYText()-mBufferInput);
    aPosition->size     = YYLeng();
}

void qcphLexer::strUpper( qcNamePosition* aPosition )
{
    SChar * sIndex = mBuffer + aPosition->offset;
    SChar * sFence = sIndex + aPosition->size;

    // PRJ-1678 : For multi-byte character set strings
    for( ;sIndex < sFence; mMtlModule->nextCharPtr( (UChar**) &sIndex,
                                                    (UChar*)sFence ) )
    {
        *sIndex = idlOS::toupper(*sIndex);
    }
    
    // BUG-9020
    // mBuffer는 복사된 버퍼이므로 mStatement->stmtText에도 반영해야한다.
    sIndex = mStatement->myPlan->stmtText + mTextOffset + aPosition->offset;
    sFence = sIndex + aPosition->size;

    // PRJ-1678 : For multi-byte character set strings
    for( ;sIndex < sFence; mMtlModule->nextCharPtr( (UChar**) &sIndex,
                                                    (UChar*) sFence ) )
    {
        *sIndex = idlOS::toupper(*sIndex);
    }
}

int qcphLexer::LexerInput( char* aBuffer,
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


void qcphLexer::setInitState( void )
{
    if( gCharType == IDN_CF_UNKNOWN )
    {
        switch(mMtlModule->id)
        {
            case MTL_SHIFTJIS_ID:
            /* PROJ-2590 [기능성] CP932 database character set 지원 */ 
            case MTL_MS932_ID:
            {
                gCharType = IDN_CF_SJIS;
                break;
            }
            case MTL_BIG5_ID:
            case MTL_GB231280_ID:
            /* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
            case MTL_MS936_ID:
            case MTL_MS949_ID:
            {
                gCharType = IDN_CF_POS_TRAIL;
                break;
            }
            default:
            {
                gCharType = IDN_CF_NEG_TRAIL;
                break;
            }
        }
    }

    switch(gCharType)
    {
        case IDN_CF_SJIS:
        {
            BEGIN(SJIS);
            break;
        }
        case IDN_CF_POS_TRAIL:
        {
            BEGIN(POS_TRAIL);
            break;
        }
        default:
        {
            BEGIN(NEG_TRAIL);
            break;
        }
    }
}

int qcphlex( YYSTYPE* lvalp,
             void*    param )
{
    int        sToken;
    qcphLexer* sLexer  = ((qcphx *)param)->mLexer;

    sLexer->setInitState();

    sToken = sLexer->yylex();

    sLexer->getPosition( &lvalp->position );

    if( sToken == TI_NONQUOTED_IDENTIFIER )
    {
        sLexer->strUpper( &lvalp->position );
    }

    return sToken;
}
