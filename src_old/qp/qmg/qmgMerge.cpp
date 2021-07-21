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
 * $Id: qmgMerge.cpp 53774 2012-06-15 04:53:31Z eerien $
 *
 * Description :
 *     Merge Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgMerge.h>
#include <qmo.h>
#include <qmoMultiNonPlan.h>

IDE_RC
qmgMerge::init( qcStatement  * aStatement,
                qmgGraph    ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgMerge Graph의 초기화
 *
 * Implementation :
 *    (1) qmgMerge을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgMRGE           * sMyGraph;
    qmmMergeParseTree * sParseTree;

    IDU_FIT_POINT_FATAL( "qmgMerge::init::__FT__" );

    sParseTree = (qmmMergeParseTree *)aStatement->myPlan->parseTree;

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //---------------------------------------------------
    // Merge Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgMerge을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgMRGE ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_MERGE;

    sMyGraph->graph.optimize = qmgMerge::optimize;
    sMyGraph->graph.makePlan = qmgMerge::makePlan;
    sMyGraph->graph.printGraph = qmgMerge::printGraph;

    //---------------------------------------------------
    // Merge Graph 만을 위한 초기화
    //---------------------------------------------------

    // 최상위 graph인 merge graph에 parse tree 정보를 설정
    sMyGraph->selectSourceStatement = sParseTree->selectSourceStatement;
    sMyGraph->selectTargetStatement = sParseTree->selectTargetStatement;

    // 최상위 graph인 merge graph에 parse tree 정보를 설정
    sMyGraph->updateStatement = sParseTree->updateStatement;
    sMyGraph->deleteStatement = sParseTree->deleteStatement;
    sMyGraph->insertStatement = sParseTree->insertStatement;
    sMyGraph->insertNoRowsStatement = sParseTree->insertNoRowsStatement;

    // 최상위 graph인 merge graph에 insert where 정보를 설정
    sMyGraph->whereForInsert = sParseTree->whereForInsert;
    
    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMerge::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgMerge의 최적화
 *
 * Implementation :
 *    (1) CASE 1 : MERGE...VALUE(...(subquery)...)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgMRGE             * sMyGraph;
    qcStatement         * sStatement;
    qmgChildren         * sPrevChildren = NULL;
    qmgChildren         * sCurrChildren;

    IDU_FIT_POINT_FATAL( "qmgMerge::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgMRGE*) aGraph;

    //---------------------------------------------------
    // child graph 생성
    //---------------------------------------------------

    // MERGE graph는 여러개의 child를 가지며
    // 각 child의 위치는 정해져있다.
    
    // graph->children구조체의 메모리 할당.
    IDE_TEST( QC_QMP_MEM(aStatement)->cralloc(
                  ID_SIZEOF(qmgChildren) * QMO_MERGE_IDX_MAX,
                  (void**) &sMyGraph->graph.children )
              != IDE_SUCCESS );
    
    //---------------------------
    // select source graph
    //---------------------------
    
    sStatement = sMyGraph->selectSourceStatement;

    IDE_TEST( qmoSubquery::makeGraph( sStatement ) != IDE_SUCCESS );

    sCurrChildren = & (sMyGraph->graph.children[QMO_MERGE_SELECT_SOURCE_IDX]);

    sCurrChildren->childGraph = sStatement->myPlan->graph;
    
    sPrevChildren = sCurrChildren;
    
    //---------------------------
    // select target graph
    //---------------------------
    
    sStatement = sMyGraph->selectTargetStatement;
    
    IDE_TEST( qmoSubquery::makeGraph( sStatement ) != IDE_SUCCESS );

    sCurrChildren = & (sMyGraph->graph.children[QMO_MERGE_SELECT_TARGET_IDX]);
    
    sCurrChildren->childGraph = sStatement->myPlan->graph;

    // next 연결
    sPrevChildren->next = sCurrChildren;
    sPrevChildren = sCurrChildren;
    
    //---------------------------
    // update graph
    //---------------------------

    if ( sMyGraph->updateStatement != NULL )
    {
        sStatement = sMyGraph->updateStatement;
    
        IDE_TEST( qmo::makeUpdateGraph( sStatement ) != IDE_SUCCESS );

        sCurrChildren = & (sMyGraph->graph.children[QMO_MERGE_UPDATE_IDX]);
        
        sCurrChildren->childGraph = sStatement->myPlan->graph;

        // next 연결
        sPrevChildren->next = sCurrChildren;
        sPrevChildren = sCurrChildren;
    }
    else
    {
        // Nothing to do.
    }
    //---------------------------
    // delete graph
    //---------------------------

    if ( sMyGraph->deleteStatement != NULL )
    {
        sStatement = sMyGraph->deleteStatement;
    
        IDE_TEST( qmo::makeDeleteGraph( sStatement ) != IDE_SUCCESS );

        sCurrChildren = & (sMyGraph->graph.children[QMO_MERGE_DELETE_IDX]);
        
        sCurrChildren->childGraph = sStatement->myPlan->graph;

        // next 연결
        sPrevChildren->next = sCurrChildren;
        sPrevChildren = sCurrChildren;
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------
    // insert graph
    //---------------------------

    if ( sMyGraph->insertStatement != NULL )
    {
        sStatement = sMyGraph->insertStatement;
    
        IDE_TEST( qmo::makeInsertGraph( sStatement ) != IDE_SUCCESS );

        sCurrChildren = & (sMyGraph->graph.children[QMO_MERGE_INSERT_IDX]);
        
        sCurrChildren->childGraph = sStatement->myPlan->graph;

        // next 연결
        sPrevChildren->next = sCurrChildren;
        sPrevChildren = sCurrChildren;
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------
    // insert empty graph
    //---------------------------

    if ( sMyGraph->insertNoRowsStatement != NULL )
    {
        sStatement = sMyGraph->insertNoRowsStatement;
    
        IDE_TEST( qmo::makeInsertGraph( sStatement ) != IDE_SUCCESS );

        sCurrChildren = & (sMyGraph->graph.children[QMO_MERGE_INSERT_NOROWS_IDX]);
        
        sCurrChildren->childGraph = sStatement->myPlan->graph;

        // next 연결
        sPrevChildren->next = sCurrChildren;
        sPrevChildren = sCurrChildren;
    }
    else
    {
        // Nothing to do.
    }

    // next 연결
    sPrevChildren->next = NULL;
    
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
qmgMerge::makePlan( qcStatement     * aStatement,
                    const qmgGraph  * aParent,
                    qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgMerge으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgMerge으로 생성가능한 Plan
 *
 *           [MRGE]
 *
 ***********************************************************************/

    qmgChildren       * sChildren;
    qcStatement       * sStatement;
    qmgMRGE           * sMyGraph;
    qmnPlan           * sPlan;
    UInt                sResetPlanFlagStartIndex;
    UInt                sResetPlanFlagEndIndex;
    UInt                sResetExecInfoStartIndex;
    UInt                sResetExecInfoEndIndex;
    qmoMRGEInfo         sMRGEInfo;

    IDU_FIT_POINT_FATAL( "qmgMerge::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgMRGE*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;

    //---------------------------
    // Plan의 생성
    //---------------------------

    // 최상위 plan이다.
    IDE_DASSERT( aParent == NULL );
    
    IDE_TEST( qmoMultiNonPlan::initMRGE( aStatement ,
                                         &sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;

    //---------------------------
    // select source plan의 생성
    //---------------------------

    sStatement = sMyGraph->selectSourceStatement;

    sChildren = & (sMyGraph->graph.children[QMO_MERGE_SELECT_SOURCE_IDX]);

    IDE_TEST( sChildren->childGraph->makePlan( sStatement,
                                               &sMyGraph->graph,
                                               sChildren->childGraph )
              != IDE_SUCCESS );
    
    //---------------------------
    // select target plan의 생성
    //---------------------------
    
    // select target plan 부터 insert plan까지 merge시마다 plan을 reset하기 위해 기록한다.
    sResetPlanFlagStartIndex = QC_SHARED_TMPLATE(aStatement)->planCount;
    sResetExecInfoStartIndex = QC_SHARED_TMPLATE(aStatement)->tmplate.execInfoCnt;
    
    sStatement = sMyGraph->selectTargetStatement;
    
    sChildren = & (sMyGraph->graph.children[QMO_MERGE_SELECT_TARGET_IDX]);
    
    IDE_TEST( sChildren->childGraph->makePlan( sStatement,
                                               &sMyGraph->graph,
                                               sChildren->childGraph )
              != IDE_SUCCESS );
    
    //---------------------------
    // update plan의 생성
    //---------------------------

    if ( sMyGraph->updateStatement != NULL )
    {
        sStatement = sMyGraph->updateStatement;
    
        sChildren = & (sMyGraph->graph.children[QMO_MERGE_UPDATE_IDX]);
        
        IDE_TEST( sChildren->childGraph->makePlan( sStatement,
                                                   NULL,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------
    // delete plan의 생성
    //---------------------------

    if ( sMyGraph->deleteStatement != NULL )
    {
        sStatement = sMyGraph->deleteStatement;
    
        sChildren = & (sMyGraph->graph.children[QMO_MERGE_DELETE_IDX]);
        
        IDE_TEST( sChildren->childGraph->makePlan( sStatement,
                                                   NULL,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
        
    //---------------------------
    // insert plan의 생성
    //---------------------------

    if ( sMyGraph->insertStatement != NULL )
    {
        sStatement = sMyGraph->insertStatement;
    
        sChildren = & (sMyGraph->graph.children[QMO_MERGE_INSERT_IDX]);
        
        IDE_TEST( sChildren->childGraph->makePlan( sStatement,
                                                   NULL,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS );

        //------------------------------------------
        // insert where 구문 내의 Subquery 최적화
        //------------------------------------------
    
        if ( sMyGraph->whereForInsert != NULL )
        {
            // Subquery 존재할 경우 Subquery 최적화
            if ( (sMyGraph->whereForInsert->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                // BUG-32584
                // 모든 서브쿼리에 대해서 MakeGraph 한후에 MakePlan을 해야 한다.
                IDE_TEST( qmoSubquery::optimizeExprMakeGraph(
                              aStatement,
                              ID_UINT_MAX,
                              sMyGraph->whereForInsert )
                          != IDE_SUCCESS );

                IDE_TEST( qmoSubquery::optimizeExprMakePlan(
                              aStatement,
                              ID_UINT_MAX,
                              sMyGraph->whereForInsert )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------
    // insert empty plan의 생성
    //---------------------------

    if ( sMyGraph->insertNoRowsStatement != NULL )
    {
        sStatement = sMyGraph->insertNoRowsStatement;
    
        sChildren = & (sMyGraph->graph.children[QMO_MERGE_INSERT_NOROWS_IDX]);
        
        IDE_TEST( sChildren->childGraph->makePlan( sStatement,
                                                   NULL,
                                                   sChildren->childGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    sResetPlanFlagEndIndex = QC_SHARED_TMPLATE(aStatement)->planCount;
    sResetExecInfoEndIndex = QC_SHARED_TMPLATE(aStatement)->tmplate.execInfoCnt;
    
    //----------------------------
    // MRGE의 생성
    //----------------------------

    sMRGEInfo.tableRef                =
        ((qmsParseTree*)sMyGraph->selectTargetStatement->myPlan->parseTree)
        ->querySet->SFWGH->from->tableRef;

    sMRGEInfo.selectSourceStatement   = sMyGraph->selectSourceStatement;
    sMRGEInfo.selectTargetStatement   = sMyGraph->selectTargetStatement;

    sMRGEInfo.updateStatement         = sMyGraph->updateStatement;
    sMRGEInfo.deleteStatement         = sMyGraph->deleteStatement;
    sMRGEInfo.insertStatement         = sMyGraph->insertStatement;
    sMRGEInfo.insertNoRowsStatement   = sMyGraph->insertNoRowsStatement;

    sMRGEInfo.whereForInsert          = sMyGraph->whereForInsert;
    
    sMRGEInfo.resetPlanFlagStartIndex = sResetPlanFlagStartIndex;
    sMRGEInfo.resetPlanFlagEndIndex   = sResetPlanFlagEndIndex; 
    sMRGEInfo.resetExecInfoStartIndex = sResetExecInfoStartIndex;
    sMRGEInfo.resetExecInfoEndIndex   = sResetExecInfoEndIndex;
    
    IDE_TEST( qmoMultiNonPlan::makeMRGE( aStatement ,
                                         & sMRGEInfo ,
                                         sMyGraph->graph.children ,
                                         sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMerge::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgMerge::printGraph::__FT__" );

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
    // Graph 고유 정보의 출력
    //-----------------------------------


    //-----------------------------------
    // Child Graph 고유 정보의 출력
    //-----------------------------------

    if ( aGraph->children != NULL )
    {
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
    }
    else
    {
        // Nothing to do.
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
