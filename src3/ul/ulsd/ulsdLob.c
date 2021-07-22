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

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsdFailover.h>
#include <ulsdCommunication.h>
#include <ulsdnExecute.h>
#include <ulsdnLob.h>
#include <ulsdLob.h>

/*
 * PROJ-2739 Client-side Sharding LOB
 */

SQLRETURN ulsdGetLobLength( ulnStmt      *aStmt,
                            acp_uint64_t  aLocator,
                            acp_sint16_t  aLocatorType,
                            acp_uint32_t *aLengthPtr )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETLOBLENGTH, aStmt, SQL_HANDLE_STMT);

    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    sRet = ulsdModuleGetLobLength( &sFnContext,
                                   aStmt,
                                   aLocator,
                                   aLocatorType,
                                   aLengthPtr );

    return sRet;

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdGetLob( ulnStmt      *aStmt,
                      acp_sint16_t  aLocatorCType,
                      acp_uint64_t  aSrcLocator,
                      acp_uint32_t  aFromPosition,
                      acp_uint32_t  aForLength,
                      acp_sint16_t  aTargetCType,
                      void         *aBuffer,
                      acp_uint32_t  aBufferSize,
                      acp_uint32_t *aLengthWritten )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETLOB, aStmt, SQL_HANDLE_STMT);

    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    sRet = ulsdModuleGetLob( &sFnContext,
                             aStmt,
                             aLocatorCType,
                             aSrcLocator,
                             aFromPosition,
                             aForLength,
                             aTargetCType,
                             aBuffer,
                             aBufferSize,
                             aLengthWritten );

    return sRet;

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdPutLob( ulnStmt      *aStmt,
                      acp_sint16_t  aLocatorCType,
                      acp_uint64_t  aLocator,
                      acp_uint32_t  aFromPosition,
                      acp_uint32_t  aForLength,
                      acp_sint16_t  aSourceCType,
                      void         *aBuffer,
                      acp_uint32_t  aBufferSize )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PUTLOB, aStmt, SQL_HANDLE_STMT);

    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    sRet = ulsdModulePutLob( &sFnContext,
                             aStmt,
                             aLocatorCType,
                             aLocator,
                             aFromPosition,
                             aForLength,
                             aSourceCType,
                             aBuffer,
                             aBufferSize );

    return sRet;

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdFreeLob( ulnStmt      *aStmt,
                       acp_uint64_t  aLocator )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREELOB, aStmt, SQL_HANDLE_STMT);

    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    sRet = ulsdModuleFreeLob( &sFnContext,
                              aStmt,
                              aLocator );

    return sRet;

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdTrimLob( ulnStmt       *aStmt,
                       acp_sint16_t   aLocatorCType,
                       acp_uint64_t   aLocator,
                       acp_uint32_t   aStartOffset )
{
    SQLRETURN      sRet = SQL_ERROR;
    ulnFnContext   sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_TRIMLOB, aStmt, SQL_HANDLE_STMT);

    ACI_TEST( ulsdEnter( &sFnContext ) != ACI_SUCCESS );

    sRet = ulsdModuleTrimLob( &sFnContext,
                              aStmt,
                              aLocatorCType,
                              aLocator,
                              aStartOffset );

    return sRet;

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdGetLobLengthNodes( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt,
                                 acp_uint64_t   aLocator,
                                 acp_sint16_t   aLocatorType,
                                 acp_uint32_t * aLengthPtr )
{
    SQLRETURN          sNodeResult   = SQL_ERROR;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt     = NULL;
    ulsdLobLocator    *sShardLocator = NULL;
    acp_uint16_t       sNodeDbcIndex;
    acp_uint16_t       sIsNull;
             
    ACI_TEST_RAISE( aLengthPtr == NULL, LABEL_INVALID_NULL );

    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sShardLocator = (ulsdLobLocator *)aLocator;

    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mLobLocator == 0, LABEL_INVALID_LOCATOR );

    sNodeDbcIndex = sShardLocator->mNodeDbcIndex;
    sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

    SHARD_LOG("(GetLobLength) NodeId=%d, Server=%s:%d, StmtID=%d\n",
              sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
              sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
              sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
              sNodeStmt->mStatementID);

    sNodeResult = ulnGetLobLength( SQL_HANDLE_STMT,
                                   (ulnObject *)sNodeStmt,
                                   sShardLocator->mLobLocator,
                                   aLocatorType,
                                   aLengthPtr,
                                   &sIsNull );

    ACI_TEST_RAISE( sNodeResult != SQL_SUCCESS, LABEL_NODE_GETLOBLENGTH_FAIL );

    return sNodeResult;

    ACI_EXCEPTION( LABEL_INVALID_NULL )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER );
    }
    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION( LABEL_NODE_GETLOBLENGTH_FAIL )
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)sNodeStmt,
                                  sShard->mNodeInfo[sNodeDbcIndex],
                                  (acp_char_t *)"GetLobLength");
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

SQLRETURN ulsdGetLobNodes( ulnFnContext * aFnContext,
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
    SQLRETURN          sNodeResult   = SQL_ERROR;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt     = NULL;
    ulsdLobLocator    *sShardLocator = NULL;
    acp_uint16_t       sNodeDbcIndex;
             
    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sShardLocator = (ulsdLobLocator *)aSrcLocator;

    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mLobLocator == 0, LABEL_INVALID_LOCATOR );

    sNodeDbcIndex = sShardLocator->mNodeDbcIndex;
    sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

    SHARD_LOG("(GetLob) NodeId=%d, Server=%s:%d, StmtID=%d\n",
              sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
              sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
              sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
              sNodeStmt->mStatementID);

    sNodeResult = ulnGetLob( SQL_HANDLE_STMT,
                             (ulnObject *)sNodeStmt,
                             aLocatorCType,
                             sShardLocator->mLobLocator,
                             aFromPosition,
                             aForLength,
                             aTargetCType,
                             aBuffer,
                             aBufferSize,
                             aLengthWritten );

    ACI_TEST_RAISE( sNodeResult != SQL_SUCCESS, LABEL_NODE_GETLOB_FAIL );

    return sNodeResult;

    ACI_EXCEPTION( LABEL_NODE_GETLOB_FAIL )
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)sNodeStmt,
                                  sShard->mNodeInfo[sNodeDbcIndex],
                                  (acp_char_t *)"GetLob");
    }
    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sNodeResult;
}

// using callback
SQLRETURN ulsdPutLobNodes( ulnFnContext *aFnContext,
                           ulnStmt      *aStmt,
                           acp_sint16_t  aLocatorCType,
                           acp_uint64_t  aLocator,
                           acp_uint32_t  aFromPosition,
                           acp_uint32_t  aForLength,
                           acp_sint16_t  aSourceCType,
                           void         *aBuffer,
                           acp_uint32_t  aBufferSize )
{
    SQLRETURN          sNodeResult    = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult   = SQL_ERROR;
    acp_bool_t         sSuccess       = ACP_TRUE;
    ulsdFuncCallback  *sCallback      = NULL;
    ulsdDbc           *sShard         = NULL;
    ulnStmt           *sNodeStmt      = NULL;
    ulsdLobLocator    *sShardLocator  = NULL;
    acp_uint16_t       sNodeDbcIndex;
    acp_uint16_t       i;
             
    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sShardLocator = (ulsdLobLocator *)aLocator;

    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mArraySize == 0, LABEL_INVALID_LOCATOR );

    for ( i = 0; i < sShardLocator->mArraySize; i++ )
    {
        if ( sShardLocator[i].mLobLocator == 0 )
        {
            continue;
        }

        sNodeDbcIndex = sShardLocator[i].mNodeDbcIndex;
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        sErrorResult = ulsdPutLobAddCallback( sNodeDbcIndex,
                                              sNodeStmt,
                                              aLocatorCType,
                                              sShardLocator[i].mLobLocator,
                                              aFromPosition,
                                              aForLength,
                                              aSourceCType,
                                              aBuffer,
                                              aBufferSize, 
                                              &sCallback );
        ACI_TEST( sErrorResult != SQL_SUCCESS );
    }

    /* node execute 병렬수행 */
    ulsdDoCallback( sCallback );

    for ( i = 0; i < sShardLocator->mArraySize; i++ )
    {
        if ( sShardLocator[i].mLobLocator == 0 )
        {
            continue;
        }

        sNodeDbcIndex = sShardLocator[i].mNodeDbcIndex;
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        sNodeResult = ulsdGetResultCallback( sNodeDbcIndex, sCallback, (acp_uint8_t)0 );

        SHARD_LOG("(PutLob) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                  sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
                  sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
                  sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
                  sNodeStmt->mStatementID);

        if ( sNodeResult == SQL_SUCCESS )
        {
            /* Do Nothing */
        }
        else if ( sNodeResult == SQL_SUCCESS_WITH_INFO )
        {
            /* info의 전달 */
            ulsdNativeErrorToUlnError(aFnContext,
                                      SQL_HANDLE_STMT,
                                      (ulnObject *)sNodeStmt,
                                      sShard->mNodeInfo[sNodeDbcIndex],
                                      (acp_char_t *)"PutLob");

            sSuccessResult = sNodeResult;
        }
        else
        {
            /* BUGBUG 여러개의 데이터 노드에 대해서 retry가 가능한가? */
            if ( ulsdNodeFailConnectionLost( SQL_HANDLE_STMT,
                                             (ulnObject *)sNodeStmt ) == ACP_TRUE )
            {
                if ( aStmt->mParentDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
                {
                    ulsdClearOnTransactionNode(aStmt->mParentDbc);
                }
                else
                {
                    /* Do Nothing */
                }
            }

            ulsdNativeErrorToUlnError(aFnContext,
                                      SQL_HANDLE_STMT,
                                      (ulnObject *)sNodeStmt,
                                      sShard->mNodeInfo[sNodeDbcIndex],
                                      (acp_char_t *)"PutLob");

            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }
    }

    ACI_TEST_RAISE( sSuccess == ACP_FALSE, ERR_EXECUTE_FAIL );

    ulsdRemoveCallback( sCallback );

    return sSuccessResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION( ERR_EXECUTE_FAIL )
    {
        ulsdProcessExecuteError( aFnContext, aStmt, sCallback );
    }
    ACI_EXCEPTION_END;

    ulsdRemoveCallback( sCallback );

    return sErrorResult;
}

SQLRETURN ulsdFreeLobNodes( ulnFnContext * aFnContext,
                            ulnStmt      * aStmt,
                            acp_uint64_t   aLocator )
{
    SQLRETURN          sNodeResult    = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult   = SQL_ERROR;
    acp_bool_t         sSuccess       = ACP_TRUE;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt      = NULL;
    ulsdLobLocator    *sShardLocator  = NULL;
    acp_uint16_t       sNodeDbcIndex;
    acp_uint16_t       i;
             
    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sShardLocator = (ulsdLobLocator *)aLocator;

    /* 이미 free 된 것이어도 success 반환 */
    ACI_TEST_CONT( sShardLocator == NULL, NORMAL_EXIT );
    ACI_TEST_CONT( sShardLocator->mArraySize == 0, NORMAL_EXIT );

    for ( i = 0; i < sShardLocator->mArraySize; i++ )
    {
        if ( sShardLocator[i].mLobLocator == 0 )
        {
            continue;
        }

        sNodeDbcIndex = sShardLocator[i].mNodeDbcIndex;
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

        SHARD_LOG("(FreeLob) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                  sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
                  sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
                  sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
                  sNodeStmt->mStatementID);

        sNodeResult = ulnFreeLob( SQL_HANDLE_STMT,
                                  (ulnObject *)sNodeStmt,
                                  sShardLocator[i].mLobLocator);
        if ( sNodeResult == SQL_SUCCESS )
        {
            /* Nothing to do */
        }
        else
        {
            ulsdNativeErrorToUlnError(aFnContext,
                                      SQL_HANDLE_STMT,
                                      (ulnObject *)sNodeStmt,
                                      sShard->mNodeInfo[sNodeDbcIndex],
                                      (acp_char_t *)"FreeLob");

            sErrorResult = sNodeResult;
            sSuccess = ACP_FALSE;
        }

        sShardLocator[i].mLobLocator = 0;
    }

    sShardLocator->mArraySize = 0;

    ulsdLobLocatorDestroy( aStmt->mParentDbc,
                           sShardLocator,
                           ACP_TRUE /* NeedLock */);

    ACI_TEST( sSuccess == ACP_FALSE );

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return sSuccessResult;

    ACI_EXCEPTION_END;

    return sErrorResult;
}

SQLRETURN ulsdTrimLobNodes( ulnFnContext * aFnContext,
                            ulnStmt      * aStmt,
                            acp_sint16_t   aLocatorCType,
                            acp_uint64_t   aLocator,
                            acp_uint32_t   aStartOffset )
{
    SQLRETURN          sNodeResult    = SQL_ERROR;
    SQLRETURN          sSuccessResult = SQL_SUCCESS;
    SQLRETURN          sErrorResult   = SQL_ERROR;
    acp_bool_t         sSuccess       = ACP_TRUE;
    ulsdDbc           *sShard;
    ulnStmt           *sNodeStmt      = NULL;
    ulsdLobLocator    *sShardLocator  = NULL;
    acp_uint16_t       sNodeDbcIndex;
    //acp_uint16_t       i;
             
    ulsdGetShardFromDbc(aStmt->mParentDbc, &sShard);

    sShardLocator = (ulsdLobLocator *)aLocator;

    ACI_TEST_RAISE( sShardLocator == NULL, LABEL_INVALID_LOCATOR );
    /* select-fetch한 locator에 대해서만 trim이 가능하다 */
    ACI_TEST_RAISE( sShardLocator->mArraySize != 1, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sShardLocator->mLobLocator == 0, LABEL_INVALID_LOCATOR );

    sNodeDbcIndex = sShardLocator->mNodeDbcIndex;
    sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];

    SHARD_LOG("(TrimLob) NodeId=%d, Server=%s:%d, StmtID=%d\n",
              sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
              sShard->mNodeInfo[sNodeDbcIndex]->mServerIP,
              sShard->mNodeInfo[sNodeDbcIndex]->mPortNo,
              sNodeStmt->mStatementID);

    sNodeResult = ulnTrimLob( SQL_HANDLE_STMT,
                              (ulnObject *)sNodeStmt,
                              aLocatorCType,
                              sShardLocator->mLobLocator,
                              aStartOffset );

    if ( sNodeResult == SQL_SUCCESS )
    {
        /* Nothing to do */
    }
    else
    {
        ulsdNativeErrorToUlnError(aFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)sNodeStmt,
                                  sShard->mNodeInfo[sNodeDbcIndex],
                                  (acp_char_t *)"TrimLob");

        sErrorResult = sNodeResult;
        sSuccess = ACP_FALSE;
    }

    ACI_TEST( sSuccess == ACP_FALSE );

    return sSuccessResult;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return sErrorResult;
}

/***********************************************************************
 * Description : Lob Locator List를 삭제한다.
 *
 *    aMetaDbc       - [IN] Meta Dbc handle
 **********************************************************************/
ACI_RC ulsdLobLocatorFreeAll( ulnDbc * aMetaDbc )
{
    ulsdLobLocator  *sLocator;
    acp_list_node_t *sIterator1;
    acp_list_node_t *sIterator2;

    acpThrMutexLock( & aMetaDbc->mShardDbcCxt.mLock4LocatorList);
    ACP_LIST_ITERATE_SAFE(&(aMetaDbc->mShardDbcCxt.mLobLocatorList), sIterator1, sIterator2)
    {
        sLocator = (ulsdLobLocator *)sIterator1;

        (void) ulsdLobLocatorDestroy(aMetaDbc, sLocator, ACP_FALSE);
    }
    acpThrMutexUnlock( & aMetaDbc->mShardDbcCxt.mLock4LocatorList);

    return ACI_SUCCESS;
}

/***********************************************************************
 * Description : Shard Lob Locator를 삭제한다.
 *
 *    aMetaDbc       - [IN] Meta Dbc handle
 *    aLobLocator    - [IN] 삭제할 Lob Locator
 **********************************************************************/
ACI_RC ulsdLobLocatorDestroy( ulnDbc         * aMetaDbc,
                              ulsdLobLocator * aLobLocator,
                              acp_bool_t       aNeedLock )
{ 
    ACI_TEST(acpListIsEmpty(&aMetaDbc->mShardDbcCxt.mLobLocatorList));
         
    /* list 에서 삭제 */
    if ( aNeedLock == ACP_TRUE )
    {
        acpThrMutexLock( & aMetaDbc->mShardDbcCxt.mLock4LocatorList);
    }

    acpListDeleteNode(&(aLobLocator->mList));

    if ( aNeedLock == ACP_TRUE )
    {
        acpThrMutexUnlock( & aMetaDbc->mShardDbcCxt.mLock4LocatorList);
    }

    /* 메모리 해제 */
    aLobLocator->mArraySize  = 0;
    aLobLocator->mLobLocator = 0;
    acpMemFree(aLobLocator);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/***********************************************************************
 * Description : shard용 locator를 생성한다.
 *    1. shard locator 배열을 aCount 크기만큼 생성한다.
 *    2. 리스트를 초기화한다.
 *    3. aMetaDbc->mShardDbcCxt.mLobLocatorList에 매단다.
 *    4. 1을 반환한다.
 *
 *    aMetaDbc       - [IN] Meta Dbc handle
 *    aCount         - [IN] 생성할 배열 요소의 개수
 *    aOutputLocator - [OUT] 생성된 locator 주소 반환
 **********************************************************************/
ACI_RC ulsdLobLocatorCreate( ulnDbc          *aMetaDbc,
                             acp_uint16_t     aCount,
                             ulsdLobLocator **aOutputLocator )
{
    ulsdLobLocator *sLocator = NULL;

    // 1.
    ACI_TEST( acpMemCalloc((void **)&sLocator,
                           ACI_SIZEOF(ulsdLobLocator),
                           aCount)
              != ACI_SUCCESS );

    sLocator->mArraySize = aCount;

    // 2.
    acpListInitObj(&sLocator->mList, sLocator);

    // 3.
    acpThrMutexLock( & aMetaDbc->mShardDbcCxt.mLock4LocatorList);
    acpListAppendNode(&(aMetaDbc->mShardDbcCxt.mLobLocatorList), (acp_list_t *)sLocator); 
    acpThrMutexUnlock( & aMetaDbc->mShardDbcCxt.mLock4LocatorList);

    // 4.
    *aOutputLocator = sLocator;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/***********************************************************************
 * Description : shard용 locator를 생성한다(for Fetch).
 *    1. 수행중인 stmt의 Node Dbc Index를 구한다.
 *       server-side인 경우 Index를 -1 로 셋팅한다.
 *    2. shard locator 생성
 *    3. 2에 NodeDbcIndex, CType, Node Locator 저장
 *    4. 2 반환
 *
 *    aMetaDbc       - [IN] Statement handle
 *    aCType         - [IN] SQL_C_BLOB_LOCATOR or SQL_C_CLOB_LOCATOR
 *    aUserBufferPtr - [IN/OUT] 사용자 버퍼의 주소.
 *                          Node로부터 받은 Locator 값 입력.
 *                          생성한 shard locator 주소값 반환.
 **********************************************************************/
ACI_RC ulsdLobAddFetchLocator( ulnStmt      * aStmt,
                               acp_uint64_t * aUserBufferPtr )
{
    ulnDbc          *sMetaDbc      = NULL;
    ulsdLobLocator  *sShardLocator = NULL;
    ulsdStmtContext *sShardStmtCxt = NULL;
    acp_sint16_t     sNodeDbcIndex = -1;

    if ( ULSD_IS_SHARD_LIB_SESSION(aStmt->mParentDbc) == ACP_TRUE )
    {
        sShardStmtCxt = & aStmt->mShardStmtCxt.mParentStmt->mShardStmtCxt;
        sNodeDbcIndex = sShardStmtCxt->mNodeDbcIndexArr[sShardStmtCxt->mNodeDbcIndexCur];
        sMetaDbc = aStmt->mParentDbc->mShardDbcCxt.mParentDbc;
    }
    else // for COORD(Server-side) stmt
    {
        sNodeDbcIndex = -1;
        sMetaDbc = aStmt->mParentDbc;
    }

    ACI_TEST( ulsdLobLocatorCreate( sMetaDbc,
                                    1, // Array Size
                                    &sShardLocator )
              != ACI_SUCCESS );

    sShardLocator->mLobLocator = *aUserBufferPtr;
    sShardLocator->mNodeDbcIndex = sNodeDbcIndex;

    /* 사용자 버퍼에 shard locator 주소 복사하기 */
    *aUserBufferPtr = (acp_uint64_t)sShardLocator;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/***********************************************************************
 * Description : shard용 locator를 생성한다(for OutBind).
 *    ** 여러 노드로 수행되는 경우를 고려하여야 한다**
 *    1. 수행중인 stmt의 Node Dbc Index를 구한다.
 *       server-side인 경우 Index를 -1 로 셋팅한다.
 *    2. MetaStmt의 DescRecIpd를 구한다.
 *       수행되는 모든 노드에서 shard locator 배열을 공유하기 위해
 *       DescRecIpd->mShardLocator 변수를 사용한다.
 *
 *    aStmt           - [IN] Statement handle
 *    aParamNumber    - [IN] Parameter number
 *    aParamInOutType - [IN] INPUT/OUTPUT
 *    aInBindLocator  - [IN] 사용자가 입력한 Locator. INPUT일 때만 유효.
 *    aUserBufferPtr  - [IN/OUT] 사용자 버퍼의 주소.
 *                          Node로부터 받은 Locator 값 입력.
 *                          생성한 shard locator 주소값 반환.
 **********************************************************************/
ACI_RC ulsdLobAddOutBindLocator( ulnStmt           * aStmt,
                                 acp_uint16_t        aParamNumber,
                                 ulnParamInOutType   aParamInOutType,
                                 ulsdLobLocator    * aInBindLocator,
                                 acp_uint64_t      * aUserBufferPtr )
{
    ulnDbc          *sMetaDbc          = NULL;
    ulnStmt         *sMetaStmt         = NULL;
    ulsdLobLocator  *sShardLocator     = NULL;
    ulsdStmtContext *sMetaStmtShardCxt = NULL;
    ulnDescRec      *sMetaStmtDescRecIpd;
    acp_uint16_t     sNodeDbcIndexCount;
    acp_sint32_t     sNodeDbcIndexCur;
    acp_sint16_t     sNodeDbcIndex;

    if ( ULSD_IS_SHARD_LIB_SESSION(aStmt->mParentDbc) == ACP_TRUE )
    {
        sMetaDbc  = aStmt->mParentDbc->mShardDbcCxt.mParentDbc;
        sMetaStmt = aStmt->mShardStmtCxt.mParentStmt;

        sMetaStmtShardCxt   = &(sMetaStmt->mShardStmtCxt);
        sMetaStmtDescRecIpd = ulnStmtGetIpdRec(sMetaStmt, aParamNumber),

        sNodeDbcIndexCount = sMetaStmtShardCxt->mNodeDbcIndexCount;
        sNodeDbcIndexCur   = aStmt->mShardStmtCxt.mMyNodeDbcIndexCur;
        sNodeDbcIndex      = sMetaStmtShardCxt->mNodeDbcIndexArr[sNodeDbcIndexCur];
    }
    else // for COORD(Server-side) stmt
    {
        sMetaDbc  = aStmt->mParentDbc;
        sMetaStmt = aStmt;

        sMetaStmtDescRecIpd = ulnStmtGetIpdRec(sMetaStmt, aParamNumber),

        sNodeDbcIndexCount = 1;
        sNodeDbcIndexCur   = 0;
        sNodeDbcIndex      = -1; 
    }

    /* 1. 노드 중 처음 실행이라면  실행 노드 개수와 동일한
     *      크기로 ulsdLobLocator 배열을 생성하고,
     *      그 주소값을 parent stmt의 DescRecIpd 에 보관해 둔다.
     * 2. else, parent stmt의 DescRecIpd 에서 1에서 보관해 두었던
     *      shard Locator 를 꺼내온다. */

    sShardLocator = (ulsdLobLocator *)
            ulnDescRecGetShardLobLocator( sMetaStmtDescRecIpd );

    if ( sShardLocator == NULL )
    {
        ACI_TEST( ulsdLobLocatorCreate( sMetaDbc,
                                        sNodeDbcIndexCount,
                                        &sShardLocator )
                  != ACI_SUCCESS );

        ulnDescRecSetShardLobLocator( sMetaStmtDescRecIpd,
                                     (void *)sShardLocator );
    }
    else
    {        
        /* Nothing to do */
    }

    sShardLocator[sNodeDbcIndexCur].mLobLocator = *aUserBufferPtr;
    sShardLocator[sNodeDbcIndexCur].mNodeDbcIndex = sNodeDbcIndex;

    /* INPUT이 OUTPUT으로 바뀌어 실행된 경우라면,
     *   flag를 셋팅하고 실행 노드로부터의 out locator를 
     *   InBindLocator->mDestLobLocators에 돌려준다.
     * 그렇지 않으면, 사용자 버퍼에 돌려준다 */
    if ( aParamInOutType == ULN_PARAM_INOUT_TYPE_INPUT )
    {
        sMetaStmt->mShardStmtCxt.mHasLocatorParamToCopy = ACP_TRUE;

        aInBindLocator->mDestLobLocators = sShardLocator;

        /* 사용자가 InBind 했던 값을 다시 복사하기 */
        *aUserBufferPtr = (acp_uint64_t)aInBindLocator;
    }
    else
    {
        /* 사용자 버퍼에 shard locator 주소 복사하기 */
        *aUserBufferPtr = (acp_uint64_t)sShardLocator;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/***********************************************************************
 * Description : Src Locator로부터 읽어서 Dest Locator로 쓰기
 *    1. Src Locator에서 길이와 NULL 여부를 가져온다.
 *    2. NULL이 아니면, 각 노드에 대해 아래 수행
 *       2-1. Prepare4Write
 *       2-2. 길이가 0보다 길면, 32KB씩 Get -> Write
 *       2-3. 길이가 0이면, Write size 0
 *       2-4. FinishWrite
 *    3. NULL이면 아무것도 하지 않는다.
 *    
 *
 *    aFnContext     - [IN] Function Context
 *    aShard         - [IN] shard Dbc
 *    aStmt          - [IN] 수행중인 statement handle
 **********************************************************************/
static SQLRETURN ulsdLobCopyNodes( ulnFnContext     * aFnContext,
                                   ulsdDbc          * aShard,
                                   ulnStmt          * aStmt,
                                   ulsdLobLocator   * aSrcLocator,
                                   acp_sint16_t       aLocatorCType )
{
    SQLRETURN          sRet;
    ulnDbc            *sSrcDbc;
    ulnStmt           *sDestStmt            = NULL;
    acp_uint32_t       sSrcLobLen;
    acp_uint16_t       sIsNullLob;
    acp_uint32_t       sReadBytes           = 0;
    acp_uint32_t       sReadLength          = 0;
    acp_uint32_t       sOffset              = 0;
    acp_uint32_t       sRemainingSize       = 0;
    acp_char_t        *sLobBuffer           = NULL;
    acp_uint16_t       i;
    ulsdLobLocator   * sDestLocators        = NULL;

    acp_sint16_t       sHandleType;
    ulnObject        * sErrorRaiseObject    = NULL;
    ulsdNodeInfo     * sNodeInfo            = NULL;

    sDestLocators = aSrcLocator->mDestLobLocators;

    // INPUT -> INPUT
    ACI_TEST_CONT( sDestLocators == NULL, NORMAL_EXIT );

    ACI_TEST_RAISE( aSrcLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( aSrcLocator->mLobLocator == 0, LABEL_INVALID_LOCATOR );

    if (aSrcLocator->mNodeDbcIndex >= 0 ) // from NODE
    {
        sSrcDbc = aStmt->mParentDbc->mShardDbcCxt.mShardDbc->
                      mNodeInfo[aSrcLocator->mNodeDbcIndex]->mNodeDbc;
    }
    else // from COORD(Server-side)
    {
        sSrcDbc = aStmt->mParentDbc;
    }

    /* 1. Src Locator에서 길이와 NULL 여부를 가져온다. */
    sRet = ulnGetLobLength ( SQL_HANDLE_DBC,
                             (ulnObject *)sSrcDbc,
                             aSrcLocator->mLobLocator, 
                             aLocatorCType,
                             &sSrcLobLen,
                             &sIsNullLob );

    ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_SRC_ERROR );

    /* 2. NULL이 아니면, 각 노드에 대해 아래 수행 */
    if ( sIsNullLob != ACP_TRUE )
    {
        /*    2-1. Prepare4Write */
        for ( i = 0; i < sDestLocators->mArraySize; i++ )
        {
            if ( sDestLocators[i].mLobLocator != 0 )
            {
                sDestStmt = ulsdLobGetExecuteStmt(aStmt, & sDestLocators[i]);

                sRet = ulsdnLobPrepare4Write( SQL_HANDLE_STMT,
                                              (ulnObject *)sDestStmt,
                                              aLocatorCType,
                                              sDestLocators[i].mLobLocator,
                                              0,            // offset
                                              sSrcLobLen ); // size

                ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_DEST_ERROR );
            }
            else
            {
                /* Nothing to do */
            }
        }

        /*    2-2. 길이가 0보다 길면, 32KB씩 Get -> Write */
        if ( sSrcLobLen > 0 )
        {     
            ACI_TEST( acpMemAlloc((void **)&sLobBuffer,
                                   ULN_LOB_PIECE_SIZE)
                      != ACI_SUCCESS );

            while (sOffset < sSrcLobLen)
            {
                sRemainingSize = sSrcLobLen - sOffset;
                sReadBytes = ACP_MIN(sRemainingSize, ULN_LOB_PIECE_SIZE);

                sRet = ulnGetLob( SQL_HANDLE_DBC,
                                  (ulnObject *)sSrcDbc,
                                  aLocatorCType,
                                  aSrcLocator->mLobLocator,
                                  sOffset,
                                  sReadBytes,
                                  SQL_C_BINARY,
                                  sLobBuffer,
                                  ULN_LOB_PIECE_SIZE,
                                  &sReadLength );

                ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_SRC_ERROR );
 
                for ( i = 0; i < sDestLocators->mArraySize; i++ )
                {
                    if ( sDestLocators[i].mLobLocator != 0 )
                    {
                        sDestStmt = ulsdLobGetExecuteStmt(aStmt, & sDestLocators[i]);

                        sRet = ulsdnLobWrite ( SQL_HANDLE_STMT,
                                               (ulnObject *)sDestStmt,
                                               aLocatorCType,
                                               sDestLocators[i].mLobLocator,
                                               SQL_C_BINARY,
                                               sLobBuffer,
                                               sReadLength );

                        ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_DEST_ERROR );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                sOffset += sReadLength;
            }

        }
        /*    2-3. 길이가 0이면, Write size 0 */
        else
        {
            for ( i = 0; i < sDestLocators->mArraySize; i++ )
            {
                if ( sDestLocators[i].mLobLocator != 0 )
                {
                    sDestStmt = ulsdLobGetExecuteStmt(aStmt, & sDestLocators[i]);

                    sRet = ulsdnLobWrite ( SQL_HANDLE_STMT,
                                           (ulnObject *)sDestStmt,
                                           aLocatorCType,
                                           sDestLocators[i].mLobLocator,
                                           SQL_C_BINARY,
                                           sLobBuffer,
                                           0 ); // size 0

                    ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_DEST_ERROR );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        /*    2-4. FinishWrite */
        for ( i = 0; i < sDestLocators->mArraySize; i++ )
        {
            if ( sDestLocators[i].mLobLocator != 0 )
            {
                sDestStmt = ulsdLobGetExecuteStmt(aStmt, & sDestLocators[i]);

                sRet = ulsdnLobFinishWrite( SQL_HANDLE_STMT,
                                            (ulnObject *)sDestStmt,
                                            aLocatorCType,
                                            sDestLocators[i].mLobLocator );

                ACI_TEST_RAISE( sRet != SQL_SUCCESS, LABEL_DEST_ERROR );
            }
            else
            {
                /* Nothing to do */
            }
        }

    }
    /* 3. NULL이면 아무것도 하지 않는다. */
    else
    {
        /* Nothing to do */
    }

    (void) ulsdFreeLob( aStmt,
                        (acp_uint64_t)sDestLocators );

    if ( sLobBuffer != NULL )
    {
        acpMemFree(sLobBuffer);
    }

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_SRC_ERROR)
    {
        sHandleType = SQL_HANDLE_DBC;
        sErrorRaiseObject = (ulnObject *)sSrcDbc;
        if ( ULSD_IS_SHARD_LIB_SESSION(sSrcDbc) == ACP_TRUE )
        {
            sNodeInfo = sSrcDbc->mShardDbcCxt.mNodeInfo;
        }
        else
        {
            sNodeInfo = NULL;
        }
    }
    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
        sNodeInfo = NULL;
    }
    ACI_EXCEPTION(LABEL_DEST_ERROR)
    {
        if ( ULSD_IS_SHARD_LIB_SESSION(sDestStmt->mParentDbc) == ACP_TRUE )
        {
            /* BUGBUG 여러개의 데이터 노드에 대해서 retry가 가능한가? */
            if ( ulsdNodeFailConnectionLost( SQL_HANDLE_STMT,
                                             (ulnObject *)sDestStmt ) == ACP_TRUE )
            {
                if ( aStmt->mParentDbc->mAttrAutoCommit == SQL_AUTOCOMMIT_OFF )
                {
                    ulsdClearOnTransactionNode(aStmt->mParentDbc);
                }
                else
                {
                    /* Do Nothing */
                }
            }

            sNodeInfo = aShard->mNodeInfo[sDestLocators[i].mNodeDbcIndex];
        }
        else
        {
            sNodeInfo = NULL;
        }

        sHandleType = SQL_HANDLE_STMT;
        sErrorRaiseObject = (ulnObject *)sDestStmt;
    }
    ACI_EXCEPTION_END;

    if ( sNodeInfo != NULL )
    {
        ulsdNativeErrorToUlnError( aFnContext,
                                   sHandleType,
                                   sErrorRaiseObject,
                                   sNodeInfo,
                                   (acp_char_t *)"ulsdLobCopy" );
    }

    (void) ulsdFreeLob( aStmt,
                        (acp_uint64_t)sDestLocators );

    if ( sLobBuffer != NULL )
    {
        acpMemFree(sLobBuffer);
    }

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

/***********************************************************************
 * Description : InBind Locator의 Node(Src.)가 Dest. Node와 다른 경우,
 *    BindParam list중 IN -> OUT으로 바뀌어 수행된 경우 
 *    Src -> Dest로의 Copy 작업을 수행한다.
 *
 *    aFnContext     - [IN] Function Context
 *    aShard         - [IN] shard Dbc
 *    aStmt          - [IN] 수행중인 statement handle
 **********************************************************************/
SQLRETURN ulsdLobCopy( ulnFnContext     * aFnContext,
                       ulsdDbc          * aShard,
                       ulnStmt          * aStmt )
{
    SQLRETURN     sResult = SQL_SUCCESS;

    acp_list_node_t       * sNode     = NULL;
    ulsdBindParameterInfo * sBind     = NULL;

    ACP_LIST_ITERATE( & aStmt->mShardStmtCxt.mBindParameterList, sNode )
    {
        sBind = (ulsdBindParameterInfo *)sNode->mObj;
                                   
        if ( ulsdParamIsLocInBoundLob( sBind->mValueType,
                                       sBind->mInputOutputType )
             == ACP_TRUE )
        {
            sResult = ulsdLobCopyNodes( aFnContext,
                                        aShard,
                                        aStmt,
                                        (ulsdLobLocator *)(*(acp_uint64_t *)
                                           sBind->mParameterValuePtr),
                                        sBind->mValueType );

            ACI_TEST( sResult != SQL_SUCCESS );
        }
    }

    return sResult;

    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(aFnContext);
}

ACI_RC ulsdLobBindInfoBuild4Param(ulnFnContext      *aFnContext,
                                  void              *aUserBufferPtr,
                                  acp_bool_t        *aChangeInOutType)
{
    ulnStmt        *sMetaStmt        = NULL;
    ulnStmt        *sStmt            = aFnContext->mHandle.mStmt;
    ulsdLobLocator *sSrcLocator      = NULL;
    acp_bool_t      sChangeInOutType = ACP_FALSE;
    acp_sint16_t    sDestNodeDbcIndex;

    if ( ULSD_IS_SHARD_LIB_SESSION(sStmt->mParentDbc) == ACP_TRUE )
    {
        sMetaStmt = sStmt->mShardStmtCxt.mParentStmt;
        sDestNodeDbcIndex = sMetaStmt->mShardStmtCxt.mNodeDbcIndexArr[sStmt->mShardStmtCxt.mMyNodeDbcIndexCur];
    }
    else // COORD(Server-side) stmt.
    {
        sMetaStmt = sStmt;
        sDestNodeDbcIndex = -1;
    }

    /*
     * Lob Locator를 InBind한 경우,
     * InBind locator의 노드와 execute 대상 노드가 서로 다르면
     * OutBind로 바꾸어서 execute 하여 Locator를 받아온 후
     * copy 작업(ulsdCopyLob)을 별도로 수행한다.
     */
    sSrcLocator = (ulsdLobLocator *)(*(acp_uint64_t *)aUserBufferPtr);

    ACI_TEST_RAISE( sSrcLocator == NULL, LABEL_INVALID_LOCATOR );
    ACI_TEST_RAISE( sSrcLocator->mLobLocator == 0, LABEL_INVALID_LOCATOR );

    if ( sSrcLocator->mNodeDbcIndex != sDestNodeDbcIndex )
    {
        sChangeInOutType = ACP_TRUE;
    }
    else
    {
        sChangeInOutType = ACP_FALSE;
    }

    sMetaStmt->mShardStmtCxt.mHasLocatorInBoundParam = ACP_TRUE;

    *aChangeInOutType = sChangeInOutType;

    return ACI_SUCCESS;

    ACI_EXCEPTION( LABEL_INVALID_LOCATOR )
    {
        ulnError( aFnContext, ulERR_ABORT_INVALID_LOCATOR_HANDLE );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

