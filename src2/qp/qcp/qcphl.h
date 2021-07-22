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
 * $Id: qcphl.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _QCPHL_H_
#define _QCPHL_H_ 1

#undef yyFlexLexer
#define yyFlexLexer qcphFlexLexer
#if !defined(yyFlexLexerOnce)
#   include <FlexLexer.h>
#endif

#define QCPH_BUFFER_SIZE  (2048)

// If you change the following class descriptions,
// the changes should be reflected into qcphl.l also.
class qcphLexer : public qcphFlexLexer
{
 public:
    SChar           mMessage[QCPH_BUFFER_SIZE];
    qcStatement*    mStatement;
    SInt            mTextOffset;
    // TODO - A4에 맞도록 수정해야 함
    /*
    qmsHints**  mHints;
    */

    const mtlModule * mMtlModule;

 private:
    SChar* mBuffer;
    SChar* mBufferCursor;
    SChar* mBufferLast;
    SChar* mBufferInput;
    UInt   mBufferLength;
    UInt   mBufferRemain;

 public:
    qcphLexer( char* aBuffer,
               UInt  aBufferLength );
    int    yylex( void );
    SChar* getLexLastError( SChar *msg );

    void   getPosition( qcNamePosition* aPosition );
    void   strUpper( qcNamePosition* aPosition );
    void   setInitState( void );

 private:
    int LexerInput( char* aBuffer,
                    int   aMaximum );
};

#endif /* _QCPHL_H_ */
