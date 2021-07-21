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
#include <mtcc.h>

#include <ulsd.h>
#include <ulsdnFailover.h>

ACI_RC ulsdCallbackGetNodeListResult(cmiProtocolContext *aProtocolContext,
                                     cmiProtocol        *aProtocol,
                                     void               *aServiceSession,
                                     void               *aUserContext);
ACI_RC ulsdCallbackUpdateNodeListResult(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *aProtocol,
                                        void               *aServiceSession,
                                        void               *aUserContext);
ACI_RC ulsdCallbackAnalyzeResult(cmiProtocolContext *aProtocolContext,
                                 cmiProtocol        *aProtocol,
                                 void               *aServiceSession,
                                 void               *aUserContext);
ACI_RC ulsdCallbackShardTransactionResult(cmiProtocolContext *aProtocolContext,
                                          cmiProtocol        *aProtocol,
                                          void               *aServiceSession,
                                          void               *aUserContext);
ACI_RC ulsdCallbackShardHandshakeResult( cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aServiceSession,
                                         void               *aUserContext );

static ACI_RC ulsdShardNodeConnectionReportRequest( ulnFnContext          * aFnContext,
                                                    ulnPtContext          * aPtContext,
                                                    acp_uint32_t            aNodeId,
                                                    ulsdDataNodeConntectTo  aDestination )
{
    cmiProtocol               sPacket;
    cmiProtocolContext      * sCtx = &(aPtContext->mCmiPtContext);
    ulsdReportType            sType = ULSD_REPORT_TYPE_CONNECTION;

    sPacket.mOpID = CMP_OP_DB_ShardNodeReport;

    CMI_WRITE_CHECK( sCtx, 1 +
                           4 +  // mType
                           4 +  // NodeID
                           1 ); // Destination

    CMI_WOP( sCtx, CMP_OP_DB_ShardNodeReport );

    CMI_WR4( sCtx, &sType );
    CMI_WR4( sCtx, &aNodeId );
    CMI_WR1( sCtx, aDestination );

    ACI_TEST(ulnWriteProtocol( aFnContext,
                               aPtContext,
                               &sPacket)
             != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulsdShardNodeTransactionBrokenReportRequest( ulnFnContext     * aFnContext,
                                                           ulnPtContext     * aPtContext )
{
    cmiProtocol               sPacket;
    cmiProtocolContext      * sCtx = &(aPtContext->mCmiPtContext);
    ulsdReportType            sType = ULSD_REPORT_TYPE_TRANSACTION_BROKEN;;

    sPacket.mOpID = CMP_OP_DB_ShardNodeReport;

    CMI_WRITE_CHECK( sCtx, 1 +
                           4 );  // mType

    CMI_WOP( sCtx, CMP_OP_DB_ShardNodeReport );

    CMI_WR4( sCtx, &sType );

    ACI_TEST(ulnWriteProtocol( aFnContext,
                               aPtContext,
                               &sPacket)
             != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC ulsdShardNodeReportMain( ulnFnContext     * aFnContext,
                                       ulnDbc           * aDbc,
                                       ulsdNodeReport   * aReport )
{
    ULN_FLAG( sNeedFinPtContext );
    cmiProtocolContext      * sCtx = NULL;
    ulsdNodeConnectReport   * sConnectReport;

    ACI_TEST_RAISE( aDbc->mIsConnected == ACP_FALSE, LABEL_ABORT_NO_CONNECTION );

    ACI_TEST( ulnInitializeProtocolContext( aFnContext,
                                            &(aDbc->mPtContext),
                                            &(aDbc->mSession) )
              != ACI_SUCCESS );
    ULN_FLAG_UP( sNeedFinPtContext );

    /* request */
    switch( aReport->mType )
    {
        case ULSD_REPORT_TYPE_CONNECTION:
            sConnectReport = &(aReport->mArg.mConnectReport);
            ACI_TEST( ulsdShardNodeConnectionReportRequest( aFnContext,
                                                           &(aDbc->mPtContext),
                                                           sConnectReport->mNodeId,
                                                           sConnectReport->mDestination )
                      != ACI_SUCCESS );
            break;

        case ULSD_REPORT_TYPE_TRANSACTION_BROKEN:
            ACI_TEST( ulsdShardNodeTransactionBrokenReportRequest( aFnContext,
                                                                  &(aDbc->mPtContext) )
                      != ACI_SUCCESS );
            break;

        default:
            ACE_DASSERT(0);
            break;
    }

    ACI_TEST( ulnFlushProtocol( aFnContext,
                                &(aDbc->mPtContext) )
              != ACI_SUCCESS );

    /* Waiting for response */
    sCtx = &(aDbc->mPtContext.mCmiPtContext);
    if ( cmiGetLinkImpl( sCtx ) == CMI_LINK_IMPL_IPCDA )
    {
        ACI_TEST( ulnReadProtocolIPCDA( aFnContext,
                                        &(aDbc->mPtContext),
                                        aDbc->mConnTimeoutValue )
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( ulnReadProtocol( aFnContext,
                                   &(aDbc->mPtContext),
                                   aDbc->mConnTimeoutValue )
                  != ACI_SUCCESS );
    }

    ULN_FLAG_DOWN( sNeedFinPtContext );
    ACI_TEST( ulnFinalizeProtocolContext( aFnContext, &(aDbc->mPtContext) ) !=
              ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( LABEL_ABORT_NO_CONNECTION )
    {
        ulnError( aFnContext, ulERR_ABORT_NO_CONNECTION, "" );
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext(aFnContext,&(aDbc->mPtContext));
    }

    return ACI_FAILURE;
}

static ACI_RC ulsdShardNodeReportResult( cmiProtocolContext * aProtocolContext,
                                         cmiProtocol        * aProtocol,
                                         void               * aServiceSession,
                                         void               * aUserContext )
{
    ACP_UNUSED( aProtocolContext );
    ACP_UNUSED( aProtocol );
    ACP_UNUSED( aServiceSession );
    ACP_UNUSED( aUserContext );

    return ACI_SUCCESS;
}

static ACI_RC ulsdCallbackShardRebuildNoti( cmiProtocolContext * aProtocolContext,
                                            cmiProtocol        * aProtocol,
                                            void               * aServiceSession,
                                            void               * aUserContext )
{
    ulnFnContext  * sFnContext       = (ulnFnContext *)aUserContext;
    ulnDbc        * sDbc             = NULL;
    acp_uint64_t    sShardMetaNumber = 0;

    ULN_FNCONTEXT_GET_DBC(sFnContext, sDbc);

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    SHARD_LOG("ShardRebuildNoti\n");

    CMI_RD8( aProtocolContext, &sShardMetaNumber );

    if ( sDbc->mShardDbcCxt.mParentDbc != NULL )
    {
        ulnDbcSetTargetShardMetaNumber( sDbc->mShardDbcCxt.mParentDbc, sShardMetaNumber );
    }
    else
    {
        ulnDbcSetTargetShardMetaNumber( sDbc, sShardMetaNumber );
    }

    return ACI_SUCCESS;
}

ACI_RC ulsdHandshake(ulnFnContext *aFnContext)
{
    ulnDbc             *sDbc = ulnFnContextGetDbc(aFnContext);
    cmiProtocolContext *sCtx = &(sDbc->mPtContext.mCmiPtContext);

    acp_time_t          sTimeout;

    CMI_WRITE_CHECK( sCtx, 5 ); 

    CMI_WR1(sCtx, CMP_OP_DB_ShardHandshake);
    CMI_WR1(sCtx, SHARD_MAJOR_VERSION);
    CMI_WR1(sCtx, SHARD_MINOR_VERSION);
    CMI_WR1(sCtx, SHARD_PATCH_VERSION);
    CMI_WR1(sCtx, 0);

    if ( cmiGetLinkImpl( sCtx ) == CMI_LINK_IMPL_IPCDA )
    {
        acpMemBarrier();
        cmiIPCDAIncDataCount( sCtx );
        /* Finalize to write data block. */
        (void)cmiFinalizeSendBufferForIPCDA( (void*)sCtx );

        ACI_TEST( cmiRecvIPCDA( sCtx,
                                aFnContext,
                                sDbc->mIPCDAULExpireCount,
                                sDbc->mIPCDAMicroSleepTime )
                  != ACI_SUCCESS );
    }
    else
    {
        ACI_TEST( cmiSend( sCtx, ACP_TRUE ) != ACI_SUCCESS );

        sTimeout = acpTimeFrom( ulnDbcGetLoginTimeout( sDbc ), 0 );

        ACI_TEST( cmiRecv( sCtx,
                           aFnContext,
                           sTimeout)
                  != ACI_SUCCESS );
    }

    ACI_TEST( SQL_SUCCEEDED( ULN_FNCONTEXT_GET_RC( aFnContext ) ) == 0 );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    
    if ( aciGetErrorCode() != 0 )
    {
        (void)ulnErrHandleCmError( aFnContext, NULL );
    }
    else
    {
        /* Nothing to do */
    }

    return ACI_FAILURE;
}

ACI_RC ulsdUpdateNodeListRequest(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR( sCtx );

    sPacket.mOpID = CMP_OP_DB_ShardNodeUpdateList;

    CMI_WRITE_CHECK(sCtx, 1);

    CMI_WOP(sCtx, CMP_OP_DB_ShardNodeUpdateList);

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    CMI_SET_CURSOR( sCtx, sOrgWriteCursor );

    return ACI_FAILURE;
}

ACI_RC ulsdGetNodeListRequest(ulnFnContext *aFnContext, ulnPtContext *aPtContext)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR( sCtx );

    sPacket.mOpID = CMP_OP_DB_ShardNodeGetList;

    CMI_WRITE_CHECK(sCtx, 1);

    CMI_WOP(sCtx, CMP_OP_DB_ShardNodeGetList);

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    CMI_SET_CURSOR( sCtx, sOrgWriteCursor );

    return ACI_FAILURE;
}

ACI_RC ulsdAnalyzeRequest(ulnFnContext  *aFnContext,
                          ulnPtContext  *aProtocolContext,
                          acp_char_t    *aString,
                          acp_sint32_t   aLength)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx     = &(aProtocolContext->mCmiPtContext);
    ulnStmt            *sStmt    = aFnContext->mHandle.mStmt;
    acp_uint8_t        *sRow     = (acp_uint8_t*)aString;
    acp_uint64_t        sRowSize = aLength;
    acp_uint8_t         sDummy   = 0;
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);

    sPacket.mOpID = CMP_OP_DB_ShardAnalyze;

    CMI_WRITE_CHECK(sCtx, 10);

    CMI_WOP(sCtx, CMP_OP_DB_ShardAnalyze);
    CMI_WR4(sCtx, &(sStmt->mStatementID));
    CMI_WR1(sCtx, sDummy);
    CMI_WR4(sCtx, (acp_uint32_t*)&aLength);

    ACI_TEST( cmiSplitWrite( sCtx,
                             sRowSize,
                             sRow )
              != ACI_SUCCESS );
    sRowSize = 0;

    ACI_TEST(ulnWriteProtocol(aFnContext, aProtocolContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulsdShardTransactionCommitRequest( ulnFnContext     * aFnContext,
                                          ulnPtContext     * aPtContext,
                                          ulnTransactionOp   aTransactOp,
                                          acp_uint32_t     * aTouchNodeArr,
                                          acp_uint16_t       aTouchNodeCount)
{
    cmiProtocol         sPacket;
    cmiProtocolContext *sCtx            = &(aPtContext->mCmiPtContext);
    acp_uint16_t        sOrgWriteCursor = CMI_GET_CURSOR(sCtx);
    acp_uint8_t         sState          = 0;
    acp_uint16_t        i;

    sPacket.mOpID = CMP_OP_DB_ShardTransactionV3;  /* PROJ-2733-Protocol */

    CMI_WRITE_CHECK(sCtx, 1 + 2 + aTouchNodeCount * 4);
    sState = 1;

    CMI_WOP(sCtx, CMP_OP_DB_ShardTransactionV3);

    switch( aTransactOp )
    {
        case ULN_TRANSACT_COMMIT:
            CMI_WR1(sCtx, 1);
            break;

        case ULN_TRANSACT_ROLLBACK:
            CMI_WR1(sCtx, 2);
            break;

        default:
            ACI_RAISE(LABEL_INVALID_OPCODE);
            break;
    }

    /* touch node array */
    CMI_WR2(sCtx, &aTouchNodeCount );
    for ( i = 0; i < aTouchNodeCount; i++ )
    {
        CMI_WR4(sCtx, &(aTouchNodeArr[i]));
    }

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_OPCODE)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_TRANSACTION_OPCODE);
    }
    ACI_EXCEPTION_END;

    if ( (sState == 0) && (cmiGetLinkImpl(sCtx) == CMN_LINK_IMPL_IPCDA) )
    {
        (void)ulnError(aFnContext, ulERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE);
    }
    else
    {
        /* Nothing to do */
    }

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

ACI_RC ulsdInitializeDBProtocolCallbackFunctions(void)
{
    /* PROJ-2598 Shard */
    ACI_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardNodeGetListResult),
                            ulsdCallbackGetNodeListResult) != ACI_SUCCESS);
    /* PROJ-2622 Shard Retry Execution */
    ACI_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardNodeUpdateListResult),
                            ulsdCallbackUpdateNodeListResult) != ACI_SUCCESS);

    /*
     * PROJ-2598 Shard pilot(shard analyze)
     * SHARD PREPARE
     */
    ACI_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardAnalyzeResult),
                            ulsdCallbackAnalyzeResult) != ACI_SUCCESS);

    /* BUG-45411 client-side global transaction */
    ACI_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardTransactionV3Result),  /* PROJ-2733-Protocol */
                            ulsdCallbackShardTransactionResult) != ACI_SUCCESS);

    ACI_TEST( cmiSetCallback( CMI_PROTOCOL( DB, ShardHandshakeResult ),
                              ulsdCallbackShardHandshakeResult ) != ACI_SUCCESS );

    ACI_TEST( cmiSetCallback( CMI_PROTOCOL( DB, ShardNodeReportResult ),
                              ulsdShardNodeReportResult ) != ACI_SUCCESS );

    ACI_TEST( cmiSetCallback( CMI_PROTOCOL(DB, ShardRebuildNotiV3 ),
                              ulsdCallbackShardRebuildNoti ) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulsdFODoSTF( ulnFnContext     * aFnContext,
                    ulnDbc           * aDbc,
                    ulnErrorMgr      * aErrorMgr )
{
    
    ACP_UNUSED( aDbc );

    ACI_TEST( ulnFailoverDoSTF( aFnContext ) != ACI_SUCCESS );

    (void)ulnError( aFnContext,
                    ulERR_ABORT_FAILOVER_SUCCESS,
                    ulnErrorMgrGetErrorCode(aErrorMgr),
                    ulnErrorMgrGetSQLSTATE(aErrorMgr),
                    ulnErrorMgrGetErrorMessage(aErrorMgr) );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    (void)ulnErrHandleCmError( aFnContext, NULL );

    return ACI_FAILURE;
}

ACI_RC ulsdFODoReconnect( ulnFnContext     * aFnContext,
                          ulnDbc           * aDbc,
                          ulnErrorMgr      * aErrorMgr )
{
    ACI_TEST( ulsdnReconnectWithoutEnter( SQL_HANDLE_DBC, &aDbc->mObj ) != SQL_SUCCESS );

    (void)ulnError( aFnContext,
                    ulERR_ABORT_FAILOVER_SUCCESS,
                    ulnErrorMgrGetErrorCode(aErrorMgr),
                    ulnErrorMgrGetSQLSTATE(aErrorMgr),
                    ulnErrorMgrGetErrorMessage(aErrorMgr) );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    (void)ulnErrHandleCmError(aFnContext, NULL);

    return ACI_FAILURE;
}

ACI_RC ulsdShardNodeReport( ulnFnContext      * aFnContext,
                            ulsdNodeReport    * aReport )
{
    ulnDbc      * sDbc = NULL;

    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );
    ACI_TEST_RAISE( sDbc == NULL, InvalidHandleException );

    ACI_TEST( ulsdShardNodeReportMain( aFnContext,
                                       sDbc,
                                       aReport )
              != ACI_SUCCESS );

    return ACI_SUCCESS;

    ACI_EXCEPTION( InvalidHandleException )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

