/*******************************************************************************
 * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $Id: genmsgLex.re 1379 2007-12-05 01:58:45Z shawn $
 ******************************************************************************/

#include <genmsg.h>
#include <genmsgParse.h>


acp_sint32_t genmsgGetToken(genmsgParseContext *aParseContext)
{
#define NEWLINE(aSkip)                                          \
    do                                                          \
    {                                                           \
        aParseContext->mLineNo++;                               \
        aParseContext->mLine = aParseContext->mToken + aSkip;   \
    } while (0)

#define YYFILL                                                  \
    do                                                          \
    {                                                           \
        if (aParseContext->mCursor > aParseContext->mLimit)     \
        {                                                       \
            return GENMSG_TOKEN_EOF;                            \
        }                                                       \
        else                                                    \
        {                                                       \
        }                                                       \
    } while (0)

    /*!re2c

      re2c:define:YYCTYPE   = "unsigned char";
      re2c:define:YYCURSOR  = aParseContext->mCursor;
      re2c:define:YYLIMIT   = aParseContext->mLimit;
      re2c:define:YYMARKER  = aParseContext->mMarker;
      re2c:yyfill:enable    = 1;
      re2c:yyfill:parameter = 0;
      re2c:yych:conversion  = 1;

      lower    = [a-z];
      upper    = [A-Z];
      digit    = [0-9];
      alpha    = [a-zA-Z];
      alnum    = [a-zA-Z0-9];
      macro    = [a-zA-Z0-9_];
      white    = [ \t\v\f];
      hexdigit = [a-fA-F0-9];

    */

std:
    aParseContext->mToken = aParseContext->mCursor;

    /*!re2c

      "/*"              { goto comment_block; }
      "//"              { goto comment_line;  }
      "#"               { goto comment_line;  }

      "\""              { goto quoted_string; }

      "{"               { return GENMSG_TOKEN_BRACE_OPEN; }
      "}"               { return GENMSG_TOKEN_BRACE_CLOSE; }
      ","               { return GENMSG_TOKEN_COMMA; }
      ";"               { return GENMSG_TOKEN_SEMICOLON; }
      "="               { return GENMSG_TOKEN_ASSIGN; }

      "SECTION"         { return GENMSG_TOKEN_SECTION; }
      "FILE"            { return GENMSG_TOKEN_FILE; }
      "ERROR"           { return GENMSG_TOKEN_ERROR; }
      "LOG"             { return GENMSG_TOKEN_LOG; }

      "PRODUCT_ID"      { return GENMSG_TOKEN_PRODUCT_ID; }
      "PRODUCT_NAME"    { return GENMSG_TOKEN_PRODUCT_NAME; }
      "PRODUCT_ABBR"    { return GENMSG_TOKEN_PRODUCT_ABBR; }
      "LANGUAGES"       { return GENMSG_TOKEN_LANGUAGES; }

      "ERR_ID"          { return GENMSG_TOKEN_ERR_ID; }
      "KEY"             { return GENMSG_TOKEN_KEY; }
      "MSG_" upper+     { aParseContext->mToken += 4; return GENMSG_TOKEN_MSG; }

      '0x' hexdigit+    { return GENMSG_TOKEN_NUMBER; }
      digit+            { return GENMSG_TOKEN_NUMBER; }
      alpha macro*      { return GENMSG_TOKEN_IDENTIFIER; }

      "\r\n" | "\n"     { NEWLINE(1); goto std; }
      white+            { goto std; }
      .                 { return GENMSG_TOKEN_INVALID; }

    */

comment_block:
    /*!re2c

      "*\/"             { goto std; }
      "\r\n" | "\n"     { NEWLINE(1); goto comment_block; }
      .                 { goto comment_block; }

    */

comment_line:
    /*!re2c

      "\r\n" | "\n"     { NEWLINE(1); goto std; }
      .                 { goto comment_line; }

    */

quoted_string:
    /*!re2c

      "\\\""            { goto quoted_string; }
      "\r\n" | "\n"     { NEWLINE(1); goto quoted_string; }
      "\""              { return GENMSG_TOKEN_STRING; }
      .                 { goto quoted_string; }

    */
}
