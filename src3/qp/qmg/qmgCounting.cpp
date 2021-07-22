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
 * $Id: qmgCounting.cpp 20233 2007-08-06 01:58:21Z sungminee $
 *
 * Description :
 *     Counting Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmoCost.h>
#include <qmgCounting.h>
#include <qmoOneNonPlan.h>
#include <qmoSelectivity.h>
#include <qmo.h>

IDE_RC
qmgCounting::init( qcStatement  * aStatement,
                   qmsQuerySet  * aQuerySet,
                   qmoPredicate * aRownumPredicate,
                   qmgGraph     * aChildGraph,
                   qmgGraph    ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgCounting의 초기화
 *
 * Implementation :
 *    (1) qmgCounting을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgCNTG      * sMyGraph;
    qmsQuerySet  * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgCounting::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Counting Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgCounting을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgCNTG ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );
    
    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );
    
    sMyGraph->graph.type = QMG_COUNTING;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );
    
    sMyGraph->graph.myFrom = NULL;
    sMyGraph->graph.myQuerySet = aQuerySet;
    sMyGraph->graph.myPredicate = aRownumPredicate;
    
    sMyGraph->graph.optimize = qmgCounting::optimize;
    sMyGraph->graph.makePlan = qmgCounting::makePlan;
    sMyGraph->graph.printGraph = qmgCounting::printGraph;

    // Disk/Memory 정보 설정
    for ( sQuerySet = aQuerySet;
          sQuerySet->left != NULL;
          sQuerySet = sQuerySet->left ) ;
    
    switch(  sQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            // 중간 결과 Type Hint가 없는 경우, 하위의 Type을 따른다.
            if ( ( aChildGraph->flag & QMG_GRAPH_TYPE_MASK )
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
    // Counting Graph 고유 정보를 위한 기본 초기화
    //---------------------------------------------------

    sMyGraph->stopkeyPredicate = NULL;
    sMyGraph->filterPredicate = NULL;
    
    sMyGraph->stopRecordCount = 0;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC
qmgCounting::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgCounting의 최적화
 *
 * Implementation :
 *    (1) rownum predicate의 subquery 처리
 *    (2) stopkey 추출
 *    (3) 공통 비용 정보
 *    (4) Preserved Order
 *
 ***********************************************************************/

    qmgCNTG           * sMyGraph;
    qmsQuerySet       * sQuerySet;
    SDouble             sFilterCost;

    IDU_FIT_POINT_FATAL( "qmgCounting::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgCNTG*) aGraph;
    sQuerySet = sMyGraph->graph.myQuerySet;
    
    //------------------------------------------
    // Predicate 최적화
    //------------------------------------------

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

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        // rownum predicate의 subquery 처리
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sMyGraph->graph.myPredicate,
                                               ID_FALSE )
                  != IDE_SUCCESS );

        // stopkey predicate 추출
        IDE_TEST( qmoPred::separateRownumPred( aStatement,
                                               sQuerySet,
                                               sMyGraph->graph.myPredicate,
                                               & sMyGraph->stopkeyPredicate,
                                               & sMyGraph->filterPredicate,
                                               & sMyGraph->stopRecordCount )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Preserved Order
    //------------------------------------------

    IDE_TEST( makeOrderFromChild( aStatement, aGraph )
              != IDE_SUCCESS );

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // selectivity
    IDE_TEST( qmoSelectivity::setCountingSelectivity(
                  sMyGraph->stopkeyPredicate,
                  sMyGraph->stopRecordCount,
                  sMyGraph->graph.left->costInfo.outputRecordCnt,
                  & sMyGraph->graph.costInfo.selectivity )
              != IDE_SUCCESS );

    // outputRecordCnt
    IDE_TEST( qmoSelectivity::setCountingOutputCnt(
                  sMyGraph->stopkeyPredicate,
                  sMyGraph->stopRecordCount,
                  sMyGraph->graph.left->costInfo.outputRecordCnt,
                  & sMyGraph->graph.costInfo.outputRecordCnt )
              != IDE_SUCCESS );

    if( sMyGraph->filterPredicate != NULL )
    {
        sFilterCost = qmoCost::getFilterCost(
            aStatement->mSysStat,
            sMyGraph->filterPredicate,
            sMyGraph->graph.costInfo.outputRecordCnt );
    }
    else
    {
        sFilterCost = 0;
    }

    // recordSize
    sMyGraph->graph.costInfo.recordSize =
        sMyGraph->graph.left->costInfo.recordSize;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    sMyGraph->graph.costInfo.myAccessCost = sFilterCost;
    sMyGraph->graph.costInfo.myDiskCost   = 0;
    sMyGraph->graph.costInfo.myAllCost    = sFilterCost;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->graph.left->costInfo.totalAccessCost;
    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->graph.left->costInfo.totalDiskCost;

    // PROJ-2242
    // ROWNUM 이 있을때는 특별이 cost 값을 줄일 필요가 있다.
    sMyGraph->graph.costInfo.totalAllCost =
        ( sMyGraph->graph.left->costInfo.totalAllCost * 
          sMyGraph->graph.costInfo.selectivity ) +
        sMyGraph->graph.costInfo.myAllCost;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgCounting::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgCounting으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgCounting으로 부터 생성가능한 Plan
 *
 *         1.  stopkey가 없는 경우
 *
 *                 [COUNTER]
 *
 *         2.  stopkey가 있는 경우
 *
 *              [COUNTER STOPKEY]
 *
 *         3.  filter가 있는 경우
 *
 *                   [FILT]
 *
 ***********************************************************************/

    qmgCNTG     * sMyGraph;
    qmnPlan     * sCNTR;
    qmnPlan     * sFILT;
    qtcNode     * sFilter;
    qmcAttrDesc * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmgCounting::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgCNTG*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // SCAN parallel flag 를 자식 노드로 물려준다.
    aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

    sMyGraph->graph.myPlan = aParent->myPlan;

    IDE_TEST( qmoOneNonPlan::initCNTR( aStatement,
                                       sMyGraph->graph.myQuerySet,
                                       sMyGraph->graph.myPlan,
                                       &sCNTR )
              != IDE_SUCCESS );
    sMyGraph->graph.myPlan = sCNTR;

    if ( sMyGraph->filterPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkPredicate( aStatement,
                                          sMyGraph->filterPredicate,
                                          & sFilter )
                  != IDE_SUCCESS );

        IDE_TEST( qmoPred::setPriorNodeID( aStatement,
                                           sMyGraph->graph.myQuerySet,
                                           sFilter )
                  != IDE_SUCCESS );

        IDE_TEST( qmoOneNonPlan::initFILT( aStatement,
                                           sMyGraph->graph.myQuerySet,
                                           sFilter,
                                           sMyGraph->graph.myPlan,
                                           &sFILT ) != IDE_SUCCESS);

        // PROJ-2179
        // FILTER가 COUNTER보다 하위에 위치하므로, 하위 operator가 상위 operator의
        // 결과를 참조하게 된다. 이 때 FILTER의 하위에서 ROWNUM을 상속받지 않도록
        // TERMINAL flag를 설정해준다.
        for( sItrAttr = sFILT->resultDesc;
             sItrAttr != NULL;
             sItrAttr = sItrAttr->next )
        {
            if( ( sItrAttr->expr->lflag & QTC_NODE_ROWNUM_MASK ) == QTC_NODE_ROWNUM_EXIST )
            {
                sItrAttr->flag &= ~QMC_ATTR_TERMINAL_MASK;
                sItrAttr->flag |= QMC_ATTR_TERMINAL_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }

        sMyGraph->graph.myPlan = sFILT;
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // 하위 Plan의 생성
    //---------------------------------------------------
    
    IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                              &sMyGraph->graph,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS);
    
    sMyGraph->graph.myPlan = sMyGraph->graph.left->myPlan;

    //-----------------------------------------------------
    // Filter 노드 생성
    //-----------------------------------------------------
    
    if ( sMyGraph->filterPredicate != NULL )
    {
        IDE_TEST( qmoOneNonPlan::makeFILT( aStatement,
                                           sMyGraph->graph.myQuerySet,
                                           sFilter,
                                           NULL,
                                           sMyGraph->graph.myPlan,
                                           sFILT ) != IDE_SUCCESS);

        sMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(sMyGraph->graph) );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------------------------
    // Counter 노드 생성
    //-----------------------------------------------------
    
    IDE_TEST( qmoOneNonPlan::makeCNTR( aStatement,
                                       sMyGraph->graph.myQuerySet,
                                       sMyGraph->stopkeyPredicate,
                                       sMyGraph->graph.myPlan,
                                       sCNTR )
              != IDE_SUCCESS);

    sMyGraph->graph.myPlan = sCNTR;

    qmg::setPlanInfo( aStatement, sCNTR, &(sMyGraph->graph) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    

}


IDE_RC
qmgCounting::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgCounting::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

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
    // Child Graph 고유 정보의 출력
    //-----------------------------------

    IDE_TEST( aGraph->left->printGraph( aStatement,
                                        aGraph->left,
                                        aDepth + 1,
                                        aString )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}


IDE_RC
qmgCounting::makeOrderFromChild( qcStatement * aStatement,
                                 qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description :
 *    Left Child Graph의 Preserved Order를 복사한다.
 *
 * Implementation :
 * 
 ***********************************************************************/  

    qmgPreservedOrder * sGraphOrder;
    qmgPreservedOrder * sCurrOrder;
    qmgPreservedOrder * sNewOrder;
    qmgPreservedOrder * sChildOrder;

    IDU_FIT_POINT_FATAL( "qmgCounting::makeOrderFromChild::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sGraphOrder = NULL;
    
    //------------------------------------------
    // Left의 Preserved Order를 복사한다.
    //------------------------------------------
    
    for ( sChildOrder = aGraph->left->preservedOrder;
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
    aGraph->flag |= ( aGraph->left->flag & QMG_PRESERVED_ORDER_MASK );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC
qmgCounting::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Preserved Order의 direction을 결정한다.
 *                direction이 NOT_DEFINED 일 경우에만 호출하여야 한다.
 *
 *  Implementation :
 *     Child Graph의 order로 direction을 결정한다.
 *
 ***********************************************************************/

    idBool sIsSamePresOrderWithChild;

    IDU_FIT_POINT_FATAL( "qmgCounting::finalizePreservedOrder::__FT__" );

    // BUG-36803 Counting Graph must have a left child graph
    IDE_DASSERT( aGraph->left != NULL );

    sIsSamePresOrderWithChild =
        qmg::isSamePreservedOrder( aGraph->preservedOrder,
                                   aGraph->left->preservedOrder );

    if ( sIsSamePresOrderWithChild == ID_TRUE )
    {
        // Child graph의 Preserved order direction을 복사한다.
        IDE_TEST( qmg::copyPreservedOrderDirection(
                      aGraph->preservedOrder,
                      aGraph->left->preservedOrder )
                  != IDE_SUCCESS );    
    }
    else
    {
        // Projection은 preserved order를 생성할 수 없는 graph이므로
        // 하위와 preserved order가 다를수 없다.
        IDE_DASSERT(0);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
