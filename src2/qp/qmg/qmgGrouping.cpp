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
 * $Id: qmgGrouping.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * Description :
 *     Grouping Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgGrouping.h>
#include <qmgSelection.h>
#include <qmoCost.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoNormalForm.h>
#include <qmoSelectivity.h>
#include <qcgPlan.h>
#include <qmoParallelPlan.h>
#include <qmv.h>
#include <qcg.h>

/* BUG-46906 */
extern mtfModule mtfListagg;
extern mtfModule mtfPercentileDisc;
extern mtfModule mtfPercentileCont;

IDE_RC
qmgGrouping::init( qcStatement * aStatement,
                   qmsQuerySet * aQuerySet,
                   qmgGraph    * aChildGraph,
                   idBool        aIsNested,
                   qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgGrouping Graph의 초기화
 *
 * Implementation :
 *    (1) qmgGrouping을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgGROP      * sMyGraph;
    qtcNode      * sNormalForm;
    qtcNode      * sNode;
    qmoPredicate * sNewPred = NULL;
    qmoPredicate * sHavingPred = NULL;

    qmoNormalType sNormalType;

    IDU_FIT_POINT_FATAL( "qmgGrouping::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Grouping Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgGrouping을 위한 공간 할당
    IDU_FIT_POINT( "qmgGrouping::init::alloc::Graph" );
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgGROP),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( &sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_GROUPING;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myFrom = aChildGraph->myFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgGrouping::optimize;
    sMyGraph->graph.makePlan = qmgGrouping::makePlan;
    sMyGraph->graph.printGraph = qmgGrouping::printGraph;

    // Disk/Memory 정보 설정
    switch(  aQuerySet->SFWGH->hints->interResultType )
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
    // Grouping Graph 만을 위한 기본 초기화
    //---------------------------------------------------

    if ( aIsNested == ID_TRUE )
    {
        // Nested Aggregation
        sMyGraph->graph.flag &= ~QMG_GROP_TYPE_MASK;
        sMyGraph->graph.flag |= QMG_GROP_TYPE_NESTED;
        sMyGraph->aggregation = aQuerySet->SFWGH->aggsDepth2;
        sMyGraph->groupBy = NULL;
        sMyGraph->having = NULL;
        sMyGraph->havingPred = NULL;
        sMyGraph->distAggArg = NULL;
    }
    else
    {
        // 일반 Aggregation
        sMyGraph->graph.flag &= ~QMG_GROP_TYPE_MASK;
        sMyGraph->graph.flag |= QMG_GROP_TYPE_GENERAL;
        sMyGraph->aggregation = aQuerySet->SFWGH->aggsDepth1;
        sMyGraph->groupBy = aQuerySet->SFWGH->group;

        // To Fix BUG-8287
        if ( aQuerySet->SFWGH->having != NULL )
        {
            // To Fix PR-12743 NNF Filter지원
            IDE_TEST( qmoCrtPathMgr
                      ::decideNormalType( aStatement,
                                          sMyGraph->graph.myFrom,
                                          aQuerySet->SFWGH->having,
                                          aQuerySet->SFWGH->hints,
                                          ID_TRUE, // CNF Only임
                                          & sNormalType )
                      != IDE_SUCCESS );

            switch ( sNormalType )
            {
                case QMO_NORMAL_TYPE_NNF:

                    if ( aQuerySet->SFWGH->having != NULL )
                    {
                        aQuerySet->SFWGH->having->lflag
                            &= ~QTC_NODE_NNF_FILTER_MASK;
                        aQuerySet->SFWGH->having->lflag
                            |= QTC_NODE_NNF_FILTER_TRUE;
                    }
                    else
                    {
                        // nothing to do
                    }

                    sMyGraph->having = aQuerySet->SFWGH->having;
                    sMyGraph->havingPred = NULL;
                    break;

                case QMO_NORMAL_TYPE_CNF:

                    IDE_TEST( qmoNormalForm
                              ::normalizeCNF( aStatement,
                                              aQuerySet->SFWGH->having,
                                              & sNormalForm )
                              != IDE_SUCCESS );

                    sMyGraph->having = sNormalForm;

                    for ( sNode = (qtcNode*)sNormalForm->node.arguments;
                          sNode != NULL;
                          sNode = (qtcNode*)sNode->node.next )
                    {
                        IDU_FIT_POINT( "qmgGrouping::init::alloc::NewPred" );
                        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                      ID_SIZEOF( qmoPredicate ),
                                      (void**) & sNewPred )
                                  != IDE_SUCCESS );

                        sNewPred->node = sNode;
                        sNewPred->flag = QMO_PRED_CLEAR;
                        sNewPred->mySelectivity = 1;
                        sNewPred->totalSelectivity = 1;
                        sNewPred->next = NULL;
                        sNewPred->more = NULL;

                        if ( sHavingPred == NULL )
                        {
                            sMyGraph->havingPred = sHavingPred = sNewPred;
                        }
                        else
                        {
                            sHavingPred->next = sNewPred;
                            sHavingPred = sHavingPred->next;
                        }
                    }
                    break;

                case QMO_NORMAL_TYPE_DNF:
                case QMO_NORMAL_TYPE_NOT_DEFINED:
                default:
                    IDE_FT_ASSERT( 0 );
                    break;
            }
        }
        else
        {
            sMyGraph->having = NULL;
            sMyGraph->havingPred = NULL;
        }
    }


    sMyGraph->hashBucketCnt = 0;
    sMyGraph->distAggArg = NULL;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgGrouping::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgGrouping의 최적화
 *
 * Implementation :
 *    (1) 일반 Grouping인 경우 다음을 수행
 *       A. Subquery graph 생성
 *         - 일반 Grouping : having절,aggsDepth1의 subquery 처리
 *         - nested Grouping :  aggsDepth2의 subquery 처리
 *       B. Grouping 최적화 적용
 *         ( 최적화 적용 시 Preserved Order 생성 및 설정됨 )
 *    (2) groupingMethod 선택 및 Preserved Order flag 설정
 *        - Hash Based Grouping : Never Preserved Order
 *        - Sort Based Grouping : Preserved Order Defined
 *    (3) hash based grouping인 경우, hashBucketCnt 설정
 *    (4) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgGROP            * sMyGraph;
    qmsAggNode         * sAggr;
    qmsConcatElement   * sGroup;
    qmsConcatElement   * sSubGroup;
    qmoDistAggArg      * sNewDistAggr;
    qmoDistAggArg      * sFirstDistAggr;
    qmoDistAggArg      * sCurDistAggr;
    qtcNode            * sNode;
    mtcColumn          * sMtcColumn;
    qtcNode            * sList;

    idBool               sIsGroupExt;
    idBool               sEnableParallel = ID_TRUE;
    SDouble              sRecordSize;
    SDouble              sAggrCost;
    idBool               sIsFuncData = ID_FALSE; /* BUG-46906 */
    qtcNode            * sArgument   = NULL;     /* BUG-46922 */

    IDU_FIT_POINT_FATAL( "qmgGrouping::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph       = (qmgGROP*)aGraph;
    sAggr          = sMyGraph->aggregation;
    sFirstDistAggr = NULL;
    sCurDistAggr   = NULL;
    sRecordSize    = 0;
    sAggrCost      = 0;
    sIsGroupExt    = ID_FALSE;

    // BUG-38416 count(column) to count(*)
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_COUNT_COLUMN_TO_COUNT_ASTAR );

    //------------------------------------------
    //    - recordSize 계산
    //    - Subquery Graph의 처리 및 distAggArg 설정
    //------------------------------------------

    if ( sAggr != NULL )
    {
        for ( ; sAggr != NULL; sAggr = sAggr->next )
        {
            sNode = sAggr->aggr;

            sMtcColumn   = QTC_TMPL_COLUMN(QC_SHARED_TMPLATE(aStatement), sNode);
            sRecordSize += sMtcColumn->column.size;

            /* BUG-46906 BUG-46922 */
            if ( ( sNode->node.module == & mtfListagg ) ||
                 ( sNode->node.module == & mtfPercentileDisc ) ||
                 ( sNode->node.module == & mtfPercentileCont ) )
            {
                sIsFuncData = ID_TRUE;
            }
            else
            {
                /* BUG-46906 */
                sArgument = (qtcNode*)sAggr->aggr->node.arguments;

                if ( sArgument != NULL )
                {
                    if ( ( sArgument->node.module == & mtfListagg ) ||
                         ( sArgument->node.module == & mtfPercentileDisc ) ||
                         ( sArgument->node.module == & mtfPercentileCont ) )
                    {
                        sIsFuncData = ID_TRUE;
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

            // aggregation의 subquery 최적화
            IDE_TEST(
                qmoPred::optimizeSubqueryInNode( aStatement,
                                                 sAggr->aggr,
                                                 ID_FALSE,  //No Range
                                                 ID_FALSE ) //No ConstantFilter
                != IDE_SUCCESS );

            // distAggArg 설정
            if ( ( sAggr->aggr->node.lflag & MTC_NODE_DISTINCT_MASK )
                 == MTC_NODE_DISTINCT_TRUE )
            {
                IDU_FIT_POINT( "qmgGrouping::optimize::alloc::NewDistAggr" );
                IDE_TEST(
                    QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoDistAggArg ),
                                                   (void**)&sNewDistAggr )
                    != IDE_SUCCESS );

                sNode = (qtcNode*)sAggr->aggr->node.arguments;

                sNewDistAggr->aggArg  = sNode;
                sNewDistAggr->next    = NULL;

                if ( sFirstDistAggr == NULL )
                {
                    sFirstDistAggr = sCurDistAggr = sNewDistAggr;
                }
                else
                {
                    sCurDistAggr->next = sNewDistAggr;
                    sCurDistAggr = sCurDistAggr->next;
                }
            }
            else
            {
                // nothing to do
            }
        }
        sMyGraph->distAggArg = sFirstDistAggr;
    }
    else
    {
        // nothing to do
    }

    if ( ( sMyGraph->graph.flag & QMG_GROP_TYPE_MASK )
         == QMG_GROP_TYPE_GENERAL )
    {
        //------------------------------------------
        // 일반 Grouping 인 경우
        //    - group by 칼럼의 subquery 처리
        //    - having의 subquery 처리
        //    - Grouping Optimization Tip 적용
        //------------------------------------------

        // group의 subquery 처리
        for ( sGroup = sMyGraph->groupBy;
              sGroup != NULL;
              sGroup = sGroup->next )
        {
            if ( sGroup->type == QMS_GROUPBY_NORMAL )
            {
                sNode = sGroup->arithmeticOrList;

                sMtcColumn   = QTC_TMPL_COLUMN(QC_SHARED_TMPLATE(aStatement), sNode);
                sRecordSize += sMtcColumn->column.size;

                IDE_TEST(
                    qmoPred::optimizeSubqueryInNode( aStatement,
                                                     sGroup->arithmeticOrList,
                                                     ID_FALSE, // No Range
                                                     ID_FALSE )// No ConstantFilter
                    != IDE_SUCCESS );
            }
            else
            {
                /* PROJ-1353 */
                sMyGraph->graph.flag &= ~QMG_GROUPBY_EXTENSION_MASK;
                sMyGraph->graph.flag |= QMG_GROUPBY_EXTENSION_TRUE;

                switch ( sGroup->type )
                {
                    case QMS_GROUPBY_ROLLUP:
                        sMyGraph->graph.flag &= ~QMG_GROP_OPT_TIP_MASK;
                        sMyGraph->graph.flag |= QMG_GROP_OPT_TIP_ROLLUP;
                        break;
                    case QMS_GROUPBY_CUBE:
                        sMyGraph->graph.flag &= ~QMG_GROP_OPT_TIP_MASK;
                        sMyGraph->graph.flag |= QMG_GROP_OPT_TIP_CUBE;
                        break;
                    case QMS_GROUPBY_GROUPING_SETS:
                        sMyGraph->graph.flag &= ~QMG_GROP_OPT_TIP_MASK;
                        sMyGraph->graph.flag |= QMG_GROP_OPT_TIP_GROUPING_SETS;
                        break;
                    default:
                        break;
                }
                sIsGroupExt = ID_TRUE;
                for ( sSubGroup = sGroup->arguments;
                      sSubGroup != NULL;
                      sSubGroup = sSubGroup->next )
                {
                    if ( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                         == MTC_NODE_OPERATOR_LIST )
                    {
                        for ( sList = (qtcNode *)sSubGroup->arithmeticOrList->node.arguments;
                              sList != NULL;
                              sList = (qtcNode *)sList->node.next )
                        {
                            sNode = sList;

                            sMtcColumn   = QTC_TMPL_COLUMN(QC_SHARED_TMPLATE(aStatement), sNode);
                            sRecordSize += sMtcColumn->column.size;
                        }
                    }
                    else
                    {
                        sNode = sSubGroup->arithmeticOrList;

                        sMtcColumn   = QTC_TMPL_COLUMN(QC_SHARED_TMPLATE(aStatement), sNode);
                        sRecordSize += sMtcColumn->column.size;
                    }
                }
            }
        }

        if ( sMyGraph->having != NULL )
        {
            // having의 subquery 처리
            IDE_TEST(
                qmoPred::optimizeSubqueryInNode( aStatement,
                                                 sMyGraph->having,
                                                 ID_FALSE, // No Range
                                                 ID_FALSE )// No ConstantFilter
                != IDE_SUCCESS );
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // Nested Grouping 인 경우
        // nothing to do
    }
    // BUG-36463 sRecordSize 는 0이 되어서는 안된다.
    sRecordSize = IDL_MAX( sRecordSize, 1 );

    //------------------------------------------
    // Grouping Method 선택 및 Preserved Order 설정
    //------------------------------------------

    // TASK-6699 TPC-H 성능 개선
    // AGGR 함수가 늘어날수록 수행시간이 증가함
    sAggrCost = qmoCost::getAggrCost( aStatement->mSysStat,
                                      sMyGraph->aggregation,
                                      sMyGraph->graph.left->costInfo.outputRecordCnt );

    if ( sIsGroupExt == ID_FALSE )
    {
        //------------------------------------------
        // Hash Bucket Count 설정
        //------------------------------------------

        // BUG-38132 group by의 temp table 을 메모리로 고정하는 프로퍼티
        if ( (QCU_OPTIMIZER_FIXED_GROUP_MEMORY_TEMP == 1) &&
             (sMyGraph->distAggArg == NULL) )
        {
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;

            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_OPTIMIZER_FIXED_GROUP_MEMORY_TEMP );
        }
        else
        {
            // nothing todo.
        }

        if ( sMyGraph->groupBy != NULL )
        {
            IDE_TEST(
                getBucketCnt4Group(
                    aStatement,
                    sMyGraph,
                    sMyGraph->graph.myQuerySet->SFWGH->hints->groupBucketCnt,
                    & sMyGraph->hashBucketCnt )
                != IDE_SUCCESS );
        }
        else
        {
            // BUG-37074
            // group by 가 없을때는 bucket 갯수는 1
            sMyGraph->hashBucketCnt = 1;
        }

        if ( sMyGraph->groupBy != NULL )
        {
            // GROUP BY 가 존재하는 경우 Grouping Method를 결정함.
            IDE_TEST( setGroupingMethod( aStatement,
                                         sMyGraph,
                                         sRecordSize,
                                         sAggrCost ) != IDE_SUCCESS );
        }
        else
        {
            // GROUP BY가 존재하지 않는 경우
            sMyGraph->graph.flag &= ~QMG_GROUPBY_NONE_MASK;
            sMyGraph->graph.flag |= QMG_GROUPBY_NONE_TRUE;

            // 다양한 최적화 Tip의 적용이 가능한 지 검사
            IDE_TEST( nonGroupOptTip( aStatement,
                                      sMyGraph,
                                      sRecordSize,
                                      sAggrCost ) != IDE_SUCCESS );
        }
    }
    else
    {
        /* PROJ-1353 */
        IDE_TEST( setPreOrderGroupExtension( aStatement, sMyGraph, sRecordSize )
                  != IDE_SUCCESS );
    }

    /* BUG-46906 */
    if ( sIsFuncData == ID_TRUE )
    {
        sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
        sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_SORT;
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-37074
    if( sMyGraph->distAggArg != NULL )
    {
        for( sCurDistAggr = sMyGraph->distAggArg;
             sCurDistAggr != NULL;
             sCurDistAggr = sCurDistAggr->next )
        {
            // group by 를 고려하여 distinct 의 bucket 갯수를 지정.
            IDE_TEST( qmg::getBucketCnt4DistAggr(
                          aStatement,
                          sMyGraph->graph.left->costInfo.outputRecordCnt,
                          sMyGraph->hashBucketCnt,
                          sCurDistAggr->aggArg,
                          sMyGraph->graph.myQuerySet->SFWGH->hints->groupBucketCnt,
                          &sCurDistAggr->bucketCnt )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // nothing todo.
    }

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // recordSize = group by column size + aggregation column size
    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // selectivity
    IDE_TEST( qmoSelectivity::setGroupingSelectivity(
                  & sMyGraph->graph,
                  sMyGraph->havingPred,
                  sMyGraph->graph.left->costInfo.outputRecordCnt,
                  & sMyGraph->graph.costInfo.selectivity )
              != IDE_SUCCESS );

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    // outputRecordCnt
    IDE_TEST( qmoSelectivity::setGroupingOutputCnt(
                  aStatement,
                  & sMyGraph->graph,
                  sMyGraph->groupBy,
                  sMyGraph->graph.costInfo.inputRecordCnt,
                  sMyGraph->graph.costInfo.selectivity,
                  & sMyGraph->graph.costInfo.outputRecordCnt )
              != IDE_SUCCESS );

    //----------------------------------
    // 해당 Graph의 비용 정보 설정
    //----------------------------------

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->graph.left->costInfo.totalAccessCost +
        sMyGraph->graph.costInfo.myAccessCost;
    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->graph.left->costInfo.totalDiskCost +
        sMyGraph->graph.costInfo.myDiskCost;
    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->graph.left->costInfo.totalAllCost +
        sMyGraph->graph.costInfo.myAllCost;

    // PROJ-2444 Parallel Aggreagtion
    for ( sAggr = sMyGraph->aggregation; sAggr != NULL; sAggr = sAggr->next )
    {
        // psm not support
        if( ( sAggr->aggr->lflag & QTC_NODE_PROC_FUNCTION_MASK )
            == QTC_NODE_PROC_FUNCTION_FALSE )
        {
            //nothing to do
        }
        else
        {
            sEnableParallel = ID_FALSE;
            break;
        }

        // subQ not support
        if( ( sAggr->aggr->lflag & QTC_NODE_SUBQUERY_MASK )
            == QTC_NODE_SUBQUERY_ABSENT )
        {
            //nothing to do
        }
        else
        {
            sEnableParallel = ID_FALSE;
            break;
        }

        // BUG-43830 parallel aggregation
        if ( ( QTC_STMT_EXECUTE( aStatement, sAggr->aggr )->merge != mtf::calculateNA ) ||
             ( QTC_STMT_EXECUTE( aStatement, sAggr->aggr )->merge == NULL ) )
        {
            //nothing to do
        }
        else
        {
            sEnableParallel = ID_FALSE;
            break;
        }
    }

    for ( sGroup = sMyGraph->groupBy; sGroup != NULL; sGroup = sGroup->next )
    {
        // psm not support
        if ( sGroup->arithmeticOrList != NULL )
        {
            if ( ( sGroup->arithmeticOrList->lflag & QTC_NODE_PROC_FUNCTION_MASK )
                 == QTC_NODE_PROC_FUNCTION_FALSE )
            {
                //nothing to do
            }
            else
            {
                sEnableParallel = ID_FALSE;
                break;
            }
        }
        else
        {
            //nothing to do
        }
    }

    // PROJ-2444 Parallel Aggreagtion
    if ( sEnableParallel == ID_TRUE )
    {
        aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
        aGraph->flag |= (aGraph->left->flag & QMG_PARALLEL_EXEC_MASK);
    }
    else
    {
        aGraph->flag &= ~QMG_PARALLEL_EXEC_MASK;
        aGraph->flag |= QMG_PARALLEL_EXEC_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
qmgGrouping::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgGrouping으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgGrouping으로 부터 생성가능한 Plan
 *         1.  최적화 Tip 이 적용된 경우
 *
 *             A.  Indexable Distinct Aggregation인 경우
 *                 예) SELECT SUM(DISTINCT I1) FROM T1;
 *
 *                       [GRAG]        : SUM(I1)을 처리
 *                         |
 *                       [GRBY]        : DISTINCT I1의 처리
 *                                     : distinct option을 사용
 *
 *             B.  COUNT ASTERISK 최적화가 적용된 경우
 *
 *                    [CUNT]
 *
 *             C.  Indexable Min-Max 최적화가 적용된 경우
 *
 *                    없음
 *
 *         2.  GROUP BY의 처리
 *
 *             - Sort-based인 경우
 *
 *                    [GRBY]
 *                      |
 *                  ( [SORT] ) : Indexable Group By인 경우 생성되지 않음
 *
 *             - Hash-based인 경우
 *
 *                    없음
 *
 *             - Group By 가 없는 경우
 *
 *                    없음
 *
 *         3.  Aggregation의 처리
 *             - Sort-based인 경우 ( Distinct Aggregation도 포함 )
 *
 *                    [AGGR]
 *                      |
 *                    Group By의 처리
 *
 *             - Hash-based인 경우
 *
 *                    [GRAG]
 *
 ***********************************************************************/

    qmgGROP          * sMyGraph;
    qmnPlan          * sFILT;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgGROP*) aGraph;

    sMyGraph->graph.myPlan = aParent->myPlan;

    //----------------------------
    // Having절의 처리
    //----------------------------

    if( sMyGraph->having != NULL )
    {
        IDE_TEST( qmoOneNonPlan::initFILT( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           sMyGraph->having ,
                                           sMyGraph->graph.myPlan,
                                           &sFILT ) != IDE_SUCCESS);
        sMyGraph->graph.myPlan = sFILT;
    }
    else
    {
        //nothing to do
    }

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // Materialize 될 경우 하위 노드들은 한번만 실행되고
    // materialized 된 내용만 참조한다.
    // 따라서 자식 노드에게 SCAN 에 대한 parallel 을 허용한다.
    aGraph->left->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
    aGraph->left->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;

    switch( sMyGraph->graph.flag & QMG_GROP_OPT_TIP_MASK )
    {
        case QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY:
        case QMG_GROP_OPT_TIP_NONE:
            switch( sMyGraph->graph.flag & QMG_SORT_HASH_METHOD_MASK )
            {
                case QMG_SORT_HASH_METHOD_SORT:
                    IDE_TEST( makeSortGroup( aStatement, sMyGraph )
                              != IDE_SUCCESS );
                    break;

                case QMG_SORT_HASH_METHOD_HASH:
                    IDE_TEST( makeHashGroup( aStatement, sMyGraph )
                              != IDE_SUCCESS );
                    break;
            }
            break;
        case QMG_GROP_OPT_TIP_COUNT_STAR:
            IDE_TEST( makeCountAll( aStatement,
                                    sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMG_GROP_OPT_TIP_INDEXABLE_MINMAX:
            IDE_TEST( makeIndexMinMax( aStatement,
                                       sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG:
            IDE_TEST( makeIndexDistAggr( aStatement,
                                         sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMG_GROP_OPT_TIP_ROLLUP:
            IDE_TEST( makeRollup( aStatement, sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMG_GROP_OPT_TIP_CUBE:
            IDE_TEST( makeCube( aStatement, sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMG_GROP_OPT_TIP_GROUPING_SETS:
            break;
        default:
            break;
    }

    //----------------------------
    // Having절의 처리
    //----------------------------

    if( sMyGraph->having != NULL )
    {
        IDE_TEST( qmoOneNonPlan::makeFILT( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           sMyGraph->having ,
                                           NULL ,
                                           sMyGraph->graph.myPlan ,
                                           sFILT ) != IDE_SUCCESS);
        sMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(sMyGraph->graph) );
    }
    else
    {
        //nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::makeChildPlan( qcStatement * aStatement,
                            qmgGROP     * aMyGraph )
{
    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeChildPlan::__FT__" );

    //---------------------------------------------------
    // 하위 plan의 생성
    //---------------------------------------------------
    IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                              &aMyGraph->graph,
                                              aMyGraph->graph.left )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = aMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정
    //---------------------------------------------------
    aMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_GROUPBY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::makeHashGroup( qcStatement * aStatement,
                            qmgGROP     * aMyGraph )
{
    UInt                sFlag;
    qmnPlan           * sGRAG         = NULL;
    qmnPlan           * sParallelGRAG = NULL;
    qmnChildren       * sChildren;
    qmcAttrDesc       * sResultDescOrg;
    qmcAttrDesc       * sResultDesc;
    UShort              sSourceTable;
    UShort              sDestTable;
    idBool              sMakeParallel;

    //-----------------------------------------------------
    //        [GRAG]
    //           |
    //        [LEFT]
    //-----------------------------------------------------

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeHashGroup::__FT__" );

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //----------------------------
    // init GRAG
    //----------------------------

    IDE_TEST( qmoOneMtrPlan::initGRAG(
                  aStatement ,
                  aMyGraph->graph.myQuerySet ,
                  aMyGraph->aggregation ,
                  aMyGraph->groupBy ,
                  aMyGraph->graph.myPlan,
                  &sGRAG )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRAG;

    //----------------------------
    // 하위 plan 생성
    //----------------------------
    IDE_TEST( makeChildPlan( aStatement, aMyGraph ) != IDE_SUCCESS );

    // PROJ-2444
    // Parallel 플랜의 생성여부를 결정한다.
    sMakeParallel = checkParallelEnable( aMyGraph );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //----------------------------
    // make GRAG
    //----------------------------

    sFlag = 0;

    //저장 매체의 선택
    if( aMyGraph->groupBy == NULL )
    {
        //Group by 컬럼이 없다면 저장 매체를 memory로 선택
        sFlag &= ~QMO_MAKEGRAG_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEGRAG_MEMORY_TEMP_TABLE;
    }
    else
    {
        //Graph에서 지정
        if( (aMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
            QMG_GRAPH_TYPE_MEMORY )
        {
            sFlag &= ~QMO_MAKEGRAG_TEMP_TABLE_MASK;
            sFlag |= QMO_MAKEGRAG_MEMORY_TEMP_TABLE;
        }
        else
        {
            sFlag &= ~QMO_MAKEGRAG_TEMP_TABLE_MASK;
            sFlag |= QMO_MAKEGRAG_DISK_TEMP_TABLE;
        }
    }

    // PROJ-2444 Parallel Aggreagtion
    if ( sMakeParallel == ID_TRUE )
    {
        //-----------------------------------------------------
        //        [GRAG ( merge ) ]
        //           |
        //        [GRAG ( aggr ) ]
        //           |
        //        [LEFT]
        //-----------------------------------------------------

        if ( aMyGraph->graph.left->myPlan->type == QMN_PSCRD )
        {
            sFlag &= ~QMO_MAKEGRAG_PARALLEL_STEP_MASK;
            sFlag |= QMO_MAKEGRAG_PARALLEL_STEP_AGGR;

            sDestTable   = aMyGraph->graph.left->myFrom->tableRef->table;

            for ( sChildren  = aMyGraph->graph.myPlan->childrenPRLQ;
                  sChildren != NULL;
                  sChildren  = sChildren->next )
            {
                // 기존 GRAG 노드를 복사한다.
                IDE_TEST( qmoParallelPlan::copyGRAG( aStatement,
                                                     aMyGraph->graph.myQuerySet,
                                                     sDestTable,
                                                     sGRAG,
                                                     &sParallelGRAG )
                          != IDE_SUCCESS );

                // initGRAG 에서 ResultDesc->expr Node 의 값이 변경된다.
                // 따라서 Node를 복사를 해야한다.
                for( sResultDescOrg  = sGRAG->resultDesc,
                         sResultDesc     = sParallelGRAG->resultDesc;
                     sResultDescOrg != NULL;
                     sResultDescOrg  = sResultDescOrg->next,
                         sResultDesc     = sResultDesc->next )
                {
                    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                                     sResultDescOrg->expr,
                                                     &sResultDesc->expr,
                                                     ID_FALSE,
                                                     ID_FALSE,
                                                     ID_FALSE,
                                                     ID_FALSE )
                              != IDE_SUCCESS );
                }
                IDE_DASSERT( sResultDesc == NULL );

                IDE_TEST( qmoOneMtrPlan::makeGRAG(
                              aStatement,
                              aMyGraph->graph.myQuerySet,
                              sFlag,
                              aMyGraph->groupBy,
                              aMyGraph->hashBucketCnt,
                              aMyGraph->graph.myPlan,
                              sParallelGRAG )
                          != IDE_SUCCESS);

                qmg::setPlanInfo( aStatement, sParallelGRAG, &(aMyGraph->graph) );

                // PSCRD 의 경우는 PRLQ-SCAN 순으로 달려있다.
                IDE_DASSERT( sChildren->childPlan->type       == QMN_PRLQ );
                IDE_DASSERT( sChildren->childPlan->left->type == QMN_SCAN );

                sParallelGRAG->left        = sChildren->childPlan->left;
                sChildren->childPlan->left = sParallelGRAG;
            }

            sFlag &= ~QMO_MAKEGRAG_PARALLEL_STEP_MASK;
            sFlag |= QMO_MAKEGRAG_PARALLEL_STEP_MERGE;

            IDE_TEST( qmoOneMtrPlan::makeGRAG(
                          aStatement,
                          aMyGraph->graph.myQuerySet,
                          sFlag,
                          aMyGraph->groupBy,
                          aMyGraph->hashBucketCnt,
                          aMyGraph->graph.myPlan,
                          sGRAG )
                      != IDE_SUCCESS);
        }
        else if ( aMyGraph->graph.left->myPlan->type == QMN_PPCRD )
        {
            sFlag &= ~QMO_MAKEGRAG_PARALLEL_STEP_MASK;
            sFlag |= QMO_MAKEGRAG_PARALLEL_STEP_AGGR;

            for ( sChildren  = aMyGraph->graph.myPlan->children;
                  sChildren != NULL;
                  sChildren  = sChildren->next )
            {
                sSourceTable = aMyGraph->graph.left->myFrom->tableRef->table;
                sDestTable   = sChildren->childPlan->depInfo.depend[0];

                IDE_TEST( qmoParallelPlan::copyGRAG( aStatement,
                                                     aMyGraph->graph.myQuerySet,
                                                     sDestTable,
                                                     sGRAG,
                                                     &sParallelGRAG )
                          != IDE_SUCCESS );

                for( sResultDescOrg  = sGRAG->resultDesc,
                         sResultDesc     = sParallelGRAG->resultDesc;
                     sResultDescOrg != NULL;
                     sResultDescOrg  = sResultDescOrg->next,
                         sResultDesc     = sResultDesc->next )
                {
                    // 파티션의 경우 SCAN 노드의 tuple 값이 서로 다르다.
                    // 따라서 새로 생성된 Node 가 해당 파티션의 tuple을 바라보도록 수정해주어야 한다.
                    IDE_TEST( qtc::cloneQTCNodeTree4Partition(
                                  QC_QMP_MEM(aStatement),
                                  sResultDescOrg->expr,
                                  &sResultDesc->expr,
                                  sSourceTable,
                                  sDestTable,
                                  ID_FALSE )
                              != IDE_SUCCESS );

                    // cloneQTCNodeTree4Partition 함수에서 컨버젼 노드를 삭제한다.
                    // 재 생성해야 한다.
                    IDE_TEST( qtc::estimate(
                                  sResultDesc->expr,
                                  QC_SHARED_TMPLATE(aStatement),
                                  aStatement,
                                  aMyGraph->graph.myQuerySet,
                                  aMyGraph->graph.myQuerySet->SFWGH,
                                  NULL )
                              != IDE_SUCCESS );
                }
                IDE_DASSERT( sResultDesc == NULL );

                IDE_TEST( qmoOneMtrPlan::makeGRAG(
                              aStatement,
                              aMyGraph->graph.myQuerySet,
                              sFlag,
                              aMyGraph->groupBy,
                              aMyGraph->hashBucketCnt,
                              aMyGraph->graph.myPlan,
                              sParallelGRAG )
                          != IDE_SUCCESS);

                qmg::setPlanInfo( aStatement, sParallelGRAG, &(aMyGraph->graph) );

                // PPCRD 의 경우는 SCAN 만 달려있다.
                IDE_DASSERT( sChildren->childPlan->type       == QMN_SCAN );

                sParallelGRAG->left     = sChildren->childPlan;
                sChildren->childPlan    = sParallelGRAG;
            }

            sFlag &= ~QMO_MAKEGRAG_PARALLEL_STEP_MASK;
            sFlag |=  QMO_MAKEGRAG_PARALLEL_STEP_MERGE;

            IDE_TEST( qmoOneMtrPlan::makeGRAG(
                          aStatement,
                          aMyGraph->graph.myQuerySet,
                          sFlag,
                          aMyGraph->groupBy,
                          aMyGraph->hashBucketCnt,
                          aMyGraph->graph.myPlan,
                          sGRAG )
                      != IDE_SUCCESS);
        }
        else
        {
            IDE_DASSERT( 0 );
        }
    }
    else
    {
        IDE_TEST( qmoOneMtrPlan::makeGRAG(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFlag ,
                      aMyGraph->groupBy ,
                      aMyGraph->hashBucketCnt ,
                      aMyGraph->graph.myPlan ,
                      sGRAG )
                  != IDE_SUCCESS);
    }

    aMyGraph->graph.myPlan = sGRAG;

    qmg::setPlanInfo( aStatement, sGRAG, &(aMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::makeSortGroup( qcStatement * aStatement,
                            qmgGROP     * aMyGraph )
{
    UInt          sFlag;
    qmnPlan     * sAGGR = NULL;
    qmnPlan     * sGRBY = NULL;
    qmnPlan     * sSORT = NULL;

    //-----------------------------------------------------
    //        [*AGGR]
    //           |
    //        [*GRBY]
    //           |
    //        [*SORT]
    //           |
    //        [LEFT]
    // * Option
    //-----------------------------------------------------

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeSortGroup::__FT__" );

    //----------------------------
    // Top-down 초기화
    //----------------------------

    if( aMyGraph->aggregation != NULL )
    {
        //-----------------------
        // init AGGR
        //-----------------------

        IDE_TEST( qmoOneNonPlan::initAGGR(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      aMyGraph->aggregation ,
                      aMyGraph->groupBy ,
                      aMyGraph->graph.myPlan,
                      &sAGGR ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sAGGR;
    }
    else
    {
        //nothing to od
    }

    if( aMyGraph->groupBy != NULL )
    {
        //-----------------------
        // init GRBY
        //-----------------------

        IDE_TEST( qmoOneNonPlan::initGRBY(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      aMyGraph->aggregation ,
                      aMyGraph->groupBy ,
                      &sGRBY ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sGRBY;

        if( (aMyGraph->graph.flag & QMG_GROP_OPT_TIP_MASK) !=
            QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY )
        {
            //-----------------------
            // init SORT
            //-----------------------

            IDE_TEST( qmoOneMtrPlan::initSORT(
                          aStatement ,
                          aMyGraph->graph.myQuerySet ,
                          aMyGraph->graph.myPlan,
                          &sSORT ) != IDE_SUCCESS);
            aMyGraph->graph.myPlan = sSORT;
        }
        else
        {
            //nothing to do
        }
    }
    else
    {
        //nothing to do
    }

    //----------------------------
    // 하위 plan 생성
    //----------------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    if( aMyGraph->groupBy != NULL )
    {
        if( (aMyGraph->graph.flag & QMG_GROP_OPT_TIP_MASK) !=
            QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY )
        {
            //-----------------------
            // make SORT
            //-----------------------
            sFlag = 0;
            sFlag &= ~QMO_MAKESORT_METHOD_MASK;
            sFlag |= QMO_MAKESORT_SORT_BASED_GROUPING;
            sFlag &= ~QMO_MAKESORT_POSITION_MASK;
            sFlag |= QMO_MAKESORT_POSITION_LEFT;
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

            IDE_TEST( qmoOneMtrPlan::makeSORT(
                          aStatement ,
                          aMyGraph->graph.myQuerySet ,
                          sFlag ,
                          aMyGraph->graph.preservedOrder ,
                          aMyGraph->graph.myPlan ,
                          aMyGraph->graph.costInfo.inputRecordCnt,
                          sSORT ) != IDE_SUCCESS);
            aMyGraph->graph.myPlan = sSORT;

            qmg::setPlanInfo( aStatement, sSORT, &(aMyGraph->graph) );
        }
        else
        {
            //nothing to do
        }

        //-----------------------
        // make GRBY
        //-----------------------
        sFlag = 0;
        sFlag &= ~QMO_MAKEGRBY_SORT_BASED_METHOD_MASK;

        if( aMyGraph->aggregation != NULL )
        {
            sFlag |= QMO_MAKEGRBY_SORT_BASED_GROUPING;
        }
        else
        {
            sFlag |= QMO_MAKEGRBY_SORT_BASED_DISTAGGR;
        }

        IDE_TEST( qmoOneNonPlan::makeGRBY(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFlag ,
                      aMyGraph->graph.myPlan ,
                      sGRBY ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sGRBY;

        qmg::setPlanInfo( aStatement, sGRBY, &(aMyGraph->graph) );
    }
    else
    {
        //nothing to do
    }

    if( aMyGraph->aggregation != NULL )
    {
        //-----------------------
        // make AGGR
        //-----------------------
        sFlag = 0;
        //저장 매체의 선택
        if( (aMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
            QMG_GRAPH_TYPE_MEMORY )
        {
            sFlag &= ~QMO_MAKEAGGR_TEMP_TABLE_MASK;
            sFlag |= QMO_MAKEAGGR_MEMORY_TEMP_TABLE;
        }
        else
        {
            sFlag &= ~QMO_MAKEAGGR_TEMP_TABLE_MASK;
            sFlag |= QMO_MAKEAGGR_DISK_TEMP_TABLE;
        }

        IDE_TEST( qmoOneNonPlan::makeAGGR(
                      aStatement ,
                      aMyGraph->graph.myQuerySet ,
                      sFlag ,
                      aMyGraph->distAggArg ,
                      aMyGraph->graph.myPlan ,
                      sAGGR ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sAGGR;

        qmg::setPlanInfo( aStatement, sAGGR, &(aMyGraph->graph) );
    }
    else
    {
        //nothing to od
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::makeCountAll( qcStatement * aStatement,
                           qmgGROP     * aMyGraph )
{
    qmoLeafInfo       sLeafInfo;
    qmnPlan         * sCUNT;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeCountAll::__FT__" );

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //----------------------------
    // init CUNT
    //----------------------------

    IDE_TEST( qmoOneNonPlan::initCUNT( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       aMyGraph->graph.myPlan,
                                       &sCUNT )
              != IDE_SUCCESS );

    //----------------------------
    // 하위 plan을 생성하지 않는다.
    //----------------------------

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //---------------------------------------------------
    // Current CNF의 등록
    //---------------------------------------------------

    if ( aMyGraph->graph.left->myCNF != NULL )
    {
        aMyGraph->graph.myQuerySet->SFWGH->crtPath->currentCNF =
            aMyGraph->graph.left->myCNF;
    }
    else
    {
        // Nothing To Do
    }

    sLeafInfo.predicate         = aMyGraph->graph.left->myPredicate;
    sLeafInfo.levelPredicate    = NULL;
    sLeafInfo.constantPredicate = aMyGraph->graph.left->constantPredicate;
    sLeafInfo.preservedOrder    = aMyGraph->graph.preservedOrder;
    sLeafInfo.ridPredicate      = aMyGraph->graph.left->ridPredicate;

    // BUG-17483 파티션 테이블 count(*) 지원
    if( aMyGraph->graph.left->type == QMG_SELECTION )
    {
        sLeafInfo.index             = ((qmgSELT*)aMyGraph->graph.left)->selectedIndex;
        sLeafInfo.sdf               = ((qmgSELT*)aMyGraph->graph.left)->sdf;
        sLeafInfo.forceIndexScan    = ((qmgSELT*)aMyGraph->graph.left)->forceIndexScan;
    }
    else if ( ( aMyGraph->graph.left->type == QMG_PARTITION ) ||
              ( aMyGraph->graph.left->type == QMG_SHARD_SELECT ) ) // PROJ-2638
    {
        sLeafInfo.index             = NULL;
        sLeafInfo.sdf               = NULL;
        sLeafInfo.forceIndexScan    = ID_FALSE;
    }
    else
    {
        IDE_FT_ASSERT( 0 );
    }

    //----------------------------
    // make CUNT
    //----------------------------

    IDE_TEST( qmoOneNonPlan::makeCUNT( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myFrom ,
                                       aMyGraph->aggregation ,
                                       &sLeafInfo ,
                                       sCUNT )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sCUNT;

    qmg::setPlanInfo( aStatement, sCUNT, &(aMyGraph->graph) );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::makeIndexMinMax( qcStatement * aStatement,
                              qmgGROP     * aMyGraph )
{
    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeIndexMinMax::__FT__" );

    //----------------------------
    // 하위 plan 생성
    //----------------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph )
              != IDE_SUCCESS );

    //해당 graph로 생성되는 node가 없으므로 하위의 childplan을
    //현재 graph의 myPlan으로 등록해둔다.
    //따라서 상위에서 하위 Plan을 찾을수 있다.
    aMyGraph->graph.myPlan = aMyGraph->graph.left->myPlan;

    // To Fix PR-9602
    // 다음과 같은 질의를 처리하기 위해서는
    // INDEXABLE MINMAX 최적화가 적용된 경우,
    // Conversion 유무에 관계 없이 정상 동작하게 하려면,
    // Aggregation Node의 ID를
    // 해당 argument의 ID로 변경시켜 주어야 한다.
    //     SELECT MIN(i1) FROM T1;
    //     SELECT MIN(i1) + 0.1 FROM T1;

    IDE_DASSERT( aMyGraph->aggregation != NULL );
    IDE_DASSERT( aMyGraph->aggregation->next == NULL );

    aMyGraph->aggregation->aggr->node.table
        = aMyGraph->aggregation->aggr->node.arguments->table;
    aMyGraph->aggregation->aggr->node.column
        = aMyGraph->aggregation->aggr->node.arguments->column;

    // PROJ-2179
    //aMyGraph->aggregation->aggr->node.arguments = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::makeIndexDistAggr( qcStatement * aStatement,
                                qmgGROP     * aMyGraph )
{
    UInt          sFlag;
    qmnPlan     * sGRBY;
    qmnPlan     * sGRAG;
    qmsConcatElement * sGroupBy;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeIndexDistAggr::__FT__" );

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //----------------------------
    // init GRAG
    //----------------------------

    IDE_TEST( qmoOneMtrPlan::initGRAG( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->aggregation ,
                                       NULL,
                                       aMyGraph->graph.myPlan ,
                                       &sGRAG )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRAG;

    //----------------------------
    // init GRBY
    //----------------------------

    // To Fix PR-7960
    // Aggregation의 Argument를 이용하여
    // Grouping 대상 Column을 구성한다.

    IDU_FIT_POINT( "qmgGrouping::makeIndexDistAggr::alloc::Group" );
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsConcatElement),
                                             (void **) & sGroupBy )
              != IDE_SUCCESS );
    sGroupBy->arithmeticOrList =
        (qtcNode*) aMyGraph->aggregation->aggr->node.arguments;
    sGroupBy->next = NULL;

    IDE_TEST( qmoOneNonPlan::initGRBY( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->aggregation,
                                       sGroupBy ,
                                       &sGRBY ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRBY;

    //----------------------------
    // 하위 plan 생성
    //----------------------------

    IDE_TEST( makeChildPlan( aStatement,
                             aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // make GRBY
    //----------------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEGRBY_SORT_BASED_METHOD_MASK;
    sFlag |= QMO_MAKEGRBY_SORT_BASED_DISTAGGR;

    // GRBY 노드의 생성
    IDE_TEST( qmoOneNonPlan::makeGRBY( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       aMyGraph->graph.myPlan ,
                                       sGRBY ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRBY;

    qmg::setPlanInfo( aStatement, sGRBY, &(aMyGraph->graph) );

    //----------------------------
    // GRAG 노드의 생성
    // - Aggregation을 처리 한다.
    //----------------------------

    sFlag = 0;

    //저장 매체의 선택
    //GROUP BY가 없는 경우 이다.(bug-7696)
    sFlag &= ~QMO_MAKEGRAG_TEMP_TABLE_MASK;
    sFlag |= QMO_MAKEGRAG_MEMORY_TEMP_TABLE;

    IDE_DASSERT( aMyGraph->groupBy == NULL );

    IDE_TEST( qmoOneMtrPlan::makeGRAG( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       NULL ,
                                       aMyGraph->hashBucketCnt ,
                                       aMyGraph->graph.myPlan ,
                                       sGRAG )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRAG;

    qmg::setPlanInfo( aStatement, sGRBY, &(aMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 makeRollup
 *
 * - Rollup 은 하위에 Sort Plan이 생성되며 만약 Index가 설정될 시에는 Sort를 생성하지 않는다.
 * - Memory Table일 경우 상위 Plan( Sort, Window Sort, Limit Sort )에서 데이터를 쌓아서
 *   처리할 필요가 있을 경우 Memory Table의 value를 STORE하는 Temp Table을 생성해서
 *   이 Table의 Row Pointer를 쌓아서 처리하도록 한다.
 */
IDE_RC qmgGrouping::makeRollup( qcStatement    * aStatement,
                                qmgGROP        * aMyGraph )
{
    UInt      sFlag        = 0;
    UInt      sRollupCount = 0;
    qmnPlan * sROLL        = NULL;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeRollup::__FT__" );

    IDE_DASSERT( aMyGraph->groupBy != NULL );

    IDE_TEST( qmoOneMtrPlan::initROLL( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       &sRollupCount,
                                       aMyGraph->aggregation,
                                       aMyGraph->groupBy,
                                       &sROLL )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sROLL;

    IDE_TEST( makeChildPlan( aStatement, aMyGraph )
              != IDE_SUCCESS );

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

    if ( ( aMyGraph->graph.flag & QMG_VALUE_TEMP_MASK )
         == QMG_VALUE_TEMP_TRUE )
    {
        sFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
        sFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( aMyGraph->graph.flag & QMG_CHILD_PRESERVED_ORDER_USE_MASK ) ==
         QMG_CHILD_PRESERVED_ORDER_CANNOT_USE )
    {
        sFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
        sFlag |= QMO_MAKESORT_PRESERVED_FALSE;
    }
    else
    {
        sFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
        sFlag |= QMO_MAKESORT_PRESERVED_TRUE;
    }

    IDE_TEST( qmoOneMtrPlan::makeROLL( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       sFlag,
                                       sRollupCount,
                                       aMyGraph->aggregation,
                                       aMyGraph->distAggArg,
                                       aMyGraph->groupBy,
                                       aMyGraph->graph.myPlan,
                                       sROLL )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sROLL;

    qmg::setPlanInfo( aStatement, sROLL, &(aMyGraph->graph) );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-1353 makeCube
 *
 * - Memory Table일 경우 상위 Plan( Sort, Window Sort, Limit Sort )에서 데이터를 쌓아서
 *   처리할 필요가 있을 경우 Memory Table의 value를 STORE하는 Temp Table을 생성해서
 *   이 Table의 Row Pointer를 쌓아서 처리하도록 한다.
 */
IDE_RC qmgGrouping::makeCube( qcStatement    * aStatement,
                              qmgGROP        * aMyGraph )
{
    UInt      sFlag        = 0;
    UInt      sCubeCount   = 0;
    qmnPlan * sCUBE        = NULL;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeCube::__FT__" );

    IDE_DASSERT( aMyGraph->groupBy != NULL );

    IDE_TEST( qmoOneMtrPlan::initCUBE( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       &sCubeCount,
                                       aMyGraph->aggregation,
                                       aMyGraph->groupBy,
                                       &sCUBE )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sCUBE;

    IDE_TEST( makeChildPlan( aStatement, aMyGraph )
              != IDE_SUCCESS );

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

    if ( ( aMyGraph->graph.flag & QMG_VALUE_TEMP_MASK )
         == QMG_VALUE_TEMP_TRUE )
    {
        sFlag &= ~QMO_MAKESORT_VALUE_TEMP_MASK;
        sFlag |= QMO_MAKESORT_VALUE_TEMP_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qmoOneMtrPlan::makeCUBE( aStatement,
                                       aMyGraph->graph.myQuerySet,
                                       sFlag,
                                       sCubeCount,
                                       aMyGraph->aggregation,
                                       aMyGraph->distAggArg,
                                       aMyGraph->groupBy,
                                       aMyGraph->graph.myPlan,
                                       sCUBE )
              != IDE_SUCCESS );
    aMyGraph->graph.myPlan = sCUBE;

    qmg::setPlanInfo( aStatement, sCUBE, &(aMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgGrouping::printGraph( qcStatement  * aStatement,
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

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::printGraph::__FT__" );

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
qmgGrouping::setGroupingMethod( qcStatement * aStatement,
                                qmgGROP     * aGroupGraph,
                                SDouble       aRecordSize,
                                SDouble       aAggrCost )
{
/***********************************************************************
 *
 * Description : GROUP BY 컬럼에 대한 최적화를 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool               sIndexable;
    qmoGroupMethodType   sGroupMethodHint;

    SDouble              sTotalCost     = 0;
    SDouble              sDiskCost      = 0;
    SDouble              sAccessCost    = 0;

    SDouble              sSelAccessCost = 0;
    SDouble              sSelDiskCost   = 0;
    SDouble              sSelTotalCost  = 0;
    idBool               sIsDisk;
    qmgGraph           * sGraph;
    UInt                 sSelectedMethod = 0;
    qmsAggNode         * sAggr;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::setGroupingMethod::__FT__" );

    //-------------------------------------------
    // 적합성 검사
    //-------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGroupGraph != NULL );
    IDE_DASSERT( aGroupGraph->groupBy != NULL );

    //-------------------------------------------
    // GROUPING METHOD HINT 추출
    //-------------------------------------------
    sGraph = &(aGroupGraph->graph);

    IDE_TEST( qmg::isDiskTempTable( sGraph, & sIsDisk ) != IDE_SUCCESS );

    /* TASK-6744 */
    if ( (QCU_PLAN_RANDOM_SEED != 0) &&
         (aStatement->mRandomPlanInfo != NULL) &&
         (smiGetStartupPhase() == SMI_STARTUP_SERVICE) )
    {
        aStatement->mRandomPlanInfo->mTotalNumOfCases = aStatement->mRandomPlanInfo->mTotalNumOfCases + 2;
        sSelectedMethod = QCU_PLAN_RANDOM_SEED % (aStatement->mRandomPlanInfo->mTotalNumOfCases - aStatement->mRandomPlanInfo->mWeightedValue);

        if ( sSelectedMethod >= 2 )
        {
            sSelectedMethod = sSelectedMethod % 2;
        }
        else
        {
            // Nothing to do.
        }

        IDE_DASSERT( sSelectedMethod < 2 );

        switch ( sSelectedMethod )
        {
            case 0 :
                sGroupMethodHint = QMO_GROUP_METHOD_TYPE_SORT;
                break;
            case 1 :
                sGroupMethodHint = QMO_GROUP_METHOD_TYPE_HASH;
                break;
            default :
                IDE_DASSERT( 0 );
                break;
        }

        aStatement->mRandomPlanInfo->mWeightedValue++;

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_PLAN_RANDOM_SEED );
    }
    else
    {
        // BUG-48132 group method property 추가 / grouping hint 우선
        if ( aGroupGraph->graph.myQuerySet->SFWGH->hints->groupMethodType == QMO_GROUP_METHOD_TYPE_NOT_DEFINED )
        {
            getPropertyGroupingMethod( aStatement,
                                       &sGroupMethodHint ); 
        }
        else
        {
            sGroupMethodHint =
                aGroupGraph->graph.myQuerySet->SFWGH->hints->groupMethodType;
        }
    }

    /* BUG-44250
     * aggregation에 group by 가 있고 하위에 hierarchy query가
     * 존재 하는데 aggregation의 인자로 prior가 쓰였다면 Sort로
     * 풀리지 않도록 한다. hint가 사용됐을 지라도
     */
    if ( ( aGroupGraph->aggregation != NULL ) &&
         ( aGroupGraph->groupBy != NULL ) &&
         ( aGroupGraph->graph.left->type == QMG_HIERARCHY ) )
    {
        for ( sAggr = aGroupGraph->aggregation;
              sAggr != NULL;
              sAggr = sAggr->next )
        {
            if ( ( sAggr->aggr->lflag & QTC_NODE_PRIOR_MASK )
                 == QTC_NODE_PRIOR_EXIST )
            {
                sGroupMethodHint = QMO_GROUP_METHOD_TYPE_HASH;
                break;
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

    // BUG-48128
    // outer query에 aggregation + group by 이고,
    // target or having 절에 사용한 subquery ( scalar subquery)에서 outer column을 사용한 경우 
    // sort 메소드를 사용하지 않도록 합니다.
    if ( ( sGroupMethodHint != QMO_GROUP_METHOD_TYPE_HASH ) &&
         ( aGroupGraph->aggregation != NULL ) &&
         ( aGroupGraph->groupBy != NULL ) &&
         ( (aGroupGraph->graph.myQuerySet->lflag & QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_MASK)
           == QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_TRUE ) )
    {
        sGroupMethodHint = QMO_GROUP_METHOD_TYPE_HASH;
    }
    else
    {
        /* Nothing to do */
    }

    switch( sGroupMethodHint )
    {
        case QMO_GROUP_METHOD_TYPE_NOT_DEFINED :
            // Group Method Hint가 없는 경우

            // To Fix PR-12394
            // GROUPING 관련 힌트가 없는 경우에만
            // 최적화 Tip을 적용해야 함.

            //------------------------------------------
            // To Fix PR-12396
            // 비용 계산을 통해 수행 방법을 결정한다.
            //------------------------------------------

            //------------------------------------------
            // Sorting 을 이용한 방식의 비용 계산
            //------------------------------------------

            // 모든 Grouping은 Sorting방식을 사용할 수 있다.
            if( sIsDisk == ID_FALSE )
            {
                sSelTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                             &(sGraph->left->costInfo),
                                                             QMO_COST_DEFAULT_NODE_CNT );
                sSelAccessCost = sSelTotalCost;
                sSelDiskCost   = 0;
            }
            else
            {
                sSelTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                              &(sGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT,
                                                              aRecordSize );
                sSelAccessCost = 0;
                sSelDiskCost   = sSelTotalCost;
            }

            /* BUG-39284 The sys_connect_by_path function with Aggregate
             * function is not correct. 
             */
            if ( ( aGroupGraph->graph.myQuerySet->SFWGH->lflag & QMV_SFWGH_CONNECT_BY_FUNC_MASK )
                 == QMV_SFWGH_CONNECT_BY_FUNC_TRUE ) 
            {
                IDE_TEST_RAISE( aGroupGraph->distAggArg != NULL, ERR_NOT_DIST_AGGR_WITH_CONNECT_BY_FUNC );

                sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
                sGraph->flag |= QMG_SORT_HASH_METHOD_HASH;
            }
            else
            {
                sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
                sGraph->flag |= QMG_SORT_HASH_METHOD_SORT;
            }

            //------------------------------------------
            // Hashing 을 이용한 방식의 비용계산
            //------------------------------------------
            if ( aGroupGraph->distAggArg == NULL )
            {
                if( sIsDisk == ID_FALSE )
                {
                    sTotalCost = qmoCost::getMemHashTempCost( aStatement->mSysStat,
                                                              &(sGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT );
                    sAccessCost = sTotalCost;
                    sDiskCost   = 0;
                }
                else
                {
                    // BUG-37752
                    // DiskHashGroup 일때 동작방식이 다르므로 별도의 cost 를 계산한다.
                    // hashBucketCnt 에 영향을 받는다.
                    sTotalCost = qmoCost::getDiskHashGroupCost( aStatement->mSysStat,
                                                                &(sGraph->left->costInfo),
                                                                aGroupGraph->hashBucketCnt,
                                                                QMO_COST_DEFAULT_NODE_CNT,
                                                                aRecordSize );

                    sAccessCost = 0;
                    sDiskCost   = sTotalCost;
                }
            }
            else
            {
                sTotalCost = QMO_COST_INVALID_COST;
            }

            if (QMO_COST_IS_EQUAL(sTotalCost, QMO_COST_INVALID_COST) == ID_TRUE)
            {
                // Hashing 방식을 적용할 수 없는 경우임
            }
            else
            {
                if (QMO_COST_IS_GREATER(sTotalCost, sSelTotalCost) == ID_TRUE)
                {
                    // Nothing to do
                }
                else
                {
                    // Hashing 방식이 보다 나음
                    sSelTotalCost  = sTotalCost;
                    sSelDiskCost   = sDiskCost;
                    sSelAccessCost = sAccessCost;

                    sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
                    sGraph->flag |= QMG_SORT_HASH_METHOD_HASH;

                    sGraph->preservedOrder = NULL;

                    sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sGraph->flag |= QMG_PRESERVED_ORDER_NEVER;
                }
            }

            //------------------------------------------
            // Preserved Order 를 이용한 방식의 비용계산
            //------------------------------------------

            IDE_TEST( getCostByPrevOrder( aStatement,
                                          aGroupGraph,
                                          & sAccessCost,
                                          & sDiskCost,
                                          & sTotalCost )
                      != IDE_SUCCESS );

            if (QMO_COST_IS_EQUAL(sTotalCost, QMO_COST_INVALID_COST) == ID_TRUE)
            {
                // Preserved Order 방식을 적용할 수 없는 경우임
            }
            else
            {
                if (QMO_COST_IS_GREATER(sTotalCost, sSelTotalCost) == ID_TRUE)
                {
                    // Nothing to do
                }
                else
                {
                    // Preserved Order 방식이 보다 나음
                    sSelTotalCost  = sTotalCost;
                    sSelDiskCost   = sDiskCost;
                    sSelAccessCost = sAccessCost;

                    // To Fix PR-12394
                    // GROUPING 관련 힌트가 없는 경우에만
                    // 최적화 Tip을 적용해야 함.
                    IDE_TEST( indexableGroupBy( aStatement,
                                                aGroupGraph,
                                                & sIndexable )
                              != IDE_SUCCESS );

                    IDE_DASSERT( sIndexable == ID_TRUE );

                    // Indexable Group By 가 가능한 경우
                    sGraph->flag &= ~QMG_GROP_OPT_TIP_MASK;
                    sGraph->flag
                        |= QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY;

                    sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
                    sGraph->flag |= QMG_SORT_HASH_METHOD_SORT;

                    // 이미 Preserved Order가 설정됨

                    sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
                }
            }

            //------------------------------------------
            // 마무리
            //------------------------------------------

            // Sorting 방식이 선택된 경우 Preserved Order 생성
            if ( ( ( sGraph->flag & QMG_SORT_HASH_METHOD_MASK )
                   == QMG_SORT_HASH_METHOD_SORT )
                 &&
                 ( ( sGraph->flag & QMG_GROP_OPT_TIP_MASK )
                   != QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY ) )
            {
                sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
                sGraph->flag |= QMG_SORT_HASH_METHOD_SORT;

                // Group By 컬럼을 이용하여 Preserved Order를 생성함.
                IDE_TEST(
                    makeGroupByOrder( aStatement,
                                      aGroupGraph->groupBy,
                                      & sGraph->preservedOrder )
                    != IDE_SUCCESS );

                sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
                sGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
            }
            else
            {
                // 다른 Method가 선택됨
            }

            break;

        case QMO_GROUP_METHOD_TYPE_HASH :
            // Group Method Hint가 Hash based 인 경우

            if ( aGroupGraph->distAggArg == NULL )
            {
                if( sIsDisk == ID_FALSE )
                {
                    sSelTotalCost = qmoCost::getMemHashTempCost( aStatement->mSysStat,
                                                                 &(sGraph->left->costInfo),
                                                                 QMO_COST_DEFAULT_NODE_CNT );
                    sSelAccessCost = sSelTotalCost;
                    sSelDiskCost   = 0;
                }
                else
                {
                    // BUG-37752
                    // DiskHashGroup 일때 동작방식이 다르므로 별도의 cost 를 계산한다.
                    // hashBucketCnt 에 영향을 받는다.
                    sSelTotalCost = qmoCost::getDiskHashGroupCost( aStatement->mSysStat,
                                                                   &(sGraph->left->costInfo),
                                                                   aGroupGraph->hashBucketCnt,
                                                                   QMO_COST_DEFAULT_NODE_CNT,
                                                                   aRecordSize );

                    sSelAccessCost = 0;
                    sSelDiskCost   = sSelTotalCost;
                }

                sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
                sGraph->flag |= QMG_SORT_HASH_METHOD_HASH;

                sGraph->preservedOrder = NULL;

                sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
                sGraph->flag |= QMG_PRESERVED_ORDER_NEVER;

                break;
            }
            else
            {
                // Distinct Aggregation의 경우
                // Hash-based Grouping을 사용할 수 없다.
                // 아래의 Sort-based Grouping을 사용한다.
            }
            /* fall through  */
        case QMO_GROUP_METHOD_TYPE_SORT :

            // Group Method Hint가 Sort based 인 경우
            if( sIsDisk == ID_FALSE )
            {
                sSelTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                             &(sGraph->left->costInfo),
                                                             QMO_COST_DEFAULT_NODE_CNT );
                sSelAccessCost = sSelTotalCost;
                sSelDiskCost   = 0;
            }
            else
            {
                sSelTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                              &(sGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT,
                                                              aRecordSize );
                sSelAccessCost = 0;
                sSelDiskCost   = sSelTotalCost;
            }

            sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
            sGraph->flag |= QMG_SORT_HASH_METHOD_SORT;

            // Group By 컬럼을 이용하여 Preserved Order를 생성함.
            IDE_TEST( makeGroupByOrder( aStatement,
                                        aGroupGraph->groupBy,
                                        & sGraph->preservedOrder )
                      != IDE_SUCCESS );

            sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
            sGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
            break;
        default :
            IDE_DASSERT( 0 );
            break;
    }

    /* BUG-39284 The sys_connect_by_path function with Aggregate
     * function is not correct. 
     */
    IDE_TEST_RAISE( ( ( aGroupGraph->graph.myQuerySet->SFWGH->lflag & QMV_SFWGH_CONNECT_BY_FUNC_MASK )
                      == QMV_SFWGH_CONNECT_BY_FUNC_TRUE ) &&
                    ( ( sGraph->flag & QMG_SORT_HASH_METHOD_MASK )
                      == QMG_SORT_HASH_METHOD_SORT ), 
                    ERR_NOT_DIST_AGGR_WITH_CONNECT_BY_FUNC );

    // 비용 정보 설정
    sGraph->costInfo.myAccessCost = sSelAccessCost + aAggrCost;
    sGraph->costInfo.myDiskCost   = sSelDiskCost;
    sGraph->costInfo.myAllCost    = sSelTotalCost  + aAggrCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_DIST_AGGR_WITH_CONNECT_BY_FUNC )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgGrouping::makeGroupByOrder( qcStatement        * aStatement,
                               qmsConcatElement   * aGroupBy,
                               qmgPreservedOrder ** sNewGroupByOrder )
{
/***********************************************************************
 *
 * Description : Group By 컬럼을 이용하여
 *               Preserved Order 자료 구조를 구축함.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsConcatElement  * sGroupBy;
    qmgPreservedOrder * sWantOrder;
    qmgPreservedOrder * sNewOrder;
    qmgPreservedOrder * sCurOrder;
    qtcNode           * sNode;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::makeGroupByOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sWantOrder = NULL;
    sCurOrder  = NULL;

    //------------------------------------------
    // Group by 칼럼에 대한 want order를 생성
    //------------------------------------------

    for ( sGroupBy = aGroupBy; sGroupBy != NULL; sGroupBy = sGroupBy->next )
    {
        sNode = sGroupBy->arithmeticOrList;

        //------------------------------------------
        // Group by 칼럼에 대한 want order를 생성
        //------------------------------------------
        IDU_FIT_POINT( "qmgGrouping::makeGroupByOrder::alloc::NewOrder" );
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                 (void**)&sNewOrder )
                  != IDE_SUCCESS );

        sNewOrder->table = sNode->node.table;
        sNewOrder->column = sNode->node.column;
        sNewOrder->direction = QMG_DIRECTION_NOT_DEFINED;
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

    *sNewGroupByOrder = sWantOrder;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgGrouping::indexableGroupBy( qcStatement        * aStatement,
                               qmgGROP            * aGroupGraph,
                               idBool             * aIndexableGroupBy )
{
/***********************************************************************
 *
 * Description : Indexable Group By 최적화 가능한 경우, 적용
 *
 * Implementation :
 *
 *    Preserved Order 사용 가능한 경우, 적용
 *
 ***********************************************************************/

    qmgPreservedOrder * sWantOrder;

    idBool              sSuccess;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::indexableGroupBy::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGroupGraph != NULL );
    IDE_DASSERT( aGroupGraph->groupBy != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sSuccess   = ID_FALSE;
    sWantOrder = NULL;

    //------------------------------------------
    // Group by 칼럼에 대한 want order를 생성
    //------------------------------------------

    IDE_TEST( makeGroupByOrder( aStatement,
                                aGroupGraph->groupBy,
                                & sWantOrder )
              != IDE_SUCCESS );

    //------------------------------------------
    // Preserved Order의 사용 가능 여부를 검사
    //------------------------------------------

    IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                      & aGroupGraph->graph,
                                      sWantOrder,
                                      & sSuccess )
              != IDE_SUCCESS );

    if ( sSuccess == ID_TRUE )
    {
        aGroupGraph->graph.preservedOrder = sWantOrder;
        *aIndexableGroupBy = ID_TRUE;
    }
    else
    {
        aGroupGraph->graph.preservedOrder = NULL;
        *aIndexableGroupBy = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
qmgGrouping::countStar( qcStatement      * aStatement,
                        qmgGROP          * aGroupGraph,
                        idBool           * aCountStar )
{
/***********************************************************************
 *
 * Description : Count(*) 최적화 가능한 경우, 적용
 *
 * Implementation :
 *    (1) CNF인 경우
 *    (2) 하나의 base table에 대한 질의
 *    (3) limit이 없는 경우
 *    (4) hierarchy query가 없는 경우
 *    (5) distinct가 없는 경우
 *    (6) order by가 없는 경우
 *    (7) subquery filter가 없는 경우
 *    (8) rownum이 사용되지 않은 경우
 *
 ***********************************************************************/

    idBool sTemp;
    idBool sIsCountStar;

    qmsSFWGH    * sMySFWGH;
    qmgChildren * sChild;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::countStar::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGroupGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sTemp = ID_TRUE;
    sIsCountStar = ID_TRUE;
    sMySFWGH = aGroupGraph->graph.myQuerySet->SFWGH;

    while( sTemp == ID_TRUE )
    {
        sTemp = ID_FALSE;

        // CNF인 경우
        if ( sMySFWGH->crtPath->normalType != QMO_NORMAL_TYPE_CNF )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // BUG-35155 Partial CNF
        // NNF 필터가 존재하면 count star 최적화를 수행하면 안된다.
        // (여기 올 경우 항상 CNF 이다)
        if ( sMySFWGH->crtPath->crtCNF->nnfFilter != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // 하나의 base table에 대한 질의
        if ( sMySFWGH->from->next != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // To Fix BUG-9072
        // base graph의 type이 join이 아닌 경우
        if ( sMySFWGH->from->joinType != QMS_NO_JOIN )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // To Fix BUG-9148, 8753, 8745, 8786
        // view가 아닌 경우
        if ( sMySFWGH->from->tableRef->view != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // PROJ-1502 PARTITIONED DISK TABLE
            if( sMySFWGH->from->tableRef->tableInfo->tablePartitionType
                == QCM_PARTITIONED_TABLE )
            {
                sChild = aGroupGraph->graph.left->children;

                // BUG-17483 파티션 테이블 count(*) 지원
                // 파티션 테이블의 경우 필터가 달려있는 경우에는 count(*) 최적화를 안함
                // 파티션 테이블은 QMND_CUNT_METHOD_HANDLE 방식만 지원한다.
                if( ((qmsParseTree*)(aStatement->myPlan->parseTree))->querySet->setOp != QMS_NONE )
                {
                    sIsCountStar = ID_FALSE;
                    break;
                }
                else
                {
                    // BUG-41700
                    if( ( aGroupGraph->graph.left->constantPredicate != NULL ) ||
                        ( aGroupGraph->graph.left->myPredicate       != NULL ) ||
                        ( aGroupGraph->graph.left->ridPredicate      != NULL ) )
                    {
                        sIsCountStar = ID_FALSE;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    while( sChild != NULL )
                    {
                        if( ( sChild->childGraph->constantPredicate != NULL ) ||
                            ( sChild->childGraph->myPredicate       != NULL ) ||
                            ( sChild->childGraph->ridPredicate      != NULL ) )
                        {
                            sIsCountStar = ID_FALSE;
                            break;
                        }
                        else
                        {
                            sChild = sChild->next;
                        }
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }

        // To Fix BUG-8711
        // select type이 distinct가 아닌 경우
        if ( sMySFWGH->selectType == QMS_DISTINCT )
        {
            sIsCountStar = ID_FALSE;
        }
        else
        {
            // nothing to do
        }

        // limit이 없는 경우
        if ( ((qmsParseTree*)(aStatement->myPlan->parseTree))->limit != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // hierarchy가 없는 경우
        if ( sMySFWGH->hierarchy != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // distinct aggregation이 없는 경우
        if ( aGroupGraph->distAggArg != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // order by가 없는 경우
        if ( ((qmsParseTree*)(aStatement->myPlan->parseTree))->orderBy != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        // subquery filter가 없는 경우
        if ( sMySFWGH->where != NULL )
        {
            if ( ( sMySFWGH->where->lflag & QTC_NODE_SUBQUERY_MASK )
                 == QTC_NODE_SUBQUERY_EXIST )
            {
                sIsCountStar = ID_FALSE;
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

        // rownum이 사용되지 않은 경우
        if ( sMySFWGH->rownum != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            // nothing to do
        }

        /* PROJ-1832 New database link */
        if ( sMySFWGH->from->tableRef->remoteTable != NULL )
        {
            sIsCountStar = ID_FALSE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aCountStar = sIsCountStar;

    return IDE_SUCCESS;

}

IDE_RC
qmgGrouping::nonGroupOptTip( qcStatement * aStatement,
                             qmgGROP     * aGroupGraph,
                             SDouble       aRecordSize,
                             SDouble       aAggrCost )
{
/***********************************************************************
 *
 * Description : GROUP BY가 없는 경우의 tip 적용
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode            * sAggNode;
    qtcNode            * sColNode;
    qmgPreservedOrder  * sNewOrder;
    qmgGraph           * sLeftGraph;
    qmgGraph           * sGraph;

    SDouble              sTotalCost     = 0;
    SDouble              sDiskCost      = 0;
    SDouble              sAccessCost    = 0;
    idBool               sIsDisk;
    idBool               sSuccess;

    extern mtfModule     mtfMin;
    extern mtfModule     mtfMax;
    extern mtfModule     mtfCount;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::nonGroupOptTip::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aGroupGraph != NULL );
    IDE_FT_ASSERT( aGroupGraph->aggregation != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sSuccess = ID_FALSE;
    sGraph   = &(aGroupGraph->graph);

    //------------------------------------------
    //  aggregation이 하나만 존재하면서
    //  그 aggregation 대상 칼럼이 순수한 칼럼인 경우
    //------------------------------------------

    if ( aGroupGraph->aggregation->next == NULL )
    {
        sAggNode = (qtcNode*) aGroupGraph->aggregation->aggr;
        sColNode = (qtcNode*)sAggNode->node.arguments;

        if ( sColNode == NULL )
        {
            if ( sAggNode->node.module == & mtfCount )
            {

                //------------------------------------------
                // count(*) 최적화
                //------------------------------------------

                IDE_TEST( countStar( aStatement,
                                     aGroupGraph,
                                     & sSuccess )
                          != IDE_SUCCESS );

                if ( sSuccess == ID_TRUE )
                {
                    sGraph->flag &= ~QMG_GROP_OPT_TIP_MASK;
                    sGraph->flag |= QMG_GROP_OPT_TIP_COUNT_STAR;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // BUG-12542
                // COUNT(*) 임에도 불구하고 이곳으로 오는 경우가 있음.
                // Nothing To Do
            }
        }
        else
        {
            // want order 생성
            IDU_FIT_POINT( "qmgGrouping::nonGroupOptTip::alloc::NewOrder" );
            IDE_TEST(
                QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                               (void**)&sNewOrder )
                != IDE_SUCCESS );

            sNewOrder->table = sColNode->node.table;
            sNewOrder->column = sColNode->node.column;
            sNewOrder->direction = QMG_DIRECTION_NOT_DEFINED;
            sNewOrder->next = NULL;

            if ( ( sAggNode->node.lflag & MTC_NODE_DISTINCT_MASK )
                 == MTC_NODE_DISTINCT_TRUE )
            {
                //------------------------------------------
                //  indexable Distinct Aggregation 최적화
                //------------------------------------------

                IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                                  sGraph,
                                                  sNewOrder,
                                                  & sSuccess )
                          != IDE_SUCCESS );

                if ( sSuccess == ID_TRUE )
                {
                    sGraph->flag &= ~QMG_GROP_OPT_TIP_MASK;
                    sGraph->flag |=
                        QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG;
                }
                else
                {
                    // nothing to
                }
            }
            else
            {
                if ( ( sAggNode->node.module == & mtfMin ) ||
                     ( sAggNode->node.module == & mtfMax ) )
                {
                    // To Fix PR-11562
                    // Indexable MIN-MAX 최적화의 경우,
                    // Direction이 정의되어야 함.
                    if ( sAggNode->node.module == & mtfMin )
                    {
                        // MIN() 인 경우 ASC Order가 사용가능한지 검사
                        sNewOrder->direction = QMG_DIRECTION_ASC;
                    }
                    else
                    {
                        // MAX() 인 경우 DESC Order가 사용가능한지 검사
                        sNewOrder->direction = QMG_DIRECTION_DESC;
                    }

                    //------------------------------------------
                    //  indexable Min Max 최적화
                    //------------------------------------------

                    IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                                      sGraph,
                                                      sNewOrder,
                                                      & sSuccess )
                              != IDE_SUCCESS );

                    if ( sSuccess == ID_TRUE )
                    {
                        sGraph->flag &= ~QMG_GROP_OPT_TIP_MASK;
                        sGraph->flag |=
                            QMG_GROP_OPT_TIP_INDEXABLE_MINMAX;
                        sGraph->flag &= ~QMG_INDEXABLE_MIN_MAX_MASK;
                        sGraph->flag |= QMG_INDEXABLE_MIN_MAX_TRUE;

                        // To Fix BUG-9919
                        for ( sLeftGraph = sGraph->left;
                              sLeftGraph != NULL;
                              sLeftGraph = sLeftGraph->left )
                        {
                            if ( (sLeftGraph->type == QMG_SELECTION) ||
                                 (sLeftGraph->type == QMG_PARTITION) )
                            {
                                if ( sLeftGraph->myFrom->tableRef->view
                                     != NULL )
                                {
                                    continue;
                                }
                                else
                                {
                                    sLeftGraph->flag &= ~QMG_SELT_NOTNULL_KEYRANGE_MASK;
                                    sLeftGraph->flag |=  QMG_SELT_NOTNULL_KEYRANGE_TRUE;
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
                        // preserved order 실패한 경우
                        // nothing to
                    }
                }
                else
                {
                    // min 또는 max가 아닌 경우
                    // nothing to do
                }
            }
        }
    }
    else
    {
        // aggregation이 두개 이상인 경우,
        // nothing to do
    }

    IDE_TEST( qmg::isDiskTempTable( sGraph, & sIsDisk ) != IDE_SUCCESS );

    // TASK-6699 TPC-H 성능 개선
    // group by 가 없을때에도 cost 를 계산함
    if ( aGroupGraph->distAggArg != NULL )
    {
        // Distinct Aggregation의 경우 sort-based로 수행해야 함.
        sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
        sGraph->flag |= QMG_SORT_HASH_METHOD_SORT;

        if ( sGraph->preservedOrder != NULL )
        {
            sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
            sGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
        }
        else
        {
            sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
            sGraph->flag |= QMG_PRESERVED_ORDER_NEVER;
        }

        if( sIsDisk == ID_FALSE )
        {
            sTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                      &(sGraph->left->costInfo),
                                                      QMO_COST_DEFAULT_NODE_CNT );
            sAccessCost = sTotalCost;
            sDiskCost   = 0;
        }
        else
        {
            sTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                       &(sGraph->left->costInfo),
                                                       QMO_COST_DEFAULT_NODE_CNT,
                                                       aRecordSize );
            sAccessCost = 0;
            sDiskCost   = sTotalCost;
        }
    }
    else
    {
        // Grouping Method를 hash-based로 선택
        sGraph->flag &= ~QMG_SORT_HASH_METHOD_MASK;
        sGraph->flag |= QMG_SORT_HASH_METHOD_HASH;

        sGraph->preservedOrder = NULL;

        sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
        sGraph->flag |= QMG_PRESERVED_ORDER_NEVER;

        if( sIsDisk == ID_FALSE )
        {
            sTotalCost = qmoCost::getMemHashTempCost( aStatement->mSysStat,
                                                      &(sGraph->left->costInfo),
                                                      QMO_COST_DEFAULT_NODE_CNT );
            sAccessCost = sTotalCost;
            sDiskCost   = 0;
        }
        else
        {
            sTotalCost = qmoCost::getDiskHashGroupCost( aStatement->mSysStat,
                                                        &(sGraph->left->costInfo),
                                                        aGroupGraph->hashBucketCnt,
                                                        QMO_COST_DEFAULT_NODE_CNT,
                                                        aRecordSize );

            sAccessCost = 0;
            sDiskCost   = sTotalCost;
        }
    }

    switch ( sGraph->flag & QMG_GROP_OPT_TIP_MASK )
    {
        case QMG_GROP_OPT_TIP_NONE :

            sGraph->costInfo.myAccessCost = aAggrCost + sAccessCost;
            sGraph->costInfo.myDiskCost   = sDiskCost;
            sGraph->costInfo.myAllCost    = aAggrCost + sTotalCost;
            break;

        case QMG_GROP_OPT_TIP_COUNT_STAR :
        case QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG :
        case QMG_GROP_OPT_TIP_INDEXABLE_MINMAX :

            sGraph->costInfo.myAccessCost =
                sGraph->left->costInfo.outputRecordCnt *
                aStatement->mSysStat->mMTCallTime;
            sGraph->costInfo.myDiskCost   = 0;
            sGraph->costInfo.myAllCost    = sGraph->costInfo.myAccessCost;
            break;
        default :
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgGrouping::getBucketCnt4Group( qcStatement  * aStatement,
                                 qmgGROP      * aGroupGraph,
                                 UInt           aHintBucketCnt,
                                 UInt         * aBucketCnt )
{
/***********************************************************************
 *
 * Description : hash bucket count의 설정
 *
 * Implementation :
 *    - hash bucket count hint가 존재하지 않을 경우
 *      hash bucket count = MIN( 하위 graph의 outputRecordCnt / 2,
 *                               Group Column들의 cardinality 곱 )
 *    - hash bucket count hint가 존재할 경우
 *      hash bucket count = hash bucket count hint 값
 *
 ***********************************************************************/

    qmoColCardInfo   * sColCardInfo;
    qmsConcatElement * sGroup;
    qtcNode          * sNode;

    SDouble            sCardinality;
    SDouble            sBucketCnt;
    ULong              sBucketCntOutput;

    idBool             sAllColumn;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::getBucketCnt4Group::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGroupGraph != NULL );

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

        sBucketCnt = aGroupGraph->graph.left->costInfo.outputRecordCnt / 2.0;
        sBucketCnt = ( sBucketCnt < 1 ) ? 1 : sBucketCnt;

        //------------------------------------------
        // group column들의 cardinality 값을 구한다.
        //------------------------------------------

        for ( sGroup = aGroupGraph->groupBy;
              sGroup != NULL;
              sGroup = sGroup->next )
        {
            sNode = sGroup->arithmeticOrList;

            if ( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
            {
                // group 대상이 순수한 칼럼인 경우
                sColCardInfo = QC_SHARED_TMPLATE(aStatement)->
                    tableMap[sNode->node.table].
                    from->tableRef->statInfo->colCardInfo;

                sCardinality *= sColCardInfo[sNode->node.column].columnNDV;
            }
            else
            {
                // BUG-37778 disk hash temp table size estimate
                // tpc-H Q9 에서 group by 예측을 제대로 하지 못함
                // EXTRACT(O_ORDERDATE,'year') AS O_YEAR
                // group by O_YEAR
                if( (sNode->node.arguments != NULL) &&
                    QTC_IS_COLUMN( aStatement, (qtcNode*)(sNode->node.arguments) ) == ID_TRUE )
                {
                    sColCardInfo = QC_SHARED_TMPLATE(aStatement)->
                        tableMap[sNode->node.arguments->table].
                        from->tableRef->statInfo->colCardInfo;

                    sCardinality *= sColCardInfo[sNode->node.arguments->column].columnNDV;
                }
                else
                {
                    sAllColumn = ID_FALSE;
                    break;
                }
            }
        }

        if ( sAllColumn == ID_TRUE )
        {
            //------------------------------------------
            // MIN( 하위 graph의 outputRecordCnt / 2,
            //      Group Column들의 cardinality 곱 )
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

        //  hash bucket count의 보정
        if ( sBucketCnt < QCU_OPTIMIZER_BUCKET_COUNT_MIN )
        {
            sBucketCnt = QCU_OPTIMIZER_BUCKET_COUNT_MIN;
        }
        else
        {
            if ( (aGroupGraph->graph.flag & QMG_GRAPH_TYPE_MASK) == QMG_GRAPH_TYPE_MEMORY )
            {
                // QMC_MEM_HASH_MAX_BUCKET_CNT 값으로 보정해준다.
                /* BUG-48161 */
                if ( sBucketCnt > QCG_GET_BUCKET_COUNT_MAX( aStatement )  )
                {
                    sBucketCnt = QCG_GET_BUCKET_COUNT_MAX( aStatement );
                }
                else
                {
                    // nothing todo.
                }
            }
            else
            {
                // nothing todo.
            }
        }
    }
    else
    {
        // bucket count hint가 존재하는 경우
        sBucketCnt = aHintBucketCnt;
    }

    // BUG-36403 플랫폼마다 BucketCnt 가 달라지는 경우가 있습니다.
    sBucketCntOutput = DOUBLE_TO_UINT64( sBucketCnt );
    *aBucketCnt      = (UInt)sBucketCntOutput;

    return IDE_SUCCESS;

}

IDE_RC
qmgGrouping::getCostByPrevOrder( qcStatement      * aStatement,
                                 qmgGROP          * aGroupGraph,
                                 SDouble          * aAccessCost,
                                 SDouble          * aDiskCost,
                                 SDouble          * aTotalCost )
{
/***********************************************************************
 *
 * Description :
 *
 *    Preserved Order 방식을 사용한 Grouping 비용을 계산한다.
 *
 * Implementation :
 *
 *    이미 Child가 원하는 Preserved Order를 가지고 있다면
 *    별도의 비용 없이 Grouping이 가능하다.
 *
 *    반면 Child에 특정 인덱스를 적용하는 경우라면,
 *    Child의 인덱스를 이용한 비용이 포함되게 된다.
 *
 ***********************************************************************/

    SDouble             sTotalCost;
    SDouble             sAccessCost;
    SDouble             sDiskCost;

    idBool              sUsable;

    qmgPreservedOrder * sWantOrder;

    qmoAccessMethod   * sOrgMethod;
    qmoAccessMethod   * sSelMethod;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::getCostByPrevOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGroupGraph != NULL );

    //------------------------------------------
    // Preserved Order를 사용할 수 있는 지를 검사
    //------------------------------------------

    // Group by 칼럼에 대한 want order를 생성
    sWantOrder = NULL;
    IDE_TEST( makeGroupByOrder( aStatement,
                                aGroupGraph->groupBy,
                                & sWantOrder )
              != IDE_SUCCESS );

    // preserved order 적용 가능 검사
    IDE_TEST( qmg::checkUsableOrder( aStatement,
                                     sWantOrder,
                                     aGroupGraph->graph.left,
                                     & sOrgMethod,
                                     & sSelMethod,
                                     & sUsable )
              != IDE_SUCCESS );

    //------------------------------------------
    // 비용 계산
    //------------------------------------------

    if ( sUsable == ID_TRUE )
    {
        if ( aGroupGraph->graph.left->preservedOrder == NULL )
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
            sAccessCost = aGroupGraph->graph.left->costInfo.outputRecordCnt *
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

IDE_RC
qmgGrouping::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Preserved Order의 direction을 결정한다.
 *                direction이 NOT_DEFINED 일 경우에만 호출하여야 한다.
 *
 *  Implementation :
 *    - 하위의 preserved order와 같은 경우,
 *      Child graph의 Preserved order direction을 복사한다.
 *    - 하위의 preserved order와 다른 경우,
 *      Ascending으로 direction을 설정한다.
 *
 ***********************************************************************/

    idBool              sIsSamePrevOrderWithChild;
    qmgPreservedOrder * sPreservedOrder;
    qmgDirectionType    sPrevDirection;

    
    IDU_FIT_POINT_FATAL( "qmgGrouping::finalizePreservedOrder::__FT__" );

    // BUG-36803 Grouping Graph must have a left child graph
    IDE_DASSERT( aGraph->left != NULL );

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

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * setGroupExtensionMethod
 *
 *   Rollup일 경우 Preserved Order를 수행해서 Sort Plan의 생성 여부를 결정하고
 *   Output order 를 설정과 Cost 계산을 수행한다.
 *   Cube일 경우는 Cust 계산만 수행한다.
 */
IDE_RC qmgGrouping::setPreOrderGroupExtension( qcStatement * aStatement,
                                               qmgGROP     * aGroupGraph,
                                               SDouble       aRecordSize )
{
    qmgGraph           * sGraph;
    qmsConcatElement  * sGroupBy;
    qmsConcatElement  * sSubGroup;
    qmgPreservedOrder * sWantOrder;
    qmgPreservedOrder * sNewOrder;
    qmgPreservedOrder * sCurOrder;
    idBool              sSuccess;
    qtcNode           * sNode;
    qtcNode           * sList;
    UInt                sCount;

    SDouble             sSelAccessCost = 0;
    SDouble             sSelDiskCost   = 0;
    SDouble             sSelTotalCost  = 0;
    idBool              sIsDisk;
    
    IDU_FIT_POINT_FATAL( "qmgGrouping::setPreOrderGroupExtension::__FT__" );

    sWantOrder = NULL;
    sCurOrder  = NULL;

    sGraph = &(aGroupGraph->graph);

    IDE_TEST( qmg::isDiskTempTable( sGraph, & sIsDisk ) != IDE_SUCCESS );

    if ( ( sGraph->flag & QMG_GROP_OPT_TIP_MASK ) ==
         QMG_GROP_OPT_TIP_ROLLUP )
    {
        for ( sGroupBy = aGroupGraph->groupBy;
              sGroupBy != NULL;
              sGroupBy = sGroupBy->next )
        {
            sNode = sGroupBy->arithmeticOrList;

            if ( sGroupBy->type != QMS_GROUPBY_NORMAL )
            {
                continue;
            }
            else
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                         (void**)&sNewOrder )
                          != IDE_SUCCESS );
                sNewOrder->table = sNode->node.table;
                sNewOrder->column = sNode->node.column;
                sNewOrder->direction = QMG_DIRECTION_ASC;
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

        for ( sGroupBy = aGroupGraph->groupBy;
              sGroupBy != NULL;
              sGroupBy = sGroupBy->next )
        {
            if ( sGroupBy->type != QMS_GROUPBY_NORMAL )
            {
                for ( sSubGroup = sGroupBy->arguments;
                      sSubGroup != NULL;
                      sSubGroup = sSubGroup->next )
                {
                    sNode = sSubGroup->arithmeticOrList;

                    if ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                         == MTC_NODE_OPERATOR_LIST )
                    {
                        for ( sList = ( qtcNode * )sNode->node.arguments;
                              sList != NULL;
                              sList = ( qtcNode * )sList->node.next )
                        {
                            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                                     (void**)&sNewOrder )
                                      != IDE_SUCCESS );
                            sNewOrder->table = sList->node.table;
                            sNewOrder->column = sList->node.column;
                            sNewOrder->direction = QMG_DIRECTION_ASC;
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
                    else
                    {
                        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                                 (void**)&sNewOrder )
                                  != IDE_SUCCESS );
                        sNewOrder->table = sNode->node.table;
                        sNewOrder->column = sNode->node.column;
                        sNewOrder->direction = QMG_DIRECTION_ASC;
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
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                          sGraph,
                                          sWantOrder,
                                          & sSuccess )
                  != IDE_SUCCESS );

        /* Child Plan에 Preserved Order가 사용될 수 있는지 설정한다. */
        if ( sSuccess == ID_TRUE )
        {
            sGraph->flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
            sGraph->flag |= QMG_CHILD_PRESERVED_ORDER_CAN_USE;

            sSelAccessCost = sGraph->left->costInfo.outputRecordCnt;
            sSelDiskCost   = 0;
            sSelTotalCost  = sSelAccessCost + sSelDiskCost;
        }
        else
        {
            sGraph->flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
            sGraph->flag |= QMG_CHILD_PRESERVED_ORDER_CANNOT_USE;

            if( sIsDisk == ID_FALSE )
            {
                sSelTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                             &(sGraph->left->costInfo),
                                                             QMO_COST_DEFAULT_NODE_CNT );
                sSelAccessCost = sSelTotalCost;
                sSelDiskCost   = 0;
            }
            else
            {
                sSelTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                              &(sGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT,
                                                              aRecordSize );
                sSelAccessCost = 0;
                sSelDiskCost   = sSelTotalCost;
            }
        }
        /* Rollup은 항상 preservedOrder가 생성된다 */
        sGraph->preservedOrder = sWantOrder;
        sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
        sGraph->flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
    }
    else
    {
        if( sIsDisk == ID_FALSE )
        {
            sSelTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                         &(sGraph->left->costInfo),
                                                         QMO_COST_DEFAULT_NODE_CNT );
            sSelAccessCost = sSelTotalCost;
            sSelDiskCost   = 0;
        }
        else
        {
            sSelTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                          &(sGraph->left->costInfo),
                                                          QMO_COST_DEFAULT_NODE_CNT,
                                                          aRecordSize );
            sSelAccessCost = 0;
            sSelDiskCost   = sSelTotalCost;
        }

        sGraph->preservedOrder = NULL;

        /* BUG-43835 cube perserved order */
        sGraph->flag &= ~QMG_PRESERVED_ORDER_MASK;
        sGraph->flag |= QMG_PRESERVED_ORDER_NEVER;

        sCount = 0;
        for ( sGroupBy = aGroupGraph->groupBy;
              sGroupBy != NULL;
              sGroupBy = sGroupBy->next )
        {
            if ( sGroupBy->type == QMS_GROUPBY_CUBE )
            {
                for ( sSubGroup = sGroupBy->arguments;
                      sSubGroup != NULL;
                      sSubGroup = sSubGroup->next )
                {
                    sCount++;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        if (sCount > 0)
        {
            /* Cube는 2^(n-1) 만큼 Sorting 을 수행한다. */
            sCount = 0x1 << ( sCount - 1 );
            sSelAccessCost = sSelAccessCost * sCount;
            sSelDiskCost   = sSelDiskCost   * sCount;
            sSelTotalCost  = sSelTotalCost  * sCount;
        }
        else
        {
            // Nothing to do.
        }
    }

    sGraph->costInfo.myAccessCost = sSelAccessCost;
    sGraph->costInfo.myDiskCost   = sSelDiskCost;
    sGraph->costInfo.myAllCost    = sSelTotalCost;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmgGrouping::checkParallelEnable( qmgGROP * aMyGraph )
{
/***********************************************************************
 *
 * Description : PROJ-2444 Parallel Aggreagtion
 *    Group 노드가 parallel 이 가능한지 여부를 판단한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool  sMakeParallel = ID_TRUE;
    SDouble sCost;
    SDouble sGroupCount;

    // Group 갯수를 구한다.
    // output 값은 Group 갯수 * selectivity 로 계산된므로 이를 역산한다.
    sGroupCount = aMyGraph->graph.costInfo.outputRecordCnt /
        aMyGraph->graph.costInfo.selectivity;

    if ( ((aMyGraph->graph.flag & QMG_PARALLEL_IMPOSSIBLE_MASK)
          == QMG_PARALLEL_IMPOSSIBLE_FALSE) &&
         ((aMyGraph->graph.flag & QMG_PARALLEL_EXEC_MASK)
          == QMG_PARALLEL_EXEC_TRUE) )
    {
        if ( (aMyGraph->graph.left->myPlan->type == QMN_PSCRD) ||
             (aMyGraph->graph.left->myPlan->type == QMN_PPCRD) )
        {
            // MERGE 단계의 작업량이 많으면 parallel 해서는 안된다.
            sCost = ( aMyGraph->graph.costInfo.inputRecordCnt /
                      aMyGraph->graph.myPlan->mParallelDegree ) +
                ( aMyGraph->graph.myPlan->mParallelDegree *
                  sGroupCount );

            if ( QMO_COST_IS_GREATER( sCost, aMyGraph->graph.costInfo.inputRecordCnt ) == ID_TRUE )
            {
                sMakeParallel = ID_FALSE;
            }
            else
            {
                //nothing to do
            }

            // PPCRD 에 필터가 있으면 parallel 해서는 안된다.
            if (aMyGraph->graph.left->myPlan->type == QMN_PPCRD)
            {
                if ( (((qmncPPCRD*)aMyGraph->graph.left->myPlan)->subqueryFilter != NULL) ||
                     (((qmncPPCRD*)aMyGraph->graph.left->myPlan)->nnfFilter != NULL) )
                {
                    sMakeParallel = ID_FALSE;
                }
                else
                {
                    //nothing to do
                }
            }
            else
            {
                //nothing to do
            }
        }
        else
        {
            sMakeParallel = ID_FALSE;
        }
    }
    else
    {
        sMakeParallel = ID_FALSE;
    }

    return sMakeParallel;
}

// BUG-48132 grouping method property
void qmgGrouping::getPropertyGroupingMethod( qcStatement        * aStatement,
                                             qmoGroupMethodType * aGroupingMethod )
{
    qmoGroupMethodType sGroupingMethod = QMO_GROUP_METHOD_TYPE_NOT_DEFINED;

    switch ( QCG_GET_PLAN_HASH_OR_SORT_METHOD( aStatement ) & QMG_HASH_OR_SORT_METHOD_GROUPING_MASK )
    {
        case QMG_HASH_OR_SORT_METHOD_GROUPING_HASH:
            sGroupingMethod = QMO_GROUP_METHOD_TYPE_HASH;
            break;
        case QMG_HASH_OR_SORT_METHOD_GROUPING_SORT:
            sGroupingMethod =QMO_GROUP_METHOD_TYPE_SORT;
            break;
        default:
            sGroupingMethod = QMO_GROUP_METHOD_TYPE_NOT_DEFINED;
            break;
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_PLAN_HASH_OR_SORT_METHOD );

    *aGroupingMethod = sGroupingMethod;
}
