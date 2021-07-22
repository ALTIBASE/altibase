/** 
 *  Copyright (c) 1999~2018, Altibase Corp. and/or its affiliates. All rights reserved.
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
 

/**********************************************************************
 * $Id$
 **********************************************************************/
#ifndef _O_ULSDN_FAILOVER_H_
#define _O_ULSDN_FAILOVER_H_ 1

SQLRETURN ulsdnGetFailoverIsNeeded( acp_sint16_t   aHandleType,
                                    ulnObject     *aObject,
                                    acp_sint32_t  *aIsNeed );

ACI_RC ulsdnFailoverConnectToSpecificServer( ulnFnContext *aFnContext, ulnFailoverServerInfo *aNewServerInfo );

SQLRETURN ulsdnReconnect( acp_sint16_t aHandleType, ulnObject *aObject );

SQLRETURN ulsdnReconnectWithoutEnter( acp_sint16_t aHandleType, ulnObject *aObject );

void ulsdnRaiseShardNodeFailoverIsNotAvailableError( ulnFnContext *aFnContext );

#endif // _O_ULSDN_FAILOVER_H_
