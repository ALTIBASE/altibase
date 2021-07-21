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

#ifndef _O_ULN_EXECUTE_H_
#define _O_ULN_EXECUTE_H_ 1

#include <uln.h>
#include <ulnPrivate.h>

ACI_RC ulnSFID_70(ulnFnContext *aContext);

ACI_RC ulnSFID_19(ulnFnContext *aContext);
ACI_RC ulnSFID_20(ulnFnContext *aContext);
ACI_RC ulnSFID_21(ulnFnContext *aContext);
ACI_RC ulnSFID_22(ulnFnContext *aContext);
ACI_RC ulnSFID_23(ulnFnContext *aContext);

void   ulnExecuteCheckNoData(ulnFnContext *aFnContext);
ACI_RC ulnExecLobPhase(ulnFnContext *aFnContext, ulnPtContext *aPtContext);

ACI_RC ulnExecuteCore(ulnFnContext *aFnContext, ulnPtContext *aPtContext);
ACI_RC ulnExecuteLob(ulnFnContext *aFnContext, ulnPtContext *aPtContext);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC ulnExecuteBeginFetchAsync(ulnFnContext *aFnContext,
                                 ulnPtContext *aPtContext);

ACI_RC ulnExecProcessErrorResult(ulnFnContext *aFnContext, acp_uint32_t aErrorRowNumber);

ACI_RC ulnCallbackExecuteResult(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               *aServiceSession,
                                void               *aUserContext);

/* BUG-45967 Data NodeÀÇ Shard Session Á¤¸® */
ACI_RC ulnCallbackExecuteResultInternal(cmiProtocolContext *aProtocolContext,
                                        void               *aUserContext,
                                        acp_uint32_t        aStatementID,
                                        acp_uint32_t        aRowNumber,
                                        acp_uint16_t        aResultSetCount,
                                        acp_sint64_t        aAffectedRowCount,
                                        acp_sint64_t        aFetchedRowCount,
                                        acp_uint8_t         aIsSimpleSelectExecute);

#define ULN_EXECUTE_CHECK_NO_DATA(aFnContext, aStmt)                    \
    do                                                                  \
    {                                                                   \
        if (ulnStmtGetStatementType((aStmt)) == ULN_STMT_UPDATE ||      \
            ulnStmtGetStatementType((aStmt)) == ULN_STMT_DELETE)        \
        {                                                               \
            if (ulnStmtGetColumnCount((aStmt)) == 0)                    \
            {                                                           \
                if ((aStmt)->mTotalAffectedRowCount == 0)               \
                {                                                       \
                    ULN_FNCONTEXT_SET_RC((aFnContext), SQL_NO_DATA);    \
                }                                                       \
            }                                                           \
        }                                                               \
    } while (0)

#endif /* _O_ULN_EXECUTE_H_ */

