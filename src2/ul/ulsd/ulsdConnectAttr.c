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

ACI_RC ulsdCreateConnectAttrInfo( ulnFnContext           * aFnContext,
                                  acp_sint32_t             aAttribute,
                                  void                   * aValuePtr,
                                  acp_sint32_t             aStringLength,
                                  ulsdConnectAttrInfo   ** aConnectAttrInfo )
{
    ulnConnAttrID         sConnAttrID = ULN_CONN_ATTR_MAX;
    ulsdConnectAttrInfo * sNewObj     = NULL;
    acp_sint32_t          sRealLength = 0;

    ACI_TEST_RAISE( acpMemAlloc( (void **) & sNewObj,
                                 ACI_SIZEOF( ulsdConnectAttrInfo ) )
                    != ACP_RC_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

    sNewObj->mAttribute       = aAttribute;
    sNewObj->mValuePtr        = aValuePtr;
    sNewObj->mStringLength    = aStringLength;
    sNewObj->mBufferForString = NULL;

    sConnAttrID = ulnGetConnAttrIDfromSQL_ATTR_ID( aAttribute );

    if ( ( sConnAttrID != ULN_CONN_ATTR_MAX ) &&
         ( aValuePtr != NULL ) )
    {
        if ( ( gUlnConnAttrTable[sConnAttrID].mAttrType == ULN_CONN_ATTR_TYPE_STRING ) ||
             ( gUlnConnAttrTable[sConnAttrID].mAttrType == ULN_CONN_ATTR_TYPE_UPPERCASE_STRING ) )
        {
            if ( aStringLength == SQL_NTS )
            {
                sRealLength = (acp_sint32_t)acpCStrLen( (const acp_char_t *)aValuePtr, ACP_SINT32_MAX );
            }
            else
            {
                sRealLength = aStringLength;
            }

            if ( sRealLength > 0 )
            {
                ACI_TEST_RAISE( acpMemAlloc( (void **) & sNewObj->mBufferForString,
                                             sRealLength + 1 )
                                != ACP_RC_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

                acpMemCpy( (void *) sNewObj->mBufferForString,
                           (const void *) aValuePtr,
                           sRealLength );
                sNewObj->mBufferForString[sRealLength] = '\0';
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    acpListInitObj( & sNewObj->mListNode, (void *)sNewObj );

    *aConnectAttrInfo = sNewObj;

    return ACI_SUCCESS;

    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "AddConnectAttrList",
                  "Memory allocation error." );
    }
    ACI_EXCEPTION_END;

    if ( sNewObj != NULL )
    {
        if ( sNewObj->mBufferForString != NULL )
        {
            acpMemFree( sNewObj->mBufferForString );
        }
        else
        {
            /* Nothing to do */
        }

        acpMemFree( sNewObj );
    }
    else
    {
        /* Nothing to do */
    }

    return ACI_FAILURE;
}

void ulsdDestroyConnectAttrInfo( ulsdConnectAttrInfo * aConnectAttrInfo )
{
    if ( aConnectAttrInfo->mBufferForString != NULL )
    {
        acpMemFree( aConnectAttrInfo->mBufferForString );
    }

    acpMemFree( aConnectAttrInfo );
}

void ulsdRemoveConnectAttrList( ulnDbc           * aMetaDbc,
                                acp_sint32_t       aAttribute )
{
    acp_list_node_t     * sNode       = NULL;
    acp_list_node_t     * sNext       = NULL;
    ulsdConnectAttrInfo * sObj        = NULL;

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACP_LIST_ITERATE_SAFE( & aMetaDbc->mShardDbcCxt.mConnectAttrList, sNode, sNext )
    {
        sObj = (ulsdConnectAttrInfo *)sNode->mObj;
        if ( sObj->mAttribute == aAttribute )
        {
            acpListDeleteNode( sNode );

            ulsdDestroyConnectAttrInfo( sObj );
            sObj = NULL;

            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
}

ACI_RC ulsdSetConnectAttrOnDataNodes( ulnFnContext   * aFnContext,
                                      ulsdDbc        * aShard,
                                      acp_sint32_t     aAttribute,
                                      void           * aValuePtr,
                                      acp_sint32_t     aStringLength )
{
    acp_uint32_t        sNodeArr[ULSD_SD_NODE_MAX_COUNT];
    acp_uint16_t        sNodeCount = 0;
    acp_uint16_t        sNodeIndex;
    acp_uint16_t        i = 0;
    SQLRETURN           sRet = SQL_ERROR;

    ulsdGetTouchedAllNodeList( aShard, sNodeArr, &sNodeCount );

    for ( i = 0; i < sNodeCount; i++ )
    {
        sNodeIndex = sNodeArr[i];

        sRet = ulnSetConnectAttr( aShard->mNodeInfo[sNodeIndex]->mNodeDbc,
                                  aAttribute,
                                  aValuePtr,
                                  aStringLength) ;
        ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_NODE_SETCONNECTATTR_FAIL );

        SHARD_LOG( "(Set Connect Attr) Attr=%d, NodeId=%d, Server=%s:%d\n",
                   aAttribute,
                   aShard->mNodeInfo[sNodeIndex]->mNodeId,
                   aShard->mNodeInfo[sNodeIndex]->mServerIP,
                   aShard->mNodeInfo[sNodeIndex]->mPortNo );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_NODE_SETCONNECTATTR_FAIL)
    {
        ulsdNativeErrorToUlnError( aFnContext,
                                   SQL_HANDLE_DBC,
                                   (ulnObject *)aShard->mNodeInfo[sNodeIndex]->mNodeDbc,
                                   aShard->mNodeInfo[sNodeIndex],
                                   "Set Connect Attr" );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulsdSetConnectAttr(ulnDbc       *aMetaDbc,
                             acp_sint32_t  aAttribute,
                             void         *aValuePtr,
                             acp_sint32_t  aStringLength)
{
    ulsdDbc             * sShard = NULL;
    SQLRETURN             sRet = SQL_ERROR;
    ulnFnContext          sFnContext;
    ulsdConnectAttrInfo * sConnectAttrInfo = NULL;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETCONNECTATTR, aMetaDbc, ULN_OBJ_TYPE_DBC);

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    /* data node */
    /* BUG-45411 touch한 data노드에서 먼저 수행하고, 모두 성공하면 meta노드에서 수행한다. */
    ulsdGetShardFromDbc( aMetaDbc, &sShard );

    ACI_TEST_RAISE( ulsdSetConnectAttrOnDataNodes( &sFnContext,
                                                   sShard,
                                                   aAttribute,
                                                   aValuePtr,
                                                   aStringLength )
                    != ACI_SUCCESS, ERR_SET_CONNECT_ATTR_ON_DATA_NODES );

    /* meta node */
    sRet = ulnSetConnectAttr( aMetaDbc,
                              aAttribute,
                              aValuePtr,
                              aStringLength );
    ACI_TEST( sRet != SQL_SUCCESS );

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACI_TEST_RAISE( ulsdCreateConnectAttrInfo( &sFnContext,
                                               aAttribute,
                                               aValuePtr,
                                               aStringLength,
                                               &sConnectAttrInfo )
                    != ACI_SUCCESS, ERR_CREATE_CONNECT_ATTR_INFO );

    ulsdRemoveConnectAttrList( aMetaDbc, aAttribute );

    acpListAppendNode( &aMetaDbc->mShardDbcCxt.mConnectAttrList,
                       &sConnectAttrInfo->mListNode );

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION( ERR_SET_CONNECT_ATTR_ON_DATA_NODES )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION( ERR_CREATE_CONNECT_ATTR_INFO )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION_END;

    if ( sConnectAttrInfo != NULL )
    {
        ulsdDestroyConnectAttrInfo( sConnectAttrInfo );
        sConnectAttrInfo = NULL;
    }

    return sRet;
}

SQLRETURN ulsdSetConnectAttrOnNode( ulnFnContext   * aFnContext,
                                    ulsdDbcContext * aMetaShardDbcCxt,
                                    ulsdNodeInfo   * aNodeInfo )
{
    acp_list_node_t       * sNode = NULL;
    ulsdConnectAttrInfo   * sAttr = NULL;
    SQLRETURN               sRet  = SQL_ERROR;

    ACP_LIST_ITERATE( & aMetaShardDbcCxt->mConnectAttrList, sNode )
    {
        sAttr = (ulsdConnectAttrInfo *)sNode->mObj;

        if ( sAttr->mBufferForString != NULL )
        {
            sRet = ulnSetConnectAttr( aNodeInfo->mNodeDbc,
                                      sAttr->mAttribute,
                                      sAttr->mBufferForString,
                                      sAttr->mStringLength );
        }
        else
        {
            sRet = ulnSetConnectAttr( aNodeInfo->mNodeDbc,
                                      sAttr->mAttribute,
                                      sAttr->mValuePtr,
                                      sAttr->mStringLength );
        }

        ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_NODE_SETCONNECTATTR_FAIL );
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NODE_SETCONNECTATTR_FAIL )
    {
        ulsdNativeErrorToUlnError( aFnContext,
                                   SQL_HANDLE_DBC,
                                   (ulnObject *)aNodeInfo->mNodeDbc,
                                   aNodeInfo,
                                   "Set Connect Attr" );
    }
    ACI_EXCEPTION_END;

    return sRet;
}

acp_bool_t ulsdIsSetConnectAttr( ulnFnContext     * aFnContext, 
                                 ulnConnAttrID      aConnAttrID )
{
    ulnDbc          * sDbc = NULL;
    acp_bool_t        sIsSet = ACP_TRUE;

    ACE_DASSERT( aFnContext->mObjType == SQL_HANDLE_DBC );

    sDbc = aFnContext->mHandle.mDbc;
    if ( sDbc->mShardDbcCxt.mShardClient == ULSD_SHARD_CLIENT_TRUE )
    {
        switch( sDbc->mShardDbcCxt.mShardSessionType )
        {
            case ULSD_SESSION_TYPE_USER:
                sIsSet = gUlnConnAttrTable[aConnAttrID].mIsSetShardUserConn;
                break;

            case ULSD_SESSION_TYPE_LIB:
                sIsSet = gUlnConnAttrTable[aConnAttrID].mIsSetShardLibConn;
                break;

            case ULSD_SESSION_TYPE_COORD:
            default:
                ACE_DASSERT(0);
                sIsSet = ACP_TRUE;
                break;
        }
    }
    else
    {
        sIsSet = ACP_TRUE;
    }

    return sIsSet;
}
