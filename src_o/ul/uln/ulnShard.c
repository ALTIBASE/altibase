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

#include <ulnPrivate.h>
#include <ulsdDef.h>

ACI_RC ulnShardDbcContextInitialize(ulnFnContext *aFnContext, ulnDbc *aDbc)
{
    /* PROJ-2598 altibase sharding */
    aDbc->mShardDbcCxt.mShardDbc                    = NULL;
    aDbc->mShardDbcCxt.mParentDbc                   = NULL;
    aDbc->mShardDbcCxt.mNodeInfo                    = NULL;

    aDbc->mShardDbcCxt.mShardIsNodeTransactionBegin = ACP_FALSE;

    /* PROJ-2638 shard native linker */
    aDbc->mShardDbcCxt.mShardTargetDataNodeName[0]  = '\0';
    aDbc->mShardDbcCxt.mShardLinkerType             = 0;

    /* PROJ-2660 hybrid sharding */
    aDbc->mShardDbcCxt.mShardPin                    = ULSD_SHARD_PIN_INVALID;

    /* BUG-46090 Meta Node SMN 전파 */
    aDbc->mShardDbcCxt.mShardMetaNumber             = 0;

    aDbc->mShardDbcCxt.mSentShardMetaNumber         = 0UL;
    aDbc->mShardDbcCxt.mSentRebuildShardMetaNumber  = 0UL;
    aDbc->mShardDbcCxt.mTargetShardMetaNumber       = 0UL;

    /* BUG-45509 nested commit */
    aDbc->mShardDbcCxt.mCallback                    = NULL;

    /* BUG-45411 */
    aDbc->mShardDbcCxt.mReadOnlyTx                  = ACP_FALSE;

    aDbc->mShardDbcCxt.mShardConnType               = ULN_CONNTYPE_INVALID;

    /* BUG-45707 */
    aDbc->mShardDbcCxt.mShardClient                 = ULSD_SHARD_CLIENT_FALSE;
    aDbc->mShardDbcCxt.mShardSessionType            = ULSD_SESSION_TYPE_USER;

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    aDbc->mShardDbcCxt.mOrgConnString               = NULL;
    acpListInit( & aDbc->mShardDbcCxt.mConnectAttrList );

#ifdef COMPILE_SHARDCLI /* BUG-46092 */
    ulsdAlignInfoInitialize( aDbc );

    /* PROJ-2739 Client-side Sharding LOB */
    acpThrMutexCreate( & aDbc->mShardDbcCxt.mLock4LocatorList, ACP_THR_MUTEX_DEFAULT);
    acpListInit( & aDbc->mShardDbcCxt.mLobLocatorList );

#endif /* COMPILE_SHARDCLI */

    aDbc->mShardDbcCxt.m2PhaseCommitState           = ULSD_2PC_NORMAL;

    aDbc->mShardModule                              = NULL;

    /* PROJ-2733-DistTxInfo */
    aDbc->mShardDbcCxt.mBeforeExecutedNodeDbcIndex  = ACP_UINT16_MAX;

    /* BUG-46814 안정성을 위해 메모리를 할당하자. */
    ACI_TEST_RAISE(acpMemAlloc((void**)&aDbc->mShardDbcCxt.mFuncCallback,
                               ACI_SIZEOF(ulsdFuncCallback)) != ACP_RC_SUCCESS,
                   LABEL_NOT_ENOUGH_MEM);
    aDbc->mShardDbcCxt.mFuncCallback->mInUse = ACP_FALSE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NOT_ENOUGH_MEM)
    {
        ulnError(aFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulnShardDbcContextInitialize");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulnShardDbcContextFinalize( ulnDbc *aDbc )
{
#ifdef COMPILE_SHARDCLI
    /* BUG-46092 */
    ulsdAlignInfoFinalize( aDbc );

    /* PROJ-2738 Client-side Sharding LOB */
    acpThrMutexDestroy( & aDbc->mShardDbcCxt.mLock4LocatorList);
#endif /* COMPILE_SHARDCLI */

    /* BUG-46814 */
    if ( aDbc->mShardDbcCxt.mFuncCallback != NULL )
    {
        acpMemFree( aDbc->mShardDbcCxt.mFuncCallback );
        aDbc->mShardDbcCxt.mFuncCallback = NULL;
    }
}

void ulnShardStmtContextInitialize(ulnStmt *aStmt)
{
    aStmt->mShardStmtCxt.mShardNodeStmt          = NULL;

    /* PROJ-2598 Shard pilot(shard analyze) */
    aStmt->mShardStmtCxt.mShardRangeInfoCnt      = 0;
    aStmt->mShardStmtCxt.mShardRangeInfo         = NULL;

    /* PROJ-2638 shard native linker */
    aStmt->mShardStmtCxt.mRowDataBuffer          = NULL;
    aStmt->mShardStmtCxt.mRowDataBufLen          = 0;
    aStmt->mShardStmtCxt.mIsMtDataFetch          = ACP_FALSE;

    /* PROJ-2646 New shard analyzer */
    aStmt->mShardStmtCxt.mShardValueCnt          = 0;
    aStmt->mShardStmtCxt.mShardValueInfoPtrArray = NULL;
    aStmt->mShardStmtCxt.mShardIsShardQuery      = ACP_FALSE;

    /* PROJ-2655 Composite shard key */
    aStmt->mShardStmtCxt.mShardIsSubKeyExists    = ACP_FALSE;
    aStmt->mShardStmtCxt.mShardSubValueCnt       = 0;
    aStmt->mShardStmtCxt.mShardSubValueInfoPtrArray = NULL;

    /* PROJ-2670 nested execution */
    aStmt->mShardStmtCxt.mCallback               = NULL;

    /* PROJ-2660 hybrid sharding */
    aStmt->mShardStmtCxt.mShardCoordQuery        = ACP_FALSE;

    /* BUG-45499 result merger */
    aStmt->mShardStmtCxt.mNodeDbcIndexCount      = 0;
    aStmt->mShardStmtCxt.mNodeDbcIndexCur        = -1;

    aStmt->mShardModule                          = NULL;

    /* BUG-46100 Session SMN Update */
    aStmt->mShardStmtCxt.mShardMetaNumber         = 0;
    aStmt->mShardStmtCxt.mOrgPrepareTextBuf       = NULL;
    aStmt->mShardStmtCxt.mOrgPrepareTextBufMaxLen = 0;
    aStmt->mShardStmtCxt.mOrgPrepareTextBufLen    = 0;

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    acpListInit( & aStmt->mShardStmtCxt.mStmtAttrList );
    acpListInit( & aStmt->mShardStmtCxt.mBindParameterList );
    acpListInit( & aStmt->mShardStmtCxt.mBindColList );
    aStmt->mShardStmtCxt.mRowCount               = 0;

    /* PROJ-2739 Client-side Sharding LOB */
    aStmt->mShardStmtCxt.mHasLocatorInBoundParam = ACP_FALSE;
    aStmt->mShardStmtCxt.mHasLocatorParamToCopy  = ACP_FALSE;

    /* TASK-7219 Non-shard DML */
    aStmt->mShardStmtCxt.mPartialExecType = ULN_SHARD_PARTIAL_EXEC_TYPE_NONE;
}

void ulnShardStmtFreeAllShardValueInfo( ulnStmt * aStmt )
{
    acp_uint32_t        i = 0;
    ulsdStmtContext   * sShardStmtCxt = NULL;

    sShardStmtCxt = &(aStmt->mShardStmtCxt);

    if ( sShardStmtCxt->mShardValueInfoPtrArray != NULL )
    {
        for ( i = 0; i < sShardStmtCxt->mShardValueCnt; i++ )
        {
            if ( sShardStmtCxt->mShardValueInfoPtrArray[i] != NULL )
            {
                acpMemFree( sShardStmtCxt->mShardValueInfoPtrArray[i] );
                sShardStmtCxt->mShardValueInfoPtrArray[i] = NULL;
            }
        }

        acpMemFree( sShardStmtCxt->mShardValueInfoPtrArray );
        sShardStmtCxt->mShardValueInfoPtrArray = NULL;
        sShardStmtCxt->mShardValueCnt = 0;
    }

    if ( sShardStmtCxt->mShardSubValueInfoPtrArray != NULL )
    {
        for ( i = 0; i < sShardStmtCxt->mShardSubValueCnt; i++ )
        {
            if ( sShardStmtCxt->mShardSubValueInfoPtrArray[i] != NULL )
            {
                acpMemFree( sShardStmtCxt->mShardSubValueInfoPtrArray[i] );
                sShardStmtCxt->mShardSubValueInfoPtrArray[i] = NULL;
            }
        }

        acpMemFree( sShardStmtCxt->mShardSubValueInfoPtrArray );
        sShardStmtCxt->mShardSubValueInfoPtrArray = NULL;
        sShardStmtCxt->mShardSubValueCnt = 0;
    }
}

