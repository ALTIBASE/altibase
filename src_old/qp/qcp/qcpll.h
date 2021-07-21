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
 * $Id: qcpll.h 82186 2018-02-05 05:17:56Z lswhh $
 **********************************************************************/

#ifndef _QCPLL_H_
# define _QCPLL_H_ 1

#undef yyFlexLexer
#define yyFlexLexer qcplFlexLexer
#if !defined(yyFlexLexerOnce)
#   include <FlexLexer.h>
#endif

# define QCPL_BUFFER_SIZE  (2048)

class qcplLexer : public qcplFlexLexer
{
 public:
    SChar           mMessage[QCPL_BUFFER_SIZE];
    qcStatement*    mStatement;
    const mtlModule * mMtlModule;
    
 private:
    SChar* mBuffer;
    SChar* mBufferCursor;
    SChar* mBufferLast;
    SChar* mBufferInput;
    UInt   mBufferLength;
    UInt   mBufferRemain;
    SInt   mFirstToken;
    qcNamePosition mLastTokenPos[2];

 public:
    qcplLexer( char* aBuffer,
               UInt  aBufferLength,
               SInt  aStart,
               SInt  aSize,
               SInt  aFirstToken ); 
    void   setInitState( void );
    int    yylex( void );
    SChar* getLexLastError( SChar *msg );

    SInt   getFirstToken( void );
    void   updateLastTokenPosition( qcNamePosition* aPosition );
    void   setLastTokenPosition( qcNamePosition* aPosition );
    void   setEndTokenPosition( qcNamePosition* aPosition );
    void   setNullPosition( qcNamePosition* aPosition );
    void   getPosition( qcNamePosition* aPosition );
    void   strUpper( qcNamePosition* aPosition );
    void   printToken(  ){printf("Token(%s)\n", yytext);};

 private:
    int LexerInput( char* aBuffer,
                    int   aMaximum );
};

#endif /* _QCPLL_H_ */
