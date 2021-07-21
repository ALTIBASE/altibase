/***********************************************************************
 * Copyright 1999-2012, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsd.h>
#include <ulsdFailover.h>
#include <ulsdDistTxInfo.h>
#include <ulsdRebuild.h>
#include <ulsdUtils.h>

ACI_RC ulsdModuleHandshake_COORD(ulnFnContext *aFnContext)
{
    return ulsdHandshake(aFnContext);
}

SQLRETURN ulsdModuleNodeDriverConnect_COORD(ulnDbc       *aDbc,
                                            ulnFnContext *aFnContext,
                                            acp_char_t   *aConnString,
                                            acp_sint16_t  aConnStringLength)
{
    return ulsdNodeDriverConnect(aDbc,
                                 aFnContext,
                                 aConnString,
                                 aConnStringLength);
}

SQLRETURN ulsdModuleNodeConnect_COORD( ulnDbc       *aDbc,
                                       ulnFnContext *aFnContext,
                                       acp_char_t   *aServerName,
                                       acp_sint16_t  aServerNameLength,
                                       acp_char_t   *aUserName,
                                       acp_sint16_t  aUserNameLength,
                                       acp_char_t   *aPassword,
                                       acp_sint16_t  aPasswordLength )
{
    return ulsdNodeConnect( aDbc,
                            aFnContext,
                            aServerName,
                            aServerNameLength,
                            aUserName,
                            aUserNameLength,
                            aPassword,
                            aPasswordLength );
}

ACI_RC ulsdModuleEnvRemoveDbc_COORD(ulnEnv *aEnv, ulnDbc *aDbc)
{
    ACI_TEST(ulnEnvRemoveDbc(aEnv, aDbc) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulsdModuleStmtDestroy_COORD(ulnStmt *aStmt)
{
    ulsdNodeStmtDestroy(aStmt);
}

SQLRETURN ulsdModulePrepare_COORD(ulnFnContext *aFnContext,
                                  ulnStmt      *aStmt,
                                  acp_char_t   *aStatementText,
                                  acp_sint32_t  aTextLength,
                                  acp_char_t   *aAnalyzeText)
{
    SQLRETURN     sResult;

    sResult = ulsdCheckDbcSMN( aFnContext, aStmt->mParentDbc );
    ACI_TEST( SQL_SUCCEEDED( sResult ) == 0 );

    sResult = ulnPrepare(aStmt,
                         aStatementText,
                         aTextLength,
                         aAnalyzeText);
    ACI_TEST_RAISE( SQL_SUCCEEDED( sResult ) == 0,
                    ERR_PREPARE_FAIL );

    return sResult;

    ACI_EXCEPTION( ERR_PREPARE_FAIL )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, sResult );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleExecute_COORD(ulnFnContext *aFnContext,
                                  ulnStmt      *aStmt)
{
    ulsdDbc      *sShard = NULL;
    SQLRETURN     sNodeResult;

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sNodeResult = ulsdCheckDbcSMN( aFnContext, aStmt->mParentDbc );
    ACI_TEST( sNodeResult != SQL_SUCCESS );

    if ( aStmt->mParentDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        ACI_TEST_RAISE( aStmt->mParentDbc->mAttrGlobalTransactionLevel
                        == ULN_GLOBAL_TX_ONE_NODE,
                        LABEL_SHARD_META_CANNOT_TOUCH );

        sShard->mTouched = ACP_TRUE;
    }
    else
    {
        /* Do Nothing */
    }

    /* PROJ-2733-DistTxInfo DistTxInfo를 계산한다. */
    ulsdCalcDistTxInfoForCoord(sShard->mMetaDbc);

    /* BUG-48315 */
    ACI_TEST( ulsdBuildClientTouchNodeArr( aFnContext,
                                           sShard,
                                           ulnStmtGetStatementType(aStmt) )
              != ACI_SUCCESS );

    sNodeResult = ulnExecute(aStmt);

    SHARD_LOG("(Execute) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    /* PROJ-2739 Client-side Sharding LOB */
    if ( aStmt->mShardStmtCxt.mHasLocatorParamToCopy == ACP_TRUE &&
         sNodeResult != SQL_ERROR )
    {
        sNodeResult = ulsdLobCopy( aFnContext,
                                   sShard,
                                   aStmt );
    }
    else
    {
        /* Nothing to do */
    }

    return sNodeResult;

    ACI_EXCEPTION(LABEL_SHARD_META_CANNOT_TOUCH)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Server-side query can not be performed in single-node transaction");
    }
    ACI_EXCEPTION_END;

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| fail: %"ACI_INT32_FMT,
            "ulsdExecute", aFnContext->mSqlReturn);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdModuleFetch_COORD(ulnFnContext *aFnContext,
                                ulnStmt      *aStmt)
{
    SQLRETURN     sNodeResult;

    ACP_UNUSED(aFnContext);

    sNodeResult = ulnFetch(aStmt);

    SHARD_LOG("(Fetch) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;
}

SQLRETURN ulsdModuleCloseCursor_COORD(ulnStmt *aStmt)
{
    return ulnCloseCursor(aStmt);
}

SQLRETURN ulsdModuleRowCount_COORD(ulnFnContext *aFnContext,
                                   ulnStmt      *aStmt,
                                   ulvSLen      *aRowCount)
{
    SQLRETURN     sNodeResult;

    ACP_UNUSED(aFnContext);

    sNodeResult = ulnRowCount(aStmt, aRowCount);

    SHARD_LOG("(Row Count) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;
}

SQLRETURN ulsdModuleMoreResults_COORD(ulnFnContext *aFnContext,
                                      ulnStmt      *aStmt)
{
    SQLRETURN     sNodeResult;

    ACP_UNUSED(aFnContext);

    sNodeResult = ulnMoreResults(aStmt);

    SHARD_LOG("(More Results) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;
}

ulnStmt* ulsdModuleGetPreparedStmt_COORD(ulnStmt *aStmt)
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

void ulsdModuleOnCmError_COORD(ulnFnContext     *aFnContext,
                               ulnDbc           *aDbc,
                               ulnErrorMgr      *aErrorMgr)
{
    ulsdDbc      *sShard = NULL;

    (void)ulsdFODoSTF(aFnContext, aDbc, aErrorMgr);

    if ( aDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        sShard = aDbc->mShardDbcCxt.mShardDbc;
        ulsdSetTouchedToAllNodes( sShard );
    }

    return;
}

ACI_RC ulsdModuleUpdateNodeList_COORD(ulnFnContext  *aFnContext,
                                      ulnDbc        *aDbc)
{
    ACI_RC sRc;

    /* BUG-47131 Stop shard META DBC failover. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_ON_STATE );

    sRc = ulsdUpdateNodeList(aFnContext, &(aDbc->mPtContext));
    ACI_TEST( sRc != ACI_SUCCESS );

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;

    ACI_EXCEPTION_END;

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;
}

ACI_RC ulsdModuleNotifyFailOver_COORD( ulnDbc *aDbc )
{
    ACI_RC sRc;

    /* BUG-47131 Stop shard META DBC failover. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_ON_STATE );

    sRc = ulsdNotifyFailoverOnMeta( aDbc );
    ACI_TEST( sRc != ACI_SUCCESS );

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;

    ACI_EXCEPTION_END;

    /* BUG-47131 Restore shard META DBC failover suspend status. */
    ulnDbcSetFailoverSuspendState( aDbc, ULN_FAILOVER_SUSPEND_OFF_STATE );

    return sRc;
}

void ulsdModuleAlignDataNodeConnection_COORD( ulnFnContext * aFnContext,
                                              ulnDbc       * aNodeDbc )
{
    ulsdAlignDataNodeConnection( aFnContext,
                                 aNodeDbc );
}

void ulsdModuleErrorCheckAndAlignDataNode_COORD( ulnFnContext * aFnContext )
{
    ulsdErrorCheckAndAlignDataNode( aFnContext );
}

acp_bool_t ulsdModuleHasNoData_COORD( ulnStmt * aStmt )
{
    return ulnCursorHasNoData( ulnStmtGetCursor( aStmt ) );
}

/*
 * PROJ-2739 Client-side Sharding LOB
 */
SQLRETURN ulsdModuleGetLobLength_COORD( ulnFnContext * aFnContext,
                                        ulnStmt      * aStmt,
                                        acp_uint64_t   aLocator,
                                        acp_sint16_t   aLocatorType,
                                        acp_uint32_t * aLengthPtr )
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    acp_uint16_t       sIsNull;
    ulsdLobLocator    *sShardLocator;

    ACP_UNUSED(aFnContext);

    sShardLocator = (ulsdLobLocator *)aLocator;
    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACE_ASSERT( sShardLocator->mNodeDbcIndex == -1 );

    sNodeResult = ulnGetLobLength( SQL_HANDLE_STMT,
                                   (ulnObject *)aStmt,
                                   sShardLocator->mLobLocator,
                                   aLocatorType,
                                   aLengthPtr,
                                   &sIsNull );

    SHARD_LOG("(Get LobLengh) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

SQLRETURN ulsdModuleGetLob_COORD( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_sint16_t   aLocatorCType,
                                  acp_uint64_t   aLocator,
                                  acp_uint32_t   aFromPosition,
                                  acp_uint32_t   aForLength,
                                  acp_sint16_t   aTargetCType,
                                  void         * aBuffer,
                                  acp_uint32_t   aBufferSize,
                                  acp_uint32_t * aLengthWritten )
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    ulsdLobLocator    *sShardLocator;

    ACP_UNUSED(aFnContext);

    sShardLocator = (ulsdLobLocator *)aLocator;
    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACE_ASSERT( sShardLocator->mNodeDbcIndex == -1 );

    sNodeResult = ulnGetLob( SQL_HANDLE_STMT,
                             (ulnObject *)aStmt,
                             aLocatorCType,
                             sShardLocator->mLobLocator,
                             aFromPosition,
                             aForLength,
                             aTargetCType,
                             aBuffer,
                             aBufferSize,
                             aLengthWritten );

    SHARD_LOG("(Get Lob) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

SQLRETURN ulsdModulePutLob_COORD( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_sint16_t   aLocatorCType,
                                  acp_uint64_t   aLocator,
                                  acp_uint32_t   aFromPosition,
                                  acp_uint32_t   aForLength,
                                  acp_sint16_t   aSourceCType,
                                  void         * aBuffer,
                                  acp_uint32_t   aBufferSize )
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    ulsdLobLocator    *sShardLocator;

    ACP_UNUSED(aFnContext);

    sShardLocator = (ulsdLobLocator *)aLocator;
    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACE_ASSERT( sShardLocator->mNodeDbcIndex == -1 );

    sNodeResult = ulnPutLob( SQL_HANDLE_STMT,
                             (ulnObject *)aStmt,
                             aLocatorCType,
                             sShardLocator->mLobLocator,
                             aFromPosition,
                             aForLength,
                             aSourceCType,
                             aBuffer,
                             aBufferSize );

    SHARD_LOG("(Put Lob) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

SQLRETURN ulsdModuleFreeLob_COORD( ulnFnContext * aFnContext,
                                   ulnStmt      * aStmt,
                                   acp_uint64_t   aLocator )
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    ulsdLobLocator    *sShardLocator;

    sShardLocator = (ulsdLobLocator *)aLocator;
    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_CONT( sShardLocator->mArraySize == 0, NORMAL_EXIT );
    ACE_ASSERT( sShardLocator->mNodeDbcIndex == -1 );

    sNodeResult = ulnFreeLob( SQL_HANDLE_STMT,
                              (ulnObject *)aStmt,
                              sShardLocator->mLobLocator );

    SHARD_LOG("(Free Lob) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    ulsdLobLocatorDestroy( aStmt->mParentDbc,
                           sShardLocator,
                           ACP_TRUE /* NeedLock */);

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return sNodeResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

SQLRETURN ulsdModuleTrimLob_COORD( ulnFnContext  * aFnContext,
                                   ulnStmt       * aStmt,
                                   acp_sint16_t    aLocatorCType,
                                   acp_uint64_t    aLocator,
                                   acp_uint32_t    aStartOffset )
{
    SQLRETURN          sNodeResult = SQL_ERROR;
    ulsdLobLocator    *sShardLocator;

    sShardLocator = (ulsdLobLocator *)aLocator;
    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACE_ASSERT( sShardLocator->mNodeDbcIndex == -1 );

    sNodeResult = ulnTrimLob( SQL_HANDLE_STMT,
                              (ulnObject *)aStmt,
                              aLocatorCType,
                              sShardLocator->mLobLocator,
                              aStartOffset );

    SHARD_LOG("(Trim Lob) Meta Node, StmtID=%d\n",
              aStmt->mStatementID);

    return sNodeResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

ulsdModule gShardModuleCOORD =
{
    ulsdModuleHandshake_COORD,
    ulsdModuleNodeDriverConnect_COORD,
    ulsdModuleNodeConnect_COORD,
    ulsdModuleEnvRemoveDbc_COORD,
    ulsdModuleStmtDestroy_COORD,
    ulsdModulePrepare_COORD,
    ulsdModuleExecute_COORD,
    ulsdModuleFetch_COORD,
    ulsdModuleCloseCursor_COORD,
    ulsdModuleRowCount_COORD,
    ulsdModuleMoreResults_COORD,
    ulsdModuleGetPreparedStmt_COORD,
    ulsdModuleOnCmError_COORD,
    ulsdModuleUpdateNodeList_COORD,
    ulsdModuleNotifyFailOver_COORD,
    ulsdModuleAlignDataNodeConnection_COORD,
    ulsdModuleErrorCheckAndAlignDataNode_COORD,
    ulsdModuleHasNoData_COORD,

    /* PROJ-2739 Client-side Sharding LOB */
    ulsdModuleGetLobLength_COORD,
    ulsdModuleGetLob_COORD,
    ulsdModulePutLob_COORD,
    ulsdModuleFreeLob_COORD,
    ulsdModuleTrimLob_COORD
};
