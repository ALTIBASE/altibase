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
 * $Id: ulsdUtils.c 90169 2021-03-10 05:57:45Z donlet $
 **********************************************************************/

#include <ulsdUtils.h>

/**
 *  ulsdBuildClientTouchNodeArr
 *
 *  Client-side로 touch된 노드들을 DBC의 mTouchNodeArr에 구성한다.
 */
ACI_RC ulsdBuildClientTouchNodeArr(ulnFnContext *aFnContext, ulsdDbc *aShard, acp_uint32_t aStmtType)
{
    acp_rc_t      sRC = ACP_RC_MAX;
    acp_uint32_t  i = 0;
    acp_uint32_t  j = 0;

    aShard->mMetaDbc->mClientTouchNodeCount = 0;

    if ( (aStmtType == ULN_STMT_SAVEPOINT) ||
         (aStmtType == ULN_STMT_ROLLBACK_TO_SAVEPOINT) ||
         (ulnStmtTypeIsSP(aStmtType) == ACP_TRUE) )  /* BUG-48384 */
    {
        for ( i = 0, j = 0; i < aShard->mNodeCount; i++ )
        {
            if ( aShard->mNodeInfo[i]->mTouched == ACP_TRUE )
            {
                if ( aShard->mMetaDbc->mClientTouchNodeArr == NULL )
                {
                    sRC = acpMemCalloc( (void **)&aShard->mMetaDbc->mClientTouchNodeArr,
                                        ULSD_SD_NODE_MAX_COUNT,
                                        ACI_SIZEOF(*aShard->mMetaDbc->mClientTouchNodeArr) );
                    ACI_TEST_RAISE( sRC != ACP_RC_SUCCESS, LABEL_MEM_ALLOC_ERR );
                }

                SHARD_LOG( "(ulsdBuildClientTouchNodeArr) NodeIndex[%d], NodeId=%d, Server=%s:%d\n",
                           i,
                           aShard->mNodeInfo[i]->mNodeId,
                           aShard->mNodeInfo[i]->mServerIP,
                           aShard->mNodeInfo[i]->mPortNo );

                aShard->mMetaDbc->mClientTouchNodeArr[j++] = aShard->mNodeInfo[i]->mNodeId;
                aShard->mMetaDbc->mClientTouchNodeCount += 1;
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_ALLOC_ERR)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulsdBuildClientTouchNodeArr");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 *  ulsdPropagateClientTouchNodeArrToNode
 *  @aNodeStmt
 *  @aMetaDbc
 *
 *  Client-side로 touch된 노드들을 각 노드에 전파한다.
 */
void ulsdPropagateClientTouchNodeArrToNode(ulnDbc *aNodeDbc, ulnDbc *aMetaDbc)
{
    /* BUG-48384 */
    aNodeDbc->mClientTouchNodeCount = aMetaDbc->mClientTouchNodeCount;

    if ( (aMetaDbc->mClientTouchNodeCount > 0) && (aNodeDbc->mClientTouchNodeArr == NULL) )
    {
        ACE_DASSERT( aMetaDbc->mClientTouchNodeArr != NULL );
        aNodeDbc->mClientTouchNodeArr = aMetaDbc->mClientTouchNodeArr;
    }
}
