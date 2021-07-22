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

#ifndef _UTP_LIB_ERROR_H_
#define _UTP_LIB_ERROR_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <sqlcli.h>
#include <ulpLibInterface.h>
#include <ulpLibConnection.h>
#include <ulpLibMacro.h>
#include <ulpLibInterCoreFuncB.h>

/*************************
 * CLI 리턴 값에 따른 type구분.
 * TYPE1         = 리턴값 { SQL_SUCCESS, SQL_ERROR, SQL_INVALID_HANDLE,
 *                        SQL_SUCCESS_WITH_INFO, SQL_NO_DATA_FOUND }
 * TYPE1_EXECDML = TYPE1에 포함되며 SQLExecute나 SQLExecDirect를 이용해 DML query를 수행 한 경우.
 * TYPE2         = 리턴값 { SQL_SUCCESS, SQL_ERROR, SQL_INVALID_HANDLE, SQL_SUCCESS_WITH_INFO }
 * TYPE3         = 리턴값 { SQL_SUCCESS, SQL_ERROR, SQL_INVALID_HANDLE }
 *************************/
typedef enum
{
    ERR_TYPE1,
    ERR_TYPE1_DML,
    ERR_TYPE2,
    ERR_TYPE3
} ulpLibErrType;

/* 내장 SQL문 수행 결과정보를 저장하는 sqlca, sqlcode, sqlstate를  *
 * 명시적으로 인자로 넘겨준 값으로 설정 해주는 함수.                 *
 * precompiler 내부 에러 설정 담당.                             */
void ulpSetErrorInfo4PCOMP( ulpSqlca          *aSqlca,
                            ulpSqlcode        *aSqlcode,
                            ulpSqlstate       *aSqlstate,
                            const acp_char_t  *aErrMsg,
                            acp_sint32_t       aErrCode,
                            const acp_char_t  *aErrState );

/* 내장 SQL문 수행 결과정보를 ODEBC CLI 함수를 이용해 설정 해주는 함수. *
 * CLI에서 발생한 에러 설정 담당.                                  */
ACI_RC ulpSetErrorInfo4CLI( ulpLibConnNode    *aConnNode,
                            SQLHSTMT           aHstmt,
                            const SQLRETURN    aSqlRes,
                            ulpSqlca          *aSqlca,
                            ulpSqlcode        *aSqlcode,
                            ulpSqlstate       *aSqlstate,
                            ulpLibErrType      aCLIType );

ACI_RC ulpGetConnErrorMsg( ulpLibConnNode *aConnNode,
                           acp_char_t     *aMsg );

#endif

