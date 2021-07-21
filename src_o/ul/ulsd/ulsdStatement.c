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

SQLRETURN ulsdNodeFreeStmt(ulnStmt      *aMetaStmt,
                           acp_uint16_t  aOption)
{
    SQLRETURN           sRet = SQL_ERROR;
    ulnFnContext        sFnContext;
    ulsdDbc            *sShard;
    acp_uint16_t        i;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_FREESTMT, aMetaStmt, ULN_OBJ_TYPE_STMT );

    /* BUG-47553 */
    ACI_TEST_RAISE( ulsdEnter( &sFnContext ) != ACI_SUCCESS, LABEL_ENTER_ERROR );

    ulsdGetShardFromDbc(aMetaStmt->mParentDbc, &sShard);

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        SHARD_LOG("(Free Stmt) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                  sShard->mNodeInfo[i]->mNodeId,
                  sShard->mNodeInfo[i]->mServerIP,
                  sShard->mNodeInfo[i]->mPortNo,
                  aMetaStmt->mShardStmtCxt.mShardNodeStmt[i]->mStatementID);

        sRet = ulnFreeStmt(aMetaStmt->mShardStmtCxt.mShardNodeStmt[i], aOption);
        ACI_TEST_RAISE(sRet != ACI_SUCCESS, LABEL_NODE_FREESTMT_FAIL);
    }

    return sRet;

    ACI_EXCEPTION( LABEL_ENTER_ERROR )
    {
        sRet = ULN_FNCONTEXT_GET_RC( &sFnContext );
    }
    ACI_EXCEPTION(LABEL_NODE_FREESTMT_FAIL)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREESTMT, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulsdNativeErrorToUlnError(&sFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)aMetaStmt->mShardStmtCxt.mShardNodeStmt[i],
                                  sShard->mNodeInfo[i],
                                  (acp_char_t *)"FreeStmt");
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdNodeFreeHandleStmt(ulsdDbc       *aShard,
                                 ulnStmt       *aMetaStmt)
{
    SQLRETURN           sRet = SQL_ERROR;
    ulnFnContext        sFnContext;
    acp_uint16_t        i;

    for ( i = 0; i < aShard->mNodeCount; i++ )
    {
        sRet = ulnFreeHandle(SQL_HANDLE_STMT, aMetaStmt->mShardStmtCxt.mShardNodeStmt[i]);
        ACI_TEST_RAISE(sRet != ACI_SUCCESS, LABEL_NODE_FREEHANDLE_STMT_FAIL);

        SHARD_LOG("(Free Handle Stmt) NodeId=%d, Server=%s:%d, StmtID=%d\n",
                  aShard->mNodeInfo[i]->mNodeId,
                  aShard->mNodeInfo[i]->mServerIP,
                  aShard->mNodeInfo[i]->mPortNo,
                  aMetaStmt->mShardStmtCxt.mShardNodeStmt[i]->mStatementID);
    }

    return sRet;

    ACI_EXCEPTION(LABEL_NODE_FREEHANDLE_STMT_FAIL)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREEHANDLE_STMT, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulsdNativeErrorToUlnError(&sFnContext,
                                  SQL_HANDLE_STMT,
                                  (ulnObject *)aMetaStmt->mShardStmtCxt.mShardNodeStmt[i],
                                  aShard->mNodeInfo[i],
                                  (acp_char_t *)"FreeHandleStmt");
    }
    ACI_EXCEPTION_END;

    return sRet;
}

SQLRETURN ulsdNodeDecideStmt(ulnStmt       *aMetaStmt,
                             acp_uint16_t   aFuncId)
{
    ulnFnContext    sFnContext;

    ACE_ASSERT( aMetaStmt->mShardStmtCxt.mShardCoordQuery == ACP_FALSE );
    ACE_ASSERT( aMetaStmt->mShardStmtCxt.mShardIsShardQuery == ACP_TRUE );

    if ( aFuncId == ULN_FID_EXECUTE )
    {
        /* 분산정보가 없으면 에러를 발생시킨다. */
        ACI_TEST_RAISE( ( aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt == 0 ) &&
                        ( aMetaStmt->mShardStmtCxt.mShardDefaultNodeID == ACP_UINT32_MAX ),
                        LABEL_NO_RANGE_FOUNDED );

        ACI_TEST( ulsdNodeDecideStmtByValues(aMetaStmt,
                                             aFuncId) != SQL_SUCCESS );
    }
    else
    {
        /* fetch, rowcount등 execute이후에 호출되는 경우
         * execute시 decide한 node를 활용한다.
         */
    }

    ACI_TEST_RAISE( aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount == 0,
                    LABEL_NOT_EXECUTED );

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_NO_RANGE_FOUNDED)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "No shard range");
    }
    ACI_EXCEPTION(LABEL_NOT_EXECUTED)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Not executed");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdNodeDecideStmtByValues(ulnStmt       *aMetaStmt,
                                     acp_uint16_t   aFuncId )
{
/*********************************************************************************************
 *
 * PROJ-2655 Composite shard key
 *
 * STEP 1.
 *        Shard values( bind or constant value )가
 *        Server로 부터 전달 받은 analysis result상의 range info에서
 *        몇 번 째(range index) 위치한 value인지 찾는다.
 *
 * STEP 2.
 *        STEP 1.에서 구해진 수행 대상 node의 range index로 node index를 구한다.
 *        두 개 이상의 nodes가 수행 대상이 되면, 에러를 발생 시킨다.
 *
 *********************************************************************************************/

    ulnFnContext    sFnContext;

    acp_uint16_t    sNodeDbcIndex;

    ulsdRangeIndex  sRangeIndex[1000];
    acp_uint16_t    sRangeIndexCount = 0;

    ulsdRangeIndex  sSubRangeIndex[1000];
    acp_uint16_t    sSubRangeIndexCount = 0;

    acp_uint16_t    sSubValueIndex = ACP_UINT16_MAX;

    acp_bool_t      sExecDefaultNode = ACP_FALSE;

    acp_uint16_t i;
    acp_uint16_t j;

    acp_bool_t      sIsFound = ACP_FALSE;

    ulsdValueInfo ** sValuePtrArray = NULL;
    ulsdValueInfo ** sSubValuePtrArray = NULL;

    acp_bool_t      sNodeDbcFlags[ULSD_SD_NODE_MAX_COUNT] = { 0, }; /* ACP_FALSE */
    ulsdDbc        *sShard;

    if ( aMetaStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_CLONE )
    {
        /* SPLIT CLONE 일 경우 */
        if ( aMetaStmt->mShardStmtCxt.mShardValueCnt == 0 )
        {
            /* mShardValueCnt가 0이면 random range 선택 */

            /* Analysis의 결과가 split clone 일 경우 수행 대상 노드가 하나 이하이다. */
            ACI_TEST(ulsdGetRangeIndexFromClone(aMetaStmt,
                                                &sRangeIndex[0].mRangeIndex)
                     != SQL_SUCCESS);

            ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                         aMetaStmt,
                         aMetaStmt->mShardStmtCxt.
                         mShardRangeInfo[sRangeIndex[0].mRangeIndex].mShardNodeID,
                         &sNodeDbcIndex,
                         aFuncId)
                     != SQL_SUCCESS);

            /* 기록 */
            sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
        }
        else
        {
            /* mShardValueCnt가 0이 아니면 전 range 선택 */
            for (i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++)
            {
                ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                             aMetaStmt,
                             aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardNodeID,
                             &sNodeDbcIndex,
                             aFuncId)
                         != SQL_SUCCESS);

                /* 기록 */
                sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
            }
        }
    }
    else if ( aMetaStmt->mShardStmtCxt.mShardSplitMethod == ULN_SHARD_SPLIT_SOLO )
    {
        /* SPLIT SOLO 일 경우 */
        /* mShardValueCnt가 1이 아니면 에러 */
        ACP_TEST_RAISE( aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt != 1,
                        LABEL_UNEXPECTED_ERROR );

        ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                     aMetaStmt,
                     aMetaStmt->mShardStmtCxt.mShardRangeInfo[0].mShardNodeID,
                     &sNodeDbcIndex,
                     aFuncId)
                 != SQL_SUCCESS);

        /* 기록 */
        sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
    }
    else
    {
        // SPLIT HASH, RANGE, LIST 일 경우

        sValuePtrArray = aMetaStmt->mShardStmtCxt.mShardValueInfoPtrArray;

        // 첫 번 째 shard key value에 대한, range index를 구한다.
        for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardValueCnt; i++ )
        {
            ACI_TEST( ulsdGetRangeIndexByValues( aMetaStmt,
                                                 i,
                                                 sValuePtrArray[i],
                                                 sRangeIndex,
                                                 &sRangeIndexCount,
                                                 &sExecDefaultNode,
                                                 ACP_FALSE,
                                                 aFuncId )
                      != SQL_SUCCESS );
        }

        if ( aMetaStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE )
        {
            /*
             * 두 번 째 shard key value에 대한, range index를 구한다.
             */
            for ( i = 0; i < aMetaStmt->mShardStmtCxt.mShardSubValueCnt; i++ )
            {
                if ( aMetaStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE )
                {
                    sSubValuePtrArray = aMetaStmt->mShardStmtCxt.mShardSubValueInfoPtrArray;

                    ACI_TEST( ulsdGetRangeIndexByValues( aMetaStmt,
                                                         i,
                                                         sSubValuePtrArray[i],
                                                         sSubRangeIndex,
                                                         &sSubRangeIndexCount,
                                                         &sExecDefaultNode,
                                                         ACP_TRUE,
                                                         aFuncId )
                              != SQL_SUCCESS );
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }
        else
        {
            /* Nothing to do. */
        }

        /*
         * Range index로 node index를 구한다.
         */
        if ( aMetaStmt->mShardStmtCxt.mShardValueCnt > 0 )
        {
            if ( ( aMetaStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE ) &&
                 ( aMetaStmt->mShardStmtCxt.mShardSubValueCnt > 0 ) )
            {
                /*
                 * CASE 1 : ( mValueCount > 0 && mSubValueCount > 0 )
                 *
                 * value와 sub value의 range index가 같은 노드들이 수행대상이 된다.
                 */
                sSubValueIndex = sSubRangeIndex[0].mValueIndex;

                for ( i = 0; i < sSubRangeIndexCount; i++ )
                {
                    if ( sSubValueIndex != sSubRangeIndex[i].mValueIndex )
                    {
                        if ( sIsFound == ACP_FALSE )
                        {
                            sExecDefaultNode = ACP_TRUE;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }

                        sSubValueIndex = sSubRangeIndex[i].mValueIndex;
                        sIsFound = ACP_FALSE;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }

                    for ( j = 0; j < sRangeIndexCount; j++ )
                    {
                        if ( sRangeIndex[j].mRangeIndex == sSubRangeIndex[i].mRangeIndex )
                        {
                            ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                                         aMetaStmt,
                                         aMetaStmt->mShardStmtCxt.
                                         mShardRangeInfo[sRangeIndex[j].mRangeIndex].mShardNodeID,
                                         &sNodeDbcIndex,
                                         aFuncId)
                                     != SQL_SUCCESS);

                            /* 기록 */
                            sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
                            sIsFound = ACP_TRUE;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                }
            }
            else
            {
                /*
                 * CASE 2 : ( mValueCount > 0 && mSubValueCount == 0 )
                 *
                 * value의 range index에 해당하는 노드들이 수행대상이 된다.
                 */
                for ( i = 0; i < sRangeIndexCount; i++ )
                {
                    ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                                 aMetaStmt,
                                 aMetaStmt->mShardStmtCxt.
                                 mShardRangeInfo[sRangeIndex[i].mRangeIndex].mShardNodeID,
                                 &sNodeDbcIndex,
                                 aFuncId)
                             != SQL_SUCCESS);

                    /* 기록 */
                    sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
                    sIsFound = ACP_TRUE;
                }

                /* BUG-45738 */
                if ( aMetaStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE )
                {
                    /* Default node를 수행노드에 포함한다. */
                    sExecDefaultNode = ACP_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else
        {
            if ( ( aMetaStmt->mShardStmtCxt.mShardIsSubKeyExists == ACP_TRUE ) &&
                 ( aMetaStmt->mShardStmtCxt.mShardSubValueCnt > 0 ) )
            {
                /*
                 * CASE 3 : ( mValueCount == 0 && mSubValueCount > 0 )
                 *
                 * sub value의 range index에 해당하는 노드들이 수행대상이 된다.
                 */
                for ( j = 0; j < sSubRangeIndexCount; j++ )
                {
                    ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                                 aMetaStmt,
                                 aMetaStmt->mShardStmtCxt.
                                 mShardRangeInfo[sSubRangeIndex[j].mRangeIndex].mShardNodeID,
                                 &sNodeDbcIndex,
                                 aFuncId)
                             != SQL_SUCCESS);

                    /* 기록 */
                    sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
                    sIsFound = ACP_TRUE;

                    /* BUG-45738 */
                    /* Default node를 수행노드에 포함한다. */
                    sExecDefaultNode = ACP_TRUE; 
                }
            }
            else
            {
                /*
                 * CASE 4 : ( mValueCount == 0 && mSubValueCount == 0 )
                 *
                 * Shard value가 없다면, 모든 노드가 수행 대상이 된다.
                 */
                for (i = 0; i < aMetaStmt->mShardStmtCxt.mShardRangeInfoCnt; i++)
                {
                    ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                                 aMetaStmt,
                                 aMetaStmt->mShardStmtCxt.mShardRangeInfo[i].mShardNodeID,
                                 &sNodeDbcIndex,
                                 aFuncId)
                             != SQL_SUCCESS);

                    /* 기록 */
                    sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
                    sIsFound = ACP_TRUE;
                }

                /* BUG-45738 */
                /* Default node를 수행노드에 포함한다. */
                sExecDefaultNode = ACP_TRUE;
            }
        }

        /*
         * value또는 sub value의 값이 shard range info에 없거나, ( sExecDefaultNode == ACP_TRUE )
         * value와 sub value의 range index가 겹치지 않는 경우 ( sIsFound == ACP_FALSE )
         * default node가 있다면 default node를 수행 대상에 포함시킨다.
         */
        if ( ( sExecDefaultNode == ACP_TRUE ) || ( sIsFound == ACP_FALSE ) )
        {
            /* BUG-45738 */
            // Default node외에 수행 대상 노드가 없는데
            // Default node가 설정 되어있지 않다면 에러
            ACI_TEST_RAISE(( sIsFound == ACP_FALSE ) &&
                           ( aMetaStmt->mShardStmtCxt.mShardDefaultNodeID == ACP_UINT32_MAX ),
                           LABEL_NO_NODE_FOUNDED);

            // Default node가 없더라도, 수행 대상 노드가 하나라도 있으면
            // 그 노드에서만 수행시킨다. ( for SELECT )
            if ( aMetaStmt->mShardStmtCxt.mShardDefaultNodeID != ACP_UINT32_MAX )
            {
                ACI_TEST(ulsdConvertNodeIdToNodeDbcIndex(
                             aMetaStmt,
                             aMetaStmt->mShardStmtCxt.mShardDefaultNodeID,
                             &sNodeDbcIndex,
                             aFuncId)
                         != SQL_SUCCESS);
                /* 기록 */
                sNodeDbcFlags[sNodeDbcIndex] = ACP_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }

    ulsdGetShardFromDbc(aMetaStmt->mParentDbc, &sShard);

    /* mShardStmtCxt에 기록 */
    aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount = 0;
    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        if ( sNodeDbcFlags[i] == ACP_TRUE )
        {
            aMetaStmt->mShardStmtCxt.
                mNodeDbcIndexArr[aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount] = i;
            aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount++;
        }
        else
        {
            /* Nothing to do. */
        }
    }

    ACE_DASSERT( aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount > 0 );

    SHARD_LOG("(Decide Stmt By values) ParamData=%s, NodeId=%d, NodeIndex=%d\n",
              "(Not available)", /* Cannot get param data in here */
              sShard->mNodeInfo[sNodeDbcIndex]->mNodeId,
              sNodeDbcIndex);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_NO_NODE_FOUNDED)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "No data node");
    }
    ACI_EXCEPTION(LABEL_UNEXPECTED_ERROR)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Unexpected error");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}


SQLRETURN ulsdGetRangeIndexByValues(ulnStmt        *aMetaStmt,
                                    acp_uint16_t    aValueIndex,
                                    ulsdValueInfo  *aValue,
                                    ulsdRangeIndex *aRangeIndex,
                                    acp_uint16_t   *aRangeIndexCount,
                                    acp_bool_t     *aHasDefaultNode,
                                    acp_bool_t      aIsSubKey,
                                    acp_uint16_t    aFuncId)
{
    ulsdKeyData   sShardKeyData;
    ulsdKeyData  *sValue;

    acp_uint32_t sShardKeyDataType;
    acp_uint8_t  sShardSplitMethod;
    mtdModule  * sShardKeyModule;

    ulnDescRec * sDescRecIpd;
    ulnDescRec * sDescRecApd;

    acp_uint32_t sHashValue;

    ulnFnContext sFnContext;

    if ( aIsSubKey == ACP_FALSE )
    {
        sShardKeyDataType = aMetaStmt->mShardStmtCxt.mShardKeyDataType;
        sShardSplitMethod = aMetaStmt->mShardStmtCxt.mShardSplitMethod;
    }
    else
    {
        sShardKeyDataType = aMetaStmt->mShardStmtCxt.mShardSubKeyDataType;
        sShardSplitMethod = aMetaStmt->mShardStmtCxt.mShardSubSplitMethod;
    }

    if ( ( sShardSplitMethod == ULN_SHARD_SPLIT_HASH ) ||
         ( sShardSplitMethod == ULN_SHARD_SPLIT_RANGE ) ||
         ( sShardSplitMethod == ULN_SHARD_SPLIT_LIST ) )
    {
        if ( aValue->mType == 0 ) // bind param
        {
            sDescRecIpd = ulnStmtGetIpdRec(aMetaStmt, aValue->mValue.mBindParamId + 1);
            sDescRecApd = ulnStmtGetApdRec(aMetaStmt, aValue->mValue.mBindParamId + 1);

            ACI_TEST( ulsdGetShardKeyMtdModule(aMetaStmt,
                                               &sShardKeyModule,
                                               sShardKeyDataType,
                                               sDescRecIpd,
                                               aFuncId ) != SQL_SUCCESS );

            ACI_TEST(ulsdGetParamData(aMetaStmt,
                                      sDescRecApd,
                                      sDescRecIpd,
                                      &sShardKeyData,
                                      sShardKeyModule,
                                      aFuncId) != SQL_SUCCESS);

            sValue = &sShardKeyData;
        }
        else if ( aValue->mType == 1 ) // constant value
        {
            ACI_TEST( ulsdMtdModuleById( aMetaStmt,
                                         &sShardKeyModule,
                                         sShardKeyDataType,
                                         aFuncId )
                      != SQL_SUCCESS );

            sValue = (ulsdKeyData*)&aValue->mValue;
        }
        else
        {
            ACI_RAISE(LABEL_INVALID_SHARD_VALUE);
        }

        if ( sShardSplitMethod == ULN_SHARD_SPLIT_HASH )
        {
            sHashValue = sShardKeyModule->hash( mtcHashInitialValue,
                                                NULL,
                                                (*sValue).mValue,
                                                MTD_OFFSET_USELESS );

            /* BUG-46288 */
            if ( aMetaStmt->mParentDbc->mShardDbcCxt.mShardDbc->mIsTestEnable == ACP_FALSE )
            {
                sHashValue %= ULSD_HASH_MAX_VALUE;
            }
            else
            {
                sHashValue %= ULSD_HASH_MAX_VALUE_FOR_TEST;
            }

            ACI_TEST(ulsdGetRangeIndexFromHash(aMetaStmt,
                                               aValueIndex,
                                               sHashValue,
                                               aRangeIndex,
                                               aRangeIndexCount,
                                               aHasDefaultNode,
                                               aIsSubKey,
                                               aFuncId) != SQL_SUCCESS);
        }
        else if ( sShardSplitMethod == ULN_SHARD_SPLIT_RANGE ) 
        {       
            ACI_TEST(ulsdGetRangeIndexFromRange(aMetaStmt,
                                                aValueIndex,
                                                sValue,
                                                sShardKeyModule,
                                                aRangeIndex,
                                                aRangeIndexCount,
                                                aHasDefaultNode,
                                                aIsSubKey,
                                                aFuncId) != SQL_SUCCESS);
        }
        else if ( sShardSplitMethod == ULN_SHARD_SPLIT_LIST )
        {
            ACI_TEST(ulsdGetRangeIndexFromList(aMetaStmt,
                                               aValueIndex,
                                               sValue,
                                               sShardKeyModule,
                                               aRangeIndex,
                                               aRangeIndexCount,
                                               aHasDefaultNode,
                                               aIsSubKey,
                                               aFuncId) != SQL_SUCCESS);
        }         
        else
        {
            ACI_RAISE( ERR_INVALID_SPLIT_METHOD );
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return SQL_SUCCESS;
    
    ACI_EXCEPTION(LABEL_INVALID_SHARD_VALUE)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Invalid shard value");
    }
    ACI_EXCEPTION(ERR_INVALID_SPLIT_METHOD)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Invalid shard split method");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdCheckValuesSame(ulnStmt       *aMetaStmt,
                              acp_uint32_t   aKeyDataType,
                              ulsdValueInfo *aValue1,
                              ulsdValueInfo *aValue2,
                              acp_bool_t    *aIsSame,
                              acp_uint16_t   aFuncId)
{
    ulnDescRec    * sDescRecIpd;
    ulnDescRec    * sDescRecApd;

    ulsdKeyData     sShardKeyData;
    mtdModule     * sShardKeyModule;

    ulsdValueInfo * sValue[2];
    mtdValueInfo    sCompareValue[2];

    ulnFnContext    sFnContext;

    acp_uint16_t    i;

    sValue[0] = aValue1;
    sValue[1] = aValue2;

    for ( i = 0; i < 2; i++ )
    {
        sCompareValue[i].column = NULL;
        sCompareValue[i].flag   = MTD_OFFSET_USELESS;

        if ( sValue[i]->mType == 0 ) // bind param
        {
            sDescRecIpd = ulnStmtGetIpdRec(aMetaStmt, sValue[i]->mValue.mBindParamId + 1);
            sDescRecApd = ulnStmtGetApdRec(aMetaStmt, sValue[i]->mValue.mBindParamId + 1);

            ACI_TEST( ulsdGetShardKeyMtdModule(aMetaStmt,
                                               &sShardKeyModule,
                                               aKeyDataType,
                                               sDescRecIpd,
                                               aFuncId ) != SQL_SUCCESS );

            ACI_TEST(ulsdGetParamData(aMetaStmt,
                                      sDescRecApd,
                                      sDescRecIpd,
                                      &sShardKeyData,
                                      sShardKeyModule,
                                      aFuncId) != SQL_SUCCESS);

            sCompareValue[i].value = sShardKeyData.mValue;
        }
        else if ( sValue[i]->mType == 1 ) // constant value
        {
            ACI_TEST( ulsdMtdModuleById( aMetaStmt,
                                         &sShardKeyModule,
                                         aKeyDataType,
                                         aFuncId )
                      != SQL_SUCCESS );

            sCompareValue[i].value = sValue[i]->mValue.mMax;
        }
        else
        {
            ACI_RAISE(LABEL_INVALID_SHARD_VALUE);
        }
    }

    if ( sShardKeyModule->logicalCompare( &sCompareValue[0],
                                          &sCompareValue[1] ) == 0 )
    {
        *aIsSame = ACP_TRUE;
    }
    else
    {
        *aIsSame = ACP_FALSE;
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_SHARD_VALUE)
    {
        ULN_INIT_FUNCTION_CONTEXT(sFnContext, aFuncId, aMetaStmt, ULN_OBJ_TYPE_STMT);

        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "Shard Executor",
                 "Invalid shard value");
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulsdCreateNodeStmt(ulnDbc        *aDbc,
                             ulnStmt       *aMetaStmt)
{
    ulnFnContext    sFnContext;
    SQLRETURN       sNodeReturnCode;
    ulsdDbc        *sShard;
    acp_uint16_t    sCnt;

    ulnStmt       **sShardNodeStmt = NULL;
    
    acp_uint16_t    sAllocHandleCnt = 0;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_ALLOCHANDLE, aDbc, ULN_OBJ_TYPE_DBC );

    ulsdGetShardFromDbc(aDbc, &sShard);

    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS(acpMemAlloc((void **)&sShardNodeStmt,
                                                   ACI_SIZEOF(ulnStmt *) * sShard->mNodeCount) ),
                    LABEL_SHARD_NODE_ALLOC_STMT_MEM_ERROR );

    for (sCnt = 0;sCnt < sShard->mNodeCount;sCnt++)
    {
        sNodeReturnCode = ulnAllocHandle( SQL_HANDLE_STMT,
                                          sShard->mNodeInfo[sCnt]->mNodeDbc,
                                          (void **)&sShardNodeStmt[sCnt] );
        ACI_TEST_RAISE(!SQL_SUCCEEDED(sNodeReturnCode), LABEL_SHARD_NODE_ALLOC_STMT_FAIL);
        sAllocHandleCnt++;

        ulsdInitalizeNodeStmt( aMetaStmt, sShardNodeStmt[sCnt] );

        SHARD_LOG("(Alloc Stmt Handle) NodeId=%d, Server=%s:%d\n",
                  sShard->mNodeInfo[sCnt]->mNodeId,
                  sShard->mNodeInfo[sCnt]->mServerIP,
                  sShard->mNodeInfo[sCnt]->mPortNo);
    }

    aMetaStmt->mShardStmtCxt.mShardNodeStmt = sShardNodeStmt;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_SHARD_NODE_ALLOC_STMT_MEM_ERROR)
    {
        ulnError(&sFnContext,
                 ulERR_ABORT_SHARD_ERROR,
                 "AllocHandleStmt",
                 "Alloc node stmt memory error");
    }
    ACI_EXCEPTION(LABEL_SHARD_NODE_ALLOC_STMT_FAIL)
    {
        ulsdNativeErrorToUlnError(&sFnContext,
                                  SQL_HANDLE_DBC,
                                  (ulnObject *)sShard->mNodeInfo[sCnt]->mNodeDbc,
                                  sShard->mNodeInfo[sCnt],
                                  (acp_char_t *)"AllocStmtHandle");
    }
    ACI_EXCEPTION_END;

    for ( sCnt = 0; sCnt < sAllocHandleCnt; sCnt++ )
    {
        ulnFreeHandle( SQL_HANDLE_STMT, &sShardNodeStmt[sCnt] );
    }

    if ( sShardNodeStmt != NULL )
    {
        acpMemFree( sShardNodeStmt );
        sShardNodeStmt = NULL;
    }

    return SQL_ERROR;
}

void ulsdInitalizeNodeStmt(ulnStmt    *aMetaStmt,
                           ulnStmt    *aNodeStmt)
{
    /* PROJ-2739 Client-side Sharding LOB */
    aNodeStmt->mShardStmtCxt.mParentStmt = aMetaStmt;

    ulsdSetStmtShardModule(aNodeStmt, &gShardModuleNODE);
}

void ulsdNodeStmtDestroy(ulnStmt *aStmt)
{
    acp_list_node_t * sNode = NULL;
    acp_list_node_t * sNext = NULL;

    if ( aStmt->mShardStmtCxt.mShardNodeStmt != NULL )
    {
        acpMemFree(aStmt->mShardStmtCxt.mShardNodeStmt);
        aStmt->mShardStmtCxt.mShardNodeStmt = NULL;
    }
    else
    {
        /* Do Nothing */
    }

    if ( aStmt->mShardStmtCxt.mShardRangeInfo != NULL )
    {
        acpMemFree(aStmt->mShardStmtCxt.mShardRangeInfo);
        aStmt->mShardStmtCxt.mShardRangeInfo = NULL;
    }
    else
    {
        /* Do Nothing */
    }

    /* BUG-46100 Session SMN Update */
    if ( aStmt->mShardStmtCxt.mOrgPrepareTextBuf != NULL )
    {
        acpMemFree( aStmt->mShardStmtCxt.mOrgPrepareTextBuf );

        aStmt->mShardStmtCxt.mOrgPrepareTextBuf       = NULL;
        aStmt->mShardStmtCxt.mOrgPrepareTextBufMaxLen = 0;
        aStmt->mShardStmtCxt.mOrgPrepareTextBufLen    = 0;
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACP_LIST_ITERATE_SAFE( & aStmt->mShardStmtCxt.mStmtAttrList, sNode, sNext )
    {
        acpListDeleteNode( sNode );
        acpMemFree( sNode->mObj );
    }

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACP_LIST_ITERATE_SAFE( & aStmt->mShardStmtCxt.mBindParameterList, sNode, sNext )
    {
        acpListDeleteNode( sNode );
        acpMemFree( sNode->mObj );
    }

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    ACP_LIST_ITERATE_SAFE( & aStmt->mShardStmtCxt.mBindColList, sNode, sNext )
    {
        acpListDeleteNode( sNode );
        acpMemFree( sNode->mObj );
    }

    /* TASK-7219 Non-shard DML */
    aStmt->mShardStmtCxt.mPartialExecType = ULN_SHARD_PARTIAL_EXEC_TYPE_NONE;
}

SQLRETURN ulsdNodeStmtEnsureAllocOrgPrepareTextBuf( ulnFnContext * aFnContext,
                                                    ulnStmt      * aStmt,
                                                    acp_char_t   * aStatementText,
                                                    acp_sint32_t   aTextLength )
{
    acp_sint32_t   sBufLen = 0;

    if ( aTextLength == SQL_NTS )
    {
        sBufLen = acpCStrLen( aStatementText, ACP_SINT32_MAX );
    }
    else
    {
        sBufLen = aTextLength;
    }

    if ( aStmt->mShardStmtCxt.mOrgPrepareTextBufMaxLen < sBufLen )
    {
        if ( aStmt->mShardStmtCxt.mOrgPrepareTextBuf != NULL )
        {
            acpMemFree( aStmt->mShardStmtCxt.mOrgPrepareTextBuf );
        }
        else
        {
            /* Nothing to do */
        }

        ACI_TEST_RAISE( acpMemAlloc( (void **) & aStmt->mShardStmtCxt.mOrgPrepareTextBuf,
                                     sBufLen )
                        != ACP_RC_SUCCESS, LABEL_NOT_ENOUGH_MEMORY );

        aStmt->mShardStmtCxt.mOrgPrepareTextBufMaxLen = sBufLen;
    }
    else
    {
        /* Nothing to do */
    }

    aStmt->mShardStmtCxt.mOrgPrepareTextBufLen = sBufLen;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "NodeStmtEnsureAllocOrgPrepareTextBuf",
                  "Memory allocation error." );
    }
    ACI_EXCEPTION_END;

    aStmt->mShardStmtCxt.mOrgPrepareTextBuf       = NULL;
    aStmt->mShardStmtCxt.mOrgPrepareTextBufMaxLen = 0;
    aStmt->mShardStmtCxt.mOrgPrepareTextBufLen    = 0;

    return SQL_ERROR;
}

SQLRETURN ulsdAddNodeToStmt( ulnFnContext * aFnContext,
                             ulnStmt      * aMetaStmt,
                             ulsdNodeInfo * aNewNodeInfo )
{
    ulsdDbc           * sShard              = NULL;
    ulnStmt          ** sNewShardNodeStmt   = NULL;
    ulnStmt           * sNewStmt            = NULL;
    SQLRETURN           sRet                = SQL_ERROR;
    acp_uint16_t        i                   = 0;

    ulsdGetShardFromDbc( aMetaStmt->mParentDbc, &sShard );

    ACI_TEST_RAISE( ACP_RC_NOT_SUCCESS( acpMemCalloc( (void **) & sNewShardNodeStmt,
                                                      sShard->mNodeCount + 1,
                                                      ACI_SIZEOF(ulnStmt *) ) ),
                    LABEL_SHARD_NODE_ALLOC_STMT_MEM_ERROR );

    for ( i = 0; i < sShard->mNodeCount; i++ )
    {
        sNewShardNodeStmt[i] = aMetaStmt->mShardStmtCxt.mShardNodeStmt[i];
    }

    sRet = ulnAllocHandle( SQL_HANDLE_STMT,
                           aNewNodeInfo->mNodeDbc,
                           (void **) & sNewShardNodeStmt[i] );
    ACI_TEST_RAISE( !SQL_SUCCEEDED(sRet), LABEL_SHARD_NODE_ALLOC_STMT_FAIL );
    sNewStmt = sNewShardNodeStmt[i];

    ulsdInitalizeNodeStmt( aMetaStmt, sNewStmt );

    sRet = ulsdSetStmtAttrOnNode( aFnContext,
                                  & aMetaStmt->mShardStmtCxt,
                                  sNewStmt,
                                  aNewNodeInfo );
    ACI_TEST( sRet != SQL_SUCCESS );

    sRet = ulsdNodeBindParameterOnNode( aFnContext,
                                        & aMetaStmt->mShardStmtCxt,
                                        sNewStmt,
                                        aNewNodeInfo );
    ACI_TEST( sRet != SQL_SUCCESS );

    sRet = ulsdNodeBindColOnNode( aFnContext,
                                  & aMetaStmt->mShardStmtCxt,
                                  sNewStmt,
                                  aNewNodeInfo );
    ACI_TEST( sRet != SQL_SUCCESS );

    acpMemFree( (void *)aMetaStmt->mShardStmtCxt.mShardNodeStmt );
    aMetaStmt->mShardStmtCxt.mShardNodeStmt = sNewShardNodeStmt;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_SHARD_NODE_ALLOC_STMT_MEM_ERROR )
    {
        ulnError( aFnContext,
                  ulERR_ABORT_SHARD_ERROR,
                  "AddNodeToStmt",
                  "Alloc node stmt memory error" );

        sRet = ULN_FNCONTEXT_GET_RC( aFnContext );
    }
    ACI_EXCEPTION( LABEL_SHARD_NODE_ALLOC_STMT_FAIL )
    {
        ulsdNativeErrorToUlnError( aFnContext,
                                   SQL_HANDLE_DBC,
                                   (ulnObject *)aNewNodeInfo->mNodeDbc,
                                   aNewNodeInfo,
                                   (acp_char_t *)"AllocStmtHandle" );
    }
    ACI_EXCEPTION_END;

    if ( sNewStmt != NULL )
    {
        (void)ulnFreeHandle( SQL_HANDLE_STMT, (void *)sNewStmt );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sNewShardNodeStmt != NULL )
    {
        acpMemFree( (void *)sNewShardNodeStmt );
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;
}

void ulsdRemoveNodeFromStmt( ulnStmt      * aMetaStmt,
                             ulsdNodeInfo * aRemoveNodeInfo )
{
    ulsdDbc           * sShard        = NULL;
    acp_uint16_t        sTarget       = 0;
    acp_uint16_t        sNodeDbcIndex = 0;
    acp_uint16_t        i             = 0;

    /* Disconnect 시에 Statement Free를 수행하므로, 여기에서는 Mapping 정보만 정리한다.
     *  struct ulsdStmtContext
     *  {
     *      ulnStmt      ** mShardNodeStmt;
     *  ...
     *      acp_uint16_t    mNodeDbcIndexArr[ULSD_SD_NODE_MAX_COUNT];
     *      acp_uint16_t    mNodeDbcIndexCount;
     *      acp_sint16_t    mNodeDbcIndexCur;   // 현재 fetch중인 dbc index
     *  ...
     *  }
     */

    ulsdGetShardFromDbc( aMetaStmt->mParentDbc, &sShard );

    for ( sTarget = 0; sTarget < sShard->mNodeCount; sTarget++ )
    {
        if ( sShard->mNodeInfo[sTarget]->mNodeId == aRemoveNodeInfo->mNodeId )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    ACE_DASSERT( sTarget < sShard->mNodeCount );

    for ( i = sTarget; i < (sShard->mNodeCount - 1); i++ )
    {
        aMetaStmt->mShardStmtCxt.mShardNodeStmt[i] = aMetaStmt->mShardStmtCxt.mShardNodeStmt[i + 1];
    }

    for ( sNodeDbcIndex = 0; sNodeDbcIndex < aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount; sNodeDbcIndex++ )
    {
        if ( aMetaStmt->mShardStmtCxt.mNodeDbcIndexArr[sNodeDbcIndex] == sTarget )
        {
            for ( i = sNodeDbcIndex; i < (aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount - 1); i++ )
            {
                aMetaStmt->mShardStmtCxt.mNodeDbcIndexArr[i] = aMetaStmt->mShardStmtCxt.mNodeDbcIndexArr[i + 1];
            }

            aMetaStmt->mShardStmtCxt.mNodeDbcIndexCount--;

            /* Fetch 시에 SQL_NO_DATA를 반환하는 상태이므로, mNodeDbcIndexCur를 감소시키지 않는다. */
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return;
}
