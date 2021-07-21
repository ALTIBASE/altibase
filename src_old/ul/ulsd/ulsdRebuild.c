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

#include <ulsdRebuild.h>
#include <smErrorCodeClient.h>
#include <sdErrorCodeClient.h>


static SQLRETURN ulsdPropagateRebuildShardMetaNumber( ulnDbc       * aMetaDbc,
                                                      ulnFnContext * aFnContext )
{
    ulsdDbc      * sShard           = aMetaDbc->mShardDbcCxt.mShardDbc;
    ulnDbc       * sNodeDbc         = NULL;
    acp_uint64_t   sNewSMN          = 0;
    acp_uint64_t   sSentSMN         = 0;
    acp_uint16_t   i                = 0;

    SQLRETURN      sNodeResult      = SQL_ERROR;
    SQLRETURN      sSuccessResult   = SQL_SUCCESS;
    SQLRETURN      sErrorResult     = SQL_ERROR;
    acp_bool_t     sSuccess         = ACP_TRUE;

    sNewSMN = ulnDbcGetTargetShardMetaNumber( aMetaDbc );

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        sNodeDbc = sShard->mNodeInfo[i]->mNodeDbc;
        sSentSMN = ACP_MAX( ulnDbcGetSentShardMetaNumber( sNodeDbc ),
                            ulnDbcGetSentRebuildShardMetaNumber( sNodeDbc ) );
        if ( ( sShard->mNodeInfo[i]->mSMN == sNewSMN ) &&
             ( sNewSMN != sSentSMN ) )
        {
            ACE_DASSERT( sNewSMN > sSentSMN );

            ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

            sNodeResult = ulsdSetConnectAttrNode( sShard->mNodeInfo[i]->mNodeDbc,
                                                  ULN_PROPERTY_REBUILD_SHARD_META_NUMBER,
                                                  (void *)sNewSMN );
            if ( sNodeResult == SQL_SUCCESS )
            {
                ulnDbcSetSentRebuildShardMetaNumber( sShard->mNodeInfo[i]->mNodeDbc,
                                                     sNewSMN );
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                ulnDbcSetSentRebuildShardMetaNumber( sShard->mNodeInfo[i]->mNodeDbc,
                                                     sNewSMN );

                /* info의 전달 */
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_DBC,
                                           (ulnObject *)sShard->mNodeInfo[i]->mNodeDbc,
                                           sShard->mNodeInfo[i],
                                           (acp_char_t *)"SetConnectAtt(REBUILD_SHARD_META_NUMBER)" );

                sSuccessResult = sNodeResult;
            }
            else
            {
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_DBC,
                                           (ulnObject *)sShard->mNodeInfo[i]->mNodeDbc,
                                           sShard->mNodeInfo[i],
                                           (acp_char_t *)"SetConnectAtt(REBUILD_SHARD_META_NUMBER)");

                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }
        }
        else
        {
            /* sNewSMN == sSentSMN
             * Nothing to do */
        }
    }

    sSentSMN = ACP_MAX( ulnDbcGetSentShardMetaNumber( aMetaDbc ),
                        ulnDbcGetSentRebuildShardMetaNumber( aMetaDbc ) );

    if ( sNewSMN != sSentSMN )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

        sNodeResult = ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_REBUILD_SHARD_META_NUMBER,
                                          (void *)sNewSMN );

        if ( sNodeResult == SQL_SUCCESS )
        {
            /* Nothing to do */
            ulnDbcSetSentRebuildShardMetaNumber( aMetaDbc,
                                                 sNewSMN );
        }
        else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
        {
            ulnDbcSetSentRebuildShardMetaNumber( aMetaDbc,
                                                 sNewSMN );

            sSuccessResult = sNodeResult;
        }
        else
        {
            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }
    }

    ACI_TEST_RAISE( sSuccess == ACP_FALSE,
                    ERR_SEND_SHARD_META_NUMBER_FAIL );

    ULN_FNCONTEXT_SET_RC( aFnContext, sSuccessResult );

    return sSuccessResult;

    ACI_EXCEPTION( ERR_SEND_SHARD_META_NUMBER_FAIL )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, sErrorResult );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

static SQLRETURN ulsdPropagateShardMetaNumber( ulnDbc       * aMetaDbc,
                                               ulnFnContext * aFnContext )
{
    ulsdDbc      * sShard           = aMetaDbc->mShardDbcCxt.mShardDbc;
    ulnDbc       * sNodeDbc         = NULL;
    acp_uint64_t   sNewSMN          = 0;
    acp_uint64_t   sSentSMN         = 0;
    acp_uint16_t   i                = 0;

    SQLRETURN      sNodeResult      = SQL_ERROR;
    SQLRETURN      sSuccessResult   = SQL_SUCCESS;
    SQLRETURN      sErrorResult     = SQL_ERROR;
    acp_bool_t     sSuccess         = ACP_TRUE;

    sNewSMN = ulnDbcGetTargetShardMetaNumber( aMetaDbc );

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        sNodeDbc = sShard->mNodeInfo[i]->mNodeDbc;
        sSentSMN = ulnDbcGetSentShardMetaNumber( sNodeDbc );

        if ( ( sShard->mNodeInfo[i]->mSMN == sNewSMN ) &&
             ( sNewSMN != sSentSMN ) )
        {
            ACE_DASSERT( sNewSMN > sSentSMN );

            ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

            sNodeResult = ulsdSetConnectAttrNode( sShard->mNodeInfo[i]->mNodeDbc,
                                                  ULN_PROPERTY_SHARD_META_NUMBER,
                                                  (void *)sNewSMN );
            if ( sNodeResult == SQL_SUCCESS )
            {
                ulnDbcSetSentShardMetaNumber( sShard->mNodeInfo[i]->mNodeDbc,
                                              sNewSMN );
                ulnDbcSetShardMetaNumber( sShard->mNodeInfo[i]->mNodeDbc,
                                          sNewSMN );
            }
            else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
            {
                ulnDbcSetSentShardMetaNumber( sShard->mNodeInfo[i]->mNodeDbc,
                                              sNewSMN );
                ulnDbcSetShardMetaNumber( sShard->mNodeInfo[i]->mNodeDbc,
                                          sNewSMN );

                /* info의 전달 */
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_DBC,
                                           (ulnObject *)sShard->mNodeInfo[i]->mNodeDbc,
                                           sShard->mNodeInfo[i],
                                           (acp_char_t *)"SetConnectAtt(SHARD_META_NUMBER)" );

                sSuccessResult = sNodeResult;
            }
            else
            {
                ulsdNativeErrorToUlnError( aFnContext,
                                           SQL_HANDLE_DBC,
                                           (ulnObject *)sShard->mNodeInfo[i]->mNodeDbc,
                                           sShard->mNodeInfo[i],
                                           (acp_char_t *)"SetConnectAtt(SHARD_META_NUMBER)");

                sErrorResult = sNodeResult;
                sSuccess = ACP_FALSE;
            }
        }
        else
        {
            /* sNewSMN == sSentSMN
             * Nothing to do */
        }
    }

    sSentSMN = ulnDbcGetSentShardMetaNumber( aMetaDbc );

    if ( sNewSMN != sSentSMN )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

        sNodeResult = ulnSendConnectAttr( aFnContext,
                                          ULN_PROPERTY_SHARD_META_NUMBER,
                                          (void *)sNewSMN );

        if ( sNodeResult == SQL_SUCCESS )
        {
            ulnDbcSetSentShardMetaNumber( aMetaDbc,
                                          sNewSMN );
            ulnDbcSetShardMetaNumber( aMetaDbc,
                                      sNewSMN );
        }
        else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
        {
            ulnDbcSetSentShardMetaNumber( aMetaDbc,
                                          sNewSMN );
            ulnDbcSetShardMetaNumber( aMetaDbc,
                                      sNewSMN );

            sSuccessResult = sNodeResult;
        }
        else
        {
            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }
    }

    ACI_TEST_RAISE( sSuccess == ACP_FALSE,
                    ERR_SEND_SHARD_META_NUMBER_FAIL );

    ULN_FNCONTEXT_SET_RC( aFnContext, sSuccessResult );

    return sSuccessResult;

    ACI_EXCEPTION( ERR_SEND_SHARD_META_NUMBER_FAIL )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, sErrorResult );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

static SQLRETURN ulsdUpdateShardSession( ulnDbc       * aMetaDbc,
                                         ulnFnContext * aFnContext,
                                         acp_bool_t     aSendRebuildSMN )
{
    ACI_TEST( ulsdUpdateShardMetaNumber( aMetaDbc,
                                         aFnContext )
              != SQL_SUCCESS );

    if ( aMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
    {
        if ( aSendRebuildSMN == ACP_TRUE )
        {
            ACI_TEST( ulsdPropagateRebuildShardMetaNumber( aMetaDbc,
                                                           aFnContext )
                      != SQL_SUCCESS );
        }
        else
        {
            ACI_TEST( ulsdPropagateShardMetaNumber( aMetaDbc,
                                                    aFnContext )
                      != SQL_SUCCESS );
        }
    }
    else
    {
        ACI_TEST( ulsdPropagateShardMetaNumber( aMetaDbc,
                                                aFnContext )
                  != SQL_SUCCESS );

        ulsdCleanupShardSession( aMetaDbc,
                                 aFnContext );
    }

    return ULN_FNCONTEXT_GET_RC( aFnContext );

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

void ulsdUpdateShardSession_Silent( ulnDbc       * aMetaDbc,
                                    ulnFnContext * aFnContext )
{
    SQLRETURN sOldResult;

    sOldResult = ULN_FNCONTEXT_GET_RC( aFnContext );
    ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

    if ( ulsdUpdateShardSession( aMetaDbc,
                                 aFnContext,
                                 ACP_TRUE )
         != SQL_SUCCESS )
    {
        /* fail case. reset original result value */
    }

    ULN_FNCONTEXT_SET_RC( aFnContext, sOldResult );
}

void ulsdCleanupShardSession( ulnDbc       * aMetaDbc,
                              ulnFnContext * aFnContext )
{
    SQLRETURN    sOldResult;
    acp_uint64_t sTargetSMN      = 0UL;
    acp_uint64_t sSentSMN        = 0UL;
    acp_uint64_t sSentRebuildSMN = 0UL;

    if ( ulsdHasNoData( aMetaDbc ) == ACP_TRUE )
    {
        sOldResult = ULN_FNCONTEXT_GET_RC( aFnContext );
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

        sTargetSMN      = ulnDbcGetTargetShardMetaNumber( aMetaDbc );
        sSentSMN        = ulnDbcGetSentShardMetaNumber( aMetaDbc );
        sSentRebuildSMN = ulnDbcGetSentRebuildShardMetaNumber( aMetaDbc );

        if ( ( sTargetSMN > sSentSMN ) && ( sTargetSMN > sSentRebuildSMN ) )
        {
            if ( aMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
            {
                if ( ulsdUpdateShardSession( aMetaDbc,
                                             aFnContext,
                                             ACP_FALSE )
                     == SQL_SUCCESS )
                {
                    /* Success. */
                    sTargetSMN      = ulnDbcGetTargetShardMetaNumber( aMetaDbc );
                    sSentSMN        = ulnDbcGetSentShardMetaNumber( aMetaDbc );
                    sSentRebuildSMN = ulnDbcGetSentRebuildShardMetaNumber( aMetaDbc );
                }
                else
                {
                    ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( ( sSentSMN != sSentRebuildSMN ) && 
             ( ( sTargetSMN == sSentSMN ) || ( sTargetSMN == sSentRebuildSMN ) ) )
        {
            if ( ulsdPropagateShardMetaNumber( aMetaDbc,
                                               aFnContext )
                 == SQL_SUCCESS )
            {
                ulsdApplyNodeInfo_RemoveOldSMN( aMetaDbc );

                if ( SQL_SUCCEEDED( sOldResult ) == 0 )
                {
                    ulnError( aFnContext, ulERR_ABORT_SHARD_META_CHANGED );
                }
            }
            else
            {
                ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );
            }
        }

        ULN_FNCONTEXT_SET_RC( aFnContext, sOldResult );
    }
}

static void ulsdCheckErrorIsShardRetry( ulnStmt                * aStmt,
                                        ulsdStmtShardRetryType * aRetryType )
{
    ulnDiagHeader  *sDiagHeader;
    acp_sint32_t    sDiagCount      = 0;
    acp_sint16_t    sRecNum         = 0;
    acp_sint32_t    sNativeError    = 0;

    acp_sint32_t    sErrCnt_RebuilRetry = 0; 
    acp_sint32_t    sErrCnt_StmtViewOld = 0;
    acp_sint32_t    sErrCnt_SMNProp     = 0; 
    acp_sint32_t    sErrCnt_Others      = 0; 

    /* Autocommit on + global transaction 이면서
     * 여러 참여 노드로 실행되는 구문은
     * Server-side 로 수행되게 되어있다. (BUG-45640 ulsdCallbackAnalyzeResult)
     * 따라서 Shard statement retry 기능은 autocommit on 에서도 동작 가능하다. */

    *aRetryType = ULSD_STMT_SHARD_RETRY_NONE;

    sDiagHeader = ulnGetDiagHeaderFromObject( &aStmt->mObj );
    ulnDiagGetNumber( sDiagHeader, (acp_sint32_t *)&sDiagCount );

    ACI_TEST_CONT( sDiagCount == 0,
                   EndOfFunction );
    
    sRecNum = 0;

    while ( ulnGetDiagRec( SQL_HANDLE_STMT,
                           &aStmt->mObj,
                           ++sRecNum,
                           NULL,
                           &sNativeError,
                           NULL,
                           0,
                           NULL,
                           ACP_FALSE )
            != SQL_NO_DATA )
    {
        if ( sNativeError == ACI_E_ERROR_CODE( smERR_ABORT_StatementTooOld ) )
        {
            ++sErrCnt_StmtViewOld;
        }
        else if ( sNativeError == ACI_E_ERROR_CODE( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) )
        {
            ++sErrCnt_RebuilRetry;
        }
        else if ( sNativeError == ACI_E_ERROR_CODE( ulERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER ) )
        {
            ++sErrCnt_SMNProp;
        }
        else if ( sNativeError == ACI_E_ERROR_CODE( sdERR_ABORT_SHARD_MULTIPLE_ERRORS ) )
        {
            /* Nothing to do */
        }
        else
        {
            /* PROJ-2745
             * 위 에러 이외의 에러가 함께 발생하면 Rebuild Retry 하지 않는다.
             * */
            ++sErrCnt_Others;
        }
    }

    if ( sErrCnt_SMNProp != 0 )
    {
        /* SMN Propagation event 를 우선한다.
         */
        *aRetryType = ULSD_STMT_SHARD_SMN_PROPAGATION;
    }
    else if ( sErrCnt_Others == 0 )
    {
        if ( sErrCnt_RebuilRetry != 0 )
        {
            /* Rebuild event 를 우선한다.
             * Statement Too Old 에러가 발생했을때 Rebuild Retry 를 수행해도 된다.
             * */
            *aRetryType = ULSD_STMT_SHARD_REBUILD_RETRY;
        }
        else
        {
            /* Statement Too Old 에러 인 경우 */
            *aRetryType = ULSD_STMT_SHARD_VIEW_OLD_RETRY;
        }
    }

    ACI_EXCEPTION_CONT( EndOfFunction );
}

SQLRETURN ulsdProcessShardRetryError( ulnFnContext           * aFnContext,
                                      ulnStmt                * aStmt,
                                      ulsdStmtShardRetryType * aRetryType,
                                      acp_sint32_t           * aRetryMax )
{
    ulnDbc                 * sMetaDbc = NULL;
    acp_uint64_t             sTargetSMN      = 0UL;
    acp_uint64_t             sSentSMN        = 0UL;
    acp_uint64_t             sSentRebuildSMN = 0UL;

    sMetaDbc = aStmt->mParentDbc;
    *aRetryType = ULSD_STMT_SHARD_RETRY_NONE;

    ACE_DASSERT( sMetaDbc->mShardDbcCxt.mParentDbc == NULL );

    sTargetSMN      = ulnDbcGetTargetShardMetaNumber( sMetaDbc );
    sSentSMN        = ulnDbcGetSentShardMetaNumber( sMetaDbc );
    sSentRebuildSMN = ulnDbcGetSentRebuildShardMetaNumber( sMetaDbc );

    ulsdCheckErrorIsShardRetry( aStmt, aRetryType );

    switch ( *aRetryType )
    {
        case ULSD_STMT_SHARD_REBUILD_RETRY :
            ACI_TEST( aStmt->mShardModule == &gShardModuleCOORD );

            if ( ( sMetaDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_ON ) &&
                 ( aStmt->mShardModule == &gShardModuleMETA ) )
            {
                ACI_TEST( aStmt->mShardStmtCxt.mNodeDbcIndexCount > 1 );
            }

            ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

            ACI_TEST_RAISE( ulnClearDiagnosticInfoFromObject( aFnContext->mHandle.mObj ) != ACI_SUCCESS,
                            LABEL_MEM_MAN_ERR );

            ACI_TEST( ulsdNodeCloseCursor( aStmt )
                      != SQL_SUCCESS );

            ACI_TEST( ulsdUpdateShardSession( sMetaDbc,
                                              aFnContext,
                                              ACP_TRUE )
                      != SQL_SUCCESS );
            break;

        case ULSD_STMT_SHARD_SMN_PROPAGATION :
            if ( ( sTargetSMN == sSentSMN ) || ( sTargetSMN == sSentRebuildSMN ) )
            {
                /* User Connection DBC SMN 전파는 완료된 상태.
                 * Library Connection DBC SMN 전파 확인을 위해 ulsdPropagateRebuildShardMetaNumber 수행
                 * Statement SMN 이 다른 경우를 위해 SQLPrepare 를 재수행하면 된다.
                 */
                ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

                ACI_TEST_RAISE( ulnClearDiagnosticInfoFromObject( aFnContext->mHandle.mObj ) != ACI_SUCCESS,
                                LABEL_MEM_MAN_ERR );

                ACI_TEST( ulsdPropagateRebuildShardMetaNumber( sMetaDbc,
                                                               aFnContext )
                          != SQL_SUCCESS );
            }
            else
            {
                ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

                ACI_TEST_RAISE( ulnClearDiagnosticInfoFromObject( aFnContext->mHandle.mObj ) != ACI_SUCCESS,
                                LABEL_MEM_MAN_ERR );

                ACI_TEST( ulsdNodeCloseCursor( aStmt )
                          != SQL_SUCCESS );

                ACI_TEST( ulsdUpdateShardSession( sMetaDbc,
                                                  aFnContext,
                                                  ACP_TRUE )
                          != SQL_SUCCESS );
            }
            break;

        case ULSD_STMT_SHARD_VIEW_OLD_RETRY :
            ACI_TEST( aStmt->mShardModule == &gShardModuleCOORD );

            ACI_TEST( *aRetryMax == 0 );

            ULN_FNCONTEXT_SET_RC( aFnContext, SQL_SUCCESS );

            ACI_TEST_RAISE( ulnClearDiagnosticInfoFromObject( aFnContext->mHandle.mObj ) != ACI_SUCCESS,
                            LABEL_MEM_MAN_ERR );

            --(*aRetryMax);

            ACI_TEST( ulsdNodeCloseCursor( aStmt ) 
                      != SQL_SUCCESS );
            break;

        default:
            /* 재시도 필요없다.
             * 현재 에러가 발생한 그대로 에러 리턴을 하면 된다.
             */
            ACI_TEST( 1 );
            break;
    }

    return ULN_FNCONTEXT_GET_RC( aFnContext );

    ACI_EXCEPTION( LABEL_MEM_MAN_ERR )
    {
        ulnError( aFnContext, ulERR_FATAL_MEMORY_MANAGEMENT_ERROR, "ulsdProcessShardRetryError" );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

acp_bool_t ulsdCheckRebuildNoti( ulnDbc * aMetaDbc )
{
    acp_uint64_t sTargetSMN      = ulnDbcGetTargetShardMetaNumber( aMetaDbc );
    acp_uint64_t sSentSMN        = ulnDbcGetSentShardMetaNumber( aMetaDbc );
    acp_uint64_t sSentRebuildSMN = ulnDbcGetSentRebuildShardMetaNumber( aMetaDbc );
    acp_bool_t   sRet            = ACP_FALSE;

    if ( ( sTargetSMN != sSentSMN ) && ( sTargetSMN != sSentRebuildSMN ) )
    {
        sRet = ACP_TRUE;
    }

    return sRet;
}

SQLRETURN ulsdCheckDbcSMN( ulnFnContext *aFnContext, ulnDbc * aDbc )
{
    ulnDbc       * sMetaDbc        = NULL;
    acp_uint64_t   sTargetSMN      = 0UL;
    acp_uint64_t   sSentSMN        = aDbc->mShardDbcCxt.mSentShardMetaNumber;
    acp_uint64_t   sSentRebuildSMN = aDbc->mShardDbcCxt.mSentRebuildShardMetaNumber;

    if ( aDbc->mShardDbcCxt.mParentDbc != NULL )
    {
        sMetaDbc = aDbc->mShardDbcCxt.mParentDbc;
    }
    else
    {
        sMetaDbc = aDbc;
    }

    sTargetSMN = ulnDbcGetTargetShardMetaNumber( sMetaDbc );

    ACI_TEST_RAISE( ( ( sTargetSMN != sSentSMN ) && ( sTargetSMN != sSentRebuildSMN ) ),
                    ERR_MISMATCH_SMN );

    return ULN_FNCONTEXT_GET_RC( aFnContext );

    ACI_EXCEPTION( ERR_MISMATCH_SMN )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

SQLRETURN ulsdCheckStmtSMN( ulnFnContext *aFnContext, ulnStmt * aStmt )
{
    ulnDbc       * sDbc            = NULL;
    ulnDbc       * sMetaDbc        = NULL;
    acp_uint64_t   sTargetSMN      = 0UL;
    acp_uint64_t   sStmtSMN        = aStmt->mShardStmtCxt.mShardMetaNumber;

    sDbc = aStmt->mParentDbc;

    if ( sDbc->mShardDbcCxt.mParentDbc != NULL )
    {
        sMetaDbc = sDbc->mShardDbcCxt.mParentDbc;
    }
    else
    {
        sMetaDbc = sDbc;
    }

    sTargetSMN = ulnDbcGetTargetShardMetaNumber( sMetaDbc );

    ACI_TEST_RAISE( sTargetSMN != sStmtSMN,
                    ERR_MISMATCH_SMN );

    return ULN_FNCONTEXT_GET_RC( aFnContext );

    ACI_EXCEPTION( ERR_MISMATCH_SMN )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_FAILED_TO_PROPAGATE_SHARD_META_NUMBER );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( aFnContext );
}

/* PROJ-2745
 * ulsdApplyNodeInfo_OnlyAdd 함수는 Rebuild 이후 추가되는 노드의 정보를 추가한다.
 * ulsdApplyNodeInfo_RemoveOldSMN 함수는 Rebuild 이후 삭제되는 노드의 정보를 삭제한다.
 * 위 두개의 함수로 Transaction 내에서 신규 노드 정보를 추가하고
 * transaction 이 끝나면 삭제 노드 정보를 삭제한다.
 * 따라서 두 함수는 서로 쌍으로 작용해야 한다.
 */
ACI_RC ulsdApplyNodeInfo_OnlyAdd( ulnFnContext  * aFnContext,
                                  ulsdNodeInfo ** aNewNodeInfoArray,
                                  acp_uint16_t    aNewNodeInfoCount,
                                  acp_uint64_t    aShardMetaNumber,
                                  acp_uint8_t     aIsTestEnable )
{
    ulnDbc        * sMetaDbc    = NULL;
    ulsdDbc       * sShard      = NULL;

    acp_uint16_t    i           = 0;
    acp_uint16_t    j           = 0;

    sMetaDbc = ulnFnContextGetDbc( aFnContext );
    ACI_TEST_RAISE( sMetaDbc == NULL, INVALID_HANDLE_EXCEPTION );

    sShard = sMetaDbc->mShardDbcCxt.mShardDbc;

    if ( ulnDbcGetShardMetaNumber( sMetaDbc ) < aShardMetaNumber )
    {
        if ( aIsTestEnable == 0 )
        {
            sShard->mIsTestEnable = ACP_FALSE;
        }
        else
        {
            sShard->mIsTestEnable = ACP_TRUE;
        }

        if ( sShard->mNodeInfo != NULL )
        {
            /* Ex) Node ID 1, 3 제거 & Node ID 4, 5 추가
             *  기존 Node ID : ( 1, 2, 3 )
             *  신규 Node ID :    ( 2,    4, 5 )
             */

            /* 유지할 노드, 제거될 노드 확인 */
            for ( i = 0; i < sShard->mNodeCount; ++i )
            {
                for ( j = 0; j < aNewNodeInfoCount; ++j )
                {
                    if ( aNewNodeInfoArray[j] != NULL )
                    {
                        if ( sShard->mNodeInfo[i]->mNodeId == 
                             aNewNodeInfoArray[j]->mNodeId )
                        {
                            sShard->mNodeInfo[i]->mSMN
                                = aNewNodeInfoArray[j]->mSMN;

                            acpMemFree( (void *)aNewNodeInfoArray[j] );
                            aNewNodeInfoArray[j] = NULL;
                            break;
                        }
                    }
                }
            }

            /* 추가할 노드 확인 */
            for ( j = 0; j < aNewNodeInfoCount; ++j )
            {
                if ( aNewNodeInfoArray[j] != NULL )
                {
                    ACI_TEST( ulsdAddNode( aFnContext,
                                           sMetaDbc,
                                           aNewNodeInfoArray[j] )
                              != SQL_SUCCESS );
                    aNewNodeInfoArray[j] = NULL;
                }
            }
        }
        else
        {
            sShard->mNodeCount = aNewNodeInfoCount;
            sShard->mNodeInfo  = aNewNodeInfoArray;
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sShard->mNodeInfo != aNewNodeInfoArray )
    {
        for ( j = 0; j < aNewNodeInfoCount; ++j )
        {
            ACE_DASSERT( aNewNodeInfoArray[j] == NULL );

            if ( aNewNodeInfoArray[j] != NULL )
            {
                acpMemFree( (void *)aNewNodeInfoArray[j] );
            }
            else
            {
                /* Nothing to do */
            }
        }

        acpMemFree( (void *)aNewNodeInfoArray );
    }
    else
    {
        /* Nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( INVALID_HANDLE_EXCEPTION )
    {
        ULN_FNCONTEXT_SET_RC( aFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulsdApplyNodeInfo_RemoveOldSMN( ulnDbc  * aDbc )
{
    ulnDbc        * sMetaDbc        = aDbc;
    ulsdDbc       * sShard          = NULL;
    ulsdNodeInfo  * sRemoveNodeInfo = NULL;

    acp_uint64_t    sShardMetaNumber = 0;
    acp_uint16_t    i                = 0;

    sShardMetaNumber = ulnDbcGetTargetShardMetaNumber( sMetaDbc );

    sShard = sMetaDbc->mShardDbcCxt.mShardDbc;

    i = 0;

    while ( i < sShard->mNodeCount )
    {
        if ( sShard->mNodeInfo[i]->mSMN < sShardMetaNumber )
        {
            /* Node 제거 */
            sRemoveNodeInfo = sShard->mNodeInfo[i];
            ulsdRemoveNode( sMetaDbc,
                            sRemoveNodeInfo );
            acpMemFree( (void *)sRemoveNodeInfo );
        }
        else
        {
            ++i;
        }
    }
}

