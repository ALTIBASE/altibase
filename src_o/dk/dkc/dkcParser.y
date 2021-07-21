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
 
%pure-parser

%{

#include <idl.h>
#include <ide.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkc.h>
#include <dkcUtil.h>

#if defined(BISON_POSTFIX_HPP)
#include "dkcParser.hpp"
#else  /* BISON_POSTFIX_CPP_H */
#include "dkcParser.cpp.h"
#endif

#include <dkcParser.h>
#include <dkcLexer.h>

struct dkcParser {

    dkcDblinkConf mDblinkConf;

    dkcDblinkConfTarget mDblinkConfTarget;

    SChar mErrorMessage[ 1024 ];
};

#define YYPARSE_PARAM           param

#define PARAM     ((dkcParser *)param)

#undef yyerror
#define yyerror(msg) dkcParser_error( &yylloc, PARAM, (msg) )

%}

%{
/*BUGBUG_NT*/
#if defined(VC_WIN32)
#include <malloc.h>
#define strcasecmp _stricmp
#endif
%}

%token DKC_ALTILINKER_ENABLE
%token DKC_ALTILINKER_PORT_NO
%token DKC_ALTILINKER_RECEIVE_TIMEOUT
%token DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT
%token DKC_ALTILINKER_QUERY_TIMEOUT
%token DKC_ALTILINKER_NON_QUERY_TIMEOUT
%token DKC_ALTILINKER_THREAD_COUNT
%token DKC_ALTILINKER_THREAD_SLEEP_TIME
%token DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT
%token DKC_ALTILINKER_TRACE_LOG_DIR
%token DKC_ALTILINKER_TRACE_LOG_FILE_SIZE
%token DKC_ALTILINKER_TRACE_LOG_FILE_COUNT
%token DKC_ALTILINKER_TRACE_LOGGING_LEVEL
%token DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE
%token DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE
%token DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE
%token DKC_TARGETS
%token DKC_NAME
%token DKC_JDBC_DRIVER
%token DKC_JDBC_DRIVER_CLASS_NAME
%token DKC_CONNECTION_URL
%token DKC_USER
%token DKC_PASSWORD
%token DKC_XADATASOURCE_CLASS_NAME
%token DKC_XADATASOURCE_URL_SETTER_NAME

%token DKC_EQUAL
%token DKC_OPENING_PARENTHESIS
%token DKC_CLOSING_PARENTHESIS
%token DKC_COMMA
%token DKC_QUOTED_VALUE
%token DKC_INTEGER_VALUE

%token DKC_ERROR

%{
/*BUGBUG_NT*/
#if ! defined(VC_WIN32)
%}

%union {

    SInt mIntegerValue;

    struct {

        SChar * mText;

        SInt mLength;

    } mValue;
}

%{
#endif
%}

%{

/*
 *
 */
static void dkcParser_error( YYLTYPE * aLocation,
                             dkcParser * aParser,
                             const SChar * aMessage )
{
    idlOS::snprintf( aParser->mErrorMessage,
                     ID_SIZEOF(aParser->mErrorMessage),
                     "%s: Line number %d\n",
                     aMessage,
                     aLocation->first_line );

}

/*
 *
 */
static void dkcParserInvalidValueError( YYLTYPE * aLocation,
                                        dkcParser * aParser,
                                        SInt aMinimumValue,
                                        SInt aMaximumValue,
                                        const SChar * aKey )
{
    idlOS::snprintf( aParser->mErrorMessage,
                     ID_SIZEOF(aParser->mErrorMessage),
                     "%s is invalid ( %d ~ %d ): Line %d\n",
                     aKey,
                     aMinimumValue,
                     aMaximumValue,
                     aLocation->first_line );
}

/*
 *
 */ 
static SInt dkcParser_lex( YYSTYPE * aValue, YYLTYPE * aLocation )
{
    return dkcLexerGetNextToken( aValue, aLocation );
}

%}

%%

configuration_file: statement_list
    | /* empty */
    ;

statement_list: statement_list statement
    | statement
    ;
    
statement: key_and_value_statement
    | target_statement
    ;

key_and_value_statement: DKC_ALTILINKER_ENABLE DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_ENABLE_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_ENABLE_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerEnable =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_ENABLE_MIN,
                                          DKC_ALTILINKER_ENABLE_MAX,
                                          "ALTILINKER_ENABLE" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_PORT_NO DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_PORT_NO_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_PORT_NO_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerPortNo = $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_PORT_NO_MIN,
                                          DKC_ALTILINKER_PORT_NO_MAX,
                                          "ALTILINKER_PORT_NO" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_RECEIVE_TIMEOUT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_RECEIVE_TIMEOUT_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_RECEIVE_TIMEOUT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerReceiveTimeout =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_RECEIVE_TIMEOUT_MIN,
                                          DKC_ALTILINKER_RECEIVE_TIMEOUT_MAX,
                                          "ALTILINKER_RECEIVE_TIMEOUT" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerRemoteNodeReceiveTimeout =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_MIN,
                  DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT_MAX,
                  "ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_QUERY_TIMEOUT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_QUERY_TIMEOUT_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_QUERY_TIMEOUT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerQueryTimeout =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_QUERY_TIMEOUT_MIN,
                                          DKC_ALTILINKER_QUERY_TIMEOUT_MAX,
                                          "ALTILINKER_QUERY_TIMEOUT" );
              YYERROR;
          }
      }    
    | DKC_ALTILINKER_NON_QUERY_TIMEOUT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_NON_QUERY_TIMEOUT_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_NON_QUERY_TIMEOUT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerNonQueryTimeout =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_NON_QUERY_TIMEOUT_MIN,
                                          DKC_ALTILINKER_NON_QUERY_TIMEOUT_MAX,
                                          "ALTILINKER_NON_QUERY_TIMEOUT" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_THREAD_COUNT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_THREAD_COUNT_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_THREAD_COUNT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerThreadCount =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_THREAD_COUNT_MIN,
                                          DKC_ALTILINKER_THREAD_COUNT_MAX,
                                          "ALTILINKER_THREAD_COUNT" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_THREAD_SLEEP_TIME DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >= DKC_ALTILINKER_THREAD_SLEEP_TIME_MIN ) &&
               ( $<mIntegerValue>3 <= DKC_ALTILINKER_THREAD_SLEEP_TIME_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerThreadSleepTime =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError( &(@3),
                                          PARAM,
                                          DKC_ALTILINKER_THREAD_SLEEP_TIME_MIN,
                                          DKC_ALTILINKER_THREAD_SLEEP_TIME_MAX,
                                          "ALTILINKER_THREAD_SLEEP_TIME" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerRemoteNodeSessionCount =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_MIN,
                  DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT_MAX,
                  "ALTILINKER_REMOTE_NODE_SESSION_COUNT" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_TRACE_LOG_DIR DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConf).mAltilinkerTraceLogDir) );

          if ( (PARAM->mDblinkConf).mAltilinkerTraceLogDir == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_ALTILINKER_TRACE_LOG_FILE_SIZE DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerTraceLogFileSize =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_MIN,
                  DKC_ALTILINKER_TRACE_LOG_FILE_SIZE_MAX,
                  "ALTILINKER_TRACE_LOG_FILE_SIZE" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_TRACE_LOG_FILE_COUNT DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerTraceLogFileCount =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_MIN,
                  DKC_ALTILINKER_TRACE_LOG_FILE_COUNT_MAX,
                  "ALTILINKER_TRACE_LOG_FILE_COUNT" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_TRACE_LOGGING_LEVEL DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_TRACE_LOGGING_LEVEL_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_TRACE_LOGGING_LEVEL_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerTraceLoggingLevel =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_TRACE_LOGGING_LEVEL_MIN,
                  DKC_ALTILINKER_TRACE_LOGGING_LEVEL_MAX,
                  "ALTILINKER_TRACE_LOGGING_LEVEL" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerJvmMemoryPoolInitSize =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_MIN,
                  DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE_MAX,
                  "ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerJvmMemoryPoolMaxSize =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_MIN,
                  DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE_MAX,
                  "ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE" );
              YYERROR;
          }
      }
    | DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE DKC_EQUAL integer_value
      {
          if ( ( $<mIntegerValue>3 >=
                 DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_MIN ) &&
               ( $<mIntegerValue>3 <=
                 DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_MAX ) )
          {
              (PARAM->mDblinkConf).mAltilinkerJvmBitDataModelValue =
                  $<mIntegerValue>3;
          }
          else
          {
              dkcParserInvalidValueError(
                  &(@3),
                  PARAM,
                  DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_MIN,
                  DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE_MAX,
                  "ALTILINKER_JVM_BIT_DATA_MODEL_VALUE" );
              YYERROR;
          }
      }
    ;

target_statement: DKC_TARGETS DKC_EQUAL DKC_OPENING_PARENTHESIS target_list DKC_CLOSING_PARENTHESIS
    ;

target_list: target_list DKC_COMMA target_item
    | target_item
    ;

target_item: DKC_OPENING_PARENTHESIS target_property_list DKC_CLOSING_PARENTHESIS
      {
          if ( dkcUtilAppendDblinkConfTarget( &(PARAM->mDblinkConf),
                                              &(PARAM->mDblinkConfTarget) )
               != ID_TRUE )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    ;

target_property_list: target_property_list target_property
    | target_property
    ;

target_property: DKC_NAME DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConfTarget).mName) );

          if ( (PARAM->mDblinkConfTarget).mName == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_JDBC_DRIVER DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConfTarget).mJdbcDriver) );

          if ( (PARAM->mDblinkConfTarget).mJdbcDriver == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_JDBC_DRIVER_CLASS_NAME DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConfTarget).mJdbcDriverClassName) );

          if ( (PARAM->mDblinkConfTarget).mJdbcDriverClassName == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_CONNECTION_URL DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConfTarget).mConnectionUrl) );

          if ( (PARAM->mDblinkConfTarget).mConnectionUrl == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_USER DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConfTarget).mUser) );

          if ( (PARAM->mDblinkConfTarget).mUser == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_PASSWORD DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&((PARAM->mDblinkConfTarget).mPassword) );

          if ( (PARAM->mDblinkConfTarget).mPassword == NULL )
          {
              dkcParser_error( &(@1), PARAM, "Memory allocation error" );
              YYERROR;
          }
          else
          {
              /* nothing to do */
          }
      }
    | DKC_XADATASOURCE_CLASS_NAME DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&( ( PARAM->mDblinkConfTarget).mXADataSourceClassName ) );
      }
    | DKC_XADATASOURCE_URL_SETTER_NAME DKC_EQUAL quoted_value
      {
          dkcUtilSetString(
              $<mValue>3.mText,
              $<mValue>3.mLength,
              (SChar **)&( ( PARAM->mDblinkConfTarget).mXADataSourceUrlSetterName ) );
      }
    ;

quoted_value: DKC_QUOTED_VALUE
      {
          /* remove quotation mark */
          $<mValue>$.mText = $<mValue>1.mText + 1;
          $<mValue>$.mLength = $<mValue>1.mLength - 2;
      }
    ;
integer_value: DKC_INTEGER_VALUE
      {
          $<mIntegerValue>$ = idlOS::atoi( $<mValue>1.mText );
      }
    ;

%%

/*
 *
 */
IDE_RC dkcParserCreate( dkcParser ** aParser )
{
    dkcParser * sParser = NULL;
    SInt sStage = 0;
    
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( *sParser ),
                                       (void **)&sParser,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERROR_MEMORY_ALLOC );
    sStage = 1;

    dkcUtilInitializeDblinkConf( &(sParser->mDblinkConf) );
    dkcUtilInitializeDblinkConfTarget( &(sParser->mDblinkConfTarget) );
    
    IDE_TEST( dkcLexerInitialize() != IDE_SUCCESS );
    
    *aParser = sParser;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 1:
            (void)iduMemMgr::free( sParser );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkcParserDestroy( dkcParser * aParser )
{
    IDE_TEST( dkcLexerFinalize() != IDE_SUCCESS );

    dkcUtilFinalizeDblinkConf( &(aParser->mDblinkConf) );
    dkcUtilFinalizeDblinkConfTarget( &(aParser->mDblinkConfTarget) );

    (void)iduMemMgr::free( aParser );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkcParserRun( dkcParser * aParser, dkcDblinkConf * aDblinkConf )
{
    SInt sReturn = 0;
    
    sReturn = dkcParser_parse( (void *)aParser );
    switch ( sReturn )
    {
        case 2: /* memory exhaustion */
        case 1: /* invalid input */
        default:
            IDE_TEST( ID_TRUE );
            break;

        case 0: /* success */
            break;
    }

    dkcUtilMoveDblinkConf( &(aParser->mDblinkConf), aDblinkConf );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_PARSING_DBLINK_CONF_FAILED,
                              aParser->mErrorMessage ) );
    
    return IDE_FAILURE;
}
