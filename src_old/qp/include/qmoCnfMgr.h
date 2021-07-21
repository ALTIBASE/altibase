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
 * $Id: qmoCnfMgr.h 86122 2019-09-04 07:20:21Z donovan.seo $
 *
 * Description :
 *     CNF Critical Path Manager
 *
 *     CNF Normalized Form에 대한 최적화를 수행하고
 *     해당 Graph를 생성한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_CNF_MGR_H_
#define _O_QMO_CNF_MGR_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoPredicate.h>

//---------------------------------------------------------
// [PROJ-1352] JOIN SELECTIVITY THRESHOLD
//
// Join Selectivity가 아주 작아지면, join selectivity만으로
// 평가하는 것은 문제가 있다 (TPC-H 21).
// 이를 보완하기 위하여 특정 threshold 이하인 경우에 한해서는
// Output record count를 반영하여야 한다.
//
// Join Selectivity의 의미 파악
//    [0.5]  : 100 JOIN 100 ==> 100
//             WHERE T1.i1 = T2.i1 와 같이
//             T1.i1 과 T2.i1에 중복이 없고, 1 : 1 로 매핑되는 경우임
//             아주 효율적인 죠인 조건이 있는 경우임
//    [0.1]  : 100 JOIN 100 ==> 20
//             WHERE T1.i1 = T2.i1 AND T1.i2 > ? 과 같이
//             [0.5] 인 경우의 조건에 일정 부분(1/3~1/5)
//             걸러 주는 조건이 추가된 경우임.
//    [0.05] : 100 JOIN 100 ==> 10
//             WHERE T1.i1 = T2.i1 AND T1.I2 = ? 과 같이
//             [0.5] 인 경우의 조건에 상당 부분(1/10)을
//             걸러 주는 조건이 추가된 경우임
//---------------------------------------------------------
// PROJ-1718 Semi/anti join을 추가한 결과 0.1일 때 성능이 더 좋다.
#define QMO_JOIN_SELECTIVITY_THRESHOLD           (0.1)

// PROJ-1718 Subquery unnesting
enum qmoJoinRelationType
{
    QMO_JOIN_RELATION_TYPE_INNER = 0,
    QMO_JOIN_RELATION_TYPE_SEMI,
    QMO_JOIN_RELATION_TYPE_ANTI
};

//------------------------------------------------------
// Table Order 정보를 표현하기 위한 자료 구조
//------------------------------------------------------
typedef struct qmoTableOrder
{
    qcDepInfo       depInfo;
    qmoTableOrder * next;
} qmoTableOrder;

/* BUG-41343 */
typedef enum qmoPushApplicableType
{
    QMO_PUSH_APPLICABLE_FALSE = 0,
    QMO_PUSH_APPLICABLE_TRUE,
    QMO_PUSH_APPLICABLE_LEFT,
    QMO_PUSH_APPLICABLE_RIGHT
} qmoPushApplicableType;

//-------------------------------------------------------
// Join Relation을 나타내기 위한 자료 구조
//-------------------------------------------------------

typedef struct qmoJoinRelation
{
    qcDepInfo             depInfo;
    qmoJoinRelation     * next;

    // PROJ-1718 Subquery unnesting
    // Semi/anti join의 경우 join type과 방향을 나타낸다.
    qmoJoinRelationType   joinType;
    UShort                innerRelation;
} qmoJoinRelation;

//----------------------------------------------------
// Join Group을 관리하기 위한 자료 구조
//----------------------------------------------------

typedef struct qmoJoinGroup
{
    qmoPredicate        * joinPredicate;    // join predicate

    // join 관계에 있는 table들의 dependencies를 ORing 한 값
    qmoJoinRelation     * joinRelation;     // linked list
    UInt                  joinRelationCnt;  // join graph 생성 시 필요

    // join group 내의 모든 table들의 dependencies를 ORing 한 값
    qcDepInfo             depInfo;

    // join ordering이 끝난 후, top graph를 pointing
    qmgGraph            * topGraph;

    //--------------------------------------------------------
    // Base Graph 관련 자료 구조
    //-------------------------------------------------------
    UInt                  baseGraphCnt;    // baseGraph 개수
    qmgGraph           ** baseGraph;       // baseGraph pointer 저장할 배열
    idBool                mIsOnlyNL;
} qmoJoinGroup;


//---------------------------------------------------
// CNF Critical Path를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmoCNF
{
    qtcNode       * normalCNF;   // where절을 CNF로 normalize한 결과
    qtcNode       * nnfFilter;   // NNF Filter의 지원, PR-12743

    qmsQuerySet   * myQuerySet;  // 해당 query set을 가리킴
    qmgGraph      * myGraph;     // CNF Form 결과 graph의 top
    SDouble         cost;        // CNF Total Cost

    // from의 dependencies 값을 복사해서 가진다.
    qcDepInfo       depInfo;

    //-------------------------------------------------------------------
    // Predicate 관련 자료 구조
    //
    // constantPredicate : FROM 절과 관계없는 predicate 정보
    //                     ex) 1 = 1
    // oneTablePredicate : FROM 절에 존재하는 개념상 table 중 오직 하나의
    //                     table과 관련된 predicate
    //                     ex) T1.I1 = 1
    // joinPredicate     : FROM 절에 존재하는 개념상 table 중 두개 이상의
    //                     table과 관련된 predicate
    //                     ex) T1.I1 = T2.I1
    // levelPredicate    : level predicate
    //                     ex) level = 1
    //-------------------------------------------------------------------

    qmoPredicate  * constantPredicate;
    qmoPredicate  * oneTablePredicate;
    qmoPredicate  * joinPredicate;
    qmoPredicate  * levelPredicate;
    qmoPredicate  * connectByRownumPred;

    //-------------------------------------------------------
    // Base Graph 관련 자료 구조
    //-------------------------------------------------------

    UInt            graphCnt4BaseTable;    // baseGraph 개수
    qmgGraph     ** baseGraph;             // baseGraph pointer를 저장할 배열

    //------------------------------------------------------
    // Join Group 관련 자료 구조
    //
    //   - maxJoinGroupCnt : 최대 joinGroupCnt = graphCnt4BaseTable
    //   - joinGroupCnt    : 실제 joinGroupCnt
    //   - joinGroup       : joinGroup 배열
    //------------------------------------------------------

    UInt            maxJoinGroupCnt;
    UInt            joinGroupCnt;
    qmoJoinGroup  * joinGroup;

    //-------------------------------------------------------------------
    // Table Order 정보
    //    - tableCnt : table 개수
    //      outer join이 없는 경우 : graphCnt4BaseTable과 동일한 값을 가짐
    //      outer join이 있는 경우 : outer join에 참가하는 모든 table 개수
    //                               포함하므로 graphCnt4BaseTable보가 큼
    //   - tableOrder : table 순서
    //-------------------------------------------------------------------

    UInt            tableCnt;
    qmoTableOrder * tableOrder;

    // BUG-34295 Join ordering ANSI style query
    qmgGraph      * outerJoinGraph;
    idBool          mIsOnlyNL;
} qmoCNF;

//----------------------------------------------------------
// CNF Critical Path를 관리하기 위한 함수
//----------------------------------------------------------

class qmoCnfMgr
{
public:

    //-----------------------------------------------------
    // CNF Critical Path 생성 및 초기화
    //-----------------------------------------------------

    // 일반 qmoCNF의 초기화
    static IDE_RC    init( qcStatement * aStatement,
                           qmoCNF      * aCNF,
                           qmsQuerySet * aQuerySet,
                           qtcNode     * aNormalCNF,
                           qtcNode     * aNnfFilter );

    // on Condition을 위한 qmoCNF의 초기화
    static IDE_RC    init( qcStatement * aStatement,
                           qmoCNF      * aCNF,
                           qmsQuerySet * aQuerySet,
                           qmsFrom     * aFrom,
                           qtcNode     * aNormalCNF,
                           qtcNode     * aNnfFilter );

    //-----------------------------------------------------
    // CNF Critical Path에 대한 최적화
    //     - Predicate 분류, 각 base graph최적화, Join의 처리 수행
    //     - qmgHierarcy와 left outer graph 계열은 호출하지 않음
    //-----------------------------------------------------

    static IDE_RC   optimize( qcStatement * aStatement,
                              qmoCNF      * aCNF );


    // PROJ-1446 Host variable을 포함한 질의 최적화
    // optimization때 만든 정보를 지울 필요가 있을 때
    // 이 함수에 추가하면 된다.
    static IDE_RC    removeOptimizationInfo( qcStatement * aStatement,
                                             qmoCNF      * aCNF );

    //-----------------------------------------------------
    // Predicate 분류 함수
    //-----------------------------------------------------

    // where의 Predicate 분류
    static IDE_RC    classifyPred4Where( qcStatement       * aStatement,
                                         qmoCNF            * aCNF,
                                         qmsQuerySet       * aQuerySet );

    // where의 Predicate 분류
    static IDE_RC    classifyPred4WhereHierachyJoin( qcStatement  * aStatement,
                                                     qmoCrtPath   * aCrtPath,
                                                     qmgGraph     * aGraph );
    // on Condition Predicate 분류
    static IDE_RC    classifyPred4OnCondition( qcStatement       * aStatement,
                                               qmoCNF            * aCNF,
                                               qmoPredicate     ** aUpperPred,
                                               qmoPredicate      * aLowerPred,
                                               qmsJoinType         aJoinType );

    // startWith의 Predicate 분류
    static IDE_RC    classifyPred4StartWith( qcStatement       * aStatement,
                                             qmoCNF            * aCNF,
                                             qcDepInfo         * aDepInfo );

    // connectBy의 Predicate 분류
    static IDE_RC    classifyPred4ConnectBy( qcStatement       * aStatement,
                                             qmoCNF            * aCNF,
                                             qcDepInfo         * aDepInfo );

    // baseGraph들의 초기화 함수 호출하여 baseGraph를 생성 및 초기화함
    static IDE_RC    initBaseGraph( qcStatement   * aStatement,
                                    qmgGraph     ** aBaseGraph,
                                    qmsFrom       * aFrom,
                                    qmsQuerySet   * aQuerySet );

    // Predicate을 복사하는 함수
    static IDE_RC    copyPredicate( qcStatement   * aStatement,
                                    qmoPredicate ** aDstPredicate,
                                    qmoPredicate  * aSrcPredicate );

    // Join Graph의 selectivity를 구하는 함수
    static IDE_RC    getJoinGraphSelectivity( qcStatement  * aStatement,
                                              qmgGraph     * aJoinGraph,
                                              qmoPredicate * aJoinPredicate,
                                              SDouble      * aJoinSelectivity,
                                              SDouble      * aJoinSize );

    // fix BUG-9791, BUG-10419
    // constant filter를 처리가능한 최하위 left graph로 내려주는 함수
    static IDE_RC    pushSelection4ConstantFilter( qcStatement * aStatement,
                                                   qmgGraph    * aGraph,
                                                   qmoCNF      * aCNF );

    // PROJ-1404
    // where절의 predicate으로 transitive predicate을 생성하고 연결함
    static IDE_RC    generateTransitivePredicate( qcStatement * aStatement,
                                                  qmoCNF      * aCNF,
                                                  idBool        aIsOnCondition );

    // PROJ-1404
    // on절의 predicate으로 transitive predicate을 생성하고 연결함
    // on절과 upper predicate으로 lower predicate을 생성하고 반환함
    static IDE_RC    generateTransitivePred4OnCondition( qcStatement   * aStatement,
                                                         qmoCNF        * aCNF,
                                                         qmoPredicate  * aUpperPred,
                                                         qmoPredicate ** aLowerPred );

    // fix BUG-19203
    // on Condition CNF의 각 Predicate의 subuqery 처리
    static IDE_RC   optimizeSubQ4OnCondition( qcStatement * aStatement,
                                              qmoCNF      * aCNF );

private:
    //------------------------------------------------------
    // 초기화 함수에서 사용
    //------------------------------------------------------

    // joinGroup들을 초기화
    static IDE_RC    initJoinGroup( qcStatement   * aStatement,
                                    qmoJoinGroup  * aJoinGroup,
                                    UInt            aJoinGroupCnt,
                                    UInt            aBaseGraphCnt );

    //-----------------------------------------------------
    // Predicate 분류 함수에서 사용
    //-----------------------------------------------------

    // classifyPred4Where 함수에서 호출 :
    //    constant predicate을 level/prior/constant 분류하여 해당 위치에 추가
    static IDE_RC    addConstPred4Where( qmoPredicate * aPredicate,
                                         qmoCNF       * aCNF,
                                         idBool         aExistHierarchy );

    static idBool    checkPushPredHint( qmoCNF         * aCNF,
                                        qtcNode        * aNode,
                                        qmsJoinType      aJoinType );

    // PROJ-1495
    // classifyPred4Where 함수에서 호출 :
    //    PUSH_PRED hint로 인해 hint의 view table과 관련된
    //    join predicate을 view내부로 내림
    static IDE_RC  pushJoinPredInView( qmoPredicate     * aPredicate,
                                       qmsPushPredHints * aPushPredHint,
                                       UInt               aBaseGraphCnt,
                                       qmsJoinType        aJoinType,
                                       qmgGraph        ** aBaseGraph,
                                       idBool           * aIsPush );

    // classifyPred4OnCondition 함수에서 호출 :
    //    onConditionCNF의 one table predicate을 해당 위치에 추가
    static IDE_RC    addOneTblPredOnJoinType( qmoCNF       * aCNF,
                                              qmgGraph     * aGraph,
                                              qmoPredicate * aPredicate,
                                              qmsJoinType    aJoinType,
                                              idBool         aIsLeft );

    // classifyPred4OnCondition 함수에서 호출
    //    onConditionCNF를 가지는 Join Graph의 myPredicate을 push selection
    static IDE_RC   pushSelectionOnJoinType( qcStatement   * aStatement,
                                             qmoPredicate ** aUpperPred,
                                             qmgGraph     ** aBaseGraph,
                                             qcDepInfo     * aFromDependencies,
                                             qmsJoinType     aJoinType );


    // classifyPred4ConnectBy 함수에서 호출 :
    //    constant predicate을 level/prior/constant 분류하여 해당 위치에 추가
    static IDE_RC    addConstPred4ConnectBy( qmoCNF       * aCNF,
                                             qmoPredicate * aPredicate );

    //-----------------------------------------------------
    // 최적화 함수에서 사용
    //-----------------------------------------------------

    // Join Order를 결정하는 함수
    static IDE_RC    joinOrdering( qcStatement    * aStatement,
                                   qmoCNF         * aCNF );

    //-----------------------------------------------------
    // Join Ordering 함수에서 사용
    //-----------------------------------------------------

    // Join Group을 분류하는 함수
    static IDE_RC    joinGrouping( qmoCNF         * aCNF );

    // Join Group 내 Join Relation
    static IDE_RC    joinRelationing( qcStatement    * aStatement,
                                      qmoJoinGroup   * aJoinGroup );

    // Join Group 내에서 Join Order를 결정하는 함수
    static IDE_RC    joinOrderingInJoinGroup( qcStatement    * aStatement,
                                              qmoJoinGroup   * aJoinGroup );

    static idBool    isSemiAntiJoinable( qmgGraph        * aJoinGraph,
                                         qmoJoinRelation * aJoinRelation );

    static idBool    isFeasibleJoinOrder( qmgGraph        * aGraph,
                                          qmoJoinRelation * aJoinRelation );

    static IDE_RC    initJoinGraph( qcStatement     * aStatement,
                                    qmoJoinRelation * aJoinRelation,
                                    qmgGraph        * aLeft,
                                    qmgGraph        * aRight,
                                    qmgGraph        * aJoinGraph,
                                    idBool            aExistOrderFactor );

    static IDE_RC    initJoinGraph( qcStatement     * aStatement,
                                    UInt              aJoinGraphType,
                                    qmgGraph        * aLeft,
                                    qmgGraph        * aRight,
                                    qmgGraph        * aJoinGraph,
                                    idBool            aExistOrderFactor );

    // PROJ-1495
    // join group간의 조인순서 재배치
    static IDE_RC relocateJoinGroup4PushPredHint(
        qcStatement      * aStatement,
        qmsPushPredHints * aPushPredHint,
        qmoJoinGroup    ** aJoinGroup,
        UInt               aJoinGroupCnt );

    // Table Order 정보를 생성하는 함수
    static IDE_RC    getTableOrder( qcStatement    * aStatement,
                                    qmgGraph       * aGraph,
                                    qmoTableOrder ** aTableOrder);

    // Join Ordering, Join Groupint에서 호출
    // Join Predicate에서 해당 Join Graph의 predicate을 분리하여 연결
    static IDE_RC connectJoinPredToJoinGraph(qcStatement   * aStatement,
                                             qmoPredicate ** aJoinGroupPred,
                                             qmgGraph     * aJoinGraph );

    //-----------------------------------------------------
    // Join Grouping 에서 사용하는 함수
    //-----------------------------------------------------

    // BUG-42447 leading hint support
    static void     makeLeadingGroup( qmoCNF         * aCNF,
                                      qmsLeadingHints* aLeadingHints );

    static IDE_RC    joinGroupingWithJoinPred( qmoJoinGroup  * aJoinGroup,
                                               qmoCNF        * aCNF,
                                               UInt          * aJoinGroupCnt );

    // base graph를 해당 Join Group에 연결시켜주는 함수
    static
        IDE_RC    connectBaseGraphToJoinGroup( qmoJoinGroup  * aJoinGroup,
                                               qmgGraph     ** aCNFBaseGraph,
                                               UInt            aCNFBaseGraphCnt );

    //-----------------------------------------------------
    // Join Relation에서 사용하는 함수
    //-----------------------------------------------------

    // 이미 생성된 Join Relation이 있는지 확인
    static idBool    existJoinRelation( qmoJoinRelation * aJoinRelationList,
                                        qcDepInfo       * aDepInfo );

    // Semi/anti join시 inner table의 ID를 얻는 함수
    static IDE_RC    getInnerTable( qcStatement * aStatement,
                                    qtcNode     * aNode,
                                    SInt        * aTableID );

    // Semi/anti join용 Join Relation List를 만들어주는 함수
    static IDE_RC    makeSemiAntiJoinRelationList( qcStatement      * aStatement,
                                                   qtcNode          * aNode,
                                                   qmoJoinRelation ** aJoinRelationList,
                                                   UInt             * aJoinRelationCnt );

    // Semi/anti join시 outer relation간 Join Relation List를 만들어주는 함수
    static IDE_RC    makeCrossJoinRelationList( qcStatement      * aStatement,
                                                qmoJoinGroup     * aJoinGroup,
                                                qmoJoinRelation ** aJoinRelationList,
                                                UInt             * aJoinRelationCnt );

    // Join Relation List를 만들어주는 함수
    static IDE_RC    makeJoinRelationList( qcStatement      * aStatement,
                                           qcDepInfo        * aDependencies,
                                           qmoJoinRelation ** aJoinRelationList,
                                           UInt             * aJoinRelationCnt );

    // 두 개의 conceptual table로만 구성된 Node 여부
    static IDE_RC   isTwoConceptTblNode( qtcNode      * aNode,
                                         qmgGraph    ** aBaseGraph,
                                         UInt           aBaseGraphCnt,
                                         idBool       * aIsTwoConceptTbl);

    // aDependencis와 연관된 두 개의 Graph를 찾아주는 함수
    static IDE_RC    getTwoRelatedGraph( qcDepInfo * aDependencies,
                                         qmgGraph ** aBaseGraph,
                                         UInt        aBaseGraphCnt,
                                         qmgGraph ** aFirstGraph,
                                         qmgGraph ** aSecondGraph );

    //-----------------------------------------------------
    // 그 외 함수에서 호출
    //-----------------------------------------------------

    // pushSelectionOnJoinType() 함수에서 사용
    // push selection할 predicate의 위치
    static IDE_RC    getPushPredPosition( qmoPredicate  * aPredicate,
                                          qmgGraph     ** aBaseGraph,
                                          qcDepInfo     * aFromDependencies,
                                          qmsJoinType     aJoinType,
                                          UInt          * aPredPos );

    // addOneTblPredOnJoinType(), getPushPredPosition() 함수에서 사용
    // Push Selection 가능한 Predicate인지 검사
    static qmoPushApplicableType isPushApplicable( qmoPredicate * aPredicate,
                                                   qmsJoinType    aJoinType );

    // PROJ-2418 
    // Lateral View에 관여된 Predicate을 CNF에서 빼내서 반환
    static IDE_RC    discardLateralViewJoinPred( qmoCNF        * aCNF,
                                                 qmoPredicate ** aDiscardPredList );

    // PROJ-2418
    // Lateral View와 외부 참조하는 Relation과의 Left/Full-Outer Join 검증
    static IDE_RC    validateLateralViewJoin( qcStatement * aStatement,
                                              qmsFrom     * aFrom );
};


#endif /* _O_QMO_CRT_CNF_MGR_H_ */
