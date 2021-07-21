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
 * $Id$
 *
 * Description :
 *     PROJ-1718 Subquery Unnesting
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_UNNESTING_H_
#define _O_QMO_UNNESTING_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmoDependency.h>

// Subquery unnesting과 관련되어 내부적으로 사용할 자료구조
typedef struct qmoPredList
{
    qtcNode     * predicate;
    qmoPredList * next;
    idBool        hit;
} qmoPredList;

class qmoUnnesting
{
public:

    //------------------------------------------
    // Subquery unnesting의 수행
    //------------------------------------------
    
    static IDE_RC  doTransform( qcStatement  * aStatement,
                                idBool       * aChanged );

    static IDE_RC  doTransformSubqueries( qcStatement * aStatement,
                                          qtcNode     * aPredicate,
                                          idBool      * aChanged );
private:
    static IDE_RC  doTransformQuerySet( qcStatement * aStatement,
                                        qmsQuerySet * aQuerySet,
                                        idBool      * aChanged );

    static IDE_RC  doTransformFrom( qcStatement * aStatement,
                                    qmsSFWGH    * aSFWGH,
                                    qmsFrom     * aFrom,
                                    idBool      * aChanged );

    static IDE_RC  findAndUnnestSubquery( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          idBool        aUnnestSubquery,
                                          qtcNode     * aPredicate,
                                          idBool      * aChanged );

    static idBool  isExistsTransformable( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          qtcNode     * aNode,
                                          idBool        aUnnestSubquery );

    static IDE_RC  transformToExists( qcStatement * aStatement,
                                      qtcNode     * aNode );

    static idBool  isUnnestableSubquery( qcStatement * aStatement,
                                         qmsSFWGH    * aSFWGH,
                                         qtcNode     * aSubqueryPredicate );

    static IDE_RC  unnestSubquery( qcStatement * aStatement,
                                   qmsSFWGH    * aSFWGH,
                                   qtcNode     * aSQPredicate );

    static IDE_RC  setJoinMethodHint( qcStatement * aStatement,
                                      qmsSFWGH    * aSFWGH,
                                      qtcNode     * aJoinPred,
                                      qmsFrom     * aViewFrom,
                                      idBool        aIsAntiJoin );

    static void    setNoMergeHint( qmsFrom * aViewFrom );

    static IDE_RC  setJoinType( qtcNode * aPredicate,
                                idBool    aType,
                                idBool    aIsRemoveSemi,
                                qmsFrom * aViewFrom );

    static IDE_RC  makeDummyConstant( qcStatement  * aStatement,
                                      qtcNode     ** aResult );

    static IDE_RC  removePassNode( qcStatement * aStatement,
                                   qtcNode     * aNode );

    static IDE_RC  setDummySelect( qcStatement * aStatement,
                                   qtcNode     * aNode,
                                   idBool        aCheckAggregation );

    static IDE_RC  concatPredicate( qcStatement  * aStatement,
                                    qtcNode      * aPredicate1,
                                    qtcNode      * aPredicate2,
                                    qtcNode     ** aResult );

    static idBool  isSubqueryPredicate( qtcNode * aPredicate );

    static idBool  isQuantifiedSubquery( qtcNode * aPredicate );

    static IDE_RC  genCorrPredicate( qcStatement  * aStatement,
                                     qtcNode      * aPredicate,
                                     qtcNode      * aOperand1,
                                     qtcNode      * aOperand2,
                                     idBool         aExistsTrans, /* BUG-46952 */
                                     qtcNode     ** aResult );

    static IDE_RC  genCorrPredicates( qcStatement  * aStatement,
                                      qtcNode      * aNode,
                                      idBool         aExistsTrans, /* BUG-46952 */
                                      qtcNode     ** aResult );

    static mtfModule * toNonQuantifierModule( const mtfModule * aQuantifer );

    static mtfModule * toExistsModule( const mtfModule * aQuantifer );

    static mtfModule * toExistsModule4CountAggr( qcStatement * aStatement,
                                                 qtcNode     * aNode );

    static idBool  isNullable( qcStatement * aStatement,
                               qtcNode     * aNode );

    static idBool  isSingleRowSubquery( qcStatement * aSubQStatement );

    static idBool  containsUniqueKeyPredicate( qcStatement * aStatement,
                                               qtcNode     * aPredicate,
                                               UInt          aJoinTable,
                                               qcmIndex    * aUniqueIndex );

    static void    findUniqueKeyPredicate( qcStatement * aStatement,
                                           qtcNode     * aPredicate,
                                           UInt          aJoinTable,
                                           qcmIndex    * aUniqueIndex,
                                           UChar       * aRefVector );

    static UInt    findUniqueKeyColumn( qcStatement * aStatement,
                                        qtcNode     * aNode,
                                        UInt          aTable,
                                        qcmIndex    * aUniqueIndex );

    static idBool  containsGroupByPredicate( qmsSFWGH * aSFWGH );

    static void    findGroupByPredicate( qmsSFWGH         * aSFWGH,
                                         qmsConcatElement * aGroup,
                                         qtcNode          * aPredicate,
                                         UChar            * aRefVector );

    static UInt    findGroupByExpression( qmsConcatElement * aGroup,
                                          qtcNode          * aExpression );

    static idBool  isSimpleSubquery( qcStatement * aStatement );

    static idBool  isUnnestablePredicate( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          qtcNode     * aSubqueryPredicate,
                                          qmsSFWGH    * aSQSFWGH );

    static idBool  existOuterJoinCorrelation( qtcNode   * aNode,
                                              qcDepInfo * aOuterDepInfo );

    static idBool  isAntiJoinablePredicate( qtcNode   * aNode,
                                            qcDepInfo * aDepInfo );

    static void    existConjunctiveJoin( qtcNode   * aSubqueryPredicate,
                                         qtcNode   * aNode,
                                         qcDepInfo * aInnerDepInfo,
                                         qcDepInfo * aOuterDepInfo,
                                         idBool    * aIsConjunctive,
                                         UInt      * aJoinPredCount );

    static IDE_RC  transformSubqueryToView( qcStatement  * aStatement,
                                            qtcNode      * aSQPredicate,
                                            qmsFrom     ** aView );

    static IDE_RC  genUniqueViewName( qcStatement * aStatement, UChar ** aViewName );

    static IDE_RC  removeCorrPredicate( qcStatement  * aStatement,
                                        qtcNode     ** aPredicate,
                                        qcDepInfo    * aOuterDepInfo,
                                        qtcNode     ** aRemoved );

    static IDE_RC  genViewSelect( qcStatement * aStatement,
                                  qtcNode     * aCorrPredicate,
                                  idBool        aIsOuterExpr );

    static IDE_RC  genViewSetOp( qcStatement * aStatement,
                                 qmsQuerySet * aSetQuerySet,
                                 idBool        aIsOuterExpr );
    
    static IDE_RC  appendViewSelect( qcStatement * aStatement,
                                     qtcNode     * aNode );

    static IDE_RC  toViewColumns( qcStatement  * aViewStatement,
                                  qmsTableRef  * aViewTableRef,
                                  qtcNode     ** aNode,
                                  idBool         aIsOuterExpr );

    static IDE_RC  toViewSetOp( qcStatement  * aViewStatement,
                                qmsTableRef  * aViewTableRef,
                                qmsQuerySet  * aSetQuerySet,
                                idBool         aIsOuterExpr );

    static IDE_RC  findAndRemoveSubquery( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          qtcNode     * aPredicate,
                                          idBool      * aResult );

    static IDE_RC  isRemovableSubquery( qcStatement  * aStatement,
                                        qmsSFWGH     * aSFWGH,
                                        qtcNode      * aSubqueryPredicate,
                                        idBool       * aResult,
                                        UShort      ** aRelationMap );

    static idBool  isRemovableTarget( qtcNode   * aNode,
                                      qcDepInfo * aOuterCommonDepInfo );

    static IDE_RC  isSubsumed( qcStatement  * aStatement,
                               qmsSFWGH     * aOQSFWGH,
                               qmsSFWGH     * aSQSFWGH,
                               idBool       * aResult,
                               UShort      ** aRelationMap,
                               qcDepInfo    * aOuterCommonDepInfo );

    static IDE_RC  changeRelation( qcStatement * aStatement,
                                   qtcNode     * aPredicate,
                                   qcDepInfo   * aDepInfo,
                                   UShort      * aRelationMap );

    static IDE_RC  createRelationMap( qcStatement  * aStatement,
                                      qmsSFWGH     * aOQSFWGH,
                                      qmsSFWGH     * aSQSFWGH,
                                      UShort      ** aRelationMap );

    static idBool  isConjunctiveForm( qtcNode * aPredicate );

    static IDE_RC  genPredicateList( qcStatement  * aStatement,
                                     qtcNode      * aPredicate,
                                     qmoPredList ** aPredList );

    static IDE_RC  freePredicateList( qcStatement * aStatement,
                                      qmoPredList * aPredList );

    static IDE_RC  changeSemiJoinInnerTable( qmsSFWGH * aSFWGH,
                                             qmsSFWGH * aViewSFWGH,
                                             SInt       aViewID );

    static IDE_RC  removeSubquery( qcStatement * aStatement,
                                   qmsSFWGH    * aSFWGH,
                                   qtcNode     * aPredicate,
                                   UShort      * aRelationMap );

    static IDE_RC  transformToCase2Expression( qtcNode * aSubqueryPredicate );

    static IDE_RC  transformAggr2Window( qcStatement * aStatement,
                                         qtcNode     * aNode,
                                         UShort      * aRelationMap );

    static IDE_RC  movePredicates( qcStatement  * aStatement,
                                   qtcNode     ** aPredicate,
                                   qmsSFWGH     * aSFWGH );

    static IDE_RC  addPartition( qcStatement    * aStatement,
                                 qtcNode        * aExpression,
                                 qtcOverColumn ** aPartitions );

    static IDE_RC  genPartitions( qcStatement    * aStatement,
                                  qmsSFWGH       * aSFWGH,
                                  qtcNode        * aPredicate,
                                  qtcOverColumn ** aPartitions );

    // BUG-38827
    static idBool  existViewTarget( qtcNode   * aNode,
                                    qcDepInfo * aDepInfo );

    // BUG-40753
    static IDE_RC  setAggrNode( qcStatement * aSQStatement,
                                qmsSFWGH    * aSQSFWGH,
                                qtcNode     * aNode );
    // BUG-45591
    static void    delAggrNode( qmsSFWGH    * aSQSFWGH,
                                qtcNode     * aNode );

    // BUG-41564
    static idBool existOuterSubQueryArgument( qtcNode   * aNode,
                                              qcDepInfo * aInnerDepInfo );

    static idBool isOuterRefSubquery( qtcNode   * aArg,
                                      qcDepInfo * aInnerDepInfo );

    // BUG-45238
    static idBool findCountAggr4Target( qtcNode  * aTarget );
    
    static idBool isRemoveSemiJoin( qcStatement  * aSQStatement,
                                    qmsParseTree * aSQParseTree );

    static idBool findUniquePredicate( qcStatement * aStatement,
                                       qmsSFWGH    * aSFWGH,
                                       UInt          aTableID,
                                       UInt          aUniqueID,
                                       qtcNode     * aNode );

    static void   removeDownSemiJoinFlag( qmsSFWGH * aSFWGH );

    static void isAggrExistTransformable( qtcNode   * aNode,
                                          qcDepInfo * aOuterDep,
                                          idBool    * aIsTrue );
};

#endif /* _O_QMO_UNNESTING_H_ */

