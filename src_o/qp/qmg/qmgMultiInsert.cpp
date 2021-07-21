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
 *     Multi-Insert Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgInsert.h>
#include <qmgMultiInsert.h>
#include <qmo.h>
#include <qmoMultiNonPlan.h>
#include <qmoPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE
#include <qcmPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE

IDE_RC
qmgMultiInsert::init( qcStatement * aStatement,
                      qmgGraph    * aChildGraph,
                      qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgInsert Graph의 초기화
 *
 * Implementation :
 *    (1) qmgInsert을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgMTIT         * sMyGraph;
    qmmInsParseTree * sParseTree;
    qmsTableRef     * sInsertTableRef;
    qmgGraph       ** sChildGraph;
    UInt              sInsertCount = 0;
    UInt              i;

    IDU_FIT_POINT_FATAL( "qmgMultiInsert::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Multi-Insert Graph를 위한 기본 초기화
    //---------------------------------------------------
    
    // qmgInsert을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgMTIT ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_MULTI_INSERT;

    // insert는 querySet이 없음
    sMyGraph->graph.myQuerySet = NULL;

    sMyGraph->graph.optimize = qmgMultiInsert::optimize;
    sMyGraph->graph.makePlan = qmgMultiInsert::makePlan;
    sMyGraph->graph.printGraph = qmgMultiInsert::printGraph;

    //-------------------------------------------
    // BUG-36596 multi-table insert
    //-------------------------------------------

    for ( sParseTree = (qmmInsParseTree *)aStatement->myPlan->parseTree;
          sParseTree != NULL;
          sParseTree = sParseTree->next )
    {
        IDE_DASSERT( (sParseTree->flag & QMM_MULTI_INSERT_MASK) == QMM_MULTI_INSERT_TRUE );

        sInsertTableRef = sParseTree->insertTableRef;
    
        // Disk/Memory 정보 설정
        if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sInsertTableRef->table].lflag
               & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_DISK )
        {
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
        }
        else
        {
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
        }
        
        sInsertCount++;
    }
    
    //---------------------------------------------------------------
    // child graph의 생성
    //---------------------------------------------------------------

    // graph->children구조체의 메모리 할당.
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(qmgChildren) * sInsertCount,
                  (void**) &sMyGraph->graph.children )
              != IDE_SUCCESS );

    // child graph pointer의 메모리 할당.
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(qmgGraph *) * sInsertCount,
                  (void**) &sChildGraph )
              != IDE_SUCCESS );

    for ( sParseTree = (qmmInsParseTree *)aStatement->myPlan->parseTree, i = 0;
          sParseTree != NULL;
          sParseTree = sParseTree->next, i++ )
    {
        IDE_DASSERT( (sParseTree->flag & QMM_MULTI_INSERT_MASK) == QMM_MULTI_INSERT_TRUE );
        
        if ( i == 0 )
        {
            // 첫번째 child에 subquery를 전달한다.
            IDE_TEST( qmgInsert::init( aStatement,
                                       sParseTree,
                                       aChildGraph,
                                       &sChildGraph[i] )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmgInsert::init( aStatement,
                                       sParseTree,
                                       NULL,
                                       &sChildGraph[i] )
                      != IDE_SUCCESS );
        }

        sMyGraph->graph.children[i].childGraph = sChildGraph[i];
        
        if( i == sInsertCount - 1 )
        {
            sMyGraph->graph.children[i].next = NULL;
        }
        else
        {
            sMyGraph->graph.children[i].next =
                &sMyGraph->graph.children[i+1];
        }
    }

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMultiInsert::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgInsert의 최적화
 *
 * Implementation :
 *    (1) CASE 1 : INSERT...VALUE(...(subquery)...)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgMTIT         * sMyGraph;
    qmgChildren     * sChildren;

    IDU_FIT_POINT_FATAL( "qmgMultiInsert::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgMTIT*) aGraph;

    //-------------------------------------------
    // BUG-36596 multi-table insert
    //-------------------------------------------

    for( sChildren = sMyGraph->graph.children;
         sChildren != NULL;
         sChildren = sChildren->next )
    {
        IDE_TEST( sChildren->childGraph->optimize( aStatement ,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS );
    }

    //---------------------------------------------------
    // 공통 비용 정보 설정
    //---------------------------------------------------

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt = 0;

    // recordSize
    sMyGraph->graph.costInfo.recordSize = 1;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = 1;

    // outputRecordCnt
    sMyGraph->graph.costInfo.outputRecordCnt = 0;

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = 0;
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = 0;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost = 0;
    sMyGraph->graph.costInfo.totalDiskCost = 0;
    sMyGraph->graph.costInfo.totalAllCost = 0;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMultiInsert::makePlan( qcStatement     * aStatement,
                          const qmgGraph  * aParent,
                          qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgInsert으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgInsert으로 생성가능한 Plan
 *
 *           [INST]
 *
 ***********************************************************************/

    qmgChildren     * sChildren;
    qmgMTIT         * sMyGraph;
    qmnPlan         * sPlan;

    IDU_FIT_POINT_FATAL( "qmgMultiInsert::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgMTIT*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= QMG_PARALLEL_IMPOSSIBLE_FALSE;

    // BUG-38410
    // 최상위 plan 이므로 기본값을 세팅한다.
    aGraph->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aGraph->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    //---------------------------
    // Plan의 생성
    //---------------------------

    // 최상위 plan이다.
    IDE_DASSERT( aParent == NULL );
    
    IDE_TEST( qmoMultiNonPlan::initMTIT( aStatement ,
                                         &sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;
    
    //---------------------------
    // child plan의 생성
    //---------------------------
    
    for( sChildren = sMyGraph->graph.children;
         sChildren != NULL;
         sChildren = sChildren->next )
    {
        // BUG-38410
        // SCAN parallel flag 를 자식 노드로 물려준다.
        sChildren->childGraph->flag |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

        IDE_TEST( sChildren->childGraph->makePlan( aStatement,
                                                   aParent,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS);
    }
    
    //----------------------------
    // MTIT의 생성
    //----------------------------

    IDE_TEST( qmoMultiNonPlan::makeMTIT( aStatement ,
                                         sMyGraph->graph.children ,
                                         sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMultiInsert::printGraph( qcStatement  * aStatement,
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

    qmgChildren  * sChildren;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmgMultiInsert::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    //-----------------------------------
    // Graph의 시작 출력
    //-----------------------------------

    if ( aDepth == 0 )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "----------------------------------------------------------" );
    }
    else
    {
        // Nothing To Do
    }

    //-----------------------------------
    // Graph 공통 정보의 출력
    //-----------------------------------

    IDE_TEST( qmg::printGraph( aStatement,
                               aGraph,
                               aDepth,
                               aString )
              != IDE_SUCCESS );

    //-----------------------------------
    // Child Graph 고유 정보의 출력
    //-----------------------------------

    for( sChildren = aGraph->children;
         sChildren != NULL;
         sChildren = sChildren->next )
    {
        IDE_TEST( sChildren->childGraph->printGraph( aStatement,
                                                     sChildren->childGraph,
                                                     aDepth + 1,
                                                     aString )
                  != IDE_SUCCESS );
    }

    //-----------------------------------
    // Graph의 마지막 출력
    //-----------------------------------

    if ( aDepth == 0 )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "----------------------------------------------------------\n\n" );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
