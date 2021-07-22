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
 *     Semi join graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgSemiJoin.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoTwoNonPlan.h>
#include <qmoSelectivity.h>
#include <qmgJoin.h>
#include <qmo.h>

IDE_RC
qmgSemiJoin::init( qcStatement * aStatement,
                   qmgGraph    * aLeftGraph,
                   qmgGraph    * aRightGraph,
                   qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSemiJoin Graph의 초기화
 *
 * Implementation :
 *    (1)  qmgSemiJoin를 위한 공간 할당
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

    qmgJOIN   * sMyGraph;
    qcDepInfo   sOrDependencies;

    IDU_FIT_POINT_FATAL( "qmgSemiJoin::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftGraph != NULL );
    IDE_DASSERT( aRightGraph != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // Semi Join Graph를 위한 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgJOIN *)aGraph;

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    // Graph의 종류 표기
    sMyGraph->graph.type = QMG_SEMI_JOIN;
    sMyGraph->graph.left  = aLeftGraph;
    sMyGraph->graph.right = aRightGraph;

    sMyGraph->graph.myFrom = NULL;
    sMyGraph->graph.myQuerySet = aLeftGraph->myQuerySet;

    IDE_TEST( qtc::dependencyOr( & aLeftGraph->depInfo,
                                 & aRightGraph->depInfo,
                                 & sOrDependencies )
              != IDE_SUCCESS );

    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & sOrDependencies );

    // Graph의 함수 포인터를 설정
    sMyGraph->graph.optimize = qmgSemiJoin::optimize;
    sMyGraph->graph.makePlan = qmgJoin::makePlan;
    sMyGraph->graph.printGraph = qmgJoin::printGraph;

    // Disk/Memory 정보 설정
    switch(  sMyGraph->graph.myQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            // 중간 결과 Type Hint가 없는 경우,
            // left가 disk이면 disk
            if ( ( sMyGraph->graph.left->flag & QMG_GRAPH_TYPE_MASK )
                   == QMG_GRAPH_TYPE_DISK )
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

    //---------------------------------------------------
    // Semi Join Graph 만을 위한 자료 구조 초기화
    //---------------------------------------------------

    // Semi join은 사용자가 명시할 수 없으므로 ON절은 사용하지 않는다.
    sMyGraph->onConditionCNF = NULL;

    sMyGraph->joinMethods = NULL;
    sMyGraph->joinablePredicate = NULL;
    sMyGraph->nonJoinablePredicate = NULL;
    sMyGraph->hashBucketCnt = 0;
    sMyGraph->hashTmpTblCnt = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSemiJoin::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSemiJoin Graph의 최적화
 *
 * Implementation :
 *    (0) left graph의 최적화 수행
 *    (1) right graph의 최적화 수행
 *    (2) subquery의 처리
 *    (3) Join Method의 초기화
 *    (4) Join Method의 선택
 *    (5) Join Method 결정 후 처리
 *    (6) 공통 비용 정보의 설정
 *    (7) Preserved Order, DISK/MEMORY 설정
 *
 ***********************************************************************/

    qmgJOIN           * sMyGraph;
    qmoPredicate      * sPredicate;

    IDU_FIT_POINT_FATAL( "qmgSemiJoin::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgJOIN*) aGraph;

    // On condition이 존재하지 않으므로 left와 right의 optimization은 필요없다.

    // BUG-32703
    // 최적화 시에 view의 graph가 생성된다
    // left outer join의 하위 graph에 view가 있다면
    // 최적화 이후에 ( 즉, view graph가 최적화 된 이후에 )
    // left outer join에 대한 type ( disk or memory )를 새로 설정해야 한다.
    // BUG-40191 __OPTIMIZER_DEFAULT_TEMP_TBS_TYPE 힌트를 고려해야 한다.
    if ( aGraph->myQuerySet->SFWGH->hints->interResultType == QMO_INTER_RESULT_TYPE_NOT_DEFINED )
    {
        if ( ( sMyGraph->graph.left->flag & QMG_GRAPH_TYPE_MASK )
               == QMG_GRAPH_TYPE_DISK )
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

        IDE_TEST(
            qmoPred::classifyJoinPredicate( aStatement,
                                            sMyGraph->graph.myPredicate,
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
                         sMyGraph->graph.myPredicate,
                         &sPredicate,
                         0,
                         0,
                         ID_TRUE )
                     != IDE_SUCCESS );
        
            sMyGraph->graph.myPredicate = sPredicate;
        }
        else
        {
            // CNF이거나 NNF인 경우 복사할 필요가 없다.
            // Nothing To Do
        }
    }
    else
    {
        // Join Graph에 해당하는 Predicate이 없는 경우임
        // Nothing To Do
    }

    //------------------------------------------
    // Selectivity 설정
    //------------------------------------------

    // WHERE clause, ON clause 모두 고려된 상태
    IDE_TEST( qmoSelectivity::setJoinSelectivity(
                  aStatement,
                  & sMyGraph->graph,
                  sMyGraph->graph.myPredicate,
                  & sMyGraph->graph.costInfo.selectivity )
              != IDE_SUCCESS );

    //------------------------------------------
    // Join Method의 초기화
    //------------------------------------------

    // Join Method 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethod ) * QMG_SEMI_JOIN_METHOD_COUNT,
                                             (void **) &sMyGraph->joinMethods )
              != IDE_SUCCESS );

    // nested loop join method의 초기화
    IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                      & sMyGraph->graph,
                                      sMyGraph->firstRowsFactor,
                                      sMyGraph->graph.myPredicate,
                                      QMO_JOIN_METHOD_NL,
                                      &sMyGraph->joinMethods[QMG_SEMI_JOIN_METHOD_NESTED] )
              != IDE_SUCCESS );

    // hash based join method의 초기화
    IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                      & sMyGraph->graph,
                                      sMyGraph->firstRowsFactor,
                                      sMyGraph->graph.myPredicate,
                                      QMO_JOIN_METHOD_HASH,
                                      &sMyGraph->joinMethods[QMG_SEMI_JOIN_METHOD_HASH] )
              != IDE_SUCCESS );

    // sort based join method의 초기화
    IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                      & sMyGraph->graph,
                                      sMyGraph->firstRowsFactor,
                                      sMyGraph->graph.myPredicate,
                                      QMO_JOIN_METHOD_SORT,
                                      &sMyGraph->joinMethods[QMG_SEMI_JOIN_METHOD_SORT] )
              != IDE_SUCCESS );

    // merge join method의 초기화
    IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                      & sMyGraph->graph,
                                      sMyGraph->firstRowsFactor,
                                      sMyGraph->graph.myPredicate,
                                      QMO_JOIN_METHOD_MERGE,
                                      &sMyGraph->joinMethods[QMG_SEMI_JOIN_METHOD_MERGE] )
              != IDE_SUCCESS );

    // Semi join을 위한 join method selectivity 조정
    setJoinMethodsSelectivity( &sMyGraph->graph );

    //------------------------------------------
    // 각 Join Method 중 가장 좋은 cost의 Join Method를 선택
    //------------------------------------------

    IDE_TEST(
        qmgJoin::selectJoinMethodCost( aStatement,
                                       & sMyGraph->graph,
                                       sMyGraph->graph.myPredicate,
                                       sMyGraph->joinMethods,
                                       & sMyGraph->selectedJoinMethod )
        != IDE_SUCCESS );

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // record size 결정
    // Semi join은 left의 record만을 결과로 출력한다.
    sMyGraph->graph.costInfo.recordSize =
        sMyGraph->graph.left->costInfo.recordSize;

    // BUG-37134 semi join, anti join 도 setJoinOrderFactor 를 호출해야합니다.
    // 각 qmgJoin 의 joinOrderFactor, joinSize 계산
    IDE_TEST( qmoSelectivity::setJoinOrderFactor(
                  aStatement,
                  & sMyGraph->graph,
                  sMyGraph->graph.myPredicate,
                  &sMyGraph->joinOrderFactor,
                  &sMyGraph->joinSize )
              != IDE_SUCCESS );

    // input record count 설정
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    // BUG-37407 semi, anti join cost
    // output record count 설정
    sMyGraph->graph.costInfo.outputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt *
        sMyGraph->graph.costInfo.selectivity;

    sMyGraph->graph.costInfo.outputRecordCnt =
        IDL_MAX( sMyGraph->graph.costInfo.outputRecordCnt, 1.0 );

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

    // PROJ-2179
    sMyGraph->pushedDownPredicate = sMyGraph->graph.myPredicate;

    IDE_TEST(
        qmgJoin::afterJoinMethodDecision(
            aStatement,
            & sMyGraph->graph,
            sMyGraph->selectedJoinMethod,
            & sMyGraph->graph.myPredicate,
            & sMyGraph->joinablePredicate,
            & sMyGraph->nonJoinablePredicate)
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmgSemiJoin::setJoinMethodsSelectivity( qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description :
 *     Semi/anti join의 join method들의 selectivity를 조절한다.
 *
 * Implementation :
 *     Nested loop join과 stored nested loop join을 제외한 나머지
 *     모든 join method들의 selectivity를 0으로 설정한다.
 *     위의 두 join method들은 selectivity에 따라 비용이 다른 반면
 *     나머지 method들은 selectivity에 관계 없이 semi/anti join의
 *     비용이 모두 동일하기 때문이다.
 *
 ***********************************************************************/

    qmgJOIN           * sMyGraph;
    qmoJoinMethodCost * sJoinMethodCost;
    UInt                i, j;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgJOIN*) aGraph;

    for( i = 0; i < QMG_SEMI_JOIN_METHOD_COUNT; i++ )
    {
        for( j = 0; j < sMyGraph->joinMethods[i].joinMethodCnt; j++ )
        {
            sJoinMethodCost = &sMyGraph->joinMethods[i].joinMethodCost[j];
            switch( sJoinMethodCost->flag & QMO_JOIN_METHOD_MASK )
            {
                case QMO_JOIN_METHOD_FULL_NL:
                case QMO_JOIN_METHOD_FULL_STORE_NL:
                    break;
                default:
                    sMyGraph->joinMethods[i].joinMethodCost[j].selectivity = 0;
                    break;
            }
        }
    }

}
