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
 * $Id: qmgWindowing.cpp 29304 2008-11-14 08:17:42Z jakim $
 *
 * Description :
 *     Windowing Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgWindowing.h>
#include <qmoCost.h>
#include <qmoOneMtrPlan.h>
#include <qmgSet.h>
#include <qmgGrouping.h>
#include <qcg.h>

extern mtfModule mtfRowNumberLimit;

IDE_RC
qmgWindowing::init( qcStatement * aStatement,
                    qmsQuerySet * aQuerySet,
                    qmgGraph    * aChildGraph,
                    qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgWindowing의 초기화
 *
 * Implementation :
 *    (1) qmgWindowing을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgWINDOW   * sMyGraph;
    qmsQuerySet * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgWindowing::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Windowing Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgWindowing을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgWINDOW ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_WINDOWING;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myFrom = NULL;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgWindowing::optimize;
    sMyGraph->graph.makePlan = qmgWindowing::makePlan;
    sMyGraph->graph.printGraph = qmgWindowing::printGraph;

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
    // Windowing Graph 고유 정보를 위한 기본 초기화
    //---------------------------------------------------

    sMyGraph->analyticFuncList = aQuerySet->analyticFuncList;
    sMyGraph->analyticFuncListPtrArr = NULL;
    sMyGraph->sortingKeyCount = 0;
    sMyGraph->sortingKeyPtrArr = NULL;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgWindowing::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgWindowing의 최적화
 *
 * Implementation :
 *    (1) Analytic Function 개수 구하고, sorting key 분류를 위한
 *        자료 구조 생성
 *    (2) Sorting Key 분류
 *    (3) Analytic Function 분류
 *    (4) Sorting 수행 순서 결정
 *    (5) Preserved Order 설정
 *    (6) 공통 비용 정보의 설정
 *
 ***********************************************************************/

    qmgWINDOW           * sMyGraph;
    qmsAnalyticFunc     * sCurAnalFunc;
    qmoDistAggArg       * sNewDistAggr;
    qmoDistAggArg       * sFirstDistAggr;
    qmoDistAggArg       * sCurDistAggr          = NULL;
    qtcNode             * sNode;
    mtcColumn           * sMtcColumn;
    UInt                  sAnalFuncCount;
    UInt                  sRowNumberFuncCount;
    qmgPreservedOrder  ** sSortingKeyPtrArr;
    qmsAnalyticFunc    ** sAnalyticFuncListPtrArr;
    qmsTarget           * sTarget;
    SDouble               sRecordSize;
    SDouble               sTempNodeCount;
    SDouble               sCost;
    qtcOverColumn       * sOverColumn;
    UInt                  sHashBucketCnt;
    UInt                  i;
    qmgPreservedOrder   * sWantOrder = NULL;
    idBool                sSuccess = ID_FALSE;
    idBool                sIsDisk;

    IDU_FIT_POINT_FATAL( "qmgWindowing::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph            = (qmgWINDOW*) aGraph;
    sAnalFuncCount      = 0;
    sRowNumberFuncCount = 0;
    sFirstDistAggr      = NULL;

    // analytic function 개수로 sorting key 개수를 초기화한다.
    // sorting key 분류 및 analytic function 분류 시에 정확한 개수가
    // 설정된다.
    // distinct aggregation argument 설정
    for ( sCurAnalFunc = sMyGraph->analyticFuncList;
          sCurAnalFunc != NULL;
          sCurAnalFunc = sCurAnalFunc->next )
    {
        sAnalFuncCount++;

        // aggregation의 subquery 최적화
        IDE_TEST(
            qmoPred::optimizeSubqueryInNode(aStatement,
                                            sCurAnalFunc->analyticFuncNode,
                                            ID_FALSE,
                                            ID_FALSE )
            != IDE_SUCCESS );

        for ( sOverColumn =
                  sCurAnalFunc->analyticFuncNode->overClause->overColumn;
              sOverColumn != NULL;
              sOverColumn = sOverColumn->next )
        {
            // partition by 의 subquery 최적화
            IDE_TEST(
                qmoPred::optimizeSubqueryInNode(aStatement,
                                                sOverColumn->node,
                                                ID_FALSE,
                                                ID_FALSE )
                != IDE_SUCCESS );
        }

        // distinct aggreagtion argument 설정
        if( ( sCurAnalFunc->analyticFuncNode->node.lflag &
              MTC_NODE_DISTINCT_MASK )
            == MTC_NODE_DISTINCT_TRUE )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoDistAggArg ),
                                                     (void**) & sNewDistAggr )
                      != IDE_SUCCESS );

            sNode = (qtcNode*)sCurAnalFunc->analyticFuncNode->node.arguments;

            sNewDistAggr->aggArg = sNode;
            sNewDistAggr->next   = NULL;

            if( sCurAnalFunc->analyticFuncNode->overClause->partitionByColumn != NULL )
            {
                // distinct 의 bucket 갯수를 구하기 위해서
                // partition by 의 bucket 갯수를 구한다.
                IDE_TEST(
                    getBucketCnt4Windowing(
                        aStatement,
                        sMyGraph,
                        sCurAnalFunc->analyticFuncNode->overClause->partitionByColumn,
                        sMyGraph->graph.myQuerySet->SFWGH->hints->groupBucketCnt,
                        &sHashBucketCnt )
                    != IDE_SUCCESS );
            }
            else
            {
                sHashBucketCnt = 1;
            }

            IDE_TEST(
                qmg::getBucketCnt4DistAggr(
                    aStatement,
                    sMyGraph->graph.left->costInfo.outputRecordCnt,
                    sHashBucketCnt,
                    sNode,
                    sMyGraph->graph.myQuerySet->SFWGH->hints->groupBucketCnt,
                    & sNewDistAggr->bucketCnt )
                != IDE_SUCCESS );

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
            /* BUG-40354 pushed rank */
            if ( ( sCurAnalFunc->analyticFuncNode->overClause->partitionByColumn == NULL ) &&
                 ( sCurAnalFunc->analyticFuncNode->node.module == &mtfRowNumberLimit ) )
            {
                sRowNumberFuncCount++;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    sMyGraph->distAggArg      = sFirstDistAggr;
    sMyGraph->sortingKeyCount = sAnalFuncCount;

    /* BUG-40354 pushed rank */
    if ( sAnalFuncCount == sRowNumberFuncCount )
    {
        sMyGraph->graph.flag &= ~QMG_WINDOWING_PUSHED_RANK_MASK;
        sMyGraph->graph.flag |= QMG_WINDOWING_PUSHED_RANK_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }
    
    //------------------------------------------
    // Sorting Key Pointer Array와
    // Analytic Function List를 가리키는 Pointer Array의 생성 및 초기화
    //-----------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc((ID_SIZEOF(qmgPreservedOrder*) *
                                             sAnalFuncCount),
                                            (void**) & sSortingKeyPtrArr )
              != IDE_SUCCESS );

    for ( i = 0; i < sAnalFuncCount; i++ )
    {
        sSortingKeyPtrArr[i] = NULL;
    }
    
    sMyGraph->sortingKeyPtrArr = sSortingKeyPtrArr;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc((ID_SIZEOF(qmsAnalyticFunc*) *
                                             sAnalFuncCount),
                                            (void**) & sAnalyticFuncListPtrArr )
              != IDE_SUCCESS );

    for ( i = 0; i < sAnalFuncCount; i++ )
    {
        sAnalyticFuncListPtrArr[i] = NULL;
    }

    sMyGraph->analyticFuncListPtrArr = sAnalyticFuncListPtrArr;

    //------------------------------------------
    // Sorting Key 및 Analytic Function  분류
    //-----------------------------------------

    IDE_TEST( classifySortingKeyNAnalFunc( aStatement, sMyGraph )
              != IDE_SUCCESS );
                                           

    // 실제 sorting key count 설정
    for ( i = 0 ; i < sMyGraph->sortingKeyCount; i ++ )
    {
        if ( sSortingKeyPtrArr[i] == NULL )
        {
            break;
        }
        else
        {
            // nothing to do
        }
    }
    sMyGraph->sortingKeyCount = i;
    
    //------------------------------------------
    // 첫번째 Sorting 수행 순서 결정 및
    // Preserved Order 설정
    //-----------------------------------------
    
    if ( sMyGraph->graph.left->preservedOrder != NULL )
    {
        // child의 preserved order와 동일한 sorting key가 있으면
        // 이를 첫번째로 옮겨 preserved order를 이용하도록 한다.
        IDE_TEST( alterSortingOrder( aStatement,
                                     aGraph,
                                     aGraph->left->preservedOrder,
                                     ID_TRUE )// 첫번째 sorting key 변경 여부
                  != IDE_SUCCESS );
    }
    else
    {
        sMyGraph->graph.flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
        sMyGraph->graph.flag |= QMG_CHILD_PRESERVED_ORDER_CANNOT_USE;

        if ( sMyGraph->sortingKeyCount > 0 )
        {
            /* BUG-40361 supporting to indexable analytic function */
            sWantOrder = sMyGraph->sortingKeyPtrArr[0];
            IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                              &sMyGraph->graph,
                                              sWantOrder,
                                              & sSuccess )
                      != IDE_SUCCESS );

            /* BUG-40361 supporting to indexable analytic function */
            if ( sSuccess == ID_TRUE )
            {
                sMyGraph->graph.flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
                sMyGraph->graph.flag |= QMG_CHILD_PRESERVED_ORDER_CAN_USE;
            }
            else
            {
                /* Nothing to do */
            }

            // BUG-21812
            // child plan에 preserved order가 없는 경우,
            // 현재 sorting order의 마지막 sorting key가
            // preserved order가 됨
            sMyGraph->graph.preservedOrder =
                sMyGraph->sortingKeyPtrArr[sMyGraph->sortingKeyCount - 1];
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_NOT_FIXED;
        }
        else
        {
            // child plan에 preserved order 없고,
            // window graph의 sorting key도 없을 경우에는
            // preserved order가 not defined 임
            sMyGraph->graph.preservedOrder = NULL;

            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NOT_DEFINED;
        }
    }

    /* PROJ-1805 */
    IDE_TEST_RAISE( ( sMyGraph->graph.left->flag & QMG_GROUPBY_NONE_MASK )
                    == QMG_GROUPBY_NONE_TRUE, ERR_NO_GROUP_EXPRESSION );
    /* PROJ-1353 */
    IDE_TEST_RAISE( ( sMyGraph->graph.left->flag & QMG_GROUPBY_EXTENSION_MASK )
                    == QMG_GROUPBY_EXTENSION_TRUE, ERR_NOT_WINDOWING );

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // recordSize 계산
    sRecordSize = 0;
    for ( sTarget = aGraph->myQuerySet->SFWGH->target;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        sNode = sTarget->targetColumn;

        sMtcColumn   = QTC_TMPL_COLUMN(QC_SHARED_TMPLATE(aStatement), sNode);
        sRecordSize += sMtcColumn->column.size;
    }
    // BUG-36463 sRecordSize 는 0이 되어서는 안된다.
    sRecordSize = IDL_MAX( sRecordSize, 1 );

    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = 1;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    // outputRecordCnt
    sMyGraph->graph.costInfo.outputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    // myCost
    IDE_TEST( qmg::isDiskTempTable( aGraph, & sIsDisk ) != IDE_SUCCESS );

    // BUG-36463 sortingKeyCount 가 0인 경우는 sort 를 안하고 저장만 함
    sTempNodeCount = IDL_MAX( sMyGraph->sortingKeyCount, 1 );

    if( sIsDisk == ID_FALSE )
    {
        sCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                             &(aGraph->left->costInfo),
                                             sTempNodeCount );

        // analytic function 구할때 두번 검색함?
        sMyGraph->graph.costInfo.myAccessCost = sCost;
        sMyGraph->graph.costInfo.myDiskCost   = 0;
    }
    else
    {
        sCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                              &(aGraph->left->costInfo),
                                              sTempNodeCount,
                                              sRecordSize );

        // analytic function 구할때 두번 검색함?
        sMyGraph->graph.costInfo.myAccessCost = 0;
        sMyGraph->graph.costInfo.myDiskCost   = sCost;
    }

    // My Access Cost와 My Disk Cost는 이미 계산됨.
    sMyGraph->graph.costInfo.myAllCost =
        sMyGraph->graph.costInfo.myAccessCost +
        sMyGraph->graph.costInfo.myDiskCost;

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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_WINDOWING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QTC_INVALID_WINDOW_FUNCTION,
                                  "ROLLUP, CUBE, GROUPING SETS"));
    }
    IDE_EXCEPTION( ERR_NO_GROUP_EXPRESSION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NO_GROUP_EXPRESSION ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgWindowing::classifySortingKeyNAnalFunc( qcStatement     * aStatement,
                                           qmgWINDOW       * aMyGraph )
{
    /***********************************************************************
     *
     * Description : sorting key와 analytic function 분류
     *
     * Implementation :
     *    (1) 이미 등록된 sorting key들을 방문하여
     *        현재 Partition By Column 이용 가능한지 검사
     *    (2) 사용 가능한 경우,
     *        (i)   sorting key 확장 여부 결정
     *        (ii) 확장해야 하는 경우, sorting key 확장
     *        사용 불가능한 경우, sorting key 생성
     *    (3) analytic function을 대응되는 sorting key 위치에 등록
     *
     ***********************************************************************/

    qmsAnalyticFunc     * sCurAnalFunc;
    qmsAnalyticFunc     * sNewAnalFunc;
    qtcOverColumn       * sOverColumn;
    qtcOverColumn       * sOverColumn2 = NULL;
    qtcOverColumn       * sOverColumn3 = NULL;
    qmsAnalyticFunc     * sInnerAnalyticFunc;
    idBool                sIsSame;
    idBool                sFoundSameSortingKey;
    idBool                sReplaceSortingKey;
    UInt                  sSortingKeyPosition;
    qmgPreservedOrder  ** sSortingKeyPtrArr;
    qmsAnalyticFunc    ** sAnalyticFuncPtrArr;
    qtcOverColumn       * sExpandOverColumn;
    qtcOverColumn       * sCurOverColumn;
    qmgPreservedOrder   * sNewSortingKeyCol;
    qmgPreservedOrder   * sLastSortingKeyCol;
    mtcNode             * sNode;

    IDU_FIT_POINT_FATAL( "qmgWindowing::classifySortingKeyNAnalFunc::__FT__" );

    //--------------
    // 기본 초기화
    //--------------
    sSortingKeyPosition  = 0;
    sSortingKeyPtrArr    = aMyGraph->sortingKeyPtrArr;
    sAnalyticFuncPtrArr  = aMyGraph->analyticFuncListPtrArr;

    /* BUG-46264 window sort의 order by 에서 같은 컬럼이 나올경우 중복 제거 */
    for ( sCurAnalFunc = aMyGraph->analyticFuncList;
          sCurAnalFunc != NULL;
          sCurAnalFunc = sCurAnalFunc->next )
    {
        for ( sOverColumn = sCurAnalFunc->analyticFuncNode->overClause->orderByColumn;
              sOverColumn != NULL;
              sOverColumn = sOverColumn->next )
        {
            sIsSame = ID_FALSE;

            for ( sOverColumn2 = sCurAnalFunc->analyticFuncNode->overClause->orderByColumn;
                  sOverColumn2 != NULL;
                  sOverColumn2 = sOverColumn2->next )
            {
                if ( sOverColumn == sOverColumn2 )
                {
                    break;
                }
                else
                {
                    if ( ( sOverColumn->node->node.table == sOverColumn2->node->node.table ) &&
                         ( sOverColumn->node->node.column == sOverColumn2->node->node.column ) )
                    {
                        sIsSame = ID_TRUE;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }

            if ( sIsSame == ID_TRUE )
            {
                sOverColumn3->next = sOverColumn->next;
            }
            else
            {
                sOverColumn3 = sOverColumn;
            }
        }
    }

    sCurAnalFunc = aMyGraph->analyticFuncList;
    while ( sCurAnalFunc != NULL )
    {
        for( sInnerAnalyticFunc = aMyGraph->analyticFuncList;
             sInnerAnalyticFunc != sCurAnalFunc;
             sInnerAnalyticFunc = sInnerAnalyticFunc->next )
        {
            /* PROJ-1805 window Caluse */
            if ( sInnerAnalyticFunc->analyticFuncNode->overClause->window == NULL )
            {
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sCurAnalFunc->analyticFuncNode,
                                                       sInnerAnalyticFunc->analyticFuncNode,
                                                       &sIsSame )
                          != IDE_SUCCESS );
                if( sIsSame == ID_TRUE )
                {
                    goto loop_end;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        sOverColumn = sCurAnalFunc->analyticFuncNode->overClause->overColumn;

        if ( sOverColumn == NULL )
        {
            // Partition By 절이 존재하지 않으므로
            // 별도의 sorting이 필요없음
            sFoundSameSortingKey = ID_TRUE;
            sReplaceSortingKey   = ID_FALSE;
            sSortingKeyPosition  = 0;
        }
        else
        {
            sFoundSameSortingKey = ID_FALSE;
            sReplaceSortingKey   = ID_FALSE;

            // partition by와 동일한 sorting key가 등록되어있는지 검사
            IDE_TEST( existSameSortingKeyAndDirection(
                          aMyGraph->sortingKeyCount,
                          sSortingKeyPtrArr,
                          sOverColumn,
                          & sFoundSameSortingKey,
                          & sReplaceSortingKey,
                          & sSortingKeyPosition )
                      != IDE_SUCCESS );
        }

        if ( sFoundSameSortingKey == ID_TRUE )
        {
            if ( sReplaceSortingKey == ID_TRUE )
            {
                //----------------------------------------
                // 현재 등록된 sorting key를 확장
                //----------------------------------------

                // (1) 마지막 sorting 찾음
                // (2) 추가할 sorting key col 정보를 가지는 첫번째
                //     partition column 찾음
                for ( sLastSortingKeyCol =
                          sSortingKeyPtrArr[sSortingKeyPosition],
                          sCurOverColumn = sOverColumn;
                      sLastSortingKeyCol->next != NULL;
                      sLastSortingKeyCol = sLastSortingKeyCol->next,
                          sCurOverColumn = sCurOverColumn->next ) ;
                
                sExpandOverColumn = sCurOverColumn->next;
                
                // (3) 남아있는 partition key column들을
                //     sorting key column들 뒤에 추가한다.
                while ( sExpandOverColumn != NULL )
                {
                    IDE_TEST(
                        QC_QMP_MEM(aStatement)->alloc(
                            ID_SIZEOF(qmgPreservedOrder),
                            (void**) &sNewSortingKeyCol )
                        != IDE_SUCCESS );

                    // BUG-34966 Pass node 일 수 있으므로 실제 값의 위치를 설정한다.
                    sNode = &sExpandOverColumn->node->node;

                    while( sNode->module == &qtc::passModule )
                    {
                        sNode = sNode->arguments;
                    }

                    sNewSortingKeyCol->table  = sNode->table;
                    sNewSortingKeyCol->column = sNode->column;

                    // To Fix BUG-21966
                    if ( (sExpandOverColumn->flag & QTC_OVER_COLUMN_MASK)
                         == QTC_OVER_COLUMN_NORMAL )
                    {
                        // others column
                        sNewSortingKeyCol->direction = QMG_DIRECTION_NOT_DEFINED;
                    }
                    else
                    {   
                        IDE_DASSERT( (sExpandOverColumn->flag & QTC_OVER_COLUMN_MASK)
                                     == QTC_OVER_COLUMN_ORDER_BY );
                            
                        // BUG-33663 Ranking Function
                        // order by column
                        if ( (sExpandOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                             == QTC_OVER_COLUMN_ORDER_ASC )
                        {
                            // BUG-40361 supporting to indexable analytic function
                            if ( ( sExpandOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                 == QTC_OVER_COLUMN_NULLS_ORDER_NONE )
                            {
                                sNewSortingKeyCol->direction = QMG_DIRECTION_ASC;
                            }
                            else if ( ( sExpandOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                      == QTC_OVER_COLUMN_NULLS_ORDER_FIRST )
                            {
                                sNewSortingKeyCol->direction = QMG_DIRECTION_ASC_NULLS_FIRST;
                            }
                            else
                            {
                                sNewSortingKeyCol->direction = QMG_DIRECTION_ASC_NULLS_LAST;
                            }
                        }
                        else
                        {
                            // BUG-40361 supporting to indexable analytic function
                            if ( ( sExpandOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                 == QTC_OVER_COLUMN_NULLS_ORDER_NONE )
                            {
                                sNewSortingKeyCol->direction = QMG_DIRECTION_DESC;
                            }
                            else if ( ( sExpandOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                      == QTC_OVER_COLUMN_NULLS_ORDER_FIRST )
                            {
                                sNewSortingKeyCol->direction = QMG_DIRECTION_DESC_NULLS_FIRST;
                            }
                            else
                            {
                                sNewSortingKeyCol->direction = QMG_DIRECTION_DESC_NULLS_LAST;
                            }
                        }
                    }
                    
                    sNewSortingKeyCol->next = NULL;
                    
                    sLastSortingKeyCol->next = sNewSortingKeyCol;
                    sLastSortingKeyCol       = sNewSortingKeyCol;
                    
                    sExpandOverColumn = sExpandOverColumn->next;
                }
            }
            else
            {
                // 이미 sorting key가 등록되었으며,
                // sorting key를 확장할 필요도 없는 경우
                // nothing to do 
            }
        }
        else
        {
            //----------------------------------------
            // 새로운 sorting key를 등록
            //----------------------------------------
            sLastSortingKeyCol = NULL;
            
            for ( sCurOverColumn = sOverColumn;
                  sCurOverColumn != NULL;
                  sCurOverColumn = sCurOverColumn->next )
            {
                IDE_TEST(
                    QC_QMP_MEM(aStatement)->alloc(
                        ID_SIZEOF(qmgPreservedOrder),
                        (void**) & sNewSortingKeyCol )
                    != IDE_SUCCESS );

                // BUG-34966 Pass node 일 수 있으므로 실제 값의 위치를 설정한다.
                sNode = &sCurOverColumn->node->node;

                while( sNode->module == &qtc::passModule )
                {
                    sNode = sNode->arguments;
                }

                sNewSortingKeyCol->table  = sNode->table;
                sNewSortingKeyCol->column = sNode->column;

                // To Fix BUG-21966
                if ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                     == QTC_OVER_COLUMN_NORMAL )
                {
                    sNewSortingKeyCol->direction = QMG_DIRECTION_NOT_DEFINED;
                }
                else
                {                    
                    IDE_DASSERT( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                                 == QTC_OVER_COLUMN_ORDER_BY );
                        
                    // BUG-33663 Ranking Function
                    // order by column
                    if ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                         == QTC_OVER_COLUMN_ORDER_ASC )
                    {
                        // BUG-40361 supporting to indexable analytic function
                        if ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                             == QTC_OVER_COLUMN_NULLS_ORDER_NONE )
                        {
                            sNewSortingKeyCol->direction = QMG_DIRECTION_ASC;
                        }
                        else if ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                  == QTC_OVER_COLUMN_NULLS_ORDER_FIRST )
                        {
                            sNewSortingKeyCol->direction = QMG_DIRECTION_ASC_NULLS_FIRST;
                        }
                        else
                        {
                            sNewSortingKeyCol->direction = QMG_DIRECTION_ASC_NULLS_LAST;
                        }
                    }
                    else
                    {
                        // BUG-40361 supporting to indexable analytic function
                        if ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                             == QTC_OVER_COLUMN_NULLS_ORDER_NONE )
                        {
                            sNewSortingKeyCol->direction = QMG_DIRECTION_DESC;
                        }
                        else if ( ( sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK )
                                  == QTC_OVER_COLUMN_NULLS_ORDER_FIRST )
                        {
                            sNewSortingKeyCol->direction = QMG_DIRECTION_DESC_NULLS_FIRST;
                        }
                        else
                        {
                            sNewSortingKeyCol->direction = QMG_DIRECTION_DESC_NULLS_LAST;
                        }
                    }
                }
                
                sNewSortingKeyCol->next = NULL;
                
                if ( sLastSortingKeyCol == NULL )
                {
                    // 첫번째 column인 경우
                    sSortingKeyPtrArr[sSortingKeyPosition] = sNewSortingKeyCol;
                    sLastSortingKeyCol = sNewSortingKeyCol;
                }
                else
                {
                    sLastSortingKeyCol->next = sNewSortingKeyCol;
                    sLastSortingKeyCol       = sNewSortingKeyCol;
                }
            }
        }

        //----------------------------------------
        // 현재 analytic function을
        // Sorting key와 대응되는 위치의
        // analytic function list에 연결한다.
        //----------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmsAnalyticFunc),
                                                (void**) & sNewAnalFunc )
                  != IDE_SUCCESS );

        idlOS::memcpy( sNewAnalFunc,
                       sCurAnalFunc,
                       ID_SIZEOF(qmsAnalyticFunc) );
        sNewAnalFunc->next = NULL;
        
        if( sAnalyticFuncPtrArr[sSortingKeyPosition] != NULL )
        {
            sNewAnalFunc->next =
                sAnalyticFuncPtrArr[sSortingKeyPosition];
            sAnalyticFuncPtrArr[sSortingKeyPosition] = sNewAnalFunc;
        }
        else
        {
            // analytic function 첫 등록인 경우
            sAnalyticFuncPtrArr[sSortingKeyPosition] = sNewAnalFunc;
        }

loop_end:
        sCurAnalFunc = sCurAnalFunc->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgWindowing::existSameSortingKeyAndDirection( UInt                 aSortingKeyCount,
                                               qmgPreservedOrder ** aSortingKeyPtrArr,
                                               qtcOverColumn      * aOverColumn,
                                               idBool             * aFoundSameSortingKey,
                                               idBool             * aReplaceSortingKey,
                                               UInt               * aSortingKeyPosition )
{
/***********************************************************************
 *
 * Description : 동일한 sorting key, direction 존재 유무 반환
 *
 * Implementation :
 *    (1) 이미 등록된 sorting key들을 방문하여
 *        현재 Partition By Column 이용 가능한지 검사
 *    (2) 사용 가능한 경우,
 *        (i)   sorting key 확장 여부 결정
 *        (ii) 확장해야 하는 경우, sorting key 확장
 *        사용 불가능한 경우, sorting key 생성
 *    (3) analytic function을 대응되는 sorting key 위치에 등록
 *
 ***********************************************************************/

    qmgPreservedOrder   * sCurSortingKeyCol;
    qtcOverColumn       * sCurOverColumn;
    UInt                  i;

    IDU_FIT_POINT_FATAL( "qmgWindowing::existSameSortingKeyAndDirection::__FT__" );

    //----------------------------------------------------
    // 이미 등록되어 있는 sorting key를 따라가며
    // 현재 partition by column list와 동일한 sorting key가
    // 존재하는지 검사
    //----------------------------------------------------
    
    for ( i = 0; i < aSortingKeyCount; i ++ )
    {
        // 현재 Sorting Key 얻음
        if ( aSortingKeyPtrArr[i] == NULL )
        {
            // 더이상 등록된 sorting key가 없음
            *aSortingKeyPosition = i;
            break;
        }
        else
        {
            // nothing to do 
        }
        
        //----------------------------------------
        // 등록된 sorting key column들과
        // partition by, order by column들이 동일한지 검사
        //----------------------------------------
        *aFoundSameSortingKey = ID_TRUE;

        for ( sCurOverColumn = aOverColumn, sCurSortingKeyCol = aSortingKeyPtrArr[i];
              ( sCurOverColumn != NULL ) && ( sCurSortingKeyCol != NULL );
              sCurOverColumn = sCurOverColumn->next, sCurSortingKeyCol = sCurSortingKeyCol->next )
        {
            if ((sCurOverColumn->node->node.table == sCurSortingKeyCol->table) &&
                (sCurOverColumn->node->node.column == sCurSortingKeyCol->column))
            {
                // BUG-33663 Ranking Function
                // order by column의 경우 order까지 같아야 한다.
                switch ( sCurSortingKeyCol->direction )
                {
                    case QMG_DIRECTION_NOT_DEFINED:
                    {
                        if ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                             == QTC_OVER_COLUMN_NORMAL )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    case QMG_DIRECTION_ASC:
                    {
                        if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                               == QTC_OVER_COLUMN_ORDER_BY )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_ASC ) &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK)
                               == QTC_OVER_COLUMN_NULLS_ORDER_NONE ) )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    case QMG_DIRECTION_DESC:
                    {
                        if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                               == QTC_OVER_COLUMN_ORDER_BY )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_DESC ) &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK)
                               == QTC_OVER_COLUMN_NULLS_ORDER_NONE ) )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    case QMG_DIRECTION_ASC_NULLS_FIRST:
                    {
                        if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                               == QTC_OVER_COLUMN_ORDER_BY )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_ASC ) &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK)
                               == QTC_OVER_COLUMN_NULLS_ORDER_FIRST ) )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    case QMG_DIRECTION_ASC_NULLS_LAST:
                    {
                        if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                               == QTC_OVER_COLUMN_ORDER_BY )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_ASC ) &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK)
                               == QTC_OVER_COLUMN_NULLS_ORDER_LAST ) )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    case QMG_DIRECTION_DESC_NULLS_FIRST:
                    {
                        if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                               == QTC_OVER_COLUMN_ORDER_BY )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_DESC ) &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK)
                               == QTC_OVER_COLUMN_NULLS_ORDER_FIRST ) )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    case QMG_DIRECTION_DESC_NULLS_LAST:
                    {
                        if ( ( (sCurOverColumn->flag & QTC_OVER_COLUMN_MASK)
                               == QTC_OVER_COLUMN_ORDER_BY )
                             &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK)
                               == QTC_OVER_COLUMN_ORDER_DESC ) &&
                             ( (sCurOverColumn->flag & QTC_OVER_COLUMN_NULLS_ORDER_MASK)
                               == QTC_OVER_COLUMN_NULLS_ORDER_LAST ) )
                        {
                            // 현재 칼럼은 동일
                        }
                        else
                        {
                            *aFoundSameSortingKey = ID_FALSE;
                        }
                        break;
                    }
                    default:
                    {
                        IDE_DASSERT(0);
                        *aFoundSameSortingKey = ID_FALSE;
                        break;
                    }
                }

                if ( *aFoundSameSortingKey == ID_FALSE )
                {
                    break;
                }
                else
                {
                    // 다음 칼럼 검사 진행
                }
            }
            else
            {
                *aFoundSameSortingKey = ID_FALSE;
                break;
            }
        }

        if ( *aFoundSameSortingKey == ID_TRUE )
        {
            // 동일 sorting key 찾은 경우
            if ( ( sCurOverColumn != NULL ) &&
                 ( sCurSortingKeyCol == NULL ) )
            {
                // Partition Key가 Sorting Key보다 칼럼이 더 많은 경우,
                // 등록된 Sorting Key를 확장한다.
                // ex) SELECT sum(i1) over ( partition by i1 ),
                //            sum(i2) over ( partition by i1, i2 )
                //     FROM t1;
                // (i1)이 sorting key로 등록된 경우,
                // (i1, i2)로 sorting key를 확장해야 한다.
                *aReplaceSortingKey = ID_TRUE;
            }
            else
            {
                // 현재 sorting key를 그대로 이용 가능함
                *aReplaceSortingKey = ID_FALSE;
            }
            
            *aSortingKeyPosition = i;
            break;
        }
        else
        {
            // nothing to do 
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmgWindowing::existWantOrderInSortingKey( qmgGraph          * aGraph,
                                          qmgPreservedOrder * aWantOrder,
                                          idBool            * aFind,
                                          UInt              * aFindSortingKeyPosition )
{
/***********************************************************************
 *
 * Description : 원하는 order가 sorting key들 중에 존재하는지 검사
 *
 * Implementation :
 *    (1) 원하는 preserved order와 동일한 sorting key가 존재하는지
 *        검사
 *    (2) 존재하는 경우, 첫번째 또는 마지막 sorting 순서로 변경
 *        - 첫번째 sorting key로 설정
 *        - 마지막 sorting key로 설정
 *    (3) 나의 preserved order를 마지막 sorting key로 설정
 ***********************************************************************/

    qmgPreservedOrder  ** sSortingKeyPtrArr;
    UInt                  sSortingKeyCount;
    qmgPreservedOrder   * sCurSortingKeyCol;
    qmgPreservedOrder   * sCurOrderCol;
    idBool                sFind;
    qmgDirectionType      sPrevDirection;
    idBool                sSameDirection;
    UInt                  i;

    IDU_FIT_POINT_FATAL( "qmgWindowing::existWantOrderInSortingKey::__FT__" );

    //--------------
    // 기본 초기화
    //--------------

    sFind                   = ID_FALSE;
    sSortingKeyPtrArr       = ((qmgWINDOW*)aGraph)->sortingKeyPtrArr;
    sSortingKeyCount        = ((qmgWINDOW*)aGraph)->sortingKeyCount;
    sSameDirection          = ID_FALSE;
    
    for ( i = 0 ; i < sSortingKeyCount; i ++ )
    {
        //------------------------------
        // 동일한 sorting key를 찾음
        //------------------------------

        sPrevDirection = QMG_DIRECTION_NOT_DEFINED;
        
        for ( sCurSortingKeyCol = sSortingKeyPtrArr[i],
                  sCurOrderCol = aWantOrder ;
              sCurSortingKeyCol != NULL &&
                  sCurOrderCol != NULL;
              sCurSortingKeyCol = sCurSortingKeyCol->next,
                  sCurOrderCol = sCurOrderCol->next )
        {
            if ( ( sCurSortingKeyCol->table == sCurOrderCol->table ) &&
                 ( sCurSortingKeyCol->column == sCurOrderCol->column ) )
            {
                /* BUG-42145
                 * Table이나 Partition에 Index가 있는 경우라면
                 * Nulls Option 이 고려된 Direction Check를 한다.
                 */
                if ( ( ( aGraph->left->type == QMG_SELECTION ) ||
                       ( aGraph->left->type == QMG_PARTITION ) ) &&
                     ( aGraph->left->myFrom->tableRef->view == NULL ) )
                {
                    IDE_TEST( qmg::checkSameDirection4Index( sCurOrderCol,
                                                             sCurSortingKeyCol,
                                                             sPrevDirection,
                                                             & sPrevDirection,
                                                             & sSameDirection )
                              != IDE_SUCCESS );
                }
                else
                {
                    // BUG-28507
                    // sorting key가 동일한 경우,
                    // soring의 방향성(ASC, DESC) 검사
                    IDE_TEST( qmg::checkSameDirection( sCurOrderCol,
                                                       sCurSortingKeyCol,
                                                       sPrevDirection,
                                                       & sPrevDirection,
                                                       & sSameDirection )
                              != IDE_SUCCESS );
                }

                if ( sSameDirection == ID_TRUE )
                {
                    // 다음 칼럼 비교
                    sFind = ID_TRUE;
                }
                else
                {
                    // 방향성 달라 동일 order로 볼 수 없음
                    sFind = ID_FALSE;
                    break;
                }
            }
            else
            {
                sFind = ID_FALSE;
                break;
            }

            sPrevDirection = sCurSortingKeyCol->direction;
        }
        
        if ( sFind == ID_TRUE )
        {
            if ( ( sCurOrderCol != NULL ) || ( sCurSortingKeyCol != NULL ) )
            {
                // Order Column이 남은 경우,
                // 동일 order를 가지는 sorting key 찾는것은 실패한 것임
                sFind = ID_FALSE;
            }
            else
            {
                // order와 동일한 sorting key 찾음
                *aFindSortingKeyPosition = i;
                break;
            }
        }
        else
        {
            // nothing to do 
        }
    }

    *aFind = sFind;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgWindowing::alterSortingOrder( qcStatement       * aStatement,
                                 qmgGraph          * aGraph,
                                 qmgPreservedOrder * aWantOrder,
                                 idBool              aAlterFirstOrder )
{
/***********************************************************************
 *
 * Description : first sorting key 설정
 *
 * Implementation :
 *    (1) 원하는 preserved order와 동일한 sorting key가 존재하는지
 *        검사
 *    (2) 존재하는 경우, 첫번째 또는 마지막 sorting 순서로 변경
 *        - 첫번째 sorting key로 설정
 *        - 마지막 sorting key로 설정
 *    (3) 나의 preserved order를 마지막 sorting key로 설정
 ***********************************************************************/

    qmgWINDOW           * sMyGraph;
    qmgPreservedOrder  ** sSortingKeyPtrArr;
    qmsAnalyticFunc    ** sAnalFuncListPtrArr;
    idBool                sFind;
    UInt                  sFindSortingKeyPosition;
    qmgPreservedOrder   * sFindSortingKeyPtr        = NULL;
    qmgPreservedOrder   * sCurOrder;
    qmgPreservedOrder   * sCurSortingKey            = NULL;
    qmsAnalyticFunc     * sFindAnalFuncListPtr;
    idBool                sSuccess = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmgWindowing::alterSortingOrder::__FT__" );

    //--------------
    // 기본 초기화
    //--------------

    sMyGraph                = (qmgWINDOW*)aGraph;
    sFind                   = ID_FALSE;
    sSortingKeyPtrArr       = sMyGraph->sortingKeyPtrArr;
    sAnalFuncListPtrArr     = sMyGraph->analyticFuncListPtrArr;
    sFindSortingKeyPosition = 0;
    
    //------------------------------
    // want order와 동일한 sorting 찾음
    //------------------------------
    
    IDE_TEST( existWantOrderInSortingKey( aGraph,
                                          aWantOrder,
                                          & sFind, 
                                          & sFindSortingKeyPosition )
              != IDE_SUCCESS );

    if ( aAlterFirstOrder == ID_TRUE )
    {
        //------------------------------
        // 찾은 sorting key를 첫번째로 옮김
        // 하위 graph에서 올라오는 preserved order 사용 가능한 경우,
        // 이를 첫번째 sorting 위치로 두어 별도의 sorting을
        // 하지 않도록 함
        //------------------------------
        if ( sFind == ID_TRUE )
        {
            sFindSortingKeyPtr = sSortingKeyPtrArr[sFindSortingKeyPosition];

            sSortingKeyPtrArr[sFindSortingKeyPosition] = sSortingKeyPtrArr[0];
            sSortingKeyPtrArr[0] = sFindSortingKeyPtr;

            sFindAnalFuncListPtr =
                sAnalFuncListPtrArr[sFindSortingKeyPosition];

            sAnalFuncListPtrArr[sFindSortingKeyPosition] =
                sAnalFuncListPtrArr[0];
            sAnalFuncListPtrArr[0] = sFindAnalFuncListPtr;

            /* BUG-43690 하위 preserved order의 direction도 결정한다. */
            if ( ( aGraph->left->preservedOrder->direction == QMG_DIRECTION_NOT_DEFINED ) &&
                 ( aGraph->left->preservedOrder->table == sFindSortingKeyPtr->table ) &&
                 ( aGraph->left->preservedOrder->column == sFindSortingKeyPtr->column ) )
            {
                aGraph->left->preservedOrder->direction = sFindSortingKeyPtr->direction;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            // 사용 가능한 preserved order가 없는 경우,
            // child preserved order 사용할 수 없음
            aGraph->flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
            aGraph->flag |= QMG_CHILD_PRESERVED_ORDER_CANNOT_USE;
        }

        if ( sMyGraph->sortingKeyCount > 0 )
        {
            //------------------------------
            // 마지막 sorting key를 Preserved Order 설정
            // ( 상위 Graph에서 원하는 preserved order로 변경 가능하므로
            //    Defined Not Fixed 이며, 동일 order 변경 여부에 상관없이
            //    마지막 sorting key를 나의 preserved order로 설정 )
            //------------------------------
            sMyGraph->graph.preservedOrder =
                sSortingKeyPtrArr[sMyGraph->sortingKeyCount - 1];
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_NOT_FIXED;
        }
        else
        {
            // sorting key가 없는 경우
            // 하위의 preserved order를 나의 order로 설정함
            sMyGraph->graph.preservedOrder = aWantOrder;

            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= ( aGraph->left->flag &
                                      QMG_PRESERVED_ORDER_MASK );
        }
    }
    else
    {
        //------------------------------
        // 찾은 sorting key를 마지막으로 옮김
        // qmgWindowing 상위 Graph (즉, qmgSorting)에서
        // 필요한 want order를 찾은 경우,
        // 마지막에 sorting 하도록 하여 상위 graph에서
        // 별도의 sorting을 하지 않도록 함
        //------------------------------
        
        if ( sFind == ID_TRUE )
        {
            if ( ( sFindSortingKeyPosition == 0 ) &&
                 ( sMyGraph->sortingKeyCount > 1 ) )
            {
                // BUG-28507
                // 찾은 sorting order가 첫번째 인 경우,
                // 하위 preserved order를 사용할 수 없다고 설정해야함
                aGraph->flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
                aGraph->flag |= QMG_CHILD_PRESERVED_ORDER_CANNOT_USE;
            }
            else
            {
                // BUG-40361
                // Sort Key가 1개이고 하위 Order 정해져 있지 않다면
                // 하위에도 상위에서 내려온 Sort Key를 정해주도록 한다.
                if ( sMyGraph->sortingKeyCount == 1 )
                {
                    IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                                      &sMyGraph->graph,
                                                      aWantOrder,
                                                      & sSuccess )
                              != IDE_SUCCESS );
                    if ( sSuccess == ID_FALSE )
                    {
                        aGraph->flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
                        aGraph->flag |= QMG_CHILD_PRESERVED_ORDER_CANNOT_USE;
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

            sFindSortingKeyPtr = sSortingKeyPtrArr[sFindSortingKeyPosition];

            sSortingKeyPtrArr[sFindSortingKeyPosition] =
                sSortingKeyPtrArr[sMyGraph->sortingKeyCount - 1];
            sSortingKeyPtrArr[sMyGraph->sortingKeyCount - 1]
                = sFindSortingKeyPtr;

            sFindAnalFuncListPtr = sAnalFuncListPtrArr[sFindSortingKeyPosition];

            sAnalFuncListPtrArr[sFindSortingKeyPosition] =
                sAnalFuncListPtrArr[sMyGraph->sortingKeyCount - 1];
            sAnalFuncListPtrArr[sMyGraph->sortingKeyCount - 1] =
                sFindAnalFuncListPtr;

            //------------------------------
            // 마지막 sorting key를 Preserved Order 설정
            // ( 상위에서 필요한 prserved order로 설정한 경우이므로
            //    Defined Fixed 이며, 마지막 sorting key가 변경되었을
            //    때에만 나의 preserved order 변경 )
            //------------------------------
            sMyGraph->graph.preservedOrder =
                sSortingKeyPtrArr[sMyGraph->sortingKeyCount - 1];
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
        }
        else
        {
            // 사용 가능한 preserved order가 없는 경우,
            // child preserved order 사용할 수 없음
            aGraph->flag &= ~QMG_CHILD_PRESERVED_ORDER_USE_MASK;
            aGraph->flag |= QMG_CHILD_PRESERVED_ORDER_CANNOT_USE;
        }
    }

    if ( sFind == ID_TRUE )
    {
        // To Fix BUG-21966
        for ( sCurSortingKey = sFindSortingKeyPtr,
                  sCurOrder = aWantOrder;
              sCurSortingKey != NULL &&
                  sCurOrder != NULL;
              sCurSortingKey = sCurSortingKey->next,
                  sCurOrder = sCurOrder->next )
        {
            if ( sCurOrder->direction != QMG_DIRECTION_NOT_DEFINED )
            {
                sCurSortingKey->direction = sCurOrder->direction;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        // nothing to do 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgWindowing::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgWindowing으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    (1) WNST의 materialize node 구성 방식 결정
 *        Base Table + Column + Analytic Functions
 *       < RID 방식 >
 *       - 일반
 *         Base Table         : 관련 table들의 RID
 *         Column             : Partition By Columns + Arguments
 *         Analytic Functions 
 *       - 하위에 Hash Based Grouping 처리가 있을 경우
 *         Base Table         : 하위 GRAG의 RID
 *         Column             : Partition By Columns + Arguments
 *         Analytic Functions 
 *       - 하위에 Sort Based Grouping 처리가 있을 경우
 *         Base Table         : target list에 속한 column들
 *         Column             : Partition By Columns + Arguments
 *         Analytic Functions 
 *       < Push Projection 방식 >
 *       Base Table        : 상위 plan 수행에 필요한 칼럼
 *       Column            : Partition By Columns + Arguments
 *       Analytic Functions 
 *    (2) WNST plan 생성
 ***********************************************************************/

    qmgWINDOW       * sMyGraph;
    qmnPlan         * sWNST;
    UInt              sFlag;

    IDU_FIT_POINT_FATAL( "qmgWindowing::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------
    sMyGraph = (qmgWINDOW*) aGraph;
    sFlag    = 0;
    
    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // SCAN parallel flag 를 자식 노드로 물려준다.
    aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

    // BUG-37277
    if ( ( aParent->type == QMG_SORTING ) &&
         ( sMyGraph->graph.left->type == QMG_GROUPING ) )
    {
        sFlag &= ~QMO_MAKEWNST_ORDERBY_GROUPBY_MASK;
        sFlag |= QMO_MAKEWNST_ORDERBY_GROUPBY_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qmoOneMtrPlan::initWNST( aStatement,
                                       sMyGraph->graph.myQuerySet,
                                       sMyGraph->sortingKeyCount,
                                       sMyGraph->analyticFuncListPtrArr,
                                       sFlag,
                                       aParent->myPlan,
                                       &sWNST ) != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sWNST;

    //---------------------------------------------------
    // 하위 Plan의 생성
    //---------------------------------------------------

    IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                              &sMyGraph->graph ,
                                              sMyGraph->graph.left )
              != IDE_SUCCESS);
    sMyGraph->graph.myPlan = sMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정
    //---------------------------------------------------
    sMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_WINDOW;

    //-----------------------------------------------------
    // WNST 노드 생성시 Base Table을 저장하는 방법
    //
    // 1. 하위 Graph가 일반 Graph인 경우
    //    Child의 dependencies를 이용한 Base Table의 생성
    //
    // 2. 하위가 Grouping인 경우
    //    - Sort-Based인 경우 : Target과 Order By를 모두저장
    //                          (후에 target을 sort의 dst로 변경)
    //    - Hash-Based인 경우 : child에서 GRAG를 찾아 Base Table로 저장
    //                          (중간에 FILT가 올수있다 )
    //
    // 3. 하위가 Set인 경우
    //    child인 HSDS를 Base Table로 저장
    //-----------------------------------------------------

    sFlag = 0;
    if( sMyGraph->graph.left->type == QMG_GROUPING )
    {
        if( (sMyGraph->graph.left->flag & QMG_SORT_HASH_METHOD_MASK )
            == QMG_SORT_HASH_METHOD_SORT )
        {
            sFlag &= ~QMO_MAKEWNST_BASETABLE_MASK;
            sFlag |= QMO_MAKEWNST_BASETABLE_TARGET;
        }
        else
        {
            sFlag &= ~QMO_MAKEWNST_BASETABLE_MASK;
            sFlag |= QMO_MAKEWNST_BASETABLE_GRAG;
        }
    }
    else if( sMyGraph->graph.left->type == QMG_SET )
    {
        //----------------------------------------------------------
        // PR-8369
        // 하위가 SET인경우 VIEW가 생성되므로 DEPENDENCY로 부터
        // BASE TABLE을 생성한다.
        // 다만, UNION인 경우 , HSDS로 부터 BASE TABLE을 저장한다.
        //----------------------------------------------------------
        if( ((qmgSET*)sMyGraph->graph.left)->setOp == QMS_UNION )
        {
            sFlag &= ~QMO_MAKEWNST_BASETABLE_MASK;
            sFlag |= QMO_MAKEWNST_BASETABLE_HSDS;
        }
        else
        {
            sFlag &= ~QMO_MAKEWNST_BASETABLE_MASK;
            sFlag |= QMO_MAKEWNST_BASETABLE_DEPENDENCY;
        }
    }
    else
    {
        sFlag &= ~QMO_MAKEWNST_BASETABLE_MASK;
        sFlag |= QMO_MAKEWNST_BASETABLE_DEPENDENCY;
    }

    //----------------------------
    // WNST 노드의 생성
    //----------------------------
    
    if ( ( sMyGraph->graph.flag & QMG_CHILD_PRESERVED_ORDER_USE_MASK )
         == QMG_CHILD_PRESERVED_ORDER_CANNOT_USE )
    {
        sFlag &= ~QMO_MAKEWNST_PRESERVED_ORDER_MASK;
        sFlag |= QMO_MAKEWNST_PRESERVED_ORDER_FALSE;
    }
    else
    {
        sFlag &= ~QMO_MAKEWNST_PRESERVED_ORDER_MASK;
        sFlag |= QMO_MAKEWNST_PRESERVED_ORDER_TRUE;
    }

    //저장 매체의 선택
    if( (sMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
        QMG_GRAPH_TYPE_MEMORY )
    {
        sFlag &= ~QMO_MAKEWNST_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEWNST_MEMORY_TEMP_TABLE;
    }
    else
    {
        sFlag &= ~QMO_MAKEWNST_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEWNST_DISK_TEMP_TABLE;
    }
    
    // sorting key는 analytic function의 partition by column들의
    // (table, column) 값과 동일하다.
    // 그러나 partition by column 의 (table, column)값이
    // 하위 plan 생성 시에 변할 수도 있다.
    // 따라서 이에 따라 sorting key 를 보정하여야 한다.
    IDE_TEST( resetSortingKey( sMyGraph->sortingKeyPtrArr,
                               sMyGraph->sortingKeyCount,
                               sMyGraph->analyticFuncListPtrArr )
              != IDE_SUCCESS );

    // BUG-33663 Ranking Function
    // partition by expr의 order가 결정된 후 sorting key array에서
    // 제거가능한 sorting key를 제거
    IDE_TEST( compactSortingKeyArr( sMyGraph->graph.flag,
                                    sMyGraph->sortingKeyPtrArr,
                                    & sMyGraph->sortingKeyCount,
                                    sMyGraph->analyticFuncListPtrArr )
              != IDE_SUCCESS );

    /* BUG-40354 pushed rank */
    if ( (sMyGraph->graph.flag & QMG_WINDOWING_PUSHED_RANK_MASK) ==
         QMG_WINDOWING_PUSHED_RANK_TRUE )
    {
        if ( sMyGraph->sortingKeyCount == 1 )
        {
            sFlag &= ~QMO_MAKEWNST_TEMP_TABLE_MASK;
            sFlag |= QMO_MAKEWNST_MEMORY_TEMP_TABLE;
            
            sFlag &= ~QMO_MAKEWNST_PUSHED_RANK_MASK;
            sFlag |= QMO_MAKEWNST_PUSHED_RANK_TRUE;
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
    
    IDE_TEST( qmoOneMtrPlan::makeWNST( aStatement,
                                       sMyGraph->graph.myQuerySet,
                                       sFlag,
                                       sMyGraph->distAggArg,
                                       sMyGraph->sortingKeyCount,
                                       sMyGraph->sortingKeyPtrArr,
                                       sMyGraph->analyticFuncListPtrArr,
                                       sMyGraph->graph.myPlan ,
                                       sMyGraph->graph.costInfo.inputRecordCnt,
                                       sWNST ) != IDE_SUCCESS);

    sMyGraph->graph.myPlan = sWNST;

    qmg::setPlanInfo( aStatement, sWNST, &(sMyGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgWindowing::resetSortingKey( qmgPreservedOrder ** aSortingKeyPtrArr,
                               UInt                 aSortingKeyCount,
                               qmsAnalyticFunc   ** aAnalyticFuncListPtrArr)
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *    Child Plan 생성 시에 Sorting Key (table, column) 정보가
 *    변경되는 경우가 있음
 *    따라서 Child Plan 생성 후, 이를 보정해줌
 *
 ***********************************************************************/

    qmgPreservedOrder   * sCurSortingKeyCol;
    UInt                  sSortingKeyColCnt;
    UInt                  sOverColumnCnt;
    qmsAnalyticFunc     * sAnalFunc;
    qtcOverColumn       * sCurPartAndOrderByCol;
    UInt                  i;
    mtcNode             * sNode;

    IDU_FIT_POINT_FATAL( "qmgWindowing::resetSortingKey::__FT__" );

    for ( i = 0; i < aSortingKeyCount; i++ )
    {
        //--------------------------------------------------
        // Sorting Key와 동일한 Partition By Column을 가지는
        // Analytic Function을 찾음
        //--------------------------------------------------
        sSortingKeyColCnt = 0;
        for ( sCurSortingKeyCol = aSortingKeyPtrArr[i];
              sCurSortingKeyCol != NULL;
              sCurSortingKeyCol = sCurSortingKeyCol->next )
        {
            sSortingKeyColCnt++;
        }

        // sorting key array에서 i번째 sorting key는
        // analytic function list pointer array에서 i번째에 있는
        // anlaytic function list들의 sorting key 이므로
        // 이중에서 동일 key count를 가지는 partition by column을 찾으면
        // 된다.
        for ( sAnalFunc = aAnalyticFuncListPtrArr[i];
              sAnalFunc != NULL;
              sAnalFunc = sAnalFunc->next )
        {
            sOverColumnCnt = 0;
            for ( sCurPartAndOrderByCol =
                      sAnalFunc->analyticFuncNode->overClause->overColumn;
                  sCurPartAndOrderByCol != NULL;
                  sCurPartAndOrderByCol = sCurPartAndOrderByCol->next )
            {
                sOverColumnCnt++;
            }

            if ( sSortingKeyColCnt == sOverColumnCnt )
            {
                break;
            }
            else
            {
                // 새로운 analytic function 찾음
            }
        }

        IDE_TEST_RAISE( sAnalFunc == NULL,
                        ERR_INVALID_ANAL_FUNC );

        //--------------------------------------------------
        // Sorting Key 정보가 변경된 경우, 재 설정
        //--------------------------------------------------
        
        for ( sCurSortingKeyCol = aSortingKeyPtrArr[i],
                  sCurPartAndOrderByCol =
                  sAnalFunc->analyticFuncNode->overClause->overColumn;
              sCurSortingKeyCol != NULL &&
                  sCurPartAndOrderByCol != NULL;
              sCurSortingKeyCol = sCurSortingKeyCol->next,
                  sCurPartAndOrderByCol = sCurPartAndOrderByCol->next )
        {
            sNode = &sCurPartAndOrderByCol->node->node;

            while( sNode->module == &qtc::passModule )
            {
                sNode = sNode->arguments;
            }

            sCurSortingKeyCol->table  = sNode->table;
            sCurSortingKeyCol->column = sNode->column;
        }

        // sorting key와 partition by col은 동일한 갯수로 있어야 함
        // 남는 경우 없음
        IDE_DASSERT( sCurSortingKeyCol == NULL );
        IDE_DASSERT( sCurPartAndOrderByCol == NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ANAL_FUNC )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgWindowing::resetSortingKey",
                                  "sAnalFunc is NULL" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgWindowing::compactSortingKeyArr( UInt                 aFlag,
                                    qmgPreservedOrder ** aSortingKeyPtrArr,
                                    UInt               * aSortingKeyCount,
                                    qmsAnalyticFunc   ** aAnalyticFuncListPtrArr )
{
/***********************************************************************
 *
 * Description :
 *    order by expr은 order가 고정되고 별도의 sorting key로 등록되었다.
 *    이후 partition by expr의 order가 결정되면 동일한 order를
 *    가질 수 있으므로 sorting key를 제거할 수 있다.
 *
 * Implementation :
 *    first order(child preserved order)와
 *    last order(parent preserved order)를 고려하여 sorting key array에서
 *    동일한 order를 갖는 sorting key를 제거한다.
 *
 ***********************************************************************/

    qmsAnalyticFunc  * sAnalyticFuncList;
    UInt               sSortingKeyCount;
    UInt               sStartKey;
    UInt               sEndKey;
    idBool             sIsFound;
    UInt               i;
    UInt               j;

    IDU_FIT_POINT_FATAL( "qmgWindowing::resetSortingKey::__FT__" );

    sSortingKeyCount = *aSortingKeyCount;

    if ( sSortingKeyCount > 1 )
    {
        //----------------------------------
        // last order를 제외한 부분 중복 제거
        //----------------------------------

        sStartKey = 0;

        if ( (aFlag & QMG_PRESERVED_ORDER_MASK)
             == QMG_PRESERVED_ORDER_DEFINED_FIXED )
        {
            sEndKey = sSortingKeyCount - 1;
        }
        else
        {
            sEndKey = sSortingKeyCount;
        }

        for ( i = sStartKey; i < sEndKey; i++ )
        {
            if ( aSortingKeyPtrArr[i] == NULL )
            {
                continue;
            }
            else
            {
                // Nothing to do.
            }

            for ( j = i + 1; j < sEndKey; j++ )
            {
                if ( aSortingKeyPtrArr[j] == NULL )
                {
                    continue;
                }
                else
                {
                    // Nothing to do.
                }

                if ( isSameSortingKey( aSortingKeyPtrArr[i],
                                       aSortingKeyPtrArr[j] ) == ID_TRUE )
                {
                    // merge sorting key
                    mergeSortingKey( aSortingKeyPtrArr[i],
                                     aSortingKeyPtrArr[j] );
                    aSortingKeyPtrArr[j] = NULL;

                    // merge analytic function
                    for ( sAnalyticFuncList = aAnalyticFuncListPtrArr[i];
                          sAnalyticFuncList->next != NULL;
                          sAnalyticFuncList = sAnalyticFuncList->next ) ;

                    sAnalyticFuncList->next = aAnalyticFuncListPtrArr[j];
                    aAnalyticFuncListPtrArr[j] = NULL;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        //----------------------------------
        // last order 중복 제거
        //----------------------------------

        if ( (aFlag & QMG_PRESERVED_ORDER_MASK)
             == QMG_PRESERVED_ORDER_DEFINED_FIXED )
        {
            if ( (aFlag & QMG_CHILD_PRESERVED_ORDER_USE_MASK)
                 == QMG_CHILD_PRESERVED_ORDER_CAN_USE )
            {
                sStartKey = 1;
            }
            else
            {
                sStartKey = 0;
            }

            sEndKey = sSortingKeyCount - 1;

            i = sSortingKeyCount - 1;

            for ( j = sStartKey; j < sEndKey; j++ )
            {
                if ( aSortingKeyPtrArr[j] == NULL )
                {
                    continue;
                }
                else
                {
                    // Nothing to do.
                }

                if ( isSameSortingKey( aSortingKeyPtrArr[i],
                                       aSortingKeyPtrArr[j] ) == ID_TRUE )
                {
                    // merge sorting key
                    mergeSortingKey( aSortingKeyPtrArr[i],
                                     aSortingKeyPtrArr[j] );
                    aSortingKeyPtrArr[j] = NULL;

                    // merge analytic function
                    for ( sAnalyticFuncList = aAnalyticFuncListPtrArr[i];
                          sAnalyticFuncList->next != NULL;
                          sAnalyticFuncList = sAnalyticFuncList->next ) ;

                    sAnalyticFuncList->next = aAnalyticFuncListPtrArr[j];
                    aAnalyticFuncListPtrArr[j] = NULL;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else
        {
            // Nothing to do.
        }

        //----------------------------------
        // 제거한 sort key 정리
        //----------------------------------

        sStartKey = 0;
        sEndKey = sSortingKeyCount;

        for ( i = sStartKey; i < sEndKey; i++ )
        {
            if ( aSortingKeyPtrArr[i] == NULL )
            {
                sIsFound = ID_FALSE;

                for ( j = i + 1; j < sEndKey; j++ )
                {
                    if ( aSortingKeyPtrArr[j] != NULL )
                    {
                        sIsFound = ID_TRUE;
                        break;
                    }
                }

                if ( sIsFound == ID_TRUE )
                {
                    aSortingKeyPtrArr[i] = aSortingKeyPtrArr[j];
                    aSortingKeyPtrArr[j] = NULL;

                    aAnalyticFuncListPtrArr[i] = aAnalyticFuncListPtrArr[j];
                    aAnalyticFuncListPtrArr[j] = NULL;
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

        sSortingKeyCount = 0;

        for ( i = sStartKey; i < sEndKey; i++ )
        {
            if ( aSortingKeyPtrArr[i] != NULL )
            {
                sSortingKeyCount++;
            }
            else
            {
                break;
            }
        }

        *aSortingKeyCount = sSortingKeyCount;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

idBool
qmgWindowing::isSameSortingKey( qmgPreservedOrder * aSortingKey1,
                                qmgPreservedOrder * aSortingKey2 )
{
/***********************************************************************
 *
 * Description :
 *     aSortingKey1과 aSortingKey2가 같은 order를 갖는지 비교한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgPreservedOrder  * sOrder1;
    qmgPreservedOrder  * sOrder2;
    idBool               sIsSame;

    sIsSame = ID_TRUE;

    for ( sOrder1 = aSortingKey1, sOrder2 = aSortingKey2;
          (sOrder1 != NULL) && (sOrder2 != NULL);
          sOrder1 = sOrder1->next, sOrder2 = sOrder2->next )
    {
        if ( (sOrder1->table == sOrder2->table ) &&
             (sOrder1->column == sOrder2->column ) )
        {
            switch ( sOrder1->direction )
            {
                case QMG_DIRECTION_NOT_DEFINED:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_ASC:
                        case QMG_DIRECTION_DESC:
                        case QMG_DIRECTION_ASC_NULLS_FIRST:
                        case QMG_DIRECTION_ASC_NULLS_LAST:
                        case QMG_DIRECTION_DESC_NULLS_FIRST:
                        case QMG_DIRECTION_DESC_NULLS_LAST:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }
                    
                    break;
                }
                case QMG_DIRECTION_ASC:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_ASC:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }
                    
                    break;
                }
                case QMG_DIRECTION_DESC:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_DESC:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }
                    
                    break;
                }
                case QMG_DIRECTION_ASC_NULLS_FIRST:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_ASC_NULLS_FIRST:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }

                    break;
                }
                case QMG_DIRECTION_ASC_NULLS_LAST:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_ASC_NULLS_LAST:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }

                    break;
                }
                case QMG_DIRECTION_DESC_NULLS_FIRST:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_DESC_NULLS_FIRST:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }

                    break;
                }
                case QMG_DIRECTION_DESC_NULLS_LAST:
                {
                    switch ( sOrder2->direction )
                    {
                        case QMG_DIRECTION_NOT_DEFINED:
                        case QMG_DIRECTION_DESC_NULLS_LAST:
                        {
                            break;
                        }
                        default:
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }

                    break;
                }
                default:
                {
                    sIsSame = ID_FALSE;
                    break;
                }
            }
        }
        else
        {
            sIsSame = ID_FALSE;
        }
    }

    if ( sIsSame == ID_TRUE )
    {
        if ( (sOrder1 == NULL) && (sOrder2 == NULL) )
        {
            // Nothing to do.
        }
        else
        {
            sIsSame = ID_FALSE;
        }
    }
    else
    {
        // Nothing to do.
    }

    return sIsSame;
}

void
qmgWindowing::mergeSortingKey( qmgPreservedOrder * aSortingKey1,
                               qmgPreservedOrder * aSortingKey2 )
{
/***********************************************************************
 *
 * Description :
 *     aSortingKey1에 aSortingKey2의 order를 병합한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgPreservedOrder  * sOrder1;
    qmgPreservedOrder  * sOrder2;

    for ( sOrder1 = aSortingKey1, sOrder2 = aSortingKey2;
          (sOrder1 != NULL) && (sOrder2 != NULL);
          sOrder1 = sOrder1->next, sOrder2 = sOrder2->next )
    {
        switch ( sOrder1->direction )
        {
            case QMG_DIRECTION_NOT_DEFINED:
            {
                switch ( sOrder2->direction )
                {
                    case QMG_DIRECTION_NOT_DEFINED:
                    case QMG_DIRECTION_ASC:
                    case QMG_DIRECTION_DESC:
                    case QMG_DIRECTION_ASC_NULLS_FIRST:
                    case QMG_DIRECTION_ASC_NULLS_LAST:
                    case QMG_DIRECTION_DESC_NULLS_FIRST:
                    case QMG_DIRECTION_DESC_NULLS_LAST:
                    {
                        sOrder1->direction = sOrder2->direction;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                
                break;
            }
            default:
            {
                break;
            }
        }
    }

}

IDE_RC
qmgWindowing::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgWindowing::printGraph::__FT__" );

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
qmgWindowing::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Windowing Graph의 Preserved order Finalizing은
 *                다른 graph와 다르게 Sorting key를 처리한다.
 *                배열의 첫번째 key는 child graph의 direction을 따른다.
 *  (단, graph의 flag에 QMG_CHILD_PRESERVED_ORDER_CANNOT_USE가 세팅되어 있으면 안된다.)
 *                배열의 두번째부터 마지막 key까지는 direction은 ascending으로 임의 지정한다.
 *
 *  Implementation :
 *     1. Child graph의 Preserved order 사용 여부에 따라 첫번째 Sorting Key를 설정한다.
 *     2. 나머지는 Ascending으로 지정한다.
 *
 ***********************************************************************/

    qmgWINDOW         *  sMyGraph;
    qmgPreservedOrder ** sSortingKeyPtrArr;
    qmgPreservedOrder *  sPreservedOrder;
    qmgDirectionType     sPrevDirection;
    qmgPreservedOrder *  sChildOrder;

    UInt                 i;

    IDU_FIT_POINT_FATAL( "qmgWindowing::finalizePreservedOrder::__FT__" );

    // BUG-36803 Windowing Graph must have a left child graph
    IDE_DASSERT( aGraph->left != NULL );

    sMyGraph = (qmgWINDOW *)aGraph;

    sSortingKeyPtrArr = sMyGraph->sortingKeyPtrArr;
    sPreservedOrder   = aGraph->preservedOrder;
    sChildOrder       = aGraph->left->preservedOrder;

    /* BUG-43380 windowsort Optimize 도중에 fatal */
    /* 하위에 index가 존재해 sorting할 필요가 없을수 있음 */
    if ( sMyGraph->sortingKeyCount > 0 )
    {
        // 1. Child graph의 Preserved order 사용 여부에 따라 첫번째 Sorting Key를 설정한다.
        if ( ( aGraph->flag & QMG_CHILD_PRESERVED_ORDER_USE_MASK )
             == QMG_CHILD_PRESERVED_ORDER_CAN_USE )
        {
            sPreservedOrder = sSortingKeyPtrArr[0];

            if ( sPreservedOrder->direction == QMG_DIRECTION_NOT_DEFINED )
            {
                sPreservedOrder->direction = sChildOrder->direction;
                sPrevDirection = sChildOrder->direction;
            }
            else
            {
                IDE_DASSERT( sPreservedOrder->direction == sChildOrder->direction );
                sPrevDirection = sPreservedOrder->direction;
            }
        }
        else
        {
            sPreservedOrder = sSortingKeyPtrArr[0];

            if ( sPreservedOrder->direction == QMG_DIRECTION_NOT_DEFINED )
            {
                sPreservedOrder->direction = QMG_DIRECTION_ASC;
                sPrevDirection = QMG_DIRECTION_ASC;
            }
            else
            {
                sPrevDirection = sPreservedOrder->direction;
            }
        }

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
                    break;
                case QMG_DIRECTION_DESC :
                    break;
                case QMG_DIRECTION_ASC_NULLS_FIRST :
                    break;
                case QMG_DIRECTION_ASC_NULLS_LAST :
                    break;
                case QMG_DIRECTION_DESC_NULLS_FIRST :
                    break;
                case QMG_DIRECTION_DESC_NULLS_LAST :
                    break;
                default:
                    break;
            }
                
            sPrevDirection = sPreservedOrder->direction;
        }
    }
    else
    {
        /* Nothing to do */
    }

    // 2. 나머지는 Ascending으로 지정한다.
    for ( i = 1; i < sMyGraph->sortingKeyCount; i++ )
    {
        sPreservedOrder = sSortingKeyPtrArr[i];

        if ( sPreservedOrder->direction == QMG_DIRECTION_NOT_DEFINED )
        {
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
                        break;
                    case QMG_DIRECTION_DESC :
                        break;
                    case QMG_DIRECTION_ASC_NULLS_FIRST :
                        break;
                    case QMG_DIRECTION_ASC_NULLS_LAST :
                        break;
                    case QMG_DIRECTION_DESC_NULLS_FIRST :
                        break;
                    case QMG_DIRECTION_DESC_NULLS_LAST :
                        break;
                    default:
                        break;
                }
            
                sPrevDirection = sPreservedOrder->direction;
            }
        }
        else
        {
            // nothing to do 
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmgWindowing::getBucketCnt4Windowing( qcStatement  * aStatement,
                                      qmgWINDOW    * aMyGraph,
                                      qtcOverColumn* aGroupBy,
                                      UInt           aHintBucketCnt,
                                      UInt         * aBucketCnt )
{
/***********************************************************************
 *
 *  Description : qmgGrouping::getBucketCnt4Group과
 *                동일한 함수 인터페이스가 서로 달라서 복사를 했음
 *
 ***********************************************************************/

    qmoColCardInfo   * sColCardInfo;
    qtcOverColumn    * sGroup;
    qtcNode          * sNode;

    SDouble            sCardinality;
    SDouble            sBucketCnt;
    ULong              sBucketCntOutput;

    idBool             sAllColumn;

    IDU_FIT_POINT_FATAL( "qmgWindowing::getBucketCnt4Windowing::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aMyGraph   != NULL );

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

        sBucketCnt = aMyGraph->graph.left->costInfo.outputRecordCnt / 2.0;
        sBucketCnt = ( sBucketCnt < 1 ) ? 1 : sBucketCnt;

        //------------------------------------------
        // group column들의 cardinality 값을 구한다.
        //------------------------------------------

        for ( sGroup = aGroupBy;
              sGroup != NULL;
              sGroup = sGroup->next )
        {
            sNode = sGroup->node;

            if ( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
            {
                // group 대상이 순수한 칼럼인 경우
                sColCardInfo = QC_SHARED_TMPLATE(aStatement)->
                    tableMap[sNode->node.table].
                    from->tableRef->statInfo->colCardInfo;

                sCardinality *= sColCardInfo[sNode->node.column].columnNDV;

                if ( sCardinality > QMC_MEM_HASH_MAX_BUCKET_CNT )
                {
                    // max bucket count보다 클 경우
                    sCardinality = QMC_MEM_HASH_MAX_BUCKET_CNT;
                    break;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                sAllColumn = ID_FALSE;
                break;
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
            /* BUG-48161 */
            if ( sBucketCnt > QCG_GET_BUCKET_COUNT_MAX( aStatement ) )
            {
                // fix BUG-14070
                // bucketCnt가 QMC_MEM_HASH_MAX_BUCKET_CNT(10240000)를 넘으면
                // QMC_MEM_HASH_MAX_BUCKET_CNT 값으로 보정해준다.
                sBucketCnt = QCG_GET_BUCKET_COUNT_MAX( aStatement );
            }
            else
            {
                // nothing to do
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
