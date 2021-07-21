/** 
 *  Copyright (c) 1999~2018, Altibase Corp. and/or its affiliates. All rights reserved.
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
 * $Id$
 **********************************************************************/
#include <uln.h>
#include <ulnPrivate.h>
#include <ulsdnFailover.h>
#include <ulnConnectCore.h>
#include <ulsdModule.h>
#include <ulsdError.h>
#include <mmErrorCodeClient.h>


SQLRETURN ulsdnGetFailoverIsNeeded( acp_sint16_t   aHandleType,
                                    ulnObject     *aObject,
                                    acp_sint32_t  *aIsNeed )
{
    ulnFnContext  sFnContext;
    ulnDbc       *sDbc           = NULL;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aObject, aHandleType );

    ACI_TEST_RAISE( aObject == NULL, InvalidHandle );

    ULN_FNCONTEXT_GET_DBC( &sFnContext, sDbc );

    ACI_TEST_RAISE( sDbc == NULL, InvalidHandle );

    /*
     * ======================================
     * Function BEGIN
     * ======================================
     */
    *aIsNeed = 0;

    if ( ulnDiagRecIsNeedFailover( aObject ) == ACP_TRUE )
    {
        *aIsNeed = 1;
    }

    /*
     * ======================================
     * Function END
     * ======================================
     */

    return ULN_FNCONTEXT_GET_RC( &sFnContext );

    ACI_EXCEPTION( InvalidHandle )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }

    ACI_EXCEPTION_END;

    *aIsNeed = 0;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

/* Call from
 *  1. SQLReconnect()
 *     -> ulsdnReconnect()
 *      - Server-side retry connection.
 *  2. ulsdErrorHandleShardingError()
 *     -> ulsdModuleAlignDataNodeConnection()
 *     -> ulsdModuleAlignDataNodeConnection_COORD/META()
 *     -> ulsdAlignDataNodeConnection()
 *      - Server-side failover and align data node connection.
 *  3. ulsdModuleOnCmError_NODE()
 *     -> ulsdFODoReconnect()
 *     -> ulsdnReconnectWithoutEnter()
 *      - Node connection retry.
 */
ACI_RC ulsdnFailoverConnectToSpecificServer( ulnFnContext *aFnContext, ulnFailoverServerInfo *aNewServerInfo )
{
    ulnDbc       * sDbc       = NULL;
    ACI_RC         sRC        = ACI_FAILURE;

    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );
    ACI_TEST_RAISE( sDbc == NULL, INVALID_HANDLE_EXCEPTION );

    ACI_TEST( ( ulnDbcGetConnType( sDbc ) != ULN_CONNTYPE_TCP     ) &&
              ( ulnDbcGetConnType( sDbc ) != ULN_CONNTYPE_IB      ) && /* PROJ-2681 */
              ( ulnDbcGetConnType( sDbc ) != ULN_CONNTYPE_SSL     ) );

    ULN_TRACE_LOG( aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                   "%-18s| Server: %s:%d",
                   "ulnFailoverConnectToSpecificServer",
                   aNewServerInfo->mHost,
                   aNewServerInfo->mPort );

    sRC = ulnFailoverConnect( aFnContext, ULN_FAILOVER_TYPE_STF, aNewServerInfo );
#ifdef COMPILE_SHARDCLI
    if ( sRC == ACI_SUCCESS )
    {
        sRC = ulsdModuleUpdateNodeList( aFnContext, sDbc );
    }
    if ( sRC == ACI_SUCCESS )
    {
        sRC = ulsdModuleNotifyFailOver( sDbc );
    }
#endif /* COMPILE_SHARDCLI */
    if ( sRC == ACI_SUCCESS )
    {
        ulnDbcCloseAllStatement( sDbc );
        if ( sDbc->mXaConnection != NULL )
        {
            ulnDbcSetFailoverCallbackState(sDbc, ULN_FAILOVER_CALLBACK_IN_STATE);
            sRC = ulnFailoverXaReOpen( sDbc );
            ulnDbcSetFailoverCallbackState(sDbc, ULN_FAILOVER_CALLBACK_OUT_STATE);
        }
        if ( sRC == ACI_SUCCESS )
        {
            sRC = ulnClearDiagnosticInfoFromObject( aFnContext->mHandle.mObj );
            ACI_TEST_RAISE( sRC != ACI_SUCCESS, LABEL_MEM_MAN_ERR );
        }
    }

    ACI_TEST( sRC != ACI_SUCCESS );

    ULN_TRACE_LOG( aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                   "%-18s|", "ulnFailoverConnectToSpecificServer" );

    return ACI_SUCCESS;

    ACI_EXCEPTION( INVALID_HANDLE_EXCEPTION )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_MEM_MAN_ERR )
    {
        (void)ulnError( aFnContext,
                        ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                        "ulsdnFailoverConnectToSpecificServer" );
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG( aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                   "%-18s| fail", "ulnFailoverConnectToSpecificServer" );

    if ( sDbc != NULL )
    {
        if ( ulnDbcIsConnected( sDbc ) == ACP_TRUE )
        {
            ulnDbcSetIsConnected( sDbc, ACP_FALSE );
            ulnClosePhysicalConn( sDbc );
        }
    }

    ulsdnRaiseShardNodeFailoverIsNotAvailableError( aFnContext );

    return sRC;
}

SQLRETURN ulsdnReconnectCore( ulnFnContext * aFnContext )
{
    ulnDbc                *sDbc           = NULL;
    ulnFailoverServerInfo *sServerInfo    = NULL;
    acp_bool_t             sHasAlternate  = ACP_TRUE;

    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );

    ACI_TEST_RAISE( sDbc == NULL, InvalidHandle );

    sServerInfo = ulnDbcGetCurrentServer( sDbc );
    if ( sServerInfo == NULL )
    {
        /* No alternate server */
        sHasAlternate = ACP_FALSE;
        ACI_TEST( ulnFailoverCreatePrimaryServerInfo( aFnContext, &sServerInfo ) != ACI_SUCCESS );
        ulnDbcSetCurrentServer( sDbc, sServerInfo );
    }
    ACI_TEST_RAISE( sServerInfo == NULL, InvalidCurrrentServer );
    
    ACI_TEST( ulsdnFailoverConnectToSpecificServer( aFnContext,
                                                    sServerInfo )
              != ACI_SUCCESS );

    if ( sHasAlternate == ACP_FALSE )
    {
        ulnDbcSetCurrentServer( sDbc, NULL );
        ulnFailoverDestroyServerInfo( sServerInfo );
    }

    return ULN_FNCONTEXT_GET_RC(aFnContext);

    ACI_EXCEPTION( InvalidHandle )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( InvalidCurrrentServer )
    {
        (void)ulnError( aFnContext, ulERR_ABORT_NO_CONNECTION );
    }

    ACI_EXCEPTION_END;

    if ( sHasAlternate == ACP_FALSE )
    {
        ulnDbcSetCurrentServer( sDbc, NULL );
        ulnFailoverDestroyServerInfo( sServerInfo );
    }

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdnReconnect( acp_sint16_t aHandleType, ulnObject *aObject )
{
    ULN_FLAG(sNeedExit);
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_FOR_SD, aObject, aHandleType );

    ACI_TEST_RAISE( aObject == NULL, InvalidHandle );

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST( ulsdnReconnectCore( &sFnContext )
              != SQL_SUCCESS );

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION( InvalidHandle )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN ulsdnReconnectWithoutEnter( acp_sint16_t aHandleType, ulnObject *aObject )
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aObject, aHandleType );

    ACI_TEST_RAISE( aObject == NULL, InvalidHandle );

    ACI_TEST_RAISE( ulnClearDiagnosticInfoFromObject( aObject )
                    != ACI_SUCCESS,
                    LABEL_MEM_MAN_ERR );

    ACI_TEST( ulsdnReconnectCore( &sFnContext )
              != SQL_SUCCESS );

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION( InvalidHandle )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_MEM_MAN_ERR )
    {
        (void)ulnError( &sFnContext,
                        ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                        "ulsdnReconnectWithoutEnter" );
    }

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

void ulsdnRaiseShardNodeFailoverIsNotAvailableError(ulnFnContext *aFnContext)
{
    (void)ulnClearDiagnosticInfoFromObject( aFnContext->mHandle.mObj );

    (void)ulnError( aFnContext, ulERR_ABORT_SHARD_NODE_FAILOVER_IS_NOT_AVAILABLE );
}

