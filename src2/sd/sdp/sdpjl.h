/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _SDPJL_H_
#define _SDPJL_H_ 1

#undef yyFlexLexer
#define yyFlexLexer sdpjFlexLexer
#if !defined(yyFlexLexerOnce)
#   include <FlexLexer.h>
#endif

#define SDPJ_BUFFER_SIZE  (2048)

// If you change the following class descriptions,
// the changes should be reflected into sdpjl.l also.
class sdpjLexer : public sdpjFlexLexer
{
 public:
    SChar           mMessage[SDPJ_BUFFER_SIZE];

 private:
    SChar         * mBuffer;
    SChar         * mBufferCursor;
    SChar         * mBufferLast;
    SChar         * mBufferInput;
    UInt            mBufferLength;
    UInt            mBufferRemain;

 public:
    sdpjLexer( char* aBuffer, UInt  aBufferLength );

    int    yylex( void );
    SChar* getLexLastError( SChar *msg );

    SInt   getProgress( void );
    void   getPosition( qcNamePosition* aPosition );
    void   printToken() {printf("Token(%s)\n", yytext);};

 private:
    int    LexerInput( char* aBuffer, int   aMaximum );
};

#endif /* _SDPJL_H_ */
