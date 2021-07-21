/***********************************************************************
 * Copyright 1999-2017, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

%pure_parser

%{
#include <idl.h>
#include <mtcDef.h>
#include <qc.h>
#include <sdpjManager.h>
#include <sdi.h>

#define PARAM       ((sdpjx*)param)
#define BUFFER      (PARAM->mParseBuffer)
#define REMAIN      (PARAM->mParseBufferLength)
#define ALLOC_COUNT (PARAM->mAllocCount)
#define TEXT        (PARAM->mText)
#define OBJECT      (PARAM->mObject)

#define SDPJY_ALLOC(target, type)                         \
{                                                         \
    if (ID_SIZEOF(type) <= REMAIN)                        \
    {                                                     \
        (target) = (type*)BUFFER;                         \
        BUFFER += ID_SIZEOF(type);                        \
        REMAIN -= ID_SIZEOF(type);                        \
        ALLOC_COUNT++;                                    \
    }                                                     \
    else                                                  \
    {                                                     \
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDPJ_ALLOC,   \
                                sdpjManager::getProgress(PARAM->mLexer))); \
        YYABORT;                                          \
    }                                                     \
}

/*BUGBUG_NT*/
#if defined(VC_WIN32)
# include <malloc.h>
//# define alloca _alloca
# define strcasecmp _stricmp
#endif
/*BUGBUG_NT ADD*/

#if defined(SYMBIAN)
void * alloca(unsigned int);
#endif
%}

%union {
    qcNamePosition     position;
    sdpjObject       * object;
    sdpjArray        * array;
    sdpjKeyValue       keyValue;
    sdpjString         string;
    sdpjValue          value;
}

%token TR_TRUE
%token TR_FALSE
%token TR_NULL

%token TI_QUOTED_IDENTIFIER

%token TL_INTEGER
%token TL_NUMERIC

%token TS_OPENING_BRACE
%token TS_CLOSING_BRACE
%token TS_OPENING_BRACKET
%token TS_CLOSING_BRACKET

%token TS_COMMA
%token TS_COLON

%{
#include <idl.h>
#include <sde.h>
#include "sdpjx.h"

#define YYPARSE_PARAM param
#define YYLEX_PARAM   param

extern int      sdpjlex(YYSTYPE * lvalp, void * param );

static void     sdpjerror( const char * );
%}


%%
json_object
    : TS_OPENING_BRACE TS_CLOSING_BRACE
    {
        $<object>$ = NULL;
        OBJECT = $<object>$;
    }
    | TS_OPENING_BRACE json_object_elements TS_CLOSING_BRACE
    {
        $<object>$ = $<object>2;
        OBJECT = $<object>$;
    }
    ;

json_object_elements
    : json_key_value TS_COMMA json_object_elements
    {
        SDPJY_ALLOC($<object>$, sdpjObject);
        $<object>$->mKeyValue = $<keyValue>1;
        $<object>$->mNext = $<object>3;
    }
    | json_key_value
    {
        SDPJY_ALLOC($<object>$, sdpjObject);
        $<object>$->mKeyValue = $<keyValue>1;
        $<object>$->mNext = NULL;
    }
    ;

json_key_value
    : json_string TS_COLON json_value
    {
        $<keyValue>$.mKey = $<string>1;
        $<keyValue>$.mValue = $<value>3;
    }
    ;

json_array
    : TS_OPENING_BRACKET TS_CLOSING_BRACKET
    {
        $<array>$ = NULL;
    }
    | TS_OPENING_BRACKET json_array_elements TS_CLOSING_BRACKET
    {
        $<array>$ = $<array>2;
    }
    ;

json_array_elements
    : json_value TS_COMMA json_array_elements
    {
        SDPJY_ALLOC($<array>$, sdpjArray);
        $<array>$->mValue = $<value>1;
        $<array>$->mNext = $<array>3;
    }
    | json_value
    {
        SDPJY_ALLOC($<array>$, sdpjArray);
        $<array>$->mValue = $<value>1;
        $<array>$->mNext = NULL;
    }
    ;

json_value
    : json_object
    {
        $<value>$.mType = SDPJ_OBJECT;
        $<value>$.mValue.mObject = $<object>1;
    }
    | json_array
    {
        $<value>$.mType = SDPJ_ARRAY;
        $<value>$.mValue.mArray = $<array>1;
    }
    | json_string
    {
        $<value>$.mType = SDPJ_STRING;
        $<value>$.mValue.mString = $<string>1;
    }
    | TL_INTEGER
    {
        $<value>$.mType = SDPJ_LONG;
        $<value>$.mValue.mLong = 0;
    }
    | TL_NUMERIC
    {
        $<value>$.mType = SDPJ_DOUBLE;
        $<value>$.mValue.mDouble = 0;
    }
    | TR_TRUE
    {
        $<value>$.mType = SDPJ_TRUE;
    }
    | TR_FALSE
    {
        $<value>$.mType = SDPJ_FALSE;
    }
    | TR_NULL
    {
        $<value>$.mType = SDPJ_NULL;
    }
    ;

json_string
    : TI_QUOTED_IDENTIFIER
    {
        $<string>$.mOffset = $<position>1.offset + 1;
        $<string>$.mSize  = $<position>1.size - 2;
    }
    ;

%%

# undef yyFlexLexer
# define yyFlexLexer sdpjFlexLexer

# include <FlexLexer.h>

#include "sdpjl.h"

void sdpjerror( const char* /*msg*/ )
{
    IDE_SET(ideSetErrorCode(sdERR_ABORT_SDPJ_SYNTAX));
}
