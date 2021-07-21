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

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsd.h>

void ulsdSetNodeInfo( ulsdNodeInfo * aShardNodeInfo,
                      acp_uint32_t   aNodeId,
                      acp_char_t   * aNodeName,
                      acp_char_t   * aServerIP,
                      acp_uint16_t   aPortNo,
                      acp_char_t   * aAlternateServerIP,
                      acp_uint16_t   aAlternatePortNo )
{
    aShardNodeInfo->mSMN    = 0;
    aShardNodeInfo->mNodeId = aNodeId;

    acpMemSet( aShardNodeInfo->mNodeName,
               0,
               ACI_SIZEOF( aShardNodeInfo->mNodeName ) );
    acpMemCpy( (void *)aShardNodeInfo->mNodeName,
               (const void *)aNodeName,
               acpCStrLen( aNodeName, ULSD_MAX_NODE_NAME_LEN ) );

    acpMemSet( aShardNodeInfo->mServerIP,
               0,
               ACI_SIZEOF( aShardNodeInfo->mServerIP ) );
    acpMemCpy( (void *)aShardNodeInfo->mServerIP,
               (const void *)aServerIP,
               acpCStrLen( aServerIP, ULSD_MAX_SERVER_IP_LEN ) );

    aShardNodeInfo->mPortNo = aPortNo;

    acpMemSet( aShardNodeInfo->mAlternateServerIP,
               0,
               ACI_SIZEOF( aShardNodeInfo->mAlternateServerIP ) );
    acpMemCpy( (void *)aShardNodeInfo->mAlternateServerIP,
               (const void *)aAlternateServerIP,
               acpCStrLen( aAlternateServerIP, ULSD_MAX_SERVER_IP_LEN ) );

    aShardNodeInfo->mAlternatePortNo = aAlternatePortNo;

    return;
}

SQLRETURN ulsdNodeInfoFree(ulsdNodeInfo *aShardNodeInfo)
{
    if ( aShardNodeInfo != NULL )
    {
        acpMemFree(aShardNodeInfo);
    }
    else
    {
        /* Do Nothing */
    }

    return SQL_SUCCESS;
}

SQLRETURN ulsdNodeFreeConnect(ulsdDbc *aShard)
{
    SQLRETURN           sRet = SQL_ERROR;
    ulnFnContext        sFnContext;
    acp_uint16_t        i;

    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        sRet = ulnFreeHandle(SQL_HANDLE_DBC, (ulnObject *)aShard->mNodeInfo[i]->mNodeDbc);
        ACI_TEST_RAISE(sRet != ACI_SUCCESS, LABEL_NODE_FREEHANDLE_DBC_FAIL);

        SHARD_LOG("(Free Handle Dbc) NodeId=%d, Server=%s:%d\n",
                  aShard->mNodeInfo[i]->mNodeId,
                  aShard->mNodeInfo[i]->mServerIP,
                  aShard->mNodeInfo[i]->mPortNo);
    }

    return sRet;

    ACI_EXCEPTION(LABEL_NODE_FREEHANDLE_DBC_FAIL)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREEHANDLE_DBC, aShard->mMetaDbc, ULN_OBJ_TYPE_DBC);

        ulsdNativeErrorToUlnError(&sFnContext,
                                  SQL_HANDLE_DBC,
                                  (ulnObject *)aShard->mNodeInfo[i]->mNodeDbc,
                                  aShard->mNodeInfo[i],
                                  "Free Handle Dbc");
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdGetRangeIndexFromHash(ulnStmt        *aMetaStmt,
                                    acp_uint16_t    aValueIndex,
                                    acp_uint32_t    aHashValue,
                                    ulsdRangeIndex *aRangeIndex,
                                    acp_uint16_t   *aRangeIndexCount,
                                    acp_bool_t     *aHasDefaultNode,
                                    acp_bool_t      aIsSubKey,
                                    acp_uint16_t    aFuncId )
{
    ulnFnContext    sFnContext;
    acp_uint16_t    i;
    acp_uint16_t    j;

    acp_uint16_t    sRangeIndexCount = 0;

    acp_uint32_t    sHashMax;
    acp_bool_t      sNewPriorGroup = ACP_TRUE;
    mtdModule      *sPriorKeyModule;
    acp_uint32_t    sPriorKeyDataType;
    mtdValueInfo    sValueInfo1;
    mtdValueInfo    sValueInfo2;

    sRangeIndexCount = *aRangeIndexCount;

    if ( aIsSubKey == ACP_FALSE )
    {
        for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++ )
        {
            sHashMax = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardRange.mHashMax;

            /* BUG-46288 */
            if ( aHashValue < sHashMax )
            {
                for ( j = i; j < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; j++ )
                {
                    if ( sHashMax == aMetaStmt->mShardStmtCxt.mShardRangeInfo[j].mShardRange.mHashMax )
                    {
                        ACI_TEST_RAISE( sRangeIndexCount >= 1000, LABEL_RANGE_MAX_ERROR );

                        aRangeIndex[sRangeIndexCount].mRangeIndex = j;
                        aRangeIndex[sRangeIndexCount].mValueIndex = aValueIndex;
                        sRangeIndexCount++;
                    }
                    else
                    {
                        break;
                    }
                }

                break;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Get prior key module */
        if ( aMetaStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_HASH )
        {
            sPriorKeyDataType = MTD_INTEGER_ID;
        }
        else
        {
            sPriorKeyDataType = aMetaStmt->mShardStmtCxt.mShardKeyDataType;
        }

        ACI_TEST( ulsdMtdModuleById( aMetaStmt,
                                     &sPriorKeyModule,
                                     sPriorKeyDataType,
                                     aFuncId )
                  != SQL_SUCCESS );

        for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++ )
        {
            sHashMax = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardSubRange.mHashMax;

            /* BUG-45462 mShardRange의 값으로 new group을 판단한다. */
            if ( i == 0 )
            {
                sNewPriorGroup = ACP_TRUE;
            }
            else
            {
                sValueInfo1.column = NULL;
                sValueInfo1.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i - 1].mShardRange.mMax;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = NULL;
                sValueInfo2.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardRange.mMax;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                if ( sPriorKeyModule->logicalCompare( &sValueInfo1,
                                                      &sValueInfo2 ) != 0 )
                {
                    sNewPriorGroup = ACP_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }

            /* BUG-46288 */
            if ( ( sNewPriorGroup == ACP_TRUE ) && ( aHashValue < sHashMax ) )
            {
                ACI_TEST_RAISE( sRangeIndexCount >= 1000, LABEL_RANGE_MAX_ERROR );

                aRangeIndex[sRangeIndexCount].mRangeIndex = i;
                aRangeIndex[sRangeIndexCount].mValueIndex = aValueIndex;
                sRangeIndexCount++;

                sNewPriorGroup = ACP_FALSE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }

    if ( sRangeIndexCount == *aRangeIndexCount )
    {
        /* default node가 있다면 설정한다 */
        ACI_TEST_RAISE(aMetaStmt->mShardStmtCxt.mShardDefaultNodeID == ACP_UINT32_MAX,
                       LABEL_NO_NODE_FOUNDED);

        *aHasDefaultNode = ACP_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    *aRangeIndexCount = sRangeIndexCount;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_NO_NODE_FOUNDED)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "No shard node founded by shard key value");
    }
    ACI_EXCEPTION(LABEL_RANGE_MAX_ERROR)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Range overflow");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdGetRangeIndexFromClone(ulnStmt         *aMetaStmt,
                                     acp_uint16_t    *aRangeIndex)
{
    ulsdDbcContext  * sDbcCtx        = &aMetaStmt->mParentDbc->mShardDbcCxt;
    ulsdStmtContext * sStmtCtx       = &aMetaStmt->mShardStmtCxt;
    acp_bool_t        sIsFound       = ACP_FALSE;
    acp_uint32_t      sSeed;
    acp_uint32_t      sTouchNode;
    acp_uint16_t      sRangeIndex;
    acp_uint16_t      i;

    if ( ( sDbcCtx->mShardIsNodeTransactionBegin == ACP_TRUE ) &&
         ( sDbcCtx->mShardOnTransactionNodeIndex < sDbcCtx->mShardDbc->mNodeCount ) )
    {
        // Touch 된 노드가 있다.
        sTouchNode = sDbcCtx->mShardDbc->mNodeInfo
            [ sDbcCtx->mShardOnTransactionNodeIndex ]->mNodeId;

        for ( i = 0; i < sStmtCtx->mShardRangeInfoCnt; i++ )
        {
            if ( sTouchNode == sStmtCtx->mShardRangeInfo[i].mShardNodeID )
            {
                // Touch 된 노드가 수행 가능한 노드목록에 있다.
                sRangeIndex = i;
                sIsFound = ACP_TRUE;

                break;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        // Touch 된 노드가 없다.
        /* Nothing to do. */
    }

    if ( sIsFound == ACP_FALSE )
    {
        sSeed = acpRandSeedAuto();

        sRangeIndex = ( acpRand(&sSeed) % sStmtCtx->mShardRangeInfoCnt );
    }
    else
    {
        /* Nothing to do. */
    }

    (*aRangeIndex) = sRangeIndex;

    return SQL_SUCCESS;
}

SQLRETURN ulsdGetRangeIndexFromRange(ulnStmt        *aMetaStmt,
                                     acp_uint16_t    aValueIndex,
                                     ulsdKeyData    *aShardKeyData,
                                     mtdModule      *aShardKeyModule,
                                     ulsdRangeIndex *aRangeIndex,
                                     acp_uint16_t   *aRangeIndexCount,
                                     acp_bool_t     *aHasDefaultNode,
                                     acp_bool_t      aIsSubKey,
                                     acp_uint16_t    aFuncId)
{
    ulnFnContext    sFnContext;
    mtdValueInfo    sKeyValue;    
    mtdValueInfo    sRangeValue1;
    mtdValueInfo    sRangeValue2;

    acp_uint16_t    sNewPriorGroup = ACP_TRUE;
    mtdModule      *sPriorKeyModule;
    acp_uint32_t    sPriorKeyDataType;
    mtdValueInfo    sValueInfo1;
    mtdValueInfo    sValueInfo2;

    acp_uint16_t    i;
    acp_uint16_t    j;

    acp_uint16_t    sRangeIndexCount = 0;

    sRangeIndexCount = *aRangeIndexCount;


    ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

    sKeyValue.column = NULL;
    sKeyValue.value  = aShardKeyData->mValue;
    sKeyValue.flag   = MTD_OFFSET_USELESS;

    if ( aIsSubKey == ACP_FALSE )
    {
        for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++ )
        {
            /* mtdModule의 compare로 비교한다. */
            sRangeValue1.column = NULL;
            sRangeValue1.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardRange.mMax;
            sRangeValue1.flag   = MTD_OFFSET_USELESS;

            if ( aShardKeyModule->logicalCompare( &sKeyValue,
                                                  &sRangeValue1 ) < 0 )
            {
                for ( j = i; j < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; j++ )
                {
                    sRangeValue2.column = NULL;
                    sRangeValue2.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[j].mShardRange.mMax;
                    sRangeValue2.flag   = MTD_OFFSET_USELESS;

                    if ( aShardKeyModule->logicalCompare( &sRangeValue1,
                                                          &sRangeValue2 ) == 0 )
                    {
                        ACI_TEST_RAISE( sRangeIndexCount >= 1000, LABEL_RANGE_MAX_ERROR );

                        aRangeIndex[sRangeIndexCount].mRangeIndex = j;
                        aRangeIndex[sRangeIndexCount].mValueIndex = aValueIndex;
                        sRangeIndexCount++;
                    }
                    else
                    {
                        break;
                    }
                }

                break;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Get prior key module */
        if ( aMetaStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_HASH )
        {
            sPriorKeyDataType = MTD_INTEGER_ID;
        }
        else
        {
            sPriorKeyDataType = aMetaStmt->mShardStmtCxt.mShardKeyDataType;
        }

        ACI_TEST( ulsdMtdModuleById( aMetaStmt,
                                     &sPriorKeyModule,
                                     sPriorKeyDataType,
                                     aFuncId )
                  != SQL_SUCCESS );

        for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++ )
        {
            /* BUG-45462 mShardRange의 값으로 new group을 판단한다. */
            if ( i == 0 )
            {
                sNewPriorGroup = ACP_TRUE;
            }
            else
            {
                sValueInfo1.column = NULL;
                sValueInfo1.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i - 1].mShardRange.mMax;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = NULL;
                sValueInfo2.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardRange.mMax;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                if ( sPriorKeyModule->logicalCompare( &sValueInfo1,
                                                      &sValueInfo2 ) != 0 )
                {
                    sNewPriorGroup = ACP_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }

            /* mtdModule의 compare로 비교한다. */
            sRangeValue1.column = NULL;
            sRangeValue1.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardSubRange.mMax;
            sRangeValue1.flag   = MTD_OFFSET_USELESS;

            if ( ( sNewPriorGroup == ACP_TRUE ) &&
                 ( aShardKeyModule->logicalCompare( &sKeyValue,
                                                    &sRangeValue1 ) < 0 ) )
            {
                ACI_TEST_RAISE( sRangeIndexCount >= 1000, LABEL_RANGE_MAX_ERROR );

                aRangeIndex[sRangeIndexCount].mRangeIndex = i;
                aRangeIndex[sRangeIndexCount].mValueIndex = aValueIndex;
                sRangeIndexCount++;

                sNewPriorGroup = ACP_FALSE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }

    if ( sRangeIndexCount == *aRangeIndexCount )
    {
        /* default node가 있다면 설정한다 */
        ACI_TEST_RAISE(aMetaStmt->mShardStmtCxt.mShardDefaultNodeID == ACP_UINT32_MAX,
                       LABEL_NO_NODE_FOUNDED);

        *aHasDefaultNode = ACP_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    *aRangeIndexCount = sRangeIndexCount;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_NO_NODE_FOUNDED)
    {
        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "No shard node founded by shard key value");
    }
    ACI_EXCEPTION(LABEL_RANGE_MAX_ERROR)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Range overflow");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdGetRangeIndexFromList(ulnStmt        *aMetaStmt,
                                    acp_uint16_t    aValueIndex,
                                    ulsdKeyData    *aShardKeyData,
                                    mtdModule      *aShardKeyModule,
                                    ulsdRangeIndex *aRangeIndex,
                                    acp_uint16_t   *aRangeIndexCount,
                                    acp_bool_t     *aHasDefaultNode,
                                    acp_bool_t      aIsSubKey,
                                    acp_uint16_t    aFuncId)
{
    ulnFnContext    sFnContext;
    mtdValueInfo    sKeyValue;
    mtdValueInfo    sListValue;

    acp_uint16_t    i;

    acp_uint16_t    sRangeIndexCount = 0;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

    sRangeIndexCount = *aRangeIndexCount;
    
    sKeyValue.column = NULL;
    sKeyValue.value  = aShardKeyData->mValue;
    sKeyValue.flag   = MTD_OFFSET_USELESS;

    sListValue.column = NULL;
    sListValue.flag   = MTD_OFFSET_USELESS;
  
    for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++ )
    {
        /* mtdModule의 compare로 비교한다. */
        if ( aIsSubKey == ACP_FALSE )
        {
            sListValue.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardRange.mMax;
        }
        else
        {
            sListValue.value  = aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardSubRange.mMax;
        }

        if ( aShardKeyModule->logicalCompare( &sKeyValue,
                                              &sListValue ) == 0 )
        {
            ACI_TEST_RAISE( sRangeIndexCount >= 1000, LABEL_RANGE_MAX_ERROR );

            aRangeIndex[sRangeIndexCount].mRangeIndex = i;
            aRangeIndex[sRangeIndexCount].mValueIndex = aValueIndex;

            sRangeIndexCount++;
        }
        else
        {
            /* Nothing to do. */
        }
    }

    if ( sRangeIndexCount == *aRangeIndexCount )
    {
        /* default node가 있다면 설정한다 */
        ACI_TEST_RAISE(aMetaStmt->mShardStmtCxt.mShardDefaultNodeID == ACP_UINT32_MAX,
                       LABEL_NO_NODE_FOUNDED);

        *aHasDefaultNode = ACP_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    *aRangeIndexCount = sRangeIndexCount;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_NO_NODE_FOUNDED)
    {
        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "No shard node founded by shard key value");
    }
    ACI_EXCEPTION(LABEL_RANGE_MAX_ERROR)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Range overflow");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdAddNodeToDbc( ulnFnContext * aFnContext,
                            ulnDbc       * aMetaDbc,
                            ulsdNodeInfo * aNewNodeInfo )
{
    acp_list_node_t * sIterator = NULL;
    ulnStmt         * sStmt     = NULL;
    SQLRETURN         sRet      = SQL_ERROR;

    ACP_LIST_ITERATE( & aMetaDbc->mStmtList, sIterator )
    {
        sStmt = (ulnStmt *)sIterator;

        sRet = ulsdAddNodeToStmt( aFnContext,
                                  sStmt,
                                  aNewNodeInfo );
        ACI_TEST( sRet != SQL_SUCCESS );
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return sRet;
}

void ulsdRemoveNodeFromDbc( ulnDbc       * aMetaDbc,
                            ulsdNodeInfo * aRemoveNodeInfo )
{
    acp_list_node_t * sIterator = NULL;
    ulnStmt         * sStmt     = NULL;

    ACP_LIST_ITERATE( & aMetaDbc->mStmtList, sIterator )
    {
        sStmt = (ulnStmt *)sIterator;

        ulsdRemoveNodeFromStmt( sStmt, aRemoveNodeInfo );
    }

    return;
}

SQLRETURN ulsdAddNode( ulnFnContext * aFnContext,
                       ulnDbc       * aMetaDbc,
                       ulsdNodeInfo * aNewNodeInfo )
{
    ulsdDbc         * sShard       = NULL;
    ulsdNodeInfo   ** sNodeInfoArr = NULL;
    SQLRETURN         sRet         = SQL_ERROR;
    acp_uint16_t      i            = 0;

    ulsdGetShardFromDbc( aMetaDbc, &sShard );

    aNewNodeInfo->mNodeDbc = NULL;

    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemCalloc( (void **) & sNodeInfoArr,
                                                      sShard->mNodeCount + 1,
                                                      ACI_SIZEOF(ulsdNodeInfo *) ) ),
                    LABEL_NOT_ENOUGH_MEMORY );

    sRet = ulsdAllocHandleNodeDbc( aFnContext,
                                   aMetaDbc->mParentEnv,
                                   & aNewNodeInfo->mNodeDbc );
    ACI_TEST( sRet != SQL_SUCCESS );

    ulsdInitalizeNodeDbc( aNewNodeInfo->mNodeDbc, aMetaDbc );

    sRet = ulsdSetConnectAttrOnNode( aFnContext,
                                     & aMetaDbc->mShardDbcCxt,
                                     aNewNodeInfo );
    ACI_TEST( sRet != SQL_SUCCESS );

    sRet = ulsdDriverConnectToNodeInternal( aFnContext,
                                            aNewNodeInfo,
                                            aMetaDbc->mShardDbcCxt.mOrgConnString );
    ACI_TEST( sRet != SQL_SUCCESS );

    sRet = ulsdAddNodeToDbc( aFnContext,
                             aMetaDbc,
                             aNewNodeInfo );
    ACI_TEST( sRet != SQL_SUCCESS );

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        sNodeInfoArr[i] = sShard->mNodeInfo[i];
    }
    sNodeInfoArr[sShard->mNodeCount] = aNewNodeInfo;

    acpMemFree( (void *)sShard->mNodeInfo );
    sShard->mNodeInfo = sNodeInfoArr;

    sShard->mNodeCount++;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "AddNode",
                  "Memory allocation error." );

        sRet = ULN_FNCONTEXT_GET_RC( aFnContext );
    }
    ACI_EXCEPTION_END;

    if ( aNewNodeInfo->mNodeDbc != NULL )
    {
        if ( ulnDbcIsConnected( aNewNodeInfo->mNodeDbc ) == ACP_TRUE )
        {
            (void)ulnDisconnectLocal( aNewNodeInfo->mNodeDbc );
        }
        else
        {
            /* Nothing to do */
        }

        (void)ulnFreeHandle( SQL_HANDLE_DBC, aNewNodeInfo->mNodeDbc );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sNodeInfoArr != NULL )
    {
        acpMemFree( (void *)sNodeInfoArr );
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;
}

void ulsdRemoveNode( ulnDbc       * aMetaDbc,
                     ulsdNodeInfo * aRemoveNodeInfo )
{
    ulsdDbc         * sShard = NULL;
    SQLRETURN         sRet   = SQL_ERROR;
    acp_uint16_t      i      = 0;
    acp_uint16_t      j      = 0;

    ulsdGetShardFromDbc( aMetaDbc, &sShard );

    ulsdRemoveNodeFromDbc( aMetaDbc, aRemoveNodeInfo );

    sRet = ulnDisconnectLocal( aRemoveNodeInfo->mNodeDbc );
    if ( sRet != SQL_SUCCESS )
    {
        SHARD_LOG( "(Disconnect Local) NodeId=%d, Server=%s:%d\n",
                   aRemoveNodeInfo->mNodeId,
                   aRemoveNodeInfo->mServerIP,
                   aRemoveNodeInfo->mPortNo );
    }
    else
    {
        /* Nothing to do */
    }

    sRet = ulnFreeHandle( SQL_HANDLE_DBC, aRemoveNodeInfo->mNodeDbc );
    if ( sRet != SQL_SUCCESS )
    {
        SHARD_LOG( "(Free Handle) NodeId=%d, Server=%s:%d\n",
                   aRemoveNodeInfo->mNodeId,
                   aRemoveNodeInfo->mServerIP,
                   aRemoveNodeInfo->mPortNo );
    }
    else
    {
        /* Nothing to do */
    }

    for ( i = 0; i < sShard->mNodeCount; )
    {
        if ( sShard->mNodeInfo[i]->mNodeId == aRemoveNodeInfo->mNodeId )
        {
            --sShard->mNodeCount;
            for ( j = i; j < sShard->mNodeCount; ++j )
            {
                sShard->mNodeInfo[j] = sShard->mNodeInfo[j + 1];
            }
        }
        else
        {
            /* Nothing to do */
            ++i;
        }
    }

    return;
}
