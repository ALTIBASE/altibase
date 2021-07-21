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
 * $Id: qmoCnfMgr.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * Description :
 *     CNF Critical Path Manager
 *
 *     CNF Normalized Form에 대한 최적화를 수행하고
 *     해당 Graph를 생성한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qcuSqlSourceInfo.h>
#include <qmoCnfMgr.h>
#include <qmoPredicate.h>
#include <qmgHierarchy.h>
#include <qmgSelection.h>
#include <qmgPartition.h>
#include <qmgJoin.h>
#include <qmgSemiJoin.h>
#include <qmgAntiJoin.h>
#include <qmgLeftOuter.h>
#include <qmgFullOuter.h>
#include <qmgProjection.h>
#include <qmoSelectivity.h>
#include <qmo.h>
#include <qmoHint.h>
#include <qmoCrtPathMgr.h>
#include <qmoTransitivity.h>
#include <qmoAnsiJoinOrder.h>
#include <qmoCostDef.h>
#include <qmvQTC.h>
#include <qcgPlan.h>
#include <qmgShardSelect.h>
#include <qmv.h>

extern mtfModule mtfEqual;

IDE_RC
qmoCnfMgr::init( qcStatement * aStatement,
                 qmoCNF      * aCNF,
                 qmsQuerySet * aQuerySet,
                 qtcNode     * aNormalCNF,
                 qtcNode     * aNnfFilter )
{
/***********************************************************************
 *
 * Description : qmoCnf 생성 및 초기화
 *
 * Implementation :
 *    (1) aNormalCNF를 qmoCNF::normalCNF에 연결한다.
 *       ( aNormalCNF는 where절을 CNF로 normalize 한 값이다. )
 *    (2) dependencies 설정
 *    (3) base graph count 설정
 *    (4) base graph의 생성 및 초기화
 *    (5) joinGroup 배열 공간 확보
 *
 ***********************************************************************/

    qmoCNF      * sCNF;
    qmsQuerySet * sQuerySet;
    qmsFrom     * sFrom;
    qmsFrom     * sCurFrom;
    UInt          sBaseGraphCnt;
    UInt          sMaxJoinGroupCnt;
    UInt          sTableCnt;
    UInt          i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sQuerySet = aQuerySet;
    sFrom     = sQuerySet->SFWGH->from;

    //------------------------------------------
    // CNF 초기화
    //------------------------------------------

    sCNF = aCNF;
    sCNF->normalCNF = aNormalCNF;
    sCNF->nnfFilter = NULL;
    sCNF->myQuerySet = sQuerySet;
    sCNF->myGraph = NULL;
    sCNF->cost = 0;
    sCNF->constantPredicate = NULL;
    sCNF->oneTablePredicate = NULL;
    sCNF->joinPredicate = NULL;
    sCNF->levelPredicate = NULL;
    sCNF->connectByRownumPred = NULL;
    sCNF->graphCnt4BaseTable = 0;
    sCNF->baseGraph = NULL;
    sCNF->maxJoinGroupCnt = 0;
    sCNF->joinGroupCnt = 0;
    sCNF->joinGroup = NULL;
    sCNF->tableCnt = 0;
    sCNF->tableOrder = NULL;
    sCNF->outerJoinGraph = NULL;
    sCNF->mIsOnlyNL = ID_FALSE;

    //------------------------------------------
    // dependencies 설정
    //------------------------------------------

    qtc::dependencySetWithDep( & sCNF->depInfo,
                               & sQuerySet->SFWGH->depInfo );

    //------------------------------------------
    // PROJ-2418
    // Lateral View와 참조하는 Relation 간 Left/Full-Outer Join 검증
    //------------------------------------------

    for ( sCurFrom = sFrom; sCurFrom != NULL; sCurFrom = sCurFrom->next )
    {
        IDE_TEST( validateLateralViewJoin( aStatement, sCurFrom )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // PROJ-1405
    // rownum predicate 처리
    //------------------------------------------

    if ( aNnfFilter != NULL )
    {
        if ( ( aNnfFilter->lflag & QTC_NODE_ROWNUM_MASK )
             == QTC_NODE_ROWNUM_EXIST )
        {
            // NNF filter가 rownum predicate인 경우 critical path로 올린다.
            IDE_TEST( qmoCrtPathMgr::addRownumPredicateForNode( aStatement,
                                                                aQuerySet,
                                                                aNnfFilter,
                                                                ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            sCNF->nnfFilter = aNnfFilter;
        }
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // baseGraph count, table count 계산
    //------------------------------------------

    sBaseGraphCnt = 0;
    sTableCnt = 0;
    for ( sCurFrom = sFrom; sCurFrom != NULL; sCurFrom = sCurFrom->next )
    {
        sBaseGraphCnt++;
        if ( sCurFrom->joinType != QMS_NO_JOIN )
        {
            // outer join에 참가하는 table 개수의 합
            sTableCnt += qtc::getCountBitSet( & sCurFrom->depInfo );
        }
        else
        {
            sTableCnt++;
        }
    }

    sCNF->graphCnt4BaseTable = sBaseGraphCnt;
    sCNF->maxJoinGroupCnt    = sMaxJoinGroupCnt = sBaseGraphCnt;
    sCNF->tableCnt           = sTableCnt;

    //------------------------------------------
    // baseGraph의 생성 및 초기화
    //------------------------------------------

    // baseGraph pointer 저장할 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgGraph*) * sBaseGraphCnt,
                                             (void **) & sCNF->baseGraph )
              != IDE_SUCCESS );

    if( aQuerySet->SFWGH->outerJoinTree != NULL )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgGraph*),
                                                 (void **) & sCNF->outerJoinGraph )
                  != IDE_SUCCESS );
    }

    // baseGraph들의 초기화
    for ( sFrom = aQuerySet->SFWGH->from, i = 0;
          sFrom != NULL;
          sFrom = sFrom->next, i++ )
    {
        IDE_TEST( initBaseGraph( aStatement,
                                 & sCNF->baseGraph[i],
                                 sFrom,
                                 aQuerySet )
                  != IDE_SUCCESS );
    }

    if( sCNF->outerJoinGraph != NULL )
    {
        IDE_TEST( initBaseGraph( aStatement,
                                 & sCNF->outerJoinGraph,
                                 aQuerySet->SFWGH->outerJoinTree,
                                 aQuerySet )
                  != IDE_SUCCESS );
    }

    if ( sMaxJoinGroupCnt > 0 )
    {
        //------------------------------------------
        // joinGroup 배열 공간 확보 및 초기화
        //------------------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoJoinGroup) *
                                                 sMaxJoinGroupCnt,
                                                 (void **) &sCNF->joinGroup )
                  != IDE_SUCCESS );

        // joinGroup들의 초기화
        IDE_TEST( initJoinGroup( aStatement, sCNF->joinGroup,
                                 sMaxJoinGroupCnt, sBaseGraphCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Join이 아님
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::init( qcStatement * aStatement,
                 qmoCNF      * aCNF,
                 qmsQuerySet * aQuerySet,
                 qmsFrom     * aFrom,
                 qtcNode     * aNormalCNF,
                 qtcNode     * aNnfFilter )
{
/***********************************************************************
 *
 * Description : on Condition CNF를 위한  qmoCnf 생성 및 초기화
 *
 * Implementation :
 *    (1) aNormalCNF를 qmoCNF::normalCNF에 연결한다.
 *       (aNormalCNF는 where절을 CNF로 normalize 한 값이다.)
 *    (2) dependencies 설정
 *    (3) base graph count 설정
 *    (4) base graph의 생성 및 초기화
 *
 ***********************************************************************/

    qmoCNF      * sCNF;
    qmgGraph   ** sBaseGraph;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aFrom != NULL );

    //------------------------------------------
    // CNF 초기화
    //------------------------------------------

    sCNF = aCNF;
    sCNF->normalCNF = aNormalCNF;
    sCNF->nnfFilter = NULL;
    sCNF->myQuerySet = aQuerySet;
    sCNF->myGraph = NULL;
    sCNF->cost = 0;
    sCNF->constantPredicate = NULL;
    sCNF->oneTablePredicate = NULL;
    sCNF->joinPredicate = NULL;
    sCNF->levelPredicate = NULL;
    sCNF->connectByRownumPred = NULL;
    sCNF->graphCnt4BaseTable = 0;
    sCNF->baseGraph = NULL;
    sCNF->maxJoinGroupCnt = 0;
    sCNF->joinGroupCnt = 0;
    sCNF->joinGroup = NULL;
    sCNF->tableCnt = qtc::getCountBitSet( & aFrom->depInfo );
    sCNF->tableOrder = NULL;

    //------------------------------------------
    // dependencies 설정
    //------------------------------------------

    qtc::dependencySetWithDep( & sCNF->depInfo, & aFrom->depInfo );

    //------------------------------------------
    // PROJ-1405
    // rownum predicate 처리
    //------------------------------------------

    if ( aNnfFilter != NULL )
    {
        if ( ( aNnfFilter->lflag & QTC_NODE_ROWNUM_MASK )
             == QTC_NODE_ROWNUM_EXIST )
        {
            // NNF filter가 rownum predicate인 경우 critical path로 올린다.
            IDE_TEST( qmoCrtPathMgr::addRownumPredicateForNode( aStatement,
                                                                aQuerySet,
                                                                aNnfFilter,
                                                                ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            sCNF->nnfFilter = aNnfFilter;
        }
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // baseGraph의 생성 및 초기화
    //------------------------------------------

    sCNF->graphCnt4BaseTable = 2;
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgGraph *) * 2,
                                             (void**) &sBaseGraph )
              != IDE_SUCCESS );

    if ( aFrom->left != NULL )
    {
        IDE_TEST( qmoCnfMgr::initBaseGraph( aStatement,
                                            & sBaseGraph[0],
                                            aFrom->left,
                                            aQuerySet )
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    if ( aFrom->right != NULL )
    {
        IDE_TEST( qmoCnfMgr::initBaseGraph( aStatement,
                                            & sBaseGraph[1],
                                            aFrom->right,
                                            aQuerySet )
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    sCNF->baseGraph = sBaseGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::optimize( qcStatement * aStatement,
                     qmoCNF      * aCNF )
{
/***********************************************************************
 *
 * Description : CNF Processor의 최적화( 즉, qmoCNF의 최적화)
 *
 * Implementation :
 *    (1) Predicate의 분류
 *    (2) 각 base graph의 최적화
 *    (3) Join의 처리
 *
 ***********************************************************************/

    qmsQuerySet * sQuerySet;
    qmoCNF      * sCNF;
    UInt          sBaseGraphCnt;
    UInt          i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF = aCNF;
    sQuerySet = sCNF->myQuerySet;
    sBaseGraphCnt = sCNF->graphCnt4BaseTable;

    //------------------------------------------
    // PROJ-1404 Transitive Predicate 생성
    //------------------------------------------

    IDE_TEST( generateTransitivePredicate( aStatement,
                                           sCNF,
                                           ID_FALSE )
              != IDE_SUCCESS );

    //------------------------------------------
    // Predicate의 분류
    //------------------------------------------
    IDE_TEST( classifyPred4Where( aStatement, sCNF, sQuerySet )
              != IDE_SUCCESS );

    //------------------------------------------
    // 각 Base Graph의 최적화
    //------------------------------------------

    for ( i = 0; i < sBaseGraphCnt; i++ )
    {
        if ( sCNF->mIsOnlyNL == ID_TRUE )
        {
            sCNF->baseGraph[i]->flag &= ~QMG_JOIN_ONLY_NL_MASK;
            sCNF->baseGraph[i]->flag |= QMG_JOIN_ONLY_NL_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( sCNF->baseGraph[i]->optimize( aStatement,
                                                sCNF->baseGraph[i] )
                  != IDE_SUCCESS );
    }

    if( sCNF->outerJoinGraph != NULL )
    {
        IDE_TEST( sCNF->outerJoinGraph->optimize( aStatement,
                                                  sCNF->outerJoinGraph )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // fix BUG-19203
    // subquery에 대해 optimize를 한다.
    // selection graph의 optimize에서 view에 대한 통계 정보가 구축되기 때문에
    // 그 이후에 하는 것이 맞다.
    //------------------------------------------

    // To Fix BUG-8067, BUG-8742
    // constant predicate의 subquery 처리
    if ( sCNF->constantPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->constantPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    // To Fix PR-12743
    // NNF Filter 지원
    if ( sCNF->nnfFilter != NULL )
    {
        IDE_TEST( qmoPred
                  ::optimizeSubqueryInNode( aStatement,
                                            sCNF->nnfFilter,
                                            ID_FALSE,  //No Range
                                            ID_FALSE ) //No ConstantFilter
                  != IDE_SUCCESS );

    }
    else
    {
        // nothing to do
    }

    //------------------------------------------
    // Join의 처리
    //------------------------------------------

    if ( sBaseGraphCnt == 1 )
    {
        sCNF->myGraph = sCNF->baseGraph[0];
    }
    else
    {
        IDE_TEST( joinOrdering( aStatement, sCNF ) != IDE_SUCCESS );
        IDE_FT_ASSERT( sCNF->myGraph != NULL );
    }

    // To Fix PR-9700
    // Cost 결정 시 Join Ordering 시점이 아닌
    // CNF의 최종 Graph가 결정된 후에 설정하여야 함.
    sCNF->cost    = sCNF->myGraph->costInfo.totalAllCost;

    //------------------------------------------
    // Constant Predicate의 처리
    //------------------------------------------

    // fix BUG-9791, BUG-10419, BUG-29551
    // constant filter를 처리가능한 최하위 left graph로 내린다 (X)
    // constant filter는 처리가능한 최상위 graph에 연결한다 (O)
    IDE_TEST( pushSelection4ConstantFilter( aStatement,
                                            sCNF->myGraph,
                                            sCNF )
              != IDE_SUCCESS );

    //------------------------------------------
    // NNF Filter의 연결
    //------------------------------------------

    sCNF->myGraph->nnfFilter = sCNF->nnfFilter;

    //------------------------------------------
    // Table Order 의 설정
    //------------------------------------------

    IDE_TEST( getTableOrder( aStatement, sCNF->myGraph, & sCNF->tableOrder )
              != IDE_SUCCESS );

    // dependency 설정을 위해 CNF의 최상위 graph는 해당 CNF를 참조하고
    // 있어야 한다.
    sCNF->myGraph->myCNF = sCNF;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::removeOptimizationInfo( qcStatement * aStatement,
                                   qmoCNF      * aCNF )
{
    UInt i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::removeOptimizationInfo::__FT__" );

    for( i = 0; i < aCNF->graphCnt4BaseTable; i++ )
    {
        IDE_TEST( qmg::removeSDF( aStatement,
                                  aCNF->baseGraph[i] )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::classifyPred4Where( qcStatement       * aStatement,
                               qmoCNF            * aCNF,
                               qmsQuerySet       * aQuerySet)
{
/***********************************************************************
 *
 * Description : Where절 처리를 위한 Predicate의 분류
 *               ( 일반 CNF의 Predicate 분류 )
 *
 * Implementation :
 *    qmoCNF::normalCNF의 각 Predicate에 대하여 다음을 수행
 *    (1) Rownum Predicate 분류
 *        rownum predicate은 where에서 처리하지 않고 critical path에서 처리한다.
 *    (2) Constant Predicate 분류
 *        constant predicate, level predicate, prior predicate이 선택된다.
 *        이는 addConstPred4Where() 함수에서 hierarhcy graph 존재 유무에 따라
 *        분리하여 처리한다.
 *    (3) One Table Predicate의 분류
 *    (4) Join Predicate의 분류
 *
 ***********************************************************************/

    qmoCNF         * sCNF;
    qtcNode        * sNode;
    qmoPredicate   * sNewPred = NULL;
    qcDepInfo      * sFromDependencies;
    qcDepInfo      * sGraphDependencies;
    qmgGraph      ** sBaseGraph;

    idBool           sExistHierarchy;
    idBool           sIsRownum;
    idBool           sIsConstant;
    idBool           sIsOneTable = ID_FALSE;    // TASK-3876 Code Sonar
    idBool           sIsHierJoin = ID_FALSE;

    UInt             sBaseGraphCnt;
    UInt             i;

    idBool           sIsPush = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::classifyPred4Where::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF              = aCNF;
    sFromDependencies = & sCNF->depInfo;
    sBaseGraphCnt     = sCNF->graphCnt4BaseTable;
    sBaseGraph        = sCNF->baseGraph;

    // Hierarchy 존재 유무 확인
    if ( sCNF->baseGraph[0]->type == QMG_HIERARCHY )
    {
        sExistHierarchy = ID_TRUE;
    }
    else
    {
        sExistHierarchy = ID_FALSE;
    }

    if ( ( aQuerySet->SFWGH->hierarchy != NULL ) &&
         ( ( aQuerySet->SFWGH->from->joinType != QMS_NO_JOIN ) ||
           ( aQuerySet->SFWGH->from->next != NULL ) ) )
    {
        sIsHierJoin = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    // Predicate 분류 대상 Node
    if ( sCNF->normalCNF != NULL )
    {
        sNode = (qtcNode *)sCNF->normalCNF->node.arguments;
    }
    else
    {
        sNode = NULL;
    }

    while( sNode != NULL )
    {
        // qmoPredicate 생성
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            &sNewPred )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Rownum Predicate 검사
        //-------------------------------------------------

        IDE_TEST( qmoPred::isRownumPredicate( sNewPred,
                                              & sIsRownum )
                  != IDE_SUCCESS );

        if ( ( sIsRownum == ID_TRUE ) &&
             ( sIsHierJoin == ID_FALSE ) )
        {
            //---------------------------------------------
            // Rownum Predicate 분류
            //    Rownum Predicate을 critical path의 rownumPredicate에 연결
            //---------------------------------------------

            IDE_TEST( qmoCrtPathMgr::addRownumPredicate( aQuerySet,
                                                         sNewPred )
                      != IDE_SUCCESS );
        }
        else
        {
            //-------------------------------------------------
            // Constant Predicate 검사
            //-------------------------------------------------

            IDE_TEST( qmoPred::isConstantPredicate( sNewPred,
                                                    sFromDependencies,
                                                    & sIsConstant )
                      != IDE_SUCCESS );

            if ( ( sIsConstant == ID_TRUE ) &&
                 ( sIsHierJoin == ID_FALSE ) )
            {
                sNewPred->flag &= ~QMO_PRED_CONSTANT_FILTER_MASK;
                sNewPred->flag |= QMO_PRED_CONSTANT_FILTER_TRUE;

                //-------------------------------------------------
                // Constant Predicate 분류
                //   Hierarhcy 존재 유무에 따라 constant predicate,
                //   level predicate, prior predicat을 분류하여 해당 위치에 추가
                //-------------------------------------------------

                IDE_TEST( addConstPred4Where( sNewPred,
                                              sCNF,
                                              sExistHierarchy )
                          != IDE_SUCCESS );
            }
            else
            {
                //---------------------------------------------
                // One Table Predicate 검사
                //---------------------------------------------

                for ( i = 0 ; i < sBaseGraphCnt; i++ )
                {
                    sGraphDependencies = & sBaseGraph[i]->myFrom->depInfo;
                    IDE_TEST( qmoPred::isOneTablePredicate( sNewPred,
                                                            sFromDependencies,
                                                            sGraphDependencies,
                                                            & sIsOneTable )
                              != IDE_SUCCESS );

                    if ( sIsOneTable == ID_TRUE )
                    {
                        break;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                if ( ( sIsOneTable == ID_TRUE ) &&
                     ( sIsHierJoin == ID_FALSE ) )
                {
                    //---------------------------------------------
                    // One Table Predicate 분류
                    //    oneTablePredicate을 해당 graph의 myPredicate에 연결
                    //    rid predicate 이면 해당 graph의 ridPredicate에 연결
                    //---------------------------------------------
                    if ((sNewPred->node->lflag & QTC_NODE_COLUMN_RID_MASK) ==
                        QTC_NODE_COLUMN_RID_EXIST)
                    {
                        sNewPred->next = sBaseGraph[i]->ridPredicate;
                        sBaseGraph[i]->ridPredicate = sNewPred;
                    }
                    else
                    {
                        sNewPred->next = sBaseGraph[i]->myPredicate;
                        sBaseGraph[i]->myPredicate = sNewPred;
                    }
                }
                else
                {
                    if ( ( sIsOneTable == ID_TRUE ) &&
                         ( sIsHierJoin == ID_TRUE ) )
                    {
                        /** PROJ-2509 HierarchyQuery Join
                         * OneTable이면서 HierarchyJoin이면 Hierarchy Graph
                         * 에서 처리되어야한다.
                         */
                    }
                    else
                    {
                        //---------------------------------------------
                        // Join Predicate 분류
                        //  constantPredicate, oneTablePredicate이 아니면
                        //  1. PUSH_PRED hint가 존재하면,
                        //     view 내부로 내리고,
                        //  2. PUSH_PRED hint가 존재하지 않으면,
                        //     joinPredicate이므로 이를 sCNF->joinPredicate에 연결
                        //---------------------------------------------

                        sIsPush = ID_FALSE;

                        if ( aQuerySet->SFWGH->hints->pushPredHint != NULL )
                        {
                            //---------------------------------------------
                            // PROJ-1495
                            // PUSH_PRED hint가 존재하는 경우
                            // VIEW 내부로 join predicate을 내린다.
                            //---------------------------------------------

                            IDE_TEST( pushJoinPredInView( sNewPred,
                                                          aQuerySet->SFWGH->hints->pushPredHint,
                                                          sBaseGraphCnt,
                                                          QMS_NO_JOIN, // aIsPushPredInnerJoin
                                                          sBaseGraph,
                                                          & sIsPush )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing To Do
                        }

                        if ( sIsPush == ID_TRUE )
                        {
                            // Nothing To Do
                        }
                        else
                        {
                            //---------------------------------------------
                            // constantPredicate, oneTablePredicate이 아니고,
                            // PUSH_PRED hint도 없는 경우 이를
                            // sCNF->joinPredicate에 연결
                            //---------------------------------------------

                            sNewPred->flag &= ~QMO_PRED_JOIN_PRED_MASK;
                            sNewPred->flag  |= QMO_PRED_JOIN_PRED_TRUE;

                            sNewPred->next = sCNF->joinPredicate;
                            sCNF->joinPredicate = sNewPred;

                        }
                    }
                }
            }
        }
        sNode = (qtcNode*)sNode->node.next;
    }

    //---------------------------------------------------------------------
    // BUG-34295 Join ordering ANSI style query
    //     Where 절의 predicate 중 outerJoinGraph 와 연관된
    //     one table predicate 을 찾아 이동시킨다.
    //     outerJoinGraph 의 one table predicate 은 baseGraph 와
    //     dependency 가 겹치지 않아서 predicate 분류 과정에서
    //     constant predicate 으로 잘못 분류된다.
    //     이를 바로잡기 위해 sCNF->constantPredicate 의 predicate 들에서
    //     outerJoinGraph 에 관련된 one table predicate 들을 찾아내어
    //     outerJoinGraph 로 이동시킨다.
    //---------------------------------------------------------------------
    if( sCNF->outerJoinGraph != NULL )
    {
        IDE_TEST( qmoAnsiJoinOrder::fixOuterJoinGraphPredicate( aStatement,
                                                                sCNF )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::classifyPred4OnCondition( qcStatement     * aStatement,
                                     qmoCNF          * aCNF,
                                     qmoPredicate   ** aUpperPred,
                                     qmoPredicate    * aLowerPred,
                                     qmsJoinType       aJoinType )
{
/***********************************************************************
 *
 * Description : onConditionCNF의 Predicate 분류
 *
 * Implementation :
 *    (1) onConditionCNF Predicate의 분류
 *        각 onConditionCNF의 predicate에 대하여 다음을 수행
 *        A. Rownum Predicate의 분류
 *        B. Constant Predicate의 분류
 *        C. One Table Predicate의 분류
 *        D. Join Predicate의 분류
 *    (2) Join 계열 graph의 myPredicate에 대한 push selection 수행
 *        각 UpperPredicate에 대하여 다음을 수행
 *        ( UpperPredicate : onConditionCNF를 가지는 Join Graph의 myPredicate )
 *        각 UpperPredicate에 대하여 다음을 수행
 *        A. One Table Predicate 검사
 *        B. One Table Predicate이면, Push Selection 수행
 *        참고. One Table Predicate 만이 push selection 대상인 이유
 *        - constant predicate : 상위 predicate에 존재하지 않음
 *        - one table predicate : push selection
 *        - join predicate : 추후 filter로 사용해야 함으로 그대로 둠
 *
 ***********************************************************************/

    qmoCNF         * sCNF;
    qmsQuerySet    * sQuerySet;
    qtcNode        * sNode;
    qmoPredicate   * sNewPred;
    qmoPredicate   * sUpperPred;
    qmoPredicate   * sLowerPred;
    qcDepInfo      * sFromDependencies;
    qcDepInfo      * sGraphDependencies;
    qmgGraph      ** sBaseGraph;
    idBool           sIsRownum;
    idBool           sIsConstant;
    idBool           sIsOneTable;
    idBool           sIsLeft;
    UInt             i;
    idBool           sPushDownAble;
    idBool           sIsPush;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::classifyPred4OnCondition::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF              = aCNF;
    sQuerySet         = sCNF->myQuerySet;
    sUpperPred        = *aUpperPred;
    sLowerPred        = aLowerPred;
    sFromDependencies = & sCNF->depInfo;
    sBaseGraph        = sCNF->baseGraph;

    //------------------------------------------
    // onCondition에 대한 Predicate의 분류
    //------------------------------------------

    // Predicate 분류 대상 Node
    if ( sCNF->normalCNF != NULL )
    {
        sNode = (qtcNode *)sCNF->normalCNF->node.arguments;
    }
    else
    {
        sNode = NULL;
    }

    // BUG-42968 ansi 스타일 조인일때 push_pred 힌트 적용
    if ( sQuerySet->SFWGH->hints->pushPredHint != NULL )
    {
        sPushDownAble = checkPushPredHint( sCNF,
                                           sNode,
                                           aJoinType );
    }
    else
    {
        sPushDownAble = ID_FALSE;
    }

    while( sNode != NULL )
    {

        // qmoPredicate 생성
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            &sNewPred )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Rownum Predicate 검사
        //-------------------------------------------------

        IDE_TEST( qmoPred::isRownumPredicate( sNewPred,
                                              & sIsRownum )
                  != IDE_SUCCESS );

        if ( sIsRownum == ID_TRUE )
        {
            //---------------------------------------------
            // Rownum Predicate 분류
            //    Rownum Predicate을 critical path의 rownumPredicate에 연결
            //---------------------------------------------

            IDE_TEST( qmoCrtPathMgr::addRownumPredicate( sQuerySet,
                                                         sNewPred )
                      != IDE_SUCCESS );
        }
        else
        {
            //-------------------------------------------------
            // Constant Predicate 검사
            //-------------------------------------------------

            IDE_TEST( qmoPred::isConstantPredicate( sNewPred,
                                                    sFromDependencies,
                                                    & sIsConstant )
                      != IDE_SUCCESS );

            if ( sIsConstant == ID_TRUE )
            {
                //-------------------------------------------------
                // Constant Predicate 분류
                //    onConditionCNF의 constantPredicate에 연결
                //-------------------------------------------------

                sNewPred->next = sCNF->constantPredicate;
                sCNF->constantPredicate = sNewPred;
            }
            else
            {
                //---------------------------------------------
                // One Table Predicate 검사
                //---------------------------------------------

                for ( i = 0 ; i < 2 ; i++ )
                {
                    // BUG-42944 ansi 스타일 조인일때 push_pred 힌트를 사용해도 1개의 프리디킷만 내려갑니다.
                    sGraphDependencies = & sBaseGraph[i]->myFrom->depInfo;
                    IDE_TEST( qmoPred::isOneTablePredicate( sNewPred,
                                                            sFromDependencies,
                                                            sGraphDependencies,
                                                            & sIsOneTable )
                              != IDE_SUCCESS );

                    if ( sIsOneTable == ID_TRUE )
                    {
                        if ( i == 0 )
                        {
                            sIsLeft = ID_TRUE;
                        }
                        else
                        {
                            sIsLeft = ID_FALSE;
                        }

                        /* BUG-48405 */
                        if ( ( sQuerySet->lflag & QMV_QUERYSET_ANSI_JOIN_ORDERING_MASK )
                             == QMV_QUERYSET_ANSI_JOIN_ORDERING_TRUE )
                        {
                            if ( sNewPred->node->depInfo.depCount > sGraphDependencies->depCount )
                            {
                                sIsOneTable = ID_FALSE;
                            }
                        }
                        break;
                    }
                    else
                    {
                        // nothing to do
                    }
                }

                if ( sIsOneTable == ID_TRUE )
                {
                    //---------------------------------------------
                    // One Table Predicate 분류
                    //    joinType과 left/right graph 방향에 따라
                    //    push selection 여부 결정하여 해당 위치에 추가
                    //    - push selection 가능한 경우
                    //      해당 graph의 myPredicate에 연결
                    //    - push selection 할 수 없는 경우
                    //      onConditionCNF의 oneTablePredicate에 연결
                    //---------------------------------------------

                    IDE_TEST( addOneTblPredOnJoinType( sCNF,
                                                       sBaseGraph[i],
                                                       sNewPred,
                                                       aJoinType,
                                                       sIsLeft )
                              != IDE_SUCCESS );

                }
                else
                {
                    //---------------------------------------------
                    // Join Predicate 분류
                    //    constantPredicate, oneTablePredicate이 아니면
                    //    joinPredicate이므로 이를 sCNF->joinPredicate에 연결
                    //---------------------------------------------

                    sIsPush = ID_FALSE;

                    if ( sPushDownAble == ID_TRUE )
                    {
                        //---------------------------------------------
                        // PROJ-1495
                        // PUSH_PRED hint가 존재하는 경우
                        // VIEW 내부로 join predicate을 내린다.
                        //---------------------------------------------
                        IDE_TEST( pushJoinPredInView( sNewPred,
                                                      sQuerySet->SFWGH->hints->pushPredHint,
                                                      sCNF->graphCnt4BaseTable,
                                                      aJoinType,
                                                      sCNF->baseGraph,
                                                      & sIsPush )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing To Do
                    }

                    if( sIsPush == ID_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sNewPred->flag &= ~QMO_PRED_JOIN_PRED_MASK;
                        sNewPred->flag |= QMO_PRED_JOIN_PRED_TRUE;

                        sNewPred->next = sCNF->joinPredicate;
                        sCNF->joinPredicate = sNewPred;
                    }
                }
            }
        }
        sNode = (qtcNode*)sNode->node.next;
    }

    //---------------------------------------------
    //  PROJ-1404
    //  Transitive Predicate Generation으로 현재 graph에서 upper predicate과
    //  on절로 transitive predicate을 생성할수 있다.
    //  upper predicate은 joinType에 따라 복사해서 내려가든 바로 내려가든
    //  결국에는 해당 graph로 내려간다. (IS NULL, IS NOT NULL은 제외)
    //
    //  upper predicate과 on절에 의해 생성되는 transitive predicate은
    //  IS NULL, IS NOT NULL predicate이 아니며 joinType에 상관없이 해당
    //  graph로 바로 내릴 수 있는데, 이는 upper predicate과 on절에의해
    //  생성된 lower predicate이기 때문이다. (upper predicate은 어떻게든
    //  내려갈 predicate이므로 처음부터 lower predicate이라고도 볼 수 있다.)
    //
    //  lower predicate은 graph의 left나 right에 대한 one table predicate이거나
    //  join predicate일 수 있다. join predicate이 생성된 경우 해당 graph로
    //  내릴 수 없기 때문에 현재 graph에서 처리해야 하나,
    //  left outer join이나 full outer join에서 filter로 처리하게 되므로
    //  bad transitive predicate이 되어 삭제한다.
    //
    //  lower predicate의 처리는 one table predicate일 때 해당 graph에
    //  내리면 된다. 이것은 upper predicate을 처리하는 pushSelectionOnJoinType
    //  함수의 INNER_JOIN type일때의 처리방법과 같아 이 함수를 그대로
    //  이용한다.
    //---------------------------------------------

    if ( sLowerPred != NULL )
    {
        IDE_TEST( pushSelectionOnJoinType(aStatement, & sLowerPred, sBaseGraph,
                                          sFromDependencies, QMS_INNER_JOIN )
                  != IDE_SUCCESS );

        if ( aJoinType == QMS_INNER_JOIN )
        {
            // INNER JOIN인 경우 join predicate을 upper predicate에 연결한다.
            IDE_TEST( qmoTransMgr::linkPredicate( sLowerPred,
                                                  aUpperPred )
                      != IDE_SUCCESS );

            sUpperPred = *aUpperPred;
        }
        else
        {
            // INNER JOIN이 아닌 경우 남은 join predicate은 bad transitive
            // predicate이므로 제거한다.(버린다.)

            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------------
    // Join 계열 Graph의 myPredicate에 대한 Push Selection
    //    Upper Predicate이 One Table Predicate이면 Join Type에 맞게
    //    push selection 수행
    //---------------------------------------------


    if ( sUpperPred != NULL )
    {
        IDE_TEST( pushSelectionOnJoinType(aStatement, aUpperPred, sBaseGraph,
                                          sFromDependencies, aJoinType )
                  != IDE_SUCCESS );
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
qmoCnfMgr::classifyPred4StartWith( qcStatement   * aStatement ,
                                   qmoCNF        * aCNF,
                                   qcDepInfo     * aDepInfo )
{
/***********************************************************************
 *
 * Description : startWithCNF의 Predicate의 분류
 *
 * Implementation :
 *    (1) Constant Predicate의 분류
 *    (2) One Table Predicate의 분류
 *    참고 : Hierarchy Query는 join을 사용할 수 없기 때문에
 *           startWithCNF에는 joinPredicate이 올 수 없음
 *
 ***********************************************************************/

    qmoCNF         * sCNF;
    qtcNode        * sNode;
    qmoPredicate   * sNewPred;
    qcDepInfo      * sFromDependencies;

    idBool           sIsConstant;
    idBool           sIsOneTable;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::classifyPred4StartWith::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF              = aCNF;
    sFromDependencies = aDepInfo;

    // Predicate 분류 대상 Node
    if ( sCNF->normalCNF != NULL )
    {
        sNode = (qtcNode *)sCNF->normalCNF->node.arguments;
    }
    else
    {
        sNode = NULL;
    }

    while( sNode != NULL )
    {

        // qmoPredicate 생성
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            &sNewPred )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Constant Predicate 검사
        //-------------------------------------------------

        IDE_TEST( qmoPred::isConstantPredicate( sNewPred,
                                                sFromDependencies,
                                                & sIsConstant )
                  != IDE_SUCCESS );

        if ( sIsConstant == ID_TRUE )
        {
            //-------------------------------------------------
            // Constant Predicate 분류
            //    onConditionCNF의 constantPredicate에 연결
            //-------------------------------------------------

            if ( ( sNewPred->node->lflag & QTC_NODE_LEVEL_MASK )
                 == QTC_NODE_LEVEL_EXIST )
            {
                // level predicate인 경우, oneTablePredicate에 연결
                sNewPred->next = sCNF->oneTablePredicate;
                sCNF->oneTablePredicate = sNewPred;
            }
            else
            {
                sNewPred->next = sCNF->constantPredicate;
                sCNF->constantPredicate = sNewPred;
            }
        }
        else
        {
            //---------------------------------------------
            // One Table Predicate 분류
            //---------------------------------------------

            // To fix BUG-14370
            IDE_TEST( qmoPred::isOneTablePredicate( sNewPred,
                                                    sFromDependencies,
                                                    sFromDependencies,
                                                    &sIsOneTable )
                      != IDE_SUCCESS );

            IDE_DASSERT( sIsOneTable == ID_TRUE );

            sNewPred->next = sCNF->oneTablePredicate;
            sCNF->oneTablePredicate = sNewPred;
        }
        sNode = (qtcNode*)sNode->node.next;
    }

    //---------------------------------------------
    // start with CNF의 각 Predicate의 subuqery 처리
    //    statr with CNF의 각 Predicate은 해당 graph의 myPredicate에 연결되지
    //    않아 해당 graph의 myPredicate처리 시에 subquery가 생성되지 않는다.
    //---------------------------------------------

    if ( sCNF->constantPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->constantPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    // To Fix PR-12743
    // NNF Filter 지원
    if ( sCNF->nnfFilter != NULL )
    {
        IDE_TEST( qmoPred
                  ::optimizeSubqueryInNode( aStatement,
                                            sCNF->nnfFilter,
                                            ID_FALSE,  //No Range
                                            ID_FALSE ) //No ConstantFilter
                  != IDE_SUCCESS );

    }
    else
    {
        // nothing to do
    }

    if ( sCNF->oneTablePredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->oneTablePredicate,
                                               ID_TRUE ) // Use KeyRange Tip
                  != IDE_SUCCESS );
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
qmoCnfMgr::classifyPred4ConnectBy( qcStatement     * aStatement,
                                   qmoCNF          * aCNF,
                                   qcDepInfo       * aDepInfo )
{
/***********************************************************************
 *
 * Description : connectBy 처리를 Predicate의 분류
 *               ( connectByCNF의 Predicate 분류 )
 *
 * Implementation :
 *    (1) Constant Predicate의 분류
 *        constant predicate, level predicate, prior predicate이 선택된다.
 *        이는 addConstPred4ConnectBy() 함수에서 hierarhcy graph 존재 유무에
 *        따라 분리하여 처리한다.
 *    (2) One Table Predicate의 분류
 *    참고 : Hierarchy Query는 join을 사용할 수 없기 때문에
 *           connectByCNF에는 joinPredicate이 올 수 없음
 *
 ***********************************************************************/

    qmoCNF         * sCNF;
    qtcNode        * sNode;
    qmoPredicate   * sNewPred;
    qcDepInfo      * sFromDependencies;

    idBool           sIsConstant;
    idBool           sIsOneTable;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::classifyPred4ConnectBy::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF              = aCNF;

    if ( sCNF->normalCNF != NULL )
    {
        sNode = (qtcNode *)sCNF->normalCNF->node.arguments;
    }
    else
    {
        sNode = NULL;
    }

    sFromDependencies = aDepInfo;

    while( sNode != NULL )
    {

        // qmoPredicate 생성
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            &sNewPred )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Constant Predicate 분류
        //-------------------------------------------------

        IDE_TEST( qmoPred::isConstantPredicate( sNewPred,
                                                sFromDependencies,
                                                & sIsConstant )
                  != IDE_SUCCESS );

        if ( sIsConstant == ID_TRUE )
        {
            // Rownum/Level/Prior/Constant Predicate 분류하여 predicate 추가
            IDE_TEST( addConstPred4ConnectBy( sCNF,
                                              sNewPred )
                      != IDE_SUCCESS );
        }
        else
        {
            //-------------------------------------------------
            // One Table Predicate 분류
            //-------------------------------------------------

            // BUG-38675
            if ( ( sNewPred->node->lflag & QTC_NODE_LEVEL_MASK )
                 == QTC_NODE_LEVEL_EXIST )
            {
                sNewPred->next = sCNF->levelPredicate;
                sCNF->levelPredicate = sNewPred;
            }
            else
            {
                // To fix BUG-14370
                IDE_TEST( qmoPred::isOneTablePredicate( sNewPred,
                                                        sFromDependencies,
                                                        sFromDependencies,
                                                        &sIsOneTable )
                          != IDE_SUCCESS );

                IDE_DASSERT( sIsOneTable == ID_TRUE );

                sNewPred->next = sCNF->oneTablePredicate;
                sCNF->oneTablePredicate = sNewPred;
            }
        }
        sNode = (qtcNode*)sNode->node.next;
    }

    //---------------------------------------------
    // connect by CNF의 각 Predicate의 subuqery 처리
    //    statr with CNF의 각 Predicate은 해당 graph의 myPredicate에 연결되지
    //    않아 해당 graph의 myPredicate처리 시에 subquery가 생성되지 않는다.
    //---------------------------------------------

    if ( sCNF->constantPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->constantPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    // To Fix PR-12743
    // NNF Filter 지원
    if ( sCNF->nnfFilter != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueryInNode( aStatement,
                                                   sCNF->nnfFilter,
                                                   ID_FALSE,  //No Range
                                                   ID_FALSE ) //No ConstantFilter
                  != IDE_SUCCESS );

    }
    else
    {
        // nothing to do
    }

    if ( sCNF->oneTablePredicate != NULL )
    {
        // BUG-47522 Connect by 절에서 subquery keyrange tip을 사용하지 않습니다.
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->oneTablePredicate,
                                               ID_FALSE ) // Use KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    if ( sCNF->levelPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->levelPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    /* BUG-39434 The connect by need rownum pseudo column. */
    if ( sCNF->connectByRownumPred != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               sCNF->connectByRownumPred,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
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
qmoCnfMgr::initBaseGraph( qcStatement * aStatement,
                          qmgGraph   ** aBaseGraph,
                          qmsFrom     * aFrom,
                          qmsQuerySet * aQuerySet )
{
/***********************************************************************
 *
 * Description : Base Graph의 생성 및 초기화
 *
 * Implementation :
 *    aFrom을 따라가면서 해당 Table에 대응하는 base graph의 초기화 함수 호출
 *
 ***********************************************************************/

    qmsQuerySet     * sQuerySet;
    qmsHierarchy    * sHierarchy;
    qcShardStmtType   sShardStmtType;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::initBaseGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sQuerySet      = aQuerySet;
    sHierarchy     = sQuerySet->SFWGH->hierarchy;

    if ( ( sHierarchy != NULL ) &&
         ( sQuerySet->SFWGH->from->next == NULL ) &&
         ( sQuerySet->SFWGH->from->joinType == QMS_NO_JOIN ) )
    {
        //------------------------------------------
        // Hierarchy Graph 초기화
        //------------------------------------------

        IDE_TEST( qmgHierarchy::init( aStatement,
                                      sQuerySet,
                                      NULL,
                                      aFrom,
                                      sHierarchy,
                                      aBaseGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------------
        // 그 외의 base Graph 초기화
        //------------------------------------------

        switch( aFrom->joinType )
        {
            case QMS_NO_JOIN :
                if( (aFrom->tableRef->view == NULL) &&
                    (aFrom->tableRef->tableInfo->tablePartitionType
                     == QCM_PARTITIONED_TABLE ) )
                {
                    // PROJ-1502 PARTITIONED DISK TABLE
                    // base table이고 partitioned table이면
                    // partition graph를 생성한다.
                    IDE_TEST( qmgPartition::init( aStatement,
                                                  sQuerySet,
                                                  aFrom,
                                                  aBaseGraph )
                              != IDE_SUCCESS );
                }
                else
                {
                    // PROJ-2638
                    if ( aFrom->tableRef->view != NULL )
                    {
                        sShardStmtType = aFrom->tableRef->view->myPlan->parseTree->stmtShard;
                    }
                    else
                    {
                        sShardStmtType = QC_STMT_SHARD_NONE;
                    }

                    if ( ( sShardStmtType != QC_STMT_SHARD_NONE ) &&
                         ( sShardStmtType != QC_STMT_SHARD_META ) )
                    {
                        IDE_TEST( qmgShardSelect::init( aStatement,
                                                        sQuerySet,
                                                        aFrom,
                                                        aBaseGraph )
                                     != IDE_SUCCESS );
                    }
                    else
                    {
                        // PROJ-1502 PARTITIONED DISK TABLE
                        // base table이고 non-partitioned table이거나,
                        // view이면 selection graph를 생성한다.
                        IDE_TEST( qmgSelection::init( aStatement,
                                                      sQuerySet,
                                                      aFrom,
                                                      aBaseGraph )
                                  != IDE_SUCCESS );
                    }
                }

                break;

            case QMS_INNER_JOIN :
                IDE_TEST( qmgJoin::init( aStatement,
                                         sQuerySet,
                                         aFrom,
                                         aBaseGraph )
                          != IDE_SUCCESS );
                break;

            case QMS_LEFT_OUTER_JOIN :
                IDE_TEST( qmgLeftOuter::init( aStatement,
                                              sQuerySet,
                                              aFrom,
                                              aBaseGraph )
                          != IDE_SUCCESS );
                break;

            case QMS_FULL_OUTER_JOIN :
                IDE_TEST( qmgFullOuter::init( aStatement,
                                              sQuerySet,
                                              aFrom,
                                              aBaseGraph )
                          != IDE_SUCCESS );
                break;

            default :
                IDE_DASSERT( 0 );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::initJoinGroup( qcStatement  * aStatement,
                          qmoJoinGroup * aJoinGroup,
                          UInt           aJoinGroupCnt,
                          UInt           aBaseGraphCnt )
{
/***********************************************************************
 *
 * Description : Join Group의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoJoinGroup * sCurJoinGroup;
    UInt         i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::initJoinGroup::__FT__" );

    // 각 Join Group의 초기화
    for ( i = 0; i < aJoinGroupCnt; i++ )
    {
        sCurJoinGroup = & aJoinGroup[i];

        sCurJoinGroup->joinPredicate = NULL;
        sCurJoinGroup->joinRelation = NULL;
        sCurJoinGroup->joinRelationCnt = 0;
        qtc::dependencyClear( & sCurJoinGroup->depInfo );
        sCurJoinGroup->topGraph = NULL;
        sCurJoinGroup->baseGraphCnt = 0;
        sCurJoinGroup->baseGraph = NULL;
        sCurJoinGroup->mIsOnlyNL = ID_FALSE;
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgGraph* ) * aBaseGraphCnt,
                                                 (void**)&sCurJoinGroup->baseGraph )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::addConstPred4Where( qmoPredicate * aPredicate,
                               qmoCNF       * aCNF,
                               idBool         aExistHierarchy )
{
/***********************************************************************
 *
 * Description : Where의 Level/Prior/Constant Predicate의 분류
 *
 * Implementation :
 *    (1) Hierarchy가 존재하는 경우
 *        A. Level Predicate인 경우 : qmgHierarchy::myPredicate에 연결
 *        B. Prior Predicate인 경우 : qmgHierarchy::myPredicate에 연결
 *        C. 그 외 일반 Constant Predicate : qmoCNF::constantPredicate에 연결
 *        참고 : qmgHierarhcy::myPredicate은 PLAN NODE 구성 시,
 *               filter로 처리됨
 *    (2) Hierarchy가 존재하지 않는 경우
 *        qmoCnf::constantPredicate에 연결
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoCNF       * sCNF;
    qmgGraph     * sHierGraph;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::addConstPred4Where::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sPredicate = aPredicate;
    sCNF = aCNF;

    if ( aExistHierarchy == ID_TRUE )
    {
        //------------------------------------------
        // Hierarchy가 존재하는 경우,
        //     - Level Predicate,Prior Predicate  :
        //       ( PLAN NODE 생성 시 )filter로 처리될 수 있도록
        //         Hierarchy Graph의 myPredicate에 연결
        //     - Prior Predicate : Hierarchy Graph의 myPredicate에 연결
        //     - Constant Predicate : CNF의 constantPredicate에 연결
        //------------------------------------------

        sHierGraph = sCNF->baseGraph[0];


        /* Level, isLeaf, Predicate의 처리 */
        if ( ( (sPredicate->node->lflag & QTC_NODE_LEVEL_MASK )
             == QTC_NODE_LEVEL_EXIST) ||
             ( (sPredicate->node->lflag & QTC_NODE_ISLEAF_MASK )
             == QTC_NODE_ISLEAF_EXIST) )
        {
            sPredicate->next = sHierGraph->myPredicate;
            sHierGraph->myPredicate = sPredicate;
        }
        else
        {
            // Prior Predicate 처리
            if ( ( sPredicate->node->lflag & QTC_NODE_PRIOR_MASK )
                 == QTC_NODE_PRIOR_EXIST )
            {
                sPredicate->next = sHierGraph->myPredicate;
                sHierGraph->myPredicate = sPredicate;
            }
            else
            {
                // Constant Predicate 처리
                sPredicate->next = sCNF->constantPredicate;
                sCNF->constantPredicate = sPredicate;
            }
        }
    }
    else
    {
        //------------------------------------------
        // Hierarchy가 존재하지 않는 경우,
        // Level Predicate, Prior Predicate을 분류하지 않음
        // 모두 constant predicate으로 분류하여 처리
        //------------------------------------------

        sPredicate->next = sCNF->constantPredicate;
        sCNF->constantPredicate = sPredicate;
    }

    return IDE_SUCCESS;
}

idBool qmoCnfMgr::checkPushPredHint( qmoCNF           * aCNF,
                                     qtcNode          * aNode,
                                     qmsJoinType        aJoinType )
{
/***********************************************************************
 *
 * Description : BUG-42968 ansi 스타일 조인일때 push_pred 힌트 적용여부를 판단한다.
 *
 * Implementation :
 *                  1. inner join : 모두 가능
 *                  2. left outer : ON 절에 조인 프리디킷만 존재할때 가능
                                    오른쪽 view로만 내릴수 있음
 *                  3. full outer : 모두 불가능
 *
 ***********************************************************************/
    qtcNode          * sNode;
    qmsPushPredHints * sPushPredHint;

    // BUG-26800
    // inner join on절의 join조건에 push_pred 힌트 적용
    if ( aJoinType == QMS_INNER_JOIN )
    {
        // nothing to do
    }
    else if ( aJoinType == QMS_LEFT_OUTER_JOIN )
    {
        sNode         = aNode;

        //------------------------------------------
        // 힌트가 오른쪽 view 인지 검사
        //------------------------------------------

        for( sPushPredHint = aCNF->myQuerySet->SFWGH->hints->pushPredHint;
             sPushPredHint != NULL;
             sPushPredHint = sPushPredHint->next )
        {
            if ( qtc::dependencyEqual( &(sPushPredHint->table->table->depInfo),
                                       &(aCNF->baseGraph[1]->depInfo) ) == ID_TRUE )
            {
                break;
            }
            else
            {
                // nothing to do
            }
        }

        // 힌트중에 1개라도 사용가능하면 push down 한다.
        if ( sPushPredHint == NULL )
        {
            IDE_CONT( DISABLE );
        }
        else
        {
            // nothing to do
        }

        //------------------------------------------
        // ON절 검사
        // 1. 조인 프리디킷으로만 구성되었는지 여부
        // 2. 서브쿼리가 없는지 여부
        //------------------------------------------

        while( sNode != NULL )
        {
            if ( qtc::dependencyEqual( &(aCNF->depInfo),
                                       &(sNode->depInfo) ) == ID_FALSE )
            {
                IDE_CONT( DISABLE );
            }
            else
            {
                // nothing to do
            }

            if ( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK ) == QTC_NODE_SUBQUERY_EXIST )
            {
                IDE_CONT( DISABLE );
            }
            else
            {
                // nothing to do
            }

            sNode = (qtcNode*)sNode->node.next;
        }
    }
    else
    {
        IDE_CONT( DISABLE );
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( DISABLE );

    return ID_FALSE;
}

IDE_RC
qmoCnfMgr::pushJoinPredInView( qmoPredicate     * aPredicate,
                               qmsPushPredHints * aPushPredHint,
                               UInt               aBaseGraphCnt,
                               qmsJoinType        aJoinType,
                               qmgGraph        ** aBaseGraph,
                               idBool           * aIsPush )
{
/***********************************************************************
 *
 * Description : join predicate을 VIEW 내부로 push
 *
 * Implementation : PROJ-1495
 *
 *    1. PUSH_PRED(view)의 해당 view의 base graph를 찾는다.
 *    2. 찾았으면,
 *       (1) predicate
 *           . view의 base graph에 연결
 *           . predicate에 PUSH_PRED hint에 의해 내려졌다는 정보 저장
 *       (2) view의 tableRef
 *           . PUSH_PRED hint view이며,
 *             이에 대한 predicate이 내려졌다는 정보 표시
 *           . tableRef에 sameViewRef가 있으면, 이를 제거
 *           . view tableRef의 view opt type 설정 (QMO_VIEW_OPT_TYPE_PUSH)
 *           . view base graph에 dependency oring
 *
 ***********************************************************************/

    qmsPushPredHints     * sPushPredHint;
    qcDepInfo              sAndDependencies;
    qcDepInfo            * sDependencies;
    qmgGraph            ** sBaseGraph;

    UInt                   sBaseGraphCnt;
    UInt                   sCnt;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::pushJoinPredInView::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aPushPredHint != NULL );
    IDE_DASSERT( aBaseGraph != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sBaseGraph = aBaseGraph;
    sBaseGraphCnt = aBaseGraphCnt;

    //------------------------------------------
    // PUSH_PRED의 해당 VIEW를 찾아서 join predicate을 내린다.
    //------------------------------------------

    // subquery 제외
    if( ( aPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
        == QTC_NODE_SUBQUERY_EXIST )
    {
        *aIsPush = ID_FALSE;
    }
    else
    {
        //------------------------------------------
        // PUSH_PRED의 해당 VIEW를 찾아서
        // join predicate을 내린다.
        //------------------------------------------

        for( sPushPredHint = aPushPredHint;
             sPushPredHint != NULL;
             sPushPredHint = sPushPredHint->next )
        {
            if ( aJoinType == QMS_LEFT_OUTER_JOIN )
            {
                if( qtc::dependencyEqual( &sBaseGraph[1]->myFrom->depInfo,
                                          &sPushPredHint->table->table->depInfo ) == ID_TRUE )
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
                qtc::dependencyAnd( &aPredicate->node->depInfo,
                                    &sPushPredHint->table->table->depInfo,
                                    &sAndDependencies );

                if( qtc::dependencyEqual( &sAndDependencies,
                                          &sPushPredHint->table->table->depInfo )
                    == ID_TRUE )
                {
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }

        if( sPushPredHint != NULL )
        {
            // push join predicate
            for( sCnt = 0; sCnt < sBaseGraphCnt; sCnt++ )
            {
                sDependencies = & sBaseGraph[sCnt]->myFrom->depInfo;

                if( qtc::dependencyEqual( sDependencies,
                                          & sPushPredHint->table->table->depInfo )
                    == ID_TRUE )
                {
                    aPredicate->next = sBaseGraph[sCnt]->myPredicate;
                    sBaseGraph[sCnt]->myPredicate = aPredicate;

                    // predicate flag 설정
                    aPredicate->flag &= ~QMO_PRED_PUSH_PRED_HINT_MASK;
                    aPredicate->flag |= QMO_PRED_PUSH_PRED_HINT_TRUE;

                    // sameViewRef가 존재하는 경우, 이를 제거한다.
                    if( sBaseGraph[sCnt]->myFrom->tableRef->sameViewRef
                        != NULL )
                    {
                        sBaseGraph[sCnt]->myFrom->tableRef->sameViewRef = NULL;
                    }
                    else
                    {
                        // Nothing To Do
                    }

                    // tableRef flag 설정
                    sBaseGraph[sCnt]->myFrom->tableRef->flag
                        &= ~ QMS_TABLE_REF_PUSH_PRED_HINT_MASK;
                    sBaseGraph[sCnt]->myFrom->tableRef->flag
                        |= QMS_TABLE_REF_PUSH_PRED_HINT_TRUE;

                    if ( aJoinType == QMS_INNER_JOIN )
                    {
                        sBaseGraph[sCnt]->myFrom->tableRef->flag
                            &= ~QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_MASK;
                        sBaseGraph[sCnt]->myFrom->tableRef->flag
                            |= QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_TRUE;
                    }
                    else
                    {
                        // Nothing To Do
                    }

                    // tableRef viewOpt flag 설정
                    sBaseGraph[sCnt]->myFrom->tableRef->viewOptType
                        = QMO_VIEW_OPT_TYPE_PUSH;

                    // dependency 설정
                    IDE_TEST( qtc::dependencyOr( &(sBaseGraph[sCnt]->depInfo),
                                                 & aPredicate->node->depInfo,
                                                 &(sBaseGraph[sCnt]->depInfo) )
                              != IDE_SUCCESS );

                    sPushPredHint->table->table->tableRef->view->disableLeftStore = ID_TRUE;

                    *aIsPush = ID_TRUE;

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
            *aIsPush = ID_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCnfMgr::addOneTblPredOnJoinType( qmoCNF       * aCNF,
                                    qmgGraph     * aBaseGraph,
                                    qmoPredicate * aPredicate,
                                    qmsJoinType    aJoinType,
                                    idBool         aIsLeft )
{
/***********************************************************************
 *
 * Description : Join Type에 따라 One Table Predicate을 처리
 *
 * Implementation :
 *    (1) INNER_JOIN
 *        left, right 상관없이 push selection 수행 가능
 *        one table predicate을 해당 graph의 myPredicate에 내려줌
 *    (2) FULL_OUTER_JOIN
 *        left, right 상관없이 push selection 수행할 수 없음
 *        onConditionCNF->oneTablePredicate에 연결
 *    (3) LEFT_OUTER_JOIN
 *        - left : push selection 수행할 수 없음,
 *                 따라서 onConditionCNF->oneTablePredicate에 연결
 *        - right : push selection 수행 가능
 *                  one table predicate을 해당 graph의 myPredicate에 내려줌
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoCNF       * sCNF;
    qmgGraph     * sBaseGraph;
    qmoPushApplicableType sPushApplicable;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::addOneTblPredOnJoinType::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aBaseGraph != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sPredicate = aPredicate;
    sBaseGraph = aBaseGraph;
    sCNF = aCNF;

    //------------------------------------------
    // To Fix BUG-10806
    // Push Selection 가능 Predicate 인지 검사
    //------------------------------------------

    sPushApplicable = isPushApplicable(aPredicate, aJoinType);

    // BUG-44805 QMO_PUSH_APPLICABLE_LEFT 도 허용해야 한다.
    // aIsLeft 변수로 인해 left 로는 Push Selection이 안됨
    if ( sPushApplicable != QMO_PUSH_APPLICABLE_FALSE )
    {
        switch( aJoinType )
        {
            case QMS_INNER_JOIN :
                // push selection 수행
                sPredicate->next = sBaseGraph->myPredicate;
                sBaseGraph->myPredicate = sPredicate;
                break;

            case QMS_LEFT_OUTER_JOIN :
                if ( aIsLeft == ID_TRUE )
                {
                    // left conceptual table에 대한 predicate
                    // push selection 할 수 없음
                    sPredicate->next = sCNF->oneTablePredicate;
                    sCNF->oneTablePredicate = sPredicate;
                }
                else
                {
                    // right conceptual table에 대한 predicate
                    // push selection 수행
                    sPredicate->next = sBaseGraph->myPredicate;
                    sBaseGraph->myPredicate = sPredicate;
                }
                break;

            case QMS_FULL_OUTER_JOIN :
                // push selection 할 수 없음
                sPredicate->next = sCNF->oneTablePredicate;
                sCNF->oneTablePredicate = sPredicate;
                break;
            default:
                IDE_FT_ASSERT( 0 );
                break;
        }
    }
    else
    {
        // push selection 할 수 없음
        sPredicate->next = sCNF->oneTablePredicate;
        sCNF->oneTablePredicate = sPredicate;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoCnfMgr::pushSelectionOnJoinType( qcStatement   * aStatement,
                                    qmoPredicate ** aUpperPred,
                                    qmgGraph     ** aBaseGraph,
                                    qcDepInfo     * aFromDependencies,
                                    qmsJoinType     aJoinType )
{
/***********************************************************************
 *
 * Description : Join Type에 따라 push selection 최적화 수행
 *
 * Implementation :
 *    upper predicate( where절에서 내려온 predicate ) 중 IS NULL, IS NOT NULL
 *    predicate을 제외하고 아래 과정을 수행한다.
 *    (1) INNER_JOIN
 *        upper predicate에서 해당 graph의 myPredicate에 내려줌
 *    (2) FULL_OUTER_JOIN
 *        upper predicate에서 one table predicate을 복사하여
 *        해당 graph의 myPredicate에 내려줌
 *    (3) LEFT_OUTER_JOIN
 *        - left  : upper predicate에서 해당 graph의 myPredicate에 복사하여 내려줌
 *        - right : upper predicate에서 one table predicate을 복사하여
 *                  해당 graph의 myPredicate에 내려줌
 *
 *    Ex)
 *       < 상위의 Predicate에서 내려줄 Predicate을 분리하여 하위의 Graph에
 *         달아주는 경우 >
 *
 *        [p2] Predicate을 해당 Join Graph에 연결하는 경우
 *
 *                             sPrevUpperPred
 *                               |
 *                               |       sAddPred
 *                               |       sCurUpperPred
 *                               |        |
 *                               V        V
 *              [UpperPred] --> [p1] --> [p2] --> [p3]
 *
 *        [해당 하위 Graph] --> [q1] --> [q2] --> [q3]
 *
 *
 *         ==>   UpperPred에서 연결을 끊고, sCurUpperPred를 다음으로 진행
 *
 *                             sPrevJoinPred
 *                               |
 *                               |     sAddPred
 *                               |       |
 *                               |       V
 *                               |      [p2]  sCurUpperPred
 *                               V         \   |
 *                                          V  V
 *              [UpperPred] --> [p1] -------> [p3]
 *
 *        [해당 하위 Graph] --> [q1] --> [q2] --> [q3]
 *
 *
 *         ==>  [p2] Predicate 연결
 *
 *                             sPrevUpperPred
 *                               |
 *                               |       sCurUpperPred
 *                               |        |
 *                               V        V
 *              [UpperPred] --> [p1] --> [p3]
 *
 *                           +[p2]
 *                          /      \
 *                         /        V
 *        [해당 하위 Graph] --X--> [q1] --> [q2] --> [q3]
 *
 *
 *         ==>  [p2] Predicate 연결 후
 *
 *                             sPrevUpperPred
 *                               |
 *                               |       sCurUpperPred
 *                               |        |
 *                               V        V
 *              [UpperPred] --> [p1] --> [p3]
 *
 *        [해당 하위 Graph] --> [p2]-->[q1] --> [q2] --> [q3]
 *
 *
 *        < 상위의 Predicate에서 내려줄 Predicate을 복사하여 하위의 Graph에
 *         달아주는 경우 >
 *
 *         [p2] Predicate을 해당 Join Graph에 연결하는 경우
 *
 *                                  sPrevUpperPred
 *                                        |
 *                                        |    sCurUpperPred
 *                                        |        |
 *                                        V        V
 *              [UpperPred] --> [p1] --> [p2] --> [p3]
 *
 *        [해당 하위 Graph] -X-> [q1] --> [q2] --> [q3]
 *                        \    +
 *                         V  /
 *                         [p2]
 *                          +
 *                          |
 *                        sAddNode ( 복사한 Node )
 *
 ***********************************************************************/

    qmgGraph     ** sBaseGraph;
    qmoPredicate  * sCurUpperPred;
    qmoPredicate  * sPrevUpperPred;
    qmoPredicate  * sAddPred;
    UInt            sPredPos;

    //------------------------------------------------------
    //  aUpperPred : 상위에서 내려온 Predicate
    //  sCurUpperPred : 상위에서 내려온 predicate의 Traverse 관리
    //  sPrevUpperPred : Push Selection 할 수 없는 Predicate의 연결 관리
    //  sAddPred : Push Selection 대상 Predicate의 연결 관리
    //  sPredPos : Base Graph에서 Predicate이 push selection될 Graph의 position
    //------------------------------------------------------

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::pushSelectionOnJoinType::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aUpperPred != NULL );
    IDE_DASSERT( aBaseGraph != NULL );
    IDE_DASSERT( aFromDependencies != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sCurUpperPred     = *aUpperPred;
    sPrevUpperPred    = NULL;
    sBaseGraph        = aBaseGraph;
    sPredPos          = ID_UINT_MAX;

    while ( sCurUpperPred != NULL )
    {
        //------------------------------------------
        // Push Selection 가능한 Predicate인 경우,
        // Push Selection 대상 graph의 위치를 찾는다.
        // ( Push Selection 할 수 없는 경우, ID_UINT_MAX 값을 반환 )
        //------------------------------------------

        IDE_TEST( getPushPredPosition( sCurUpperPred,
                                       sBaseGraph,
                                       aFromDependencies,
                                       aJoinType,
                                       & sPredPos )
                  != IDE_SUCCESS );

        //------------------------------------------
        // 해당 graph에 Predicate 연결
        //------------------------------------------

        if ( sPredPos != ID_UINT_MAX )
        {
            switch( aJoinType )
            {
                case QMS_INNER_JOIN :
                    // Upper Predicate에서 연결을 끊음
                    if ( sPrevUpperPred == NULL )
                    {
                        *aUpperPred = sCurUpperPred->next;
                    }
                    else
                    {
                        sPrevUpperPred->next = sCurUpperPred->next;
                    }

                    sAddPred = sCurUpperPred;

                    // 다음으로 진행
                    sCurUpperPred = sCurUpperPred->next;

                    // 해당 graph의 myPredicate에 연결
                    sAddPred->next = sBaseGraph[sPredPos]->myPredicate;
                    sBaseGraph[sPredPos]->myPredicate = sAddPred;

                    break;
                case QMS_LEFT_OUTER_JOIN :
                    if ( sPredPos == 0 )
                    {
                        // Upper Predicate에서 연결을 끊음
                        if ( sPrevUpperPred == NULL )
                        {
                            *aUpperPred = sCurUpperPred->next;
                        }
                        else
                        {
                            sPrevUpperPred->next = sCurUpperPred->next;
                        }

                        // BUG-40682 left outer join 이 중첩으로 사용될때
                        // filter 가 잘못된 위치를 참조 할 수 있다.
                        // predicate 를 복사해준다.
                        IDE_TEST( qmoPred::copyOnePredicate( QC_QMP_MEM(aStatement),
                                                             sCurUpperPred,
                                                             & sAddPred )
                                  != IDE_SUCCESS );

                        // 다음으로 진행
                        sCurUpperPred = sCurUpperPred->next;

                        // 해당 graph의 myPredicate에 연결
                        sAddPred->next = sBaseGraph[sPredPos]->myPredicate;
                        sBaseGraph[sPredPos]->myPredicate = sAddPred;
                    }
                    else
                    {
                        // Predicate 복사
                        IDE_TEST( qmoPred::copyOnePredicate( QC_QMP_MEM(aStatement),
                                                             sCurUpperPred,
                                                             & sAddPred )
                                  != IDE_SUCCESS );


                        // 복사한 Predicate을 graph의 myPredicate에 연결
                        sAddPred->next = sBaseGraph[sPredPos]->myPredicate;
                        sBaseGraph[sPredPos]->myPredicate = sAddPred;

                        // 다음으로 진행
                        sPrevUpperPred = sCurUpperPred;
                        sCurUpperPred = sCurUpperPred->next;
                    }

                    break;
                case QMS_FULL_OUTER_JOIN :
                    // Predicate 복사
                    IDE_TEST( qmoPred::copyOnePredicate( QC_QMP_MEM(aStatement),
                                                         sCurUpperPred,
                                                         & sAddPred )
                              != IDE_SUCCESS );

                    // 복사한 Predicate을 해당 graph의 myPredicate에 연결
                    sAddPred->next = sBaseGraph[sPredPos]->myPredicate;
                    sBaseGraph[sPredPos]->myPredicate = sAddPred;

                    // 다음으로 진행
                    sPrevUpperPred = sCurUpperPred;
                    sCurUpperPred = sCurUpperPred->next;
                    break;
                default :
                    IDE_FT_ASSERT( 0 );
                    break;
            }

        }
        else
        {
            // subquery를 포함하는 Predicate이거나,
            // one table predicate이 아닌 경우
            sPrevUpperPred = sCurUpperPred;
            sCurUpperPred = sCurUpperPred->next;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::addConstPred4ConnectBy( qmoCNF       * aCNF,
                                   qmoPredicate * aPredicate )
{
/***********************************************************************
 *
 * Description : Connect By의 Level/Prior/Constant Predicate의 분류
 *
 * Implementation :
 *    (1) Connect by Rownum Predicate인 경우 : qmoCNF::connectByRownumPredicate에 연결
 *    (2) Level Predicate인 경우 : qmoCNF::levelPredicate에 연결
 *    (3) Prior Predicate인 경우 : qmoCNF::oneTablePredicate에 연결
 *    (4) Constant Predicate인 경우 : qmoCNF::constantPredicate에 연결
 *
 ***********************************************************************/

    qmoPredicate * sPredicate;
    qmoCNF       * sCNF;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::addConstPred4ConnectBy::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCNF != NULL );
    IDE_DASSERT( aPredicate != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sPredicate = aPredicate;
    sCNF = aCNF;

    /* BUG-39434 The connect by need rownum pseudo column. */
    if ( ( sPredicate->node->lflag & QTC_NODE_ROWNUM_MASK )
         == QTC_NODE_ROWNUM_EXIST )
    {
        sPredicate->next = sCNF->connectByRownumPred;
        sCNF->connectByRownumPred = sPredicate;
    }
    else if ( ( sPredicate->node->lflag & QTC_NODE_LEVEL_MASK )
         == QTC_NODE_LEVEL_EXIST )
    {
        sPredicate->next = sCNF->levelPredicate;
        sCNF->levelPredicate = sPredicate;
    }
    else
    {
        // To Fix PR-9050
        // Prior Node의 존재 여부를 검사하기 위해서는
        // qtcNode->flag을 검사하여야 함.
        // Prior Predicate 처리
        if ( ( sPredicate->node->lflag & QTC_NODE_PRIOR_MASK )
             == QTC_NODE_PRIOR_EXIST )
        {
            sPredicate->next = sCNF->oneTablePredicate;
            sCNF->oneTablePredicate = sPredicate;
        }
        else
        {
            // Constant Predicate 처리
            sPredicate->next = sCNF->constantPredicate;
            sCNF->constantPredicate = sPredicate;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoCnfMgr::joinOrdering( qcStatement * aStatement,
                         qmoCNF      * aCNF )
{
/***********************************************************************
 *
 * Description : Join Order의 결정
 *
 * Implemenation :
 *    (1) Join Group의 분류
 *    (2) 각 Join Group 내 Join Relation의 표현
 *    (3) 각 Join Group 내 joinPredicate들의 selectivity 계산
 *    (4) 각 Join Group 내 Join Order의 결정
 *       따라가면서 base graph가 두개 이상이면 다음을 수행
 *        A. JoinGroup 내 qmgJoin의 생성 및 초기화
 *        B. Join Order 결정 및 qmgJoin의 최적화
 *    (5) Join Group간 Join Order의 결정
 *
 ***********************************************************************/

    qmoCNF       * sCNF;
    UInt           sJoinGroupCnt;
    qmoJoinGroup * sJoinGroup;
    qmgJOIN      * sResultJoinGraph;
    qmgGraph     * sLeftGraph;
    qmsQuerySet  * sQuerySet;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::joinOrdering::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF             = aCNF;
    sResultJoinGraph = NULL;

    // fix BUG-33880
    sQuerySet        = sCNF->myQuerySet;

    // fix BUG-11166
    // ordered hint적용시 join group에 predicate연결을 하지 않기 때문에
    // JoinPredicate의 selectivity는
    // join grouping하기 전에 계산해 놓아야 함
    if ( sCNF->joinPredicate != NULL )
    {
        //------------------------------------------
        // joinPredicate들의 selectivity 계산
        //------------------------------------------

        IDE_TEST( qmoSelectivity::setMySelectivity4Join( aStatement,
                                                         NULL,
                                                         sCNF->joinPredicate,
                                                         ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }


    //------------------------------------------
    // Join Grouping의 분류
    //------------------------------------------

    // BUG-42447 leading hint support
    if( sQuerySet->SFWGH->hints->leading != NULL )
    {
        makeLeadingGroup( sCNF,
                          sQuerySet->SFWGH->hints->leading );
    }
    else
    {
        // Nothing to do
    }

    IDE_TEST( joinGrouping( sCNF ) != IDE_SUCCESS );

    sJoinGroup       = sCNF->joinGroup;
    sJoinGroupCnt    = sCNF->joinGroupCnt;

    //------------------------------------------
    // 각 Join Group의 처리
    //    (1) JoinRelation의 표현
    //    (2) Join Predicate의 selectivity 계산
    //    (3) Join Order의 결정
    //------------------------------------------

    for ( i = 0; i < sJoinGroupCnt; i++ )
    {
        //------------------------------------------
        // 해당 Join Group내 JoinRelation의 표현
        //------------------------------------------

        IDE_TEST( joinRelationing( aStatement, & sJoinGroup[i] )
                  != IDE_SUCCESS );
        sJoinGroup[i].mIsOnlyNL = aCNF->mIsOnlyNL;
        //------------------------------------------
        // 해당 Join Group 내 Join Order의 결정
        //------------------------------------------

        if ( sJoinGroup[i].baseGraphCnt > 1 )
        {
            IDE_TEST( joinOrderingInJoinGroup( aStatement, & sJoinGroup[i] )
                      != IDE_SUCCESS );
        }
        else
        {
            // To Fix PR-7993
            sJoinGroup[i].topGraph = sJoinGroup[i].baseGraph[0];
        }
    }


    //------------------------------------------
    // 각 Group 간 Join Order의 결정
    // base graph 순서로 처리 : base graph 순서는 SFWGH::from의 순서와 동일
    //                          따라서 Ordered 순서를 따름
    //------------------------------------------

    // PROJ-1495
    // PUSH_PRED hint가 적용된 경우, join group 재배치
    if( sQuerySet->setOp == QMS_NONE )
    {
        // hint는 SFWGH 단위로 줄 수 있다.

        if( sQuerySet->SFWGH->hints->pushPredHint != NULL )
        {
            IDE_TEST( relocateJoinGroup4PushPredHint( aStatement,
                                                      sQuerySet->SFWGH->hints->pushPredHint,
                                                      & sJoinGroup,
                                                      sJoinGroupCnt )
                      != IDE_SUCCESS );
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

    for ( i = 0; i < sJoinGroupCnt; i++ )
    {
        if ( i == 0 )
        {
            sLeftGraph = sJoinGroup[0].topGraph;

            // To Fix PR-7989
            sResultJoinGraph = (qmgJOIN*) sLeftGraph;
        }
        else
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgJOIN ),
                                                     (void**)&sResultJoinGraph )
                != IDE_SUCCESS );

            IDE_TEST( qmgJoin::init( aStatement,
                                     sLeftGraph,
                                     sJoinGroup[i].topGraph,
                                     & sResultJoinGraph->graph,
                                     ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( connectJoinPredToJoinGraph( aStatement,
                                                  & sCNF->joinPredicate,
                                                  & sResultJoinGraph->graph )
                != IDE_SUCCESS );

            // To Fix PR-8266
            // Cartesian Product에도 통계 정보를 구축해야 함.
            // 각 qmgJoin 의 joinOrderFactor, joinSize 계산
            IDE_TEST( qmoSelectivity::setJoinOrderFactor( aStatement,
                                                          & sResultJoinGraph->graph,
                                                          sResultJoinGraph->graph.myPredicate,
                                                          & sResultJoinGraph->joinOrderFactor,
                                                          & sResultJoinGraph->joinSize )
                      != IDE_SUCCESS );

            if ( sCNF->mIsOnlyNL == ID_TRUE )
            {
                sResultJoinGraph->graph.flag &= ~QMG_JOIN_ONLY_NL_MASK;
                sResultJoinGraph->graph.flag |= QMG_JOIN_ONLY_NL_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( sResultJoinGraph->graph.optimize( aStatement,
                                                        & sResultJoinGraph->graph )
                      != IDE_SUCCESS );

            sLeftGraph = (qmgGraph*)sResultJoinGraph;
        }

    }

    sCNF->myGraph = & sResultJoinGraph->graph;

    //------------------------------------------
    // BUG-34295 Join ordering ANSI style query
    //     outerJoinGraph 와 myGraph 를 결합한다.
    //------------------------------------------
    if( sCNF->outerJoinGraph != NULL )
    {
        IDE_TEST( qmoAnsiJoinOrder::mergeOuterJoinGraph2myGraph( aCNF ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmoCnfMgr::makeLeadingGroup( qmoCNF         * aCNF,
                                  qmsLeadingHints* aLeadingHints )
{
/***********************************************************************
 *
 * Description : BUG-42447 leading hint support
 *
 * Implemenation : leading 힌트에 사용된 테이블의 순서대로 조인 그룹을 생성한다.
 *
 ***********************************************************************/

    qmsHintTables    * sLeadingTables;
    qmgGraph        ** sBaseGraph;
    qmoJoinGroup     * sJoinGroup;
    UInt               sBaseGraphCnt;
    UInt               sJoinGroupCnt;
    UInt               i;

    sBaseGraph      = aCNF->baseGraph;
    sJoinGroup      = aCNF->joinGroup;
    sBaseGraphCnt   = aCNF->graphCnt4BaseTable;
    sJoinGroupCnt   = aCNF->joinGroupCnt;

    for ( sLeadingTables = aLeadingHints->mLeadingTables;
          sLeadingTables != NULL;
          sLeadingTables = sLeadingTables->next )
    {
        for( i = 0; i < sBaseGraphCnt; i++ )
        {
            // 없는 테이블을 사용했을때
            if ( sLeadingTables->table == NULL )
            {
                continue;
            }

            // 이미 조인 그룹이 생성된 테이블
            if ( (sBaseGraph[i]->flag & QMG_INCLUDE_JOIN_GROUP_MASK) ==
                 QMG_INCLUDE_JOIN_GROUP_TRUE )
            {
                continue;
            }

            if ( qtc::dependencyContains( &(sBaseGraph[i]->depInfo),
                                          &(sLeadingTables->table->depInfo) ) == ID_TRUE )
            {
                break;
            }
        }

        if ( i < sBaseGraphCnt )
        {
            sBaseGraph[i]->flag &= ~QMG_INCLUDE_JOIN_GROUP_MASK;
            sBaseGraph[i]->flag |= QMG_INCLUDE_JOIN_GROUP_TRUE;

            sJoinGroup[sJoinGroupCnt].baseGraph[0]  = sBaseGraph[i];
            sJoinGroup[sJoinGroupCnt].baseGraphCnt  = 1;
            sJoinGroup[sJoinGroupCnt].depInfo       = sBaseGraph[i]->depInfo;

            sJoinGroupCnt++;

            IDE_DASSERT( sJoinGroupCnt <= aCNF->maxJoinGroupCnt );
        }
        else
        {
            // Nothing to do
        }
    }

    aCNF->joinGroupCnt = sJoinGroupCnt;
}

IDE_RC
qmoCnfMgr::joinGrouping( qmoCNF      * aCNF )
{
/***********************************************************************
 *
 * Description : Join Group의 분류
 *
 * Implemenation :
 *    (1) Ordered Hint가 없으면서, Join Predicate이 존재하는 경우
 *        A. Join Predicate을 이용한 Join Grouping
 *        B. Join Group에 속하는 base graph 연결
 *    (2) Join Group에 포함되지 않은 base graph들을 하나의 Join Group으로
 *        구성
 *        < Join Group에 포함되지 않은 base graph >
 *        A. Ordered Hint가 존재하는 경우, 모든 base graph
 *        B. Join Predicate이 없는 경우, 모든 base graph
 *        C. Join Predicate이 존재하지만, 해당 base graph가 참여한
 *           Join Predicate이 없어 Join Grouping 되지 않은 base graph
 *
 ***********************************************************************/

    qmoCNF           * sCNF;
    qmoJoinOrderType   sJoinOrderHint;
    UInt               sJoinGroupCnt;
    qmoJoinGroup     * sJoinGroup;
    qmoJoinGroup     * sCurJoinGroup;
    UInt               sBaseGraphCnt;
    qmgGraph        ** sBaseGraph;
    qmoPredicate     * sJoinPred;
    qmoPredicate     * sLateralJoinPred;
    UInt               i;     // for loop

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::joinGrouping::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF             = aCNF;
    sJoinGroup       = sCNF->joinGroup;
    sJoinOrderHint   = sCNF->myQuerySet->SFWGH->hints->joinOrderType;
    sBaseGraphCnt    = sCNF->graphCnt4BaseTable;
    sBaseGraph       = sCNF->baseGraph;
    sJoinGroupCnt    = sCNF->joinGroupCnt;
    sLateralJoinPred = NULL;
    sJoinPred        = NULL;


    if ( sJoinOrderHint == QMO_JOIN_ORDER_TYPE_OPTIMIZED )
    {
        //------------------------------------------
        // Join Order Hint 가 존재하지 않는 경우의 처리
        //------------------------------------------

        // PROJ-2418
        // Lateral View의 Join Predicate은 Grouping에서 고려하지 않는다.
        // 잠시 제외시켰다가, Grouping 이후 다시 복구한다.
        IDE_TEST( discardLateralViewJoinPred( sCNF, & sLateralJoinPred )
                  != IDE_SUCCESS );

        if ( sCNF->joinPredicate != NULL )
        {
            //------------------------------------------
            // Join Predicate들을 이용하여 Join Grouping을 수행하고,
            // 해당 Join Group에 JoinPredicate을 연결
            //------------------------------------------

            IDE_TEST( joinGroupingWithJoinPred( sJoinGroup,
                                                sCNF,
                                                & sJoinGroupCnt )
                      != IDE_SUCCESS );

            //------------------------------------------
            // JoinGroup의 base graph 설정
            //    qmoCNF의 baseGraph들 중 해당 Join Group에 속하는 baseGraph를
            //    Join Group의 baseGraph에 연결함
            //------------------------------------------

            for ( i = 0; i < sJoinGroupCnt; i++ )
            {
                IDE_TEST( connectBaseGraphToJoinGroup( & sJoinGroup[i],
                                                       sBaseGraph,
                                                       sBaseGraphCnt )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            //------------------------------------------
            // Join Predicate이 없는 경우의 처리
            //    각 base graph를 하나의 Join Group으로 설정
            //------------------------------------------
        }

        // 제외했던 Lateral View의 Join Predicate을 다시 복구
        if ( sLateralJoinPred != NULL )
        {
            for ( sJoinPred = sLateralJoinPred; 
                  sJoinPred->next != NULL; 
                  sJoinPred = sJoinPred->next ) ;

            sJoinPred->next = sCNF->joinPredicate;
            sCNF->joinPredicate = sLateralJoinPred;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        //------------------------------------------
        // Join Order Hint 가 존재하는 경우의 처리
        //    각 base graph를 하나의 Join Group으로 설정
        //------------------------------------------
    }

    //------------------------------------------
    // Join Grouping 되지 않은 base graph를 하나의 Join Group으로 분류
    //    (1) Join Predicate이 없는 경우
    //    (2) Join Predicate이 존재하지만, 해당 BaseGraph가
    //        참여한 Join Predicate이 없어 Join Grouping 되지 않은 경우
    //    (3) Join Order Hint 가 존재하는 경우
    //------------------------------------------

    for ( i = 0; i < sBaseGraphCnt; i++ )
    {

        if ( ( sBaseGraph[i]->flag & QMG_INCLUDE_JOIN_GROUP_MASK )
             == QMG_INCLUDE_JOIN_GROUP_FALSE )
        {
            sBaseGraph[i]->flag &= ~QMG_INCLUDE_JOIN_GROUP_MASK;
            sBaseGraph[i]->flag |= QMG_INCLUDE_JOIN_GROUP_TRUE;

            sCurJoinGroup = & sJoinGroup[sJoinGroupCnt++];
            sCurJoinGroup->baseGraph[0] = sBaseGraph[i];
            sCurJoinGroup->baseGraphCnt = 1;
            qtc::dependencySetWithDep( & sCurJoinGroup->depInfo,
                                       & sBaseGraph[i]->depInfo );
        }
        else
        {
            // nothing to do
        }
    }

    // 실제 Join Group 개수 설정
    sCNF->joinGroupCnt = sJoinGroupCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::joinRelationing( qcStatement      * aStatement,
                            qmoJoinGroup     * aJoinGroup )
{
/***********************************************************************
 *
 * Description : 각 Join Group 내의 Join Relation 관계 도출
 *
 * Implementation :
 *    (1) 두 개의 conceptual table로만 구성된 Join Predicate의
 *        Join 관계의 도출
 *    (2) operand가 두 개의 conceptual table 로만 구성된 Join 관계의 도출
 *
 ***********************************************************************/

    qmoJoinGroup     * sJoinGroup;
    qmoPredicate     * sCurPred;
    UInt               sBaseGraphCnt;
    qmgGraph        ** sBaseGraph;
    qtcNode          * sNode;
    qmoJoinRelation  * sJoinRelationList;
    UInt               sJoinRelationCnt;
    idBool             sIsTwoConceptTblNode;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::joinRelationing::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJoinGroup != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinGroup           = aJoinGroup;
    sJoinRelationList    = NULL;
    sBaseGraph           = sJoinGroup->baseGraph;
    sBaseGraphCnt        = sJoinGroup->baseGraphCnt;
    sJoinRelationCnt     = 0;
    sIsTwoConceptTblNode = ID_FALSE;

    //------------------------------------------
    // join relation 관계 도출
    //------------------------------------------

    for ( sCurPred = aJoinGroup->joinPredicate;
          sCurPred != NULL;
          sCurPred = sCurPred->next )
    {
        if ( ( sCurPred->node->node.lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_OR )
        {
            sNode = (qtcNode *)sCurPred->node->node.arguments;
        }
        else
        {
            sNode = sCurPred->node;
        }

        if( ( sNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) != QTC_NODE_JOIN_TYPE_INNER )
        {
            // PROJ-1718 Subquery unnesting
            // Semi/anti join인 경우 별도의 join relation 생성 함수를 이용한다.

            IDE_TEST( makeSemiAntiJoinRelationList( aStatement,
                                                    sNode,
                                                    & sJoinRelationList,
                                                    & sJoinRelationCnt )
                      != IDE_SUCCESS );
        }
        else
        {
            // BUG-37963 inner join 일때는 or 노드도 체크해야 한다.
            // 참고로 OR 노드를 먼처 체크하면 semi, anti 조인이 나오지 않는다.
            if( sCurPred->node != sNode )
            {
                IDE_TEST( isTwoConceptTblNode( sCurPred->node,
                                               sBaseGraph,
                                               sBaseGraphCnt,
                                               & sIsTwoConceptTblNode )
                          != IDE_SUCCESS );

                if ( sIsTwoConceptTblNode == ID_TRUE )
                {
                    IDE_TEST( makeJoinRelationList( aStatement,
                                                    & sCurPred->node->depInfo,
                                                    & sJoinRelationList,
                                                    & sJoinRelationCnt )
                              != IDE_SUCCESS );

                    continue;
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

            // 두 개의 conceptual table로만 구성된 predicate인지 검사
            IDE_TEST( isTwoConceptTblNode( sNode,
                                           sBaseGraph,
                                           sBaseGraphCnt,
                                           & sIsTwoConceptTblNode )
                         != IDE_SUCCESS );

            if ( sIsTwoConceptTblNode == ID_TRUE )
            {
                //------------------------------------------
                // 두 개의 conceptual table로만 구성된 Join Predicate
                //------------------------------------------

                IDE_TEST( makeJoinRelationList( aStatement,
                                                & sNode->depInfo,
                                                & sJoinRelationList,
                                                & sJoinRelationCnt )
                          != IDE_SUCCESS );
            }
            else
            {
                sNode = (qtcNode *)sNode->node.arguments;

                if ( sNode->node.next == NULL )
                {
                    sNode = (qtcNode *)sNode->node.arguments;
                }
                else
                {
                    sNode = NULL;
                }

                // To Fix BUG-8789
                // 두 개의 conceptual table로만 구성되었는지 검사
                for ( sNode = (qtcNode *) sNode;
                      sNode != NULL;
                      sNode = (qtcNode *) sNode->node.next )
                {
                    IDE_TEST( isTwoConceptTblNode( sNode,
                                                   sBaseGraph,
                                                   sBaseGraphCnt,
                                                   & sIsTwoConceptTblNode )
                              != IDE_SUCCESS );

                    if ( sIsTwoConceptTblNode == ID_TRUE )
                    {
                        //------------------------------------------
                        // Operand가 두 개의 conceptual table로만 구성된 경우
                        // ex ) T1.I1 + T2.I1 = T3.I1
                        //------------------------------------------

                        IDE_TEST( makeJoinRelationList( aStatement,
                                                        & sNode->depInfo,
                                                        & sJoinRelationList,
                                                        & sJoinRelationCnt )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }
        }
    }

    // PROJ-1718 Subquery Unnesting
    IDE_TEST( makeCrossJoinRelationList( aStatement,
                                         aJoinGroup,
                                         & sJoinRelationList,
                                         & sJoinRelationCnt )
              != IDE_SUCCESS );

    // Join Group에 Join Relation 연결
    sJoinGroup->joinRelation = sJoinRelationList;
    sJoinGroup->joinRelationCnt = sJoinRelationCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::joinOrderingInJoinGroup( qcStatement  * aStatement,
                                    qmoJoinGroup * aJoinGroup )
{
/***********************************************************************
 *
 * Description : Join Group 내에서의 Join Order 결정
 *               Join ordering 의 대상은 모두 inner join 이다
 *
 * Implementation :
 *    (1) Join Graph의 생성 및 초기화
 *    (2) 최초 Join Graph 선택
 *    (3) Join Graph들의 join Ordering 수행
 *
 ***********************************************************************/

    UInt               sJoinRelationCnt;
    qmoJoinRelation  * sJoinRelation;
    UInt               sJoinGraphCnt;
    qmgJOIN          * sJoinGraph;
    qmgJOIN          * sSelectedJoinGraph = NULL;
    UInt               sBaseGraphCnt;
    qmgGraph        ** sBaseGraph;
    qmgGraph         * sLeftGraph;
    qmgGraph         * sRightGraph;
    idBool             sSelectedJoinGraphIsTop = ID_FALSE;
    UInt               sTop;
    UInt               sEnd;
    UInt               i;

    SDouble            sCurJoinOrderFactor;
    SDouble            sCurJoinSize;
    SDouble            sJoinOrderFactor         = 0.0;
    SDouble            sJoinSize                = 0.0;

    SDouble            sFirstRowsFactor;
    SDouble            sJoinGroupSelectivity;
    SDouble            sJoinGroupOutputRecord;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::joinOrderingInJoinGroup::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJoinGroup != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sBaseGraphCnt    = aJoinGroup->baseGraphCnt;
    sBaseGraph       = aJoinGroup->baseGraph;
    sJoinRelation    = aJoinGroup->joinRelation;
    sJoinRelationCnt = aJoinGroup->joinRelationCnt;

    // Join Graph 개수
    if ( sJoinRelationCnt > sBaseGraphCnt - 1 )
    {
        sJoinGraphCnt = sJoinRelationCnt;
    }
    else
    {
        sJoinGraphCnt = sBaseGraphCnt - 1;
    }

    // sTop, sEnd 설정
    if ( sJoinRelation != NULL )
    {
        sEnd = sJoinRelationCnt;
    }
    else
    {
        sEnd = 1;
    }

    //------------------------------------------
    // Join Graph 생성 및 초기화
    //------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgJOIN ) * sJoinGraphCnt,
                                             (void**) & sJoinGraph )
              != IDE_SUCCESS );

    if ( sJoinRelation != NULL )
    {
        i = 0;

        // Join Relation에 대응되는 qmgJoin 초기화
        while ( sJoinRelation != NULL )
        {
            IDE_TEST( getTwoRelatedGraph( & sJoinRelation->depInfo,
                                          sBaseGraph,
                                          sBaseGraphCnt,
                                          & sLeftGraph,
                                          & sRightGraph )
                      != IDE_SUCCESS );

            IDE_TEST( initJoinGraph( aStatement,
                                     sJoinRelation,
                                     sLeftGraph,
                                     sRightGraph,
                                     &sJoinGraph[i].graph,
                                     ID_FALSE )
                      != IDE_SUCCESS );

            sJoinRelation = sJoinRelation->next;

            // To Fix PR-7994
            i++;
        }
    }
    else
    {
        // Join Relation이 존재하지 않는 경우, 최초의 qmgJoin 초기화
        IDE_TEST( qmgJoin::init( aStatement,
                                 sBaseGraph[0],
                                 sBaseGraph[1],
                                 & sJoinGraph[0].graph,
                                 ID_FALSE )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // FirstRowsFactor
    //------------------------------------------

    if( (sBaseGraph[0])->myQuerySet->SFWGH->hints->optGoalType
         == QMO_OPT_GOAL_TYPE_FIRST_ROWS_N )
    {
        // baseGraph Selectivity 통합
        for( sJoinGroupSelectivity  = 1,
             sJoinGroupOutputRecord = 1,
             i = 0; i < sBaseGraphCnt; i++ )
        {
            // BUG-37228
            sJoinGroupSelectivity  *= sBaseGraph[i]->costInfo.selectivity;
            sJoinGroupOutputRecord *= sBaseGraph[i]->costInfo.outputRecordCnt;
        }

        // Join predicate list 의 selectivity 계산
        IDE_TEST( qmoSelectivity::setMySelectivity4Join( aStatement,
                                                         NULL,
                                                         aJoinGroup->joinPredicate,
                                                         ID_TRUE )
                  != IDE_SUCCESS );

        sJoinGroupSelectivity *= aJoinGroup->joinPredicate->mySelectivity;

        IDE_DASSERT_MSG( sJoinGroupSelectivity >= 0 && sJoinGroupSelectivity <= 1,
                         "sJoinGroupSelectivity : %"ID_DOUBLE_G_FMT"\n",
                         sJoinGroupSelectivity );

        sJoinGroupOutputRecord *= sJoinGroupSelectivity;

        sFirstRowsFactor = IDL_MIN( (sBaseGraph[0])->myQuerySet->SFWGH->hints->firstRowsN /
                                     sJoinGroupOutputRecord,
                                     QMO_COST_FIRST_ROWS_FACTOR_DEFAULT);
    }
    else
    {
        sFirstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
    }

    //------------------------------------------
    // joinOrderFactor, joinSize 를 이용한 Join Ordering
    //------------------------------------------

    for ( sTop = 0; sTop < sBaseGraphCnt - 1; sTop++ )
    {
        sSelectedJoinGraph = NULL;

        for ( i = sTop; i < sEnd; i ++ )
        {
            // 각 qmgJoin 의 joinOrderFactor, joinSize 계산
            IDE_TEST( qmoSelectivity::setJoinOrderFactor( aStatement,
                                                          (qmgGraph *) & sJoinGraph[i],
                                                          aJoinGroup->joinPredicate,
                                                          & sCurJoinOrderFactor,
                                                          & sCurJoinSize )
                      != IDE_SUCCESS );

            if( isFeasibleJoinOrder( &sJoinGraph[i].graph,
                                     aJoinGroup->joinRelation ) == ID_FALSE )
            {
                continue;
            }
            else
            {
                // Nothing to do.
            }

            sJoinGraph[i].joinOrderFactor = sCurJoinOrderFactor;
            sJoinGraph[i].joinSize        = sCurJoinSize;

            // JoinOrderFactor 가 가장 좋은 qmgJoin 선택
            if ( sSelectedJoinGraph == NULL )
            {
                sJoinOrderFactor = sCurJoinOrderFactor;
                sJoinSize = sCurJoinSize;
                sSelectedJoinGraph = & sJoinGraph[i];
                sSelectedJoinGraphIsTop = ID_TRUE;
            }
            else
            {
                //--------------------------------------------------
                // PROJ-1352
                // Join Selectivity Threshold 와 Join Size 를 고려하여
                // Join Order를 결정한다.
                //--------------------------------------------------

                if ( sJoinOrderFactor > QMO_JOIN_SELECTIVITY_THRESHOLD )
                {
                    // 최저 JoinOrderFactor 가 THRESHOLD보다 큰 경우
                    if ( sJoinOrderFactor > sCurJoinOrderFactor )
                    {
                        sJoinOrderFactor = sCurJoinOrderFactor;
                        sJoinSize = sCurJoinSize;
                        sSelectedJoinGraph = & sJoinGraph[i];
                        sSelectedJoinGraphIsTop = ID_FALSE;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
                else
                {
                    // 최저 JoinOrderFactor 가 THRESHOLD보다 작은 경우

                    if ( sCurJoinOrderFactor > QMO_JOIN_SELECTIVITY_THRESHOLD )
                    {
                        // nothing to do
                    }
                    else
                    {
                        // 최저 selectivity와 현재 selectivity가
                        // 모두 THRESHOLD보다 작은 경우 Join Size도
                        // 고려하여 비교한다.
                        if ( sJoinOrderFactor * sJoinSize >
                             sCurJoinOrderFactor * sCurJoinSize )
                        {
                            sJoinOrderFactor = sCurJoinOrderFactor;
                            sJoinSize = sCurJoinSize;
                            sSelectedJoinGraph = & sJoinGraph[i];
                            sSelectedJoinGraphIsTop = ID_FALSE;
                        }
                        else
                        {
                            // nothing to do
                        }
                    }
                }
            }
        }

        IDE_TEST_RAISE( sSelectedJoinGraph == NULL,
                        ERR_INVALID_JOIN_GRAPH );

        // 선택된 Join Graph의 left와 right graph가
        // Join Order 결정되었음을 설정
        sSelectedJoinGraph->graph.left->flag &= ~QMG_JOIN_ORDER_DECIDED_MASK;
        sSelectedJoinGraph->graph.left->flag |= QMG_JOIN_ORDER_DECIDED_TRUE;

        sSelectedJoinGraph->graph.right->flag &= ~QMG_JOIN_ORDER_DECIDED_MASK;
        sSelectedJoinGraph->graph.right->flag |= QMG_JOIN_ORDER_DECIDED_TRUE;

        // top 설정
        IDE_TEST( initJoinGraph( aStatement,
                                 (UInt)sSelectedJoinGraph->graph.type,
                                 sSelectedJoinGraph->graph.left,
                                 sSelectedJoinGraph->graph.right,
                                 & sJoinGraph[sTop].graph,
                                 ID_TRUE )
                  != IDE_SUCCESS );

        // To Fix BUG-10357
        if ( sSelectedJoinGraphIsTop == ID_FALSE )
        {
            //fix for overlap memcpy
            if( &sJoinGraph[sTop].graph.costInfo !=
                               &sSelectedJoinGraph->graph.costInfo )
            {
                // To Fix PR-8266
                // 생성하는 Join Graph의 Cost정보를 설정하여야 함.
                idlOS::memcpy( & sJoinGraph[sTop].graph.costInfo,
                               & sSelectedJoinGraph->graph.costInfo,
                               ID_SIZEOF(qmgCostInfo) );
            }
        }


        // Join Predicate의 분류 :
        // Join Group::joinPredicate에서 해당 predicate을 Join Graph의
        // myPredicate에 연결한다.
        IDE_TEST( connectJoinPredToJoinGraph( aStatement,
                                              & aJoinGroup->joinPredicate,
                                              (qmgGraph *) & sJoinGraph[sTop] )
                  != IDE_SUCCESS );

        if( sTop == 0 )
        {
            // FirstRows set
            sJoinGraph[sTop].firstRowsFactor = sFirstRowsFactor;
        }
        else
        {
            sJoinGraph[sTop].firstRowsFactor = QMO_COST_FIRST_ROWS_FACTOR_DEFAULT;
        }

        if ( aJoinGroup->mIsOnlyNL == ID_TRUE )
        {
            sJoinGraph[sTop].graph.flag &= ~QMG_JOIN_ONLY_NL_MASK;
            sJoinGraph[sTop].graph.flag |= QMG_JOIN_ONLY_NL_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
        // 선택된 qmgJoin의 최적화
        IDE_TEST( sJoinGraph[sTop].graph.optimize( aStatement,
                                                   &sJoinGraph[sTop].graph )
                  != IDE_SUCCESS );

        //Join Order가 결정되지 않은 base graph들로 qmgJoin의 초기화
        sEnd = sTop + 1;

        for ( i = 0; i < sBaseGraphCnt; i++ )
        {
            if ( ( sBaseGraph[i]->flag & QMG_JOIN_ORDER_DECIDED_MASK )
                 == QMG_JOIN_ORDER_DECIDED_FALSE )
            {
                IDE_TEST( initJoinGraph( aStatement,
                                         aJoinGroup->joinRelation,
                                         &sJoinGraph[sTop].graph,
                                         sBaseGraph[i],
                                         &sJoinGraph[sEnd].graph,
                                         ID_TRUE )
                          != IDE_SUCCESS );
                sEnd++;
            }
            else
            {
                // nothing to do
            }
        }
    }

    // To Fix PR-7989
    aJoinGroup->topGraph = & sJoinGraph[sTop - 1].graph;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_JOIN_GRAPH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoCnfMgr::joinOrderingInJoinGroup",
                                  "Invalid join graph" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoCnfMgr::isSemiAntiJoinable( qmgGraph        * aJoinGraph,
                               qmoJoinRelation * aJoinRelation )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     Join ordering 시 semi/anti join이 가능한 시점인지 확인한다.
 *     Semi/anti join 대상의 relation이 여러 outer relation과 join된 경우
 *     outer relation들이 모두 먼저 join되어있어야 한다.
 *     ex) SELECT R.*, S.* FROM R, S, T WHERE R.A (semi)= T.A AND S.B (semi)= T.B;
 *         의 경우 R과 S가 먼저 join 되기 전까지 R이나 S가 T와 join될 수 없다.
 *
 * Implementation :
 *     Join relation들 중 inner relation이 semi/anti join 대상에 해당하는
 *     것들만 찾아 outer relation들에서 모두 포함하고 있는지 확인한다.
 *
 ***********************************************************************/

    qmoJoinRelation * sJoinRelation;
    qcDepInfo         sOuterDepInfo;
    qcDepInfo         sInnerDepInfo;

    sJoinRelation = aJoinRelation;

    while( sJoinRelation != NULL )
    {
        qtc::dependencySet( sJoinRelation->innerRelation,
                            &sInnerDepInfo );
        qtc::dependencyRemove( sJoinRelation->innerRelation,
                               &sJoinRelation->depInfo,
                               &sOuterDepInfo );

        if( qtc::dependencyEqual( &aJoinGraph->right->depInfo,
                                  &sInnerDepInfo ) == ID_TRUE )
        {
            if( qtc::dependencyContains( &aJoinGraph->left->depInfo,
                                         &sOuterDepInfo ) == ID_FALSE )
            {

                return ID_FALSE;
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

        sJoinRelation = sJoinRelation->next;
    }

    return ID_TRUE;
}

idBool
qmoCnfMgr::isFeasibleJoinOrder( qmgGraph        * aJoinGraph,
                                qmoJoinRelation * aJoinRelation )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     주어진 type의 join이 가능한 시점인지 확인한다.
 *
 * Implementation :
 *     1) Semi/anti join인 경우 isSemiAntiJoinable을 호출
 *     2) inner join 하려는 table 이
 *        semi/anti join 오른쪽 table 이 아니어야함 (BUG-39249)
 *
 ***********************************************************************/

    qmoJoinRelation* sJoinRelation;
    qcDepInfo        sInnerDepInfo;
    idBool           sResult;

    sResult = ID_TRUE;

    switch (aJoinGraph->type)
    {
        case QMG_INNER_JOIN:
            sJoinRelation = aJoinRelation;
            while (sJoinRelation != NULL)
            {
                if (sJoinRelation->joinType != QMO_JOIN_RELATION_TYPE_INNER)
                {
                    qtc::dependencySet(sJoinRelation->innerRelation,
                                       &sInnerDepInfo);

                   if (qtc::dependencyEqual(&aJoinGraph->right->depInfo,
                                            &sInnerDepInfo) == ID_TRUE)
                    {
                        sResult = ID_FALSE;
                        break;
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }
                else
                {
                    /* nothing to do */
                }
                sJoinRelation = sJoinRelation->next;
            }
            break;

        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
            sResult = isSemiAntiJoinable(aJoinGraph, aJoinRelation);
            break;

        default:
            sResult = ID_TRUE;
            break;
    }

    return sResult;
}

IDE_RC
qmoCnfMgr::initJoinGraph( qcStatement     * aStatement,
                          qmoJoinRelation * aJoinRelation,
                          qmgGraph        * aLeft,
                          qmgGraph        * aRight,
                          qmgGraph        * aJoinGraph,
                          idBool            aExistOrderFactor )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     qmoJoinRelation 자료구조를 파악하여 알맞는 type의 join graph를
 *     생성해준다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoJoinRelation * sJoinRelation;
    qcDepInfo         sOuterDepInfo;
    qcDepInfo         sInnerDepInfo;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::initJoinGraph::__FT__" );

    IDE_FT_ERROR( aLeft      != NULL );
    IDE_FT_ERROR( aRight     != NULL );
    IDE_FT_ERROR( aJoinGraph != NULL );

    sJoinRelation = aJoinRelation;

    while( sJoinRelation != NULL )
    {
        if( sJoinRelation->joinType != QMO_JOIN_RELATION_TYPE_INNER )
        {
            // Semi/anti join시 join의 방향에 맞춰 graph를 초기화한다.
            qtc::dependencySet( sJoinRelation->innerRelation,
                                &sInnerDepInfo );
            qtc::dependencyRemove( sJoinRelation->innerRelation,
                                   &sJoinRelation->depInfo,
                                   &sOuterDepInfo );

            // Left가 outer, right가 inner인지 확인한다.
            if( ( qtc::dependencyContains( &aLeft->depInfo, &sOuterDepInfo ) == ID_TRUE ) &&
                ( qtc::dependencyContains( &aRight->depInfo, &sInnerDepInfo ) == ID_TRUE ) )
            {
                if( sJoinRelation->joinType == QMO_JOIN_RELATION_TYPE_SEMI )
                {
                    IDE_TEST( qmgSemiJoin::init( aStatement,
                                                 aLeft,
                                                 aRight,
                                                 aJoinGraph )
                              != IDE_SUCCESS );
                    break;
                }
                else if( sJoinRelation->joinType == QMO_JOIN_RELATION_TYPE_ANTI )
                {
                    IDE_TEST( qmgAntiJoin::init( aStatement,
                                                 aLeft,
                                                 aRight,
                                                 aJoinGraph )
                              != IDE_SUCCESS );
                    break;
                }
                else
                {
                    IDE_FT_ERROR( 0 );
                }
            }
            else
            {
                // Nothing to do.
            }

            // Right가 outer, left가 inner인지 확인한다.
            if( ( qtc::dependencyContains( &aRight->depInfo, &sOuterDepInfo ) == ID_TRUE ) &&
                ( qtc::dependencyContains( &aLeft->depInfo, &sInnerDepInfo ) == ID_TRUE ) )
            {
                if( sJoinRelation->joinType == QMO_JOIN_RELATION_TYPE_SEMI )
                {
                    IDE_TEST( qmgSemiJoin::init( aStatement,
                                                 aRight,
                                                 aLeft,
                                                 aJoinGraph )
                              != IDE_SUCCESS );
                    break;
                }
                else if( sJoinRelation->joinType == QMO_JOIN_RELATION_TYPE_ANTI )
                {
                    IDE_TEST( qmgAntiJoin::init( aStatement,
                                                 aRight,
                                                 aLeft,
                                                 aJoinGraph )
                              != IDE_SUCCESS );
                    break;
                }
                else
                {
                    IDE_FT_ERROR( 0 );
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

        sJoinRelation = sJoinRelation->next;
    }

    if( sJoinRelation == NULL )
    {
        IDE_TEST( qmgJoin::init( aStatement,
                                 aLeft,
                                 aRight,
                                 aJoinGraph,
                                 aExistOrderFactor )
                  != IDE_SUCCESS );
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
qmoCnfMgr::initJoinGraph( qcStatement     * aStatement,
                          UInt              aJoinGraphType,
                          qmgGraph        * aLeft,
                          qmgGraph        * aRight,
                          qmgGraph        * aJoinGraph,
                          idBool            aExistOrderFactor )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     주어진 join graph의 type에 따라 join graph를 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::initJoinGraph::__FT__" );

    switch( aJoinGraphType )
    {
        case QMG_INNER_JOIN:
            IDE_TEST( qmgJoin::init( aStatement,
                                     aLeft,
                                     aRight,
                                     aJoinGraph,
                                     aExistOrderFactor )
                      != IDE_SUCCESS );
            break;
        case QMG_SEMI_JOIN:
            IDE_TEST( qmgSemiJoin::init( aStatement,
                                         aLeft,
                                         aRight,
                                         aJoinGraph )
                      != IDE_SUCCESS );
            break;
        case QMG_ANTI_JOIN:
            IDE_TEST( qmgAntiJoin::init( aStatement,
                                         aLeft,
                                         aRight,
                                         aJoinGraph )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_FT_ERROR( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::relocateJoinGroup4PushPredHint( qcStatement      * aStatement,
                                           qmsPushPredHints * aPushPredHint,
                                           qmoJoinGroup    ** aJoinGroup,
                                           UInt               aJoinGroupCnt )
{
/***********************************************************************
 *
 * Description : PUSH_PRED hint가 적용되었을 경우의 join group 재배치
 *
 * Implementation :
 *
 *   PUSH_PRED(view) hint가 적용되었을 경우,
 *   join 조건의 다른 테이블이 view보다 먼저 수행되어야 한다.
 *
 ***********************************************************************/

    UInt                sCnt;
    UInt                i;
    qmoJoinGroup      * sNewJoinGroup;
    qmoJoinGroup      * sJoinGroup;
    qmsPushPredHints  * sPushPredHint;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::relocateJoinGroup4PushPredHint::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPushPredHint != NULL );
    IDE_DASSERT( *aJoinGroup != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinGroup = *aJoinGroup;

    //------------------------------------------
    // PUSH_PRED(view)의 outer dependency를 찾는다.
    //------------------------------------------

    for( sPushPredHint = aPushPredHint;
         sPushPredHint != NULL;
         sPushPredHint = sPushPredHint->next )
    {
        if( ( ( sPushPredHint->table->table->tableRef->flag &
                QMS_TABLE_REF_PUSH_PRED_HINT_MASK )
              == QMS_TABLE_REF_PUSH_PRED_HINT_TRUE )
            &&
            ( ( sPushPredHint->table->table->tableRef->flag &
                QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_MASK )
              == QMS_TABLE_REF_PUSH_PRED_HINT_INNER_JOIN_FALSE ) )
        {
            if( aJoinGroupCnt == 1 )
            {
                sNewJoinGroup = sJoinGroup;
            }
            else
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoJoinGroup) * aJoinGroupCnt,
                                                         (void **)&sNewJoinGroup )
                          != IDE_SUCCESS );

                for( sCnt = 0, i = 0; sCnt < aJoinGroupCnt; sCnt++ )
                {
                    // PROJ-1495 BUG-
                    // PUSH_PRED view와 연관된 table이 먼저 수행되도록 배치
                    if( qtc::dependencyContains( &(sJoinGroup[sCnt].depInfo),
                                                 &(sPushPredHint->table->table->depInfo) )
                        != ID_TRUE )
                    {
                        idlOS::memcpy( &sNewJoinGroup[i],
                                       &sJoinGroup[sCnt],
                                       ID_SIZEOF(qmoJoinGroup) );
                        i++;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }

                // PUSH_PRED view가 뒤에 수행되도록 배치
                for( sCnt = 0; sCnt < aJoinGroupCnt; sCnt++ )
                {
                    // PROJ-1495 BUG-

                    if( qtc::dependencyContains( &(sJoinGroup[sCnt].depInfo),
                                                 &(sPushPredHint->table->table->depInfo) )
                        == ID_TRUE )
                    {
                        idlOS::memcpy( &sNewJoinGroup[i],
                                       &sJoinGroup[sCnt],
                                       ID_SIZEOF(qmoJoinGroup) );
                        i++;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
            }

            sJoinGroup = sNewJoinGroup;
        }
        else
        {
            // Nothing To Do
        }
    }

    *aJoinGroup = sJoinGroup;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCnfMgr::getTableOrder( qcStatement    * aStatement,
                          qmgGraph       * aGraph,
                          qmoTableOrder ** aTableOrder )
{
/***********************************************************************
 *
 * Description : Table Order 정보 생성 및 설정
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgGraph      * sGraph;
    qmoTableOrder * sTableOrder;
    qmoTableOrder * sFirstOrder;
    qmoTableOrder * sTableOrderList;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::getTableOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sGraph = aGraph;
    sFirstOrder = *aTableOrder;

    if ( ( aGraph->type == QMG_SELECTION ) ||
         ( aGraph->type == QMG_SHARD_SELECT ) || // PROJ-2638
         ( aGraph->type == QMG_HIERARCHY ) ||
         ( aGraph->type == QMG_PARTITION ) )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoTableOrder),
                                                 (void **) & sTableOrder )
                  != IDE_SUCCESS );

        qtc::dependencySetWithDep( & sTableOrder->depInfo,
                                   & sGraph->depInfo );
        sTableOrder->next = NULL;

        if ( sFirstOrder == NULL )
        {
            sFirstOrder = sTableOrder;
        }
        else
        {
            for ( sTableOrderList = *aTableOrder;
                  sTableOrderList->next != NULL;
                  sTableOrderList = sTableOrderList->next ) ;

            sTableOrderList->next = sTableOrder;
        }

    }
    else
    {
        if ( aGraph->left != NULL )
        {
            IDE_TEST( getTableOrder( aStatement,
                                     aGraph->left,
                                     & sFirstOrder )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do
        }

        if ( aGraph->right != NULL )
        {
            IDE_TEST( getTableOrder( aStatement,
                                     aGraph->right,
                                     & sFirstOrder )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothingt to do
        }
    }

    *aTableOrder = sFirstOrder;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::connectJoinPredToJoinGraph( qcStatement   * aStatement,
                                       qmoPredicate ** aJoinPred,
                                       qmgGraph      * aJoinGraph )
{
/***********************************************************************
 *
 * Description : Join Group의 join predicate에서 현재 Join Graph에 해당하는
 *               join predicate을 분리하여 Join Graph에 연결한다.
 *
 * Implementaion :
 *
 *     Ex)  [p2] Predicate을 Join Graph에 연결하는 경우
 *
 *                             sPrevJoinPred
 *                               |
 *                               |       sCurJoinPred
 *                               |        |
 *                               V        V
 *              [JoinGroup] --> [p1] --> [p2] --> [p3]
 *
 *              [JoinGraph] --> [q1] --> [q2] --> [q3]
 *                               +                  +
 *                               |                  |
 *                             sJoinGraphPred      sCurJoinGraphPred
 *
 *         ==>  [p2] Predicate 연결 후
 *
 *                             sPrevJoinPred
 *                               |
 *                               |       sCurJoinPred
 *                               |        |
 *                               V        V
 *              [JoinGroup] --> [p1] --> [p3]
 *
 *              [JoinGraph] --> [q1] --> [q2] --> [q3] --> [p2]
 *                               +                           +
 *                               |                           |
 *                             sJoinGraphPred         sCurJoinGraphPred
 *
 ***********************************************************************/

    qmoPredicate * sJoinPred;
    qmoPredicate * sPrevJoinPred;
    qmoPredicate * sCurJoinPred;
    qmoPredicate * sJoinGraphPred;
    qmoPredicate * sCurJoinGraphPred;
    qmoPredicate * sCopiedJoinPred;
    qcDepInfo    * sJoinDependencies;
    qcDepInfo    * sFromDependencies;
    qcDepInfo      sAndDependencies;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::connectJoinPredToJoinGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    // join predicate ( Join Group 또는 CNF의 joinPredicate )
    sJoinPred            = *aJoinPred;
    sCurJoinPred         = *aJoinPred;
    sPrevJoinPred        = NULL;

    // Join Graph이 join predicate
    sJoinGraphPred       = NULL;
    sCurJoinGraphPred    = NULL;
    sJoinDependencies    = & aJoinGraph->depInfo;
    sFromDependencies    = & aJoinGraph->myQuerySet->SFWGH->depInfo;

    while( sCurJoinPred != NULL )
    {
        //------------------------------------------------
        //  Join Graph에 포함되는 Join Predicate인지 판단
        //  ~(Graph Dep) & (From & Pred Dep) == 0 : 포함됨
        //------------------------------------------------

        qtc::dependencyAnd( sFromDependencies,
                            & sCurJoinPred->node->depInfo,
                            & sAndDependencies );

        if ( qtc::dependencyContains( sJoinDependencies,
                                      & sAndDependencies ) == ID_TRUE )
        {
            //------------------------------------------
            // Join Graph의 join predicate인 경우
            //------------------------------------------

            // Join Group또는 CNF에서 현재 Join Predicate을 분리
            if ( sPrevJoinPred == NULL )
            {
                sJoinPred = sCurJoinPred->next;
            }
            else
            {
                sPrevJoinPred->next = sCurJoinPred->next;
            }

            IDE_TEST( qmoPred::copyOnePredicate( QC_QMP_MEM( aStatement ),
                                                 sCurJoinPred,
                                                 &sCopiedJoinPred )
                      != IDE_SUCCESS );

            // 분리한 Join Predicate을 Join Graph의 myPredicate에 연결
            if ( sJoinGraphPred == NULL )
            {
                sJoinGraphPred = sCurJoinGraphPred = sCopiedJoinPred;
                sCurJoinPred = sCurJoinPred->next;
                sCurJoinGraphPred->next = NULL;
            }
            else
            {
                sCurJoinGraphPred->next = sCopiedJoinPred;
                sCurJoinPred = sCurJoinPred->next;

                // To Fix BUG-8219
                sCurJoinGraphPred = sCurJoinGraphPred->next;

                sCurJoinGraphPred->next = NULL;
            }
        }
        else
        {
            //------------------------------------------
            // Join Graph의 join predicate이 아닌 경우
            //------------------------------------------

            sPrevJoinPred = sCurJoinPred;
            sCurJoinPred = sCurJoinPred->next;
        }
    }

    *aJoinPred = sJoinPred;
    aJoinGraph->myPredicate = sJoinGraphPred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::joinGroupingWithJoinPred( qmoJoinGroup  * aJoinGroup,
                                     qmoCNF        * aCNF,
                                     UInt          * aJoinGroupCnt )
{
/***********************************************************************
 *
 * Description : JoinPredicate을 이용하여 Join Grouping을 수행
 *
 * Implementation :
 *       Join Group에 포함되지 않은 Join Predicate이존재하지 않을때까지
 *       다음을 수행한다.
 *       (1) 새로운 Join Group 설정
 *       (2) Join Group에 포함되는 모든 Join Predicate을 등록
 *
 *        < 참고 >
 *        등록 시에 기존 Join Predicate List에서 해당 Predicate의 연결을
 *        끊는다. 따라서 Join Predicate List가 존재하지 않으면
 *        Join Group에 포함되지 않은 Join Predicate이 없음을 의미한다.
 *
 *
 *    Ex)
 *        Join Group에 포함되는 모든 Join Predicate의 등록
 *
 *        [p2] Predicate을 해당 Join Group에 연결하는 경우
 *
 *                             sFirstPred
 *                               |
 *                               |   sPrevPred
 *                               |        |    sAddPred
 *                               |        |    sCurPred
 *                               |        |        |
 *                               V        V        V
 *     [CNF::joinPredicate] --> [p1] --> [p2] --> [p3] --> [p4]
 *
 *             [Join Group] --> [q1] --> [q2] --> [q3]
 *
 *
 *      ==> CNF의 joinPredicate에서 연결을 끊고, sCurUpperPred를 다음으로 진행
 *
 *                             sFirstPred
 *                               |    sPrevJoinPred
 *                               |         | sAddPred
 *                               |         |   |
 *                               |         |   V
 *                               |         |  [p3]  sCurPred
 *                               |         |     \    |
 *                               V         V      V   V
 *     [CNF::joinPredicate] --> [p1] --> [p2] -----> [p4]
 *
 *             [Join Group] --> [q1] --> [q2] --> [q3]
 *
 *
 *     ==>  [p3] Predicate 연결
 *
 *                             sFirstPred
 *                               |   sPrevUpperPred
 *                               |        |     sCurUpperPred
 *                               |        |        |
 *                               V        V        V
 *     [CNF::joinPredicate] --> [p1] --> [p2] --> [p4]
 *
 *                           +[p3]
 *                          /      \
 *                         /        V
 *             [Join Group] --X--> [q1] --> [q2] --> [q3]
 *
 *
 *    ==>  [p3] Predicate 연결 후
 *
 *                             sFirstPred
 *                               |   sPrevUpperPred
 *                               |        |     sCurUpperPred
 *                               |        |        |
 *                               V        V        V
 *     [CNF::joinPredicate] --> [p1] --> [p2] --> [p4]
 *
 *             [Join Group] --> [p3]-->[q1] --> [q2] --> [q3]
 *
 *
 ***********************************************************************/


    UInt           sJoinGroupCnt;
    qmoJoinGroup * sJoinGroup;
    qmoJoinGroup * sCurJoinGroup;
    qmoPredicate * sFirstPred;
    qmoPredicate * sAddPred = NULL;
    qmoPredicate * sCurPred;
    qmoPredicate * sPrevPred;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::joinGroupingWithJoinPred::__FT__" );

    //------------------------------------------------------
    //  sFirstPred : 새로운 Join Group이 필요한 Predicate
    //  sAddPred : Join Group에 포함되는 Join Predicate의 연결 관리
    //  sCurPred : Join Predicate의 Traverse 관리
    //  sPrevPred : Join Group에 포함되지 않은 Join Predicate의 연결 관리
    //------------------------------------------------------

    qcDepInfo      sAndDependencies;

    idBool         sIsInsert;

    UInt               i;
    qcDepInfo          sGraphDepInfo;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinGroup != NULL );
    IDE_DASSERT( aCNF->joinPredicate != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinGroup    = aJoinGroup;
    sJoinGroupCnt = aCNF->joinGroupCnt;
    sFirstPred    = aCNF->joinPredicate;

    //------------------------------------------
    // JoinGroup의 dependencies 설정 및 관련 JoinPredicate 연결
    //------------------------------------------

    while ( sFirstPred != NULL )
    {

        //------------------------------------------
        // Join Group에 속하지 않는 첫번째 Predicate 존재 :
        //    Join Group에 속하지 않은 첫번째 Predicate의
        //    dependencies로 새로운 Join Group의 dependencies를 설정
        //------------------------------------------

        sCurJoinGroup = & sJoinGroup[sJoinGroupCnt++];

        IDE_TEST( qtc::dependencyOr( & sCurJoinGroup->depInfo,
                                     & sFirstPred->node->depInfo,
                                     & sCurJoinGroup->depInfo )
                  != IDE_SUCCESS );

        //------------------------------------------
        // BUG-19371
        // base graph가 selection이 아닌 join일 경우
        // join predicate을 어떻게 작성하느냐에따라
        // join grouping에 중복 포함될 수 있다.
        // 중복 포함되지 않도록 해야 함.
        //------------------------------------------

        for( i = 0;
             i < aCNF->graphCnt4BaseTable;
             i++ )
        {
            if( ( aCNF->baseGraph[i]->type == QMG_INNER_JOIN ) ||
                ( aCNF->baseGraph[i]->type == QMG_LEFT_OUTER_JOIN ) ||
                ( aCNF->baseGraph[i]->type == QMG_FULL_OUTER_JOIN ) )
            {
                sGraphDepInfo = aCNF->baseGraph[i]->depInfo;

                qtc::dependencyAnd( & sGraphDepInfo,
                                    & sFirstPred->node->depInfo,
                                    & sAndDependencies );

                if( qtc::dependencyEqual( & sAndDependencies,
                                          & qtc::zeroDependencies ) == ID_FALSE )
                {
                    IDE_TEST( qtc::dependencyOr( & sCurJoinGroup->depInfo,
                                                 & sGraphDepInfo,
                                                 & sCurJoinGroup->depInfo )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing To Do
                }
            }
        }

        //------------------------------------------
        // 현재 JoinGroup과 연관된 모든 Join Predicate을 찾아
        // JoinGroup에 등록
        //------------------------------------------

        sCurPred  = sFirstPred;
        sPrevPred = NULL;
        sIsInsert = ID_FALSE;
        while( sCurPred != NULL )
        {

            qtc::dependencyAnd( & sCurJoinGroup->depInfo,
                                & sCurPred->node->depInfo,
                                & sAndDependencies );

            if ( qtc::dependencyEqual( & sAndDependencies,
                                       & qtc::zeroDependencies )== ID_FALSE )
            {
                //------------------------------------------
                // Join Group과 연관된 Predicate
                //------------------------------------------

                // Join Group에 새롭게 등록하는 Predicate이 존재
                sIsInsert = ID_TRUE;

                // Join Group의 등록
                IDE_TEST( qtc::dependencyOr( & sCurJoinGroup->depInfo,
                                             & sCurPred->node->depInfo,
                                             & sCurJoinGroup->depInfo )
                          != IDE_SUCCESS );

                // Join Group의 등록
                //------------------------------------------
                // BUG-19371
                // base graph가 selection이 아닌 join일 경우
                // join predicate을 어떻게 작성하느냐에따라
                // join grouping에 중복 포함될 수 있다.
                // 중복 포함되지 않도록 해야 함.
                //------------------------------------------

                for( i = 0;
                     i < aCNF->graphCnt4BaseTable;
                     i++ )
                {
                    if( ( aCNF->baseGraph[i]->type == QMG_INNER_JOIN ) ||
                        ( aCNF->baseGraph[i]->type == QMG_LEFT_OUTER_JOIN ) ||
                        ( aCNF->baseGraph[i]->type == QMG_FULL_OUTER_JOIN ) )
                    {
                        sGraphDepInfo = aCNF->baseGraph[i]->depInfo;

                        qtc::dependencyAnd( & sGraphDepInfo,
                                            & sCurPred->node->depInfo,
                                            & sAndDependencies );

                        if( qtc::dependencyEqual( & sAndDependencies,
                                                  & qtc::zeroDependencies ) == ID_FALSE )
                        {
                            IDE_TEST( qtc::dependencyOr( & sCurJoinGroup->depInfo,
                                                         & sGraphDepInfo,
                                                         & sCurJoinGroup->depInfo )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing To Do
                        }
                    }
                }

                // Predicate이 Join Group에 포함되었음을 설정
                sCurPred->flag &= ~QMO_PRED_INCLUDE_JOINGROUP_MASK;
                sCurPred->flag |= QMO_PRED_INCLUDE_JOINGROUP_TRUE;

                // Predicate을 Join Predicate에서 연결을 끊고,
                // JoinGroup의 joinPredicate에 연결시킴

                if ( sCurJoinGroup->joinPredicate == NULL )
                {
                    sCurJoinGroup->joinPredicate = sCurPred;
                    sAddPred = sCurPred;
                }
                else
                {
                    IDE_FT_ASSERT( sAddPred != NULL );
                    sAddPred->next = sCurPred;
                    sAddPred = sAddPred->next;
                }
                sCurPred = sCurPred->next;
                sAddPred->next = NULL;

                if ( sPrevPred == NULL )
                {
                    sFirstPred = sCurPred;
                }
                else
                {
                    sPrevPred->next = sCurPred;
                }
            }
            else
            {
                // Join Group과 연관되지 않은 Predicate : nothing to do
                sPrevPred = sCurPred;
                sCurPred = sCurPred->next;
            }

            if ( ( sCurPred == NULL ) && ( sIsInsert == ID_TRUE ) )
            {
                //------------------------------------------
                // Join Group에 추가 등록한 Predicate이 존재하는 경우,
                // 추가된 Predicate과 연관된 Join Predicate이 존재할 수 있음
                //------------------------------------------

                sCurPred = sFirstPred;
                sPrevPred = NULL;
                sIsInsert = ID_FALSE;
            }
            else
            {
                // 더 이상 연관된 Predicate 없음 : nothing to do
            }
        }
    }

    *aJoinGroupCnt = sJoinGroupCnt;
    // CNF의 joinPredicate을 joinGroup의 joinPredicate으로 모두 분류시킴
//    *aJoinPredicate = NULL;
    aCNF->joinPredicate = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCnfMgr::connectBaseGraphToJoinGroup( qmoJoinGroup   * aJoinGroup,
                                        qmgGraph      ** aCNFBaseGraph,
                                        UInt             aCNFBaseGraphCnt )
{
/***********************************************************************
 *
 * Description : Join Group에 속하는 base graph 연결
 *
 * Implementaion :
 *
 ***********************************************************************/

    qmoJoinGroup     * sJoinGroup;
    UInt               sCNFBaseGraphCnt;
    qmgGraph        ** sCNFBaseGraph;
    UInt               sJoinGroupBaseGraphCnt;
    UInt               i;

    qcDepInfo          sAndDependencies;
    qcDepInfo          sGraphDepInfo;

    qmsPushPredHints * sPushPredHint;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::connectBaseGraphToJoinGroup::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinGroup != NULL );
    IDE_DASSERT( aCNFBaseGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinGroup             = aJoinGroup;
    sCNFBaseGraphCnt       = aCNFBaseGraphCnt;
    sCNFBaseGraph          = aCNFBaseGraph;
    sJoinGroupBaseGraphCnt = 0;

    for ( i = 0; i < sCNFBaseGraphCnt; i++ )
    {
        // PROJ-1495 BUG-
        for( sPushPredHint =
                 sCNFBaseGraph[i]->myQuerySet->SFWGH->hints->pushPredHint;
             sPushPredHint != NULL;
             sPushPredHint = sPushPredHint->next )
        {
            if( ( ( sPushPredHint->table->table->tableRef->flag &
                    QMS_TABLE_REF_PUSH_PRED_HINT_MASK )
                  == QMS_TABLE_REF_PUSH_PRED_HINT_TRUE ) &&
                ( sPushPredHint->table->table == sCNFBaseGraph[i]->myFrom ) )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if( sPushPredHint != NULL )
        {
            sGraphDepInfo = sCNFBaseGraph[i]->myFrom->depInfo;
        }
        else
        {
            sGraphDepInfo = sCNFBaseGraph[i]->depInfo;
        }

        qtc::dependencyAnd( & sJoinGroup->depInfo,
                            & sGraphDepInfo,
                            & sAndDependencies );

        if ( qtc::dependencyEqual( & sAndDependencies,
                                   & qtc::zeroDependencies ) == ID_FALSE )
        {
            //------------------------------------------
            // ( Join Group의 dependencies & BaseGraph의 dependencies )
            //  != 0 이면, Join Group에 속하는 Base Graph
            //------------------------------------------

            sJoinGroup->baseGraph[sJoinGroupBaseGraphCnt++] =
                sCNFBaseGraph[i];

            sCNFBaseGraph[i]->flag &= ~QMG_INCLUDE_JOIN_GROUP_MASK;
            sCNFBaseGraph[i]->flag |= QMG_INCLUDE_JOIN_GROUP_TRUE;
        }
        else
        {
            // nothing to do
        }
    }
    sJoinGroup->baseGraphCnt = sJoinGroupBaseGraphCnt;

    return IDE_SUCCESS;
}

idBool
qmoCnfMgr::existJoinRelation( qmoJoinRelation * aJoinRelationList,
                              qcDepInfo       * aDepInfo )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery Unnesting
 *     aDepInfo와 동일한 dependency를 갖는 join relation이 존재하는지
 *     확인한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoJoinRelation * sCurJoinRelation;

    sCurJoinRelation = aJoinRelationList;

    while ( sCurJoinRelation != NULL )
    {
        if ( qtc::dependencyEqual( & sCurJoinRelation->depInfo,
                                   aDepInfo )
             == ID_TRUE )
        {
            return ID_TRUE;
        }
        sCurJoinRelation = sCurJoinRelation->next;
    }

    return ID_FALSE;
}

IDE_RC
qmoCnfMgr::getInnerTable( qcStatement * aStatement,
                          qtcNode     * aNode,
                          SInt        * aTableID )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery Unnesting
 *     Semi/anti join의 predicate에서 inner table의 ID를 얻어온다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt      sTable;
    qmsFrom * sFrom;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::getInnerTable::__FT__" );

    IDE_FT_ERROR( ( ( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK )
                    == QTC_NODE_JOIN_TYPE_SEMI ) ||
                  ( ( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK )
                    == QTC_NODE_JOIN_TYPE_ANTI ) );

    sTable = qtc::getPosFirstBitSet( &aNode->depInfo );
    while( sTable != QTC_DEPENDENCIES_END )
    {
        sFrom = QC_SHARED_TMPLATE( aStatement )->tableMap[sTable].from;

        if( qtc::dependencyContains( &sFrom->semiAntiJoinDepInfo, &aNode->depInfo ) == ID_TRUE )
        {
            *aTableID = sTable;
            break;
        }
        else
        {
            // Nothing to do.
        }
        sTable = qtc::getPosNextBitSet( &aNode->depInfo, sTable );
    }

    IDE_FT_ERROR( sTable != QTC_DEPENDENCIES_END );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::makeSemiAntiJoinRelationList( qcStatement      * aStatement,
                                         qtcNode          * aNode,
                                         qmoJoinRelation ** aJoinRelationList,
                                         UInt             * aJoinRelationCnt )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery Unnesting
 *     makeJoinRelationList와 동일 역할을 하는 semi/anti join용 함수이다.
 *     makeJoinRelationList와 거의 유사하지만 다음과 같은 상황에서
 *     다르게 동작할 수 있다.
 *     ex) T1.I1 + T3.I1 = T2.I1, 이 때 inner table이 T3인 경우
 *         T1 -> T3, T2 -> T3 로 두 개의 join relation을 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoJoinRelation     * sLastJoinRelation;
    qmoJoinRelation     * sNewJoinRelation;
    qcDepInfo             sDepInfo;
    qcDepInfo             sInnerDepInfo;
    qmoJoinRelationType   sJoinType;
    UInt                  sJoinRelationCnt;
    SInt                  sInnerTable;
    SInt                  sTable;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::makeSemiAntiJoinRelationList::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinRelationCnt  = *aJoinRelationCnt;
    sLastJoinRelation = *aJoinRelationList;

    // aJoinRelationList의 마지막 node를 찾는다.
    if( sLastJoinRelation != NULL )
    {
        while( sLastJoinRelation->next != NULL )
        {
            sLastJoinRelation = sLastJoinRelation->next;
        }
    }
    else
    {
        // Nothing to do.
    }

    switch( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK )
    {
        case QTC_NODE_JOIN_TYPE_SEMI:
            sJoinType = QMO_JOIN_RELATION_TYPE_SEMI;
            break;
        case QTC_NODE_JOIN_TYPE_ANTI:
            sJoinType = QMO_JOIN_RELATION_TYPE_ANTI;
            break;
        default:
            // Semi/anti join 외 올 수 없다.
            IDE_FT_ERROR( 0 );
    }

    IDE_TEST( getInnerTable( aStatement, aNode, &sInnerTable )
              != IDE_SUCCESS );

    qtc::dependencySet( (UShort)sInnerTable, &sInnerDepInfo );

    sTable = qtc::getPosFirstBitSet( &aNode->depInfo );
    while( sTable != QTC_DEPENDENCIES_END )
    {
        if( sTable != sInnerTable )
        {
            qtc::dependencySet( (UShort)sTable, &sDepInfo );

            qtc::dependencyOr( &sDepInfo, &sInnerDepInfo, &sDepInfo );

            if( existJoinRelation( *aJoinRelationList, &sDepInfo ) == ID_FALSE )
            {
                // 동일한 join relation이 존재하지 않으면 새로 생성

                sJoinRelationCnt++;

                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinRelation ),
                                                         (void**) & sNewJoinRelation )
                          != IDE_SUCCESS );

                qtc::dependencySetWithDep( &sNewJoinRelation->depInfo,
                                           &sDepInfo );

                sNewJoinRelation->joinType      = sJoinType;
                sNewJoinRelation->innerRelation = (UShort)sInnerTable;
                sNewJoinRelation->next          = NULL;

                // Join Relation 연결
                if ( *aJoinRelationList == NULL )
                {
                    *aJoinRelationList = sNewJoinRelation;
                }
                else
                {
                    sLastJoinRelation->next = sNewJoinRelation;
                }
                sLastJoinRelation = sNewJoinRelation;
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

        sTable = qtc::getPosNextBitSet( &aNode->depInfo, sTable );
    }

    *aJoinRelationCnt  = sJoinRelationCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::makeCrossJoinRelationList( qcStatement      * aStatement,
                                      qmoJoinGroup     * aJoinGroup,
                                      qmoJoinRelation ** aJoinRelationList,
                                      UInt             * aJoinRelationCnt )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     Semi/anti join의 outer relation이 여럿인 경우 각 outer relation간
 *     cartesian product가 가능하도록 join을 생성한다.
 *
 * Implementation :
 *     Semi/anti join의 inner relation을 찾아 outer relation들의 depInfo를
 *     보고 각 join을 생성한다.
 *     이 때 cartesian product의 대상이 되는 두 relation이 이미
 *     ANSI style로 join되어 있는 경우에는 생성하지 않도록 한다.
 *
 ***********************************************************************/

    qmsFrom   * sFrom;
    qcDepInfo   sDepInfo;
    SInt        sJoinGroupTable;
    SInt        sOuterTable;
    SInt        sLastTable;
    UInt        i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::makeCrossJoinRelationList::__FT__" );

    sJoinGroupTable = qtc::getPosFirstBitSet( &aJoinGroup->depInfo );
    while( sJoinGroupTable != QTC_DEPENDENCIES_END )
    {
        sFrom = QC_SHARED_TMPLATE( aStatement )->tableMap[sJoinGroupTable].from;

        if( qtc::haveDependencies( &sFrom->semiAntiJoinDepInfo ) == ID_TRUE )
        {
            // Semi/anti join인 경우

            sLastTable = -1;
            sOuterTable = qtc::getPosFirstBitSet( &sFrom->semiAntiJoinDepInfo );
            while( sOuterTable != QTC_DEPENDENCIES_END )
            {
                if( sOuterTable != sFrom->tableRef->table )
                {
                    if( sLastTable != -1 )
                    {
                        sDepInfo.depCount = 2;
                        sDepInfo.depend[0] = sLastTable;
                        sDepInfo.depend[1] = sOuterTable;

                        // Cartesian product 생성 대상인 두 relation이 ansi style로 join되어있다면
                        // 생성하지 않고 무시해야 한다.
                        for( i = 0; i < aJoinGroup->baseGraphCnt; i++ )
                        {
                            // Cartesian product 대상의 두 relation이 모두 하나의 base graph에
                            // 속해있다면 이미 ANSI style로 join된 경우이다.
                            if( qtc::dependencyContains( &aJoinGroup->baseGraph[i]->depInfo,
                                                         &sDepInfo ) == ID_TRUE )
                            {
                                break;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }

                        if( i == aJoinGroup->baseGraphCnt )
                        {
                            IDE_TEST( makeJoinRelationList( aStatement,
                                                            &sDepInfo,
                                                            aJoinRelationList,
                                                            aJoinRelationCnt )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // ANSI style로 join된 경우이므로 무시
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sLastTable = sOuterTable;
                }
                else
                {
                    // Nothing to do.
                }
                sOuterTable = qtc::getPosNextBitSet( &sFrom->semiAntiJoinDepInfo, sOuterTable );
            }
        }
        else
        {
            // Inner join인 경우
        }
        sJoinGroupTable = qtc::getPosNextBitSet( &aJoinGroup->depInfo, sJoinGroupTable );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::makeJoinRelationList( qcStatement      * aStatement,
                                 qcDepInfo        * aDependencies,
                                 qmoJoinRelation ** aJoinRelationList,
                                 UInt             * aJoinRelationCnt )
{
/***********************************************************************
 *
 * Description : qmoJoinRelation의 생성 및 초기화
 *
 ***********************************************************************/

    qmoJoinRelation * sCurJoinRelation  = NULL;  // for traverse
    qmoJoinRelation * sLastJoinRelation = NULL;  // for traverse
    qmoJoinRelation * sNewJoinRelation  = NULL;  // 새로 생성된 join relation
    idBool            sExistCommon;
    UInt              sJoinRelationCnt;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::makeJoinRelationList::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sExistCommon      = ID_FALSE;
    sJoinRelationCnt  = *aJoinRelationCnt;

    //------------------------------------------
    // 동일 Join Relation 존재 여부 검사
    // ( Join Relation List의 마지막을 sLastJoinRelation을 가리키도록 설정 )
    //------------------------------------------

    sCurJoinRelation = *aJoinRelationList;

    while ( sCurJoinRelation != NULL )
    {
        if ( qtc::dependencyEqual( & sCurJoinRelation->depInfo,
                                   aDependencies )
             == ID_TRUE )
        {
            sExistCommon = ID_TRUE;
        }
        sLastJoinRelation = sCurJoinRelation;
        sCurJoinRelation = sCurJoinRelation->next;
    }

    if ( sExistCommon == ID_FALSE )
    {
        //------------------------------------------
        // 동일한 Join Relation이 존재하지 않으면,
        // Join Relation 생성
        //------------------------------------------

        sJoinRelationCnt++;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinRelation ),
                                                 (void**) & sNewJoinRelation )
                     != IDE_SUCCESS );

        qtc::dependencyClear( & sNewJoinRelation->depInfo );
        IDE_TEST( qtc::dependencyOr( & sNewJoinRelation->depInfo,
                                     aDependencies,
                                     & sNewJoinRelation->depInfo )
                  != IDE_SUCCESS );

        sNewJoinRelation->joinType = QMO_JOIN_RELATION_TYPE_INNER;
        sNewJoinRelation->innerRelation = ID_USHORT_MAX;
        sNewJoinRelation->next = NULL;

        // Join Relation 연결
        if ( *aJoinRelationList == NULL )
        {
            *aJoinRelationList = sNewJoinRelation;
        }
        else
        {
            sLastJoinRelation->next = sNewJoinRelation;
        }
    }
    else
    {
        // 동일한 Join Relation이 존재하는 경우
        // nothing to do
    }

    *aJoinRelationCnt  = sJoinRelationCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCnfMgr::isTwoConceptTblNode( qtcNode       * aNode,
                                qmgGraph     ** aBaseGraph,
                                UInt            aBaseGraphCnt,
                                idBool        * aIsTwoConceptTblNode)
{
/***********************************************************************
 *
 * Description : 두 개의 conceptual table로만 구성된 Node 여부
 *
 * Implementation :
 *     (1) 첫번째 관련 Graph를 찾는다.
 *     (2) 두번째 관련 Graph를 찾는다.
 *     (3) 두 개의 Concpetual Table만이 관련된 Node인지 검사한다.
 *         Node & ~( 첫번째 Graph | 두 번째 Graph ) == 0
 *
 ***********************************************************************/

    qtcNode   * sNode;
    qmgGraph  * sFirstGraph;
    qmgGraph  * sSecondGraph;
    qcDepInfo   sResultDependencies;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::isTwoConceptTblNode::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aBaseGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNode         = aNode;

    // sNode와 연관된 두 개의 Graph를 찾는다.
    IDE_TEST( getTwoRelatedGraph( & sNode->depInfo,
                                  aBaseGraph,
                                  aBaseGraphCnt,
                                  & sFirstGraph,
                                  & sSecondGraph )
              != IDE_SUCCESS );

    //------------------------------------------
    // 두 개의 Concpetual Table만이 관련된 Node인지 검사
    //------------------------------------------

    if ( sFirstGraph != NULL && sSecondGraph != NULL )
    {
        // Predicate & ~( First Graph | Seconde Graph )
        IDE_TEST( qtc::dependencyOr( & sFirstGraph->depInfo,
                                     & sSecondGraph->depInfo,
                                     & sResultDependencies )
                  != IDE_SUCCESS );

        if ( qtc::dependencyContains( & sResultDependencies,
                                      & sNode->depInfo ) == ID_TRUE)
        {
            // Predicate & ~( First Graph | Seconde Graph ) == 0 이면,
            // 두개의 conceptual table만 관련된 Predicate
            *aIsTwoConceptTblNode = ID_TRUE;
        }
        else
        {
            *aIsTwoConceptTblNode = ID_FALSE;
        }
    }
    else
    {
        *aIsTwoConceptTblNode = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCnfMgr::getTwoRelatedGraph( qcDepInfo * aDependencies,
                               qmgGraph ** aBaseGraph,
                               UInt        aBaseGraphCnt,
                               qmgGraph ** aFirstGraph,
                               qmgGraph ** aSecondGraph )
{
/***********************************************************************
 *
 * Description : base graph들 중에서 연관된 두 개의 graph를 반환
 *
 * Implementation :
 *
 ***********************************************************************/

    qcDepInfo        * sDependencies;
    UInt               sBaseGraphCnt;
    qmgGraph        ** sBaseGraph;
    qmgGraph         * sCurGraph;
    qmgGraph         * sFirstGraph;
    qmgGraph         * sSecondGraph;
    UInt               i;
    qcDepInfo          sAndDependencies;
    qcDepInfo          sGraphDepInfo;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::getTwoRelatedGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aDependencies != NULL );
    IDE_DASSERT( aBaseGraph != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sDependencies = aDependencies;
    sBaseGraph    = aBaseGraph;
    sBaseGraphCnt = aBaseGraphCnt;
    sFirstGraph   = NULL;
    sSecondGraph  = NULL;


    for ( i = 0; i < sBaseGraphCnt; i++ )
    {
        sCurGraph = sBaseGraph[i];

        sGraphDepInfo = sCurGraph->depInfo;

        qtc::dependencyAnd( & sGraphDepInfo,
                            sDependencies,
                            & sAndDependencies );

        if ( qtc::dependencyEqual( & sAndDependencies,
                                   & qtc::zeroDependencies ) == ID_FALSE )
        {
            //------------------------------------------
            // ( Node의 dependencies & Graph의 dependencies ) != 0 이면,
            // 현재 Graph는 Node와 관련있는 Graph이다.
            //------------------------------------------

            if ( sFirstGraph == NULL )
            {
                sFirstGraph = sCurGraph;
            }
            else
            {
                sSecondGraph = sCurGraph;
                break;
            }
        }
        else
        {
            // nothing to do
        }
    }

    *aFirstGraph = sFirstGraph;
    *aSecondGraph = sSecondGraph;

    return IDE_SUCCESS;
}

IDE_RC qmoCnfMgr::pushSelection4ConstantFilter(qcStatement *aStatement,
                                               qmgGraph    *aGraph,
                                               qmoCNF      *aCNF)
{
    /*
     * BUG-29551: aggregation이 있는 view에 constant filter를
     *            pushdown하면 질의 결과가 틀립니다
     *
     * 일반 filter 의 push selection 과는 다르게
     * const filter 에 대해서는 가능한 최상위 노드에 filter 를 연결한다
     * 그렇게 하면 하위 plan 을 수행하기 전에 미리 수행 할지 말지를 알 수 있다
     */
    qmgGraph     * sCurGraph = NULL;
    qmoPredicate * sLastConstantPred = NULL;
    qmoPredicate * sTempConstantPred = NULL;
    qmoPredicate * sSrcConstantPred = NULL;
    qmgChildren  * sChildren = NULL;
    idBool         sFind     = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::pushSelection4ConstantFilter::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT(aGraph != NULL);
    IDE_DASSERT(aCNF   != NULL);

    sCurGraph = aGraph;

    if (aCNF->constantPredicate == NULL)
    {
        IDE_CONT(NORMAL_EXIT);
    }
    else
    {
    }

    while (sCurGraph != NULL )
    {
        // fix BUG-9791, BUG-10419
        // fix BUG-19093

        if (sCurGraph->left == NULL)
        {
            break;
        }
        else
        {
            // Nothing to do
        }

        if (sCurGraph->type == QMG_SET)
        {
            // fix BUG-19093
            // constant filter를 SET절 안으로 내린다.
            // BUG-36803 sCurGraph->left must not be null!
            IDE_TEST( pushSelection4ConstantFilter( aStatement,
                                                    sCurGraph->left,
                                                    aCNF )
                      != IDE_SUCCESS );

            if( sCurGraph->right != NULL )
            {
                IDE_TEST( pushSelection4ConstantFilter( aStatement,
                                                        sCurGraph->right,
                                                        aCNF )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do
            }

            for (sChildren = sCurGraph->children;
                 sChildren != NULL;
                 sChildren = sChildren->next)
            {
                IDE_TEST( pushSelection4ConstantFilter( aStatement,
                                                        sChildren->childGraph,
                                                        aCNF )
                          != IDE_SUCCESS );
            }
            break;
        }
        else
        {
            /*
             * 현재 graph 가 selection 인 경우
             * left graph 가 view materialization 인 경우,
             * 현재 graph 가 full, left outer join 인 경우,
             * constant filter를 연결한다.
             */
            if ( ( sCurGraph->type == QMG_SELECTION ) ||
                 ( sCurGraph->type == QMG_PARTITION ) )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            if (sCurGraph->myFrom != NULL)
            {
                // BUG-47772 sCurGraph->left가 PROJ 그래프여야 한다.
                if ((((sCurGraph->left->flag & QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK)
                      == QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE ) &&
                     (sCurGraph->left->type == QMG_PROJECTION)) ||
                    (sCurGraph->type == QMG_FULL_OUTER_JOIN) ||
                    (sCurGraph->type == QMG_LEFT_OUTER_JOIN))
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                /* PROJ-1715
                 * Hierarchy인 경우 외부 참조에 의한 constantPredicate일 경우
                 * SCAN으로 내리면 않되고 상위에서 처리해야한다.
                 */
                if ( ( ( sCurGraph->left->flag & QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK )
                       == QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE ) &&
                     ( sCurGraph->left->type == QMG_PROJECTION ) )
                {
                    sFind = ID_FALSE;
                    for ( sSrcConstantPred = aCNF->constantPredicate;
                          sSrcConstantPred != NULL;
                          sSrcConstantPred = sSrcConstantPred->next )
                    {
                        if ( qtc::dependencyEqual( &sSrcConstantPred->node->depInfo,
                                                   &qtc::zeroDependencies )
                             != ID_TRUE )
                        {
                            sFind = ID_TRUE;
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }

                    if ( sFind == ID_TRUE )
                    {
                        break;
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
            }
        }

        sCurGraph = sCurGraph->left;
    }

    // constant predicate 연결
    if( sCurGraph != NULL )
    {
        // fix BUG-19212
        // subquery가 있으면 복사하지 않고, 주소만 달아놓는다.
        // SET절에 양쪽으로 (outer column reference가 있는 subquery)를
        // 내리는 경우는 없다.
        // outer column ref.가 있는 subquery를 view에
        // 내리지 못하도록 위의 while loop에서 처리했기 때문이다.

        if( ( aCNF->constantPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK )
            == QTC_NODE_SUBQUERY_EXIST )
        {
            if( sCurGraph->constantPredicate == NULL )
            {
                sCurGraph->constantPredicate = aCNF->constantPredicate;
            }
            else
            {
                // 현재 graph에 또다른 constant filter가 있는 경우
                // constant filter의 연결관계구성
                for( sLastConstantPred = sCurGraph->constantPredicate;
                     sLastConstantPred->next != NULL;
                     sLastConstantPred = sLastConstantPred->next ) ;

                sLastConstantPred->next = aCNF->constantPredicate;
            }
        }
        else
        {
            // constant predicate을 복사해서 내린다.

            for( sSrcConstantPred = aCNF->constantPredicate;
                 sSrcConstantPred != NULL;
                 sSrcConstantPred = sSrcConstantPred->next )
            {
                // Predicate 복사
                IDE_TEST( qmoPred::copyOneConstPredicate( QC_QMP_MEM( aStatement ),
                                                          sSrcConstantPred,
                                                          & sTempConstantPred )
                          != IDE_SUCCESS );

                if( sCurGraph->constantPredicate == NULL )
                {
                    sCurGraph->constantPredicate = sTempConstantPred;
                }
                else
                {
                    for( sLastConstantPred = sCurGraph->constantPredicate;
                         sLastConstantPred->next != NULL;
                         sLastConstantPred = sLastConstantPred->next ) ;

                    sLastConstantPred->next = sTempConstantPred;
                }
            }
        }
    }
    else
    {
        IDE_DASSERT(0);
    }

    IDE_EXCEPTION_CONT(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::getPushPredPosition( qmoPredicate  * aPredicate,
                                qmgGraph     ** aBaseGraph,
                                qcDepInfo     * aFromDependencies,
                                qmsJoinType     aJoinType,
                                UInt          * aPredPos )
{
/***********************************************************************
 *
 * Description : Push Selection할 Predicate이 속한 Base Graph의 위치를 찾음
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredicate  * sPredicate;
    qmgGraph     ** sBaseGraph;
    UInt            sPredPos;
    qcDepInfo     * sGraphDependencies;
    idBool          sIsOneTablePred;
    UInt            i;
    qmoPushApplicableType sPushApplicable;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::getPushPredPosition::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aBaseGraph != NULL );

    //------------------------------------------
    // 기본 설정
    //------------------------------------------

    sPredicate      = aPredicate;
    sBaseGraph      = aBaseGraph;
    sPredPos        = ID_UINT_MAX;
    sIsOneTablePred = ID_FALSE;

    //------------------------------------------
    // To Fix BUG-10806
    // Push Selection 가능 Predicate 인지 검사
    //------------------------------------------

    sPushApplicable = isPushApplicable(aPredicate, aJoinType);

    if ( sPushApplicable != QMO_PUSH_APPLICABLE_FALSE )
    {
        for ( i = 0 ; i < 2; i++ )
        {
            sGraphDependencies = & sBaseGraph[i]->depInfo;

            IDE_TEST( qmoPred::isOneTablePredicate( sPredicate,
                                                    aFromDependencies,
                                                    sGraphDependencies,
                                                    & sIsOneTablePred )
                      != IDE_SUCCESS );

            if ( sIsOneTablePred == ID_TRUE )
            {
                if ((sPushApplicable == QMO_PUSH_APPLICABLE_TRUE) ||
                    ((sPushApplicable == QMO_PUSH_APPLICABLE_LEFT) && (i == 0)) ||
                    ((sPushApplicable == QMO_PUSH_APPLICABLE_RIGHT) && (i == 1)))
                {
                    sPredPos = i;
                    break;
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
        // nothing to do
    }

    *aPredPos = sPredPos;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


qmoPushApplicableType qmoCnfMgr::isPushApplicable( qmoPredicate * aPredicate,
                                                   qmsJoinType    aJoinType )
{
/***********************************************************************
 *
 * Description : Push Selection 가능한 predicate인지 검사
 *
 * Implementation :
 *    (1) Function Predicate의 Push Down 가능 여부 검사
 *
 ***********************************************************************/

    qmoPushApplicableType sResult;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aPredicate != NULL );

    /* --------------------------------------------------------------------
        * fix BUG-22512 Outer Join 계열 Push Down Predicate 할 경우, 결과 오류
        * Node에 Push Down 할 수 없는 Function이
        * 존재하는 경우, Predicate을 내리지 않는다.
        * Left/Full Outer Join은 Null Padding때문에
        * Push Selection 하면 틀린 결과가 도출됨
        *
        * BUG-41343
        * EAT_NULL이 있어도 null padding을 하지 않는 relation의 predicate 이라면
        * push selection이 가능하다
        * --------------------------------------------------------------------
        */
    // BUG-44805 서브쿼리 Push Selection 을 잘못하고 있습니다.
    // 서브쿼리도 EAT_NULL 속성을 가지고 있으므로 같이 처리해야 한다.
    if ( ((aPredicate->node->node.lflag & MTC_NODE_EAT_NULL_MASK) == MTC_NODE_EAT_NULL_TRUE) ||
         ((aPredicate->node->lflag & QTC_NODE_SUBQUERY_MASK) == QTC_NODE_SUBQUERY_EXIST) )
    {
        switch (aJoinType)
        {
            case QMS_INNER_JOIN:
                sResult = QMO_PUSH_APPLICABLE_TRUE;
                break;
            case QMS_LEFT_OUTER_JOIN:
                sResult = QMO_PUSH_APPLICABLE_LEFT;
                break;
            case QMS_RIGHT_OUTER_JOIN:
                sResult = QMO_PUSH_APPLICABLE_RIGHT;
                break;
            case QMS_FULL_OUTER_JOIN:
                sResult = QMO_PUSH_APPLICABLE_FALSE;
                break;
            default:
                IDE_DASSERT(0);
                sResult = QMO_PUSH_APPLICABLE_FALSE;
                break;
        }
    }
    else
    {
        sResult = QMO_PUSH_APPLICABLE_TRUE;
    }

    return sResult;
}

IDE_RC
qmoCnfMgr::generateTransitivePredicate( qcStatement * aStatement,
                                        qmoCNF      * aCNF,
                                        idBool        aIsOnCondition )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     where절에 대한 Transitive Predicate 생성
 *
 ***********************************************************************/

    qmsQuerySet * sQuerySet;
    qtcNode     * sNode;
    qtcNode     * sTransitiveNode = NULL;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::generateTransitivePredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sQuerySet = aCNF->myQuerySet;

    //------------------------------------------
    // where절에 대한 Transitive Predicate 생성
    //------------------------------------------

    if ( ( sQuerySet->SFWGH->hints->transitivePredType
           == QMO_TRANSITIVE_PRED_TYPE_ENABLE ) &&
         ( QCU_OPTIMIZER_TRANSITIVITY_DISABLE == 0 ) )
    {
        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_TRANSITIVITY_OLD_RULE );
        if ( aCNF->normalCNF != NULL )
        {
            sNode = (qtcNode*) aCNF->normalCNF->node.arguments;

            IDE_TEST( qmoTransMgr::processTransitivePredicate( aStatement,
                                                               sQuerySet,
                                                               sNode,
                                                               aIsOnCondition,
                                                               & sTransitiveNode )
                      != IDE_SUCCESS );

            if ( sTransitiveNode != NULL )
            {
                // 생성된 transitive predicate를 where절에 연결한다.
                IDE_TEST( qmoTransMgr::linkPredicate( sTransitiveNode,
                                                      & sNode )
                          != IDE_SUCCESS );

                // estimate
                IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                            aCNF->normalCNF )
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
qmoCnfMgr::generateTransitivePred4OnCondition( qcStatement   * aStatement,
                                               qmoCNF        * aCNF,
                                               qmoPredicate  * aUpperPred,
                                               qmoPredicate ** aLowerPred )
{
/***********************************************************************
 *
 * Description : PROJ-1404 Transitive Predicate Generation
 *
 * Implementation :
 *     (1) on절에 대한 Transitive Predicate 생성
 *     (2) upper predicate과 on절 predicate으로 lower predicate 생성
 *
 ***********************************************************************/

    qmsQuerySet  * sQuerySet;
    qtcNode      * sNode;
    qtcNode      * sTransitiveNode = NULL;
    qmoPredicate * sLowerPred = NULL;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::generateTransitivePred4OnCondition::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );
    IDE_DASSERT( aLowerPred != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sQuerySet = aCNF->myQuerySet;

    //------------------------------------------
    // onCondition에 대한 Transitive Predicate 생성
    //------------------------------------------

    // where절과 동일하게 생성
    IDE_TEST( generateTransitivePredicate( aStatement,
                                           aCNF,
                                           ID_TRUE )
              != IDE_SUCCESS );

    //------------------------------------------
    // upper predicate(where절에서 내려온 predicate)과 on절에 대한
    // Transitive Predicate 생성
    //------------------------------------------

    if ( ( sQuerySet->SFWGH->hints->transitivePredType
           == QMO_TRANSITIVE_PRED_TYPE_ENABLE ) &&
         ( QCU_OPTIMIZER_TRANSITIVITY_DISABLE == 0 ) )
    {
        if ( aCNF->normalCNF != NULL )
        {
            sNode = (qtcNode*) aCNF->normalCNF->node.arguments;

            IDE_TEST( qmoTransMgr::processTransitivePredicate( aStatement,
                                                               sQuerySet,
                                                               sNode,
                                                               aUpperPred,
                                                               & sTransitiveNode )
                      != IDE_SUCCESS );

            if ( sTransitiveNode != NULL )
            {
                // 생성된 transitive predicate를 qmoPredicate 형태로 생성한다.
                IDE_TEST( qmoTransMgr::linkPredicate( aStatement,
                                                      sTransitiveNode,
                                                      & sLowerPred )
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
    }
    else
    {
        // Nothing to do.
    }

    *aLowerPred = sLowerPred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCnfMgr::optimizeSubQ4OnCondition( qcStatement * aStatement,
                                     qmoCNF      * aCNF )
{
/***********************************************************************
 *
 * Description :
 *
 * ------------------------------------------
 * fix BUG-19203
 * 각 predicate 별로 subquery에 대해 optimize를 한다.
 * selection graph의 optimize에서 view에 대한 통계 정보가 구축되기 때문에
 * left, right graph의 최적화가 수행된 이후에 하는 것이 맞다.
 * ------------------------------------------
 *
 * ---------------------------------------------
 * on Condition CNF의 각 Predicate의 subuqery 처리
 * on Condition CNF의 각 Predicate은 해당 graph의 myPredicate에 연결되지
 * 않아 해당 graph의 myPredicate처리 시에 subquery가 생성되지 않는다.
 * 따라서 on Condition CNF의 predicate 분류 후에 subquery의 graph를
 * 생성해준다.
 * ( To Fix BUG-8742 )
 * ---------------------------------------------
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::optimizeSubQ4OnCondition::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // Display 정보의 제거
    //------------------------------------------

    if ( aCNF->constantPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               aCNF->constantPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    // To Fix PR-12743
    // NNF Filter 지원
    if ( aCNF->nnfFilter != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueryInNode( aStatement,
                                                   aCNF->nnfFilter,
                                                   ID_FALSE,  //No Range
                                                   ID_FALSE ) //No ConstantFilter
                  != IDE_SUCCESS );

    }
    else
    {
        // nothing to do
    }

    if ( aCNF->oneTablePredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               aCNF->oneTablePredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    if ( aCNF->joinPredicate != NULL )
    {
        IDE_TEST( qmoPred::optimizeSubqueries( aStatement,
                                               aCNF->joinPredicate,
                                               ID_FALSE ) // No KeyRange Tip
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoCnfMgr::discardLateralViewJoinPred( qmoCNF        * aCNF,
                                              qmoPredicate ** aDiscardPredList )
{
/***********************************************************************
 *
 * Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 *  
 *   Lateral View의 Join Predicate을 잠시 제외시켜서,
 *   Join Grouping 과정에 Lateral View가 참여하지 못하도록 한다.
 *
 * Implementation :
 *  
 *   1) Predicate을 순차적으로 탐색하면서, Predicate과 관련된 Base Graph의
 *      lateralDepInfo를 구한다.
 *   2) lateralDepInfo가 존재하면, 해당 Predicate은 Lateral View에 관련된
 *      Predicate 이므로, joinPredicateList에서 빼고 discardPredList에 넣는다.
 *   3) 모든 Predicate에 대해 (1)-(2) 과정을 반복한다.
 *   4) discardPredList를 반환한다.
 *
***********************************************************************/

    qmoPredicate * sCurrPred        = NULL;
    qmoPredicate * sJoinPredHead    = NULL;
    qmoPredicate * sJoinPredTail    = NULL;
    qmoPredicate * sDiscardPredHead = NULL;
    qmoPredicate * sDiscardPredTail = NULL;
    idBool         sInLateralView   = ID_FALSE;
    qcDepInfo      sBaseLateralDepInfo;
    UInt           sBaseGraphCnt;
    qmgGraph    ** sBaseGraph;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::discardLateralViewJoinPred::__FT__" );

    sBaseGraphCnt    = aCNF->graphCnt4BaseTable;
    sBaseGraph       = aCNF->baseGraph;

    // 초기화
    sCurrPred        = aCNF->joinPredicate;
    sJoinPredHead    = aCNF->joinPredicate;

    // Predicate depInfo에 존재하는 Base Graph가
    // lateralDepInfo를 가지고 있는지 검사한다.
    while ( sCurrPred != NULL )
    {
        for ( i = 0; i < sBaseGraphCnt; i++ )
        {
            if ( qtc::dependencyContains( & sCurrPred->node->depInfo,
                                          & sBaseGraph[i]->depInfo ) == ID_TRUE )
            {
                IDE_TEST( qmg::getGraphLateralDepInfo( sBaseGraph[i],
                                                       & sBaseLateralDepInfo )
                          != IDE_SUCCESS );

                if ( qtc::haveDependencies( & sBaseLateralDepInfo ) == ID_TRUE )
                {
                    sInLateralView = ID_TRUE;
                    break;
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

        if ( sInLateralView == ID_TRUE )
        {
            // Predicate이 Lateral View와 관련있다.
            if ( sJoinPredTail == NULL )
            {
                // Lateral View와 관련 없는 Predicate이 아직 나오지 않은 경우
                sJoinPredHead = sCurrPred->next;
            }
            else
            {
                // Lateral View와 관련 없는 Predicate이 이전에 나온 경우
                sJoinPredTail->next = sCurrPred->next;
            }

            if ( sDiscardPredTail == NULL )
            {
                // Lateral View Predicate이 처음 나온 경우
                sDiscardPredHead = sCurrPred;
                sDiscardPredTail = sCurrPred;
            }
            else
            {
                // Lateral View Predicate이 처음이 아닌 경우
                sDiscardPredTail->next = sCurrPred;
                sDiscardPredTail       = sDiscardPredTail->next;
            }
        }
        else
        {
            // Lateral View와 관련없는 Predicate인 경우
            sJoinPredTail = sCurrPred;
        }

        // 초기화
        sInLateralView  = ID_FALSE;
        sCurrPred       = sCurrPred->next;
    }

    // 리스트 반환
    aCNF->joinPredicate = sJoinPredHead;
    *aDiscardPredList   = sDiscardPredHead;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoCnfMgr::validateLateralViewJoin( qcStatement   * aStatement,
                                           qmsFrom       * aFrom )
{
/***********************************************************************
 *
 * Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 *
 *   다음의 경우는 수행할 수 없다. 에러메시지를 출력해야 한다.
 *   - Left-Outer Join의 왼쪽이 LView, 오른쪽이 LView가 참조하는 Relation
 *   - Full-Outer Join의 한 쪽이 LView, 다른 쪽이 LView가 참조하는 Relation
 *
 *   qmv가 아닌 qmo에서 validation을 진행하는 이유는,
 *   qmo에서 Oracle-style Outer Operator를 ANSI-Join으로 변환하기 때문이며
 *   기존 ANSI-Join과 함께 한번에 validation 하기 위해서이다.
 *
 * Implementation :
 *
 *   - Left에  Join Tree가 있다면, Left에  대해서 재귀호출
 *   - Right에 Join Tree가 있다면, Right에 대해서 재귀호출
 *   - Left / Right에서 얻어온 lateralDepInfo를 통해 Right/Full Outer 검증
 *
***********************************************************************/

    qcDepInfo          sLateralDepInfo;
    qcDepInfo          sLeftLateralDepInfo;
    qcDepInfo          sRightLateralDepInfo;

    IDU_FIT_POINT_FATAL( "qmoCnfMgr::validateLateralViewJoin::__FT__" );

    qtc::dependencyClear( & sLeftLateralDepInfo  );
    qtc::dependencyClear( & sRightLateralDepInfo );

    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        IDE_DASSERT( aFrom->left  != NULL );
        IDE_DASSERT( aFrom->right != NULL );

        if ( aFrom->left->joinType != QMS_NO_JOIN )
        {
            // LEFT에 대한 Validation
            IDE_TEST( validateLateralViewJoin( aStatement,
                                               aFrom->left )
                      != IDE_SUCCESS );
        }
        else
        {
            // LEFT의 lateralDepInfo 계산
            IDE_TEST( qmvQTC::getFromLateralDepInfo( aFrom->left,
                                                     & sLeftLateralDepInfo )
                      != IDE_SUCCESS );
        }

        if ( aFrom->right->joinType != QMS_NO_JOIN )
        {
            // RIGHT에 대한 Validation
            IDE_TEST( validateLateralViewJoin( aStatement,
                                               aFrom->right )
                      != IDE_SUCCESS );
        }
        else
        {
            // RIGHT의 lateralDepInfo 계산
            IDE_TEST( qmvQTC::getFromLateralDepInfo( aFrom->right,
                                                     & sRightLateralDepInfo )
                      != IDE_SUCCESS );
        }

        // Left/Full-Outer Join인 경우, Left->Right 참조 여부 검증
        if ( ( aFrom->joinType == QMS_LEFT_OUTER_JOIN ) ||
             ( aFrom->joinType == QMS_FULL_OUTER_JOIN ) )
        {
            qtc::dependencyAnd( & sLeftLateralDepInfo,
                                & aFrom->right->depInfo,
                                & sLateralDepInfo );

            // 외부 참조하는 경우, Left/Full-Outer Join을 할 수 없다.
            IDE_TEST_RAISE( qtc::haveDependencies( & sLateralDepInfo ) == ID_TRUE,
                            ERR_LVIEW_LEFT_FULL_JOIN_WITH_REF );
        }
        else
        {
            // Nothing to do.
        }

        // Full-Outer Join인 경우, Right->Left 참조 여부도 검증
        if ( aFrom->joinType == QMS_FULL_OUTER_JOIN )
        {
            qtc::dependencyAnd( & sRightLateralDepInfo,
                                & aFrom->left->depInfo,
                                & sLateralDepInfo );

            // 외부 참조하는 경우, Full-Outer Join을 할 수 없다.
            IDE_TEST_RAISE( qtc::haveDependencies( & sLateralDepInfo ) == ID_TRUE,
                            ERR_LVIEW_LEFT_FULL_JOIN_WITH_REF );
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

    IDE_EXCEPTION( ERR_LVIEW_LEFT_FULL_JOIN_WITH_REF )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMO_NOT_ALLOWED_LVIEW_LEFT_FULL_JOIN_WITH_REF ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoCnfMgr::classifyPred4WhereHierachyJoin( qcStatement * aStatement,
                                                  qmoCrtPath  * aCrtPath,
                                                  qmgGraph    * aGraph )
{
    qmoCNF         * sCNF               = NULL;
    qtcNode        * sNode              = NULL;
    qmoPredicate   * sNewPred           = NULL;
    qcDepInfo      * sFromDependencies  = NULL;
    qcDepInfo      * sGraphDependencies = NULL;
    qmgGraph      ** sBaseGraph         = NULL;

    idBool           sIsRownum       = ID_FALSE;
    idBool           sIsConstant     = ID_FALSE;
    idBool           sIsOneTable     = ID_FALSE;    // TASK-3876 Code Sonar
    UInt             sBaseGraphCnt   = 0;
    UInt             i               = 0;

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF              = aCrtPath->crtCNF;
    sFromDependencies = &sCNF->depInfo;
    sBaseGraphCnt     = sCNF->graphCnt4BaseTable;
    sBaseGraph        = sCNF->baseGraph;

    // Predicate 분류 대상 Node
    if ( sCNF->normalCNF != NULL )
    {
        sNode = (qtcNode *)sCNF->normalCNF->node.arguments;
    }
    else
    {
        sNode = NULL;
    }

    while ( sNode != NULL )
    {
        // qmoPredicate 생성
        IDE_TEST( qmoPred::createPredicate( QC_QMP_MEM(aStatement),
                                            sNode,
                                            &sNewPred )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Rownum Predicate 검사
        //-------------------------------------------------

        IDE_TEST( qmoPred::isRownumPredicate( sNewPred,
                                              &sIsRownum )
                  != IDE_SUCCESS );

        if ( sIsRownum == ID_TRUE )
        {
            sNewPred->next = aCrtPath->rownumPredicate;
            aCrtPath->rownumPredicate = sNewPred;
        }
        else
        {
            //-------------------------------------------------
            // Constant Predicate 검사
            //-------------------------------------------------

            IDE_TEST( qmoPred::isConstantPredicate( sNewPred,
                                                    sFromDependencies,
                                                    & sIsConstant )
                      != IDE_SUCCESS );

            if ( sIsConstant == ID_TRUE )
            {
                sNewPred->flag &= ~QMO_PRED_CONSTANT_FILTER_MASK;
                sNewPred->flag |= QMO_PRED_CONSTANT_FILTER_TRUE;

                //-------------------------------------------------
                // Constant Predicate 분류
                //   Hierarhcy 존재 유무에 따라 constant predicate,
                //   level predicate, prior predicat을 분류하여 해당 위치에 추가
                //-------------------------------------------------
                /* Level, isLeaf, Predicate의 처리 */
                if ( ( (sNewPred->node->lflag & QTC_NODE_LEVEL_MASK )
                     == QTC_NODE_LEVEL_EXIST) ||
                     ( (sNewPred->node->lflag & QTC_NODE_ISLEAF_MASK )
                     == QTC_NODE_ISLEAF_EXIST) )
                {
                    sNewPred->next = aGraph->myPredicate;
                    aGraph->myPredicate = sNewPred;
                }
                else
                {
                    // Prior Predicate 처리
                    if ( ( sNewPred->node->lflag & QTC_NODE_PRIOR_MASK )
                         == QTC_NODE_PRIOR_EXIST )
                    {
                        sNewPred->next = aGraph->myPredicate;
                        aGraph->myPredicate = sNewPred;
                    }
                    else
                    {
                        // Constant Predicate 처리
                        sNewPred->next = aGraph->constantPredicate;
                        aGraph->constantPredicate = sNewPred;
                    }
                }
            }
            else
            {
                for ( i = 0 ; i < sBaseGraphCnt; i++ )
                {
                    sGraphDependencies = & sBaseGraph[i]->myFrom->depInfo;
                    IDE_TEST( qmoPred::isOneTablePredicate( sNewPred,
                                                            sFromDependencies,
                                                            sGraphDependencies,
                                                            & sIsOneTable )
                              != IDE_SUCCESS );

                    if ( sIsOneTable == ID_TRUE )
                    {
                        break;
                    }
                    else
                    {
                        // nothing to do
                    }
                }

                if ( sIsOneTable == ID_TRUE )
                {
                    //---------------------------------------------
                    // One Table Predicate 분류
                    //    oneTablePredicate을 해당 graph의 myPredicate에 연결
                    //    rid predicate 이면 해당 graph의 ridPredicate에 연결
                    //---------------------------------------------
                    if ( ( sNewPred->node->lflag & QTC_NODE_COLUMN_RID_MASK ) ==
                            QTC_NODE_COLUMN_RID_EXIST )
                    {
                        sNewPred->next = aGraph->ridPredicate;
                        aGraph->ridPredicate = sNewPred;
                    }
                    else
                    {
                        sNewPred->next = aGraph->myPredicate;
                        aGraph->myPredicate = sNewPred;
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        sNode = (qtcNode*)sNode->node.next;
    }

    //---------------------------------------------------------------------
    // BUG-34295 Join ordering ANSI style query
    //     Where 절의 predicate 중 outerJoinGraph 와 연관된
    //     one table predicate 을 찾아 이동시킨다.
    //     outerJoinGraph 의 one table predicate 은 baseGraph 와
    //     dependency 가 겹치지 않아서 predicate 분류 과정에서
    //     constant predicate 으로 잘못 분류된다.
    //     이를 바로잡기 위해 sCNF->constantPredicate 의 predicate 들에서
    //     outerJoinGraph 에 관련된 one table predicate 들을 찾아내어
    //     outerJoinGraph 로 이동시킨다.
    //---------------------------------------------------------------------
    if ( sCNF->outerJoinGraph != NULL )
    {
        IDE_TEST( qmoAnsiJoinOrder::fixOuterJoinGraphPredicate( aStatement,
                                                                sCNF )
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

