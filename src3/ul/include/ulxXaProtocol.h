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

#ifndef _O_ULX_XA_PROTOCOL_H_
#define _O_ULX_XA_PROTOCOL_H_ 1

#include <uln.h>
#include <ulnPrivate.h>
#include <xa.h>

ACI_RC ulxConnectionProtocol(ulnDbc       *aDbc,
                             acp_sint32_t  aOp,
                             acp_sint32_t  aRmid,
                             acp_slong_t   aFlag,
                             acp_sint32_t *aResult);

ACI_RC ulxTransactionProtocol(ulnDbc       *aDbc,
                              acp_sint32_t  aOp,
                              acp_sint32_t  aRmid,
                              acp_slong_t   aFlag,
                              XID          *aXid,
                              acp_sint32_t *aResult);

ACI_RC ulxRecoverProtocol(ulnDbc        *aDbc,
                          acp_sint32_t   aOp,
                          acp_sint32_t   aRmid,
                          acp_slong_t    aFlag,
                          XID          **aXid,
                          acp_sint32_t   aCount,
                          acp_sint32_t  *aResult);

ACI_RC ulxCallbackXaGetResult(cmiProtocolContext *aProtocolContext,
                              cmiProtocol        *sProtocol,
                              void               *aServiceSession,
                              void               *aUserContext);

ACI_RC ulxCallbackXaXid(cmiProtocolContext *aProtocolContext,
                        cmiProtocol        *sProtocol,
                        void               *aServiceSession,
                        void               *aUserContext);
#endif /* _O_ULX_XA_PROTOCOL_H_ */
