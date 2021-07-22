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
 * Description : PROJ-2242 Filter Subsumption Transformation
 *
 *       - AND, OR 에 대한 TRUE, FALSE predicate 적용
 *       - QTC_NODE_JOIN_OPERATOR_EXIST 일 경우 수행 안함
 *       - subquery, host variable, GEOMETRY type arguments 제외
 *       - __OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION property 로 동작
 *
 * 용어 설명 :
 *
 * 약어 : CFS (Constant Filter Subsumption)
 *
 *****************************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qcgPlan.h>
#include <qcuProperty.h>
#include <qmoCSETransform.h>
#include <qmoCFSTransform.h>
#include <qcg.h> /* TASK-7219 Shard Transformer Refactoring */
#include <sdi.h> /* TASK-7219 Shard Transformer Refactoring */

IDE_RC
qmoCFSTransform::doTransform4NNF( qcStatement  * aStatement,
                                  qmsQuerySet  * aQuerySet )
{
/******************************************************************************
 *
 * PROJ-2242 : 최초 NNF 형태의 모든 조건절에 대해 CFS (Filter Subsumption) 수행
 *
 * Implementation : 1. CFS for where clause predicate
 *                  2. CFS for from tree
 *                  3. CFS for having clause predicate
 *
 ******************************************************************************/

    IDU_FIT_POINT_FATAL( "qmoCFSTransform::doTransform4NNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    //--------------------------------------
    // CFS 함수 수행
    //--------------------------------------

    if ( aQuerySet->setOp == QMS_NONE )
    {
        // From on clause predicate 처리
        IDE_TEST( doTransform4From( aStatement, aQuerySet->SFWGH->from )
                  != IDE_SUCCESS );

        // Where clause predicate 처리
        IDE_TEST( doTransform( aStatement, &aQuerySet->SFWGH->where )
                  != IDE_SUCCESS );

        // Having clause predicate 처리
        IDE_TEST( doTransform( aStatement, &aQuerySet->SFWGH->having )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( doTransform4NNF( aStatement, aQuerySet->left ) != IDE_SUCCESS );

        IDE_TEST( doTransform4NNF( aStatement, aQuerySet->right ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCFSTransform::doTransform4From( qcStatement  * aStatement,
                                   qmsFrom      * aFrom )
{
/******************************************************************************
 *
 * PROJ-2242 : From 절에 대한 CFS (Constant Filter Subsumption) 수행
 *
 * Implementation : From tree 를 순회하며 CFS 수행
 *
 ******************************************************************************/

    IDU_FIT_POINT_FATAL( "qmoCFSTransform::doTransform4From::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFrom      != NULL );

    //--------------------------------------
    // CFS 수행
    //--------------------------------------

    IDE_TEST( doTransform( aStatement, &aFrom->onCondition ) != IDE_SUCCESS );

    if( aFrom->joinType != QMS_NO_JOIN ) // INNER, OUTER JOIN
    {
        IDE_TEST( doTransform4From( aStatement, aFrom->left ) != IDE_SUCCESS );
        IDE_TEST( doTransform4From( aStatement, aFrom->right ) != IDE_SUCCESS );
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
qmoCFSTransform::doTransform( qcStatement  * aStatement,
                              qtcNode     ** aNode )
{
/******************************************************************************
 *
 * PROJ-2242 : NNF 의 CFS (Constant Filter subsumption) 수행
 *             단, 조건절에 oracle style outer mask '(+)' 가 존재할 경우 제외
 *
 * Implementation :
 *
 *       1. ORACLE style outer mask 존재 검사
 *       2. NNF 의 constant filter subsumption 수행
 *       3. Envieronment 의 기록
 *
 ******************************************************************************/

    idBool sExistOuter;

    IDU_FIT_POINT_FATAL( "qmoCFSTransform::doTransform::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // CFS 수행
    //--------------------------------------

    if( *aNode != NULL && QCU_OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION == 1 )
    {
        IDE_TEST( qmoCSETransform::doCheckOuter( *aNode, &sExistOuter )
                  != IDE_SUCCESS );

        if( sExistOuter == ID_FALSE )
        {
            IDE_TEST( constantFilterSubsumption( aStatement, aNode, ID_TRUE )
                      != IDE_SUCCESS );
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
            PLAN_PROPERTY_OPTIMIZER_CONSTANT_FILTER_SUBSUMPTION );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCFSTransform::constantFilterSubsumption( qcStatement * aStatement,
                                            qtcNode    ** aNode,
                                            idBool        aIsRoot )
{
/******************************************************************************
 *
 * PROJ-2242 : NNF 에 대해 constant filter subsumption 을 수행한다.
 *
 * Implementation : AND, OR 에 대해 다음과 같이 적용한다.
 *                  참고로 CSE 수행이후라 T^T, F^F, TvT, FvF 비교는 존재할 수
 *                  없지만 독립적 수행이 가능하도록 모두 고려하기로 한다.
 *
 *      1. OR  : T ^ F = F, T ^ P = P, F ^ P = F, F ^ F = F, T ^ T = T
 *      2. AND : T v F = T, T v P = T, F v P = P, F v F = F, T v T = T
 *         (최상위 level 의 F ^ P 의 경우 subsumption 적용하지 않는다.)
 *
 *      ex) NNF : Predicate list pointer (<-aNode)
 *                 |
 *                OR
 *                 |
 *                P1 (<-sTarget) -- AND (<-sCompare:재귀) -- P2 -- AND --...
 *                                   |
 *                                  Pn ( 수행결과 한 개이면 level up) ...
 *
 *      cf) 비교 결과(sResult)를 다음과 같이 구별한다.
 *        - QMO_CFS_COMPARE_RESULT_BOTH : sTarget, sCompare 유지
 *        - QMO_CFS_COMPARE_RESULT_WIN  : sTarget 유지, sCompare 제거
 *        - QMO_CFS_COMPARE_RESULT_LOSE : sTarget 제거, sCompare 유지
 *
 ******************************************************************************/

    qtcNode            ** sTarget;
    qtcNode            ** sCompare;
    qtcNode             * sTargetPrev;
    qtcNode             * sComparePrev;
    qtcNode             * sTemp;
    mtdBooleanType        sTargetValue;
    mtdBooleanType        sCompareValue;
    qmoCFSCompareResult   sResult = QMO_CFS_COMPARE_RESULT_BOTH;

    IDU_FIT_POINT_FATAL( "qmoCFSTransform::constantFilterSubsumption::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // CFS 수행
    //--------------------------------------

    if( ( (*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_OR ||
        ( (*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_AND )
    {
        sTargetPrev = NULL;
        sTarget = (qtcNode **)(&(*aNode)->node.arguments);

        // sTarget에 대해 우선 수행, 이후 next 순회하며 sCompare 에서 수행
        if( *sTarget != NULL )
        {
            sTemp = (qtcNode *)((*sTarget)->node.next);

            IDE_TEST( constantFilterSubsumption( aStatement,
                                                 sTarget,
                                                 ID_FALSE )
                      != IDE_SUCCESS );

            (*sTarget)->node.next = &(sTemp->node);
        }
        else
        {
            // Nothing to do.
        }

        // sTarget 순회
        while( *sTarget != NULL )
        {
            sResult = QMO_CFS_COMPARE_RESULT_BOTH;
            sTargetValue = MTD_BOOLEAN_NULL;

            if( (*sTarget)->node.module == &qtc::valueModule &&
                ( (*sTarget)->node.lflag & QTC_NODE_PROC_VAR_MASK )
                == QTC_NODE_PROC_VAR_ABSENT )
            {
                sTargetValue = 
                    *( (mtdBooleanType *)
                     ( (SChar*)QTC_STMT_TUPLE( aStatement, *sTarget )->row +
                      QTC_STMT_COLUMN( aStatement, *sTarget )->column.offset ) );
            }
            else
            {
                // Nothing to do.
            }

            sComparePrev = (qtcNode *)(&(*sTarget)->node);
            sCompare = (qtcNode **)(&(*sTarget)->node.next);

            // sCompare 순회
            while( *sCompare != NULL )
            {
                sTemp = (qtcNode *)((*sCompare)->node.next);

                IDE_TEST( constantFilterSubsumption( aStatement,
                                                     sCompare,
                                                     ID_FALSE )
                          != IDE_SUCCESS );

                (*sCompare)->node.next = &(sTemp->node);

                sResult = QMO_CFS_COMPARE_RESULT_BOTH;
                sCompareValue = MTD_BOOLEAN_NULL;

                if( (*sCompare)->node.module == &qtc::valueModule &&
                    ( (*sCompare)->node.lflag & QTC_NODE_PROC_VAR_MASK )
                    == QTC_NODE_PROC_VAR_ABSENT )
                {
                    sCompareValue =
                        *( (mtdBooleanType *)
                         ( (SChar*)QTC_STMT_TUPLE( aStatement, *sCompare )->row +
                          QTC_STMT_COLUMN( aStatement, *sCompare )->column.offset ) );
                }
                else
                {
                    // Nothing to do.
                }

                // 두 노드의 비교
                if( ( (*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK )
                    == MTC_NODE_OPERATOR_AND )
                {
                    sResult = 
                        ( sTargetValue == MTD_BOOLEAN_TRUE && sCompareValue == MTD_BOOLEAN_TRUE) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_TRUE && sCompareValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_LOSE:
                        ( sTargetValue == MTD_BOOLEAN_TRUE ) ? QMO_CFS_COMPARE_RESULT_LOSE:
                        ( sTargetValue == MTD_BOOLEAN_FALSE && sCompareValue == MTD_BOOLEAN_TRUE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_FALSE && sCompareValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_FALSE && aIsRoot == ID_TRUE ) ? QMO_CFS_COMPARE_RESULT_BOTH:
                        ( sTargetValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sCompareValue == MTD_BOOLEAN_TRUE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sCompareValue == MTD_BOOLEAN_FALSE && aIsRoot == ID_TRUE) ? QMO_CFS_COMPARE_RESULT_BOTH:
                        ( sCompareValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_LOSE: QMO_CFS_COMPARE_RESULT_BOTH;
                }
                else // OR
                {
                    sResult = 
                        ( sTargetValue == MTD_BOOLEAN_TRUE && sCompareValue == MTD_BOOLEAN_TRUE) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_TRUE && sCompareValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_TRUE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_FALSE && sCompareValue == MTD_BOOLEAN_TRUE ) ? QMO_CFS_COMPARE_RESULT_LOSE:
                        ( sTargetValue == MTD_BOOLEAN_FALSE && sCompareValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_WIN:
                        ( sTargetValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_LOSE:
                        ( sCompareValue == MTD_BOOLEAN_TRUE ) ? QMO_CFS_COMPARE_RESULT_LOSE:
                        ( sCompareValue == MTD_BOOLEAN_FALSE ) ? QMO_CFS_COMPARE_RESULT_WIN: QMO_CFS_COMPARE_RESULT_BOTH;
                }

                if( sResult == QMO_CFS_COMPARE_RESULT_WIN )
                {
                    // sCompare 제거
                    sTemp = *sCompare;
                    sComparePrev->node.next = (*sCompare)->node.next;
                    sCompare = (qtcNode **)(&sComparePrev->node.next);

                    /* TASK-7219 Shard Transformer Refactoring */
                    if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                        SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
                         == ID_TRUE )
                    {
                        IDE_TEST( sdi::preAnalyzeSubquery( aStatement,
                                                           sTemp,
                                                           QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST( QC_QMP_MEM( aStatement )->free( sTemp )
                              != IDE_SUCCESS );
                }
                else if( sResult == QMO_CFS_COMPARE_RESULT_LOSE )
                {
                    // sTarget 제거
                    break;
                }
                else
                {
                    // sTarget, sCompare 유지
                    sComparePrev = (qtcNode *)(&(*sCompare)->node);
                    sCompare = (qtcNode **)(&(*sCompare)->node.next);
                }
            }

            if( sResult == QMO_CFS_COMPARE_RESULT_LOSE )
            {
                // sTarget 제거
                sTemp = *sTarget;
                if( sTargetPrev == NULL )
                {
                    (*aNode)->node.arguments = (*sTarget)->node.next;
                    sTarget = (qtcNode **)(&(*aNode)->node.arguments);
                }
                else
                {
                    sTargetPrev->node.next = (*sTarget)->node.next;
                    sTarget = (qtcNode **)(&sTargetPrev->node.next);
                }

                /* TASK-7219 Shard Transformer Refactoring */
                if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                    SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
                         == ID_TRUE )
                {
                    IDE_TEST( sdi::preAnalyzeSubquery( aStatement,
                                                       sTemp,
                                                       QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( QC_QMP_MEM( aStatement )->free( sTemp )
                          != IDE_SUCCESS );
            }
            else
            {
                sTargetPrev = (qtcNode *)(&(*sTarget)->node);
                sTarget = (qtcNode **)(&(*sTarget)->node.next);
            }
        }

        // 하나의 인자를 갖는 logical operator 처리
        if( (*aNode)->node.arguments->next == NULL )
        {
            sTemp = *aNode;
            *aNode = (qtcNode *)((*aNode)->node.arguments);

            /* TASK-7219 Shard Transformer Refactoring */
            if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
                         == ID_TRUE )
            {
                IDE_TEST( sdi::preAnalyzeSubquery( aStatement,
                                                   sTemp,
                                                   QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( QC_QMP_MEM( aStatement )->free( sTemp ) != IDE_SUCCESS );
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
