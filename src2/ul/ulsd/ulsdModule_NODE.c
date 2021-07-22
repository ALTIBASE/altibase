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

#include <ulnConnectCore.h>

#include <ulsd.h>
#include <ulsdFailover.h>
#include <ulsdnFailover.h>
#include <ulsdRebuild.h>

ACI_RC ulsdModuleHandshake_NODE(ulnFnContext *aFnContext)
{
    ACP_UNUSED(aFnContext);

    return ACI_SUCCESS;
}

SQLRETURN ulsdModuleNodeDriverConnect_NODE(ulnDbc       *aDbc,
                                           ulnFnContext *aFnContext,
                                           acp_char_t   *aConnString,
                                           acp_sint16_t  aConnStringLength)
{
    ACP_UNUSED(aDbc);
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aConnString);
    ACP_UNUSED(aConnStringLength);

    return SQL_SUCCESS;
}

SQLRETURN ulsdModuleNodeConnect_NODE( ulnDbc       * aDbc,
                                      ulnFnContext * aFnContext,
                                      acp_char_t   * aServerName,
                                      acp_sint16_t   aServerNameLength,
                                      acp_char_t   * aUserName,
                                      acp_sint16_t   aUserNameLength,
                                      acp_char_t   * aPassword,
                                      acp_sint16_t   aPasswordLength )
{
    ACP_UNUSED( aDbc );
    ACP_UNUSED( aFnContext );
    ACP_UNUSED( aServerName );
    ACP_UNUSED( aServerNameLength );
    ACP_UNUSED( aUserName );
    ACP_UNUSED( aUserNameLength );
    ACP_UNUSED( aPassword );
    ACP_UNUSED( aPasswordLength );

    return SQL_SUCCESS;
}

ACI_RC ulsdModuleEnvRemoveDbc_NODE(ulnEnv *aEnv, ulnDbc *aDbc)
{
    ACP_UNUSED(aEnv);
    ACP_UNUSED(aDbc);

    return ACI_SUCCESS;
}

void ulsdModuleStmtDestroy_NODE(ulnStmt *aStmt)
{
    ACP_UNUSED(aStmt);
}

SQLRETURN ulsdModulePrepare_NODE(ulnFnContext *aFnContext,
                                 ulnStmt      *aStmt,
                                 acp_char_t   *aStatementText,
                                 acp_sint32_t  aTextLength,
                                 acp_char_t   *aAnalyzeText)
{
    ACP_UNUSED(aFnContext);

    return ulnPrepare(aStmt,
                      aStatementText,
                      aTextLength,
                      aAnalyzeText);
}

SQLRETURN ulsdModuleExecute_NODE(ulnFnContext *aFnContext,
                                 ulnStmt      *aStmt)
{
    ACP_UNUSED(aFnContext);

    return ulnExecute(aStmt);
}

SQLRETURN ulsdModuleFetch_NODE(ulnFnContext *aFnContext,
                               ulnStmt      *aStmt)
{
    ACP_UNUSED(aFnContext);

    return ulnFetch(aStmt);
}

SQLRETURN ulsdModuleCloseCursor_NODE(ulnStmt *aStmt)
{
    return ulnCloseCursor(aStmt);
}

SQLRETURN ulsdModuleRowCount_NODE(ulnFnContext *aFnContext,
                                  ulnStmt      *aStmt,
                                  acp_sint32_t *aRowCount)
{
    ACP_UNUSED(aFnContext);

    return ulnRowCount(aStmt, aRowCount);
}

SQLRETURN ulsdModuleMoreResults_NODE(ulnFnContext *aFnContext,
                                     ulnStmt      *aStmt)
{
    ACP_UNUSED(aFnContext);

    return ulnMoreResults(aStmt);
}

ulnStmt* ulsdModuleGetPreparedStmt_NODE(ulnStmt *aStmt)
{
    ulnStmt  *sStmt = NULL;

    if ( ulnStmtIsPrepared(aStmt) == ACP_TRUE )
    {
        sStmt = aStmt;
    }
    else
    {
        /* Nothing to do */
    }

    return sStmt;
}

void ulsdModuleOnCmError_NODE(ulnFnContext     *aFnContext,
                              ulnDbc           *aDbc,
                              ulnErrorMgr      *aErrorMgr)
{
    ACI_RC    sRet     = ACI_FAILURE;
    ulnDbc  * sMetaDbc = aDbc->mShardDbcCxt.mParentDbc;
    ulsdDbc * sShard   = sMetaDbc->mShardDbcCxt.mShardDbc;

    if ( ulnFailoverIsOn( aDbc ) == ACP_TRUE )
    {
        sRet = ulsdFODoSTF(aFnContext, aDbc, aErrorMgr);
    }
    else if ( ulnDbcGetSessionFailover( aDbc ) == ACP_TRUE )
    {
        sRet = ulsdFODoReconnect( aFnContext, aDbc, aErrorMgr );
    }
    else
    {
        /* sRet == ACI_FAILURE */
        if ( ulnDbcIsConnected( aDbc ) == ACP_TRUE )
        {
            ulnDbcSetIsConnected( aDbc, ACP_FALSE );
            ulnClosePhysicalConn( aDbc );
        }
    }

    if ( sMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        /* BUG-47143 샤드 All meta 환경에서 Failover 를 검증합니다. */
        sShard->mTouched = ACP_TRUE;
    }

    if ( sRet != ACI_SUCCESS )
    {
        ulsdnRaiseShardNodeFailoverIsNotAvailableError( aFnContext );
    }
}

ACI_RC ulsdModuleUpdateNodeList_NODE(ulnFnContext  *aFnContext,
                                     ulnDbc        *aDbc)
{
    ACP_UNUSED(aFnContext);
    ACP_UNUSED(aDbc);

    return ACI_SUCCESS;
}

ACI_RC ulsdModuleNotifyFailOver_NODE( ulnDbc *aDbc )
{
    ulnDbc                   * sMetaDbc = aDbc->mShardDbcCxt.mParentDbc;
    ulsdNodeReport             sReport;
    ulnFnContext               sMetaFnContext;

    ULN_INIT_FUNCTION_CONTEXT( sMetaFnContext, ULN_FID_NONE, sMetaDbc, ULN_OBJ_TYPE_DBC );

    ulsdGetNodeConnectionReport( aDbc, &sReport );

    /* BUG-47131 Stop shard META DBC failover. */
    ulnDbcSetFailoverSuspendState( sMetaDbc, ULN_FAILOVER_SUSPEND_ON_STATE );

    ACI_TEST( ulsdSendNodeConnectionReport( &sMetaFnContext, &sReport ) != ACI_SUCCESS );

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( sMetaDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( sMetaDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return ACI_FAILURE;
}

void ulsdModuleAlignDataNodeConnection_NODE( ulnFnContext * aFnContext,
                                             ulnDbc       * aNodeDbc )
{
    ACP_UNUSED( aFnContext       );
    ACP_UNUSED( aNodeDbc         );
}

void ulsdModuleErrorCheckAndAlignDataNode_NODE( ulnFnContext * aFnContext )
{
    ACP_UNUSED( aFnContext       );
}

acp_bool_t ulsdModuleHasNoData_NODE( ulnStmt * aStmt )
{
    return ulnCursorHasNoData( ulnStmtGetCursor( aStmt ) );
}

/*
 * PROJ-2739 Client-side Sharding LOB
 */
SQLRETURN ulsdModuleGetLobLength_NODE( ulnFnContext * aFnContext,
                                       ulnStmt      * aStmt,
                                       acp_uint64_t   aLocator,
                                       acp_sint16_t   aLocatorType,
                                       acp_uint32_t * aLengthPtr )
{
    ACP_UNUSED( aFnContext );

    acp_uint16_t sIsNull;
     
    return ulnGetLobLength( SQL_HANDLE_STMT,
                            (ulnObject *)aStmt,
                            aLocator,
                            aLocatorType,
                            aLengthPtr,
                            &sIsNull );
}

SQLRETURN ulsdModuleGetLob_NODE( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt,
                                 acp_sint16_t   aLocatorCType,
                                 acp_uint64_t   aSrcLocator,
                                 acp_uint32_t   aFromPosition,
                                 acp_uint32_t   aForLength,
                                 acp_sint16_t   aTargetCType,
                                 void         * aBuffer,
                                 acp_uint32_t   aBufferSize,
                                 acp_uint32_t * aLengthWritten )
{
    ACP_UNUSED( aFnContext );

    return ulnGetLob( SQL_HANDLE_STMT,
                      (ulnObject *)aStmt,
                      aLocatorCType,
                      aSrcLocator,
                      aFromPosition,
                      aForLength,
                      aTargetCType,
                      aBuffer,
                      aBufferSize,
                      aLengthWritten );
}

SQLRETURN ulsdModulePutLob_NODE( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt,
                                 acp_sint16_t   aLocatorCType,
                                 acp_uint64_t   aLocator,
                                 acp_uint32_t   aFromPosition,
                                 acp_uint32_t   aForLength,
                                 acp_sint16_t   aSourceCType,
                                 void         * aBuffer,
                                 acp_uint32_t   aBufferSize )
{
    ACP_UNUSED( aFnContext );

    return ulnPutLob( SQL_HANDLE_STMT,
                      (ulnObject *)aStmt,
                      aLocatorCType,
                      aLocator,
                      aFromPosition,
                      aForLength,
                      aSourceCType,
                      aBuffer,
                      aBufferSize );
}

SQLRETURN ulsdModuleFreeLob_NODE( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_uint64_t   aLocator )
{
    ACP_UNUSED( aFnContext );

    return ulnFreeLob( SQL_HANDLE_STMT,
                       (ulnObject *)aStmt,
                       aLocator );
}

SQLRETURN ulsdModuleTrimLob_NODE( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_sint16_t   aLocatorCType,
                                  acp_uint64_t   aLocator,
                                  acp_uint32_t   aStartOffset )
{
    ACP_UNUSED( aFnContext );

    return ulnTrimLob( SQL_HANDLE_STMT,
                       (ulnObject *)aStmt,
                       aLocatorCType,
                       aLocator,
                       aStartOffset );
}

ulsdModule gShardModuleNODE =
{
    ulsdModuleHandshake_NODE,
    ulsdModuleNodeDriverConnect_NODE,
    ulsdModuleNodeConnect_NODE,
    ulsdModuleEnvRemoveDbc_NODE,
    ulsdModuleStmtDestroy_NODE,
    ulsdModulePrepare_NODE,
    ulsdModuleExecute_NODE,
    ulsdModuleFetch_NODE,
    ulsdModuleCloseCursor_NODE,
    ulsdModuleRowCount_NODE,
    ulsdModuleMoreResults_NODE,
    ulsdModuleGetPreparedStmt_NODE,
    ulsdModuleOnCmError_NODE,
    ulsdModuleUpdateNodeList_NODE,
    ulsdModuleNotifyFailOver_NODE,
    ulsdModuleAlignDataNodeConnection_NODE,
    ulsdModuleErrorCheckAndAlignDataNode_NODE,
    ulsdModuleHasNoData_NODE,

    /* PROJ-2739 Client-side Sharding LOB */
    ulsdModuleGetLobLength_NODE,
    ulsdModuleGetLob_NODE,
    ulsdModulePutLob_NODE,
    ulsdModuleFreeLob_NODE,
    ulsdModuleTrimLob_NODE
};
