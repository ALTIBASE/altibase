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
 * $Id: qmoCrtPathMgr.cpp 89925 2021-02-03 04:40:48Z ahra.cho $
 *
 * Description :
 *     Critical Path Manager
 *
 *     FROM, WHERE로 구성되는 Critical Path에 대한 최적화를 수행하고
 *     이에 대한 Graph를 생성한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qcg.h>
#include <qmoCrtPathMgr.h>
#include <qmoNormalForm.h>
#include <qmgCounting.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmoOuterJoinOper.h>
#include <qmoAnsiJoinOrder.h>
#include <qmoOuterJoinElimination.h>
#include <qmoInnerJoinPushDown.h>
#include <qmv.h>
#include <qmoCostDef.h>
#include <qmgHierarchy.h>

IDE_RC
qmoCrtPathMgr::init( qcStatement * aStatement,
                     qmsQuerySet * aQuerySet,
                     qmoCrtPath ** aCrtPath )
{
/***********************************************************************
 *
 * Description : qmoCrtPath 생성 및 초기화
 *
 * Implementation :
 *    (1) qmoCrtPath 만큼 메모리 할당
 *    (2) Normalization Type 결정
 *    (3) Normalization Type에 따라 qmoCNF, qmoDNF 초기화
 *        A. qmoCNF 또는 qmoDNF 메모리 할당
 *        B. where 절을 CNF 또는 DNF Normalize
 *        C. qmoCnfMgr또는 qmoDnfMgr의 초기화 함수 호출
 *
 ***********************************************************************/

    qmsQuerySet        * sQuerySet;
    qmoCrtPath         * sCrtPath;
    qmoNormalType        sNormalType;
    qtcNode            * sWhere;
    qmsFrom            * sFrom;
    qtcNode            * sNormalForm;
    qmoCNF             * sCNF;
    qmoDNF             * sDNF;
    // BUG-34295 Join ordering ANSI style query
    qmsFrom            * sFromArr;
    qmsFrom            * sFromTree;
    idBool               sMakeFail;
    qmsFrom            * sIter;
    qcDepInfo            sFromArrDep;
    idBool               sCrossProduct;
    qtcNode            * sNode;
    qtcNode            * sNNFFilter;
    idBool               sCNFOnly;
    idBool               sChanged;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    sQuerySet   = aQuerySet;
    sWhere      = sQuerySet->SFWGH->where;
    sFrom       = sQuerySet->SFWGH->from;

    sCrtPath    = NULL;
    sNormalForm = NULL;
    sNNFFilter  = NULL;

    // qmoCrtPath 메모리 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoCrtPath) ,
                                             (void **) & sCrtPath)
              != IDE_SUCCESS);

    // qmoCrtPath 초기화
    sCrtPath->crtDNF     = NULL;
    sCrtPath->crtCNF     = NULL;
    sCrtPath->currentCNF = NULL;
    sCrtPath->myGraph    = NULL;

    sCrtPath->currentNormalType = QMO_NORMAL_TYPE_NOT_DEFINED;

    sCrtPath->rownumPredicateForCNF = NULL;
    sCrtPath->rownumPredicateForDNF = NULL;
    sCrtPath->rownumPredicate       = NULL;

    sCrtPath->myQuerySet = aQuerySet;
    sCrtPath->mIsOnlyNL = ID_FALSE;
    // SFWGH에 Critical Path 를 등록
    sQuerySet->SFWGH->crtPath = sCrtPath;

    // BUG-36926
    // updatable view는 CNF로만 plan이 생성되어야 한다.
    if ( ( sQuerySet->SFWGH->lflag & QMV_SFWGH_UPDATABLE_VIEW_MASK )
         == QMV_SFWGH_UPDATABLE_VIEW_TRUE )
    {
        sCNFOnly = ID_TRUE;
    }
    else
    {
        // BUG-43894
        // join는 CNF로만 plan이 생성되어야 한다.
        if ( ( aQuerySet->SFWGH->from->next == NULL ) &&
             ( aQuerySet->SFWGH->from->joinType == QMS_NO_JOIN ) )
        {
            sCNFOnly = ID_FALSE;
        }
        else
        {
            sCNFOnly = ID_TRUE;
        }
    }

    // Normalization Type 결정
    // BUG-35155 Partial CNF
    // Where 절만 partial CNF 처리를 위해 별도의 함수를 호출한다.
    IDE_TEST( decideNormalType4Where( aStatement,
                                      sFrom,
                                      sWhere,
                                      sQuerySet->SFWGH->hints,
                                      sCNFOnly,
                                      &sNormalType )
              != IDE_SUCCESS );

    sCrtPath->normalType = sNormalType;

    //------------------------------------------------
    // Normalization Type에 따라 qmoCNF, qmoDNF 초기화
    //------------------------------------------------

    switch ( sNormalType )
    {
        case QMO_NORMAL_TYPE_NNF:

            //-----------------------------------------
            // NNF 인 경우
            //-----------------------------------------

            // qmoCNF 메모리 할당
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ),
                                                     (void **) & sCNF )
                      != IDE_SUCCESS);

            if ( sWhere != NULL )
            {
                sWhere->lflag &= ~QTC_NODE_NNF_FILTER_MASK;
                sWhere->lflag |= QTC_NODE_NNF_FILTER_TRUE;
            }
            else
            {
                // Nothing To Do
            }

            // critical path는 현재 NNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_NNF;

            // qmoCNF 초기화
            IDE_TEST( qmoCnfMgr::init( aStatement,
                                       sCNF,
                                       sQuerySet,
                                       NULL,
                                       sWhere )
                      != IDE_SUCCESS );

            sCrtPath->crtCNF = sCNF;

            break;
        case QMO_NORMAL_TYPE_CNF:

            //-----------------------------------------
            // CNF 인 경우
            //-----------------------------------------

            // qmoCNF 메모리 할당
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ),
                                                     (void **) & sCNF )
                      != IDE_SUCCESS);

            // where 절을 CNF Normalization
            if ( sWhere == NULL )
            {
                sNormalForm = NULL;
            }
            else
            {
                // where 절을 CNF Normalization
                IDE_TEST( qmoNormalForm::normalizeCNF( aStatement,
                                                       sWhere,
                                                       &sNormalForm,
                                                       ID_TRUE, /* aIsWhere */
                                                       sQuerySet->SFWGH->hints )
                          != IDE_SUCCESS );

                // BUG-35155 Partial CNF
                // Partial CNF 에서 제외된 qtcNode 를 NNF 필터로 만든다.
                IDE_TEST( qmoNormalForm::extractNNFFilter4CNF( aStatement,
                                                               sWhere,
                                                               &sNNFFilter )
                          != IDE_SUCCESS );

                //-----------------------------------------
                // PROJ-1653 Outer Join Operator (+)
                //
                // Outer Join Operator 를 사용하는 normalizedCNF 의 경우
                // ANSI Join 형태의 자료구조로 변경
                //-----------------------------------------
                if ( ( sNormalForm->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                        == QTC_NODE_JOIN_OPERATOR_EXIST )
                {
                    // BUGBUG PROJ-1718 Subquery unnesting
                    // LEFT OUTER JOIN의 조건이 WHERE절과 ON절에 동시에 남아있어 unparsing 시
                    // 중복되어 출력된다.
                    // WHERE절에 포함된 LEFT OUTER JOIN의 조건은 제거되어야 한다.
                    IDE_TEST( qmoOuterJoinOper::transform2ANSIJoin ( aStatement,
                                                                     sQuerySet,
                                                                     &sNormalForm )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
            }

            //------------------------------------------
            // BUG-38375 Outer Join Elimination OracleStyle Join
            //------------------------------------------

            IDE_TEST( qmoOuterJoinElimination::doTransform( aStatement,
                                                            aQuerySet,
                                                            & sChanged )
                      != IDE_SUCCESS );

            //------------------------------------------
            // BUG-43039 inner join push down
            //------------------------------------------

            if ( QCU_OPTIMIZER_INNER_JOIN_PUSH_DOWN == 1 )
            {
                IDE_TEST( qmoInnerJoinPushDown::doTransform( aStatement,
                                                             sQuerySet )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do
            }

            qcgPlan::registerPlanProperty(
                aStatement,
                PLAN_PROPERTY_OPTIMIZER_INNER_JOIN_PUSH_DOWN );

            // BUG-34295 Join ordering ANSI style query
            // 제약사항
            // 1. Property 가 켜져 있어야 함
            // 2. from 절에 ANSI style join 1개만 존재해야 함
            // 3. table 1개만 쓰인경우는 처리하지 않음
            // 4. Right or full outer join 이 있으면 처리하지 않음
            if( ( QCU_OPTIMIZER_ANSI_JOIN_ORDERING == 1 ) &&
                ( aQuerySet->SFWGH->from->next == NULL ) &&
                ( aQuerySet->SFWGH->from->joinType != QMS_NO_JOIN ) &&
                ( ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_JOIN_FULL_OUTER ) != QMV_SFWGH_JOIN_FULL_OUTER ) &&
                  ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_JOIN_RIGHT_OUTER ) != QMV_SFWGH_JOIN_RIGHT_OUTER ) ) )
            {
                sCrossProduct = ID_FALSE;

                qtc::dependencyClear( &sFromArrDep );

                sFromArr  = NULL;
                sFromTree = NULL;
                sMakeFail = ID_FALSE;

                IDE_TEST( qmoAnsiJoinOrder::traverseFroms( aStatement,
                                                           aQuerySet->SFWGH->from,
                                                           & sFromTree,
                                                           & sFromArr,
                                                           & sFromArrDep,
                                                           & sMakeFail )
                          != IDE_SUCCESS );

                // BUG-40028
                // qmsFrom 의 tree 가 왼쪽으로 치우친(skewed)형태가 아닐 때는
                // ANSI_JOIN_ORDERING 해서는 안된다.
                if ( sMakeFail == ID_TRUE )
                {
                    sFromArr  = NULL;
                    sFromTree = NULL;
                }
                else
                {
                    // Nothing to do.
                }

                // sFromArr가 NULL인 경우: left outer join만 존재하는 경우
                // sFromTree가 NULL인 경우: inner join만 존재하는 경우
                // sFromArr가 NULL이 아닌 경우: inner join이 최소한 하나라도 존재하는 경우
                if( sFromArr != NULL )
                {
                    for( sIter = sFromArr; 
                         sIter != NULL; 
                         sIter = sIter->next )
                    {
                        if( sIter->onCondition != NULL )
                        {
                            if( qtc::dependencyContains( &sFromArrDep,
                                                         &sIter->onCondition->depInfo ) != ID_TRUE )
                            {
                                sCrossProduct = ID_TRUE;

                                break;
                            }
                        }

                        // fix INC000000004933
                        if( sIter->tableRef->view != NULL )
                        {
                            sCrossProduct = ID_TRUE;

                            break;
                        }
                    }

                    // check where clause
                    if ( sNormalForm != NULL )
                    {
                        sNode = (qtcNode *)sNormalForm->node.arguments;
                    }
                    else
                    {
                        sNode = NULL;
                    }

                    while( sNode != NULL )
                    {
                        // check FROM ARRAY contains sNode
                        if( sNode->depInfo.depCount > 1 )
                        {
                            sCrossProduct = ID_TRUE;

                            break;
                        }

                        sNode = (qtcNode *)sNode->node.next;
                    }

                    if( sCrossProduct != ID_TRUE )
                    {
                        qtc::dependencyClear( &aQuerySet->SFWGH->depInfo );

                        for( sIter = sFromArr;
                             sIter != NULL;
                             sIter = sIter->next )
                        {
                            qtc::dependencyOr( &aQuerySet->SFWGH->depInfo,
                                               &sIter->depInfo,
                                               &aQuerySet->SFWGH->depInfo );

                            if( sIter->onCondition != NULL )
                            {
                                if( sNormalForm == NULL )
                                {
                                    sNormalForm = sIter->onCondition;
                                }
                                else
                                {
                                    IDE_TEST( qmoNormalForm::addToMerge( sNormalForm,
                                                                         sIter->onCondition,
                                                                         & sNormalForm )
                                              != IDE_SUCCESS );
                                }
                            }
                        }

                        // ANSI style join 은 from tree 로 따로 저장한다.
                        aQuerySet->SFWGH->outerJoinTree = sFromTree;

                        // From 절을 추출된 from array 로 대체한다.
                        aQuerySet->SFWGH->from = sFromArr;
                        
                        /* BUG-48405 */
                        aQuerySet->lflag &= QMV_QUERYSET_ANSI_JOIN_ORDERING_MASK;
                        aQuerySet->lflag |= QMV_QUERYSET_ANSI_JOIN_ORDERING_TRUE;
                    }
                }
                else
                {
                    // pass
                }
            }
            else
            {
                // ANSI join ordering 을 할 수 없음
                // Nothing to do.
            }

            // critical path는 현재 CNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_CNF;

            // qmoCNF 초기화
            IDE_TEST( qmoCnfMgr::init( aStatement,
                                       sCNF,
                                       sQuerySet,
                                       sNormalForm,
                                       sNNFFilter )  // BUG-35155
                      != IDE_SUCCESS );

            sCrtPath->crtCNF = sCNF;
            break;

        case QMO_NORMAL_TYPE_DNF:

            //-----------------------------------------
            // DNF 인 경우
            //-----------------------------------------

            // qmoDNF 메모리 할당
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoDNF ),
                                                     (void **) & sDNF )
                      != IDE_SUCCESS);

            // where 절을 DNF Normalization
            IDE_TEST( qmoNormalForm::normalizeDNF( aStatement,
                                                   sWhere,
                                                   &sNormalForm )
                      != IDE_SUCCESS );

            // critical path는 현재 DNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_DNF;

            // qmoDNF 초기화
            IDE_TEST( qmoDnfMgr::init( aStatement,
                                       sDNF,
                                       sQuerySet,
                                       sNormalForm )
                      != IDE_SUCCESS );

            sCrtPath->crtDNF = sDNF;
            break;

        case QMO_NORMAL_TYPE_NOT_DEFINED:

            //-----------------------------------------
            // Normal Form이 정의되지 않은 경우
            //-----------------------------------------

            // qmoCNF 초기화
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ) ,
                                                     (void**) & sCNF )
                      != IDE_SUCCESS);

            // where 절을 CNF Normalization
            IDE_TEST( qmoNormalForm::normalizeCNF( aStatement,
                                                   sWhere,
                                                   &sNormalForm,
                                                   ID_TRUE, /* aIsWhere */
                                                   sQuerySet->SFWGH->hints )
                      != IDE_SUCCESS );

            // BUG-35155 Partial CNF
            // Partial CNF 에서 제외된 qtcNode 를 NNF 필터로 만든다.
            IDE_TEST( qmoNormalForm::extractNNFFilter4CNF( aStatement,
                                                           sWhere,
                                                           &sNNFFilter )
                      != IDE_SUCCESS );

            // critical path는 현재 CNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_CNF;

            IDE_TEST( qmoCnfMgr::init( aStatement,
                                       sCNF,
                                       sQuerySet,
                                       sNormalForm,
                                       sNNFFilter )  // BUG-35155
                      != IDE_SUCCESS );

            sCrtPath->crtCNF = sCNF;

            // qmoDNF 초기화
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoDNF ) ,
                                                     (void**) & sDNF )
                      != IDE_SUCCESS);

            IDE_TEST( qmoNormalForm::normalizeDNF( aStatement,
                                                   sWhere,
                                                   &sNormalForm )
                      != IDE_SUCCESS );

            // critical path는 현재 DNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_DNF;

            IDE_TEST( qmoDnfMgr::init( aStatement,
                                       sDNF,
                                       sQuerySet,
                                       sNormalForm )
                      != IDE_SUCCESS );

            sCrtPath->crtDNF = sDNF;
            break;

        default:
            IDE_FT_ASSERT( 0 );
            break;
    }

    // critical path는 처리가 끝났다.
    sCrtPath->currentNormalType = QMO_NORMAL_TYPE_NOT_DEFINED;

    // out 설정
    *aCrtPath = sCrtPath;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCrtPathMgr::optimize( qcStatement * aStatement,
                         qmoCrtPath  * aCrtPath )
{
/***********************************************************************
 *
 * Description : Critical Path의 최적화( 즉, qmoCrtPathMgr의 최적화)
 *
 * Implementation :
 *    Normalization Type에 따라 qmoCnfMgf또는 qmoDnfMgr의 최적화 함수 호출
 *
 ***********************************************************************/

    SDouble        sCnfCost;
    SDouble        sDnfCost;
    qmoNormalType  sNormalType;
    qmoCrtPath   * sCrtPath;
    qmsQuerySet  * sQuerySet;
    qmgGraph     * sMyGraph;
    qmsQuerySet  * sTmp;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCrtPath != NULL );

    sCrtPath = aCrtPath;
    sNormalType = sCrtPath->normalType;
    sQuerySet = sCrtPath->myQuerySet;

    //------------------------------------------
    // Normalization Form에 따른 최적화
    //------------------------------------------

    switch ( sNormalType )
    {
        case QMO_NORMAL_TYPE_NNF:

            //------------------------------------------
            // NNF 최적화
            //------------------------------------------

            // critical path는 현재 NNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_NNF;
            sCrtPath->crtCNF->mIsOnlyNL = sCrtPath->mIsOnlyNL;
            IDE_TEST( qmoCnfMgr::optimize( aStatement,
                                           sCrtPath->crtCNF )
                      != IDE_SUCCESS );
            break;

        case QMO_NORMAL_TYPE_CNF:

            //------------------------------------------
            // CNF 최적화
            //------------------------------------------

            // critical path는 현재 CNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_CNF;
            sCrtPath->crtCNF->mIsOnlyNL = sCrtPath->mIsOnlyNL;
            IDE_TEST( qmoCnfMgr::optimize( aStatement,
                                           sCrtPath->crtCNF )
                      != IDE_SUCCESS );
            break;

        case QMO_NORMAL_TYPE_DNF:

            //------------------------------------------
            // DNF 최적화
            //------------------------------------------

            // critical path는 현재 DNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_DNF;

            IDE_TEST( qmoDnfMgr::optimize( aStatement,
                                           sCrtPath->crtDNF,
                                           0 )
                      != IDE_SUCCESS );
            break;

        case QMO_NORMAL_TYPE_NOT_DEFINED:
            //------------------------------------------
            // 정의되지 않은 경우
            //------------------------------------------

            // critical path는 현재 CNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_CNF;
            sCrtPath->crtCNF->mIsOnlyNL = sCrtPath->mIsOnlyNL;
            // CNF 최적화
            IDE_TEST( qmoCnfMgr::optimize( aStatement,
                                           sCrtPath->crtCNF )
                      != IDE_SUCCESS );

            sCnfCost = sCrtPath->crtCNF->cost;

            /* BUG-47769  recursive with 구문이 2번이상 사용되고 distinct가 함께
             * 사용되었을 경우 FATAL 발생
             */
            if ( sCrtPath->crtCNF->myQuerySet != NULL )
            {
                sTmp = sCrtPath->crtCNF->myQuerySet;
                if ( sTmp->SFWGH != NULL )
                {
                    if ( sTmp->SFWGH->from != NULL )
                    {
                        if ( sTmp->SFWGH->from->tableRef != NULL )
                        {
                            if ( sTmp->SFWGH->from->tableRef->recursiveView != NULL )
                            {
                                sNormalType = QMO_NORMAL_TYPE_CNF;
                                break;
                            }
                        }
                    }
                }
            }

            // critical path는 현재 DNF로 처리한다.
            sCrtPath->currentNormalType = QMO_NORMAL_TYPE_DNF;

            // DNF 최적화
            IDE_TEST( qmoDnfMgr::optimize( aStatement,
                                           sCrtPath->crtDNF,
                                           sCnfCost )
                      != IDE_SUCCESS );

            sDnfCost = sCrtPath->crtDNF->cost;

            // CNF와 DNF의 cost 비교
            // BUG-42400 cost 비교시 매크로를 사용해야 합니다.
            if ( QMO_COST_IS_GREATER(sCnfCost, sDnfCost) == ID_TRUE )
            {
                sNormalType = QMO_NORMAL_TYPE_DNF;
                IDE_TEST( qmoCnfMgr::removeOptimizationInfo( aStatement,
                                                             sCrtPath->crtCNF )
                          != IDE_SUCCESS );
            }
            else // sCnfCost <= sDnfCost
            {
                sNormalType = QMO_NORMAL_TYPE_CNF;
                IDE_TEST( qmoDnfMgr::removeOptimizationInfo( aStatement,
                                                             sCrtPath->crtDNF )
                          != IDE_SUCCESS );
            }
            break;

        default:
            IDE_FT_ASSERT( 0 );
            break;
    }

    // critical path는 처리가 끝났다.
    sCrtPath->currentNormalType = QMO_NORMAL_TYPE_NOT_DEFINED;

    //------------------------------------------
    // 결정된 normalType 설정 및  결과 Graph 연결
    //------------------------------------------

    switch ( sNormalType )
    {
        case QMO_NORMAL_TYPE_NNF:
            sCrtPath->normalType      = QMO_NORMAL_TYPE_NNF;
            sCrtPath->myGraph         = sCrtPath->crtCNF->myGraph;
            sCrtPath->rownumPredicate = sCrtPath->rownumPredicateForCNF;
            break;
        case QMO_NORMAL_TYPE_CNF:
            sCrtPath->normalType      = QMO_NORMAL_TYPE_CNF;
            sCrtPath->myGraph         = sCrtPath->crtCNF->myGraph;
            sCrtPath->rownumPredicate = sCrtPath->rownumPredicateForCNF;
            break;
        case QMO_NORMAL_TYPE_DNF:
            sCrtPath->normalType      = QMO_NORMAL_TYPE_DNF;
            sCrtPath->myGraph         = sCrtPath->crtDNF->myGraph;
            sCrtPath->rownumPredicate = sCrtPath->rownumPredicateForDNF;

            // fix BUG-21478
            // push projection 처리시 dnf에 대한 고려가 빠졌음.
            // dnf에 대한 고려가 필요함.
            sQuerySet->materializeType = QMO_MATERIALIZE_TYPE_RID;
            sQuerySet->SFWGH->hints->materializeType = QMO_MATERIALIZE_TYPE_RID;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    //------------------------------------------
    // PROJ-2509 Join에서의 Hierarhcy 처리
    //------------------------------------------
    if ( ( sQuerySet->SFWGH->hierarchy != NULL ) &&
         ( ( sQuerySet->SFWGH->from->joinType != QMS_NO_JOIN ) ||
           ( sQuerySet->SFWGH->from->next != NULL ) ) )
    {
        sMyGraph = sCrtPath->myGraph;

        IDE_TEST( qmgHierarchy::init( aStatement,
                                      sQuerySet,
                                      sMyGraph,
                                      sQuerySet->SFWGH->from,
                                      sQuerySet->SFWGH->hierarchy,
                                      &sMyGraph )
                  != IDE_SUCCESS );

        IDE_TEST( qmoCnfMgr::classifyPred4WhereHierachyJoin( aStatement,
                                                             sCrtPath,
                                                             sMyGraph )
                  != IDE_SUCCESS );

        IDE_TEST( qmgHierarchy::optimize( aStatement,
                                          sMyGraph )
                  != IDE_SUCCESS );

        sCrtPath->myGraph = sMyGraph;
    }
    else
    {
        /* Nothing to do */
    }

    //------------------------------------------
    // Counting의 처리
    //------------------------------------------

    if ( sQuerySet->SFWGH->rownum != NULL )
    {
        sMyGraph = sCrtPath->myGraph;

        IDE_TEST( qmgCounting::init( aStatement,
                                     sQuerySet,
                                     sCrtPath->rownumPredicate,
                                     sMyGraph,         // child
                                     & sMyGraph )      // result graph
                  != IDE_SUCCESS );

        IDE_TEST( qmgCounting::optimize( aStatement, sMyGraph )
                  != IDE_SUCCESS);

        // 결과 Graph 연결
        sCrtPath->myGraph = sMyGraph;
    }
    else
    {
        // Nohting To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCrtPathMgr::decideNormalType( qcStatement   * aStatement ,
                                 qmsFrom       * aFrom,
                                 qtcNode       * aWhere,
                                 qmsHints      * aHint,
                                 idBool          aIsCNFOnly,
                                 qmoNormalType * aNormalType)
{
/***********************************************************************
 *
 * Description : Normalization Type 결정
 *
 *    To Fix PR-12743
 *    Normal Form으로 변경할 수 없다고 하여 에러 처리하지 않는다.
 *
 *    다음 네가지 Type중에 하나가 결정된다.
 *       : NOT_DEFINED ( 추후 비용 계산에 의하여 CNF, DNF가 결정됨 )
 *       : CNF
 *       : DNF
 *       : NNF
 *
 * Implementation :
 *
 *    (0) CNF Only 조건 검사
 *        - QCU_OPTIMIZER_DNF_DISABLE == 1,
 *        - No Where, View, DML, Subquery, Outer Join Operator
 *
 *    (1) Normalize 가능 여부 검사
 *        - CNF 불가, DNF 불가 : NNF 사용
 *        - CNF 가능, DNF 불가 : CNF 사용
 *        - CNF 불가, DNF 가능
 *           - CNF Only 조건인 경우       : NNF 사용
 *           - DNF 제약 조건이 없는 경우  : DNF 사용
 *        - CNF 가능, DNF 가능
 *           - CNF Only 조건인 경우       : CNF 사용
 *           - 없는 경우                  : 다음 단계로
 *
 *    (2) 힌트 검사
 *        - 힌트가 존재하면 힌트 사용
 *        - 없다면, 다음 단계로
 *
 *    (3) NOT DEFINED
 *
 ***********************************************************************/

    qtcNode     * sWhere;
    qmsFrom     * sFrom;
    qmsHints    * sHint;
    idBool        sIsCNFOnly;
    idBool        sIsExistView;
    idBool        sExistsJoinOper = ID_FALSE;
    qmoNormalType sNormalType;
    UInt          sNormalFormMaximum;
    UInt          sEstimateCnfCnt = 0;
    UInt          sEstimateDnfCnt = 0;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::decideNormalType::__FT__" );

    sWhere       = aWhere;
    sFrom        = aFrom;
    sHint        = aHint;
    sIsExistView = ID_FALSE;

    // NormalType 초기화
    sNormalType = QMO_NORMAL_TYPE_NOT_DEFINED;

    //----------------------------------------------
    // (0) CNF Only 여부를 판단
    //----------------------------------------------

    sIsCNFOnly = ID_FALSE;

    while ( 1 )
    {
        //----------------------------------------
        // BUG-38434
        // __OPTIMIZER_DNF_DISABLE property
        //----------------------------------------

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_DNF_DISABLE );

        if ( ( QCU_OPTIMIZER_DNF_DISABLE == 1 ) &&
             ( sHint->normalType != QMO_NORMAL_TYPE_DNF ) )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // CNF Only로 입력 인자를 받은 경우
        // ON 절, START WITH절, CONNECT BY절, HAVING절 등
        //----------------------------------------

        if ( aIsCNFOnly == ID_TRUE )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // DML 인 경우
        //----------------------------------------

        // To Fix BUG-10576
        // To Fix BUG-12320 MOVE DML 은 CNF로만 처리되어야 함
        // BUG-45357 Select for update는 DNF 플랜을 생성하지 않도록 합니다.
        if ( ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_UPDATE ) ||
             ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DELETE ) ||
             ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_MOVE )   ||
             ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE ) )
        {
            // Update, Delete, MOVE 문은 CNF로만 처리되어야 함
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // VIEW 가 존재하는 경우
        //----------------------------------------

        // from절에 view가 있으면 CNF only
        IDE_TEST( existsViewinFrom( sFrom, &sIsExistView )
                  != IDE_SUCCESS );

        if( sIsExistView == ID_TRUE )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // WHERE 절이 없거나 Outer Join Operator (+) 가 있는 경우
        //----------------------------------------

        // CNF Only Predicate 검사
        if ( sWhere == NULL )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            //----------------------------------------
            // PROJ-1653 Outer Join Operator (+)
            //----------------------------------------

            if ( ( sWhere->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                   == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                sIsCNFOnly = ID_TRUE;
                sExistsJoinOper = ID_TRUE;
                break;
            }
            else
            {
                // GO GO
            }
        }

        //----------------------------------------
        // Subquery 조건이 있는 경우
        //----------------------------------------

        IDE_TEST( qmoNormalForm::normalizeCheckCNFOnly( sWhere,
                                                        & sIsCNFOnly )
                  != IDE_SUCCESS );

        break;
    }

    //----------------------------------------------
    // (1) Normalization이 가능한지를 판단
    //----------------------------------------------

    // fix BUG-8806
    // CNF/DNF normalForm 구축비용예측
    if ( sWhere != NULL )
    {
        IDE_TEST( qmoNormalForm::estimateCNF( sWhere, &sEstimateCnfCnt )
                  != IDE_SUCCESS );
        IDE_TEST( qmoNormalForm::estimateDNF( sWhere, &sEstimateDnfCnt )
                  != IDE_SUCCESS);
    }
    else
    {
        sEstimateCnfCnt = 0;
        sEstimateDnfCnt = 0;
    }

    sNormalFormMaximum = QCG_GET_SESSION_NORMALFORM_MAXIMUM( aStatement );

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_NORMAL_FORM_MAXIMUM );

    if ( ( sEstimateCnfCnt > sNormalFormMaximum ) &&
         ( sEstimateDnfCnt > sNormalFormMaximum ) )
    {
        //------------------------------
        // CNF 불가 DNF 불가
        //------------------------------

        sNormalType = QMO_NORMAL_TYPE_NNF;
    }
    else if ( ( sEstimateCnfCnt <= sNormalFormMaximum ) &&
              ( sEstimateDnfCnt > sNormalFormMaximum ) )
    {
        //------------------------------
        // CNF 가능 DNF 불가
        //------------------------------

        sNormalType = QMO_NORMAL_TYPE_CNF;
    }
    else if ( ( sEstimateCnfCnt > sNormalFormMaximum ) &&
              ( sEstimateDnfCnt <= sNormalFormMaximum ) )
    {
        //------------------------------
        // CNF 불가 DNF 가능
        //------------------------------

        if ( sIsCNFOnly == ID_TRUE )
        {
            sNormalType = QMO_NORMAL_TYPE_NNF;
        }
        else
        {
            sNormalType = QMO_NORMAL_TYPE_DNF;
        }
    }
    else
    {
        //------------------------------
        // CNF 가능 DNF 가능
        //------------------------------

        if ( sIsCNFOnly == ID_TRUE )
        {
            sNormalType = QMO_NORMAL_TYPE_CNF;
        }
        else
        {
            // 다음 단계로 진행
        }
    }

    //----------------------------------------------
    // 힌트를 이용한 검사
    //----------------------------------------------

    if ( sNormalType == QMO_NORMAL_TYPE_NOT_DEFINED )
    {
        // Normal Form이 정해지지 않은 경우
        if ( sHint->normalType != QMO_NORMAL_TYPE_NOT_DEFINED )
        {
            //---------------------------------------------------------------
            // Normalization Form Hint 검사 :
            //    Hint가 존재하면 Hint의 NormalType을 따른다.
            //    QMO_NORMAL_TYPE_CNF 또는 QMO_NORMAL_TYPE_DNF
            //---------------------------------------------------------------

            sNormalType = sHint->normalType;
        }
        else
        {
            // 힌트가 없음
        }
    }
    else
    {
        // 이미 정해짐
    }

    //----------------------------------------------
    // PROC-1653 Outer Join Operator (+)
    //----------------------------------------------

    IDE_TEST_RAISE( sExistsJoinOper == ID_TRUE && sNormalType != QMO_NORMAL_TYPE_CNF,
                    ERR_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF );


    // out 설정
    *aNormalType = sNormalType;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCrtPathMgr::addRownumPredicate( qmsQuerySet  * aQuerySet,
                                   qmoPredicate * aPredicate )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1405
 *     Rownum Predicate을 critical path의 rownumPredicate으로 연결한다.
 *
 * Implementation :
 *     1. where절의 normalization type이 CNF/NNF인 경우
 *        1.1 where,on절의 Rownum Predicate은 rownumPrecicateForCNF에 연결한다.
 *     2. where절의 normalization type이 DNF인 경우
 *        2.1 where,on절의 Rownum Predicate은 rownumPrecicateForDNF에 연결한다.
 *
 ***********************************************************************/

    qmoCrtPath     * sCrtPath;
    qmoNormalType    sCurrentNormalType;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::addRownumPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sCrtPath = aQuerySet->SFWGH->crtPath;
    sCurrentNormalType = sCrtPath->currentNormalType;

    //------------------------------------------
    // Rownum Predicate 연결
    //------------------------------------------
    if ( aPredicate != NULL )
    {
        switch ( sCurrentNormalType )
        {
            case QMO_NORMAL_TYPE_NNF:
            case QMO_NORMAL_TYPE_CNF:
                aPredicate->next = sCrtPath->rownumPredicateForCNF;
                sCrtPath->rownumPredicateForCNF = aPredicate;
                break;

            case QMO_NORMAL_TYPE_DNF:
                aPredicate->next = sCrtPath->rownumPredicateForDNF;
                sCrtPath->rownumPredicateForDNF = aPredicate;
                break;

            default:
                IDE_DASSERT( 0 );
                break;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoCrtPathMgr::addRownumPredicateForNode( qcStatement  * aStatement,
                                          qmsQuerySet  * aQuerySet,
                                          qtcNode      * aNode,
                                          idBool         aNeedCopy )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1405
 *     qtcNode형태의 Rownum Predicate을 critical path의 rownumPredicate으로
 *     연결한다.
 *
 * Implementation :
 *     1. where절의 normalization type이 CNF/NNF인 경우
 *        1.1 복사가 필요한 경우 node를 복사한다.
 *        1.2 node를 qmoPredicate형태로 감싼다.
 *        1.3 where,on절의 Rownum Predicate은 rownumPrecicateForCNF에 연결한다.
 *     2. where절의 normalization type이 DNF인 경우
 *        2.1 복사가 필요한 경우 node를 복사한다.
 *        2.2 node를 qmoPredicate형태로 감싼다.
 *        2.3 where,on절의 Rownum Predicate은 rownumPrecicateForDNF에 연결한다.
 *
 ***********************************************************************/

    qmoCrtPath     * sCrtPath;
    qmoNormalType    sCurrentNormalType;
    qmoPredicate   * sNewPred = NULL;
    qtcNode        * sNode;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::addRownumPredicateForNode::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sCrtPath = aQuerySet->SFWGH->crtPath;
    sCurrentNormalType = sCrtPath->currentNormalType;

    //------------------------------------------
    // NNF Predicate 생성
    //------------------------------------------

    // NNF filter인 경우 rownumPredicate에 연결하기 위해
    // qmoPredicate으로 감싼다.
    if ( aNode != NULL )
    {
        if ( aNeedCopy == ID_TRUE )
        {
            // NNF filter 복사
            IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                             aNode,
                                             & sNode,
                                             ID_FALSE,
                                             ID_FALSE,
                                             ID_TRUE,
                                             ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            sNode = aNode;
        }

        // qmoPredicate 생성
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            & sNewPred )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Rownum Predicate 연결
    //------------------------------------------

    if ( sNewPred != NULL )
    {
        switch ( sCurrentNormalType )
        {
            case QMO_NORMAL_TYPE_NNF:
            case QMO_NORMAL_TYPE_CNF:
                sNewPred->next = sCrtPath->rownumPredicateForCNF;
                sCrtPath->rownumPredicateForCNF = sNewPred;
                break;

            case QMO_NORMAL_TYPE_DNF:
                sNewPred->next = sCrtPath->rownumPredicateForDNF;
                sCrtPath->rownumPredicateForDNF = sNewPred;
                break;

            default:
                IDE_DASSERT( 0 );
                break;
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

IDE_RC qmoCrtPathMgr::existsViewinFrom( qmsFrom * aFrom,
                                        idBool  * aIsExistView )
{
/***********************************************************************
 *
 * Description : From절 내에 view가 존재하는지 검사
 *
 * Implementation :
 *    (1) from->left, right존재 : traverse
 *    (2) from->left, right가 존재하지 않음 : 최하위 from으로,
 *                        view가 존재할 가능성이 있음.
 *
 ***********************************************************************/

    qmsFrom * sFrom;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::existsViewinFrom::__FT__" );

    // To Fix PR-11530, PR-11536
    // FROM절 모두를 검사하여야 함.
    for ( sFrom = aFrom; sFrom != NULL; sFrom = sFrom->next )
    {
        if( sFrom->left == NULL && sFrom->right == NULL )
        {
            if( sFrom->tableRef->view != NULL )
            {
                *aIsExistView = ID_TRUE;
                break;
            }
        }
        else
        {
            IDE_TEST( existsViewinFrom( sFrom->left, aIsExistView )
                      != IDE_SUCCESS );

            IDE_TEST( existsViewinFrom( sFrom->right, aIsExistView )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCrtPathMgr::decideNormalType4Where( qcStatement   * aStatement,
                                       qmsFrom       * aFrom,
                                       qtcNode       * aWhere,
                                       qmsHints      * aHint,
                                       idBool          aIsCNFOnly,
                                       qmoNormalType * aNormalType )
{
/***********************************************************************
 *
 * Description : Normalization Type 결정
 *
 *    BUG-35155 Partial CNF
 *    CNF estimate 중 NORMALFORM_MAXIMUM 보다 커지는 경우
 *    qtcNode 일부분을 CNF 에서 제외한다.
 *
 *    다음 네가지 Type중에 하나가 결정된다.
 *       : NOT_DEFINED ( 추후 비용 계산에 의하여 CNF, DNF가 결정됨 )
 *       : CNF
 *       : DNF
 *       : NNF
 *
 * Implementation :
 *
 *    (0) CNF Only 조건 검사
 *        - QCU_OPTIMIZER_DNF_DISABLE == 1,
 *        - No Where, View, DML, Subquery, Outer Join Operator
 *
 *    (1) Normalize 가능 여부 검사
 *        - CNF 불가, DNF 불가 : NNF 사용
 *        - CNF 가능, DNF 불가 : CNF 사용
 *        - CNF 불가, DNF 가능
 *           - CNF Only 조건인 경우       : NNF 사용
 *           - DNF 제약 조건이 없는 경우  : DNF 사용
 *        - CNF 가능, DNF 가능
 *           - CNF Only 조건인 경우       : CNF 사용
 *           - 없는 경우                  : 다음 단계로
 *
 *    (2) 힌트 검사
 *        - 힌트가 존재하면 힌트 사용
 *        - 없다면, 다음 단계로
 *
 *    (3) NOT DEFINED
 *
 ***********************************************************************/

    qtcNode     * sWhere;
    qmsFrom     * sFrom;
    qmsHints    * sHint;
    idBool        sIsCNFOnly;
    idBool        sIsExistView;
    idBool        sExistsJoinOper = ID_FALSE;
    qmoNormalType sNormalType;
    UInt          sNormalFormMaximum;
    UInt          sEstimateCnfCnt = 0;
    UInt          sEstimateDnfCnt = 0;

    IDU_FIT_POINT_FATAL( "qmoCrtPathMgr::decideNormalType4Where::__FT__" );

    sWhere       = aWhere;
    sFrom        = aFrom;
    sHint        = aHint;
    sIsExistView = ID_FALSE;

    // NormalType 초기화
    sNormalType = QMO_NORMAL_TYPE_NOT_DEFINED;

    //----------------------------------------------
    // (0) CNF Only 여부를 판단
    //----------------------------------------------

    sIsCNFOnly = ID_FALSE;

    while ( 1 )
    {
        //----------------------------------------
        // BUG-38434
        // __OPTIMIZER_DNF_DISABLE property
        //----------------------------------------

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_DNF_DISABLE );

        if ( ( QCU_OPTIMIZER_DNF_DISABLE == 1 ) &&
             ( sHint->normalType != QMO_NORMAL_TYPE_DNF ) )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // CNF Only로 입력 인자를 받은 경우
        // ON 절, START WITH절, CONNECT BY절, HAVING절 등
        //----------------------------------------

        if ( aIsCNFOnly == ID_TRUE )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // DML 인 경우
        //----------------------------------------

        // To Fix BUG-10576
        // To Fix BUG-12320 MOVE DML 은 CNF로만 처리되어야 함
        // BUG-45357 Select for update는 DNF 플랜을 생성하지 않도록 합니다.
        if ( ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_UPDATE ) ||
             ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_DELETE ) ||
             ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_MOVE )   ||
             ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE ) )
        {
            // Update, Delete, MOVE 문은 CNF로만 처리되어야 함
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // VIEW 가 존재하는 경우
        //----------------------------------------

        // from절에 view가 있으면 CNF only
        IDE_TEST( existsViewinFrom( sFrom, &sIsExistView ) != IDE_SUCCESS );

        if( sIsExistView == ID_TRUE )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            // Go Go
        }

        //----------------------------------------
        // WHERE 절이 없거나 Outer Join Operator (+) 가 있는 경우
        //----------------------------------------

        // CNF Only Predicate 검사
        if ( sWhere == NULL )
        {
            sIsCNFOnly = ID_TRUE;
            break;
        }
        else
        {
            //----------------------------------------
            // PROJ-1653 Outer Join Operator (+)
            //----------------------------------------

            if ( ( sWhere->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                 == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                sIsCNFOnly = ID_TRUE;
                sExistsJoinOper = ID_TRUE;
                break;
            }
            else
            {
                // GO GO
            }
        }

        //----------------------------------------
        // Subquery 조건이 있는 경우
        //----------------------------------------

        IDE_TEST( qmoNormalForm::normalizeCheckCNFOnly( sWhere, &sIsCNFOnly )
                     != IDE_SUCCESS );

        break;
    }

    //----------------------------------------------
    // (1) Normalization이 가능한지를 판단
    //----------------------------------------------

    sNormalFormMaximum = QCG_GET_SESSION_NORMALFORM_MAXIMUM( aStatement );

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_NORMAL_FORM_MAXIMUM );

    // CNF/DNF normalForm 구축비용예측
    if ( sWhere != NULL )
    {
        if ( sNormalFormMaximum > 0 )
        {
            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_OPTIMIZER_PARTIAL_NORMALIZE );

            if ( QCU_OPTIMIZER_PARTIAL_NORMALIZE == 0 )
            {
                // Estimate CNF and DNF
                IDE_TEST( qmoNormalForm::estimateCNF( sWhere, &sEstimateCnfCnt )
                          != IDE_SUCCESS );
                IDE_TEST( qmoNormalForm::estimateDNF( sWhere, &sEstimateDnfCnt )
                          != IDE_SUCCESS);
            }
            else
            {
                // Estimate partial CNF
                qmoNormalForm::estimatePartialCNF( sWhere,
                                                   &sEstimateCnfCnt,
                                                   NULL,
                                                   sNormalFormMaximum );

                // 최상위 qtcNode 가 CNF UNUSABLE 이면 조건절 전체가 NNF 필터이므로
                // CNF 는 불가능하다.
                if ( ( sWhere->node.lflag &  MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK )
                     == MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE )
                {
                    sEstimateCnfCnt = UINT_MAX;
                }

                // Estimate DNF
                IDE_TEST( qmoNormalForm::estimateDNF( sWhere, &sEstimateDnfCnt )
                          != IDE_SUCCESS);
            }
        }
        else
        {
            sEstimateCnfCnt = UINT_MAX;
            sEstimateDnfCnt = UINT_MAX;
        }
    }
    else
    {
        sEstimateCnfCnt = 0;
        sEstimateDnfCnt = 0;
    }

    if ( ( sEstimateCnfCnt > sNormalFormMaximum ) &&
         ( sEstimateDnfCnt > sNormalFormMaximum ) )
    {
        //------------------------------
        // CNF 불가 DNF 불가
        //------------------------------

        sNormalType = QMO_NORMAL_TYPE_NNF;
    }
    else if ( ( sEstimateCnfCnt <= sNormalFormMaximum ) &&
              ( sEstimateDnfCnt > sNormalFormMaximum ) )
    {
        //------------------------------
        // CNF 가능 DNF 불가
        //------------------------------

        sNormalType = QMO_NORMAL_TYPE_CNF;
    }
    else if ( ( sEstimateCnfCnt > sNormalFormMaximum ) &&
              ( sEstimateDnfCnt <= sNormalFormMaximum ) )
    {
        //------------------------------
        // CNF 불가 DNF 가능
        //------------------------------

        if ( sIsCNFOnly == ID_TRUE )
        {
            sNormalType = QMO_NORMAL_TYPE_NNF;
        }
        else
        {
            sNormalType = QMO_NORMAL_TYPE_DNF;
        }
    }
    else
    {
        //------------------------------
        // CNF 가능 DNF 가능
        //------------------------------

        if ( sIsCNFOnly == ID_TRUE )
        {
            sNormalType = QMO_NORMAL_TYPE_CNF;
        }
        else
        {
            // 다음 단계로 진행
        }
    }

    //----------------------------------------------
    // 힌트를 이용한 검사
    //----------------------------------------------

    if ( sNormalType == QMO_NORMAL_TYPE_NOT_DEFINED )
    {
        // Normal Form이 정해지지 않은 경우
        if ( sHint->normalType != QMO_NORMAL_TYPE_NOT_DEFINED )
        {
            //---------------------------------------------------------------
            // Normalization Form Hint 검사 :
            //    Hint가 존재하면 Hint의 NormalType을 따른다.
            //    QMO_NORMAL_TYPE_CNF 또는 QMO_NORMAL_TYPE_DNF
            //---------------------------------------------------------------

            sNormalType = sHint->normalType;
        }
        else
        {
            // 힌트가 없음
        }
    }
    else
    {
        // 이미 정해짐
    }

    //----------------------------------------------
    // PROC-1653 Outer Join Operator (+)
    //----------------------------------------------

    IDE_TEST_RAISE( sExistsJoinOper == ID_TRUE && sNormalType != QMO_NORMAL_TYPE_CNF,
                    ERR_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF );


    // out 설정
    *aNormalType = sNormalType;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_OUTER_JOIN_OPERATOR_NOT_ALLOW_NOCNF ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
