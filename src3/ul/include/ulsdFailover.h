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
#ifndef _O_ULSD_FAILOVER_H_
#define _O_ULSD_FAILOVER_H_ 1

void ulsdSetNextFailoverServer( ulnDbc *sNodeDbc );

void ulsdAlignDataNodeConnection( ulnFnContext * aFnContext,
                                  ulnDbc       * aNodeDbc );

void ulsdGetNodeConnectionReport( ulnDbc *aNodeDbc, ulsdNodeReport *aReport );

ACI_RC ulsdSendNodeConnectionReport( ulnFnContext *aMetaFnContext, ulsdNodeReport *aReport );

ACI_RC ulsdNotifyFailoverOnMeta( ulnDbc *aMetaDbc );

acp_bool_t ulsdIsFailoverExecute( ulnFnContext  * aFnContext );

#endif // _O_ULSD_FAILOVER_H_
