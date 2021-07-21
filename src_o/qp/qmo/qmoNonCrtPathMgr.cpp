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
 * $Id: qmoNonCrtPathMgr.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * Description :
 *     Non Critical Path Manager
 *
 *     Critical Path 이외의 부분에 대한 최적화 및 그래프를 생성한다.
 *     즉, 다음과 같은 부분에 대한 그래프 최적화를 수행한다.
 *         - Projection Graph
 *         - Set Graph
 *         - Sorting Graph
 *         - Distinction Graph
 *         - Grouping Graph
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qmoNonCrtPathMgr.h>
#include <qmoConstExpr.h>
#include <qmgProjection.h>
#include <qmgSet.h>
#include <qmgSorting.h>
#include <qmgDistinction.h>
#include <qmgGrouping.h>
#include <qmgWindowing.h>
#include <qmgSetRecursive.h>
#include <qmv.h>

IDE_RC
qmoNonCrtPathMgr::init( qcStatement    * aStatement,
                        qmsQuerySet    * aQuerySet,
                        idBool           aIsTop,
                        qmoNonCrtPath ** aNonCrtPath )
{
/***********************************************************************
 *
 * Description : qmoNonCrtPath 생성 및 초기화
 *
 * Implementation :
 *    (1) qmoNonCrtPath 만큼 메모리 할당
 *    (2) qmoNonCrtPath 초기화
 *    (2) crtPath 생성 및 초기화
 *    (3) left, right 생성 및 초기화 함수 호출
 *
 ***********************************************************************/

    qmsParseTree  * sParseTree;
    qmsQuerySet   * sQuerySet;
    qmoNonCrtPath * sNonCrtPath;
    qmoCrtPath    * sCrtPath;

    IDU_FIT_POINT_FATAL( "qmoNonCrtPathMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sParseTree  = (qmsParseTree *)aStatement->myPlan->parseTree;
    sQuerySet   = aQuerySet;
    sNonCrtPath = NULL;
    sCrtPath    = NULL;

    //------------------------------------------
    // qmoNonCrtPath 초기화
    //------------------------------------------

    // qmoNonCrtPath 메모리 할당 받음
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoNonCrtPath),
                                             (void**) & sNonCrtPath )
              != IDE_SUCCESS);

    sNonCrtPath->flag = QMO_NONCRT_PATH_INITIALIZE;

    // flag 정보 설정
    if ( aIsTop == ID_TRUE )
    {
        sNonCrtPath->flag &= ~QMO_NONCRT_PATH_TOP_MASK;
        sNonCrtPath->flag |= QMO_NONCRT_PATH_TOP_TRUE;
        sQuerySet->nonCrtPath = sNonCrtPath;
    }
    else
    {
        sNonCrtPath->flag &= ~QMO_NONCRT_PATH_TOP_MASK;
        sNonCrtPath->flag |= QMO_NONCRT_PATH_TOP_FALSE;
    }

    sNonCrtPath->myGraph = NULL;
    sNonCrtPath->myQuerySet = sQuerySet;

    //------------------------------------------
    // crtPath 생성 및 초기화 : Leaf Non Critical Path 인 경우에만
    //------------------------------------------

    if ( sQuerySet->setOp == QMS_NONE )
    {
        //------------------------------------------
        // PROJ-1413 constant exprssion의 상수 변환
        //------------------------------------------

        IDE_TEST( qmoConstExpr::processConstExpr( aStatement,
                                                  sQuerySet->SFWGH )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Leaf Non Critical Path : crtPath 생성 및 초기화
        //------------------------------------------

        IDE_TEST( qmoCrtPathMgr::init( aStatement, sQuerySet, &sCrtPath )
                  != IDE_SUCCESS );
        sNonCrtPath->crtPath = sCrtPath;
    }
    else
    {
        //------------------------------------------
        // Non-Leaf Non Critical Path : left, right 생성 및 초기화 함수 호출
        //------------------------------------------

        IDE_TEST( init( aStatement, sQuerySet->left, ID_FALSE,
                        &(sNonCrtPath->left) )
                  != IDE_SUCCESS );

        IDE_TEST( init( aStatement, sQuerySet->right, ID_FALSE,
                        &(sNonCrtPath->right) )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // PROJ-1413 constant exprssion의 상수 변환
    //------------------------------------------

    IDE_TEST( qmoConstExpr::processConstExprForOrderBy( aStatement,
                                                        sParseTree )
              != IDE_SUCCESS );

    // out 설정
    *aNonCrtPath = sNonCrtPath;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoNonCrtPathMgr::optimize( qcStatement    * aStatement,
                            qmoNonCrtPath  * aNonCrtPath )
{
/***********************************************************************
 *
 * Description : Non Critical Path의 최적화( 즉, qmoNonCrtPath의 최적화)
 *
 * Implementation :
 *    (1) Leaf Non Critical Path의 최적화 ( myQuerySet->setOp == QMS_NONE )
 *        a. Critical Path의 최적화 수행
 *        b. Grouping의 처리
 *        c. Nested Grouping의 처리
 *        d. Distinction의 처리
 *        e. 결과 graph를 qmoNonCrtPath::myGraph에 연결
 *    (2) Non Leaf Non Critical Path의 최적화 ( myQuerySet->setOp != QMS_NONE )
 *        a. left, right 최적화 함수 호출
 *        b. qmgSet 생성 및 초기화
 *        c. qmgSet 최적화
 *        d. 최적화한 qmgSet을 qmoNonCrtPath::myGraph에 연결
 *    (4) Top Non Critical Path의 처리
 *        a. Order By의 처리
 *        b. Projection 처리
 *        c. 결과 graph를 qmoNonCrtPath::myGraph에 연결
 *
 ***********************************************************************/

    qmoNonCrtPath * sNonCrtPath;
    qmsParseTree  * sParseTree;
    qmsQuerySet   * sQuerySet;
    qmgGraph      * sMyGraph;

    IDU_FIT_POINT_FATAL( "qmoNonCrtPathMgr::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNonCrtPath != NULL );

    //------------------------------------------
    // Leaf인 경우, Non-Leat 경우의 처리
    //------------------------------------------

    sNonCrtPath = aNonCrtPath;
    sParseTree  = (qmsParseTree *)aStatement->myPlan->parseTree;
    sQuerySet   = sNonCrtPath->myQuerySet;

    if ( sQuerySet->setOp == QMS_NONE )
    {
        //------------------------------------------
        // Leaf Non Critical Path인 경우
        //------------------------------------------

        IDE_TEST( optimizeLeaf( aStatement, sNonCrtPath ) != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------------
        // Non-Leaf Non Critical Path인 경우
        //------------------------------------------
        IDE_TEST( optimizeNonLeaf( aStatement, sNonCrtPath ) != IDE_SUCCESS );
    }
    sMyGraph = sNonCrtPath->myGraph;

    //------------------------------------------
    // Top 여부에 따른 처리
    //------------------------------------------

    if ( ( sNonCrtPath->flag & QMO_NONCRT_PATH_TOP_MASK )
         == QMO_NONCRT_PATH_TOP_TRUE )
    {
        //------------------------------------------
        // ORDER BY 처리
        //------------------------------------------

        if ( sParseTree->orderBy != NULL )
        {
            IDE_TEST( qmgSorting::init( aStatement,
                                        sQuerySet,
                                        sMyGraph,           // child
                                        &sMyGraph )         // result graph
                      != IDE_SUCCESS );

            IDE_TEST( qmgSorting::optimize( aStatement, sMyGraph )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do
        }

        //------------------------------------------
        // Projection 처리
        //------------------------------------------

        IDE_TEST( qmgProjection::init( aStatement,
                                       sQuerySet,
                                       sMyGraph,        // child
                                       &sMyGraph )      // result graph
                  != IDE_SUCCESS );

        IDE_TEST( qmgProjection::optimize( aStatement,  sMyGraph )
                  != IDE_SUCCESS );

        sNonCrtPath->myGraph = sMyGraph;
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
qmoNonCrtPathMgr::optimizeLeaf( qcStatement   * aStatement,
                                qmoNonCrtPath * aNonCrtPath )
{
/***********************************************************************
 *
 * Description : Leaf Non Critical Path의 최적화
 *
 * Implementation :
 *    (1) Critical Path 의 최적화 수행
 *    (2) Grouping의 수행
 *    (3) Nested Grouping의 수행
 *    (4) Distinction의 수행
 *    (5) 결과 graph를 qmoNonCrtPath::myGraph에 연결
 *
 ***********************************************************************/

    qmoNonCrtPath * sNonCrtPath;
    qmsQuerySet   * sQuerySet;
    qmgGraph      * sMyGraph;
    // dummy node
    qmsAggNode    * sAggr;
    qmsAggNode    * sPrev;

    IDU_FIT_POINT_FATAL( "qmoNonCrtPathMgr::optimizeLeaf::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNonCrtPath != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNonCrtPath = aNonCrtPath;
    sQuerySet   = sNonCrtPath->myQuerySet;

    //------------------------------------------
    // Critical Path 최적화
    //------------------------------------------

    IDE_TEST( qmoCrtPathMgr::optimize( aStatement,sNonCrtPath->crtPath )
              != IDE_SUCCESS );

    sMyGraph = sNonCrtPath->crtPath->myGraph;

    //------------------------------------------
    // Grouping의 처리
    //------------------------------------------

    // dummyNode
    if( (sQuerySet->SFWGH->group != NULL) &&
        (sQuerySet->SFWGH->aggsDepth1 != NULL) &&
        (sQuerySet->SFWGH->aggsDepth2 != NULL) )
    {
        // aggr1 -> aggr2 ( for grouping column, aggr1 )

        sPrev = NULL;
        sAggr = sQuerySet->SFWGH->aggsDepth1;
        while( sAggr != NULL )
        {
            if( sAggr->aggr->indexArgument == 1 )
            {
                sPrev = sAggr;
                sAggr = sAggr->next;
                continue;
            }

            // BUG-37496
            // nested aggregation 을 사용한 쿼리의 결과가 올바르지 않습니다.
            if( sPrev == NULL )
            {
                sQuerySet->SFWGH->aggsDepth1 = sAggr->next;
            }
            else
            {
                sPrev->next = sAggr->next;
            }

            sAggr->next = sQuerySet->SFWGH->aggsDepth2;
            sQuerySet->SFWGH->aggsDepth2 = sAggr;

            if( sPrev == NULL )
            {
                sAggr = sQuerySet->SFWGH->aggsDepth1;
            }
            else
            {
                sAggr = sPrev->next;
            }
        }
    }

    if ( (sQuerySet->SFWGH->group != NULL) ||
         (sQuerySet->SFWGH->aggsDepth1 != NULL) )
    {
        IDE_TEST( qmgGrouping::init( aStatement,
                                     sQuerySet,
                                     sMyGraph,      // child
                                     ID_FALSE,      // is nested
                                     &sMyGraph )    // result graph
                  != IDE_SUCCESS );

        IDE_TEST ( qmgGrouping::optimize( aStatement, sMyGraph )
                   != IDE_SUCCESS);
    }
    else
    {
        // Nohting To Do
    }

    //------------------------------------------
    // Nested Grouping의 처리
    //------------------------------------------

    if ( sQuerySet->SFWGH->aggsDepth2 != NULL )
    {
        IDE_TEST( qmgGrouping::init( aStatement,
                                     sQuerySet,
                                     sMyGraph,         // child
                                     ID_TRUE,          // is nested
                                     &sMyGraph )       // result graph
                  != IDE_SUCCESS );

        IDE_TEST( qmgGrouping::optimize( aStatement, sMyGraph )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nohting To Do
    }

    //------------------------------------------
    // Windowing의 처리
    //------------------------------------------

    if ( sQuerySet->analyticFuncList != NULL )
    {
        IDE_TEST( qmgWindowing::init( aStatement,
                                      sQuerySet,
                                      sMyGraph,
                                      & sMyGraph )
                  != IDE_SUCCESS );

        IDE_TEST( qmgWindowing::optimize( aStatement, sMyGraph )
                  != IDE_SUCCESS);
    }
    else
    {
        // nothing to do
    }

    //------------------------------------------
    // Distinction의 처리
    //------------------------------------------

    if ( sQuerySet->SFWGH->selectType == QMS_DISTINCT )
    {
        IDE_TEST( qmgDistinction::init( aStatement,
                                        sQuerySet,
                                        sMyGraph,         // child
                                        &sMyGraph )       // result graph
                  != IDE_SUCCESS );

        IDE_TEST( qmgDistinction::optimize( aStatement, sMyGraph )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nohting To Do
    }

    // 결과 Graph 연결
    sNonCrtPath->myGraph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoNonCrtPathMgr::optimizeNonLeaf( qcStatement   * aStatement,
                                   qmoNonCrtPath * aNonCrtPath )
{
/***********************************************************************
 *
 * Description : Non-Leaf Non Critical Path의 최적화
 *
 * Implementation :
 *    (1) left, right 최적화 함수 호출
 *    (2) qmgSet 초기화
 *    (3) qmgSet 최적화
 *    (4) 결과 graph를 qmoNonCrtPath::myGraph에 연결
 *
 ***********************************************************************/

    qmoNonCrtPath * sNonCrtPath = NULL;
    qmsQuerySet   * sQuerySet = NULL;

    IDU_FIT_POINT_FATAL( "qmoNonCrtPathMgr::optimizeNonLeaf::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNonCrtPath != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNonCrtPath = aNonCrtPath;
    sQuerySet   = sNonCrtPath->myQuerySet;

    // PROJ-2582 recursive with
    if ( ( sQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
         == QMV_QUERYSET_RECURSIVE_VIEW_TOP )
    {
        IDE_TEST( optimizeSetRecursive( aStatement,
                                        aNonCrtPath )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( optimizeSet( aStatement,
                               aNonCrtPath )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoNonCrtPathMgr::optimizeSet( qcStatement   * aStatement,
                                      qmoNonCrtPath * aNonCrtPath )
{
/***********************************************************************
 *
 * Description : Non-Leaf Non Critical Path의 최적화
 *
 * Implementation :
 *    (1) left, right 최적화 함수 호출
 *    (2) qmgSet 초기화
 *    (3) qmgSet 최적화
 *    (4) 결과 graph를 qmoNonCrtPath::myGraph에 연결
 *
 ***********************************************************************/

    qmoNonCrtPath * sNonCrtPath;
    qmsQuerySet   * sQuerySet;
    qmgGraph      * sMyGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmoNonCrtPathMgr::optimizeSet::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNonCrtPath != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNonCrtPath = aNonCrtPath;
    sQuerySet   = sNonCrtPath->myQuerySet;

    //------------------------------------------
    // left, right 최적화 함수 호출
    //------------------------------------------

    IDE_TEST( optimize( aStatement, sNonCrtPath->left ) != IDE_SUCCESS );
    IDE_TEST( optimize( aStatement, sNonCrtPath->right ) != IDE_SUCCESS );

    // To Fix PR-9567
    // Non Leaf Query Set의 경우
    // Child Query Set은 다음과 같이 다양한 형태로 구성될 수 있다.
    // ----------------------------------------------------
    // Left   :  Leaf   | Leaf     | Non-Leaf | Non-Leaf
    // Right  :  Leaf   | Non-Leaf | Leaf     | Non-Leaf
    // ----------------------------------------------------
    // 위와 같은 다양한 처리를 위하여
    // Left Query Set과 Right Query Set을 별도로 처리하여야 한다.

    if ( sNonCrtPath->left->myQuerySet->setOp == QMS_NONE )
    {
        //------------------------------------------
        // left projection graph의 생성
        //------------------------------------------

        IDE_TEST( qmgProjection::init( aStatement,
                                       aNonCrtPath->left->myQuerySet,
                                       aNonCrtPath->left->myGraph,
                                       & aNonCrtPath->left->myGraph )
                  != IDE_SUCCESS );

        IDE_TEST( qmgProjection::optimize( aStatement,
                                           aNonCrtPath->left->myGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        // To Fix PR-9567
        // 하위 Query Set이 SET을 의미할 경우
        // Parent가 SET임을 명시하여야 함.
        aNonCrtPath->left->myGraph->flag &= ~QMG_SET_PARENT_TYPE_SET_MASK;
        aNonCrtPath->left->myGraph->flag |= QMG_SET_PARENT_TYPE_SET_TRUE;
    }

    if ( sNonCrtPath->right->myQuerySet->setOp == QMS_NONE )
    {
        //------------------------------------------
        // right projection graph의 생성
        //------------------------------------------

        IDE_TEST( qmgProjection::init( aStatement,
                                       aNonCrtPath->right->myQuerySet,
                                       aNonCrtPath->right->myGraph,
                                       & aNonCrtPath->right->myGraph )
                  != IDE_SUCCESS );

        IDE_TEST( qmgProjection::optimize( aStatement,
                                           aNonCrtPath->right->myGraph )
                  != IDE_SUCCESS );
    }
    else
    {
        // To Fix PR-9567
        // 하위 Query Set이 SET을 의미할 경우
        // Parent가 SET임을 명시하여야 함.
        aNonCrtPath->right->myGraph->flag &= ~QMG_SET_PARENT_TYPE_SET_MASK;
        aNonCrtPath->right->myGraph->flag |= QMG_SET_PARENT_TYPE_SET_TRUE;
    }

    // qmgSet 초기화
    IDE_TEST( qmgSet::init( aStatement,
                            sQuerySet,
                            sNonCrtPath->left->myGraph, // left child
                            sNonCrtPath->right->myGraph,// right child
                            &sMyGraph )                 // result graph
              != IDE_SUCCESS );

    // qmgSet 최적화
    IDE_TEST( qmgSet::optimize( aStatement, sMyGraph ) != IDE_SUCCESS );

    // 결과 Graph 연결
    sNonCrtPath->myGraph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoNonCrtPathMgr::optimizeSetRecursive( qcStatement   * aStatement,
                                               qmoNonCrtPath * aNonCrtPath )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *    Non-Leaf Non Critical Path ( recrusvie union all)의 최적화
 *
 * Implementation :
 *    (1) left, right 최적화 함수 호출
 *    (2) qmgSetRecursive 초기화
 *    (3) qmgSetRecursive 최적화
 *    (4) 결과 graph를 qmoNonCrtPath::myGraph에 연결
 *
 ***********************************************************************/

    qmoNonCrtPath * sNonCrtPath = NULL;
    qmsQuerySet   * sQuerySet = NULL;
    qmgGraph      * sMyGraph = NULL;

    IDU_FIT_POINT_FATAL( "qmoNonCrtPathMgr::optimizeSetRecursive::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNonCrtPath != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNonCrtPath = aNonCrtPath;
    sQuerySet   = sNonCrtPath->myQuerySet;

    //------------------------------------------
    // left 최적화 함수 호출
    //------------------------------------------

    IDE_TEST( optimize( aStatement, sNonCrtPath->left )
              != IDE_SUCCESS );

    //------------------------------------------
    // left projection graph의 생성
    //------------------------------------------

    // Recursive Query Set의 경우
    // Child Query Set은 Leaf만 가능하다.
    IDE_TEST_RAISE( ( sNonCrtPath->left->myQuerySet->setOp != QMS_NONE ) ||
                    ( sNonCrtPath->right->myQuerySet->setOp != QMS_NONE ),
                    ERR_INVALID_CHILD );

    IDE_TEST( qmgProjection::init( aStatement,
                                   aNonCrtPath->left->myQuerySet,
                                   aNonCrtPath->left->myGraph,
                                   & aNonCrtPath->left->myGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgProjection::optimize( aStatement,
                                       aNonCrtPath->left->myGraph )
              != IDE_SUCCESS );

    //------------------------------------------
    // right 최적화 함수 호출
    //------------------------------------------

    // 임시로 recursive view의 left를 기록하고
    // right의 graph를 생성할 때 사용한다.
    aStatement->myPlan->graph = aNonCrtPath->left->myGraph;

    IDE_TEST( optimize( aStatement, sNonCrtPath->right )
              != IDE_SUCCESS );

    //------------------------------------------
    // right projection graph의 생성
    //------------------------------------------

    IDE_TEST( qmgProjection::init( aStatement,
                                   aNonCrtPath->right->myQuerySet,
                                   aNonCrtPath->right->myGraph,
                                   & aNonCrtPath->right->myGraph )
              != IDE_SUCCESS );

    IDE_TEST( qmgProjection::optimize( aStatement,
                                       aNonCrtPath->right->myGraph )
              != IDE_SUCCESS );

    //------------------------------------------
    // set recursive graph의 생성
    //------------------------------------------

    // right query의 하위 recursive view graph가 달려있어야 한다.
    IDE_DASSERT( aStatement->myPlan->graph != NULL );

    // qmgSet 초기화
    IDE_TEST( qmgSetRecursive::init( aStatement,
                                     sQuerySet,
                                     sNonCrtPath->left->myGraph, // left child
                                     sNonCrtPath->right->myGraph,// right child
                                     aStatement->myPlan->graph,  // right query의 하위 recursive view
                                     & sMyGraph )                 // result graph
              != IDE_SUCCESS );

    // qmgSet 최적화
    IDE_TEST( qmgSetRecursive::optimize( aStatement, sMyGraph )
              != IDE_SUCCESS );

    // 결과 Graph 연결
    sNonCrtPath->myGraph = sMyGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CHILD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoNonCrtPathMgr::optimizeSetRecursive",
                                  "invalid children" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
