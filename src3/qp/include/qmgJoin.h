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
 * $Id: qmgJoin.h 88830 2020-10-08 02:14:27Z donovan.seo $
 *
 * Description :
 *     Join Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_JOIN_H_
#define _O_QMG_JOIN_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoJoinMethod.h>
#include <qmoPredicate.h>

// To Fix BUG-20259
#define QMG_JOIN_MAX_TMPTABLE_COUNT        (10)

// PROJ-1718
enum qmgInnerJoinMethod
{
    QMG_INNER_JOIN_METHOD_NESTED = 0,
    QMG_INNER_JOIN_METHOD_HASH,
    QMG_INNER_JOIN_METHOD_SORT,
    QMG_INNER_JOIN_METHOD_MERGE,
    QMG_INNER_JOIN_METHOD_COUNT
};

//---------------------------------------------------
// Join Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgJOIN
{
    qmgGraph graph;  // 공통 Graph 정보

    // Join Graph를 위한 정보

    qmoCNF            * onConditionCNF;

    //-----------------------------------------------
    // Join Method 정보
    //-----------------------------------------------

    qmoJoinMethod     * joinMethods;

    qmoJoinMethodCost * selectedJoinMethod; // 가장 cost가 낮은 Join Method

    //----------------------------------------------
    // Join Predicate 정보:
    //    선택된 Join Method Type에 따라 다음과 같이 Join Predicate이 분류된다.
    //
    //    - joinablePredicate
    //      Index Nested Loop or Anti Outer      : indexablePredicate
    //      One Pass/Two Pass Sort Join or Merge : sortJoinablePredicate
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
} qmgJOIN;

//---------------------------------------------------
// Join Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgJoin
{
public:
    // Inner Join에 대응하는 qmgJoin Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmsFrom     * aFrom,
                         qmgGraph   ** aGraph );

    // Join Relation에 대응하는 qmgJoin Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmgGraph    * aLeftGraph,
                         qmgGraph    * aRightGraph,
                         qmgGraph    * aGraph,
                         idBool        aExistOrderFactor );

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    static IDE_RC  makeChildPlan( qcStatement * aStatement,
                                  qmgJOIN     * aMyGraph,
                                  qmnPlan     * aLeftPlan,
                                  qmnPlan     * aRightPlan );

    static IDE_RC  makeSortRange( qcStatement  * aStatement,
                                  qmgJOIN      * aMyGraph,
                                  qtcNode     ** aRange );

    static IDE_RC  makeHashFilter( qcStatement  * aStatement,
                                   qmgJOIN      * aMyGraph,
                                   qtcNode     ** aFilter );

    static IDE_RC  makeFullNLJoin( qcStatement    * aStatement,
                                   qmgJOIN        * aMyGraph,
                                   idBool           aIsStored );

    static IDE_RC  makeIndexNLJoin( qcStatement    * aStatement,
                                    qmgJOIN        * aMyGraph );

    static IDE_RC  makeInverseIndexNLJoin( qcStatement    * aStatement,
                                           qmgJOIN        * aMyGraph );

    static IDE_RC  makeHashJoin( qcStatement    * aStatement,
                                 qmgJOIN        * aMyGraph,
                                 idBool           aIsTwoPass,
                                 idBool           aIsInverse );

    static IDE_RC  makeSortJoin( qcStatement     * aStatement,
                                 qmgJOIN         * aMyGraph,
                                 idBool            aIsTwoPass,
                                 idBool            aIsInverse );

    static IDE_RC  makeMergeJoin( qcStatement     * aStatement,
                                  qmgJOIN         * aMyGraph );

    // Join method에 따라 joinable predicate으로 분리되지 못한
    // predicate들을 모아 filter로 생성
    static IDE_RC  extractFilter( qcStatement   * aStatement,
                                  qmgJOIN       * aMyGraph,
                                  qmoPredicate  * aPredicate,
                                  qtcNode      ** aFilter );

    // Constant predicate이 있을 경우에만 생성
    static IDE_RC  initFILT( qcStatement   * aStatement,
                             qmgJOIN       * aMyGraph,
                             qmnPlan      ** aFILT );

    static IDE_RC  makeFILT( qcStatement  * aStatement,
                             qmgJOIN      * aMyGraph,
                             qmnPlan      * aFILT );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

    // 가장 cost가 좋은 Join Method Cost를 선택
    static IDE_RC selectJoinMethodCost( qcStatement        * aStatement,
                                        qmgGraph           * aGraph,
                                        qmoPredicate       * aJoinPredicate,
                                        qmoJoinMethod      * aJoinMethods,
                                        qmoJoinMethodCost ** aSelected );

    // Join Method 결정 후 처리
    static
        IDE_RC afterJoinMethodDecision( qcStatement       * aStatement,
                                        qmgGraph          * aGraph,
                                        qmoJoinMethodCost * aSelectedMethod,
                                        qmoPredicate     ** aPredicate,
                                        qmoPredicate     ** aJoinablePredicate,
                                        qmoPredicate     ** aNonJoinablePredicate);


    // Preserved Order 생성
    static IDE_RC makePreservedOrder( qcStatement       * aStatement,
                                      qmgGraph          * aGraph,
                                      qmoJoinMethodCost * aSelectedMethod,
                                      qmoPredicate      * aJoinablePredicate );

    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );

private:

    // bucket count과 temp table count을 구하는 함수
    static
        IDE_RC getBucketCntNTmpTblCnt( qcStatement       * aStatement,
                                       qmoJoinMethodCost * aSelectedMethod,
                                       qmgGraph          * aGraph,
                                       idBool              aIsLeftGraph,
                                       UInt              * aBucketCnt,
                                       UInt              * aTmpTblCnt );

    // BUG-13257
    // Temporary_Table의 갯수를 Temporary_TableSpace의 현재 크기를 고려하여 계산한다
    static IDE_RC decideTmpTblCnt(  qcStatement       * aStatement,
                                    qmgGraph          * aGraph,
                                    SDouble             aMemoryBufCnt,
                                    UInt              * aTmpTblCnt );

    // Child의 Graph로부터 Preserved Order를 생성한다.
    static IDE_RC makeOrderFromChild( qcStatement * aStatement,
                                      qmgGraph    * aGraph,
                                      idBool        aIsRightGraph );

    // Two Pass Sort, Merge Join의 Join Predicate으로부터 Preserved Order 생성
    static IDE_RC makeOrderFromJoin( qcStatement        * aStatement,
                                     qmgGraph           * aGraph,
                                     qmoJoinMethodCost  * aSelectedMethod,
                                     qmgDirectionType     aDirection,
                                     qmoPredicate       * aJoinablePredicate );

    // 새로운 Preserved Order 생성 및 설정
    static IDE_RC makeNewOrder( qcStatement       * aStatement,
                                qmgGraph          * aGraph,
                                qcmIndex          * aSelectedIndex );
    
    // 새로운 Preserved Order 생성 및 설정
    static IDE_RC makeNewOrder4Selection( qcStatement       * aStatement,
                                          qmgGraph          * aGraph,
                                          qcmIndex          * aSelectedIndex );

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition graph에 대해 새로운 preserved order 생성 및 설정.
    static IDE_RC makeNewOrder4Partition( qcStatement       * aStatement,
                                          qmgGraph          * aGraph,
                                          qcmIndex          * aSelectedIndex );

    // push-down join predicate를 받아서 그래프에 연결.
    static IDE_RC setJoinPushDownPredicate( qcStatement   * aStatement,
                                            qmgGraph      * aGraph,
                                            qmoPredicate ** aPredicate );
    
    // push-down non-join predicate를 받아서 자신의 그래프에 연결.
    static IDE_RC setNonJoinPushDownPredicate( qcStatement   * aStatement,
                                               qmgGraph      * aGraph,
                                               qmoPredicate ** aPredicate );
    
    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 상위 graph에 의해 access method가 바뀐 경우
    // selection graph의 sdf를 disable 시킨다.
    static IDE_RC alterSelectedIndex( qcStatement * aStatement,
                                      qmgGraph    * aGraph,
                                      qcmIndex    * aNewIndex );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 상위 JOIN graph에서 ANTI로 처리할 때
    // 하위 SELT graph를 복사하는데 이때 이 함수를
    // 통해서 복사하도록 해야 안전하다.
    static IDE_RC copyGraphAndAlterSelectedIndex( qcStatement * aStatement,
                                                  qmgGraph    * aSource,
                                                  qmgGraph   ** aTarget,
                                                  qcmIndex    * aNewSelectedIndex,
                                                  UInt          aWhichOne );
    
    static IDE_RC randomSelectJoinMethod( qcStatement        * aStatement,
                                          UInt                 aJoinMethodCnt,
                                          qmoJoinMethod      * aJoinMethods,
                                          qmoJoinMethodCost ** aSelected );

    static void moveConstantPred( qmgJOIN * aMyGraph, idBool aIsInverse );

    static void checkOrValueIndex( qmgGraph * aGraph );

#endif /* _O_QMG_JOIN_H_ */
};
