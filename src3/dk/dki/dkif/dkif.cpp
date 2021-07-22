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
 * $id$
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <mt.h>

#include <dkDef.h>
#include <dkErrorCode.h>

extern mtfModule dkifRemoteAllocStatement;
extern mtfModule dkifRemoteExecuteStatement;
extern mtfModule dkifRemoteExecuteImmediateInternal;
extern mtfModule dkifRemoteBindVariable;
extern mtfModule dkifRemoteFreeStatement;
extern mtfModule dkifRemoteNextRow;
extern mtfModule dkifRemoteGetColumnValueChar;
extern mtfModule dkifRemoteGetColumnValueInteger;
extern mtfModule dkifRemoteGetColumnValueSmallint;
extern mtfModule dkifRemoteGetColumnValueBigint;
extern mtfModule dkifRemoteGetColumnValueDouble;
extern mtfModule dkifRemoteGetColumnValueReal;
extern mtfModule dkifRemoteGetColumnValueVarchar;
extern mtfModule dkifRemoteGetColumnValueFloat;
extern mtfModule dkifRemoteGetColumnValueDate;
extern mtfModule dkifRemoteGetErrorInfo;
extern mtfModule dkifRemoteAllocStatementBatch;
extern mtfModule dkifRemoteFreeStatementBatch;
extern mtfModule dkifRemoteBindVariableBatch;
extern mtfModule dkifRemoteAddBatch;
extern mtfModule dkifRemoteExecuteBatch;
extern mtfModule dkifRemoteGetResultCountBatch;
extern mtfModule dkifRemoteGetResultBatch;

static const mtfModule * gDatabaseLinkFunction[] =
{
    &dkifRemoteAllocStatement,
    &dkifRemoteExecuteStatement,
    &dkifRemoteExecuteImmediateInternal,
    &dkifRemoteBindVariable,
    &dkifRemoteFreeStatement,
    &dkifRemoteNextRow,
    &dkifRemoteGetColumnValueChar,
    &dkifRemoteGetColumnValueInteger,
    &dkifRemoteGetColumnValueSmallint,
    &dkifRemoteGetColumnValueBigint,
    &dkifRemoteGetColumnValueDouble,
    &dkifRemoteGetColumnValueReal,
    &dkifRemoteGetColumnValueVarchar,
    &dkifRemoteGetColumnValueFloat,
    &dkifRemoteGetColumnValueDate,
    &dkifRemoteGetErrorInfo,
    &dkifRemoteAllocStatementBatch,
    &dkifRemoteFreeStatementBatch,
    &dkifRemoteBindVariableBatch,
    &dkifRemoteAddBatch,
    &dkifRemoteExecuteBatch,
    &dkifRemoteGetResultCountBatch,
    &dkifRemoteGetResultBatch,
    
    NULL
};

/*
 *
 */ 
IDE_RC dkifRegisterFunction( void )
{
    IDE_TEST( mtc::addExtFuncModule( (mtfModule **)
                                     gDatabaseLinkFunction )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}
