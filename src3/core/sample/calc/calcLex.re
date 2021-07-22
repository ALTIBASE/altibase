/*******************************************************************************
 * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $Id: calcLex.re 1296 2007-11-29 04:36:41Z shsuh $
 ******************************************************************************/

#include <calc.h>
#include <calcParse.h>


acp_sint32_t calcGetToken(calcContext *aContext)
{
#define YYFILL                                          \
    do                                                  \
    {                                                   \
        if (aContext->mCursor > aContext->mLimit)       \
        {                                               \
            return CALC_TOKEN_EOF;                      \
        }                                               \
        else                                            \
        {                                               \
        }                                               \
    } while (0)

    /*!re2c

      re2c:define:YYCTYPE     = "unsigned char";
      re2c:define:YYCURSOR    = aContext->mCursor;
      re2c:define:YYLIMIT     = aContext->mLimit;
      re2c:yyfill:enable      = 1;
      re2c:yyfill:parameter   = 0;
      re2c:yych:conversion    = 1;

      alpha = [A-Za-z];
      alnum = [A-Za-z0-9];
      digit = [0-9];
      white = [ \t\v\f];

    */

std:
    aContext->mToken = aContext->mCursor;

    /*!re2c

      "/*"              { goto comment_block; }
      "//"              { goto comment_line;  }

      "+"               { return CALC_TOKEN_PLUS; }
      "-"               { return CALC_TOKEN_MINUS; }
      "*"               { return CALC_TOKEN_TIMES; }
      "/"               { return CALC_TOKEN_DIV; }
      "%"               { return CALC_TOKEN_MOD; }
      "("               { return CALC_TOKEN_PAREN_OPEN; }
      ")"               { return CALC_TOKEN_PAREN_CLOSE; }
      "="               { return CALC_TOKEN_ASSIGN; }

      'quit'            { return CALC_TOKEN_QUIT; }

      alpha alnum*      { return CALC_TOKEN_VAR; }
      digit+            { return CALC_TOKEN_VALUE; }

      "\r\n" | "\n"     { return CALC_TOKEN_NEWLINE; }
      white+            { goto std; }
      .                 { return CALC_TOKEN_INVALID; }

    */

comment_block:
    /*!re2c

      "*\/"             { goto std; }
      .                 { goto comment_block; }

    */

comment_line:
    /*!re2c

      "\r\n" | "\n"     { return CALC_TOKEN_NEWLINE; }
      .                 { goto comment_line; }

    */
}
