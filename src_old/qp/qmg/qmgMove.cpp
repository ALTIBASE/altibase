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
 * $Id: qmgMove.cpp 53774 2012-06-15 04:53:31Z eerien $
 *
 * Description :
 *     Move Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgMove.h>
#include <qmoOneNonPlan.h>
#include <qmgSelection.h>
#include <qmoPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE
#include <qcmPartition.h>   // PROJ-1502 PARTITIONED DISK TABLE

IDE_RC
qmgMove::init( qcStatement * aStatement,
               qmsQuerySet * aQuerySet,
               qmgGraph    * aChildGraph,
               qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgMove Graph의 초기화
 *
 * Implementation :
 *    (1) qmgMove을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgMOVE          * sMyGraph;
    qmmMoveParseTree * sParseTree;
    qmsQuerySet      * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgMove::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Move Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgMove을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgMOVE ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_MOVE;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgMove::optimize;
    sMyGraph->graph.makePlan = qmgMove::makePlan;
    sMyGraph->graph.printGraph = qmgMove::printGraph;

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
    // Move Graph 만을 위한 초기화
    //---------------------------------------------------

    sParseTree = (qmmMoveParseTree *)aStatement->myPlan->parseTree;
    
    // 최상위 graph인 move graph에 target table 정보를 설정
    sMyGraph->targetTableRef = sParseTree->targetTableRef;

    // 최상위 graph인 move graph에 column 정보를 설정
    sMyGraph->columns = sParseTree->columns;

    // 최상위 graph인 move graph에 value 정보를 설정
    sMyGraph->values         = sParseTree->values;
    sMyGraph->valueIdx       = sParseTree->valueIdx;
    sMyGraph->canonizedTuple = sParseTree->canonizedTuple;
    sMyGraph->compressedTuple= sParseTree->compressedTuple;     // PROJ-2264
    
    // 최상위 graph인 move graph에 sequence 정보를 설정
    sMyGraph->nextValSeqs = sParseTree->common.nextValSeqs;
    
    // 최상위 graph인 move graph에 limit을 연결
    sMyGraph->limit = sParseTree->limit;

    // 최상위 graph인 move graph에 constraint를 연결
    sMyGraph->parentConstraints = sParseTree->parentConstraints;
    sMyGraph->childConstraints  = sParseTree->childConstraints;
    sMyGraph->checkConstrList   = sParseTree->checkConstrList;

    // 최상위 graph인 insert graph에 Default Expr을 연결
    sMyGraph->defaultExprTableRef = sParseTree->defaultTableRef;
    sMyGraph->defaultExprColumns  = sParseTree->defaultExprColumns;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMove::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgMove의 최적화
 *
 * Implementation :
 *    (1) SCAN Limit 최적화
 *    (2) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgMOVE           * sMyGraph;
    qmgGraph          * sChildGraph;
    qtcNode           * sNode;
    qmsLimit          * sLimit;
    idBool              sIsScanLimit;

    SDouble             sOutputRecordCnt;
    SDouble             sMoveSelectivity = 1;    // BUG-17166

    IDU_FIT_POINT_FATAL( "qmgMove::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgMOVE*) aGraph;
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
        // MOVE 노드 생성시,
        // 하위 SCAN 노드에서 SCAN Limit 적용이 확정되면,
        // MOVE 노드의 limit start를 1로 변경한다.

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
    sMyGraph->graph.costInfo.selectivity = sMoveSelectivity;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sChildGraph->costInfo.outputRecordCnt;

    // outputRecordCnt
    sOutputRecordCnt = sChildGraph->costInfo.outputRecordCnt * sMoveSelectivity;
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
qmgMove::makePlan( qcStatement     * aStatement,
                   const qmgGraph  * aParent,
                   qmgGraph        * aGraph )
{
/***********************************************************************
 *
 * Description : qmgMove으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgMove으로 생성가능한 Plan
 *
 *           [MOVE]
 *
 ***********************************************************************/

    qmgMOVE         * sMyGraph;
    qmnPlan         * sPlan;
    qmnPlan         * sChildPlan;
    qmmValueNode    * sValueNode;
    qmoMOVEInfo       sMOVEInfo;

    IDU_FIT_POINT_FATAL( "qmgMove::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgMOVE*) aGraph;

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
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;

    //---------------------------
    // Plan의 생성
    //---------------------------

    // 최상위 plan이다.
    IDE_DASSERT( aParent == NULL );
    
    IDE_TEST( qmoOneNonPlan::initMOVE( aStatement ,
                                       &sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;
    
    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                              &sMyGraph->graph,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS);

    // fix BUG-13482
    // SCAN Limit 최적화 적용에 따른 MOVE plan의 start value 결정유무
    if( sMyGraph->graph.left->type == QMG_SELECTION )
    {
        // projection 하위가 SCAN이고,
        // SCAN limit 최적화가 적용된 경우이면,
        // MOVE의 limit start value를 1로 조정한다.
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
    sMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_MOVE;

    //----------------------------
    // MOVE의 생성
    //----------------------------

    sMOVEInfo.targetTableRef    = sMyGraph->targetTableRef;
    sMOVEInfo.columns           = sMyGraph->columns;
    sMOVEInfo.values            = sMyGraph->values;
    sMOVEInfo.valueIdx          = sMyGraph->valueIdx;
    sMOVEInfo.canonizedTuple    = sMyGraph->canonizedTuple;
    sMOVEInfo.compressedTuple   = sMyGraph->compressedTuple;    // PROJ-2264
    sMOVEInfo.nextValSeqs       = sMyGraph->nextValSeqs;
    sMOVEInfo.limit             = sMyGraph->limit;
    sMOVEInfo.parentConstraints = sMyGraph->parentConstraints;
    sMOVEInfo.childConstraints  = sMyGraph->childConstraints;
    sMOVEInfo.checkConstrList   = sMyGraph->checkConstrList;

    /* PROJ-1090 Function-based Index */
    sMOVEInfo.defaultExprTableRef = sMyGraph->defaultExprTableRef;
    sMOVEInfo.defaultExprColumns  = sMyGraph->defaultExprColumns;

    IDE_TEST( qmoOneNonPlan::makeMOVE( aStatement ,
                                       sMyGraph->graph.myQuerySet ,
                                       & sMOVEInfo ,
                                       sChildPlan ,
                                       sPlan )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sPlan;

    qmg::setPlanInfo( aStatement, sPlan, &(sMyGraph->graph) );

    //------------------------------------------
    // INSERT ... VALUES 구문 내의 Subquery 최적화
    //------------------------------------------

    // BUG-32584
    // 모든 서브쿼리에 대해서 MakeGraph 한후에 MakePlan을 해야 한다.
    for ( sValueNode = sMyGraph->values;
          sValueNode != NULL;
          sValueNode = sValueNode->next )
    {
        // Subquery 존재할 경우 Subquery 최적화
        if ( (sValueNode->value->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            IDE_TEST( qmoSubquery::optimizeExprMakeGraph( aStatement,
                                                          ID_UINT_MAX,
                                                          sValueNode->value )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nohting To Do
        }
    }

    for ( sValueNode = sMyGraph->values;
          sValueNode != NULL;
          sValueNode = sValueNode->next )
    {
        // Subquery 존재할 경우 Subquery 최적화
        if ( (sValueNode->value->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            IDE_TEST( qmoSubquery::optimizeExprMakePlan( aStatement,
                                                         ID_UINT_MAX,
                                                         sValueNode->value )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nohting To Do
        }
    }

    //----------------------------
    // into절에 대한 파티션드 테이블 최적화
    //----------------------------
        
    // PROJ-1502 PARTITIONED DISK TABLE
    IDE_TEST( qmoPartition::optimizeInto( aStatement,
                                          sMyGraph->targetTableRef )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgMove::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgMove::printGraph::__FT__" );

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
