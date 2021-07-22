/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: qmvShardTransform.h 23857 2008-03-19 02:36:53Z sungminee $
 *
 * Description :
 *     Shard View Transform을 위한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMV_SHARD_TRANSFORM_H_
#define _O_QMV_SHARD_TRANSFORM_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmoDependency.h>
#include <sdi.h>

class qmvShardTransform
{
public:
    static IDE_RC doTransform( qcStatement  * aStatement );

    static IDE_RC raiseInvalidShardQuery( qcStatement  * aStatement );

    /* TASK-7219 Shard Transformer Refactoring */
    static IDE_RC doShardAnalyze( qcStatement    * aStatement,
                                  qcNamePosition * aStmtPos,
                                  ULong            aSMN,
                                  qmsQuerySet    * aQuerySet );

private:

    //------------------------------------------
    // transform shard view의 수행
    //------------------------------------------

    // query block을 순회하며 transform 수행
    static IDE_RC  processTransform( qcStatement  * aStatement );

    // expression을 순회하며 subquery의 transform 수행
    static IDE_RC  processTransformForExpr( qcStatement  * aStatement,
                                            qtcNode      * aExpr );

    //------------------------------------------
    // shard view 생성
    //------------------------------------------

    // for qcStatement
    static IDE_RC  makeShardStatement( qcStatement    * aStatement,
                                       qcNamePosition * aParsePosition,
                                       qcShardStmtType  aShardStmtType,
                                       sdiAnalyzeInfo * aShardAnalysis );

    // for from
    static IDE_RC  makeShardView( qcStatement    * aStatement,
                                  qmsTableRef    * aTableRef,
                                  sdiAnalyzeInfo * aShardAnalysis,
                                  qcNamePosition * aFilter );

    static IDE_RC addColumnListToText( qtcNode        * aNode,
                                       UInt           * aColumnCount,
                                       SChar          * aQueryBuf,
                                       UInt             aQueryBufSize,
                                       qcNamePosition * aQueryPosition );

    static IDE_RC addAggrListToText( qcStatement       * aStatement,
                                     qcParamOffsetInfo * aParamOffsetInfo,
                                     qmsTarget         * aTarget,
                                     qtcNode           * aNode,
                                     UInt              * aAggrCount,
                                     SChar             * aQueryBuf,
                                     UInt                aQueryBufSize,
                                     qcNamePosition    * aQueryPosition,
                                     idBool            * aUnsupportedAggr );

    static IDE_RC addSumMinMaxCountToText( qmsTarget      * aTarget,
                                           qtcNode        * aNode,
                                           SChar          * aQueryBuf,
                                           UInt             aQueryBufSize,
                                           qcNamePosition * aQueryPosition );

    static IDE_RC addAvgToText( qmsTarget      * aTarget,
                                qtcNode        * aNode,
                                SChar          * aQueryBuf,
                                UInt             aQueryBufSize,
                                qcNamePosition * aQueryPosition );

    static IDE_RC modifyOrgAggr( qcStatement  * aStatement,
                                 qtcNode     ** aNode,
                                 UInt         * aViewTargetOrder );

    static IDE_RC changeAggrExpr( qcStatement  * aStatement,
                                  qtcNode     ** aNode,
                                  UInt         * aViewTargetOrder );

    //------------------------------------------
    // PROJ-2701 Sharding online data rebuild
    //------------------------------------------    
    static IDE_RC rebuildTransform( qcStatement * aStatement );

    static IDE_RC checkRebuildTransformable( qcStatement    * aStatement,
                                             sdiAnalyzeInfo * aSessionSMNAnalysis,
                                             idBool           aDataSMNIsShardQuery,
                                             sdiAnalyzeInfo * aDataSMNAnalysis,
                                             idBool         * aIsTransformable );

    static void getNodeIdFromName( sdiNodeInfo * aNodeInfo,
                                   SChar       * aNodeName,
                                   UShort      * aNodeId );

    static void getNodeNameFromId( sdiNodeInfo * aNodeInfo,
                                   UShort        aNodeId,
                                   SChar      ** aNodeName );

    static IDE_RC checkNeedRebuild4ShardQuery( qcStatement     * aStatement,
                                               sdiRebuildInfo  * aRebuildInfo,
                                               idBool          * aRebuild );

    static IDE_RC rebuildNonShardTransform( qcStatement    * aStatement,
                                            sdiRebuildInfo * aRebuildInfo );

    static IDE_RC rebuildTransformQuerySet( qcStatement    * aStatement,
                                            sdiRebuildInfo * aRebuildInfo,
                                            qmsQuerySet    * aQuerySet );

    static IDE_RC rebuildTransformFrom( qcStatement    * aStatement,
                                        sdiRebuildInfo * aRebuildInfo, 
                                        qmsFrom        * aFrom );

    static idBool checkRebuildTransformable4NonShard( sdiObjectInfo * aSessionSMNObj,
                                                      sdiObjectInfo * aDataSMNObj );

    static IDE_RC detectNeedRebuild( UShort                 aMyNodeId,
                                     sdiSplitMethod         aSplitMethod,
                                     UInt                   aKeyDataType,
                                     sdiRangeInfo         * aSessionSMNRangeInfo,
                                     UShort                 aSessionSMNDefaultNodeId,
                                     sdiRangeInfo         * aDataSMNRangeInfo,
                                     UShort                 aDataSMNDefaultNodeId,
                                     idBool               * aIsTransformNeeded );

    static IDE_RC rebuildTransformExpr( qcStatement    * aStatement,
                                        sdiRebuildInfo * aRebuildInfo,
                                        qtcNode        * aExpr );

    /* TASK-7219 */
    static IDE_RC addTargetAliasToText( qmsTarget      * aTarget,
                                        UShort           aTargetPos,
                                        SChar          * aQueryBuf,
                                        UInt             aQueryBufSize,
                                        qcNamePosition * aQueryPosition );

    static IDE_RC checkPsmBindVariable( qcStatement    * aStatement,
                                        qtcNode        * aNode,
                                        idBool           aExistCast,
                                        idBool         * aIsValid );

    static IDE_RC processOrderByTransform( qcStatement  * aStatement,
                                           qmsQuerySet  * aQuerySet,
                                           qmsParseTree * aParseTree,
                                           UInt           aTransType );

    static IDE_RC checkAsteriskTarget( qcStatement * aStatement,
                                       qmsTarget   * aTarget,
                                       qtcNode     * aSortNode,
                                       idBool      * aIsFound );

    static IDE_RC checkEquivalentTarget( qcStatement * aStatement,
                                         qmsTarget   * aTarget,
                                         qtcNode     * aSortNode,
                                         idBool      * aIsFound );

    static IDE_RC makeExplicitPosition( qcStatement * aStatement,
                                        UShort        aTargetPos,
                                        qtcNode    ** aValueNode );

    static IDE_RC makeShardColTransNode( qcStatement * aStatement,
                                         qmsTarget   * aTarget,
                                         UShort        aTargetPos,
                                         qtcNode    ** aColumnNode );

    static IDE_RC appendShardColTrans( qcStatement    * aStatement,
                                       qmsQuerySet    * aQuerySet,
                                       qmsSortColumns * aSort,
                                       UInt             aTransType );

    static IDE_RC checkAppendAbleType( qmsSFWGH * aSFWGH,
                                       qtcNode  * aNode,
                                       UInt       aTransType,
                                       UInt     * aFlag );

    static IDE_RC checkSortNodeTree( qmsSFWGH * aSFWGH,
                                     qtcNode  * aNode,
                                     UInt       aTransType,
                                     idBool     aIsRoor,
                                     UInt     * aFlag );

    static IDE_RC checkTableColumn( qmsSFWGH * aSFWGH,
                                    qtcNode  * aNode,
                                    UInt       aTransType,
                                    UInt     * aFlag );

    static IDE_RC isValidTableName( qmsFrom * aFrom,
                                    qtcNode * aNode,
                                    idBool  * aIsValid );

    static IDE_RC isValidAliasName( qmsSFWGH * aSFWGH,
                                    qtcNode  * aNode,
                                    idBool   * aIsValid,
                                    idBool   * aIsAggr );

    static IDE_RC isAggrNode( qtcNode * aNode,
                              idBool  * aIsAggr );

    static IDE_RC isSupportedNode( qtcNode * aNode,
                                   idBool  * aIsSupported );

    static IDE_RC checkGroupColumn( qmsSFWGH * aSFWGH,
                                    qtcNode  * aNode,
                                    UInt       aTransType,
                                    UInt     * aFlag );

    static IDE_RC checkAggrColumn( qmsSFWGH * aSFWGH,
                                   qtcNode  * aNode,
                                   UInt       aTransType,
                                   UInt     * aFlag );

    static IDE_RC checkExprColumn( qmsSFWGH * aSFWGH,
                                   qtcNode  * aNode,
                                   UInt       aTransType,
                                   UInt     * aFlag );

    static IDE_RC adjustTargetListAndSortNode( qcStatement  * aStatement,
                                               qmsParseTree * aParseTree,
                                               qmsParseTree * aViewParseTree,
                                               UInt         * aViewTargetOrder );

    static IDE_RC getShardColTransSort( qmsTarget       * aTarget,
                                        qmsSortColumns  * aOrderBy,
                                        qmsSortColumns ** aSortNode);

    static IDE_RC modifySortNode( qcStatement * aStatement,
                                  qmsTarget   * aTarget,
                                  qtcNode     * aSortNode,
                                  qtcNode    ** aNode,
                                  UInt        * aViewTargetOrder );

    static IDE_RC changeSortExpr( qcStatement * aStatement,
                                  qmsTarget   * aTarget,
                                  qtcNode     * aSortNode,
                                  qtcNode    ** aNode,
                                  UInt        * aViewTargetOrder );

    static IDE_RC pushShardColTransFlag( qmsTarget * aTarget,
                                         qmsSFWGH  * aSFWGH );

    /* TASK-7219 Shard Transformer Refactoring */
    static IDE_RC doShardAnalyzeForStatement( qcStatement     * aStatement,
                                              ULong             aSMN,
                                              qcNamePosition  * aStmtPos,
                                              qmsQuerySet     * aQuerySet );

    static IDE_RC makeShardForConvert( qcStatement * aStatement,
                                       qcParseTree * aParseTree );

    static IDE_RC makeShardForStatement( qcStatement * aStatement,
                                         qcParseTree * aParseTree );

    static IDE_RC makeShardForQuerySet( qcStatement    * aStatement,
                                        qcNamePosition * aStmtPos,
                                        qmsQuerySet    * aQuerySet,
                                        qcParseTree    * aParseTree );

    static IDE_RC makeShardForFrom( qcStatement    * aStatement,
                                    qcShardStmtType  aStmtType,
                                    qmsTableRef    * aTableRef,
                                    sdiObjectInfo  * aShardObjInfo );

    static IDE_RC makeShardViewQueryString( qcStatement    * aStatement,
                                            qmsTableRef    * aOldTableRef,
                                            qmsTableRef    * aNewTableRef,
                                            qmsFrom        * aFrom,
                                            qcNamePosition * aStmtPos );

    static IDE_RC makeShardForAggr( qcStatement       * aStatement,
                                    qcShardStmtType     aStmtType,
                                    qmsQuerySet       * aQuerySet,
                                    qcParseTree       * aParseTree,
                                    qcParamOffsetInfo * aParamOffsetInfo,
                                    qcNamePosition    * aTransfromedQuery,
                                    UInt              * aOrderByTarget );

    static IDE_RC makeShardAggrQueryString( qcStatement       * aStatement,
                                            qmsQuerySet       * aQuerySet,
                                            qcParamOffsetInfo * aParamOffsetInfo,
                                            idBool            * aUnsupportedAggr,
                                            UInt              * aGroupKeyCount,
                                            qcNamePosition    * aStmtPos );

    static IDE_RC alllocQueryStruct( qcStatement   * aStatement,
                                     qcStatement  ** aOutStatement,
                                     qmsParseTree ** aOutParseTree,
                                     qmsQuerySet  ** aOutQuerySet,
                                     qmsSFWGH     ** aOutSFWGH,
                                     qmsFrom      ** aOutFrom,
                                     qmsTableRef  ** aOutTableRef );

    static IDE_RC processTransformForShard( qcStatement * aStatement,
                                            qcParseTree * aParseTree );

    static IDE_RC processTransformForNonShard( qcStatement * aStatement,
                                               qcParseTree * aParseTree );

    static IDE_RC processTransformForQuerySet( qcStatement * aStatement,
                                               qmsQuerySet * aQuerySet,
                                               qcParseTree * aParseTree );

    static IDE_RC processTransformForAggr( qcStatement    * aStatement,
                                           qmsQuerySet    * aQuerySet,
                                           qcParseTree    * aParseTree,
                                           idBool         * aIsTransformed );

    static IDE_RC processTransformForFrom( qcStatement    * aStatement,
                                           qcShardStmtType  aStmtType,
                                           qmsFrom        * aFrom );

    static IDE_RC processTransformForInsUptDel( qcStatement    * aStatement,
                                                qcParseTree    * aParseTree );

    static IDE_RC processTransformForKeyword( qcStatement    * aStatement,
                                              qciStmtType      aStmtKind,
                                              qcShardStmtType  aStmtType,
                                              qcNamePosition * aStmtPos,
                                              qmsQuerySet    * aQuerySet,
                                              qcParseTree    * aParseTree );

    static IDE_RC processTransformForSubquery( qcStatement    * aStatement,
                                               qmsQuerySet    * aQuerySet,
                                               qcParseTree    * aParseTree );

    static IDE_RC processTransformForSFWGH( qcStatement    * aStatement,
                                            qcShardStmtType  aStmtType,
                                            qmsSFWGH       * aSFWGH );

    //------------------------------------------
    // TASK-7219 Non-shard DML
    //------------------------------------------
    static IDE_RC makeShardForInsert( qcStatement      * aStatement,
                                      qcParseTree      * aParseTree );

    static IDE_RC makeShardForUptDel( qcStatement      * aStatement,
                                      qcParseTree      * aParseTree );

    static IDE_RC isFullRange( sdiObjectInfo  * aObjectInfo,
                               idBool         * aIsFullRange );

    static IDE_RC partialCoordTransform( qcStatement    * aStatement );

    static IDE_RC partialInsertSelectTransform( qcStatement   * aStatement,
                                                qcmTableInfo  * aTableInfo,
                                                sdiObjectInfo * aShardObj,
                                                qcmColumn     * aInsertColumns,
                                                ULong           aSMN );

    static IDE_RC makeRangeCondition( qcStatement   * aStatement,
                                      qmsSFWGH      * sSFWGH,
                                      qcmTableInfo  * aTableInfo,
                                      sdiObjectInfo * aShardObj,
                                      qcmColumn     * aInsertColumns,
                                      ULong           aSMN );

    static IDE_RC makeMyRanges( qcStatement  * aStatement,
                                UInt           aMyNodeId,
                                UInt           aDefaultNodeId,
                                sdiRangeInfo * aRangeInfo,
                                sdiMyRanges ** aMyRanges );

    static IDE_RC makeKeyString( qcStatement     * aStatement,
                                 SChar           * aMyKeyColName,
                                 UInt              aMyKeyDataType,
                                 sdiSplitMethod    aMyKeySplitMethod,
                                 SInt              aMyKeyPrecision,
                                 SChar          ** aMyKeyString );

    static IDE_RC makeRangeString( qcStatement     * aStatement,
                                   sdiMyRanges     * aMyRanges,
                                   UInt              aMyKeyDataType,
                                   sdiSplitMethod    aMyKeySplitMethod,
                                   SChar           * aMyKeyString,
                                   SChar          ** aMyRangeString,
                                   UInt            * aMyRangeStringLen );

    static IDE_RC makeMinValueString( UInt              aCompareType,
                                      const SChar     * aCompareString,
                                      sdiValue        * aValue,
                                      SChar           * aMyKeyString,
                                      SChar          ** aMyRangeString,
                                      UInt            * aLen,
                                      UInt              aLenMax );

    static IDE_RC setColumnOrderForce( qtcNode * aNode,
                                       SChar   * aMyKeyColName,
                                       UShort    aMyKeyColOrder );
};

#endif /* _O_QMV_VIEW_MERGING_H_ */
