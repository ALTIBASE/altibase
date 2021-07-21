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

#ifndef _ULP_LIB_INTERFUNCA_H_
#define _ULP_LIB_INTERFUNCA_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <ulo.h>
#include <sqlcli.h>
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

/* BUG-28209 : AIX 에서 c compiler로 컴파일하면 생성자 호출안됨. */
ACI_RC ulpInitializeConnMgr( void );

ACI_RC ulpSetOptionThread( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

ACI_RC ulpConnect( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

ACI_RC ulpDisconnect( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

ACI_RC ulpRunDirectQuery ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

/* BUG-45779 */
ACI_RC ulpRunDirectQueryCache(acp_char_t *aConnName, ulpSqlstmt *aSqlstmt);

ACI_RC ulpRunDirectQueryUnCache(acp_char_t *aConnName, ulpSqlstmt *aSqlstmt);

ACI_RC ulpExecuteImmediate ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpPrepare ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpBindVariable( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

ACI_RC ulpSetArraySize( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

ACI_RC ulpExecute ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpSelectList ( acp_char_t *aConnName, ulpSqlstmt *aSqlstmt, void *aReserved );

ACI_RC ulpDeclareCursor ( acp_char_t *aConnName, ulpSqlstmt *aInSqlstmt, void *aReserved );

ACI_RC ulpBindParam( ulpLibConnNode *aConnNode,
                     ulpLibStmtNode *aStmtNode,
                     SQLHSTMT       *aHstmt,
                     ulpSqlstmt     *aSqlstmt,
                     acp_bool_t      aIsReBindCheck,
                     ulpSqlca       *aSqlca,
                     ulpSqlcode     *aSqlcode,
                     ulpSqlstate    *aSqlstate ) ;

#endif
