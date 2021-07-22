#include <uln.h>
#include <ulnPrivate.h>
#include <ulnConnectCore.h>
#include <ulsdError.h>
#include <ulsdnFailover.h>
#include <sdErrorCodeClient.h>
#include <ulsdFailover.h>
#include <ulsdCommunication.h>

void ulsdSetNextFailoverServer( ulnDbc *sNodeDbc )
{
    ulnFailoverServerInfo * sNewServerInfo = NULL;
    ulnFailoverServerInfo * sOldServerInfo = ulnDbcGetCurrentServer( sNodeDbc );
    acp_uint32_t            sIdx;

    ACP_TEST_RAISE( sNodeDbc->mFailoverServers == NULL, END_FUNCTION );

    for ( sIdx = 0; sIdx < sNodeDbc->mFailoverServersCnt; ++sIdx )
    {
        sNewServerInfo = sNodeDbc->mFailoverServers[sIdx];
        if ( sNewServerInfo == sOldServerInfo )
        {
            continue;
        }
        ulnDbcSetCurrentServer( sNodeDbc, sNewServerInfo );
        break;
    }

    ACI_EXCEPTION_CONT( END_FUNCTION );

    return;
}

void ulsdAlignDataNodeConnection( ulnFnContext * aFnContext,
                                  ulnDbc       * aNodeDbc )
{
    ulnFnContext            sNodeFnContext;
    ulsdAlignInfo         * sAlignInfo         = NULL;
    ulnFailoverServerInfo * sNewServerInfo     = NULL;

    sAlignInfo = &aNodeDbc->mShardDbcCxt.mAlignInfo;

    ACI_TEST_RAISE( sAlignInfo->mIsNeedAlign == ACP_FALSE,
                    END_OF_FUNCTION )
    ACI_TEST_RAISE( sAlignInfo->mNativeErrorCode
                        != ACI_E_ERROR_CODE( sdERR_ABORT_SHARD_LIBRARY_LINK_FAILURE_ERROR ),
                    END_OF_FUNCTION )


    ULN_INIT_FUNCTION_CONTEXT( sNodeFnContext, ULN_FID_NONE, aNodeDbc, ULN_OBJ_TYPE_DBC );

    if ( ulnFailoverIsOn( aNodeDbc ) == ACP_TRUE )
    {
        ulsdSetNextFailoverServer( aNodeDbc );

        ulnDbcSetIsConnected( aNodeDbc, ACP_FALSE );
        ulnClosePhysicalConn( aNodeDbc );

        sNewServerInfo = ulnDbcGetCurrentServer( aNodeDbc );

        if ( ulsdnFailoverConnectToSpecificServer( &sNodeFnContext,
                                                   sNewServerInfo )
             == ACI_SUCCESS )
        {
            ulnError( aFnContext,
                      ulERR_ABORT_FAILOVER_SUCCESS,
                      sAlignInfo->mNativeErrorCode,
                      sAlignInfo->mSQLSTATE,
                      sAlignInfo->mMessageText );
        }
        else
        {
            ulsdNativeErrorToUlnError( aFnContext,
                                       SQL_HANDLE_DBC,
                                       (ulnObject*)aNodeDbc,
                                       aNodeDbc->mShardDbcCxt.mNodeInfo,
                                       (acp_char_t *)"ulsdnFailoverConnectToSpecificServer");
        }
    }

    ACI_EXCEPTION_CONT( END_OF_FUNCTION );

    sAlignInfo->mIsNeedAlign = ACP_FALSE;
}

void ulsdGetNodeConnectionReport( ulnDbc *aNodeDbc, ulsdNodeReport *aReport )
{
    ulnFailoverServerInfo    *sCurServerInfo = NULL;
    ulsdNodeInfo             *sDataNodeInfo  = NULL;
    ulsdNodeConnectReport    *sConnectReport = NULL;

    sDataNodeInfo = aNodeDbc->mShardDbcCxt.mNodeInfo;
    sConnectReport = &(aReport->mArg.mConnectReport);

    aReport->mType = ULSD_REPORT_TYPE_CONNECTION;

    sCurServerInfo = ulnDbcGetCurrentServer( aNodeDbc );
    if ( sCurServerInfo == NULL )
    {
        /* No alternate server */
        sConnectReport->mDestination = ULSD_CONN_TO_ACTIVE;
    }
    else
    {
        /* 현재 data node 주소는 active, alternate 두개만 지정 가능하므로
         * active 와 비교하여 동일하면 active 에 접속,
         * active 와 비교하여 동일하지 않으면 alternate 에 접속한것으로 가정한다.
         */
        if ( ( acpCStrCmp( sCurServerInfo->mHost,
                           sDataNodeInfo->mServerIP,
                           ULSD_MAX_SERVER_IP_LEN ) == 0 )
             && ( sCurServerInfo->mPort == sDataNodeInfo->mPortNo ) )
        {
            sConnectReport->mDestination = ULSD_CONN_TO_ACTIVE;
        }
        else
        {
            sConnectReport->mDestination = ULSD_CONN_TO_ALTERNATE;
        }
    }

    sConnectReport->mNodeId = sDataNodeInfo->mNodeId;
}

ACI_RC ulsdSendNodeConnectionReport( ulnFnContext *aMetaFnContext, ulsdNodeReport *aReport )
{
    ulnDbc  * sDbc   = NULL;
    ulsdDbc * sShard = NULL;

    ULN_FNCONTEXT_GET_DBC( aMetaFnContext, sDbc );

    ACI_TEST_RAISE( sDbc == NULL, InvalidHandleException );

    sShard = sDbc->mShardDbcCxt.mShardDbc;

    if ( ulsdShardNodeReport( aMetaFnContext,
                              aReport )
         == ACI_SUCCESS )
    {
        if ( sDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
        {
            sShard->mTouched = ACP_TRUE;
        }
    }
    else
    {
        /* Failover Succes is not error.
         * Disconnect status (or failover failure) is not error.
         * All data node connection status will be sent after Meta failover success.
         */
        ACE_DASSERT( SQL_SUCCEEDED( ULN_FNCONTEXT_GET_RC( aMetaFnContext ) ) == 0 );

        if ( ulsdIsFailoverExecute( aMetaFnContext ) == ACP_TRUE )
        {
            ACI_TEST_RAISE( ulnClearDiagnosticInfoFromObject( aMetaFnContext->mHandle.mObj )
                            != ACI_SUCCESS,
                            LABEL_MEM_MAN_ERR );

            ULN_FNCONTEXT_SET_RC( aMetaFnContext, SQL_SUCCESS );
        }
    }

    ACI_TEST( SQL_SUCCEEDED( ULN_FNCONTEXT_GET_RC( aMetaFnContext ) ) == 0 );

    return ACI_SUCCESS;

    ACI_EXCEPTION( InvalidHandleException )
    {
        ULN_FNCONTEXT_SET_RC( aMetaFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_MEM_MAN_ERR )
    {
        ulnError( aMetaFnContext,
                  ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                  "ulsdSendNodeConnectionReport" );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulsdNotifyFailoverOnMeta( ulnDbc *aMetaDbc )
{
    ulnFnContext              sMetaFnContext;
    ulnDbc                   *sNodeDbc       = NULL;
    ulsdDbc                  *sShard         = NULL;
    ulsdNodeReport            sReport;
    acp_sint32_t              sIdx;

    ULN_INIT_FUNCTION_CONTEXT( sMetaFnContext, ULN_FID_NONE, aMetaDbc, ULN_OBJ_TYPE_DBC );

    sShard = aMetaDbc->mShardDbcCxt.mShardDbc;

    for ( sIdx = 0; sIdx < sShard->mNodeCount; ++sIdx )
    {
        sNodeDbc = sShard->mNodeInfo[sIdx]->mNodeDbc;
        if ( sNodeDbc == NULL )
        {
            ACE_DASSERT( 0 );
            continue;
        }

        ulsdGetNodeConnectionReport( sNodeDbc, &sReport );

        ACI_TEST( ulsdSendNodeConnectionReport( &sMetaFnContext, &sReport ) != ACI_SUCCESS );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_bool_t ulsdIsFailoverExecute( ulnFnContext  * aFnContext )
{                
    ulnDbc          * sDbc = NULL;
    acp_bool_t        sRet = ACP_FALSE;
                 
    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );
    if ( sDbc != NULL )
    {            
        if ( ( aFnContext->mIsFailoverSuccess == ACP_TRUE ) ||
             ( ulnDbcIsConnected( sDbc ) == ACP_FALSE ) )
        {
            sRet = ACP_TRUE;
        }
    }

    return sRet;
}
