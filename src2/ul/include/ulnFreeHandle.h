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

#ifndef _O_ULN_FREE_HANDLE_H_
#define _O_ULN_FREE_HANDLE_H_ 1

ACI_RC ulnSFID_26(ulnFnContext *aContext);
ACI_RC ulnSFID_74(ulnFnContext *aContext);
ACI_RC ulnSFID_75(ulnFnContext *aContext);

ACI_RC ulnSFID_71(ulnFnContext *aContext);
ACI_RC ulnSFID_95(ulnFnContext *aContext);

ACI_RC ulnSFID_94(ulnFnContext *aContext);

ACI_RC ulnFreeHandleStmtBody(ulnFnContext *aContext,
                             ulnDbc       *aParentDbc,
                             ulnStmt      *aStmt,
                             acp_bool_t    aSendFreeReq);

ACI_RC ulnFreeHandleDescBody(ulnFnContext *aContext,
                             ulnDbc       *aParentDbc,
                             ulnDesc      *aDesc);

ACI_RC ulnCallbackFreeResult(cmiProtocolContext *aProtocolContext,
                             cmiProtocol        *aProtocol,
                             void               *aServiceSession,
                             void               *aUserContext);

ACI_RC ulnFreeHandleStmtSendFreeREQ(ulnFnContext *aContext,
                                    ulnDbc       *aParentDbc,
                                    acp_uint8_t   aMode);

#endif /* _ULN_FREE_HANDLE_H_ */

