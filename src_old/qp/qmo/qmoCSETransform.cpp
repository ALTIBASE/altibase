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
 
/******************************************************************************
 * $Id$
 *
 * Description : PROJ-2242 Common Subexpression Elimination Transformation
 *
 *       - QTC_NODE_JOIN_OPERATOR_EXIST 일 경우 수행 안함
 *       - subquery, host variable, GEOMETRY type arguments 제외
 *       - __OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION property 로 동작
 *
 * 용어 설명 :
 *
 *            1. Idempotent law
 *             - A and A = A
 *             - A or A = A
 *            2. Absorption law
 *             - A and (A or B) = A
 *             - A or (A and B) = A
 *
 *
 * 약어 : CSE (Common Subexpression Elimination)
 *        NNF (Not Normal Form)
 *
 *****************************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qcgPlan.h>
#include <qmoCSETransform.h>
#include <qcg.h>

IDE_RC
qmoCSETransform::doTransform4NNF( qcStatement  * aStatement,
                                  qmsQuerySet  * aQuerySet,
                                  idBool         aIsNNF )
{
/******************************************************************************
 *
 * PROJ-2242 : Normalize 이전 최초 NNF 형태의 모든 조건절에 대해
 *             CSE (common subexpression elimination) 수행
 *
 * Implementation : 1. CSE for where clause predicate
 *                  2. CSE for from tree
 *                  3. CSE for having clause predicate
 *
 ******************************************************************************/

    IDU_FIT_POINT_FATAL( "qmoCSETransform::doTransform4NNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    //--------------------------------------
    // 각 조건절에 대한 CSE 함수 호출
    //--------------------------------------

    if ( aQuerySet->setOp == QMS_NONE )
    {
        // From on clause predicate 처리
        IDE_TEST( doTransform4From( aStatement, aQuerySet->SFWGH->from, aIsNNF )
                  != IDE_SUCCESS );

        // Where clause predicate 처리
        IDE_TEST( doTransform( aStatement, &aQuerySet->SFWGH->where, aIsNNF )
                  != IDE_SUCCESS );

        // Having clause predicate 처리
        IDE_TEST( doTransform( aStatement, &aQuerySet->SFWGH->having, aIsNNF )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( doTransform4NNF( aStatement, aQuerySet->left, aIsNNF )
                  != IDE_SUCCESS );

        IDE_TEST( doTransform4NNF( aStatement, aQuerySet->right, aIsNNF )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCSETransform::doTransform4From( qcStatement  * aStatement,
                                   qmsFrom      * aFrom,
                                   idBool         aIsNNF )
{
/******************************************************************************
 *
 * PROJ-2242 : From 절에 대한 CSE (common subexpression elimination) 수행
 *
 * Implementation :
 *
 ******************************************************************************/

    IDU_FIT_POINT_FATAL( "qmoCSETransform::doTransform4From::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFrom      != NULL );

    //--------------------------------------
    // CSE 함수 호출
    //--------------------------------------

    IDE_TEST( doTransform( aStatement, &aFrom->onCondition, aIsNNF )
              != IDE_SUCCESS );

    if( aFrom->joinType != QMS_NO_JOIN ) // INNER, OUTER JOIN
    {
        IDE_TEST( doTransform4From( aStatement, aFrom->left, aIsNNF )
                  != IDE_SUCCESS);
        IDE_TEST( doTransform4From( aStatement, aFrom->right, aIsNNF )
                  != IDE_SUCCESS);
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
qmoCSETransform::doTransform( qcStatement  * aStatement,
                              qtcNode     ** aNode,
                              idBool         aIsNNF,
                              idBool         aIsWhere,
                              qmsHints     * aHints )
{
/******************************************************************************
 *
 * PROJ-2242 : CSE (common subexpression elimination) 수행
 *             단, 조건절에 oracle style outer mask '(+)' 가 존재할 경우 제외
 *
 * Implementation :
 *
 *          1. ORACLE style outer mask 존재 검사
 *          2. NNF
 *          2.1. Depth 줄이기
 *          2.2. Idempotent law or absorption law 적용
 *          2.3. 하나의 인자를 갖는 logical operator node 제거
 *          3. Normal form
 *          3.1. Idempotent law or absorption law 적용
 *
 ******************************************************************************/

    idBool sExistOuter;

    idBool              sIsTransform    = ID_TRUE;
    qmsHints          * sHints;

    IDU_FIT_POINT_FATAL( "qmoCSETransform::doTransform::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    sHints = aHints;

    //--------------------------------------
    // CSE 수행
    //--------------------------------------
    if ( *aNode == NULL )
    {
        sIsTransform = ID_FALSE;
    }
    else
    {
        // hint 우선
        if ( sHints != NULL )
        {
            if ( ( sHints->partialCSE == ID_TRUE ) &&
                 ( aIsWhere == ID_TRUE ) )
            {
                sIsTransform = ID_FALSE;
            }
        }

        if( sIsTransform == ID_TRUE )
        {
            switch ( QCG_GET_SESSION_ELIMINATE_COMMON_SUBEXPRESSION( aStatement ) )
            {
                case 0:
                    sIsTransform = ID_FALSE;
                    break;
                case 1:
                    break;
                case 2:
                    if ( aIsWhere == ID_TRUE )
                    {
                        sIsTransform = ID_FALSE;
                    }
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }

    if ( sIsTransform == ID_TRUE )
    {
        IDE_TEST( doCheckOuter( *aNode, &sExistOuter ) != IDE_SUCCESS );

        if( sExistOuter == ID_FALSE )
        {
            if( aIsNNF == ID_TRUE )
            {
                IDE_TEST( unnestingAndOr4NNF( aStatement, *aNode )
                          != IDE_SUCCESS );
                IDE_TEST( idempotentAndAbsorption( aStatement, *aNode )
                          != IDE_SUCCESS );
                IDE_TEST( removeLogicalNode4NNF( aStatement, aNode )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( idempotentAndAbsorption( aStatement, *aNode )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // 조건절에 oracle style outer mask '(+)' 가 존재할 경우
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    // environment의 기록
    qcgPlan::registerPlanProperty(
            aStatement,
            PLAN_PROPERTY_OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCSETransform::doCheckOuter( qtcNode  * aNode,
                               idBool   * aExistOuter )
{
/******************************************************************************
 *
 * PROJ-2242 : ORACLE style outer mask 검사
 *
 * Implementation : 노드를 순회하며 QTC_NODE_JOIN_OPERATOR_MASK 검사
 *                  ( parsing 과정에서 setting )
 *
 ******************************************************************************/

    qtcNode * sNode;
    qtcNode * sNext;
    idBool    sExistOuter;

    IDU_FIT_POINT_FATAL( "qmoCSETransform::doCheckOuter::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode       != NULL );
    IDE_DASSERT( aExistOuter != NULL );

    //--------------------------------------
    // Outer mask 검사
    //--------------------------------------

    sNode = aNode;

    do
    {
        sNext = (qtcNode *)(sNode->node.next);

        if( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_OR ||
            ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_AND )
        {
            sNode = (qtcNode *)(sNode->node.arguments);

            IDE_TEST( doCheckOuter( sNode, & sExistOuter )
                      != IDE_SUCCESS );
        }
        else
        {
            if( ( sNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                sExistOuter = ID_TRUE;
            }
            else
            {
                sExistOuter = ID_FALSE;
            }
        }

        if( sExistOuter == ID_TRUE )
        {
            break;
        }
        else
        {
            sNode = sNext;
        }

    } while( sNode != NULL );

    *aExistOuter = sExistOuter;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoCSETransform::unnestingAndOr4NNF( qcStatement * aStatement,
                                     qtcNode     * aNode )
{
/******************************************************************************
 *
 * PROJ-2242 : NNF 에 대해 중첩된 AND,OR 노드 제거
 *
 * Implementation : Parent level, child level 노드가
 *                  AND 또는 OR 노드로 동일할 경우
 *                  child level 의 logical 노드를 제거
 *
 * Parent level :   OR (<- aNode)
 *                   |
 *  Child level :    P --- OR (<- sNode : 제거) --- P (<-sNext) --- ...
 *                          |
 *                          P1 (<- sArg) --- ... --- Pn (<-sArgTail)
 *
 ******************************************************************************/

    qtcNode * sNode;
    qtcNode * sArg;
    qtcNode * sPrev;
    qtcNode * sArgTail = NULL;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDU_FIT_POINT_FATAL( "qmoCSETransform::unnestingAndOr4NNF::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // AND, OR 노드 unnesting 수행
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_OR ||
        ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_AND )
    {
        sPrev = NULL;
        sNode = (qtcNode *)(aNode->node.arguments);

        while( sNode != NULL )
        {
            IDE_TEST( unnestingAndOr4NNF( aStatement, sNode )
                      != IDE_SUCCESS );

            if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK ) ==
                ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK ) )
            {
                // Tail 연결
                sArg = (qtcNode *)(sNode->node.arguments);
                while( sArg != NULL )
                {
                    sArgTail = sArg;
                    sArg = (qtcNode *)(sArg->node.next);
                }

                if( sArgTail != NULL )
                {
                    sArgTail->node.next = sNode->node.next;
                }
                else
                {
                    // Nothing to do.
                }

                // Head 연결
                if( sPrev == NULL )
                {
                    aNode->node.arguments = sNode->node.arguments;
                    IDE_TEST( QC_QMP_MEM( aStatement )->free( sNode )
                              != IDE_SUCCESS );
                    sNode = (qtcNode *)(aNode->node.arguments);
                }
                else
                {
                    sPrev->node.next = sNode->node.arguments;
                    IDE_TEST( QC_QMP_MEM( aStatement )->free( sNode )
                              != IDE_SUCCESS );
                    sNode = (qtcNode *)(sPrev->node.next);
                }
            }
            else
            {
                sPrev = sNode;
                sNode = (qtcNode *)(sNode->node.next);
            }
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
qmoCSETransform::idempotentAndAbsorption( qcStatement * aStatement,
                                          qtcNode     * aNode )
{
/******************************************************************************
 *
 * PROJ-2242 : 두 노드를 비교하여 아래 두 법칙을 적용한다.
 *             NNF 의 경우 수행결과 AND, OR 노드가 하나의 인자를 가질수 있다.
 *             (따라서 본 함수 수행이후 반드시 removeLogicalNode4NNF 를 적용해야 함)
 *
 *          1. Idempotent law : A and A = A, A or A = A
 *          2. Absorption law : A and (A or B) = A, A or (A and B) = A
 *
 * ex) NNF : OR (<-aNode)
 *           |
 *           P1 (<-sTarget) -- AND (<-sCompare:재귀) -- P2 -- AND --...
 *                             |                              |
 *                             Pn ( 결과 : 한 개 존재 가능)   ...
 *
 * ex) CNF : AND (<-aNode)
 *           |
 *           OR (<-sTarget) -- OR (<-sCompare) -- OR --...
 *           |                      |             |
 *           Pn                     Pm            ...
 *
 *      cf) 비교 결과(sResult)를 다음과 같이 구별한다.
 *        - QMO_CFS_COMPARE_RESULT_BOTH : sTarget, sCompare 유지
 *        - QMO_CFS_COMPARE_RESULT_WIN  : sTarget 유지, sCompare 제거
 *        - QMO_CFS_COMPARE_RESULT_LOSE : sTarget 제거, sCompare 유지
 *
 *****************************************************************************/

    qtcNode   * sTarget;
    qtcNode   * sCompare;
    qtcNode   * sTargetPrev;
    qtcNode   * sComparePrev;

    qmoCSECompareResult sResult = QMO_CSE_COMPARE_RESULT_BOTH;

    IDU_FIT_POINT_FATAL( "qmoCSETransform::idempotentAndAbsorption::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // Idempotent, absorption law 수행
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_OR ||
        ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_AND )
    {
        sTargetPrev = NULL;
        sTarget = (qtcNode *)(aNode->node.arguments);

        // sTarget에 대해 우선 수행, 이후 next 순회하며 sCompare 에서 수행
        if( sTarget != NULL )
        {
            IDE_TEST( idempotentAndAbsorption( aStatement, sTarget )
                      != IDE_SUCCESS );
        }

        while( sTarget != NULL )
        {
            sComparePrev = sTarget;
            sCompare = (qtcNode *)(sTarget->node.next);

            while( sCompare != NULL )
            {
                sResult = QMO_CSE_COMPARE_RESULT_BOTH;
                IDE_TEST( idempotentAndAbsorption( aStatement, sCompare )
                          != IDE_SUCCESS );

                // 두 노드의 비교
                IDE_TEST( compareNode( aStatement,
                                       sTarget,
                                       sCompare,
                                       &sResult )
                          != IDE_SUCCESS );

                if( sResult == QMO_CSE_COMPARE_RESULT_WIN )
                {
                    sComparePrev->node.next = sCompare->node.next;

                    // BUGBUG : sCompare argument list free
                    IDE_TEST( QC_QMP_MEM( aStatement )->free( sCompare )
                              != IDE_SUCCESS );
                    sCompare = (qtcNode *)(sComparePrev->node.next);
                }
                else if( sResult == QMO_CSE_COMPARE_RESULT_LOSE )
                {
                    break;
                }
                else
                {
                    sComparePrev = sCompare;
                    sCompare = (qtcNode *)(sCompare->node.next);
                }
            }

            if( sResult == QMO_CSE_COMPARE_RESULT_LOSE )
            {
                if( sTargetPrev == NULL )
                {
                    aNode->node.arguments = sTarget->node.next;

                    // BUGBUG : sTarget argument list free
                    IDE_TEST( QC_QMP_MEM( aStatement )->free( sTarget )
                              != IDE_SUCCESS );
                    sTarget = (qtcNode *)(aNode->node.arguments);
                }
                else
                {
                    sTargetPrev->node.next = sTarget->node.next;

                    // BUGBUG : sTarget argument list free
                    IDE_TEST( QC_QMP_MEM( aStatement )->free( sTarget )
                              != IDE_SUCCESS );
                    sTarget = (qtcNode *)(sTargetPrev->node.next);
                }
            }
            else
            {
                sTargetPrev = sTarget;
                sTarget = (qtcNode *)(sTarget->node.next);
            }

            sResult = QMO_CSE_COMPARE_RESULT_BOTH;
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
qmoCSETransform::compareNode( qcStatement         * aStatement,
                              qtcNode             * aTarget,
                              qtcNode             * aCompare,
                              qmoCSECompareResult * aResult )
{
/******************************************************************************
 *
 * PROJ-2242 : 두 노드를 비교하여 제거 가능한 노드를 결과로 반환한다.
 *
 * Implementation : 노드의 종류와 비교형태에 따라 다음과 같이 적용된다.
 *                  ( LO : AND, OR 노드, OP: 이 외 노드 )
 *
 *               1. OP vs OP -> Idempotent law 적용
 *               2. LO vs OP -> Absorption law 적용
 *               3. OP vs LO -> Absorption law 적용
 *               4. LO vs LO -> Absorption law 적용
 *
 *      cf) 비교 결과(sResult)를 다음과 같이 구별한다.
 *        - QMO_CFS_COMPARE_RESULT_BOTH : sTarget, sCompare 유지
 *        - QMO_CFS_COMPARE_RESULT_WIN  : sTarget 유지, sCompare 제거
 *        - QMO_CFS_COMPARE_RESULT_LOSE : sTarget 제거, sCompare 유지
 *
 *****************************************************************************/

    qtcNode   * sTarget;
    qtcNode   * sTargetArg;
    qtcNode   * sCompare;
    qtcNode   * sCompareArg;
    UInt        sTargetArgCnt;
    UInt        sCompareArgCnt;
    UInt        sArgMatchCnt;
    idBool      sIsEqual;

    qmoCSECompareResult sResult;

    IDU_FIT_POINT_FATAL( "qmoCSETransform::compareNode::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTarget    != NULL );
    IDE_DASSERT( aCompare   != NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sTargetArgCnt = 0;
    sCompareArgCnt = 0;
    sArgMatchCnt = 0;
    sResult = QMO_CSE_COMPARE_RESULT_BOTH;

    sTarget  = aTarget;
    sCompare = aCompare;

    //--------------------------------------
    // target node 와 compare node 의 비교 
    //--------------------------------------

    // BUG-35040 : fix 후 if 조건 제거
    if( ( sTarget->lflag  & QTC_NODE_SUBQUERY_MASK )
        == QTC_NODE_SUBQUERY_EXIST ||
        ( sCompare->lflag & QTC_NODE_SUBQUERY_MASK )
        == QTC_NODE_SUBQUERY_EXIST )
    {
        // 두 노드 중 하나라도 subquery 노드를 인자로 갖는 경우
        sResult = QMO_CSE_COMPARE_RESULT_BOTH;
    }
    else
    {
        if( ( sTarget->node.lflag & MTC_NODE_OPERATOR_MASK )
            != MTC_NODE_OPERATOR_OR &&
            ( sTarget->node.lflag & MTC_NODE_OPERATOR_MASK )
            != MTC_NODE_OPERATOR_AND &&
            ( sCompare->node.lflag & MTC_NODE_OPERATOR_MASK )
            != MTC_NODE_OPERATOR_OR &&
            ( sCompare->node.lflag & MTC_NODE_OPERATOR_MASK )
            != MTC_NODE_OPERATOR_AND )
        {
            // sTarget, sCompare 모두 논리 연산자(OR, AND)가 아님
            // target 과 compare 비교
            sIsEqual = ID_FALSE;

            IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                   sTarget,
                                                   sCompare,
                                                   & sIsEqual )
                      != IDE_SUCCESS )

            sResult = ( sIsEqual == ID_TRUE ) ?
                      QMO_CSE_COMPARE_RESULT_WIN: sResult;
        }
        else if( ( ( sTarget->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_OR ||
                   ( sTarget->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_AND )
                 &&
                 ( ( sCompare->node.lflag & MTC_NODE_OPERATOR_MASK )
                   != MTC_NODE_OPERATOR_OR &&
                   ( sCompare->node.lflag & MTC_NODE_OPERATOR_MASK )
                   != MTC_NODE_OPERATOR_AND ) )
        {
            // sTarget 만 논리 연산자(OR, AND)
            sTargetArg = (qtcNode *)(sTarget->node.arguments);

            while( sTargetArg != NULL )
            {
                // target arguments 와 compare 비교
                sIsEqual = ID_FALSE;
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sTargetArg,
                                                       sCompare,
                                                       & sIsEqual )
                          != IDE_SUCCESS )
                    if( sIsEqual == ID_TRUE )
                    {
                        sResult = QMO_CSE_COMPARE_RESULT_LOSE;
                        break;
                    }
                sTargetArg = (qtcNode *)(sTargetArg->node.next);
            }
        }
        else if( ( ( sTarget->node.lflag & MTC_NODE_OPERATOR_MASK )
                   != MTC_NODE_OPERATOR_OR &&
                   ( sTarget->node.lflag & MTC_NODE_OPERATOR_MASK )
                   != MTC_NODE_OPERATOR_AND )
                 &&
                 ( ( sCompare->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_OR ||
                   ( sCompare->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_AND ) )
        {
            // sCompare 만 논리 연산자(OR, AND)
            sCompareArg = (qtcNode *)(sCompare->node.arguments);

            while( sCompareArg != NULL )
            {
                // target 와 compare arguments 비교
                sIsEqual = ID_FALSE;
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sTarget,
                                                       sCompareArg,
                                                       & sIsEqual )
                          != IDE_SUCCESS )
                if( sIsEqual == ID_TRUE )
                {
                    sResult = QMO_CSE_COMPARE_RESULT_WIN;
                    break;
                }
                sCompareArg = (qtcNode *)(sCompareArg->node.next);
            }
        }
        else
        {   // sTarget, sCompare 모두 논리 연산자(OR, AND)

            // sTarget, sCompare arguments 갯수 획득
            sTargetArg = (qtcNode *)(sTarget->node.arguments);
            sCompareArg = (qtcNode *)(sCompare->node.arguments);

            while( sTargetArg != NULL )
            {
                sTargetArgCnt++;
                sTargetArg = (qtcNode *)(sTargetArg->node.next);
            }

            while( sCompareArg != NULL )
            {
                sCompareArgCnt++;
                sCompareArg = (qtcNode *)(sCompareArg->node.next);
            }

            // sTargetArg, sCompareArg 비교
            sTargetArg = (qtcNode *)(sTarget->node.arguments);

            while( sTargetArg != NULL )
            {
                sCompareArg = (qtcNode *)(sCompare->node.arguments);

                while( sCompareArg != NULL )
                {
                    sIsEqual = ID_FALSE;

                    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                           sTargetArg,
                                                           sCompareArg,
                                                           & sIsEqual )
                              != IDE_SUCCESS )
                    if( sIsEqual == ID_TRUE )
                    {
                        sArgMatchCnt++;
                        break;
                    }
                    sCompareArg = (qtcNode *)(sCompareArg->node.next);
                }
                sTargetArg = (qtcNode *)(sTargetArg->node.next);
            }

            sResult =
                ( sArgMatchCnt == sTargetArgCnt )  ? QMO_CSE_COMPARE_RESULT_WIN:
                ( sArgMatchCnt == sCompareArgCnt ) ? QMO_CSE_COMPARE_RESULT_LOSE:
                QMO_CSE_COMPARE_RESULT_BOTH;
        }
    }

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCSETransform::removeLogicalNode4NNF( qcStatement * aStatement,
                                        qtcNode    ** aNode )
{
/******************************************************************************
 *
 * PROJ-2242 : NNF 에 대해 하나의 argument 를 갖는 logical operator 제거
 *
 * Implementation : 하나의 argument 를 갖는 logical 노드의 인자를 level up
 *
 * Predicate list : where (<- aNode)
 *                    |
 *                   OR (<- sNode)
 *                    |
 *                   P1 --- AND (<- sArg : 제거) --- P2 (<- sNext)-- ...
 *                           |
 *                           P (상위 level 로 끌어올림)
 *
 ******************************************************************************/

    qtcNode  * sNode;
    qtcNode  * sNext;
    qtcNode ** sArg;

    IDU_FIT_POINT_FATAL( "qmoCSETransform::removeLogicalNode4NNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    sNode = *aNode;

    //--------------------------------------
    // 중첩된 logical node 의 제거
    //--------------------------------------

    if( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_OR ||
        ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_AND )
    {
        sArg = (qtcNode **)(&sNode->node.arguments);

        while( *sArg != NULL )
        {
            sNext = (qtcNode *)((*sArg)->node.next);

            IDE_TEST( removeLogicalNode4NNF( aStatement, sArg )
                      != IDE_SUCCESS );

            (*sArg)->node.next = &(sNext->node);
            sArg = (qtcNode **)(&(*sArg)->node.next);
        }

        if( sNode->node.arguments->next == NULL )
        {
            *aNode = (qtcNode *)(sNode->node.arguments);
            IDE_TEST( QC_QMP_MEM( aStatement )->free( sNode )
                      != IDE_SUCCESS );
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
