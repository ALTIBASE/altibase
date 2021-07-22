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
 * $Id: qmoDnfMgr.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     DNF Critical Path Manager
 *
 *     DNF Normalized Form에 대한 최적화를 수행하고
 *     해당 Graph를 생성한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qmoDnfMgr.h>
#include <qmgSelection.h>
#include <qmoCrtPathMgr.h>
#include <qmoCostDef.h>
#include <qmoNormalForm.h>

IDE_RC
qmoDnfMgr::init( qcStatement * aStatement,
                 qmoDNF      * aDNF,
                 qmsQuerySet * aQuerySet,
                 qtcNode     * aNormalDNF )
{
/***********************************************************************
 *
 * Description : qmoDnf의 초기화
 *
 * Implementation :
 *    (1) aNormalDNF를 qmoDNF::normalDNF에 연결한다.
 *    (2) AND 개수만큼 cnfCnt를 설정한다.
 *    (3) cnfCnt 개수만큼 qmoCnf를 위한 배열 공간을 할당받는다.
 *
 ***********************************************************************/

    qmsQuerySet * sQuerySet;
    qmoDNF      * sDNF;
    qtcNode     * sAndNode;
    UInt          sCnfCnt;

    IDU_FIT_POINT_FATAL( "qmoDnfMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDNF != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aNormalDNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sQuerySet = aQuerySet;
    sCnfCnt   = 0;

    //------------------------------------------
    // DNF 초기화
    //------------------------------------------

    sDNF = aDNF;

    sDNF->normalDNF = aNormalDNF;
    sDNF->myGraph = NULL;
    sDNF->myQuerySet = sQuerySet;
    sDNF->cost = 0;

    //------------------------------------------
    // PROJ-1405
    // Rownum Predicate 처리
    //------------------------------------------

    if ( ( aNormalDNF->lflag & QTC_NODE_ROWNUM_MASK )
         == QTC_NODE_ROWNUM_EXIST )
    {
        // rownum predicate인 경우 critical path로 복사해서 올린다.
        IDE_TEST( qmoCrtPathMgr::addRownumPredicateForNode( aStatement,
                                                            sQuerySet,
                                                            aNormalDNF,
                                                            ID_TRUE )
                  != IDE_SUCCESS );

        // rownum predicate을 제거한다.
        IDE_TEST( removeRownumPredicate( aStatement, sDNF )
                  != IDE_SUCCESS );
    }
    else
    {
        // Noting to do.
    }

    //------------------------------------------
    // CNF 개수 계산
    //------------------------------------------

    if ( aNormalDNF->node.arguments != NULL )
    {
        for ( sAndNode = (qtcNode *) aNormalDNF->node.arguments, sCnfCnt = 0;
              sAndNode != NULL;
              sAndNode = (qtcNode *)sAndNode->node.next, sCnfCnt++ ) ;
    }
    else
    {
        // BUG-17950
        // 빈 AND그룹이 발생하여 full(?) selection을 수행하는 AND 그룹을
        // 한개 생성한다.
        sCnfCnt = 1;
    }

    sDNF->cnfCnt = sCnfCnt;
    sDNF->dnfGraphCnt = sCnfCnt - 1;

    //------------------------------------------
    // CNF 개수만큼 qmoCNF를 위한 배열 공간 할당 받음
    //------------------------------------------

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoCNF ) * sCnfCnt,
                                             (void **)& sDNF->myCNF )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDnfMgr::optimize( qcStatement * aStatement,
                     qmoDNF      * aDNF,
                     SDouble       aCnfCost )
{
/***********************************************************************
 *
 * Description : DNF Processor의 최적화( 즉, qmoDNF의 최적화)
 *
 * Implementation :
 *    (1) 각 qmoCNF(들)의 초기화, 최적화 함수 호출
 *        - aCnfCost != 0 : DNF Prunning 수행
 *        - aCnfCost == 0 : DNF Prunning 수행하지 않음
 *    (2)(cnfCnt - 1)개수만큼 dnfGraph를 위한 배열 공간을 할당받음
 *    (3) dnfNotNormalForm의 생성
 *    (4) 각 dnf의 초기화
 *    (5) 각 dnf의 최적화
 *
 ***********************************************************************/

    qmoCNF   * sCNF;
    qmoDNF   * sDNF;
    qtcNode  * sAndNode;
    SDouble    sDnfCost;
    idBool     sIsPrunning;
    qmgGraph * sResultGraph = NULL;
    UInt       i;
    UInt       sCnfCnt;
    UInt       sDnfGraphCnt;

    IDU_FIT_POINT_FATAL( "qmoDnfMgr::optimize::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sDNF         = aDNF;
    sDnfCost     = 0;
    sIsPrunning  = ID_FALSE;
    sCnfCnt      = sDNF->cnfCnt;
    sDnfGraphCnt = sDNF->dnfGraphCnt;

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // dnf optimization시 생성된 cnf의 개수를 초기화한다.
    sDNF->madeCnfCnt = 0;

    //------------------------------------------
    // 각 CNF들의 초기화, 최적화
    //------------------------------------------

    sAndNode = (qtcNode *)sDNF->normalDNF->node.arguments;

    for ( i = 0; i < sCnfCnt; i++ )
    {
        sCNF = & sDNF->myCNF[i];

        IDE_TEST( qmoCnfMgr::init( aStatement,
                                   sCNF,
                                   sDNF->myQuerySet,
                                   sAndNode,
                                   NULL )
                  != IDE_SUCCESS );

        IDE_TEST( qmoCnfMgr::optimize( aStatement, sCNF ) != IDE_SUCCESS );

        // PROJ-1446 Host variable을 포함한 질의 최적화
        // dnf optimization시 생성된 cnf의 개수를 증가시킨다.
        sDNF->madeCnfCnt++;

        sDnfCost += sCNF->cost;

        // Join을 DNF로 처리할 경우에 대한 Penalty 부여
        switch ( sCNF->myGraph->type )
        {
            case QMG_INNER_JOIN:
            case QMG_LEFT_OUTER_JOIN:
            case QMG_FULL_OUTER_JOIN:
                sDnfCost += ( sCNF->cost * QMO_COST_DNF_JOIN_PENALTY );
                break;
            default:
                // Nothing To Do
                break;
        }

        if (QMO_COST_IS_EQUAL(aCnfCost, 0.0) == ID_FALSE)
        {
            //------------------------------------------
            // DNF Prunning 수행
            //------------------------------------------

            // BUG-42400 cost 비교시 매크로를 사용해야 합니다.
            if ( QMO_COST_IS_GREATER(aCnfCost, sDnfCost) == ID_TRUE )
            {
                // nothing to do
            }
            else
            {
                // DNF Prunning
                sIsPrunning = ID_TRUE;
                break;
            }
        }
        else
        {
            //------------------------------------------
            // DNF Prunning 수행하지 않음
            //------------------------------------------

            // nothing to do
        }

        if ( sAndNode != NULL )
        {
            sAndNode = (qtcNode *)sAndNode->node.next;
        }
        else
        {
            // Nothing to do.
        }
    }

    // To Fix PR-8237
    sDNF->cost = sDnfCost;

    if ( sIsPrunning == ID_FALSE )
    {
        // To Fix PR-8727
        // 사용자 Hint나 CNF Only Predicate으로 판단되지 않는
        // CNF Only Predicate의 경우 DNF Graph의 개수가 0일 수 있음.
        // 따라서, 0보다 클 경우에만 DNF Graph를 생성해야 함.

        if ( sDnfGraphCnt > 0 )
        {
            //------------------------------------------
            // dnf Graph를 위한 배열 공간을 할당 받음
            //------------------------------------------

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmgDNF) * sDnfGraphCnt,
                                                     (void**) & sDNF->dnfGraph )
                != IDE_SUCCESS );

            //------------------------------------------
            // dnfNotNormalForm 배열 생성
            //------------------------------------------

            IDE_TEST( makeNotNormal( aStatement, sDNF ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
            // 사용자 Hint나 CNF Only Predicate으로 판단되지 않는
            // CNF Only Predicate의 경우 DNF Graph가 필요하지 않다.
        }

        for ( i = 0; i < sCnfCnt ; i++ )
        {
            if ( i == 0 )
            {
                sResultGraph = sDNF->myCNF[i].myGraph;
            }
            else
            {
                //------------------------------------------
                // 각 Dnf Graph의 초기화
                //------------------------------------------

                IDE_TEST( qmgDnf::init( aStatement,
                                        sDNF->notNormal[i-1],
                                        sResultGraph,
                                        sDNF->myCNF[i].myGraph,
                                        & sDNF->dnfGraph[i-1].graph )
                          != IDE_SUCCESS );

                sResultGraph = & sDNF->dnfGraph[i-1].graph;

                //------------------------------------------
                // 각 Dnf Graph의 최적화
                //------------------------------------------

                IDE_TEST( qmgDnf::optimize( aStatement,
                                            sResultGraph )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        // Prunning 되었으므로 Dnf Graph 생성할 필요 없음
        // Nothing to do...
    }

    // To Fix BUG-8247
    sDNF->myGraph = sResultGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDnfMgr::removeOptimizationInfo( qcStatement * aStatement,
                                   qmoDNF      * aDNF )
{
    UInt i;
    UInt j;

    IDU_FIT_POINT_FATAL( "qmoDnfMgr::removeOptimizationInfo::__FT__" );

    for( i = 0; i < aDNF->madeCnfCnt; i++ )
    {
        for( j = 0; j < aDNF->myCNF[i].graphCnt4BaseTable; j++ )
        {
            IDE_TEST( qmg::removeSDF( aStatement,
                                      aDNF->myCNF[i].baseGraph[j] )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDnfMgr::makeNotNormal( qcStatement * aStatement,
                          qmoDNF      * aDNF )
{
/***********************************************************************
 *
 * Description : Dnf Not Normal Form들을 만든다.
 *
 * Implementation :
 *
 *    다음과 같은 DNF Normal Form이 있다고 가정하자
 *
 *           [OR]
 *            |
 *           [AND1]-->[AND2]-->[AND3]
 *
 *   첫번째 DNF Not Normal Form을 작성
 *
 *       1.  makeDnfNotNormal()              2.  AND 노드로 연결
 *
 *                                              [AND]
 *                                                 |
 *           [LNNVL]                            [LNNVL]
 *              |                   ==>            |
 *           [AND1]'                            [AND1]'
 *
 *    두번째 DNF Not Normal Form을 작성
 *
 *       3. qtc::copyAndForDnfFilter() 로 2를 복사
 *
 *           [AND]'
 *              |
 *           [LNNVL]' <-- sLastNode
 *              |
 *           [AND1]''
 *
 *       4.  [AND2]에 대한 DNF Not Normal Form을 작성
 *
 *           [LNNVL]
 *              |
 *           [AND2]'
 *
 *       5.  3과 4를 연결
 *
 *           [AND]'
 *              |
 *           [LNNVL]' --->   [LNNVL]
 *              |               |
 *           [AND1]''        [AND2]'
 *
 *
 *
 ***********************************************************************/

    qtcNode      * sNormalForm;
    qtcNode     ** sNotNormalForm;
    qtcNode      * sDnfNotNode;
    qtcNode      * sAndNode[2];
    qtcNode      * sLastNode;
    qcNamePosition sNullPosition;
    UInt           sNotNormalFormCnt;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmoDnfMgr::makeNotNormal::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNotNormalFormCnt = aDNF->dnfGraphCnt;
    SET_EMPTY_POSITION(sNullPosition);

    // 공간 할당
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode *) * sNotNormalFormCnt,
                                             (void **) & sNotNormalForm )
              != IDE_SUCCESS );

    // 각 not normal form 생성
    for ( i = 0, sNormalForm = (qtcNode *)aDNF->normalDNF->node.arguments;
          i < sNotNormalFormCnt;
          i++, sNormalForm = (qtcNode *)sNormalForm->node.next )
    {
        if ( i == 0 )
        {
            IDE_TEST( makeDnfNotNormal( aStatement,
                                        sNormalForm,
                                        & sDnfNotNode )
                      != IDE_SUCCESS );

            // And Node 생성
            IDE_TEST( qtc::makeNode( aStatement,
                                     sAndNode,
                                     & sNullPosition,
                                     (const UChar*)"AND",
                                     3 )
                      != IDE_SUCCESS );

            sAndNode[0]->node.arguments = (mtcNode *)sDnfNotNode;

            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        sAndNode[0] )
                      != IDE_SUCCESS );

            sNotNormalForm[i] = sAndNode[0];
        }
        else
        {
            IDE_TEST( qtc::copyAndForDnfFilter( aStatement,
                                                sNotNormalForm[i-1],
                                                & sNotNormalForm[i],
                                                & sLastNode )
                      != IDE_SUCCESS );

            // TASK-3876 codesonar
            IDE_TEST_RAISE( sLastNode == NULL,
                            ERR_DNF_FILETER );

            IDE_TEST( makeDnfNotNormal( aStatement,
                                        sNormalForm,
                                        & sDnfNotNode )
                      != IDE_SUCCESS );

            sLastNode->node.next = (mtcNode *)sDnfNotNode;
        }
    }

    aDNF->notNormal = sNotNormalForm;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DNF_FILETER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoDnfMgr::makeNotNormal",
                                  "fail to copy dnf filter" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDnfMgr::makeDnfNotNormal( qcStatement * aStatement,
                             qtcNode     * aNormalForm,
                             qtcNode    ** aDnfNotNormal )
{
/***********************************************************************
 *
 * Description : Dnf Not Normal Form을 만든다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode      * sNode;

    IDU_FIT_POINT_FATAL( "qmoDnfMgr::makeDnfNotNormal::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNormalForm != NULL );

    //------------------------------------------
    // Normal Form 복사
    //------------------------------------------
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                     aNormalForm,
                                     & sNode,
                                     ID_FALSE,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                           &sNode )
              != IDE_SUCCESS );

    IDE_TEST( qtc::lnnvlNode( aStatement,
                              sNode )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sNode )
              != IDE_SUCCESS );

    *aDnfNotNormal = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDnfMgr::removeRownumPredicate( qcStatement * aStatement,
                                  qmoDNF      * aDNF )
{
/***********************************************************************
 *
 * Description :
 *     각 AND 그룹에서 rownum predicate을 제거한다.
 *
 *     rownum predicate이 제거하게 되면 predicate이 없는 AND 그룹이
 *     만들어질 수 있는데, 이 AND 그룹은 모든 레코드가 올라와야
 *     하는 selection을 수행한다.
 *
 *     그러나, 이런 그룹은 하나 이상일 수 있어 predicate이 없는
 *     AND 그룹은 모두 제거하고, 후에 빈 AND 그룹을 하나 추가할 수
 *     있도록 flag를 반환한다.
 *
 *
 *     예1) where ROWNUM < 1 and ( i2 = 2 or ROWNUM < 3 );
 *
 *     [where]
 *
 *       OR
 *       |
 *      AND ----------------- AND
 *       |                     |
 *       = ------ =            = -------- <
 *       |        |            |          |
 *    ROWNUM - 1  i2 - 2    ROWNUM - 1  ROWNUM - 3
 *
 *                      |
 *                      V
 *
 *     [where]
 *
 *     [AND그룹 1]            [AND그룹 2]
 *
 *      AND                   AND
 *       |                     |
 *       = ------ =            = -------- <
 *       |        |            |          |
 *    ROWNUM - 1  i2 - 2    ROWNUM - 1  ROWNUM - 3
 *
 *                      |
 *                      V
 *
 *     [where]
 *
 *     [AND그룹 1]            [AND그룹 2]
 *
 *      AND                   AND
 *       |
 *       =
 *       |
 *      i2 - 2
 *
 *                      |
 *                      V
 *
 *     [where]
 *
 *     [AND그룹 1]            [AND그룹 2]
 *
 *      AND                   빈 AND 그룹
 *       |
 *       =
 *       |
 *      i2 - 2
 *
 *
 *    예2) where ROWNUM < 1 or ROWNUM < 3;
 *
 *     [where]
 *
 *       OR
 *       |
 *      AND ----------------- AND
 *       |                     |
 *       =                     =
 *       |                     |
 *    ROWNUM - 1            ROWNUM - 3
 *
 *                      |
 *                      V
 *
 *     [where]
 *
 *     [AND그룹 1]            [AND그룹 2]
 *
 *      AND                   AND
 *       |                     |
 *       =                     =
 *       |                     |
 *    ROWNUM - 1            ROWNUM - 3
 *
 *                      |
 *                      V
 *
 *     [where]
 *
 *     [AND그룹 1]            [AND그룹 2]
 *
 *      AND                   AND
 *
 *                      |
 *                      V
 *
 *     [where]
 *
 *     [AND그룹 1]
 *
 *     빈 AND 그룹
 *
 *
 * Implementation :
 *     1. AND 그룹내의 rownum predicate을 제거한다.
 *     2. predicate이 없는 AND 그룹을 제거한다.
 *
 ***********************************************************************/

    qtcNode  * sNormalDNF;
    qtcNode  * sAndNode;
    qtcNode  * sCompareNode;
    qtcNode  * sFirstNode;
    qtcNode  * sPrevNode;
    qtcNode  * sNode;
    idBool     sEmptyAndGroup;

    IDU_FIT_POINT_FATAL( "qmoDnfMgr::removeRownumPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sNormalDNF = aDNF->normalDNF;
    sEmptyAndGroup = ID_FALSE;

    //------------------------------------------
    // AND 그룹 내의 rownum predicate을 제거한다.
    //------------------------------------------

    for ( sAndNode = (qtcNode *)sNormalDNF->node.arguments;
          sAndNode != NULL;
          sAndNode = (qtcNode *)sAndNode->node.next )
    {
        sFirstNode = NULL;
        sPrevNode = NULL;

        for ( sCompareNode = (qtcNode *) sAndNode->node.arguments;
              sCompareNode != NULL;
              sCompareNode = (qtcNode *) sCompareNode->node.next )
        {
            if ( ( sCompareNode->lflag & QTC_NODE_ROWNUM_MASK )
                 == QTC_NODE_ROWNUM_EXIST )
            {
                // rownum predicate은 제거된다.

                // Nothing to do.
            }
            else
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                         (void **)&sNode )
                             != IDE_SUCCESS );

                idlOS::memcpy( sNode, sCompareNode, ID_SIZEOF(qtcNode) );

                if ( sPrevNode == NULL )
                {
                    sFirstNode = sNode;
                    sPrevNode = sNode;
                    sNode->node.next = NULL;
                }
                else
                {
                    sPrevNode->node.next = (mtcNode*) sNode;
                    sPrevNode = sNode;
                    sNode->node.next = NULL;
                }
            }
        }

        sAndNode->node.arguments = (mtcNode*) sFirstNode;
    }

    //------------------------------------------
    // predicate이 없는 AND 그룹을 제거한다.
    //------------------------------------------

    for ( sAndNode = (qtcNode *)sNormalDNF->node.arguments;
          sAndNode != NULL;
          sAndNode = (qtcNode *)sAndNode->node.next )
    {
        if ( sAndNode->node.arguments == NULL )
        {
            // BUG-17950
            // predicate이 없는 빈 AND 그룹이 있는 경우
            // 나머지 AND 그룹은 제거할 수 있다.
            sEmptyAndGroup = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sEmptyAndGroup == ID_TRUE )
    {
        // 빈 AND그룹이 생성되어야 함을 normalDNF의 arguments가
        // NULL인것으로 알린다.
        aDNF->normalDNF->node.arguments = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
