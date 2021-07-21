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
 * $Id: qmg.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     모든 Graph에서 공통적으로 사용하는 기능
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qtc.h>
#include <qcg.h>
#include <qmgDef.h>
#include <qmgJoin.h>
#include <qmgLeftOuter.h>
#include <qmgFullOuter.h>
#include <qmgSelection.h>
#include <qmgPartition.h>
#include <qmgWindowing.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoJoinMethod.h>
#include <qmgSorting.h>
#include <qmgGrouping.h>
#include <qmgDistinction.h>
#include <qmgCounting.h>
#include <qmoCostDef.h>
#include <qmoViewMerging.h>
#include <qcgPlan.h>
#include <qmvQTC.h>
#include <qmv.h>

extern mtfModule mtfDecrypt;
extern mtdModule mtdClob;
extern mtdModule mtdBlob;
extern mtdModule mtdSmallint;

IDE_RC
qmg::initGraph( qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description :
 *    Graph를 구성하는 공통 정보를 초기화한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::initGraph::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // Graph의 기본 정보 초기화
    //---------------------------------------------------

    aGraph->flag = QMG_FLAG_CLEAR;
    qtc::dependencyClear( & aGraph->depInfo );

    //---------------------------------------------------
    // Graph의 Child Graph 초기화
    //---------------------------------------------------

    aGraph->left     = NULL;
    aGraph->right    = NULL;
    aGraph->children = NULL;

    //---------------------------------------------------
    // Graph의 부가 정보 초기화
    //---------------------------------------------------

    aGraph->myPlan            = NULL;
    aGraph->myPredicate       = NULL;
    aGraph->constantPredicate = NULL;
    aGraph->ridPredicate      = NULL;
    aGraph->nnfFilter         = NULL;
    aGraph->myFrom            = NULL;
    aGraph->myQuerySet        = NULL;
    aGraph->myCNF             = NULL;
    aGraph->preservedOrder    = NULL;
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmg::printGraph::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aString != NULL );

    //-----------------------------------
    // Graph 종류의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "-------------------------------------------------" );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    switch ( aGraph->type )
    {
        case QMG_SELECTION :
            iduVarStringAppend( aString,
                                "[[ SELECTION GRAPH ]]" );
            break;
        case QMG_PROJECTION :
            iduVarStringAppend( aString,
                                "[[ PROJECTION GRAPH ]]" );
            break;
        case QMG_DISTINCTION :
            iduVarStringAppend( aString,
                                "[[ DISTINCTION GRAPH ]]" );
            break;
        case QMG_GROUPING :
            iduVarStringAppend( aString,
                                "[[ GROUPING GRAPH ]]" );
            break;
        case QMG_SORTING :
            iduVarStringAppend( aString,
                                "[[ SORTING GRAPH ]]" );
            break;
        case QMG_INNER_JOIN :
            iduVarStringAppend( aString,
                                "[[ INNER JOIN GRAPH ]]" );
            break;
        case QMG_SEMI_JOIN :
            iduVarStringAppend( aString,
                                "[[ SEMI JOIN GRAPH ]]" );
            break;
        case QMG_ANTI_JOIN :
            iduVarStringAppend( aString,
                                "[[ ANTI JOIN GRAPH ]]" );
            break;
        case QMG_LEFT_OUTER_JOIN :
            iduVarStringAppend( aString,
                                "[[ LEFT OUTER JOIN GRAPH ]]" );
            break;
        case QMG_FULL_OUTER_JOIN :
            iduVarStringAppend( aString,
                                "[[ FULL OUTER JOIN GRAPH ]]" );
            break;
        case QMG_SET :
            iduVarStringAppend( aString,
                                "[[ SET GRAPH ]]" );
            break;
        case QMG_HIERARCHY :
            iduVarStringAppend( aString,
                                "[[ HIERARCHY GRAPH ]]" );
            break;
        case QMG_DNF :
            iduVarStringAppend( aString,
                                "[[ DNF GRAPH ]]" );
            break;
        case QMG_PARTITION :
            iduVarStringAppend( aString,
                                "[[ PARTITION GRAPH ]]" );
            break;
        case QMG_COUNTING :
            iduVarStringAppend( aString,
                                "[[ COUNTING GRAPH ]]" );
            break;
        case QMG_WINDOWING :
            iduVarStringAppend( aString,
                                "[[ WINDOWING GRAPH ]]" );
            break;         
        case QMG_INSERT :
            iduVarStringAppend( aString,
                                "[[ INSERT GRAPH ]]" );
            break;         
        case QMG_MULTI_INSERT :
            iduVarStringAppend( aString,
                                "[[ MULTIPLE INSERT GRAPH ]]" );
            break;         
        case QMG_UPDATE :
            iduVarStringAppend( aString,
                                "[[ UPDATE GRAPH ]]" );
            break;         
        case QMG_DELETE :
            iduVarStringAppend( aString,
                                "[[ DELETE GRAPH ]]" );
            break;         
        case QMG_MOVE :
            iduVarStringAppend( aString,
                                "[[ MOVE GRAPH ]]" );
            break;         
        case QMG_MERGE :
            iduVarStringAppend( aString,
                                "[[ MERGE GRAPH ]]" );
            break;         
        case QMG_RECURSIVE_UNION_ALL :
            iduVarStringAppend( aString,
                                "[[ RECURSIVE UNION ALL GRAPH ]]" );
            break;         
        case QMG_SHARD_SELECT:
            iduVarStringAppend( aString,
                                "[[ SHARD SELECT GRAPH ]]" );
            break;
        case QMG_SHARD_DML:
            iduVarStringAppend( aString,
                                "[[ SHARD DML GRAPH ]]" );
            break;
        case QMG_SHARD_INSERT:
            iduVarStringAppend( aString,
                                "[[ SHARD INSERT GRAPH ]]" );
            break;
        default :
            IDE_DASSERT(0);
            break;
    }

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "-------------------------------------------------" );

    //-----------------------------------
    // Graph 공통 비용 정보의 출력
    //-----------------------------------

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppend( aString,
                        "== Cost Information ==" );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "INPUT_RECORD_COUNT : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.inputRecordCnt );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "OUTPUT_RECORD_COUNT: %"ID_PRINT_G_FMT,
                              aGraph->costInfo.outputRecordCnt );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "RECORD_SIZE        : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.recordSize );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "SELECTIVITY        : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.selectivity );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "GRAPH_ACCESS_COST  : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.myAccessCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "GRAPH_DISK_COST    : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.myDiskCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "GRAPH_TOTAL_COST   : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.myAllCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "TOTAL_ACCESS_COST  : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.totalAccessCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "TOTAL_DISK_COST    : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.totalDiskCost );

    QMG_PRINT_LINE_FEED( i, aDepth, aString );
    iduVarStringAppendFormat( aString,
                              "TOTAL_ALL_COST     : %"ID_PRINT_G_FMT,
                              aGraph->costInfo.totalAllCost );
    
    return IDE_SUCCESS;

}

IDE_RC
qmg::printSubquery( qcStatement  * aStatement,
                    qtcNode      * aSubQuery,
                    ULong          aDepth,
                    iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *    Subquery 내의 Graph 정보를 출력한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  i;
    UInt  sArguNo;
    UInt  sArguCount;

    qtcNode  * sNode;
    qmgGraph * sGraph;

    IDU_FIT_POINT_FATAL( "qmg::printSubquery::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSubQuery != NULL );
    IDE_DASSERT( aString != NULL );

    //-----------------------------------
    // Graph 종류의 출력
    //-----------------------------------

    if ( ( aSubQuery->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "::::SUB-QUERY BEGIN" );
        QMG_PRINT_LINE_FEED( i, aDepth, aString );

        sGraph = aSubQuery->subquery->myPlan->graph;
        IDE_TEST( sGraph->printGraph( aStatement,
                                      sGraph,
                                      aDepth + 1,
                                      aString ) != IDE_SUCCESS );

        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
        iduVarStringAppend( aString,
                            "::::SUB-QUERY END" );
        QMG_PRINT_LINE_FEED( i, aDepth, aString );
    }
    else
    {
        sArguCount = ( aSubQuery->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

        if ( sArguCount > 0 ) // This node has arguments.
        {
            if ( (aSubQuery->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                 == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                for (sNode = (qtcNode *)aSubQuery->node.arguments;
                     sNode != NULL;
                     sNode = (qtcNode *)sNode->node.next)
                {
                    IDE_TEST( printSubquery( aStatement,
                                             sNode,
                                             aDepth,
                                             aString ) != IDE_SUCCESS );
                }
            }
            else
            {
                for (sArguNo = 0,
                         sNode = (qtcNode *)aSubQuery->node.arguments;
                     sArguNo < sArguCount && sNode != NULL;
                     sArguNo++,
                         sNode = (qtcNode *)sNode->node.next)
                {
                    IDE_TEST( printSubquery( aStatement,
                                             sNode,
                                             aDepth,
                                             aString ) != IDE_SUCCESS );
                }
            }
        }
        else
        {
            // Nothing To Do
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC
qmg::checkUsableOrder( qcStatement       * aStatement,
                       qmgPreservedOrder * aWantOrder,
                       qmgGraph          * aLeftGraph,
                       qmoAccessMethod  ** aOriginalMethod,
                       qmoAccessMethod  ** aSelectMethod,
                       idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Graph가 원하는 Order를 사용할 수 있는 지를 검사한다.
 *    [주의]
 *       - 입력된 aWantOrder는 처리 과정에서 임의로
 *         연결 관계가 사라질 수 있으므로 재사용할 수 없다.
 *
 *    [리턴값]
 *        사용 가능 여부                     : aUsable
 *        사용 가능시 선택되는 Access Method : aSelectMethod
 *        기존에 사용되던 Access Method      : aOriginalMethod
 *
 * Implementation :
 *
 *    [aWantOrder의 구성]
 *
 *       - Graph가 원하는 Order
 *
 *       - Order가 중요한 경우에는 원하는 Order에
 *         ASC, DESC등을 기록한다.
 *           - Ex) Sorting에서의 Order
 *                 SELECT * FROM T1 ORDER BY I1 ASC, I2 DESC;
 *                 (1,1,ASC) --> (1,2,DESC)
 *
 *           - Ex) Merge Join에서의 Order (모두 Ascending)
 *                 SELECT * FROM T1, T2 WHERE T1.i1 = T2.i1;
 *                 (1,1,ASC) --> (2,1, DESC)
 *
 *       - Order가 중요하지 않은 경우에는 원하는 Order에
 *         방향 정보를 기록하지 않는다.
 *           - Ex) Distinction에서의 Order
 *                 SELECT DISTINCT t1.i1, t2.i2, t1.i2 from t1, t2;
 *                 (1,1,NOT) --> (2,2,NOT) --> (1,2,NOT)
 *
 *    [ 처리 절차 ]
 *
 *    1. Child Graph에 소속된 Want Order로 구분
 *        - 원하는 Order가 좌우 Child 에 속하는 지를 검사하고,
 *          이를 좌우로 구분한다.
 *
 *    2. Child Graph에서 해당 Order를 처리할 수 있는 지를 검사.
 *
 ***********************************************************************/

    idBool sUsable;

    IDU_FIT_POINT_FATAL( "qmg::checkUsableOrder::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aLeftGraph != NULL );
    IDE_DASSERT( aUsable != NULL );
    IDE_DASSERT( aOriginalMethod != NULL );
    IDE_DASSERT( aSelectMethod != NULL );

    IDE_TEST( checkOrderInGraph( aStatement,
                                 aLeftGraph,
                                 aWantOrder,
                                 aOriginalMethod,
                                 aSelectMethod,
                                 & sUsable )
              != IDE_SUCCESS );

    *aUsable = sUsable;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
qmg::setOrder4Child( qcStatement       * aStatement,
                     qmgPreservedOrder * aWantOrder,
                     qmgGraph          * aLeftGraph )
{
/***********************************************************************
 *
 * Description :
 *
 *    원하는 Order를 Child Graph에 적용시킨다.
 *
 *    [주의 1]
 *       - 입력된 aWantOrder는 처리 과정에서 임의로
 *         연결 관계가 사라질 수 있으므로 재사용할 수 없다.
 *
 *    [주의 2]
 *       - 반드시 Preserved Order를 사용할 수 있는 경우에 한하여 수행
 *       - 즉, qmg::checkUsableOrder() 를 통해 사용할 수 있는 Order일
 *         경우에 한함.
 *
 *    [주의 3]
 *       - 해당 Graph의 Preserved Order는 외부에서 직접 생성해야 함.
 *
 * Implementation :
 *
 *    1. Want Order를 각 Child Graph의 Order로 분리
 *
 *    2. 해당 Graph의 종류에 따라 처리
 *        - Selection Graph인 경우
 *            - Base Table 인 경우 :
 *                해당 Index를 찾은 후 Preserved Order Build
 *            - View인 경우
 *                하위 Target의 ID로 변경하여 Child Graph에 대한 처리
 *                입력된 Want Order로 Preserved Order Build
 *        - Set, Dnf, Hierarchy인 경우
 *            - 해당 사항 없음.
 *        - 이 외의 Graph인 경우
 *            - 입력된 Want Order로 Preserved Order Build
 *            - Recursive한 수행
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::setOrder4Child::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aLeftGraph != NULL );

    //---------------------------------------------------
    // Left Graph에 대한 Preserved Order 구축
    //---------------------------------------------------

    IDE_TEST( makeOrder4Graph( aStatement,
                               aLeftGraph,
                               aWantOrder )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC
qmg::tryPreservedOrder( qcStatement       * aStatement,
                        qmgGraph          * aGraph,
                        qmgPreservedOrder * aWantOrder,
                        idBool            * aSuccess )
{
/***********************************************************************
 *
 * Description : Preserved Order 사용 가능한 경우, 적용
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgPreservedOrder * sCur;
    qmgPreservedOrder * sNewOrder;

    qmgPreservedOrder * sCheckOrder;
    qmgPreservedOrder * sCheckCurOrder;
    qmgPreservedOrder * sPushOrder;
    qmgPreservedOrder * sPushCurOrder;
    qmgPreservedOrder * sChildCur;

    qmoAccessMethod   * sOriginalMethod;
    qmoAccessMethod   * sSelectMethod;
    idBool              sIsDefined = ID_FALSE;
    idBool              sUsable;

    IDU_FIT_POINT_FATAL( "qmg::tryPreservedOrder::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );
    IDE_FT_ASSERT( aWantOrder != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sUsable        = ID_FALSE;
    sCheckOrder    = NULL;
    sCheckCurOrder = NULL;
    sPushOrder     = NULL;
    sPushCurOrder  = NULL;


    for ( sCur = aWantOrder; sCur != NULL; sCur = sCur->next )
    {
        // 검사할 Order, Push할 Order를 위한 공간 할당
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder) * 2,
                                                 (void**)&sNewOrder )
                  != IDE_SUCCESS );

        sNewOrder[0].table = sCur->table;
        sNewOrder[0].column = sCur->column;
        sNewOrder[0].direction = sCur->direction;
        sNewOrder[0].next = NULL;

        idlOS::memcpy( & sNewOrder[1],
                       & sNewOrder[0],
                       ID_SIZEOF( qmgPreservedOrder ) );

        if ( sCheckOrder == NULL )
        {
            sCheckOrder = sCheckCurOrder = & sNewOrder[0];
            sPushOrder = sPushCurOrder = & sNewOrder[1];
        }
        else
        {
            sCheckCurOrder->next = & sNewOrder[0];
            sCheckCurOrder = sCheckCurOrder->next;

            sPushCurOrder->next = & sNewOrder[1];
            sPushCurOrder = sPushCurOrder->next;
        }

        //BUG-40361 supporting to indexable analytic function
        if ( sCur->direction != QMG_DIRECTION_NOT_DEFINED )
        {
            sIsDefined = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    // preserved order 적용 가능 검사
    IDE_TEST( qmg::checkUsableOrder( aStatement,
                                     sCheckOrder,
                                     aGraph->left,
                                     & sOriginalMethod,
                                     & sSelectMethod,
                                     & sUsable )
              != IDE_SUCCESS );

    if ( sUsable == ID_TRUE )
    {
        //---------------------------------------------------
        // preserved order 적용 가능한 경우
        //---------------------------------------------------

        // To Fix PR-11945
        // Child Graph의 preserved order가 존재하고,
        // preserved order의 방향도 존재하는 경우라면
        // 하위 preserved order를 재구축해서는 안된다.
        // 하위에서 Merger Join을 위해 ASC order를 구축한 경우,
        // 상위의 Group By등이 사용될 때 하위의 방향을 변경시켜서는 안된다.
        
        if ( aGraph->left->preservedOrder != NULL )
        {
            if ( ( aGraph->left->flag & QMG_PRESERVED_ORDER_MASK )
                 == QMG_PRESERVED_ORDER_DEFINED_NOT_FIXED )
            {
                // 하위에 Window Graph가 존재하는 경우
                IDE_TEST( qmg::setOrder4Child( aStatement,
                                               sPushOrder,
                                               aGraph->left )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( aGraph->left->preservedOrder->direction !=
                     QMG_DIRECTION_NOT_DEFINED )
                {
                    // 하위에 Preserved Order가 존재하고
                    // 방향도 이미 결정된 경우
                    // Nothing To Do
                }
                else
                {
                    // 하위에 Preserved Order는 있으나,
                    // 방향이 결정되지 않은 경우
                    IDE_TEST( qmg::setOrder4Child( aStatement,
                                                   sPushOrder,
                                                   aGraph->left )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            // 하위에 Preserved Order가 없는 경우
            // 하위의 preserved order 설정
            IDE_TEST( qmg::setOrder4Child( aStatement,
                                           sPushOrder,
                                           aGraph->left )
                      != IDE_SUCCESS );
        }

        // 자신의 preserved order 설정
        if ( sIsDefined == ID_FALSE )
        {
            // To Fix BUG-8710
            // ex ) want order ( not defined, not defined )
            //      set  order ( not defined, same ( diff ) with prev, ... )
            // want order가 not defined라 하더라도 direction을 변경시켜야함
            // To Fix BUG-11373
            // child의 order는 indexable group by 의 경우
            // index의 순서대로 바뀌는 경우가 있기 때문에
            // child의 order를 그대로 복사해야함.
            // ex ) index            : i1->i2->i3
            //      group by         : i2->i1->i3
            //      selection graph  : i2->i1->i3을 내려주면 index순서대로
            //                         i1->i2->i3으로 변경하여 저장됨
            for ( sCur=aWantOrder,sChildCur = aGraph->left->preservedOrder;
                  (sCur != NULL) && (sChildCur != NULL);
                  sCur = sCur->next, sChildCur = sChildCur->next )
            {
                sCur->direction = sChildCur->direction;
                sCur->table     = sChildCur->table;
                sCur->column    = sChildCur->column;
            }
        }
        else
        {
            // nothing to do
        }

        aGraph->preservedOrder = aWantOrder;
    }
    else
    {
        aGraph->preservedOrder = NULL;
    }

    *aSuccess = sUsable;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ----------------------------------------------------------------------------
 * BUG-39298 improve preserved order
 *
 * ex)
 * create index .. on t1 (i1, i2, i3)
 * select .. from .. where t1.i1 = 1 and t1.i2 = 2 order by i3;
 * (i1, i2) 는 고정된 값이므로 preserved order 를 사용할 수 있도록 한다.
 *
 * 다음과 같은 상황에서만 처리한다.
 * - SORT 를 위한 preserved order (grouping, distinct 제외)
 * - qmgSort 밑에 qmgSelection 인 경우
 * - index 가 선택된 경우 (preserved order 를 위해 index 강제 변경 X)
 * - index hint 에 ASC, DESC 가 없는경우
 * ----------------------------------------------------------------------------
 */
IDE_RC qmg::retryPreservedOrder(qcStatement       * aStatement,
                                qmgGraph          * aGraph,
                                qmgPreservedOrder * aWantOrder,
                                idBool            * aUsable)
{
    qmgGraph         * sChildGraph;
    qmgSELT          * sSELTGraph;
    qmgPreservedOrder* sIter;
    qmgPreservedOrder* sIter2;
    qmgPreservedOrder* sStart;
    qmgPreservedOrder* sNewOrder;
    qmoKeyRangeColumn* sKeyRangeCol;
    idBool             sUsable;
    UInt               sOrderCount;
    UInt               sStartIdx;
    UInt               i;

    IDU_FIT_POINT_FATAL( "qmg::retryPreservedOrder::__FT__" );

    sUsable = ID_FALSE;

    IDE_DASSERT(aWantOrder   != NULL);
    IDE_DASSERT(aGraph->left != NULL);

    sChildGraph = aGraph->left;

    if (aWantOrder->direction == QMG_DIRECTION_NOT_DEFINED)
    {
        IDE_CONT(LABEL_EXIT);
    }

    if (sChildGraph->preservedOrder == NULL)
    {
        IDE_CONT(LABEL_EXIT);
    }

    if (sChildGraph->preservedOrder->direction != QMG_DIRECTION_NOT_DEFINED)
    {
        IDE_CONT(LABEL_EXIT);
    }

    if (sChildGraph->type != QMG_SELECTION)
    {
        IDE_CONT(LABEL_EXIT);
    }

    sSELTGraph = (qmgSELT*)sChildGraph;

    if (sSELTGraph->selectedMethod == NULL)
    {
        IDE_CONT(LABEL_EXIT);
    }

    if (sSELTGraph->selectedMethod->method == NULL)
    {
        IDE_CONT(LABEL_EXIT);
    }

    if (sChildGraph->myFrom->tableRef->table != aWantOrder->table)
    {
        IDE_CONT(LABEL_EXIT);
    }

    sKeyRangeCol  = &sSELTGraph->selectedMethod->method->mKeyRangeColumn;

    for (sOrderCount = 0, sIter = sChildGraph->preservedOrder;
         sIter != NULL;
         sIter = sIter->next, sOrderCount++);

    for (sIter = sChildGraph->preservedOrder, sStartIdx = 0;
         sIter != NULL;
         sIter = sIter->next, sStartIdx++)
    {
        if (sIter->column != aWantOrder->column)
        {
            for (i = 0; i < sKeyRangeCol->mColumnCount; i++)
            {
                if (sKeyRangeCol->mColumnArray[i] == sIter->column)
                {
                    break;
                }
            }
            if (i == sKeyRangeCol->mColumnCount)
            {
                /* no way match */
                IDE_CONT(LABEL_EXIT);
            }
            else
            {
                /* try next column */
            }
        }
        else
        {
            /* start position found */
            break;
        }
    }

    if (sIter == NULL)
    {
        IDE_CONT(LABEL_EXIT);
    }

    sStart = sIter;

    for (sIter = aWantOrder, sIter2 = sStart;
         sIter != NULL && sIter2 != NULL;
         sIter = sIter->next, sIter2 = sIter2->next)
    {
        if ((sIter->table != sIter2->table) ||
            (sIter->column != sIter2->column))
        {
            break;
        }
    }

    if (sIter != NULL)
    {
        /* do not cover all want orders */
        IDE_CONT(LABEL_EXIT);
    }

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmgPreservedOrder) *
                                           sOrderCount,
                                           (void**)&sNewOrder)
             != IDE_SUCCESS);

    /*
     * forward 가 가능한지 비교
     */
    sNewOrder[0].table     = sChildGraph->preservedOrder->table;
    sNewOrder[0].column    = sChildGraph->preservedOrder->column;
    sNewOrder[0].direction = QMG_DIRECTION_ASC;

    for (i = 1, sIter = sChildGraph->preservedOrder->next;
         i < sOrderCount;
         i++, sIter = sIter->next)
    {
        sNewOrder[i].table  = sIter->table;
        sNewOrder[i].column = sIter->column;
        switch (sIter->direction)
        {
            case QMG_DIRECTION_SAME_WITH_PREV:
                if (sNewOrder[i-1].direction == QMG_DIRECTION_ASC)
                {
                    sNewOrder[i].direction = QMG_DIRECTION_ASC;
                }
                else
                {
                    sNewOrder[i].direction = QMG_DIRECTION_DESC;
                }
                break;
            case QMG_DIRECTION_DIFF_WITH_PREV:
                if (sNewOrder[i-1].direction == QMG_DIRECTION_ASC)
                {
                    sNewOrder[i].direction = QMG_DIRECTION_DESC;
                }
                else
                {
                    sNewOrder[i].direction = QMG_DIRECTION_ASC;
                }
                break;
            default:
                IDE_CONT(LABEL_EXIT);
        }
        sNewOrder[i-1].next = &sNewOrder[i];
    }

    for (sIter = aWantOrder, sIter2 = &sNewOrder[sStartIdx];
         sIter != NULL;
         sIter = sIter->next, sIter2 = sIter2->next)
    {
        if (sIter->direction != sIter2->direction)
        {
            break;
        }
    }

    if (sIter == NULL)
    {
        sUsable = ID_TRUE;
        IDE_CONT(LABEL_EXIT);
    }

    /*
     * forward 가 실패한 경우 backward 가 가능한지 비교
     */
    for (i = 0; i < sOrderCount; i++)
    {
        if (sNewOrder[i].direction == QMG_DIRECTION_ASC)
        {
            sNewOrder[i].direction = QMG_DIRECTION_DESC;
        }
        else
        {
            sNewOrder[i].direction = QMG_DIRECTION_ASC;
        }
    }

    for (sIter = aWantOrder, sIter2 = &sNewOrder[sStartIdx];
         sIter != NULL;
         sIter = sIter->next, sIter2 = sIter2->next)
    {
        if (sIter->direction != sIter2->direction)
        {
            break;
        }
    }

    if (sIter == NULL)
    {
        sUsable = ID_TRUE;
    }

    IDE_EXCEPTION_CONT(LABEL_EXIT);

    *aUsable = sUsable;

    if (sUsable == ID_TRUE)
    {
        /*
         * match 된 경우 child 의 preserved order 를 fix
         */
        for (sIter = sChildGraph->preservedOrder, i = 0;
             i < sOrderCount;
             sIter = sIter->next, i++)
        {
            sIter->direction = sNewOrder[i].direction;
        }
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getBucketCntWithTarget(qcStatement* aStatement,
                                   qmgGraph   * aGraph,
                                   qmsTarget  * aTarget,
                                   UInt         aHintBucketCnt,
                                   UInt       * aBucketCnt )
{
/***********************************************************************
 *
 * Description : target의 칼럼들의 cardinality를 이용하여 bucket count 구하는
 *               함수
 *
 * Implementation :
 *    - hash bucket count hint가 존재하지 않을 경우
 *      hash bucket count = MIN( 하위 graph의 outputRecordCnt / 2,
 *                               Target Column들의 cardinality 곱 )
 *    - hash bucket count hint가 존재할 경우
 *      hash bucket count = hash bucket count hint 값
 *
 ***********************************************************************/

    qmsTarget      * sTarget;
    qtcNode        * sNode;
    qmoColCardInfo * sColCardInfo;
    SDouble          sCardinality;
    SDouble          sBucketCnt;
    ULong            sBucketCntOutput;
    idBool           sAllColumn;

    IDU_FIT_POINT_FATAL( "qmg::getBucketCntWithTarget::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aTarget != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sAllColumn = ID_TRUE;
    sCardinality = 1;
    sBucketCnt = 1;

    if ( aHintBucketCnt == QMS_NOT_DEFINED_BUCKET_CNT )
    {

        //------------------------------------------
        // hash bucket count hint가 존재하지 않는 경우
        //------------------------------------------

        //------------------------------------------
        // 기본 bucket count 설정
        // bucket count = 하위 graph의 ouput record count / 2
        //------------------------------------------

        if ( ( aGraph->type == QMG_SET ) &&
             ( aGraph->myQuerySet->setOp == QMS_UNION ) )
        {
            sBucketCnt = ( aGraph->left->costInfo.outputRecordCnt +
                           aGraph->right->costInfo.outputRecordCnt ) / 2.0;
        }
        else
        {
            sBucketCnt = aGraph->left->costInfo.outputRecordCnt / 2.0;
        }
        sBucketCnt = (sBucketCnt < 1) ? 1 : sBucketCnt;

        //------------------------------------------
        // target column들의 cardinality 곱을 구함
        // ( 단, target column들이 모두 순수한 칼럼이어야 함 )
        //------------------------------------------

        for ( sTarget = aTarget; sTarget != NULL; sTarget = sTarget->next )
        {
            sNode = sTarget->targetColumn;

            // BUG-38193 target의 pass node 를 고려해야 합니다.
            if ( sNode->node.module == &qtc::passModule )
            {
                sNode = (qtcNode*)(sNode->node.arguments);
            }
            else
            {
                // Nothing to do.
            }

            // BUG-20272
            if ( sNode->node.module == &mtfDecrypt )
            {
                sNode = (qtcNode*) sNode->node.arguments;
            }
            else
            {
                // Nothing to do.
            }

            if ( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
            {
                // QMG_SET 의 경우
                // validation 과정에서 tuple 을 할당받아 target 을 새로 생성
                // tablemap[table].from 이 NULL 이 되어
                // one column list 및 statInfo 획득 불가
                IDE_DASSERT( aGraph->type != QMG_SET );

                if( sNode->node.column == MTC_RID_COLUMN_ID )
                {
                    /*
                     * prowid pseudo column 에 대한 통계정보가 고려 안되어있다
                     * 그냥 default 값으로 처리
                     */
                    sCardinality *= QMO_STAT_COLUMN_NDV;
                }
                else
                {
                    /* target 대상이 순수한 칼럼인 경우 */
                    sColCardInfo = QC_SHARED_TMPLATE(aStatement)->
                        tableMap[sNode->node.table].
                        from->tableRef->statInfo->colCardInfo;

                    sCardinality *= sColCardInfo[sNode->node.column].columnNDV;
                }
            }
            else
            {
                // target 대상이 하나라도 칼럼이 아닌 경우, 중단
                sAllColumn = ID_FALSE;
                break;
            }

        }

        if ( sAllColumn == ID_TRUE )
        {
            //------------------------------------------
            // MIN( 하위 graph의 outputRecordCnt / 2,
            //      Target Column들의 cardinality 곱 )
            //------------------------------------------

            if ( sBucketCnt > sCardinality )
            {
                sBucketCnt = sCardinality;
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

        // hash bucket count 보정
        if ( sBucketCnt < QCU_OPTIMIZER_BUCKET_COUNT_MIN )
        {
            sBucketCnt = QCU_OPTIMIZER_BUCKET_COUNT_MIN;
        }
        else
        {
            /* BUG-48161 */
            if ( sBucketCnt > QCG_GET_BUCKET_COUNT_MAX( aStatement ) )
            {
                sBucketCnt = QCG_GET_BUCKET_COUNT_MAX( aStatement );
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        //------------------------------------------
        // hash bucket count hint가 존재하는 경우
        //------------------------------------------

        sBucketCnt = aHintBucketCnt;
    }

    // BUG-36403 플랫폼마다 BucketCnt 가 달라지는 경우가 있습니다.
    sBucketCntOutput = DOUBLE_TO_UINT64( sBucketCnt );
    *aBucketCnt      = (UInt)sBucketCntOutput;
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::getBucketCnt4DistAggr( qcStatement * aStatement,
                            SDouble       aChildOutputRecordCnt,
                            UInt          aGroupBucketCnt,
                            qtcNode     * aNode,
                            UInt          aHintBucketCnt,
                            UInt        * aBucketCnt )
{
/***********************************************************************
 *
 * Description : hash bucket count의 설정
 *
 * Implementation :
 *    - hash bucket count hint가 존재하지 않을 경우
 *      - distinct 대상이 컬럼인 경우
 *         sDistBucketCnt = Distinct Aggregation Column의 cardinality
 *      - 컬럼이 아닌경우
 *        sDistBucketCnt = 하위 graph의 outputRecordCnt / 2
 *
 *      sBucketCnt = MAX( ChildOutputRecordCnt / GroupBucketCnt, 1.0 )
 *      sBucketCnt = MIN( sBucketCnt, sDistBucketCnt )
 *
 *    - hash bucket count hint가 존재할 경우
 *      hash bucket count = hash bucket count hint 값
 *
 ***********************************************************************/

    qmoColCardInfo * sColCardInfo;
    SDouble          sBucketCnt;
    SDouble          sDistBucketCnt;
    ULong            sBucketCntOutput;

    IDU_FIT_POINT_FATAL( "qmg::getBucketCnt4DistAggr::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );


    if ( aHintBucketCnt == QMS_NOT_DEFINED_BUCKET_CNT )
    {
        // hash bucket count hint가 존재하지 않는 경우

        if ( QTC_IS_COLUMN( aStatement, aNode ) == ID_TRUE )
        {
            sColCardInfo = QC_SHARED_TMPLATE(aStatement)->
                tableMap[aNode->node.table].
                from->tableRef->statInfo->colCardInfo;

            sDistBucketCnt = sColCardInfo[aNode->node.column].columnNDV;
        }
        else
        {
            sDistBucketCnt = IDL_MAX( aChildOutputRecordCnt / 2.0, 1.0 );
        }

        sBucketCnt = IDL_MAX( aChildOutputRecordCnt / aGroupBucketCnt, 1.0 );
        sBucketCnt = IDL_MIN( sBucketCnt, sDistBucketCnt );

        if ( sBucketCnt > QMC_MEM_HASH_MAX_BUCKET_CNT )
        {
            sBucketCnt = QMC_MEM_HASH_MAX_BUCKET_CNT;
        }
        else
        {
            // nothing to do
        }

        // BUG-36403 플랫폼마다 BucketCnt 가 달라지는 경우가 있습니다.
        sBucketCntOutput = DOUBLE_TO_UINT64( sBucketCnt );
        *aBucketCnt      = (UInt)sBucketCntOutput;
    }
    else
    {
        // bucket count hint가 존재하는 경우
        *aBucketCnt      = aHintBucketCnt;
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::isDiskTempTable( qmgGraph    * aGraph,
                      idBool      * aIsDisk )
{
/***********************************************************************
 *
 * Description :
 *     Graph 가 사용할 저장 매체를 판단한다.
 *     Join Graph의 경우 저장할 Child Graph를 인자로 받아야 한다.
 *
 * Implementation :
 *
 *     판단방법
 *         - Hint 가 존재할 경우 해당 Hint를 따른다.
 *         - Hint가 없을 경우 해당 Graph의 저장 매체를 따른다.
 *
 ***********************************************************************/

    qmgGraph    * sHintGraph;
    idBool        sIsDisk = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmg::isDiskTempTable::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aIsDisk != NULL );

    // SET 과 같이 힌트가 별도로 존재하지 않는 Graph를 위해
    // 힌트를 획득할 수 있는 Graph로 이동
    for ( sHintGraph = aGraph;
          ;
          sHintGraph = sHintGraph->left )
    {
        IDE_DASSERT( sHintGraph != NULL );
        IDE_DASSERT( sHintGraph->myQuerySet != NULL );

        if( sHintGraph->myQuerySet->SFWGH != NULL )
        {
            break;
        }
    }

    //------------------------------------------
    // 저장 매체 판단
    //------------------------------------------

    switch ( sHintGraph->myQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED:
            // Hint 가 없는 경우
            // 현재 Graph의 저장 매체를 그대로 사용한다.
            if ( ( aGraph->flag & QMG_GRAPH_TYPE_MASK )
                 == QMG_GRAPH_TYPE_MEMORY )
            {
                sIsDisk = ID_FALSE;
            }
            else
            {
                sIsDisk = ID_TRUE;
            }

            break;

        case QMO_INTER_RESULT_TYPE_MEMORY :
            // Memory Temp Table 사용 Hint

            sIsDisk = ID_FALSE;
            break;

        case QMO_INTER_RESULT_TYPE_DISK :
            // Disk Temp Table 사용 Hint

            sIsDisk = ID_TRUE;
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    *aIsDisk = sIsDisk;
    
    return IDE_SUCCESS;

}

// Plan Tree 생성시 공통 함수
IDE_RC
qmg::makeColumnMtrNode( qcStatement * aStatement ,
                        qmsQuerySet * aQuerySet ,
                        qtcNode     * aSrcNode ,
                        idBool        aConverted,
                        UShort        aNewTupleID ,
                        UInt          aFlag,
                        UShort      * aColumnCount ,
                        qmcMtrNode ** aMtrNode)
{
/***********************************************************************
 *
 * Description : 기능에 맞게 Materialize할 컬럼들을 구성한다
 *
 * Implementation :
 *     - srNode의 구성
 *     - 컴럼의 종류에 맞는 identifier를 구분하여 flag를 세팅한다.
 *     - aStartColumnID로 부터 시작하여, 추가로 생성된 컬럼의 개수를
 *       더하여 aColumnCount를 계산한다.
 *
 ***********************************************************************/

    qmcMtrNode        * sNewMtrNode;
    const mtfModule   * sModule;
    qtcNode           * sSrcNode;
    mtcNode           * sArgs;
    mtcTemplate       * sMtcTemplate;
    idBool              sNeedConversion;
    idBool              sNeedEvaluation;
    ULong               sFlag;

    IDU_FIT_POINT_FATAL( "qmg::makeColumnMtrNode::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSrcNode   != NULL );

    //----------------------------------
    // Source 노드 구성
    //----------------------------------

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmcMtrNode, &sNewMtrNode)
              != IDE_SUCCESS);

    // Node를 복사하여 복사된 node를 srcNode로 설정한다.
    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sSrcNode)
              != IDE_SUCCESS);

    idlOS::memcpy( sSrcNode,
                   aSrcNode,
                   ID_SIZEOF( qtcNode ) );

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sSrcNode,
                                       aNewTupleID,
                                       ID_FALSE )
              != IDE_SUCCESS );

    sFlag = sMtcTemplate->rows[sSrcNode->node.table].lflag;

    if( ( QTC_IS_AGGREGATE( aSrcNode ) == ID_TRUE ) &&
        ( ( sFlag & MTC_TUPLE_PLAN_MASK ) == MTC_TUPLE_PLAN_TRUE ) )
    {
        // PROJ-2179
        // Aggregate function은 tuple-set 내에서 여러개의 공간을 차지하는데,
        // 최초 수행 이후에는 최종 결과가 담겨있는 1개 공간만을 참조해도 된다.
        // 따라서 value/column module로 변경하여 공간을 줄인다.
        aSrcNode->node.module = &qtc::valueModule;
        sSrcNode->node.module = &qtc::valueModule;
    }
    else
    {
        // Nothing to do.
    }

    // source node 연결
    sNewMtrNode->srcNode = sSrcNode;

    // To Fix PR-10182
    // GROUP BY, SUM() 등에도 PRIOR Column이 존재할 수 있다.
    IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                       aQuerySet ,
                                       sNewMtrNode->srcNode )
              != IDE_SUCCESS );

    //----------------------------------
    // To Fix PR-12093
    //     - Destine Node의 모듈 정보 재구성을 위해서는
    //     - Mtr Node의 flag 정보 구성 시점 전에 이루어져야 한다.
    // Destine 노드 구성
    //----------------------------------

    //dstNode의 구성
    IDE_TEST( qtc::makeInternalColumn( aStatement,
                                       aNewTupleID,
                                       *aColumnCount,
                                       &(sNewMtrNode->dstNode) )
              != IDE_SUCCESS);

    // To Fix PR-9208
    // Destine Node는 대부분의 Source Node 정보를 유지하여야 한다.
    // 아래와 같이 몇 개의 정보만을 취할 경우, flag 정보등은 사라지게 된다.

    // sNewMtrNode->dstNode->node.arguments
    //     = sNewMtrNode->srcNode->node.arguments;
    // sNewMtrNode->dstNode->subquery
    //     = sNewMtrNode->srcNode->subquery;

    idlOS::memcpy( sNewMtrNode->dstNode,
                   sNewMtrNode->srcNode,
                   ID_SIZEOF(qtcNode) );

    // To Fix PR-9208
    // Destine Node의 필요한 정보만을 재설정한다.
    sNewMtrNode->dstNode->node.table = aNewTupleID;
    sNewMtrNode->dstNode->node.column = *aColumnCount;

    // To Fix PR-9208
    // Destine Node에서 불필요한 정보는 초기화한다.

    sNewMtrNode->dstNode->node.conversion = NULL;
    sNewMtrNode->dstNode->node.leftConversion = NULL;
    sNewMtrNode->dstNode->node.next = NULL;

    // To Fix PR-9208
    // Desinte Node는 항상 실제 값을 가진다.
    // 따라서, Indirection일 수 없다.
    sNewMtrNode->dstNode->node.lflag &= ~MTC_NODE_INDIRECT_MASK;
    sNewMtrNode->dstNode->node.lflag |= MTC_NODE_INDIRECT_FALSE;

    //----------------------------------
    // flag 세팅
    //----------------------------------

    sNewMtrNode->flag = 0;

    // Column locate 변경이 기본값이다.
    sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
    sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE;

    // Evaluation이 필요한 expression인지 확인
    // (expression이라 할지라도 view나 다른 operator의 tuple에 결과가 존재한다면
    //  이전에 이미 evaluation이 된 것이므로 다시 evaluation하지 않는다.)
    if( ( ( sFlag & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_INTERMEDIATE ) &&
        ( ( sFlag & MTC_TUPLE_PLAN_MASK ) == MTC_TUPLE_PLAN_FALSE ) &&
        ( ( sFlag & MTC_TUPLE_VIEW_MASK ) == MTC_TUPLE_VIEW_FALSE ) &&
        ( QMC_NEED_CALC( aSrcNode ) == ID_TRUE ) )
    {
        sNeedEvaluation = ID_TRUE;
    }
    else
    {
        sNeedEvaluation = ID_FALSE;
    }

    // Conversion이 필요한지 확인
    if( ( aConverted == ID_TRUE ) &&
        ( aSrcNode->node.conversion != NULL ) )
    {
        sNeedConversion = ID_TRUE;
    }
    else
    {
        sNeedConversion = ID_FALSE;
        sSrcNode->node.conversion = NULL;
    }

    // 다음 조건중 한가지라도 해당하면 반드시 calculate함수를 수행한다.
    // 1. Evaluation이 필요한 expression인 경우
    // 2. Conversion이 필요한 경우
    if( ( sNeedEvaluation == ID_TRUE ) ||
        ( sNeedConversion == ID_TRUE ) )
    {
        if( ( aSrcNode->node.lflag & MTC_NODE_INDIRECT_MASK ) == MTC_NODE_INDIRECT_FALSE )
        {
            // Expression인 경우 calculate한다.
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE;

            aSrcNode->node.lflag &= ~MTC_NODE_COLUMN_LOCATE_CHANGE_MASK;
            aSrcNode->node.lflag |= MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE;
        }
        else
        {
            // Calculate 후 copy를 해야하는 경우
            // (Pass node, subquery 등은 결과를 stack으로부터 얻음)
            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
            sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE;

            if( aSrcNode->node.module == &qtc::subqueryModule )
            {
                // Subquery의 결과가 materialize된 이후부터는 일반 column과 동일하게 접근한다.
                aSrcNode->node.module = &qtc::valueModule;
                aSrcNode->node.lflag &= ~MTC_NODE_INDIRECT_MASK;
                aSrcNode->node.lflag |= MTC_NODE_INDIRECT_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        switch ( sFlag & MTC_TUPLE_TYPE_MASK )
        {
            case MTC_TUPLE_TYPE_CONSTANT:
            case MTC_TUPLE_TYPE_VARIABLE:
            case MTC_TUPLE_TYPE_INTERMEDIATE:
                if( ( sFlag & MTC_TUPLE_PLAN_MTR_MASK ) == MTC_TUPLE_PLAN_MTR_FALSE )
                {
                    // Temp table이 아닌 경우 무조건 복사한다.
                    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                    sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

                    // BUG-28212 참조
                    if( isDatePseudoColumn( aStatement, sSrcNode ) == ID_TRUE )
                    {
                        // SYSDATE등의 pseudo column은 materialize되더라도 위치를 변경하지 않는다.
                        sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
                        sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    break;
                }
                else
                {
                    // Nothing to do.
                    // Temp table인 경우 table의 종류에 따라 materialize 방법을 설정한다.
                }
                /* fall through */
            case MTC_TUPLE_TYPE_TABLE:
                /* PROJ-2464 hybrid partitioned table 지원 */
                if ( ( sFlag & MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK ) == MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                    sNewMtrNode->flag |= QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN;
                }
                else
                {
                    if ( ( sFlag & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_MEMORY )
                    {
                        // Src가 memory table일 때
                        if ( ( ( sFlag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
                               == MTC_TUPLE_PARTITIONED_TABLE_TRUE ) ||
                             ( ( sFlag & MTC_TUPLE_PARTITION_MASK )
                               == MTC_TUPLE_PARTITION_TRUE ) )
                        {
                            // BUG-39896
                            // key column이 partitioned table의 컬럼인 경우
                            // row pointer뿐만아니라 column정보까지 필요하기 때문에
                            // 별도의 mtrNode를 정의하여 처리한다.
                            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                            sNewMtrNode->flag |= QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN;
                        }
                        else
                        {
                            sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                            sNewMtrNode->flag |= QMC_MTR_TYPE_MEMORY_KEY_COLUMN;
                        }

                        if ( ( sMtcTemplate->rows[sNewMtrNode->dstNode->node.table].lflag & MTC_TUPLE_STORAGE_MASK )
                             == MTC_TUPLE_STORAGE_MEMORY )
                        {
                            // Dst가 memory table인 경우, pointer만 복사한다.
                            sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
                            sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_FALSE;
                        }
                        else
                        {
                            // Dst가 disk table인 경우, 내용을 복사한다.
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Src가 disk table인 경우, 내용을 복사한다.
                        sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                        sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

                        if ( ( aFlag & QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_MASK )
                             == QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_TRUE )
                        {
                            sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
                            sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_FALSE;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                }
                break;
        }
    }

    sNewMtrNode->next = NULL;
    sNewMtrNode->myDist = NULL;
    sNewMtrNode->bucketCnt = 0;

    // To Fix PR-12093
    // Destine Node를 사용하여 mtcColumgn의 Count를 구하는 것이 원칙에 맞음
    // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
    //     - Memory 공간 낭비
    //     - offset 조정 오류 (PR-12093)
    sModule = sNewMtrNode->dstNode->node.module;
    (*aColumnCount) += ( sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK );

    // Argument들의 위치가 더 이상 변경되지 않도록 설정한다.
    for( sArgs = aSrcNode->node.arguments;
         sArgs != NULL;
         sArgs = sArgs->next )
    {
        // BUG-37355
        // argument node tree에 존재하는 passNode가 qtcNode를 공유하는 경우
        // flag를 설정하더라도 column locate가 변경되므로
        // 이를 복사생성하여 독립시킨다.
        IDE_TEST( isolatePassNode( aStatement, (qtcNode*) sArgs )
                  != IDE_SUCCESS );
        
        sArgs->lflag &= ~MTC_NODE_COLUMN_LOCATE_CHANGE_MASK;
        sArgs->lflag |= MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE;
    }

    if( ( sNewMtrNode->flag & QMC_MTR_CHANGE_COLUMN_LOCATE_MASK )
        == QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE )
    {
        // 상위에서 temp table의 값을 참조하도록 변경된 위치를 설정한다.
        aSrcNode->node.table  = sNewMtrNode->dstNode->node.table;
        aSrcNode->node.column = sNewMtrNode->dstNode->node.column;

        if( ( aSrcNode->node.conversion != NULL ) || ( sSrcNode->node.conversion != NULL ) )
        {
            // Conversion의 결과를 materialize하는 경우 위치 변경을 취소한다.
            // 추후 qmg::chagneColumnLocate 호출 시 conversion하지 않은 column 참조시에도
            // conversion된 결과를 참조할 수 있기 때문이다.
            // Ex) SELECT /*+USE_SORT(t1, t2)*/ * FROM t1, t2 WHERE t1.c1 = t2.c2;
            //     * 이 때 t1.c1과 t2.c1의 type이 달라 conversion이 발생하는 경우
            //       PROJECTION에서는 SORT의 t1.c1이나 t2.c2가 아닌 SCAN의 결과를
            //       참조해야 한다.
            sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
            sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-2362 memory temp 저장 효율성 개선
    // aggregation이 아니고, variable type이며, memory temp를 사용하는 경우
    if( ( QTC_IS_AGGREGATE( sNewMtrNode->srcNode ) != ID_TRUE )
        &&
        ( ( ( sFlag & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_TABLE ) ||
          ( ( sFlag & MTC_TUPLE_VIEW_MASK ) == MTC_TUPLE_VIEW_TRUE )  ||
          ( isTempTable( sFlag ) == ID_TRUE ) )
        &&
        ( ( QTC_STMT_TUPLE( aStatement, sNewMtrNode->dstNode )->lflag
            & MTC_TUPLE_STORAGE_MASK )
          == MTC_TUPLE_STORAGE_MEMORY )
        &&
        ( QCU_REDUCE_TEMP_MEMORY_ENABLE == 1 ) )
    {
        if ( QTC_STMT_TUPLE( aStatement, sNewMtrNode->srcNode )->columns != NULL )
        {
            if ( QTC_STMT_COLUMN( aStatement, sNewMtrNode->srcNode )->module->id
                 != MTD_LIST_ID )
            {
                sNewMtrNode->flag &= ~QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK;
                sNewMtrNode->flag |= QMC_MTR_TEMP_VAR_TYPE_ENABLE_TRUE;
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
        // Nothing to do.
    }
    
    // BUG-34341
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_USE_TEMP_TYPE );

    /* PROJ-2462 Result Cache */
    if ( ( aSrcNode->lflag & QTC_NODE_LOB_COLUMN_MASK )
         == QTC_NODE_LOB_COLUMN_EXIST )
    {
        sNewMtrNode->flag &= ~QMC_MTR_LOB_EXIST_MASK;
        sNewMtrNode->flag |= QMC_MTR_LOB_EXIST_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-2462 Result Cache */
    if ( ( aSrcNode->lflag & QTC_NODE_PRIOR_MASK )
         == QTC_NODE_PRIOR_EXIST )
    {
        sNewMtrNode->flag &= ~QMC_MTR_PRIOR_EXIST_MASK;
        sNewMtrNode->flag |= QMC_MTR_PRIOR_EXIST_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    *aMtrNode = sNewMtrNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::makeBaseTableMtrNode( qcStatement * aStatement ,
                           qtcNode     * aSrcNode ,
                           UShort        aDstTupleID ,
                           UShort      * aColumnCount ,
                           qmcMtrNode ** aMtrNode )
{
    qmcMtrNode  * sNewMtrNode;
    ULong         sFlag;
    idBool        sIsRecoverRid = ID_FALSE;
    UShort        sSrcTupleID   = 0;

    IDU_FIT_POINT_FATAL( "qmg::makeBaseTableMtrNode::__FT__" );

    sSrcTupleID = aSrcNode->node.table;
    sFlag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sSrcTupleID].lflag;

    if( ( sFlag & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_TABLE )
    {
        // PROJ-1789 PROWID
        // need to recover rid when setTuple
        if( (sFlag & MTC_TUPLE_TARGET_RID_MASK) ==
            MTC_TUPLE_TARGET_RID_EXIST)
        {
            sIsRecoverRid = ID_TRUE;
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                           qmcMtrNode,
                           &sNewMtrNode) != IDE_SUCCESS);

    IDE_TEST( qtc::makeInternalColumn( aStatement,
                                       sSrcTupleID,
                                       0,
                                       &(sNewMtrNode->srcNode) )
              != IDE_SUCCESS );

    sNewMtrNode->flag = 0;
    sNewMtrNode->next = NULL;
    sNewMtrNode->myDist = NULL;
    sNewMtrNode->bucketCnt = 0;

    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
    sNewMtrNode->flag |= (getBaseTableType( sFlag ) & QMC_MTR_TYPE_MASK);

    // PROJ-2362 memory temp 저장 효율성 개선
    // baseTable을 위한 mtrNode임을 기록한다.
    sNewMtrNode->flag &= ~QMC_MTR_BASETABLE_MASK;
    sNewMtrNode->flag |= QMC_MTR_BASETABLE_TRUE;
    
    if( isTempTable( sFlag ) == ID_TRUE )
    {
        sNewMtrNode->flag &= ~QMC_MTR_BASETABLE_TYPE_MASK;
        sNewMtrNode->flag |= QMC_MTR_BASETABLE_TYPE_DISKTEMPTABLE;
    }
    else
    {
        sNewMtrNode->flag &= ~QMC_MTR_BASETABLE_TYPE_MASK;
        sNewMtrNode->flag |= QMC_MTR_BASETABLE_TYPE_DISKTABLE;
    }

    /* BUG-39830 */
    if ((sFlag & MTC_TUPLE_STORAGE_MASK) == MTC_TUPLE_STORAGE_LOCATION_REMOTE)
    {
        sNewMtrNode->flag &= ~QMC_MTR_REMOTE_TABLE_MASK;
        sNewMtrNode->flag |= QMC_MTR_REMOTE_TABLE_TRUE;
    }
    else
    {
        sNewMtrNode->flag &= ~QMC_MTR_REMOTE_TABLE_MASK;
        sNewMtrNode->flag |= QMC_MTR_REMOTE_TABLE_FALSE;
    }

    IDE_TEST( qtc::makeInternalColumn( aStatement,
                                       aDstTupleID,
                                       *aColumnCount,
                                       &(sNewMtrNode->dstNode) )
              != IDE_SUCCESS );

    (*aColumnCount)++;

    if (sIsRecoverRid == ID_TRUE)
    {
        /*
         * PROJ-1789 PROWID
         *
         * SELECT _PROWID FROM T1 ORDER BY c1;
         * Pointer or RID 방식이고 select target 에 RID 가 있는 경우
         * 나중에 setTupleXX 과정에서 rid 까지 복구하도록
         *
         * 사실상 recover 가 필요한 경우는 memory table 에서
         * base table ptr 를 구성할때이다.
         * 이때 항상 alCoccount = 1 이므로 sFirstMtrNode 에만 처리하였다.
         */
        sNewMtrNode->flag &= ~QMC_MTR_RECOVER_RID_MASK;
        sNewMtrNode->flag |= QMC_MTR_RECOVER_RID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-2462 Result Cache */
    if ( ( aSrcNode->lflag & QTC_NODE_LOB_COLUMN_MASK )
         == QTC_NODE_LOB_COLUMN_EXIST )
    {
        sNewMtrNode->flag &= ~QMC_MTR_LOB_EXIST_MASK;
        sNewMtrNode->flag |= QMC_MTR_LOB_EXIST_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    *aMtrNode = sNewMtrNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::setDisplayInfo( qmsFrom          * aFrom ,
                     qmsNamePosition  * aTableOwnerName ,
                     qmsNamePosition  * aTableName ,
                     qmsNamePosition  * aAliasName )
{
/***********************************************************************
 *
 * Description : display 정보를 세팅한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcNamePosition     sTableOwnerNamePos;
    qcNamePosition     sTableNamePos;
    qcNamePosition     sAliasNamePos;

    IDU_FIT_POINT_FATAL( "qmg::setDisplayInfo::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aFrom != NULL );

    sTableOwnerNamePos = aFrom->tableRef->userName;
    sTableNamePos = aFrom->tableRef->tableName;
    sAliasNamePos = aFrom->tableRef->aliasName;

    //table owner name 세팅
    if ( ( aFrom->tableRef->tableInfo->tableType == QCM_FIXED_TABLE ) ||
         ( aFrom->tableRef->tableInfo->tableType == QCM_DUMP_TABLE ) ||
         ( aFrom->tableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW ) )
    {
        // performance view는 tableInfo가 없다.
        aTableOwnerName->name = NULL;
        aTableOwnerName->size = QC_POS_EMPTY_SIZE;
    }
    else
    {
        aTableOwnerName->name = sTableOwnerNamePos.stmtText + sTableOwnerNamePos.offset;
        aTableOwnerName->size = sTableOwnerNamePos.size;
    }

    if ( ( sTableNamePos.stmtText != NULL ) && ( sTableNamePos.size > 0 ) )
    {
        //table name 세팅
        aTableName->name = sTableNamePos.stmtText + sTableNamePos.offset;
        aTableName->size = sTableNamePos.size;
    }
    else
    {
        aTableName->name = NULL;
        aTableName->size = QC_POS_EMPTY_SIZE;
    }


    //table name 과 alias name이 같은 경우
    if( sTableNamePos.offset == sAliasNamePos.offset )
    {
        aAliasName->name = NULL;
        aAliasName->size = QC_POS_EMPTY_SIZE;
    }
    else
    {
        aAliasName->name = sAliasNamePos.stmtText + sAliasNamePos.offset;
        aAliasName->size = sAliasNamePos.size;
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::copyMtcColumnExecute( qcStatement      * aStatement ,
                           qmcMtrNode       * aMtrNode )
{
/***********************************************************************
 *
 * Description : mtcColumn정보와 Execute정보를 복사한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMtrNode       * sMtrNode;
    mtcNode          * sConvertedNode;

    UShort             sTable1;
    UShort             sColumn1;
    UShort             sTable2;
    UShort             sColumn2;

    UShort             sColumnCount;
    UShort             i;

    IDU_FIT_POINT_FATAL( "qmg::copyMtcColumnExecute::__FT__" );
    
    for( sMtrNode = aMtrNode ; sMtrNode != NULL ; sMtrNode = sMtrNode->next )
    {
        // To Fix PR-12093
        // Destine Node를 사용하여 mtcColumn의 Count를 구하는 것이 원칙에 맞음
        // 사용하지도 않을 mtcColumn 정보를 유지하는 것은 불합리함.
        //     - Memory 공간 낭비
        //     - offset 조정 오류 (PR-12093)

        sColumnCount = sMtrNode->dstNode->node.module->lflag &
            MTC_NODE_COLUMN_COUNT_MASK;

        // fix BUG-20659
        if( ( sMtrNode->srcNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_AGGREGATION )
        {
            sConvertedNode = (mtcNode*)&(sMtrNode->srcNode->node);
        }
        else
        {
            sConvertedNode =
                mtf::convertedNode( &( sMtrNode->srcNode->node ),
                                    &( QC_SHARED_TMPLATE(aStatement)->tmplate ) );
        }

        for ( i = 0; i < sColumnCount ; i++ )
        {
            sTable1 = sMtrNode->dstNode->node.table;
            sColumn1 = sMtrNode->dstNode->node.column + i;

            //BUG-8785
            //Aggregation의 처음 result에는 conversion이 달릴수있다.
            if( i == 0 )
            {
                sTable2 = sConvertedNode->table;
                sColumn2 = sConvertedNode->column;
            }
            else
            {
                sTable2 = sMtrNode->srcNode->node.table;
                sColumn2 = sMtrNode->srcNode->node.column + i;
            }

            if (sColumn2 != MTC_RID_COLUMN_ID)
            {
                idlOS::memcpy(&(QC_SHARED_TMPLATE(aStatement)->
                                tmplate.rows[sTable1].columns[sColumn1]) ,
                              &(QC_SHARED_TMPLATE(aStatement)->
                                tmplate.rows[sTable2].columns[sColumn2]) ,
                              ID_SIZEOF(mtcColumn));

                idlOS::memcpy(&(QC_SHARED_TMPLATE(aStatement)->
                                tmplate.rows[sTable1].execute[sColumn1]) ,
                              &(QC_SHARED_TMPLATE(aStatement)->
                                tmplate.rows[sTable2].execute[sColumn2]) ,
                              ID_SIZEOF(mtcExecute));

                /* BUG-44047
                 * CLOB을 포함한 테이블과 HASH 조인 힌트를 사용하면, 에러가 발생
                 * Intermediate Tuple에서 Pointer 만 쌓을 경우도 첫 번째 Table의
                 * Column Size 만큼 할당하도록 되어있다.
                 * 이 버그는 일단 CLob 인 경우만 column size를 조정한다.
                 * 본래 partition pointer size가 26 정도 mtdBigintType * 5 = 40
                 * 정도면 충분하리라 본다. execute에 다시 조정이 일어나므로 딱
                 * 마출 필요는 없다.
                 */
                if ( (( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable1].columns[sColumn1].module == &mtdClob ) ||
                      ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable1].columns[sColumn1].module == &mtdBlob )) &&
                     ( ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_MEMORY_TABLE ) ||
                       ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_DISK_TABLE ) ||
                       ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE ) ||
                       ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_DISK_PARTITIONED_TABLE ) ||
                       ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE ) ) )
                {
                    QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable1].
                        columns[sColumn1].column.size = ID_SIZEOF(mtdBigintType) * 5;

                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                idlOS::memcpy(&(QC_SHARED_TMPLATE(aStatement)->
                                tmplate.rows[sTable1].columns[sColumn1]),
                              &gQtcRidColumn,
                              ID_SIZEOF(mtcColumn));

                idlOS::memcpy(&(QC_SHARED_TMPLATE(aStatement)->
                                tmplate.rows[sTable1].execute[sColumn1]),
                              &gQtcRidExecute,
                              ID_SIZEOF(mtcExecute));
            }

            //--------------------------------------------------------
            // BUG-21020
            // Ex)  SELECT DISTINCT A.S1,
            //            (SELECT S1(Variable Column) FROM T2) S4,
            //            A.S2,
            //            A.S3
            //      FROM T1 A
            //      ORDER BY A.S1;
            // 위와 같이 SubQuery안에 Variable Column이 있을 경우, Fixed로 변경해 줘야한다.
            // SubQuery가 아닌 다른 Column들은 하위에서 처리되지만 Subquery에 대한 처리가 없다.
            // SubQuery안의 Column일 Variable 일 경우, Fixed로 변경하는 처리가 없다.
            // 만약 이 처리가 없다면, SORT시 Fixed로 변경된 Column이 Variable Column으로
            // 인식되고 서버 비정상 종료를 야기한다.
            // 이 코드가 문제가 있을시 Test는 반드시 4가지 조건을 모두 만족하도록 하여야 한다.
            // 첫째, Memory Variable Column이 Memory Temptable로 저장될 때,
            // 둘째, Memory Variable Column이 Disk Temptable에 저장될 때,
            // 셋째, Disk Variable Column이 Memory Temptable로 저장될 때,
            // 넷째, Disk Variable column이 Disk Temptable로 저장될때.
            // Subquery에 대한 FIXED 변경 코드가 없을 경우, 둘째와 넷째 조건에서 비정상 종료한다.
            //--------------------------------------------------------
            if ( QTC_IS_SUBQUERY( sMtrNode->srcNode ) == ID_TRUE )
            {
                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag &=
                    ~SMI_COLUMN_TYPE_MASK;

                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag |=
                    SMI_COLUMN_TYPE_FIXED;        

                // BUG-38494
                // Compressed Column 역시 값 자체가 저장되므로
                // Compressed 속성을 삭제한다
                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag &=
                    ~SMI_COLUMN_COMPRESSION_MASK;

                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag |=
                    SMI_COLUMN_COMPRESSION_FALSE;        
            }

            // fix BUG-9494
            // memory variable column이 disk temp table에 저장될때는
            // pointer가 아닌 값이 저장된다.
            // 따라서, 이 경우의 smiColumn.flag를 variable에서 fixed로
            // 변경시켜주어야 한다.
            // ex) SELECT /*+ TEMP_TBS_DISK */ DISTINCT *
            //     FROM M1 ORDER BY M1.I2(variable column);
            // 아래와 같이 HSDS부터는 disk temp table에 저장되며
            // variable column은 pointer가 아닌 값이 저장된다.
            // 이때, HSDS에서 이 variable column을 fixed로 변경하지 않으면,
            // SORT에서는 disk variable column으로 인식해서
            // disk variable column header로 부터 값을 찾으려고 시도하게 되므로
            // 서버가 비정상종료하게 됨.
            // HSDS에서 variable column을 fixed로 변경함으로써,
            // SORT에서는 disk fixed column으로 인식해서,
            // 저장된 값을 참조하게 되어, 올바른 질의결과를 수행하게 됨.
            //     [PROJ]
            //       |
            //     [SORT] -- disk temp table
            //       |
            //     [HSDS] --  disk temp table
            //       |
            //     [SCAN] -- memory table
            //     
            
            if( ( ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK )
                    == QMC_MTR_TYPE_MEMORY_KEY_COLUMN ) ||
                  ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK )
                    == QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN ) ||
                  ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK )
                    == QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN ) ) /* PROJ-2464 hybrid partitioned table 지원 */
                &&
                ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTable1].lflag
                    & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_DISK ) )
            {
                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag &=
                    ~SMI_COLUMN_TYPE_MASK;

                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag |=
                    SMI_COLUMN_TYPE_FIXED;

                // BUG-38494
                // Compressed Column 역시 값 자체가 저장되므로
                // Compressed 속성을 삭제한다
                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag &=
                    ~SMI_COLUMN_COMPRESSION_MASK;

                QC_SHARED_TMPLATE(aStatement)->tmplate.
                    rows[sTable1].columns[sColumn1].column.flag |=
                    SMI_COLUMN_COMPRESSION_FALSE;        
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::setCalcLocate( qcStatement * aStatement,
                    qmcMtrNode  * aMtrNode )
{
/***********************************************************************
 *
 * Description :
 *    Calculation만으로 materialization이 완료되는 node들에 대해
 *    src node의 수행 위치를 dest node로 변경해준다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMtrNode * sMtrNode;
    mtcNode    * sConverted;

    IDU_FIT_POINT_FATAL( "qmg::setCalcLocate::__FT__" );

    for (sMtrNode = aMtrNode;
         sMtrNode != NULL;
         sMtrNode = sMtrNode->next )
    {
        if( ( sMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_CALCULATE )
        {
            sConverted = mtf::convertedNode( &sMtrNode->srcNode->node,
                                             &QC_SHARED_TMPLATE( aStatement )->tmplate );
            sConverted->table  = sMtrNode->dstNode->node.table;
            sConverted->column = sMtrNode->dstNode->node.column;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::checkOrderInGraph( qcStatement        * aStatement,
                        qmgGraph           * aGraph,
                        qmgPreservedOrder  * aWantOrder,
                        qmoAccessMethod   ** aOriginalMethod,
                        qmoAccessMethod   ** aSelectMethod,
                        idBool             * aUsable )
{
/***********************************************************************
 *
 * Description :
 *
 *    입력된 Order가 해당 Graph내에서 사용 가능한 지를 검사
 *
 * Implementation :
 *
 *    Graph가 Preserved Order가 정의되지 않은 경우
 *        - 해당 Graph의 종류에 따라 처리
 *            - 일반 Table의 Selection Graph인 경우
 *                - Index 사용 가능 여부 검사
 *            - View의 Selection Graph인 경우
 *                - Child Graph에 대한 Order 형태로 변경 후
 *                - Child Graph를 이용한 처리
 *            - Set, Hierarchy, Dnf Graph인 경우
 *                - 처리할 수 없으며,
 *                - Preserved Order가 결코 존재할 수없다.
 *            - 이 외의 Graph인 경우
 *                - Child Graph를 이용한 처리
 *    Graph가 Preserved Order를 가지고 있는 경우
 *        - 원하는 Preserved Order와 Graph Preserved Order를 검사.
 *    Graph가 NEVER Defined인 경우
 *        - 원하는 Order를 처리할 수 없음.
 *
 ***********************************************************************/

    idBool            sUsable = ID_FALSE;
    idBool            sOrderImportant;
    qmoAccessMethod * sOrgMethod;
    qmoAccessMethod * sSelMethod;

    qmgDirectionType  sPrevDirection;
    qmsParseTree    * sParseTree;

    qmgPreservedOrder * sWantOrder;
    qmgPreservedOrder * sPreservedOrder;

    UInt              i;
    UInt              sOrderCnt;
    UInt              sDummy;
    UInt              sJoinMethod;

    IDU_FIT_POINT_FATAL( "qmg::checkOrderInGraph::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aWantOrder != NULL );
    IDE_FT_ASSERT( aGraph != NULL );
    IDE_FT_ASSERT( aUsable != NULL );
    IDE_FT_ASSERT( aOriginalMethod != NULL );
    IDE_FT_ASSERT( aSelectMethod != NULL );

    sOrgMethod = NULL;
    sSelMethod = NULL;

    // Order가 중요한지를 판단.
    // BUG-40361 supporting to indexable analytic function
    // aWantOrder의 모든 Node가 NOT DEFINED 인지 확인한다
    sOrderImportant = ID_FALSE;
    for ( sWantOrder = aWantOrder; sWantOrder != NULL; sWantOrder = sWantOrder->next )
    {
        if ( sWantOrder->direction != QMG_DIRECTION_NOT_DEFINED )
        {
            sOrderImportant = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    //---------------------------------------------------
    // Graph의 정보에 따른 검사
    //---------------------------------------------------

    // Graph가 가진 Preserved Order에 따른 검사
    switch ( aGraph->flag & QMG_PRESERVED_ORDER_MASK )
    {
        case QMG_PRESERVED_ORDER_DEFINED_NOT_FIXED :
            //---------------------------------------------------
            // Graph가 Preserved Order가 존재하나 고정되지 않은 경우
            // 원하는 order로 변경 가능한 경우
            //---------------------------------------------------

            if ( aGraph->type == QMG_WINDOWING )
            {
                IDE_TEST( qmgWindowing::existWantOrderInSortingKey( aGraph,
                                                                    aWantOrder,
                                                                    & sUsable,
                                                                    & sDummy )
                          != IDE_SUCCESS );
            }
            else
            {
                // Child Graph로부터 Order를 사용 가능한지 검사.
                IDE_TEST(
                    checkUsableOrder( aStatement,
                                      aWantOrder,
                                      aGraph->left,
                                      & sOrgMethod,
                                      & sSelMethod,
                                      & sUsable )
                    != IDE_SUCCESS );
            }
            break;
            
        case QMG_PRESERVED_ORDER_NOT_DEFINED :

            //---------------------------------------------------
            // Graph의 Preserved Order가 정의되지 않는 경우
            //---------------------------------------------------

            // Graph 종류에 따른 판단
            switch ( aGraph->type )
            {
                case QMG_SELECTION :
                    if ( aGraph->myFrom->tableRef->view == NULL )
                    {
                        // 일반 Table에 대한 Selection인 경우

                        // 원하는 Order에 대응하는 Index가 존재하는 지 검사
                        IDE_TEST( checkUsableIndex4Selection( aGraph,
                                                              aWantOrder,
                                                              sOrderImportant,
                                                              & sOrgMethod,
                                                              & sSelMethod,
                                                              & sUsable )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // View 에 대한 Selection인 경우
                        // Ex) SELECT * FROM ( SELECT i1, i2 FROM T1 ) V1
                        //     ORDER BY V1.i1;

                        sParseTree = (qmsParseTree *)
                            aGraph->myFrom->tableRef->view->myPlan->parseTree;

                        // 원하는 Order의 ID를
                        // View의 Target에 부합하는 ID로 변경
                        IDE_TEST(
                            refineID4Target( aWantOrder,
                                             sParseTree->querySet->target )
                            != IDE_SUCCESS );

                        // Child Graph로부터 Order를 사용 가능한지 검사.
                        IDE_TEST(
                            checkUsableOrder( aGraph->myFrom->tableRef->view,
                                              aWantOrder,
                                              aGraph->left,
                                              & sOrgMethod,
                                              & sSelMethod,
                                              & sUsable )
                            != IDE_SUCCESS );
                    }

                    break;
                case QMG_PARTITION :
                    // 원하는 Order에 대응하는 Index가 존재하는 지 검사
                    IDE_TEST( checkUsableIndex4Partition( aGraph,
                                                          aWantOrder,
                                                          sOrderImportant,
                                                          & sOrgMethod,
                                                          & sSelMethod,
                                                          & sUsable )
                              != IDE_SUCCESS );
                    break;

                case QMG_WINDOWING :
                    // BUG-35001
                    // disk temp table 을 사용하는 경우에는
                    // insert 순서와 fetch 하는 순서가 서로 다를수 있다.
                    if( ( aGraph->flag & QMG_GRAPH_TYPE_MASK )
                        == QMG_GRAPH_TYPE_MEMORY )
                    {
                        // Child Graph로부터 Order를 사용 가능한지 검사.
                        IDE_TEST(
                            checkUsableOrder( aStatement,
                                              aWantOrder,
                                              aGraph->left,
                                              & sOrgMethod,
                                              & sSelMethod,
                                              & sUsable )
                            != IDE_SUCCESS );
                    }
                    else
                    {
                        // nothing todo.
                    }
                    break;

                case QMG_SET :
                case QMG_HIERARCHY :
                case QMG_DNF :
                case QMG_SHARD_SELECT:    // PROJ-2638

                    // Set인 경우에는 Index를 사용한다 하더라도
                    // 올바른 결과를 보장할 수 없다.
                    // Ex) SELECT * FROM ( SELECT i1, i2 FROM T1
                    //                     UNOIN ALL
                    //                     SELECT i1, i2 FROM T2 ) V1
                    //     ORDER BY V1.i1;
                    // Ex) SELECT DISTINCT V1.i1
                    //       FROM ( SELECT i1, i2 FROM T1
                    //              UNOIN ALL
                    //              SELECT i1, i2 FROM T2 ) V1;

                    // NOT_DEFINED일 수 없음.
                    IDE_DASSERT(0);

                    sUsable = ID_FALSE;

                    break;

                default :

                    // Child Graph로부터 Order를 사용 가능한지 검사.
                    IDE_TEST(
                        checkUsableOrder( aStatement,
                                          aWantOrder,
                                          aGraph->left,
                                          & sOrgMethod,
                                          & sSelMethod,
                                          & sUsable )
                        != IDE_SUCCESS );
                    break;
            }

            break;

        case QMG_PRESERVED_ORDER_DEFINED_FIXED :

            //---------------------------------------------------
            // Graph가 Preserved Order를 가지는 경우
            //---------------------------------------------------

            IDE_DASSERT( aGraph->preservedOrder != NULL );

            if ( sOrderImportant == ID_TRUE )
            {
                //------------------------------------
                // Order가 중요한 경우
                //------------------------------------

                // Preserved Order와 Order의 순서 및 방향이 일치해야 함
                // Ex) Preserved Order i1(asc) -> i2(asc) -> i3(asc)
                //     ORDER BY i1, i2 :  O
                //     ORDER BY i1, i2, i3 desc : X
                //     ORDER BY i3, i2, i1 :  X
                //     ORDER BY i1, i3 : X
                //     ORDER BY i2, i3 : X
                //     ORDER BY i1, i2, i3, i4 : X

                sPrevDirection = QMG_DIRECTION_NOT_DEFINED;

                for ( sWantOrder = aWantOrder,
                          sPreservedOrder = aGraph->preservedOrder;
                      sWantOrder != NULL && sPreservedOrder != NULL;
                      sWantOrder = sWantOrder->next,
                          sPreservedOrder = sPreservedOrder->next )
                {
                    if ( sWantOrder->table == sPreservedOrder->table &&
                         sWantOrder->column == sPreservedOrder->column )
                    {
                        // 동일한 Column에 대한 Preserved Order임
                        /* BUG-42145
                         * Table이나 Partition에 Index가 있는 경우라면
                         * Nulls Option 이 고려된 Direction Check를 한다.
                         */
                        if ( ( ( aGraph->type == QMG_SELECTION ) ||
                               ( aGraph->type == QMG_PARTITION ) ) &&
                             ( aGraph->myFrom->tableRef->view == NULL ) )
                        {
                            IDE_TEST( checkSameDirection4Index( sWantOrder,
                                                                sPreservedOrder,
                                                                sPrevDirection,
                                                                & sPrevDirection,
                                                                & sUsable )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // 방향성 검사
                            IDE_TEST( checkSameDirection( sWantOrder,
                                                          sPreservedOrder,
                                                          sPrevDirection,
                                                          & sPrevDirection,
                                                          & sUsable )
                                      != IDE_SUCCESS );
                        }

                        if ( sUsable == ID_FALSE )
                        {
                            // 방향성이 달라 Preserved Order를
                            // 사용할 수 없음.
                            break;
                        }
                        else
                        {
                            // Go Go
                        }
                    }
                    else
                    {
                        // 서로 다른 Column에 대한 Order를 가지고 있음.
                        break;
                    }
                }

                if ( sWantOrder != NULL )
                {
                    // 원하는 Order를 모두 만족시키지 않음
                    sUsable = ID_FALSE;
                }
                else
                {
                    // 원하는 Order를 모두 만족시킴
                    sUsable = ID_TRUE;
                }
            }
            else
            {
                //------------------------------------
                // Order가 중요하지 않은 경우
                //------------------------------------

                // Preserved Order의 순서에 해당하는 Order가 존재해야 함.
                // Ex) Preserved Order : i1 -> i2 -> i3 일때,
                //     DISTINCT i1, i2 :  O
                //     DISTINCT i3, i2, i1 :  O
                //     DISTINCT i1, i3 : X
                //     DISTINCT i2, i3 : X
                //     DISTINCT i1, i2, i3, i4 : X

                // Order의 개수 계산
                for ( sWantOrder = aWantOrder, sOrderCnt = 0;
                      sWantOrder != NULL;
                      sWantOrder = sWantOrder->next, sOrderCnt++ ) ;

                for ( sPreservedOrder = aGraph->preservedOrder, i = 0;
                      sPreservedOrder != NULL;
                      sPreservedOrder = sPreservedOrder->next, i++ )
                {
                    for ( sWantOrder = aWantOrder;
                          sWantOrder != NULL;
                          sWantOrder = sWantOrder->next )
                    {
                        if ( sWantOrder->table == sPreservedOrder->table &&
                             sWantOrder->column == sPreservedOrder->column )
                        {
                            // Order가 존재함.
                            break;
                        }
                        else
                        {
                            // Nothing To Do
                        }
                    }

                    if ( sWantOrder != NULL )
                    {
                        // 원하는 Order를 Preserved Order에서 찾은 경우
                        // Nothing To Do
                    }
                    else
                    {
                        // 원하는 Order가 Preserved Order에 존재하지 않음
                        break;
                    }
                }

                if ( i == sOrderCnt )
                {
                    /* BUG-44261 roup By, View Join 를 사용시 Order 검사 버그로 비정상 종료 합니다.
                     *  - 모든 Want Order가 Preserved Order에 실제로 포함되는 지 검사합니다.
                     *     - Want Order가 Preserved Order에 존재하는 지 검사합니다.
                     *     - Want Order가 Preserved Order에 없습니다.
                     *     - 모든 Want Order가 Preserved Order에 포함됩니다.
                     */
                    for ( sWantOrder  = aWantOrder;
                          sWantOrder != NULL;
                          sWantOrder  = sWantOrder->next )
                    {
                        for ( sPreservedOrder  = aGraph->preservedOrder;
                              sPreservedOrder != NULL;
                              sPreservedOrder  = sPreservedOrder->next )
                        {
                            /* Want Order가 Preserved Order에 존재하는 지 검사합니다. */
                            if ( sWantOrder->table  == sPreservedOrder->table &&
                                 sWantOrder->column == sPreservedOrder->column )
                            {
                                break;
                            }
                            else
                            {
                                /* Nothing to do */
                            }
                        }

                        if ( sPreservedOrder == NULL )
                        {
                            /* Want Order가 Preserved Order에 없습니다. */
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }

                    if ( sPreservedOrder != NULL )
                    {
                        /* 모든 Want Order가 Preserved Order에 포함됩니다. */
                        sUsable = ID_TRUE;
                    }
                    else
                    {
                        sUsable = ID_FALSE;
                    }
                }
                else
                {
                    // 원하는 Order가 Preserved Order에 없거나
                    // 존재하더라도 Preserved Order의 순서대로
                    // 포함되지 않음

                    sUsable = ID_FALSE;
                }
            }

            if ( sUsable == ID_TRUE )
            {
                switch ( aGraph->type )
                {
                    case QMG_SELECTION :

                        if ( aGraph->myFrom->tableRef->view == NULL )
                        {
                            sOrgMethod = ((qmgSELT*) aGraph)->selectedMethod;
                            sSelMethod = ((qmgSELT*) aGraph)->selectedMethod;
                        }
                        else
                        {
                            // Nothing To Do
                        }

                        break;

                    case QMG_PARTITION :

                        sOrgMethod = ((qmgPARTITION*) aGraph)->selectedMethod;
                        sSelMethod = ((qmgPARTITION*) aGraph)->selectedMethod;

                        break;

                    default:
                        // Nothing To Do
                        break;
                }
            }
            else
            {
                qcgPlan::registerPlanProperty(
                    aStatement,
                    PLAN_PROPERTY_OPTIMIZER_ORDER_PUSH_DOWN );

                if ( QCU_OPTIMIZER_ORDER_PUSH_DOWN != 1 )
                {
                    break;
                }
                else
                {
                    // Nothing To Do
                }

                // BUG-43068 Indexable order by 개선
                switch ( aGraph->type )
                {
                    case QMG_SELECTION :
                        if ( aGraph->myFrom->tableRef->view == NULL )
                        {
                            IDE_TEST( checkUsableIndex4Selection( aGraph,
                                                                  aWantOrder,
                                                                  sOrderImportant,
                                                                  &sOrgMethod,
                                                                  &sSelMethod,
                                                                  &sUsable )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing To Do
                        }
                        break;

                    case QMG_INNER_JOIN :
                    case QMG_LEFT_OUTER_JOIN :

                        if ( aGraph->type == QMG_INNER_JOIN )
                        {
                            sJoinMethod = (((qmgJOIN*)aGraph)->selectedJoinMethod->flag & QMO_JOIN_METHOD_MASK);
                        }
                        else
                        {
                            sJoinMethod = (((qmgLOJN*)aGraph)->selectedJoinMethod->flag & QMO_JOIN_METHOD_MASK);
                        }

                        // 일부 조인 메소드만 허용한다.
                        switch ( sJoinMethod )
                        {
                            case QMO_JOIN_METHOD_FULL_NL :
                            case QMO_JOIN_METHOD_FULL_STORE_NL :
                            case QMO_JOIN_METHOD_INDEX :
                            case QMO_JOIN_METHOD_ONE_PASS_HASH :

                                IDE_TEST( checkUsableOrder( aStatement,
                                                            aWantOrder,
                                                            aGraph->left,
                                                            &sOrgMethod,
                                                            &sSelMethod,
                                                            &sUsable )
                                          != IDE_SUCCESS );
                                break;
                            default :
                                break;
                        } // end switch
                        break;

                    default :
                        break;
                } // end switch
            }

            break;

        case QMG_PRESERVED_ORDER_NEVER :

            //---------------------------------------------------
            // Graph가 Preserved Order를 사용할 수 없는 경우
            //---------------------------------------------------

            sUsable = ID_FALSE;
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    *aOriginalMethod = sOrgMethod;
    *aSelectMethod = sSelMethod;
    *aUsable = sUsable;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
qmg::checkUsableIndex4Selection( qmgGraph          * aGraph,
                                 qmgPreservedOrder * aWantOrder,
                                 idBool              aOrderNeed,
                                 qmoAccessMethod  ** aOriginalMethod,
                                 qmoAccessMethod  ** aSelectMethod,
                                 idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *     원하는 Order에 사용할 수 있는 Index가 존재하는 지 검사.
 *
 * Implementation :
 *     Index의 Key Column과 원하는 Order가 일치하는지 검사
 *         - Order가 중요한 경우 : 정확히 일치해야 함.
 *         - Order가 중요하지 않은 경우 : 해당 Column의 존재 여부
 *     해당 Index가 존재한다 하더라도 Index를 선택했을 때의
 *     비용이 Selection 처리 후 저장 처리하는 비용보다 클 경우는
 *     Index를 선택하지 않는다.
 *
 ***********************************************************************/

    UInt    i;
    idBool  sUsable;
    UShort  sTableID;

    qmgSELT         * sSELTgraph;

    qmoAccessMethod * sMethod = NULL;
    qmoAccessMethod * sSelectedMethod = NULL;

    IDU_FIT_POINT_FATAL( "qmg::checkUsableIndex4Selection::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aGraph->type == QMG_SELECTION );
    IDE_DASSERT( aGraph->myFrom->tableRef->view == NULL );
    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aUsable != NULL );

    //---------------------------------------------------
    // 적절한 index 검사
    //---------------------------------------------------

    // 기본 정보 추출
    sSELTgraph = (qmgSELT *) aGraph;
    sTableID = aGraph->myFrom->tableRef->table;

    sMethod = sSELTgraph->accessMethod;

    // 원하는 Order와 동일한 Index가 존재하는 지 검사.
    for ( i = 0; i < sSELTgraph->accessMethodCnt; i++ )
    {
        // fix BUG-12307
        // IN subquery keyRange로 range scan을 하는 경우,
        // order를 보장할 수 없으므로
        // access method가 in subquery를 포함하는 경우는 제외
        if ( sMethod[i].method != NULL )
        {
            if ( ( sMethod[i].method->flag & QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK )
                 == QMO_STAT_CARD_IDX_IN_SUBQUERY_FALSE )
            {
                // Index Scan인 경우

                // 사용 가능한 Index인지 검사

                if ( aOrderNeed == ID_TRUE )
                {
                    // Order가 중요한 경우로
                    // Order와 Index의 방향성이 일치하는 지를 검사

                    IDE_TEST( checkIndexOrder( & sMethod[i],
                                               sTableID,
                                               aWantOrder,
                                               & sUsable )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Order가 중요하지 않은 경우로
                    // Order에 해당하는 모든 Column이 Index내에 존재하는지 검사

                    IDE_TEST( checkIndexColumn( sMethod[i].method->index,
                                                sTableID,
                                                aWantOrder,
                                                & sUsable )
                              != IDE_SUCCESS );
                }

                if ( sUsable == ID_TRUE )
                {
                    // To Fix BUG-8336
                    // 사용 가능한 Index일 경우
                    // 해당 Index를 사용하는 것이 적합한지 검사
                    if ( sSelectedMethod == NULL )
                    {
                        sSelectedMethod = & sMethod[i];
                    }
                    else
                    {
                        if (QMO_COST_IS_LESS(sMethod[i].totalCost,
                                             sSelectedMethod->totalCost) == ID_TRUE)
                        {
                            sSelectedMethod = & sMethod[i];
                        }
                        else if (QMO_COST_IS_EQUAL(sMethod[i].totalCost,
                                                   sSelectedMethod->totalCost) == ID_TRUE)
                        {
                            // 비용이 동일하다면 Primary Index를 사용하도록 한다.
                            if ( ( sMethod[i].method->flag
                                   & QMO_STAT_CARD_IDX_PRIMARY_MASK )
                                 == QMO_STAT_CARD_IDX_PRIMARY_TRUE )
                            {
                                sSelectedMethod = & sMethod[i];
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
                    }
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
        }
        else
        {
            // Full Scan인 경우로 비교 대상이 아님
            // Nothing To Do
        }
    }

    // 선택된 Index가 존재할 경우
    // 원하는 Order를 Index로 처리할 수 있음.
    if ( sSelectedMethod != NULL )
    {
        *aSelectMethod = sSelectedMethod;
        *aUsable = ID_TRUE;
    }
    else
    {
        *aSelectMethod = NULL;
        *aUsable = ID_FALSE;
    }

    // 기존에 사용되던 Method
    *aOriginalMethod = sSELTgraph->selectedMethod;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::checkUsableIndex4Partition( qmgGraph          * aGraph,
                                 qmgPreservedOrder * aWantOrder,
                                 idBool              aOrderNeed,
                                 qmoAccessMethod  ** aOriginalMethod,
                                 qmoAccessMethod  ** aSelectMethod,
                                 idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *     원하는 Order에 사용할 수 있는 Index가 존재하는 지 검사.
 *
 * Implementation :
 *     Index의 Key Column과 원하는 Order가 일치하는지 검사
 *         - Order가 중요한 경우 : 정확히 일치해야 함.
 *         - Order가 중요하지 않은 경우 : 해당 Column의 존재 여부
 *     해당 Index가 존재한다 하더라도 Index를 선택했을 때의
 *     비용이 Selection 처리 후 저장 처리하는 비용보다 클 경우는
 *     Index를 선택하지 않는다.
 *
 ***********************************************************************/

    UInt    i;
    idBool  sUsable;
    UShort  sTableID;

    qmgPARTITION    * sPARTgraph;

    qmoAccessMethod * sMethod = NULL;
    qmoAccessMethod * sSelectedMethod = NULL;

    IDU_FIT_POINT_FATAL( "qmg::checkUsableIndex4Partition::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aGraph->type == QMG_PARTITION );
    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aUsable != NULL );

    //---------------------------------------------------
    // 적절한 index 검사
    //---------------------------------------------------

    // 기본 정보 추출
    sPARTgraph = (qmgPARTITION *) aGraph;
    sTableID = aGraph->myFrom->tableRef->table;

    sMethod = sPARTgraph->accessMethod;

    // 원하는 Order와 동일한 Index가 존재하는 지 검사.
    for ( i = 0; i < sPARTgraph->accessMethodCnt; i++ )
    {
        // fix BUG-12307
        // IN subquery keyRange로 range scan을 하는 경우,
        // order를 보장할 수 없으므로
        // access method가 in subquery를 포함하는 경우는 제외
        if ( sMethod[i].method != NULL )
        {
            // index table scan인 경우
            if ( ( sMethod[i].method->index->indexPartitionType ==
                   QCM_NONE_PARTITIONED_INDEX )
                 &&
                 ( ( sMethod[i].method->flag & QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK )
                   == QMO_STAT_CARD_IDX_IN_SUBQUERY_FALSE ) )
            {
                // 사용 가능한 Index인지 검사

                if ( aOrderNeed == ID_TRUE )
                {
                    // Order가 중요한 경우로
                    // Order와 Index의 방향성이 일치하는 지를 검사

                    IDE_TEST( checkIndexOrder( & sMethod[i],
                                               sTableID,
                                               aWantOrder,
                                               & sUsable )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Order가 중요하지 않은 경우로
                    // Order에 해당하는 모든 Column이 Index내에 존재하는지 검사

                    IDE_TEST( checkIndexColumn( sMethod[i].method->index,
                                                sTableID,
                                                aWantOrder,
                                                & sUsable )
                              != IDE_SUCCESS );
                }

                if ( sUsable == ID_TRUE )
                {
                    // To Fix BUG-8336
                    // 사용 가능한 Index일 경우
                    // 해당 Index를 사용하는 것이 적합한지 검사
                    if ( sSelectedMethod == NULL )
                    {
                        sSelectedMethod = & sMethod[i];
                    }
                    else
                    {
                        if (QMO_COST_IS_LESS(sMethod[i].totalCost,
                                             sSelectedMethod->totalCost) == ID_TRUE)
                        {
                            sSelectedMethod = & sMethod[i];
                        }
                        else if (QMO_COST_IS_EQUAL(sMethod[i].totalCost,
                                                   sSelectedMethod->totalCost) == ID_TRUE)
                        {
                            // 비용이 동일하다면 Primary Index를 사용하도록 한다.
                            if ( ( sMethod[i].method->flag
                                   & QMO_STAT_CARD_IDX_PRIMARY_MASK )
                                 == QMO_STAT_CARD_IDX_PRIMARY_TRUE )
                            {
                                sSelectedMethod = & sMethod[i];
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
                    }
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
        }
        else
        {
            // Full Scan인 경우로 비교 대상이 아님
            // Nothing To Do
        }
    }

    // 선택된 Index가 존재할 경우
    // 원하는 Order를 Index로 처리할 수 있음.
    if ( sSelectedMethod != NULL )
    {
        *aSelectMethod = sSelectedMethod;
        *aUsable = ID_TRUE;
    }
    else
    {
        *aSelectMethod = NULL;
        *aUsable = ID_FALSE;
    }

    // 기존에 사용되던 Method
    *aOriginalMethod = sPARTgraph->selectedMethod;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::checkIndexOrder( qmoAccessMethod   * aMethod,
                      UShort              aTableID,
                      qmgPreservedOrder * aWantOrder,
                      idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *     원하는 Order와 해당 Index의 Order가 동일한 지 검사
 *
 *     원하는 Order의 방향성 및 Order가 중요한 경우에만
 *     호출하여 검사한다.
 *
 * Implementation :
 *     원하는 Order와 동일한 Order로 해당 Index를 사용가능한 지를
 *     검사한다.
 *
 *                          <방향성 검사표>
 *     -------------------------------------------------------------
 *      Index 방향  |  Column Order | 원하는 Order | 적용 가능
 *     -------------------------------------------------------------
 *      Foward       |     ASC       |    ASC        |     O
 *                   |               |------------------------------
 *                   |               |    DESC       |     X
 *                   -----------------------------------------------
 *                   |     DESC      |    ASC        |     X
 *                   |               |------------------------------
 *                   |               |    DESC       |     O
 *     -------------------------------------------------------------
 *      Backward     |     ASC       |    ASC        |     X
 *                   |               |------------------------------
 *                   |               |    DESC       |     O
 *                   -----------------------------------------------
 *                   |     DESC      |    ASC        |     O
 *                   |               |------------------------------
 *                   |               |    DESC       |     X
 *     -------------------------------------------------------------
 *
 ***********************************************************************/

    UInt   i;
    idBool sUsable;

    UShort              sColumnPosition;
    idBool              sIsForward;
    qmgPreservedOrder * sOrder;

    qcmIndex          * sIndex;

    IDU_FIT_POINT_FATAL( "qmg::checkIndexOrder::__FT__" );
    
    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aMethod != NULL );
    IDE_DASSERT( aMethod->method != NULL );
    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aUsable != NULL );

    //---------------------------------------------------
    // 해당 index가 원하는 order가 동일한지를 검사
    //---------------------------------------------------

    //----------------------------------
    // 첫번째 Column이 동일한 지 검사.
    //----------------------------------

    sIndex = aMethod->method->index;

    sColumnPosition = (UShort)(sIndex->keyColumns[0].column.id % SMI_COLUMN_ID_MAXIMUM);

    sOrder = aWantOrder;

    if ( ( aTableID == sOrder->table ) &&
         ( sColumnPosition == sOrder->column ) )
    {
        // 첫번째 Column 정보가 동일한 경우
        // 원하는 Order의 방향과 Index의 방향을 일치시킨다.

        switch ( aMethod->method->flag & QMO_STAT_CARD_IDX_HINT_MASK )
        {
            case QMO_STAT_CARD_IDX_INDEX_ASC :

                // Index에 대하여 Ascending Hint가 있는 경우

                if ( sOrder->direction == QMG_DIRECTION_ASC )
                {
                    if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                         == SMI_COLUMN_ORDER_ASCENDING )
                    {
                        sIsForward = ID_TRUE;
                    }
                    else
                    {
                        sIsForward = ID_FALSE;
                    }
                    
                    sUsable = ID_TRUE;
                }
                else
                {
                    sUsable = ID_FALSE;
                }
                break;

            case QMO_STAT_CARD_IDX_INDEX_DESC :

                // Index에 대하여 Descending Hint가 있는 경우

                if ( sOrder->direction == QMG_DIRECTION_DESC )
                {
                    if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                         == SMI_COLUMN_ORDER_ASCENDING )
                    {
                        sIsForward = ID_FALSE;
                    }
                    else
                    {
                        sIsForward = ID_TRUE;
                    }
                    sUsable = ID_TRUE;
                }
                else
                {
                    sUsable = ID_FALSE;
                }
                break;

            default :

                // 별도의 Hint 가 없는 경우

                switch ( sOrder->direction )
                {
                    case QMG_DIRECTION_ASC:

                        // 원하는 Order가 Ascending인 경우

                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sIsForward = ID_TRUE;
                        }
                        else
                        {
                            sIsForward = ID_FALSE;
                        }

                        sUsable = ID_TRUE;

                        break;

                    case QMG_DIRECTION_DESC:

                        // 원하는 Order가 Descending인 경우

                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sIsForward = ID_FALSE;
                        }
                        else
                        {
                            sIsForward = ID_TRUE;
                        }
                        sUsable = ID_TRUE;
                        break;
                    case QMG_DIRECTION_NOT_DEFINED:
                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sIsForward = ID_TRUE;
                        }
                        else
                        {
                            sIsForward = ID_FALSE;
                        }
                        sUsable = ID_TRUE;
                        break;
                    case QMG_DIRECTION_ASC_NULLS_FIRST:
                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sUsable = ID_FALSE;
                        }
                        else
                        {
                            sIsForward = ID_FALSE;
                            sUsable    = ID_TRUE;
                        }
                        break;
                    case QMG_DIRECTION_ASC_NULLS_LAST:
                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sIsForward = ID_TRUE;
                            sUsable    = ID_TRUE;
                        }
                        else
                        {
                            sUsable = ID_FALSE;
                        }
                        break;
                    case QMG_DIRECTION_DESC_NULLS_FIRST:
                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sIsForward = ID_FALSE;
                            sUsable    = ID_TRUE;
                        }
                        else
                        {
                            sUsable = ID_FALSE;
                        }
                        break;
                    case QMG_DIRECTION_DESC_NULLS_LAST:
                        if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                             == SMI_COLUMN_ORDER_ASCENDING )
                        {
                            sUsable = ID_FALSE;
                        }
                        else
                        {
                            sIsForward = ID_TRUE;
                            sUsable    = ID_TRUE;
                        }
                        break;
                    default:
                        IDE_DASSERT(0);
                        sUsable = ID_FALSE;
                        break;
                }
                break;
        } // end of switch
    }
    else
    {
        // 첫번째 Column 정보가 다름
        sUsable = ID_FALSE;
    }

    //----------------------------------
    // 두번째 이후의 Column이 동일한 지 검사.
    //----------------------------------

    if ( sUsable == ID_TRUE )
    {
        // 원하는 두 번째 Order부터 시작하여
        // Index내에 동일한 Column이 존재하고
        // 방향성을 적용할 수 있는 지 검사한다.

        for ( i = 1, sOrder = sOrder->next;
              i < sIndex->keyColCount && sOrder != NULL;
              i++, sOrder = sOrder->next )
        {
            sColumnPosition = (UShort)(sIndex->keyColumns[i].column.id % SMI_COLUMN_ID_MAXIMUM);

            if ( (sOrder->table == aTableID) &&
                 (sOrder->column == sColumnPosition) )
            {
                // 인덱스 Column의 방향성과 원하는 Order의 방향성 검사
                // 참조 : <방향성 검사표>

                if ( sIsForward == ID_TRUE )
                {
                    // Forward Index인 경우

                    switch ( sIndex->keyColsFlag[i] & SMI_COLUMN_ORDER_MASK )
                    {
                        case SMI_COLUMN_ORDER_ASCENDING :
                            switch ( sOrder->direction )
                            {
                                case QMG_DIRECTION_ASC:
                                    sUsable = ID_TRUE;
                                    break;
                                case QMG_DIRECTION_ASC_NULLS_LAST:
                                    if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                                         == SMI_COLUMN_ORDER_ASCENDING )
                                    {
                                        sUsable = ID_TRUE;
                                    }
                                    else
                                    {
                                        sUsable = ID_FALSE;
                                    }
                                    break;
                                default:
                                    sUsable = ID_FALSE;
                                    break;
                            }
                            break;
                        case SMI_COLUMN_ORDER_DESCENDING :
                            switch ( sOrder->direction )
                            {
                                case QMG_DIRECTION_DESC:
                                    sUsable = ID_TRUE;
                                    break;
                                case QMG_DIRECTION_DESC_NULLS_LAST:
                                    if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                                         == SMI_COLUMN_ORDER_ASCENDING )
                                    {
                                        sUsable = ID_FALSE;
                                    }
                                    else
                                    {
                                        sUsable = ID_TRUE;
                                    }
                                    break;
                                default:
                                    sUsable = ID_FALSE;
                                    break;
                            }
                            break;
                        default:
                            IDE_DASSERT(0);
                            sUsable = ID_FALSE;
                            break;
                    }
                }
                else
                {
                    // Backward Index의 경우

                    switch ( sIndex->keyColsFlag[i] & SMI_COLUMN_ORDER_MASK )
                    {
                        case SMI_COLUMN_ORDER_ASCENDING :
                            switch ( sOrder->direction )
                            {
                                case QMG_DIRECTION_DESC:
                                    sUsable = ID_TRUE;
                                    break;
                                case QMG_DIRECTION_DESC_NULLS_FIRST:
                                    if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                                         == SMI_COLUMN_ORDER_ASCENDING )
                                    {
                                        sUsable = ID_TRUE;
                                    }
                                    else
                                    {
                                        sUsable = ID_FALSE;
                                    }
                                    break;
                                default:
                                    sUsable = ID_FALSE;
                                    break;
                            }
                            break;
                        case SMI_COLUMN_ORDER_DESCENDING :
                            switch ( sOrder->direction )
                            {
                                case QMG_DIRECTION_ASC:
                                    sUsable = ID_TRUE;
                                    break;
                                case QMG_DIRECTION_ASC_NULLS_FIRST:
                                    if ( ( sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK )
                                         == SMI_COLUMN_ORDER_ASCENDING )
                                    {
                                        sUsable = ID_FALSE;
                                    }
                                    else
                                    {
                                        sUsable = ID_TRUE;
                                    }
                                    break;
                                default:
                                    sUsable = ID_FALSE;
                                    break;
                            }
                            break;
                        default:
                            IDE_DASSERT(0);
                            sUsable = ID_FALSE;
                            break;
                    }
                }

                if ( sUsable == ID_FALSE )
                {
                    // 방향성이 달라 Index를 사용할 수 없는 경우
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // 컬럼 정보가 달라 Index를 사용할 수 없는 경우
                break;
            }
        } // end of for()

        if ( sOrder != NULL )
        {
            // 원하는 Order를 Index가 모두 만족하지 않음.
            sUsable = ID_FALSE;
        }
        else
        {
            // 원하는 Order를 모두 만족함.
            sUsable = ID_TRUE;
        }
    }
    else
    {
        // 첫번째 Key Column이 만족하지 않음
        sUsable = ID_FALSE;
    }

    *aUsable = sUsable;
    
    return IDE_SUCCESS;
    
}


IDE_RC
qmg::checkIndexColumn( qcmIndex          * aIndex,
                       UShort              aTableID,
                       qmgPreservedOrder * aWantOrder,
                       idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *     원하는 Order가 해당 Index의 Column에 모두 포함되는 지 검사
 *
 *     원하는 Order의 방향성 및 Order가 중요하지 않은 경우에만
 *     호출하여 검사한다.
 *
 * Implementation :
 *
 *     원하는 Order의 Column이 인덱스 Column에 모두 포함되는 지를
 *     검사한다.
 *
 *     모든 Order가 모든 Index Key Column의 순서에 부합되어야 한다.
 *         T1(i1,i2,i3) Index
 *         SELECT DISTINCT i1, i2 FROM T1;         ---- O
 *         SELECT DISTINCT i2, i1, i3 FROM T1;     ---- O
 *         SELECT DISTINCT i1, i3, i4 FROM T1;     ---- X
 *         SELECT DISTINCT i2, i3 FROM T1;         ---- X
 *
 ***********************************************************************/

    UInt                i;
    UInt                sOrderCnt;

    idBool              sUsable = ID_FALSE;
    UShort              sColumnPosition;

    qmgPreservedOrder * sOrder;

    IDU_FIT_POINT_FATAL( "qmg::checkIndexColumn::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aUsable != NULL );

    //---------------------------------------------------
    // 해당 index에 원하는 order의 모든 column을 가지는지를 검사
    // 모든 Index Key Column에 해당하는 Order가 존재하는 지 검사
    //---------------------------------------------------

    // Order의 개수 계산
    for ( sOrder = aWantOrder, sOrderCnt = 0;
          sOrder != NULL;
          sOrder = sOrder->next, sOrderCnt++ ) ;

    if ( sOrderCnt <= aIndex->keyColCount )
    {
        // Index Key Column의 순서대로
        // Order를 포함하고 있는 지 검사.

        for ( i = 0; i < sOrderCnt; i++ )
        {
            sColumnPosition = (UShort)(aIndex->keyColumns[i].column.id % SMI_COLUMN_ID_MAXIMUM);

            sUsable = ID_FALSE;

            for ( sOrder = aWantOrder;
                  sOrder != NULL;
                  sOrder = sOrder->next )
            {
                if ( ( sOrder->table == aTableID ) &&
                     ( sOrder->column == sColumnPosition ) )
                {
                    // 동일한 Key Column이 존재
                    sUsable = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }

            if ( sUsable == ID_FALSE )
            {
                // Key Column에 해당하는 Order가 없음
                break;
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        // Order 개수가 Index Key Column의 개수보다 많음
        sUsable = ID_FALSE;
    }

    *aUsable = sUsable;
    
    return IDE_SUCCESS;
    
}


IDE_RC
qmg::refineID4Target( qmgPreservedOrder * aWantOrder,
                      qmsTarget         * aTarget )
{
/***********************************************************************
 *
 * Description :
 *     원하는 Order를 Target의 ID로 대체한다.
 *
 * Implementation :
 *     원하는 Order의 Column Position으로부터 Target의 위치를
 *     구하고 Target의 ID로 Order의 ID를 대체한다.
 *
 ***********************************************************************/

    UShort  i;

    qmgPreservedOrder * sOrder;
    qmsTarget         * sTarget;
    qtcNode           * sTargetNode;

    IDU_FIT_POINT_FATAL( "qmg::refineID4Target::__FT__" );
    
    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aTarget != NULL );

    //---------------------------------------------------
    // Order의 ID를 변경
    //---------------------------------------------------

    for ( sOrder = aWantOrder;
          sOrder != NULL;
          sOrder = sOrder->next )
    {
        // Column Position에 해당하는 Target을 추출
        for ( i = 0, sTarget = aTarget;
              i < sOrder->column && sTarget != NULL;
              i++, sTarget = sTarget->next ) ;

        // 반드시 Target이 존재해야 함.
        IDE_FT_ASSERT( sTarget != NULL );

        // BUG-38193 target의 pass node 를 고려해야 합니다.
        if ( sTarget->targetColumn->node.module == &qtc::passModule )
        {
            sTargetNode = (qtcNode*)(sTarget->targetColumn->node.arguments);
        }
        else
        {
            sTargetNode = sTarget->targetColumn;
        }

        // Order의 ID 변경
        sOrder->table  = sTargetNode->node.table;
        sOrder->column = sTargetNode->node.column;
    }
    
    return IDE_SUCCESS;
    
}

/**
 * Bug-42145
 *
 * Index가 존재하는 상황에서 Nulls Option이 표함된 방향 체크를한다.
 *
 * Index에서 이전 Direction이 Not Defined인 경우 ASC NULLS FIRST와 DESC NULLS
 * LAST 는 같게 처리될 수 없다.
 * Index에서 ASC = ASC NULLS LAST,
 *           DESC = DESC NULLS FIRST 와 같다.
 */
IDE_RC qmg::checkSameDirection4Index( qmgPreservedOrder * aWantOrder,
                                      qmgPreservedOrder * aPresOrder,
                                      qmgDirectionType    aPrevDirection,
                                      qmgDirectionType  * aNowDirection,
                                      idBool            * aUsable )
{
    idBool sUsable = ID_FALSE;
    qmgDirectionType sDirection = QMG_DIRECTION_NOT_DEFINED;

    IDU_FIT_POINT_FATAL( "qmg::checkSameDirection4Index::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aPresOrder != NULL );
    IDE_DASSERT( aNowDirection != NULL );
    IDE_DASSERT( aUsable    != NULL );

    //---------------------------------------------------
    // 방향성 검사
    //---------------------------------------------------

    switch ( aPresOrder->direction )
    {
        case QMG_DIRECTION_NOT_DEFINED :

            if ( ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_FIRST ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_LAST )  || 
                 ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_FIRST ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_LAST ) )
            {
                sUsable = ID_FALSE;
            }
            else
            {
                // Preserved Order에 어떠한 방향성도 없는 경우
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            break;

        case QMG_DIRECTION_SAME_WITH_PREV :

            // Preserved Order가 이전 방향과 동일한 경우

            if ( aPrevDirection == aWantOrder->direction )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;

        case QMG_DIRECTION_DIFF_WITH_PREV :

            // Preserved Order가 이전 방향과 다른 경우

            if ( aPrevDirection == aWantOrder->direction )
            {
                sUsable = ID_FALSE;
            }
            else
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            break;

        case QMG_DIRECTION_ASC :
            if ( ( aWantOrder->direction == QMG_DIRECTION_ASC ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_LAST ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_NOT_DEFINED ) )
            {
                sDirection = QMG_DIRECTION_ASC;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;

        case QMG_DIRECTION_DESC :
            if ( ( aWantOrder->direction == QMG_DIRECTION_DESC ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_FIRST ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_NOT_DEFINED ) )
            {
                sDirection = QMG_DIRECTION_DESC;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;

        case QMG_DIRECTION_ASC_NULLS_FIRST :
            if ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_FIRST )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        case QMG_DIRECTION_ASC_NULLS_LAST :
            if ( ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_LAST ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_ASC ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_NOT_DEFINED ) )
            {
                sDirection = QMG_DIRECTION_ASC;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        case QMG_DIRECTION_DESC_NULLS_FIRST :
            if ( ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_FIRST ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_DESC ) ||
                 ( aWantOrder->direction == QMG_DIRECTION_NOT_DEFINED ) )
            {
                sDirection = QMG_DIRECTION_DESC;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        case QMG_DIRECTION_DESC_NULLS_LAST :
            if ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_LAST )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        default :
            IDE_DASSERT(0);
            sUsable = ID_FALSE;
            break;
    }

    *aUsable = sUsable;
    *aNowDirection = sDirection;
    
    return IDE_SUCCESS;
    
}

IDE_RC
qmg::checkSameDirection( qmgPreservedOrder * aWantOrder,
                         qmgPreservedOrder * aPresOrder,
                         qmgDirectionType    aPrevDirection,
                         qmgDirectionType  * aNowDirection,
                         idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *     원하는 Order가 Preserved Order와 동일한(사용 가능한)
 *     방향성을 갖고 있는 지 검사한다.
 *
 * Implementation :
 *     Preserved Order가 갖는 방향성에 따라,
 *     원하는 Order를 사용할 수 있는지를 판단
 *
 ***********************************************************************/

    idBool sUsable = ID_FALSE;
    qmgDirectionType sDirection = QMG_DIRECTION_NOT_DEFINED;

    IDU_FIT_POINT_FATAL( "qmg::checkSameDirection::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aWantOrder != NULL );
    IDE_DASSERT( aPresOrder != NULL );
    IDE_DASSERT( aNowDirection != NULL );
    IDE_DASSERT( aUsable    != NULL );

    //---------------------------------------------------
    // 방향성 검사
    //---------------------------------------------------

    switch ( aPresOrder->direction )
    {
        case QMG_DIRECTION_NOT_DEFINED :

            // Preserved Order에 어떠한 방향성도 없는 경우
            sDirection = aWantOrder->direction;
            sUsable = ID_TRUE;
            break;

        case QMG_DIRECTION_SAME_WITH_PREV :

            // Preserved Order가 이전 방향과 동일한 경우

            if ( aPrevDirection == aWantOrder->direction )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;

        case QMG_DIRECTION_DIFF_WITH_PREV :

            // Preserved Order가 이전 방향과 다른 경우

            if ( aPrevDirection == aWantOrder->direction )
            {
                sUsable = ID_FALSE;
            }
            else
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            break;

        case QMG_DIRECTION_ASC :
            if ( aWantOrder->direction == QMG_DIRECTION_ASC )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;

        case QMG_DIRECTION_DESC :
            if ( aWantOrder->direction == QMG_DIRECTION_DESC )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;

        case QMG_DIRECTION_ASC_NULLS_FIRST :
            if ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_FIRST )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        case QMG_DIRECTION_ASC_NULLS_LAST :
            if ( aWantOrder->direction == QMG_DIRECTION_ASC_NULLS_LAST )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        case QMG_DIRECTION_DESC_NULLS_FIRST :
            if ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_FIRST )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        case QMG_DIRECTION_DESC_NULLS_LAST :
            if ( aWantOrder->direction == QMG_DIRECTION_DESC_NULLS_LAST )
            {
                sDirection = aWantOrder->direction;
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
            break;
        default :
            IDE_DASSERT(0);
            sUsable = ID_FALSE;
            break;
    }

    *aUsable = sUsable;
    *aNowDirection = sDirection;
    
    return IDE_SUCCESS;

}

IDE_RC
qmg::makeOrder4Graph( qcStatement       * aStatement,
                      qmgGraph          * aGraph,
                      qmgPreservedOrder * aWantOrder )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 Order를 이용하여 Graph의
 *    Preserved Order를 생성한다.
 *
 * Implementation :
 *
 *    해당 Graph의 종류에 따라 처리
 *        - Selection Graph인 경우
 *            - Base Table 인 경우 :
 *                해당 Index를 찾은 후 Preserved Order Build
 *            - View인 경우
 *                하위 Target의 ID로 변경하여 Child Graph에 대한 처리
 *                입력된 Want Order로 Preserved Order Build
 *        - Set, Dnf, Hierarchy인 경우
 *            - 해당 사항 없음.
 *        - 이 외의 Graph인 경우
 *            - Non-Leaf Graph에 대한 Preserved Order Build
 *
 ***********************************************************************/

    idBool            sOrderImportant;
    idBool            sUsable;
    idBool            sIsSorted;

    qmoAccessMethod * sOrgMethod;
    qmoAccessMethod * sSelMethod;

    qmsParseTree      * sParseTree;

    qmgPreservedOrder * sOrder;
    qmgPreservedOrder * sWantOrder;

    // To Fix BUG-8838
    qmgPreservedOrder * sFirstOrder = NULL;
    qmgPreservedOrder * sLastOrder = NULL;
    qmgPreservedOrder * sNewOrder = NULL;

    IDU_FIT_POINT_FATAL( "qmg::makeOrder4Graph::__FT__" );

    sIsSorted = ID_FALSE;

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );
    IDE_FT_ASSERT( aWantOrder != NULL );

    // Order가 중요한지를 판단.
    // BUG-40361 supporting to indexable analytic function
    // aWantOrder의 모든 Node가 NOT DEFINED 인지 확인한다
    sOrderImportant = ID_FALSE;
    for ( sWantOrder = aWantOrder; sWantOrder != NULL; sWantOrder = sWantOrder->next )
    {
        if ( sWantOrder->direction != QMG_DIRECTION_NOT_DEFINED )
        {
            sOrderImportant = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
    // Graph 종류에 따른 Preserved Order 구축
    switch ( aGraph->type )
    {
        case QMG_SET :
        case QMG_DNF :
        case QMG_HIERARCHY :

            //--------------------------------
            // Order를 적용할 수 없는 Graph인 경우
            //--------------------------------

            IDE_DASSERT( 0 );
            break;
            
        case QMG_WINDOWING :
            
            if ( ( aGraph->flag & QMG_PRESERVED_ORDER_MASK )
                 == QMG_PRESERVED_ORDER_DEFINED_NOT_FIXED )
            {
                // Windowing Graph에 preserved order가 존재하는 경우,
                // Want Order와 동일한 Sorting Key의 Sorting 순서를
                // 마지막으로 변경한다.
                IDE_TEST( qmgWindowing::alterSortingOrder( aStatement,
                                                           aGraph,
                                                           aWantOrder,
                                                           ID_FALSE )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( ( aGraph->flag & QMG_CHILD_PRESERVED_ORDER_USE_MASK )
                     == QMG_CHILD_PRESERVED_ORDER_CAN_USE )
                {
                    IDE_TEST( checkUsableOrder( aStatement,
                                                aWantOrder,
                                                aGraph->left,
                                                & sOrgMethod,
                                                & sSelMethod,
                                                & sUsable )
                              != IDE_SUCCESS );
                    if ( sUsable == ID_TRUE )
                    {
                        // BUG-21812
                        // Windowing Graph에 preserved order가 존재하지 않는 경우
                        // ( 빈 over절을 가진 analytic function들만이 존재하는
                        //   경우 )
                        IDE_TEST( makeOrder4NonLeafGraph( aStatement,
                                                          aGraph,
                                                          aWantOrder )
                                  != IDE_SUCCESS );
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
            
            break;

        case QMG_SELECTION :

            if ( aGraph->myFrom->tableRef->view == NULL )
            {
                // 일반 Table을 위한 Selection Graph인 경우

                // Order에 부합하는 Index 추출
                IDE_TEST( checkUsableIndex4Selection( aGraph,
                                                      aWantOrder,
                                                      sOrderImportant,
                                                      & sOrgMethod,
                                                      & sSelMethod,
                                                      & sUsable )
                          != IDE_SUCCESS );

                // BUG-45062 sUsable 값을 체크해야 합니다.
                if ( sUsable == ID_TRUE )
                {
                    // Selection Graph에 대한 Order 생성
                    IDE_TEST( makeOrder4Index( aStatement,
                                               aGraph,
                                               sSelMethod,
                                               aWantOrder )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                // View를 위한 Selection Graph인 경우

                // 해당 Graph의 Preserved Order를 위한 복사
                for ( sOrder = aWantOrder;
                      sOrder != NULL;
                      sOrder = sOrder->next )
                {
                    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                  ID_SIZEOF(qmgPreservedOrder),
                                  (void**) & sNewOrder )
                              != IDE_SUCCESS );

                    idlOS::memcpy( sNewOrder,
                                   sOrder,
                                   ID_SIZEOF(qmgPreservedOrder) );
                    sNewOrder->next = NULL;

                    if ( sFirstOrder == NULL )
                    {
                        sFirstOrder = sNewOrder;
                        sLastOrder = sFirstOrder;
                    }
                    else
                    {
                        sLastOrder->next = sNewOrder;
                        sLastOrder = sLastOrder->next;
                    }
                }

                // Child Graph를 위한 ID 변경
                sParseTree = (qmsParseTree *)
                    aGraph->myFrom->tableRef->view->myPlan->parseTree;

                // 원하는 Order의 ID를
                // View의 Target에 부합하는 ID로 변경
                IDE_TEST(
                    refineID4Target( aWantOrder,
                                     sParseTree->querySet->target )
                    != IDE_SUCCESS );

                // Child Graph에 대한 Preserved Order 생성
                IDE_TEST( makeOrder4NonLeafGraph(
                              aGraph->myFrom->tableRef->view,
                              aGraph->left,
                              aWantOrder )
                          != IDE_SUCCESS );

                IDE_DASSERT( aGraph->left->preservedOrder != NULL );

                // View의 Preserved Order에 대한 direction 정보 설정
                for ( sNewOrder = sFirstOrder,
                          sOrder = aGraph->left->preservedOrder;
                      sNewOrder != NULL && sOrder != NULL;
                      sNewOrder = sNewOrder->next, sOrder = sOrder->next )
                {
                    sNewOrder->direction = sOrder->direction;
                }

                // View의 Selection Graph에 대한 정보 설정
                aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
                aGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
                aGraph->preservedOrder = sFirstOrder;
            }

            break;

        case QMG_PARTITION :

            // Order에 부합하는 Index 추출
            IDE_TEST( checkUsableIndex4Partition( aGraph,
                                                  aWantOrder,
                                                  sOrderImportant,
                                                  & sOrgMethod,
                                                  & sSelMethod,
                                                  & sUsable )
                      != IDE_SUCCESS );

            // BUG-45062 sUsable 값을 체크해야 합니다.
            if ( sUsable == ID_TRUE )
            {
                // Selection Graph에 대한 Order 생성
                IDE_TEST( makeOrder4Index( aStatement,
                                           aGraph,
                                           sSelMethod,
                                           aWantOrder )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            break;
            
            // To fix BUG-14336
            // join의 preserved order는 child와 관련된 order임.
        case QMG_PROJECTION :
        case QMG_INNER_JOIN :
        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
        case QMG_LEFT_OUTER_JOIN :
        case QMG_FULL_OUTER_JOIN :
            // BUG-22034
        case QMG_COUNTING :

            IDE_TEST( makeOrder4NonLeafGraph( aStatement,
                                              aGraph,
                                              aWantOrder )
                      != IDE_SUCCESS );
            break;

            // To Fix BUG-8344
        case QMG_GROUPING :
        case QMG_SORTING :
        case QMG_DISTINCTION :
            if ( aGraph->preservedOrder != NULL )
            {
                // To fix BUG-14336
                // grouping, sorting, distinction graph가 sorting에 의해
                // preserved order가 생성된 경우는 다음과 같다.
                switch( aGraph->type )
                {
                    case QMG_GROUPING:
                        // grouping 팁이 none인 경우 sort에 의해 preserved order 생성
                        if( ( aGraph->flag & QMG_GROP_OPT_TIP_MASK ) == QMG_GROP_OPT_TIP_NONE )
                        {
                            sIsSorted = ID_TRUE;
                        }
                        else
                        {
                            sIsSorted = ID_FALSE;
                        }
                        break;

                    case QMG_SORTING:
                        // sort 팁이 none 이거나 lmst인 경우 sort에 의해 preserved order 생성
                        if( ( ( aGraph->flag & QMG_SORT_OPT_TIP_MASK ) == QMG_SORT_OPT_TIP_NONE ) ||
                            ( ( aGraph->flag & QMG_SORT_OPT_TIP_MASK ) == QMG_SORT_OPT_TIP_LMST ) )
                        {
                            sIsSorted = ID_TRUE;
                        }
                        else
                        {
                            sIsSorted = ID_FALSE;
                        }
                        break;

                    case QMG_DISTINCTION:
                        // distinct 팁이 none인 경우 sort에 의해 preserved order 생성
                        if( ( aGraph->flag & QMG_DIST_OPT_TIP_MASK ) == QMG_DIST_OPT_TIP_NONE )
                        {
                            sIsSorted = ID_TRUE;
                        }
                        else
                        {
                            sIsSorted = ID_FALSE;
                        }
                        break;

                    default:
                        IDE_DASSERT(0);
                        break;
                }

                if ( sIsSorted == ID_TRUE )
                {
                    //---------------------------------------------------
                    // sorting에 의하여 새로 preserved order가 생성된 경우
                    //---------------------------------------------------

                    if ( aGraph->preservedOrder->direction ==
                         QMG_DIRECTION_NOT_DEFINED )
                    {
                        for ( sOrder = aGraph->preservedOrder,
                                  sWantOrder = aWantOrder;
                              sOrder != NULL && sWantOrder != NULL;
                              sOrder = sOrder->next,
                                  sWantOrder = sWantOrder->next )
                        {
                            sOrder->direction = sWantOrder->direction;
                        }
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    IDE_TEST( makeOrder4NonLeafGraph( aStatement,
                                                      aGraph,
                                                      aWantOrder )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                IDE_TEST( makeOrder4NonLeafGraph( aStatement,
                                                  aGraph,
                                                  aWantOrder )
                          != IDE_SUCCESS );
            }
            break;

        default :
            IDE_DASSERT( 0 );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::makeOrder4Index( qcStatement       * aStatement,
                      qmgGraph          * aGraph,
                      qmoAccessMethod   * aMethod,
                      qmgPreservedOrder * aWantOrder )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 Order와 index를 이용하여 Selection Graph의
 *    Preserved Order를 생성한다.
 *
 * Implementation :
 *
 *    상위 Graph에서의 Merge를 위해 Index의 모든 Presreved Order를
 *    생성하지 않고, 주어진 Order에 해당하는 Preserved Order를
 *    생성한다.
 *
 ***********************************************************************/

    UInt                i;
    idBool              sOrderImportant;
    qmgPreservedOrder * sOrder;

    qcmIndex          * sIndex;
    idBool              sHintExist;
    UInt                sPrevOrder;
    UShort              sColumnPosition;

    qmgPreservedOrder * sFirstOrder = NULL;
    qmgPreservedOrder * sLastOrder = NULL;
    qmgPreservedOrder * sNewOrder = NULL;

    qmgSELT           * sSELTgraph;
    qmgPARTITION      * sPARTgraph;

    IDU_FIT_POINT_FATAL( "qmg::makeOrder4Index::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGraph != NULL );
    IDE_FT_ASSERT( (aGraph->type == QMG_SELECTION) ||
                   (aGraph->type == QMG_PARTITION) );
    IDE_FT_ASSERT( aMethod != NULL );
    IDE_FT_ASSERT( aWantOrder != NULL );

    //---------------------------------------------------
    // Index를 이용한 Preserved Order 구성
    //---------------------------------------------------

    // Order가 중요한지를 판단.
    // BUG-40361 supporting to indexable analytic function
    // aWantOrder의 모든 Node가 NOT DEFINED 인지 확인한다
    sOrderImportant = ID_FALSE;
    for ( sOrder = aWantOrder; sOrder != NULL; sOrder = sOrder->next )
    {
        if ( sOrder->direction != QMG_DIRECTION_NOT_DEFINED )
        {
            sOrderImportant = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sOrderImportant == ID_TRUE )
    {
        // Order가 중요한 경우
        // Want Order와 동일한 Preserved Order를 생성한다.

        // Key Column Order와Want Order가 동일한 순서로 구성되어 있으며,
        // 이미 해당 index를 이용한 적합성 검사는 모두 이루어진 상태임

        for ( sOrder = aWantOrder;
              sOrder != NULL;
              sOrder = sOrder->next )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                     (void**) & sNewOrder )
                      != IDE_SUCCESS );

            idlOS::memcpy( sNewOrder, sOrder, ID_SIZEOF(qmgPreservedOrder) );
            sNewOrder->next = NULL;

            if ( sFirstOrder == NULL )
            {
                sFirstOrder = sNewOrder;
                sLastOrder = sFirstOrder;
            }
            else
            {
                sLastOrder->next = sNewOrder;
                sLastOrder = sLastOrder->next;
            }
        }
    }
    else
    {
        // Order가 중요하지 않은 경우
        // 각 Preserved Order의 방향성을 결정함.

        // Key Column 순서에 해당하는 Want Order는 존재하나
        // 그 순서가 일정치 않음

        // 인덱스 정보 획득
        sIndex = aMethod->method->index;

        // 첫번째 Column에 대한 Order 정보 설정
        // 반드시 첫번째 Column은 Want Order에 포함되어 있음을
        // 보장한다.

        sOrder = aWantOrder;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                 (void**) & sNewOrder )
                  != IDE_SUCCESS );

        // To Fix PR-8572
        // 순서가 중요하지 않다면, Index의 Key Column의 순서대로
        // Selection Graph에 Preserved Order를 생성하여야 한다.
        sNewOrder->table = sOrder->table;
        sNewOrder->column =
            sIndex->keyColumns[0].column.id % SMI_COLUMN_ID_MAXIMUM;
        sNewOrder->next = NULL;

        // Order의 방향 설정
        switch ( aMethod->method->flag & QMO_STAT_CARD_IDX_HINT_MASK )
        {
            case QMO_STAT_CARD_IDX_INDEX_ASC :

                // Index에 대하여 Ascending Hint가 있는 경우
                sNewOrder->direction = QMG_DIRECTION_ASC;
                sHintExist = ID_TRUE;
                break;

            case QMO_STAT_CARD_IDX_INDEX_DESC :

                // Index에 대하여 Descending Hint가 있는 경우
                sNewOrder->direction = QMG_DIRECTION_DESC;
                sHintExist = ID_TRUE;
                break;

            default:
                // 별도의 Hint 가 없는 경우
                sNewOrder->direction = QMG_DIRECTION_NOT_DEFINED;
                sHintExist = ID_FALSE;
                break;
        }

        sFirstOrder = sNewOrder;
        sLastOrder = sFirstOrder;

        sPrevOrder = sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK;

        for ( i = 1; i < sIndex->keyColCount; i++ )
        {
            // Index Key Column과 동일한 Want Order를 검색한다.
            // 존재하지 않을 때까지 반복한다.

            for ( sOrder = aWantOrder; sOrder != NULL; sOrder = sOrder->next )
            {
                sColumnPosition = (UShort)(sIndex->keyColumns[i].column.id % SMI_COLUMN_ID_MAXIMUM);
                if ( sColumnPosition == sOrder->column )
                {
                    // 동일한 Column이 존재하는 경우
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }

            if ( sOrder != NULL )
            {
                // Key Column과 동일한 Order가 존재함

                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                         (void**) & sNewOrder )
                          != IDE_SUCCESS );

                idlOS::memcpy( sNewOrder, sOrder, ID_SIZEOF(qmgPreservedOrder) );

                // Direction 결정
                if ( ( sIndex->keyColsFlag[i] & SMI_COLUMN_ORDER_MASK )
                     == sPrevOrder )
                {
                    // 이전과 방향이 동일한 경우

                    if ( sHintExist == ID_TRUE )
                    {
                        sNewOrder->direction = sLastOrder->direction;
                    }
                    else
                    {
                        sNewOrder->direction = QMG_DIRECTION_SAME_WITH_PREV;
                    }
                }
                else
                {
                    // 이전과 방향이 다른 경우

                    if ( sHintExist == ID_TRUE )
                    {
                        switch ( sLastOrder->direction )
                        {
                            case QMG_DIRECTION_ASC:
                                sNewOrder->direction = QMG_DIRECTION_DESC;
                                break;
                            case QMG_DIRECTION_DESC:
                                sNewOrder->direction = QMG_DIRECTION_ASC;
                                break;
                            default:
                                IDE_DASSERT(0);
                                break;
                        }
                    }
                    else
                    {
                        sNewOrder->direction = QMG_DIRECTION_DIFF_WITH_PREV;
                    }

                    sPrevOrder =
                        sIndex->keyColsFlag[i] & SMI_COLUMN_ORDER_MASK;
                }
                sNewOrder->next = NULL;

                sLastOrder->next = sNewOrder;
                sLastOrder = sLastOrder->next;
            }
            else
            {
                // Key Column과 동일한 Order가 존재하지 않음
                // 더 이상 진행할 필요가 없음
                break;
            }
        }
    }

    //---------------------------------------------------
    // Selection Graph에 정보 설정
    //---------------------------------------------------

    switch ( aGraph->type )
    {
        case QMG_SELECTION :
            
            sSELTgraph = (qmgSELT *) aGraph;

            // 선택 Index 변경
            IDE_TEST( qmgSelection::alterSelectedIndex( aStatement,
                                                        sSELTgraph,
                                                        aMethod->method->index )
                      != IDE_SUCCESS );

            // Preserved Order 연결
            sSELTgraph->graph.preservedOrder = sFirstOrder;

            // Preserved Order Mask 설정
            sSELTgraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sSELTgraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
            
            break;
            
        case QMG_PARTITION :

            sPARTgraph = (qmgPARTITION *) aGraph;

            // 선택 Index 변경
            IDE_TEST( qmgPartition::alterSelectedIndex( aStatement,
                                                        sPARTgraph,
                                                        aMethod->method->index )
                      != IDE_SUCCESS );

            // Preserved Order 연결
            sPARTgraph->graph.preservedOrder = sFirstOrder;

            // Preserved Order Mask 설정
            sPARTgraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sPARTgraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
            
            break;
            
        default :
            IDE_DASSERT( 0 );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::makeOrder4NonLeafGraph( qcStatement       * aStatement,
                             qmgGraph          * aGraph,
                             qmgPreservedOrder * aWantOrder )
{
/***********************************************************************
 *
 * Description :
 *
 *    주어진 Order를 이용하여 Non Leaf Graph의
 *    Preserved Order를 생성한다.
 *
 * Implementation :
 *
 *    Child Graph에 대한 Preserved Order Build
 *    Child Graph의 Preserved Order 정보를 조합하여 Preserverd Order Build
 *
 ***********************************************************************/

    qmgPreservedOrder * sOrder;

    qmgPreservedOrder * sFirstOrder = NULL;
    qmgPreservedOrder * sLastOrder = NULL;
    qmgPreservedOrder * sNewOrder = NULL;

    IDU_FIT_POINT_FATAL( "qmg::makeOrder4NonLeafGraph::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aWantOrder != NULL );

    //---------------------------------------------------
    // Child Graph에 대한 Preserved Order 생성
    //---------------------------------------------------

    // Child Graph 에 대한 Order정보 구축
    IDE_TEST( setOrder4Child( aStatement,
                              aWantOrder,
                              aGraph->left )
              != IDE_SUCCESS );

    //---------------------------------------------------
    // Non-Leaf Graph에 대한 Preserved Order 생성
    //---------------------------------------------------

    // 기존의 Preserved Order 정보는 무시하며,
    // 주어진 Preserved Order를 이용하여 새로 생성한다.

    aGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
    aGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;

    // Child의 Order 정보를 이용하여 Order정보를 구성한다.
    // Child Graph는 주어진 Order만으로 Preserved Order가
    // 구성되어 있다.

    // Left Graph는 반드시 Preserved Order가 존재한다.
    IDE_DASSERT( aGraph->left->preservedOrder != NULL );

    //------------------------------------
    // Left Graph로부터 Order 추출
    //------------------------------------

    for ( sOrder = aGraph->left->preservedOrder;
          sOrder != NULL;
          sOrder = sOrder->next )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
                                                 (void **) & sNewOrder )
                  != IDE_SUCCESS );

        idlOS::memcpy( sNewOrder, sOrder, ID_SIZEOF(qmgPreservedOrder) );
        sNewOrder->next = NULL;

        if ( sLastOrder == NULL )
        {
            sFirstOrder = sNewOrder;
            sLastOrder = sFirstOrder;
        }
        else
        {
            sLastOrder->next = sNewOrder;
            sLastOrder = sLastOrder->next;
        }
    }

    //------------------------------------
    // Right Graph로부터 Preserved Order 추출
    //------------------------------------

    if ( aGraph->right != NULL )
    {
        // Right Graph에는 Preserved Order가 존재하지 않을 수 있음
        for ( sOrder = aGraph->right->preservedOrder;
              sOrder != NULL;
              sOrder = sOrder->next )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                          ID_SIZEOF(qmgPreservedOrder),
                          (void **) & sNewOrder )
                      != IDE_SUCCESS );

            idlOS::memcpy( sNewOrder,
                           sOrder,
                           ID_SIZEOF(qmgPreservedOrder) );
            sNewOrder->next = NULL;

            IDE_FT_ASSERT( sFirstOrder != NULL );
            IDE_FT_ASSERT( sLastOrder != NULL );

            sLastOrder->next = sNewOrder;
            sLastOrder = sLastOrder->next;
        }
    }
    else
    {
        // Nothing To Do
    }

    aGraph->preservedOrder = sFirstOrder;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::makeAnalFuncResultMtrNode( qcStatement       * aStatement,
                                qmsAnalyticFunc   * aAnalyticFunc,
                                UShort              aTupleID,
                                UShort            * aColumnCount,
                                qmcMtrNode       ** aMtrNode,
                                qmcMtrNode       ** aLastMtrNode )
{
/***********************************************************************
 *
 * Description : Analytic Function 결과가 저장될 materialize node를
 *               생성
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMtrNode        * sLastMtrNode;
    qmcMtrNode        * sNewMtrNode;

    IDU_FIT_POINT_FATAL( "qmg::makeAnalFuncResultMtrNode::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aAnalyticFunc != NULL );
    IDE_DASSERT( aColumnCount != NULL );

    //----------------------------------
    // 기본 초기화
    //----------------------------------
    
    sLastMtrNode = *aLastMtrNode;
    
    //----------------------------------
    //  1. Analytic Function의 Result가 저장될 materialize node 생성
    //----------------------------------

    IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmcMtrNode, &sNewMtrNode)
              != IDE_SUCCESS);

    // 초기화
    sNewMtrNode->srcNode = NULL;
    sNewMtrNode->dstNode = NULL;
    sNewMtrNode->flag = 0;
    sNewMtrNode->next = NULL;
    sNewMtrNode->myDist = NULL;
    sNewMtrNode->bucketCnt = 0;

    // flag 설정
    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
    sNewMtrNode->flag |= QMC_MTR_TYPE_COPY_VALUE;

    sNewMtrNode->flag &= ~QMC_MTR_CHANGE_COLUMN_LOCATE_MASK;
    sNewMtrNode->flag |= QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE;

    //--------------
    // srcNode 구성
    //--------------
    
    IDE_TEST(
        qtc::makeInternalColumn( aStatement,
                                 aAnalyticFunc->analyticFuncNode->node.table,
                                 aAnalyticFunc->analyticFuncNode->node.column,
                                 &(sNewMtrNode->srcNode) )
        != IDE_SUCCESS);

    idlOS::memcpy( sNewMtrNode->srcNode,
                   aAnalyticFunc->analyticFuncNode,
                   ID_SIZEOF( qtcNode ) );

    //--------------
    // dstNode 구성
    //--------------
    
    IDE_TEST( qtc::makeInternalColumn( aStatement,
                                       aTupleID,
                                       *aColumnCount,
                                       &(sNewMtrNode->dstNode) )
              != IDE_SUCCESS);

    idlOS::memcpy( sNewMtrNode->dstNode,
                   sNewMtrNode->srcNode,
                   ID_SIZEOF( qtcNode ) );

    sNewMtrNode->dstNode->node.table  = aTupleID;
    sNewMtrNode->dstNode->node.column = *aColumnCount;
    // Aggregate function의 결과만 복사하면 되므로 value/column node로 설정한다.
    sNewMtrNode->dstNode->node.module = &qtc::valueModule;

    *aColumnCount += (sNewMtrNode->dstNode->node.module->lflag & MTC_NODE_COLUMN_COUNT_MASK);
    
    //----------------------------------
    // 새로운 materialize node 생성한 경우, 이를 연결
    //----------------------------------
    if( sLastMtrNode == NULL )
    {
        *aMtrNode    = sNewMtrNode;
        sLastMtrNode = sNewMtrNode;
    }
    else
    {
        sLastMtrNode->next = sNewMtrNode;
        sLastMtrNode       = sNewMtrNode;
    }

    *aLastMtrNode = sLastMtrNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmg::makeSortMtrNode( qcStatement        * aStatement,
                      UShort               aTupleID,
                      qmgPreservedOrder  * aSortKey,
                      qmsAnalyticFunc    * aAnalFuncList,
                      qmcMtrNode         * aMtrNode,
                      qmcMtrNode        ** aSortMtrNode,
                      UInt               * aSortMtrNodeCount )
{
/***********************************************************************
 *
 * Description : sort key에 대한 materialize node를 생성함
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort               sTable;
    UShort               sColumn;
    qmgPreservedOrder  * sCurSortKeyCol;
    qmsAnalyticFunc    * sAnalyticFunc;
    qmsAnalyticFunc    * sCurAnalyticFunc;
    qtcOverColumn      * sOverColumn;
    UInt                 sOverColumnCount;
    UInt                 sCurOverColumnCount;
    qmcMtrNode         * sCurNode;
    qmcMtrNode         * sNewMtrNode;
    qmcMtrNode         * sFirstSortMtrNode;
    qmcMtrNode         * sLastSortMtrNode       = NULL;
    idBool               sExist;

    IDU_FIT_POINT_FATAL( "qmg::makeSortMtrNode::__FT__" );

    // 초기화
    sFirstSortMtrNode = NULL;
    sAnalyticFunc     = NULL;
    sOverColumnCount  = 0;

    // BUG-33663 Ranking Function
    // sort key에는 여러개의 analytic function들이 달려있고
    // sort node를 생성시 이 들중에서 over column이 가장 많은 것을 기준으로 생성한다.
    // 기존에는 preserved order만으로 가능했었으나
    // order를 고려하여 over column을 직접 참조해야 한다.
    for ( sCurAnalyticFunc = aAnalFuncList;
          sCurAnalyticFunc != NULL;
          sCurAnalyticFunc = sCurAnalyticFunc->next )
    {
        sCurOverColumnCount = 0;
        
        for ( sOverColumn = sCurAnalyticFunc->analyticFuncNode->overClause->overColumn;
              sOverColumn != NULL;
              sOverColumn = sOverColumn->next )
        {
            sCurOverColumnCount++;
        }

        if ( sCurOverColumnCount >= sOverColumnCount )
        {
            sAnalyticFunc = sCurAnalyticFunc;
            sOverColumnCount = sCurOverColumnCount;
        }
        else
        {
            // Nothing to do.
        }
    }

    // sort key에 해당하는 analytic function은 반드시 존재한다.
    IDE_TEST_RAISE( sAnalyticFunc == NULL, ERR_INVALID_ANALYTIC_FUNCTION );
    
    // Sort Key에 해당하는 materialize node를 찾음
    for ( sCurSortKeyCol = aSortKey,
              sOverColumn = sAnalyticFunc->analyticFuncNode->overClause->overColumn;
          sCurSortKeyCol != NULL;
          sCurSortKeyCol = sCurSortKeyCol->next,
              sOverColumn = sOverColumn->next )
    {
        // sort key에 대응되는 over column이 반드시 존재해야 한다.
        IDE_TEST_RAISE( sOverColumn == NULL, ERR_INVALID_OVER_COLUMN );
        
        // 칼럼의 현재 (table, column) 정보를 찾음
        IDE_TEST( qmg::findColumnLocate( aStatement,
                                         aTupleID,
                                         sCurSortKeyCol->table,
                                         sCurSortKeyCol->column,
                                         & sTable,
                                         & sColumn )
                  != IDE_SUCCESS );
    
        // 현재 sort key column의 mtr node를 찾음
        sExist = ID_FALSE;
        
        for ( sCurNode = aMtrNode;
              sCurNode != NULL;
              sCurNode = sCurNode->next )
        {
            if ( ( sCurNode->srcNode->node.table == sTable )
                 &&
                 ( sCurNode->srcNode->node.column == sColumn ) )
            {
                if ( ( (sCurNode->flag & QMC_MTR_TYPE_MASK)
                       != QMC_MTR_TYPE_MEMORY_TABLE )
                     &&
                     ( (sCurNode->flag & QMC_MTR_TYPE_MASK)
                       != QMC_MTR_TYPE_DISK_TABLE ) )
                {
                    if ( ( sCurNode->flag & QMC_MTR_SORT_NEED_MASK )
                         == QMC_MTR_SORT_NEED_TRUE )
                    {
                        // partition by column이어야 함
                        // partition by column은 sorting 해야 하므로
                        // QMC_MTR_SORT_NEED_TRUE가 설정되어 있음
                        
                        // BUG-33663 Ranking Function
                        // mtr node가 order column인 경우에 대해서 동일한 order를 갖는 컬럼을 찾는다.
                        if ( (sOverColumn->flag & QTC_OVER_COLUMN_MASK)
                             == QTC_OVER_COLUMN_NORMAL )
                        {
                            if ( (sCurNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                                 == QMC_MTR_SORT_ORDER_FIXED_FALSE )
                            {
                                sExist = ID_TRUE;
                                break;
                            }
                            else
                            {
                                // Noting to do.
                            }
                        }
                        else
                        {
                            IDE_DASSERT( (sOverColumn->flag & QTC_OVER_COLUMN_MASK)
                                         == QTC_OVER_COLUMN_ORDER_BY );

                            if ( (sCurNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                                 == QMC_MTR_SORT_ORDER_FIXED_TRUE )
                            {
                                if ( ( (sCurNode->flag & QMC_MTR_SORT_ORDER_MASK)
                                       == QMC_MTR_SORT_ASCENDING )
                                     &&
                                     ( (sOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                                       == QTC_OVER_COLUMN_ORDER_ASC ) )
                                {
                                    // BUG-42145 Nulls Option 이 다른 경우도
                                    // 체크해야한다.
                                    if ( ( ( sCurNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                           == QMC_MTR_SORT_NULLS_ORDER_NONE ) &&
                                         ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                           == QTC_OVER_COLUMN_NULLS_ORDER_NONE ) )
                                    {
                                        // order by column
                                        sExist = ID_TRUE;
                                        break;
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                    if ( ( ( sCurNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                           == QMC_MTR_SORT_NULLS_ORDER_FIRST ) &&
                                         ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                           == QTC_OVER_COLUMN_NULLS_ORDER_FIRST ) )
                                    {
                                        // order by column
                                        sExist = ID_TRUE;
                                        break;
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                    if ( ( ( sCurNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                           == QMC_MTR_SORT_NULLS_ORDER_LAST ) &&
                                         ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                           == QTC_OVER_COLUMN_NULLS_ORDER_LAST ) )
                                    {
                                        // order by column
                                        sExist = ID_TRUE;
                                        break;
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                                    
                                if ( ( (sCurNode->flag & QMC_MTR_SORT_ORDER_MASK)
                                       == QMC_MTR_SORT_DESCENDING )
                                     &&
                                     ( (sOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                                       == QTC_OVER_COLUMN_ORDER_DESC ) )
                                {
                                    // BUG-42145 Nulls Option 이 다른 경우도
                                    // 체크해야한다.
                                    if ( ( ( sCurNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                           == QMC_MTR_SORT_NULLS_ORDER_NONE ) &&
                                         ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                           == QTC_OVER_COLUMN_NULLS_ORDER_NONE ) )
                                    {
                                        // order by column
                                        sExist = ID_TRUE;
                                        break;
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                    if ( ( ( sCurNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                           == QMC_MTR_SORT_NULLS_ORDER_FIRST ) &&
                                         ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                           == QTC_OVER_COLUMN_NULLS_ORDER_FIRST ) )
                                    {
                                        // order by column
                                        sExist = ID_TRUE;
                                        break;
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                    if ( ( ( sCurNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                                           == QMC_MTR_SORT_NULLS_ORDER_LAST ) &&
                                         ( ( sOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                           == QTC_OVER_COLUMN_NULLS_ORDER_LAST ) )
                                    {
                                        // order by column
                                        sExist = ID_TRUE;
                                        break;
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                // Nothing to do.
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
                    // table을 표현하기 위한 column인 경우
                    // 다른 칼럼임에도 불구하고 같을 수 있음
                }
            }
            else
            {
                // nothing to do 
            }
        }

        // Partition By Column에 대하여 materialize node가 이미
        // 생성되있으므로, 무조건 존재해야함
        IDE_TEST_RAISE( sExist == ID_FALSE, ERR_MTR_NODE_NOT_EXISTS );

        IDE_TEST( STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmcMtrNode, &sNewMtrNode)
                  != IDE_SUCCESS);

        // BUG-28507
        // Partition By Column에 sorting direction을 설정함
        if ( (sCurNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
             == QMC_MTR_SORT_ORDER_FIXED_FALSE )
        {
            if ( sCurSortKeyCol->direction == QMG_DIRECTION_DESC )
            {
                sCurNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sCurNode->flag |= QMC_MTR_SORT_DESCENDING;
            }
            else
            {
                sCurNode->flag &= ~QMC_MTR_SORT_ORDER_MASK;
                sCurNode->flag |= QMC_MTR_SORT_ASCENDING;
            }
        }
        else
        {
            // Nothing to do.
        }

        *sNewMtrNode = *sCurNode;
        sNewMtrNode->next = NULL;

        *aSortMtrNodeCount = *aSortMtrNodeCount + 1;

        if ( sFirstSortMtrNode == NULL )
        {
            sFirstSortMtrNode = sNewMtrNode;
            sLastSortMtrNode  = sNewMtrNode;
        }
        else
        {
            sLastSortMtrNode->next = sNewMtrNode;
            sLastSortMtrNode = sLastSortMtrNode->next;
        }
    }

    *aSortMtrNode = sFirstSortMtrNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MTR_NODE_NOT_EXISTS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeSortMtrNode",
                                  "Materialized Node is not exists" ));
    }
    IDE_EXCEPTION( ERR_INVALID_ANALYTIC_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeSortMtrNode",
                                  "analytic function is null" ));
    }
    IDE_EXCEPTION( ERR_INVALID_OVER_COLUMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeSortMtrNode",
                                  "over column is null" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::makeDistNode( qcStatement        * aStatement,
                   qmsQuerySet        * aQuerySet,
                   UInt                 aFlag,
                   qmnPlan            * aChildPlan,
                   UShort               aTupleID ,
                   qmoDistAggArg      * aDistAggArg,
                   qtcNode            * aAggrNode,
                   qmcMtrNode         * aMtrNode,
                   qmcMtrNode        ** aPlanDistNode,
                   UShort             * aDistNodeCount )
{
/***********************************************************************
 *
 * Description : Materialize Node의 distNode를 구성하고,
 *               Plan의 distNode에 등록한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qmcMtrNode        * sDistNode;
    qmcMtrNode        * sNewMtrNode;
    idBool              sExistSameDistNode;
    UShort              sDistTupleID;
    UShort              sDistNodeCount;
    qmoDistAggArg     * sDistAggArg;
    mtcTemplate       * sMtcTemplate;
    UShort              sDistColumnCount;
    qtcNode           * sPassNode;
    qtcNode             sCopiedNode;
    mtcNode           * sMtcNode;

    IDU_FIT_POINT_FATAL( "qmg::makeDistNode::__FT__" );

    // 기본 초기화
    sExistSameDistNode = ID_FALSE;
    sDistNodeCount = *aDistNodeCount;
    sDistAggArg = NULL;
    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    /* PROJ-1353 dist node를 중복해서 만들기 위해 사용 */
    if ( ( aMtrNode->flag & QMC_MTR_DIST_DUP_MASK )
         == QMC_MTR_DIST_DUP_FALSE )
    {
        // Plan에 등록된 distinct node들 중에서
        // 동일한 distinct node 존재하는지 검사
        for ( sDistNode = *aPlanDistNode;
              sDistNode != NULL;
              sDistNode = sDistNode->next )
        {
            IDE_TEST(
                qtc::isEquivalentExpression(
                    aStatement,
                    (qtcNode *)aAggrNode->node.arguments,
                    sDistNode->srcNode,
                    & sExistSameDistNode )
                != IDE_SUCCESS );

            if ( sExistSameDistNode == ID_TRUE )
            {
                break;
            }
            else
            {
                // nothing to do
            }
        }
    }
    else
    {
        /* Nothing to do */
    }
    if ( sExistSameDistNode == ID_TRUE )
    {
        // 동일한 Distinct Argument가 있을 경우,
        // 새로운 distNode 생성하지 않고 동일한 distNode를 가리킴
        aMtrNode->myDist = sDistNode;
        aMtrNode->bucketCnt = sDistNode->bucketCnt;
    }
    else
    {
        //해당 bucketCount를 찾기 위해서 같은 aggregation인지 찾음
        for( sDistAggArg = aDistAggArg ;
             sDistAggArg != NULL ;
             sDistAggArg = sDistAggArg->next )
        {
            //같은 aggregation이라면
            if( aAggrNode->node.arguments == (mtcNode*)sDistAggArg->aggArg )
            {
                sDistNodeCount++;
                sDistColumnCount = 0;

                //----------------------------------
                // Distinct에 대한 튜플의 할당
                //----------------------------------
                IDE_TEST( qtc::nextTable( &sDistTupleID ,
                                          aStatement ,
                                          NULL,
                                          ID_TRUE,
                                          MTC_COLUMN_NOTNULL_TRUE )
                          != IDE_SUCCESS );

                if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
                {
                    if( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
                        == QMN_PLAN_STORAGE_DISK )
                    {            
                        sMtcTemplate->rows[sDistTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
                        sMtcTemplate->rows[sDistTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
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

                // PROJ-1358
                // Next Table 후에는 Internal Tuple Set의
                // 메모리 확장으로 기존 값을 사용하지 못하는
                // 경우가 있다.
                // mtcTuple * 지역변수를 사용하면 안됨
                // 컬럼의 대체 여부를 결정하기 위해서는
                // Tuple의 저장 매체 정보를 미리 기록하고 있어야 한다.
                if( (aFlag & QMN_PLAN_STORAGE_MASK) ==
                    QMN_PLAN_STORAGE_MEMORY )
                {
                    sMtcTemplate->rows[sDistTupleID].lflag
                        &= ~MTC_TUPLE_STORAGE_MASK;
                    sMtcTemplate->rows[sDistTupleID].lflag
                        |= MTC_TUPLE_STORAGE_MEMORY;
                }
                else
                {
                    sMtcTemplate->rows[sDistTupleID].lflag
                        &= ~MTC_TUPLE_STORAGE_MASK;
                    sMtcTemplate->rows[sDistTupleID].lflag
                        |= MTC_TUPLE_STORAGE_DISK;
                }

                sCopiedNode = *sDistAggArg->aggArg;

                //distNode 생성
                IDE_TEST( qmg::makeColumnMtrNode( aStatement ,
                                                  aQuerySet ,
                                                  &sCopiedNode ,
                                                  ID_TRUE,
                                                  sDistTupleID ,
                                                  0,
                                                  &sDistColumnCount ,
                                                  &sNewMtrNode )
                          != IDE_SUCCESS );

                sNewMtrNode->flag &= ~QMC_MTR_HASH_NEED_MASK;
                sNewMtrNode->flag |= QMC_MTR_HASH_NEED_TRUE;

                sNewMtrNode->bucketCnt = sDistAggArg->bucketCnt;

                //connect
                if( (*aPlanDistNode) == NULL )
                {
                    *aPlanDistNode    = sNewMtrNode;
                }
                else
                {
                    for ( sDistNode = *aPlanDistNode;
                          sDistNode->next != NULL;
                          sDistNode = sDistNode->next ) ;
                                        
                    sDistNode->next = sNewMtrNode;
                }
                
                aMtrNode->myDist = sNewMtrNode;

                //각 Tuple의 할당
                IDE_TEST(
                    qtc::allocIntermediateTuple(
                        aStatement ,
                        & QC_SHARED_TMPLATE(aStatement)->tmplate,
                        sDistTupleID ,
                        sDistColumnCount )
                    != IDE_SUCCESS);
                
                sMtcTemplate->rows[sDistTupleID].lflag
                    &= ~MTC_TUPLE_PLAN_MASK;
                sMtcTemplate->rows[sDistTupleID].lflag
                    |= MTC_TUPLE_PLAN_TRUE;

                sMtcTemplate->rows[aTupleID].lflag
                    &= ~MTC_TUPLE_PLAN_MTR_MASK;
                sMtcTemplate->rows[aTupleID].lflag
                    |= MTC_TUPLE_PLAN_MTR_TRUE;

                if( aQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
                {
                    if( ( aChildPlan->flag & QMN_PLAN_STORAGE_MASK )
                        == QMN_PLAN_STORAGE_DISK )
                    {            
                        sMtcTemplate->rows[sDistTupleID].lflag &= ~MTC_TUPLE_MATERIALIZE_MASK;
                        sMtcTemplate->rows[sDistTupleID].lflag |= MTC_TUPLE_MATERIALIZE_VALUE;
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
                        
                //GRAPH에서 지정한 저장매체를 사용한다.
                if( (aFlag & QMN_PLAN_STORAGE_MASK) ==
                    QMN_PLAN_STORAGE_MEMORY )                    
                {
                    sMtcTemplate->rows[sDistTupleID].lflag
                        &= ~MTC_TUPLE_STORAGE_MASK;
                    sMtcTemplate->rows[sDistTupleID].lflag
                        |= MTC_TUPLE_STORAGE_MEMORY;
                }
                else
                {
                    sMtcTemplate->rows[sDistTupleID].lflag
                        &= ~MTC_TUPLE_STORAGE_MASK;
                    sMtcTemplate->rows[sDistTupleID].lflag
                        |= MTC_TUPLE_STORAGE_DISK;
                }

                IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                                     sNewMtrNode )
                          != IDE_SUCCESS);

                if( ( sNewMtrNode->flag & QMC_MTR_TYPE_MASK ) == QMC_MTR_TYPE_CALCULATE )
                {
                    sNewMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
                    sNewMtrNode->flag |= QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE;
                }
                else
                {
                    // Nothing to do.
                }

                //passNode의 생성
                // qtc::columnModule이거나
                // memory temp table인 경우는 pass node를 생성하지
                // 않는다. 이때, conversion은 NULL이어야 한다.
                if( ( ( sNewMtrNode->srcNode->node.module
                        == &(qtc::columnModule ) ) ||
                      ( ( sNewMtrNode->flag & QMC_MTR_TYPE_MASK )
                        == QMC_MTR_TYPE_MEMORY_KEY_COLUMN ) ||
                      ( ( sNewMtrNode->flag & QMC_MTR_TYPE_MASK )
                        == QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN ) )
                    &&
                    ( sNewMtrNode->srcNode->node.conversion == NULL ) )
                {
                    //memory Temp Table에 저장되고 Memory Column을
                    //제외하고는 myNode->dstNoode->node.argument를
                    //변경 시킨다
                    if( ( ( sMtcTemplate
                            ->rows[sDistAggArg->aggArg->node.table]
                            .lflag
                            & MTC_TUPLE_STORAGE_MASK ) ==
                          MTC_TUPLE_STORAGE_MEMORY ) &&
                        ( ( sMtcTemplate->rows[sDistTupleID].lflag
                            & MTC_TUPLE_STORAGE_MASK ) ==
                          MTC_TUPLE_STORAGE_MEMORY ) )
                    {
                        //nothing to do
                    }
                    else
                    {
                        /* BUG-43698 GROUP_CONCAT()에서 DISTINCT()의 next를 사용한다. */
                        sNewMtrNode->dstNode->node.next = aMtrNode->dstNode->node.arguments->next;

                        aMtrNode->dstNode->node.arguments =
                            (mtcNode *)sNewMtrNode->dstNode;
                        aAggrNode->node.arguments =
                            (mtcNode *)sNewMtrNode->dstNode;
                    }
                }
                else
                {
                    if( ( aQuerySet->materializeType
                          == QMO_MATERIALIZE_TYPE_VALUE )
                        &&
                        ( sMtcTemplate->rows[sNewMtrNode->dstNode->node.table].
                          lflag & MTC_TUPLE_STORAGE_MASK )
                        == MTC_TUPLE_STORAGE_DISK ) 
                    {
                        // Nothing To Do 
                    }
                    else
                    {
                        IDE_TEST( qtc::makePassNode( aStatement ,
                                                     NULL ,
                                                     sNewMtrNode->dstNode ,
                                                     &sPassNode )
                                  != IDE_SUCCESS );
                        /* BUG-45387 GROUP_CONCAT()에서 DISTINCT()의 next를 사용한다. */
                        sMtcNode = aMtrNode->dstNode->node.arguments->next;

                        aMtrNode->dstNode->node.arguments =
                            (mtcNode *)sPassNode;

                        aAggrNode->node.arguments = (mtcNode *)sPassNode;

                        /* BUG-45387 GROUP_CONCAT()에서 DISTINCT()의 next를 사용한다. */
                        sPassNode->node.next = sMtcNode;
                    }
                }
                break;
            }
            else
            {
                // nothing to do
            }
        }
    }

    *aDistNodeCount = sDistNodeCount;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmg::setDirection4SortColumn( qmgPreservedOrder  * aPreservedOrder,
                              UShort               aColumnID,
                              UInt               * aFlag )
{
/***********************************************************************
 *
 * Description : sort 컬럼구성시에 정렬 방향을 결정한다.
 *
 *
 * Implementation :
 *
 *     - NOT DEFINE : ASC
 *     - ACS        : ASC
 *     - DESC       : DESC
 *     - SAME_PREV  : 앞과 방향이 동일.
 *     - DIFF_PREV  : 앞과 방향이 반대.
 *
 ***********************************************************************/

    qmgPreservedOrder * sPreservedOrder;
    UShort              sColumnCount;
    idBool              sIsASC = ID_TRUE;
    idBool              sIsNullsFirst = ID_FALSE;
    idBool              sIsNullsLast = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmg::setDirection4SortColumn::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------
    IDE_DASSERT( aPreservedOrder != NULL );

    for( sPreservedOrder = aPreservedOrder , sColumnCount = 0 ;
         sPreservedOrder != NULL && sColumnCount <= aColumnID ;
         sPreservedOrder = sPreservedOrder->next , sColumnCount++ )
    {
        switch( sPreservedOrder->direction )
        {
            case QMG_DIRECTION_NOT_DEFINED:
            case QMG_DIRECTION_ASC:
                sIsASC = ID_TRUE;
                break;
            case QMG_DIRECTION_DESC:
                sIsASC = ID_FALSE;
                break;
            case QMG_DIRECTION_SAME_WITH_PREV:
                //이전과 동일
                break;
            case QMG_DIRECTION_DIFF_WITH_PREV:
                //반대 방향이다.
                if( sIsASC == ID_TRUE )
                {
                    sIsASC = ID_FALSE;
                }
                else
                {
                    sIsASC = ID_TRUE;
                }
                break;
            case QMG_DIRECTION_ASC_NULLS_FIRST:
                sIsNullsFirst = ID_TRUE;
                break;
            case QMG_DIRECTION_ASC_NULLS_LAST:
                sIsNullsLast = ID_TRUE;
                break;
            case QMG_DIRECTION_DESC_NULLS_FIRST:
                sIsNullsFirst = ID_TRUE;
                break;
            case QMG_DIRECTION_DESC_NULLS_LAST:
                sIsNullsLast = ID_TRUE;
                break;
            default:
                IDE_DASSERT(0);
                break;
        }

    }

    if( sIsASC == ID_TRUE )
    {
        *aFlag &= ~QMC_MTR_SORT_ORDER_MASK;
        *aFlag |= QMC_MTR_SORT_ASCENDING;
    }
    else
    {
        *aFlag &= ~QMC_MTR_SORT_ORDER_MASK;
        *aFlag |= QMC_MTR_SORT_DESCENDING;
    }

    if ( sIsNullsFirst == ID_TRUE )
    {
        *aFlag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
        *aFlag |= QMC_MTR_SORT_NULLS_ORDER_FIRST;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsNullsLast == ID_TRUE )
    {
        *aFlag &= ~QMC_MTR_SORT_NULLS_ORDER_MASK;
        *aFlag |= QMC_MTR_SORT_NULLS_ORDER_LAST;
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_SUCCESS;

}

IDE_RC
qmg::makeOuterJoinFilter(qcStatement   * aStatement,
                         qmsQuerySet   * aQuerySet,
                         qmoPredicate ** aPredicate ,
                         qtcNode       * aNnfFilter,
                         qtcNode      ** aFilter)
{
/***********************************************************************
 *
 * Description : Outer Join에서 쓰일 Filter를 구성한다
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate    * sOnFirstPredicate = NULL;
    qmoPredicate    * sOnLastPredicate = NULL;
    UInt              sIndex;

    IDU_FIT_POINT_FATAL( "qmg::makeOuterJoinFilter::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //---------------------------------------------------
    // To Fix BUG-10988
    // on condition CNF의 constant predicate, one table predicate,
    // nonJoinable Predicate을 연결
    //
    // - Input : aPredicate
    //   aPredicate[0] : constant predicate의 pointer
    //   aPredicate[1] : one table predicate의 pointer
    //   aPredicate[2] : join predicate의 pointer
    //   -------------------
    //  |     |      |      |
    //   ---|-----|------|--
    //      |     |       -->p1->p2
    //      |     ---> q1->q2
    //      ----> r1->r2
    //
    // - Output : r1->r2->q1->q2->p1->p2
    //---------------------------------------------------

    for( sIndex = 0 ; sIndex < 3 ; sIndex++ )
    {
        if ( sOnLastPredicate == NULL )
        {
            sOnFirstPredicate = sOnLastPredicate = aPredicate[sIndex];
        }
        else
        {
            sOnLastPredicate->next = aPredicate[sIndex];
        }

        if ( sOnLastPredicate != NULL )
        {
            // constant predicate, one table predicate, nonjoinable predicate
            // 각각이 두개 이상의 predicate 가질 경우,
            // last predicate으로 이동해 주지 않으면 두번째 이상의 predicate을
            // 잃어버리게 됨
            for ( ; sOnLastPredicate->next != NULL;
                  sOnLastPredicate = sOnLastPredicate->next ) ;
        }
        else
        {
            // nothing to do
        }
    }

    if( sOnFirstPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                sOnFirstPredicate ,
                                                aFilter ) != IDE_SUCCESS );

        // BUG-35155 Partial CNF
        if ( aNnfFilter != NULL )
        {
            IDE_TEST( qmoPred::addNNFFilter4linkedFilter(
                          aStatement,
                          aNnfFilter,
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
        if ( aNnfFilter != NULL )
        {
            *aFilter = aNnfFilter;
        }
        else
        {
            *aFilter = NULL;
        }
    }

    if ( *aFilter != NULL )
    {
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           *aFilter ) != IDE_SUCCESS );
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
qmg::removeSDF( qcStatement * aStatement, qmgGraph * aGraph )
{
    qmgChildren * sChildren;

    IDU_FIT_POINT_FATAL( "qmg::removeSDF::__FT__" );

    if( aGraph->left != NULL )
    {
        IDE_TEST( removeSDF( aStatement, aGraph->left )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do...
    }

    if( aGraph->right != NULL )
    {
        IDE_TEST( removeSDF( aStatement, aGraph->right )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do...
    }

    // BUG-33132
    if ( aGraph->type == QMG_FULL_OUTER_JOIN )
    {
        if ( ((qmgFOJN*)aGraph)->antiLeftGraph != NULL )
        {
            IDE_TEST( removeSDF( aStatement, ((qmgFOJN*)aGraph)->antiLeftGraph )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do 
        }

        if ( ((qmgFOJN*)aGraph)->antiRightGraph != NULL )
        {
            IDE_TEST( removeSDF( aStatement, ((qmgFOJN*)aGraph)->antiRightGraph )
                      != IDE_SUCCESS );
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

    // fix BUG-19147
    // aGraph가 QMG_PARTITION일 경우, aGraph->children이 달린다.
    for( sChildren = aGraph->children;
         sChildren != NULL;
         sChildren = sChildren->next )
    {
        IDE_TEST( removeSDF( aStatement, sChildren->childGraph )
                  != IDE_SUCCESS );
    }

    if( (aGraph->left == NULL) && (aGraph->right == NULL) && 
        (aGraph->children == NULL) && (aGraph->type == QMG_SELECTION) )
    {
        if( ((qmgSELT*)aGraph)->sdf != NULL )
        {
            IDE_TEST( qtc::removeSDF( aStatement,
                                      ((qmgSELT*)aGraph)->sdf )
                      != IDE_SUCCESS );
            ((qmgSELT*)aGraph)->sdf = NULL;
        }
        else
        {
            // Nothing to do...
        }
    }
    else
    {
        // Nothing to do...
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::makeLeftHASH4Join( qcStatement  * aStatement,
                        qmgGraph     * aGraph,
                        UInt           aMtrFlag,
                        UInt           aJoinFlag,
                        qtcNode      * aFilter,
                        qmnPlan      * aChild,
                        qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::makeLeftHASH4Join::__FT__" );

    if ( ( aJoinFlag &
           QMO_JOIN_METHOD_LEFT_STORAGE_MASK )
         == QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY )
    {
        aMtrFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKEHASH_MEMORY_TEMP_TABLE;
    }
    else
    {
        aMtrFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKEHASH_DISK_TEMP_TABLE;
    }

    switch( aGraph->type )
    {
        case QMG_INNER_JOIN:
        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
            IDE_TEST( qmoOneMtrPlan::makeHASH(
                          aStatement ,
                          aGraph->myQuerySet ,
                          aMtrFlag ,
                          ((qmgJOIN*)aGraph)->hashTmpTblCnt ,
                          ((qmgJOIN*)aGraph)->hashBucketCnt ,
                          aFilter ,
                          aChild ,
                          aPlan,
                          ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            break;
        case QMG_LEFT_OUTER_JOIN:
            IDE_TEST( qmoOneMtrPlan::makeHASH(
                          aStatement ,
                          aGraph->myQuerySet ,
                          aMtrFlag ,
                          ((qmgLOJN*)aGraph)->hashTmpTblCnt ,
                          ((qmgLOJN*)aGraph)->hashBucketCnt ,
                          aFilter ,
                          aChild ,
                          aPlan,
                          ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            break;
        case QMG_FULL_OUTER_JOIN:
            IDE_TEST( qmoOneMtrPlan::makeHASH(
                          aStatement ,
                          aGraph->myQuerySet ,
                          aMtrFlag ,
                          ((qmgFOJN*)aGraph)->hashTmpTblCnt ,
                          ((qmgFOJN*)aGraph)->hashBucketCnt ,
                          aFilter ,
                          aChild ,
                          aPlan,
                          ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            break;
        default:
            IDE_FT_ASSERT( 0 );
    }

    qmg::setPlanInfo( aStatement, aPlan, aGraph );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::makeRightHASH4Join( qcStatement  * aStatement,
                         qmgGraph     * aGraph,
                         UInt           aMtrFlag,
                         UInt           aJoinFlag,
                         qtcNode      * aRange,
                         qmnPlan      * aChild,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::makeRightHASH4Join::__FT__" );

    if ( ( aJoinFlag &
           QMO_JOIN_METHOD_RIGHT_STORAGE_MASK )
         == QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY )
    {
        aMtrFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKEHASH_MEMORY_TEMP_TABLE;
    }
    else
    {
        aMtrFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKEHASH_DISK_TEMP_TABLE;
    }

    switch( aGraph->type )
    {
        case QMG_INNER_JOIN:
        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
            IDE_TEST( qmoOneMtrPlan::makeHASH(
                          aStatement ,
                          aGraph->myQuerySet ,
                          aMtrFlag ,
                          ((qmgJOIN*)aGraph)->hashTmpTblCnt ,
                          ((qmgJOIN*)aGraph)->hashBucketCnt ,
                          aRange ,
                          aChild ,
                          aPlan,
                          ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            break;
        case QMG_LEFT_OUTER_JOIN:
            IDE_TEST( qmoOneMtrPlan::makeHASH(
                          aStatement ,
                          aGraph->myQuerySet ,
                          aMtrFlag ,
                          ((qmgLOJN*)aGraph)->hashTmpTblCnt ,
                          ((qmgLOJN*)aGraph)->hashBucketCnt ,
                          aRange ,
                          aChild ,
                          aPlan,
                          ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            break;
        case QMG_FULL_OUTER_JOIN:
            IDE_TEST( qmoOneMtrPlan::makeHASH(
                          aStatement ,
                          aGraph->myQuerySet ,
                          aMtrFlag ,
                          ((qmgFOJN*)aGraph)->hashTmpTblCnt ,
                          ((qmgFOJN*)aGraph)->hashBucketCnt ,
                          aRange ,
                          aChild ,
                          aPlan,
                          ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            break;
        default:
            IDE_FT_ASSERT( 0 );
    }

    qmg::setPlanInfo( aStatement, aPlan, aGraph );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::initLeftSORT4Join( qcStatement  * aStatement,
                        qmgGraph     * aGraph,
                        UInt           aJoinFlag,
                        qtcNode      * aRange,
                        qmnPlan      * aParent,
                        qmnPlan     ** aPlan )
{

    IDU_FIT_POINT_FATAL( "qmg::initLeftSORT4Join::__FT__" );

    switch( aJoinFlag & QMO_JOIN_LEFT_NODE_MASK )
    {
        case QMO_JOIN_LEFT_NODE_NONE:
            *aPlan = aParent;
            break;
        case QMO_JOIN_LEFT_NODE_STORE:
        case QMO_JOIN_LEFT_NODE_SORTING:
            if ( ( ( aGraph->myQuerySet->SFWGH->lflag & QMV_SFWGH_JOIN_MASK )
                   == QMV_SFWGH_JOIN_RIGHT_OUTER ) ||
                 ( ( aGraph->myQuerySet->SFWGH->lflag & QMV_SFWGH_JOIN_MASK )
                   == QMV_SFWGH_JOIN_FULL_OUTER ) )
            {
                qmc::disableSealTrueFlag( aParent->resultDesc );
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( qmoOneMtrPlan::initSORT(
                          aStatement,
                          aGraph->myQuerySet,
                          aRange,
                          ID_TRUE,
                          ID_FALSE,
                          aParent,
                          aPlan )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::makeLeftSORT4Join( qcStatement  * aStatement,
                        qmgGraph     * aGraph,
                        UInt           aMtrFlag,
                        UInt           aJoinFlag,
                        qmnPlan      * aChild,
                        qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : Sort Join시, Join의 왼쪽에 위치할 SORT plan node 생성
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::makeLeftSORT4Join::__FT__" );

    if ( ( aJoinFlag & QMO_JOIN_METHOD_LEFT_STORAGE_MASK )
         == QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY )
    {
        aMtrFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKESORT_MEMORY_TEMP_TABLE;
    }
    else
    {
        aMtrFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKESORT_DISK_TEMP_TABLE;
    }

    switch ( aJoinFlag & QMO_JOIN_LEFT_NODE_MASK )
    {
        case QMO_JOIN_LEFT_NODE_NONE :
            // 아무런 노드도 생성하지 않는다.
            break;
        case QMO_JOIN_LEFT_NODE_STORE :

            aMtrFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
            aMtrFlag |= QMO_MAKESORT_PRESERVED_TRUE;

            IDE_TEST(
                qmoOneMtrPlan::makeSORT( aStatement ,
                                         aGraph->myQuerySet ,
                                         aMtrFlag ,
                                         aGraph->left->preservedOrder ,
                                         aChild ,
                                         aGraph->costInfo.inputRecordCnt,
                                         aPlan ) != IDE_SUCCESS);

            aGraph->myPlan = aPlan;

            qmg::setPlanInfo( aStatement, aPlan, aGraph );

            break;
        case QMO_JOIN_LEFT_NODE_SORTING :

            aMtrFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
            aMtrFlag |= QMO_MAKESORT_PRESERVED_FALSE;

            IDE_TEST(
                qmoOneMtrPlan::makeSORT( aStatement ,
                                         aGraph->myQuerySet ,
                                         aMtrFlag ,
                                         aGraph->left->preservedOrder ,
                                         aChild ,
                                         aGraph->costInfo.inputRecordCnt,
                                         aPlan ) != IDE_SUCCESS);

            aGraph->myPlan = aPlan;

            qmg::setPlanInfo( aStatement, aPlan, aGraph );

            break;
        default:
            IDE_DASSERT(0);
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::initRightSORT4Join( qcStatement  * aStatement,
                         qmgGraph     * aGraph,
                         UInt           aJoinFlag,
                         idBool         aOrderCheckNeed,
                         qtcNode      * aRange,
                         idBool         aIsRangeSearch,
                         qmnPlan      * aParent,
                         qmnPlan     ** aPlan )
{

    IDU_FIT_POINT_FATAL( "qmg::initRightSORT4Join::__FT__" );

    if( aOrderCheckNeed == ID_TRUE )
    {
        switch( aJoinFlag & QMO_JOIN_RIGHT_NODE_MASK )
        {
            case QMO_JOIN_RIGHT_NODE_NONE :
                if( ( aGraph->type == QMG_INNER_JOIN ) ||
                    ( aGraph->type == QMG_SEMI_JOIN ) ||
                    ( aGraph->type == QMG_ANTI_JOIN ) )
                {
                    IDE_TEST_RAISE(
                        ((qmgJOIN*)aGraph)->selectedJoinMethod == NULL,
                        ERR_INVALID_INNER_JOIN_METHOD );
                    IDE_TEST_RAISE(
                        ((((qmgJOIN*)aGraph)->selectedJoinMethod->flag &
                          QMO_JOIN_METHOD_MASK)
                         == QMO_JOIN_METHOD_ONE_PASS_SORT) ||
                        ((((qmgJOIN*)aGraph)->selectedJoinMethod->flag &
                          QMO_JOIN_METHOD_MASK)
                         == QMO_JOIN_METHOD_TWO_PASS_SORT),
                        ERR_INVALID_INNER_JOIN_METHOD_SORT );
                }
                else if( aGraph->type == QMG_LEFT_OUTER_JOIN )
                {
                    IDE_TEST_RAISE(
                        ((qmgLOJN*)aGraph)->selectedJoinMethod == NULL,
                        ERR_INVALID_LEFT_OUTER_JOIN_METHOD );
                    IDE_TEST_RAISE(
                        ((((qmgLOJN*)aGraph)->selectedJoinMethod->flag &
                          QMO_JOIN_METHOD_MASK)
                         == QMO_JOIN_METHOD_ONE_PASS_SORT) ||
                        ((((qmgLOJN*)aGraph)->selectedJoinMethod->flag &
                          QMO_JOIN_METHOD_MASK)
                         == QMO_JOIN_METHOD_TWO_PASS_SORT),
                        ERR_INVALID_LEFT_OUTER_JOIN_METHOD_SORT );
                }
                else if( aGraph->type == QMG_FULL_OUTER_JOIN )
                {
                    IDE_TEST_RAISE(
                        ((qmgFOJN*)aGraph)->selectedJoinMethod == NULL,
                        ERR_INVALID_FULL_OUTER_JOIN_METHOD );
                    IDE_TEST_RAISE(
                        ((((qmgFOJN*)aGraph)->selectedJoinMethod->flag &
                          QMO_JOIN_METHOD_MASK)
                         == QMO_JOIN_METHOD_ONE_PASS_SORT) ||
                        ((((qmgFOJN*)aGraph)->selectedJoinMethod->flag &
                          QMO_JOIN_METHOD_MASK)
                         == QMO_JOIN_METHOD_TWO_PASS_SORT),
                        ERR_INVALID_FULL_OUTER_JOIN_METHOD_SORT );
                }
                *aPlan = aParent;
                break;
            case QMO_JOIN_RIGHT_NODE_STORE:
            case QMO_JOIN_RIGHT_NODE_SORTING:
                qmc::disableSealTrueFlag( aParent->resultDesc );
                IDE_TEST( qmoOneMtrPlan::initSORT(
                              aStatement,
                              aGraph->myQuerySet,
                              aRange,
                              ID_FALSE,
                              aIsRangeSearch,
                              aParent,
                              aPlan )
                          != IDE_SUCCESS );
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }
    else
    {
        IDE_TEST( qmoOneMtrPlan::initSORT(
                      aStatement,
                      aGraph->myQuerySet,
                      aRange,
                      ID_FALSE,
                      aIsRangeSearch,
                      aParent,
                      aPlan )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_INNER_JOIN_METHOD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeRightSORT4Join",
                                  "INNER JOIN: selectedJoinMethod is NULL" ));
    }
    IDE_EXCEPTION( ERR_INVALID_INNER_JOIN_METHOD_SORT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeRightSORT4Join",
                                  "INNER JOIN: selectedJoinMethod is SORT" ));
    }
    IDE_EXCEPTION( ERR_INVALID_LEFT_OUTER_JOIN_METHOD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeRightSORT4Join",
                                  "LEFT OUTER JOIN: selectedJoinMethod is NULL" ));
    }
    IDE_EXCEPTION( ERR_INVALID_LEFT_OUTER_JOIN_METHOD_SORT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeRightSORT4Join",
                                  "LEFT OUTER JOIN: selectedJoinMethod is SORT" ));
    }
    IDE_EXCEPTION( ERR_INVALID_FULL_OUTER_JOIN_METHOD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeRightSORT4Join",
                                  "FULL OUTER JOIN: selectedJoinMethod is NULL" ));
    }
    IDE_EXCEPTION( ERR_INVALID_FULL_OUTER_JOIN_METHOD_SORT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeRightSORT4Join",
                                  "FULL OUTER JOIN: selectedJoinMethod is SORT" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::makeRightSORT4Join( qcStatement  * aStatement,
                         qmgGraph     * aGraph,
                         UInt           aMtrFlag,
                         UInt           aJoinFlag,
                         idBool         aOrderCheckNeed,
                         qmnPlan      * aChild,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : Sort Join시, Join의 오른쪽에 위치할 SORT plan node 생성
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::makeRightSORT4Join::__FT__" );
    
    //저장 매체의 선택
    if ( ( aJoinFlag &
           QMO_JOIN_METHOD_RIGHT_STORAGE_MASK )
         == QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY )
    {
        aMtrFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKESORT_MEMORY_TEMP_TABLE;
    }
    else
    {
        aMtrFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
        aMtrFlag |= QMO_MAKESORT_DISK_TEMP_TABLE;
    }

    if( aOrderCheckNeed == ID_TRUE )
    {
        switch ( aJoinFlag & QMO_JOIN_RIGHT_NODE_MASK )
        {
            case QMO_JOIN_RIGHT_NODE_NONE :
                break;
            case QMO_JOIN_RIGHT_NODE_STORE :

                aMtrFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
                aMtrFlag |= QMO_MAKESORT_PRESERVED_TRUE;

                IDE_TEST(
                    qmoOneMtrPlan::makeSORT( aStatement ,
                                             aGraph->myQuerySet ,
                                             aMtrFlag ,
                                             aGraph->right->preservedOrder ,
                                             aChild ,
                                             aGraph->costInfo.inputRecordCnt,
                                             aPlan )
                    != IDE_SUCCESS);
                aGraph->myPlan = aPlan;

                qmg::setPlanInfo( aStatement, aPlan, aGraph );

                break;
            case QMO_JOIN_RIGHT_NODE_SORTING :

                aMtrFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
                aMtrFlag |= QMO_MAKESORT_PRESERVED_FALSE;

                IDE_TEST(
                    qmoOneMtrPlan::makeSORT( aStatement ,
                                             aGraph->myQuerySet ,
                                             aMtrFlag ,
                                             NULL ,
                                             aChild ,
                                             aGraph->costInfo.inputRecordCnt,
                                             aPlan )
                    != IDE_SUCCESS);

                aGraph->myPlan = aPlan;

                qmg::setPlanInfo( aStatement, aPlan, aGraph );

                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }
    else
    {
        IDE_TEST( qmoOneMtrPlan::makeSORT( aStatement ,
                                           aGraph->myQuerySet ,
                                           aMtrFlag ,
                                           NULL ,
                                           aChild ,
                                           aGraph->costInfo.inputRecordCnt,
                                           aPlan ) != IDE_SUCCESS);

        qmg::setPlanInfo( aStatement, aPlan, aGraph );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmg::usableIndexScanHint( qcmIndex            * aHintIndex,
                          qmoTableAccessType    aHintAccessType,
                          qmoIdxCardInfo      * aIdxCardInfo,
                          UInt                  aIdxCnt,
                          qmoIdxCardInfo     ** aSelectedIdxInfo,
                          idBool              * aUsableHint )
{
/***********************************************************************
 *
 * Description : 사용 가능한 INDEX SCAN Hint인지 검사
 *
 * Implementation :
 *    (1) Hint Index가 존재하는지 검사
 *    (2) 해당 index에 이미 다른 Hint가 적용되었는지 검사
 *
 ***********************************************************************/

    qmoIdxCardInfo * sSelected = NULL;
    idBool           sUsableIndexHint = ID_FALSE;
    UInt             i;

    IDU_FIT_POINT_FATAL( "qmg::usableIndexScanHint::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    //---------------------------------------------------
    // 해당 Index가 존재하는지 검사
    //---------------------------------------------------

    if( aHintIndex != NULL )
    {
        for ( i = 0; i < aIdxCnt; i++ )
        {
            // PROJ-1502 PARTITIONED DISK TABLE
            // index 포인터로 비교하지 않고 index id가 같은 것으로 비교한다.
            // 그 이유는, partitioned table의 index를 hint로 주었을 때
            // partition의 index가 선택되어야 하기 때문이다.
            if ( aIdxCardInfo[i].index->indexId == aHintIndex->indexId )
            {
                // Index Hint와 동일한 index가 존재하는 경우
                if ( aIdxCardInfo[i].index->isOnlineTBS == ID_TRUE )
                {
                    sSelected = & aIdxCardInfo[i];
                    break;
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

    if ( sSelected != NULL )
    {
        //---------------------------------------------------
        // 해당 Index가 존재할 경우
        //---------------------------------------------------

        if ( ( sSelected->flag & QMO_STAT_CARD_IDX_HINT_MASK) ==
             QMO_STAT_CARD_IDX_HINT_NONE )
        {
            //---------------------------------------------------
            // 해당 Index에 대해 이전에 다른 Hint가 없었을 경우
            //---------------------------------------------------

            switch ( aHintAccessType )
            {
                case QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN :
                    sSelected->flag &= ~QMO_STAT_CARD_IDX_HINT_MASK;
                    sSelected->flag |= QMO_STAT_CARD_IDX_NO_INDEX;

                    // 의미상으로 사용 가능한 Hint지만 No Index Hint의 경우,
                    // 해당 index의 access method 정보를 구축하지 않기 위해
                    sUsableIndexHint = ID_FALSE;
                    break;
                case QMO_ACCESS_METHOD_TYPE_INDEXACCESS_SCAN :
                    sSelected->flag &= ~QMO_STAT_CARD_IDX_HINT_MASK;
                    sSelected->flag |= QMO_STAT_CARD_IDX_INDEX;
                    sUsableIndexHint = ID_TRUE;
                    break;
                case QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN :
                    sSelected->flag &= ~QMO_STAT_CARD_IDX_HINT_MASK;
                    sSelected->flag |= QMO_STAT_CARD_IDX_INDEX_ASC;
                    sUsableIndexHint = ID_TRUE;
                    break;
                case QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN :
                    sSelected->flag &= ~QMO_STAT_CARD_IDX_HINT_MASK;
                    sSelected->flag |= QMO_STAT_CARD_IDX_INDEX_DESC;
                    sUsableIndexHint = ID_TRUE;
                    break;
                default :
                    IDE_DASSERT( 0 );
                    break;
            }
        }
        else
        {
            // 이전에 다른 Hint가 존재했을 경우
            sUsableIndexHint = ID_FALSE;
        }
    }
    else
    {
        // 해당 Index가 존재하지 않을 경우
        sUsableIndexHint = ID_FALSE;
    }

    *aSelectedIdxInfo = sSelected;
    *aUsableHint = sUsableIndexHint;

    return IDE_SUCCESS;
}

IDE_RC qmg::resetColumnLocate( qcStatement * aStatement, UShort aTupleID )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2179
 *    tuple-set에 columnLocate의 값들을 초기상태로 되돌린다.
 *    Disjunctive query 사용 시 CONCATENATOR의 child들의 생성 시
 *    left 생성후 right 생성 전 호출된다.
 *    Left에서 HASH등이 생성되는 경우 원래 table의 SCAN ID가 아닌 HASH의
 *    ID를 right생성 시 잘못 참조할 수 있기 때문이다.
 *
 *    BUG-37324
 *    외부 참조 컬럼에 대해서는 디펜던시를 reset 하면 안됨
 * Implementation :
 *    입력받은 aTupleID 에 columnLocate가 할당되어있는 경우
 *    초기값으로 되돌린다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::resetColumnLocate::__FT__" );

    mtcTemplate * sMtcTemplate;
    UShort        i;

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    for( i = 0; i < sMtcTemplate->rows[aTupleID].columnCount; i++ )
    {
        if( sMtcTemplate->rows[aTupleID].columnLocate != NULL )
        {
            sMtcTemplate->rows[aTupleID].columnLocate[i].table  = aTupleID;
            sMtcTemplate->rows[aTupleID].columnLocate[i].column = i;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC qmg::setColumnLocate( qcStatement * aStatement,
                             qmcMtrNode  * aMtrNode )
{
    qmcMtrNode        * sLastMtrNode;
    qmcMtrNode        * sMtrNode;
    mtcTemplate       * sMtcTemplate;
    qcTemplate        * sQcTemplate;
    qtcNode           * sConversionNode;
    idBool              sPushProject;

    IDU_FIT_POINT_FATAL( "qmg::setColumnLocate::__FT__" );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;
    sQcTemplate  = QC_SHARED_TMPLATE(aStatement);
    
    IDE_FT_ASSERT( aMtrNode != NULL );

    for( sLastMtrNode = aMtrNode;
         sLastMtrNode != NULL;
         sLastMtrNode = sLastMtrNode->next )
    {
        for( sMtrNode = aMtrNode;
             sMtrNode != NULL;
             sMtrNode = sMtrNode->next )
        {
            if( ( sMtrNode->flag & QMC_MTR_CHANGE_COLUMN_LOCATE_MASK )
                == QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE )
            {
                if( ( sMtrNode->srcNode->node.table ==
                      sLastMtrNode->srcNode->node.table )
                    &&
                    ( sMtrNode->srcNode->node.column ==
                      sLastMtrNode->srcNode->node.column ) )
                {
                    break;            
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
        }

        sConversionNode =
            (qtcNode*)mtf::convertedNode(
                &(sLastMtrNode->srcNode->node),
                &(QC_SHARED_TMPLATE(aStatement)->tmplate) );

        if( sLastMtrNode->srcNode == sConversionNode )
        {
            if( sLastMtrNode != sMtrNode ) 
            {
                // Nothing to do.
            }
            else
            {
                sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].
                    columnLocate[sLastMtrNode->srcNode->node.column].table
                    = sLastMtrNode->dstNode->node.table;                
                sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].      
                    columnLocate[sLastMtrNode->srcNode->node.column].column
                    = sLastMtrNode->dstNode->node.column;
            }
        }
        else
        {
            //------------------------------------------------------
            // srcNode에 conversion node가 있어 이 계산된 값을
            // dstNode에 저장하는 경우로,
            // 상위 노드에서 이 노드의 값을 이용하도록 해야 함.
            // base column의 column locate는 변경하면 안됨.
            // 예 ) TC/Server/qp4/Bugs/PR-13286/PR-13286.sql
            //      SELECT /*+ USE_ONE_PASS_SORT( D2, D1 ) */
            //      * FROM D1, D2
            //      WHERE D1.I1 > D2.I1 AND D1.I1 < D2.I1 + '10';
            //------------------------------------------------------

            if( sLastMtrNode != sMtrNode )
            {
                // Nothing To Do 
            }
            else
            {
                sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].
                    columnLocate[sLastMtrNode->srcNode->node.column].table
                    = sLastMtrNode->dstNode->node.table;                
                sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].
                    columnLocate[sLastMtrNode->srcNode->node.column].column
                    = sLastMtrNode->dstNode->node.column;
            }                

            sMtcTemplate->rows[sLastMtrNode->dstNode->node.table].
                columns[sLastMtrNode->dstNode->node.column].column.flag
                &= ~SMI_COLUMN_TYPE_MASK;
            sMtcTemplate->rows[sLastMtrNode->dstNode->node.table].
                columns[sLastMtrNode->dstNode->node.column].column.flag
                |= SMI_COLUMN_TYPE_FIXED;

            // BUG-38494
            // Compressed Column 역시 값 자체가 저장되므로
            // Compressed 속성을 삭제한다
            sMtcTemplate->rows[sLastMtrNode->dstNode->node.table].
                columns[sLastMtrNode->dstNode->node.column].column.flag
                &= ~SMI_COLUMN_COMPRESSION_MASK;

            sMtcTemplate->rows[sLastMtrNode->dstNode->node.table].
                columns[sLastMtrNode->dstNode->node.column].column.flag
                |= SMI_COLUMN_COMPRESSION_FALSE;        
        }        

        // fix BUG-22068
        if( ( ( sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].lflag
                & MTC_TUPLE_TYPE_MASK ) == MTC_TUPLE_TYPE_TABLE ) ||
            ( ( sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].lflag
                & MTC_TUPLE_VIEW_MASK ) == MTC_TUPLE_VIEW_TRUE ) )
        {
            // TABLE type
            // RID 방식과 Push Projection 방식이 함께 쓰여도
            // 원래 테이블의 수행 방식은 바뀌지 않는다. ( BUG-22068, BUG-31873 )
            // ex ) RID 방식 t1, Push Projection 방식 t2
            //      SELECT t1.i1, t2.i1 FROM t1, t2 ORDER BY t1.i1, t2.i1;
            //      ORDER BY 처리를 위하여 중간 결과를 저장할때,
            //      t1 테이블의 칼럼들은 RID 방식으로
            //      t2 테이블의 칼럼들은 Push Projection 방식으로 저장된다.
        }
        else
        {
            // CONSTANT, VARIABLE, INTERMEDIATE type
            // 위의 tuple들은 RID 타입으로 초기화된다.
            // 하지만 Push Projection 방식일때는 위 tuple의 타입도 Push Projection
            // 타입으로 변경한다.
            
            // BUG-28212
            // sysdate, unix_date, current_date 타입은 statement의 tmplate에 설정된다.
            // 그리고 그 값은 RID 타입으로 접근해야 한다.

            sPushProject = ID_TRUE;

            if( sQcTemplate->unixdate != NULL )
            {
                if( sQcTemplate->unixdate->node.table ==
                    sLastMtrNode->srcNode->node.table )
                {
                    sPushProject = ID_FALSE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }

            if( sQcTemplate->sysdate != NULL )
            {
                if( sQcTemplate->sysdate->node.table ==
                    sLastMtrNode->srcNode->node.table )
                {
                    sPushProject = ID_FALSE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }

            if( sQcTemplate->currentdate != NULL )
            {
                if( sQcTemplate->currentdate->node.table ==
                    sLastMtrNode->srcNode->node.table )
                {
                    sPushProject = ID_FALSE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }

            if ( sPushProject == ID_TRUE )
            {
                sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].lflag
                    &= ~MTC_TUPLE_MATERIALIZE_MASK;
                sMtcTemplate->rows[sLastMtrNode->srcNode->node.table].lflag
                    |= MTC_TUPLE_MATERIALIZE_VALUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }

        sMtcTemplate->rows[sLastMtrNode->dstNode->node.table].lflag
            &= ~MTC_TUPLE_MATERIALIZE_MASK;
        sMtcTemplate->rows[sLastMtrNode->dstNode->node.table].lflag
            |= MTC_TUPLE_MATERIALIZE_VALUE;
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC qmg::changeColumnLocate( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qtcNode      * aNode,
                                UShort         aTupleID,
                                idBool         aNext )
{
/***********************************************************************
 *
 * Description : validation시 설정된 노드의 기본위치정보를
 *               실제로 참조해야 할 변경된 위치정보로 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode        * sNode;
    UShort           sOrgTable;
    UShort           sOrgColumn;
    UShort           sChangeTable;
    UShort           sChangeColumn;

    IDU_FIT_POINT_FATAL( "qmg::changeColumnLocate::__FT__" );

    if( aNode == NULL )        
    {
        // Nothing To Do
    }
    else
    {
        if( ( aNode->node.lflag & MTC_NODE_COLUMN_LOCATE_CHANGE_MASK )
            == MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE )
        {
            // Nothing To Do
        }
        else
        {
            if( aNext == ID_FALSE )        
            {
                //---------------------------------------------------
                // target column등의 node 처리시
                // node->next 는 따라가지 않도록 한다.
                //---------------------------------------------------
        
                if( aNode->node.arguments != NULL )
                {
                    sNode = (qtcNode*)(aNode->node.arguments);
                }
                else
                {
                    sNode = NULL;
                }        
            }
            else
            {
                sNode = aNode;        
            }

            for( ; sNode != NULL; sNode = (qtcNode*)(sNode->node.next) )
            {
                if( ( sNode->node.lflag & MTC_NODE_COLUMN_LOCATE_CHANGE_MASK )
                    == MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE )
                {
                    // BUG-43723 disk table에서 merge join 수행 결과가 다릅니다.
                    // MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE 여도 next는 확인해야 한다.
                }
                else
                {
                    IDE_TEST( changeColumnLocateInternal( aStatement,
                                                          aQuerySet,
                                                          sNode,
                                                          aTupleID )
                              != IDE_SUCCESS );
                }
            }

            if( aNext == ID_FALSE )        
            {
                /*
                 * BUG-39605
                 * procedure variable 일 경우에도 changeColumnLocate 하지 않는다.
                 */
                if ((aNode->node.column != MTC_RID_COLUMN_ID) &&
                    (aNode->node.table != QC_SHARED_TMPLATE(aStatement)->tmplate.variableRow))
                {
                    sOrgTable = aNode->node.table;
                    sOrgColumn = aNode->node.column;

                    IDE_TEST(findColumnLocate(aStatement,
                                              aQuerySet->parentTupleID,
                                              aTupleID,
                                              sOrgTable,
                                              sOrgColumn,
                                              &sChangeTable,
                                              &sChangeColumn)
                             != IDE_SUCCESS);        

                    aNode->node.table = sChangeTable;
                    aNode->node.column = sChangeColumn;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing To Do 
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}


IDE_RC qmg::changeColumnLocateInternal( qcStatement  * aStatement,
                                        qmsQuerySet  * aQuerySet,
                                        qtcNode      * aNode,
                                        UShort         aTupleID )
{
/***********************************************************************
 *
 * Description : validation시 설정된 노드의 기본위치정보를
 *               실제로 참조해야 할 변경된 위치정보로 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode     * sNode;
    UShort        sOrgTable;
    UShort        sOrgColumn;
    UShort        sChangeTable;
    UShort        sChangeColumn;

    IDU_FIT_POINT_FATAL( "qmg::changeColumnLocateInternal::__FT__" );

    for( sNode = (qtcNode*)(aNode->node.arguments);
         sNode != NULL;
         sNode = (qtcNode*)(sNode->node.next) )
    {
        if( ( sNode->node.lflag & MTC_NODE_COLUMN_LOCATE_CHANGE_MASK )
            == MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE )
        {
            // BUG-43723 disk table에서 merge join 수행 결과가 다릅니다.
            // MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE 여도 next는 확인해야 한다.
        }
        else
        {
            IDE_TEST( changeColumnLocateInternal( aStatement,
                                                  aQuerySet,
                                                  sNode,
                                                  aTupleID )
                      != IDE_SUCCESS );
        }
    }

    /*
     * BUG-39605
     * procedure variable 일 경우에도 changeColumnLocate 하지 않는다.
     */
    if ((aNode->node.column != MTC_RID_COLUMN_ID) &&
        (aNode->node.table != QC_SHARED_TMPLATE(aStatement)->tmplate.variableRow))
    {
        sOrgTable = aNode->node.table;
        sOrgColumn = aNode->node.column;

        IDE_TEST( findColumnLocate( aStatement,
                                    aQuerySet->parentTupleID,
                                    aTupleID,
                                    sOrgTable,
                                    sOrgColumn,
                                    & sChangeTable,
                                    & sChangeColumn )
                  != IDE_SUCCESS );

        aNode->node.table = sChangeTable;
        aNode->node.column = sChangeColumn;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC qmg::findColumnLocate( qcStatement  * aStatement,
                              UInt           aParentTupleID,
                              UShort         aTableID,
                              UShort         aOrgTable,
                              UShort         aOrgColumn,
                              UShort       * aChangeTable,
                              UShort       * aChangeColumn )
{
/***********************************************************************
 *
 * Description : 
 *    PROJ-2179
 *    기존의 findColumnLocate와 달리 추가 인자로 aParentTupleID를 갖는다.
 *    Subquery의 execution plan에서 상위 operator로 materialize된 값을
 *    참조하는것을 방지하도록 한다
 *
 *    Ex) SELECT i1, (SELECT COUNT(*) FROM t2 WHERE t2.i1 = t1.i1)
 *            FROM t1 ORDER BY 2, 1;
 *        * t1, t2는 모두 disk table
 *
 *        위 SQL구문의 경우 outerquery에서 SORT를 생성하고 sort에서
 *        t3.i1과 subquery의 결과를 materialize한다.
 *        이 때 subquery의 WHERE절에서 t1.i1을 outerquery의 SORT에서
 *        materialize된 위치를 가리키지 않도록 SORT의 ID를 aParentTupleID
 *        로 설정해준다.
 *
 * Implementation :
 *    aParentTupleID를 columnLocate에서 가리키는 경우 이를 무시하고
 *    이전까지 찾은 위치를 반환한다.
 *
 ***********************************************************************/

    UShort        sTable;
    UShort        sColumn;

    IDU_FIT_POINT_FATAL( "qmg::findColumnLocate::__FT__" );

    if( aOrgTable != aTableID )
    {
        sTable = QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[aOrgTable].columnLocate[aOrgColumn].table;
        sColumn = QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[aOrgTable].columnLocate[aOrgColumn].column;

        if( ( sTable < aTableID ) &&
            ( aParentTupleID != sTable ) )
        {
            if( ( sTable == aOrgTable ) &&
                ( sColumn == aOrgColumn ) )
            {
                *aChangeTable = sTable;
                *aChangeColumn = sColumn;
            }
            else
            {
                IDE_TEST( findColumnLocate( aStatement,
                                            aTableID,
                                            sTable,
                                            sColumn,
                                            aChangeTable,
                                            aChangeColumn )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // tuple이 아직 할당되지 않은 상태.
            // mtrNode 구성시, 질의에 사용된 컬럼들을 저장하는데.....
            *aChangeTable = aOrgTable;
            *aChangeColumn = aOrgColumn;
        }
    }
    else
    {
        // tuple이 아직 할당되지 않은 상태.
        // mtrNode 구성시, 질의에 사용된 컬럼들을 저장하는데.....
        *aChangeTable = aOrgTable;
        *aChangeColumn = aOrgColumn;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;    
}


IDE_RC qmg::findColumnLocate( qcStatement  * aStatement,
                              UShort         aTableID,
                              UShort         aOrgTable,
                              UShort         aOrgColumn,
                              UShort       * aChangeTable,
                              UShort       * aChangeColumn )
{
    UShort sTable;
    UShort sColumn;

    IDU_FIT_POINT_FATAL( "qmg::findColumnLocate::__FT__" );

    if( aOrgTable != aTableID )
    {
        IDE_DASSERT( aOrgColumn != MTC_RID_COLUMN_ID );

        sTable = QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[aOrgTable].columnLocate[aOrgColumn].table;
        sColumn = QC_SHARED_TMPLATE(aStatement)->tmplate.
            rows[aOrgTable].columnLocate[aOrgColumn].column;

        if( sTable != aTableID )
        {
            if( ( sTable == aOrgTable ) &&
                ( sColumn == aOrgColumn ) )
            {

                *aChangeTable = sTable;
                *aChangeColumn = sColumn;
            }
            else
            {
                IDE_TEST( findColumnLocate( aStatement,
                                            aTableID,
                                            sTable,
                                            sColumn,
                                            aChangeTable,
                                            aChangeColumn )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // tuple이 아직 할당되지 않은 상태.
            // mtrNode 구성시, 질의에 사용된 컬럼들을 저장하는데.....
            *aChangeTable = aOrgTable;
            *aChangeColumn = aOrgColumn;
        }
    }
    else
    {
        // tuple이 아직 할당되지 않은 상태.
        // mtrNode 구성시, 질의에 사용된 컬럼들을 저장하는데.....
        *aChangeTable = aOrgTable;
        *aChangeColumn = aOrgColumn;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;    
}

IDE_RC
qmg::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description :
 *    BUG-32258 
 *    When Preserved Order is NOT-DEFINED state, Sort-Join may misjudge
 *    sorting order. (ASC/DESC)
 *
 * Implementation :
 *    (1) Finalize left child graph's Preserved Order (Recursive)
 *    (2) Finalize right child graph's Preserved Order (Recursive)
 *    (3) If this graph's Preserved Order is NOT-DEFINED, finalize it.
 *    (3-1) Selection Graph : Set order to selected index's one.
 *    (3-2) Join Graph : 
 *    (3-3) Group Graph : 
 *    (3-4) Distinct Graph :
 *    (3-5) Windowing Graph : In this case, Preserved Order maybe DEFINED-NOT-
 *    (4) Return this graph's Preserved Order
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::finalizePreservedOrder::__FT__" );

    // (1) Finalize Left
    if( aGraph->left != NULL )
    {
        IDE_TEST( finalizePreservedOrder( aGraph->left ) != IDE_SUCCESS );
    }

    // (2) Finalize Right
    if( aGraph->right != NULL )
    {
        IDE_TEST( finalizePreservedOrder( aGraph->right ) != IDE_SUCCESS );
    }

    // (3) When Preserved Order's Direction is Not defined
    if ( aGraph->preservedOrder != NULL )
    {
        // Preserved Order has created
        if ( aGraph->preservedOrder->direction == QMG_DIRECTION_NOT_DEFINED )
        {
            // Direction is not fixed
            switch( aGraph->type )
            {
                case QMG_SELECTION :       // Selection Graph
                    IDE_TEST( qmgSelection::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_PARTITION :       // PROJ-1502 Partition Graph
                    IDE_TEST( qmgPartition::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;
                    
                case QMG_PROJECTION :      // Projection Graph
                    IDE_TEST( qmgProjection::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_DISTINCTION :     // Duplicate Elimination Graph
                    IDE_TEST( qmgDistinction::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_GROUPING :        // Grouping Graph
                    IDE_TEST( qmgGrouping::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_SORTING :         // Sorting Graph
                    // Sort graph always set direction
                    break;

                case QMG_WINDOWING :       // Windowing Graph
                    IDE_TEST( qmgWindowing::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_INNER_JOIN :      // Inner Join Graph
                case QMG_SEMI_JOIN :       // Semi Join Graph
                case QMG_ANTI_JOIN :       // Anti Join Graph
                case QMG_LEFT_OUTER_JOIN : // Left Outer Join Graph
                case QMG_FULL_OUTER_JOIN : // Full Outer Join Graph
                    // Join graph uses common function
                    IDE_TEST( qmgJoin::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_SET :             // SET Graph
                case QMG_HIERARCHY :       // Hierarchy Graph
                case QMG_DNF :             // DNF Graph
                case QMG_SHARD_SELECT:    // Shard Select Graph
                    // These graphs does not use Preserved order
                    break;

                case QMG_COUNTING :        // PROJ-1405 Counting Graph
                    IDE_TEST( qmgCounting::finalizePreservedOrder( aGraph )
                              != IDE_SUCCESS );
                    break;

                default :
                    // All kinds of node must be considered.
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            // Direction has defined.
            // Nothing to do.
        }
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
qmg::copyPreservedOrderDirection( qmgPreservedOrder * aDstOrder,
                                  qmgPreservedOrder * aSrcOrder )
{
/***********************************************************************
 *
 * Description : Copy direction info.
 *               Source must compatible to Dest.
 *
 * Implementation :
 *     Copy Direction
 *
 ***********************************************************************/
    
    qmgPreservedOrder * sSrcOrder;
    qmgPreservedOrder * sDstOrder;

    IDU_FIT_POINT_FATAL( "qmg::copyPreservedOrderDirection::__FT__" );
    
    // Source Preserved order로 direction을 copy 한다.
    for ( sDstOrder = aDstOrder,
              sSrcOrder = aSrcOrder;
          sDstOrder != NULL && sSrcOrder != NULL;
          sDstOrder = sDstOrder->next,
              sSrcOrder = sSrcOrder->next )
    {
        // Direction copy
        sDstOrder->direction = sSrcOrder->direction;
    }
    
    return IDE_SUCCESS;
    
}

idBool
qmg::isSamePreservedOrder( qmgPreservedOrder * aDstOrder,
                           qmgPreservedOrder * aSrcOrder )
{
/***********************************************************************
 *
 * Description : 두 preserved order가 같은지 검사
 *
 * Implementation :
 *         1. Compatability Check
 *         2. Check Preserved order size
 *
 ***********************************************************************/
    
    qmgPreservedOrder * sSrcOrder;
    qmgPreservedOrder * sDstOrder;
    idBool              sIsSame;

    sIsSame = ID_TRUE;

    // 1. Source와 Destination Preserved order가 동일한 컬럼을 의미하는지 검사한다.
    for ( sDstOrder = aDstOrder,
              sSrcOrder = aSrcOrder;
          sDstOrder != NULL && sSrcOrder != NULL;
          sDstOrder = sDstOrder->next,
              sSrcOrder = sSrcOrder->next )
    {
        if ( ( sDstOrder->table  == sSrcOrder->table ) &&
             ( sDstOrder->column == sSrcOrder->column ) )
        {
            // sIsCompat is ID_TRUE
        }
        else
        {
            sIsSame = ID_FALSE;
            break;
        }
    }

    // 2. Check Preserved order size
    if ( sDstOrder != NULL )
    {
        // Source가 Destination Preserved order를 모두 만족시키지 못함
        sIsSame = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    return sIsSame;
}

IDE_RC
qmg::makeDummyMtrNode( qcStatement  * aStatement ,
                       qmsQuerySet  * aQuerySet,
                       UShort         aTupleID,
                       UShort       * aColumnCount,
                       qmcMtrNode  ** aMtrNode )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2179 SORT에서 result descriptor가 비어있어있는 경우에 대비하여
 *    dummy 값을 materialize 하도록 한다.
 *
 * Implementation :
 *    CHAR형의 '1'을 materlialize하도록 node를 생성한다.
 *
 ***********************************************************************/

    qtcNode        * sConstNode[2];
    qcNamePosition   sPosition;

    IDU_FIT_POINT_FATAL( "qmg::makeDummyMtrNode::__FT__" );

    SET_EMPTY_POSITION( sPosition );

    IDE_TEST( qtc::makeValue( aStatement,
                              sConstNode,
                              (const UChar*)"CHAR",
                              4,
                              &sPosition,
                              (const UChar*)"1",
                              1,
                              MTC_COLUMN_NORMAL_LITERAL ) 
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sConstNode[0] )
              != IDE_SUCCESS );

    IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                      aQuerySet,
                                      sConstNode[0],
                                      ID_FALSE,
                                      aTupleID,
                                      0,
                                      aColumnCount,
                                      aMtrNode ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

idBool
qmg::getMtrMethod( qcStatement * aStatement,
                   UShort        aSrcTupleID,
                   UShort        aDstTupleID )
{
/***********************************************************************
 *
 * Description :
 *    어떤 방법으로 materialize 해야하는지 결정하여 반화한다.
 *    - Complete record : TRUE를 반환
 *    - Surrogate key   : FALSE를 반환
 *
 * Implementation :
 *    Source와 destination tuple의 flag와 종류를 보고 판단한다.
 *
 ***********************************************************************/

    mtcTemplate * sMtcTemplate;

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    if( ( ( sMtcTemplate->rows[aSrcTupleID].lflag & MTC_TUPLE_MATERIALIZE_MASK )
          == MTC_TUPLE_MATERIALIZE_VALUE ) &&
        ( ( sMtcTemplate->rows[aDstTupleID].lflag & MTC_TUPLE_MATERIALIZE_MASK )
          == MTC_TUPLE_MATERIALIZE_VALUE ) )
    {
        return ID_TRUE;
    }

    // View는 surrogate-key가 존재하지 않는다.
    if( ( sMtcTemplate->rows[aSrcTupleID].lflag & MTC_TUPLE_VIEW_MASK )
        == MTC_TUPLE_VIEW_TRUE )
    {
        return ID_TRUE;
    }

    // Expression을 위한 intermediate tuple인 경우
    if( ( ( sMtcTemplate->rows[aSrcTupleID].lflag & MTC_TUPLE_TYPE_MASK )
          == MTC_TUPLE_TYPE_INTERMEDIATE ) &&
        ( ( sMtcTemplate->rows[aSrcTupleID].lflag & MTC_TUPLE_PLAN_MTR_MASK )
          == MTC_TUPLE_PLAN_MTR_FALSE ) )
    {
        return ID_TRUE;
    }

    /* BUG-36468 for DB Link's Remote Table */
    if ( ( sMtcTemplate->rows[aSrcTupleID].lflag & MTC_TUPLE_STORAGE_LOCATION_MASK )
         == MTC_TUPLE_STORAGE_LOCATION_REMOTE )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

idBool
qmg::existBaseTable( qmcMtrNode * aMtrNode,
                     UInt         aFlag,
                     UShort       aTable )
{
    // aMtrNode에서 aFlag와 aTable을 갖는 node가 존재하는지 확인한다.

    qmcMtrNode * sMtrNode;

    for( sMtrNode = aMtrNode;
         sMtrNode != NULL;
         sMtrNode = sMtrNode->next )
    {
        if( ( sMtrNode->srcNode->node.table == aTable ) &&
            ( ( sMtrNode->flag & aFlag ) != 0 ) )
        {
            return ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    return ID_FALSE;
}

UInt
qmg::getBaseTableType( ULong aTupleFlag )
{
    // Tuple의 flag에 따라 base table의 materialize type을 반환한다.

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( ( aTupleFlag & MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK ) == MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE )
    {
        return QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE;
    }
    else
    {
        if ( ( aTupleFlag & MTC_TUPLE_STORAGE_MASK ) == MTC_TUPLE_STORAGE_MEMORY )
        {
            if ( ( aTupleFlag & MTC_TUPLE_PARTITIONED_TABLE_MASK ) == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
            {
                return QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE;
            }
            else
            {
                return QMC_MTR_TYPE_MEMORY_TABLE;
            }
        }
        else
        {
            if ( ( aTupleFlag & MTC_TUPLE_PARTITIONED_TABLE_MASK ) == MTC_TUPLE_PARTITIONED_TABLE_TRUE )
            {
                return QMC_MTR_TYPE_DISK_PARTITIONED_TABLE;
            }
            else
            {
                return QMC_MTR_TYPE_DISK_TABLE;
            }
        }
    }
}

idBool
qmg::isTempTable( ULong aTupleFlag )
{
    // 해당 tuple이 temp table의 tuple인지 여부를 반환한다.

    if( ( ( aTupleFlag & MTC_TUPLE_PLAN_MTR_MASK ) == MTC_TUPLE_PLAN_MTR_TRUE ) ||
        ( ( aTupleFlag & MTC_TUPLE_VSCN_MASK ) == MTC_TUPLE_VSCN_TRUE ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

idBool
qmg::isDatePseudoColumn( qcStatement * aStatement,
                         qtcNode     * aNode )
{
    qtcNode * sDateNode;
    idBool    sResult = ID_FALSE;

    if( ( aNode->lflag & QTC_NODE_SYSDATE_MASK ) == QTC_NODE_SYSDATE_EXIST )
    {
        if( QC_SHARED_TMPLATE( aStatement )->sysdate != NULL )
        {
            sDateNode = QC_SHARED_TMPLATE( aStatement )->sysdate;

            if( ( sDateNode->node.table  == aNode->node.table  ) &&
                ( sDateNode->node.column == aNode->node.column ) )
            {
                sResult = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        if( QC_SHARED_TMPLATE( aStatement )->unixdate != NULL )
        {
            sDateNode = QC_SHARED_TMPLATE( aStatement )->unixdate;

            if( ( sDateNode->node.table  == aNode->node.table  ) &&
                ( sDateNode->node.column == aNode->node.column ) )
            {
                sResult = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        if( QC_SHARED_TMPLATE( aStatement )->currentdate != NULL )
        {
            sDateNode = QC_SHARED_TMPLATE( aStatement )->currentdate;

            if( ( sDateNode->node.table  == aNode->node.table  ) &&
                ( sDateNode->node.column == aNode->node.column ) )
            {
                sResult = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

    }
    else
    {
        // Nothing to do.
    }
    
    return sResult;
}

// PROJ-2242 qmgGraph의 cost 정보를 qmnPlan 으로 복사함
void qmg::setPlanInfo( qcStatement  * aStatement,
                       qmnPlan      * aPlan,
                       qmgGraph     * aGraph )
{
    aPlan->qmgOuput   = aGraph->costInfo.outputRecordCnt;

    // insert, delete 구문에서는 mSysStat이 null 이된다.
    if( aStatement->mSysStat != NULL )
    {
        aPlan->qmgAllCost = aGraph->costInfo.totalAllCost /
            aStatement->mSysStat->singleIoScanTime;
    }
    else
    {
        aPlan->qmgAllCost = aGraph->costInfo.totalAllCost;
    }    
}

IDE_RC qmg::isolatePassNode( qcStatement * aStatement,
                             qtcNode     * aSource )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *    Source Node Tree를 Traverse하면서 passNode의 인자를 독립시킨다.
 *
 ***********************************************************************/

    qtcNode * sNewNode;

    IDU_FIT_POINT_FATAL( "qmg::isolatePassNode::__FT__" );

    IDE_FT_ASSERT( aSource != NULL );
    
    if( aSource->node.arguments != NULL )
    {
        if( ( aSource->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_SUBQUERY )
        {
            // Subquery노드일 경우엔 arguments를 복사하지 않는다.
        }
        else
        {
            if( qtc::dependencyEqual( & aSource->depInfo,
                                      & qtc::zeroDependencies ) == ID_FALSE )
            {
                IDE_TEST( isolatePassNode( aStatement,
                                           (qtcNode *)aSource->node.arguments )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do...
            }
        }
    }
    else
    {
        // Nothing to do.
    }
    
    // BUG-37355
    // argument가 passNode라면 qtcNode를 복사해서 더 이상 변경되지 않도록 한다.
    if( aSource->node.module == &qtc::passModule )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qtcNode, &sNewNode )
                  != IDE_SUCCESS );
        
        idlOS::memcpy( sNewNode, aSource->node.arguments, ID_SIZEOF(qtcNode) );

        aSource->node.arguments = (mtcNode*) sNewNode;
    }
    else
    {
        // Nothing to do.
    }

    if( aSource->node.next != NULL )
    {
        if( qtc::dependencyEqual( &((qtcNode*)aSource->node.next)->depInfo,
                                  & qtc::zeroDependencies ) == ID_FALSE )
        {
            IDE_TEST( isolatePassNode( aStatement,
                                       (qtcNode *)aSource->node.next )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getGraphLateralDepInfo( qmgGraph      * aGraph,
                                    qcDepInfo     * aGraphLateralDepInfo )
{
/*********************************************************************
 * 
 *  Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 *
 *   Graph의 lateralDepInfo를 반환한다.
 *  
 *  Implementation :
 *  - Graph가 SELECTION / PARTITION이라면 내부의 lateralDepInfo를 반환
 *  - 그 외의 Graph인 경우, 빈 depInfo를 반환
 * 
 ********************************************************************/

    IDU_FIT_POINT_FATAL( "qmg::getGraphLateralDepInfo::__FT__" );

    qtc::dependencyClear( aGraphLateralDepInfo );

    switch ( aGraph->type )
    {
        case QMG_SELECTION:
        case QMG_PARTITION:
        {
            IDE_DASSERT( aGraph->myFrom != NULL );
            IDE_TEST( qmvQTC::getFromLateralDepInfo( aGraph->myFrom, 
                                                     aGraphLateralDepInfo )
                      != IDE_SUCCESS );
            break;
        }
        default:
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::resetDupNodeToMtrNode( qcStatement * aStatement,
                                   UShort        aOrgTable,
                                   UShort        aOrgColumn,
                                   qtcNode     * aChangeNode,
                                   qtcNode     * aNode )
{
/*********************************************************************
 * 
 *  Description : BUG-43088 (PR-13286 diff)
 *                mtrNode 의 original 노드와 중복된 노드를 찾아
 *                mtrNode 값으로 변경하기 위함
 *  
 *  Implementation :
 *
 *          aNode tree 를 순회하며
 *          aOrgTable, aOrgColumn 과 동일한 table, column 을 찾아
 *          aChageNode 의 table, column 으로 변경한다.
 * 
 ********************************************************************/

    qtcNode * sNode = NULL;

    IDU_FIT_POINT_FATAL( "qmg::resetDupNodeToMtrNode::__FT__" );

    IDE_DASSERT( aStatement  != NULL );
    IDE_DASSERT( aChangeNode != NULL );
    IDE_DASSERT( aNode       != NULL );

    if ( QTC_IS_COLUMN( aStatement, aNode ) &&
         ( aNode->node.table  == aOrgTable ) &&
         ( aNode->node.column == aOrgColumn ) )
    {
        aNode->node.table  = aChangeNode->node.table;
        aNode->node.column = aChangeNode->node.column;
    }
    else
    {
        sNode = (qtcNode *)(aNode->node.arguments);

        while ( sNode != NULL )
        {
            IDE_TEST( resetDupNodeToMtrNode( aStatement,
                                             aOrgTable,
                                             aOrgColumn,
                                             aChangeNode,
                                             sNode )
                      != IDE_SUCCESS );

            sNode = (qtcNode *)(sNode->node.next);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::lookUpMaterializeGraph( qmgGraph * aGraph, idBool * aFound )
{
/***********************************************************************
 *
 * Description :
 *    BUG-43493 delay operator를 추가해 execution time을 줄인다.
 *
 * Implementation :
 *    graph의 left만 순회하며 materialize할만한(가능성 있는)
 *    operator가 있는지 찾아본다.
 *
 ***********************************************************************/

    qmgGraph * sGraph = aGraph;
    idBool     sFound = ID_FALSE;

    while ( sGraph != NULL )
    {
        switch( sGraph->type )
        {
            // materialize 가능성 있는 graph
            case QMG_DISTINCTION :
            case QMG_GROUPING :
            case QMG_SORTING :
            case QMG_WINDOWING :
            case QMG_SET :
            case QMG_HIERARCHY :
            case QMG_RECURSIVE_UNION_ALL :
                sFound = ID_TRUE;
                break;

            default :
                break;
        }

        if ( sFound == ID_TRUE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        sGraph = sGraph->left;
    }

    *aFound = sFound;
    
    return IDE_SUCCESS;
}

/* TASK-6744 */
void qmg::initializeRandomPlanInfo( qmgRandomPlanInfo * aRandomPlanInfo )
{
    aRandomPlanInfo->mWeightedValue   = 0;
    aRandomPlanInfo->mTotalNumOfCases = 0;
}

/* TASK-7219 */
IDE_RC qmg::getNodeOffset( qtcNode * aNode,
                           idBool    aIsRecursive,
                           SInt    * aStart,
                           SInt    * aEnd )
{
/****************************************************************************************
 *
 * Description : Node의 Query Text내의 시작 Offset과 마지막 Offset를 반환한다.
 *               Target, Order By, Where 절 복제 및 Shard 최적화시에 사용한다. 마지막
 *               Offset를 추출할 때에 Subquery 인 경우, 더이상 argument를 수색하지 않고,
 *               마지막 Offset를 반환한다. Target의 경우, 최상위 Node 정보로 충분하며,
 *               argument를 수색할 필요가 없다.
 *
 * Implementation : 1.1. 반환이 필요한 시작 Offset를 계산한다.
 *                  1.2. 시작 Offset를 기록한다.
 *                  1.3. " " 로. 인해서 보정이 필요한 경우 보정한다.
 *                  2.1. 반환이 필요한 마지막 Offset를 계산한다.
 *                  2.2. 마지막 Offset를 기록한다.
 *                  2.3. " " 로. 인해서 보정이 필요한 경우 보정한다.
 *                  2.4. argument가 있다면 재귀한다.
 *
 ****************************************************************************************/

    qtcNode * sNode  = NULL;
    SInt      sStart = 0;
    SInt      sEnd   = 0;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );

    /* 1.1. 반환이 필요한 시작 Offset를 계산한다. */
    if ( aStart != NULL )
    {
        /* 1.2. 시작 Offset를 기록한다. */
        sStart = aNode->position.offset;

        /* 1.3. " " 로. 인해서 보정이 필요한 경우 보정한다. */
        if ( aNode->position.stmtText[ sStart - 1 ] == '"' )
        {
            sStart--;
        }
        else
        {
            /* Nothing to do */
        }

        *aStart = sStart;
    }
    else
    {
        /* Nothing to do */
    }

    /* 2.1. 반환이 필요한 마지막 Offset를 계산한다. */
    if ( aEnd != NULL )
    {
        /* 2.2. 마지막 Offset를 기록한다. */
        sEnd = aNode->position.offset + aNode->position.size;

        /* 2.3. " " 로. 인해서 보정이 필요한 경우 보정한다. */
        if ( aNode->position.stmtText[ sEnd ] == '"' )
        {
            sEnd++;
        }
        else
        {
            /* Nothing to do */
        }

        /* 2.4. argument가 있다면 재귀한다. */
        if ( aIsRecursive == ID_TRUE )
        {
            for ( sNode  = (qtcNode *)(aNode->node.arguments);
                  sNode != NULL;
                  sNode  = (qtcNode *)(sNode->node.next) )
            {
                if ( ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                       != MTC_NODE_OPERATOR_SUBQUERY ) &&
                     ( sNode->node.next == NULL ) )
                {
                    IDE_TEST( getNodeOffset( sNode,
                                             aIsRecursive,
                                             NULL,
                                             &( sEnd ) )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }

        *aEnd = sEnd;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getNodeOffset",
                                  "node is null" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getFromEnd( qmsFrom * aFrom,
                        SInt    * aFromWhereEnd )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               From tree를 순회하며 query string의 마지막 위치에 해당하는 from의
 *               End position을 찾아 반환한다.
 *
 *               TASK-7219 에서 qmvShardTransform -> qmg 로 함수를 옮겼다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt sThisIsTheEnd = 0;

    if ( aFrom != NULL )
    {
        /* ON clause가 존재하면 on clause의 end position을 기록한다. */
        if ( aFrom->onCondition != NULL )
        {
            sThisIsTheEnd = aFrom->onCondition->position.offset + aFrom->onCondition->position.size;

            if ( aFrom->onCondition->position.stmtText[aFrom->onCondition->position.offset +
                                                       aFrom->onCondition->position.size] == '"' )
            {
                sThisIsTheEnd++;
            }
            else
            {
                /* Nothing to do */
            }
            if ( *aFromWhereEnd < sThisIsTheEnd )
            {
                /*
                 * From tree를 순회하던 도중 기록된 from의 end position보다
                 * 더 큰(더 뒤에 등장하는) end position일 경우 값을 갱신
                 */
                *aFromWhereEnd = sThisIsTheEnd;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* ON clause가 존재하지 않으면 tableRef의 end position을 찾는다. */
            IDE_DASSERT( aFrom->tableRef != NULL );

            if ( QC_IS_NULL_NAME( aFrom->tableRef->aliasName ) == ID_FALSE )
            {
                if ( aFrom->tableRef->aliasName.
                     stmtText[aFrom->tableRef->aliasName.offset +
                              aFrom->tableRef->aliasName.size] == '"' )
                {
                    sThisIsTheEnd = aFrom->tableRef->aliasName.offset +
                        aFrom->tableRef->aliasName.size + 1;
                }
                else if ( aFrom->tableRef->aliasName.
                          stmtText[aFrom->tableRef->aliasName.offset +
                                   aFrom->tableRef->aliasName.size] == '\'' )
                {
                    sThisIsTheEnd = aFrom->tableRef->aliasName.offset +
                        aFrom->tableRef->aliasName.size + 1;
                }
                else
                {
                    sThisIsTheEnd = aFrom->tableRef->aliasName.offset +
                        aFrom->tableRef->aliasName.size;
                }
            }
            else
            {
                sThisIsTheEnd = aFrom->tableRef->position.offset +
                    aFrom->tableRef->position.size;

                if ( aFrom->tableRef->position.
                     stmtText[aFrom->tableRef->position.offset +
                              aFrom->tableRef->position.size] == '"' )
                {
                    sThisIsTheEnd++;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( *aFromWhereEnd < sThisIsTheEnd )
            {
                *aFromWhereEnd = sThisIsTheEnd;
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* Traverse */
        IDE_TEST( getFromEnd( aFrom->left,
                              aFromWhereEnd )
                  != IDE_SUCCESS );

        IDE_TEST( getFromEnd( aFrom->right,
                              aFromWhereEnd )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getFromStart( qmsFrom * aFrom,
                          SInt    * aFromWhereStart )
{
/****************************************************************************************
 *
 * Description : Shard Query의 From절 시작 Offset를 추출한다. From절을 복제할 때에
 *               사용하며, Join 연산과 From절 최적화로 ParseTree와 다른 순서로 From 리스트화
 *               된 것을 고려해서 최소값을 찾는다.
 *
 * Implementation : 1. Join 연산이 없다면, 해당 TableRef Position Offset를 전달한다.
 *                  2. Join 연산이 있다면, Left 으로 재귀한다.
 *                  3. Join 연산이 있다면, Right 으로 재귀한다.
 *
 *****************************************************************************************/

    SInt sThisIsTheStart = ID_SINT_MAX;

    if ( aFrom != NULL )
    {
        /* 1. Join 연산이 없다면, 해당 TableRef Position Offset를 전달한다. */
        if ( aFrom->joinType == QMS_NO_JOIN )
        {
            IDE_TEST_RAISE( aFrom->tableRef == NULL, ERR_NULL_TABLEREF );

            sThisIsTheStart = aFrom->tableRef->position.offset;

            if ( *aFromWhereStart > sThisIsTheStart )
            {
                *aFromWhereStart = sThisIsTheStart;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* 2. Join 연산이 있다면, Left 으로 재귀한다. */
            IDE_TEST( getFromStart( aFrom->left,
                                    aFromWhereStart )
                      != IDE_SUCCESS );

            /* 3. Join 연산이 있다면, Right 으로 재귀한다.
             *   - Join 시 From Parsing 순서에 따라서 3개 이상, Ansi Join 이 포함, Comma 유무로
             *     Right 에 첫번째 From 이 구성되어 있는 경우가 있다.
             */
            IDE_TEST( getFromStart( aFrom->right,
                                    aFromWhereStart )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_TABLEREF )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getFromStart",
                                  "tableref is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getFromOffset( qmsFrom * aFrom,
                           SInt    * aStart,
                           SInt    * aEnd )
{
/****************************************************************************************
 *
 * Description : Shard Query의 From절 Offset를 추출한다. From절을 복제할 때에 사용한다.
 *
 * Implementation : 1. 반환이 필요한 시작 Offset를 계산한다.
 *                  2. 반환이 필요한 마지막 Offset를 계산한다.
 *
 ****************************************************************************************/

    qmsFrom * sFrom  = NULL;
    SInt      sStart = ID_SINT_MAX;
    SInt      sEnd   = 0;

    IDE_TEST_RAISE( aFrom == NULL, ERR_NULL_FROM );

    /* 1. 반환이 필요한 시작 Offset를 계산한다. */
    if ( aStart != NULL )
    {
        for ( sFrom  = aFrom;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {
            IDE_TEST( getFromStart( sFrom,
                                    & sStart )
                      != IDE_SUCCESS );
        }

        *aStart = sStart;
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. 반환이 필요한 마지막 Offset를 계산한다. */
    if ( aEnd != NULL )
    {
        for ( sFrom  = aFrom;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {
            IDE_TEST( getFromEnd( sFrom,
                                  & sEnd )
                      != IDE_SUCCESS );
        }

        *aEnd = sEnd;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FROM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getFromOffset",
                                  "from is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmg::makeShardParamOffsetArrayForGraph( qcStatement       * aStatement,
                                               qcParamOffsetInfo * aParamOffsetInfo,
                                               UShort            * aOutParamCount,
                                               qcShardParamInfo ** aOutParamInfo )
{
/****************************************************************************************
 *
 * Description : Shard Bind Offset Array를 생성한다.
 *               여기서 Bind Offset란 Variable Tuple 내 Column 번호이다.
 *               Shard View Transform으로 인하여 Bind 개수나 위치가 변경될 수 있기 때문에,
 *               Shard View에서 사용하는 순서대로 Bind 연결해주기 위해서, Offset를 Array로
 *               구성한다.
 *
 *               새로운 Bind Count를 추출해서, 기존의 위치로 조정해서 Array를 구성해야
 *               한다. 또한, 해당 Bind Node의 Column를 조정해주어야, Node Estimate에서
 *               찾을 수 있다. 또한 Graph 단계의 Bind Transform에서 이용하기 위해
 *               Offset를 반환한다.
 *
 * Implementation :
 *
 ****************************************************************************************/

    UShort    sIdx        = 0;
    UShort    sParamCount = 0;
    qcShardParamInfo * sOutParamInfo = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aParamOffsetInfo == NULL, ERR_NULL_INFO );

    sParamCount = aParamOffsetInfo->mCount;

    if ( sParamCount > 0 )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qcShardParamInfo,
                                           sParamCount,
                                           &( sOutParamInfo ) )
                  != IDE_SUCCESS );

        for ( sIdx = 0;
              sIdx < sParamCount;
              sIdx++ )
        {
            ( sOutParamInfo + sIdx )->mIsOutRefColumnBind = aParamOffsetInfo->mShardParamInfo[ sIdx ].mIsOutRefColumnBind;
            ( sOutParamInfo + sIdx )->mOffset = aParamOffsetInfo->mShardParamInfo[ sIdx ].mOffset;
            ( sOutParamInfo + sIdx )->mOutRefTuple = aParamOffsetInfo->mShardParamInfo[ sIdx ].mOutRefTuple;
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutParamCount != NULL )
    {
        *aOutParamCount = sParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutParamInfo != NULL )
    {
        *aOutParamInfo = sOutParamInfo;
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayForGraph",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayForGraph",
                                  "info NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::makeShardParamOffsetArrayWithInfo( qcStatement       * aStatement,
                                               sdiAnalyzeInfo    * aAnalyzeInfo,
                                               qcParamOffsetInfo * aParamOffsetInfo,
                                               UShort            * aOutParamCount,
                                               UShort            * aOutParamOffset,
                                               qcShardParamInfo ** aOutShardParamInfo )
{
/****************************************************************************************
 *
 * Description : Shard Bind Offset Array를 생성한다.
 *               여기서 Bind Offset란 Variable Tuple 내 Column 번호이다.
 *               Shard View Transform으로 인하여 Bind 개수나 위치가 변경될 수 있기 때문에,
 *               Shard View에서 사용하는 순서대로 Bind 연결해주기 위해서, Offset를 Array로
 *               구성한다.
 *
 *               새로운 Bind Count를 추출해서, 기존의 위치로 조정해서 Array를 구성해야
 *               한다. 또한, 해당 Bind Node의 Column를 조정해주어야, Node Estimate에서
 *               찾을 수 있다. 또한 Graph 단계의 Bind Transform에서 이용하기 위해
 *               Offset를 반환한다.
 *
 * Implementation : 1. 변경된 Bind Count에서 변경전 Bind Count를 제외한다.
 *                  2. 관련된 Bind가 있다면, 기존 위치로 조정해서 Array를 구성한다.
 *                  3. Analyze Info 에서 Key 관련된 Bind가 있다면, Array 정보로 조정한다
 *
 ****************************************************************************************/

    UShort             sIdx               = 0;
    UShort             sParamOffset       = 0;
    UShort             sParamCount        = 0;
    qcShardParamInfo * sOutShardParamInfo = NULL;
    qtcNode          * sNode              = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aAnalyzeInfo == NULL, ERR_NULL_ANALYZE );
    IDE_TEST_RAISE( aParamOffsetInfo == NULL, ERR_NULL_INFO );
    IDE_TEST_RAISE( aStatement->myPlan->stmtListMgr == NULL, ERR_NULL_STMTLISTMGR );

    /* 1. 변경된 Bind Count에서 변경전 Bind Count를 제외한다. */
    IDE_TEST( getHostVarOffset( aStatement,
                                &( sParamOffset ) )
              != IDE_SUCCESS );

    sParamCount = qcg::getBindCount( aStatement ) - sParamOffset;

    /* 2. 관련된 Bind가 있다면, 기존 위치로 조정해서 Array를 구성한다. */
    if ( sParamCount > 0 )
    {
        IDE_TEST_RAISE( aParamOffsetInfo->mCount != sParamCount, ERR_INVALID_BIND_INFO );

        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qcShardParamInfo,
                                           sParamCount,
                                           &( sOutShardParamInfo ) )
                  != IDE_SUCCESS );

        for ( sIdx = 0;
              sIdx < aParamOffsetInfo->mCount;
              sIdx++ )
        {
            sNode = aStatement->myPlan->stmtListMgr->mHostVarNode[ sParamOffset + sIdx ];

            sNode->node.column = aParamOffsetInfo->mShardParamInfo[sIdx].mOffset;
            ( sOutShardParamInfo + sIdx )->mOffset = aParamOffsetInfo->mShardParamInfo[sIdx].mOffset;
            ( sOutShardParamInfo + sIdx )->mIsOutRefColumnBind = aParamOffsetInfo->mShardParamInfo[sIdx].mIsOutRefColumnBind;
            ( sOutShardParamInfo + sIdx )->mOutRefTuple = aParamOffsetInfo->mShardParamInfo[sIdx].mOutRefTuple;

        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 3. Analyze Info 에서 Key 관련된 Bind가 있다면, Array 정보로 조정한다. */
    IDE_TEST( adjustParamOffsetForAnalyzeInfo( aAnalyzeInfo,
                                               sParamCount,
                                               &sOutShardParamInfo )
              != IDE_SUCCESS );

    if ( aOutParamOffset != NULL )
    {
        *aOutParamOffset = sParamOffset;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutParamCount != NULL )
    {
        *aOutParamCount = sParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutShardParamInfo != NULL )
    {
        *aOutShardParamInfo = sOutShardParamInfo;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayWithInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayWithInfo",
                                  "info is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayWithInfo",
                                  "analyze info is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_BIND_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayWithInfo",
                                  "invalid bind info" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMTLISTMGR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArrayWithInfo",
                                  "stmtListMgr is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::makeShardParamOffsetArray( qcStatement       * aStatement,
                                       qcNamePosition    * aParsePosition,
                                       UShort            * aOutParamCount,
                                       UShort            * aOutParamOffset,
                                       qcShardParamInfo ** aOutShardParamInfo )
{
/****************************************************************************************
 *
 * Description : Shard Bind Offset Array를 생성한다.
 *               여기서 Bind Offset란 Variable Tuple 내 Column 번호이다.
 *               Shard View Transform으로 인하여 Bind 개수나 위치가 변경될 수 있기 때문에,
 *               Shard View에서 사용하는 순서대로 Bind 연결해주기 위해서, Offset를 Array로
 *               구성한다.
 *
 *               기존에 수집된 Bind 정보로 Array를 구성한다. 또한 Graph 단계의
 *               Bind Transform에서 이용하기 위해 Offset를 반환한다.
 *
 * Implementation : 1. 기존 위치를 확인한다.
 *                  2. 기존 위치를 그대로 기록한다.
 *
 ****************************************************************************************/

    UShort             sIdx               = 0;
    UShort             sParamOffset       = ID_USHORT_MAX;
    UShort             sParamCount        = ID_USHORT_MAX;
    qcShardParamInfo * sOutShardParamInfo = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aParsePosition == NULL, ERR_NULL_POSITION );

    /* 1. 기존 위치를 확인한다. */
    IDE_TEST( getParamOffsetAndCount( aStatement,
                                      aParsePosition->offset,
                                      aParsePosition->offset + aParsePosition->size,
                                      0,
                                      qcg::getBindCount( aStatement ),
                                      &( sParamOffset ),
                                      &( sParamCount ) )
              != IDE_SUCCESS );

    /* 2. 기존 위치를 그대로 기록한다. */
    if ( sParamCount > 0 )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qcShardParamInfo,
                                           sParamCount,
                                           aOutShardParamInfo )
                  != IDE_SUCCESS );

        sOutShardParamInfo = *aOutShardParamInfo;

        for ( sIdx = 0;
              sIdx < sParamCount;
              sIdx ++ )
        {
            ( sOutShardParamInfo + sIdx )->mOffset = sParamOffset + sIdx;
            ( sOutShardParamInfo + sIdx )->mIsOutRefColumnBind = ID_FALSE;
            ( sOutShardParamInfo + sIdx )->mOutRefTuple = ID_USHORT_MAX;
        }
    }
    else
    {
        *aOutShardParamInfo = NULL;
    }

    if ( aOutParamOffset != NULL )
    {
        *aOutParamOffset = sParamOffset;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutParamCount != NULL )
    {
        *aOutParamCount = sParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArray",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::makeShardParamOffsetArray",
                                  "position is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::findAndCollectParamOffset( qcStatement       * aStatement,
                                       qtcNode           * aNode,
                                       qcParamOffsetInfo * aParamOffsetInfo )
{
/****************************************************************************************
 *
 * Description : Parse 단계에서 Shard Transform으로 Bind 추가, 위치 변경이 발생하면 Bind
 *               정보를 연결해 주기위해서, 변경한 Bind 마다 위치를 수집한다.
 *               Bind가 새로 추가되거나, 다른 위치의 Bind가 이동할 때 마다 호출해야 한다.
 *
 *
 *  BEFORE / SELECT SUM( C1 + ? ) AS A FROM T1 GROUP BY C2;
 *                  ************* OLD
 *
 *  AFTER  / SELECT SUM( B ) AS A
 *            FROM SHARD( SELECT C2, SUM( C1 + ? ) B
 *                         FROM T1   ************* NEW
 *                          GROUP BY C2 )
 *             GROUP BY C2;
 *
 *
 *  BEFORE / SELECT AVG( C1 + ? ) AS A FROM T1 GROUP BY C2;
 *                  ************* OLD
 *
 *  AFTER  / SELECT SUM( B ) / SUM( C ) AS A
 *            FROM SHARD( SELECT C2, SUM( C1 + ? ) AS B, COUNT( C1 + ? ) AS C
 *                          FROM T1  *************       *************** NEW 2회
 *                           GROUP BY C2 )
 *             GROUP BY C2;
 *
 *
 *  BEFORE / SELECT SUM( COL ) AS A FROM T1 GROUP BY C2 HAVING COUNT( C1 + ? ) > ?
 *                                                             ******************* OLD
 *
 *  AFTER  / SELECT SUM( B ) AS A
 *            FROM SHARD( SELECT C2, SUM( COL ) AS B, COUNT( C1 + ? ) AS C
 *                         FROM T1                    *************** NEW
 *                          GROUP BY C2 )
 *             GROUP BY C2
 *              HAVING C > ?
 *
 *
 * Implementation : 1. BIND 노드인 지 검사한다.
 *                  2. 있다면, BIND 위치를 기록시킨다.
 *                  3. 없다면, 노드 하위로 BIND 노드을 찾는다.
 *
 ****************************************************************************************/

    qtcNode * sArgNode = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_ARGUMENT );

    /* 1. BIND 노드인 지 검사한다.*/
    if ( qtc::isHostVariable( QC_SHARED_TMPLATE( aStatement ), aNode ) == ID_TRUE )
    {
        IDE_TEST_RAISE( aParamOffsetInfo == NULL, ERR_NULL_BIND_INFO );
        IDE_TEST_RAISE( aParamOffsetInfo->mCount >= MTC_TUPLE_COLUMN_ID_MAXIMUM, ERR_HOST_VAR_LIMIT );

        /* 2. 있다면, BIND 위치를 기록시킨다 */
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mIsOutRefColumnBind = ID_FALSE;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOffset = aNode->node.column;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOutRefTuple = ID_USHORT_MAX;
        aParamOffsetInfo->mCount++;
    }
    else if ( ( aNode->lflag & QTC_NODE_OUT_REF_COLUMN_MASK )
              == QTC_NODE_OUT_REF_COLUMN_TRUE ) 
    {
        IDE_TEST_RAISE( aParamOffsetInfo == NULL, ERR_NULL_BIND_INFO );
        IDE_TEST_RAISE( aParamOffsetInfo->mCount >= MTC_TUPLE_COLUMN_ID_MAXIMUM, ERR_HOST_VAR_LIMIT );

        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mIsOutRefColumnBind = ID_TRUE;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOutRefTuple = aNode->node.table;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOffset = aNode->node.column;

        aParamOffsetInfo->mCount++;
    }   
    else
    {
        /* 3. 없다면, 노드 하위로 BIND 노드을 찾는다. */
        for ( sArgNode  = (qtcNode *)aNode->node.arguments;
              sArgNode != NULL;
              sArgNode  = (qtcNode *)sArgNode->node.next )
        {
            IDE_TEST( findAndCollectParamOffset( aStatement,
                                                 sArgNode,
                                                 aParamOffsetInfo )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::findAndCollectParamOffset",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::findAndCollectParamOffset",
                                  "node is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_BIND_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::findAndCollectParamOffset",
                                  "bind info is null" ) );
    }
    IDE_EXCEPTION( ERR_HOST_VAR_LIMIT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_HOST_VAR_LIMIT_EXCEED,
                                  MTC_TUPLE_COLUMN_ID_MAXIMUM ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::collectReaminParamOffset( qcStatement       * aStatement,
                                      SInt                aStartOffset,
                                      SInt                aEndOffset,
                                      qcParamOffsetInfo * aParamOffsetInfo )
{
/****************************************************************************************
 *
 * Description : Shard Transform으로 순서만 뒤로 밀린 Bind를 수집한다.
 *               findAndCollectParamOffset 이후에 호출하며, From 절부터 Where절까지의
 *               Bind를 aParamOffsetInfo에 수집한 Bind 다음 순서에, 본래의 Bind 위치로
 *               기록한다.
 *
 *               aStartOffset은 From 절 시작, aEndOffset은 Where절 끝을 사용한다.
 *
 *
 *  BEFORE / SELECT SUM( C1 + ? ) AS A FROM T1 WHERE C3 != ? GROUP BY C2;
 *                                     *********************
 *
 *  AFTER  / SELECT SUM( B ) AS A
 *            FROM SHARD( SELECT C2, SUM( C1 + ? ) AS B
 *                         FROM T1 WHERE C3 != ?
 *                          GROUP BY C2 )
 *             GROUP BY C2;
 *
 *
 *  BEFORE / SELECT SUM( C1 + ? ) A
 *            FROM ( SELECT * FROM T1 WHERE C4 = ? ) WHERE C3 != ? GROUP BY C2;
 *            ****************************************************
 *
 *  AFTER  / SELECT SUM( B ) / SUM( C ) AS A
 *            FROM SHARD( SELECT C2, SUM( C1 + ? ) AS B, COUNT( C1 + ? ) AS C
 *                         FROM ( SELECT * FROM T1 WHERE C4 = ? ) WHERE C3 != ?
 *                          GROUP BY C2 )
 *             GROUP BY C2;
 *
 *
 * Implementation : 1. Bind Offset, Bind Count를 얻는다.
 *                  2. 남은 Bind 개수 만큰 본래의 Bind Offset를 기록한다.
 *
 ****************************************************************************************/

    UShort sParamOffset = 0;
    UShort sParamCount  = 0;
    UInt   sIdx;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aParamOffsetInfo == NULL, ERR_NULL_BIND_INFO );
    IDE_TEST_RAISE( aStatement->myPlan->stmtListMgr == NULL, ERR_NULL_STMTLISTMGR );

    /* 1. Bind Offset, Bind Count를 얻는다. */
    IDE_TEST( getParamOffsetAndCount( aStatement,
                                      aStartOffset,
                                      aEndOffset,
                                      0,
                                      qcg::getBindCount( aStatement ),
                                      &( sParamOffset ),
                                      &( sParamCount ) )
              != IDE_SUCCESS );

    /* 2.   남은 Bind 개수 만큰 본래의 Bind Offset를 기록한다. */
    for ( sIdx = 0;
          sIdx < sParamCount;
          sIdx ++ )
    {
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mIsOutRefColumnBind = ID_FALSE ;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOffset = sParamOffset + sIdx;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOutRefTuple = ID_USHORT_MAX;

        aParamOffsetInfo->mCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::collectReaminParamOffset",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_BIND_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::collectReaminParamOffset",
                                  "bind info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMTLISTMGR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::collectReaminParamOffset",
                                  "stmtListMgr is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::copyAndCollectParamOffset( qcStatement       * aStatement,
                                       SInt                aStartOffset,
                                       SInt                aEndOffset,
                                       UShort              aParamOffset,
                                       UShort              aParamCount,
                                       qcShardParamInfo  * aShardParamInfo,
                                       qcParamOffsetInfo * aParamOffsetInfo )
{
/****************************************************************************************
 *
 * Description : Graph 시점에 Shard Transform으로 순서만 뒤로 밀린 Bind를 복제한다.
 *               findAndCollectParamOffset 이후에 호출하며, Transform 대상이 아닌
 *               기존에 있던 Bind 정보를 복제한다.
 *
 *
 *  BEFORE / SELECT CAST( ? AS INTEGER )
 *            FROM ( SELECT CAST( ? AS INTEGER ), C1
 *                    FROM T1 WHERE C3 != ?
 *                    *********************
 *                     ORDER BY C2 )
 *             WHERE C1 != ?;
 *
 *  BIND       / COUNT 4 / OFFSET 0 1 2 3
 *                                    *
 *                                      \_____________________
 *                                                            |
 *  AFTER1 / SELECT CAST( ? AS INTEGER )                      |
 *            FROM ( SELECT *                                 |
 *                    FROM ( SELECT CAST( ? AS INTEGER ), C1  |
 *                            FROM T1 WHERE C3 != ?           |
 *                             ORDER BY C2 ) )                |
 *             WHERE C1 != ?;          _______________________|
 *                                    /                        collectReaminParamOffset
 *  SHARD BIND / COUNT 2 / OFFSET 1 2
 *                                N N
 *                                    \_______________________
 *                                                            |
 *  AFTER2 / SELECT CAST( ? AS INTEGER )                      |
 *            FROM ( SELECT *                                 |
 *                    FROM ( SELECT CAST( ? AS INTEGER ), C1  |
 *                            FROM T1 WHERE C3 != ?           |
 *                             AND C1 != ?                    |
 *                              ORDER BY C2 ) )               |
 *             WHERE C1 != ?;         ________________________|
 *                                   /                         copyAndCollectParamOffset
 *  SHARD BIND / COUNT 3 / OFFSET 1 2 3
 *                                C C N
 *
 *
 * Implementation : 1. Bind Offset, Bind Count를 얻는다.
 *                  2. 남은 Bind 개수 만큰 본래의 Bind Offset를 복제한다.
 *
 ****************************************************************************************/

    UShort   sShardParamOffset = aParamOffset;
    UShort   sShardParamCount  = aParamOffset + aParamCount;
    UShort   sParamOffset      = 0;
    UShort   sParamCount       = 0;
    UInt     sIdx;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aParamOffsetInfo == NULL, ERR_NULL_BIND_INFO );
    IDE_TEST_RAISE( aStatement->myPlan->stmtListMgr == NULL, ERR_NULL_STMTLISTMGR );

    /* 1. Bind Offset, Bind Count를 얻는다. */
    IDE_TEST( getParamOffsetAndCount( aStatement,
                                      aStartOffset,
                                      aEndOffset,
                                      sShardParamOffset,
                                      sShardParamCount,
                                      &( sParamOffset ),
                                      &( sParamCount ) )
              != IDE_SUCCESS );

    /* 2. 남은 Bind 개수 만큰 본래의 Bind Offset를 기록한다. */
    for ( sIdx = 0;
          sIdx < sParamCount;
          sIdx ++ )
    {
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mIsOutRefColumnBind = (aShardParamInfo + sParamOffset + sIdx)->mIsOutRefColumnBind ;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOffset = (aShardParamInfo + sParamOffset + sIdx)->mOffset;
        aParamOffsetInfo->mShardParamInfo[aParamOffsetInfo->mCount].mOutRefTuple = (aShardParamInfo + sParamOffset + sIdx)->mOffset;

        aParamOffsetInfo->mCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::copyAndCollectParamOffset",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_BIND_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::copyAndCollectParamOffset",
                                  "bind info is null" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMTLISTMGR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::copyAndCollectParamOffset",
                                  "stmtListMgr is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::setHostVarOffset( qcStatement * aStatement )
{
 /****************************************************************************************
 *
 * Description : Shard Transform으로 Bind 변경이 발생할 것을 대비해여, 이전 Bind 정보를
 *               유지해야 합니다. 먼저 새로운 Bind 정보를 이전 Bind 정보이후에 구축하기
 *               위해서, 현재 Bind 개수를 mHostVarOffset에 기록합니다.
 *
 *               그리고 Shard Transfrom 완료 후에 mHostVarOffset를 0으로 변경해야 합니다.
 *
 * Implementation : 1. mHostVarOffset을 현재 Bind 끝으로 조정한다.
 *                  2. 다시 호출될 때에 원복한다.
 *                  3. 이전 Bind 개수를 기록한다.
 *
 ****************************************************************************************/

    qcTemplate * sTemplate   = NULL;
    UShort       sParamCount = ID_USHORT_MAX;
    UShort       sBindTuple  = ID_USHORT_MAX;

    IDE_TEST_RAISE( aStatement->myPlan->stmtListMgr == NULL, ERR_NULL_STMTLISTMGR );

    sTemplate  = QC_SHARED_TMPLATE( aStatement );
    sBindTuple = sTemplate->tmplate.variableRow;

    /* 1. mHostVarOffset을 현재 Bind 끝으로 조정한다. */
    if ( aStatement->myPlan->stmtListMgr->mHostVarOffset == 0 )
    {
        sParamCount = qcg::getBindCount( aStatement );

        aStatement->myPlan->stmtListMgr->mHostVarOffset = sParamCount;
    }
    else
    {
        /* 2. 다시 호출될 때에 원복한다. */
        sParamCount = aStatement->myPlan->stmtListMgr->mHostVarOffset;

        aStatement->myPlan->stmtListMgr->mHostVarOffset = 0;
    }

    /* 3. 이전 Bind 개수를 기록한다. */
    if ( sBindTuple != ID_USHORT_MAX )
    {
        sTemplate->tmplate.rows[sBindTuple].columnCount = sParamCount;
    }
    else
    {
        /* Nothing do do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STMTLISTMGR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::setParamCountAndOffset",
                                  "stmtListMgr is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::checkStackOverflow( mtcNode * aNode,
                                SInt      aRemain,
                                idBool  * aIsOverflow )
{
 /****************************************************************************************
 *
 * Description : 주어진 Node의 Mtc Stack 제한을 검사하는 함수이다.
 *
 * Implementation : 1. 현재 노드의 Stack 제한을 검사한다.
 *                  2. 제한에 만족하면, 현재 노드의 Argument로 재귀한다.
 *                  3. Overflow 여부를 반환한다.
 *
 ****************************************************************************************/

    mtcNode * sNode        = NULL;
    SInt      sRemain      = 0;
    idBool    sIskOverflow = ID_FALSE;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );

    /* 1. 현재 노드의 Stack 제한을 검사한다. */
    sRemain = aRemain - (SInt)( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    if ( sRemain < 1 )
    {
        sIskOverflow = ID_TRUE;
    }
    else
    {
        /* 2. 제한에 만족하면, 현재 노드의 Argument로 재귀한다. */
        for ( sNode  = aNode->arguments, sRemain = aRemain - 1;
              sNode != NULL;
              sNode  = sNode->next, sRemain-- )
        {
            IDE_TEST( checkStackOverflow( sNode,
                                          sRemain,
                                          &( sIskOverflow ) )
                      != IDE_SUCCESS );

            if ( sIskOverflow == ID_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    /* 3. Overflow 여부를 반환한다. */
    if ( aIsOverflow != NULL )
    {
        *aIsOverflow = sIskOverflow;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::checkStackOverflow",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getParamOffsetAndCount( qcStatement * aStatement,
                                    SInt          aStartOffset,
                                    SInt          aEndOffset,
                                    UShort        aStartParamOffset,
                                    UShort        aEndParamCount,
                                    UShort      * aParamOffset,
                                    UShort      * aParamCount )
{
/****************************************************************************************
 *
 * Description :
 *
 * Implementation : 1. aStartParamOffset 부터 aEndParamCount 까지 Bind Offset, Bind Count 를 계산한다.
 *                  2. aStartOffset 전까지 Bind 개수를 확인, Bind Offset를 얻는다.
 *                  3. aEndOffset 전까지 Bind 개수를 확인, Bind Count를 얻는다.
 *
 ****************************************************************************************/

    UShort   sParamOffset = 0;
    UShort   sParamCount  = 0;
    UInt     sIdx;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStatement->myPlan->stmtListMgr == NULL, ERR_NULL_STMTLISTMGR );

    /* 1. From 절부터 Where절까지 Bind Offset, Bind Count 를 계산한다. */
    for ( sIdx = aStartParamOffset;
          sIdx < aEndParamCount;
          sIdx++ )
    {
        /* 2. aStartOffset 전까지 Bind 개수를 확인, Bind Offset를 얻는다. */
        if ( aStatement->myPlan->stmtListMgr->hostVarOffset[ sIdx ] < aStartOffset )
        {
            sParamOffset++;
        }
        else
        {
            /* 3. aEndOffset 전까지 Bind 개수를 확인, Bind Count를 얻는다.. */
            if ( aStatement->myPlan->stmtListMgr->hostVarOffset[ sIdx ] <= aEndOffset )
            {
                sParamCount++;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    if ( aParamOffset != NULL )
    {
        *aParamOffset = sParamOffset;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aParamCount != NULL )
    {
        *aParamCount = sParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getParamOffsetAndCount",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMTLISTMGR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getParamOffsetAndCount",
                                  "stmtListMgr is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::adjustParamOffsetForAnalyzeInfo( sdiAnalyzeInfo    * aAnalyzeInfo,
                                             UShort              aParamCount,
                                             qcShardParamInfo ** aOutShardParamInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                PROJ-2646 New shard analyzer 의 구현 사항을 함수화
 *
 *                 Analyze 반복 수행 구조에서는 수행마다 Bind Offset 위치가 변경되어,
 *                  매번 호출하여 보정하였지만,
 *                   수정된 Analyze 최소 수행 구조에서는 Shard Query 가 변경되는
 *                    TransformAble Query 와 Optimize Shard Query 에서만 호출한다.
 *
 *                     - qmg::makeShardParamOffsetArrayWithInfo
 *                      <- qmvShardTransform::makeShardForAggr
 *                       <- qmvShardTransform::processTransformForAggr
 *
 *                     - qmgShardSelect::setShardParameter
 *                      <- qmgShardSelect::pushSeriesOptimize
 *
 * Implementation :
 *
 ***********************************************************************/
    UShort             sIndex = 0;
    qcShardParamInfo * sShardParamInfo = *aOutShardParamInfo;
    UShort             sOrgBindId = 0;

    IDE_TEST_RAISE( aAnalyzeInfo == NULL, ERR_NULL_ANALYZE );

    if ( aParamCount > 0 )
    {
        for ( sIndex = 0;
              sIndex < aAnalyzeInfo->mValuePtrCount;
              sIndex++ )
        {
            if ( aAnalyzeInfo->mValuePtrArray[ sIndex ]->mType == 0 )
            {
                if ( ( sShardParamInfo + aAnalyzeInfo->mValuePtrArray[ sIndex ]->mValue.mBindParamId )->mIsOutRefColumnBind
                     == ID_TRUE )
                {
                    /* TASK-7219 Non-shard DML */
                    sOrgBindId = aAnalyzeInfo->mValuePtrArray[ sIndex ]->mValue.mBindParamId;

                    aAnalyzeInfo->mValuePtrArray[ sIndex ]->mType = SDI_VALUE_INFO_OUT_REF_COL;

                    aAnalyzeInfo->mValuePtrArray[ sIndex ]->mValue.mOutRefInfo.mTuple =
                        &(( sShardParamInfo + sOrgBindId )->mOutRefTuple);

                    aAnalyzeInfo->mValuePtrArray[ sIndex ]->mValue.mOutRefInfo.mColumn =
                        &(( sShardParamInfo + sOrgBindId )->mOffset);
                }
                else
                {
                    aAnalyzeInfo->mValuePtrArray[ sIndex ]->mValue.mBindParamId =
                        ( sShardParamInfo + aAnalyzeInfo->mValuePtrArray[ sIndex ]->mValue.mBindParamId )->mOffset;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( aAnalyzeInfo->mSubKeyExists == ID_TRUE )
        {
            for ( sIndex = 0;
                  sIndex < aAnalyzeInfo->mSubValuePtrCount;
                  sIndex++ )
            {
                if ( aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mType == 0 )
                {
                    if ( ( sShardParamInfo + aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mValue.mBindParamId )->mIsOutRefColumnBind
                         == ID_TRUE )
                    {
                        /* TASK-7219 Non-shard DML */
                        sOrgBindId = aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mValue.mBindParamId;

                        aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mType = SDI_VALUE_INFO_OUT_REF_COL;

                        aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mValue.mOutRefInfo.mTuple =
                            &(( sShardParamInfo + sOrgBindId )->mOutRefTuple);

                        aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mValue.mOutRefInfo.mColumn =
                            &(( sShardParamInfo + sOrgBindId )->mOffset);
                    }
                    else
                    {
                        aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mValue.mBindParamId =
                            ( sShardParamInfo + aAnalyzeInfo->mSubValuePtrArray[ sIndex ]->mValue.mBindParamId )->mOffset;
                    }
                }
                else
                {
                    /* Nothing to do */
                }
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_ANALYZE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "qmg::adjustParamOffsetForAnalyzeInfo",
                                  "analyze is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmg::getHostVarOffset( qcStatement * aStatement,
                              UShort      * aParamOffset )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard Transform 으로 Bind 변경이 발생할 것을 대비해여,
 *                 유지한 이전 Bind 정보를 가져온다.
 *                  유지한 것이 없다면, 현재 Bind 정보를 가져온다.
 *
 *                   관련 함수
 *                    - qmg::setHostVarOffset
 *
 * Implementation : 1. 유지한 것이 없다면, 현재 Bind 정보를 가져온다.
 *                  2. 유지한 이전 Bind 정보를 가져온다.
 *
 ***********************************************************************/

    UShort sParamOffset = ID_USHORT_MAX;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStatement->myPlan->stmtListMgr == NULL, ERR_NULL_STMTLISTMGR );

    /* 1. 유지한 것이 없다면, 현재 Bind 정보를 가져온다. */
    if ( aStatement->myPlan->stmtListMgr->mHostVarOffset == 0 )
    {
        sParamOffset = qcg::getBindCount( aStatement );
    }
    else
    {
        /* 2. 유지한 이전 Bind 정보를 가져온다. */
        sParamOffset = aStatement->myPlan->stmtListMgr->mHostVarOffset;
    }


    if ( aParamOffset != NULL )
    {
        *aParamOffset = sParamOffset;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getHostVarOffset",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMTLISTMGR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmg::getHostVarOffset",
                                  "stmtListMgr is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
