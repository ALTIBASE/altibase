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
 * $Id: qmgPushPred.h 24873 2008-01-18 02:23:38Z sungminee $
 *
 * Description :
 *     BUG-19756 View Predicate Pushdown
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_PUSH_PRED_H_
#define _O_QMO_PUSH_PRED_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmgDef.h>
#include <qmoPredicate.h>

/* TASK-7219 */
#define QMO_CHANGE_COLUMN_NAME_DISABLE (0)
#define QMO_CHANGE_COLUMN_NAME_ENABLE  (1)
#define QMO_CHANGE_COLUMN_NAME_ONLY    (2)

class qmoPushPred
{
public:
    
    // BUG-18367 SFWGH의 predicate을 view의 predicate으로 pushdown 수행
    static IDE_RC doPushDownViewPredicate( qcStatement  * aStatement,
                                           qmsParseTree * aViewParseTree,
                                           qmsQuerySet  * aViewQuerySet,
                                           UShort         aViewTupleId,
                                           qmsSFWGH     * aSFWGH,
                                           qmsFrom      * aFrom,
                                           qmoPredicate * aPredicate,
                                           idBool       * aIsPushed,
                                           idBool       * aIsPushedAll,
                                           idBool       * aRemainPushedPred );

    /* TASK-7219 */
    static IDE_RC checkPushDownPredicate( qcStatement  * aStatement,
                                          qmsParseTree * aViewParseTree,
                                          qmsQuerySet  * aViewQuerySet,
                                          qmsSFWGH     * aOuterQuery,
                                          UShort         aViewTupleId,
                                          qmoPredicate * aPredicate,
                                          idBool       * aIsPushed );

    static IDE_RC changePredicateNodeName( qcStatement * aStatement,
                                           qmsQuerySet * aViewQuerySet,
                                           UShort        aViewTupleId,
                                           qtcNode     * aSrcNode,
                                           qtcNode    ** aDstNode );

    /* TASK-7219 Non-shard DML */
    static void setForcePushedPredForShardView( mtcNode * aNode );
    

private:

    // Push selection 해도 되는 구문인지 queryset 단위로 검사
    static IDE_RC canPushSelectionQuerySet( qmsParseTree * aViewParseTree,
                                            qmsQuerySet  * aViewQuerySet,
                                            idBool       * aCanPushDown,
                                            idBool       * aRemainPushedPred );
    

    // Push slection 해도 되는 predicate인지 검사 
    static IDE_RC canPushDownPredicate( qcStatement  * aStatement,
                                        qmsQuerySet  * aViewQuerySet,
                                        qmsTarget    * aViewTarget,
                                        UShort         aViewTupleId,
                                        qmsSFWGH     * aOuterQuery,
                                        qtcNode      * aNode,
                                        idBool         aContainRootsNext,
                                        idBool         aPushIntoShardView,
                                        idBool       * aCanPushDown );

    /* TASK-7219 Non-shard DML */
    static IDE_RC findOutRefTuple( UShort    aTupleId,
                                   qmsFrom * aOutRefFrom,
                                   idBool  * aIsFound );

    /* TASK-7219 Non-shard DML */
    static IDE_RC findOutRefTupleForFromTree( UShort    aTupleId,
                                              qmsFrom * aFrom,
                                              idBool  * aIsFound );
    
    // BUG-18367 view의 predicate으로 pushdown 수행
    static IDE_RC pushDownPredicate( qcStatement  * aStatement,
                                     qmsQuerySet  * aViewQuerySet,
                                     UShort         aViewTupleId,
                                     qmsSFWGH     * aSFWGH,
                                     qmsFrom      * aFrom,
                                     qmoPredicate * aPredicate );

    /* BUG-40354 pushed rank */
    static IDE_RC isPushableRankPred( qcStatement  * aStatement,
                                      qmsParseTree * aViewParseTree,
                                      qmsQuerySet  * aViewQuerySet,
                                      UShort         aViewTupleId,
                                      qtcNode      * aNode,
                                      idBool       * aCanPushDown,
                                      UInt         * aPushedRankTargetOrder,
                                      qtcNode     ** aPushedRankLimit );

    /* BUG-40354 pushed rank */
    static IDE_RC isStopKeyPred( UShort         aViewTupleId,
                                 qtcNode      * aColumn,
                                 qtcNode      * aValue,
                                 idBool       * aIsStopKey );

    /* BUG-40354 pushed rank */
    static IDE_RC pushDownRankPredicate( qcStatement  * aStatement,
                                         qmsQuerySet  * aViewQuerySet,
                                         UInt           aRankTargetOrder,
                                         qtcNode      * aRankLimit );

    // BUG-19756 view의 predicate을 view 내부의 table predicate으로 변환
    static IDE_RC changeViewPredIntoTablePred( qcStatement  * aStatement,
                                               qmsTarget    * aViewTarget,
                                               UShort         aViewTupleId,
                                               qtcNode      * aNode,
                                               UShort         aChangeName, /* TASK-7219 */
                                               idBool         aContainRootsNext );

    // BUG-19756 view 컬럼을 view target column으로 변환
    static IDE_RC transformToTargetColumn( qtcNode      * aNode,
                                           UShort         aChangeName, /* TASK-7219 */
                                           qtcNode      * aTargetColumn );
    
    // BUG-19756 view 컬럼을 view target value로 변환
    static IDE_RC transformToTargetValue( qtcNode      * aNode,
                                          qtcNode      * aTargetColumn );
                                             
    // BUG-19756 view 컬럼을 view의 target expression으로 변환
    static IDE_RC transformToTargetExpression( qcStatement  * aStatement,
                                               qtcNode      * aNode,
                                               qtcNode      * aTargetColumn );
};

#endif /* _O_QMO_PUSH_PRED_H_ */
