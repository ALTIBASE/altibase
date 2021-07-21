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
 
#ifndef _O_ULSDN_EX_H_
#define _O_ULSDN_EX_H_ 1

SQLRETURN ulsdFetch(ulnStmt *aStmt);
SQLRETURN ulsdExecute( ulnFnContext * aFnContext,
                       ulnStmt      * aStmt );
SQLRETURN ulsdDriverConnect(ulnDbc       *aDbc,
                           acp_char_t   *aConnString,
                           acp_sint16_t  aConnStringLength,
                           acp_char_t   *aOutConnStr,
                           acp_sint16_t  aOutBufferLength,
                           acp_sint16_t *aOutConnectionStringLength);
SQLRETURN ulsdDisconnect(ulnDbc *aDbc);
SQLRETURN ulsdExecuteAndRetry(ulnFnContext *aFnContext,
                              ulnStmt      *aStmt);
#endif /* _O_ULSDN_EX_H_ */
