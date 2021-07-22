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
 * $Id: ulsdDistTxInfo.h 88046 2020-07-14 07:12:07Z donlet $
 **********************************************************************/

#ifndef _O_ULSD_DIST_TX_INFO_H_
#define _O_ULSD_DIST_TX_INFO_H_ 1

#define ULSD_NON_GCTX_TX_FIRST_STMT_SCN (1)  /* BUG-48109 Non-GCTx
                                                SM_SCN_NON_GCTX_TX_FIRST_STMT_SCN */

/* PROJ-2733-DistTxInfo */
typedef enum
{
    ULSD_DIST_LEVEL_INIT     = 0,
    ULSD_DIST_LEVEL_SINGLE   = 1,  /* 1개의 노드에서만 수행된 경우 */
    ULSD_DIST_LEVEL_MULTI    = 2,  /* 순차적으로 2개의 노드에서 수행된 경우 or Serverside로 수행된 경우 */
    ULSD_DIST_LEVEL_PARALLEL = 3   /* 동시에 2개 이상의 노드에서 수행된 경우 */
} ulsdDistLevel;                   /* sdiDistLevel, smiDistLevel와 동일 */

/* PROJ-2733-DistTxInfo */
void ulsdInitDistTxInfo(ulnDbc *aDbc);
void ulsdUpdateSCNToEnv(ulnEnv *aEnv, acp_uint64_t *aSCN);
void ulsdUpdateSCNToDbc(ulnDbc *aDbc, acp_uint64_t *aSCN);
void ulsdUpdateSCN(ulnDbc *aDbc, acp_uint64_t *aSCN);

void ulsdCalcDistTxInfoForMeta(ulnDbc       *aMetaDbc,
                               acp_uint16_t  aExecuteNodeCnt,
                               acp_uint16_t *aNodeDbcIndexArr);
void ulsdCalcDistTxInfoForCoord(ulnDbc *aMetaDbc);
void ulsdPropagateDistTxInfoToNode(ulnDbc *aNodeDbc, ulnDbc *aMetaDbc);

#endif /* _O_ULSD_DIST_TX_INFO_H_ */
