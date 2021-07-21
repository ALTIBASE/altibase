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

#ifndef _O_ULN_BINDDATA_H_
#define _O_ULN_BINDDATA_H_ 1

ACI_RC ulnBindDataWriteRow(ulnFnContext *aFnContext,
                           ulnPtContext *aPtContext,
                           ulnStmt      *aStmt,
                           acp_uint32_t  aRow);

// bug-28259: ipc needs paramDataInResult
ACI_RC ulnCallbackParamDataInResult(cmiProtocolContext *aPtContext,
                                        cmiProtocol        *aProtocol,
                                        void               *aServiceSession,
                                        void               *aUserContext);

ACI_RC ulnCallbackParamDataOut(cmiProtocolContext *aPtContext,
                                   cmiProtocol        *aProtocol,
                                   void               *aServiceSession,
                                   void               *aUserContext);

ACI_RC ulnCallbackParamDataOutList(cmiProtocolContext *aPtContext,
                                       cmiProtocol        *aProtocol,
                                       void               *aServiceSession,
                                       void               *aUserContext);

ACI_RC ulnCallbackParamDataInListResult(cmiProtocolContext *aPtContext,
                                            cmiProtocol        *aProtocol,
                                            void               *aServiceSession,
                                            void               *aUserContext);

ACI_RC ulnBindDataSetUserIndLenValue(ulnStmt      *aStmt,
                                     acp_uint32_t  aRowNumber);

#endif /* _O_ULN_BINDDATA_H_ */
