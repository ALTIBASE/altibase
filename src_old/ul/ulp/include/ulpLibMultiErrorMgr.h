/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ULP_LIB_MULTI_ERROR_MGR_H_
#define _ULP_LIB_MULTI_ERROR_MGR_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <sqlcli.h>
#include <ulnDef.h>
#include <ulpLibErrorMgr.h>
#include <ulpLibMultiErrorMgr.h>
#include <ulpLibInterface.h>
#include <ulpLibMacro.h>

/*
 * TASK-7218 Handling Multiple Errors
 */

typedef struct ulpMultiErrorStack
{
    ulvSLen             mRowNum;
    acp_sint32_t        mColumnNum;
    acp_sint32_t        mSqlcode;
    acp_char_t          mSqlstate[6];
    acp_char_t          mMessage[2048];
} ulpMultiErrorStack;

typedef struct ulpMultiErrorMgr
{
    acp_sint32_t        mErrorNum;
    acp_sint32_t        mStackSize;
    ulpMultiErrorStack *mErrorStack;
} ulpMultiErrorMgr;

ulpMultiErrorMgr    *ulpLibGetMultiErrorMgr( void ); 

void ulpLibInitMultiErrorMgr( ulpMultiErrorMgr *aMultiErrorMgr );

void ulpLibSetMultiErrorMgr( ulpMultiErrorMgr *aMultiErrorMgr );

void ulpLibMultiErrorMgrSetDiagRec( SQLHSTMT            aHstmt,
                              ulpMultiErrorMgr   *aMultiErrorMgr,
                              acp_sint32_t        aRecordNo,
                              ulpSqlcode          aSqlcode,
                              acp_char_t         *aSqlstate,
                              acp_char_t         *aErrMsg );

ACI_RC ulpLibMultiErrorMgrSetNumber( SQLHSTMT          aHstmt,
                                  ulpMultiErrorMgr *aMultiErrMgr );

acp_sint32_t ulpLibMultiErrorMgrGetNumber( ulpMultiErrorMgr *aMultiErrorMgr );

acp_sint32_t ulpLibMultiErrorMgrGetSqlcode( ulpMultiErrorStack *aError );

void ulpLibMultiErrorMgrGetSqlstate( ulpMultiErrorStack *aError,
                                  acp_char_t         *aBuffer,
                                  acp_sint16_t        aBufferSize,
                                  ulvSLen            *aActualLength );

void ulpLibMultiErrorMgrGetMessageText( ulpMultiErrorStack *aError,
                                     acp_char_t         *aBuffer,
                                     acp_sint16_t        aBufferSize,
                                     ulvSLen            *aActualLength );

acp_sint32_t ulpLibMultiErrorMgrGetRowNumber( ulpMultiErrorStack *aError );

acp_sint32_t ulpLibMultiErrorMgrGetColumnNumber( ulpMultiErrorStack *aError );

#endif /* _ULP_LIB_MULTI_ERROR_MGR_H_ */
