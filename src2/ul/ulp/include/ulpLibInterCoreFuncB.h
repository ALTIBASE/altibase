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

#ifndef _ULP_LIB_INTERCOREFUNCB_H_
#define _ULP_LIB_INTERCOREFUNCB_H_ 1

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>
#include <ulo.h>
#include <sqlcli.h>
#include <ulpLibStmtCur.h>
#include <ulpLibConnection.h>
#include <ulpLibInterface.h>
#include <ulpLibOption.h>
#include <ulpLibError.h>
#include <ulpLibMacro.h>
#include <ulpTypes.h>
#include <mtcl.h>            /* mtlUTF16, mtlUTF8*/


/* library internal core functions */

ACI_RC ulpSetStmtAttrParamCore( ulpLibConnNode *aConnNode,
                                ulpLibStmtNode *aStmtNode,
                                SQLHSTMT       *aHstmt,
                                ulpSqlstmt     *aSqlstmt,
                                ulpSqlca       *aSqlca,
                                ulpSqlcode     *aSqlcode,
                                ulpSqlstate    *aSqlstate );

ACI_RC ulpSetStmtAttrRowCore(   ulpLibConnNode *aConnNode,
                                ulpLibStmtNode *aStmtNode,
                                SQLHSTMT       *aHstmt,
                                ulpSqlstmt     *aSqlstmt,
                                ulpSqlca       *aSqlca,
                                ulpSqlcode     *aSqlcode,
                                ulpSqlstate    *aSqlstate );

ACI_RC ulpSetStmtAttrDynamicParamCore( ulpLibConnNode *aConnNode,
                                       ulpLibStmtNode *aStmtNode,
                                       SQLHSTMT       *aHstmt,
                                       ulpSqlca       *aSqlca,
                                       ulpSqlcode     *aSqlcode,
                                       ulpSqlstate    *aSqlstate );

ACI_RC ulpSetStmtAttrDynamicRowCore( ulpLibConnNode *aConnNode,
                                     ulpLibStmtNode *aStmtNode,
                                     SQLHSTMT       *aHstmt,
                                     ulpSqlca       *aSqlca,
                                     ulpSqlcode     *aSqlcode,
                                     ulpSqlstate    *aSqlstate );

ACI_RC ulpBindParamCore( ulpLibConnNode *aConnNode,
                         ulpLibStmtNode *aStmtNode,
                         SQLHSTMT       *aHstmt,
                         ulpSqlstmt     *aSqlstmt,
                         ulpHostVar     *aHostVar,
                         acp_sint16_t    aIndex,
                         ulpSqlca       *aSqlca,
                         ulpSqlcode     *aSqlcode,
                         ulpSqlstate    *aSqlstate,
                         SQLSMALLINT     aInOut );

ACI_RC ulpBindColCore( ulpLibConnNode *aConnNode,
                       SQLHSTMT       *aHstmt,
                       ulpSqlstmt     *aSqlstmt,
                       ulpHostVar     *aHostVar,
                       acp_sint16_t    aIndex,
                       ulpSqlca       *aSqlca,
                       ulpSqlcode     *aSqlcode,
                       ulpSqlstate    *aSqlstate );

ACI_RC ulpSetDateFmtCore( ulpLibConnNode *aConnNode );

void ulpSetColRowSizeCore( ulpSqlstmt *aSqlstmt );

/* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
void ulpSetFailoverFlag( ulpLibConnNode *aConnNode );

/* BUG-25643 : apre 에서 ONERR 구문이 잘못 동작합니다. */
ACI_RC ulpGetOnerrErrCodeCore( ulpLibConnNode *aConnNode,
                               ulpSqlstmt     *aSqlstmt,
                               SQLHSTMT       *aHstmt,
                               acp_sint32_t   *aErrCode );

ACI_RC ulpAdjustArraySize(ulpSqlstmt *aSqlstmt);

/* BUG-45779 PSM Array Bind 관련 함수 추가 */
ACI_RC ulpPSMArrayWrite(ulpSqlstmt *aSqlstmt, ulpHostVar *aHostVar, void* aBuffer);

ACI_RC ulpPSMArrayRead(ulpSqlstmt *aSqlstmt, ulpHostVar *aHostVar, void* aBuffer);

ACI_RC ulpPSMArrayHasNullCheck(ulpSqlstmt *aSqlstmt, void* aBuffer);

void ulpPSMArrayMetaFree(ulpPSMArrInfo  *aUlpPSMArrInfo);

#endif
