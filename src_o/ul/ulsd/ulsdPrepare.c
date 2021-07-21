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
#include <ulnStateMachine.h>
#include <mtcc.h>

#include <ulsd.h>
#include <ulsdnExecute.h>
#include <ulsdRebuild.h>

static ACI_RC ulsdReadShardValue( cmiProtocolContext        * aProtocolContext,
                                  ulnDbc                    * aDbc,
                                  acp_uint32_t                aValueType,
                                  acp_uint32_t                aValueLength,
                                  ulsdValue                 * aValue )
{
    switch( aValueType )
    {
        case MTD_SMALLINT_ID:
            ACE_DASSERT( aValueLength == 2 );
            CMI_RD2(aProtocolContext, (acp_uint16_t*)
                    &(aValue->mSmallintMax));
            break;

        case MTD_INTEGER_ID:
            ACE_DASSERT( aValueLength == 4 );
            CMI_RD4(aProtocolContext, (acp_uint32_t*)
                    &(aValue->mIntegerMax));
            break;

        case MTD_BIGINT_ID:
            ACE_DASSERT( aValueLength == 8 );
            CMI_RD8(aProtocolContext, (acp_uint64_t*)
                    &(aValue->mBigintMax));
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            ACI_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                          aValueLength,
                                          aValue->mCharMax.value,
                                          aDbc->mConnTimeoutValue )
                            != ACI_SUCCESS, CM_ERROR );
            aValue->mCharMax.length = aValueLength;
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( CM_ERROR )
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

static ACI_RC ulsdSkipReadShardValue( cmiProtocolContext        * aProtocolContext,
                                      ulnDbc                    * aDbc,
                                      acp_uint32_t                aValueType,
                                      acp_uint32_t                aValueLength )
{
    switch( aValueType )
    {
        case MTD_SMALLINT_ID:
            CMI_SKIP_READ_BLOCK(aProtocolContext, 2);
            break;

        case MTD_INTEGER_ID:
            CMI_SKIP_READ_BLOCK(aProtocolContext, 4);
            break;

        case MTD_BIGINT_ID:
            CMI_SKIP_READ_BLOCK(aProtocolContext, 8);
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:

            ACI_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                              aValueLength,
                                              aDbc->mConnTimeoutValue )
                            != ACI_SUCCESS, CM_ERROR );
            break;

        default:
            ACE_ASSERT(0);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( CM_ERROR )
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}


static ACI_RC ulsdReadShardValueInfo( cmiProtocolContext    * aProtocolContext,
                                      ulnDbc                * aDbc,
                                      ulsdValueInfo        ** aValueInfo )
{
    ulsdValueInfo   * sValueInfo = NULL;
    acp_uint8_t       sType = 0;
    acp_uint32_t      sValueType = 0;
    acp_uint32_t      sLength = 0;
    acp_bool_t        sIsAlloc = ACP_FALSE;
    acp_uint32_t      sStage = 0;

    CMI_RD1( aProtocolContext, sType );
    ACE_ASSERT( sType < 2 );

    CMI_RD4( aProtocolContext, &sValueType );
    CMI_RD4( aProtocolContext, &sLength );
    sStage = 1;

    if ( sLength <= ACI_SIZEOF( ulsdValue ) )
    {
        ACI_TEST( ACP_RC_NOT_SUCCESS( acpMemAlloc( (void **)&sValueInfo,
                                                   ACI_SIZEOF(ulsdValueInfo) ) ) );
    }
    else
    {
        ACI_TEST( ACP_RC_NOT_SUCCESS( acpMemAlloc( (void **)&sValueInfo,
                                                   ACI_SIZEOF(ulsdValueInfo) + sLength ) ) );
    }
    sIsAlloc = ACP_TRUE;

    sValueInfo->mDataValueType = sValueType;

    ACI_TEST_RAISE( ulsdReadShardValue( aProtocolContext,
                                        aDbc,
                                        sValueType,
                                        sLength,
                                        &(sValueInfo->mValue) )
                    != ACI_SUCCESS, CM_ERROR );

    sValueInfo->mType = sType;

    *aValueInfo = sValueInfo;

    return ACI_SUCCESS;

    ACI_EXCEPTION( CM_ERROR )
    {
        (void)acpMemFree( sValueInfo );

        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    if ( sIsAlloc == ACP_TRUE )
    {
        (void)acpMemFree( sValueInfo );
    }

    switch( sStage )
    {
        case 1:
            if ( ulsdSkipReadShardValue( aProtocolContext,
                                         aDbc,
                                         sValueType,
                                         sLength )
                 != ACI_SUCCESS )
            {
                return ACI_FAILURE;
            }

        default:
            break;
    }

    return ACI_SUCCESS;
}

static ACI_RC ulsdSkipReadShardValueInfo( cmiProtocolContext    * aProtocolContext,
                                          ulnDbc                * aDbc )
{
    acp_char_t        sType;
    acp_uint32_t      sValueType = 0;
    acp_uint32_t      sLength = 0;

    CMI_RD1( aProtocolContext, sType );

    CMI_RD4( aProtocolContext, &sValueType );
    CMI_RD4( aProtocolContext, &sLength );

    ACP_UNUSED( sType );

    ACI_TEST_RAISE( ulsdSkipReadShardValue( aProtocolContext,
                                            aDbc,
                                            sValueType,
                                            sLength )
                    != ACI_SUCCESS, CM_ERROR );

    return ACI_SUCCESS;

    ACI_EXCEPTION( CM_ERROR )
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    return ACI_SUCCESS;
}

ACI_RC ulsdCallbackAnalyzeResult(cmiProtocolContext *aProtocolContext,
                                 cmiProtocol        *aProtocol,
                                 void               *aServiceSession,
                                 void               *aUserContext)
{
    ulnFnContext          *sFnContext  = (ulnFnContext *)aUserContext;
    ulnStmt               *sStmt       = sFnContext->mHandle.mStmt;
    ulnDbc                *sDbc        = NULL;
    acp_uint32_t           sStatementID;
    acp_uint32_t           sStatementType;
    acp_uint16_t           sParamCount;
    acp_uint16_t           sResultSetCount;
    acp_uint16_t           sStatePoints;
    acp_bool_t             sCoordQuery = ACP_FALSE;

    /* PROJ-2598 Shard pilot(shard analyze) */
    acp_uint8_t            sShardSplitMethod;
    acp_uint32_t           sShardKeyDataType;
    acp_uint32_t           sShardDefaultNodeID;
    acp_uint16_t           sShardRangeInfoCnt;
    acp_uint16_t           sShardRangeIdx = 0;
    ulsdRangeInfo         *sShardRange;

    /* PROJ-2646 New shard analyzer */
    acp_uint16_t           sShardValueCnt = 0;
    acp_uint16_t           sShardValueIdx = 0;
    acp_uint16_t           sTryShardValueCnt = 0;
    acp_uint8_t            sIsShardQuery;
    ulsdValueInfo        * sShardValueInfo = NULL;

    /* PROJ-2655 Composite shard key */
    acp_uint8_t            sIsSubKeyExists;
    acp_uint8_t            sShardSubSplitMethod;
    acp_uint32_t           sShardSubKeyDataType;
    acp_uint16_t           sShardSubValueCnt = 0;
    acp_uint16_t           sShardSubValueIdx = 0;
    acp_uint16_t           sTryShardSubValueCnt = 0;

    acp_uint16_t           sState = 0;

    /* TASK-7219 Non-shard DML */
    acp_uint8_t            sIsPartialCoordExecNeeded;

    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aServiceSession);

    ULN_FNCONTEXT_GET_DBC( sFnContext, sDbc );

    SHARD_LOG("(Shard Prepare Result) Shard Prepare Result\n");

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD4(aProtocolContext, &sStatementType);
    CMI_RD2(aProtocolContext, &sParamCount);
    CMI_RD2(aProtocolContext, &sResultSetCount);
    CMI_RD1(aProtocolContext, sShardSplitMethod);
    CMI_RD1(aProtocolContext, sIsPartialCoordExecNeeded); /* TASK-7219 Non-shard DML */
    CMI_RD4(aProtocolContext, &sShardKeyDataType);
    CMI_RD1(aProtocolContext, sIsSubKeyExists );
    if ( sIsSubKeyExists == ACP_TRUE )
    {
        CMI_RD1(aProtocolContext, sShardSubSplitMethod);
        CMI_RD4(aProtocolContext, &sShardSubKeyDataType);
    }
    CMI_RD4(aProtocolContext, &sShardDefaultNodeID);

    CMI_RD1(aProtocolContext, sIsShardQuery);

    CMI_RD2(aProtocolContext, &sShardValueCnt);
    if ( sIsSubKeyExists == ACP_TRUE )
    {
        CMI_RD2(aProtocolContext, &sShardSubValueCnt);
    }

    ACI_TEST_RAISE(ULN_OBJ_GET_TYPE(sStmt) != ULN_OBJ_TYPE_STMT, LABEL_MEM_MANAGE_ERR);

    ulnStmtFreeAllResult(sStmt);
    ulnShardStmtFreeAllShardValueInfo( sStmt );

    sStmt->mIsPrepared  = ACP_TRUE;
    ulnStmtSetStatementID(sStmt, sStatementID);
    ulnStmtSetStatementType(sStmt, sStatementType);

    ulnStmtSetParamCount(sStmt, sParamCount);
    ulnStmtSetResultSetCount(sStmt, sResultSetCount);

    if (sResultSetCount > 0)
    {
        sStmt->mResultType = ULN_RESULT_TYPE_RESULTSET;
    }
    else
    {
        sStmt->mResultType = ULN_RESULT_TYPE_ROWCOUNT;
    }

    if (ulnStmtGetAttrDeferredPrepare(sStmt) == ULN_CONN_DEFERRED_PREPARE_ON)
    {
        if(sStmt->mDeferredPrepareStateFunc != NULL)
        {
            sStatePoints= sFnContext->mWhere;
            sFnContext->mWhere = ULN_STATE_EXIT_POINT;
            ACI_TEST_RAISE(sStmt->mDeferredPrepareStateFunc(sFnContext) != ACI_SUCCESS,
                           LABEL_DEFERRED_PREPARE);

            sFnContext->mWhere = sStatePoints;
        }
    }

    /* PROJ-2598 Shard pilot(shard analyze) */
    ulsdStmtSetShardSplitMethod( sStmt, sShardSplitMethod );
    ulsdStmtSetShardKeyDataType( sStmt, sShardKeyDataType );

    ulsdStmtSetShardDefaultNodeID( sStmt, sShardDefaultNodeID );

    SHARD_LOG("(Shard Prepare Result) shardSplitMethod : %d\n",sStmt->mShardStmtCxt.mShardSplitMethod);
    SHARD_LOG("(Shard Prepare Result) shardKeyDataType : %d\n",sStmt->mShardStmtCxt.mShardKeyDataType);
    SHARD_LOG("(Shard Prepare Result) shardDefaultNodeID : %d\n",sStmt->mShardStmtCxt.mShardDefaultNodeID);

    if ( sIsSubKeyExists == ACP_TRUE )
    {
        ulsdStmtSetShardSubSplitMethod( sStmt, sShardSubSplitMethod );
        ulsdStmtSetShardSubKeyDataType( sStmt, sShardSubKeyDataType );
        ulsdStmtSetShardSubValueCnt( sStmt, sShardSubValueCnt );

        SHARD_LOG("(Shard Prepare Result) shardSubSplitMethod : %d\n",sStmt->mShardStmtCxt.mShardSubSplitMethod);
        SHARD_LOG("(Shard Prepare Result) shardSubKeyDataType : %d\n",sStmt->mShardStmtCxt.mShardSubKeyDataType);
    }


    /* PROJ-2655 Composite shard key */
    ulsdStmtSetShardIsSubKeyExists( sStmt, sIsSubKeyExists );
    SHARD_LOG("(Shard Prepare Result) shardIsSubKeyExists : %d\n",sStmt->mShardStmtCxt.mShardIsSubKeyExists);

    /* PROJ-2646 New shard analyzer */
    if ( sIsShardQuery == 1 )
    {
        ulsdStmtSetShardIsShardQuery( sStmt, ACP_TRUE );
    }
    else
    {
        ulsdStmtSetShardIsShardQuery( sStmt, ACP_FALSE );
    }

    /* TASK-7219 Non-shard DML */
    if ( sIsPartialCoordExecNeeded == 1 )
    {
        ulsdStmtSetPartialExecType( sStmt, ULN_SHARD_PARTIAL_EXEC_TYPE_COORD );
    }
    else if ( sIsPartialCoordExecNeeded == 2 )
    {
        ulsdStmtSetPartialExecType( sStmt, ULN_SHARD_PARTIAL_EXEC_TYPE_QUERY );
    }
    else
    {
        ulsdStmtSetPartialExecType( sStmt, ULN_SHARD_PARTIAL_EXEC_TYPE_NONE );
    }

    ulsdStmtSetShardValueCnt( sStmt, sShardValueCnt );
    ulsdStmtSetShardSubValueCnt( sStmt, sShardSubValueCnt );

    SHARD_LOG("(Shard Prepare Result) shardIsShardQuery : %d\n",sStmt->mShardStmtCxt.mShardIsShardQuery);
    SHARD_LOG("(Shard Prepare Result) shardValueCount : %d\n",sStmt->mShardStmtCxt.mShardValueCnt);

    if ( sStmt->mShardStmtCxt.mShardValueCnt > 0 )
    {
        ACI_TEST( ACP_RC_NOT_SUCCESS( acpMemCalloc( (void **)&sStmt->mShardStmtCxt.mShardValueInfoPtrArray,
                                                    ACI_SIZEOF(ulsdValueInfo*),
                                                    sShardValueCnt ) ) );

        for ( sShardValueIdx = 0; sShardValueIdx < sShardValueCnt; sShardValueIdx++ )
        {
            sTryShardValueCnt++;

            ACI_TEST_RAISE( ulsdReadShardValueInfo( aProtocolContext,
                                                    sDbc,
                                                    &sShardValueInfo )
                            != ACI_SUCCESS, CM_ERROR );

            SHARD_LOG("(Shard Prepare Result) mType : %d\n", sShardValueInfo->mType );
            sStmt->mShardStmtCxt.mShardValueInfoPtrArray[sShardValueIdx] = sShardValueInfo;
        }
    }
    else
    {
        /* Nothing to do. */
    }

    sState = 1;

    if ( sStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE )
    {
        SHARD_LOG("(Shard Prepare Result) shardSubValueCount : %d\n",sStmt->mShardStmtCxt.mShardSubValueCnt);

        if ( sStmt->mShardStmtCxt.mShardSubValueCnt > 0 )
        {
                ACI_TEST( ACP_RC_NOT_SUCCESS( acpMemCalloc( (void **)&sStmt->mShardStmtCxt.mShardSubValueInfoPtrArray,
                                                            ACI_SIZEOF(ulsdValueInfo*),
                                                            sShardSubValueCnt) ) );

            for ( sShardSubValueIdx = 0; sShardSubValueIdx < sShardSubValueCnt; sShardSubValueIdx++ )
            {
                sTryShardSubValueCnt++;

                ACI_TEST_RAISE( ulsdReadShardValueInfo( aProtocolContext,
                                                        sDbc,
                                                        &sShardValueInfo )
                                != ACI_SUCCESS, CM_ERROR );

                SHARD_LOG("(Shard Prepare Result) mType : %d\n", sShardValueInfo->mType);
                sStmt->mShardStmtCxt.mShardSubValueInfoPtrArray[sShardSubValueIdx] = sShardValueInfo;
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    CMI_RD2(aProtocolContext, &sShardRangeInfoCnt);
    ulsdStmtSetShardRangeInfoCnt( sStmt, sShardRangeInfoCnt );
    SHARD_LOG("(Shard Prepare Result) shardNodeInfoCnt : %d\n",sStmt->mShardStmtCxt.mShardRangeInfoCnt);

    if ( sStmt->mShardStmtCxt.mShardRangeInfo != NULL )
    {
        acpMemFree( sStmt->mShardStmtCxt.mShardRangeInfo );
        sStmt->mShardStmtCxt.mShardRangeInfo = NULL;
    }

    sState = 2;

    if ( sStmt->mShardStmtCxt.mShardRangeInfoCnt > 0 )
    {
        ACI_TEST(ACP_RC_NOT_SUCCESS(acpMemAlloc((void **)&sStmt->mShardStmtCxt.mShardRangeInfo,
                                                ACI_SIZEOF(ulsdRangeInfo) * sShardRangeInfoCnt)));

        for ( sShardRangeIdx = 0; sShardRangeIdx < sShardRangeInfoCnt; sShardRangeIdx++ )
        {
            sShardRange = &(sStmt->mShardStmtCxt.mShardRangeInfo[sShardRangeIdx]);

            if ( sShardSplitMethod == ULN_SHARD_SPLIT_HASH )  /* hash shard */
            {
                CMI_RD4(aProtocolContext, &(sShardRange->mShardRange.mHashMax));
            }
            else if ( sShardSplitMethod == ULN_SHARD_SPLIT_RANGE )  /* range shard */
            {
                ulsdReadMtValue(aProtocolContext,
                                sShardKeyDataType,
                                &sShardRange->mShardRange);
            }
            else if ( sShardSplitMethod == ULN_SHARD_SPLIT_LIST )  /* list shard */
            {
                ulsdReadMtValue(aProtocolContext,
                                sShardKeyDataType,
                                &sShardRange->mShardRange);
            }
            else if ( sShardSplitMethod == ULN_SHARD_SPLIT_CLONE )  /* clone */
            {
                /* Nothing to do. */
                /* Clone table은 NodeID만 전달받는다. */
            }
            else if ( sShardSplitMethod == ULN_SHARD_SPLIT_SOLO )  /* solo */
            {
                /* Nothing to do. */
                /* Solo table은 NodeID만 전달받는다. */
            }
            else
            {
                ACE_ASSERT(0);
            }

            if ( sStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE )
            {
                if ( sShardSubSplitMethod == ULN_SHARD_SPLIT_HASH )  /* hash shard */
                {
                    CMI_RD4(aProtocolContext, &(sShardRange->mShardSubRange.mHashMax));
                }
                else if ( sShardSubSplitMethod == ULN_SHARD_SPLIT_RANGE )  /* range shard */
                {
                    ulsdReadMtValue(aProtocolContext,
                                    sShardSubKeyDataType,
                                    &sShardRange->mShardSubRange );
                }
                else if ( sShardSubSplitMethod == ULN_SHARD_SPLIT_LIST )  /* list shard */
                {
                    ulsdReadMtValue(aProtocolContext,
                                    sShardSubKeyDataType,
                                    &sShardRange->mShardSubRange );
                }
                else if ( sShardSubSplitMethod == ULN_SHARD_SPLIT_CLONE )  /* clone */
                {
                    /* Nothing to do. */
                    /* Clone table은 NodeID만 전달받는다. */
                }
                else if ( sShardSubSplitMethod == ULN_SHARD_SPLIT_SOLO )  /* solo */
                {
                    /* Nothing to do. */
                    /* Solo table은 NodeID만 전달받는다. */
                }
                else
                {
                    ACE_ASSERT(0);
                }
            }
            else
            {
                /* Nothing to do. */
            }

            CMI_RD4(aProtocolContext, &(sShardRange->mShardNodeID));

            SHARD_LOG("(Shard Prepare Result) shardNodeId[%d] : %d\n",
                      sShardRangeIdx, sStmt->mShardStmtCxt.mShardRangeInfo[sShardRangeIdx].mShardNodeID);
        }
    }
    else
    {
        /* Do Nothing */
    }

    /* shard meta query */
    if ( sStmt->mShardStmtCxt.mShardIsShardQuery == ACP_FALSE )
    {
        sCoordQuery = ACP_TRUE;
    }
    else
    {
        /* BUG-45640 autocommit on + global_tx + multi-shard query */
        if ( ( sStmt->mParentDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_ON ) &&
             ( ulsdIsGTx( sStmt->mParentDbc->mAttrGlobalTransactionLevel ) == ACP_TRUE ) &&
             ( sStmt->mShardStmtCxt.mShardRangeInfoCnt > 1 ) )
        {
            if ( ( sStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_HASH ) ||
                 ( sStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_RANGE ) ||
                 ( sStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_LIST ) )
            {
                if ( sStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_FALSE )
                {
                    if ( sStmt->mShardStmtCxt.mShardValueCnt != 1 )
                    {
                        sCoordQuery = ACP_TRUE;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    if ( ( sStmt->mShardStmtCxt.mShardValueCnt != 1 ) ||
                         ( sStmt->mShardStmtCxt.mShardSubValueCnt != 1 ) )
                    {
                        sCoordQuery = ACP_TRUE;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            else if ( sStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_CLONE )
            {
                if ( sStmt->mShardStmtCxt.mShardValueCnt != 0 )
                {
                    sCoordQuery = ACP_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }

    if ( sCoordQuery == ACP_TRUE )
    {
        sStmt->mShardStmtCxt.mShardCoordQuery = ACP_TRUE;

        ulsdSetStmtShardModule(sStmt, &gShardModuleCOORD);
    }
    else
    {
        sStmt->mShardStmtCxt.mShardCoordQuery = ACP_FALSE;

        ulsdSetStmtShardModule(sStmt, &gShardModuleMETA);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_MEM_MANAGE_ERR)
    {
        ulnError(sFnContext,
                 ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                 "Shard Prepare Result",
                 "Object is not a DBC handle.");
    }
    ACI_EXCEPTION(LABEL_DEFERRED_PREPARE)
    {
        sFnContext->mWhere = sStatePoints;
    }
    ACI_EXCEPTION( CM_ERROR )
    {
        return ACI_FAILURE;
    }
    ACI_EXCEPTION_END;

    switch ( sState )
    {
        case 0:
            for ( sShardValueIdx = sTryShardValueCnt; sShardValueIdx < sShardValueCnt; sShardValueIdx++ )
            {
                if ( ulsdSkipReadShardValueInfo( aProtocolContext, sDbc ) != ACI_SUCCESS )
                {
                    return ACI_FAILURE;
                }
            }

        case 1:
            for ( sShardSubValueIdx = sTryShardSubValueCnt; 
                  sShardSubValueIdx < sShardSubValueCnt; 
                  sShardSubValueIdx++ )
            {
                if ( ulsdSkipReadShardValueInfo( aProtocolContext, sDbc ) != ACI_SUCCESS )
                {
                    return ACI_FAILURE;
                }
            }

            CMI_RD2(aProtocolContext, &sShardRangeInfoCnt);

        case 2:
            for ( sShardRangeIdx = 0; sShardRangeIdx < sShardRangeInfoCnt; sShardRangeIdx++ )
            {
                sShardRange = &(sStmt->mShardStmtCxt.mShardRangeInfo[sShardRangeIdx]);

                if ( sShardSplitMethod == ULN_SHARD_SPLIT_HASH )
                {
                    CMI_SKIP_READ_BLOCK(aProtocolContext, 4);   // mShardRange.mHashMax
                }
                else if ( ( sShardSplitMethod == ULN_SHARD_SPLIT_RANGE ) ||
                          ( sShardSplitMethod == ULN_SHARD_SPLIT_LIST ) )
                {
                    ulsdSkipReadMtValue( aProtocolContext, sShardKeyDataType );
                }
                else if ( ( sShardSplitMethod == ULN_SHARD_SPLIT_CLONE ) ||
                          ( sShardSplitMethod == ULN_SHARD_SPLIT_SOLO ) )
                {
                    /* Nothing to do. */
                    /* Clone, Solo table은 NodeID만 전달받는다. */
                }
                else
                {
                    ACE_ASSERT(0);
                }

                if ( sIsSubKeyExists == ACP_TRUE )
                {
                    if ( sShardSubSplitMethod == ULN_SHARD_SPLIT_HASH )
                    {
                        CMI_SKIP_READ_BLOCK(aProtocolContext, 4); // mShardSubRange.mHashMax
                    }
                    else if ( ( sShardSubSplitMethod == ULN_SHARD_SPLIT_RANGE ) ||
                              ( sShardSubSplitMethod == ULN_SHARD_SPLIT_LIST ) )
                    {
                        ulsdSkipReadMtValue( aProtocolContext, sShardSubKeyDataType );
                    }
                    else if ( ( sShardSubSplitMethod == ULN_SHARD_SPLIT_CLONE ) ||
                              ( sShardSubSplitMethod == ULN_SHARD_SPLIT_SOLO ) )
                    {
                        /* Nothing to do. */
                        /* Clone, Solo table은 NodeID만 전달받는다. */
                    }
                    else
                    {
                        ACE_ASSERT(0);
                    }
                }
                else
                {
                    /* Nothing to do. */
                }
                CMI_SKIP_READ_BLOCK(aProtocolContext, 4);       // mShardNodeID
            }
            break;
        default:
            ACE_ASSERT(0);
    }

    return ACI_SUCCESS;
}

void ulsdSetCoordQuery( ulnStmt  *aStmt )
{
    /* BUG-46872 */
    if ( aStmt != NULL )
    {
        aStmt->mShardStmtCxt.mShardCoordQuery = ACP_TRUE;

        ulsdSetStmtShardModule(aStmt, &gShardModuleCOORD);
    }
}

SQLRETURN ulsdAnalyze(ulnFnContext *aFnContext,
                      ulnStmt      *aStmt,
                      acp_char_t   *aStatementText,
                      acp_sint32_t  aTextLength)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    acp_bool_t    sNeedFinPtContext = ACP_FALSE;
    acp_sint32_t  sLength = aTextLength;

    ACI_TEST(ulnEnter(aFnContext, NULL) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /* 넘겨진 객체의 validity 체크를 포함한 ODBC 3.0 에서 정의하는 각종 Error 체크 */
    ACI_TEST(ulnPrepCheckArgs(aFnContext, aStatementText, aTextLength) != ACI_SUCCESS);

    ACI_TEST( ulnInitializeProtocolContext(aFnContext,
                                           &(aStmt->mParentDbc->mPtContext),
                                           &(aStmt->mParentDbc->mSession))
              != ACI_SUCCESS );

    sNeedFinPtContext = ACP_TRUE;

    if (sLength == SQL_NTS)
    {
        sLength = acpCStrLen(aStatementText, ACP_SINT32_MAX);
    }
    else
    {
        /* Nothing to do */
    }

    ACI_TEST(ulsdAnalyzeRequest(aFnContext,
                                &(aStmt->mParentDbc->mPtContext),
                                aStatementText,
                                sLength)
             != ACI_SUCCESS);

    ACI_TEST(ulnFlushAndReadProtocol(aFnContext,
                                     &(aStmt->mParentDbc->mPtContext),
                                     aStmt->mParentDbc->mConnTimeoutValue)
             != ACI_SUCCESS);

    sNeedFinPtContext = ACP_FALSE;

    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(aFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s|\n [%s]", "ulsdAnalyze", aStatementText);

    return ULN_FNCONTEXT_GET_RC(aFnContext);

    ACI_EXCEPTION_END;

    if (sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        (void)ulnFinalizeProtocolContext( aFnContext, &(aStmt->mParentDbc->mPtContext) );
    }
    else
    {
        /* Nothing to do */
    }

    if (sNeedExit == ACP_TRUE)
    {
        (void)ulnExit( aFnContext );
    }
    else
    {
        /* Nothing to do */
    }

    ULN_TRACE_LOG(aFnContext, ULN_TRACELOG_LOW, NULL, 0,
                  "%-18s| fail\n [%s]", "ulsdAnalyze", aStatementText);

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

SQLRETURN ulsdPrepare(ulnFnContext *aFnContext,
                      ulnStmt      *aStmt,
                      acp_char_t   *aStatementText,
                      acp_sint32_t  aTextLength,
                      acp_char_t   *aAnalyzeText)
{
    SQLRETURN sRet = SQL_ERROR;
    

    /* BUG-46100 Session SMN Update */
    aStmt->mShardStmtCxt.mShardMetaNumber = 0;

    if ( aStatementText != aStmt->mShardStmtCxt.mOrgPrepareTextBuf )
    {
        ACI_TEST( ulsdNodeStmtEnsureAllocOrgPrepareTextBuf( aFnContext,
                                                            aStmt,
                                                            aStatementText,
                                                            aTextLength )
                  != ACP_RC_SUCCESS );

        /* BUG-46100 Session SMN Update */
        acpMemCpy( aStmt->mShardStmtCxt.mOrgPrepareTextBuf,
                   aStatementText,
                   aStmt->mShardStmtCxt.mOrgPrepareTextBufLen );
    }

    sRet = ulsdModulePrepare(aFnContext,
                             aStmt,
                             aStatementText,
                             aTextLength,
                             aAnalyzeText);

    /* BUG-46100 Session SMN Update */
    if ( SQL_SUCCEEDED( sRet ) )
    {
        aStmt->mShardStmtCxt.mShardMetaNumber = 
            ACP_MAX( aStmt->mParentDbc->mShardDbcCxt.mSentShardMetaNumber,
                     aStmt->mParentDbc->mShardDbcCxt.mSentRebuildShardMetaNumber );
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdAnalyzePreapre( ulnFnContext *aFnContext,
                              ulnStmt      *aStmt,
                              acp_char_t   *aStatementText,
                              acp_sint32_t  aTextLength,
                              acp_char_t   *aAnalyzeText )
{
    ulnDbc      *sDbc  = NULL;
    SQLRETURN    sRet  = SQL_ERROR;

    sDbc = aStmt->mParentDbc;

    /* Prepare 수행하기 전에
     * Server 의 User session SMN 과 Dbc 의 SMN 이 동일한지 확인한다. 
     * Server 의 SMN 이 변경되었다면 ( ulsdCallbackShardRebuildNoti )
     * Target SMN 으로 확인할 수 있다.
     */
    sRet = ulsdCheckDbcSMN( aFnContext, sDbc );
    ACI_TEST( !SQL_SUCCEEDED( sRet ) );

    /* shard module 설정 */
    if ( sDbc->mAttrGlobalTransactionLevel == ULN_GLOBAL_TX_ONE_NODE )
    {
        sRet = ulsdAnalyze(aFnContext,
                           aStmt,
                           aStatementText,
                           aTextLength);
        ACI_TEST(!SQL_SUCCEEDED(sRet));

        /* TASK-7219 Non-shard DML */
        ACI_TEST_RAISE( aStmt->mShardStmtCxt.mShardCoordQuery == ACP_TRUE,
                        LABEL_SHARD_META_CANNOT_TOUCH );
    }
    else
    {
        sRet = ulsdAnalyze(aFnContext,
                           aStmt,
                           aStatementText,
                           aTextLength);
        if (!SQL_SUCCEEDED(sRet))
        {
            ACI_TEST( ulnDbcIsConnected( sDbc ) == ACP_FALSE );
            ACI_TEST( aFnContext->mIsFailoverSuccess == ACP_TRUE );

            ulsdSetCoordQuery(aStmt);

            ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* analyze 도중 세션의 SMN 이 갱신될 수 있다. */
    sRet = ulsdCheckDbcSMN( aFnContext, sDbc );
    ACI_TEST( !SQL_SUCCEEDED( sRet ) );

    sRet = ulsdPrepare(aFnContext,
                       aStmt,
                       aStatementText,
                       aTextLength,
                       aAnalyzeText);

    return sRet;

    ACI_EXCEPTION(LABEL_SHARD_META_CANNOT_TOUCH)
    {
        ulnError(aFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "SQLPrepare",
                 "Server-side query can not be performed in single-node transaction");
        sRet  = SQL_ERROR;
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdAnalyzePrepareAndRetry( ulnStmt      *aStmt,
                                      acp_char_t   *aStatementText,
                                      acp_sint32_t  aTextLength,
                                      acp_char_t   *aAnalyzeText )
{
    ulnFnContext sFnContext;
    SQLRETURN    sPrepareRet = SQL_ERROR;
    acp_sint32_t sRetryMax   = 0;
    acp_sint32_t sLoopMax    = ULSD_SHARD_RETRY_LOOP_MAX; /* For prohibit infinity loop */
    ulsdStmtShardRetryType   sRetryType;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PREPARE, aStmt, ULN_OBJ_TYPE_STMT);

    /* BUG-47553 */
    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    sRetryMax = ulnDbcGetShardStatementRetry( aStmt->mParentDbc );

    sPrepareRet = ulsdAnalyzePreapre( &sFnContext,
                                      aStmt,
                                      aStatementText,
                                      aTextLength,
                                      aAnalyzeText );

    /* BUGBUG retrun result not matched with FNCONTEXT result */
    ULN_FNCONTEXT_SET_RC( &sFnContext, sPrepareRet );

    while ( SQL_SUCCEEDED( sPrepareRet ) == 0 )
    {
        ACI_TEST( (sLoopMax--) <= 0 );

        ACI_TEST( ulsdProcessShardRetryError( &sFnContext,
                                              aStmt,
                                              &sRetryType,
                                              &sRetryMax )
                  != ACI_SUCCESS );

        sPrepareRet = ulsdAnalyzePreapre( &sFnContext,
                                          aStmt,
                                          aStatementText,
                                          aTextLength,
                                          aAnalyzeText );

        /* BUGBUG retrun result not matched with FNCONTEXT result */
        ULN_FNCONTEXT_SET_RC( &sFnContext, sPrepareRet );
    }

    /* PROJ-2745
     * SMN 변경이 현재 Statement 수행에 영향을 미치지 않으면
     * Rebuild retry error 가 발생하지 않고
     * 대신 SMN 을 올리라는 Rebuild Noti 로 수신되어
     * ulnDbcGetTargetShardMetaNumber() 값이 상승한다.
     */
    if ( ulsdCheckRebuildNoti( aStmt->mParentDbc ) == ACP_TRUE )
    {
        ulsdUpdateShardSession_Silent( aStmt->mParentDbc,
                                       &sFnContext );
    }

    return ULN_FNCONTEXT_GET_RC( &sFnContext );

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdPrepareNodes(ulnFnContext *aFnContext,
                           ulnStmt      *aStmt,
                           acp_char_t   *aStatementText,
                           acp_sint32_t  aTextLength,
                           acp_char_t   *aAnalyzeText)
{
    SQLRETURN          sNodeResult    = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult   = SQL_ERROR;
    acp_bool_t         sSuccess       = ACP_TRUE;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt;
    acp_bool_t         sNodeDbcFlags[ULSD_SD_NODE_MAX_COUNT] = { 0, }; /* ACP_FALSE */
    acp_uint16_t       sNodeDbcIndex;
    ulsdFuncCallback  *sCallback = NULL;
    acp_uint16_t       i;

    ACP_UNUSED(aAnalyzeText);

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    /* mShardRangeInfo의 모든 노드에 prepare를 수행한다. */
    for (i = 0; i < aStmt->mShardStmtCxt.mShardRangeInfoCnt; i++)
    {
        sErrorResult = ulsdConvertNodeIdToNodeDbcIndex(
                            aStmt,
                            aStmt->mShardStmtCxt.mShardRangeInfo[i].mShardNodeID,
                            &sNodeDbcIndex,
                            ULN_FID_PREPARE );
        ACI_TEST( sErrorResult != SQL_SUCCESS );

        /* 기록 */
        sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
    }

    /* default node 기록 */
    if ( aStmt->mShardStmtCxt.mShardDefaultNodeID != ACP_UINT32_MAX )
    {
        sErrorResult = ulsdConvertNodeIdToNodeDbcIndex(
                            aStmt,
                            aStmt->mShardStmtCxt.mShardDefaultNodeID,
                            &sNodeDbcIndex,
                            ULN_FID_PREPARE );
        ACI_TEST( sErrorResult != SQL_SUCCESS );

        /* 기록 */
        sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    for (i = 0; i < sShard->mNodeCount; i++)
    {
        if (sNodeDbcFlags[i] == ACP_TRUE)
        {
            sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[i];

            sErrorResult = ulsdCheckDbcSMN( aFnContext, sNodeStmt->mParentDbc );
            ACI_TEST( sErrorResult != SQL_SUCCESS );

            /* TASK-7219 Non-shard DML
             * Analysis result로 부터 meta statement에 세팅된 partial coord type을 node statement 에 전달한다.
             * Node statement prepare 시에 prepareProtocol을 통해 prepare mode 로 data node server에 전달된다.
             */
            ulsdStmtSetPartialExecType( sNodeStmt, ulsdStmtGetPartialExecType(aStmt) );

            sErrorResult = ulsdPrepareAddCallback( i,
                                                   sNodeStmt,
                                                   aStatementText,
                                                   aTextLength,
                                                   &sCallback );
            ACI_TEST( sErrorResult != SQL_SUCCESS );
        }
        else
        {
            /* Do Nothing */
        }
    }

    /* node prepare 병렬수행 */
    ulsdDoCallback( sCallback );

    for (i = 0; i < sShard->mNodeCount; i++)
    {
        if (sNodeDbcFlags[i] == ACP_TRUE)
        {
            sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[i];

            sNodeResult = ulsdGetResultCallback( i, sCallback, (acp_uint8_t)0 );

            if ( sNodeResult == SQL_SUCCESS )
            {
                sNodeStmt->mShardStmtCxt.mShardMetaNumber = 
                    ACP_MAX( sNodeStmt->mParentDbc->mShardDbcCxt.mSentShardMetaNumber,
                             sNodeStmt->mParentDbc->mShardDbcCxt.mSentRebuildShardMetaNumber );
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                sNodeStmt->mShardStmtCxt.mShardMetaNumber = 
                    ACP_MAX( sNodeStmt->mParentDbc->mShardDbcCxt.mSentShardMetaNumber,
                             sNodeStmt->mParentDbc->mShardDbcCxt.mSentRebuildShardMetaNumber );

                sSuccessResult = sNodeResult;
            }
            else
            {
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_STMT,
                                           (ulnObject *)sNodeStmt,
                                           sShard->mNodeInfo[i],
                                           (acp_char_t *)"Prepare" );

                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }

            SHARD_LOG("(Prepare) NodeId=%d, Server=%s:%d, Stmt=%s\n",
                      sShard->mNodeInfo[i]->mNodeId,
                      sShard->mNodeInfo[i]->mServerIP,
                      sShard->mNodeInfo[i]->mPortNo,
                      aStatementText);
        }
        else
        {
            /* Do Nothing */
        }
    }

    ACI_TEST( sSuccess == ACP_FALSE );

    ulsdRemoveCallback( sCallback );

    return sSuccessResult;

    ACI_EXCEPTION_END;

    ulsdRemoveCallback( sCallback );

    return sErrorResult;
}

