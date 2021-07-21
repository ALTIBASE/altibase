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
 * $Id: qmo.cpp 90787 2021-05-07 00:50:48Z ahra.cho $
 *
 * Description :
 *     Query Optimizer
 *
 *     Optimizer를 접근하는 최상위 Interface로 구성됨
 *     Graph 생성 및 Plan Tree를 생성한다.
 *
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qmsParseTree.h>
#include <qdn.h>
#include <qdnForeignKey.h>
#include <qmo.h>
#include <qmoNonCrtPathMgr.h>
#include <qmoSubquery.h>
#include <qmoListTransform.h>
#include <qmoCSETransform.h>
#include <qmoCFSTransform.h>
#include <qmoOBYETransform.h>
#include <qmoDistinctElimination.h>
#include <qmoSelectivity.h>
#include <qmg.h>
#include <qmoPartition.h>    // PROJ-1502 PARTITIONED DISK TABLE
#include <qcmPartition.h>    // PROJ-1502 PARTITIONED DISK TABLE
#include <qmoViewMerging.h>  // PROJ-1413 Simple View Merging
#include <qmoUnnesting.h>    // PROJ-1718 Subquery Unnesting
#include <qmoOuterJoinElimination.h>
#include <qmvQuerySet.h>
#include <qmv.h>
#include <qmoCheckViewColumnRef.h>
#include <qci.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <mtcDef.h>

IDE_RC
qmo::optimizeSelect( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : SELECT 구문의 최적화 과정
 *
 * Implementation :
 *    (1) makeGraph의 수행
 *    (2) Top Projection Graph에 통신 버퍼에 쓸 TOP PROJ를 생성해야 함을 설정
 *    (3) makePlan의 수행
 *
 ***********************************************************************/

    qmsParseTree      * sParseTree;

    IDU_FIT_POINT_FATAL( "qmo::optimizeSelect::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // Parse Tree 획득
    //------------------------------------------

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

    // BUG-20652
    // client로의 전송을 위해 taget의 geometry type을
    // binary type으로 변환
    IDE_TEST( qmvQuerySet::changeTargetForCommunication( aStatement,
                                                         sParseTree->querySet )
              != IDE_SUCCESS );

    //------------------------------------------
    // PROJ-1413
    // Query Transformation 수행
    //------------------------------------------

    IDE_TEST( qmo::doTransform( aStatement ) != IDE_SUCCESS );

    //------------------------------------------
    // PROJ-2469 Optimize View Materialization
    // Useless Column에 대한 flag 처리
    //------------------------------------------

    IDE_TEST( qmoCheckViewColumnRef::checkViewColumnRef( aStatement,
                                                         NULL,
                                                         ID_TRUE )
              != IDE_SUCCESS );

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeGraph( aStatement ) != IDE_SUCCESS );

    // PROJ-2462 Result Cache
    if ( ( (qciStmtType)aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT ) &&
         ( QCU_REDUCE_TEMP_MEMORY_ENABLE == 0 ) &&
         ( ( QC_SHARED_TMPLATE(aStatement)->resultCache.flag & QC_RESULT_CACHE_DISABLE_MASK )
           == QC_RESULT_CACHE_DISABLE_FALSE ) )

    {
        IDE_TEST( setResultCacheFlag( aStatement ) != IDE_SUCCESS );
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->resultCache.flag &= ~QC_RESULT_CACHE_DISABLE_MASK;
        QC_SHARED_TMPLATE(aStatement)->resultCache.flag |= QC_RESULT_CACHE_DISABLE_TRUE;
    }

    // Top Projection Graph 설정
    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_PROJECTION );

    aStatement->myPlan->graph->flag &= ~QMG_PROJ_COMMUNICATION_TOP_PROJ_MASK;
    aStatement->myPlan->graph->flag |= QMG_PROJ_COMMUNICATION_TOP_PROJ_TRUE;

    // BUG-32258
    // direction이 결정되지 않을 경우,
    // preserved order의 direction 불일치로 인해 에러 상황이 발생하므로
    // preserved order의 direction을 명확하게 설정해주도록 한다.
    IDE_TEST( qmg::finalizePreservedOrder( aStatement->myPlan->graph )
              != IDE_SUCCESS );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::optimizeInsert( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : INSERT 구문의 최적화
 *
 * Implementation :
 *    (1) CASE 1 : INSERT...VALUE(...(subquery)...)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) CASE 2 : INSERT...SELECT...
 *        qmoSubquery::optimizeSubquery()의 수행
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmo::optimizeInsert::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeInsertGraph( aStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_INSERT );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( aStatement ),
                                       QC_SHARED_TMPLATE( aStatement ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::optimizeMultiInsertSelect( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : INSERT 구문의 최적화
 *
 * Implementation :
 *    (1) INSERT ALL INTO... INTO... SELECT...
 *        qmoSubquery::optimizeSubquery()의 수행
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmo::optimizeMultiInsertSelect::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    aStatement->mFlag &= ~QC_STMT_NO_PART_COLUMN_REDUCE_MASK;
    aStatement->mFlag |= QC_STMT_NO_PART_COLUMN_REDUCE_TRUE;

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeMultiInsertGraph( aStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_MULTI_INSERT );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( aStatement ),
                                       QC_SHARED_TMPLATE( aStatement ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::optimizeUpdate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : UPDATE 구문의 최적화
 *
 * Implementation :
 *   (1) where 절 CNF Normalization 수행
 *   (2) Graph의 생성
 *   (3) makePlan의 수행
 *   (4) 생성한 Plan을 aStatement의 plan에 연결
 *
 ***********************************************************************/

    qmmUptParseTree * sUptParseTree;

    IDU_FIT_POINT_FATAL( "qmo::optimizeUpdate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // PROJ-1718 Where절에 대하여 subquery predicate을 transform한다.
    //------------------------------------------

    sUptParseTree = (qmmUptParseTree *) aStatement->myPlan->parseTree;

    if ( sUptParseTree->mTableList != NULL )
    {
        IDE_TEST( qmoViewMerging::doTransformForMultiDML( aStatement,
                                                          sUptParseTree->querySet )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( doTransformSubqueries( aStatement,
                                         sUptParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS );
    }
    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeUpdateGraph( aStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_UPDATE );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::optimizeDelete( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : DELETE 구문의 최적화
 *
 * Implementation :
 *   (1) where 절 CNF Normalization 수행
 *   (2) Graph의 생성
 *   (3) makePlan의 수행
 *   (4) 생성한 Plan을 aStatement의 plan에 연결
 *
 ***********************************************************************/

    qmmDelParseTree * sDelParseTree;

    IDU_FIT_POINT_FATAL( "qmo::optimizeDelete::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // PROJ-1718 Where절에 대하여 subquery predicate을 transform한다.
    //------------------------------------------

    sDelParseTree = (qmmDelParseTree *) aStatement->myPlan->parseTree;

    if ( sDelParseTree->mTableList != NULL )
    {
        IDE_TEST( qmoViewMerging::doTransformForMultiDML( aStatement,
                                                          sDelParseTree->querySet )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( doTransformSubqueries( aStatement,
                                         sDelParseTree->querySet->SFWGH->where )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeDeleteGraph( aStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_DELETE );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::optimizeMove( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : MOVE 구문의 최적화
 *
 * Implementation :
 *   (1) source expression에 대해 subquery최적화 및 host변수 등록
 *   (2) limit의 host 변수 등록
 *   (3) where 절 CNF Normalization 수행
 *   (4) Graph의 생성
 *   (5) makePlan의 수행
 *   (6) 생성한 Plan을 aStatement의 plan에 연결
 *
 ***********************************************************************/

    qmmMoveParseTree * sMoveParseTree;

    IDU_FIT_POINT_FATAL( "qmo::optimizeMove::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // PROJ-1718 Where절에 대하여 subquery predicate을 transform한다.
    //------------------------------------------

    sMoveParseTree = (qmmMoveParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST( doTransformSubqueries( aStatement,
                                     sMoveParseTree->querySet->SFWGH->where )
              != IDE_SUCCESS );

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeMoveGraph( aStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_MOVE );

    //-------------------------
    // Plan 생성
    //-------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::optimizeCreateSelect( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : CREATE AS SUBQUERY의 Subquery의 최적화
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmo::optimizeCreateSelect::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // PROJ-1413
    // Query Transformation 수행
    //------------------------------------------
    aStatement->mFlag &= ~QC_STMT_NO_PART_COLUMN_REDUCE_MASK;
    aStatement->mFlag |= QC_STMT_NO_PART_COLUMN_REDUCE_TRUE;

    IDE_TEST( qmo::doTransform( aStatement ) != IDE_SUCCESS );

    //------------------------------------------
    // PROJ-2469 Optimize View Materialization
    // Useless Column에 대한 flag 처리
    //------------------------------------------

    IDE_TEST( qmoCheckViewColumnRef::checkViewColumnRef( aStatement,
                                                         NULL,
                                                         ID_TRUE )
              != IDE_SUCCESS );

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeGraph( aStatement ) != IDE_SUCCESS );

    // 적합성 검사
    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_PROJECTION );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );
    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    // 적합성 검사
    IDE_DASSERT( aStatement->myPlan->plan != NULL );
    IDE_DASSERT( aStatement->myPlan->plan->type == QMN_PROJ );

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Non Critical Path의 생성 및 초기화
 *    (2) Non Critical Path의 최적화
 *
 ***********************************************************************/

    qmoNonCrtPath * sNonCrt;
    qmsParseTree  * sParseTree;

    IDU_FIT_POINT_FATAL( "qmo::makeGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // Non Critical Path의 생성 및 초기화
    //------------------------------------------

    sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST( qmoNonCrtPathMgr::init( aStatement,
                                      sParseTree->querySet,
                                      ID_TRUE, // 최상위 Query Set
                                      & sNonCrt )
              != IDE_SUCCESS );

    //------------------------------------------
    // Non Critical Path의 최적화
    //------------------------------------------

    IDE_TEST( qmoNonCrtPathMgr::optimize( aStatement,
                                          sNonCrt )
              != IDE_SUCCESS );

    //------------------------------------------
    // 최종 Graph 설정
    //------------------------------------------

    aStatement->myPlan->graph = sNonCrt->myGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeInsertGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Critical Path의 생성 및 초기화
 *
 ***********************************************************************/

    qmmInsParseTree * sInsParseTree;
    qcStatement     * sSubQStatement;
    qmgGraph        * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmo::makeInsertGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    sInsParseTree = (qmmInsParseTree *) aStatement->myPlan->parseTree;

    //-------------------------
    // INSERT ... SELECT 구문의 Subquery 최적화
    //-------------------------

    if ( sInsParseTree->select != NULL )
    {
        sSubQStatement = sInsParseTree->select;

        IDE_TEST( qmoSubquery::makeGraph( sSubQStatement ) != IDE_SUCCESS );

        sMyGraph = sSubQStatement->myPlan->graph;
    }
    else
    {
        sMyGraph = NULL;
    }

    //-------------------------
    // insert graph 생성
    //-------------------------

    IDE_TEST( qmgInsert::init( aStatement,
                               sInsParseTree,
                               sMyGraph,
                               &sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgInsert::optimize( aStatement,
                                   sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeMultiInsertGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Critical Path의 생성 및 초기화
 *
 ***********************************************************************/

    qmmInsParseTree * sInsParseTree;
    qcStatement     * sSubQStatement;
    qmgGraph        * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmo::makeMultiInsertGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    sInsParseTree = (qmmInsParseTree *) aStatement->myPlan->parseTree;

    IDE_DASSERT( sInsParseTree->select != NULL );

    //-------------------------
    // INSERT ... SELECT 구문의 Subquery 최적화
    //-------------------------

    sSubQStatement = sInsParseTree->select;

    IDE_TEST( qmoSubquery::makeGraph( sSubQStatement ) != IDE_SUCCESS );

    sMyGraph = sSubQStatement->myPlan->graph;

    //-------------------------
    // insert graph 생성
    //-------------------------

    IDE_TEST( qmgMultiInsert::init( aStatement,
                                    sMyGraph,
                                    &sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgMultiInsert::optimize( aStatement,
                                        sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeUpdateGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Critical Path의 생성 및 초기화
 *
 ***********************************************************************/

    qmmUptParseTree * sUptParseTree;
    qmoCrtPath      * sCrtPath;
    qmgGraph        * sMyGraph;
    qmmMultiTables  * sTmp;

    IDU_FIT_POINT_FATAL( "qmo::makeUpdateGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    sUptParseTree = (qmmUptParseTree *) aStatement->myPlan->parseTree;

    //-------------------------
    // critical-path Graph 생성
    //-------------------------

    IDE_TEST( qmoCrtPathMgr::init( aStatement,
                                   sUptParseTree->querySet,
                                   & sCrtPath )
              != IDE_SUCCESS );

    if ( sUptParseTree->mTableList != NULL )
    {
        for ( sTmp = sUptParseTree->mTableList;
              sTmp != NULL;
              sTmp = sTmp->mNext )
        {
            if ( ( ( sTmp->mTableRef->tableFlag & SMI_TABLE_TYPE_MASK )
                   == SMI_TABLE_DISK ) ||
                 ( sTmp->mViewID >= 0 ) ||
                 ( sTmp->mTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) )
            {
                sCrtPath->mIsOnlyNL = ID_TRUE;
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

    IDE_TEST( qmoCrtPathMgr::optimize( aStatement,
                                       sCrtPath )
              != IDE_SUCCESS );

    sMyGraph = sCrtPath->myGraph;

    //-------------------------
    // update graph 생성
    //-------------------------

    IDE_TEST( qmgUpdate::init( aStatement,
                               sUptParseTree->querySet,
                               sMyGraph,
                               &sMyGraph )
              != IDE_SUCCESS );

    if ( sUptParseTree->mTableList == NULL )
    {
        IDE_TEST( qmgUpdate::optimize( aStatement,
                                       sMyGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qmgUpdate::optimizeMultiUpdate( aStatement,
                                                  sMyGraph )
                  != IDE_SUCCESS );
    }

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeDeleteGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Critical Path의 생성 및 초기화
 *
 ***********************************************************************/

    qmmDelParseTree    * sDelParseTree;
    qmoCrtPath         * sCrtPath;
    qmgGraph           * sMyGraph;
    qmmDelMultiTables  * sTmp;

    IDU_FIT_POINT_FATAL( "qmo::makeDeleteGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    sDelParseTree = (qmmDelParseTree *) aStatement->myPlan->parseTree;

    //-------------------------
    // critical-path Graph 생성
    //-------------------------

    IDE_TEST( qmoCrtPathMgr::init( aStatement,
                                   sDelParseTree->querySet,
                                   & sCrtPath )
              != IDE_SUCCESS );

    if ( sDelParseTree->mTableList != NULL )
    {
        for ( sTmp = sDelParseTree->mTableList;
              sTmp != NULL;
              sTmp = sTmp->mNext )
        {
            if ( ( ( sTmp->mTableRef->tableFlag & SMI_TABLE_TYPE_MASK )
                   == SMI_TABLE_DISK ) ||
                 ( sTmp->mViewID >= 0 ) ||
                 ( sTmp->mTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) )
            {
                sCrtPath->mIsOnlyNL = ID_TRUE;
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

    IDE_TEST( qmoCrtPathMgr::optimize( aStatement,
                                       sCrtPath )
              != IDE_SUCCESS );

    sMyGraph = sCrtPath->myGraph;

    //-------------------------
    // delete graph 생성
    //-------------------------

    IDE_TEST( qmgDelete::init( aStatement,
                               sDelParseTree->querySet,
                               sMyGraph,
                               & sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgDelete::optimize( aStatement,
                                   sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeMoveGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Critical Path의 생성 및 초기화
 *
 ***********************************************************************/

    qmmMoveParseTree * sMoveParseTree;
    qmoCrtPath       * sCrtPath;
    qmgGraph         * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmo::makeMoveGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    sMoveParseTree = (qmmMoveParseTree *) aStatement->myPlan->parseTree;

    //-------------------------
    // critical-path Graph 생성
    //-------------------------

    IDE_TEST( qmoCrtPathMgr::init( aStatement,
                                   sMoveParseTree->querySet,
                                   & sCrtPath )
              != IDE_SUCCESS );

    IDE_TEST( qmoCrtPathMgr::optimize( aStatement,
                                       sCrtPath )
              != IDE_SUCCESS );

    sMyGraph = sCrtPath->myGraph;

    //-------------------------
    // delete graph 생성
    //-------------------------

    IDE_TEST( qmgMove::init( aStatement,
                             sMoveParseTree->querySet,
                             sMyGraph,
                             & sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgMove::optimize( aStatement,
                                 sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::makeMergeGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) merge의 생성 및 초기화
 *
 ***********************************************************************/

    qmgGraph        * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmo::makeMoveGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //-------------------------
    // mrege graph 생성
    //-------------------------

    IDE_TEST( qmgMerge::init( aStatement,
                              & sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgMerge::optimize( aStatement,
                                  sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::makeShardInsertGraph( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Graph의 생성 및 최적화
 *
 * Implementation :
 *    (1) Critical Path의 생성 및 초기화
 *
 ***********************************************************************/

    qmmInsParseTree * sInsParseTree;
    qcStatement     * sSubQStatement;
    qmgGraph        * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmo::makeShardInsertGraph::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );

    sInsParseTree = (qmmInsParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST_RAISE( sInsParseTree->returnInto != NULL,
                    ERR_UNSUPPORTED_FUNCTION );

    //-------------------------
    // INSERT ... SELECT 구문의 Subquery 최적화
    //-------------------------

    if ( sInsParseTree->select != NULL )
    {
        sSubQStatement = sInsParseTree->select;

        IDE_TEST( qmoSubquery::makeGraph( sSubQStatement ) != IDE_SUCCESS );

        sMyGraph = sSubQStatement->myPlan->graph;
    }
    else
    {
        sMyGraph = NULL;
    }

    //-------------------------
    // insert graph 생성
    //-------------------------

    IDE_TEST( qmgShardInsert::init( aStatement,
                                    sInsParseTree,
                                    sMyGraph,
                                    &sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgShardInsert::optimize( aStatement,
                                        sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSUPPORTED_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_UNSUPPORTED, "RETURNING INTO clause" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::currentJoinDependencies( qmgGraph  * aJoinGraph,
                              qcDepInfo * aDependencies,
                              idBool    * aIsCurrent )
{
/***********************************************************************
 *
 * Description : 현재 Join과 관련된 Dependencies인지 검사
 *
 * Implementation :
 *      Hint 또는 ( 분류되기 전의 ) Join Predicate이 현재 Join Graph에
 *      속하면서 하위 한쪽에만 속하지 않는 경우인지 검사한다.
 *      하위 한쪽에만 속하는 경우, 이미 적용된 것이기 때문이다.
 *
 *      ex ) USE_NL( T1, T2 ) USE_NL( T1, T3 )
 *
 *                 Join <---- USE_NL( T1, T2 ) Hint : skip 이미 적용
 *                 /  \       USE_NL( T1, T3 ) Hint : 적용
 *                /    \
 *              Join   T3.I1
 *              /  \
 *          T1.I1   T2.I1
 *
 *
 ***********************************************************************/

    idBool sIsCurrent;

    IDU_FIT_POINT_FATAL( "qmo::currentJoinDependencies::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinGraph != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    if ( qtc::dependencyEqual( &aJoinGraph->depInfo,
                               aDependencies ) == ID_TRUE )
    {
        // Hint == Join Graph
        // 힌트를 적용한다.
        sIsCurrent = ID_TRUE;
    }
    else if ( qtc::dependencyContains( &aJoinGraph->depInfo,
                                       aDependencies ) == ID_TRUE )
    {
        // Hint < Join Graph
        // 사용되지 않는 힌트만 적용한다.
        if ( (qtc::dependencyContains( &aJoinGraph->left->depInfo, aDependencies ) == ID_TRUE) ||
             (qtc::dependencyContains( &aJoinGraph->right->depInfo, aDependencies ) == ID_TRUE) )
        {
            sIsCurrent = ID_FALSE;
        }
        else
        {
            sIsCurrent = ID_TRUE;
        }
    }
    else if ( qtc::dependencyContains( aDependencies,
                                       &aJoinGraph->depInfo ) == ID_TRUE )
    {
        // BUG-43528 use_XXX 힌트에서 여러개의 테이블을 지원해야 합니다.
        // Hint > Join Graph
        // 힌트를 적용한다.
        sIsCurrent = ID_TRUE;
    }
    else
    {
        sIsCurrent = ID_FALSE;
    }

    *aIsCurrent = sIsCurrent;

    return IDE_SUCCESS;
}

IDE_RC qmo::currentJoinDependencies4JoinOrder( qmgGraph  * aJoinGraph,
                                               qcDepInfo * aDependencies,
                                               idBool    * aIsCurrent )
{
/***********************************************************************
 *
 * Description : 현재 Join과 관련된 Dependencies인지 검사
 *
 ***********************************************************************/

    idBool sIsCurrent;

    IDU_FIT_POINT_FATAL( "qmo::currentJoinDependencies4JoinOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinGraph != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    if ( qtc::dependencyContains( &aJoinGraph->depInfo,
                                  aDependencies ) == ID_TRUE )
    {
        //------------------------------------------
        // 현재 qmgJoin과 관련된 joinPredicate인 경우
        // 이미 적용한 Join Predicate 인지 검사한다.
        // ( left 또는 right 한쪽에만 속하는 Join Predicate은 이미 적용된
        //   Join Predicate 이다. )
        //------------------------------------------

        if ( qtc::dependencyContains( & aJoinGraph->left->depInfo,
                                      aDependencies ) == ID_TRUE )
        {
            sIsCurrent = ID_FALSE;
        }
        else
        {
            if ( qtc::dependencyContains( & aJoinGraph->right->depInfo,
                                          aDependencies ) == ID_TRUE )
            {
                sIsCurrent = ID_FALSE;
            }
            else
            {
                sIsCurrent = ID_TRUE;
            }
        }
    }
    else
    {
        sIsCurrent = ID_FALSE;
    }

    *aIsCurrent = sIsCurrent;

    return IDE_SUCCESS;
}

IDE_RC qmo::optimizeForHost( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : host 변수에 값이 바인딩된 후 scan method를 재구축한다.
 *
 * Implementation : prepare 영역의 값을 절대 바꾸면 안된다.
 *
 ***********************************************************************/

    qmoScanDecisionFactor * sSDF;
    UChar                 * sData;
    qmoAccessMethod       * sSelectedMethod;
    UInt                    i;
    const void            * sTableHandle = NULL;
    smSCN                   sTableSCN;
    idBool                  sIsFixedTable;

    IDU_FIT_POINT_FATAL( "qmo::optimizeForHost::__FT__" );

    sData = QC_PRIVATE_TMPLATE(aStatement)->tmplate.data;

    sSDF = aStatement->myPlan->scanDecisionFactors;

    while( sSDF != NULL )
    {
        // qmoOneNonPlan에 의해 candidateCount가 세팅되어야 한다.
        IDE_DASSERT( sSDF->candidateCount != 0 );

        // table에 lock을 걸기 위해 basePlan을 통해
        // tableHandle과 scn을 얻어온다.
        IDE_TEST( qmn::getReferencedTableInfo( sSDF->basePlan,
                                               & sTableHandle,
                                               & sTableSCN,
                                               & sIsFixedTable )
                  != IDE_SUCCESS );

        if ( sIsFixedTable == ID_FALSE )
        {
            // Table에 IS Lock을 건다.
            IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT(aStatement))->getTrans(),
                                                 sTableHandle,
                                                 sTableSCN,
                                                 SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                 SMI_TABLE_LOCK_IS,
                                                 ID_ULONG_MAX,
                                                 ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( qmoSelectivity::recalculateSelectivity( QC_PRIVATE_TMPLATE( aStatement ),
                                                          sSDF->baseGraph->graph.myFrom->tableRef->statInfo,
                                                          & sSDF->baseGraph->graph.depInfo,
                                                          sSDF->predicate )
                  != IDE_SUCCESS );

        // PROJ-1502 PARTITIONED DISK TABLE
        if( ( sSDF->baseGraph->graph.flag & QMG_SELT_PARTITION_MASK ) ==
            QMG_SELT_PARTITION_TRUE )
        {
            IDE_TEST( qmgSelection::getBestAccessMethodInExecutionTime(
                          aStatement,
                          & sSDF->baseGraph->graph,
                          sSDF->baseGraph->partitionRef->statInfo,
                          sSDF->predicate,
                          (qmoAccessMethod*)( sData + sSDF->accessMethodsOffset ),
                          & sSelectedMethod )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmgSelection::getBestAccessMethodInExecutionTime(
                          aStatement,
                          & sSDF->baseGraph->graph,
                          sSDF->baseGraph->graph.myFrom->tableRef->statInfo,
                          sSDF->predicate,
                          (qmoAccessMethod*)( sData + sSDF->accessMethodsOffset ),
                          & sSelectedMethod )
                      != IDE_SUCCESS );
        }

        for( i = 0; i < sSDF->candidateCount; i++ )
        {
            if( qmoStat::getMethodIndex( sSelectedMethod )
                == sSDF->candidate[i].index )
            {
                break;
            }
            else
            {
                // Nothing to do...
            }
        }

        // selected access method의 위치를 저장한다.
        if( i == sSDF->candidateCount )
        {
            // i == sSDF->candidateCount 이면
            // candidate 중에 선택된 index와 같은 후보가 없다는 의미
            // 이땐 scan plan에 설정되어 있는, 즉 prepare 시점에
            // 정해진 index를 사용하면 된다.
            // 이 선택은 qmnScan::getScanMethod()에 있다.
            *(UInt*)( sData + sSDF->selectedMethodOffset ) =
                QMO_SCAN_METHOD_UNSELECTED;
        }
        else
        {
            *(UInt*)( sData + sSDF->selectedMethodOffset ) = i;
        }

        IDE_TEST( qmn::notifyOfSelectedMethodSet( QC_PRIVATE_TMPLATE(aStatement),
                                                  sSDF->basePlan )
                  != IDE_SUCCESS );

        sSDF = sSDF->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

qmncScanMethod *
qmo::getSelectedMethod( qcTemplate            * aTemplate,
                        qmoScanDecisionFactor * aSDF,
                        qmncScanMethod        * aDefaultMethod )
{
    UInt sSelectedMethod;

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aSDF != NULL );

    sSelectedMethod = *(UInt*)( aTemplate->tmplate.data +
                                aSDF->selectedMethodOffset );

    if( sSelectedMethod == QMO_SCAN_METHOD_UNSELECTED )
    {
        return aDefaultMethod;
    }
    else if( sSelectedMethod < aSDF->candidateCount )
    {
        return aSDF->candidate + sSelectedMethod;
    }
    else
    {
        IDE_DASSERT( 0 );
    }
    
    return aDefaultMethod;
}

IDE_RC
qmo::doTransform( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Parse tree를 이용하여 transform을 수행한다.
 *
 *    SELECT 구문에 query transformation을 수행한다.
 *
 * Implementation :
 *
 *     1. Common subexpression elimination 수행
 *     2. Constant filter subsumption 수행
 *     3. Simple view merging 수행
 *     4. Subquery unnesting 수행
 *      a. 만약 unnesting된 subquery가 있다면 simple view mering을 재수행
 *
 ***********************************************************************/

    idBool          sChanged;
    qmsParseTree  * sParseTree;

    IDU_FIT_POINT_FATAL( "qmo::doTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // Parse Tree 획득
    //------------------------------------------

    sParseTree = (qmsParseTree *) aStatement->myPlan->parseTree;

    //------------------------------------------
    // BUG-36438 LIST transformation
    //------------------------------------------

    IDE_TEST( qmoListTransform::doTransform( aStatement, sParseTree->querySet )
              != IDE_SUCCESS );

    //------------------------------------------
    // PROJ-2242 : CSE transformation 수행
    //------------------------------------------

    IDE_TEST( qmoCSETransform::doTransform4NNF( aStatement,
                                                sParseTree->querySet,
                                                ID_TRUE )  // NNF
              != IDE_SUCCESS );

    //------------------------------------------
    // PROJ-2242 : CFS transformation 수행
    //------------------------------------------

    IDE_TEST( qmoCFSTransform::doTransform4NNF( aStatement,
                                                sParseTree->querySet )
              != IDE_SUCCESS );

    //------------------------------------------
    // BUG-41183 ORDER BY elimination of inline view
    //------------------------------------------

    IDE_TEST( qmoOBYETransform::doTransform( aStatement,
                                             sParseTree )
              != IDE_SUCCESS );

    //------------------------------------------
    // BUG-41249 DISTINCT elimination
    //------------------------------------------

    IDE_TEST( qmoDistinctElimination::doTransform( aStatement, sParseTree->querySet )
              != IDE_SUCCESS );

    //------------------------------------------
    // Simple View Merging 수행
    //------------------------------------------

    IDE_TEST( qmoViewMerging::doTransform( aStatement,
                                           sParseTree->querySet )
              != IDE_SUCCESS );

    //------------------------------------------
    // Subquery unnesting 수행
    //------------------------------------------

    IDE_TEST( qmoUnnesting::doTransform( aStatement,
                                         & sChanged )
              != IDE_SUCCESS );

    if( sChanged == ID_TRUE )
    {
        // Subqury unnesting 시 query가 변경된 경우 view merging을 한번 더 수행한다.
        IDE_TEST( qmoViewMerging::doTransform( aStatement,
                                               sParseTree->querySet )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // BUG-38339 Outer Join Elimination 수행
    //------------------------------------------

    IDE_TEST( qmoOuterJoinElimination::doTransform( aStatement,
                                                    sParseTree->querySet,
                                                    & sChanged )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmo::doTransformSubqueries( qcStatement * aStatement,
                            qtcNode     * aPredicate )
{
/***********************************************************************
 *
 * Description :
 *     SELECT를 제외한 구문들이 포함하는 predicate들에 대하여
 *     transformation을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool sChanged = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmo::doTransformSubqueries::__FT__" );

    //------------------------------------------
    // Simple View Merging 수행
    //------------------------------------------

    IDE_TEST( qmoViewMerging::doTransformSubqueries( aStatement,
                                                     aPredicate )
              != IDE_SUCCESS );

    //------------------------------------------
    // Subquery unnesting 수행
    //------------------------------------------

    IDE_TEST( qmoUnnesting::doTransformSubqueries( aStatement,
                                                   aPredicate,
                                                   &sChanged )
              != IDE_SUCCESS );

    if( sChanged == ID_TRUE )
    {
        // Subqury unnesting 시 query가 변경된 경우 view merging을 한번 더 수행한다.
        IDE_TEST( qmoViewMerging::doTransformSubqueries( aStatement,
                                                         aPredicate )
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

/* PROJ-1988 Implement MERGE statement */
IDE_RC qmo::optimizeMerge( qcStatement * aStatement )
{
    qmmMergeParseTree * sMergeParseTree;

    qmmMultiRows    * sMultiRows    = NULL;
    qmgMRGE         * sMyGraph      = NULL;
    qmnPlan         * sMyPlan       = NULL;

    IDU_FIT_POINT_FATAL( "qmo::optimizeMerge::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // PROJ-1718 Where절에 대하여 subquery predicate을 transform한다.
    //------------------------------------------

    sMergeParseTree = (qmmMergeParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST( doTransform( sMergeParseTree->selectSourceStatement )
              != IDE_SUCCESS );

    IDE_TEST( doTransform( sMergeParseTree->selectTargetStatement )
              != IDE_SUCCESS );

    if( sMergeParseTree->updateStatement != NULL )
    {
        IDE_TEST( doTransformSubqueries( sMergeParseTree->updateStatement,
                                         sMergeParseTree->whereForUpdate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( sMergeParseTree->deleteStatement != NULL )
    {
        IDE_TEST( doTransformSubqueries( sMergeParseTree->deleteStatement,
                                         sMergeParseTree->whereForDelete )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( sMergeParseTree->insertStatement != NULL )
    {
        IDE_TEST( doTransformSubqueries( sMergeParseTree->insertStatement,
                                         sMergeParseTree->whereForInsert )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeMergeGraph( aStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aStatement->myPlan->graph != NULL );
    IDE_DASSERT( aStatement->myPlan->graph->type == QMG_MERGE );

    //-------------------------
    // Plan 생성
    //-------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    /* BUG-44228  merge 실행시 disk table이고 hash join 인 경우 의도하지 않는 값으로 변경 됩니다.
     *  1. Plan를 검색한다.
     *  2. Update ParseTree의 Value의 Node Info를 변경한다.
     *  3. Insert ParseTree의 Value의 Node Info를 변경한다.
     */

    /* 1. Plan를 검색한다. */
    sMyGraph = (qmgMRGE*) aStatement->myPlan->graph;
    sMyPlan  = sMyGraph->graph.children[QMO_MERGE_SELECT_SOURCE_IDX].childGraph->myPlan;

    /* 2. Update ParseTree의 Value의 Node Info를 변경한다. */
    if( sMergeParseTree->updateStatement != NULL )
    {
        IDE_TEST( qmo::adjustValueNodeForMerge( aStatement,
                                                sMyPlan->resultDesc,
                                                sMergeParseTree->updateParseTree->values )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 3. Insert ParseTree의 Value의 Node Info를 변경한다. */
    if( sMergeParseTree->insertStatement != NULL )
    {
        for ( sMultiRows = sMergeParseTree->insertParseTree->rows;
              sMultiRows != NULL;
              sMultiRows = sMultiRows->next )
        {
            IDE_TEST( qmo::adjustValueNodeForMerge( aStatement,
                                                    sMyPlan->resultDesc,
                                                    sMultiRows->values )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::optimizeShardDML( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Shard DML 구문의 최적화
 *
 * Implementation :
 *
 ***********************************************************************/

    qmgGraph        * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmo::optimizeShardDML::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmgShardDML::init( aStatement,
                                 &(aStatement->myPlan->parseTree->stmtPos),
                                 aStatement->myPlan->mShardAnalysis,
                                 aStatement->myPlan->mShardParamInfo, /* TASK-7219 Non-shardDML*/
                                 aStatement->myPlan->mShardParamCount,
                                 &sMyGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgShardDML::optimize( aStatement,
                                     sMyGraph )
              != IDE_SUCCESS );

    aStatement->myPlan->graph = sMyGraph;

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::optimizeShardInsert( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Shard DML 구문의 최적화
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmo::optimizeShardInsert::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );

    //------------------------------------------
    // Graph 생성
    //------------------------------------------

    IDE_TEST( qmo::makeShardInsertGraph( aStatement ) != IDE_SUCCESS );

    IDE_FT_ASSERT( aStatement->myPlan->graph != NULL );
    IDE_FT_ASSERT( aStatement->myPlan->graph->type == QMG_SHARD_INSERT );

    //------------------------------------------
    // Plan Tree 생성
    //------------------------------------------

    IDE_TEST( aStatement->myPlan->graph->makePlan( aStatement,
                                                   NULL,
                                                   aStatement->myPlan->graph )
              != IDE_SUCCESS );

    aStatement->myPlan->plan = aStatement->myPlan->graph->myPlan;

    //------------------------------------------
    // Optimization 마무리
    //------------------------------------------

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                       QC_SHARED_TMPLATE(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::setResultCacheFlag( qcStatement * aStatement )
{
    qmsParseTree * sParseTree = NULL;
    qmgGraph     * sMyGraph   = NULL;
    UInt           sMode      = 0;
    UInt           sFlag      = 0;
    idBool         sIsTopResultCache = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmo::setResultCacheFlag::__FT__" );

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;

    sMode = QCG_GET_SESSION_TOP_RESULT_CACHE_MODE( aStatement );

    if ( sMode > 0 )
    {
        sFlag = QC_SHARED_TMPLATE(aStatement)->smiStatementFlag & SMI_STATEMENT_CURSOR_MASK;

        switch ( sMode )
        {
            case 1: /* TOP_RESULT_CACHE_MODE ONLY MEMORY TABLE */
                if ( sFlag  == SMI_STATEMENT_MEMORY_CURSOR )
                {
                    sIsTopResultCache = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 2: /* TOP_RESULT_CACHE_MODE INCLUDE DISK TABLE */
                if ( ( sFlag == SMI_STATEMENT_DISK_CURSOR ) ||
                     ( sFlag == SMI_STATEMENT_ALL_CURSOR ) )
                {
                    sIsTopResultCache = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
                break;
            case 3: /* TOP_RESULT_CACHE_MODE ALL */
                sIsTopResultCache = ID_TRUE;
                break;
            default:
                break;
        };
    }
    else
    {
        /* Nothing to do */
    }

    // top_result_cache Hint 로 설정될 수 있다.
    if ( ( QC_SHARED_TMPLATE(aStatement)->resultCache.flag & QC_RESULT_CACHE_TOP_MASK )
         == QC_RESULT_CACHE_TOP_TRUE )
    {
        sIsTopResultCache = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsTopResultCache == ID_TRUE )
    {
        checkQuerySet( sParseTree->querySet, &sIsTopResultCache );

        if ( sIsTopResultCache == ID_TRUE )
        {
            sMyGraph = aStatement->myPlan->graph;
            // Top Result Cache True 설정
            sMyGraph->flag &= ~QMG_PROJ_TOP_RESULT_CACHE_MASK;
            sMyGraph->flag |= QMG_PROJ_TOP_RESULT_CACHE_TRUE;
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

    if ( QCG_GET_SESSION_RESULT_CACHE_ENABLE(aStatement) == 1 )
    {
        QC_SHARED_TMPLATE(aStatement)->resultCache.flag &= ~QC_RESULT_CACHE_MASK;
        QC_SHARED_TMPLATE(aStatement)->resultCache.flag |= QC_RESULT_CACHE_ENABLE;
    }
    else
    {
        /* Nothing to do */
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_TOP_RESULT_CACHE_MODE );
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_RESULT_CACHE_ENABLE );

    return IDE_SUCCESS;
}

/**
 * PROJ-2462 ResultCache
 *
 * Optimize 과정에서 Cache로 사용가능한 Plan에 대해
 * Result Cache Stack을 구성한다.
 *
 * 이 Stack에는 ComponentInfo와 PlanID로 구정되는 데 이 정보로
 * Result Cache된 TempTable이 어떤 Table에 의존성이 있는지를
 * 알 수 있게 된다.
 */
IDE_RC qmo::initResultCacheStack( qcStatement   * aStatement,
                                  qmsQuerySet   * aQuerySet,
                                  UInt            aPlanID,
                                  idBool          aIsTopResult,
                                  idBool          aIsVMTR )
{
    qcTemplate    * sTemplate = NULL;
    iduVarMemList * sMemory   = NULL;

    IDU_FIT_POINT_FATAL( "qmo::initResultCacheStack::__FT__" );

    sMemory   = QC_QMP_MEM( aStatement );
    sTemplate = QC_SHARED_TMPLATE( aStatement );

    // QuerySet에 Reuslt Cache가 사용되지 못하는 쿼리가 올 경우와
    // 전체 쿼리가 Result Cache를 사용하지 못하는 경우에 는 Stack을 초기화한다.
    if ( ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_INVALID_MASK )
             == QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE ) ||
         ( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_DISABLE_MASK )
           == QC_RESULT_CACHE_DISABLE_TRUE ) )
    {
        sTemplate->resultCache.stack = NULL;
        IDE_CONT( normal_exit );
    }
    else
    {
        /* Nothing to do */
    }

    /* Top Result Cache의 경우 항상 ComponentInfo를 생성한다. */
    if ( aIsTopResult == ID_TRUE )
    {
        IDE_TEST( pushComponentInfo( sTemplate,
                                     sMemory,
                                     aPlanID,
                                     aIsVMTR )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2462 ResultCache
         * result_cache_enable Property에 따라 Component Info를 생성
         * 할지 말지 결정한다.
         */
        if ( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_MASK )
             == QC_RESULT_CACHE_ENABLE )
        {
            /* NO_RESULT_CACHE Hint 가 없다면 ComponentInfo를 생성한다. */
            if ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_MASK )
                 != QMV_QUERYSET_RESULT_CACHE_NO )
            {
                IDE_TEST( pushComponentInfo( sTemplate,
                                             sMemory,
                                             aPlanID,
                                             aIsVMTR )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* RESULT_CACHE Hint가 있다면 ComponentInfo를 생성한다 */
            if ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_MASK )
                 == QMV_QUERYSET_RESULT_CACHE )
            {
                IDE_TEST( pushComponentInfo( sTemplate,
                                             sMemory,
                                             aPlanID,
                                             aIsVMTR )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-2462 Result Cache
 *
 * Plan이 make 되는 과정에서 호출되는 함수로
 * 만약 이 Plan이 Cache가 가능하다면 Stack에서 꺼내서
 * List로 옮겨 둔다. 그렇지 못할 경우 그냥 Stack에서 꺼내버린다.
 */
void qmo::makeResultCacheStack( qcStatement      * aStatement,
                                qmsQuerySet      * aQuerySet,
                                UInt               aPlanID,
                                UInt               aPlanFlag,
                                ULong              aTupleFlag,
                                qmcMtrNode       * aMtrNode,
                                qcComponentInfo ** aInfo,
                                idBool             aIsTopResult )
{
    qcTemplate  * sTemplate   = NULL;
    qmcMtrNode  * sMtrNode    = NULL;
    idBool        sIsPossible = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmo::makeResultCacheStack::__NOFT__" );

    *aInfo = NULL;
    sTemplate = QC_SHARED_TMPLATE( aStatement );

    IDE_TEST_CONT( sTemplate->resultCache.stack == NULL, normal_exit );

    /**
     * Outer Dependency 가 있는 경우, Parallel Grag, Disk Temp인경우
     * 와 MtrNode가 없는 경우 ResultCache를 사용하지 않는다.
     */
    if ( ( ( aPlanFlag & QMN_PLAN_OUTER_REF_MASK )
           == QMN_PLAN_OUTER_REF_FALSE ) &&
         ( ( aPlanFlag & QMN_PLAN_GARG_PARALLEL_MASK )
           == QMN_PLAN_GARG_PARALLEL_FALSE ) &&
         ( ( aTupleFlag & MTC_TUPLE_STORAGE_MASK )
           == MTC_TUPLE_STORAGE_MEMORY ) &&
         ( aMtrNode != NULL ) )
    {
        sIsPossible = ID_TRUE;

        /* PROJ-2462 ResultCache
         * Node가 Lob이 존재하거나 Disk Table이거나 Disk Partitioned
         * 이면 Cache를 사용할 수 없다.
         */
        for ( sMtrNode = aMtrNode; sMtrNode != NULL; sMtrNode = sMtrNode->next )
        {
            if ( ( ( sMtrNode->flag & QMC_MTR_LOB_EXIST_MASK )
                   == QMC_MTR_LOB_EXIST_TRUE ) ||
                 ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK )
                   == QMC_MTR_TYPE_DISK_PARTITIONED_TABLE ) ||
                 ( ( sMtrNode->flag & QMC_MTR_TYPE_MASK )
                   == QMC_MTR_TYPE_DISK_TABLE ) ||
                 ( ( sMtrNode->flag & QMC_MTR_PRIOR_EXIST_MASK )
                   == QMC_MTR_PRIOR_EXIST_TRUE ) )
            {
                // 이와같이 Cache를 사용하지 못한다면 Stack Dep flag를 설정하고
                // 다음 ViewMtr 이전의 상위 Plan 역시 Cache 하지 않도록 한다.
                // 왜냐하면 같은 QuerySet내에서는 모두 Cache를 사용하던가
                // 아니면 모두 사용하지 않아야하기 때문이다.
                sTemplate->resultCache.flag &= ~QC_RESULT_CACHE_STACK_DEP_MASK;
                sTemplate->resultCache.flag |= QC_RESULT_CACHE_STACK_DEP_TRUE;
                sIsPossible = ID_FALSE;
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
        sIsPossible = ID_FALSE;

        if ( ( ( aPlanFlag & QMN_PLAN_OUTER_REF_MASK )
               == QMN_PLAN_OUTER_REF_TRUE ) ||
             ( ( aPlanFlag & QMN_PLAN_GARG_PARALLEL_MASK )
               == QMN_PLAN_GARG_PARALLEL_TRUE ) )
        {
            // 이와같이 Cache를 사용하지 못한다면 Stack Dep flag를 설정하고
            // 다음 ViewMtr 이전의 상위 Plan 역시 Cache 하지 않도록 한다.
            // 왜냐하면 같은 QuerySet내에서는 모두 Cache를 사용하던가
            // 아니면 모두 사용하지 않아야하기 때문이다.
            sTemplate->resultCache.flag &= ~QC_RESULT_CACHE_STACK_DEP_MASK;
            sTemplate->resultCache.flag |= QC_RESULT_CACHE_STACK_DEP_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( aIsTopResult == ID_TRUE )
    {
        IDE_DASSERT( aPlanID == sTemplate->resultCache.stack->planID );

        popComponentInfo( sTemplate, sIsPossible, aInfo );
    }
    else
    {
        /* PROJ-2462 ResultCache
         * result_cache_enable Property에 따라 Component Info를 Pop
         * 할지 말지 결정한다.
         */
        if ( ( sTemplate->resultCache.flag & QC_RESULT_CACHE_MASK )
             == QC_RESULT_CACHE_ENABLE )
        {
            /* NO_RESULT_CACHE hint가 없을 경우*/
            if ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_MASK )
                 != QMV_QUERYSET_RESULT_CACHE_NO )
            {
                IDE_DASSERT( aPlanID == sTemplate->resultCache.stack->planID );

                popComponentInfo( sTemplate, sIsPossible, aInfo );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* RESULT_CACHE Hint 가 있을 경우 */
            if ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_MASK )
                 == QMV_QUERYSET_RESULT_CACHE )
            {
                IDE_DASSERT( aPlanID == sTemplate->resultCache.stack->planID );

                popComponentInfo( sTemplate, sIsPossible, aInfo );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );
}

/**
 * PROJ-2462 Reuslt Cache
 * Result Cache Stack 을 초기화 한다.
 */
void qmo::flushResultCacheStack( qcStatement * aStatement )
{
    QC_SHARED_TMPLATE( aStatement )->resultCache.stack = NULL;
}

/**
 * PROJ-2462 Reuslt Cache
 * Result Cache 생성을 위한 ComponentInfo를 생성하고
 * Stack 에 달아 둔다. 이 ComponentInfo를 이용해 각
 * Cache할 Temp Table 이 어떤 Table에 의존적인지를
 * 알 수 있게된다.
 */
IDE_RC qmo::pushComponentInfo( qcTemplate    * aTemplate,
                               iduVarMemList * aMemory,
                               UInt            aPlanID,
                               idBool          aIsVMTR )
{
    qcComponentInfo * sInfo  = NULL;
    qcComponentInfo * sStack = NULL;

    IDU_FIT_POINT_FATAL( "qmo::pushComponentInfo::__FT__" );

    IDU_FIT_POINT( "qmo::pushComponentInfo::alloc::sInfo",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( aMemory->alloc( ID_SIZEOF( qcComponentInfo ),
                              (void **)&sInfo )
              != IDE_SUCCESS );

    sInfo->planID = aPlanID;
    sInfo->isVMTR = aIsVMTR;
    sInfo->count  = 0;
    sInfo->next   = NULL;

    IDU_FIT_POINT( "qmo::pushComponentInfo::alloc::sInfo->components",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( aMemory->alloc( ID_SIZEOF( UShort ) * aTemplate->tmplate.rowArrayCount,
                              (void **)&sInfo->components )
              != IDE_SUCCESS );

    sStack = aTemplate->resultCache.stack;
    aTemplate->resultCache.stack = sInfo;
    sInfo->next = sStack;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * PROJ-2462 Reuslt Cache
 * Result Cache 생성을 위한 ComponentInfo를 Stack에서 List로
 * 옮긴다. 사용하지 못하는 ComponentInfo는 list로 옮겨지지 않는다.
 */
void qmo::popComponentInfo( qcTemplate       * aTemplate,
                            idBool             aIsPossible,
                            qcComponentInfo ** aInfo )
{
    qcComponentInfo * sInfo   = NULL;
    qcComponentInfo * sTemp   = NULL;
    idBool            sIsTrue = ID_FALSE;

    sInfo = aTemplate->resultCache.stack;

    aTemplate->resultCache.stack = sInfo->next;
    sInfo->next                  = NULL;
    sIsTrue = aIsPossible;

    /**
     * Lob 이 사용된 경우와 같이 ResultCache를 사용하지 못한 경우에
     * 같은 QuerySet 내의 다른 TempTable도 Cache할수 없다.
     * 따라서 View 가 오는 새로운 QuerySet이 될때 까지 list에
     * 달지 않는다.
     */
    if ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_STACK_DEP_MASK )
         == QC_RESULT_CACHE_STACK_DEP_TRUE )
    {
        if ( sInfo->isVMTR == ID_TRUE )
        {
            aTemplate->resultCache.flag &= ~QC_RESULT_CACHE_STACK_DEP_MASK;
            aTemplate->resultCache.flag |= QC_RESULT_CACHE_STACK_DEP_FALSE;
        }
        else
        {
            sIsTrue = ID_FALSE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsTrue == ID_TRUE )
    {
        sTemp = aTemplate->resultCache.list;
        aTemplate->resultCache.list = sInfo;
        sInfo->next = sTemp;

        aTemplate->resultCache.count++;
        *aInfo = sInfo;
    }
    else
    {
        *aInfo = NULL;
    }
}

/**
 * PROJ-2462 ResultCache
 * MakeScan등에서 호출되는 함수로 관련 Table TupleID를
 * Stack에 있는 모든 ComponentInfo에 등록해준다.
 *
 */
void qmo::addTupleID2ResultCacheStack( qcStatement * aStatement,
                                       UShort        aTupleID )
{
    UInt              i;
    idBool            sIsFound = ID_FALSE;
    qcComponentInfo * sInfo    = NULL;

    for ( sInfo = QC_SHARED_TMPLATE( aStatement )->resultCache.stack;
          sInfo != NULL;
          sInfo = sInfo->next )
    {
        sIsFound = ID_FALSE;
        for ( i = 0; i < sInfo->count; i++ )
        {
            if ( sInfo->components[i] == aTupleID )
            {
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sIsFound == ID_FALSE )
        {
            sInfo->components[sInfo->count] = aTupleID;
            sInfo->count++;
        }
        else
        {
            /* Nothing to do */
        }
    }
}

void qmo::checkFromTree( qmsFrom * aFrom, idBool * aIsPossible )
{
    qmsTableRef   * sTableRef   = NULL;
    qmsParseTree  * sParseTree  = NULL;
    idBool          sIsPossible = ID_TRUE;

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        sTableRef = aFrom->tableRef;

        if ( sTableRef->view != NULL )
        {
            sParseTree = (qmsParseTree *)(sTableRef->view->myPlan->parseTree);
            checkQuerySet( sParseTree->querySet, &sIsPossible );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        checkFromTree( aFrom->left, &sIsPossible );

        if ( sIsPossible == ID_TRUE )
        {
            checkFromTree( aFrom->right, &sIsPossible );
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aIsPossible = sIsPossible;
}

void qmo::checkQuerySet( qmsQuerySet * aQuerySet, idBool * aIsPossible )
{
    qmsFrom * sFrom       = NULL;
    idBool    sIsPossible = ID_TRUE;

    if ( ( aQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_INVALID_MASK )
         == QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE )
    {
        sIsPossible = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sIsPossible == ID_TRUE )
    {
        if ( aQuerySet->setOp == QMS_NONE )
        {
            for ( sFrom = aQuerySet->SFWGH->from ; sFrom != NULL; sFrom = sFrom->next )
            {
                checkFromTree( sFrom, &sIsPossible );
                if ( sIsPossible == ID_FALSE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else // SET OPERATORS
        {
            // Recursive Call
            checkQuerySet( aQuerySet->left, &sIsPossible );

            if ( sIsPossible == ID_TRUE )
            {
                checkQuerySet( aQuerySet->right, &sIsPossible );
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

    *aIsPossible = sIsPossible;
}

IDE_RC qmo::adjustValueNodeForMerge( qcStatement  * aStatement,
                                     qmcAttrDesc  * aResultDesc,
                                     qmmValueNode * aValueNode )
{
/***********************************************************************
 *
 * Description :
 *   BUG-44228  merge 실행시 disk table이고 hash join 인 경우 의도하지 않는 값으로 변경 됩니다.
 *    1. Mtc Node를 순회한다.
 *    2. Src Argument Node는 Recursive 호출로 처리한다.
 *    3. Dst Argument Node는 Recursive 호출로 처리한다.
 *    4. Colum인 Node만 변경한다.
 *    5. Dst Node의 위치정보를 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcAttrDesc  * sResultDesc = NULL;
    qmmValueNode * sValueNode  = NULL;
    mtcNode      * sSrcNode    = NULL;
    mtcNode      * sDstNode    = NULL;

    sSrcNode = &( aResultDesc->expr->node );
    sDstNode = &( aValueNode->value->node );

    for ( sResultDesc  = aResultDesc;
          sResultDesc != NULL;
          sResultDesc  = sResultDesc->next )
    {
        /* 1. Mtc Node를 순회한다. */
        for ( sSrcNode  = &( sResultDesc->expr->node );
              sSrcNode != NULL;
              sSrcNode  = sSrcNode->next )
        {
            /* 2. Src Argument Node는 Recursive 호출로 처리한다. */
            if ( sSrcNode->arguments != NULL )
            {
                IDE_TEST( qmo::adjustArgumentNodeForMerge( aStatement,
                                                           sSrcNode->arguments,
                                                           sDstNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            for ( sValueNode  = aValueNode;
                  sValueNode != NULL;
                  sValueNode  = sValueNode->next )
            {
                /* 3. Dst Argument Node는 Recursive 호출로 처리한다. */
                for ( sDstNode  = &( sValueNode->value->node );
                      sDstNode != NULL;
                      sDstNode  = sDstNode->next )
                {
                    if ( sDstNode->arguments != NULL )
                    {
                        IDE_TEST( qmo::adjustArgumentNodeForMerge( aStatement,
                                                                   sSrcNode,
                                                                   sDstNode->arguments )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    /* 4. Colum인 Node만 변경한다. */
                    if ( ( sSrcNode->module != &( qtc::columnModule ) ) ||
                         ( sDstNode->module != &( qtc::columnModule ) ) )
                    {
                        continue;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    /* 5. Dst Node의 위치정보를 변경한다. */
                    if ( ( sDstNode->baseTable  == sSrcNode->baseTable ) &&
                         ( sDstNode->baseColumn == sSrcNode->baseColumn ) )
                    {
                        sDstNode->table  = sSrcNode->table;
                        sDstNode->column = sSrcNode->column;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::adjustArgumentNodeForMerge( qcStatement  * aStatement,
                                        mtcNode      * aSrcNode,
                                        mtcNode      * aDstNode )
{
/***********************************************************************
 *
 * Description :
 *   BUG-44228  merge 실행시 disk table이고 hash join 인 경우 의도하지 않는 값으로 변경 됩니다.
 *    1. Mtc Node를 순회한다.
 *    2. Src Argument Node는 Recursive 호출로 처리한다.
 *    3. Dst Argument Node는 Recursive 호출로 처리한다.
 *    4. Colum인 Node만 변경한다.
 *    5. Dst Node의 위치정보를 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcNode      * sSrcNode    = NULL;
    mtcNode      * sDstNode    = NULL;

    sSrcNode = aSrcNode;
    sDstNode = aDstNode;

    /* 1. Mtc Node를 순회한다. */
    for ( sSrcNode  = aSrcNode;
          sSrcNode != NULL;
          sSrcNode  = sSrcNode->next )
    {
        /* 2. Src Argument Node는 Recursive 호출로 처리한다. */
        if ( sSrcNode->arguments != NULL )
        {
            IDE_TEST( qmo::adjustArgumentNodeForMerge( aStatement,
                                                       sSrcNode->arguments,
                                                       sDstNode )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        for ( sDstNode  = aDstNode;
              sDstNode != NULL;
              sDstNode  = sDstNode->next )
        {
            /* 3. Dst Argument Node는 Recursive 호출로 처리한다. */
            if ( sDstNode->arguments != NULL )
            {
                IDE_TEST( qmo::adjustArgumentNodeForMerge( aStatement,
                                                           sSrcNode,
                                                           sDstNode->arguments )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            /* 4. Colum인 Node만 변경한다. */
            if ( ( sSrcNode->module != &( qtc::columnModule ) ) ||
                 ( sDstNode->module != &( qtc::columnModule ) ) )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            /* 5. Dst Node의 위치정보를 변경한다. */
            if ( ( sDstNode->baseTable  == sSrcNode->baseTable ) &&
                 ( sDstNode->baseColumn == sSrcNode->baseColumn ) )
            {
                sDstNode->table  = sSrcNode->table;
                sDstNode->column = sSrcNode->column;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::optimizeMultiUpdate( qcStatement * aStatement )
{
    IDE_TEST( optimizeUpdate( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmo::optimizeMultiDelete( qcStatement * aStatement )
{
    IDE_TEST( optimizeDelete( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* TASK-7219 Non-shard DML */
IDE_RC qmo::removeOutRefPredPushedForce( qmoPredicate ** aPredicate )
{
    idBool         sIsPushedForce   = ID_FALSE;
    qmoPredicate * sPredicate       = NULL;
    qmoPredicate * sRemainPredicate = NULL;
    qmoPredicate * sLast;

    for ( sPredicate = *aPredicate;
          sPredicate != NULL;
          sPredicate = sPredicate->next )
    {
        // 아직 relocate 전이다.
        IDE_DASSERT( sPredicate->more == NULL );

        sIsPushedForce = ID_FALSE;

        qmgSelection::isForcePushedPredForShardView( &sPredicate->node->node,
                                                     &sIsPushedForce );
        if ( sIsPushedForce == ID_TRUE )
        {
            /* Nothing to do. */
            // 연결관계 제거
        }
        else
        {
            // 연결관계 유지
            if ( sRemainPredicate == NULL )
            {
                sRemainPredicate = sPredicate;
                sLast = sRemainPredicate;
            }
            else
            {
                sLast->next = sPredicate;
                sLast = sLast->next;
            }
        }
    }

    if ( sRemainPredicate == NULL )
    {
        /* Nothing to do. */
    }
    else
    {
        sLast->next = NULL;
    }

    *aPredicate = sRemainPredicate;

    return IDE_SUCCESS;
}
