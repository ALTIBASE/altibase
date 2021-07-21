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
 * $Id: qmgLeftOuter.h 90785 2021-05-06 07:26:22Z hykim $
 *
 * Description :
 *     LeftOuter Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_LEFT_OUTER_H_
#define _O_QMG_LEFT_OUTER_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoJoinMethod.h>
#include <qmoPredicate.h>

//---------------------------------------------------
// Left Outer Join Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgLOJN
{
    qmgGraph graph;  // 공통 Graph 정보

    // LeftOuter Join Graph를 위한 정보

    qmoCNF            * onConditionCNF;

    //-----------------------------------------------
    // Join Method 정보
    //-----------------------------------------------

    qmoJoinMethod     * nestedLoopJoinMethod;
    qmoJoinMethod     * sortBasedJoinMethod;
    qmoJoinMethod     * hashBasedJoinMethod;

    qmoJoinMethodCost * selectedJoinMethod; // 가장 cost가 낮은 Join Method

    //----------------------------------------------
    // Join Predicate 정보:
    //    선택된 Join Method Type에 따라 다음과 같이 Join Predicate이 분류된다.
    //
    //    - joinablePredicate
    //      Index Nested Loop or Anti Outer      : indexablePredicate
    //      One Pass/Two Pass Sort Join          : sortJoinablePredicate
    //      One Pass/Two Pass Hash Join          : hashJoinablePredicate
    //    - nonJoinablePredicate
    //      Index Nested Loop or Anti Outer      : nonIndexablePredicate
    //      One Pass/Two Pass Sort Join or Merge : nonSortJoinablePredicate
    //      One Pass/Two Pass Hash Join          : nonHashJoinablePredicate
    //
    //----------------------------------------------

    qmoPredicate      * joinablePredicate;
    qmoPredicate      * nonJoinablePredicate;

    // PROJ-2179/BUG-35484
    // Full/index nested loop join시 push down된 predicate을 담아둔다.
    qmoPredicate      * pushedDownPredicate;

    //---------------------------------------------
    // Join Method Type이 Hash Based Join인 경우, 사용
    //---------------------------------------------

    UInt            hashBucketCnt;        // hash bucket count
    UInt            hashTmpTblCnt;        // hash temp table 개수

    // PROJ-2242
    SDouble         firstRowsFactor;      // FIRST_ROWS_N
    SDouble         joinOrderFactor;      // join ordering factor
    SDouble         joinSize;             // join ordering size
} qmgLOJN;

//---------------------------------------------------
// Left Outer Join Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgLeftOuter
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmsFrom     * aFrom,
                         qmgGraph   ** aGraph );

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

private:
    static IDE_RC  makeChildPlan( qcStatement * aStatement,
                                  qmgLOJN     * aMyGraph,
                                  qmnPlan     * aLeftPlan,
                                  qmnPlan     * aRightPlan );

    static IDE_RC  makeSortRange( qcStatement  * aStatement,
                                  qmgLOJN      * aMyGraph,
                                  qtcNode     ** aRange );

    static IDE_RC  makeHashFilter( qcStatement  * aStatement,
                                   qmgLOJN      * aMyGraph,
                                   qtcNode     ** aFilter );

    static IDE_RC  makeNestedLoopJoin( qcStatement * aStatement,
                                       qmgLOJN     * aMyGraph,
                                       UInt          aJoinType );

    static IDE_RC  makeHashJoin( qcStatement * aStatement,
                                 qmgLOJN     * aMyGraph,
                                 idBool        aIsTwoPass,
                                 idBool        aIsInverse );

    static IDE_RC  makeSortJoin( qcStatement    * aStatement,
                                 qmgLOJN        * aMyGraph,
                                 idBool           aIsTwoPass );

    static IDE_RC  initFILT( qcStatement  * aStatement,
                             qmgLOJN      * aMyGraph,
                             qtcNode     ** aFilter,
                             qmnPlan     ** aFILT );

    static IDE_RC  makeFILT( qcStatement * aStatement,
                             qmgLOJN     * aMyGraph,
                             qtcNode     * aFilter,
                             qmnPlan     * aFILT );

    // PROJ-2750
    static IDE_RC checkAndSetSkipRight( qmgGraph * aGraph );

    static idBool isExistsSkipRightIgnoreOperation( mtcNode   * aNode,
                                                    qcDepInfo * aTargetDepInfo );

    static IDE_RC checkAndSetSkipRightFlag( qmnPlan * aPlan );
};

#endif /* _O_QMG_LEFT_OUTER_H_ */
