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
 * $Id: qmgDnf.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     DNF Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgDnf.h>
#include <qmoOneNonPlan.h>
#include <qmoTwoNonPlan.h>

IDE_RC
qmgDnf::init( qcStatement * aStatement,
              qtcNode     * aDnfNotFilter, 
              qmgGraph    * aLeftGraph,
              qmgGraph    * aRightGraph,
              qmgGraph    * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDnf Graph의 초기화
 *
 * Implementation :
 *    (1) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (2) graph.type 설정
 *    (3) graph.left을 aLeftGraph로, graph.right를 aRightGraph로 설정
 *    (4) graph.dependencies 설정
 *    (5) aDnfNotFilter를 graph.myPredicate에 연결
 *    (6) graph.optimize와 graph.makePlan 설정
 *
 ***********************************************************************/
    
    qmgDNF       * sMyGraph;
    qmoPredicate * sPredicate;
    qcDepInfo      sOrDependencies;

    IDU_FIT_POINT_FATAL( "qmgDnf::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftGraph != NULL );
    IDE_DASSERT( aRightGraph != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgDNF*) aGraph; 

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_DNF;
    
    sMyGraph->graph.myFrom = NULL;
    sMyGraph->graph.myQuerySet = aLeftGraph->myQuerySet;
    
    sMyGraph->graph.left = aLeftGraph;
    sMyGraph->graph.right = aRightGraph;

    IDE_TEST( qtc::dependencyOr( & aLeftGraph->depInfo,
                                 & aRightGraph->depInfo,
                                 & sOrDependencies )
              != IDE_SUCCESS );
    
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & sOrDependencies );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoPredicate ),
                                             (void**)&sPredicate )
              != IDE_SUCCESS );
    
    sPredicate->node = aDnfNotFilter;
    sPredicate->flag = QMO_PRED_CLEAR;
    sPredicate->mySelectivity    = 1;
    sPredicate->totalSelectivity = 1;    
    sPredicate->next = NULL;
    sPredicate->more = NULL;
        
    sMyGraph->graph.myPredicate = sPredicate;
        
    sMyGraph->graph.optimize = qmgDnf::optimize;
    sMyGraph->graph.makePlan = qmgDnf::makePlan;
    sMyGraph->graph.printGraph = qmgDnf::printGraph;

    // DISK/MEMORY 정보 설정
    // left 또는 right가 disk이면 disk,
    // 그렇지 않을 경우 memory
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;   

}

IDE_RC
qmgDnf::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDNF Graph의 최적화
 *
 * Implementation : 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgDNF * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgDnf::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgDNF*) aGraph;

    //------------------------------------------
    // Preserved Order 설정
    //    - DNF가 존재할 경우, preserved order 사용할 수 없음
    //------------------------------------------
    
    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
    
    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // recordSize
    sMyGraph->graph.costInfo.recordSize =
        sMyGraph->graph.left->costInfo.recordSize;

    // selectivity 
    sMyGraph->graph.costInfo.selectivity = 1;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt +
        sMyGraph->graph.right->costInfo.outputRecordCnt;

    // outputRecordCnt
    sMyGraph->graph.costInfo.outputRecordCnt = 
        sMyGraph->graph.left->costInfo.outputRecordCnt +
        sMyGraph->graph.right->costInfo.outputRecordCnt ;

    sMyGraph->graph.costInfo.myAccessCost =
        sMyGraph->graph.left->costInfo.totalAccessCost +
        sMyGraph->graph.right->costInfo.totalAccessCost;
    sMyGraph->graph.costInfo.myDiskCost =
        sMyGraph->graph.left->costInfo.totalDiskCost +
        sMyGraph->graph.right->costInfo.totalDiskCost;
    sMyGraph->graph.costInfo.myAllCost =
        sMyGraph->graph.left->costInfo.totalAllCost +
        sMyGraph->graph.right->costInfo.totalAllCost;

    // total cost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->graph.costInfo.myAccessCost;
    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->graph.costInfo.myDiskCost;
    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->graph.costInfo.myAllCost;
    
    return IDE_SUCCESS;

}
    

IDE_RC
qmgDnf::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDnf로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgDnf로 부터 생성 가능한 Plan
 *
 *         [CONC]
 *              |
 *              [FILT]   : qmgDNF.graph.myPredicate
 *
 ***********************************************************************/

    qmgDNF          * sMyGraph;
    qmnPlan         * sCONC;
    qmnPlan         * sFILT;
    qmnPlan         * sLeftChildPlan;
    qmnPlan         * sRightChildPlan;
    qtcNode         * sFilter;
    UInt              i;

    IDU_FIT_POINT_FATAL( "qmgDnf::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgDNF*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // SCAN parallel flag 를 자식 노드로 물려준다.
    aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);
    aGraph->right->flag |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

    //----------------------
    // FILT의 생성
    // - qmgDNF.graph.myPredicate로 부터 생성
    //----------------------
    if( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                sMyGraph->graph.myPredicate ,
                                                &sFilter ) != IDE_SUCCESS );
                  
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           sFilter ) != IDE_SUCCESS );
    }
    else
    {
        sFilter = NULL;
    }

    sMyGraph->graph.myPlan = aParent->myPlan;

    IDE_TEST( qmoTwoNonPlan::initCONC( aStatement,
                                       sMyGraph->graph.myQuerySet,
                                       sMyGraph->graph.myPlan,
                                       &sMyGraph->graph.depInfo,
                                       &sCONC )
              != IDE_SUCCESS );
    sMyGraph->graph.myPlan = sCONC;

    //----------------------
    // 하위 Plan의 생성
    //----------------------

    IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                              &sMyGraph->graph ,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS );

    //left graph의 plan 가져 오기
    sLeftChildPlan = sMyGraph->graph.left->myPlan;

    // bug-37324 외부 참조 컬럼에 대해서는 디펜던시를 reset 하면 안됨
    for( i = 0; i < sMyGraph->graph.depInfo.depCount; i++ )
    {
        IDE_TEST( qmg::resetColumnLocate(
                      aStatement,
                      sMyGraph->graph.depInfo.depend[i] )
                  != IDE_SUCCESS );
    }

    sMyGraph->graph.myPlan = sCONC;

    IDE_TEST( qmoOneNonPlan::initFILT( aStatement,
                                       sMyGraph->graph.myQuerySet,
                                       sFilter ,
                                       sMyGraph->graph.myPlan,
                                       &sFILT )
              != IDE_SUCCESS );
    sMyGraph->graph.myPlan = sFILT;

    IDE_TEST( sMyGraph->graph.right->makePlan( aStatement ,
                                               &sMyGraph->graph ,
                                               sMyGraph->graph.right )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sMyGraph->graph.right->myPlan;

    IDE_TEST( qmoOneNonPlan::makeFILT( aStatement ,
                                       sMyGraph->graph.myQuerySet ,
                                       sFilter ,
                                       sMyGraph->graph.constantPredicate ,
                                       sMyGraph->graph.myPlan ,
                                       sFILT ) != IDE_SUCCESS );
    sMyGraph->graph.myPlan = sFILT;

    qmg::setPlanInfo( aStatement, sFILT, &(sMyGraph->graph) );

    //right graph의 plan 가져 오기
    sRightChildPlan = sMyGraph->graph.myPlan;

    //----------------------
    // CONC의 생성
    //----------------------
    IDE_TEST( qmoTwoNonPlan::makeCONC( aStatement ,
                                          sMyGraph->graph.myQuerySet ,
                                          sLeftChildPlan ,
                                          sRightChildPlan ,
                                          sCONC ) != IDE_SUCCESS );
    sMyGraph->graph.myPlan = sCONC;

    qmg::setPlanInfo( aStatement, sCONC, &(sMyGraph->graph) );
    
    return IDE_SUCCESS;


    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}


IDE_RC
qmgDnf::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgDnf::makePlan::__FT__" );

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

    IDE_TEST( aGraph->right->printGraph( aStatement,
                                         aGraph->right,
                                         aDepth + 1,
                                         aString )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;       

}
