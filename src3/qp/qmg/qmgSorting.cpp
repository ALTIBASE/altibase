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
 * $Id: qmgSorting.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Sorting Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgSorting.h>
#include <qmoCost.h>
#include <qmoOneMtrPlan.h>
#include <qmoParallelPlan.h>
#include <qmgGrouping.h>

IDE_RC
qmgSorting::init( qcStatement * aStatement,
                  qmsQuerySet * aQuerySet,
                  qmgGraph    * aChildGraph,
                  qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgSorting의 초기화
 *
 * Implementation :
 *    (1) qmgSorting을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgSORT      * sMyGraph;
    qmsQuerySet  * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgSorting::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Sorting Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgSorting을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgSORT ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SORTING;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgSorting::optimize;
    sMyGraph->graph.makePlan = qmgSorting::makePlan;
    sMyGraph->graph.printGraph = qmgSorting::printGraph;

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
    // Sorting Graph 고유 정보를 위한 기본 초기화
    //---------------------------------------------------

    sMyGraph->orderBy = ((qmsParseTree*)(aStatement->myPlan->parseTree))->orderBy;
    sMyGraph->limitCnt = 0;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSorting::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSorting의 최적화
 *
 * Implementation :
 *    (1) orderBy의 Subquery Graph 생성
 *    (2) orderBy에 subquery나 expression이 존재하지 않으면, 최적화 처리
 *        A. indexable Order By 최적화
 *        B. indexable Order By 최적화 실패한 경우, Limit Sort 최적화
 *        C. Limit Sort 최적화 선택된 경우, limitCnt 설정
 *    (3) 공통 비용 정보
 *    (4) Preserved Order
 *
 ***********************************************************************/

    qmgSORT           * sMyGraph;
    qmsSortColumns    * sOrderBy;
    qtcNode           * sSortNode;
    qmgPreservedOrder * sNewOrder;
    qmgPreservedOrder * sWantOrder;
    qmgPreservedOrder * sCurOrder;
    qmsLimit          * sLimit;
    qmgGraph          * sChild;

    SDouble             sTotalCost     = 0.0;
    SDouble             sDiskCost      = 0.0;
    SDouble             sAccessCost    = 0.0;
    SDouble             sSelAccessCost = 0.0;
    SDouble             sSelDiskCost   = 0.0;
    SDouble             sSelTotalCost  = 0.0;
    SDouble             sOutputRecordCnt;

    UInt                sLimitCnt = 0;
    idBool              sExistSubquery   = ID_FALSE;
    idBool              sSuccess         = ID_FALSE;
    idBool              sHasHostVar      = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmgSorting::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph       = (qmgSORT*) aGraph;
    sWantOrder     = NULL;

    //------------------------------------------
    // order by의 subquery graph 생성 및 want order 생성
    //------------------------------------------

    for ( sOrderBy = sMyGraph->orderBy;
          sOrderBy != NULL;
          sOrderBy = sOrderBy->next )
    {
        sSortNode = sOrderBy->sortColumn;

        if ( sSortNode->node.module == &( qtc::passModule ) )
        {
            sSortNode = (qtcNode*)sSortNode->node.arguments;
        }
        else
        {
            // nothing to do
        }

        // BUG-42041 SortColumn으로 PSM Variable이 허용되지만,
        // 이 경우 limit sort를 할 수 없다.
        if ( MTC_NODE_IS_DEFINED_TYPE( & sSortNode->node ) == ID_FALSE )
        {
            sHasHostVar = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sSortNode->lflag & QTC_NODE_SUBQUERY_MASK )
             == QTC_NODE_SUBQUERY_EXIST )
        {
            sExistSubquery = ID_TRUE;
            IDE_TEST( qmoSubquery::optimize( aStatement,
                                             sSortNode,
                                             ID_FALSE ) // No KeyRange Tip
                      != IDE_SUCCESS );
        }
        else
        {
            // want order 생성
            IDE_TEST(
                QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                               (void**)&sNewOrder )
                != IDE_SUCCESS );

            sNewOrder->table = sSortNode->node.table;
            sNewOrder->column = sSortNode->node.column;

            if ( sOrderBy->nullsOption == QMS_NULLS_NONE )
            {
                // To Fix BUG-8316
                if ( sOrderBy->isDESC == ID_TRUE )
                {
                    sNewOrder->direction = QMG_DIRECTION_DESC;
                }
                else
                {
                    sNewOrder->direction = QMG_DIRECTION_ASC;
                }
            }
            else
            {
                if ( sOrderBy->nullsOption == QMS_NULLS_FIRST )
                {
                    if ( sOrderBy->isDESC == ID_TRUE )
                    {
                        sNewOrder->direction = QMG_DIRECTION_DESC_NULLS_FIRST;
                    }
                    else
                    {
                        sNewOrder->direction = QMG_DIRECTION_ASC_NULLS_FIRST;
                    }
                }
                else
                {
                    if ( sOrderBy->isDESC == ID_TRUE )
                    {
                        sNewOrder->direction = QMG_DIRECTION_DESC_NULLS_LAST;
                    }
                    else
                    {
                        sNewOrder->direction = QMG_DIRECTION_ASC_NULLS_LAST;
                    }
                }
            }
            sNewOrder->next = NULL;

            if ( sWantOrder == NULL )
            {
                sWantOrder = sCurOrder = sNewOrder;
            }
            else
            {
                sCurOrder->next = sNewOrder;
                sCurOrder = sCurOrder->next;
            }
        }
    }

    // BUG-43194 SORT 플랜도 cost를 이용하여 index 사용 여부를 결정
    if ( (aGraph->flag & QMG_GRAPH_TYPE_MASK) == QMG_GRAPH_TYPE_MEMORY )
    {
        sSelTotalCost = qmoCost::getMemSortTempCost(
            aStatement->mSysStat,
            &(aGraph->left->costInfo),
            QMO_COST_DEFAULT_NODE_CNT ); // node cnt

        sSelAccessCost = sSelTotalCost;
        sSelDiskCost   = 0.0;
    }
    else
    {
        sSelTotalCost   = qmoCost::getDiskSortTempCost(
            aStatement->mSysStat,
            &(aGraph->left->costInfo),
            QMO_COST_DEFAULT_NODE_CNT, // node cnt
            aGraph->left->costInfo.recordSize );

        sSelAccessCost = 0.0;
        sSelDiskCost   = sSelTotalCost;
    }

    if ( sExistSubquery == ID_FALSE )
    {
        //------------------------------------------
        // order by에 subquery가 존재하지 않으면, 최적화 적용 시도
        //------------------------------------------

        //------------------------------------------
        // Indexable Order By 최적화
        //------------------------------------------

        IDE_TEST( getCostByPrevOrder( aStatement,
                                      sMyGraph,
                                      sWantOrder,
                                      &sAccessCost,
                                      &sDiskCost,
                                      &sTotalCost ) != IDE_SUCCESS );

        if ( QMO_COST_IS_EQUAL(sTotalCost, QMO_COST_INVALID_COST) == ID_TRUE )
        {
            // retryPreservedOrder 가 성공할 경우에는 무조건 사용한다.
            IDE_TEST(qmg::retryPreservedOrder( aStatement,
                                               aGraph,
                                               sWantOrder,
                                               &sSuccess )
                     != IDE_SUCCESS);

            if ( sSuccess == ID_TRUE )
            {
                sSelAccessCost = 0.0;
                sSelDiskCost   = 0.0;
                sSelTotalCost  = 0.0;

                sMyGraph->graph.flag &= ~QMG_SORT_OPT_TIP_MASK;
                sMyGraph->graph.flag |= QMG_SORT_OPT_TIP_INDEXABLE_ORDERBY;
            }
            else
            {
                // nothing to do
            }
        }
        else if ( QMO_COST_IS_LESS(sTotalCost, sSelTotalCost) == ID_TRUE )
        {
            IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                              aGraph,
                                              sWantOrder,
                                              &sSuccess )
                      != IDE_SUCCESS );

            if ( sSuccess == ID_TRUE )
            {
                sSelAccessCost = sAccessCost;
                sSelDiskCost   = sDiskCost;
                sSelTotalCost  = sTotalCost;

                sMyGraph->graph.flag &= ~QMG_SORT_OPT_TIP_MASK;
                sMyGraph->graph.flag |= QMG_SORT_OPT_TIP_INDEXABLE_ORDERBY;
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

        //------------------------------------------
        // Indexable Min Max 최적화 PR-19344
        //------------------------------------------

        if ( (aGraph->left->type == QMG_GROUPING) &&
             ((aGraph->left->flag & QMG_GROP_OPT_TIP_MASK)
              == QMG_GROP_OPT_TIP_INDEXABLE_MINMAX) ) // Indexable Min-Max
        {
            sSelAccessCost = 0.0;
            sSelDiskCost   = 0.0;
            sSelTotalCost  = 0.0;

            sMyGraph->graph.flag &= ~QMG_INDEXABLE_MIN_MAX_MASK;
            sMyGraph->graph.flag |= QMG_INDEXABLE_MIN_MAX_TRUE;
            // Indexable order-by와 동일하게 sort node를 달지 않기 위해
            sMyGraph->graph.flag &= ~QMG_SORT_OPT_TIP_MASK;
            sMyGraph->graph.flag |= QMG_SORT_OPT_TIP_INDEXABLE_ORDERBY;
        }
        else
        {
            // nothing to do
        }

        //------------------------------------------
        // Limit Sort 최적화
        //------------------------------------------

        if ( ( sMyGraph->graph.flag & QMG_SORT_OPT_TIP_MASK ) ==
             QMG_SORT_OPT_TIP_NONE )
        {
            sLimit = ((qmsParseTree*)(aStatement->myPlan->parseTree))->limit;

            // BUG-10146 fix
            // limit 절에 host variable binding을 허용한다.
            // 하지만 만약 host variable이 사용되면 limit sort를 적용할 수 없다.
            // BUG-42041 SortColum으로 PSM Variable이 사용된 경우도 적용할 수 없다.
            if ( sLimit != NULL )
            {
                if ( (qmsLimitI::hasHostBind( sLimit ) == ID_FALSE) &&
                     ( sHasHostVar == ID_FALSE ) )
                {
                    // To Fix PR-8017
                    sLimitCnt = qmsLimitI::getStartConstant( sLimit ) +
                        qmsLimitI::getCountConstant( sLimit ) - 1;

                    if ( (sLimitCnt > 0) &&
                         (sLimitCnt <= QMN_LMST_MAXIMUM_LIMIT_CNT) )
                    {
                        // Memory Temp Table만을 사용하게 된다.

                        // left 의 output record count 를 조정하여 계산한다.
                        sOutputRecordCnt = aGraph->left->costInfo.outputRecordCnt;
                        aGraph->left->costInfo.outputRecordCnt = sLimitCnt;

                        sSelAccessCost = qmoCost::getMemSortTempCost(
                            aStatement->mSysStat,
                            &(aGraph->left->costInfo),
                            QMO_COST_DEFAULT_NODE_CNT ); // node cnt

                        aGraph->left->costInfo.outputRecordCnt = sOutputRecordCnt;

                        sSelDiskCost   = 0.0;
                        sSelTotalCost  = sSelAccessCost;

                        sMyGraph->limitCnt = sLimitCnt;
                        sMyGraph->graph.flag &= ~QMG_SORT_OPT_TIP_MASK;
                        sMyGraph->graph.flag |= QMG_SORT_OPT_TIP_LMST;
                    }
                    else if ( sLimitCnt == 0 )
                    {
                        sSelAccessCost = 0.0;
                        sSelDiskCost   = 0.0;
                        sSelTotalCost  = 0.0;

                        sMyGraph->limitCnt = sLimitCnt;
                        sMyGraph->graph.flag &= ~QMG_SORT_OPT_TIP_MASK;
                        sMyGraph->graph.flag |= QMG_SORT_OPT_TIP_LMST;
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
    else
    {
        // nothing to do
    }

    // preserved order 설정
    sMyGraph->graph.preservedOrder = sWantOrder;
    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;

    /* PROJ-1353 ROLLUP, CUBE가 하위 그래프일때 OPT TIP에 따른 VALUE TEMP 설정 여부 */
    if ( ( sMyGraph->graph.left->flag & QMG_GROUPBY_EXTENSION_MASK )
         == QMG_GROUPBY_EXTENSION_TRUE )
    {
        switch ( sMyGraph->graph.flag & QMG_SORT_OPT_TIP_MASK )
        {
            case QMG_SORT_OPT_TIP_LMST :
            case QMG_SORT_OPT_TIP_NONE : /* BUG-43727 SORT 사용 시, Disk/Memory에 관계없이 Value Temp를 사용한다. */
                /* Row를 Value로 쌓기를 Rollup, Cube에 설정한다. */
                sMyGraph->graph.left->flag &= ~QMG_VALUE_TEMP_MASK;
                sMyGraph->graph.left->flag |= QMG_VALUE_TEMP_TRUE;
                break;
            default:
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-39384  A some rollup query result is wrong 
     * with distinct, order by clause.
     */
    if ( sMyGraph->graph.left->type == QMG_DISTINCTION )
    {
        sChild = sMyGraph->graph.left;

        if ( sChild->left != NULL )
        {
            /* PROJ-1353 Rollup, Cube와 같이 사용될 때 */
            if ( ( sChild->left->flag & QMG_GROUPBY_EXTENSION_MASK )
                 == QMG_GROUPBY_EXTENSION_TRUE )
            {
                /* Row를 Value로 쌓기를 Rollup, Cube에 설정한다. */
                sChild->left->flag &= ~QMG_VALUE_TEMP_MASK;
                sChild->left->flag |= QMG_VALUE_TEMP_TRUE;
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
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    // outputRecordCnt
    if ( (sMyGraph->graph.flag & QMG_SORT_OPT_TIP_MASK) == QMG_SORT_OPT_TIP_LMST )
    {
        if ( sLimitCnt == 0 )
        {
            sMyGraph->graph.costInfo.outputRecordCnt = 1;
        }
        else
        {
            sMyGraph->graph.costInfo.outputRecordCnt = sLimitCnt;
        }
    }
    else
    {
        sMyGraph->graph.costInfo.outputRecordCnt =
            sMyGraph->graph.left->costInfo.outputRecordCnt;
    }

    // myCost
    sMyGraph->graph.costInfo.myAccessCost = sSelAccessCost;
    sMyGraph->graph.costInfo.myDiskCost   = sSelDiskCost;
    sMyGraph->graph.costInfo.myAllCost    = sSelTotalCost;

    // total cost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->graph.left->costInfo.totalAccessCost +
        sMyGraph->graph.costInfo.myAccessCost;
    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->graph.left->costInfo.totalDiskCost +
        sMyGraph->graph.costInfo.myDiskCost;
    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->graph.left->costInfo.totalAllCost +
        sMyGraph->graph.costInfo.myAllCost;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
    aGraph->flag |= (aGraph->left->flag & QMG_PARALLEL_EXEC_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmgSorting::makePlan( qcStatement   * aStatement,
                             const qmgGraph* aParent,
                             qmgGraph      * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSorting으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgSorting으로 부터 생성가능한 Plan
 *
 *         1.  Indexable Order By 최적화가 적용된 경우
 *
 *                  없음
 *
 *         2.  Limit Sort 최적화가 적용된 경우
 *
 *                  [LMST]
 *
 *         3.  최적화가 없는 경우
 *
 *                  [SORT]
 *
 ***********************************************************************/

    qmgSORT         * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgSorting::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgSORT*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // Materialize 될 경우 하위 노드들은 한번만 실행되고
    // materialized 된 내용만 참조한다.
    // 따라서 자식 노드에게 SCAN 에 대한 parallel 을 허용한다.
    aGraph->left->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aGraph->left->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    sMyGraph->graph.myPlan = aParent->myPlan;

    switch( sMyGraph->graph.flag & QMG_SORT_OPT_TIP_MASK )
    {
        case QMG_SORT_OPT_TIP_INDEXABLE_ORDERBY:
            IDE_TEST( makeChildPlan( aStatement,
                                     sMyGraph )
                      != IDE_SUCCESS );

            break;

        case QMG_SORT_OPT_TIP_LMST:
            IDE_TEST( makeLimitSort( aStatement,
                                     sMyGraph )
                      != IDE_SUCCESS );
            break;

        case QMG_SORT_OPT_TIP_NONE:
            IDE_TEST( makeSort( aStatement,
                                sMyGraph )
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
qmgSorting::makeChildPlan( qcStatement * aStatement,
                           qmgSORT     * aMyGraph )
{

    IDU_FIT_POINT_FATAL( "qmgSorting::makeChildPlan::__FT__" );

    //---------------------------------------------------
    // 하위 Plan의 생성
    //---------------------------------------------------

    IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                              &aMyGraph->graph ,
                                              aMyGraph->graph.left )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = aMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정
    //---------------------------------------------------
    aMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_ORDERBY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSorting::makeSort( qcStatement * aStatement,
                      qmgSORT     * aMyGraph )
{
    UInt              sFlag = 0;
    qmnPlan         * sSORT;
    qmgGraph        * sChild;

    IDU_FIT_POINT_FATAL( "qmgSorting::makeSort::__FT__" );

    //-----------------------------------------------------
    //        [SORT]
    //          |
    //        [LEFT]
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    /* BUG-36826 A rollup or cube occured wrong result using order by count_i3 */
    if ( ( aMyGraph->graph.left->flag & QMG_GROUPBY_EXTENSION_MASK )
         == QMG_GROUPBY_EXTENSION_TRUE )
    {
        if ( ( aMyGraph->graph.left->flag & QMG_VALUE_TEMP_MASK )
             == QMG_VALUE_TEMP_TRUE )
        {
            sFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
            sFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
        }
        else
        {
            if ( aMyGraph->graph.myQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
            {
                sFlag &= ~QMO_MAKESORT_GROUP_EXT_VALUE_MASK;
                sFlag |= QMO_MAKESORT_GROUP_EXT_VALUE_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* BUG-42303 A rollup/cube is wrong result with distinct and order by */
        if ( aMyGraph->graph.left->type == QMG_DISTINCTION )
        {
            sChild = aMyGraph->graph.left;

            if ( sChild->left != NULL )
            {
                /* PROJ-1353 Rollup, Cube와 같이 사용될 때 */
                if ( ( sChild->left->flag & QMG_GROUPBY_EXTENSION_MASK )
                     == QMG_GROUPBY_EXTENSION_TRUE )
                {
                    sFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
                    sFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
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

    //-----------------------
    // init SORT
    //-----------------------
    IDE_TEST( qmoOneMtrPlan::initSORT( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       sFlag,
                                       aMyGraph->orderBy,
                                       aMyGraph->graph.myPlan,
                                       &sSORT ) != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sSORT;

    //----------------------------
    // 하위 plan 생성
    //----------------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make SORT
    //-----------------------

    sFlag &= ~QMO_MAKESORT_METHOD_MASK;
    sFlag |= QMO_MAKESORT_ORDERBY;

    //FALSE가 되는 이유는 SORT_TIP이 없기때문이다. TRUE가되는
    //상황이라면 INDEXABLE ORDER BY TIP이 적용될것이다.
    sFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
    sFlag |= QMO_MAKESORT_PRESERVED_FALSE;

    //저장 매체의 선택
    if( (aMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
        QMG_GRAPH_TYPE_MEMORY )
    {
        sFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKESORT_MEMORY_TEMP_TABLE;
    }
    else
    {
        sFlag &= ~QMO_MAKESORT_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKESORT_DISK_TEMP_TABLE;
    }

    IDE_TEST( qmoOneMtrPlan::makeSORT( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       NULL ,
                                       aMyGraph->graph.myPlan ,
                                       aMyGraph->graph.costInfo.inputRecordCnt,
                                       sSORT ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sSORT;

    qmg::setPlanInfo( aStatement, sSORT, &(aMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSorting::makeLimitSort( qcStatement * aStatement,
                           qmgSORT     * aMyGraph )
{
    UInt              sFlag = 0;
    qmnPlan         * sLMST;
    qmgGraph        * sChild;

    IDU_FIT_POINT_FATAL( "qmgSorting::makeLimitSort::__FT__" );

    //-----------------------------------------------------
    //        [LMST]
    //          |
    //        [LEFT]
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    /* BUG-36826 A rollup or cube occured wrong result using order by count_i3 */
    if ( ( aMyGraph->graph.left->flag & QMG_GROUPBY_EXTENSION_MASK )
         == QMG_GROUPBY_EXTENSION_TRUE )
    {
        if ( ( aMyGraph->graph.left->flag & QMG_VALUE_TEMP_MASK )
             == QMG_VALUE_TEMP_TRUE )
        {
            sFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
            sFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
        }
        else
        {
            if ( aMyGraph->graph.myQuerySet->materializeType == QMO_MATERIALIZE_TYPE_VALUE )
            {
                sFlag &= ~QMO_MAKESORT_GROUP_EXT_VALUE_MASK;
                sFlag |= QMO_MAKESORT_GROUP_EXT_VALUE_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* BUG-42303 A rollup/cube is wrong result with distinct and order by */
        if ( aMyGraph->graph.left->type == QMG_DISTINCTION )
        {
            sChild = aMyGraph->graph.left;

            if ( sChild->left != NULL )
            {
                /* PROJ-1353 Rollup, Cube와 같이 사용될 때 */
                if ( ( sChild->left->flag & QMG_GROUPBY_EXTENSION_MASK )
                     == QMG_GROUPBY_EXTENSION_TRUE )
                {
                    sFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
                    sFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
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

    //-----------------------
    // init LMST
    //-----------------------

    IDE_TEST( qmoOneMtrPlan::initLMST( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       sFlag,
                                       aMyGraph->orderBy,
                                       aMyGraph->graph.myPlan,
                                       &sLMST ) != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sLMST;

    //----------------------------
    // 하위 plan 생성
    //----------------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make LMST
    //-----------------------

    sFlag &= ~QMO_MAKELMST_METHOD_MASK;
    sFlag |= QMO_MAKELMST_LIMIT_ORDERBY;

    IDE_TEST( qmoOneMtrPlan::makeLMST( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->limitCnt ,
                                       aMyGraph->graph.myPlan ,
                                       sLMST )
              != IDE_SUCCESS);

    aMyGraph->graph.myPlan = sLMST;

    qmg::setPlanInfo( aStatement, sLMST, &(aMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSorting::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgSorting::printGraph::__FT__" );

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

IDE_RC qmgSorting::getCostByPrevOrder( qcStatement       * aStatement,
                                       qmgSORT           * aSortGraph,
                                       qmgPreservedOrder * aWantOrder,
                                       SDouble           * aAccessCost,
                                       SDouble           * aDiskCost,
                                       SDouble           * aTotalCost )
{
/***********************************************************************
 *
 * Description :
 *
 *    Preserved Order 방식을 사용한 Sorting 비용을 계산한다.
 *
 * Implementation :
 *
 *    이미 Child가 원하는 Preserved Order를 가지고 있다면
 *    별도의 비용 없이 Sorting이 가능하다.
 *
 *    반면 Child에 특정 인덱스를 적용하는 경우라면,
 *    Child의 인덱스를 이용한 비용이 포함되게 된다.
 *
 ***********************************************************************/

    SDouble             sTotalCost;
    SDouble             sAccessCost;
    SDouble             sDiskCost;

    idBool              sUsable;

    qmoAccessMethod   * sOrgMethod;
    qmoAccessMethod   * sSelMethod;

    IDU_FIT_POINT_FATAL( "qmgSorting::getCostByPrevOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSortGraph != NULL );

    //------------------------------------------
    // Preserved Order를 사용할 수 있는 지를 검사
    //------------------------------------------

    // preserved order 적용 가능 검사
    IDE_TEST( qmg::checkUsableOrder( aStatement,
                                     aWantOrder,
                                     aSortGraph->graph.left,
                                     & sOrgMethod,
                                     & sSelMethod,
                                     & sUsable )
              != IDE_SUCCESS );

    //------------------------------------------
    // 비용 계산
    //------------------------------------------

    if ( sUsable == ID_TRUE )
    {
        if ( aSortGraph->graph.left->preservedOrder == NULL )
        {
            if ( (sOrgMethod == NULL) || (sSelMethod == NULL) )
            {
                // BUG-43824 sorting 비용을 계산할 때 access method가 NULL일 수 있습니다
                // 기존의 것을 이용하는 경우이므로 0을 설정한다.
                sAccessCost = 0;
                sDiskCost   = 0;
            }
            else
            {
                // 선택된 Access Method와 기존의 AccessMethod 차이만큼
                // 추가 비용이 발생한다.
                sAccessCost = IDL_MAX( ( sSelMethod->accessCost - sOrgMethod->accessCost ), 0 );
                sDiskCost   = IDL_MAX( ( sSelMethod->diskCost   - sOrgMethod->diskCost   ), 0 );
            }
        }
        else
        {
            // 이미 Child가 Ordering을 하고 있음.
            // 레코드 건수만큼의 비교 비용만이 소요됨.
            // BUG-41237 compare 비용만 추가한다.
            sAccessCost = aSortGraph->graph.left->costInfo.outputRecordCnt *
                aStatement->mSysStat->mCompareTime;
            sDiskCost   = 0;
        }
        sTotalCost  = sAccessCost + sDiskCost;
    }
    else
    {
        // Preserved Order를 사용할 수 없는 경우임.
        sAccessCost = QMO_COST_INVALID_COST;
        sDiskCost   = QMO_COST_INVALID_COST;
        sTotalCost  = QMO_COST_INVALID_COST;
    }

    *aTotalCost  = sTotalCost;
    *aDiskCost   = sDiskCost;
    *aAccessCost = sAccessCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
