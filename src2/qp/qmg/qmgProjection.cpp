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
 * $Id: qmgProjection.cpp 90832 2021-05-13 23:43:20Z donovan.seo $
 *
 * Description :
 *     Projection Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgProjection.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoSelectivity.h>
#include <qmoCost.h>
#include <qmgSelection.h>
#include <qcgPlan.h>
#include <qmv.h>

IDE_RC
qmgProjection::init( qcStatement * aStatement,
                     qmsQuerySet * aQuerySet,
                     qmgGraph    * aChildGraph,
                     qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgProjection Graph의 초기화
 *
 * Implementation :
 *    (1) qmgProjection을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgPROJ      * sMyGraph;
    qmsParseTree * sParseTree;
    qmsQuerySet  * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgProjection::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aChildGraph != NULL );

    //---------------------------------------------------
    // Projection Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgProjection을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPROJ ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_PROJECTION;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aChildGraph->depInfo );

    if ( ( aChildGraph->flag & QMG_INDEXABLE_MIN_MAX_MASK )
         == QMG_INDEXABLE_MIN_MAX_TRUE )
    {
        sMyGraph->graph.flag &= ~QMG_INDEXABLE_MIN_MAX_MASK;
        sMyGraph->graph.flag |= QMG_INDEXABLE_MIN_MAX_TRUE;
    }
    else
    {
        // nothing to do
    }

    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgProjection::optimize;
    sMyGraph->graph.makePlan = qmgProjection::makePlan;
    sMyGraph->graph.printGraph = qmgProjection::printGraph;

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
    // Projection Graph 만을 위한 초기화
    //---------------------------------------------------

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
    // To Fix BUG-9060
    // 최상위 projection에만 limit을 연결해야함
    // set인 경우, 각 set의 projection에 limit이 설정되는 문제가 있었음
    if ( sParseTree->querySet == aQuerySet )
    {
        // 최상위 projection에만 limit을 연결
        sMyGraph->limit = sParseTree->limit;
        sMyGraph->loopNode = sParseTree->loopNode;
    }
    else
    {
        sMyGraph->limit = NULL;
        sMyGraph->loopNode = NULL;
    }
    
    /* BUG-36580 supported TOP */
    if ( aQuerySet->SFWGH != NULL )
    {
        if( aQuerySet->SFWGH->top != NULL )
        {
            IDE_DASSERT( sMyGraph->limit == NULL );

            sMyGraph->limit = aQuerySet->SFWGH->top;
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
    
    sMyGraph->target = aQuerySet->target;
    sMyGraph->hashBucketCnt = 0;

    // To Fix PR-7989
    sMyGraph->subqueryTipFlag = 0;
    sMyGraph->storeNSearchPred = NULL;
    sMyGraph->hashBucketCnt = 0;

    // out 설정
    *aGraph = (qmgGraph *)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgProjection의 최적화
 *
 * Implementation :
 *    (1) target의 Subquery Graph 생성
 *    (2) Indexable MIN MAX Tip이 적용된 경우, limit 1을 설정한다.
 *    (3) SCAN Limit 최적화
 *    (4) 공통 비용 정보 설정
 *    (5) Preserved Order
 *
 ***********************************************************************/

    qmgPROJ           * sMyGraph;
    qmgGraph          * sChildGraph;
    qtcNode           * sNode;
    qmsLimit          * sLimit;
    idBool              sIsScanLimit;
    qmsQuerySet       * sQuerySet;
    qmsTarget         * sTarget;
    mtcColumn         * sMtcColumn;

    SDouble             sRecordSize;
    SDouble             sOutputRecordCnt;

    // BUG-48779
    qmgChildren       * sCurrChildGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmgProjection::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgPROJ*) aGraph;
    sChildGraph = aGraph->left;
    sRecordSize = 0;

    //---------------------------------------------------
    // target의 Subquery Graph 생성
    //---------------------------------------------------

    for ( sTarget = sMyGraph->target;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        sNode = sTarget->targetColumn;

        IDE_TEST( qmoSubquery::optimize( aStatement,
                                         sNode,
                                         ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );

        sMtcColumn = QTC_TMPL_COLUMN(QC_SHARED_TMPLATE(aStatement), sNode);
        sRecordSize += sMtcColumn->column.size;
    }
    // BUG-36463 sRecordSize 는 0이 되어서는 안된다.
    sRecordSize = IDL_MAX( sRecordSize, 1 );

    //---------------------------------------------------
    // SCAN Limit 최적화
    //---------------------------------------------------

    sIsScanLimit = ID_FALSE;
    if ( sMyGraph->limit != NULL )
    {
        if ( sChildGraph->type == QMG_SELECTION )
        {
            canPushDownLimitIntoScan( sChildGraph,
                                      &sIsScanLimit );
        }
        else if ( sChildGraph->type == QMG_PARTITION )
        {
            /* BUG-48779
             * 아래와 같은 형태로 PROJECTION과 SCAN사이에 PARTITION-COORDINATOR가 낀 경우
             * 제한적으로 LIMIT을 SCAN으로 내려주도록 한다.
             *
             * PROJECT
             *  PARTITION-COODINATOR
             *   SCAN ( PARTITION : P3 )
             *   SCAN ( PARTITION : P2 )
             *   SCAN ( PARTITION : P1 )
             * ...
             */
            if ( sMyGraph->limit->start.hostBindNode == NULL )
            {
                // limit is constant value
                if ( sMyGraph->limit->start.constant == ID_ULONG(1) )
                {
                    // limit start value is 1
                    for ( sCurrChildGraph = sChildGraph->children;
                          sCurrChildGraph != NULL;
                          sCurrChildGraph = sCurrChildGraph->next )
                    {
                        canPushDownLimitIntoScan( sCurrChildGraph->childGraph,
                                                  &sIsScanLimit );

                        if ( sIsScanLimit == ID_FALSE )
                        {
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    // start value 가 1 이 아닌 경우에는 scan limit으로 내려주지 않는다.
                    // Nothing to do.
                }
            }
            else
            {
                // limit의 value가 bind param 일 경우에는 scan limit으로 내려주지 않는다.
                // Nothing to do
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
        if ( sChildGraph->type == QMG_SELECTION )
        {
            ((qmgSELT *)sChildGraph)->limit = sMyGraph->limit;
        }
        else
        {
            for ( sCurrChildGraph = sChildGraph->children;
                  sCurrChildGraph != NULL;
                  sCurrChildGraph = sCurrChildGraph->next )
            {
                ((qmgSELT *)(sCurrChildGraph->childGraph))->limit = sMyGraph->limit;
            }
        }

        // To Fix BUG-9560
        IDE_TEST(
            QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmsLimit ),
                                           ( void**) & sLimit )
            != IDE_SUCCESS );

        // fix BUG-13482
        // parse tree의 limit정보를 현재 가지며,
        // PROJ 노드 생성시,
        // 하위 SCAN 노드에서 SCAN Limit 적용이 확정되면,
        // PROJ 노드의 limit start를 1로 변경한다.

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
    // Indexable Min Max Tip이 적용된 경우, Limit 1 설정
    //---------------------------------------------------

    if ( ( sMyGraph->graph.flag & QMG_INDEXABLE_MIN_MAX_MASK )
         == QMG_INDEXABLE_MIN_MAX_TRUE )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmsLimit ),
                                                 (void**) & sLimit )
                  != IDE_SUCCESS );

        qmsLimitI::setStartValue( sLimit, 1 );
        qmsLimitI::setCountValue( sLimit, 1 );
        SET_EMPTY_POSITION( sLimit->limitPos );
        sMyGraph->limit = sLimit;
    }
    else
    {
        // nothing to do
    }

    //---------------------------------------------------
    // hash bucket count 설정
    //---------------------------------------------------

    for ( sQuerySet = sMyGraph->graph.myQuerySet;
          sQuerySet->left != NULL;
          sQuerySet = sQuerySet->left ) ;

    IDE_TEST(
        qmg::getBucketCntWithTarget( aStatement,
                                     & sMyGraph->graph,
                                     sMyGraph->target,
                                     sQuerySet->SFWGH->hints->hashBucketCnt,
                                     & sMyGraph->hashBucketCnt )
        != IDE_SUCCESS );

    //---------------------------------------------------
    // 공통 비용 정보 설정
    //---------------------------------------------------

    // recordSize
    sMyGraph->graph.costInfo.recordSize = sRecordSize;

    // selectivity
    IDE_TEST( qmoSelectivity::setProjectionSelectivity(
                  sMyGraph->limit,
                  sChildGraph->costInfo.outputRecordCnt,
                  & sMyGraph->graph.costInfo.selectivity )
              != IDE_SUCCESS );

    // inputRecordCnt
    sMyGraph->graph.costInfo.inputRecordCnt =
        sChildGraph->costInfo.outputRecordCnt;

    // outputRecordCnt
    sOutputRecordCnt = sChildGraph->costInfo.outputRecordCnt *
        sMyGraph->graph.costInfo.selectivity;
    sOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;
    sMyGraph->graph.costInfo.outputRecordCnt = sOutputRecordCnt;

    sMyGraph->graph.costInfo.myAccessCost = qmoCost::getTargetCost( aStatement->mSysStat,
                                                                    sMyGraph->target,
                                                                    sOutputRecordCnt );

    // myCost
    sMyGraph->graph.costInfo.myDiskCost = 0;
    sMyGraph->graph.costInfo.myAllCost = sMyGraph->graph.costInfo.myAccessCost;

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
    // Preserved Order
    //    하위의 Preserved Order가 존재하는 경우,
    //    Preserved Order의 각 칼럼에 대하여 해당 칼럼이
    //    target에 존재할 경우에만  Preserved Order가 존재
    //---------------------------------------------------

    if ( sChildGraph->preservedOrder != NULL )
    {
        IDE_TEST( makePreservedOrder( aStatement,
                                      sMyGraph->target,
                                      sMyGraph->graph.left->preservedOrder,
                                      & sMyGraph->graph.preservedOrder )
                  != IDE_SUCCESS );
        
        if ( sMyGraph->graph.preservedOrder != NULL )
        {
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_DEFINED_FIXED;
        }
        else
        {
            sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
            sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;
        }

        //---------------------------------------------------
        // BUG-32258
        // limit 절이 존재하고,
        // child의 preserved order direction이 not defined 이면
        // direction을 설정하도록 시킨다.
        //---------------------------------------------------

        if ( sMyGraph->limit != NULL )
        {
            if ( sMyGraph->graph.preservedOrder != NULL )
            {
                IDE_TEST( qmg::finalizePreservedOrder( &sMyGraph->graph )
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
    }
    else
    {
        sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
        sMyGraph->graph.flag |=
            ( sChildGraph->flag & QMG_PRESERVED_ORDER_MASK );
    }

    sMyGraph->graph.flag &= ~QMG_PARALLEL_EXEC_MASK;
    sMyGraph->graph.flag |= (sChildGraph->flag & QMG_PARALLEL_EXEC_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgProjection::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgProjection으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *    - qmgProjection으로 생성가능한 Plan
 *
 *        - Top 인 경우
 *
 *           [PROJ]
 *
 *        - View Materialization 이 적용된 경우
 *
 *           [VMTR]
 *             |
 *           [VIEW]
 *             |
 *           [PROJ]
 *
 *        - Transform NJ가 적용된 경우
 *
 *           [PROJ]
 *
 *        - Store And Search 최적화가 적용된 경우
 *
 *           1.  Hash 저장 방식인 경우
 *
 *
 *               [PROJ]
 *                 |
 *               [HASH] : Filter가 포함됨, NOTNULLCHECK option 포함.
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 *           2.  LMST 저장 방식인 경우
 *
 *               [PROJ]
 *                 |
 *               [LMST] : Filter가 존재하지 않음
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 *           3.  STORE Only인 경우
 *
 *               [PROJ]
 *                 |
 *               [SORT]
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 *        - SUBQUERY IN KEYRANGE 최적화가 적용된 경우
 *
 *               [PROJ]
 *                 |
 *               [HSDS]
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 *        - SUBQUERY KEYRANGE 최적화가 적용된 경우
 *
 *               [PROJ]
 *                 |
 *               [SORT] : 저장만 함.
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 ***********************************************************************/

    qmgPROJ         * sMyGraph;
    UInt              sPROJFlag         = 0;
    qmnPlan         * sPROJ             = NULL;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgPROJ*) aGraph;

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
    if (aParent != NULL)
    {
        aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
        aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

        aGraph->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
        aGraph->flag |= (aParent->flag & QMG_PLAN_EXEC_REPEATED_MASK);
    }
    else
    {
        /*
         * QMG_PARALLEL_IMPOSSIBLE_MASK flag 는 고치지 않는다.
         * (상위에서 setting 한것으로 쓴다)
         * parent 가 없다고 항상 최상위인건 아니다.
         */

        /* BUG-38410
         * 최상위 plan 이므로 기본값을 세팅한다.
         */
        aGraph->flag &= ~QMG_PLAN_EXEC_REPEATED_MASK;
        aGraph->flag |= QMG_PLAN_EXEC_REPEATED_FALSE;
    }

    // BUG-38410
    // SCAN parallel flag 를 자식 노드로 물려준다.
    aGraph->left->flag  |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);

    // INDEXABLE Min-Max의 설정
    if( (sMyGraph->graph.flag & QMG_INDEXABLE_MIN_MAX_MASK ) ==
        QMG_INDEXABLE_MIN_MAX_TRUE )
    {
        sPROJFlag &= ~QMO_MAKEPROJ_INDEXABLE_MINMAX_MASK;
        sPROJFlag |= QMO_MAKEPROJ_INDEXABLE_MINMAX_TRUE;
    }
    else
    {
        sPROJFlag &= ~QMO_MAKEPROJ_INDEXABLE_MINMAX_MASK;
        sPROJFlag |= QMO_MAKEPROJ_INDEXABLE_MINMAX_FALSE;
    }

    if( (sMyGraph->graph.flag & QMG_PROJ_COMMUNICATION_TOP_PROJ_MASK) ==
        QMG_PROJ_COMMUNICATION_TOP_PROJ_TRUE )
    {
        // PROJ-2462 Top result Cache인 경우 ViewMTR을 생성후에
        // Projection을 생성된다.
        if ( ( sMyGraph->graph.flag & QMG_PROJ_TOP_RESULT_CACHE_MASK )
             == QMG_PROJ_TOP_RESULT_CACHE_TRUE )
        {
            IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                               sMyGraph->graph.myQuerySet,
                                               sMyGraph->graph.myPlan,
                                               &sPROJ )
                      != IDE_SUCCESS );
            sMyGraph->graph.myPlan = sPROJ;

            sPROJFlag &= ~QMO_MAKEPROJ_TOP_MASK;
            sPROJFlag |= QMO_MAKEPROJ_TOP_FALSE;

            /* ViewMtr을 생성하도록 Mask 설정 */
            sMyGraph->graph.flag &= ~QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK;
            sMyGraph->graph.flag |= QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE;
            /* VMTR로 전환된 Project Graph의 flag에 Memory 설정 */
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;

            IDE_TEST( makePlan4ViewMtr( aStatement, sMyGraph, sPROJFlag )
                      != IDE_SUCCESS );

            sPROJFlag &= ~QMO_MAKEPROJ_TOP_MASK;
            sPROJFlag |= QMO_MAKEPROJ_TOP_TRUE;

            sPROJFlag &= ~QMO_MAKEPROJ_QUERYSET_TOP_MASK;
            sPROJFlag |= QMO_MAKEPROJ_QUERYSET_TOP_TRUE;

            sPROJFlag &= ~QMO_MAKEPROJ_TOP_RESULT_CACHE_MASK;
            sPROJFlag |= QMO_MAKEPROJ_TOP_RESULT_CACHE_TRUE;

            IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                               sMyGraph->graph.myQuerySet ,
                                               sPROJFlag ,
                                               sMyGraph->limit ,
                                               sMyGraph->loopNode ,
                                               sMyGraph->graph.myPlan ,
                                               sPROJ )
                      != IDE_SUCCESS);
            sMyGraph->graph.myPlan = sPROJ;

            qmg::setPlanInfo( aStatement, sPROJ, &(sMyGraph->graph) );
        }
        else
        {
            //-----------------------------------------------------
            // Top인 경우
            //-----------------------------------------------------

            sPROJFlag &= ~QMO_MAKEPROJ_TOP_MASK;
            sPROJFlag |= QMO_MAKEPROJ_TOP_TRUE;

            sPROJFlag &= ~QMO_MAKEPROJ_QUERYSET_TOP_MASK;
            sPROJFlag |= QMO_MAKEPROJ_QUERYSET_TOP_TRUE;

            //----------------------------
            // PROJ의 생성
            //----------------------------

            IDE_TEST( makePlan4Proj( aStatement,
                                     sMyGraph,
                                     sPROJFlag ) != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------------------
        // Top이 아닌 경우
        //-----------------------------------------------------

        if( aParent != NULL )
        {
            sMyGraph->graph.myPlan = aParent->myPlan;
        }
        else
        {
            sMyGraph->graph.myPlan = NULL;
        }

        // non-TOP [PROJ] 인 경우의 flag setting
        sPROJFlag &= ~QMO_MAKEPROJ_TOP_MASK;
        sPROJFlag |= QMO_MAKEPROJ_TOP_FALSE;

        // View Materialization이 적용 여부
        if( ( ( sMyGraph->graph.flag & QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK) ==
              QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE ) ||
            (  ( sMyGraph->graph.flag & QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK) ==
               QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE ) )
        {
            IDE_TEST( makePlan4ViewMtr( aStatement, sMyGraph, sPROJFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            //-----------------------------------------------------
            // View Optimization Type이 View Materialization이 아닌경우
            //-----------------------------------------------------

            // Subquery Tip의 적용
            switch( sMyGraph->subqueryTipFlag & QMG_PROJ_SUBQUERY_TIP_MASK )
            {
                case QMG_PROJ_SUBQUERY_TIP_NONE:
                    //-----------------------------------------------------
                    // subquery tip이 없는 경우
                    //-----------------------------------------------------
                case QMG_PROJ_SUBQUERY_TIP_TRANSFORMNJ:
                    //-----------------------------------------------------
                    // Transform NJ가 적용된 경우
                    //-----------------------------------------------------

                    //----------------------------
                    // [PROJ]의 생성
                    //----------------------------

                    IDE_TEST( makePlan4Proj( aStatement,
                                             sMyGraph,
                                             sPROJFlag ) != IDE_SUCCESS );

                    break;

                case QMG_PROJ_SUBQUERY_TIP_STORENSEARCH:
                    //-----------------------------------------------------
                    // Store And Search 최적화가 적용된 경우
                    //-----------------------------------------------------

                    IDE_TEST ( makePlan4StoreSearch( aStatement,
                                                     sMyGraph,
                                                     sPROJFlag )
                               != IDE_SUCCESS );

                    break;

                case QMG_PROJ_SUBQUERY_TIP_IN_KEYRANGE:

                    //-----------------------------------------------------
                    // Subquery IN Keyrange 최적화가 적용된 경우
                    //-----------------------------------------------------

                    IDE_TEST ( makePlan4InKeyRange( aStatement,
                                                    sMyGraph,
                                                    sPROJFlag )
                               != IDE_SUCCESS );

                    break;

                case QMG_PROJ_SUBQUERY_TIP_KEYRANGE:

                    //-----------------------------------------------------
                    // Subquery Keyrange 최적화가 적용된 경우
                    //-----------------------------------------------------

                    IDE_TEST ( makePlan4KeyRange( aStatement,
                                                  sMyGraph,
                                                  sPROJFlag )
                               != IDE_SUCCESS );

                    break;

                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::printGraph( qcStatement  * aStatement,
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

    UInt        i;
    qmsTarget * sTarget;
    qmgPROJ   * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgProjection::printGraph::__FT__" );

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

    /* BUG-48969 target subquery graph display */
    sMyGraph = (qmgPROJ *)aGraph;
    for ( sTarget = sMyGraph->target; sTarget != NULL ; sTarget = sTarget->next )
    {
        IDE_TEST( qmg::printSubquery( aStatement,
                                      sTarget->targetColumn,
                                      aDepth,
                                      aString )
                  != IDE_SUCCESS );
    }

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


IDE_RC
qmgProjection::makePreservedOrder( qcStatement        * aStatement,
                                   qmsTarget          * aTarget,
                                   qmgPreservedOrder  * aChildPreservedOrder,
                                   qmgPreservedOrder ** aMyPreservedOrder )
{
/***********************************************************************
 *
 * Description : qmgProjection의 preserved order의 생성
 *
 * Implementation :
 *
 *    (1) Preserved Order의 각 칼럼에 대하여 해당 칼럼이 target에 존재하면
 *        preserved order 존재,
 *        순서대로 preserved order를 생성하다가 하나라도 만족하지 않으면 중단
 *        ex ) target : i2, i1, i1 + i3
 *             child preserved order : i1, i2, i3
 *             my preserved order : i1, i2
 *
 *    (2) limit 절이 존재하고 preserved order의 direction이 not defined 인 경우,
 *        selection의 index 생성 방향에 맞게 direction을 설정하도록 한다.
 *        ( BUG-32258 ) 
 *        ex ) SELECT t1.i1
 *             FROM t1, ( SELECT i2 FROM t2 WHERE i1 > 0 limit 3 ) v1
 *             WHERE t1.i1 = v1.i1;
 ************************************************************************/
    
    qmgPreservedOrder * sPreservedOrder;
    qmgPreservedOrder * sFirstPresOrder;
    qmgPreservedOrder * sCurPresOrder;
    qmgPreservedOrder * sCur;
    qmsTarget         * sTarget;
    qtcNode           * sTargetNode;
    idBool              sExistCol;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePreservedOrder::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildPreservedOrder != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sFirstPresOrder   = NULL;
    sCurPresOrder     = NULL;
    sExistCol         = ID_FALSE;

    //---------------------------------------------------
    // Child Graph의 Preserved Order를 이용한 Order생성
    //---------------------------------------------------

    for ( sCur = aChildPreservedOrder; sCur != NULL; sCur = sCur->next )
    {
        sExistCol = ID_FALSE;
        for ( sTarget = aTarget; sTarget != NULL; sTarget = sTarget->next )
        {
            // BUG-38193 target의 pass node 를 고려해야 합니다.
            if ( sTarget->targetColumn->node.module == &qtc::passModule )
            {
                sTargetNode = (qtcNode*)(sTarget->targetColumn->node.arguments);
            }
            else
            {
                sTargetNode = sTarget->targetColumn;
            }

            if ( QTC_IS_COLUMN( aStatement, sTargetNode )
                 == ID_TRUE )
            {
                // target 칼럼이 순수한 칼럼인 경우
                if ( ( sCur->table == sTargetNode->node.table) &&
                     ( sCur->column == sTargetNode->node.column ) )
                {
                    // 동일 칼럼인 경우
                    IDE_TEST(
                        QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgPreservedOrder ),
                                                       (void**)&sPreservedOrder )
                        != IDE_SUCCESS );

                    sPreservedOrder->table = sCur->table;
                    sPreservedOrder->column = sCur->column;
                    sPreservedOrder->direction = sCur->direction;
                    sPreservedOrder->next = NULL;

                    if ( sFirstPresOrder == NULL )
                    {
                        sFirstPresOrder = sCurPresOrder = sPreservedOrder;
                    }
                    else
                    {
                        sCurPresOrder->next = sPreservedOrder;
                        sCurPresOrder = sCurPresOrder->next;
                    }

                    sExistCol = ID_TRUE;
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

        if ( sExistCol == ID_FALSE )
        {
            // preserved order 생성 중단
            break;
        }
        else
        {
            // 다음 preserved order로 계속 진행
            // nothing to do
        }
    }

    *aMyPreservedOrder = sFirstPresOrder;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::makeChildPlan( qcStatement * aStatement,
                              qmgPROJ     * aGraph )
{

    IDU_FIT_POINT_FATAL( "qmgProjection::makeChildPlan::__FT__" );

    IDE_TEST( aGraph->graph.left->makePlan( aStatement ,
                                            &aGraph->graph,
                                            aGraph->graph.left )
              != IDE_SUCCESS);

    // fix BUG-13482
    // SCAN Limit 최적화 적용에 따른 PROJ plan의 start value 결정유무
    if( aGraph->graph.left->type == QMG_SELECTION )
    {
        // projection 하위가 SCAN이고,
        // SCAN limit 최적화가 적용된 경우이면,
        // PROJ의 limit start value를 1로 조정한다.
        if( ((qmgSELT*)(aGraph->graph.left))->limit != NULL )
        {
            qmsLimitI::setStartValue( aGraph->limit, 1 );
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

    //---------------------------------------------------
    // Process 상태 설정
    //---------------------------------------------------
    aGraph->graph.myQuerySet->processPhase = QMS_MAKEPLAN_PROJECT;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::makePlan4Proj( qcStatement * aStatement,
                              qmgPROJ     * aGraph,
                              UInt          aPROJFlag )
{
    qmnPlan  * sPROJ        = NULL;
    qmnPlan  * sDLAY        = NULL;
    UInt       sDelayedExec = 0;
    idBool     sFound       = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePlan4Proj::__FT__" );

    //----------------------------
    // PROJ의 생성
    //----------------------------
    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       NULL,
                                       &sPROJ )
              != IDE_SUCCESS);
    aGraph->graph.myPlan = sPROJ;

    // BUG-43493 delay operator를 추가해 execution time을 줄인다.
    if ( ( aPROJFlag & QMO_MAKEPROJ_TOP_MASK ) == QMO_MAKEPROJ_TOP_TRUE )
    {
        // hint를 사용하는 경우 모든 materialize가능한 operator에 대해 delay를 생성한다.
        if ( aGraph->graph.myQuerySet->SFWGH != NULL )
        {
            switch ( aGraph->graph.myQuerySet->SFWGH->hints->delayedExec )
            {
                case QMS_DELAY_NONE:
                    qcgPlan::registerPlanProperty( aStatement,
                                                   PLAN_PROPERTY_OPTIMIZER_DELAYED_EXECUTION );
                    sDelayedExec = QCU_OPTIMIZER_DELAYED_EXECUTION;
                    break;

                case QMS_DELAY_TRUE:
                    sDelayedExec = 1;
                    break;

                case QMS_DELAY_FALSE:
                    sDelayedExec = 0;
                    break;

                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_OPTIMIZER_DELAYED_EXECUTION );
            sDelayedExec = QCU_OPTIMIZER_DELAYED_EXECUTION;
        }
        
        if ( sDelayedExec == 1 )
        {
            IDE_TEST( qmg::lookUpMaterializeGraph( &aGraph->graph, &sFound )
                      != IDE_SUCCESS );
            
            if ( sFound == ID_TRUE )
            {
                IDE_TEST( qmoOneNonPlan::initDLAY( aStatement ,
                                                   aGraph->graph.myPlan ,
                                                   &sDLAY )
                          != IDE_SUCCESS );
                // 미리 연결하면 아래 makeChildPlan시 parent가 QMN_PROJ이 아니어서
                // 여러가지로 오동작함
                //aGraph->graph.myPlan = sDLAY;
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
    
    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( makeChildPlan( aStatement, aGraph ) != IDE_SUCCESS );

    //----------------------------
    // PROJ의 생성
    //----------------------------

    if ( sDLAY != NULL )
    {
        IDE_TEST( qmoOneNonPlan::makeDLAY( aStatement ,
                                           aGraph->graph.myQuerySet ,
                                           aGraph->graph.left->myPlan ,
                                           sDLAY )
                  != IDE_SUCCESS );
        aGraph->graph.left->myPlan = sDLAY;

        qmg::setPlanInfo( aStatement, sDLAY, aGraph->graph.left );
    }
    else
    {
        // Nothing to do.
    }
    
    aPROJFlag &= ~QMO_MAKEPROJ_QUERYSET_TOP_MASK;
    aPROJFlag |= QMO_MAKEPROJ_QUERYSET_TOP_TRUE;

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag ,
                                       aGraph->limit ,
                                       aGraph->loopNode ,
                                       aGraph->graph.left->myPlan ,
                                       sPROJ )
              != IDE_SUCCESS);
    aGraph->graph.myPlan = sPROJ;

    qmg::setPlanInfo( aStatement, sPROJ, &(aGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::makePlan4ViewMtr( qcStatement    * aStatement,
                                 qmgPROJ        * aGraph,
                                 UInt             aPROJFlag )
{
/***********************************************************************
 *
 * Description : qmgProjection으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *
 *        - View Materialization 이 적용된 경우
 *
 *           [VMTR]
 *             |
 *           [VIEW]
 *             |
 *           [PROJ]
 *
 ***********************************************************************/

    qmnPlan         * sPROJ = NULL;
    qmnPlan         * sVIEW = NULL;
    qmnPlan         * sVMTR = NULL;
    qmnPlan         * sCMTR = NULL;
    UInt              sFlag;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePlan4ViewMtr::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //-----------------------------------------------------
    // - View Optimization Type이 View Materialization인 경우
    //   [VMTR] - [VIEW] - [PROJ]의 생성
    // - View Optimization Type이 ConnectBy Materialization인 경우
    //   [CMTR] - [VIEW] - [PROJ]의 생성
    //-----------------------------------------------------

    if ( ( aGraph->graph.flag & QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK ) ==
         QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE )
    {
        IDE_TEST( qmoOneMtrPlan::initCMTR( aStatement,
                                           aGraph->graph.myQuerySet,
                                           &sCMTR )
                  != IDE_SUCCESS);
        aGraph->graph.myPlan = sCMTR;
    }
    else
    {
        IDE_TEST( qmoOneMtrPlan::initVMTR( aStatement,
                                           aGraph->graph.myQuerySet,
                                           aGraph->graph.myPlan,
                                           &sVMTR )
                  != IDE_SUCCESS );
        aGraph->graph.myPlan = sVMTR;
    }

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement,
                                       aGraph->graph.myQuerySet,
                                       aGraph->graph.myPlan,
                                       &sVIEW )
              != IDE_SUCCESS );
    aGraph->graph.myPlan = sVIEW;

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet,
                                       aGraph->graph.myPlan,
                                       &sPROJ )
              != IDE_SUCCESS );
    aGraph->graph.myPlan = sPROJ;

    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( makeChildPlan( aStatement, aGraph ) != IDE_SUCCESS );

    //----------------------------
    // [PROJ]의 생성
    //----------------------------

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag ,
                                       aGraph->limit ,
                                       aGraph->loopNode ,
                                       aGraph->graph.left->myPlan ,
                                       sPROJ )
              != IDE_SUCCESS);
    aGraph->graph.myPlan = sPROJ;

    qmg::setPlanInfo( aStatement, sPROJ, &(aGraph->graph) );

    //----------------------------
    // [VIEW]의 생성
    //----------------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_PROJECTION;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       NULL ,
                                       sFlag ,
                                       aGraph->graph.myPlan ,
                                       sVIEW )
              != IDE_SUCCESS);
    aGraph->graph.myPlan = sVIEW;



    //----------------------------
    // [VMTR/CMTR]의 생성
    //----------------------------

    sFlag = 0;

    if( ( aGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
        QMG_GRAPH_TYPE_MEMORY )
    {
        sFlag &= ~QMO_MAKEVMTR_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEVMTR_MEMORY_TEMP_TABLE;
    }
    else
    {
        sFlag &= ~QMO_MAKEVMTR_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKEVMTR_DISK_TEMP_TABLE;
    }

    // BUG-44288 recursive with 기준 쿼리에 TEMP_TBS_DISK + TOP QUERY에 동일한
    // recursive query name join 사용 되는 경우
    if ( aGraph->graph.myQuerySet->setOp == QMS_UNION_ALL )
    {
        if ( ( ( aGraph->graph.myQuerySet->left->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
               == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) &&
             ( ( aGraph->graph.myQuerySet->right->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
               == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) )
        {
            sFlag &= ~QMO_MAKEVMTR_TEMP_TABLE_MASK;
            sFlag |= QMO_MAKEVMTR_MEMORY_TEMP_TABLE;
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

    if ( ( aGraph->graph.flag & QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK ) ==
         QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE )
    {
        IDE_TEST( qmoOneMtrPlan::makeCMTR( aStatement,
                                           aGraph->graph.myQuerySet,
                                           sFlag,
                                           aGraph->graph.myPlan,
                                           sCMTR )
                  != IDE_SUCCESS);
        aGraph->graph.myPlan = sCMTR;

        qmg::setPlanInfo( aStatement, sCMTR, &(aGraph->graph) );
    }
    else
    {
        IDE_DASSERT( ( aGraph->graph.flag & QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK ) ==
                     QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE );

        if ( ( aGraph->graph.flag & QMG_PROJ_TOP_RESULT_CACHE_MASK )
             == QMG_PROJ_TOP_RESULT_CACHE_TRUE )
        {
            sFlag &= ~QMO_MAKEVMTR_TOP_RESULT_CACHE_MASK;
            sFlag |= QMO_MAKEVMTR_TOP_RESULT_CACHE_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( qmoOneMtrPlan::makeVMTR( aStatement ,
                                           aGraph->graph.myQuerySet ,
                                           sFlag ,
                                           aGraph->graph.myPlan,
                                           sVMTR )
                  != IDE_SUCCESS);
        aGraph->graph.myPlan = sVMTR;

        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgProjection::makePlan4StoreSearch( qcStatement    * aStatement,
                                     qmgPROJ        * aGraph,
                                     UInt             aPROJFlag )
{
/***********************************************************************
 *
 * Description : qmgProjection으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *
 *        - Store And Search 최적화가 적용된 경우
 *
 *           1.  Hash 저장 방식인 경우
 *
 *
 *               [PROJ]
 *                 |
 *               [HASH] : Filter가 포함됨, NOTNULLCHECK option 포함.
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 *           2.  LMST 저장 방식인 경우
 *
 *               [PROJ]
 *                 |
 *               [LMST] : Filter가 존재하지 않음
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 *           3.  STORE Only인 경우
 *
 *               [PROJ]
 *                 |
 *               [SORT]
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 ***********************************************************************/

    qmnPlan         * sVIEW;
    qmnPlan         * sInnerPROJ;
    qmnPlan         * sOuterPROJ;

    qmnPlan         * sPlan = NULL;
    UInt              sFlag;
    qtcNode         * sHashFilter;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePlan4StoreSearch::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //----------------------------
    // Top-down 생성
    //----------------------------

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet,
                                       NULL,
                                       &sOuterPROJ )
              != IDE_SUCCESS );
    aGraph->graph.myPlan = sOuterPROJ;

    switch( aGraph->subqueryTipFlag & QMG_PROJ_SUBQUERY_STORENSEARCH_MASK )
    {
        case QMG_PROJ_SUBQUERY_STORENSEARCH_HASH:
            IDE_TEST( qmoOneMtrPlan::initHASH( aStatement,
                                               aGraph->graph.myQuerySet ,
                                               aGraph->graph.myPlan,
                                               &sPlan ) != IDE_SUCCESS);
            aGraph->graph.myPlan = sPlan;

            break;

        case QMG_PROJ_SUBQUERY_STORENSEARCH_LMST:
            IDE_TEST( qmoOneMtrPlan::initLMST( aStatement ,
                                               aGraph->graph.myQuerySet ,
                                               aGraph->graph.myPlan ,
                                               &sPlan ) != IDE_SUCCESS);
            aGraph->graph.myPlan = sPlan;

            break;

        case QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY:
            IDE_TEST( qmoOneMtrPlan::initSORT( aStatement ,
                                               aGraph->graph.myQuerySet ,
                                               aGraph->graph.myPlan ,
                                               &sPlan ) != IDE_SUCCESS);
            aGraph->graph.myPlan = sPlan;

            break;

        default:
            break;
    }

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement,
                                       aGraph->graph.myQuerySet,
                                       aGraph->graph.myPlan,
                                       &sVIEW )
              != IDE_SUCCESS );
    aGraph->graph.myPlan = sVIEW;

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet,
                                       aGraph->graph.myPlan,
                                       &sInnerPROJ )
              != IDE_SUCCESS );
    aGraph->graph.myPlan = sInnerPROJ;

    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( makeChildPlan( aStatement, aGraph ) != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //----------------------------
    // [PROJ]의 생성
    // limit를 고려한다.
    //----------------------------

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag,
                                       aGraph->limit,
                                       aGraph->loopNode ,
                                       aGraph->graph.left->myPlan,
                                       sInnerPROJ ) != IDE_SUCCESS);

    aGraph->graph.myPlan = sInnerPROJ;

    qmg::setPlanInfo( aStatement, sInnerPROJ, &(aGraph->graph) );

    //----------------------------
    // [VIEW]의 생성
    //----------------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_PROJECTION;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       NULL ,
                                       sFlag ,
                                       aGraph->graph.myPlan ,
                                       sVIEW ) != IDE_SUCCESS);

    aGraph->graph.myPlan = sVIEW;



    //-----------------------------------------------------
    // 저장 방식의 선택
    //-----------------------------------------------------
    switch( aGraph->subqueryTipFlag & QMG_PROJ_SUBQUERY_STORENSEARCH_MASK )
    {
        case QMG_PROJ_SUBQUERY_STORENSEARCH_HASH:

            //-----------------------------
            // Hash 저장 방식인 경우
            //-----------------------------

            if( aGraph->storeNSearchPred != NULL )
            {
                // To Fix PR-8019
                // Key Range 생성을 위해서는 DNF형태로 변환하여야 한다.
                IDE_TEST(
                    qmoNormalForm::normalizeDNF( aStatement ,
                                                 aGraph->storeNSearchPred ,
                                                 & sHashFilter ) != IDE_SUCCESS );

                IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                                   aGraph->graph.myQuerySet ,
                                                   sHashFilter ) != IDE_SUCCESS );
            }
            else
            {
                sHashFilter = NULL;
            }

            //----------------------------
            // [HASH]의 생성
            // Temp Table의 개수는 1개
            //----------------------------

            sFlag = 0;
            sFlag &= ~QMO_MAKEHASH_METHOD_MASK;
            sFlag |= QMO_MAKEHASH_STORE_AND_SEARCH;

            //----------------------------
            // Temp Table의 생성 종류 결정
            // - Graph는 Hint에 따라 Table Type (memory or disk) 을 갖고 있다.
            //   Plan 역시 Table Type정보를 포함하고 있으며,
            //   이는 하위 정보를 OR하여 가지게 된다.
            //   (disk가 우선순위) 따라서, 이정보에 Graph의
            //   정보를 표현해주도록 한다.
            //----------------------------

            if( ( aGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
                QMG_GRAPH_TYPE_MEMORY )
            {
                sFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;
                sFlag |= QMO_MAKEHASH_MEMORY_TEMP_TABLE;
            }
            else
            {
                sFlag &= ~QMO_MAKEHASH_TEMP_TABLE_MASK;
                sFlag |= QMO_MAKEHASH_DISK_TEMP_TABLE;
            }

            // Not null check에 대한 flag
            // fix BUG-8936
            if ( ( aGraph->subqueryTipFlag &
                   QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_MASK) ==
                 QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_TRUE )
            {
                sFlag &= ~QMO_MAKEHASH_NOTNULLCHECK_MASK;
                sFlag |= QMO_MAKEHASH_NOTNULLCHECK_TRUE;
            }
            else
            {
                sFlag &= ~QMO_MAKEHASH_NOTNULLCHECK_MASK;
                sFlag |= QMO_MAKEHASH_NOTNULLCHECK_FALSE;
            }

            IDE_TEST( qmoOneMtrPlan::makeHASH( aStatement,
                                               aGraph->graph.myQuerySet ,
                                               sFlag ,
                                               1 ,
                                               aGraph->hashBucketCnt ,
                                               sHashFilter ,
                                               aGraph->graph.myPlan ,
                                               sPlan,
                                               ID_FALSE ) // aAllAttrToKey
                      != IDE_SUCCESS);
            aGraph->graph.myPlan = sPlan;



            break;

        case QMG_PROJ_SUBQUERY_STORENSEARCH_LMST:
            //-----------------------------
            // LMST 저장 방식인 경우
            //-----------------------------

            //----------------------------
            // [LMST]의 생성
            // limiRowCount는 2가 된다
            //----------------------------
            sFlag = 0;
            sFlag &= ~QMO_MAKELMST_METHOD_MASK;
            sFlag |= QMO_MAKELMST_STORE_AND_SEARCH;

            IDE_TEST( qmoOneMtrPlan::makeLMST( aStatement ,
                                               aGraph->graph.myQuerySet ,
                                               (ULong)2 ,
                                               aGraph->graph.myPlan ,
                                               sPlan ) != IDE_SUCCESS);
            aGraph->graph.myPlan = sPlan;



            break;

        case QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY:

            //-----------------------------
            // Store Only 저장 방식인 경우
            // SORT노드의 생성
            //-----------------------------

            //----------------------------
            // [SORT]의 생성
            //----------------------------

            sFlag = 0;
            sFlag &= ~QMO_MAKESORT_METHOD_MASK;
            sFlag |= QMO_MAKESORT_STORE_AND_SEARCH;

            //Temp Table의 생성 종류 결정
            if( ( aGraph->graph.flag & QMG_GRAPH_TYPE_MASK ) ==
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
                                               aGraph->graph.myQuerySet ,
                                               sFlag ,
                                               NULL ,
                                               aGraph->graph.myPlan ,
                                               aGraph->graph.costInfo.inputRecordCnt,
                                               sPlan ) != IDE_SUCCESS);
            aGraph->graph.myPlan = sPlan;

            qmg::setPlanInfo( aStatement, sPlan, &(aGraph->graph) );

            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    //----------------------------
    // [PROJ]의 생성
    //----------------------------

    aPROJFlag &= ~QMO_MAKEPROJ_QUERYSET_TOP_MASK;
    aPROJFlag |= QMO_MAKEPROJ_QUERYSET_TOP_TRUE;

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag ,
                                       NULL ,
                                       NULL ,
                                       aGraph->graph.myPlan ,
                                       sOuterPROJ ) != IDE_SUCCESS);

    aGraph->graph.myPlan = sOuterPROJ;

    qmg::setPlanInfo( aStatement, sOuterPROJ, &(aGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::makePlan4InKeyRange( qcStatement * aStatement,
                                    qmgPROJ     * aGraph,
                                    UInt          aPROJFlag )
{
/***********************************************************************
 *
 * Description : qmgProjection으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *
 *        - SUBQUERY IN KEYRANGE 최적화가 적용된 경우
 *
 *               [PROJ]
 *                 |
 *               [HSDS]
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 ***********************************************************************/

    qmnPlan         * sInnerPROJ;
    qmnPlan         * sOuterPROJ;
    qmnPlan         * sVIEW;
    qmnPlan         * sHSDS;
    UInt              sFlag;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePlan4InKeyRange::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //----------------------------
    // Top-down 생성
    //----------------------------

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       NULL,
                                       &sOuterPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sOuterPROJ;

    IDE_TEST( qmoOneMtrPlan::initHSDS( aStatement ,
                                       aGraph->graph.myQuerySet,
                                       ID_FALSE,
                                       aGraph->graph.myPlan,
                                       & sHSDS ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sHSDS;

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aGraph->graph.myPlan,
                                       &sVIEW ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sVIEW;

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       aGraph->graph.myPlan,
                                       &sInnerPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sInnerPROJ;

    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( makeChildPlan( aStatement, aGraph ) != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------------------------------------
    // Subquery IN Keyrange 최적화가 적용된 경우
    // [PROJ] - [HSDS] - [VIEW] - [PROJ]
    //-----------------------------------------------------

    //----------------------------
    // [PROJ]의 생성
    //----------------------------

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag ,
                                       aGraph->limit ,
                                       aGraph->loopNode ,
                                       aGraph->graph.left->myPlan ,
                                       sInnerPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sInnerPROJ;

    qmg::setPlanInfo( aStatement, sInnerPROJ, &(aGraph->graph) );

    //----------------------------
    // [VIEW]의 생성
    //----------------------------
    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_PROJECTION;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       NULL ,
                                       sFlag ,
                                       aGraph->graph.myPlan ,
                                       sVIEW ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sVIEW;

    qmg::setPlanInfo( aStatement, sVIEW, &(aGraph->graph) );

    //----------------------------
    // [HSDS]의 생성
    //----------------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEHSDS_METHOD_MASK;
    sFlag |= QMO_MAKEHSDS_IN_SUBQUERY_KEYRANGE;

    //Temp Table의 생성 종류 결정
    if( (aGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
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
                                       aGraph->graph.myQuerySet,
                                       sFlag ,
                                       aGraph->hashBucketCnt ,
                                       aGraph->graph.myPlan ,
                                       sHSDS ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sHSDS;

    qmg::setPlanInfo( aStatement, sHSDS, &(aGraph->graph) );

    //----------------------------
    // [PROJ]의 생성
    //----------------------------

    aPROJFlag &= ~QMO_MAKEPROJ_QUERYSET_TOP_MASK;
    aPROJFlag |= QMO_MAKEPROJ_QUERYSET_TOP_TRUE;

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag ,
                                       NULL ,
                                       NULL ,
                                       aGraph->graph.myPlan,
                                       sOuterPROJ ) != IDE_SUCCESS);

    aGraph->graph.myPlan = sOuterPROJ;

    qmg::setPlanInfo( aStatement, sOuterPROJ, &(aGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgProjection::makePlan4KeyRange( qcStatement * aStatement,
                                  qmgPROJ     * aGraph,
                                  UInt          aPROJFlag )
{
/***********************************************************************
 *
 * Description : qmgProjection으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *
 *        - SUBQUERY KEYRANGE 최적화가 적용된 경우
 *
 *               [PROJ]
 *                 |
 *               [SORT] : 저장만 함.
 *                 |
 *               [VIEW]
 *                 |
 *               [PROJ]
 *
 ***********************************************************************/

    qmnPlan         * sInnerPROJ;
    qmnPlan         * sOuterPROJ;
    qmnPlan         * sVIEW;
    qmnPlan         * sSORT;
    UInt              sFlag;

    IDU_FIT_POINT_FATAL( "qmgProjection::makePlan4KeyRange::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //----------------------------
    // Top-down 생성
    //----------------------------

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       NULL,
                                       &sOuterPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sOuterPROJ;

    IDE_TEST( qmoOneMtrPlan::initSORT( aStatement ,
                                       aGraph->graph.myQuerySet,
                                       sOuterPROJ,
                                       & sSORT ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sSORT;

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       sSORT,
                                       &sVIEW ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sVIEW;

    IDE_TEST( qmoOneNonPlan::initPROJ( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       aGraph->graph.myPlan,
                                       &sInnerPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sInnerPROJ;

    //---------------------------
    // 하위 Plan의 생성
    //---------------------------

    IDE_TEST( makeChildPlan( aStatement, aGraph ) != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------------------------------------
    // Subquery Keyrange 최적화가 적용된 경우
    // [PROJ] - [SORT] - [VIEW] - [PROJ]
    //-----------------------------------------------------

    //----------------------------
    // [PROJ]의 생성
    //----------------------------
    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement,
                                       aGraph->graph.myQuerySet,
                                       aPROJFlag,
                                       aGraph->limit ,
                                       aGraph->loopNode ,
                                       aGraph->graph.left->myPlan,
                                       sInnerPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sInnerPROJ;

    qmg::setPlanInfo( aStatement, sInnerPROJ, &(aGraph->graph) );

    //----------------------------
    // [VIEW]의 생성
    //----------------------------
    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_PROJECTION;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       NULL ,
                                       sFlag ,
                                       aGraph->graph.myPlan ,
                                       sVIEW ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sVIEW;

    qmg::setPlanInfo( aStatement, sVIEW, &(aGraph->graph) );

    //----------------------------
    // [SORT]의 생성
    //----------------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKESORT_METHOD_MASK;
    sFlag |= QMO_MAKESORT_STORE_AND_SEARCH;

    //Temp Table의 생성 종류 결정
    if( ( aGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
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

    IDE_TEST( qmoOneMtrPlan::makeSORT( aStatement,
                                       aGraph->graph.myQuerySet ,
                                       sFlag ,
                                       NULL ,
                                       aGraph->graph.myPlan ,
                                       aGraph->graph.costInfo.inputRecordCnt,
                                       sSORT ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sSORT;

    qmg::setPlanInfo( aStatement, sSORT, &(aGraph->graph) );

    //----------------------------
    // [PROJ]의 생성
    //----------------------------

    aPROJFlag &= ~QMO_MAKEPROJ_QUERYSET_TOP_MASK;
    aPROJFlag |= QMO_MAKEPROJ_QUERYSET_TOP_TRUE;

    IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                       aGraph->graph.myQuerySet ,
                                       aPROJFlag ,
                                       NULL ,
                                       NULL ,
                                       aGraph->graph.myPlan ,
                                       sOuterPROJ ) != IDE_SUCCESS);
    aGraph->graph.myPlan = sOuterPROJ;

    qmg::setPlanInfo( aStatement, sOuterPROJ, &(aGraph->graph) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgProjection::finalizePreservedOrder( qmgGraph * aGraph )
{
/***********************************************************************
 *
 *  Description : Projection graph는 child graph의 Preserved order를 그대로 따른다.
 *                direction도 child graph의 것을 복사한다.
 *
 *  Implementation :
 *     Child graph의 Preserved order direction을 복사한다.
 *
 ***********************************************************************/

    idBool sIsSamePrevOrderWithChild;

    IDU_FIT_POINT_FATAL( "qmgProjection::finalizePreservedOrder::__FT__" );

    // BUG-36803 Projection Graph must have a left child graph
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
        // Projection은 preserved order를 생성할 수 없는 graph이므로
        // 하위와 preserved order가 다를수 없다.
        IDE_DASSERT(0);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-48779
void qmgProjection::canPushDownLimitIntoScan( qmgGraph * aGraph,
                                              idBool   * aIsScanLimit )
{
    qtcNode * sNode = NULL;

    *aIsScanLimit = ID_FALSE;

    if ( aGraph->type == QMG_SELECTION )
    {
        //---------------------------------------------------
        // 하위가 일반 qmgSelection인 경우
        // 즉, Set, Order By, Group By, Aggregation, Distinct, Join이
        //  없는 경우
        //---------------------------------------------------
        if ( aGraph->myFrom->tableRef->view == NULL )
        {
            // View 가 아닌 경우, SCAN Limit 적용
            sNode = (qtcNode *)aGraph->myQuerySet->SFWGH->where;

            if ( sNode != NULL )
            {
                // where가 존재하는 경우, subquery 존재 유무 검사
                if ( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK )
                     != QTC_NODE_SUBQUERY_EXIST )
                {
                    *aIsScanLimit = ID_TRUE;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // where가 존재하지 않는 경우,SCAN Limit 적용
                *aIsScanLimit = ID_TRUE;
            }
        }
        else
        {
            // nothing to do
        }
    }
}


