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
 * $Id: qmgSet.cpp 89426 2020-12-04 00:47:19Z donovan.seo $
 *
 * Description :
 *     SET Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgSet.h>
#include <qmoCost.h>
#include <qmoSelectivity.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmoTwoMtrPlan.h>
#include <qmoMultiNonPlan.h>

IDE_RC
qmgSet::init( qcStatement * aStatement,
              qmsQuerySet * aQuerySet,
              qmgGraph    * aLeftGraph,
              qmgGraph    * aRightGraph,
              qmgGraph   ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgSet의 초기화
 *
 * Implementation :
 *    (1) qmgSet을 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) out 설정
 *
 ***********************************************************************/

    qmgSET      * sMyGraph;
    qmsQuerySet * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmgSet::init::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aLeftGraph != NULL );
    IDE_DASSERT( aRightGraph != NULL );

    //---------------------------------------------------
    // Set Graph를 위한 기본 초기화
    //---------------------------------------------------

    // qmgSet을 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgSET ),
                                             (void**) & sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( & sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_SET;

    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->setOp = aQuerySet->setOp;

    if ( sMyGraph->graph.myQuerySet->setOp == QMS_INTERSECT )
    {
        if ( aLeftGraph->costInfo.outputRecordCnt >
             aRightGraph->costInfo.outputRecordCnt )
        {
            sMyGraph->graph.left = aRightGraph;
            sMyGraph->graph.right = aLeftGraph;
        }
        else
        {
            sMyGraph->graph.left = aLeftGraph;
            sMyGraph->graph.right = aRightGraph;
        }
    }
    else
    {
        sMyGraph->graph.left = aLeftGraph;
        sMyGraph->graph.right = aRightGraph;
    }

    // PROJ-1358
    // SET의 경우 Child의 Dependency를 누적하지 않고,
    // 자신의 VIEW에 대한 dependency만 설정한다.

    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aQuerySet->depInfo );

    sMyGraph->graph.optimize = qmgSet::optimize;
    sMyGraph->graph.makePlan = qmgSet::makePlan;
    sMyGraph->graph.printGraph = qmgSet::printGraph;

    // DISK/MEMORY 정보 설정
    for ( sQuerySet = aQuerySet;
          sQuerySet->left != NULL;
          sQuerySet = sQuerySet->left ) ;

    switch( sQuerySet->SFWGH->hints->interResultType )
    {
        case QMO_INTER_RESULT_TYPE_NOT_DEFINED :
            if ( sMyGraph->graph.myQuerySet->setOp == QMS_MINUS ||
                 sMyGraph->graph.myQuerySet->setOp == QMS_INTERSECT )
            {
                // left 를 따른다.
                sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                sMyGraph->graph.flag |=
                    (aLeftGraph->flag & QMG_GRAPH_TYPE_MASK );
            }
            else
            {
                // left 또는 right가 disk이면 disk,
                // 그렇지 않을 경우 memory
                if ( ( ( aLeftGraph->flag & QMG_GRAPH_TYPE_MASK )
                       == QMG_GRAPH_TYPE_DISK ) ||
                     ( ( aRightGraph->flag & QMG_GRAPH_TYPE_MASK )
                       == QMG_GRAPH_TYPE_DISK ) )
                {
                    sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                    sMyGraph->graph.flag |= QMG_GRAPH_TYPE_DISK;
                }
                else
                {
                    sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
                    sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MEMORY;
                }
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

    // out 설정
    *aGraph = (qmgGraph*)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSet::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSet의 최적화
 *
 * Implementation :
 *    (1) setOp가 UNION ALL이 아닌 경우, 다음을 수행
 *        A. bucketCnt 설정
 *        B. setOp가 intersect 인 경우, 저장 query 최적화
 *        C. interResultType 설정
 *    (2) 공통 비용 정보 설정
 *    (3) Preserved Order, DISK/MEMORY 설정
 *    (4) Multple Bag Union 최적화 적용
 *
 ***********************************************************************/

    qmgSET      * sMyGraph;
    qmsQuerySet * sQuerySet;
    SDouble       sCost;

    IDU_FIT_POINT_FATAL( "qmgSet::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sMyGraph = (qmgSET*) aGraph;

    // To Fix BUG-8355
    // bucket count 설정
    if ( sMyGraph->graph.myQuerySet->setOp == QMS_UNION_ALL )
    {
        // bucket count 필요없음
    }
    else
    {
        for ( sQuerySet = sMyGraph->graph.myQuerySet;
              sQuerySet->left != NULL;
              sQuerySet = sQuerySet->left ) ;

        IDE_TEST(
            qmg::getBucketCntWithTarget(
                aStatement,
                & sMyGraph->graph,
                sMyGraph->graph.myQuerySet->target,
                sQuerySet->SFWGH->hints->hashBucketCnt,
                & sMyGraph->hashBucketCnt )
            != IDE_SUCCESS );
    }

    //------------------------------------------
    // cost 계산
    //------------------------------------------
    if ( sMyGraph->graph.myQuerySet->setOp != QMS_UNION_ALL )
    {
        if( (sMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
            QMG_GRAPH_TYPE_MEMORY )
        {
            sCost = qmoCost::getMemHashTempCost( aStatement->mSysStat,
                                                 &(aGraph->left->costInfo),
                                                 1 );

            sMyGraph->graph.costInfo.myAccessCost = sCost;
            sMyGraph->graph.costInfo.myDiskCost   = 0;
        }
        else
        {
            sCost = qmoCost::getDiskHashTempCost( aStatement->mSysStat,
                                                  &(aGraph->left->costInfo),
                                                  1,
                                                  sMyGraph->graph.left->costInfo.recordSize );

            sMyGraph->graph.costInfo.myAccessCost = 0;
            sMyGraph->graph.costInfo.myDiskCost   = sCost;
        }
    }
    else
    {
        sMyGraph->graph.costInfo.myAccessCost = 0;
        sMyGraph->graph.costInfo.myDiskCost   = 0;
    }

    //------------------------------------------
    // 공통 비용 정보의 설정
    //------------------------------------------

    // record size
    sMyGraph->graph.costInfo.recordSize =
        sMyGraph->graph.left->costInfo.recordSize;

    // selectivity
    sMyGraph->graph.costInfo.selectivity = 1;

    // input record count
    sMyGraph->graph.costInfo.inputRecordCnt =
        sMyGraph->graph.left->costInfo.outputRecordCnt +
        sMyGraph->graph.right->costInfo.outputRecordCnt;

    // output record count
    IDE_TEST( qmoSelectivity::setSetOutputCnt(
                  sMyGraph->graph.myQuerySet->setOp,
                  sMyGraph->graph.left->costInfo.outputRecordCnt,
                  sMyGraph->graph.right->costInfo.outputRecordCnt,
                  & sMyGraph->graph.costInfo.outputRecordCnt )
              != IDE_SUCCESS );

    // myCost
    // My Access Cost와 My Disk Cost는 이미 계산됨.
    sMyGraph->graph.costInfo.myAllCost =
        sMyGraph->graph.costInfo.myAccessCost +
        sMyGraph->graph.costInfo.myDiskCost;

    // totalCost
    sMyGraph->graph.costInfo.totalAccessCost =
        sMyGraph->graph.left->costInfo.totalAccessCost +
        sMyGraph->graph.right->costInfo.totalAccessCost;
    sMyGraph->graph.costInfo.totalDiskCost =
        sMyGraph->graph.left->costInfo.totalDiskCost +
        sMyGraph->graph.right->costInfo.totalDiskCost;
    sMyGraph->graph.costInfo.totalAllCost =
        sMyGraph->graph.left->costInfo.totalAllCost +
        sMyGraph->graph.right->costInfo.totalAllCost;

    // preserved order 설정
    // To fix BUG-15296
    // Set연산은 hashtemptable을 사용하기 때문에
    // Intersect, Minus에 대해 order를 보장할 수 없음.
    // union all은 원래부터 order보장 안됨.
    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;

    //-----------------------------------------
    // PROJ-1486 Multiple Bag Union 최적화
    //-----------------------------------------

    // BUG-42333
    if ( ( sMyGraph->graph.myQuerySet->setOp == QMS_UNION ) ||
         ( sMyGraph->graph.myQuerySet->setOp == QMS_UNION_ALL ) )
    {
        IDE_TEST( optMultiBagUnion( aStatement, sMyGraph ) != IDE_SUCCESS );
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
qmgSet::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgSet으로 부터 Plan을 생성한다.
 *
 * Implementation :
 *     - qmgSet으로 부터 생성가능한 Plan
 *
 *         - UNION ALL 인 경우
 *
 *               ( [PROJ] ) : parent가 SET인 경우 생성됨
 *                   |
 *                 [VIEW]
 *                   |
 *                 [BUNI]
 *                 |    |
 *
 *         - UNION인 경우
 *
 *               ( [PROJ] ) : parent가 SET인 경우 생성됨
 *                   |
 *                 [HSDS]
 *                   |
 *                 [VIEW]
 *                   |
 *                 [BUNI]
 *                 |    |
 *
 *         - INTERSECT인 경우
 *
 *               ( [PROJ] ) : parent가 SET인 경우 생성됨
 *                   |
 *                 [VIEW]
 *                   |
 *                 [SITS]
 *                 |    |
 *
 *         - MINUS인 경우
 *
 *               ( [PROJ] ) : parent가 SET인 경우 생성됨
 *                   |
 *                 [VIEW]
 *                   |
 *                 [SDIF]
 *                 |    |
 *
 ***********************************************************************/

    qmgSET          * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmgSet::makePlan::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgSET*) aGraph;

    /* PROJ-1071 Parallel Query */
    aGraph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    sMyGraph->graph.myPlan = aParent->myPlan;

    switch( sMyGraph->setOp )
    {
        //---------------------------------------------------
        // UNION연산을 위한 BUNI노드의 생성
        //---------------------------------------------------
        case QMS_UNION:
            IDE_TEST( makeUnion( aStatement,
                                 sMyGraph,
                                 ID_FALSE )
                      != IDE_SUCCESS );
            break;
        case QMS_UNION_ALL:
            IDE_TEST( makeUnion( aStatement,
                                 sMyGraph,
                                 ID_TRUE )
                      != IDE_SUCCESS );
            break;
        case QMS_MINUS:
            IDE_TEST( makeMinus( aStatement,
                                 sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMS_INTERSECT:
            IDE_TEST( makeIntersect( aStatement,
                                     sMyGraph )
                      != IDE_SUCCESS );
            break;
        case QMS_NONE :
        default:
            IDE_DASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSet::makeChild( qcStatement * aStatement,
                   qmgSET      * aMyGraph )
{
    qmgChildren * sChildren;

    IDU_FIT_POINT_FATAL( "qmgSet::makeChild::__FT__" );

    if ( aMyGraph->graph.children == NULL )
    {
        // BUG-38410
        // SCAN parallel flag 를 자식 노드로 물려준다.
        aMyGraph->graph.left->flag  |= (aMyGraph->graph.flag &
                                        QMG_PLAN_EXEC_REPEATED_MASK);
        aMyGraph->graph.right->flag |= (aMyGraph->graph.flag &
                                        QMG_PLAN_EXEC_REPEATED_MASK);

        IDE_TEST( aMyGraph->graph.left->makePlan( aStatement ,
                                                  &aMyGraph->graph ,
                                                  aMyGraph->graph.left )
                  != IDE_SUCCESS);
        IDE_TEST( aMyGraph->graph.right->makePlan( aStatement ,
                                                   &aMyGraph->graph ,
                                                   aMyGraph->graph.right )
                  != IDE_SUCCESS);
    }
    else
    {
        // PROJ-1486
        // Multi Graph의 children들의 plan tree를 생성한다.
        for( sChildren = aMyGraph->graph.children;
             sChildren != NULL;
             sChildren = sChildren->next )
        {
            // BUG-38410
            // SCAN parallel flag 를 자식 노드로 물려준다.
            sChildren->childGraph->flag |= (aMyGraph->graph.flag &
                                            QMG_PLAN_EXEC_REPEATED_MASK);

            IDE_TEST(
                sChildren->childGraph->makePlan( aStatement ,
                                                 &aMyGraph->graph ,
                                                 sChildren->childGraph )
                != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgSet::makeUnion( qcStatement * aStatement,
                   qmgSET      * aMyGraph,
                   idBool        aIsUnionAll )
{
    qmnPlan * sPROJ = NULL;
    qmnPlan * sHSDS = NULL;
    qmnPlan * sVIEW = NULL;
    qmnPlan * sBUNI = NULL;

    UInt      sFlag;

    IDU_FIT_POINT_FATAL( "qmgSet::makeUnion::__FT__" );

    //-----------------------------------------------------
    //        [*PROJ]
    //           |
    //        [*HSDS]
    //           |
    //        [VIEW]
    //           |
    //        [MultiBUNI]
    //           | 
    //        [Children] - [Children]
    //           |           |
    //        [LEFT]       [RIGHT]
    // * Option
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //---------------------------------------------------
    // Parent가 SET인 경우 PROJ를 생성한다.
    //---------------------------------------------------
    if ( (aMyGraph->graph.flag & QMG_SET_PARENT_TYPE_SET_MASK) ==
         QMG_SET_PARENT_TYPE_SET_TRUE )
    {
        //-----------------------
        // init PROJ 
        //-----------------------

        IDE_TEST( qmoOneNonPlan::initPROJ( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           NULL,
                                           &sPROJ ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sPROJ;
    }
    else
    {
        // nothing to do
    }

    if( aIsUnionAll == ID_FALSE )
    {
        //-----------------------
        // init HSDS
        //-----------------------

        IDE_TEST( qmoOneMtrPlan::initHSDS( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           ID_FALSE,
                                           aMyGraph->graph.myPlan ,
                                           &sHSDS ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sHSDS;
    }
    else
    {
        // nothing to do
    }

    //-----------------------
    // init VIEW
    //-----------------------

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myPlan ,
                                       &sVIEW ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sVIEW;

    //-----------------------
    // init MultiBUNI
    //-----------------------
    IDE_DASSERT( aMyGraph->graph.children != NULL );

    IDE_TEST(
        qmoMultiNonPlan::initMultiBUNI( aStatement,
                                        aMyGraph->graph.myQuerySet,
                                        aMyGraph->graph.myPlan,
                                        & sBUNI )
        != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sBUNI;

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChild( aStatement,
                         aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make MultiBUNI
    //-----------------------

    IDE_TEST(
        qmoMultiNonPlan::makeMultiBUNI( aStatement,
                                        aMyGraph->graph.myQuerySet,
                                        aMyGraph->graph.children,
                                        sBUNI )
        != IDE_SUCCESS );

    aMyGraph->graph.myPlan = sBUNI;

    qmg::setPlanInfo( aStatement, sBUNI, &(aMyGraph->graph) );

    //-----------------------
    // make VIEW
    //-----------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_SET;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myFrom ,
                                       sFlag ,
                                       aMyGraph->graph.myPlan ,
                                       sVIEW ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sVIEW;

    qmg::setPlanInfo( aStatement, sVIEW, &(aMyGraph->graph) );

    if( aIsUnionAll == ID_FALSE )
    {
        //-----------------------
        // make HSDS
        //-----------------------

        sFlag = 0;
        sFlag &= ~QMO_MAKEHSDS_METHOD_MASK;
        sFlag |= QMO_MAKEHSDS_SET_UNION;

        //Temp Table저장 매체 선택
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
                                           sHSDS ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sHSDS;

        qmg::setPlanInfo( aStatement, sHSDS, &(aMyGraph->graph) );
    }
    else
    {
        // Nothing to do.
    }

    if ( (aMyGraph->graph.flag & QMG_SET_PARENT_TYPE_SET_MASK) ==
         QMG_SET_PARENT_TYPE_SET_TRUE )
    {
        //-----------------------
        // init PROJ 
        //-----------------------

        sFlag = 0;
        sFlag &= ~QMO_MAKEPROJ_TOP_MASK;
        sFlag |= QMO_MAKEPROJ_TOP_FALSE;

        sFlag &= ~QMO_MAKEPROJ_INDEXABLE_MINMAX_MASK;
        sFlag |= QMO_MAKEPROJ_INDEXABLE_MINMAX_FALSE;

        IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           sFlag ,
                                           NULL ,
                                           NULL ,
                                           aMyGraph->graph.myPlan ,
                                           sPROJ ) != IDE_SUCCESS);

        aMyGraph->graph.myPlan = sPROJ;

        qmg::setPlanInfo( aStatement, sPROJ, &(aMyGraph->graph) );
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
qmgSet::makeIntersect( qcStatement * aStatement,
                       qmgSET      * aMyGraph )
{
    qmnPlan * sPROJ = NULL;
    qmnPlan * sVIEW = NULL;
    qmnPlan * sSITS = NULL;

    UInt      sFlag;

    IDU_FIT_POINT_FATAL( "qmgSet::makeIntersect::__FT__" );

    //-----------------------------------------------------
    //        [*PROJ]
    //           |
    //        [VIEW]
    //           |
    //        [SITS]
    //         /  `.
    //    [LEFT]  [RIGHT]
    // * Option
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //---------------------------------------------------
    // Parent가 SET인 경우 PROJ를 생성한다.
    //---------------------------------------------------
    if ( (aMyGraph->graph.flag & QMG_SET_PARENT_TYPE_SET_MASK) ==
         QMG_SET_PARENT_TYPE_SET_TRUE )
    {
        //-----------------------
        // init PROJ 
        //-----------------------

        IDE_TEST( qmoOneNonPlan::initPROJ( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           NULL,
                                           &sPROJ ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sPROJ;
    }
    else
    {
        // nothing to do
    }

    //-----------------------
    // init VIEW
    //-----------------------

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myPlan ,
                                       &sVIEW ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sVIEW;
    //-----------------------
    // init SITS
    //-----------------------

    IDE_TEST( qmoTwoMtrPlan::initSITS( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myPlan,
                                       &sSITS )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sSITS;

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChild( aStatement,
                         aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make SITS
    //-----------------------

    sFlag = 0;
    if( (aMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
        QMG_GRAPH_TYPE_MEMORY )
    {
        sFlag &= ~QMO_MAKESITS_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKESITS_MEMORY_TEMP_TABLE;
    }
    else
    {
        sFlag &= ~QMO_MAKESITS_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKESITS_DISK_TEMP_TABLE;
    }

    IDE_TEST( qmoTwoMtrPlan::makeSITS( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       aMyGraph->hashBucketCnt ,
                                       aMyGraph->graph.left->myPlan ,
                                       aMyGraph->graph.right->myPlan ,
                                       sSITS ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sSITS;

    qmg::setPlanInfo( aStatement, sSITS, &(aMyGraph->graph) );

    //-----------------------
    // make VIEW
    //-----------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_SET;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myFrom ,
                                       sFlag ,
                                       aMyGraph->graph.myPlan ,
                                       sVIEW ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sVIEW;

    qmg::setPlanInfo( aStatement, sVIEW, &(aMyGraph->graph) );

    if ( (aMyGraph->graph.flag & QMG_SET_PARENT_TYPE_SET_MASK) ==
         QMG_SET_PARENT_TYPE_SET_TRUE )
    {
        //-----------------------
        // make PROJ 
        //-----------------------

        sFlag = 0;
        sFlag &= ~QMO_MAKEPROJ_TOP_MASK;
        sFlag |= QMO_MAKEPROJ_TOP_FALSE;

        sFlag &= ~QMO_MAKEPROJ_INDEXABLE_MINMAX_MASK;
        sFlag |= QMO_MAKEPROJ_INDEXABLE_MINMAX_FALSE;

        IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           sFlag ,
                                           NULL ,
                                           NULL ,
                                           aMyGraph->graph.myPlan ,
                                           sPROJ ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sPROJ;

        qmg::setPlanInfo( aStatement, sPROJ, &(aMyGraph->graph) );
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
qmgSet::makeMinus( qcStatement * aStatement,
                   qmgSET      * aMyGraph )
{
    qmnPlan * sPROJ = NULL;
    qmnPlan * sVIEW = NULL;
    qmnPlan * sSDIF = NULL;

    UInt      sFlag;

    IDU_FIT_POINT_FATAL( "qmgSet::makeMinus::__FT__" );

    //-----------------------------------------------------
    //        [*PROJ]
    //           |
    //        [VIEW]
    //           |
    //        [SDIF]
    //         /  `.
    //    [LEFT]  [RIGHT]
    // * Option
    //-----------------------------------------------------

    //----------------------------
    // Top-down 초기화
    //----------------------------

    //---------------------------------------------------
    // Parent가 SET인 경우 PROJ를 생성한다.
    //---------------------------------------------------
    if ( (aMyGraph->graph.flag & QMG_SET_PARENT_TYPE_SET_MASK) ==
         QMG_SET_PARENT_TYPE_SET_TRUE )
    {
        //-----------------------
        // init PROJ 
        //-----------------------

        IDE_TEST( qmoOneNonPlan::initPROJ( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           NULL,
                                           &sPROJ ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sPROJ;
    }
    else
    {
        // nothing to do
    }

    //-----------------------
    // init VIEW
    //-----------------------

    IDE_TEST( qmoOneNonPlan::initVIEW( aStatement,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myPlan ,
                                       &sVIEW ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sVIEW;

    //-----------------------
    // init SDIF
    //-----------------------

    IDE_TEST( qmoTwoMtrPlan::initSDIF( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myPlan,
                                       &sSDIF )
              != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sSDIF;

    //-----------------------
    // 하위 plan 생성
    //-----------------------

    IDE_TEST( makeChild( aStatement,
                         aMyGraph )
              != IDE_SUCCESS );

    //----------------------------
    // Bottom-up 생성
    //----------------------------

    //-----------------------
    // make SDIF
    //-----------------------

    sFlag = 0;
    if( (aMyGraph->graph.flag & QMG_GRAPH_TYPE_MASK) ==
        QMG_GRAPH_TYPE_MEMORY )
    {
        sFlag &= ~QMO_MAKESDIF_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKESDIF_MEMORY_TEMP_TABLE;
    }
    else
    {
        sFlag &= ~QMO_MAKESDIF_TEMP_TABLE_MASK;
        sFlag |= QMO_MAKESDIF_DISK_TEMP_TABLE;
    }

    IDE_TEST( qmoTwoMtrPlan::makeSDIF( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       sFlag ,
                                       aMyGraph->hashBucketCnt ,
                                       aMyGraph->graph.left->myPlan,
                                       aMyGraph->graph.right->myPlan,
                                       sSDIF ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sSDIF;

    qmg::setPlanInfo( aStatement, sSDIF, &(aMyGraph->graph) );

    //-----------------------
    // make VIEW
    //-----------------------

    sFlag = 0;
    sFlag &= ~QMO_MAKEVIEW_FROM_MASK;
    sFlag |= QMO_MAKEVIEW_FROM_SET;

    IDE_TEST( qmoOneNonPlan::makeVIEW( aStatement ,
                                       aMyGraph->graph.myQuerySet ,
                                       aMyGraph->graph.myFrom ,
                                       sFlag ,
                                       aMyGraph->graph.myPlan ,
                                       sVIEW ) != IDE_SUCCESS);
    aMyGraph->graph.myPlan = sVIEW;

    qmg::setPlanInfo( aStatement, sVIEW, &(aMyGraph->graph) );

    if ( (aMyGraph->graph.flag & QMG_SET_PARENT_TYPE_SET_MASK) ==
         QMG_SET_PARENT_TYPE_SET_TRUE )
    {
        //-----------------------
        // make PROJ 
        //-----------------------

        sFlag = 0;
        sFlag &= ~QMO_MAKEPROJ_TOP_MASK;
        sFlag |= QMO_MAKEPROJ_TOP_FALSE;

        sFlag &= ~QMO_MAKEPROJ_INDEXABLE_MINMAX_MASK;
        sFlag |= QMO_MAKEPROJ_INDEXABLE_MINMAX_FALSE;

        IDE_TEST( qmoOneNonPlan::makePROJ( aStatement ,
                                           aMyGraph->graph.myQuerySet ,
                                           sFlag ,
                                           NULL ,
                                           NULL ,
                                           aMyGraph->graph.myPlan ,
                                           sPROJ ) != IDE_SUCCESS);
        aMyGraph->graph.myPlan = sPROJ;

        qmg::setPlanInfo( aStatement, sPROJ, &(aMyGraph->graph) );
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
qmgSet::printGraph( qcStatement  * aStatement,
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

    qmgChildren * sChildren;

    IDU_FIT_POINT_FATAL( "qmgSet::printGraph::__FT__" );

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

    if ( aGraph->children == NULL )
    {
        IDE_TEST( aGraph->left->printGraph( aStatement,
                                            aGraph->left,
                                            aDepth + 1,
                                            aString )
                  != IDE_SUCCESS );

        IDE_TEST( aGraph->right->printGraph( aStatement,
                                             aGraph->right,
                                             aDepth + 1,
                                             aString )
                  != IDE_SUCCESS );
    }
    else
    {
        // Multiple Children에 대한 Graph 정보 출력
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgSet::optMultiBagUnion( qcStatement * aStatement,
                          qmgSET      * aSETGraph )
{
/***********************************************************************
 *
 * Description : PROJ-1486
 *    Multiple Bag Union 최적화를 적용함.
 *
 * Implementation :
 *
 *    Left와 Right Graph에 대하여 각각 다음을 수행한다.
 *
 *    - 1. 현재의 연결 관계를 끊는다.
 *    - 2. 각 Child의 Graph정보를 연결한다.
 *    - 3. 각 Child의 Data Type을 일치시킨다.
 *
 ***********************************************************************/

    qmgGraph    * sChildGraph;
    qmgChildren * sChildren;
    qmsTarget   * sCurTarget;
    qmsTarget   * sChildTarget;

    IDU_FIT_POINT_FATAL( "qmgSet::optMultiBagUnion::__FT__" );

    // BUG-42333
    IDE_DASSERT( ( aSETGraph->graph.myQuerySet->setOp == QMS_UNION_ALL ) ||
                 ( aSETGraph->graph.myQuerySet->setOp == QMS_UNION ) );

    //-------------------------------------------
    // Left Child에 대한 처리
    //-------------------------------------------

    // 1. 현재의 연결 관계 제거
    sChildGraph = aSETGraph->graph.left;
    aSETGraph->graph.left = NULL;

    // 2. 각 Child의 Graph정보를 연결한다.
    IDE_TEST( linkChildGraph( aStatement,
                              sChildGraph,
                              & aSETGraph->graph.children )
              != IDE_SUCCESS );

    //-------------------------------------------
    // Right Child에 대한 처리
    //-------------------------------------------

    // 0. 다음 연결을 위한 위치로 이동
    for ( sChildren = aSETGraph->graph.children;
          sChildren->next != NULL;
          sChildren = sChildren->next );

    // 1. 현재의 연결 관계 제거
    sChildGraph = aSETGraph->graph.right;
    aSETGraph->graph.right = NULL;

    // 2. 각 Child의 Graph정보를 연결한다.
    IDE_TEST( linkChildGraph( aStatement,
                              sChildGraph,
                              & sChildren->next )
              != IDE_SUCCESS );

    //-------------------------------------------
    // 3. 각 Child의 Target의 결과를
    //    최종 Target과 동일한 Data Type으로 일치시킴.
    //-------------------------------------------

    for ( sChildren = aSETGraph->graph.children;
          sChildren != NULL;
          sChildren = sChildren->next )
    {
        for ( sCurTarget = aSETGraph->graph.myQuerySet->target,
                  sChildTarget = sChildren->childGraph->myQuerySet->target;
              ( sCurTarget != NULL ) && ( sChildTarget != NULL );
              sCurTarget = sCurTarget->next,
                  sChildTarget = sChildTarget->next )
        {
            // 기존의 Child Target의 conversion을 제거
            sChildTarget->targetColumn->node.conversion = NULL;

            // 현재 Target과 동일한 Data Type으로 일치시킴
            // 현재 Target은 가장 상위의 Data Type으로 변경되지 않는다.
            // 즉, child target에 의하여 영향을 받지 않는다.
            IDE_TEST(
                qtc::makeSameDataType4TwoNode( aStatement,
                                               sCurTarget->targetColumn,
                                               sChildTarget->targetColumn )
                != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmgSet::linkChildGraph( qcStatement  * aStatement,
                        qmgGraph     * aChildGraph,
                        qmgChildren ** aChildren )
{
/***********************************************************************
 *
 * Description : PROJ-1486
 *     하나의 Child Graph에 대하여 연결을 맺는다.
 *
 * Implementation :
 *
 *    - 2-1. Child가 PROJECTION or UNION_ALL 이 아닌 SET일 경우
 *           해당 Graph를 children에 연결
 *    - 2-2. Child가 UNOIN_ALL 일 경우
 *    - 2-2-1. children이 있을 경우 이를 연결한다.
 *    - 2-2-2. children이 없을 경우 해당 Graph를 children에 연결
 *
 ***********************************************************************/

    qmgChildren * sChildren;
    qmgChildren * sNextChildren;

    IDU_FIT_POINT_FATAL( "qmgSet::linkChildGraph::__FT__" );

    if ( ( aChildGraph->type == QMG_PROJECTION ) ||
         ( ( aChildGraph->type == QMG_SET ) &&
           ( ((qmgSET*)aChildGraph)->setOp != QMS_UNION_ALL ) ) )
    {
        // BUG-42333
        //---------------------------------------
        // 2-1. Projection or UNION_ALL 이 아닌 SET 일 경우의 연결
        //---------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgChildren) ,
                                                 (void **) & sChildren )
                  != IDE_SUCCESS );

        sChildren->childGraph = aChildGraph;
        sChildren->next = NULL;

        *aChildren = sChildren;
    }
    else
    {
        //---------------------------------------
        // 2-2. UNION_ALL 인 경우의 연결
        //---------------------------------------

        IDE_DASSERT( aChildGraph->type == QMG_SET );
        IDE_DASSERT( ((qmgSET*)aChildGraph)->setOp == QMS_UNION_ALL );

        if ( aChildGraph->children != NULL )
        {
            //---------------------------------------
            // 2-2-1. children이 있을 경우 이를 연결한다.
            //---------------------------------------

            *aChildren = aChildGraph->children;
        }
        else
        {
            //---------------------------------------
            // 2-2-2. children이 없을 경우 해당 Graph를 children에 연결
            //---------------------------------------

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgChildren) ,
                                                     (void **) & sChildren )
                      != IDE_SUCCESS );

            sChildren->childGraph = aChildGraph->left;
            sChildren->next = NULL;

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgChildren) ,
                                                     (void **) & sNextChildren )
                      != IDE_SUCCESS );

            sNextChildren->childGraph = aChildGraph->right;
            sNextChildren->next = NULL;

            sChildren->next = sNextChildren;
            *aChildren = sChildren;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
