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
 * $Id: qmgJoin.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Join Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <smDef.h>
#include <smiTableSpace.h>
#include <qcg.h>
#include <qmgJoin.h>
#include <qmgFullOuter.h>
#include <qmgLeftOuter.h>
#include <qmoCostDef.h>
#include <qmoCnfMgr.h>
#include <qmoNormalForm.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoTwoNonPlan.h>
#include <qmoParallelPlan.h>
#include <qmoJoinMethod.h>
#include <qmoSelectivity.h>
#include <qmgSelection.h>
#include <qmgPartition.h>
#include <qcgPlan.h>
#include <qmo.h>

IDE_RC
qmgJoin::init( qcStatement * aStatement,
               qmsQuerySet * aQuerySet,
               qmsFrom     * aFrom,
               qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : Inner Join에 대응하는 qmgJoin Graph의 초기화
 *
 * Implementation :
 *    (1) qmgJoin을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) qmgJoin의 onCondition 처리
 *    (4) 하위 graph의 생성 및 초기화
 *    (5) graph.optimize와 graph.makePlan 설정
 *    (6) out 설정
 *
 ***********************************************************************/

    qmgJOIN   * sMyGraph;
    qtcNode   * sNormalCNF;

    qmoNormalType sNormalType;

    IDU_FIT_POINT_FATAL( "qmgJoin::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );

    //---------------------------------------------------
    // Join Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgJoin을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgJOIN),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_INNER_JOIN;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );

    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgJoin::optimize;
    sMyGraph->graph.makePlan = qmgJoin::makePlan;
    sMyGraph->graph.printGraph = qmgJoin::printGraph;

    //---------------------------------------------------
    // Join Graph 만을 위한 자료 구조 초기화
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
    sMyGraph->graph.left->flag |= QMG_ROWNUM_PUSHED_FALSE;
    
    sMyGraph->graph.right->flag &= ~QMG_ROWNUM_PUSHED_MASK;
    sMyGraph->graph.right->flag |= QMG_ROWNUM_PUSHED_FALSE;

    // To Fix BUG-8439
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

    // join method 초기화
    sMyGraph->joinMethods = NULL;

    // joinable predicate / non joinable predicate 초기화
    sMyGraph->joinablePredicate = NULL;
    sMyGraph->nonJoinablePredicate = NULL;
    sMyGraph->pushedDownPredicate = NULL;

    // bucket count, hash temp table count 초기화
    sMyGraph->hashBucketCnt = 0;
    sMyGraph->hashTmpTblCnt = 0;

    // PROJ-2242 joinOrderFactor, joinSize 초기화
    sMyGraph->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
    sMyGraph->joinOrderFactor = 0;
    sMyGraph->joinSize        = 0;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::init( qcStatement * /* aStatement*/,
               qmgGraph    * aLeftGraph,
               qmgGraph    * aRightGraph,
               qmgGraph    * aGraph,
               idBool        aExistOrderFactor )
{
/***********************************************************************
 *
 * Description : Join Relation에 대응하는 qmgJoin Graph의 초기화
 *
 * Implementation :
 *    (1) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (2) graph.type 설정
 *    (3) graph.left을 aLeftGraph로, graph.right를 aRightGraph로 설정
 *    (4) graph.dependencies 설정
 *    (5) graph.optimize와 graph.makePlan 설정
 *
 ***********************************************************************/

    qmgJOIN * sMyGraph;
    qcDepInfo sOrDependencies;

    IDU_FIT_POINT_FATAL( "qmgJoin::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aLeftGraph != NULL );
    IDE_FT_ASSERT( aRightGraph != NULL );
    IDE_FT_ASSERT( aGraph != NULL );

    //---------------------------------------------------
    // Join Graph를 위한 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgJOIN *) aGraph;

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );
    sMyGraph->graph.type = QMG_INNER_JOIN;
    sMyGraph->graph.left = aLeftGraph;
    sMyGraph->graph.right = aRightGraph;

    // BUG-45296 rownum Pred 을 left outer 의 오른쪽으로 내리면 안됩니다.
    sMyGraph->graph.left->flag &= ~QMG_ROWNUM_PUSHED_MASK;
    sMyGraph->graph.left->flag |= QMG_ROWNUM_PUSHED_FALSE;
    
    sMyGraph->graph.right->flag &= ~QMG_ROWNUM_PUSHED_MASK;
    sMyGraph->graph.right->flag |= QMG_ROWNUM_PUSHED_FALSE;

    sMyGraph->graph.myFrom = NULL;
    sMyGraph->graph.myQuerySet = aLeftGraph->myQuerySet;

    IDE_TEST( qtc::dependencyOr( & aLeftGraph->depInfo,
                                 & aRightGraph->depInfo,
                                 & sOrDependencies )
              != IDE_SUCCESS );

    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & sOrDependencies );

    // Graph의 함수 포인터를 설정
    sMyGraph->graph.optimize = qmgJoin::optimize;
    sMyGraph->graph.makePlan = qmgJoin::makePlan;
    sMyGraph->graph.printGraph = qmgJoin::printGraph;

    // Disk/Memory 정보 설정
    switch(  sMyGraph->graph.myQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            // 중간 결과 Type Hint가 없는 경우,
            // left 또는 right가 disk이면 disk
            if ( ( ( aLeftGraph->flag & QMG_GRAPH_TYPE_MASK )
                   == QMG_GRAPH_TYPE_DISK ) ||
                 ( ( aRightGraph->flag & QMG_GRAPH_TYPE_MASK )
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

    //---------------------------------------------------
    // Join Graph 만을 위한 자료 구조 초기화
    //---------------------------------------------------

    sMyGraph->onConditionCNF = NULL;

    sMyGraph->joinMethods = NULL;

    sMyGraph->joinablePredicate = NULL;
    sMyGraph->nonJoinablePredicate = NULL;
    sMyGraph->pushedDownPredicate = NULL;

    sMyGraph->hashBucketCnt = 0;
    sMyGraph->hashTmpTblCnt = 0;

    // PROJ-2242
    if( aExistOrderFactor == ID_FALSE )
    {
        sMyGraph->joinOrderFactor = 0;
        sMyGraph->joinSize = 0;
    }
    else
    {
        // join ordering 과정에서 계산된 상태
    }
    sMyGraph->firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgJoin Graph의 최적화
 *
 * Implementation :
 *    (1) Inner Join인 경우, 다음을 수행
 *        A. subquery의 처리
 *        B. on Condition으로 Transitive Predicate 생성
 *        C. on Condition Predicate의 분류
 *        D. left graph의 최적화 수행
 *        E. right graph의 최적화 수행
 *    (2) Join Method 의 초기화
 *    (3) Join Method 선택
 *    (4) Join Method 결정 후 처리
 *    (5) 공통 비용 정보의 설정
 *    (6) Preserved Order, DISK/MEMORY 설정
 *
 ***********************************************************************/

    qmgJOIN           * sMyGraph;
    qmoPredicate      * sPredicate;
    qmoPredicate      * sLowerPredicate;

    IDU_FIT_POINT_FATAL( "qmgJoin::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgJOIN*)aGraph;

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

    if ( sMyGraph->onConditionCNF != NULL )
    {
        //------------------------------------------
        // Inner Join 인 경우
        //    - Transitive Predicate 생성
        //    - onCondition Predicate의 분류
        //    - left graph의 최적화 수행
        //    - right graph의 최적화 수행
        //------------------------------------------

        IDE_TEST( qmoCnfMgr::generateTransitivePred4OnCondition(
                      aStatement,
                      sMyGraph->onConditionCNF,
                      sMyGraph->graph.myPredicate,
                      & sLowerPredicate )
                  != IDE_SUCCESS );
        
        IDE_TEST(
            qmoCnfMgr::classifyPred4OnCondition(
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

        IDE_TEST( sMyGraph->graph.left->optimize( aStatement,
                                                  sMyGraph->graph.left )
                  != IDE_SUCCESS );

        IDE_TEST( sMyGraph->graph.right->optimize( aStatement,
                                                   sMyGraph->graph.right )
                  != IDE_SUCCESS );

        // fix BUG-19203
        IDE_TEST( qmoCnfMgr::optimizeSubQ4OnCondition(aStatement,
                                                      sMyGraph->onConditionCNF)
                  != IDE_SUCCESS );

        if( sMyGraph->onConditionCNF->joinPredicate != NULL )
        {
            IDE_TEST(
                qmoPred::classifyJoinPredicate(
                    aStatement,
                    sMyGraph->onConditionCNF->joinPredicate,
                    sMyGraph->graph.left,
                    sMyGraph->graph.right,
                    & sMyGraph->graph.depInfo )
                != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        //------------------------------------------
        // Subquery의 Graph 생성
        //------------------------------------------
        // To fix BUG-19813
        // on condition 에 남아있는 predicate이 mypredicate에 붙기 전에
        // 먼저 subquery graph를 생성해야 함
        // :on condition predicate은
        // qmoCnfMgr::optimizeSubQ4OnCondition함수에 의해 이미 subquery graph를
        // 생성하였기 때문
        if ( sMyGraph->graph.myPredicate != NULL )
        {
            IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                                   sMyGraph->graph.myPredicate,
                                                   ID_FALSE ) // No KeyRange Tip
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
        
        //------------------------------------------
        // fix BUG-9972
        // on Condition CNF의 predicate 분류 후 sMyGraph->graph.myPredicate 중
        // one table predicate은 left 또는 right에 push selection 된 후
        // subquery를 동반한 predicate인 경우 push selection되지 않아 남아있을 수 있음
        // on Condition CNF에서 joinPredicate으로 분류된 predicate 및 oneTablePredicate
        // 을 sMyGraph->graph.myPredicate에 연결시켜 후에 동일하게 처리하도록 설정
        //------------------------------------------
        if( sMyGraph->graph.myPredicate == NULL )
        {
            sMyGraph->graph.myPredicate = sMyGraph->onConditionCNF->oneTablePredicate;
        }
        else
        {
            for ( sPredicate = sMyGraph->graph.myPredicate;
                  sPredicate->next != NULL;
                  sPredicate = sPredicate->next ) ;
            sPredicate->next = sMyGraph->onConditionCNF->oneTablePredicate;
        }


        if ( sMyGraph->graph.myPredicate == NULL )
        {
            sMyGraph->graph.myPredicate =
                sMyGraph->onConditionCNF->joinPredicate;
        }
        else
        {
            for ( sPredicate = sMyGraph->graph.myPredicate;
                  sPredicate->next != NULL;
                  sPredicate = sPredicate->next ) ;
            sPredicate->next = sMyGraph->onConditionCNF->joinPredicate;
        }

        //------------------------------------------
        // Constant Predicate의 처리
        //------------------------------------------

        // fix BUG-9791
        // 예) SELECT * FROM T1 INNER JOIN T2 ON 1 = 2;
        // inner join인 경우는, on절의 constant filter를
        // 처리가능한 최하위 left graph에 내린다.
        // ( left outer, full outer join인 경우는,
        //   조건에 맞지 않는 레코드에 대해서는 null padding해야 하므로,
        //   on절의 constant filter를 내릴 수 없고,
        //   left, full outer join node에서 filter처리해야 한다. )
        IDE_TEST(
            qmoCnfMgr::pushSelection4ConstantFilter( aStatement,
                                                     (qmgGraph*)sMyGraph,
                                                     sMyGraph->onConditionCNF )
            != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------------
        // Subquery의 Graph 생성
        //------------------------------------------
        
        // To Fix BUG-8219
        if ( sMyGraph->graph.myPredicate != NULL )
        {
            IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                                   sMyGraph->graph.myPredicate,
                                                   ID_FALSE ) // No KeyRange Tip
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    if ( sMyGraph->graph.myPredicate != NULL )
    {

        //------------------------------------------
        // Join Predicate의 분류
        //------------------------------------------

        IDE_TEST( qmoPred::classifyJoinPredicate( aStatement,
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
            IDE_TEST( qmoPred::copyPredicate4Partition(
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
    if ( ( sMyGraph->graph.flag & QMG_JOIN_ONLY_NL_MASK )
         == QMG_JOIN_ONLY_NL_FALSE )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoJoinMethod ) * QMG_INNER_JOIN_METHOD_COUNT,
                                                   (void **)&sMyGraph->joinMethods )
                  != IDE_SUCCESS );

        // nested loop join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->graph.myPredicate,
                                          QMO_JOIN_METHOD_NL,
                                          &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_NESTED] )
                  != IDE_SUCCESS );

        // hash based join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->graph.myPredicate,
                                          QMO_JOIN_METHOD_HASH,
                                          &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_HASH] )
                  != IDE_SUCCESS );

        // sort based join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->graph.myPredicate,
                                          QMO_JOIN_METHOD_SORT,
                                          &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_SORT] )
                  != IDE_SUCCESS );

        // merge join method의 초기화
        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->graph.myPredicate,
                                          QMO_JOIN_METHOD_MERGE,
                                          &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_MERGE] )
                  != IDE_SUCCESS );
    }
    else
    {
        IDU_FIT_POINT("qmgJoin::optimize::alloc::joinMethods",
                      idERR_ABORT_InsufficientMemory);
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoJoinMethod ),
                                                   (void **)&sMyGraph->joinMethods )
                  != IDE_SUCCESS );

        IDE_TEST( qmoJoinMethodMgr::init( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->firstRowsFactor,
                                          sMyGraph->graph.myPredicate,
                                          QMO_JOIN_METHOD_NL,
                                          &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_NESTED] )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // 각 Join Method 중 가장 좋은 cost의 Join Method를 선택
    //------------------------------------------

    IDE_TEST( qmgJoin::selectJoinMethodCost( aStatement,
                                             & sMyGraph->graph,
                                             sMyGraph->graph.myPredicate,
                                             sMyGraph->joinMethods,
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
                  sMyGraph->graph.myPredicate,
                  &sMyGraph->joinOrderFactor,
                  &sMyGraph->joinSize )
              != IDE_SUCCESS );

    // input record count 설정
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt *
        sMyGraph->graph.right->costInfo.outputRecordCnt;

    // output record count 설정
    sMyGraph->graph.costInfo.outputRecordCnt =
        sMyGraph->graph.costInfo.inputRecordCnt *
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

    // To Fix PR-8032
    // joinPredicate과 nonJoinablePredicate은 Output으로 받아야 함.
    IDE_TEST(
        qmgJoin::afterJoinMethodDecision( aStatement,
                                          & sMyGraph->graph,
                                          sMyGraph->selectedJoinMethod,
                                          & sMyGraph->graph.myPredicate,
                                          & sMyGraph->joinablePredicate,
                                          & sMyGraph->nonJoinablePredicate )
        != IDE_SUCCESS );

    //------------------------------------------
    // Preserved Order
    //------------------------------------------

    IDE_TEST( makePreservedOrder( aStatement,
                                  & sMyGraph->graph,
                                  sMyGraph->selectedJoinMethod,
                                  sMyGraph->joinablePredicate )
              != IDE_SUCCESS );

    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
    aGraph->flag |= ((aGraph->left->flag & QMG_PARALLEL_EXEC_MASK) |
                     (aGraph->right->flag & QMG_PARALLEL_EXEC_MASK));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgJoin으로 부터 Plan 을 생성한다.
 *
 * Implementation :
 *     - qmgJoin으로 부터 생성가능 한 Plan
 *
 *     1.  Nested Loop 계열
 *         1.1  Full Nested Loop Join의 경우
 *              - Filter가 없는 경우
 *
 *                   [JOIN]
 *
 *              - Filter가 있는 경우 (Right가 SCAN)
 *
 *                   [JOIN]
 *                   |    |
 *                        [SCAN]  : SCAN에 포함됨
 *
 *              - Filter가 있는 경우 (Right가 SCAN이 아닌 경우)
 *
 *                   [FILT]
 *                     |
 *                   [JOIN]
 * --> Filter가 있는 경우 (right가 SCAN일때) Join에서 Selection으로 내려주기
 *     때문에, Right가 scan인지 아닌지 구분해줄필요가 없다. 단지 Filter가
 *     존재 하는지 안하는지의 구분만 해준다.
 *
 *         1.2  Full Store Nested Loop Join의 경우
 *
 *                 ( [FILT] )
 *                     |
 *                   [JOIN]
 *                   |    |
 *                        [SORT]
 *
 *         1.3  Index Nested Loop Join의 경우
 *
 *                   [JOIN]
 *                   |    |
 *                        [SCAN]
 *
 *         1.4  Anti-Outer Nested Loop Join의 경우
 *              - qmgFullOuter로부터만 생성됨.
 *
 *     2.  Sort-based 계열
 *
 *                      |
 *                  ( [FILT] )
 *                      |
 *                    [JOIN]
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
 *                  ( [FILT] )
 *                      |
 *                    [JOIN]
 *                    |    |
 *              ([HASH])   [HASH]
 *
 *         - Two-Pass Hash Join인 경우, Left에 HASH가 구성됨
 *
 *     4.  Merge Join 계열
 *
 *                      |
 *                    [MGJN]
 *                    |    |
 *          [SORT,MGJN]    [SORT,MGJN]
 *
 *         - SCAN노드의 경우 하위 Graph로부터 생성됨.
 *
 ***********************************************************************/

    qmgJOIN         * sMyGraph;
    qmnPlan         * sOnFILT    = NULL;
    qmnPlan         * sWhereFILT = NULL;

    IDU_FIT_POINT_FATAL( "qmgJoin::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgJOIN*) aGraph;

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

    // Parent plan을 myPlan으로 일단 설정한다.
    sMyGraph->graph.myPlan = aParent->myPlan;

    //-----------------------------------------------
    // NNF Filter 의 처리
    //-----------------------------------------------

    // TODO: FILTER operator를 만들지 않고 JOIN operator의
    //       filter로 처리되어야 한다.

    // ON 절에 존재하는 NNF Filter 처리
    if ( sMyGraph->onConditionCNF != NULL )
    {
        if ( sMyGraph->onConditionCNF->nnfFilter != NULL )
        {
            //make FILT
            IDE_TEST( qmoOneNonPlan::initFILT(
                          aStatement ,
                          sMyGraph->graph.myQuerySet ,
                          sMyGraph->onConditionCNF->nnfFilter,
                          sMyGraph->graph.myPlan,
                          &sOnFILT) != IDE_SUCCESS);
            sMyGraph->graph.myPlan = sOnFILT;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    // WHERE 절에 존재하는 NNF Filter 처리
    if ( sMyGraph->graph.nnfFilter != NULL )
    {
        //make FILT
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement ,
                      sMyGraph->graph.myQuerySet ,
                      sMyGraph->graph.nnfFilter,
                      sMyGraph->graph.myPlan,
                      &sWhereFILT) != IDE_SUCCESS);
        sMyGraph->graph.myPlan = sWhereFILT;
    }
    else
    {
        // Nothing To Do
    }

    switch( sMyGraph->selectedJoinMethod->flag & QMO_JOIN_METHOD_MASK )
    {
        //---------------------------------------------------
        // Index Nested Loop Join의 경우
        //---------------------------------------------------

        case QMO_JOIN_METHOD_INDEX:
            IDE_TEST( makeIndexNLJoin( aStatement,
                                       sMyGraph )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_INVERSE_INDEX:
            IDE_TEST( makeInverseIndexNLJoin( aStatement,
                                              sMyGraph )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_FULL_NL:
            IDE_TEST( makeFullNLJoin( aStatement,
                                      sMyGraph,
                                      ID_FALSE )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_FULL_STORE_NL:
            IDE_TEST( makeFullNLJoin( aStatement,
                                      sMyGraph,
                                      ID_TRUE )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_TWO_PASS_HASH:
            IDE_TEST( makeHashJoin( aStatement,
                                    sMyGraph,
                                    ID_TRUE,    // TwoPass?
                                    ID_FALSE )  // Inverse?
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_ONE_PASS_HASH:
            IDE_TEST( makeHashJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE,
                                    ID_FALSE )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_INVERSE_HASH:
            IDE_TEST( makeHashJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE,   // TwoPass?
                                    ID_TRUE )   // Inverse?
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_TWO_PASS_SORT:
            IDE_TEST( makeSortJoin( aStatement,
                                    sMyGraph,
                                    ID_TRUE,   // TwoPass?
                                    ID_FALSE ) // Inverse?
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_ONE_PASS_SORT:
            IDE_TEST( makeSortJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE,  // TwoPass?
                                    ID_FALSE ) // Inverse?
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_INVERSE_SORT:
            IDE_TEST( makeSortJoin( aStatement,
                                    sMyGraph,
                                    ID_FALSE,  // TwoPass?
                                    ID_TRUE )  // Inverse?
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_MERGE:
            IDE_TEST( makeMergeJoin( aStatement,
                                     sMyGraph )
                      != IDE_SUCCESS );
            break;
    }

    //-----------------------------------------------
    // NNF Filter 의 처리
    //-----------------------------------------------

    // WHERE 절에 존재하는 NNF Filter 처리
    if ( sMyGraph->graph.nnfFilter != NULL )
    {
        //make FILT
        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement ,
                      sMyGraph->graph.myQuerySet ,
                      sMyGraph->graph.nnfFilter,
                      NULL,
                      sMyGraph->graph.myPlan ,
                      sWhereFILT) != IDE_SUCCESS);

        sMyGraph->graph.myPlan = sWhereFILT;

        qmg::setPlanInfo( aStatement, sWhereFILT, &(sMyGraph->graph) );
    }
    else
    {
        // Nothing To Do
    }

    // ON 절에 존재하는 NNF Filter 처리
    if ( sMyGraph->onConditionCNF != NULL )
    {
        if ( sMyGraph->onConditionCNF->nnfFilter != NULL )
        {
            //make FILT
            IDE_TEST( qmoOneNonPlan::makeFILT(
                          aStatement ,
                          sMyGraph->graph.myQuerySet ,
                          sMyGraph->onConditionCNF->nnfFilter,
                          NULL,
                          sMyGraph->graph.myPlan ,
                          sOnFILT) != IDE_SUCCESS);

            sMyGraph->graph.myPlan = sOnFILT;

            qmg::setPlanInfo( aStatement, sOnFILT, &(sMyGraph->graph) );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeChildPlan( qcStatement     * aStatement,
                        qmgJOIN         * aMyGraph,
                        qmnPlan         * aLeftPlan,
                        qmnPlan         * aRightPlan )
{

    IDU_FIT_POINT_FATAL( "qmgJoin::makeChildPlan::__FT__" );

    //---------------------------------------------------
    // 하위 Plan의 생성 , Join이므로 양쪽 모두 실행
    //---------------------------------------------------

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
qmgJoin::makeSortRange( qcStatement  * aStatement,
                        qmgJOIN      * aMyGraph,
                        qtcNode     ** aRange )
{
    qtcNode         * sCNFRange;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeSortRange::__FT__" );

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
qmgJoin::makeHashFilter( qcStatement  * aStatement,
                         qmgJOIN      * aMyGraph,
                         qtcNode     ** aFilter )
{

    IDU_FIT_POINT_FATAL( "qmgJoin::makeHashFilter::__FT__" );

    IDE_TEST( makeSortRange( aStatement, aMyGraph, aFilter ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeIndexNLJoin( qcStatement    * aStatement,
                          qmgJOIN        * aMyGraph )
{
    qmnPlan         * sJOIN = NULL;
    qmsQuerySet     * sViewQuerySet;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeIndexNLJoin::__FT__" );

    //-----------------------------------------------------
    //        [JOIN]
    //         /  `.
    //    [LEFT]  [RIGHT]
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init JOIN
    //-----------------------

    // BUG-43077
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if ( (aMyGraph->graph.right->type == QMG_SELECTION ) &&
         (aMyGraph->graph.right->myFrom->tableRef->view != NULL) )
    {
        sViewQuerySet = aMyGraph->graph.right->left->myQuerySet;
    }
    else
    {
        sViewQuerySet = NULL;
    }

    IDE_TEST( qmoTwoNonPlan::initJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sViewQuerySet,
                                       NULL,
                                       NULL,
                                       aMyGraph->pushedDownPredicate ,
                                       aMyGraph->graph.myPlan,
                                       &sJOIN ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sJOIN;

    // BUG-38410
    // 반복 실행 될 경우 SCAN Parallel 을 금지한다.
    aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_TRUE;

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph,
                             sJOIN,
                             sJOIN )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make JOIN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.type,
                                       aMyGraph->graph.left->myPlan ,
                                       aMyGraph->graph.right->myPlan ,
                                       sJOIN ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sJOIN;

    qmg::setPlanInfo( aStatement, sJOIN, &(aMyGraph->graph) );

    sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
    sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_INDEX;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmgJoin::makeInverseIndexNLJoin( qcStatement    * aStatement,
                                        qmgJOIN        * aMyGraph )
{
    //-----------------------------------------------------
    //        [JOIN]
    //         /  `.
    //    [HASH]  [RIGHT] (+index)
    //       |
    //    [LEFT]  
    //-----------------------------------------------------

    UInt              sMtrFlag    = 0;
    qmnPlan         * sJOIN       = NULL;
    qmnPlan         * sFILT       = NULL;
    qmnPlan         * sLeftHASH   = NULL;
    qtcNode         * sHashFilter = NULL;
    qtcNode         * sFilter     = NULL;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeInverseIndexNLJoin::__FT__" );

    IDE_TEST( makeHashFilter( aStatement,
                              aMyGraph,
                              &sHashFilter )
              != IDE_SUCCESS );

    IDE_TEST( extractFilter( aStatement,
                             aMyGraph,
                             aMyGraph->nonJoinablePredicate,
                             &sFilter )
              != IDE_SUCCESS );

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init FILT if filter exist
    //-----------------------

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFILT )
              != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // init JOIN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::initJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       NULL,
                                       NULL,
                                       NULL,
                                       aMyGraph->pushedDownPredicate ,
                                       aMyGraph->graph.myPlan,
                                       &sJOIN ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sJOIN;

    //-----------------------------
    // init LEFT HASH (Distinct Sequential Search)
    //-----------------------------

    IDE_TEST( qmoOneMtrPlan::initHASH( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       sHashFilter,
                                       ID_TRUE, // isLeftHash
                                       ID_TRUE, // isDistinct
                                       sJOIN,
                                       &sLeftHASH ) != IDE_SUCCESS );

    // BUG-38410
    // Hash 되므로 left 는 한번만 실행되고 hash 된 내용을 참조한다.
    // 따라서 left 는 SCAN 에 대한 parallel 을 허용한다.
    aMyGraph->graph.left->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.left->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_TRUE;

    //-----------------------
    // 하위 plan 생성 (LEFT)
    //-----------------------

    aMyGraph->graph.myPlan = sJOIN;

    IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                              &aMyGraph->graph ,
                                              aMyGraph->graph.left )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    sMtrFlag = 0;

    sMtrFlag &= ~QMO_MAKEHASH_METHOD_MASK;
    sMtrFlag |= QMO_MAKEHASH_HASH_BASED_JOIN;

    sMtrFlag &= ~QMO_MAKEHASH_POSITION_MASK;
    sMtrFlag |= QMO_MAKEHASH_POSITION_LEFT;

    sMtrFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;

    if ( ( aMyGraph->selectedJoinMethod->flag & 
           QMO_JOIN_METHOD_LEFT_STORAGE_MASK )
         == QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY )
    {
        sMtrFlag |= QMO_MAKEHASH_MEMORY_TEMP_TABLE;  
    }
    else
    {
        sMtrFlag |= QMO_MAKEHASH_DISK_TEMP_TABLE;
    }

    // isAllAttrKey를 TRUE로 두기 위해, makeHASH 직접 호출한다.
    IDE_TEST( qmoOneMtrPlan::makeHASH( 
                  aStatement,
                  aMyGraph->graph.myQuerySet,
                  sMtrFlag,
                  ((qmgJOIN*)&aMyGraph->graph)->hashTmpTblCnt,
                  ((qmgJOIN*)&aMyGraph->graph)->hashBucketCnt,
                  sHashFilter,
                  aMyGraph->graph.left->myPlan,
                  sLeftHASH,
                  ID_TRUE ) // aAllAttrToKey
              != IDE_SUCCESS );

    qmg::setPlanInfo( aStatement, sLeftHASH, &aMyGraph->graph );

    //-----------------------
    // 하위 plan 생성 (RIGHT)
    //-----------------------

    aMyGraph->graph.myPlan = sJOIN;

    IDE_TEST( aMyGraph->graph.right->makePlan( aStatement ,
                                               &aMyGraph->graph ,
                                               aMyGraph->graph.right )
              != IDE_SUCCESS );

    aMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_JOIN;

    //-----------------------
    // make JOIN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.type,
                                       sLeftHASH ,
                                       aMyGraph->graph.right->myPlan ,
                                       sJOIN ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sJOIN;

    qmg::setPlanInfo( aStatement, sJOIN, &(aMyGraph->graph) );

    //-----------------------
    // Join Type 설정
    //-----------------------

    sJOIN->flag &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
    sJOIN->flag |= QMN_PLAN_JOIN_METHOD_INVERSE_INDEX;

    //-----------------------
    // make FILT if filter exist
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeFullNLJoin( qcStatement    * aStatement,
                         qmgJOIN        * aMyGraph,
                         idBool           aIsStored )
{
    UInt              sMtrFlag;
    UInt              sJoinFlag;
    qmnPlan         * sJOIN   = NULL;
    qmnPlan         * sSORT   = NULL;
    qmnPlan         * sFILT   = NULL;
    qtcNode         * sFilter = NULL;
    qmsQuerySet     * sViewQuerySet;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeFullNLJoin::__FT__" );

    //-----------------------------------------------------
    //        [*FILT]
    //           |
    //        [JOIN]
    //         /  `.
    //    [LEFT]  [*SORT]
    //               |
    //            [RIGHT]
    // * Option
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init FILT if filter exist
    //-----------------------

    IDE_TEST( extractFilter( aStatement,
                             aMyGraph,
                             aMyGraph->graph.myPredicate,
                             &sFilter )
              != IDE_SUCCESS );

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFILT )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // init JOIN
    //-----------------------

    // BUG-43077
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if ( (aMyGraph->graph.right->type == QMG_SELECTION ) &&
         (aMyGraph->graph.right->myFrom->tableRef->view != NULL) )
    {
        sViewQuerySet = aMyGraph->graph.right->left->myQuerySet;
    }
    else
    {
        sViewQuerySet = NULL;
    }

    IDE_TEST( qmoTwoNonPlan::initJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sViewQuerySet,
                                       NULL,
                                       sFilter,
                                       aMyGraph->pushedDownPredicate,
                                       aMyGraph->graph.myPlan ,
                                       &sJOIN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sJOIN;

    if( aIsStored == ID_TRUE )
    {
        // Full store nested loop join인 경우

        //-----------------------
        // init right SORT
        //-----------------------

        IDE_TEST( qmoOneMtrPlan::initSORT( aStatement,
                                           aMyGraph->graph.myQuerySet,
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
        // Full nested loop join인 경우
        // Right child의 parent를 join으로 설정
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
                             sJOIN,
                             sSORT )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make JOIN
    //-----------------------

    if( aIsStored == ID_TRUE )
    {
        // Full store nested loop join인 경우

        //----------------------------
        // SORT의 생성
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

        if ( ( sJoinFlag &
               QMO_JOIN_METHOD_RIGHT_STORAGE_MASK )
             == QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY )
        {
            sMtrFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
            sMtrFlag |= QMO_MAKESORT_MEMORY_TEMP_TABLE;
        }
        else
        {
            sMtrFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
            sMtrFlag |= QMO_MAKESORT_DISK_TEMP_TABLE;
        }

        //-----------------------
        // make right SORT
        //-----------------------

        IDE_TEST( qmoOneMtrPlan::makeSORT( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           sMtrFlag ,
                                           NULL ,
                                           aMyGraph->graph.right->myPlan ,
                                           aMyGraph->graph.costInfo.inputRecordCnt,
                                           sSORT ) != IDE_SUCCESS);

        qmg::setPlanInfo( aStatement, sSORT, &(aMyGraph->graph) );
    }
    else
    {
        // Full nested loop join인 경우
        // Right child graph의 operator를 right child로 설정
        sSORT = aMyGraph->graph.right->myPlan;
    }

    IDE_TEST( qmoTwoNonPlan::makeJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.type,
                                       aMyGraph->graph.left->myPlan ,
                                       sSORT ,
                                       sJOIN ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sJOIN;

    qmg::setPlanInfo( aStatement, sJOIN, &(aMyGraph->graph) );

    if( aIsStored == ID_TRUE )
    {
        sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_FULL_STORE_NL;
    }
    else
    {
        sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_FULL_NL;
    }

    //-----------------------
    // make FILT if filter exist
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeHashJoin( qcStatement     * aStatement,
                       qmgJOIN         * aMyGraph,
                       idBool            aIsTwoPass,
                       idBool            aIsInverse )
{
    UInt              sMtrFlag    = 0;
    UInt              sJoinFlag;
    qmnPlan         * sFILT       = NULL;
    qmnPlan         * sJOIN       = NULL;
    qmnPlan         * sLeftHASH   = NULL;
    qmnPlan         * sRightHASH  = NULL;
    qtcNode         * sHashFilter = NULL;
    qtcNode         * sFilter     = NULL;
    qmsQuerySet     * sViewQuerySet;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeHashJoin::__FT__" );

    //-----------------------------------------------------
    //           [JOIN]
    //            /  `.
    //       [*HASH]  [HASH]
    //          |        |
    //       [LEFT]   [RIGHT]
    // * Option
    //-----------------------------------------------------

    IDE_TEST( makeHashFilter( aStatement,
                              aMyGraph,
                              &sHashFilter )
              != IDE_SUCCESS );

    IDE_TEST( extractFilter( aStatement,
                             aMyGraph,
                             aMyGraph->nonJoinablePredicate,
                             &sFilter )
              != IDE_SUCCESS );

    /* BUG-46800 */
    moveConstantPred( aMyGraph, aIsInverse );

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init FILT if filter exist
    //-----------------------

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFILT )
              != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // init JOIN
    //-----------------------

    // BUG-43077
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if ( (aMyGraph->graph.right->type == QMG_SELECTION ) &&
         (aMyGraph->graph.right->myFrom->tableRef->view != NULL) )
    {
        sViewQuerySet = aMyGraph->graph.right->left->myQuerySet;
    }
    else
    {
        sViewQuerySet = NULL;
    }

    IDE_TEST( qmoTwoNonPlan::initJOIN(
                  aStatement,
                  aMyGraph->graph.myQuerySet,
                  sViewQuerySet,
                  sHashFilter,
                  sFilter,
                  NULL,
                  sFILT,
                  &sJOIN )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sJOIN;

    //-----------------------
    // init right HASH
    //-----------------------

    qmc::disableSealTrueFlag( sJOIN->resultDesc );
    IDE_TEST( qmoOneMtrPlan::initHASH(
                  aStatement,
                  aMyGraph->graph.myQuerySet,
                  sHashFilter,
                  ID_FALSE,     // isLeftHash
                  ID_FALSE,     // isDistinct
                  sJOIN,
                  &sRightHASH )
              != IDE_SUCCESS );

    if ( aIsTwoPass == ID_TRUE )
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
                      sJOIN,
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
        // Left child의 parent로 join을 설정한다.
        sLeftHASH = sJOIN;
    }

    // BUG-38410
    // Hash 되므로 right 는 한번만 실행되고 sort 된 내용만 참조한다.
    // 따라서 right 는 SCAN 에 대한 parallel 을 허용한다.
    aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph,
                             sLeftHASH,   // left child의 parent는 hash 또는 join
                             sRightHASH ) // right child의 parent는 hash
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    if( aIsTwoPass == ID_TRUE )
    {
        // Two pass hash join인 경우

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

        IDE_TEST( qmg::makeLeftHASH4Join(
                      aStatement,
                      &aMyGraph->graph,
                      sMtrFlag,
                      sJoinFlag,
                      sHashFilter,
                      aMyGraph->graph.left->myPlan,
                      sLeftHASH )
                  != IDE_SUCCESS );
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

    IDE_TEST( qmg::makeRightHASH4Join(
                  aStatement,
                  &aMyGraph->graph,
                  sMtrFlag,
                  sJoinFlag,
                  sHashFilter,
                  aMyGraph->graph.right->myPlan,
                  sRightHASH )
              != IDE_SUCCESS );

    //-----------------------
    // make JOIN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.type,
                                       sLeftHASH ,
                                       sRightHASH ,
                                       sJOIN ) != IDE_SUCCESS);

    aMyGraph->graph.myPlan = sJOIN;

    qmg::setPlanInfo( aStatement, sJOIN, &(aMyGraph->graph) );

    if( aIsTwoPass == ID_TRUE )
    {
        sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_TWO_PASS_HASH;
    }
    else
    {
        /* PROJ-2339 */
        if ( aIsInverse == ID_TRUE )
        {
            sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_INVERSE_HASH;
        }
        else
        {
            sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_ONE_PASS_HASH;
        }
    }

    //-----------------------
    // make FILT if filter exist
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeSortJoin( qcStatement     * aStatement,
                       qmgJOIN         * aMyGraph,
                       idBool            aIsTwoPass,
                       idBool            aIsInverse )
{
    UInt              sMtrFlag   = 0;
    UInt              sJoinFlag  = 0;
    qmnPlan         * sFILT      = NULL;
    qmnPlan         * sJOIN      = NULL;
    qmnPlan         * sLeftSORT  = NULL;
    qmnPlan         * sRightSORT = NULL;
    qtcNode         * sSortRange = NULL;
    qtcNode         * sFilter    = NULL;
    qmsQuerySet     * sViewQuerySet;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeSortJoin::__FT__" );

    IDE_TEST( makeSortRange( aStatement,
                             aMyGraph,
                             &sSortRange )
              != IDE_SUCCESS );

    IDE_TEST( extractFilter( aStatement,
                             aMyGraph,
                             aMyGraph->nonJoinablePredicate,
                             &sFilter )
              != IDE_SUCCESS );

    /* BUG-46800 */
    moveConstantPred( aMyGraph, aIsInverse );

    //-----------------------------------------------------
    //          [JOIN]
    //           /  `.
    //      [*SORT]  [SORT]
    //         |        |
    //      [LEFT]   [RIGHT]
    // * Option
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init FILT if filter exist
    //-----------------------

    IDE_TEST( initFILT( aStatement,
                        aMyGraph,
                        &sFILT )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sFILT;

    //-----------------------
    // init JOIN
    //-----------------------

    // BUG-43077
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if ( (aMyGraph->graph.right->type == QMG_SELECTION ) &&
         (aMyGraph->graph.right->myFrom->tableRef->view != NULL) )
    {
        sViewQuerySet = aMyGraph->graph.right->left->myQuerySet;
    }
    else
    {
        sViewQuerySet = NULL;
    }

    // BUG-38049 sSortRange를 인자로 넘겨주어야함
    IDE_TEST( qmoTwoNonPlan::initJOIN(
                  aStatement,
                  aMyGraph->graph.myQuerySet,
                  sViewQuerySet,
                  sSortRange,
                  sFilter,
                  aMyGraph->graph.myPredicate,
                  sFILT,
                  &sJOIN )
              != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sJOIN;

    //-----------------------
    // init right SORT
    //-----------------------

    sJoinFlag = 0;
    sJoinFlag &= ~QMO_JOIN_RIGHT_NODE_MASK;
    sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                  QMO_JOIN_RIGHT_NODE_MASK);

    IDE_TEST( qmg::initRightSORT4Join( aStatement,
                                       &aMyGraph->graph,
                                       sJoinFlag,
                                       ID_TRUE,
                                       sSortRange,
                                       ID_TRUE,
                                       sJOIN,
                                       &sRightSORT )
              != IDE_SUCCESS);

    // BUG-38410
    // Sort 되므로 right 는 한번만 실행되고 sort 된 내용만 참조한다.
    // 따라서 right 는 SCAN 에 대한 parallel 을 허용한다.
    aMyGraph->graph.right->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.right->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    if( aIsTwoPass == ID_TRUE )
    {
        // Two pass sort join인 경우

        //-----------------------
        // init left SORT
        //-----------------------

        sJoinFlag = 0;
        sJoinFlag &= ~QMO_JOIN_LEFT_NODE_MASK;
        sJoinFlag |= (aMyGraph->selectedJoinMethod->flag &
                      QMO_JOIN_LEFT_NODE_MASK);

        IDE_TEST( qmg::initLeftSORT4Join( aStatement,
                                          &aMyGraph->graph,
                                          sJoinFlag,
                                          sSortRange,
                                          sJOIN,
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
        // Left child의 parent로 join을 설정한다.
        sLeftSORT = sJOIN;
    }

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph,
                             sLeftSORT,   // left child의 parent는 sort 또는 join
                             sRightSORT ) // right child의 parent는 sort
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    if( aIsTwoPass == ID_TRUE )
    {
        // Two pass sort join인 경우

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
    // make JOIN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeJOIN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.type,
                                       sLeftSORT ,
                                       sRightSORT ,
                                       sJOIN ) != IDE_SUCCESS);

    aMyGraph->graph.myPlan = sJOIN;

    qmg::setPlanInfo( aStatement, sJOIN, &(aMyGraph->graph) );

    if( aIsTwoPass == ID_TRUE )
    {
        sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
        sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_TWO_PASS_SORT;
    }
    else
    {
        if ( aIsInverse == ID_TRUE )
        {
            sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_INVERSE_SORT;
        }
        else
        {
            sJOIN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
            sJOIN->flag       |= QMN_PLAN_JOIN_METHOD_ONE_PASS_SORT;
        }
    }

    //-----------------------
    // make FILT if filter exist
    //-----------------------

    IDE_TEST( makeFILT( aStatement,
                        aMyGraph,
                        sFILT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeMergeJoin( qcStatement     * aStatement,
                        qmgJOIN         * aMyGraph )
{
    UInt              sJoinFlag;
    UInt              sMtrFlag;
    qmnPlan         * sLeftSORT  = NULL;
    qmnPlan         * sRightSORT = NULL;
    qmnPlan         * sMGJN      = NULL;
    qtcNode         * sSortRange = NULL;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeMergeJoin::__FT__" );

    IDE_TEST( makeSortRange( aStatement,
                             aMyGraph,
                             &sSortRange )
              != IDE_SUCCESS );

    //-----------------------------------------------------
    //          [MGJN]
    //           /  `.
    //      [*SORT]  [*SORT]
    //         |        |
    //      [LEFT]   [RIGHT]
    // * Option
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init MGJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::initMGJN( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->joinablePredicate ,
                                       aMyGraph->nonJoinablePredicate ,
//                                       aMyGraph->graph.myPredicate,
                                       aMyGraph->graph.myPlan,
                                       &sMGJN) != IDE_SUCCESS);

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
                                      sMGJN,
                                      &sLeftSORT )
              != IDE_SUCCESS);

    // BUG-38410
    // Sort 되므로 left 는 한번만 실행되고 sort 된 내용만 참조한다.
    // 따라서 left 는 SCAN 에 대한 parallel 을 허용한다.
    aMyGraph->graph.left->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aMyGraph->graph.left->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

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
                                       ID_FALSE,
                                       sMGJN,
                                       &sRightSORT )
              != IDE_SUCCESS);

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

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    if ( sRightSORT != sMGJN )
    {
        // Right child에 SORT가 생성된 경우

        //-----------------------
        // make right SORT
        //-----------------------

        sMtrFlag = 0;

        sMtrFlag &= ~QMO_MAKESORT_METHOD_MASK;
        sMtrFlag |= QMO_MAKESORT_SORT_MERGE_JOIN;

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
    }
    else
    {
        // Right child에 SORT가 필요없는 경우

        sRightSORT = aMyGraph->graph.right->myPlan;
    }

    if ( sLeftSORT != sMGJN )
    {
        // Left child에 SORT가 생성된 경우

        //-----------------------
        // make left SORT
        //-----------------------

        sMtrFlag = 0;

        sMtrFlag &= ~QMO_MAKESORT_METHOD_MASK;
        sMtrFlag |= QMO_MAKESORT_SORT_MERGE_JOIN;

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
        // Left child에 SORT가 필요없는 경우

        sLeftSORT = aMyGraph->graph.left->myPlan;
    }

    //-----------------------
    // make MGJN
    //-----------------------

    IDE_TEST( qmoTwoNonPlan::makeMGJN( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       aMyGraph->graph.type,
                                       aMyGraph->joinablePredicate,
                                       aMyGraph->nonJoinablePredicate,
                                       sLeftSORT,
                                       sRightSORT,
                                       sMGJN )
              != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sMGJN;

    qmg::setPlanInfo( aStatement, sMGJN, &(aMyGraph->graph) );

    sMGJN->flag       &= ~QMN_PLAN_JOIN_METHOD_TYPE_MASK;
    sMGJN->flag       |= QMN_PLAN_JOIN_METHOD_MERGE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::extractFilter( qcStatement   * aStatement,
                        qmgJOIN       * aMyGraph,
                        qmoPredicate  * aPredicate,
                        qtcNode      ** aFilter )
{
    qmoPredicate * sCopiedPred;

    IDU_FIT_POINT_FATAL( "qmgJoin::extractFilter::__FT__" );

    if( aPredicate != NULL )
    {
        if( aMyGraph->graph.myQuerySet->materializeType ==
            QMO_MATERIALIZE_TYPE_VALUE )
        {
            // To fix BUG-24919
            // normalform으로 바뀐 predicate는
            // 서로 동일한 노드를 참조할 수 있으므로
            // 중간에 materialized node 가 있는 경우
            // push projection이 적용이 안될 수 있다.
            // 따라서 predicate를 복사해서 사용한다.
            IDE_TEST( qmoPred::deepCopyPredicate(
                          QC_QMP_MEM(aStatement),
                          aPredicate,
                          &sCopiedPred )
                      != IDE_SUCCESS );
        
            IDE_TEST( qmoPred::linkFilterPredicate(
                          aStatement ,
                          sCopiedPred,
                          aFilter ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmoPred::linkFilterPredicate(
                          aStatement ,
                          aPredicate,
                          aFilter ) != IDE_SUCCESS );
        }
        
        IDE_TEST( qmoPred::setPriorNodeID(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      *aFilter ) != IDE_SUCCESS );
    }
    else
    {
        *aFilter = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::initFILT( qcStatement   * aStatement,
                   qmgJOIN       * aMyGraph,
                   qmnPlan      ** aFILT )
{
    IDU_FIT_POINT_FATAL( "qmgJoin::initFILT::__FT__" );

    if( aMyGraph->graph.constantPredicate != NULL )
    {
        IDE_TEST( qmoOneNonPlan::initFILT(
                      aStatement,
                      aMyGraph->graph.myQuerySet,
                      NULL,
                      aMyGraph->graph.myPlan,
                      aFILT )
                  != IDE_SUCCESS );
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
qmgJoin::makeFILT( qcStatement  * aStatement,
                   qmgJOIN      * aMyGraph,
                   qmnPlan      * aFILT )
{
    IDU_FIT_POINT_FATAL( "qmgJoin::makeFILT::__FT__" );

    if( aMyGraph->graph.constantPredicate != NULL )
    {
        //make FILT
        IDE_TEST( qmoOneNonPlan::makeFILT(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      NULL,
                      aMyGraph->graph.constantPredicate ,
                      aMyGraph->graph.myPlan ,
                      aFILT) != IDE_SUCCESS);

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
qmgJoin::printGraph( qcStatement  * aStatement,
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

    qmgJOIN * sMyGraph;

    qmoPredicate * sPredicate;
    qmoPredicate * sMorePred;

    IDU_FIT_POINT_FATAL( "qmgJoin::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    sMyGraph = (qmgJOIN*) aGraph;

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
            qmoJoinMethodMgr::printJoinMethod( &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_NESTED],
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );

        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_SORT],
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );

        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_HASH],
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );

        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_MERGE],
                                               aDepth,
                                               aString )
            != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST(
            qmoJoinMethodMgr::printJoinMethod( &sMyGraph->joinMethods[QMG_INNER_JOIN_METHOD_NESTED],
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

IDE_RC
qmgJoin::selectJoinMethodCost( qcStatement        * aStatement,
                               qmgGraph           * aGraph,
                               qmoPredicate       * aJoinPredicate,
                               qmoJoinMethod      * aJoinMethods,
                               qmoJoinMethodCost ** aSelected )
{
/***********************************************************************
 *
 * Description : 가장 cost가 좋은 Join Method를 선택
 *
 * Implementation :
 *    (1) 각 Join Method들의 cost 계산
 *    (2) 가장 cost가 좋은 Join Method 선택
 *        - Hint가 존재할 경우
 *          A. table order까지 만족하는 Join Method 중에서 가장 cost가 좋은 것
 *             선택
 *          B. table order까지 만족하는 Join Method가 없는 경우
 *             Join Method Hint만을 만족하는 Join Method 중에서 가장 cost가
 *             좋은 것 선택
 *        - Hint가 존재하지 않을 경우
 *          가장 cost가 좋은 Join Method 선택
 *
 ***********************************************************************/

    qmsJoinMethodHints * sJoinMethodHints;
    UInt                 sJoinMethodCnt;
    UInt                 i;

    // To Fix PR-7989
    qmoJoinMethodCost *  sSelected;

    IDU_FIT_POINT_FATAL( "qmgJoin::selectJoinMethodCost::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethods != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sSelected = NULL;
    sJoinMethodHints = aGraph->myQuerySet->SFWGH->hints->joinMethod;

    if ( ( aGraph->flag & QMG_JOIN_ONLY_NL_MASK )
         == QMG_JOIN_ONLY_NL_FALSE )
    {
        switch( aGraph->type )
        {
            case QMG_INNER_JOIN:
            case QMG_SEMI_JOIN:
            case QMG_ANTI_JOIN:
                sJoinMethodCnt = 4;
                break;
            case QMG_LEFT_OUTER_JOIN:
            case QMG_FULL_OUTER_JOIN:
                sJoinMethodCnt = 3;
                break;
            default:
                IDE_FT_ASSERT( 0 );
        }
    }
    else
    {
        sJoinMethodCnt = 1;
    }

    //------------------------------------------
    // 각 Join Method들의 cost 계산 및 가장 cost가 좋은 Join Method Cost 선택
    //------------------------------------------

    for ( i = 0; i < sJoinMethodCnt; i++ )
    {
        IDE_TEST( qmoJoinMethodMgr::getBestJoinMethodCost( aStatement,
                                                           aGraph,
                                                           aJoinPredicate,
                                                           & aJoinMethods[i] )
                  != IDE_SUCCESS );
    }

    if ( sJoinMethodHints != NULL )
    {
        //------------------------------------------
        // Join Method Hint의 table order까지 만족하는 Join Method들 중에서
        // 가장 cost가 좋은 것을 선택
        //------------------------------------------

        for ( i = 0; i < sJoinMethodCnt; i++ )
        {
            if ( aJoinMethods[i].hintJoinMethod != NULL )
            {
                if ( sSelected == NULL )
                {
                    sSelected = aJoinMethods[i].hintJoinMethod;
                }
                else
                {
                    if ( sSelected->totalCost >
                         aJoinMethods[i].hintJoinMethod->totalCost )
                    {
                        sSelected = aJoinMethods[i].hintJoinMethod;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }
            else
            {
                // nothing to do
            }
        }

        //------------------------------------------
        // Join Method Hint의 table order까지 만족하는 Join Method가 없는 경우,
        // Join Method 만을 만족하는 Join Method들 중에서 가장 cost가 좋은 것
        // 선택
        //------------------------------------------

        if ( sSelected == NULL )
        {
            // To Fix BUG-10410
            for ( i = 0; i < sJoinMethodCnt; i++ )
            {
                if ( aJoinMethods[i].hintJoinMethod2 != NULL )
                {
                    if ( sSelected == NULL )
                    {
                        sSelected = aJoinMethods[i].hintJoinMethod2;
                    }
                    else
                    {
                        if ( sSelected->totalCost >
                             aJoinMethods[i].hintJoinMethod2->totalCost )
                        {
                            sSelected = aJoinMethods[i].hintJoinMethod2;
                        }
                        else
                        {
                            // nothing to do
                        }
                    }
                }
                else
                {
                    // nothing to do
                }
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    if ( sSelected == NULL )
    {
        //------------------------------------------
        // Hint가 존재하지 않거나,
        // Hint를 만족하고 feasibility가 있는 Join Method가 없는 경우
        //------------------------------------------

        for ( i = 0; i < sJoinMethodCnt; i++ )
        {
            if ( sSelected == NULL )
            {
                sSelected = aJoinMethods[i].selectedJoinMethod;
            }
            else
            {
                if ( aJoinMethods[i].selectedJoinMethod != NULL )
                {
                    if ( sSelected->totalCost >
                         aJoinMethods[i].selectedJoinMethod->totalCost )
                    {
                        sSelected = aJoinMethods[i].selectedJoinMethod;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    // nothing to do
                }
            }
        }
    }
    else
    {
        // nothing to do
    }

    /* TASK-6744 */
    if ( (QCU_PLAN_RANDOM_SEED != 0) &&
         (aStatement->mRandomPlanInfo != NULL) &&
         (smiGetStartupPhase() == SMI_STARTUP_SERVICE) )
    {
        IDE_TEST( randomSelectJoinMethod( aStatement,
                                          sJoinMethodCnt,
                                          aJoinMethods,
                                          &sSelected )
                  !=IDE_SUCCESS );

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_PLAN_RANDOM_SEED );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST_RAISE( sSelected == NULL, ERR_NOT_FOUND );

    *aSelected = sSelected;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET(ideSetErrorCode( idERR_ABORT_InternalServerError ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::afterJoinMethodDecision( qcStatement       * aStatement,
                                  qmgGraph          * aGraph,
                                  qmoJoinMethodCost * aSelectedMethod,
                                  qmoPredicate     ** aPredicate,
                                  qmoPredicate     ** aJoinablePredicate,
                                  qmoPredicate     ** aNonJoinablePredicate )
{
/***********************************************************************
 *
 * Description : Join Method 결정 후 처리
 *
 * Implementation :
 *    (1) direction에 따른 left, right 재설정
 *    (2) Join Method에 따른 Predicate의 처리
 *    (3) index argument의 설정
 *
 ***********************************************************************/

    qmgGraph      * sTmpGraph;
    qmgGraph      * sAntiRight;
    qmgGraph      * sAntiLeft;
    qmoPredicate  * sJoinablePredicate;
    qmoPredicate  * sNonJoinablePredicate;
    qmoPredicate  * sMorePredicate;
    qmoPredicate  * sPredicate;
    qtcNode       * sNode;
    qmgGraph      * sLeftNodeGraph  = NULL;
    qmgGraph      * sRightNodeGraph = NULL;
    UInt            sBucketCnt;
    UInt            sTmpTblCnt;

    // To Fix PR-8032
    // 함수의 인자를 Input과 Output 모두를 위한 용도로
    // 사용하면 안됨.
    qmoPredicate  * sPred = NULL;
    qmoPredicate  * sFirstJoinPred = NULL;
    qmoPredicate  * sCurJoinPred = NULL;
    qmoPredicate  * sJoinPred = NULL;
    qmoPredicate  * sNonJoinPred = NULL;

    IDU_FIT_POINT_FATAL( "qmgJoin::afterJoinMethodDecision::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aSelectedMethod != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sPredicate = * aPredicate;

    //------------------------------------------
    // Direction에 따른 left, right graph 위치 보정
    //------------------------------------------

    if ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
         == QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT )
    {
        sTmpGraph = aGraph->left;
        aGraph->left = aGraph->right;
        aGraph->right = sTmpGraph;
    }
    else
    {
        // nothing to do
    }

    //------------------------------------------
    // Join Method에 따른 Predicate의 처리
    //------------------------------------------

    switch ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK )
    {
        case QMO_JOIN_METHOD_FULL_NL :
            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sPredicate,
                                                          ID_TRUE )
                      != IDE_SUCCESS );

            // 제거한 predicate을 반영한다.
            *aPredicate = sPredicate;

            if ( ( ( aGraph->type == QMG_INNER_JOIN ) ||
                   ( aGraph->type == QMG_SEMI_JOIN ) ||
                   ( aGraph->type == QMG_ANTI_JOIN ) ) &&
                 ( ( aGraph->right->type == QMG_SELECTION ) ||
                   ( aGraph->right->type == QMG_PARTITION ) ) )
            {
                // 일반 Join이면서 하위 graph가 base table에 대한 Graph인 경우,
                // predicate을 하위에 내림
                if ( sPredicate != NULL )
                {
                    // BUG-43929 FULL NL join의 pushedDownPredicate를 deep copy 해야 합니다.
                    // deep copy 하지 않으면 setNonJoinPushDownPredicate에서 훼손이 됩니다.
                    IDE_TEST( qmoPred::deepCopyPredicate( QC_QMP_MEM(aStatement),
                                                          sPredicate,
                                                          &((qmgJOIN*)aGraph)->pushedDownPredicate )
                              != IDE_SUCCESS );

                    IDE_TEST( qmoPred::makeNonJoinPushDownPredicate(
                                  aStatement,
                                  sPredicate,
                                  &aGraph->right->depInfo,
                                  aSelectedMethod->flag )
                              != IDE_SUCCESS );

                    IDE_TEST( setNonJoinPushDownPredicate(
                                  aStatement,
                                  aGraph->right,
                                  &sPredicate )
                              != IDE_SUCCESS );

                    // fix BUG-12766
                    // aGraph->graph.myPredicate을 right로 내렸으므로
                    // aGraph->graph.myPredicate = NULL
                    *aPredicate = sPredicate;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // Nothing To Do
            }
            break;
        case QMO_JOIN_METHOD_INDEX :
            // indexable / non indexable predicate의 분류
            IDE_TEST(
                qmoPred::separateJoinPred( sPredicate,
                                           aSelectedMethod->joinPredicate,
                                           & sJoinPred,
                                           & sNonJoinPred )
                != IDE_SUCCESS );
            
            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sNonJoinPred,
                                                          ID_TRUE )
                      != IDE_SUCCESS );

            if ( ( aGraph->type == QMG_INNER_JOIN ) ||
                 ( aGraph->type == QMG_SEMI_JOIN ) ||
                 ( aGraph->type == QMG_ANTI_JOIN ) )
            {
                // joinable predicate을 right에 내림
                if ( sJoinPred != NULL )
                {
                    ((qmgJOIN*)aGraph)->pushedDownPredicate = sJoinPred;

                    // PROJ-1502 PARTITIONED DISK TABLE
                    // push down joinable predicate를 만든다.
                    IDE_TEST( qmoPred::makeJoinPushDownPredicate(
                                  aStatement,
                                  sJoinPred,
                                  & aGraph->right->depInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( setJoinPushDownPredicate(
                                  aStatement,
                                  aGraph->right,
                                  &sJoinPred )
                              != IDE_SUCCESS );

                    IDE_TEST( alterSelectedIndex(
                                  aStatement,
                                  aGraph->right,
                                  aSelectedMethod->rightIdxInfo->index )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }

                // non joinable predicate을 right에 내림
                if ( sNonJoinPred != NULL )
                {
                    IDE_TEST( qmoPred::makeNonJoinPushDownPredicate(
                                  aStatement,
                                  sNonJoinPred,
                                  & aGraph->right->depInfo,
                                  aSelectedMethod->flag )
                              != IDE_SUCCESS );

                    IDE_TEST( setNonJoinPushDownPredicate(
                                  aStatement,
                                  aGraph->right,
                                  &sNonJoinPred )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }
            }
            else if ( aGraph->type == QMG_LEFT_OUTER_JOIN )
            {
                // joinable predicate을 right에 내림
                if ( sJoinPred != NULL )
                {
                    ((qmgLOJN*)aGraph)->pushedDownPredicate = sJoinPred;

                    // PROJ-1502 PARTITIONED DISK TABLE
                    // push down joinable predicate를 만든다.
                    IDE_TEST( qmoPred::makeJoinPushDownPredicate(
                                  aStatement,
                                  sJoinPred,
                                  & aGraph->right->depInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( setJoinPushDownPredicate(
                                  aStatement,
                                  aGraph->right,
                                  &sJoinPred )
                              != IDE_SUCCESS );

                    IDE_TEST( alterSelectedIndex(
                                  aStatement,
                                  aGraph->right,
                                  aSelectedMethod->rightIdxInfo->index )
                              != IDE_SUCCESS );
                        
                    *aPredicate = sNonJoinPred;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }

            break;
        case QMO_JOIN_METHOD_INVERSE_INDEX :
            // indexable / non indexable predicate의 분류
            IDE_TEST(
                qmoPred::separateJoinPred( sPredicate,
                                           aSelectedMethod->joinPredicate,
                                           & sJoinPred,
                                           & sNonJoinPred )
                != IDE_SUCCESS );
            
            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sNonJoinPred,
                                                          ID_TRUE )
                      != IDE_SUCCESS );

            // Inverse Index NL은 Semi Join에서만 지원한다.
            IDE_DASSERT( aGraph->type == QMG_SEMI_JOIN );

            if ( sJoinPred != NULL )
            {
                ((qmgJOIN*)aGraph)->pushedDownPredicate = sJoinPred;

                // PROJ-1502 PARTITIONED DISK TABLE
                // push down joinable predicate를 만든다.
                IDE_TEST( qmoPred::makeJoinPushDownPredicate(
                              aStatement,
                              sJoinPred,
                              & aGraph->right->depInfo )
                          != IDE_SUCCESS );

                IDE_TEST( setJoinPushDownPredicate(
                              aStatement,
                              aGraph->right,
                              &sJoinPred )
                          != IDE_SUCCESS );

                IDE_TEST( alterSelectedIndex(
                              aStatement,
                              aGraph->right,
                              aSelectedMethod->rightIdxInfo->index )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // PROJ-2385 
            // non-joinable Predicate은 따로 복사한 뒤 Right에 Pushdown한다.
            // Join Node Filter 용도로 한 번 더 필요하기 때문이다.
            IDE_TEST( qmoPred::deepCopyPredicate( QC_QMP_MEM(aStatement),
                                                  sNonJoinPred,
                                                  &sNonJoinablePredicate )
                      != IDE_SUCCESS );

            if ( sNonJoinablePredicate != NULL )
            {
                IDE_TEST( qmoPred::makeNonJoinPushDownPredicate(
                              aStatement,
                              sNonJoinablePredicate,
                              & aGraph->right->depInfo,
                              aSelectedMethod->flag )
                          != IDE_SUCCESS );

                IDE_TEST( setNonJoinPushDownPredicate(
                              aStatement,
                              aGraph->right,
                              &sNonJoinablePredicate )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do
            }
            break;
        case QMO_JOIN_METHOD_FULL_STORE_NL :
            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sPredicate,
                                                          ID_TRUE )
                      != IDE_SUCCESS );

            // 제거한 predicate을 반영한다.
            *aPredicate = sPredicate;
            
            break;
        case QMO_JOIN_METHOD_ANTI :
            // indexable / non indexable predicate의 분류
            IDE_TEST(
                qmoPred::separateJoinPred( sPredicate,
                                           aSelectedMethod->joinPredicate,
                                           & sJoinPred,
                                           & sNonJoinPred )
                != IDE_SUCCESS );
            
            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sNonJoinPred,
                                                          ID_TRUE )
                      != IDE_SUCCESS );
            
            // left를 복사하여 antiRightGraph에 연결
            // PROJ-1446 Host variable을 포함한 질의 최적화
            // left를 sAntiRight에 복사하는 동시에
            // sAntiRight의 selectedIndex를 바꾼다.
            // 이 처리를 qmgSelection을 통해서 하는 이유는
            // selectedIndex를 바꿀 때 scan decision factor를
            // 처리해야 하기 때문이다.
            IDE_TEST( copyGraphAndAlterSelectedIndex(
                          aStatement,
                          aGraph->left,
                          &sAntiRight,
                          aSelectedMethod->leftIdxInfo->index,
                          0 ) // sAntiRight의 selectedIndex를 바꾼다.
                      != IDE_SUCCESS );

            // To Fix BUG-10191
            // left의 myPredicate을 복사해서 right의 myPredicate에 연결
            if ( aGraph->left->myPredicate != NULL )
            {
                IDE_TEST(
                    qmoPred::copyOnePredicate( QC_QMP_MEM(aStatement),
                                               aGraph->left->myPredicate,
                                               &sAntiRight->myPredicate )
                    != IDE_SUCCESS );
            }
            else
            {
                // nothing to do
            }

            // joinable Predicate을 복사해서 AntiRight에 내림
            sPred = sJoinPred;
            while ( sPred != NULL )
            {
                IDE_TEST( qmoPred::copyOnePredicate( QC_QMP_MEM(aStatement),
                                                     sPred,
                                                     &sJoinablePredicate )
                          != IDE_SUCCESS );

                sJoinablePredicate->next = NULL;

                if ( sFirstJoinPred == NULL )
                {
                    sFirstJoinPred = sCurJoinPred = sJoinablePredicate;
                }
                else
                {
                    sCurJoinPred->next = sJoinablePredicate;
                    sCurJoinPred = sCurJoinPred->next;
                }

                sPred = sPred->next;
            }

            IDE_DASSERT( sFirstJoinPred != NULL );

            IDE_TEST( qmoPred::makeJoinPushDownPredicate(
                          aStatement,
                          sFirstJoinPred,
                          & sAntiRight->depInfo )
                      != IDE_SUCCESS );

            IDE_TEST( setJoinPushDownPredicate(
                          aStatement,
                          sAntiRight,
                          &sFirstJoinPred )
                      != IDE_SUCCESS );

            // To Fix BUG-8384
            ((qmgFOJN*)aGraph)->antiRightGraph = sAntiRight;

            // right를 복사하여 antiLeftGraph에 연결
            // PROJ-1446 Host variable을 포함한 질의 최적화
            // left를 sAntiRight에 복사하는 동시에
            // sAntiRight의 selectedIndex를 바꾼다.
            // 이 처리를 qmgSelection을 통해서 하는 이유는
            // selectedIndex를 바꿀 때 scan decision factor를
            // 처리해야 하기 때문이다.
            IDE_TEST( copyGraphAndAlterSelectedIndex(
                          aStatement,
                          aGraph->right,
                          &sAntiLeft,
                          aSelectedMethod->rightIdxInfo->index,
                          1 ) // right의 selectedIndex를 바꾼다.
                      != IDE_SUCCESS );

            // To Fix BUG-8384
            ((qmgFOJN*)aGraph)->antiLeftGraph = sAntiLeft;

            // joinable Predicate을 Right에 내림
            IDE_TEST( qmoPred::makeJoinPushDownPredicate(
                          aStatement,
                          sJoinPred,
                          & aGraph->right->depInfo )
                      != IDE_SUCCESS );

            IDE_TEST( setJoinPushDownPredicate(
                          aStatement,
                          aGraph->right,
                          &sJoinPred )
                      != IDE_SUCCESS );
            
            break;
        case QMO_JOIN_METHOD_ONE_PASS_SORT : // To Fix PR-8032
        case QMO_JOIN_METHOD_TWO_PASS_SORT : // One-Pass 또는 Two Pass만 결정됨
        case QMO_JOIN_METHOD_INVERSE_SORT  : // PROJ-2385
            // sort joinable predicate/ non sort joinable predicate 분류
            IDE_TEST(
                qmoPred::separateJoinPred( sPredicate,
                                           aSelectedMethod->joinPredicate,
                                           & sJoinPred,
                                           & sNonJoinPred )
                != IDE_SUCCESS );
            
            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sNonJoinPred,
                                                          ID_TRUE )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_MERGE :
            // sort joinable predicate/ non sort joinable predicate 분류
            IDE_TEST(
                qmoPred::separateJoinPred( sPredicate,
                                           aSelectedMethod->joinPredicate,
                                           & sJoinPred,
                                           & sNonJoinPred )
                != IDE_SUCCESS );

            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sNonJoinPred,
                                                          ID_TRUE )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_ONE_PASS_HASH : // To Fix PR-8032
        case QMO_JOIN_METHOD_TWO_PASS_HASH : // One-Pass 또는 Two Pass만 결정됨
        case QMO_JOIN_METHOD_INVERSE_HASH :  // PROJ-2339

            // hash joinable predicate/ non hash joinable predicate 분류
            IDE_TEST(
                qmoPred::separateJoinPred( sPredicate,
                                           aSelectedMethod->joinPredicate,
                                           & sJoinPred,
                                           & sNonJoinPred )
                != IDE_SUCCESS );

            // PROJ-1404
            // 생성된 transitive predicate이 non-join predicate으로
            // 선택된 경우는 bad transitive predicate이므로 제거한다.
            IDE_TEST( qmoPred::removeTransitivePredicate( & sNonJoinPred,
                                                          ID_TRUE )
                      != IDE_SUCCESS );            
            break;
        default :
            IDE_DASSERT( 0 );
            break;
    }

    //------------------------------------------
    // Index Argument 설정
    //------------------------------------------

    if ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK ) 
         == QMO_JOIN_METHOD_INDEX )
    {
        // indexable join predicate은 관리자에 의해 설정됨
    }
    else
    {
        for ( sJoinablePredicate = sJoinPred;
              sJoinablePredicate != NULL;
              sJoinablePredicate = sJoinablePredicate->next )
        {
            sNode = (qtcNode*)sJoinablePredicate->node;

            if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                 == MTC_NODE_OPERATOR_OR )
            {
                sNode = (qtcNode *)sNode->node.arguments;
            }
            else
            {
                // nothing to do
            }

            // 비교 연산자 각 하위 노드에 해당하는 graph를 찾음
            IDE_TEST( qmoPred::findChildGraph( sNode,
                                               & aGraph->depInfo,
                                               aGraph->left,
                                               aGraph->right,
                                               & sLeftNodeGraph,
                                               & sRightNodeGraph )
                      != IDE_SUCCESS );

            if ( aGraph->right == sLeftNodeGraph )
            {
                sNode->indexArgument = 0;
            }
            else // aGraph->right == sRightNodeGraph
            {
                sNode->indexArgument = 1;
            }

            for ( sMorePredicate = sJoinablePredicate->more;
                  sMorePredicate != NULL;
                  sMorePredicate = sMorePredicate->more )
            {
                sNode = (qtcNode*)sMorePredicate->node;

                if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_OR )
                {
                    sNode = (qtcNode *)sNode->node.arguments;
                }
                else
                {
                    // nothing to do
                }

                // 비교 연산자 각 하위 노드에 해당하는 graph를 찾음
                IDE_TEST( qmoPred::findChildGraph( sNode,
                                                   & aGraph->depInfo,
                                                   aGraph->left,
                                                   aGraph->right,
                                                   & sLeftNodeGraph,
                                                   & sRightNodeGraph )
                          != IDE_SUCCESS );

                if ( aGraph->right == sLeftNodeGraph )
                {
                    sNode->indexArgument = 0;
                }
                else // aGraph->right == sRightNodeGraph
                {
                    sNode->indexArgument = 1;
                }
            }
        }
    }

    checkOrValueIndex( aGraph->left );
    checkOrValueIndex( aGraph->right );

    //------------------------------------------
    // hash bucket count와 hash temp table 개수를 구한다.
    //------------------------------------------

    // To-Fix 8062
    // Index Argument 결정후에 계산하여야 함.
    // hash bucket count,  hash temp table 개수 계산

    // PROJ-2385
    // Inverse Index NL
    if ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK ) == QMO_JOIN_METHOD_INVERSE_INDEX )
    {
        IDE_TEST( getBucketCntNTmpTblCnt( aStatement,
                                          aSelectedMethod,
                                          aGraph,
                                          ID_TRUE, // aIsLeftGraph
                                          & sBucketCnt,
                                          & sTmpTblCnt )
                  != IDE_SUCCESS );

        // Semi Join에서만 Inverse Index NL 지원
        IDE_DASSERT( aGraph->type == QMG_SEMI_JOIN );

        ((qmgJOIN *)aGraph)->hashBucketCnt = sBucketCnt;
        ((qmgJOIN *)aGraph)->hashTmpTblCnt = sTmpTblCnt;
    }
    else
    {
        if ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_HASH ) 
             != QMO_JOIN_METHOD_NONE )
        {
            IDE_TEST( getBucketCntNTmpTblCnt( aStatement,
                                              aSelectedMethod,
                                              aGraph,
                                              ID_FALSE, // aIsLeftGraph
                                              & sBucketCnt,
                                              & sTmpTblCnt )
                      != IDE_SUCCESS );
    
            switch ( aGraph->type )
            {
                case QMG_INNER_JOIN :
                case QMG_SEMI_JOIN :
                case QMG_ANTI_JOIN :
                    ((qmgJOIN *)aGraph)->hashBucketCnt = sBucketCnt;
                    ((qmgJOIN *)aGraph)->hashTmpTblCnt = sTmpTblCnt;
                    break;
                case QMG_FULL_OUTER_JOIN :
                    ((qmgFOJN *)aGraph)->hashBucketCnt = sBucketCnt;
                    ((qmgFOJN *)aGraph)->hashTmpTblCnt = sTmpTblCnt;
                    break;
                case QMG_LEFT_OUTER_JOIN :
                    ((qmgLOJN *)aGraph)->hashBucketCnt = sBucketCnt;
                    ((qmgLOJN *)aGraph)->hashTmpTblCnt = sTmpTblCnt;
                    break;
                default :
                    IDE_DASSERT( 0 );
                    break;
            }
        }
        else
        {
            // Inverse Index NL이나 Hash Join이 아닌 경우로 Bucket Count 결정이 필요없음.
            // Nothing To Do
        }
    }

    *aJoinablePredicate = sJoinPred;
    *aNonJoinablePredicate = sNonJoinPred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makePreservedOrder( qcStatement        * aStatement,
                             qmgGraph           * aGraph,
                             qmoJoinMethodCost  * aSelectedMethod,
                             qmoPredicate       * aJoinablePredicate )
{
/***********************************************************************
 *
 * Description : Preserved Order의 설정
 *
 * Implementation :
 *    To Fix PR-11556
 *       Join Predicate에 대한 Preserved Order를 고려하지 않는다.
 *       효용성이 떨어지는 반면, 복잡도만 증가하게 된다.
 *       다음 예에서 L1.i1 과 R1.a1이 동일한 데 이걸 사용할까?
 *           - SELECT DISTINCT L1.i1, R1.a1 FROM L1, R1 WHERE L1.i1 = R1.a1;
 *           - SELECT L1.i1, R1.a1 FROM L1, R1 WHERE L1.i1 = R1.a1
 *             GROUP BY L1.i1, R1.a1;
 *           - SELECT * FROM L1, R1 WHERE L1.i1 = R1.a1
 *             GROUP BY L1.i1, R1.a1;
 *       BUGBUG
 *       sort-based join은 부등호도 가능하며 이때에는 preserved order를 고려하면
 *       성능 이익을 볼수 있다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgJoin::makePreservedOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aSelectedMethod != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    switch( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK )
    {
        case QMO_JOIN_METHOD_FULL_NL :
        case QMO_JOIN_METHOD_FULL_STORE_NL :

            // Left Child Graph의 Preserved Order를 이용하여
            // Join Graph의 preserved order를 결정한다.
            IDE_TEST( makeOrderFromChild( aStatement, 
                                          aGraph,
                                          ID_FALSE ) // aIsRightGraph 
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INDEX :

            // Left Child Graph의 Preserved Order를 이용하여
            // Join Graph의 preserved order를 결정한다.
            IDE_TEST( makeOrderFromChild( aStatement, 
                                          aGraph,
                                          ID_FALSE ) // aIsRightGraph 
                      != IDE_SUCCESS );

            //------------------------------------------
            // right의 index에 따른 preserved order 생성
            //    < right의 preserved order가 존재하는 경우 >
            //    - right의 preserved order와 선택된 join method의
            //      right index가 같은 경우, right preserved order 그대로 사용
            //    - right의 preserved order와 선택된 join method의
            //      right index가 다른 경우, right preserved order 새로 생성
            //    < right의 preserved order가 존재하지 않는 경우 >
            //      right의 index에 따른 preserved order 생성하여
            //      right에 내려준다.
            //------------------------------------------

            IDE_TEST( makeNewOrder( aStatement,
                                    aGraph->right,
                                    aSelectedMethod->rightIdxInfo->index )
                      != IDE_SUCCESS );

            break;
        case QMO_JOIN_METHOD_INVERSE_INDEX :
            
            // Left Child Graph의 Preserved Order를 보존할 수 없다.
            // 대신, right의 index와 비교해 새 Order를 만들어야 한다.
            aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
            aGraph->flag |= QMG_PRESERVED_ORDER_NEVER;

            IDE_TEST( makeNewOrder( aStatement,
                                    aGraph->right,
                                    aSelectedMethod->rightIdxInfo->index )
                      != IDE_SUCCESS );

            break;
        case QMO_JOIN_METHOD_ANTI :

            // Left Child Graph의 Preserved Order를 이용하여
            // Join Graph의 preserved order를 결정한다.
            IDE_TEST( makeOrderFromChild( aStatement, 
                                          aGraph,
                                          ID_FALSE ) // aIsRightGraph 
                      != IDE_SUCCESS );

            IDE_TEST( makeNewOrder( aStatement,
                                    aGraph->right,
                                    aSelectedMethod->rightIdxInfo->index )
                      != IDE_SUCCESS );

            IDE_TEST( makeNewOrder( aStatement,
                                    ((qmgFOJN*)aGraph)->antiRightGraph,
                                    aSelectedMethod->leftIdxInfo->index )
                      != IDE_SUCCESS );

            break;
        case QMO_JOIN_METHOD_ONE_PASS_HASH :
        case QMO_JOIN_METHOD_ONE_PASS_SORT :

            // Left Child Graph의 Preserved Order를 이용하여
            // Join Graph의 preserved order를 결정한다.
            IDE_TEST( makeOrderFromChild( aStatement, 
                                          aGraph,
                                          ID_FALSE ) // aIsRightGraph 
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_TWO_PASS_HASH :
        case QMO_JOIN_METHOD_INVERSE_HASH  : // PROJ-2339 

            // Two Pass, Inverse Hash Join은 preserverd order가 존재하지 않는다.
            aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
            aGraph->flag |= QMG_PRESERVED_ORDER_NEVER;
            break;

        case QMO_JOIN_METHOD_TWO_PASS_SORT :
        case QMO_JOIN_METHOD_INVERSE_SORT :
        case QMO_JOIN_METHOD_MERGE :

            // Join Predicate을 이용하여 Preserved Order를 생성한다.
            // Merge Join의 경우 ASCENDING만이 가능하다.
            IDE_TEST( makeOrderFromJoin( aStatement,
                                         aGraph,
                                         aSelectedMethod,
                                         QMG_DIRECTION_ASC,
                                         aJoinablePredicate )
                      != IDE_SUCCESS );
            break;
        default :
            // nothing to do
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmgJoin::getBucketCntNTmpTblCnt( qcStatement       * aStatement,
                                        qmoJoinMethodCost * aSelectedMethod,
                                        qmgGraph          * aGraph,
                                        idBool              aIsLeftGraph,
                                        UInt              * aBucketCnt,
                                        UInt              * aTmpTblCnt )
{
/***********************************************************************
 *
 * Description : hash bucket count와 hash temp table count를 구하는 함수
 *
 * Implementation :
 *    (1) hash bucket count의 설정
 *    (2) hash temp table count의 설정
 *
 ***********************************************************************/

    UInt             sBucketCnt;
    SDouble          sBucketCntTemp;
    SDouble          sMaxBucketCnt;
    SDouble          sTmpTblCnt;

    IDU_FIT_POINT_FATAL( "qmgJoin::getBucketCntNTmpTblCnt::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aSelectedMethod != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // hash bucket count 설정
    //------------------------------------------

    if ( aGraph->myQuerySet->SFWGH->hints->hashBucketCnt !=
         QMS_NOT_DEFINED_BUCKET_CNT )
    {
        //------------------------------------------
        // hash bucket count hint가 존재하는 경우
        //------------------------------------------

        sBucketCnt = aGraph->myQuerySet->SFWGH->hints->hashBucketCnt;
    }
    else
    {
        //------------------------------------------
        // hash bucket count hint가 존재하지 않는 경우
        //------------------------------------------

        //------------------------------------------
        // 기본 bucket count 설정
        // bucket count = 하위 graph의 ouput record count / 2
        //------------------------------------------

        // BUG-37778 disk hash temp table size estimate
        // Hash Join 의 bucket 갯수는 ditinct 하게 동작하지 않으므로
        // 컬럼의 ndv 를 참조하지 않아도 된다.

        // PROJ-2385
        // BUG-40561
        if ( aIsLeftGraph == ID_TRUE )
        {
            sBucketCntTemp = ( aGraph->left->costInfo.outputRecordCnt / 2 );

            if ((aGraph->left->flag & QMG_GRAPH_TYPE_MASK) == QMG_GRAPH_TYPE_MEMORY)
            {
                // bucketCnt가 QMC_MEM_HASH_MAX_BUCKET_CNT(10240000)를 넘으면
                // QMC_MEM_HASH_MAX_BUCKET_CNT 값으로 보정해준다.
                /* BUG-48161 */
                sBucketCnt = (UInt)IDL_MIN(sBucketCntTemp, QCG_GET_BUCKET_COUNT_MAX( aStatement ) );
            }
            else
            {
                sBucketCnt = (UInt)IDL_MIN(sBucketCntTemp, ID_UINT_MAX);
            }
        }
        else
        {
            sBucketCntTemp = ( aGraph->right->costInfo.outputRecordCnt / 2 );

            if ((aGraph->right->flag & QMG_GRAPH_TYPE_MASK) == QMG_GRAPH_TYPE_MEMORY)
            {
                // bucketCnt가 QMC_MEM_HASH_MAX_BUCKET_CNT(10240000)를 넘으면
                // QMC_MEM_HASH_MAX_BUCKET_CNT 값으로 보정해준다.
                /* BUG-48161 */
                sBucketCnt = (UInt)IDL_MIN(sBucketCntTemp, QCG_GET_BUCKET_COUNT_MAX( aStatement ) );
            }
            else
            {
                sBucketCnt = (UInt)IDL_MIN(sBucketCntTemp, ID_UINT_MAX);
            }
        }

        //  hash bucket count의 보정
        if ( sBucketCnt < QCU_OPTIMIZER_BUCKET_COUNT_MIN )
        {
            sBucketCnt = QCU_OPTIMIZER_BUCKET_COUNT_MIN;
        }
        else
        {
            /* nothing to do */
        }
    }

    *aBucketCnt = sBucketCnt;

    //------------------------------------------
    // hash temp table count 설정
    //------------------------------------------

    *aTmpTblCnt = 0;
    if ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK ) ==
         QMO_JOIN_METHOD_TWO_PASS_HASH )
    {
        // two pass hash join method가 선택된 경우
        if ( aSelectedMethod->hashTmpTblCntHint
             != QMS_NOT_DEFINED_TEMP_TABLE_CNT )
        {
            // hash temp table count hint가 주어진 경우, 이를 따름
            *aTmpTblCnt = aSelectedMethod->hashTmpTblCntHint;
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    if ( *aTmpTblCnt == 0 )
    {
        // BUG-12581 fix
        // PROJ-2339, 2385
        if ( ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK ) ==
               QMO_JOIN_METHOD_ONE_PASS_HASH ) ||
             ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK ) ==
               QMO_JOIN_METHOD_INVERSE_HASH ) ||
             ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK ) ==
               QMO_JOIN_METHOD_INVERSE_INDEX ) )
        {
            *aTmpTblCnt = 1;
        }
        else
        {
            // PROJ-2242
            // hash temp table의 메모리공간에 저장 가능한 BucketCnt 의 최대치를 계산한후
            // 총 BucketCnt 에서 나눈값을 사용한다.
            sMaxBucketCnt = smuProperty::getHashAreaSize() *
                (smuProperty::getTempHashGroupRatio() / 100.0 ) / 8.0;

            sTmpTblCnt = *aBucketCnt / sMaxBucketCnt;

            if ( sTmpTblCnt > 1.0 )
            {
                *aTmpTblCnt = (UInt)idlOS::ceil( sTmpTblCnt );
            }
            else
            {
                *aTmpTblCnt = 1;
            }
        }
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeOrderFromChild( qcStatement * aStatement,
                             qmgGraph    * aGraph,
                             idBool        aIsRightGraph )
{
/***********************************************************************
 *
 * Description :
 *    Left 또는 Right Child Graph의 Preserved Order를 복사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgPreservedOrder * sGraphOrder = NULL;
    qmgPreservedOrder * sCurrOrder  = NULL;
    qmgPreservedOrder * sNewOrder   = NULL;
    qmgPreservedOrder * sChildOrder = NULL;
    qmgGraph          * sChildGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeOrderFromChild::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sGraphOrder = NULL;

    if ( aIsRightGraph == ID_FALSE )
    {
        // Right Graph의 Order를 사용하는 경우
        sChildGraph = aGraph->left;
    }
    else
    {
        // Left Graph의 Order를 사용하는 경우
        sChildGraph = aGraph->right;
    }

    //------------------------------------------
    // Preserved Order를 복사한다.
    //------------------------------------------

    for ( sChildOrder = sChildGraph->preservedOrder;
          sChildOrder != NULL;
          sChildOrder = sChildOrder->next )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                 (void**) & sNewOrder )
                  != IDE_SUCCESS );

        idlOS::memcpy( sNewOrder,
                       sChildOrder,
                       ID_SIZEOF(qmgPreservedOrder ) );

        sNewOrder->next = NULL;

        if ( sGraphOrder == NULL )
        {
            sGraphOrder = sNewOrder;
            sCurrOrder = sNewOrder;
        }
        else
        {
            sCurrOrder->next = sNewOrder;
            sCurrOrder = sCurrOrder->next;
        }
    }

    //------------------------------------------
    // Join Graph의 Preserved Order정보 설정
    //------------------------------------------

    aGraph->preservedOrder = sGraphOrder;

    aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
    aGraph->flag |= ( sChildGraph->flag & QMG_PRESERVED_ORDER_MASK );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeOrderFromJoin( qcStatement        * aStatement,
                            qmgGraph           * aGraph,
                            qmoJoinMethodCost  * aSelectedMethod,
                            qmgDirectionType     aDirection,
                            qmoPredicate       * aJoinablePredicate )
{
/***********************************************************************
 *
 * Description :
 *    Join Predicate으로부터 Preserved Order 생성
 *    아래와 같이 Left의 정렬 순서가 보장되는 Join Method를 위하여
 *    Preserverd Order를 생성한다.
 *       - Two Pass Sort Join
 *       - Inverse Sort Join  (PROJ-2385)
 *       - Merge Join
 *
 * Implementation :
 *
 *    Left Graph에 대한 Presererved Order와
 *    Right Graph에 대한 Preserverd Order를 생성한다.
 *
 *    자신에 대한 Preserverd Order는 다음 중 하나를 결정한다.
 *       - Left Graph로부터 얻을 수 있는 경우(최대한 활용)
 *       - 없는 경우, 자신의 Graph에만 생성
 *
 ***********************************************************************/

    qmgPreservedOrder * sNewOrder               = NULL;
    qmgPreservedOrder * sLeftOrder              = NULL;
    qmgPreservedOrder * sRightOrder             = NULL;
    qtcNode           * sNode                   = NULL;
    qtcNode           * sRightNode              = NULL;
    qtcNode           * sLeftNode               = NULL;

    idBool              sCanUseFromLeftGraph    = ID_FALSE;
    idBool              sCanUseFromRightGraph   = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeOrderFromJoin::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );
    IDE_FT_ASSERT( aSelectedMethod != NULL );
    IDE_FT_ASSERT( aJoinablePredicate != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNode = aJoinablePredicate->node;

    if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_OR )
    {
        sNode = (qtcNode *)sNode->node.arguments;
    }
    else
    {
        // nothing to do
    }

    if ( sNode->indexArgument == 1 )
    {
        sLeftNode  = (qtcNode*)sNode->node.arguments;
        sRightNode = (qtcNode*)sLeftNode->node.next;
    }
    else
    {
        sRightNode = (qtcNode*)sNode->node.arguments;
        sLeftNode  = (qtcNode*)sRightNode->node.next;
    }

    //--------------------------------------------
    // Join Predicate에 대한 Preserved Order 생성
    //--------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                             (void**)&sNewOrder )
              != IDE_SUCCESS );
    sNewOrder->table = sLeftNode->node.table;
    sNewOrder->column = sLeftNode->node.column;
    sNewOrder->direction = aDirection;
    sNewOrder->next = NULL;

    sLeftOrder = sNewOrder;

    //--------------------------------------------
    // Left Graph에 대한 Preserved Order 생성
    //--------------------------------------------

    // fix BUG-9737
    // merge join일 경우 left child와 right child를 분리시켜
    // preserved order생성

    if ( ( aSelectedMethod->flag &
           QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_MASK ) ==
         QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_TRUE )
    {
        // To Fix PR-8062
        if ( aGraph->left->preservedOrder == NULL )
        {
            // To Fix PR-8110
            // 원하는 Child만을 입력 인자로 사용해야 함.
            IDE_TEST( qmg::setOrder4Child( aStatement,
                                           sNewOrder,
                                           aGraph->left )
                      != IDE_SUCCESS );
            sCanUseFromLeftGraph = ID_TRUE;
        }
        else
        {
            if ( aGraph->left->preservedOrder->direction ==
                 QMG_DIRECTION_NOT_DEFINED )
            {
                // To Fix PR-8110
                // 원하는 Child만을 입력 인자로 사용해야 함.
                IDE_TEST( qmg::setOrder4Child( aStatement,
                                               sNewOrder,
                                               aGraph->left )
                          != IDE_SUCCESS );
                sCanUseFromLeftGraph = ID_TRUE;
            }
            else
            {
                sCanUseFromLeftGraph = ID_FALSE;
            }
        }
    }
    else
    {
        sCanUseFromLeftGraph = ID_FALSE;
    }

    //--------------------------------------------
    // Right Graph에 대한 Preserved Order 생성
    //--------------------------------------------

    // fix BUG-9737
    // merge join일 경우 left child와 right child를 분리시켜
    // preserved order생성
    if( QTC_IS_COLUMN( aStatement, sRightNode ) == ID_TRUE )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                 (void**)&sNewOrder )
                  != IDE_SUCCESS );

        sNewOrder->table = sRightNode->node.table;
        sNewOrder->column = sRightNode->node.column;
        sNewOrder->direction = QMG_DIRECTION_ASC;
        sNewOrder->next = NULL;

        sRightOrder = sNewOrder;

        if ( ( aSelectedMethod->flag &
               QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_MASK ) ==
             QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_TRUE )
        {
            // To Fix PR-8062
            if ( aGraph->right->preservedOrder == NULL )
            {
                // To Fix PR-8110
                // 원하는 Child만을 입력 인자로 사용해야 함.
                IDE_TEST( qmg::setOrder4Child( aStatement,
                                               sNewOrder,
                                               aGraph->right )
                          != IDE_SUCCESS );
                sCanUseFromRightGraph = ID_TRUE;
            }
            else
            {
                // To Fix PR-8110
                // 원하는 Child만을 입력 인자로 사용해야 함.
                if ( aGraph->right->preservedOrder->direction ==
                     QMG_DIRECTION_NOT_DEFINED )
                {
                    IDE_TEST( qmg::setOrder4Child( aStatement,
                                                   sNewOrder,
                                                   aGraph->right )
                              != IDE_SUCCESS );
                    sCanUseFromRightGraph = ID_TRUE;
                }
                else
                {
                    sCanUseFromRightGraph = ID_FALSE;
                }
            }
        }
        else
        {
            sCanUseFromRightGraph = ID_FALSE;
        }
    }
    else
    {
        // nothing to do
    }

    //--------------------------------------------
    // Join Graph에 대한 Preserved Order 생성
    //--------------------------------------------

    // PROJ-2385 Inverse Sort
    if ( ( aSelectedMethod->flag & QMO_JOIN_METHOD_MASK )
         == QMO_JOIN_METHOD_INVERSE_SORT )
    {
        if ( sCanUseFromRightGraph == ID_TRUE )
        {
            // Right Graph의 Preserved Order를 최대한 활용할 수 있는 경우
            IDE_TEST( makeOrderFromChild( aStatement, 
                                          aGraph, 
                                          ID_TRUE ) // aIsRightGraph
                      != IDE_SUCCESS );
        }
        else
        {
            // Right Graph에는 별도의 Preserved Order가 없는 경우
            aGraph->preservedOrder = sRightOrder;
        }
    }
    else
    {
        // Two-Pass Sort, Merge
        if ( sCanUseFromLeftGraph == ID_TRUE )
        {
            // Left Graph의 Preserved Order를 최대한 활용할 수 있는 경우
            IDE_TEST( makeOrderFromChild( aStatement, 
                                          aGraph,
                                          ID_FALSE ) // aIsRightGraph 
                      != IDE_SUCCESS );
        }
        else
        {
            // Left Graph에는 별도의 Preserved Order가 없는 경우
            aGraph->preservedOrder = sLeftOrder;
        }
    }

    if ( aGraph->preservedOrder != NULL )
    {
        aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
        aGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
    }
    else
    {
        /* BUG-43697 Inverse Sort이면, Right Graph의 Preserved Order가 NULL이 될 수 있다.
         * 이 경우에는 Preserved Order를 사용하지 않는다. 
         */
        aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
        aGraph->flag |= QMG_PRESERVED_ORDER_NEVER;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeNewOrder( qcStatement       * aStatement,
                       qmgGraph          * aGraph,
                       qcmIndex          * aSelectedIndex )
{
/***********************************************************************
 *
 * Description : 새로운 Preserved Order의 생성 및 설정
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgJoin::makeNewOrder::__FT__" );

    if( aGraph->type == QMG_SELECTION )
    {
        IDE_TEST( makeNewOrder4Selection( aStatement,
                                          aGraph,
                                          aSelectedIndex )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aGraph->type == QMG_PARTITION )
        {
            IDE_TEST( makeNewOrder4Partition( aStatement,
                                              aGraph,
                                              aSelectedIndex )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2638
            IDE_FT_ASSERT( aGraph->type == QMG_SHARD_SELECT );
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::makeNewOrder4Selection( qcStatement        * aStatement,
                                 qmgGraph           * aGraph,
                                 qcmIndex           * aSelectedIndex )
{
/***********************************************************************
 *
 * Description : 새로운 Preserved Order의 생성 및 설정
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool              sNeedNewChildOrder;
    qmgPreservedOrder * sNewOrder;
    qmgPreservedOrder * sFirstOrder = NULL;
    qmgPreservedOrder * sCurOrder = NULL;
    UInt                sKeyColCnt;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeNewOrder4Selection::__FT__" );

    IDE_DASSERT( (aGraph->type == QMG_SELECTION) ||
                 (aGraph->type == QMG_PARTITION) );

    if ( aGraph->preservedOrder != NULL )
    {
        if ( ( aGraph->preservedOrder->table ==
               aGraph->myFrom->tableRef->table ) &&
             ( (UInt) aGraph->preservedOrder->column ==
               ( aSelectedIndex->keyColumns[0].column.id %
                 SMI_COLUMN_ID_MAXIMUM ) ) )
        {
            //------------------------------------------
            // preserved order와 선택된 join method의 right index가 같은 경우
            //------------------------------------------

            sNeedNewChildOrder = ID_FALSE;
        }
        else
        {
            sNeedNewChildOrder = ID_TRUE;
        }
    }
    else
    {
        sNeedNewChildOrder = ID_TRUE;
    }

    if ( sNeedNewChildOrder == ID_TRUE )
    {
        //------------------------------------------
        // - preserved order가 없는 경우
        // - preserved order가 있고
        //   선택된 method가 사용하는 index가 다른 경우
        //------------------------------------------

        // To Fix BUG-9631
        sKeyColCnt = aSelectedIndex->keyColCount;

        for ( i = 0; i < sKeyColCnt; i++ )
        {
            IDE_TEST(
                QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                               (void**)&sNewOrder )
                != IDE_SUCCESS );

            if( ( aGraph->flag & QMG_SELT_PARTITION_MASK ) ==
                QMG_SELT_PARTITION_TRUE )
            {
                sNewOrder->table = ((qmgSELT*)aGraph)->partitionRef->table;
            }
            else
            {
                sNewOrder->table = aGraph->myFrom->tableRef->table;
            }

            sNewOrder->column = aSelectedIndex->keyColumns[i].column.id
                % SMI_COLUMN_ID_MAXIMUM;
            sNewOrder->next = NULL;

            if ( i == 0 )
            {
                sNewOrder->direction = QMG_DIRECTION_NOT_DEFINED;
                sFirstOrder = sCurOrder = sNewOrder;
            }
            else
            {
                sNewOrder->direction = QMG_DIRECTION_SAME_WITH_PREV;
                sCurOrder->next = sNewOrder;
                sCurOrder = sCurOrder->next;
            }
        }

        aGraph->preservedOrder = sFirstOrder;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmgJoin::makeNewOrder4Partition( qcStatement        * aStatement,
                                        qmgGraph           * aGraph,
                                        qcmIndex           * aSelectedIndex )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                partitioned table은 preserved order가 없다.
 *                TODO1502: 향후 preserved order를 만들 여지는 있음.
 *                하지만, children(selection) graph에 대해서는
 *                preserved order가 있으므로, 이를 바꿔준다.
 *
 *                PROJ-1624 global non-partitioned index
 *                index table scan인 경우 preserved order가 있으므로,
 *                이를 바꿔준다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmgChildren  * sChild;

    IDU_FIT_POINT_FATAL( "qmgJoin::makeNewOrder4Partition::__FT__" );

    IDE_DASSERT( aGraph->type == QMG_PARTITION );

    // PROJ-1624 global non-partitioned index
    // global index는 partitioned table에만 생성되어 있다.
    if ( aSelectedIndex->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
    {
        IDE_TEST( makeNewOrder4Selection( aStatement,
                                          aGraph,
                                          aSelectedIndex )
                  != IDE_SUCCESS );
    }
    else
    {
        for( sChild = aGraph->children;
             sChild != NULL;
             sChild = sChild->next )
        {
            IDE_TEST( makeNewOrder4Selection( aStatement,
                                              sChild->childGraph,
                                              aSelectedIndex )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Preserved Order의 direction을 결정한다.
 *                direction이 NOT_DEFINED 일 경우에만 호출하여야 한다.
 *
 *  Implementation :
 *    
 ***********************************************************************/

    UInt                sSelectedJoinMethodFlag = 0;
    qmgPreservedOrder * sPreservedOrder;
    idBool              sIsSamePrevOrderWithChild;
    qmgDirectionType    sPrevDirection;

    IDU_FIT_POINT_FATAL( "qmgJoin::finalizePreservedOrder::__FT__" );

    // BUG-36803 Join Graph must have a left child graph
    IDE_DASSERT( aGraph->left != NULL );

    switch( aGraph->type )
    {
        case QMG_INNER_JOIN :
        case QMG_SEMI_JOIN :
        case QMG_ANTI_JOIN :
            sSelectedJoinMethodFlag = ((qmgJOIN *)aGraph)->selectedJoinMethod->flag;
            break;
        case QMG_FULL_OUTER_JOIN :
            sSelectedJoinMethodFlag = ((qmgFOJN *)aGraph)->selectedJoinMethod->flag;
            break;
        case QMG_LEFT_OUTER_JOIN :
            sSelectedJoinMethodFlag = ((qmgLOJN *)aGraph)->selectedJoinMethod->flag;
            break;
        default :
            IDE_DASSERT(0);
            break;
    }
    
    switch( sSelectedJoinMethodFlag & QMO_JOIN_METHOD_MASK )
    {
        case QMO_JOIN_METHOD_FULL_NL :
        case QMO_JOIN_METHOD_FULL_STORE_NL :
        case QMO_JOIN_METHOD_ONE_PASS_HASH :
        case QMO_JOIN_METHOD_ONE_PASS_SORT :
        case QMO_JOIN_METHOD_INDEX :
        case QMO_JOIN_METHOD_ANTI :
            // Copy direction from Left child graph's Preserved order
            IDE_TEST( qmg::copyPreservedOrderDirection(
                          aGraph->preservedOrder,
                          aGraph->left->preservedOrder )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_INVERSE_SORT :
            IDE_TEST( qmg::copyPreservedOrderDirection(
                          aGraph->preservedOrder,
                          aGraph->right->preservedOrder )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_MERGE :
        case QMO_JOIN_METHOD_TWO_PASS_SORT :

            sIsSamePrevOrderWithChild =
                qmg::isSamePreservedOrder( aGraph->preservedOrder,
                                           aGraph->left->preservedOrder );

            if ( sIsSamePrevOrderWithChild == ID_TRUE )
            {
                // Child graph의 Preserved order direction을 복사한다.
                IDE_TEST( qmg::copyPreservedOrderDirection(
                              aGraph->preservedOrder,
                              aGraph->left->preservedOrder )
                          != IDE_SUCCESS );
            }
            else
            {
                // 하위 preserved order를 따르지 않고
                // 새로 preserved order를 생성한 경우,
                // Preserved Order의 direction을 acsending으로 설정
                
                sPreservedOrder = aGraph->preservedOrder;

                // 첫번째 칼럼은 ascending으로 설정
                sPreservedOrder->direction = QMG_DIRECTION_ASC;
                sPrevDirection = QMG_DIRECTION_ASC;

                // 두번째 칼럼은 이전 칼럼의 direction 정보에 따라 수행함
                for ( sPreservedOrder = sPreservedOrder->next;
                      sPreservedOrder != NULL;
                      sPreservedOrder = sPreservedOrder->next )
                {
                    switch( sPreservedOrder->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED :
                            sPreservedOrder->direction = QMG_DIRECTION_ASC;
                            break;
                        case QMG_DIRECTION_SAME_WITH_PREV :
                            sPreservedOrder->direction = sPrevDirection;
                            break;
                        case QMG_DIRECTION_DIFF_WITH_PREV :
                            // direction이 이전 칼럼의 direction과 다를 경우
                            if ( sPrevDirection == QMG_DIRECTION_ASC )
                            {
                                sPreservedOrder->direction = QMG_DIRECTION_DESC;
                            }
                            else
                            {
                                // sPrevDirection == QMG_DIRECTION_DESC
                                sPreservedOrder->direction = QMG_DIRECTION_ASC;
                            }
                            break;
                        case QMG_DIRECTION_ASC :
                            IDE_DASSERT(0);
                            break;
                        case QMG_DIRECTION_DESC :
                            IDE_DASSERT(0);
                            break;
                        case QMG_DIRECTION_ASC_NULLS_FIRST :
                            IDE_DASSERT(0);
                            break;
                        case QMG_DIRECTION_ASC_NULLS_LAST :
                            IDE_DASSERT(0);
                            break;
                        case QMG_DIRECTION_DESC_NULLS_FIRST :
                            IDE_DASSERT(0);
                            break;
                        case QMG_DIRECTION_DESC_NULLS_LAST :
                            IDE_DASSERT(0);
                            break;
                        default:
                            IDE_DASSERT(0);
                            break;
                    }
            
                    sPrevDirection = sPreservedOrder->direction;
                }
            }
            break;

        case QMO_JOIN_METHOD_TWO_PASS_HASH :
        case QMO_JOIN_METHOD_INVERSE_HASH  : // PROJ-2339
        case QMO_JOIN_METHOD_INVERSE_INDEX : // PROJ-2385
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::setJoinPushDownPredicate( qcStatement   * aStatement,
                                   qmgGraph      * aGraph,
                                   qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 * Description : push-down join predicate를 받아서 그래프에 연결.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgJoin::setJoinPushDownPredicate::__FT__" );

    if ( aGraph->type == QMG_SELECTION )
    {
        IDE_TEST( qmgSelection::setJoinPushDownPredicate(
                      (qmgSELT*)aGraph,
                      aPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aGraph->type == QMG_PARTITION )
        {
            IDE_TEST( qmgPartition::setJoinPushDownPredicate(
                          aStatement,
                          (qmgPARTITION*)aGraph,
                          aPredicate )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2638
            IDE_FT_ASSERT( aGraph->type == QMG_SHARD_SELECT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::setNonJoinPushDownPredicate( qcStatement   * aStatement,
                                      qmgGraph      * aGraph,
                                      qmoPredicate ** aPredicate )
{
/***********************************************************************
 *
 * Description : push-down non-join predicate를 받아서 자신의 그래프에 연결.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgJoin::setNonJoinPushDownPredicate::__FT__" );

    if( aGraph->type == QMG_SELECTION )
    {
        IDE_TEST( qmgSelection::setNonJoinPushDownPredicate(
                      (qmgSELT*)aGraph,
                      aPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aGraph->type == QMG_PARTITION )
        {
            IDE_TEST( qmgPartition::setNonJoinPushDownPredicate(
                          aStatement,
                          (qmgPARTITION*)aGraph,
                          aPredicate )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2638
            IDE_FT_ASSERT( aGraph->type == QMG_SHARD_SELECT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::alterSelectedIndex( qcStatement * aStatement,
                             qmgGraph    * aGraph,
                             qcmIndex    * aNewIndex )
{
/***********************************************************************
 *
 * Description : 상위 graph에 의해 access method가 바뀐 경우
 *               selection graph의 sdf를 disable 시킨다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgJoin::alterSelectedIndex::__FT__" );

    if( aGraph->type == QMG_SELECTION )
    {
        IDE_TEST( qmgSelection::alterSelectedIndex(
                      aStatement,
                      (qmgSELT*)aGraph,
                      aNewIndex )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aGraph->type == QMG_PARTITION )
        {
            IDE_TEST( qmgPartition::alterSelectedIndex(
                          aStatement,
                          (qmgPARTITION*)aGraph,
                          aNewIndex )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2638
            IDE_FT_ASSERT( aGraph->type == QMG_SHARD_SELECT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgJoin::copyGraphAndAlterSelectedIndex( qcStatement * aStatement,
                                         qmgGraph    * aSource,
                                         qmgGraph   ** aTarget,
                                         qcmIndex    * aNewSelectedIndex,
                                         UInt          aWhichOne )
{
/***********************************************************************
 *
 * Description : 상위 JOIN graph에서 ANTI로 처리할 때
 *               하위 SELT graph를 복사하는데 이때 이 함수를
 *               통해서 복사하도록 해야 안전하다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmgJoin::copyGraphAndAlterSelectedIndex::__FT__" );

    if ( aSource->type == QMG_SELECTION )
    {
        IDE_TEST( qmgSelection::copySELTAndAlterSelectedIndex(
                      aStatement,
                      (qmgSELT*)aSource,
                      (qmgSELT**)aTarget,
                      aNewSelectedIndex,
                      aWhichOne )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aSource->type == QMG_PARTITION )
        {
            IDE_TEST( qmgPartition::copyPARTITIONAndAlterSelectedIndex(
                          aStatement,
                          (qmgPARTITION*)aSource,
                          (qmgPARTITION**)aTarget,
                          aNewSelectedIndex,
                          aWhichOne )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2638
            IDE_FT_ASSERT( aSource->type == QMG_SHARD_SELECT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmgJoin::randomSelectJoinMethod ( qcStatement        * aStatement,
                                         UInt                 aJoinMethodCnt,
                                         qmoJoinMethod      * aJoinMethods,
                                         qmoJoinMethodCost ** aSelected )
{
/****************************************************************************************
 *
 * Description : TASK-6744
 *               각 Join Type에서 선택 가능한 join method들 중에서 
 *               random하게 하나 선택 
 *               
 *     << join method의 최대 개수 >>
 *         - Inner Join      : 16
 *         - Semi Join       : 11
 *         - Anti Join       : 10
 *         - Left Outer Join : 8
 *         - Full Outer Join : 12 
 *               
 * Implementation :
 *     1. 모든 join method들 중에서 선택 가능한 join method를 찾아서 개수를 구한다.
 *     2. 모듈러 연산을 통해서 join method를 random하게 선택
 *
 ****************************************************************************************/

    UInt                i = 0;
    UInt                j = 0;
    UInt                sSelectedMethod = 0;
    UInt                sFeasibilityJoinMethodCnt = 0;
    qmoJoinMethodCost * sFeasibilityJoinMethod[16];
    qmoJoinMethodCost * sJoinMethodCost = NULL;
    qmoJoinMethod     * sJoinMethod     = NULL;
    // using well random generator
    ULong sStage1 = 0;
    ULong sStage2 = 0;
    ULong sStage3 = 0;
    ULong sStage4 = 0;
    ULong sStage5 = 0;

    // 배열 초기화
    for ( i = 0 ; i < 16 ; i++ )
    {
        sFeasibilityJoinMethod[i] = NULL;
    }

    // 모든 join method 중에서 선택 가능한 join method 및 join method의 개수를 구한다. 
    for ( i = 0 ; i < aJoinMethodCnt ; i++ )
    {
        sJoinMethod = &aJoinMethods[i];

        for ( j = 0 ; j < sJoinMethod->joinMethodCnt ; j++ )
        {
            sJoinMethodCost = &sJoinMethod->joinMethodCost[j];

            if ( (sJoinMethodCost->flag & QMO_JOIN_METHOD_FEASIBILITY_MASK) == QMO_JOIN_METHOD_FEASIBILITY_TRUE )
            {
                sFeasibilityJoinMethod[sFeasibilityJoinMethodCnt] = sJoinMethodCost;
                sFeasibilityJoinMethodCnt++;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    // join method 선택
    aStatement->mRandomPlanInfo->mTotalNumOfCases = aStatement->mRandomPlanInfo->mTotalNumOfCases + sFeasibilityJoinMethodCnt;

    // using well random number generator
    sStage1 = QCU_PLAN_RANDOM_SEED ^ aStatement->mRandomPlanInfo->mWeightedValue ^ (QCU_PLAN_RANDOM_SEED << 16) ^ (aStatement->mRandomPlanInfo->mWeightedValue << 15);
    sStage2 = aStatement->mRandomPlanInfo->mTotalNumOfCases ^ (aStatement->mRandomPlanInfo->mTotalNumOfCases >> 11);
    sStage3 = sStage1 ^ sStage2;
    sStage4 = sStage3 ^ ( (sStage3 << 5) & 0xDA442D20UL );
    sStage5 = sFeasibilityJoinMethodCnt ^ sStage1 ^ sStage4 ^ (sFeasibilityJoinMethodCnt << 2) ^ (sStage1 >> 18) ^ (sStage2 << 28);

    sSelectedMethod = sStage5 % sFeasibilityJoinMethodCnt; 

    IDE_DASSERT( sSelectedMethod < sFeasibilityJoinMethodCnt );

    aStatement->mRandomPlanInfo->mWeightedValue++;

    *aSelected = sFeasibilityJoinMethod[sSelectedMethod];

    return IDE_SUCCESS;
}

void qmgJoin::moveConstantPred( qmgJOIN * aMyGraph, idBool aIsInverse )
{
    qmoPredicate * sPredicate;
    qmoPredicate * sPrev;

    IDE_TEST_RAISE( ( aMyGraph->graph.left == NULL ) ||
                    ( aMyGraph->graph.right == NULL ) ||
                    ( aIsInverse == ID_FALSE ), normal_exit );

    if ( aMyGraph->graph.left->type == QMG_SELECTION )
    {
        if ( aMyGraph->graph.left->left == NULL )
        {
            if ( aMyGraph->graph.left->constantPredicate != NULL )
            {
                if ( aMyGraph->graph.right->constantPredicate == NULL )
                {
                    aMyGraph->graph.right->constantPredicate = aMyGraph->graph.left->constantPredicate;
                    aMyGraph->graph.left->constantPredicate = NULL;
                }
                else
                {
                    for ( sPredicate = aMyGraph->graph.right->constantPredicate;
                          sPredicate != NULL;
                          sPredicate = sPredicate->next )
                    {
                        sPrev = sPredicate;
                    }
                    sPrev->next = aMyGraph->graph.left->constantPredicate;
                    aMyGraph->graph.left->constantPredicate = NULL;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_CONT( normal_exit );
}

void qmgJoin::checkOrValueIndex( qmgGraph * aGraph )
{
    qmoPredicate  * sTemp = NULL;
    qmgChildren   * sChild = NULL;

    if ( aGraph != NULL )
    {
        if ( aGraph->type == QMG_SELECTION )
        {
            for ( sTemp = aGraph->myPredicate; sTemp != NULL; sTemp = sTemp->next )
            {
                if ( ( sTemp->flag & QMO_PRED_OR_VALUE_INDEX_MASK )
                     == QMO_PRED_OR_VALUE_INDEX_TRUE )
                {
                    sTemp->flag &= ~QMO_PRED_JOIN_OR_VALUE_INDEX_MASK;
                    sTemp->flag |= QMO_PRED_JOIN_OR_VALUE_INDEX_TRUE;
                }
            }
        }
        else if ( aGraph->type == QMG_PARTITION )
        {
            for ( sChild = ((qmgPARTITION *)aGraph)->graph.children;
                  sChild != NULL;
                  sChild = sChild->next )
            {
                for ( sTemp = sChild->childGraph->myPredicate; sTemp != NULL; sTemp = sTemp->next )
                {
                    if ( ( sTemp->flag & QMO_PRED_OR_VALUE_INDEX_MASK )
                         == QMO_PRED_OR_VALUE_INDEX_TRUE )
                    {
                        sTemp->flag &= ~QMO_PRED_JOIN_OR_VALUE_INDEX_MASK;
                        sTemp->flag |= QMO_PRED_JOIN_OR_VALUE_INDEX_TRUE;
                    }
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
}

