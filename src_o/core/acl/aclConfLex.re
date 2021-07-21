/*******************************************************************************
 * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $Id: aclConfLex.re 1263 2007-11-27 05:04:28Z shsuh $
 ******************************************************************************/

#include <aclConf.h>
#include <aclConfParse.h>
#include <aclConfPrivate.h>


acp_sint32_t aclConfGetToken(aclConfContext *aContext)
{
#define NEWLINE(aSkip)                                  \
    do                                                  \
    {                                                   \
        aContext->mLine++;                              \
    } while (0)

#define YYFILL                                          \
    do                                                  \
    {                                                   \
        if (aContext->mCursor > aContext->mLimit)       \
        {                                               \
            return ACL_CONF_TOKEN_EOF;                  \
        }                                               \
        else                                            \
        {                                               \
        }                                               \
    } while (0)

    /*!re2c

      re2c:define:YYCTYPE     = "unsigned char";
      re2c:define:YYCURSOR    = aContext->mCursor;
      re2c:define:YYLIMIT     = aContext->mLimit;
      re2c:define:YYCTXMARKER = aContext->mCtxMarker;
      re2c:define:YYMARKER    = aContext->mMarker;
      re2c:yyfill:enable      = 1;
      re2c:yyfill:parameter   = 0;
      re2c:yych:conversion    = 1;

      white = [ \t\v\f];

      key   = [a-zA-Z0-9_];

      val   = [^ (){},=#\"\t\v\f\r\n];

      empty = white* [#\r\n]*;

    */

std:
    aContext->mToken = aContext->mCursor;

    /*!re2c

      "#"                      { goto comment;  }

      "\""                     { goto quoted_string; }

      "=" / empty* "("         { return ACL_CONF_TOKEN_ASSIGN_COLLECTION; }
      "=" / white* [#\r\n]     { return ACL_CONF_TOKEN_ASSIGN_NULL; }
      "=" / white* [^ (#\r\n]  { return ACL_CONF_TOKEN_ASSIGN_VALUE; }
      ","                      { return ACL_CONF_TOKEN_COMMA; }
      "("                      { return ACL_CONF_TOKEN_PAREN_OPEN; }
      ")"                      { return ACL_CONF_TOKEN_PAREN_CLOSE; }

      key+ / white* "="        { return ACL_CONF_TOKEN_KEY; }
      val+ / white* [),#\r\n]  { return ACL_CONF_TOKEN_VALUE; }

      "\r\n" | "\n"            { NEWLINE(1); goto std; }
      white+                   { goto std; }
      .                        { return ACL_CONF_TOKEN_INVALID; }

    */

comment:
    /*!re2c

      "\r\n" | "\n"            { NEWLINE(1); goto std; }
      .                        { goto comment; }

    */

quoted_string:
    /*!re2c

      "\\\""                   { goto quoted_string; }
      "\r\n" | "\n"            { NEWLINE(1); goto quoted_string; }
      "\""                     { return ACL_CONF_TOKEN_QVALUE; }
      .                        { goto quoted_string; }

    */
}
