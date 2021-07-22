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
 * $Id: qmgLeftOuter.cpp 90785 2021-05-06 07:26:22Z hykim $
 *
 * Description :
 *     LeftOuter Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgLeftOuter.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoTwoNonPlan.h>
#include <qmoSelectivity.h>
#include <qmoParallelPlan.h>
#include <qmgJoin.h>
#include <qmoCostDef.h>
#include <qmv.h>
#include <qmo.h>
#include <qmnLeftOuter.h>

extern mtfModule mtfOr;
extern mtfModule mtfAnd;
extern mtfModule mtfNotBetween;
extern mtfModule mtfNot;
extern mtfModule mtfEqualAny;
extern mtfModule mtfGreaterThanAny;
extern mtfModule mtfGreaterEqualAny;
extern mtfModule mtfLessEqualAny;
extern mtfModule mtfLessThanAny;
extern mtfModule mtfNotEqualAny;
extern mtfModule mtfList;

IDE_RC
qmgLeftOuter::init( qcStatement * aStatement,
                    qmsQuerySet * aQuerySet,
                    qmsFrom     * aFrom,
                    qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgLeftOuter Graph의 초기화
 *
 * Implementation :
 *    (1)  qmgLeftOuter를 위한 공간 할당
 *    (2)  graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3)  graph.type 설정
 *    (4)  graph.myQuerySet을 aQuerySet으로 설정
 *    (5)  graph.myFrom을 aFrom으로 설정
 *    (6)  graph.dependencies 설정
 *    (7)  qmgJoin의 onConditonCNF 처리
 *    (8)  하위graph의 생성 및 초기화
 *    (9)  graph.optimize와 graph.makePlan 설정
 *    (10) out 설정
 *
 ***********************************************************************/

    qmgLOJN   * sMyGraph;
    qtcNode   * sNormalCNF;
    qmoNormalType sNormalType;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );

    //---------------------------------------------------
    // Left Outer Join Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgLeftOuter를 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgLOJN),
                                             (void**) & sMyGraph )
              != IDE_SUCCESS );


    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    // Graph의 종류 표기
    sMyGraph->graph.type = QMG_LEFT_OUTER_JOIN;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );

    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    // Graph의 함수 포인터를 설정
    sMyGraph->graph.optimize = qmgLeftOuter::optimize;
    sMyGraph->graph.makePlan = qmgLeftOuter::makePlan;
    sMyGraph->graph.printGraph = qmgLeftOuter::printGraph;

    //---------------------------------------------------
    // Left Outer Join Graph 만을 위한 자료 구조 초기화
    //---------------------------------------------------

    // on condition CNF를 위한 공간 할당 및 자료 구조 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ),
                                             (void**) & sMyGraph->onConditionCNF )
              != IDE_SUCCESS );

    // To Fix PR-12743 NNF Filter지원
    IDE_TEST( qmoCrtPathMgr::decideNormalType( aStatement,
                                               aFrom,
                                               aFrom->onCondition,
                                               aQuerySet->SFWGH->hints,
                                               ID_TRUE, // CNF Only임
                                               & sNormalType )
              != IDE_SUCCESS );

    switch ( sNormalType )
    {
        case QMO_NORMAL_TYPE_NNF:

            if ( aFrom->onCondition != NULL )
            {
                aFrom->onCondition->lflag &= ~QTC_NODE_NNF_FILTER_MASK;
                aFrom->onCondition->lflag |= QTC_NODE_NNF_FILTER_TRUE;
            }
            else
            {
                // Nothing To Do
            }

            IDE_TEST( qmoCnfMgr::init( aStatement,
                                       sMyGraph->onConditionCNF,
                                       aQuerySet,
                                       aFrom,
                                       NULL,
                                       aFrom->onCondition )
                      != IDE_SUCCESS );
            break;

        case QMO_NORMAL_TYPE_CNF:
            IDE_TEST( qmoNormalForm::normalizeCNF( aStatement,
                                                   aFrom->onCondition,
                                                   & sNormalCNF )
                      != IDE_SUCCESS );

            IDE_TEST( qmoCnfMgr::init( aStatement,
                                       sMyGraph->onConditionCNF,
                                       aQuerySet,
                                       aFrom,
                                       sNormalCNF,
                                       NULL )
                      != IDE_SUCCESS );
            break;

        case QMO_NORMAL_TYPE_DNF:
        case QMO_NORMAL_TYPE_NOT_DEFINED:
        default:
            IDE_FT_ASSERT( 0 );
            break;
    }

    sMyGraph->graph.left = sMyGraph->onConditionCNF->baseGraph[0];
    sMyGraph->graph.right = sMyGraph->onConditionCNF->baseGraph[1];

    // BUG-45296 rownum Pred 을 left outer 의 오른쪽으로 내리면 안됩니다.
    sMyGraph->graph.left->flag &= ~QMG_ROWNUM_PUSHED_MASK;
    sMyGraph->graph.left->flag |= QMG_ROWNUM_PUSHED_TRUE;
    
    sMyGraph->graph.right->flag &= ~QMG_ROWNUM_PUSHED_MASK;
    sMyGraph->graph.right->flag |= QMG_ROWNUM_PUSHED_FALSE;

    // join method 초기화
    sMyGraph->nestedLoopJoinMethod = NULL;
    sMyGraph->sortBasedJoinMethod = NULL;
    sMyGraph->hashBasedJoinMethod = NULL;

    // joinable predicate / non joinable predicate 초기화
    sMyGraph->joinablePredicate = NULL;
    sMyGraph->nonJoinablePredicate = NULL;
    sMyGraph->pushedDownPredicate = NULL;

    // bucket count, hash temp table count 초기화
    sMyGraph->hashBucketCnt = 0;
    sMyGraph->hashTmpTblCnt = 0;

    // Disk/Memory 정보 설정
    switch(  aQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            // 중간 결과 Type Hint가 없는 경우,
            // left 또는 right가 disk이면 disk
            if ( ( ( sMyGraph->graph.left->flag & QMG_GRAPH_TYPE_MASK )
                   == QMG_GRAPH_TYPE_DISK ) ||
                 ( ( sMyGraph->graph.right->flag & QMG_GRAPH_TYPE_MASK )
                   == QMG_GRAPH_TYPE_DISK ) )
            {
                sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
            }
            else
            {
                sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
            }
            break;
        case QMO_INTER_RESULT_TYPE_DISK :
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
            break;
        case QMO_INTER_RESULT_TYPE_MEMORY :
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
            break;
        default :
            IDE_DASSERT( 0 );
            break;
    }

    sMyGraph->joinOrderFactor = 0;
    sMyGraph->joinSize = 0;
    sMyGraph->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;

    // out 설정
    *aGraph = (qmgGraph*)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgLeftOuter Graph의 최적화
 *
 * Implementation :
 *    (0) on Condition으로 Transitive Predicate 생성
 *    (1) on Condition Predicate의 분류
 *    (2) left graph의 최적화 수행
 *    (3) right graph의 최적화 수행
 *    (4) subquery의 처리
 *    (5) Join Method의 초기화
 *    (6) Join Method의 선택
 *    (7) Join Method 결정 후 처리
 *    (8) 공통 비용 정보의 설정
 *    (9) Preserved Order, DISK/MEMORY 설정
 *
 ***********************************************************************/

    qmgLOJN       * sMyGraph;
    qmoJoinMethod * sJoinMethods;
    qmoPredicate  * sLowerPredicate;
    qmoPredicate  * sPredicate;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgLOJN*) aGraph;

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        /* TASK-7219 Non-shard DML */
        IDE_TEST( qmo::removeOutRefPredPushedForce( & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // CHECK NOT ALLOWED JOIN
    //------------------------------------------
    
    // PROJ-2582 recursive with
    IDE_TEST_RAISE( ( ( sMyGraph->graph.right->myQuerySet->lflag &
                        QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                      == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) &&
                    ( ( sMyGraph->graph.right->myFrom->tableRef->flag &
                        QMS_TABLE_REF_RECURSIVE_VIEW_MASK )
                      == QMS_TABLE_REF_RECURSIVE_VIEW_TRUE ),
                    ERR_NOT_ALLOWED_JOIN_RECURSIVE_VIEW );

    //------------------------------------------
    // PROJ-1404 Transitive Predicate 생성
    //------------------------------------------
    
    IDE_TEST( qmoCnfMgr::generateTransitivePred4OnCondition(
                  aStatement,
                  sMyGraph->onConditionCNF,
                  sMyGraph->graph.myPredicate,
                  & sLowerPredicate )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // on Condition Predicate의 분류
    //------------------------------------------

    IDE_TEST( qmoCnfMgr::classifyPred4OnCondition(
                  aStatement,
                  sMyGraph->onConditionCNF,
                  & sMyGraph->graph.myPredicate,
                  sLowerPredicate,
                  sMyGraph->graph.myFrom->joinType )
              != IDE_SUCCESS );

    if ( ( sMyGraph->graph.flag & QMG_JOIN_ONLY_NL_MASK )
           == QMG_JOIN_ONLY_NL_TRUE )
    {
        sMyGraph->graph.left->flag &= ~QMG_JOIN_ONLY_NL_MASK;
        sMyGraph->graph.left->flag |= QMG_JOIN_ONLY_NL_TRUE;
        sMyGraph->graph.right->flag &= ~QMG_JOIN_ONLY_NL_MASK;
        sMyGraph->graph.right->flag |= QMG_JOIN_ONLY_NL_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    // left graph의 최적화 수행
    IDE_TEST( sMyGraph->graph.left->optimize( aStatement,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS );

    /* BUG-47752 left outer join의 on절이 OR 조건으로 구성되고있고
     * 오른쪽 view에 aggregaion이 존재할경우 fatal
     */
    sMyGraph->graph.right->flag &= ~QMG_JOIN_RIGHT_MASK;
    sMyGraph->graph.right->flag |= QMG_JOIN_RIGHT_TRUE;

    // right graph의 최적화 수행
    IDE_TEST( sMyGraph->graph.right->optimize( aStatement,
                                               sMyGraph->graph.right )
              != IDE_SUCCESS );

    // BUG-32703
    // 최적화 시에 view의 graph가 생성된다
    // left outer join의 하위 graph에 view가 있다면
    // 최적화 이후에 ( 즉, view graph가 최적화 된 이후에 )
    // left outer join에 대한 type ( disk or memory )를 새로 설정해야 한다.

    // BUG-40191 __OPTIMIZER_DEFAULT_TEMP_TBS_TYPE 힌트를 고려해야 한다.
    if ( aGraph->myQuerySet->SFWGH->hints->interResultType == QMO_INTER_RESULT_TYPE_NOT_DEFINED )
    {
        if ( ( ( sMyGraph->graph.left->flag & QMG_GRAPH_TYPE_MASK )
               == QMG_GRAPH_TYPE_DISK ) ||
             ( ( sMyGraph->graph.right->flag & QMG_GRAPH_TYPE_MASK )
               == QMG_GRAPH_TYPE_DISK ) )
        {
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
        }
        else
        {
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
        }
    }
    else
    {
        // nothing to do
    }

    // fix BUG-19203
    IDE_TEST( qmoCnfMgr::optimizeSubQ4OnCondition( aStatement,
                                                   sMyGraph->onConditionCNF )
              != IDE_SUCCESS );

    //------------------------------------------
    // Subquery의 Graph 생성
    // - To Fix BUG-10577
    //   Left, Right 최적화 후에 subquery graph를 생성해야 하위가 view일때
    //   view 통계 정보 미구축으로 서버가 사망하는 문제가 발생하지 않는다.
    //   ( = BUG-9736 )
    //   Predicate 분류는 dependencies로 수행하기 때문에 predicate의
    //   Subquery graph 생성 전에 수행해도 된다.
    //------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sMyGraph->graph.myPredicate,
                                               ID_FALSE )  // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    //------------------------------------------
    // on Condition CNF의 Join Predicate의 분류
    //------------------------------------------

    if ( sMyGraph->onConditionCNF->joinPredicate != NULL )
    {
        IDE_TEST(
            qmoPred::classifyJoinPredicate(
                aStatement,
                sMyGraph->onConditionCNF->joinPredicate,
                sMyGraph->graph.left,
                sMyGraph->graph.right,
                & sMyGraph->graph.depInfo )
            != IDE_SUCCESS );

        // To fix BUG-26885
        // DNF의 join이 되는 경우 materialize로 인해 predicate
        // 의 참조튜플 위치가 바뀔 수 있으므로
        // join predicate를 복사한다.
        if( aGraph->myQuerySet->SFWGH->crtPath->currentNormalType
            == QMO_NORMAL_TYPE_DNF )
        {   
            IDE_TEST(qmoPred::copyPredicate4Partition(
                         aStatement,
                         sMyGraph->onConditionCNF->joinPredicate,
                         &sPredicate,
                         0,
                         0,
                         ID_TRUE )
                     != IDE_SUCCESS );
        
            sMyGraph->onConditionCNF->joinPredicate = sPredicate;
        }
        else
        {
            // CNF이거나 NNF인 경우 복사할 필요가 없다.
            // Nothing To Do
        }
    }
    else
    {
        // nothing to do
    }

    //------------------------------------------
    // selectivity 계산 
    //------------------------------------------

    IDE_TEST( qmoSelectivity::setOuterJoinSelectivity(
                  aStatement,
                  aGraph,
                  sMyGraph->graph.myPredicate,
                  sMyGraph->onConditionCNF->joinPredicate,
                  sMyGraph->onConditionCNF->oneTablePredicate,
                  & sMyGraph->graph.costInfo.selectivity )
              != IDE_SUCCESS );

    //------------------------------------------
    // Join Method의 초기화
    //------------------------------------------
    if ( ( sMyGraph->graph.flag & QMG_JOIN_ONLY_NL_MASK )
         == QMG_JOIN_ONLY_NL_FALSE )
    {
        // Join Method 공간 할당
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethod ) * 3,
                                                 (void **) &sJoinMethods )
                  != IDE_SUCCESS );
        sMyGraph->nestedLoopJoinMethod = & sJoinMethods[0];
        // To Fix BUG-9156
        sMyGraph->hashBasedJoinMethod  = & sJoinMethods[1];
        sMyGraph->sortBasedJoinMethod  = & sJoinMethods[2];

        // nested loop join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->onConditionCNF->joinPredicate,
                                          QMO_JOIN_METHOD_NL,
                                          sMyGraph->nestedLoopJoinMethod )
                  != IDE_SUCCESS );

        // hash based join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->onConditionCNF->joinPredicate,
                                          QMO_JOIN_METHOD_HASH,
                                          sMyGraph->hashBasedJoinMethod )
                  != IDE_SUCCESS );

        // sort based join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->onConditionCNF->joinPredicate,
                                          QMO_JOIN_METHOD_SORT,
                                          sMyGraph->sortBasedJoinMethod )
                  != IDE_SUCCESS );
    }
    else
    {
        IDU_FIT_POINT("qmgLeftOuter::optimize::alloc::joinMethods",
                      idERR_ABORT_InsufficientMemory);
        // Join Method 공간 할당
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethod ),
                                                 (void **) &sJoinMethods )
                  != IDE_SUCCESS );

        sMyGraph->nestedLoopJoinMethod = & sJoinMethods[0];
        sMyGraph->hashBasedJoinMethod  = NULL;
        sMyGraph->sortBasedJoinMethod  = NULL;

        // nested loop join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->onConditionCNF->joinPredicate,
                                          QMO_JOIN_METHOD_NL,
                                          sMyGraph->nestedLoopJoinMethod )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // 각 Join Method 중 가장 좋은 cost의 Join Method를 선택
    //------------------------------------------

    IDE_TEST(
        qmgJoin::selectJoinMethodCost( aStatement,
                                       & sMyGraph->graph,
                                       sMyGraph->onConditionCNF->joinPredicate,
                                       sJoinMethods,
                                       & sMyGraph->selectedJoinMethod )
        != IDE_SUCCESS );

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // record size 결정
    sMyGraph->graph.costInfo.recordSize =
        sMyGraph->graph.left->costInfo.recordSize +
        sMyGraph->graph.right->costInfo.recordSize;

    // 각 qmgJoin 의 joinOrderFactor, joinSize 계산
    IDE_TEST( qmoSelectivity::setJoinOrderFactor(
                  aStatement,
                  & sMyGraph->graph,
                  sMyGraph->onConditionCNF->joinPredicate,
                  &sMyGraph->joinOrderFactor,
                  &sMyGraph->joinSize )
              != IDE_SUCCESS );

    // input record count 설정
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt *
        sMyGraph->graph.right->costInfo.outputRecordCnt;

    // output record count 설정
    IDE_TEST( qmoSelectivity::setLeftOuterOutputCnt(
                  aStatement,
                  aGraph,
                  & sMyGraph->graph.left->depInfo,
                  sMyGraph->graph.myPredicate,
                  sMyGraph->onConditionCNF->joinPredicate,
                  sMyGraph->onConditionCNF->oneTablePredicate,
                  sMyGraph->graph.left->costInfo.outputRecordCnt,
                  sMyGraph->graph.right->costInfo.outputRecordCnt,
                  & sMyGraph->graph.costInfo.outputRecordCnt )
              != IDE_SUCCESS );

    // My Cost 계산
    sMyGraph->graph.costInfo.myAccessCost =
        sMyGraph->selectedJoinMethod->accessCost;
    sMyGraph->graph.costInfo.myDiskCost =
        sMyGraph->selectedJoinMethod->diskCost;
    sMyGraph->graph.costInfo.myAllCost =
        sMyGraph->selectedJoinMethod->totalCost;

    // Total Cost 계산
    // Join Graph 자체의 Cost는 이미 Child의 Cost를 모두 포함하고 있다.
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->graph.costInfo.myAccessCost;
    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->graph.costInfo.myDiskCost;
    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->graph.costInfo.myAllCost;

    //------------------------------------------
    // Join Method의 결정 후 처리
    //------------------------------------------

    IDE_TEST(
        qmgJoin::afterJoinMethodDecision(
            aStatement,
            & sMyGraph->graph,
            sMyGraph->selectedJoinMethod,
            & sMyGraph->onConditionCNF->joinPredicate,
            & sMyGraph->joinablePredicate,
            & sMyGraph->nonJoinablePredicate)
        != IDE_SUCCESS );
    
    // PROJ-1404
    // 생성된 transitive predicate이 one table predicate인 경우
    // 항상 filter로 처리되므로 bad transitive predicate이다.
    IDE_TEST( qmoPred::removeTransitivePredicate(
                  & sMyGraph->onConditionCNF->oneTablePredicate,
                  ID_FALSE )
              != IDE_SUCCESS );

    //------------------------------------------
    // Preserved Order
    //------------------------------------------

    IDE_TEST( qmgJoin::makePreservedOrder( aStatement,
                                           & sMyGraph->graph,
                                           sMyGraph->selectedJoinMethod,
                                           sMyGraph->joinablePredicate )
              != IDE_SUCCESS );

    // BUG-38279 Applying parallel query
    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
    aGraph->flag |= ((aGraph->left->flag & QMG_PARALLEL_EXEC_MASK) |
                     (aGraph->right->flag & QMG_PARALLEL_EXEC_MASK));

    // PROJ-2750
    if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_LEFT_OUTER_SKIP_RIGHT_MASK )
         == QC_TMP_LEFT_OUTER_SKIP_RIGHT_TRUE )
    {
        IDE_TEST( checkAndSetSkipRight( &sMyGraph->graph ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED_JOIN_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOWED_JOIN_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC
qmgLeftOuter::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgLeftOuter로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgLeftOuter로 부터 생성가능한 Plan
 *
 *     1.  Nested Loop 계열
 *         1.1  Full Nested Loop Join의 경우
 *
 *                 ( [FILT] )  : WHERE 절로부터 얻어진 Predicate
 *                     |
 *                   [LOJN]
 *                   |    |
 *                        [SCAN]  : SCAN에 포함됨
 *
 *         1.2  Full Store Nested Loop Join의 경우
 *
 *                 ( [FILT] )  : WHERE 절로부터 얻어진 Predicate
 *                     |
 *                   [LOJN]
 *                   |    |
 *                        [SORT]
 *
 *         1.3  Index Nested Loop Join의 경우
 *
 *                 ( [FILT] )  : WHERE 절로부터 얻어진 Predicate
 *                     |
 *                   [LOJN]
 *                   |    |
 *                        [SCAN]
 *
 *         1.4  Anti-Outer Nested Loop Join의 경우
 *              - qmgFullOuter로부터만 생성됨.
 *
 *     2.  Sort-based 계열
 *
 *                      |
 *                  ( [FILT] )  : WHERE 절로부터 얻어진 Predicate
 *                      |
 *                    [LOJN]
 *                    |    |
 *              ([SORT])   [SORT]
 *
 *         - Two-Pass Sort Join인 경우, Left에 SORT가 구성되나
 *           Preserved Order가 있다면 Two-Pass Sort Join이라 하더라도
 *           Left에 SORT를 생성하지 않는다.
 *
 *     3.  Hash-based 계열
 *
 *                      |
 *                  ( [FILT] )  : WHERE 절로부터 얻어진 Predicate
 *                      |
 *                    [LOJN]
 *                    |    |
 *              ([HASH])   [HASH]
 *
 *         - Two-Pass Hash Join인 경우, Left에 HASH가 구성됨
 *
 ***********************************************************************/

    qmgLOJN         * sMyGraph;
    
    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgLOJN*) aGraph;

    //---------------------------------------------------
    // Current CNF의 등록
    //---------------------------------------------------
    if ( sMyGraph->graph.myCNF != NULL )
    {
        sMyGraph->graph.myQuerySet->SFWGH->crtPath->currentCNF =
            sMyGraph->graph.myCNF;
    }
    else
    {
        // Nothing To Do
    }

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // SCAN parallel flag 를 자식 노드로 물려준다.
    aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);
    aGraph->right->flag |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

    sMyGraph->graph.myPlan = aParent->myPlan;

    switch( sMyGraph->selectedJoinMethod->flag & QMO_JOIN_METHOD_MASK )
    {
        case QMO_JOIN_METHOD_ONE_PASS_HASH:
            IDE_TEST( makeHashJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE,   // TwoPass?
                                    ID_FALSE )  // Inverse?
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INVERSE_HASH:
            IDE_TEST( makeHashJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE,   // TwoPass?
                                    ID_TRUE )   // Inverse?
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_TWO_PASS_HASH:
            IDE_TEST( makeHashJoin( aStatement,
                                    sMyGraph,
                                    ID_TRUE,    // TwoPass?
                                    ID_FALSE )  // Inverse?
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_ONE_PASS_SORT:
            IDE_TEST( makeSortJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE )   // TwoPass?
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_TWO_PASS_SORT:
            IDE_TEST( makeSortJoin( aStatement,
                                    sMyGraph,
                                    ID_TRUE )    // TwoPass?
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_FULL_STORE_NL:
            IDE_TEST( makeNestedLoopJoin(
                          aStatement,
                          sMyGraph,
                          (sMyGraph->selectedJoinMethod->flag & 
                           QMO_JOIN_METHOD_MASK) )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INDEX:
        case QMO_JOIN_METHOD_FULL_NL:
            IDE_TEST( makeNestedLoopJoin(
                          aStatement,
                          sMyGraph,
                          sMyGraph->selectedJoinMethod->flag & QMO_JOIN_METHOD_MASK )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_ANTI:
            //없음
        default:
            IDE_FT_ASSERT( 0 );
            break;
    }

    // PROJ-2750
    if ( ( aGraph->flag & QMG_LOJN_SKIP_RIGHT_COND_MASK ) == QMG_LOJN_SKIP_RIGHT_COND_TRUE )
    {
        IDE_TEST( checkAndSetSkipRightFlag( aGraph->myPlan ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeSortRange( qcStatement  * aStatement,
                             qmgLOJN      * aMyGraph,
                             qtcNode     ** aRange )
{
    qtcNode         * sCNFRange;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeSortRange::__FT__" );

    //---------------------------------------------------
    // SORT를 위한 range , HASH를 위한 filter 생성
    //---------------------------------------------------
    if( aMyGraph->joinablePredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement ,
                                          aMyGraph->joinablePredicate ,
                                          &sCNFRange ) != IDE_SUCCESS );

        // To Fix PR-8019
        // Key Range 생성을 위해서는 DNF형태로 변환하여야 한다.
        IDE_TEST( qmoNormalForm::normalizeDNF( aStatement ,
                                               sCNFRange ,
                                               aRange ) != IDE_SUCCESS );

        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           *aRange ) != IDE_SUCCESS );
    }
    else
    {
        *aRange = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeHashFilter( qcStatement  * aStatement,
                              qmgLOJN      * aMyGraph,
                              qtcNode     ** aFilter )
{
    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeHashFilter::__FT__" );

    IDE_TEST( makeSortRange( aStatement, aMyGraph, aFilter ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeChildPlan( qcStatement     * aStatement,
                             qmgLOJN         * aMyGraph,
                             qmnPlan         * aLeftPlan,
                             qmnPlan         * aRightPlan )
{
    //---------------------------------------------------
    // 하위 Plan의 생성 , Join이므로 양쪽 모두 실행
    //---------------------------------------------------

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeChildPlan::__FT__" );

    aMyGraph->graph.myPlan = aLeftPlan;

    IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                              &aMyGraph->graph ,
                                              aMyGraph->graph.left )
              != IDE_SUCCESS);

    aMyGraph->graph.myPlan = aRightPlan;

    IDE_TEST( aMyGraph->graph.right->makePlan( aStatement ,
                                               &aMyGraph->graph ,
                                               aMyGraph->graph.right )
              != IDE_SUCCESS);

    //---------------------------------------------------
    // Process 상태 설정 
    //---------------------------------------------------
    aMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_JOIN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeNestedLoopJoin( qcStatement * aStatement,
                                  qmgLOJN     * aMyGraph,
                                  UInt          aJoinType )
{
    UInt              sMtrFlag;
    UInt              sJoinFlag;
    qmnPlan         * sFILT = NULL;
    qmnPlan         * sLOJN = NULL;
    qmnPlan         * sSORT = NULL;
    qmoPredicate    * sPredicate[3];
    qtcNode         * sFilter = NULL;
    qtcNode         * sJoinFilter= NULL;
    qmsQuerySet     * sViewQuerySet;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeNestedLoopJoin::__FT__" );
    
    // 다음의 3가지 nested loop join을 생성한다.
    // 1. Full nested loop join
    // 2. Full store nested loop join
    // 3. Index nested loop join

    //---------------------------------------------------
    //       [*FILT]
    //          |
    //       [LOJN]
    //         /`.
    //   [LEFT]  [*SORT]
    //              |
    //           [RIGHT]
    // * Option
    //---------------------------------------------------

    //-----------------------
    // Top-down 초기화
    //-----------------------

    //-----------------------
    // init FILT
    //-----------------------

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFilter,
                        &sFILT )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // init LOJN
    //-----------------------

    sPredicate[0] = aMyGraph->onConditionCNF->constantPredicate;
    sPredicate[1] = aMyGraph->onConditionCNF->oneTablePredicate;
    if( aJoinType == QMO_JOIN_METHOD_INDEX )
    {
        // Index nested loop join인 경우
        sPredicate[2] = aMyGraph->nonJoinablePredicate;
    }
    else
    {
        // 그 외 full nested/full stored nested loop join인 경우
        sPredicate[2] = aMyGraph->onConditionCNF->joinPredicate;
    }

    IDE_TEST(
        qmg::makeOuterJoinFilter( aStatement ,
                                  aMyGraph->graph.myQuerySet ,
                                  sPredicate ,
                                  aMyGraph->onConditionCNF->nnfFilter,
                                  &sJoinFilter ) != IDE_SUCCESS );

    // BUG-47414
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if ( ( aJoinType == QMO_JOIN_METHOD_FULL_NL ) &&
         (aMyGraph->graph.right->type == QMG_SELECTION ) &&
         (aMyGraph->graph.right->myFrom->tableRef->view != NULL) )
    {
        // left 는 view 의 projection
        sViewQuerySet = aMyGraph->graph.right->left->myQuerySet;
    }
    else
    {
        sViewQuerySet = NULL;
    }

    IDE_TEST( qmoTwoNonPlan::initLOJN(
                  aStatement ,
                  aMyGraph->graph.myQuerySet ,
                  sViewQuerySet,
                  NULL,
                  sJoinFilter,
                  aMyGraph->pushedDownPredicate,
                  aMyGraph->graph.myPlan,
                  &sLOJN ) != IDE_SUCCESS);

    aMyGraph->graph.myPlan = sLOJN;

    if( aJoinType == QMO_JOIN_METHOD_FULL_STORE_NL )
    {
        //---------------------------------------------------
        // Full Store Nested Loop Join의 경우
        //---------------------------------------------------

        //-----------------------
        // init SORT
        //-----------------------

        sJoinFlag = 0;

        sJoinFlag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

        IDE_TEST( qmg::initRightSORT4Join( aStatement,
                                           &aMyGraph->graph,
                                           sJoinFlag,
                                           ID_FALSE,
                                           NULL,
                                           ID_FALSE,
                                           aMyGraph->graph.myPlan,
                                           &sSORT )
                  != IDE_SUCCESS );

        // BUG-38410
        // Sort 되므로 right 는 한번만 실행되고 sort 된 내용을 참조한다.
        // 따라서 right 는 SCAN 에 대한 parallel 을 허용한다.
        aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
        aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;
    }
    else
    {
        sSORT = aMyGraph->graph.myPlan;

        // BUG-38410
        // 반복 실행 될 경우 SCAN Parallel 을 금지한다.
        aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
        aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_TRUE;
    }

    //-----------------------
    // 하위 plan 생성
    //-----------------------
    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph,
                             sLOJN,
                             sSORT )
              != IDE_SUCCESS );

    //-----------------------
    // Bottom-up 생성
    //-----------------------

    if( aJoinType == QMO_JOIN_METHOD_FULL_STORE_NL )
    {
        //---------------------------------------------------
        // Full Store Nested Loop Join의 경우
        //---------------------------------------------------

        //----------------------------
        // make SORT
        //----------------------------

        sMtrFlag = 0;
        sMtrFlag &= ~QMO_MAKESORT_METHOD_MASK;
        sMtrFlag |= QMO_MAKESORT_STORE;

        sMtrFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
        sMtrFlag |= QMO_MAKESORT_PRESERVED_FALSE;

        //저장 매체의 선택
        sJoinFlag = 0;

        sJoinFlag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

        IDE_TEST( qmg::makeRightSORT4Join( aStatement,
                                           &aMyGraph->graph,
                                           sMtrFlag,
                                           sJoinFlag,
                                           ID_FALSE,
                                           aMyGraph->graph.right->myPlan,
                                           sSORT )
                  != IDE_SUCCESS );
    }
    else
    {
        sSORT = aMyGraph->graph.right->myPlan;
    }

    //-----------------------
    // make LOJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeLOJN(
                  aStatement ,
                  aMyGraph->graph.myQuerySet ,
                  sJoinFilter ,
                  aMyGraph->graph.left->myPlan,
                  sSORT,
                  sLOJN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sLOJN;

    qmg::setPlanInfo( aStatement, sLOJN, &(aMyGraph->graph) );

    switch( aJoinType )
    {
        case QMO_JOIN_METHOD_FULL_NL :
            sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_FULL_NL;
            break;

        case QMO_JOIN_METHOD_INDEX :
            sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_INDEX;
            break;

        case QMO_JOIN_METHOD_FULL_STORE_NL :
            sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_FULL_STORE_NL;
            break;

        default :
            IDE_DASSERT(0);
            break;
    }

    //-----------------------
    // make FILT
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFilter,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeHashJoin( qcStatement    * aStatement,
                            qmgLOJN        * aMyGraph,
                            idBool           aIsTwoPass,
                            idBool           aIsInverse )
{
    UInt              sMtrFlag;
    UInt              sJoinFlag;
    qmnPlan         * sFILT = NULL;
    qmnPlan         * sLOJN = NULL;
    qmnPlan         * sLeftHASH = NULL;
    qmnPlan         * sRightHASH = NULL;
    qmoPredicate    * sPredicate[3];
    qtcNode         * sFilter = NULL;
    qtcNode         * sJoinFilter = NULL;
    qtcNode         * sHashFilter = NULL;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeHashJoin::__FT__" );

    //---------------------------------------------------
    //       [*FILT]
    //          |
    //       [LOJN]
    //        /  `.
    //   [*HASH]  [HASH]
    //      |        |
    //   [LEFT]   [RIGHT]
    // * Option
    //---------------------------------------------------

    IDE_TEST( makeHashFilter( aStatement,
                              aMyGraph,
                              &sHashFilter )
              != IDE_SUCCESS );

    //-----------------------
    // Top-down 초기화
    //-----------------------

    //-----------------------
    // init FILT
    //-----------------------

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFilter,
                        &sFILT )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // make LOJN filter
    //-----------------------
    sPredicate[0] = aMyGraph->onConditionCNF->constantPredicate;
    sPredicate[1] = aMyGraph->onConditionCNF->oneTablePredicate;
    sPredicate[2] = aMyGraph->nonJoinablePredicate;
    IDE_TEST(
        qmg::makeOuterJoinFilter( aStatement ,
                                  aMyGraph->graph.myQuerySet ,
                                  sPredicate,
                                  aMyGraph->onConditionCNF->nnfFilter,
                                  &sJoinFilter ) != IDE_SUCCESS);

    //-----------------------
    // init LOJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::initLOJN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       NULL,
                                       sHashFilter,
                                       sJoinFilter,
                                       aMyGraph->graph.myPredicate,
                                       aMyGraph->graph.myPlan,
                                       &sLOJN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sLOJN;

    //-----------------------
    // init right HASH
    //-----------------------
    qmc::disableSealTrueFlag( aMyGraph->graph.myPlan->resultDesc );
    IDE_TEST( qmoOneMtrPlan::initHASH(
                  aStatement ,
                  aMyGraph->graph.myQuerySet ,
                  sHashFilter,
                  ID_FALSE, // isLeftHash
                  ID_FALSE, // isDistinct
                  aMyGraph->graph.myPlan,
                  &sRightHASH ) != IDE_SUCCESS);

    // BUG-38410
    // Hash 되므로 right 는 한번만 실행되고 sort 된 내용만 참조한다.
    // 따라서 right 는 SCAN 에 대한 parallel 을 허용한다.
    aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    if( aIsTwoPass == ID_TRUE )
    {
        // Two pass hash join인 경우

        //-----------------------
        // init left HASH
        //-----------------------
        IDE_TEST( qmoOneMtrPlan::initHASH(
                      aStatement,
                      aMyGraph->graph.myQuerySet,
                      sHashFilter,
                      ID_TRUE,  // isLeftHash
                      ID_FALSE, // isDistinct
                      aMyGraph->graph.myPlan,
                      &sLeftHASH )
                  != IDE_SUCCESS );

        // BUG-38410
        // Hash 되므로 left 는 한번만 실행되고 hash 된 내용만 참조한다.
        // 따라서 left 는 SCAN 에 대한 parallel 을 허용한다.
        aMyGraph->graph.left->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
        aMyGraph->graph.left->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;
    }
    else
    {
        // One pass hash join인 경우
        // left child의 parent로 join을 설정한다.
        sLeftHASH = aMyGraph->graph.myPlan;
    }

    //-----------------------
    // 하위 plan 생성
    //-----------------------
    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph,
                             sLeftHASH,
                             sRightHASH )
              != IDE_SUCCESS );

    //-----------------------
    // Bottom-up 생성
    //-----------------------

    if( aIsTwoPass == ID_TRUE )
    {
        //-----------------------
        // make left HASH
        //-----------------------

        sMtrFlag = 0;
        sMtrFlag &= ~QMO_MAKEHASH_METHOD_MASK;
        sMtrFlag |= QMO_MAKEHASH_HASH_BASED_JOIN;

        // To Fix PR-8032
        // HASH가 사용되는 위치를 표기해야함.
        sMtrFlag &= ~QMO_MAKEHASH_POSITION_MASK;
        sMtrFlag |= QMO_MAKEHASH_POSITION_LEFT;

        //저장 매체의 선택
        sJoinFlag = 0;

        sJoinFlag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_METHOD_LEFT_STORAGE_MASK);

        IDE_TEST( qmg::makeLeftHASH4Join( aStatement,
                                          &aMyGraph->graph,
                                          sMtrFlag,
                                          sJoinFlag,
                                          sHashFilter,
                                          aMyGraph->graph.left->myPlan,
                                          sLeftHASH )
                  != IDE_SUCCESS);
    }
    else
    {
        // One pass hash join인 경우
        // Join의 left child로 left graph의 plan을 설정한다.
        sLeftHASH = aMyGraph->graph.left->myPlan;
    }

    //-----------------------
    // make right HASH
    //-----------------------

    sMtrFlag = 0;
    sMtrFlag &= ~QMO_MAKEHASH_METHOD_MASK;
    sMtrFlag |= QMO_MAKEHASH_HASH_BASED_JOIN;

    // To Fix PR-8032
    // HASH가 사용되는 위치를 표기해야함.
    sMtrFlag &= ~QMO_MAKEHASH_POSITION_MASK;
    sMtrFlag |= QMO_MAKEHASH_POSITION_RIGHT;

    //저장 매체의 선택
    sJoinFlag = 0;

    sJoinFlag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
    sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                  QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    IDE_TEST( qmg::makeRightHASH4Join( aStatement,
                                       &aMyGraph->graph,
                                       sMtrFlag,
                                       sJoinFlag,
                                       sHashFilter,
                                       aMyGraph->graph.right->myPlan,
                                       sRightHASH )
              != IDE_SUCCESS);

    //-----------------------
    // make LOJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeLOJN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sJoinFilter ,
                                       sLeftHASH,
                                       sRightHASH,
                                       sLOJN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sLOJN;

    qmg::setPlanInfo( aStatement, sLOJN, &(aMyGraph->graph) );

    if( aIsTwoPass == ID_TRUE )
    {
        sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_TWO_PASS_HASH;
    }
    else
    {
        /* PROJ-2339 */
        if ( aIsInverse == ID_TRUE )
        {
            sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_INVERSE_HASH;
        }
        else
        {
            sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_ONE_PASS_HASH;
        }
    }

    //-----------------------
    // make FILT
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFilter,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeSortJoin( qcStatement    * aStatement,
                            qmgLOJN        * aMyGraph,
                            idBool           aIsTwoPass )
{
    UInt              sMtrFlag;
    UInt              sJoinFlag;
    qmnPlan         * sFILT = NULL;
    qmnPlan         * sLOJN = NULL;
    qmnPlan         * sLeftSORT = NULL;
    qmnPlan         * sRightSORT = NULL;
    qmoPredicate    * sPredicate[3];
    qtcNode         * sFilter = NULL;
    qtcNode         * sJoinFilter = NULL;
    qtcNode         * sSortRange = NULL;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeSortJoin::__FT__" );

    //---------------------------------------------------
    //       [*FILT]
    //          |
    //       [LOJN]
    //        /  `.
    //   [*SORT]  [SORT]
    //      |        |
    //   [LEFT]   [RIGHT]
    // * Option
    //---------------------------------------------------

    IDE_TEST( makeSortRange( aStatement,
                             aMyGraph,
                             &sSortRange )
              != IDE_SUCCESS );

    //-----------------------
    // Top-down 초기화
    //-----------------------

    //-----------------------
    // init FILT
    //-----------------------

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFilter,
                        &sFILT )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // make LOJN filter
    //-----------------------
    sPredicate[0] = aMyGraph->onConditionCNF->constantPredicate;
    sPredicate[1] = aMyGraph->onConditionCNF->oneTablePredicate;
    sPredicate[2] = aMyGraph->nonJoinablePredicate;
    IDE_TEST(
        qmg::makeOuterJoinFilter( aStatement ,
                                  aMyGraph->graph.myQuerySet ,
                                  sPredicate,
                                  aMyGraph->onConditionCNF->nnfFilter,
                                  &sJoinFilter ) != IDE_SUCCESS);

    //-----------------------
    // init LOJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::initLOJN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       NULL,
                                       sSortRange,
                                       sJoinFilter,
                                       aMyGraph->graph.myPredicate,
                                       aMyGraph->graph.myPlan,
                                       &sLOJN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sLOJN;

    //-----------------------
    // init right SORT
    //-----------------------

    sJoinFlag = 0;

    sJoinFlag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
    sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                  QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    sJoinFlag &= ~QMO_JOIN_RIGHT_NODE_MASK;
    sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                  QMO_JOIN_RIGHT_NODE_MASK);

    IDE_TEST( qmg::initRightSORT4Join( aStatement,
                                       &aMyGraph->graph,
                                       sJoinFlag,
                                       ID_TRUE,
                                       sSortRange,
                                       ID_TRUE,
                                       aMyGraph->graph.myPlan,
                                       &sRightSORT )
              != IDE_SUCCESS);

    if( aIsTwoPass == ID_TRUE )
    {
        // Two pass sort join인 경우

        //-----------------------
        // init left SORT
        //-----------------------
        sJoinFlag = 0;

        sJoinFlag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_METHOD_LEFT_STORAGE_MASK);

        sJoinFlag &= ~QMO_JOIN_LEFT_NODE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_LEFT_NODE_MASK);

        IDE_TEST( qmg::initLeftSORT4Join( aStatement,
                                          &aMyGraph->graph,
                                          sJoinFlag,
                                          sSortRange,
                                          aMyGraph->graph.myPlan,
                                          &sLeftSORT )
                  != IDE_SUCCESS);

        // BUG-38410
        // Sort 되므로 left 는 한번만 실행되고 sort 된 내용만 참조한다.
        // 따라서 left 는 SCAN 에 대한 parallel 을 허용한다.
        aMyGraph->graph.left->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
        aMyGraph->graph.left->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;
    }
    else
    {
        // One pass sort join인 경우
        // left child의 parent로 join을 설정한다.
        sLeftSORT = aMyGraph->graph.myPlan;
    }

    // BUG-38410
    // Sort 되므로 right 는 한번만 실행되고 sort 된 내용만 참조한다.
    // 따라서 right 는 SCAN 에 대한 parallel 을 허용한다.
    aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph,
                             sLeftSORT,
                             sRightSORT )
              != IDE_SUCCESS );

    //-----------------------
    // Bottom-up 생성
    //-----------------------

    if( aIsTwoPass == ID_TRUE )
    {
        //-----------------------
        // make left SORT
        //-----------------------

        sMtrFlag = 0;

        sMtrFlag &= ~QMO_MAKESORT_METHOD_MASK;
        sMtrFlag |= QMO_MAKESORT_SORT_BASED_JOIN;

        sMtrFlag &= ~QMO_MAKESORT_POSITION_MASK;
        sMtrFlag |= QMO_MAKESORT_POSITION_LEFT;

        //저장 매체의 선택
        sJoinFlag = 0;

        sJoinFlag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_METHOD_LEFT_STORAGE_MASK);

        sJoinFlag &= ~QMO_JOIN_LEFT_NODE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_LEFT_NODE_MASK);

        IDE_TEST( qmg::makeLeftSORT4Join( aStatement,
                                          &aMyGraph->graph,
                                          sMtrFlag,
                                          sJoinFlag,
                                          aMyGraph->graph.left->myPlan,
                                          sLeftSORT )
                  != IDE_SUCCESS);
    }
    else
    {
        // One pass sort join인 경우
        // Join의 left child로 left graph의 plan을 설정한다.
        sLeftSORT = aMyGraph->graph.left->myPlan;
    }

    //-----------------------
    // make right SORT
    //-----------------------

    sMtrFlag = 0;

    sMtrFlag &= ~QMO_MAKESORT_METHOD_MASK;
    sMtrFlag |= QMO_MAKESORT_SORT_BASED_JOIN;

    sMtrFlag &= ~QMO_MAKESORT_POSITION_MASK;
    sMtrFlag |= QMO_MAKESORT_POSITION_RIGHT;

    //저장 매체의 선택
    sJoinFlag = 0;

    sJoinFlag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
    sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                  QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    sJoinFlag &= ~QMO_JOIN_RIGHT_NODE_MASK;
    sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                  QMO_JOIN_RIGHT_NODE_MASK);

    IDE_TEST( qmg::makeRightSORT4Join( aStatement,
                                       &aMyGraph->graph,
                                       sMtrFlag,
                                       sJoinFlag,
                                       ID_TRUE,
                                       aMyGraph->graph.right->myPlan,
                                       sRightSORT )
              != IDE_SUCCESS);

    //-----------------------
    // make LOJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeLOJN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sJoinFilter ,
                                       sLeftSORT,
                                       sRightSORT,
                                       sLOJN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sLOJN;

    qmg::setPlanInfo( aStatement, sLOJN, &(aMyGraph->graph) );

    if( aIsTwoPass == ID_TRUE )
    {
        sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_TWO_PASS_SORT;
    }
    else
    {
        sLOJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sLOJN->flag       |= QMN_PLAN_JOIN_METHOD_ONE_PASS_SORT;
    }

    //-----------------------
    // make FILT
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFilter,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

IDE_RC
qmgLeftOuter::initFILT( qcStatement  * aStatement,
                        qmgLOJN      * aMyGraph,
                        qtcNode     ** aFilter,
                        qmnPlan     ** aFILT )
{
    IDU_FIT_POINT_FATAL( "qmgLeftOuter::initFILT::__FT__" );

    // fix BUG-9791 BUG-10419
    // constant filter만 존재하거나
    // myPredicate만 존재하는 경우, 모두 FILT node가 생성되어야 한다.
    if( ( aMyGraph->graph.myPredicate != NULL ) ||
        ( aMyGraph->graph.constantPredicate != NULL ) ||
        ( aMyGraph->graph.nnfFilter != NULL ) )
    {
        if( aMyGraph->graph.myPredicate != NULL )
        {
            IDE_TEST( qmoPred::linkFilterPredicate(
                          aStatement ,
                          aMyGraph->graph.myPredicate ,
                          aFilter ) != IDE_SUCCESS );

            // BUG-35155 Partial CNF
            if ( aMyGraph->graph.nnfFilter != NULL )
            {
                IDE_TEST( qmoPred::addNNFFilter4linkedFilter(
                              aStatement,
                              aMyGraph->graph.nnfFilter,
                              aFilter )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( aMyGraph->graph.nnfFilter != NULL )
            {
                *aFilter = aMyGraph->graph.nnfFilter;
            }
            else
            {
                *aFilter = NULL;
            }
        }

        if ( *aFilter != NULL )
        {
            IDE_TEST( qmoPred::setPriorNodeID(
                          aStatement ,
                          aMyGraph->graph.myQuerySet ,
                          *aFilter ) != IDE_SUCCESS );
        }
        else
        {
            *aFilter = NULL;
        }

        //make FILT
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      *aFilter ,
                      aMyGraph->graph.myPlan,
                      aFILT) != IDE_SUCCESS);
    }
    else
    {
        *aFILT = aMyGraph->graph.myPlan;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::makeFILT( qcStatement * aStatement,
                        qmgLOJN     * aMyGraph,
                        qtcNode     * aFilter,
                        qmnPlan     * aFILT )
{
    IDU_FIT_POINT_FATAL( "qmgLeftOuter::makeFILT::__FT__" );

    if( ( aMyGraph->graph.myPredicate != NULL ) ||
        ( aMyGraph->graph.constantPredicate != NULL ) ||
        ( aMyGraph->graph.nnfFilter != NULL ) )
    {

        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement,
                      aMyGraph->graph.myQuerySet,
                      aFilter,
                      aMyGraph->graph.constantPredicate,
                      aMyGraph->graph.myPlan,
                      aFILT) != IDE_SUCCESS );
        aMyGraph->graph.myPlan = aFILT;

        qmg::setPlanInfo( aStatement, aFILT, &(aMyGraph->graph) );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgLeftOuter::printGraph( qcStatement  * aStatement,
                          qmgGraph     * aGraph,
                          ULong          aDepth,
                          iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *    Graph를 구성하는 공통 정보를 출력한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  i;

    qmgLOJN * sMyGraph;

    qmoPredicate * sPredicate;
    qmoPredicate * sMorePred;

    IDU_FIT_POINT_FATAL( "qmgLeftOuter::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    sMyGraph = (qmgLOJN*) aGraph;

    //-----------------------------------
    // Graph 공통 정보의 출력
    //-----------------------------------

    IDE_TEST( qmg::printGraph( aStatement,
                               aGraph,
                               aDepth,
                               aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Graph 고유 정보의 출력
    //-----------------------------------

    //-----------------------------------
    // Join Method 정보의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Join Method Information ==" );

    if ( ( aGraph->flag & QMG_JOIN_ONLY_NL_MASK )
         == QMG_JOIN_ONLY_NL_FALSE )
    {
        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( sMyGraph->nestedLoopJoinMethod,
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );

        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( sMyGraph->sortBasedJoinMethod,
                                               aDepth,
                                               aString)
            != IDE_SUCCESS );

        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( sMyGraph->hashBasedJoinMethod,
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( sMyGraph->nestedLoopJoinMethod,
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );
    }

    //-----------------------------------
    // Subquery Graph 정보의 출력
    //-----------------------------------

    for ( sPredicate = sMyGraph->joinablePredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        for ( sMorePred = sPredicate;
              sMorePred != NULL;
              sMorePred = sMorePred->more )
        {
            IDE_TEST( qmg::printSubquery( aStatement,
                                          sMorePred->node,
                                          aDepth,
                                          aString )
                      != IDE_SUCCESS );
        }
    }

    for ( sPredicate = sMyGraph->joinablePredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        for ( sMorePred = sPredicate;
              sMorePred != NULL;
              sMorePred = sMorePred->more )
        {
            IDE_TEST( qmg::printSubquery( aStatement,
                                          sMorePred->node,
                                          aDepth,
                                          aString )
                      != IDE_SUCCESS );
        }
    }

    for ( sPredicate = sMyGraph->graph.myPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        for ( sMorePred = sPredicate;
              sMorePred != NULL;
              sMorePred = sMorePred->more )
        {
            IDE_TEST( qmg::printSubquery( aStatement,
                                          sMorePred->node,
                                          aDepth,
                                          aString )
                      != IDE_SUCCESS );
        }
    }

    //-----------------------------------
    // Child Graph 고유 정보의 출력
    //-----------------------------------

    IDE_TEST( aGraph->left->printGraph( aStatement,
                                        aGraph->left,
                                        aDepth + 1,
                                        aString )
              != IDE_SUCCESS );

    IDE_TEST( aGraph->right->printGraph( aStatement,
                                         aGraph->right,
                                         aDepth + 1,
                                         aString )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;
}

IDE_RC qmgLeftOuter::checkAndSetSkipRight( qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgLeftOuter skip right 조건 검사
 *
 * Implementation : PROJ-2750
 *
 *    optimize 의 가장 마지막에 호출되므로 최적화 완료된 모습임
 *    - graph 의 방향성은 QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT
 *    - graph 는 QMG_PARALLEL_EXEC_FALSE
 *    - Nested Left Outer
 *    - dependency 검사
 *
 *                          L2 - b.i1 = c.i1 (predicate node)
 *                         /  \
 *          a.i1 = b.i1 - L1   S3(c)
 *                       /  \
 *                  (a)S1    S2(b)
 *
 *           S1.dependency ( a       ) : aGraph->left->left (sLeftChildLeft)
 *           S2.dependency ( a, b    ) : aGraph->left->right
 *           S3.dependency ( b, c    ) : aGraph->right
 *           L1.dependency ( a, b    ) : aGraph->left
 *           L2.dependency ( a, b, c ) : aGraph
 *        -> S1 only dependency(a) 가 predicate node 에 포함되지 않고
 *        -> S2 only dependency(b) 가 predicate node 에 포함되는 경우
 *        -> 단, predicate 은 분류되거나 push down 되기 이전의 원형을 이용
 *
 *        cf) graph->left 의 방향성이 QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT 라면
 *            qmgJoin::afterJoinMethodDecision 을 통해 left, right 가 이미 바뀐 상태
 *
 *                                      L2 - b.i1 = c.i1 (predicate node)
 *                                     /  \
 *           a.i1 = b.i1 - (inverse) L1    S3(c)
 *                                  /  \
 *                             (b)S2   S1(a)
 *       
 *    - predicate 검사
 *        - AND 식으로 구성
 *        - ignore operation 식이 존재하지 않아야 함
 *
 ***********************************************************************/

    qmgLOJN   * sLOJN = (qmgLOJN *)aGraph;
    qmgGraph  * sLeftChildLeft = NULL;
    qcDepInfo   sLeftChildRightDepInfo;
    qtcNode   * sPredNode = NULL;

    IDE_TEST_RAISE( sLOJN->onConditionCNF == NULL, ERR_UNEXPECTED_ERROR );
    IDE_TEST_RAISE( ( sLOJN->onConditionCNF->normalCNF == NULL ) && ( sLOJN->onConditionCNF->nnfFilter == NULL ),
                    ERR_UNEXPECTED_ERROR );

    IDE_TEST_CONT( ( sLOJN->selectedJoinMethod->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
                   == QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT, SKIP_RIGHT_IGNORE );
    IDE_TEST_CONT( ( aGraph->flag & QMG_PARALLEL_EXEC_MASK ) == QMG_PARALLEL_EXEC_TRUE, SKIP_RIGHT_IGNORE );
    IDE_TEST_CONT( aGraph->left->type != QMG_LEFT_OUTER_JOIN, SKIP_RIGHT_IGNORE );

    if ( ( ((qmgLOJN *)aGraph->left)->selectedJoinMethod->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
         == QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT )
    {
        sLeftChildLeft = aGraph->left->right;
    }
    else
    {
        sLeftChildLeft = aGraph->left->left;
    }

    qtc::dependencyMinus( &aGraph->left->depInfo,
                          &sLeftChildLeft->depInfo,
                          &sLeftChildRightDepInfo );

    if ( sLOJN->onConditionCNF->normalCNF != NULL )
    {
        sPredNode = sLOJN->onConditionCNF->normalCNF;
    }
    else
    {
        sPredNode = sLOJN->onConditionCNF->nnfFilter;
    }

    if ( ( qtc::dependencyContains( &sPredNode->depInfo,
                                    &sLeftChildLeft->depInfo ) == ID_FALSE ) &&
         ( qtc::dependencyContains( &sPredNode->depInfo,
                                    &sLeftChildRightDepInfo ) == ID_TRUE ) )
    {
        // continue
    }
    else
    {
        IDE_CONT( SKIP_RIGHT_IGNORE );
    }

    IDE_TEST_CONT( sPredNode->node.module == &mtfOr, SKIP_RIGHT_IGNORE );

    if ( sPredNode->node.module == &mtfAnd )
    {
        if ( ( sPredNode->node.arguments->module == &mtfOr ) &&
             ( sPredNode->node.arguments->next == NULL ) &&
             ( sPredNode->node.arguments->arguments != NULL ) )
        {
            // ex) AND
            //     |
            //     OR
            //     |
            //     X - Y
            IDE_TEST_CONT( sPredNode->node.arguments->arguments->next != NULL, SKIP_RIGHT_IGNORE );
        }
    }

    if ( isExistsSkipRightIgnoreOperation( &sPredNode->node, &sLeftChildRightDepInfo ) == ID_FALSE )
    {
        aGraph->flag &= ~QMG_LOJN_SKIP_RIGHT_COND_MASK;
        aGraph->flag |=  QMG_LOJN_SKIP_RIGHT_COND_TRUE;
    }

    IDE_EXCEPTION_CONT( SKIP_RIGHT_IGNORE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgLeftOuter::checkAndSetSkipRight",
                                  "Unexpected left outer error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmgLeftOuter::isExistsSkipRightIgnoreOperation( mtcNode   * aNode,
                                                       qcDepInfo * aTargetDepInfo )
{
/***********************************************************************
 *
 * Description : PROJ-2750 
 *
 * Implementation : 특정 node(연산자) 의 인자로 taget dependency 존재 여부 확인
 *
 *        - MTC_NODE_EAT_NULL_TRUE flag
 *        - Subquery
 *        - NOT BETWEEN AND
 *        - NOT
 *        - =ANY, >=ANY, >ANY, <=ANY, <>ANY 의 right list parameter
 *
 ***********************************************************************/

    mtcNode * sNode = NULL;
    idBool    sResult = ID_FALSE;

    if ( ( ( aNode->lflag & MTC_NODE_EAT_NULL_MASK ) == MTC_NODE_EAT_NULL_TRUE     ) ||
         ( ( aNode->lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_SUBQUERY ) ||
         ( aNode->module == &mtfNotBetween ) ||
         ( aNode->module == &mtfNot ) )
    {
        sNode = aNode;
    }
    else
    {
        if ( ( aNode->module == &mtfEqualAny        ) ||
             ( aNode->module == &mtfGreaterThanAny  ) ||
             ( aNode->module == &mtfGreaterEqualAny ) ||
             ( aNode->module == &mtfLessEqualAny    ) ||
             ( aNode->module == &mtfLessThanAny     ) ||
             ( aNode->module == &mtfNotEqualAny     ) )
        {
            if ( aNode->arguments->next->module == &mtfList )
            {
                if ( aNode->arguments->next->arguments->next != NULL )
                {
                    // sNode is mtfList over 2 parameters
                    sNode = aNode->arguments->next;
                }
            }
        }
    }

    if ( sNode != NULL )
    {
        if ( qtc::dependencyContains( &((qtcNode*)sNode)->depInfo, aTargetDepInfo ) == ID_TRUE )
        {
            sResult = ID_TRUE;
        }
    }
    else
    {
        for ( sNode = aNode->arguments; sNode != NULL; sNode = sNode->next )
        {
            if ( isExistsSkipRightIgnoreOperation( sNode, aTargetDepInfo ) == ID_TRUE )
            {
                sResult = ID_TRUE;
                break;
            }
        }
    }

    return sResult;
}

IDE_RC qmgLeftOuter::checkAndSetSkipRightFlag( qmnPlan * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2750 
 *
 * Implementation : 그림과 같이 nested LOJN 사이에
 *                  FILT 이외의 plan 이 존재할 경우를 제외하고
 *                  QMNC_LOJN_RIGHT_SKIP_COND_TRUE flag 를 세팅한다.
 *
 *                          L1
 *                         /  \
 *                       [F]   X
 *                        |
 *                       L2
 *
 ***********************************************************************/

    qmnPlan * sPlan = NULL;
    qmnPlan * sNestedPlan = NULL;
    idBool    sIsNested = ID_FALSE;

    IDE_TEST_RAISE( ( aPlan->type != QMN_LOJN ) && ( aPlan->type != QMN_FILT ), ERR_UNEXPECTED_ERROR );

    for ( sPlan = aPlan; sPlan != NULL; sPlan = sPlan->left )
    {
        if ( sPlan->type == QMN_LOJN )
        {
            for ( sNestedPlan = sPlan->left; sNestedPlan != NULL; sNestedPlan = sNestedPlan->left )
            {
                if ( sNestedPlan->type == QMN_LOJN )
                {
                    sIsNested = ID_TRUE;
                    break;
                }
                else
                {
                    if ( sNestedPlan->type != QMN_FILT )
                    {
                        break;
                    }
                }
            }

            if ( sIsNested == ID_TRUE )
            {
                break;
            }
        }
        else
        {
            if ( sPlan->type != QMN_FILT )
            {
                break;
            }
        }
    }

    if ( sIsNested == ID_TRUE )
    {
        IDE_TEST_RAISE( sPlan->type != QMN_LOJN, ERR_UNEXPECTED_ERROR );
        IDE_TEST_RAISE( sNestedPlan->type != QMN_LOJN, ERR_UNEXPECTED_ERROR );

        ((qmncLOJN *)sPlan)->flag &= ~QMNC_LOJN_SKIP_RIGHT_COND_MASK;
        ((qmncLOJN *)sPlan)->flag |=  QMNC_LOJN_SKIP_RIGHT_COND_TRUE;
    }
    else
    {
        // L - [ H / S ] - L 인 경우
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgLeftOuter::checkAndSetSkipRightFlag",
                                  "Unexpected left outer error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
