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
 * $Id: qmgDistinction.cpp 90409 2021-04-01 04:19:46Z donovan.seo $
 *
 * Description :
 *     Distinction Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgDistinction.h>
#include <qmoCost.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoSelectivity.h>
#include <qcgPlan.h>

extern mtfModule mtfDecrypt;

IDE_RC
qmgDistinction::init( qcStatement * aStatement,
                      qmsQuerySet * aQuerySet,
                      qmgGraph    * aChildGraph,
                      qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgDistinction의 초기화
 *
 * Implementation :
 *    (1) qmgDistinction을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgDIST * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgDistinction::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Distinction Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgDistinction을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgDIST ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_DISTINCTION;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    sMyGraph->graph.myFrom = NULL;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgDistinction::optimize;
    sMyGraph->graph.makePlan = qmgDistinction::makePlan;
    sMyGraph->graph.printGraph = qmgDistinction::printGraph;

    // Disk/Memory 정보 설정
    switch(  aQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            // 중간 결과 Type Hint가 없는 경우, 하위의 Type을 따른다.
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |=
                ( aChildGraph->flag & QMG_GRAPH_TYPE_MASK );
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
    // Distinction Graph 만을 위한 기본 초기화
    //---------------------------------------------------

    sMyGraph->hashBucketCnt = 0;

    // out 설정
    *aGraph = (qmgGraph*)sMyGraph;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgDistinction::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDistinction의 최적화
 *
 * Implementation :
 *    (1) indexable Distinct 최적화
 *    (2) distinction Method 결정 및 Preserved order flag 설정
 *    (3) hash based distinction으로 결정된 경우, hashBucketCnt 설정
 *    (4) 공통 비용 정보 설정
 *
 ***********************************************************************/

    qmgDIST             * sMyGraph;
    qmsTarget           * sTarget;
    qtcNode             * sNode;
    mtcColumn           * sMtcColumn;
    idBool                sSuccess;
    qmoDistinctMethodType sDistinctMethodHint;

    SDouble               sRecordSize;

    SDouble               sTotalCost;
    SDouble               sDiskCost;
    SDouble               sAccessCost;
                          
    SDouble               sSelAccessCost;
    SDouble               sSelDiskCost;
    SDouble               sSelTotalCost;
                          
    idBool                sIsDisk;
    UInt                  sSelectedMethod = 0;

    IDU_FIT_POINT_FATAL( "qmgDistinction::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgDIST*) aGraph;
    sSuccess = ID_FALSE;
    sRecordSize    = 0;
    sSelAccessCost = 0;
    sSelDiskCost   = 0;
    sSelTotalCost  = 0;

    //------------------------------------------
    // Record Size 계산
    //------------------------------------------

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

    //------------------------------------------
    // Distinction Method 결정
    //------------------------------------------
    IDE_TEST( qmg::isDiskTempTable( aGraph, & sIsDisk ) != IDE_SUCCESS );

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
                sDistinctMethodHint = QMO_DISTINCT_METHOD_TYPE_SORT;
                break;
            case 1 :
                sDistinctMethodHint = QMO_DISTINCT_METHOD_TYPE_HASH;
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
        sDistinctMethodHint =
            sMyGraph->graph.myQuerySet->SFWGH->hints->distinctMethodType;
    }

    switch( sDistinctMethodHint )
    {
        case QMO_DISTINCT_METHOD_TYPE_NOT_DEFINED :

            // To Fix PR-12394
            // DISTINCTION 관련 힌트가 없는 경우에만
            // 최적화 Tip을 적용한다.

            //------------------------------------------
            // To Fix PR-12396
            // 비용 계산을 통해 수행 방법을 결정한다.
            //------------------------------------------

            //------------------------------------------
            // Sorting 을 이용한 방식의 비용 계산
            //------------------------------------------
            if( sIsDisk == ID_FALSE )
            {
                sSelTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                             &(aGraph->left->costInfo),
                                                             QMO_COST_DEFAULT_NODE_CNT );
                sSelAccessCost = sSelTotalCost;
                sSelDiskCost   = 0;
            }
            else
            {
                sSelTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                              &(aGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT,
                                                              sRecordSize );
                sSelAccessCost = 0;
                sSelDiskCost   = sSelTotalCost;
            }

            sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
            sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_SORT;

            //------------------------------------------
            // Hashing 을 이용한 방식의 비용계산
            //------------------------------------------
            if( sIsDisk == ID_FALSE )
            {
                sTotalCost = qmoCost::getMemHashTempCost( aStatement->mSysStat,
                                                          &(aGraph->left->costInfo),
                                                          QMO_COST_DEFAULT_NODE_CNT );
                sAccessCost = sTotalCost;
                sDiskCost   = 0;
            }
            else
            {
                sTotalCost = qmoCost::getDiskHashTempCost( aStatement->mSysStat,
                                                           &(aGraph->left->costInfo),
                                                           QMO_COST_DEFAULT_NODE_CNT,
                                                           sRecordSize );
                sAccessCost = 0;
                sDiskCost   = sTotalCost;
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

                    sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
                    sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_HASH;

                    sMyGraph->graph.preservedOrder = NULL;

                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
                }
            }

            //------------------------------------------
            // Preserved Order 를 이용한 방식의 비용계산
            //------------------------------------------

            IDE_TEST( getCostByPrevOrder( aStatement,
                                          sMyGraph,
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

                    //------------------------------------------
                    // Indexable Distinct 최적화 수행
                    //------------------------------------------

                    IDE_TEST( indexableDistinct( aStatement,
                                                 aGraph,
                                                 aGraph->myQuerySet->SFWGH->target,
                                                 & sSuccess )
                              != IDE_SUCCESS );

                    IDE_DASSERT( sSuccess == ID_TRUE );

                    sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
                    sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_SORT;

                    // Preserved Order는 이미 생성됨
                    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;

                    sMyGraph->graph.flag &= ~QMG_DIST_OPT_TIP_MASK;
                    sMyGraph->graph.flag |= QMG_DIST_OPT_TIP_INDEXABLE_DISINCT;
                }
            }

            //------------------------------------------
            // 마무리
            //------------------------------------------

            // Sorting 방식이 선택된 경우 Preserved Order 생성
            if ( ( ( sMyGraph->graph.flag & QMG_SORT_HASH_METHOD_MASK )
                   == QMG_SORT_HASH_METHOD_SORT )
                 &&
                 ( ( sMyGraph->graph.flag & QMG_DIST_OPT_TIP_MASK )
                   != QMG_DIST_OPT_TIP_INDEXABLE_DISINCT ) )
            {
                sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
                sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_SORT;

                // Sort-based Distinction의 경우 Preserved Order가 생긴다.
                IDE_TEST(
                    makeTargetOrder( aStatement,
                                     sMyGraph->graph.myQuerySet->SFWGH->target,
                                     & sMyGraph->graph.preservedOrder )
                    != IDE_SUCCESS );

                sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
                sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
            }
            else
            {
                // 다른 Method가 선택됨
            }
            break;

        case QMO_DISTINCT_METHOD_TYPE_HASH :

            if( sIsDisk == ID_FALSE )
            {
                sSelTotalCost = qmoCost::getMemHashTempCost( aStatement->mSysStat,
                                                             &(aGraph->left->costInfo),
                                                             QMO_COST_DEFAULT_NODE_CNT );
                sSelAccessCost = sSelTotalCost;
                sSelDiskCost   = 0;
            }
            else
            {
                sSelTotalCost = qmoCost::getDiskHashTempCost( aStatement->mSysStat,
                                                              &(aGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT,
                                                              sRecordSize );
                sSelAccessCost = 0;
                sSelDiskCost   = sSelTotalCost;
            }


            sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
            sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_HASH;

            sMyGraph->graph.preservedOrder = NULL;

            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;

            break;
        case QMO_DISTINCT_METHOD_TYPE_SORT :

            if( sIsDisk == ID_FALSE )
            {
                sSelTotalCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                             &(aGraph->left->costInfo),
                                                             QMO_COST_DEFAULT_NODE_CNT );
                sSelAccessCost = sSelTotalCost;
                sSelDiskCost   = 0;
            }
            else
            {
                sSelTotalCost = qmoCost::getDiskSortTempCost( aStatement->mSysStat,
                                                              &(aGraph->left->costInfo),
                                                              QMO_COST_DEFAULT_NODE_CNT,
                                                              sRecordSize );
                sSelAccessCost = 0;
                sSelDiskCost   = sSelTotalCost;
            }

            sMyGraph->graph.flag &= ~QMG_SORT_HASH_METHOD_MASK;
            sMyGraph->graph.flag |= QMG_SORT_HASH_METHOD_SORT;

            // Sort-based Distinction의 경우 Preserved Order가 생긴다.
            IDE_TEST(
                makeTargetOrder( aStatement,
                                 sMyGraph->graph.myQuerySet->SFWGH->target,
                                 & sMyGraph->graph.preservedOrder )
                != IDE_SUCCESS );


            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
            break;
        default :
            IDE_DASSERT( 0 );
            break;
    }

    //------------------------------------------
    // Hash Bucket Count의 설정
    //------------------------------------------

    IDE_TEST(
        qmg::getBucketCntWithTarget(
            aStatement,
            & sMyGraph->graph,
            sMyGraph->graph.myQuerySet->SFWGH->target,
            sMyGraph->graph.myQuerySet->SFWGH->hints->hashBucketCnt,
            & sMyGraph->hashBucketCnt )
        != IDE_SUCCESS );

    if ( ( sMyGraph->graph.flag & QMG_SORT_HASH_METHOD_MASK ) ==
         QMG_SORT_HASH_METHOD_HASH )
    {
        /* BUG-48792 Distinct Cube wrong result */
        if ( ( ( aStatement->mFlag & QC_STMT_VIEW_MASK )
               == QC_STMT_VIEW_TRUE ) &&
             ( sIsDisk == ID_FALSE ) )
        {
            /* PROJ-1353 Rollup, Cube와 같이 사용될 때 */
            if ( ( sMyGraph->graph.left->flag & QMG_GROUPBY_EXTENSION_MASK )
                 == QMG_GROUPBY_EXTENSION_TRUE )
            {
                /* Row를 Value로 쌓기를 Rollup, Cube에 설정한다. */
                sMyGraph->graph.left->flag &= ~QMG_VALUE_TEMP_MASK;
                sMyGraph->graph.left->flag |= QMG_VALUE_TEMP_TRUE;
            }
        }
    }
    else
    {
        sMyGraph->hashBucketCnt = 0;

        /* PROJ-1353 Rollup, Cube와 같이 사용될 때 */
        if ( ( sMyGraph->graph.left->flag & QMG_GROUPBY_EXTENSION_MASK )
             == QMG_GROUPBY_EXTENSION_TRUE )
        {
            /* Row를 Value로 쌓기를 Rollup, Cube에 설정한다. */
            sMyGraph->graph.left->flag &= ~QMG_VALUE_TEMP_MASK;
            sMyGraph->graph.left->flag |= QMG_VALUE_TEMP_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // recordSize = group by column size + aggregation column size
    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = 1;

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt;

    // To Fix BUG-8355
    // outputRecordCnt
    IDE_TEST( qmoSelectivity::setDistinctionOutputCnt(
                     aStatement,
                     sMyGraph->graph.myQuerySet->SFWGH->target,
                     sMyGraph->graph.costInfo.inputRecordCnt,
                     & sMyGraph->graph.costInfo.outputRecordCnt )
                 != IDE_SUCCESS );

    //----------------------------------
    // 해당 Graph의 비용 정보 설정
    //----------------------------------
    sMyGraph->graph.costInfo.myAccessCost = sSelAccessCost;
    sMyGraph->graph.costInfo.myDiskCost   = sSelDiskCost;
    sMyGraph->graph.costInfo.myAllCost    = sSelTotalCost;

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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
qmgDistinction::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgDistinction으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgDistinction로 부터 생성 가능한 Plan
 *
 *         - Sort-based 처리
 *
 *             [GRBY] : distinct option 사용
 *               |
 *           ( [SORT] ) : Indexable Distinct인 경우 생성되지 않음.
 *
 *         - Hash-based 처리
 *
 *             [HSDS]
 *
 ***********************************************************************/

    qmgDIST         * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgDistinction::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgDIST*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    // BUG-38410
    // SCAN parallel flag 를 자식 노드로 물려준다.
    aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

    sMyGraph->graph.myPlan = aParent->myPlan;

    switch( sMyGraph->graph.flag & QMG_SORT_HASH_METHOD_MASK )
    {
        case QMG_SORT_HASH_METHOD_SORT:
        {
            IDE_TEST( makeSortDistinction( aStatement,
                                           sMyGraph )
                      != IDE_SUCCESS );
            break;
        }

        case QMG_SORT_HASH_METHOD_HASH:
        {
            IDE_TEST( makeHashDistinction( aStatement,
                                           sMyGraph )
                      != IDE_SUCCESS );
            break;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmgDistinction::makeSortDistinction( qcStatement * aStatement,
                                     qmgDIST     * aMyGraph )
{
    UInt               sFlag;
    qmnPlan          * sSORT = NULL;
    qmnPlan          * sGRBY = NULL;

    IDU_FIT_POINT_FATAL( "qmgDistinction::makeSortDistinction::__FT__" );

    //-----------------------------------------------------
    //        [GRBY]
    //          |
    //        [SORT]
    //          |
    //        [LEFT]
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init GRBY
    //-----------------------

    IDE_TEST( qmoOneNonPlan::initGRBY( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myPlan ,
                                       &sGRBY )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRBY;

    //-----------------------
    // init SORT
    //-----------------------

    if( (aMyGraph->graph.flag & QMG_DIST_OPT_TIP_MASK) !=
        QMG_DIST_OPT_TIP_INDEXABLE_DISINCT )
    {
        IDE_TEST( qmoOneMtrPlan::initSORT(
                         aStatement ,
                         aMyGraph->graph.myQuerySet ,
                         aMyGraph->graph.myPlan ,
                         &sSORT ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sSORT;
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------------
    // 하위 Plan의 생성
    //---------------------------------------------------

    IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                              &aMyGraph->graph,
                                              aMyGraph->graph.left )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = aMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정
    //---------------------------------------------------
    aMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_DISTINCT;

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make SORT
    //-----------------------

    if( (aMyGraph->graph.flag & QMG_DIST_OPT_TIP_MASK) !=
        QMG_DIST_OPT_TIP_INDEXABLE_DISINCT )
    {
        //----------------------------
        // SORT의 생성
        //----------------------------
        sFlag = 0;
        sFlag &= ~QMO_MAKESORT_METHOD_MASK;
        sFlag |= QMO_MAKESORT_SORT_BASED_DISTINCTION;
        sFlag &= ~QMO_MAKESORT_POSITION_MASK;
        sFlag |= QMO_MAKESORT_POSITION_LEFT;
        sFlag &= ~QMO_MAKESORT_PRESERVED_ORDER_MASK;
        sFlag |= QMO_MAKESORT_PRESERVED_FALSE;

        //저장 매체의 결정
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

    //----------------------------
    // make GRBY
    //----------------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEGRBY_SORT_BASED_METHOD_MASK;
    sFlag |= QMO_MAKEGRBY_SORT_BASED_DISTINCTION;
    
    IDE_TEST( qmoOneNonPlan::makeGRBY( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       aMyGraph->graph.myPlan ,
                                       sGRBY )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sGRBY;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgDistinction::makeHashDistinction( qcStatement * aStatement,
                                     qmgDIST     * aMyGraph )

{
    UInt              sFlag;
    qmnPlan         * sHSDS;

  
    IDU_FIT_POINT_FATAL( "qmgDistinction::makeHashDistinction::__FT__" );

    //-----------------------------------------------------
    //        [HSDS]
    //          |
    //        [LEFT]
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //-----------------------
    // init HSDS
    //-----------------------

    IDE_TEST( qmoOneMtrPlan::initHSDS( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       ID_TRUE,
                                       aMyGraph->graph.myPlan ,
                                       &sHSDS )
                 != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sHSDS;

    //---------------------------------------------------
    // 하위 Plan의 생성
    //---------------------------------------------------

    IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                              &aMyGraph->graph,
                                              aMyGraph->graph.left )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = aMyGraph->graph.left->myPlan;

    //---------------------------------------------------
    // Process 상태 설정
    //---------------------------------------------------
    aMyGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_DISTINCT;

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make HSDS
    //-----------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEHSDS_METHOD_MASK;
    sFlag |= QMO_MAKEHSDS_HASH_BASED_DISTINCTION;

    //저장 매체의 결정
    if( (aMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
        QMG_GRAPH_TYPE_MEMORY )
    {
        sFlag &= ~QMO_MAKEHSDS_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEHSDS_MEMORY_TEMP_TABLE;
    }
    else
    {
        sFlag &= ~QMO_MAKEHSDS_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEHSDS_DISK_TEMP_TABLE;
    }

    IDE_TEST( qmoOneMtrPlan::makeHSDS( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       aMyGraph->hashBucketCnt ,
                                       aMyGraph->graph.myPlan ,
                                       sHSDS )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sHSDS;

    qmg::setPlanInfo( aStatement, sHSDS, &(aMyGraph->graph) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgDistinction::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgDistinction::printGraph::__FT__" );

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
qmgDistinction::makeTargetOrder( qcStatement        * aStatement,
                                 qmsTarget          * aDistTarget,
                                 qmgPreservedOrder ** aNewTargetOrder )
{
/***********************************************************************
 *
 * Description : DISTINCT Target 컬럼을 이용하여
 *               Preserved Order 자료 구조를 구축함.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsTarget           * sTarget;
    qmgPreservedOrder   * sNewOrder;
    qmgPreservedOrder   * sWantOrder;
    qmgPreservedOrder   * sCurOrder;
    qtcNode             * sNode;

    IDU_FIT_POINT_FATAL( "qmgDistinction::makeTargetOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDistTarget != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sWantOrder = NULL;
    sCurOrder = NULL;

    //------------------------------------------
    // Target 컬럼에 대한 Want Order를 생성
    //------------------------------------------

    for ( sTarget = aDistTarget;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        sNode = sTarget->targetColumn;

        // To Fix PR-11568
        // 원래의 Target Column을 획득하여야 한다.
        // ORDER BY indicator등과 함께 존재시 Pass Node가
        // 추가적으로 존재할 수 있으므로 이를 고려하여야 한다.
        // qmgSorting::optimize() 참조
        //
        // BUG-20272
        if ( (sNode->node.module == &qtc::passModule) ||
             (sNode->node.module == &mtfDecrypt) )
        {
            sNode = (qtcNode*) sNode->node.arguments;
        }
        else
        {
            // nothing to do
        }
        
        //------------------------------------------
        // Target 칼럼에 대한 want order를 생성
        //------------------------------------------

        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder),
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

    *aNewTargetOrder = sWantOrder;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
qmgDistinction::indexableDistinct( qcStatement      * aStatement,
                                   qmgGraph         * aGraph,
                                   qmsTarget        * aDistTarget,
                                   idBool           * aIndexableDistinct )
{
/***********************************************************************
 *
 * Description : Indexable Distinct 최적화 가능한 경우, 적용
 *
 * Implementation :
 *    Preserved Order 사용 가능한 경우, 적용
 *
 ***********************************************************************/

    qmgPreservedOrder * sWantOrder;

    IDU_FIT_POINT_FATAL( "qmgDistinction::indexableDistinct::__FT__" );

    // Target을 이용한 Order 자료 구조 생성
    IDE_TEST( makeTargetOrder( aStatement, aDistTarget, & sWantOrder )
                 != IDE_SUCCESS );

    // Preserved Order 사용가능 여부를 검사
    IDE_TEST( qmg::tryPreservedOrder( aStatement,
                                      aGraph,
                                      sWantOrder,
                                      aIndexableDistinct )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgDistinction::getCostByPrevOrder( qcStatement      * aStatement,
                                    qmgDIST          * aDistGraph,
                                    SDouble          * aAccessCost,
                                    SDouble          * aDiskCost,
                                    SDouble          * aTotalCost )
{
/***********************************************************************
 *
 * Description :
 *
 *    Preserved Order 방식을 사용한 Distinction 비용을 계산한다.
 *
 * Implementation :
 *
 *    이미 Child가 원하는 Preserved Order를 가지고 있다면
 *    별도의 비용 없이 Distinction이 가능하다.
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

    IDU_FIT_POINT_FATAL( "qmgDistinction::getCostByPrevOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDistGraph != NULL );

    //------------------------------------------
    // Preserved Order를 사용할 수 있는 지를 검사
    //------------------------------------------

    // Target 칼럼에 대한 want order를 생성
    sWantOrder = NULL;
    IDE_TEST( makeTargetOrder( aStatement,
                                  aDistGraph->graph.myQuerySet->SFWGH->target,
                                  & sWantOrder )
                 != IDE_SUCCESS );

    // preserved order 적용 가능 검사
    IDE_TEST( qmg::checkUsableOrder( aStatement,
                                     sWantOrder,
                                     aDistGraph->graph.left,
                                     & sOrgMethod,
                                     & sSelMethod,
                                     & sUsable )
              != IDE_SUCCESS );

    //------------------------------------------
    // 비용 계산
    //------------------------------------------

    if ( sUsable == ID_TRUE )
    {
        if ( aDistGraph->graph.left->preservedOrder == NULL )
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
            sAccessCost = aDistGraph->graph.left->costInfo.outputRecordCnt *
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
qmgDistinction::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Preserved Order의 direction을 결정한다.
 *                direction이 NOT_DEFINED 일 경우에만 호출하여야 한다.
 *
 *  Implementation :
 *     1. Child graph의 Preserved order와 동일한지 검사
 *     2-1. 동일하다면 direction 복사
 *     2-2. 다르다면 direction을 ascending으로 설정
 *
 ***********************************************************************/

    idBool              sIsSamePrevOrderWithChild;
    qmgPreservedOrder * sPreservedOrder;
    qmgDirectionType    sPrevDirection;

    IDU_FIT_POINT_FATAL( "qmgDistinction::finalizePreservedOrder::__FT__" );

    // BUG-36803 Distinction Graph must have a left child graph
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
