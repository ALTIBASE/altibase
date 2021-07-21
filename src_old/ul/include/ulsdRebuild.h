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
 * $Id$
 **********************************************************************/

#ifndef ULSD_REBUILD_H_
#define ULSD_REBUILD_H_

#define ULSD_SHARD_RETRY_LOOP_MAX   (10)

typedef enum 
{
    ULSD_STMT_SHARD_RETRY_NONE       = 0, 
    ULSD_STMT_SHARD_VIEW_OLD_RETRY   ,
    ULSD_STMT_SHARD_REBUILD_RETRY    ,    
    ULSD_STMT_SHARD_SMN_PROPAGATION  ,
} ulsdStmtShardRetryType;

SQLRETURN ulsdProcessShardRetryError( ulnFnContext           * aFnContext,
                                      ulnStmt                * aStmt,
                                      ulsdStmtShardRetryType * aRetryType,
                                      acp_sint32_t           * aRetryMax );

void ulsdUpdateShardSession_Silent( ulnDbc       * aMetaDbc,
                                    ulnFnContext * aFnContext );

void ulsdCleanupShardSession( ulnDbc       * aMetaDbc,
                              ulnFnContext * aFnContext );

acp_bool_t ulsdCheckRebuildNoti( ulnDbc * aMetaDbc );

SQLRETURN ulsdCheckDbcSMN( ulnFnContext *aFnContext, ulnDbc * aDbc );

SQLRETURN ulsdCheckStmtSMN( ulnFnContext *aFnContext, ulnStmt * aStmt );

ACI_RC ulsdApplyNodeInfo_OnlyAdd( ulnFnContext  * aFnContext,
                                  ulsdNodeInfo ** aNewNodeInfoArray,
                                  acp_uint16_t    aNewNodeInfoCount,
                                  acp_uint64_t    aShardMetaNumber,
                                  acp_uint8_t     aIsTestEnable );

void ulsdApplyNodeInfo_RemoveOldSMN( ulnDbc  * aDbc );

#endif /* ULSD_REBUILD_H_ */
