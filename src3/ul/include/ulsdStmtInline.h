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

#ifndef _O_ULSD_STMT_INLINE_H_
#define _O_ULSD_STMT_INLINE_H_ 1

#include <ulnStmt.h>

/* PROJ-2598 Shard pilot(shard analyze) */
ACP_INLINE void ulsdStmtSetShardSplitMethod(ulnStmt *aStmt, acp_uint8_t aShardSplitMethod)
{
    aStmt->mShardStmtCxt.mShardSplitMethod = aShardSplitMethod;
}

ACP_INLINE void ulsdStmtSetShardKeyDataType(ulnStmt *aStmt, acp_uint32_t aShardKeyDataType)
{
    aStmt->mShardStmtCxt.mShardKeyDataType = aShardKeyDataType;
}

ACP_INLINE void ulsdStmtSetShardDefaultNodeID(ulnStmt *aStmt, acp_uint32_t aShardDefaultNodeID)
{
    aStmt->mShardStmtCxt.mShardDefaultNodeID = aShardDefaultNodeID;
}

ACP_INLINE void ulsdStmtSetShardRangeInfoCnt(ulnStmt *aStmt, acp_uint16_t aShardRangeInfoCnt)
{
    aStmt->mShardStmtCxt.mShardRangeInfoCnt = aShardRangeInfoCnt;
}

/* PROJ-2646 New shard analyzer */
ACP_INLINE void ulsdStmtSetShardIsShardQuery(ulnStmt *aStmt, acp_bool_t aIsNonShardQuery)
{
    aStmt->mShardStmtCxt.mShardIsShardQuery = aIsNonShardQuery;
}

ACP_INLINE void ulsdStmtSetShardValueCnt(ulnStmt *aStmt, acp_uint16_t aValueCnt)
{
    aStmt->mShardStmtCxt.mShardValueCnt = aValueCnt;
}

/* PROJ-2655 Composite shard key */
ACP_INLINE void ulsdStmtSetShardIsSubKeyExists(ulnStmt *aStmt, acp_uint8_t aIsSubKeyExists)
{
    if ( aIsSubKeyExists == 1 )
    {
        aStmt->mShardStmtCxt.mShardIsSubKeyExists = ACP_TRUE;
    }
    else
    {
        aStmt->mShardStmtCxt.mShardIsSubKeyExists = ACP_FALSE;
    }
}

ACP_INLINE void ulsdStmtSetShardSubSplitMethod(ulnStmt *aStmt, acp_uint8_t aShardSubSplitMethod)
{
    aStmt->mShardStmtCxt.mShardSubSplitMethod = aShardSubSplitMethod;
}

ACP_INLINE void ulsdStmtSetShardSubKeyDataType(ulnStmt *aStmt, acp_uint32_t aShardSubKeyDataType)
{
    aStmt->mShardStmtCxt.mShardSubKeyDataType = aShardSubKeyDataType;
}

ACP_INLINE void ulsdStmtSetShardSubValueCnt(ulnStmt *aStmt, acp_uint16_t aSubValueCnt)
{
    aStmt->mShardStmtCxt.mShardSubValueCnt = aSubValueCnt;
}

/* TASK-7219 Non-shard DML */
ACP_INLINE void ulsdStmtSetPartialExecType(ulnStmt *aStmt, ulsdShardPartialExecType aPartialExecType )
{
    aStmt->mShardStmtCxt.mPartialExecType = aPartialExecType;
}

ACP_INLINE ulsdShardPartialExecType ulsdStmtGetPartialExecType(ulnStmt *aStmt)
{
    return aStmt->mShardStmtCxt.mPartialExecType;
}

#endif /* _O_ULSD_STMT_INLINE_H_ */
