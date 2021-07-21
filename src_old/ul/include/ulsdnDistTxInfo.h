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
 

/***********************************************************************
 * $Id: ulsdnDistTxInfo.h 88046 2020-07-14 07:12:07Z donlet $
 **********************************************************************/

#ifndef _O_ULSDN_DIST_TX_INFO_H_
#define _O_ULSDN_DIST_TX_INFO_H_ 1

#include <ulsdDistTxInfo.h>

/* PROJ-2733-DistTxInfo */
SQLRETURN SQL_API ulsdnGetSCN(ulnDbc       *aDbc,
                              acp_uint64_t *aSCN);

SQLRETURN SQL_API ulsdnSetSCN(ulnDbc       *aDbc,
                              acp_uint64_t *aSCN);

SQLRETURN SQL_API ulsdnSetTxFirstStmtSCN(ulnDbc       *aDbc,
                                         acp_uint64_t *aTxFirstStmtSCN);

SQLRETURN SQL_API ulsdnSetTxFirstStmtTime(ulnDbc       *aDbc,
                                          acp_sint64_t  aTxFirstStmtTime);

SQLRETURN SQL_API ulsdnSetDistLevel(ulnDbc        *aHandle,
                                    ulsdDistLevel  aDistLevel);

#endif /* _O_ULSDN_DIST_TX_INFO_H_ */
