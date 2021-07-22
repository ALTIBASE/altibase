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
 

%option 8bit
%option caseless
%option never-interactive
%option noreject
%option noyymore
%option noyywrap
%option nostdinit
%option prefix="dkcLexer"
%option nounput
     
%{
#include <idp.h>
#include <idl.h>
#include <ide.h>
#include <idu.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkc.h>
     
#if defined(BISON_POSTFIX_HPP)
#include "dkcParser.hpp"
#else  /* BISON_POSTFIX_CPP_H */
#include "dkcParser.cpp.h"
#endif

#include <dkcLexer.h>
     
static PDL_HANDLE gHandle;

static SInt gLineNumber = 1;

#define YY_INPUT( buf, result, max_size )                                    \
    {                                                                        \
        if ( gHandle != 0 )                                                  \
        {                                                                    \
            result = idf::read( gHandle, (void *)buf, max_size );            \
        }                                                                    \
    }

#define YY_DECL SInt dkcLexerlex( YYLTYPE * aLocation )

#define SET_CURRENT_LINE do {                           \
        aLocation->first_line = gLineNumber;            \
    } while ( 0 )

%}

%%

ALTILINKER_ENABLE {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_ENABLE;
}

ALTILINKER_PORT_NO {
    
    SET_CURRENT_LINE;
    return DKC_ALTILINKER_PORT_NO;
}

ALTILINKER_RECEIVE_TIMEOUT {
    
    SET_CURRENT_LINE;
    return DKC_ALTILINKER_RECEIVE_TIMEOUT;
}

ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT {
    
    SET_CURRENT_LINE;
    return DKC_ALTILINKER_REMOTE_NODE_RECEIVE_TIMEOUT;
}

ALTILINKER_QUERY_TIMEOUT {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_QUERY_TIMEOUT;
}

ALTILINKER_NON_QUERY_TIMEOUT {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_NON_QUERY_TIMEOUT;
}

ALTILINKER_THREAD_COUNT {
    
    SET_CURRENT_LINE;
    return DKC_ALTILINKER_THREAD_COUNT;
}

ALTILINKER_THREAD_SLEEP_TIME {

    SET_CURRENT_LINE;    
    return DKC_ALTILINKER_THREAD_SLEEP_TIME;
}

ALTILINKER_REMOTE_NODE_SESSION_COUNT {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_REMOTE_NODE_SESSION_COUNT;
}

ALTILINKER_TRACE_LOG_DIR {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_TRACE_LOG_DIR;
}

ALTILINKER_TRACE_LOG_FILE_SIZE {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_TRACE_LOG_FILE_SIZE;
}

ALTILINKER_TRACE_LOG_FILE_COUNT {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_TRACE_LOG_FILE_COUNT;
}

ALTILINKER_TRACE_LOGGING_LEVEL {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_TRACE_LOGGING_LEVEL;
}

ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_JVM_MEMORY_POOL_INIT_SIZE;
}

ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_JVM_MEMORY_POOL_MAX_SIZE;
}

ALTILINKER_JVM_BIT_DATA_MODEL_VALUE {

    SET_CURRENT_LINE;
    return DKC_ALTILINKER_JVM_BIT_DATA_MODEL_VALUE;
}

TARGETS {

    SET_CURRENT_LINE;
    return DKC_TARGETS;
}

NAME {
    SET_CURRENT_LINE;
    return DKC_NAME;
}

JDBC_DRIVER {

    SET_CURRENT_LINE;
    return DKC_JDBC_DRIVER;
}

JDBC_DRIVER_CLASS_NAME {

    SET_CURRENT_LINE;
    return DKC_JDBC_DRIVER_CLASS_NAME;
}

CONNECTION_URL {

    SET_CURRENT_LINE;
    return DKC_CONNECTION_URL;
}

USER {

    SET_CURRENT_LINE;
    return DKC_USER;
}

PASSWORD {

    SET_CURRENT_LINE;
    return DKC_PASSWORD;
}

XADATASOURCE_CLASS_NAME {

    SET_CURRENT_LINE;
    return DKC_XADATASOURCE_CLASS_NAME;
}

XADATASOURCE_URL_SETTER_NAME {

    SET_CURRENT_LINE;
    return DKC_XADATASOURCE_URL_SETTER_NAME;
}

"=" {

    SET_CURRENT_LINE;
    return DKC_EQUAL;
}

"(" {

    SET_CURRENT_LINE;
    return DKC_OPENING_PARENTHESIS;
}

")" {

    SET_CURRENT_LINE;
    return DKC_CLOSING_PARENTHESIS;
}

"," {

    SET_CURRENT_LINE;
    return DKC_COMMA;
}

"\""[^'"]+"\"" {

    SET_CURRENT_LINE;
    return DKC_QUOTED_VALUE;
}

[0-9]+ {

    SET_CURRENT_LINE;
    return DKC_INTEGER_VALUE;
}

"\n\r"|"\n"             { gLineNumber++; }

"\r"|[ \t]+             { }

.             { SET_CURRENT_LINE; return DKC_ERROR; }

%%

/*
 *
 */
static IDE_RC getDblinkConfFilePath( SChar ** aPath )
{
    SChar * sHome = NULL;
    UInt sHomeLength = 0;
    UInt sConfFileLength = 0;
    SChar * sPath = NULL;
    UInt sPathLength = 0;

    sHome = idlOS::getenv( IDP_HOME_ENV );

    IDE_TEST_RAISE( sHome == NULL, ERROR_NO_HOME_DIRECTORY );
    sHomeLength = idlOS::strlen( sHome );
    IDE_TEST_RAISE( sHomeLength == 0, ERROR_NO_HOME_DIRECTORY );

    /* Remove last file separator */
    while ( sHomeLength > 0 )
    {
        if ( ( sHome[ sHomeLength - 1 ] != '/' ) &&
             ( sHome[ sHomeLength - 1 ] != '\\' ) )
        {
            break;
        }
        sHomeLength--;
    }
    sHome[ sHomeLength ] = '\0';

    sConfFileLength = idlOS::strlen( DKC_DBLINK_CONF_FILE );
    sPathLength = sHomeLength + sConfFileLength + 2;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       sPathLength,
                                       (void **)&sPath,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERROR_MEMORY_ALLOC );

    idlOS::snprintf( sPath, sPathLength, "%s%c%s",
                     sHome, IDL_FILE_SEPARATOR, DKC_DBLINK_CONF_FILE );

    *aPath = sPath;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_HOME_DIRECTORY )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_NO_HOME_DIRECTORY ) );
    }
    IDE_EXCEPTION( ERROR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC openDblinkConfFile( void )
{
    SChar * sFilePath = NULL;

    gLineNumber = 1;

    IDE_TEST( getDblinkConfFilePath( &sFilePath ) != IDE_SUCCESS );

    gHandle = idf::open( sFilePath, O_RDONLY );
    IDE_TEST_RAISE( gHandle == PDL_INVALID_HANDLE, ERROR_OPEN_FILE );

    (void)iduMemMgr::free( sFilePath );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_OPEN_FILE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_OPEN_DBLINK_CONF_FAILED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static void closeDblinkConfFile( void )
{
    if ( gHandle != 0 )
    {
        idf::close( gHandle );
        gHandle = 0;
    }
    else
    {
        /* do nothing */
    }
}

/*
 *
 */
IDE_RC dkcLexerInitialize( void )
{
    IDE_TEST( openDblinkConfFile() != IDE_SUCCESS );

    YY_FLUSH_BUFFER;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkcLexerFinalize( void )
{
    closeDblinkConfFile();

    return IDE_SUCCESS;
}

/*
 *
 */
SInt dkcLexerGetNextToken( YYSTYPE * aValue, YYLTYPE * aLocation )
{
    SInt sToken;

    sToken = dkcLexerlex( aLocation );

    aValue->mValue.mText = dkcLexertext;
    aValue->mValue.mLength = dkcLexerleng;

    return sToken;
}
