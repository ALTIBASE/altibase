/***********************************************************************
 * Copyright 1999-2012, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>
#include <mtcc.h>

#include <ulsd.h>

static acp_bool_t ulsdIsSupportedStmtAttr( acp_sint32_t  aAttribute,
                                           void         *aValuePtr )
{
    acp_bool_t   sIsSupported = ACP_TRUE;
    acp_uint32_t sValue = (acp_uint32_t)(((acp_ulong_t)aValuePtr) & ACP_UINT32_MAX);

    switch (aAttribute)
    {
        case SQL_ATTR_PARAMSET_SIZE:
            if ( sValue > 1 )
            {
                sIsSupported = ACP_FALSE;
            }
            break;

        /* BUG-49075 샤딩에서는 FAC가 지원되지 않는다. */
        case SQL_ATTR_CURSOR_HOLD:
            if (sValue == SQL_CURSOR_HOLD_ON)
            {
                sIsSupported = ACP_FALSE;
            }
            break;

        /* BUG-47050 */
        case SQL_ATTR_CURSOR_TYPE:
            if (sValue == SQL_CURSOR_KEYSET_DRIVEN)
            {
                sIsSupported = ACP_FALSE;
            }
            break;

        default:
            break;
    }

    return sIsSupported;
}

SQLRETURN ulsdSetStmtAttr(ulnStmt      *aMetaStmt,
                          acp_sint32_t  aAttribute,
                          void         *aValuePtr,
                          acp_sint32_t  aStringLength)
{
    acp_list_node_t   * sNode      = NULL;
    acp_list_node_t   * sNext      = NULL;
    ulsdStmtAttrInfo  * sObj       = NULL;
    ulsdStmtAttrInfo  * sNewObj    = NULL;

    SQLRETURN           sRet = SQL_ERROR;
    ulsdDbc            *sShard;
    ulnFnContext        sFnContext;
    acp_uint16_t        i;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_SETSTMTATTR, aMetaStmt, ULN_OBJ_TYPE_STMT );

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    ACI_TEST_RAISE( ulsdIsSupportedStmtAttr( aAttribute, aValuePtr ) != ACP_TRUE, LABEL_NOT_SUPPORTED );

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACI_TEST_RAISE( acpMemAlloc( (void **) & sNewObj,
                                 ACI_SIZEOF( ulsdStmtAttrInfo ) )
                    != ACP_RC_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

    sNewObj->mAttribute    = aAttribute;
    sNewObj->mValuePtr     = aValuePtr;
    sNewObj->mStringLength = aStringLength;

    acpListInitObj( & sNewObj->mListNode, (void *)sNewObj );

    ulsdGetShardFromDbc(aMetaStmt->mParentDbc, &sShard);

    /* BUG-45411 data노드에서 먼저 수행하고, 모두 성공하면 meta노드에서 수행한다. */
    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        sRet = ulnSetStmtAttr(aMetaStmt->mShardStmtCxt.mShardNodeStmt[i],
                              aAttribute,
                              aValuePtr,
                              aStringLength);
        ACI_TEST_RAISE(sRet != SQL_SUCCESS, LABEL_NODE_SETSTMTATTR_FAIL);

        SHARD_LOG("(Set Stmt Attr) Attr=%d, NodeId=%d, Server=%s:%d\n",
                  aAttribute,
                  sShard->mNodeInfo[i]->mNodeId,
                  sShard->mNodeInfo[i]->mServerIP,
                  sShard->mNodeInfo[i]->mPortNo);
    }

    sRet = ulnSetStmtAttr(aMetaStmt,
                          aAttribute,
                          aValuePtr,
                          aStringLength);
    ACI_TEST(sRet != SQL_SUCCESS);

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACP_LIST_ITERATE_SAFE( & aMetaStmt->mShardStmtCxt.mStmtAttrList, sNode, sNext )
    {
        sObj = (ulsdStmtAttrInfo *)sNode->mObj;
        if ( sObj->mAttribute == aAttribute )
        {
            acpListDeleteNode( sNode );
            acpMemFree( sNode->mObj );
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    acpListAppendNode( & aMetaStmt->mShardStmtCxt.mStmtAttrList,
                       & sNewObj->mListNode );

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION( LABEL_NOT_SUPPORTED )
    {
        ulnError( &sFnContext, ulERR_ABORT_FEATURE_NOT_IMPLEMENTED_IN_SHARD );
    }
    ACI_EXCEPTION(LABEL_NODE_SETSTMTATTR_FAIL)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETSTMTATTR, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulsdNativeErrorToUlnError(&sFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)aMetaStmt->mShardStmtCxt.mShardNodeStmt[i],
                                  sShard->mNodeInfo[i],
                                  "Set Stmt Attr");
    }
    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_SETSTMTATTR, aMetaStmt, ULN_OBJ_TYPE_STMT );

        ulnError( & sFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "SetStmtAttr",
                  "Memory allocation error." );

        sRet = ULN_FNCONTEXT_GET_RC( & sFnContext );
    }
    ACI_EXCEPTION_END;

    if ( sNewObj != NULL )
    {
        acpMemFree( sNewObj );
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;
}

SQLRETURN ulsdSetStmtAttrOnNode( ulnFnContext    * aFnContext,
                                 ulsdStmtContext * aMetaShardStmtCxt,
                                 ulnStmt         * aDataStmt,
                                 ulsdNodeInfo    * aNodeInfo )
{
    acp_list_node_t       * sNode = NULL;
    ulsdStmtAttrInfo      * sAttr = NULL;
    SQLRETURN               sRet  = SQL_ERROR;

    ACP_LIST_ITERATE( & aMetaShardStmtCxt->mStmtAttrList, sNode )
    {
        sAttr = (ulsdStmtAttrInfo *)sNode->mObj;

        sRet = ulnSetStmtAttr( aDataStmt,
                               sAttr->mAttribute,
                               sAttr->mValuePtr,
                               sAttr->mStringLength );

        ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_NODE_SETSTMTATTR_FAIL );
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NODE_SETSTMTATTR_FAIL )
    {
        ulsdNativeErrorToUlnError( aFnContext,
                                   SQL_HANDLE_STMT,
                                   (ulnObject *)aDataStmt,
                                   aNodeInfo,
                                   "Set Stmt Attr" );
    }
    ACI_EXCEPTION_END;

    return sRet;
}
