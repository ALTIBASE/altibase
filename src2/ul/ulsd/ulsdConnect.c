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
#include <ulnConfigFile.h>
#include <ulnPrivate.h>
#include <mtcc.h>

#include <ulsd.h>
#include <ulsdRebuild.h>


ACI_RC ulsdCallbackUpdateNodeListResult(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *aProtocol,
                                        void               *aServiceSession,
                                        void               *aUserContext)
{
    ulnFnContext           *sFnContext      = (ulnFnContext *)aUserContext;
    ulsdNodeInfo         ** sNodeInfo       = NULL;
    ulnDbc                 *sDbc            = NULL;
    ulsdDbc                *sShard          = NULL;
    acp_uint64_t            sShardMetaNumber = 0;
    acp_uint16_t            sNodeCount = 0;
    acp_uint16_t            i = 0;
    acp_uint16_t            j;
    acp_uint8_t             sLen;

    acp_uint32_t            sNodeId;
    acp_char_t              sNodeName[ULSD_MAX_NODE_NAME_LEN+1];
    acp_char_t              sServerIP[ULSD_MAX_SERVER_IP_LEN];
    acp_uint16_t            sPortNo;
    acp_char_t              sAlternateServerIP[ULSD_MAX_SERVER_IP_LEN];
    acp_uint16_t            sAlternatePortNo;

    /* PROJ-2655 Composite shard key */
    acp_uint8_t             sIsTestEnable = 0;

    acp_uint16_t            sState = 0;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    SHARD_LOG("(Update Node List Result) Shard Node List Result\n");

    CMI_RD1(aProtocolContext, sIsTestEnable);
    CMI_RD2(aProtocolContext, &sNodeCount);

    sDbc = ulnFnContextGetDbc( sFnContext );

    ACI_TEST_RAISE( sDbc == NULL, LABEL_INVALID_SHARD_OBJECT );
    ACI_TEST_RAISE( sDbc->mParentEnv->mShardModule == &gShardModuleNODE, LABEL_INVALID_SHARD_OBJECT );

    ulsdGetShardFromDbc(sDbc, &sShard);

    ACI_TEST_RAISE(sIsTestEnable != sShard->mIsTestEnable, LABEL_INVALID_TEST_MARK);
    ACI_TEST_RAISE(sNodeCount == 0, LABEL_NO_SHARD_NODE);

    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemCalloc( (void **)&sNodeInfo,
                                                      sNodeCount,
                                                      ACI_SIZEOF(ulsdNodeInfo *) ) ),
                    LABEL_NOT_ENOUGH_MEMORY );

    sState = 1;

    for ( i = 0; i < sNodeCount; i++ )
    {
        acpMemSet(sServerIP, 0, ACI_SIZEOF(sServerIP));
        acpMemSet(sAlternateServerIP, 0, ACI_SIZEOF(sAlternateServerIP));

        CMI_RD4(aProtocolContext, &sNodeId);
        CMI_RD1(aProtocolContext, sLen);
        CMI_RCP(aProtocolContext, sNodeName, sLen);
        CMI_RCP(aProtocolContext, sServerIP, ULSD_MAX_SERVER_IP_LEN);
        CMI_RD2(aProtocolContext, &sPortNo);
        CMI_RCP(aProtocolContext, sAlternateServerIP, ULSD_MAX_SERVER_IP_LEN);
        CMI_RD2(aProtocolContext, &sAlternatePortNo);

        sNodeName[sLen] = '\0';

        ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemAlloc( (void **)&(sNodeInfo[i]),
                                                         ACI_SIZEOF(ulsdNodeInfo) ) ),
                        LABEL_NOT_ENOUGH_MEMORY );


        ulsdSetNodeInfo( sNodeInfo[i],
                         sNodeId,
                         sNodeName,
                         sServerIP,
                         sPortNo,
                         sAlternateServerIP,
                         sAlternatePortNo );

        /* run-time info */
        sNodeInfo[i]->mNodeDbc = SQL_NULL_HANDLE;
        sNodeInfo[i]->mTouched = ACP_FALSE;
    }

    CMI_RD8( aProtocolContext, &sShardMetaNumber );
    ulnDbcSetTargetShardMetaNumber( sDbc, sShardMetaNumber );

    ULN_TRACE_LOG( NULL, ULN_TRACELOG_LOW, NULL, 0,
                   "%-18s| ShardMetaNumber received from meta: [%"ACI_UINT64_FMT"]",
                   "ulsdCallbackUpdateNodeListResult", sShardMetaNumber );

    sState = 2;

    for ( j = 0; j < sNodeCount; ++j )
    {
        sNodeInfo[j]->mSMN = sShardMetaNumber;
    }

    ACI_TEST( ulsdApplyNodeInfo_OnlyAdd( sFnContext,
                                         sNodeInfo,
                                         sNodeCount,
                                         sShardMetaNumber,
                                         sIsTestEnable )
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_TEST_MARK)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "UpdateNodeList",
                 "Test mark must not be change.");
    }
    ACI_EXCEPTION(LABEL_NO_SHARD_NODE)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "UpdateNodeList",
                 "No shard node.");
    }
    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError( sFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "UpdateNodeList",
                  "Memory allocation error." );
    }
    ACI_EXCEPTION(LABEL_INVALID_SHARD_OBJECT)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "UpdateNodeList",
                 "Shard object is invalid.");
    }
    ACI_EXCEPTION_END;

    if ( sNodeInfo != NULL )
    {
        for ( i = 0; i < sNodeCount; i++ )
        {
            if ( sNodeInfo[i] != NULL )
            {
                acpMemFree( (void *)sNodeInfo[i] );
            }
            else
            {
                /* Nothing to do */
            }
        }

        acpMemFree( (void *)sNodeInfo );
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-47129
    switch ( sState )
    {
        case 2:
            break;
        case 1:
            i++;
        case 0:
            for ( ; i < sNodeCount; i++ )
            {
                CMI_SKIP_READ_BLOCK(aProtocolContext, 4);                       // sNodeId
                CMI_RD1(aProtocolContext, sLen);                                // sLen
                CMI_SKIP_READ_BLOCK(aProtocolContext, sLen);                    // sNodeName
                CMI_SKIP_READ_BLOCK(aProtocolContext, ULSD_MAX_SERVER_IP_LEN);  // sServerIP
                CMI_SKIP_READ_BLOCK(aProtocolContext, 2);                       // sPortNo
                CMI_SKIP_READ_BLOCK(aProtocolContext, ULSD_MAX_SERVER_IP_LEN);  // sAlternateServerIP
                CMI_SKIP_READ_BLOCK(aProtocolContext, 2);                       // sAlternatePortNo
            }
            CMI_SKIP_READ_BLOCK(aProtocolContext, 8);                           // sShardMetaNumber
            break;
        default:
            ACE_ASSERT(0);
    }

    /* CM 콜백 함수는 communication error가 아닌 한 ACI_SUCCESS를 반환해야 한다.
     * 콜백 에러는 function context에 설정된 값으로 판단한다. */
    return ACI_SUCCESS;
}

ACI_RC ulsdCallbackGetNodeListResult(cmiProtocolContext *aProtocolContext,
                                     cmiProtocol        *aProtocol,
                                     void               *aServiceSession,
                                     void               *aUserContext)
{
    ulnFnContext  * sFnContext       = (ulnFnContext *)aUserContext;
    ulsdNodeInfo ** sNodeInfo        = NULL;
    acp_uint64_t    sShardPin        = ULSD_SHARD_PIN_INVALID;
    acp_uint64_t    sShardMetaNumber = 0;
    acp_uint16_t    sNodeCount       = 0;
    acp_uint8_t     sLen             = 0;
    acp_uint16_t    i                = 0;
    acp_uint16_t    j;

    acp_uint32_t    sNodeId;
    acp_char_t      sNodeName[ULSD_MAX_NODE_NAME_LEN+1];
    acp_char_t      sServerIP[ULSD_MAX_SERVER_IP_LEN];
    acp_uint16_t    sPortNo;
    acp_char_t      sAlternateServerIP[ULSD_MAX_SERVER_IP_LEN];
    acp_uint16_t    sAlternatePortNo;

    /* PROJ-2655 Composite shard key */
    acp_uint8_t     sIsTestEnable = 0;

    acp_uint16_t    sState = 0;
    ulnDbc        * sMetaDbc        = NULL;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    SHARD_LOG("(Get Node List Result) Shard Node List Result\n");

    sMetaDbc = ulnFnContextGetDbc( sFnContext );

    CMI_RD1(aProtocolContext, sIsTestEnable);
    CMI_RD2(aProtocolContext, &sNodeCount);
    
    ACI_TEST_RAISE(sIsTestEnable > 1, LABEL_INVALID_TEST_MARK);
    ACI_TEST_RAISE(sNodeCount == 0, LABEL_NO_SHARD_NODE);

    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemCalloc( (void **)&sNodeInfo,
                                                      sNodeCount,
                                                      ACI_SIZEOF(ulsdNodeInfo *) ) ),
                    LABEL_NOT_ENOUGH_MEMORY );

    sState = 1;

    for ( i = 0; i < sNodeCount; i++ )
    {
        acpMemSet(sServerIP, 0, ACI_SIZEOF(sServerIP));
        acpMemSet(sAlternateServerIP, 0, ACI_SIZEOF(sAlternateServerIP));

        CMI_RD4(aProtocolContext, &sNodeId);
        CMI_RD1(aProtocolContext, sLen);
        CMI_RCP(aProtocolContext, sNodeName, sLen);
        CMI_RCP(aProtocolContext, sServerIP, ULSD_MAX_SERVER_IP_LEN);
        CMI_RD2(aProtocolContext, &sPortNo);
        CMI_RCP(aProtocolContext, sAlternateServerIP, ULSD_MAX_SERVER_IP_LEN);
        CMI_RD2(aProtocolContext, &sAlternatePortNo);

        sNodeName[sLen] = '\0';

        ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemAlloc( (void **)&(sNodeInfo[i]),
                                                         ACI_SIZEOF(ulsdNodeInfo) ) ),
                        LABEL_NOT_ENOUGH_MEMORY );

        ulsdSetNodeInfo( sNodeInfo[i],
                         sNodeId,
                         sNodeName,
                         sServerIP,
                         sPortNo,
                         sAlternateServerIP,
                         sAlternatePortNo );

        /* run-time info */
        sNodeInfo[i]->mNodeDbc = SQL_NULL_HANDLE;
        sNodeInfo[i]->mTouched = ACP_FALSE;
    }

    CMI_RD8( aProtocolContext, &sShardPin );

    ULN_TRACE_LOG( NULL, ULN_TRACELOG_LOW, NULL, 0,
                   "%-18s| ShardPin received from meta: ["ULSD_SHARD_PIN_FORMAT_STR"]",
                   "ulsdCallbackGetNodeListResult", ULSD_SHARD_PIN_FORMAT_ARG( sShardPin ) );

    CMI_RD8( aProtocolContext, &sShardMetaNumber );

    ULN_TRACE_LOG( NULL, ULN_TRACELOG_LOW, NULL, 0,
                   "%-18s| ShardMetaNumber received from meta: [%"ACI_UINT64_FMT"]",
                   "ulsdCallbackGetNodeListResult", sShardMetaNumber );

    sState = 2;

    for ( j = 0; j < sNodeCount; ++j )
    {
        sNodeInfo[j]->mSMN = sShardMetaNumber;
    }

    ulnDbcSetShardPin( sMetaDbc, sShardPin );

    /* PROJ-2745
     * GetNodeList 는 shardcli 가 최초 접속시에만 수행해야 한다.
     * 따라서 기존 분산 정보가 없기 때문에
     * ulsdApplyNodeInfo_RemoveOldSMN 함수를 호출하지 않는다.
     */
    ACE_DASSERT( sMetaDbc->mShardDbcCxt.mShardDbc->mNodeCount == 0 );
    ACI_TEST( ulsdApplyNodeInfo_OnlyAdd( sFnContext,
                                         sNodeInfo,
                                         sNodeCount,
                                         sShardMetaNumber,
                                         sIsTestEnable )
              != ACI_SUCCESS );

    ulnDbcSetShardMetaNumber( sMetaDbc, sShardMetaNumber );
    ulnDbcSetSentShardMetaNumber( sMetaDbc, sShardMetaNumber );
    ulnDbcSetSentRebuildShardMetaNumber( sMetaDbc, sShardMetaNumber );
    ulnDbcSetTargetShardMetaNumber( sMetaDbc, sShardMetaNumber );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_TEST_MARK)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "GetNodeList",
                 "Invalid test mark.");
    }
    ACI_EXCEPTION(LABEL_NO_SHARD_NODE)
    {
        ulnError(sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "GetNodeList",
                 "No shard node.");
    }
    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError( sFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "GetNodeList",
                  "Memory allocation error." );
    }
    ACI_EXCEPTION_END;

    if ( sNodeInfo != NULL )
    {
        for ( i = 0; i < sNodeCount; i++ )
        {
            if ( sNodeInfo[i] != NULL )
            {
                acpMemFree( (void *)sNodeInfo[i] );
            }
            else
            {
                /* Nothing to do */
            }
        }

        acpMemFree( (void *)sNodeInfo );
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-47129
    switch ( sState )
    {
        case 2:
            break;
        case 1:
            i++;
        case 0:
            for ( ; i < sNodeCount; i++ )
            {
                CMI_SKIP_READ_BLOCK(aProtocolContext, 4);                       // sNodeId
                CMI_RD1(aProtocolContext, sLen);                                // sLen
                CMI_SKIP_READ_BLOCK(aProtocolContext, sLen);                    // sNodeName
                CMI_SKIP_READ_BLOCK(aProtocolContext, ULSD_MAX_SERVER_IP_LEN);  // sServerIP
                CMI_SKIP_READ_BLOCK(aProtocolContext, 2);                       // sPortNo
                CMI_SKIP_READ_BLOCK(aProtocolContext, ULSD_MAX_SERVER_IP_LEN);  // sAlternateServerIP
                CMI_SKIP_READ_BLOCK(aProtocolContext, 2);                       // sAlternatePortNo
            }
            CMI_SKIP_READ_BLOCK(aProtocolContext, 8);                           // sSharPin
            CMI_SKIP_READ_BLOCK(aProtocolContext, 8);                           // sShardMetaNumber
            break;
        default:
            ACE_ASSERT(0);
    }

    /* CM 콜백 함수는 communication error가 아닌 한 ACI_SUCCESS를 반환해야 한다.
     * 콜백 에러는 function context에 설정된 값으로 판단한다. */
    return ACI_SUCCESS;
}

ACI_RC ulsdUpdateNodeList(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    ulnDbc             *sDbc = NULL;
    cmiProtocolContext *sCtx = &aPtContext->mCmiPtContext;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    ACI_TEST_RAISE( sDbc == NULL, INVALID_HANDLE_EXCEPTION );

    ACI_TEST(ulsdUpdateNodeListRequest(aFnContext,aPtContext) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    if ( cmiGetLinkImpl( sCtx ) == CMI_LINK_IMPL_IPCDA )
    {
        ACI_TEST( ulnReadProtocolIPCDA( aFnContext,
                                        aPtContext,
                                        sDbc->mConnTimeoutValue )
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( ulnReadProtocol( aFnContext,
                                   aPtContext,
                                   sDbc->mConnTimeoutValue )
                  != ACI_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( INVALID_HANDLE_EXCEPTION )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulsdGetNodeList(ulnDbc *aDbc,
                       ulnFnContext *aFnContext,
                       ulnPtContext *aPtContext)
{
    cmiProtocolContext *sCtx = &aPtContext->mCmiPtContext;

    /*
     * Shard Node List Request 전송
     */
    ACI_TEST(ulsdGetNodeListRequest(aFnContext, aPtContext) != ACI_SUCCESS);

    ACI_TEST(ulnFlushProtocol(aFnContext, aPtContext) != ACI_SUCCESS);

    if ( cmiGetLinkImpl( sCtx ) == CMI_LINK_IMPL_IPCDA )
    {
        ACI_TEST( ulnReadProtocolIPCDA( aFnContext,
                                        aPtContext,
                                        aDbc->mConnTimeoutValue )
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( ulnReadProtocol( aFnContext,
                                   aPtContext,
                                   aDbc->mConnTimeoutValue ) != ACI_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulsdNodeDriverConnect(ulnDbc       *aMetaDbc,
                                ulnFnContext *aFnContext,
                                acp_char_t   *aConnString,
                                acp_sint16_t  aConnStringLength)
{
    ACI_TEST( ulsdGetNodeList( aMetaDbc,
                               aFnContext,
                               &(aMetaDbc->mPtContext ) )
            != ACI_SUCCESS );

    /* Server에서 Error Protocol로 응답하는 경우에도 ACI_SUCCESS를 반환합니다. */
    ACI_TEST( ULN_FNCONTEXT_GET_RC( aFnContext ) != SQL_SUCCESS );

    ACI_TEST( ulsdDriverConnectToNode( aMetaDbc,
                                       aFnContext,
                                       aConnString,
                                       aConnStringLength )
            != SQL_SUCCESS );

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    /* To make Success ulnExit()
     * and Disconnect Meta node connection by ulnDisconnect() */
    ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS_WITH_INFO );

    return SQL_ERROR;
}

SQLRETURN ulsdDriverConnectToNode(ulnDbc       *aMetaDbc,
                                  ulnFnContext *aFnContext,
                                  acp_char_t   *aConnString,
                                  acp_sint16_t  aConnStringLength)
{
    ulsdDbc            *sShard;
    ulsdNodeInfo      **sShardNodeInfo;
    acp_char_t         *sOrgConnString = NULL;
    acp_uint16_t        i;

    sShard = aMetaDbc->mShardDbcCxt.mShardDbc;

    ACI_TEST(sShard->mNodeCount == 0);
    ACI_TEST(sShard->mNodeInfo == NULL);

    sShardNodeInfo = sShard->mNodeInfo;

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        ACI_TEST(ulsdAllocHandleNodeDbc(aFnContext,
                                        aMetaDbc->mParentEnv,
                                        &(sShardNodeInfo[i]->mNodeDbc))
                 != SQL_SUCCESS);

        ulsdInitalizeNodeDbc(sShardNodeInfo[i]->mNodeDbc, aMetaDbc);
    }

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        ACI_TEST( ulsdDriverConnectToNodeInternal( aFnContext,
                                                   sShardNodeInfo[i],
                                                   aConnString )
                  != SQL_SUCCESS );
    }

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemAlloc( (void **) &sOrgConnString,
                                                     aConnStringLength + 1 ) ),
                    LABEL_NOT_ENOUGH_MEMORY );

    acpMemCpy( (void *)sOrgConnString, (const void *)aConnString, aConnStringLength );
    sOrgConnString[aConnStringLength] = '\0';

    aMetaDbc->mShardDbcCxt.mOrgConnString = sOrgConnString;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "Driver Connect To Node",
                  "Memory allocation error." );
    }
    ACI_EXCEPTION_END;

    if ( sOrgConnString != NULL )
    {
        acpMemFree( sOrgConnString );
        sOrgConnString = NULL;
    }

    return SQL_ERROR;
}

ACI_RC ulsdSetConnAttrForLibConn( ulnFnContext  * aFnContext )
{
    ulnDbc        * sNodeDbc = NULL;
    ulnDbc        * sMetaDbc = NULL;
    ulsdNodeInfo  * sNodeInfo = NULL;
    ulsdDbc       * sShard = NULL;
    acp_uint32_t    sLength = 0;
    ulnConnType     sShardConnType = ULN_CONNTYPE_INVALID;
    acp_uint8_t     sAttrGlobalTransactionLevel = ULN_GLOBAL_TX_NONE;

    acp_char_t      sAlternateServer[ULSD_MAX_ALTERNATE_SERVERS_LEN+1] = { 0, };

    ACE_DASSERT( aFnContext->mObjType == SQL_HANDLE_DBC );

    sNodeDbc = aFnContext->mHandle.mDbc;

    sMetaDbc = sNodeDbc->mShardDbcCxt.mParentDbc;
    ulsdGetShardFromDbc( sMetaDbc, &sShard );

    sNodeInfo = sNodeDbc->mShardDbcCxt.mNodeInfo;

    sShardConnType = ulnDbcGetShardConnType( sMetaDbc );
    if ( sShardConnType == ULN_CONNTYPE_INVALID )
    {
        // 사용자가 명시하지 않은 경우
        sShardConnType = ULN_CONNTYPE_TCP;
    }

    ACI_TEST( ulnSetConnAttrConnType( aFnContext, sShardConnType ) != ACI_SUCCESS );

    ACI_TEST( ulnDbcSetHostNameString( sNodeDbc,
                                       sNodeInfo->mServerIP,
                                       acpCStrLen( sNodeInfo->mServerIP, ULSD_MAX_SERVER_IP_LEN ) )
              != ACI_SUCCESS );

    ACI_TEST( ulnDbcSetPortNumber( sNodeDbc,
                                   sNodeInfo->mPortNo )
              != ACI_SUCCESS );

    if ( ( sNodeInfo->mAlternateServerIP[0] != '\0' ) && 
         ( sNodeInfo->mAlternatePortNo != 0 ) )
    {
        ACI_TEST( acpSnprintf( sAlternateServer,
                               ULSD_MAX_ALTERNATE_SERVERS_LEN+1,
                               "(%s:%"ACI_INT32_FMT")",
                               sNodeInfo->mAlternateServerIP,
                               sNodeInfo->mAlternatePortNo )
                  != ACI_SUCCESS );
        ACI_TEST( ulnDbcSetAlternateServers( sNodeDbc,
                                             sAlternateServer,
                                             acpCStrLen( sAlternateServer,
                                                         ULSD_MAX_ALTERNATE_SERVERS_LEN ) )
                  != ACI_SUCCESS );
    }

    ulnDbcSetShardPin( sNodeDbc,
                       sMetaDbc->mShardDbcCxt.mShardPin );

    ulnDbcSetShardMetaNumber( sNodeDbc,
                              sMetaDbc->mShardDbcCxt.mShardMetaNumber );

    sLength = acpCStrLen( sNodeInfo->mNodeName, ULSD_MAX_NODE_NAME_LEN );
    acpMemCpy( sNodeDbc->mShardDbcCxt.mShardTargetDataNodeName,
               sNodeInfo->mNodeName,
               sLength );
    sNodeDbc->mShardDbcCxt.mShardTargetDataNodeName[sLength] = '\0';

    if ( sShard->mIsTestEnable == ACP_TRUE )
    {
        ACI_TEST( ulnDbcSetUserName( sNodeDbc,
                                     sNodeInfo->mNodeName,
                                     sLength )
                  != ACI_SUCCESS );

        ACI_TEST( ulnDbcSetPassword( sNodeDbc,
                                     sNodeInfo->mNodeName,
                                     sLength )
                  != ACI_SUCCESS );
    }

    /* BUG-48225 */
    sAttrGlobalTransactionLevel = ulnDbcGetGlobalTransactionLevel( sMetaDbc );
    ulnDbcSetGlobalTransactionLevel( sNodeDbc, sAttrGlobalTransactionLevel );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulsdDriverConnectToNodeInternal( ulnFnContext * aFnContext,
                                           ulsdNodeInfo * aNodeInfo,
                                           acp_char_t   * aConnString )
{
    SQLRETURN       sRc  = SQL_ERROR;

    aNodeInfo->mNodeDbc->mShardDbcCxt.mNodeInfo = aNodeInfo;

    /* BUG-45707 */
    ulsdDbcSetShardCli( aNodeInfo->mNodeDbc, ULSD_SHARD_CLIENT_TRUE );
    ulsdDbcSetShardSessionType( aNodeInfo->mNodeDbc, ULSD_SESSION_TYPE_LIB );

    sRc = ulnDriverConnect( aNodeInfo->mNodeDbc,
                            aConnString,
                            SQL_NTS,
                            NULL,
                            0,
                            NULL );
    ACI_TEST_RAISE( sRc != SQL_SUCCESS, LABEL_NODE_CONNECTION_FAIL );

    SHARD_LOG( "(Driver Connect) NodeId=%d, Server=%s:%d\n",
               aNodeInfo->mNodeId,
               aNodeInfo->mServerIP,
               aNodeInfo->mPortNo );

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NODE_CONNECTION_FAIL )
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_DBC,
                                  (ulnObject *)aNodeInfo->mNodeDbc,
                                  aNodeInfo,
                                  (acp_char_t *)"SQLDriverConnect");
    }
    ACI_EXCEPTION_END;

    return sRc;
}

SQLRETURN ulsdAllocHandleNodeDbc(ulnFnContext  *aFnContext,
                                 ulnEnv        *aEnv,
                                 ulnDbc       **aDbc)
{
    ACI_TEST(ulnAllocHandle(SQL_HANDLE_DBC,
                            aEnv,
                            (void **)aDbc) != ACI_SUCCESS);

    /* Node Dbc 는 Env 에 매달리면 안되므로 여기서 제거함 */
    ACI_TEST_RAISE(ulnEnvRemoveDbc(aEnv,
                                   (*aDbc)) != ACI_SUCCESS,
                   LABEL_MEM_MAN_ERR);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MAN_ERR)
    {
        ulnError(aFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "Freeing DBC handle.");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

void ulsdInitalizeNodeDbc(ulnDbc      *aNodeDbc,
                          ulnDbc      *aMetaDbc)
{
    aNodeDbc->mParentEnv = aMetaDbc->mParentEnv;

    /* BUG-45967 Data Node의 Shard Session 정리 */
    aNodeDbc->mShardDbcCxt.mParentDbc = aMetaDbc;

    ulsdSetDbcShardModule(aNodeDbc, &gShardModuleNODE);
}

void ulsdMakeNodeAlternateServersString(ulsdNodeInfo      *aShardNodeInfo,
                                        acp_char_t        *aAlternateServers,
                                        acp_sint32_t       aMaxAlternateServersLen)
{
    if ( ( aShardNodeInfo->mAlternateServerIP[0] == '\0' ) ||
         ( aShardNodeInfo->mAlternatePortNo == 0 ) )
    {
        acpSnprintf(aAlternateServers,
                    aMaxAlternateServersLen,
                    "(%s:%d)",
                    aShardNodeInfo->mServerIP,
                    aShardNodeInfo->mPortNo);
    }
    else
    {
        acpSnprintf(aAlternateServers,
                    aMaxAlternateServersLen,
                    "(%s:%d,%s:%d)",
                    aShardNodeInfo->mServerIP,
                    aShardNodeInfo->mPortNo,
                    aShardNodeInfo->mAlternateServerIP,
                    aShardNodeInfo->mAlternatePortNo);
    }
}

SQLRETURN ulsdDriverConnect(ulnDbc       *aDbc,
                            acp_char_t   *aConnString,
                            acp_sint16_t  aConnStringLength,
                            acp_char_t   *aOutConnStr,
                            acp_sint16_t  aOutBufferLength,
                            acp_sint16_t *aOutConnectionStringLength)
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;
    acp_char_t     sBackupErrorMessage[ULSD_MAX_ERROR_MESSAGE_LEN];

    sRet = ulnDriverConnect(aDbc,
                            aConnString,
                            aConnStringLength,
                            aOutConnStr,
                            aOutBufferLength,
                            aOutConnectionStringLength);

    ACI_TEST_RAISE(sRet == SQL_SUCCESS_WITH_INFO, LABEL_SUCCESS_WITH_INFO);

    return sRet;

    ACI_EXCEPTION(LABEL_SUCCESS_WITH_INFO)
    {
        if ( ulnGetDiagRec(SQL_HANDLE_DBC,
                           (ulnObject *)aDbc,
                           1,
                           NULL,
                           NULL,
                           sBackupErrorMessage, 
                           ULSD_MAX_ERROR_MESSAGE_LEN,
                           NULL,
                           ACP_FALSE) != SQL_NO_DATA )
        {
            /*
             * ulsdSilentDisconnect() 내 ulnEnter() 호출 후 MetaDbc 의 Diagnostic Record 가 초기화 되기 때문에
             * 먼저 에러 메세지를 sBackupErrorMessage 에 백업한 다음 나중에 ulnError() 로 MetaDbc 에 추가한다.
             * 반드시 아래 순서대로 호출 해야한다.
             *
             * 1. ulsdSilentDisconnect()
             * 2. ulnError()
             */
            ulsdSilentDisconnect(aDbc);

            ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_DRIVERCONNECT, aDbc, ULN_OBJ_TYPE_DBC);

            ulnError(&sFnContext,
                     ulERR_ABORT_SHARD_DATA_NODE_CONNECTION_FAILURE,
                     sBackupErrorMessage);
        }
        else
        {
            ulsdSilentDisconnect(aDbc);
        }
        sRet = SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    return sRet;
}

acp_bool_t ulsdHasNoData( ulnDbc * aMetaDbc )
{
    acp_list_node_t * sIterator = NULL;
    ulnStmt         * sStmt     = NULL;

    ACP_LIST_ITERATE( & aMetaDbc->mStmtList, sIterator )
    {
        sStmt = (ulnStmt *)sIterator;

        if ( ulnStmtIsPrepared( sStmt ) == ACP_TRUE )
        {
            ACI_TEST( ulsdModuleHasNoData( sStmt ) == ACP_FALSE );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return ACP_TRUE;

    ACI_EXCEPTION_END;

    return ACP_FALSE;
}

SQLRETURN ulsdUpdateShardMetaNumber( ulnDbc       * aMetaDbc,
                                     ulnFnContext * aFnContext )
{
    ACI_TEST_RAISE( aMetaDbc->mIsConnected == ACP_FALSE, LABEL_ABORT_NO_CONNECTION );

    ACI_TEST( ulsdUpdateNodeList( aFnContext,
                                  & aMetaDbc->mPtContext )
              != ACI_SUCCESS );

    /* Server에서 Error Protocol로 응답하는 경우에도 ACI_SUCCESS를 반환합니다. */
    ACI_TEST( ULN_FNCONTEXT_GET_RC( aFnContext ) != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC( aFnContext );

    ACI_EXCEPTION( LABEL_ABORT_NO_CONNECTION )
    {
        ulnError( aFnContext, ulERR_ABORT_NO_CONNECTION, "" );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

SQLRETURN ulsdSetConnectAttrNode( ulnDbc        * aDbc,
                                  ulnPropertyId   aCmPropertyID,
                                  void          * aValuePtr )
{
    ULN_FLAG( sNeedExit );
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_SETCONNECTATTR, aDbc, ULN_OBJ_TYPE_DBC );

    ACI_TEST( ulnEnter( & sFnContext, (void *) & aCmPropertyID ) != ACI_SUCCESS );

    ULN_FLAG_UP( sNeedExit );

    ACI_TEST( ulnSendConnectAttr( & sFnContext,
                                  aCmPropertyID,
                                  aValuePtr )
              != ACI_SUCCESS );

    ULN_FLAG_DOWN( sNeedExit );

    ACI_TEST( ulnExit( & sFnContext ) != ACI_SUCCESS );

    return ULN_FNCONTEXT_GET_RC( & sFnContext );

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP( sNeedExit )
    {
        (void)ulnExit( & sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC( & sFnContext );
}

SQLRETURN ulsdConnect( ulnDbc       * aDbc,
                       acp_char_t   * aServerName,
                       acp_sint16_t   aServerNameLength,
                       acp_char_t   * aUserName,
                       acp_sint16_t   aUserNameLength,
                       acp_char_t   * aPassword,
                       acp_sint16_t   aPasswordLength )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;
    acp_char_t     sBackupErrorMessage[ULSD_MAX_ERROR_MESSAGE_LEN];

    sRet = ulnConnect( aDbc,
                       aServerName,
                       aServerNameLength,
                       aUserName,
                       aUserNameLength,
                       aPassword,
                       aPasswordLength );

    ACI_TEST_RAISE(sRet == SQL_SUCCESS_WITH_INFO, LABEL_SUCCESS_WITH_INFO);

    return sRet;

    ACI_EXCEPTION(LABEL_SUCCESS_WITH_INFO)
    {
        if ( ulnGetDiagRec(SQL_HANDLE_DBC,
                           (ulnObject *)aDbc,
                           1,
                           NULL,
                           NULL,
                           sBackupErrorMessage, 
                           ULSD_MAX_ERROR_MESSAGE_LEN,
                           NULL,
                           ACP_FALSE) != SQL_NO_DATA )
        {
            /*
             * ulsdSilentDisconnect() 내 ulnEnter() 호출 후 MetaDbc 의 Diagnostic Record 가 초기화 되기 때문에
             * 먼저 에러 메세지를 sBackupErrorMessage 에 백업한 다음 나중에 ulnError() 로 MetaDbc 에 추가한다.
             * 반드시 아래 순서대로 호출 해야한다.
             *
             * 1. ulsdSilentDisconnect()
             * 2. ulnError()
             */
            ulsdSilentDisconnect(aDbc);

            ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_CONNECT, aDbc, ULN_OBJ_TYPE_DBC);

            ulnError(&sFnContext,
                     ulERR_ABORT_SHARD_DATA_NODE_CONNECTION_FAILURE,
                     sBackupErrorMessage);
        }
        else
        {
            ulsdSilentDisconnect(aDbc);
        }
        sRet = SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdNodeConnect( ulnDbc       * aMetaDbc,
                           ulnFnContext * aFnContext,
                           acp_char_t   * aServerName,
                           acp_sint16_t   aServerNameLength,
                           acp_char_t   * aUserName,
                           acp_sint16_t   aUserNameLength,
                           acp_char_t   * aPassword,
                           acp_sint16_t   aPasswordLength )
{
    ACI_TEST( ulsdGetNodeList( aMetaDbc,
                               aFnContext,
                               &(aMetaDbc->mPtContext ) )
              != ACI_SUCCESS );

    /* Server에서 Error Protocol로 응답하는 경우에도 ACI_SUCCESS를 반환합니다. */
    ACI_TEST( ULN_FNCONTEXT_GET_RC( aFnContext ) != SQL_SUCCESS );

    ACI_TEST( ulsdConnectToNode( aMetaDbc,
                                 aFnContext,
                                 aServerName,
                                 aServerNameLength,
                                 aUserName,
                                 aUserNameLength,
                                 aPassword,
                                 aPasswordLength )
              != SQL_SUCCESS );

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    /* To make Success ulnExit()
     * and Disconnect Meta node connection by ulnDisconnect() */
    ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS_WITH_INFO );

    return SQL_ERROR;
}

SQLRETURN ulsdConnectToNode( ulnDbc       * aMetaDbc,
                             ulnFnContext * aFnContext,
                             acp_char_t   * aServerName,
                             acp_sint16_t   aServerNameLength,
                             acp_char_t   * aUserName,
                             acp_sint16_t   aUserNameLength,
                             acp_char_t   * aPassword,
                             acp_sint16_t   aPasswordLength )
{
    ulsdDbc            *sShard;
    ulsdNodeInfo      **sShardNodeInfo;
    acp_uint16_t        i;

    sShard = aMetaDbc->mShardDbcCxt.mShardDbc;

    ACI_TEST( sShard->mNodeCount == 0 );
    ACI_TEST( sShard->mNodeInfo == NULL );

    sShardNodeInfo = sShard->mNodeInfo;

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        ACI_TEST(ulsdAllocHandleNodeDbc(aFnContext,
                                        aMetaDbc->mParentEnv,
                                        &(sShardNodeInfo[i]->mNodeDbc))
                 != SQL_SUCCESS);

        ulsdInitalizeNodeDbc(sShardNodeInfo[i]->mNodeDbc, aMetaDbc);
    }

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        ACI_TEST( ulsdConnectToNodeInternal( aFnContext,
                                             sShardNodeInfo[i],
                                             aServerName,
                                             aServerNameLength,
                                             aUserName,
                                             aUserNameLength,
                                             aPassword,
                                             aPasswordLength )
                  != SQL_SUCCESS );
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdConnectToNodeInternal( ulnFnContext * aFnContext,
                                     ulsdNodeInfo * aNodeInfo,
                                     acp_char_t   * aServerName,
                                     acp_sint16_t   aServerNameLength,
                                     acp_char_t   * aUserName,
                                     acp_sint16_t   aUserNameLength,
                                     acp_char_t   * aPassword,
                                     acp_sint16_t   aPasswordLength )
{
    SQLRETURN sRc = SQL_ERROR;

    aNodeInfo->mNodeDbc->mShardDbcCxt.mNodeInfo = aNodeInfo;

    /* BUG-45707 */
    ulsdDbcSetShardCli( aNodeInfo->mNodeDbc, ULSD_SHARD_CLIENT_TRUE );
    ulsdDbcSetShardSessionType( aNodeInfo->mNodeDbc, ULSD_SESSION_TYPE_LIB );

    sRc = ulnConnect( aNodeInfo->mNodeDbc,
                      aServerName,
                      aServerNameLength,
                      aUserName,
                      aUserNameLength,
                      aPassword,
                      aPasswordLength );

    ACI_TEST_RAISE( sRc != SQL_SUCCESS, LABEL_NODE_CONNECTION_FAIL );

    SHARD_LOG( "(Connect) NodeId=%d, Server=%s:%d\n",
               aNodeInfo->mNodeId,
               aNodeInfo->mServerIP,
               aNodeInfo->mPortNo );

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NODE_CONNECTION_FAIL )
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_DBC,
                                  (ulnObject *)aNodeInfo->mNodeDbc,
                                  aNodeInfo,
                                  (acp_char_t *)"SQLConnect");
    }
    ACI_EXCEPTION_END;

    return sRc;
}
