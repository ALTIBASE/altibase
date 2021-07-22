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
 

/**********************************************************************
 * $Id: ulsdnStmt.h 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/
#ifndef _O_ULSDN_STMT_H_
#define _O_ULSDN_STMT_H_ 1

/* PROJ-2638 shard native linker */
ACP_INLINE acp_char_t * ulsdStmtGetShardTargetDataNodeName(ulnStmt *aStmt)
{
    return aStmt->mShardStmtCxt.mShardTargetDataNodeName;
}

/* PROJ-2638 shard native linker */
ACP_INLINE void ulsdStmtSetShardTargetDataNodeName(ulnStmt *aStmt, acp_uint8_t * aNodeName)
{
    acpSnprintf( aStmt->mShardStmtCxt.mShardTargetDataNodeName,
                 ULSD_MAX_NODE_NAME_LEN + 1,
                 "%s",
                 aNodeName);
}

SQLRETURN ulsdnStmtShardStmtPartialRollback( ulnDbc * aDbc );

ACI_RC ulsdnStmtShardStmtPartialRollbackResult( cmiProtocolContext * aProtocolContext,
                                                cmiProtocol        * aProtocol,
                                                void               * aServiceSession,
                                                void               * aUserContext );

/* TASK-7219 Non-shard DML */
SQLRETURN ulsdnStmtSetPartialExecType( ulnStmt      * aStmt,
                                       acp_sint32_t   aPartialCoordType );

#endif // _O_ULSDN_STMT_H_ 1
