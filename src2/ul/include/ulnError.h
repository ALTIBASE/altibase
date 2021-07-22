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

#ifndef _O_ULN_ERROR_H_
#define _O_ULN_ERROR_H_ 1

#include <uln.h>
#include <ulnPrivate.h>
#include <ulErrorCode.h>

#include <aciErrorMgr.h>

#define ACI_UL_ERROR_SECTION  5

typedef aci_client_error_mgr_t ulnErrorMgr;

/*
 * Function Declarations
 */

/* BUG-39817 */
ACP_EXTERN_C_BEGIN

ACP_INLINE acp_char_t *ulnErrorMgrGetSQLSTATE(ulnErrorMgr  *aManager)
{
    return aManager->mErrorState;
}

ACP_INLINE acp_uint32_t ulnErrorMgrGetErrorCode(ulnErrorMgr  *aManager)
{
    return ACI_E_ERROR_CODE(aManager->mErrorCode);
}

ACP_INLINE acp_char_t *ulnErrorMgrGetErrorMessage(ulnErrorMgr  *aManager)
{
    return aManager->mErrorMessage;
}

void ulnErrorMgrSetUlError( ulnErrorMgr *aErrorMgr, acp_uint32_t aErrorCode, ...);
void ulnErrorMgrSetCmError( ulnDbc       *aDbc,
                            ulnErrorMgr  *aErrorMgr,
                            acp_uint32_t  aCmErrorCode );

ACI_RC ulnError(ulnFnContext *aFnContext, acp_uint32_t aErrorCode, ...);

ACI_RC ulnErrorExtended(ulnFnContext *aFnContext,
                        acp_sint32_t  aRowNumber,
                        acp_sint32_t  aColumnNumber,
                        acp_uint32_t  aErrorCode, ...);

ACI_RC ulnCallbackErrorResult(cmiProtocolContext *aProtocolContext,
                              cmiProtocol        *aProtocol,
                              void               *aServiceSession,
                              void               *aUserContext);

ACI_RC ulnErrHandleCmError(ulnFnContext *aFnContext, ulnPtContext *aPtContext);

SQLRETURN ulnErrorDecideSqlReturnCode(acp_char_t *aSqlState);

/* BUG-39817 */
ACP_EXTERN_C_END

#endif /* _O_ULN_ERROR_H_ */
