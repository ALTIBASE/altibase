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

#ifndef _O_ULSD_MAIN_H_
#define _O_ULSD_MAIN_H_ 1

#include <ulsdDef.h>
#include <ulsdError.h>
#include <ulsdDriverConnect.h>

#define SHARD_DEBUG 0
#define SHARD_PRINT_TAG "[SHARD] "
#define SHARD_LOG(...) \
    do { if (SHARD_DEBUG) acpFprintf( acpStdGetStderr(), SHARD_PRINT_TAG __VA_ARGS__); } while (0)

void ulsdSetNodeInfo( ulsdNodeInfo * aShardNodeInfo,
                      acp_uint32_t   aNodeId,
                      acp_char_t   * aNodeName,
                      acp_char_t   * aServerIP,
                      acp_uint16_t   aPortNo,
                      acp_char_t   * aAlternateServerIP,
                      acp_uint16_t   aAlternatePortNo );

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
SQLRETURN ulsdAddNode( ulnFnContext * aFnContext,
                       ulnDbc       * aMetaDbc,
                       ulsdNodeInfo * aNewNodeInfo );

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
SQLRETURN ulsdAddNodeToDbc( ulnFnContext * aFnContext,
                            ulnDbc       * aMetaDbc,
                            ulsdNodeInfo * aNewNodeInfo );

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
SQLRETURN ulsdAddNodeToStmt( ulnFnContext * aFnContext,
                             ulnStmt      * aMetaStmt,
                             ulsdNodeInfo * aNewNodeInfo );

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
void ulsdRemoveNode( ulnDbc       * aMetaDbc,
                     ulsdNodeInfo * aRemoveNodeInfo );

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
void ulsdRemoveNodeFromDbc( ulnDbc       * aMetaDbc,
                            ulsdNodeInfo * aRemoveNodeInfo );

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
void ulsdRemoveNodeFromStmt( ulnStmt      * aMetaStmt,
                             ulsdNodeInfo * aRemoveNodeInfo );

SQLRETURN ulsdShardCreate(ulnDbc    *aDbc);

void ulsdShardDestroy(ulnDbc   *aDbc);

/* BUG-46092 */
ACI_RC ulsdReallocAlignInfo( ulnFnContext  * aFnContext,
                             ulsdAlignInfo * aAlignInfo,
                             acp_sint32_t    aLen );
void ulsdAlignInfoInitialize( ulnDbc *aDbc );
void ulsdAlignInfoFinalize( ulnDbc *aDbc );

SQLRETURN ulsdNodeInfoFree(ulsdNodeInfo *aShardNodeInfo);

void ulsdGetShardFromDbc(ulnDbc        *aDbc,
                         ulsdDbc      **aShard);

/* BUG-47553 */
ACI_RC ulsdEnter( ulnFnContext *aFnContext );

SQLRETURN ulsdNodeFreeHandleStmt(ulsdDbc      *aShard,
                                 ulnStmt      *aMetaStmt);

SQLRETURN ulsdNodeFreeStmt(ulnStmt      *aMetaStmt,
                           acp_uint16_t aOption);

SQLRETURN ulsdCreateNodeStmt(ulnDbc    *aDbc,
                             ulnStmt   *aMetaStmt);

void ulsdInitalizeNodeStmt(ulnStmt    *aMetaStmt,
                           ulnStmt    *aNodeStmt);

void ulsdNodeStmtDestroy(ulnStmt *aStmt);

SQLRETURN ulsdNodeStmtEnsureAllocOrgPrepareTextBuf( ulnFnContext * aFnContext,
                                                    ulnStmt      * aStmt,
                                                    acp_char_t   * aStatementText,
                                                    acp_sint32_t   aTextLength );

SQLRETURN ulsdNodeSilentFreeStmt(ulsdDbc      *aShard,
                                 ulnStmt      *aMetaStmt);

SQLRETURN ulsdNodeDecideStmt(ulnStmt       *aMetaStmt,
                             acp_uint16_t   aFuncId);

SQLRETURN ulsdNodeDriverConnect(ulnDbc       *aMetaDbc,
                                ulnFnContext *aFnContext,
                                acp_char_t   *aConnString,
                                acp_sint16_t  aConnStringLength);

SQLRETURN ulsdDriverConnectToNode(ulnDbc       *aMetaDbc,
                                  ulnFnContext *aFnContext,
                                  acp_char_t   *aConnString,
                                  acp_sint16_t  aConnStringLength);

SQLRETURN ulsdDriverConnectToNodeInternal( ulnFnContext * aFnContext,
                                           ulsdNodeInfo * aNodeInfo,
                                           acp_char_t   * aConnString );

/* BUG-47327 */
SQLRETURN ulsdConnect( ulnDbc       * aDbc,
                       acp_char_t   * aServerName,
                       acp_sint16_t   aServerNameLength,
                       acp_char_t   * aUserName,
                       acp_sint16_t   aUserNameLength,
                       acp_char_t   * aPassword,
                       acp_sint16_t   aPasswordLength );

SQLRETURN ulsdNodeConnect( ulnDbc       * aMetaDbc,
                           ulnFnContext * aFnContext,
                           acp_char_t   * aServerName,
                           acp_sint16_t   aServerNameLength,
                           acp_char_t   * aUserName,
                           acp_sint16_t   aUserNameLength,
                           acp_char_t   * aPassword,
                           acp_sint16_t   aPasswordLength );

SQLRETURN ulsdConnectToNode( ulnDbc       * aMetaDbc,
                             ulnFnContext * aFnContext,
                             acp_char_t   * aServerName,
                             acp_sint16_t   aServerNameLength,
                             acp_char_t   * aUserName,
                             acp_sint16_t   aUserNameLength,
                             acp_char_t   * aPassword,
                             acp_sint16_t   aPasswordLength );

SQLRETURN ulsdConnectToNodeInternal( ulnFnContext * aFnContext,
                                     ulsdNodeInfo * aNodeInfo,
                                     acp_char_t   * aServerName,
                                     acp_sint16_t   aServerNameLength,
                                     acp_char_t   * aUserName,
                                     acp_sint16_t   aUserNameLength,
                                     acp_char_t   * aPassword,
                                     acp_sint16_t   aPasswordLength );

SQLRETURN ulsdAllocHandleNodeDbc(ulnFnContext  *aFnContext,
                                 ulnEnv        *aEnv,
                                 ulnDbc       **aDbc);

void ulsdInitalizeNodeDbc(ulnDbc      *aNodeDbc,
                          ulnDbc      *aMetaDbc);

SQLRETURN ulsdDisconnect(ulnDbc *aDbc);

void ulsdSilentDisconnect(ulnDbc *aDbc);

SQLRETURN ulsdNodeDisconnect(ulsdDbc    *aShard);

SQLRETURN ulsdNodeSilentDisconnect(ulsdDbc    *aShard);

void ulsdNodeFree(ulsdDbc    *aShard);

SQLRETURN ulsdNodeDecideStmtByValues(ulnStmt        *aMetaStmt,
                                     acp_uint16_t    aFuncId);

/* PROJ-2655 Composite shard key */
SQLRETURN ulsdGetRangeIndexByValues(ulnStmt        *aMetaStmt,
                                    acp_uint16_t    aValueIndex,
                                    ulsdValueInfo  *aValue,
                                    ulsdRangeIndex *aRangeIndex,
                                    acp_uint16_t   *aRangeIndexCount,
                                    acp_bool_t     *aHasDefaultNode,
                                    acp_bool_t      aIsSubKey,
                                    acp_uint16_t    aFuncId);

SQLRETURN ulsdCheckValuesSame(ulnStmt       *aMetaStmt,
                              acp_uint32_t   aKeyDataType,
                              ulsdValueInfo *aValue1,
                              ulsdValueInfo *aValue2,
                              acp_bool_t    *aIsSame,
                              acp_uint16_t   aFuncId);

SQLRETURN ulsdGetParamData(ulnStmt          *aStmt,
                           ulnDescRec       *aDescRecApd,
                           ulnDescRec       *aDescRecIpd,
                           ulsdKeyData      *aShardKeyData,
                           mtdModule        *aShardKeyModule,
                           acp_uint16_t      aFuncId);

SQLRETURN ulsdConvertParamData(ulnStmt          *aMetaStmt,
                               ulnDescRec       *aDescRecApd,
                               ulnDescRec       *aDescRecIpd,
                               void             *aUserDataPtr,
                               ulsdKeyData      *aShardKeyData,
                               acp_uint16_t      aFuncId);

SQLRETURN ulsdGetRangeIndexFromHash(ulnStmt        *aMetaStmt,
                                    acp_uint16_t    aValueIndex,
                                    acp_uint32_t    aHashValue,
                                    ulsdRangeIndex *aRangeIndex,
                                    acp_uint16_t   *aRangeIndexCount,
                                    acp_bool_t     *aHasDefaultNode,
                                    acp_bool_t      aIsSubKey,
                                    acp_uint16_t    aFuncId );

SQLRETURN ulsdGetRangeIndexFromClone(ulnStmt      *aMetaStmt,
                                     acp_uint16_t *aRangeIndex);


SQLRETURN ulsdGetRangeIndexFromRange(ulnStmt        *aMetaStmt,
                                     acp_uint16_t    aValueIndex,
                                     ulsdKeyData    *aShardKeyData,
                                     mtdModule      *aShardKeyModule,
                                     ulsdRangeIndex *aRangeIndex,
                                     acp_uint16_t   *aRangeIndexCount,
                                     acp_bool_t     *aHasDefaultNode,
                                     acp_bool_t      aIsSubKey,
                                     acp_uint16_t    aFuncId);

SQLRETURN ulsdGetRangeIndexFromList(ulnStmt        *aMetaStmt,
                                    acp_uint16_t    aValueIndex,
                                    ulsdKeyData    *aShardKeyData,
                                    mtdModule      *aShardKeyModule,
                                    ulsdRangeIndex *aRangeIndex,
                                    acp_uint16_t   *aRangeIndexCount,
                                    acp_bool_t     *aHasDefaultNode,
                                    acp_bool_t      aIsSubKey,
                                    acp_uint16_t    aFuncId);

void ulsdReadMtValue(cmiProtocolContext *aProtocolContext,
                     acp_uint32_t        aKeyDataType,
                     ulsdValue          *aRangeValue);

// BUG-47129
void ulsdSkipReadMtValue(cmiProtocolContext * aProtocolContext,
                         acp_uint32_t         aKeyDataType );

SQLRETURN ulsdNodeBindParameter( ulnStmt      *aStmt,
                                 acp_uint16_t  aParamNumber,
                                 acp_char_t   *aParamName,
                                 acp_sint16_t  aInputOutputType,
                                 acp_sint16_t  aValueType,
                                 acp_sint16_t  aParamType,
                                 ulvULen       aColumnSize,
                                 acp_sint16_t  aDecimalDigits,
                                 void         *aParamValuePtr,
                                 ulvSLen       aBufferLength,
                                 ulvSLen      *aStrLenOrIndPtr,
                                 ulvSLen      *aFileNameLengthArray,
                                 acp_uint32_t *aFileOptionPtr );

SQLRETURN ulsdNodeBindParameterOnNode( ulnFnContext    * aFnContext,
                                       ulsdStmtContext * aMetaShardStmtCxt,
                                       ulnStmt         * aDataStmt,
                                       ulsdNodeInfo    * aNodeInfo );

SQLRETURN ulsdGetShardKeyMtdModule(ulnStmt      *aMetaStmt,
                                   mtdModule   **aModule,
                                   acp_uint32_t  aKeyDataType,
                                   ulnDescRec   *aDescRecIpd,
                                   acp_uint16_t  aFuncId);

SQLRETURN ulsdMtdModuleById(ulnStmt       *aMetaStmt,
                            mtdModule    **aModule,
                            acp_uint32_t   aId,
                            acp_uint16_t   aFuncId);

SQLRETURN ulsdConvertNodeIdToNodeDbcIndex(ulnStmt          *aMetaStmt,
                                          acp_uint32_t      aNodeId,
                                          acp_uint16_t     *aNodeDbcIndex,
                                          acp_uint16_t      aFuncId);

SQLRETURN ulsdNodeBindCol( ulnStmt      *aStmt,
                           acp_uint16_t  aColumnNumber,
                           acp_sint16_t  aTargetType,
                           void         *aTargetValuePtr,
                           ulvSLen       aBufferLength,
                           ulvSLen      *aStrLenOrIndPtr,
                           ulvSLen      *aFileNameLengthArray,
                           acp_uint32_t *aFileOptionPtr );

SQLRETURN ulsdNodeBindColOnNode( ulnFnContext    * aFnContext,
                                 ulsdStmtContext * aMetaShardStmtCxt,
                                 ulnStmt         * aDataStmt,
                                 ulsdNodeInfo    * aNodeInfo );

SQLRETURN ulsdSetConnectAttr(ulnDbc       *aMetaDbc,
                             acp_sint32_t  aAttribute,
                             void         *aValuePtr,
                             acp_sint32_t  aStringLength);

SQLRETURN ulsdSetConnectAttrOnNode( ulnFnContext   * aFnContext,
                                    ulsdDbcContext * aMetaShardDbcCxt,
                                    ulsdNodeInfo   * aNodeInfo );

acp_bool_t ulsdIsSetConnectAttr( ulnFnContext     * aFnContext, 
                                 ulnConnAttrID      aConnAttrID );

SQLRETURN ulsdSetStmtAttr(ulnStmt      *aMetaStmt,
                          acp_sint32_t  aAttribute,
                          void         *aValuePtr,
                          acp_sint32_t  aStringLength);

SQLRETURN ulsdSetStmtAttrOnNode( ulnFnContext    * aFnContext,
                                 ulsdStmtContext * aMetaShardStmtCxt,
                                 ulnStmt         * aDataStmt,
                                 ulsdNodeInfo    * aNodeInfo );

ACI_RC ulsdPrepareResultCallback(cmiProtocolContext *aProtocolContext,
                                 cmiProtocol        *aProtocol,
                                 void               *aServiceSession,
                                 void               *aUserContext);

SQLRETURN ulsdAnalyze(ulnFnContext *aFnContext,
                      ulnStmt      *aStmt,
                      acp_char_t   *aStatementText,
                      acp_sint32_t  aTextLength);

void ulsdSetCoordQuery(ulnStmt  *aStmt);

SQLRETURN ulsdAnalyzePrepareAndRetry( ulnStmt      *aStmt,
                                      acp_char_t   *aStatementText,
                                      acp_sint32_t  aTextLength,
                                      acp_char_t   *aAnalyzeText );

SQLRETURN ulsdPrepare(ulnFnContext *aFnContext,
                      ulnStmt      *aStmt,
                      acp_char_t   *aStatementText,
                      acp_sint32_t  aTextLength,
                      acp_char_t   *aAnalyzeText);

SQLRETURN ulsdPrepareNodes(ulnFnContext *aFnContext,
                           ulnStmt      *aStmt,
                           acp_char_t   *aStatementText,
                           acp_sint32_t  aTextLength,
                           acp_char_t   *aAnalyzeText);

SQLRETURN ulsdExecuteNodes(ulnFnContext *aFnContext,
                           ulnStmt      *aStmt);

SQLRETURN ulsdFetchNodes(ulnFnContext *aFnContext,
                         ulnStmt      *aStmt);

/* BUG-47433 */
SQLRETURN ulsdGetData( ulnStmt      * aStmt,
                       acp_uint16_t   aColumnNumber,
                       acp_sint16_t   aTargetType,
                       void         * aTargetValue,
                       ulvSLen        aBufferLength,
                       ulvSLen      * aStrLenOrInd );

SQLRETURN ulsdRowCountNodes(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            ulvSLen      *aRowCount);

SQLRETURN ulsdMoreResultsNodes(ulnFnContext *aFnContext,
                               ulnStmt      *aStmt);

acp_bool_t ulsdStmtHasNoDataOnNodes( ulnStmt * aMetaStmt );

acp_bool_t ulsdHasNoData( ulnDbc * aMetaDbc );

/* BUG-46100 Session SMN Update */
SQLRETURN ulsdUpdateShardMetaNumber( ulnDbc       * aMetaDbc,
                                     ulnFnContext * aFnContext );

/* BUG-46100 Session SMN Update */
SQLRETURN ulsdSetConnectAttrNode( ulnDbc        * aDbc,
                                  ulnPropertyId   aCmPropertyID,
                                  void          * aValuePtr );

SQLRETURN ulsdCloseCursor(ulnStmt *aStmt);

SQLRETURN ulsdNodeSilentCloseCursor(ulsdDbc *aShard,
                                    ulnStmt *aStmt);

SQLRETURN ulsdNodeCloseCursor( ulnStmt * aStmt );

SQLRETURN ulsdFetch(ulnStmt *aStmt);

SQLRETURN ulsdRowCount(ulnStmt *aStmt,
                       ulvSLen *aRowCountPtr);

SQLRETURN ulsdMoreResults(ulnStmt *aStmt);

void ulsdNativeErrorToUlnError(ulnFnContext       *aFnContext,
                               acp_sint16_t        aHandleType,
                               ulnObject          *aErrorRaiseObject,
                               ulsdNodeInfo       *aNodeInfo,
                               acp_char_t         *aOperation);

/* BUG-46092 */
void ulsdMoveNodeDiagRec( ulnFnContext * aFnContext,
                          ulnObject    * aObjectTo,
                          ulnObject    * aObjectFrom,
                          acp_uint32_t   aNodeId,
                          acp_char_t   * aNodeString,
                          acp_char_t   * aOperation );

void ulsdMakeErrorMessage(acp_char_t         *aOutputErrorMessage,
                          acp_uint16_t        aOutputErrorMessageLength,
                          acp_char_t         *aOriginalErrorMessage,
                          acp_char_t         *aNodeString,
                          acp_char_t         *aOperation );

acp_bool_t ulsdNodeFailConnectionLost(acp_sint16_t  aHandleType,
                                      ulnObject    *aObject);

acp_bool_t ulsdNodeInvalidTouch(acp_sint16_t  aHandleType,
                                ulnObject    *aObject);

void ulsdMakeNodeAlternateServersString(ulsdNodeInfo      *aShardNodeInfo,
                                        acp_char_t        *aAlternateServers,
                                        acp_sint32_t       aMaxAlternateServersLen);

ACI_RC ulsdSetConnAttrForLibConn( ulnFnContext  * aFnContext );

SQLRETURN ulsdDriverConnect(ulnDbc       *aDbc,
                            acp_char_t   *aConnString,
                            acp_sint16_t  aConnStringLength,
                            acp_char_t   *aOutConnStr,
                            acp_sint16_t  aOutBufferLength,
                            acp_sint16_t *aOutConnectionStringLength);

void ulsdClearOnTransactionNode(ulnDbc *aMetaDbc);

void ulsdSetOnTransactionNodeIndex(ulnDbc       *aMetaDbc,
                                   acp_uint16_t  aNodeIndex);

void ulsdGetOnTransactionNodeIndex(ulnDbc       *aMetaDbc,
                                   acp_uint16_t *aNodeIndex);

void ulsdIsValidOnTransactionNodeIndex(ulnDbc       *aMetaDbc,
                                       acp_uint16_t  aDecidedNodeIndex,
                                       acp_bool_t   *aIsValid);

SQLRETURN ulsdEndTranDbc(acp_sint16_t  aHandleType,
                         ulnDbc       *aMetaDbc,
                         acp_sint16_t  aCompletionType);

SQLRETURN ulsdShardEndTranDbc( ulnFnContext * aFnContext,
                               ulsdDbc      * aShard );

SQLRETURN ulsdnSimpleShardEndTranDbc( ulnDbc *aDbc, ulnTransactionOp aCompletionType );

SQLRETURN ulsdTouchNodeEndTranDbc(ulnFnContext  *aFnContext,
                                  ulsdDbc       *aShard,
                                  acp_sint16_t   aCompletionType);

SQLRETURN ulsdTouchNodeShardEndTranDbc(ulnFnContext      *aFnContext,
                                       ulsdDbc           *aShard,
                                       acp_sint16_t       aCompletionType);

SQLRETURN ulsdNodeEndTranDbc(ulnFnContext  *aFnContext,
                             ulsdDbc       *aShard,
                             acp_sint16_t   aCompletionType);

void ulsdGetTouchNodeCount(ulsdDbc       *aShard,
                           acp_sint16_t  *aTouchNodeCount);

ACI_RC ulsdUpdateNodeList(ulnFnContext *aFnContext, ulnPtContext *aPtContext);

ACI_RC ulsdGetNodeList(ulnDbc *aDbc,
                       ulnFnContext *aFnContext,
                       ulnPtContext *aPtContext);

ACI_RC ulsdHandshake(ulnFnContext *aFnContext);

ACI_RC ulsdUpdateNodeListRequest(ulnFnContext *aFnContext, ulnPtContext *aPtContext);

ACI_RC ulsdGetNodeListRequest(ulnFnContext *aFnContext, ulnPtContext *aPtContext);

ACI_RC ulsdAnalyzeRequest(ulnFnContext  *aFnContext,
                          ulnPtContext  *aProtocolContext,
                          acp_char_t    *aString,
                          acp_sint32_t   aLength);

ACI_RC ulsdShardTransactionCommitRequest( ulnFnContext     * aFnContext,
                                          ulnPtContext     * aPtContext,
                                          ulnTransactionOp   aTransactOp,
                                          acp_uint32_t     * aTouchNodeArr,
                                          acp_uint16_t       aTouchNodeCount);

ACI_RC ulsdInitializeDBProtocolCallbackFunctions(void);

ACI_RC ulsdFODoSTF(ulnFnContext     *aFnContext,
                   ulnDbc           *aDbc,
                   ulnErrorMgr      *aErrorMgr);

ACI_RC ulsdFODoReconnect( ulnFnContext     *aFnContext,
                          ulnDbc           *aDbc,
                          ulnErrorMgr      *aErrorMgr );

ACP_INLINE void ulsdFnContextSetHandle( ulnFnContext * aFnContext,
                                        ulnObjType     aObjType,
                                        void         * aObject )
{
    switch ( aObjType )
    {
        case ULN_OBJ_TYPE_DBC:
            aFnContext->mHandle.mDbc = (ulnDbc *) aObject;
            break;
        case ULN_OBJ_TYPE_STMT:
            aFnContext->mHandle.mStmt = (ulnStmt *) aObject;
            break;
        default:
            break;
    }
}

void ulsdGetTouchedAllNodeList(ulsdDbc      *aShard,
                               acp_uint32_t *aNodeArr,
                               acp_uint16_t *aNodeCount);

void ulsdSetTouchedToAllNodes( ulsdDbc * aShard );

ACP_INLINE void ulsdDbcSetShardCli( ulnDbc *aDbc, acp_uint8_t aShardClient )
{
    aDbc->mShardDbcCxt.mShardClient = aShardClient;
}

ACP_INLINE void ulsdDbcSetShardSessionType( ulnDbc * aDbc, acp_uint8_t  aShardSessionType )
{
    aDbc->mShardDbcCxt.mShardSessionType = aShardSessionType;
}

void ulsdProcessExecuteError( ulnFnContext       * aFnContext,
                              ulnStmt            * aStmt,
                              ulsdFuncCallback   * aCallback );

#endif /* _O_ULSD_MAIN_H_ */
