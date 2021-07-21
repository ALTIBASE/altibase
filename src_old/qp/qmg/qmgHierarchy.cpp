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
 * $Id: qmgHierarchy.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Hierarchy Graph를 위한 수행 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmoCost.h>
#include <qmoCostDef.h>
#include <qmoSelectivity.h>
#include <qmoOneNonPlan.h>
#include <qmoOneMtrPlan.h>
#include <qmgHierarchy.h>
#include <qmgSelection.h>
#include <qmo.h>
#include <qmv.h>
#include <qmvQTC.h>

IDE_RC
qmgHierarchy::init( qcStatement  * aStatement,
                    qmsQuerySet  * aQuerySet,
                    qmgGraph     * aChildGraph,
                    qmsFrom      * aFrom,
                    qmsHierarchy * aHierarchy,
                    qmgGraph    ** aGraph )
{
/***********************************************************************
 *
 * Description : qmgHierarchy Graph의 최적화
 *
 * Implementation :
 *    (1) qmgHierarchy를 위한 공간 할당
 *    (2) graph( 모든 Graph를 위한 공통 자료 구조 ) 초기화
 *    (3) graph.type 설정
 *    (4) graph.myQuerySet을 aQuerySet으로 설정
 *    (5) graph.myFrom을 aFrom으로 설정
 *    (6) graph.dependencies 설정
 *    (7) qmgHierarchy의 startWithCNF 구성
 *    (8) qmgHierarchy의 connectByCNF 구성
 *    (9) DISK/MEMORY 설정
 *
 ***********************************************************************/

    qmgHIER * sMyGraph;
    qmoCNF  * sStartWithCNF = NULL;
    qmoCNF  * sConnectByCNF = NULL;
    qtcNode * sNormalCNF;
    qmoNormalType     sNormalType;
    qtcNode         * sNode[2];
    qcNamePosition    sPosition;

    IDU_FIT_POINT_FATAL( "qmgHierarchy::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );
    IDE_DASSERT( aHierarchy != NULL );

    //---------------------------------------------------
    // Hierarchy Graph를 위한 기본 초기화
    //---------------------------------------------------

    sStartWithCNF = NULL;
    sConnectByCNF = NULL;

    // qmgHierarchy를 위한 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgHIER ),
                                             (void**) &sMyGraph )
              != IDE_SUCCESS );

    // Graph 공통 정보의 초기화
    IDE_TEST( qmg::initGraph( &sMyGraph->graph ) != IDE_SUCCESS );

    sMyGraph->graph.type = QMG_HIERARCHY;
    sMyGraph->graph.left = aChildGraph;
    qtc::dependencySetWithDep( & sMyGraph->graph.depInfo,
                               & aFrom->depInfo );
    sMyGraph->graph.myFrom = aFrom;
    sMyGraph->graph.myQuerySet = aQuerySet;

    sMyGraph->graph.optimize = qmgHierarchy::optimize;
    sMyGraph->graph.makePlan = qmgHierarchy::makePlan;
    sMyGraph->graph.printGraph = qmgHierarchy::printGraph;

    if ( aChildGraph != NULL )
    {
        sMyGraph->graph.flag &= ~QMG_HIERARCHY_JOIN_MASK;
        sMyGraph->graph.flag |= QMG_HIERARCHY_JOIN_TRUE;
    }
    else
    {
        if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.
             rows[sMyGraph->graph.myFrom->tableRef->table].lflag & MTC_TUPLE_STORAGE_MASK )
             == MTC_TUPLE_STORAGE_DISK )
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

    /* BUG-48300 */
    if ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_CONNECT_BY_FUNC_MASK )
         == QMV_SFWGH_CONNECT_BY_FUNC_TRUE )
    {
        /* PROJ-1715 Connect By Stack Address Pseudo Column */
        if ( aQuerySet->SFWGH->cnbyStackAddr == NULL )
        {
            SET_EMPTY_POSITION( sPosition );

            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       NULL,
                                       &sPosition,
                                       NULL ) != IDE_SUCCESS );

            aQuerySet->SFWGH->cnbyStackAddr = sNode[0];

            /* make tuple for Connect By Stack Address */
            IDE_TEST(qmvQTC::makeOneTupleForPseudoColumn( aStatement,
                                                          aQuerySet->SFWGH->cnbyStackAddr,
                                                          (SChar *)"BIGINT",
                                                          6 )
                     != IDE_SUCCESS);
        }
    }

    //---------------------------------------------------
    // Hierarchy Graph 만을 위한 자료 구조 초기화
    //---------------------------------------------------

    sMyGraph->myHierarchy = aHierarchy;

    //---------------------------------------------------
    // startWithCNF 구성
    //---------------------------------------------------

    // start with CNF 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ),
                                             (void**)& sStartWithCNF )
              != IDE_SUCCESS);

    sMyGraph->startWithCNF = sStartWithCNF;
    sMyGraph->mStartWithMethodCnt      = 0;
    sMyGraph->mStartWithAccessMethod   = NULL;
    sMyGraph->mSelectedStartWithMethod = NULL;
    sMyGraph->mSelectedStartWithIdx    = NULL;

    if ( sMyGraph->myHierarchy->startWith != NULL )
    {
        // To Fix PR-12743 NNF Filter지원
        IDE_TEST( qmoCrtPathMgr
                  ::decideNormalType( aStatement,
                                      aFrom,
                                      sMyGraph->myHierarchy->startWith,
                                      aQuerySet->SFWGH->hints,
                                      ID_TRUE, // CNF Only임
                                      & sNormalType )
                  != IDE_SUCCESS );

        switch ( sNormalType )
        {
            case QMO_NORMAL_TYPE_NNF:

                if ( sMyGraph->myHierarchy->startWith != NULL )
                {
                    sMyGraph->myHierarchy->startWith->lflag
                        &= ~QTC_NODE_NNF_FILTER_MASK;
                    sMyGraph->myHierarchy->startWith->lflag
                        |= QTC_NODE_NNF_FILTER_TRUE;
                }
                else
                {
                    // Nothing To Do
                }

                IDE_TEST( qmoCnfMgr::init( aStatement,
                                           sMyGraph->startWithCNF,
                                           aQuerySet,
                                           aFrom,
                                           NULL,
                                           sMyGraph->myHierarchy->startWith )
                          != IDE_SUCCESS );

                break;

            case QMO_NORMAL_TYPE_CNF:
                // normalizeCNF
                IDE_TEST(
                    qmoNormalForm
                    ::normalizeCNF( aStatement,
                                    sMyGraph->myHierarchy->startWith,
                                    & sNormalCNF )
                    != IDE_SUCCESS );

                IDE_TEST( qmoCnfMgr::init( aStatement,
                                           sMyGraph->startWithCNF,
                                           aQuerySet,
                                           aFrom,
                                           sNormalCNF,
                                           NULL )
                          != IDE_SUCCESS );

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
        // To Fix BUG-9180
        IDE_TEST( qmoCnfMgr::init( aStatement,
                                   sMyGraph->startWithCNF,
                                   aQuerySet,
                                   aFrom,
                                   NULL,
                                   NULL )
                  != IDE_SUCCESS );
    }

    //---------------------------------------------------
    // connectBy 초기화
    //---------------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ),
                                             (void**)& sConnectByCNF )
              != IDE_SUCCESS);

    sMyGraph->connectByCNF = sConnectByCNF;
    sMyGraph->mConnectByMethodCnt      = 0;
    sMyGraph->mConnectByAccessMethod   = NULL;
    sMyGraph->mSelectedConnectByMethod = NULL;
    sMyGraph->mSelectedConnectByIdx    = NULL;

    if ( sMyGraph->myHierarchy->connectBy != NULL )
    {
        // To Fix PR-12743 NNF Filter지원
        IDE_TEST( qmoCrtPathMgr
                  ::decideNormalType( aStatement,
                                      aFrom,
                                      sMyGraph->myHierarchy->connectBy,
                                      aQuerySet->SFWGH->hints,
                                      ID_TRUE, // CNF Only임
                                      & sNormalType )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sNormalType != QMO_NORMAL_TYPE_CNF,
                        ERR_CONNECT_BY_NOCNF );

        IDE_TEST( qmoNormalForm
                  ::normalizeCNF( aStatement,
                                  sMyGraph->myHierarchy->connectBy,
                                  & sNormalCNF )
                  != IDE_SUCCESS );

        IDE_TEST( qmoCnfMgr::init( aStatement,
                                   sMyGraph->connectByCNF,
                                   aQuerySet,
                                   aFrom,
                                   sNormalCNF,
                                   NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        /* hierarchy query에는 connect by가 반드시 존재해야 함 */
        IDE_RAISE( ERR_NO_CONNECT_BY );
    }

    // out 설정
    *aGraph = (qmgGraph*)sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONNECT_BY_NOCNF );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMV_CONNECT_BY_NOT_ALLOW_NOCNF ) );
    }
    IDE_EXCEPTION( ERR_NO_CONNECT_BY )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                "sMyGraph->myHierarchy->connectBy == NULL"));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmgHierarchy::optimize( qcStatement * aStatement, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgHierarchy Graph의 최적화
 *
 * Implementation :
 *    (1)  subquery graph 생성
 *         - Start With절의 Subquery 처리
 *         - Connect By절의 Subquery 처리
 *         : 단, Hierarchy Query는 Subquery와 함께 쓰이지 못한다.
 *           추후 확장을 위해 위의 처리를 남겨둔다.
 *    (2)  공통 비용 정보의 설정( recordSize, inputRecordCnt, pageCnt )
 *    (3)  startWithCNF의 Predicate 분류
 *    (4)  startWith의 accessMethod의 선택
 *    (5)  connectByCNF의 Predicate 분류
 *    (6)  connectBy의 accessMethod의 선택
 *    (7)  전체 selectivity 계산 및 공통 비용 정보의 selectivity에 저장
 *         selectivity = where selectivity *  (=> SCAN 으로 내리지 않음)
 *                       start with selectivity *
 *                       connect by selectivity
 *         output record count = T(R) * T(R) * selectivity
 *    (8) Preserved Order 설정
 *
 ***********************************************************************/

    qmgHIER     * sMyGraph;
    qmsTableRef * sTableRef;
    qcmColumn   * sColumns   = NULL;
    UInt          sIndexCnt  = 0;
    UInt          sColumnCnt = 0;
    UInt          i          = 0;
    UInt          sFlag      = 0;
    UInt          sSelectedScanHint = 0;
    UShort        sTableID          = 0;
    idBool        sFind             = ID_FALSE;
    qmsFrom     * sFrom             = NULL;
    qmsFrom     * sFindFrom         = NULL;
    qmoPredicate * sPredicate       = NULL;

    SDouble       sOutputRecordCnt = 0;
    SDouble       sInputRecordCnt  = 0;
    SDouble       sSortCost        = 0;
    SDouble       sFilterCost      = 0;
    SDouble       sRecordSize      = 0;

    IDU_FIT_POINT_FATAL( "qmgHierarchy::optimize::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //---------------------------------------------------
    // 기본 초기화
    //---------------------------------------------------

    sMyGraph = (qmgHIER*) aGraph;
    sTableRef = sMyGraph->graph.myFrom->tableRef;

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        /* TASK-7219 Non-shard DML */
        IDE_TEST( qmo::removeOutRefPredPushedForce( & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
         == QMG_HIERARCHY_JOIN_FALSE )
    {
        /*
         * PROJ-1715 Hierarchy
         * View Graph의 생성 및 통계 정보 구축
         *   ( 일반 Table의 통계 정보는 Validation 과정에서 설정됨 )
         */
        if ( sTableRef->view != NULL )
        {
            sTableRef->viewOptType = QMO_VIEW_OPT_TYPE_CMTR;
            // View Graph의 생성
            IDE_TEST( qmgSelection::makeViewGraph( aStatement, & sMyGraph->graph )
                      != IDE_SUCCESS );

            sMyGraph->graph.left->flag &= ~QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK;
            sMyGraph->graph.left->flag |= QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE;

            // 통계 정보의 구축
            IDE_TEST( qmoStat::getStatInfo4View( aStatement,
                                                 & sMyGraph->graph,
                                                 & sTableRef->statInfo )
                      != IDE_SUCCESS );

            // fix BUG-11209
            // selection graph는 하위에 view가 올 수 있으므로
            // view가 있을 때는 view의 projection graph의 타입으로 flag를 보정한다.
            sMyGraph->graph.flag &= ~QMG_GRAPH_TYPE_MASK;
            sMyGraph->graph.flag |= QMG_GRAPH_TYPE_MASK & sMyGraph->graph.left->flag;
        }
        else
        {
            sIndexCnt = sMyGraph->graph.myFrom->tableRef->tableInfo->indexCount;
        }
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------------------------
    // Subquery Graph 생성
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sMyGraph->graph.myPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // startWithCNF의 Predicate 분류 및 access method의 선택
    //---------------------------------------------------

    if ( sMyGraph->startWithCNF != NULL )
    {
        IDE_TEST( optimizeStartWith( aStatement, sMyGraph, sTableRef, sIndexCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* connectByCNF의 Predicate 분류 및 access method의 선택 */
    // Predicate 분류
    if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
         == QMG_HIERARCHY_JOIN_FALSE )
    {
        IDE_TEST(
            qmoCnfMgr::classifyPred4ConnectBy( aStatement,
                                               sMyGraph->connectByCNF,
                                               &sMyGraph->connectByCNF->depInfo )
            != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST(
            qmoCnfMgr::classifyPred4ConnectBy( aStatement,
                                               sMyGraph->connectByCNF,
                                               &sMyGraph->graph.myQuerySet->depInfo )
            != IDE_SUCCESS );
    }

    sFindFrom = sMyGraph->graph.myFrom;
    // To Fix BUG-9050
    if ( sMyGraph->connectByCNF->oneTablePredicate != NULL )
    {
        if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
             == QMG_HIERARCHY_JOIN_FALSE )
        {
            // Predicate 재배치
            IDE_TEST(
                qmoPred::relocatePredicate(
                    aStatement,
                    sMyGraph->connectByCNF->oneTablePredicate,
                    & sMyGraph->graph.depInfo,
                    & qtc::zeroDependencies,
                    sMyGraph->graph.myFrom->tableRef->statInfo,
                    & sMyGraph->connectByCNF->oneTablePredicate )
                != IDE_SUCCESS );
        }
        else
        {
            for ( sPredicate = sMyGraph->connectByCNF->oneTablePredicate;
                  sPredicate != NULL;
                  sPredicate = sPredicate->next )
            {
                IDE_TEST( searchTableID( sPredicate->node,
                                         &sTableID,
                                         &sFind )
                          != IDE_SUCCESS );
                if ( sFind == ID_TRUE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            sFind = ID_FALSE;
            sFindFrom = NULL;
            for ( sFrom = sMyGraph->graph.myFrom; sFrom != NULL; sFrom = sFrom->next )
            {
                IDE_TEST( searchFrom( sTableID, sFrom, &sFindFrom, &sFind )
                          != IDE_SUCCESS );
                if ( sFind == ID_TRUE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            /* BUG-47807 계층형 쿼리 수행시 optmize에서 FATAL */
            IDE_TEST_RAISE( sFindFrom == NULL, ERR_NOT_FOUND_FROM );

            // Predicate 재배치
            IDE_TEST(
                qmoPred::relocatePredicate(
                    aStatement,
                    sMyGraph->connectByCNF->oneTablePredicate,
                    & sMyGraph->graph.depInfo,
                    & qtc::zeroDependencies,
                    sFindFrom->tableRef->statInfo,
                    & sMyGraph->connectByCNF->oneTablePredicate )
                != IDE_SUCCESS );
            sTableRef = sFindFrom->tableRef;
        }
    }
    else
    {
        // To Fix PR-9050
        // CONNECT BY절에 반드시 One Table Predicate이 존재함을
        // 보장할 수 없다. 즉, 다음과 같은 Predicate이 존재할 수 있다.
        // Ex) CONNECT BY PRIOR a1 = PRIOR a2
        // Ex) CONNECT BY 1 = 1

        // nothing to do
    }

    /* PROJ-2641 Hierarchy Index Support */
    if ( ( sTableRef->view == NULL ) &&
         ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
           == QMG_HIERARCHY_JOIN_FALSE ) )
    {
        if ( sMyGraph->mSelectedStartWithMethod == NULL )
        {
            IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmoAccessMethod),
                                                   (void**)&sMyGraph->mSelectedStartWithMethod)
                     != IDE_SUCCESS );
            sMyGraph->mSelectedStartWithMethod->method = NULL;

            qmgSelection::setFullScanMethod( aStatement,
                                             sTableRef->statInfo,
                                             NULL,
                                             sMyGraph->mSelectedStartWithMethod,
                                             1,
                                             ID_FALSE );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmoAccessMethod) *
                                               (sIndexCnt + 1),
                                               (void**)&sMyGraph->mConnectByAccessMethod)
                 != IDE_SUCCESS );

        sFlag &= ~QMG_BEST_ACCESS_METHOD_HIERARCHY_MASK;
        sFlag |= QMG_BEST_ACCESS_METHOD_HIERARCHY_TRUE;
        IDE_TEST( qmgSelection::getBestAccessMethod( aStatement,
                                                     & sMyGraph->graph,
                                                     sTableRef->statInfo,
                                                     sMyGraph->connectByCNF->oneTablePredicate,
                                                     sMyGraph->mConnectByAccessMethod,
                                                     & sMyGraph->mSelectedConnectByMethod,
                                                     & sMyGraph->mConnectByMethodCnt,
                                                     & sMyGraph->mSelectedConnectByIdx,
                                                     & sSelectedScanHint,
                                                     1,
                                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------------------------
    // Predicate의 재배치 및 개별 Predicate의 Selectivity 계산
    //---------------------------------------------------

    if ( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::relocatePredicate(
                      aStatement,
                      sMyGraph->graph.myPredicate,
                      & sMyGraph->graph.depInfo,
                      & qtc::zeroDependencies,
                      sFindFrom->tableRef->statInfo,
                      & sMyGraph->graph.myPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // 공통 비용 정보의 설정
    // output record count = ( input record count * input record count )
    //                       * selectivity
    // output page count = record size * output record count / page size
    //---------------------------------------------------
 
    if ( ( sTableRef->view != NULL ) ||
         ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
            == QMG_HIERARCHY_JOIN_TRUE ) )
    {
        // recordSize 설정
        sMyGraph->graph.costInfo.recordSize =
            sMyGraph->graph.left->costInfo.recordSize;

        // input Record Cnt 설정
        sMyGraph->graph.costInfo.inputRecordCnt = sInputRecordCnt =
            sMyGraph->graph.left->costInfo.outputRecordCnt;

        // selectivity 설정
        IDE_TEST( qmoSelectivity::setHierarchySelectivity(
                      aStatement,
                      sMyGraph->graph.myPredicate,
                      sMyGraph->connectByCNF,
                      sMyGraph->startWithCNF,
                      ID_FALSE,    // execution time
                      & sMyGraph->graph.costInfo.selectivity )
                  != IDE_SUCCESS );

        if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
             == QMG_HIERARCHY_JOIN_FALSE )
        {
            // output Record Cnt 설정
            sOutputRecordCnt = sInputRecordCnt * sInputRecordCnt *
                sMyGraph->graph.costInfo.selectivity;
        }
        else
        {
            sOutputRecordCnt = sInputRecordCnt * sMyGraph->graph.costInfo.selectivity;
        }

        sOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

        sMyGraph->graph.costInfo.outputRecordCnt = sOutputRecordCnt;

        /*  Cost */
        if ( sMyGraph->graph.myPredicate != NULL )
        {
            sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                                  sMyGraph->graph.myPredicate,
                                                  sOutputRecordCnt );
        }
        else
        {
            sFilterCost = 0;
        }

        sSortCost = qmoCost::getMemSortTempCost( aStatement->mSysStat,
                                                 &(sMyGraph->graph.left->costInfo),
                                                 1 );

        sMyGraph->graph.costInfo.myAccessCost = sFilterCost + sSortCost;
        sMyGraph->graph.costInfo.myDiskCost   = 0;

        sMyGraph->graph.costInfo.myAllCost =
            sMyGraph->graph.costInfo.myAccessCost +
            sMyGraph->graph.costInfo.myDiskCost;

        // total cost 설정
        sMyGraph->graph.costInfo.totalAccessCost =
            sMyGraph->graph.left->costInfo.totalAccessCost +
            sMyGraph->graph.costInfo.myAccessCost;
        sMyGraph->graph.costInfo.totalDiskCost =
            sMyGraph->graph.left->costInfo.totalDiskCost +
            sMyGraph->graph.costInfo.myDiskCost;
        sMyGraph->graph.costInfo.totalAllCost =
            sMyGraph->graph.left->costInfo.totalAllCost +
            sMyGraph->graph.costInfo.myAllCost;
    }
    else
    {
        sColumns   = sTableRef->tableInfo->columns;
        sColumnCnt = sTableRef->tableInfo->columnCount;

        for ( i = 0; i < sColumnCnt; i++ )
        {
            sRecordSize += sColumns[i].basicInfo->column.size;
        }

        sRecordSize = IDL_MAX( sRecordSize, 1 );
        sMyGraph->graph.costInfo.recordSize = sRecordSize;

        // input Record Cnt 설정
        sMyGraph->graph.costInfo.inputRecordCnt = sInputRecordCnt =
            sMyGraph->graph.myFrom->tableRef->statInfo->totalRecordCnt;

        // selectivity 설정
        IDE_TEST( qmoSelectivity::setHierarchySelectivity(
                      aStatement,
                      sMyGraph->graph.myPredicate,
                      sMyGraph->connectByCNF,
                      sMyGraph->startWithCNF,
                      ID_FALSE,    // execution time
                      & sMyGraph->graph.costInfo.selectivity )
                  != IDE_SUCCESS );

        // output Record Cnt 설정
        sOutputRecordCnt = sInputRecordCnt * sInputRecordCnt *
            sMyGraph->graph.costInfo.selectivity;
        sOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

        sMyGraph->graph.costInfo.outputRecordCnt = sOutputRecordCnt;

        /*  Cost */
        sMyGraph->graph.costInfo.myAccessCost = sMyGraph->mSelectedStartWithMethod->accessCost +
            sMyGraph->mSelectedConnectByMethod->accessCost;
        sMyGraph->graph.costInfo.myDiskCost   = sMyGraph->mSelectedStartWithMethod->diskCost +
            sMyGraph->mSelectedConnectByMethod->diskCost;
        sMyGraph->graph.costInfo.myAllCost    = sMyGraph->graph.costInfo.myAccessCost +
            sMyGraph->graph.costInfo.myDiskCost;

        // total cost 설정
        sMyGraph->graph.costInfo.totalAccessCost = sMyGraph->graph.costInfo.myAccessCost;
        sMyGraph->graph.costInfo.totalDiskCost   = sMyGraph->graph.costInfo.myDiskCost;
        sMyGraph->graph.costInfo.totalAllCost    = sMyGraph->graph.costInfo.myAllCost;
    }

    //---------------------------------------------------
    // Preserved Order 설정
    //---------------------------------------------------

    sMyGraph->graph.flag &= ~QMG_PRESERVED_ORDER_MASK;
    sMyGraph->graph.flag |= QMG_PRESERVED_ORDER_NEVER;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_FROM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgHierarchy::optimize",
                                  "Not found From" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qmgHierarchy::optimizeStartWith( qcStatement * aStatement,
                                        qmgHIER     * aMyGraph,
                                        qmsTableRef * aTableRef,
                                        UInt          aIndexCnt )
{
    UInt          sSelectedScanHint = 0;
    qmsTableRef * sTableRef         = NULL;
    UShort        sTableID          = 0;
    idBool        sFind             = ID_FALSE;
    qmsFrom     * sFrom             = NULL;
    qmsFrom     * sFindFrom         = NULL;

    IDU_FIT_POINT_FATAL( "qmgHierarchy::optimizeStartWith::__FT__" );

    //---------------------------------------------------
    // start with절이 존재하는 경우,
    // start with의 access method 선택
    //---------------------------------------------------

    if ( ( aMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
         == QMG_HIERARCHY_JOIN_FALSE )
    {
        // Predicate 분류
        IDE_TEST(
            qmoCnfMgr::classifyPred4StartWith( aStatement,
                                               aMyGraph->startWithCNF,
                                               &aMyGraph->startWithCNF->depInfo )
            != IDE_SUCCESS );
    }
    else
    {
        // Predicate 분류
        IDE_TEST(
            qmoCnfMgr::classifyPred4StartWith( aStatement,
                                               aMyGraph->startWithCNF,
                                               &aMyGraph->graph.myQuerySet->depInfo )
            != IDE_SUCCESS );
    }

    // To Fix BUG-9050
    if ( aMyGraph->startWithCNF->oneTablePredicate != NULL )
    {
        if ( ( aMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
             == QMG_HIERARCHY_JOIN_FALSE )
        {
            sTableRef = aTableRef;
        }
        else
        {
            IDE_TEST( searchTableID( aMyGraph->startWithCNF->oneTablePredicate->node,
                                     &sTableID,
                                     &sFind )
                      != IDE_SUCCESS );

            sFind = ID_FALSE;
            for ( sFrom = aMyGraph->graph.myFrom; sFrom != NULL; sFrom = sFrom->next )
            {
                IDE_TEST( searchFrom( sTableID, sFrom, &sFindFrom, &sFind )
                          != IDE_SUCCESS );
                if ( sFind == ID_TRUE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            if ( sFindFrom != NULL )
            {
                sTableRef = sFindFrom->tableRef;
            }
            else
            {
                /* Nothing to do */
            }

            /* BUG-47807 계층형 쿼리 수행시 optmize에서 FATAL */
            IDE_TEST_RAISE( sTableRef == NULL, ERR_NOT_FOUND_TABLE_REF );
        }

        // Predicate 재배치
        IDE_TEST(
            qmoPred::relocatePredicate(
                aStatement,
                aMyGraph->startWithCNF->oneTablePredicate,
                & aMyGraph->graph.depInfo,
                & qtc::zeroDependencies,
                sTableRef->statInfo,
                & aMyGraph->startWithCNF->oneTablePredicate )
            != IDE_SUCCESS );
       
        /* PROJ-2641 Hierarchy Query Index Support */
        if ( ( sTableRef->view == NULL ) &&
             ( ( aMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
             == QMG_HIERARCHY_JOIN_FALSE ) )
        {
            // rid predicate 이 있는 경우 무조건 rid scan 을 시도한다.
            // rid predicate 이 있더라도  rid scan 을 할수 없는 경우도 있다.
            // 이 경우에도 index scan 이 되지 않고 full scan 을 하게 된다.
            //---------------------------------------------------------------
            if ( aMyGraph->graph.ridPredicate != NULL )
            {
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmoAccessMethod),
                                                       (void**)&aMyGraph->mStartWithAccessMethod)
                         != IDE_SUCCESS);

                aMyGraph->mStartWithAccessMethod->method               = NULL;
                aMyGraph->mStartWithAccessMethod->keyRangeSelectivity  = 0;
                aMyGraph->mStartWithAccessMethod->keyFilterSelectivity = 0;
                aMyGraph->mStartWithAccessMethod->filterSelectivity    = 0;
                aMyGraph->mStartWithAccessMethod->methodSelectivity    = 0;

                aMyGraph->mStartWithAccessMethod->totalCost = qmoCost::getTableRIDScanCost(
                    sTableRef->statInfo,
                    &aMyGraph->mStartWithAccessMethod->accessCost,
                    &aMyGraph->mStartWithAccessMethod->diskCost );
                aMyGraph->mSelectedStartWithIdx = NULL;
            }
            else
            {
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qmoAccessMethod) *
                                                       (aIndexCnt + 1),
                                                       (void**)&aMyGraph->mStartWithAccessMethod)
                         != IDE_SUCCESS );

                IDE_TEST( qmgSelection::getBestAccessMethod( aStatement,
                                                             & aMyGraph->graph,
                                                             sTableRef->statInfo,
                                                             aMyGraph->startWithCNF->oneTablePredicate,
                                                             aMyGraph->mStartWithAccessMethod,
                                                             & aMyGraph->mSelectedStartWithMethod,
                                                             & aMyGraph->mStartWithMethodCnt,
                                                             & aMyGraph->mSelectedStartWithIdx,
                                                             & sSelectedScanHint,
                                                             1,
                                                             0 )
                          != IDE_SUCCESS );
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
    
    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE_REF )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmgHierarchy::optimizeStartWith",
                                  "Not found Table Ref" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmgHierarchy::makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph )
{
/***********************************************************************
 *
 * Description : qmgHierachy로 부터 Plan을 생성한다.
 *
 * Implementation :
 *
 *     - qmgHierachy로 부터 생성가능한 Plan
 *       1) One Table
 *       ( [FILT] ) : WHERE절의 처리 (qmgHIER.graph.myPredicate )a
 *           |
 *         [CNBY]   : Connect By 절의 처리
 *           |
 *         [SCAN]   : Start With절의 처리
 *                    Constant Predicate의 처리
 *
 *       2) View
 *       ( [FILT] ) : WHERE절의 처리 (qmgHIER.graph.myPredicate )
 *           |
 *         [CNBY]   : Connect By 절의 처리
 *                    Start With절의 처
 *                    Constant Predicate의 처리
 *
 *
 ***********************************************************************/

    qmgHIER         * sMyGraph = NULL;
    qmnPlan         * sCNBY = NULL;
    qmnPlan         * sFILT = NULL;
    qmnPlan         * sSCAN = NULL;
    qtcNode         * sFilter = NULL;
    qmoPredicate    * sConstantPredicate = NULL;
    qmoLeafInfo       sLeafInfo[2];
    qmoSCANInfo       sSCANInfo;
    qtcNode         * sLobFilter = NULL;

    IDU_FIT_POINT_FATAL( "qmgHierarchy::makePlan::__FT__" );

    //---------------------------------------------------
    // 적합성 검사
    //---------------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    sMyGraph = (qmgHIER*) aGraph;

    //---------------------------------------------------
    // Current CNF의 등록
    //---------------------------------------------------

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
    aGraph->flag |= (aParent->flag & QMG_PARALLEL_IMPOSSIBLE_MASK);

    if ( aGraph->left != NULL )
    {
        // BUG-38410
        // SCAN parallel flag 를 자식 노드로 물려준다.
        aGraph->left->flag |= (aGraph->flag & QMG_PLAN_EXEC_REPEATED_MASK);
    }
    else
    {
        /* Nothing to do */
    }

    sMyGraph->graph.myPlan = aParent->myPlan;

    //---------------------------------------------------
    // FILT 노드의 생성
    //---------------------------------------------------

    //FILT노드의 생성
    if( sMyGraph->graph.myPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                sMyGraph->graph.myPredicate ,
                                                &sFilter ) != IDE_SUCCESS );

        // BUG-35155 Partial CNF
        if ( sMyGraph->graph.nnfFilter != NULL )
        {
            IDE_TEST( qmoPred::addNNFFilter4linkedFilter(
                          aStatement,
                          sMyGraph->graph.nnfFilter,
                          & sFilter )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        if ( sMyGraph->graph.nnfFilter != NULL )
        {
            sFilter = sMyGraph->graph.nnfFilter;
        }
        else
        {
            sFilter = NULL;
        }
    }

    if ( sFilter != NULL )
    {
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           sFilter ) != IDE_SUCCESS );

        IDE_TEST( qmoOneNonPlan::initFILT( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           sFilter ,
                                           sMyGraph->graph.myPlan ,
                                           &sFILT )
                  != IDE_SUCCESS );
        sMyGraph->graph.myPlan = sFILT;
    }
    else
    {
        // Nothing To Do
    }

    //Predicate의 통합
    if( sMyGraph->graph.constantPredicate != NULL )
    {
        sConstantPredicate = sMyGraph->graph.constantPredicate;
        if( sMyGraph->startWithCNF->constantPredicate != NULL )
        {
            while( sConstantPredicate->next != NULL )
            {
                sConstantPredicate = sConstantPredicate->next;
            }
            sConstantPredicate->next =
                sMyGraph->startWithCNF->constantPredicate;
        }
        else
        {
            //nothing to do
        }
        sConstantPredicate = sMyGraph->graph.constantPredicate;
    }
    else
    {
        if( sMyGraph->startWithCNF->constantPredicate != NULL )
        {
            sConstantPredicate = sMyGraph->startWithCNF->constantPredicate;
        }
        else
        {
            sConstantPredicate = NULL;
        }
    }

    if ( ( sMyGraph->graph.left == NULL ) &&
         ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
           == QMG_HIERARCHY_JOIN_FALSE ) )
    {
        /* CNBY 노드의 생성 */
        sLeafInfo[0].predicate         = NULL;
        sLeafInfo[0].levelPredicate    = NULL;
        sLeafInfo[0].constantPredicate = NULL;
        sLeafInfo[0].ridPredicate      = NULL;
        sLeafInfo[0].connectByRownumPred = NULL; /* BUG-39434 Start with의 rownum 은 constant 로 처리된다. */
        sLeafInfo[0].index             = NULL;
        sLeafInfo[0].preservedOrder    = NULL;
        sLeafInfo[0].sdf               = NULL;
        sLeafInfo[0].nnfFilter         = NULL;
        sLeafInfo[0].forceIndexScan    = ID_FALSE;

        sLeafInfo[1].predicate         = sMyGraph->connectByCNF->oneTablePredicate;
        sLeafInfo[1].constantPredicate = sMyGraph->connectByCNF->constantPredicate;
        sLeafInfo[1].levelPredicate    = sMyGraph->connectByCNF->levelPredicate;
        sLeafInfo[1].ridPredicate      = NULL;
        sLeafInfo[1].connectByRownumPred  = sMyGraph->connectByCNF->connectByRownumPred;
        sLeafInfo[1].index             = sMyGraph->mSelectedConnectByIdx;
        sLeafInfo[1].preservedOrder    = NULL;
        sLeafInfo[1].sdf               = NULL;
        sLeafInfo[1].nnfFilter         = NULL;
        sLeafInfo[1].forceIndexScan    = ID_FALSE;

        IDE_TEST( qmoOneMtrPlan::initCNBY( aStatement,
                                           sMyGraph->graph.myQuerySet,
                                           sLeafInfo,
                                           sMyGraph->myHierarchy->siblings,
                                           sMyGraph->graph.myPlan ,
                                           &sCNBY)
                  != IDE_SUCCESS );
        sMyGraph->graph.myPlan = sCNBY;

        sSCANInfo.flag = QMO_SCAN_INFO_INITIALIZE;
        if ( ( sMyGraph->graph.flag & QMG_SELT_NOTNULL_KEYRANGE_MASK ) ==
             QMG_SELT_NOTNULL_KEYRANGE_TRUE )
        {
            sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
            sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_TRUE;
        }
        else
        {
            // To Fix PR-10288
            // NOTNULL KEY RANGE가 아닌 경우로 반드시 설정해 주어야 함.
            sSCANInfo.flag &= ~QMO_SCAN_INFO_NOTNULL_KEYRANGE_MASK;
            sSCANInfo.flag |= QMO_SCAN_INFO_NOTNULL_KEYRANGE_FALSE;
        }

        sSCANInfo.flag &= ~QMO_SCAN_INFO_FORCE_RID_SCAN_MASK;
        sSCANInfo.flag |= QMO_SCAN_INFO_FORCE_RID_SCAN_FALSE;

        sSCANInfo.predicate         = sMyGraph->startWithCNF->oneTablePredicate;
        sSCANInfo.constantPredicate = sConstantPredicate;
        sSCANInfo.ridPredicate      = sMyGraph->graph.ridPredicate;
        sSCANInfo.limit             = NULL;
        sSCANInfo.index             = sMyGraph->mSelectedStartWithIdx;
        sSCANInfo.preservedOrder    = sMyGraph->graph.preservedOrder;
        sSCANInfo.sdf               = NULL;
        sSCANInfo.nnfFilter         = sMyGraph->startWithCNF->nnfFilter;
        sSCANInfo.mParallelInfo.mDegree = 1;
        sSCANInfo.mParallelInfo.mSeqNo  = 1;

        IDE_TEST( qmoOneNonPlan::makeSCAN( aStatement,
                                           sMyGraph->graph.myQuerySet,
                                           sMyGraph->graph.myFrom,
                                           &sSCANInfo,
                                           sMyGraph->graph.myPlan,
                                           &sSCAN )
                  != IDE_SUCCESS);
        sMyGraph->graph.myPlan = sSCAN;
        qmg::setPlanInfo( aStatement, sSCAN, &(sMyGraph->graph) );

        sLobFilter = ((qmncSCAN*)sSCAN)->method.lobFilter;

        IDE_TEST_RAISE( sLobFilter != NULL, ERR_NOT_SUPPORT_LOB_FILTER );
    }
    else /* PROJ-1715 View 인 경우 */
    {
        /* CNBY 노드의 생성 */
        sLeafInfo[0].predicate         = sMyGraph->startWithCNF->oneTablePredicate;
        sLeafInfo[0].levelPredicate    = NULL;
        sLeafInfo[0].constantPredicate = sConstantPredicate;
        sLeafInfo[0].ridPredicate      = NULL;
        sLeafInfo[0].connectByRownumPred = NULL; /* BUG-39434 Start with의 rownum 은 constant 로 처리된다. */
        sLeafInfo[0].index             = NULL;
        sLeafInfo[0].preservedOrder    = NULL;
        sLeafInfo[0].sdf               = NULL;
        sLeafInfo[0].nnfFilter         = sMyGraph->startWithCNF->nnfFilter;
        sLeafInfo[0].forceIndexScan    = ID_FALSE;

        sLeafInfo[1].predicate         = sMyGraph->connectByCNF->oneTablePredicate;
        sLeafInfo[1].constantPredicate = sMyGraph->connectByCNF->constantPredicate;
        sLeafInfo[1].levelPredicate    = sMyGraph->connectByCNF->levelPredicate;
        sLeafInfo[1].ridPredicate      = NULL;
        sLeafInfo[1].connectByRownumPred  = sMyGraph->connectByCNF->connectByRownumPred;
        sLeafInfo[1].index             = NULL;
        sLeafInfo[1].preservedOrder    = NULL;
        sLeafInfo[1].sdf               = NULL;
        sLeafInfo[1].nnfFilter         = NULL;
        sLeafInfo[1].forceIndexScan    = ID_FALSE;

        if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
             == QMG_HIERARCHY_JOIN_FALSE )
        {
            IDE_TEST( qmoOneMtrPlan::initCNBY( aStatement,
                                               sMyGraph->graph.myQuerySet,
                                               sLeafInfo,
                                               sMyGraph->myHierarchy->siblings,
                                               sMyGraph->graph.myPlan ,
                                               &sCNBY)
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmoOneMtrPlan::initCNBYForJoin( aStatement,
                                                      sMyGraph->graph.myQuerySet,
                                                      sLeafInfo,
                                                      sMyGraph->myHierarchy->siblings,
                                                      sMyGraph->graph.myPlan ,
                                                      &sCNBY )
                      != IDE_SUCCESS );
        }

        sMyGraph->graph.myPlan = sCNBY;

        if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
             == QMG_HIERARCHY_JOIN_FALSE )
        {
            IDE_DASSERT( sMyGraph->graph.left->type == QMG_PROJECTION );

            if ( sMyGraph->graph.left->myPlan == NULL )
            {
                /* 항상 Memeory Temp Table 을 이용한다. */
                sMyGraph->graph.left->flag &= ~QMG_GRAPH_TYPE_MASK;
                sMyGraph->graph.left->flag |= QMG_GRAPH_TYPE_MEMORY;
                IDE_TEST( sMyGraph->graph.left->makePlan(
                              sMyGraph->graph.myFrom->tableRef->view,
                              &sMyGraph->graph,
                              sMyGraph->graph.left)
                          != IDE_SUCCESS );

                sMyGraph->graph.myPlan = sMyGraph->graph.left->myPlan;
            }
            else
            {
                sMyGraph->graph.myPlan = sMyGraph->graph.left->myPlan;
            }
        }
        else
        {
            if ( sMyGraph->graph.left != NULL )
            {
                IDE_TEST( sMyGraph->graph.left->makePlan( aStatement ,
                                                          &sMyGraph->graph,
                                                          sMyGraph->graph.left )
                          != IDE_SUCCESS);
                sMyGraph->graph.myPlan = sMyGraph->graph.left->myPlan;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    if ( ( sMyGraph->graph.flag & QMG_HIERARCHY_JOIN_MASK )
         == QMG_HIERARCHY_JOIN_FALSE )
    {
        IDE_TEST( qmoOneMtrPlan::makeCNBY( aStatement,
                                           sMyGraph->graph.myQuerySet,
                                           sMyGraph->graph.myFrom,
                                           &sLeafInfo[0],
                                           &sLeafInfo[1],
                                           sMyGraph->graph.myPlan,
                                           sCNBY)
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmoOneMtrPlan::makeCNBYForJoin( aStatement,
                                                  sMyGraph->graph.myQuerySet,
                                                  sMyGraph->myHierarchy->siblings,
                                                  &sLeafInfo[0],
                                                  &sLeafInfo[1],
                                                  sMyGraph->graph.myPlan,
                                                  sCNBY)
                  != IDE_SUCCESS );

    }
    sMyGraph->graph.myPlan = sCNBY;

    qmg::setPlanInfo( aStatement, sCNBY, &(sMyGraph->graph) );

    if ( sFilter != NULL )
    {
        IDE_TEST( qmoOneNonPlan::makeFILT( aStatement ,
                                           sMyGraph->graph.myQuerySet ,
                                           sFilter ,
                                           NULL ,
                                           sMyGraph->graph.myPlan ,
                                           sFILT )
                  != IDE_SUCCESS );
        sMyGraph->graph.myPlan = sFILT;

        qmg::setPlanInfo( aStatement, sFILT, &(sMyGraph->graph) );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_LOB_FILTER ) 
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMO_NOT_ALLOWED_LOB_FILTER ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC
qmgHierarchy::printGraph( qcStatement  * aStatement,
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

    IDU_FIT_POINT_FATAL( "qmgHierarchy::printGraph::__FT__" );

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

    if ( aGraph->left != NULL )
    {
        //-----------------------------------
        // Child Graph 고유 정보의 출력
        //-----------------------------------
        IDE_TEST( aGraph->left->printGraph( aStatement,
                                            aGraph->left,
                                            aDepth + 1,
                                            aString )
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

IDE_RC qmgHierarchy::searchTableID( qtcNode * aNode,
                                    UShort  * aTableID,
                                    idBool  * aFind )
{
    if ( *aFind == ID_FALSE )
    {
        /* BUG-47807 계층형 쿼리 수행시 optmize에서 FATAL */
        if ( ( aNode->node.module == &qtc::columnModule ) &&
             ( QTC_IS_PSEUDO( aNode ) == ID_FALSE ) )
        {
            *aTableID = aNode->node.table;
            if ( ( aNode->lflag & QTC_NODE_PRIOR_MASK) ==
                   QTC_NODE_PRIOR_EXIST )
            {
                *aFind = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( aNode->node.arguments != NULL )
            {
                IDE_TEST( searchTableID( (qtcNode *)aNode->node.arguments,
                                         aTableID,
                                         aFind )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            if ( aNode->node.next != NULL )
            {
                IDE_TEST( searchTableID( (qtcNode *)aNode->node.next,
                                         aTableID,
                                         aFind )
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmgHierarchy::searchFrom( UShort     aTableID,
                                 qmsFrom  * aFrom,
                                 qmsFrom ** aFindFrom,
                                 idBool   * aFind )
{
    qmsFrom * sFrom = aFrom;

    if ( *aFind == ID_FALSE )
    {
        if ( sFrom->joinType == QMS_NO_JOIN )
        {
            if ( sFrom->tableRef->table == aTableID )
            {
                *aFindFrom = sFrom;
                *aFind = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( sFrom->left != NULL )
            {
                IDE_TEST( searchFrom( aTableID,
                                      sFrom->left,
                                      aFindFrom,
                                      aFind )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            if ( sFrom->right != NULL )
            {
                IDE_TEST( searchFrom( aTableID,
                                      sFrom->right,
                                      aFindFrom,
                                      aFind )
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

