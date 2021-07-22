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
 * $Id: qmgDelete.cpp 53774 2012-06-15 04:53:31Z eerien $
 *
 * Description :
 *     Delete Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgDelete.h>
#include <qmoOneNonPlan.h>
#include <qmgSelection.h>
#include <qmgPartition.h>

IDE_RC
qmgDelete::init( qcStatement * aStatement,
                 qmsQuerySet * aQuerySet,
                 qmgGraph    * aChildGraph,
                 qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgDelete Graph의 초기화
 *
 * Implementation :
 *    (1) qmgDelete을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgDETE         * sMyGraph;
    qmmDelParseTree * sParseTree;
    qmsQuerySet     * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgDelete::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Delete Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgDelete을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgDETE ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_DELETE;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgDelete::optimize;
    sMyGraph->graph.makePlan = qmgDelete::makePlan;
    sMyGraph->graph.printGraph = qmgDelete::printGraph;

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
    // Delete Graph 만을 위한 초기화
    //---------------------------------------------------

    sParseTree = (qmmDelParseTree *)aStatement->myPlan->parseTree;

    /* PROJ-2204 Join Update, Delete */
    sMyGraph->deleteTableRef = sParseTree->deleteTableRef;

    // 최상위 graph인 delete graph에 instead of trigger정보를 설정
    sMyGraph->insteadOfTrigger = sParseTree->insteadOfTrigger;

    // 최상위 graph인 delete graph에 limit을 연결
    sMyGraph->limit = sParseTree->limit;

    // 최상위 graph인 delete graph에 child constraint를 연결
    sMyGraph->childConstraints = sParseTree->childConstraints;

    // 최상위 graph인 delete graph에 return into를 연결
    sMyGraph->returnInto = sParseTree->returnInto;

    /* PROJ-2714 Multiple update, delete support */
    sMyGraph->mTableList = sParseTree->mTableList;
    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgDelete::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDelete의 최적화
 *
 * Implementation :
 *    (1) SCAN Limit 최적화
 *    (2) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgDETE           * sMyGraph;
    qmgGraph          * sChildGraph;
    qtcNode           * sNode;
    qmsLimit          * sLimit;
    idBool              sIsScanLimit;

    SDouble             sOutputRecordCnt;
    SDouble             sDeleteSelectivity = 1;    // BUG-17166

    IDU_FIT_POINT_FATAL( "qmgDelete::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgDETE*) aGraph;
    sChildGraph = aGraph->left;

    //---------------------------------------------------
    // SCAN Limit 최적화
    //---------------------------------------------------

    sIsScanLimit = ID_FALSE;
    if ( sMyGraph->limit != NULL )
    {
        if ( sChildGraph->type == QMG_SELECTION )
        {
            //---------------------------------------------------
            // 하위가 일반 qmgSelection인 경우
            // 즉, Set, Order By, Group By, Aggregation, Distinct, Join이
            //  없는 경우
            //---------------------------------------------------
            if ( sChildGraph->myFrom->tableRef->view == NULL )
            {
                // View 가 아닌 경우, SCAN Limit 적용
                sNode = (qtcNode *)sChildGraph->myQuerySet->SFWGH->where;

                if ( sNode != NULL )
                {
                    // where가 존재하는 경우, subquery 존재 유무 검사
                    if ( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK )
                         != QTC_NODE_SUBQUERY_EXIST )
                    {
                        sIsScanLimit = ID_TRUE;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    // where가 존재하지 않는 경우,SCAN Limit 적용
                    sIsScanLimit = ID_TRUE;
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // Set, Order By, Group By, Distinct, Aggregation, Join, View가
            // 있는 경우 :
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    //---------------------------------------------------
    // SCAN Limit Tip이 적용된 경우
    //---------------------------------------------------

    if ( sIsScanLimit == ID_TRUE )
    {
        ((qmgSELT *)sChildGraph)->limit = sMyGraph->limit;

        // To Fix BUG-9560
        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmsLimit ),
                                           ( void**) & sLimit )
            != IDE_SUCCESS );

        // fix BUG-13482
        // parse tree의 limit정보를 현재 가지며,
        // DETE 노드 생성시,
        // 하위 SCAN 노드에서 SCAN Limit 적용이 확정되면,
        // DETE 노드의 limit start를 1로 변경한다.

        qmsLimitI::setStart( sLimit,
                             qmsLimitI::getStart( sMyGraph->limit ) );

        qmsLimitI::setCount( sLimit,
                             qmsLimitI::getCount( sMyGraph->limit ) );

        SET_EMPTY_POSITION( sLimit->limitPos );

        sMyGraph->limit = sLimit;
    }
    else
    {
        // nothing to do
    }

    //---------------------------------------------------
    // 공통 비용 정보 설정
    //---------------------------------------------------

    // recordSize
    sMyGraph->graph.costInfo.recordSize =
        sChildGraph->costInfo.recordSize;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = sDeleteSelectivity;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sChildGraph->costInfo.outputRecordCnt;

    // outputRecordCnt
    sOutputRecordCnt = sChildGraph->costInfo.outputRecordCnt * sDeleteSelectivity;
    sMyGraph->graph.costInfo.outputRecordCnt = sOutputRecordCnt;

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = 0;
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = 0;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost =
        sChildGraph->costInfo.totalAccessCost +
        sMyGraph->graph.costInfo.myAccessCost;

    sMyGraph->graph.costInfo.totalDiskCost =
        sChildGraph->costInfo.totalDiskCost +
        sMyGraph->graph.costInfo.myDiskCost;

    sMyGraph->graph.costInfo.totalAllCost =
        sChildGraph->costInfo.totalAllCost +
        sMyGraph->graph.costInfo.myAllCost;

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |=
        ( sChildGraph->flag & QMG_PRESERVED_ORDER_MASK );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgDelete::makePlan( qcStatement     * aStatement,
                     const qmgGraph  * aParent,
                     qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDelete으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgDelete으로 생성가능한 Plan
 *
 *           [DETE]
 *
 ***********************************************************************/

    qmgDETE         * sMyGraph;
    qmnPlan         * sPlan;
    qmnPlan         * sChildPlan;
    qmoDETEInfo       sDETEInfo;

    IDU_FIT_POINT_FATAL( "qmgDelete::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgDETE*) aGraph;

    //---------------------------
    // Current CNF의 등록
    //---------------------------

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
    sMyGraph->graph.flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    sMyGraph->graph.flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;

    //---------------------------
    // Plan의 생성
    //---------------------------

    // 최상위 plan이다.
    IDE_DASSERT( aParent == NULL );

    if ( sMyGraph->mTableList == NULL )
    {
        IDE_TEST( qmoOneNonPlan::initDETE( aStatement ,
                                           &sPlan )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( qmoOneNonPlan::initMultiDETE( aStatement ,
                                                sMyGraph->graph.myQuerySet,
                                                sMyGraph->mTableList,
                                                &sPlan )
                  != IDE_SUCCESS);
    }

    sMyGraph->graph.myPlan = sPlan;

    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                              &sMyGraph->graph,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS);

    // fix BUG-13482
    // SCAN Limit 최적화 적용에 따른 DETE plan의 start value 결정유무
    if( sMyGraph->graph.left->type == QMG_SELECTION )
    {
        // projection 하위가 SCAN이고,
        // SCAN limit 최적화가 적용된 경우이면,
        // DETE의 limit start value를 1로 조정한다.
        if( ((qmgSELT*)(sMyGraph->graph.left))->limit != NULL )
        {
            qmsLimitI::setStartValue( sMyGraph->limit, 1 );
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

    // child가져오기
    sChildPlan = sMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정 
    //---------------------------------------------------
    sMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_DELETE;

    //----------------------------
    // DETE의 생성
    //----------------------------

    sDETEInfo.deleteTableRef   = sMyGraph->deleteTableRef;
    sDETEInfo.insteadOfTrigger = sMyGraph->insteadOfTrigger;
    sDETEInfo.limit            = sMyGraph->limit;
    sDETEInfo.childConstraints = sMyGraph->childConstraints;
    sDETEInfo.returnInto       = sMyGraph->returnInto;
    sDETEInfo.mTableList       = sMyGraph->mTableList;

    if ( sMyGraph->mTableList == NULL )
    {
        IDE_TEST( qmoOneNonPlan::makeDETE( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           & sDETEInfo ,
                                           sChildPlan ,
                                           sPlan )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( qmoOneNonPlan::makeMultiDETE( aStatement,
                                                sMyGraph->graph.myQuerySet,
                                                &sDETEInfo,
                                                sChildPlan,
                                                sPlan )
                  != IDE_SUCCESS );
    }

    sMyGraph->graph.myPlan = sPlan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgDelete::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgDelete::printGraph::__FT__" );

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

    IDE_TEST( aGraph->left->printGraph( aStatement,
                                        aGraph->left,
                                        aDepth + 1,
                                        aString )
              != IDE_SUCCESS );

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
