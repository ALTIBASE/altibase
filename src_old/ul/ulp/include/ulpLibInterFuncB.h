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

#ifndef _ULP_LIB_INTERFUNCB_H_
#define _ULP_LIB_INTERFUNCB_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <ulo.h>
#include <sqlcli.h>
#include <ulpLibInterFuncA.h>
#include <ulpLibInterCoreFuncA.h>
#include <ulpLibInterCoreFuncB.h>
#include <ulpLibStmtCur.h>
#include <ulpLibConnection.h>
#include <ulpLibInterface.h>
#include <ulpLibOption.h>
#include <ulpLibError.h>
#include <ulpLibMacro.h>
#include <ulpTypes.h>
#include <mtcl.h>            /* mtlUTF16, mtlUTF8*/

/* library internal functions */

ACI_RC ulpOpen ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpFetch ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpClose ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpCloseRelease ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpDeclareStmt ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpAutoCommit ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpCommit ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpBatch ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpFree ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpAlterSession ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpFreeLob ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpFailOver ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );


/* XA function */
void   ulpAfterXAOpen ( acp_sint32_t    aRmid,
                        SQLHENV         aEnv,
                        SQLHDBC         aDbc );

void   ulpAfterXAClose ( void );

/* TASK-7218 Handling Multi-Error */
ACI_RC ulpGetStmtDiag ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved );

ACI_RC ulpGetConditionDiag ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *reserved );

#endif
