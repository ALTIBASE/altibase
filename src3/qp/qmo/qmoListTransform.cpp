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
 * $Id: qmoListTransform.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description : BUG-36438 LIST Transformation
 *
 * ex) (t1.i1, t1.i2) = (t2.i1, t2.i2)  => t1.i1=t2.i1 AND t1.i2=t2.i2
 *     (t1.i1, t2.i1) = (1,1)           => t1.i1=1 AND t1.i2=1
 *     (t1.i1, t1.i2) != (t2.i1, t2.i2) => t1.i1!=t2.i1 OR t2.i1!=t2.i2
 *     (t1.i1, t2.i1) != (1,1)          => t1.i1!=1 OR t2.i1!=1
 *
 * 용어 설명 :
 *
 *****************************************************************************/

#include <idl.h>
#include <qtc.h>
#include <qcgPlan.h>
#include <qcuProperty.h>
#include <qmoCSETransform.h>
#include <qmoListTransform.h>

extern mtfModule mtfAnd;
extern mtfModule mtfOr;
extern mtfModule mtfEqual;
extern mtfModule mtfNotEqual;
extern mtfModule mtfList;

IDE_RC qmoListTransform::doTransform( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet )
{
/******************************************************************************
 *
 * Description : qmsQuerySet 의 모든 조건절에 대해 수행
 *
 * Implementation : QCU_OPTIMIZER_LIST_TRANSFORMATION property 로 동작하며
 *                  다음 항목에 대해 변환
 *
 *               - for from tree
 *               - for where clause predicate
 *               - for having clause predicate
 *
 ******************************************************************************/

    IDU_FIT_POINT_FATAL( "qmoListTransform::doTransform::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    //--------------------------------------
    // 조건절에 대한 변환 함수 호출
    //--------------------------------------

    if ( QCU_OPTIMIZER_LIST_TRANSFORMATION == 1 )
    {
        if ( aQuerySet->setOp == QMS_NONE )
        {
            IDE_TEST( doTransform4From( aStatement,
                                        aQuerySet->SFWGH->from )
                      != IDE_SUCCESS );

            IDE_TEST( listTransform( aStatement,
                                     & aQuerySet->SFWGH->where )
                      != IDE_SUCCESS );

            IDE_TEST( listTransform( aStatement,
                                     & aQuerySet->SFWGH->having )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( doTransform( aStatement,
                                   aQuerySet->left )
                      != IDE_SUCCESS );

            IDE_TEST( doTransform( aStatement,
                                   aQuerySet->right )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_LIST_TRANSFORMATION );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoListTransform::doTransform4From( qcStatement * aStatement,
                                           qmsFrom     * aFrom )
{
/******************************************************************************
 *
 * Description : From 절에 대한 수행
 *
 * Implementation : From tree 순회
 *
 ******************************************************************************/

    IDU_FIT_POINT_FATAL( "qmoListTransform::doTransform4From::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFrom      != NULL );

    //--------------------------------------
    // FROM tree 순회
    //--------------------------------------

    if ( aFrom->joinType != QMS_NO_JOIN ) // INNER, OUTER JOIN
    {
        IDE_TEST( listTransform( aStatement,
                                 & aFrom->onCondition )
                  != IDE_SUCCESS );

        IDE_TEST( doTransform4From( aStatement,
                                    aFrom->left  )
                  != IDE_SUCCESS );

        IDE_TEST( doTransform4From( aStatement,
                                    aFrom->right )
                  != IDE_SUCCESS );
    }
    else
    {
        // QMS_NO_JOIN 일 경우 onCondition 은 존재하지 않는다.
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoListTransform::listTransform( qcStatement  * aStatement,
                                        qtcNode     ** aNode )
{
/******************************************************************************
 *
 * Description : LIST 변환을 수행한다.
 *
 * Implementation :
 *
 *     Logical operator 이하의 [NOT] EQUAL 연산자에 한해 수행된다.
 *
 *     [Before] AND/OR(sParent)
 *               |
 *               ... --- =(sCompare) --- ...
 *                       |
 *                       LIST(sLeftList) ------ LIST (sRightList)
 *                       |                      |
 *                       t1.i1(sLeftArg) - ...  t2.i1(sRightArg) - ...
 *
 *     [After]  AND/OR
 *               |
 *               ... --- AND --- ...
 *                       |
 *                       = ------------- = ------------- ...
 *                       |               |
 *                       t1.i1 - t2.i1   ...
 *
 ******************************************************************************/

    qtcNode ** sTarget  = NULL;
    qtcNode  * sPrev    = NULL;
    qtcNode  * sNewPred = NULL;

    IDU_FIT_POINT_FATAL( "qmoListTransform::listTransform::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // 노드 순회하면서 변환
    //--------------------------------------

    if ( *aNode != NULL )
    {
        if ( ( ( (*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_OR ) ||
             ( ( (*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_AND ) )
        {
            // Logical operator 이면 argument 의 next 순회
            for ( sPrev = NULL, sTarget = (qtcNode **)&((*aNode)->node.arguments);
                  *sTarget != NULL;
                  sPrev = *sTarget, sTarget = (qtcNode **)&((*sTarget)->node.next) )
            {
                IDE_TEST( listTransform( aStatement, sTarget ) != IDE_SUCCESS );

                // Target 이 변경되었을 수 있으므로 연결관계 복구
                if ( sPrev == NULL )
                {
                    (*aNode)->node.arguments = (mtcNode *)(*sTarget);
                }
                else
                {
                    sPrev->node.next = (mtcNode *)(*sTarget);
                }
            }
        }
        else
        {
            // Logical operator 아니면 변환
            sTarget = aNode;

            IDE_TEST( makePredicateList( aStatement,
                                         *sTarget,
                                         &sNewPred )
                      != IDE_SUCCESS );

            // 연결관계 복구
            if ( sNewPred != NULL )
            {
                *aNode = sNewPred;
            }
            else
            {
                // Nothing to do.
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

IDE_RC qmoListTransform::makePredicateList( qcStatement  * aStatement,
                                            qtcNode      * aCompareNode,
                                            qtcNode     ** aResult )
{
/***********************************************************************
 *
 * Description : Compare predicate 으로부터 predicate list 를 생성한다.
 *
 * Implementation :
 *
 * ex) (t1.i1, t1.i2) = (t2.i1, t2.i2) => t1.i1 = t2.i1 AND t1.i2 = t2.i2
 *
 *     [Before] =(sCompare)
 *              |
 *              LIST(sLeftList) ------ LIST (sRightList)
 *              |                      |
 *              t1.i1(sLeftArg) - ...  t2.i1(sRightArg) - ...
 *
 *     [After]  AND
 *              |
 *              = ------------- = ------------- ...
 *              |               |
 *              t1.i1 - t2.i1   ...
 *
 ***********************************************************************/

    qtcNode * sLogicalNode[2];
    qtcNode * sFirst    = NULL;
    qtcNode * sLast     = NULL;
    qtcNode * sNewNode  = NULL;
    mtcNode * sLeftArg  = NULL;
    mtcNode * sRightArg = NULL;
    mtcNode * sLeftArgNext  = NULL;
    mtcNode * sRightArgNext = NULL;
    mtcNode * sCompareNext  = NULL;
    idBool    sResult = ID_FALSE;
    UInt      sCount = 0;
    qcNamePosition   sEmptyPosition;

    IDU_FIT_POINT_FATAL( "qmoListTransform::makePredicateList::__FT__" );

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aCompareNode != NULL );

    IDE_TEST( checkCondition( aCompareNode,
                              &sResult )
              != IDE_SUCCESS );

    if ( sResult == ID_TRUE )
    {
        sCompareNext = aCompareNode->node.next;

        // Predicate list 생성
        for ( sLeftArg  = aCompareNode->node.arguments->arguments,
              sRightArg = aCompareNode->node.arguments->next->arguments;
              ( sLeftArg != NULL ) && ( sRightArg != NULL );
              sLeftArg = sLeftArgNext, sRightArg = sRightArgNext, sCount++ )
        {
            sLeftArgNext  = sLeftArg->next;
            sRightArgNext = sRightArg->next;

            // Predicate 생성
            IDE_TEST( makePredicate( aStatement,
                                     aCompareNode,
                                     (qtcNode*)sLeftArg,
                                     (qtcNode*)sRightArg,
                                     &sNewNode )
                      != IDE_SUCCESS );

            // 생성된 predicate을 연결한다.
            if ( sFirst == NULL )
            {
                sFirst = sLast = sNewNode;
            }
            else
            {
                sLast->node.next = (mtcNode *)sNewNode;
                sLast = (qtcNode*)sLast->node.next;
            }
        }

        // Logical operator 생성
        SET_EMPTY_POSITION( sEmptyPosition );

        IDE_TEST( qtc::makeNode( aStatement,
                                 sLogicalNode,
                                 &sEmptyPosition,
                                 ( aCompareNode->node.module == &mtfEqual ) ? &mtfAnd: &mtfOr )
                  != IDE_SUCCESS );

        sLogicalNode[0]->node.arguments = (mtcNode *)sFirst;

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sLogicalNode[0] )
                  != IDE_SUCCESS );

        sLogicalNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sLogicalNode[0]->node.lflag |= sCount;
        sLogicalNode[0]->node.next = sCompareNext;

        *aResult = sLogicalNode[0];
    }
    else
    {
        *aResult = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoListTransform::checkCondition( qtcNode     * aNode,
                                         idBool      * aResult )
{
/******************************************************************************
 *
 * Description : 변환 가능한 조건에 대한 결과를 반환한다.
 *
 * Implementation : 변환 가능한 조건은 다음과 같다.
 *
 *             - ORACLE style outer mask 존재하지 않아야 함
 *             - Subquery 를 포함하지 않아야 함
 *             - [NOT] EQUAL 연산자
 *             - 인자는 모두 LIST 연산자
 *             - Predicate dependency 가 QMO_LIST_TRANSFORM_DEPENDENCY_COUNT 이상
 *             - LIST 인자의 갯수는 QMO_LIST_TRANSFORM_ARGUMENTS_COUNT 이하일 경우
 *
 *             ex) (t1.i1, t1.i2) = (t2.i1, t2.i2)
 *                 (t1.i1, t2.i2) = (t3.i1, t3.i2)
 *                 (t1.i1, t2.i1) = (1, 1)
 *
 *****************************************************************************/

    qtcNode * sListNode   = NULL;
    qtcNode * sNode       = NULL;
    UInt      sArgCnt     = 0;
    idBool    sExistOuter = ID_FALSE;
    idBool    sIsResult   = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoListTransform::checkCondition::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode      != NULL );

    //--------------------------------------
    // 조건 검사
    //--------------------------------------

    IDE_TEST( qmoCSETransform::doCheckOuter( aNode,
                                             & sExistOuter )
              != IDE_SUCCESS );

    if ( ( sExistOuter == ID_FALSE ) &&
         ( QTC_HAVE_SUBQUERY( aNode ) == ID_FALSE ) &&
         ( ( aNode->node.module == &mtfEqual ) || ( aNode->node.module == &mtfNotEqual ) ) &&
         ( aNode->node.arguments->module == &mtfList ) &&
         ( aNode->node.arguments->next->module == &mtfList ) &&
         ( aNode->depInfo.depCount >= QMO_LIST_TRANSFORM_DEPENDENCY_COUNT ) )
    {
        sListNode = (qtcNode*)aNode->node.arguments;

        for ( sNode = (qtcNode*)sListNode->node.arguments, sArgCnt = 0;
              sNode != NULL;
              sNode = (qtcNode*)sNode->node.next, sArgCnt++ );

        if ( sArgCnt > QMO_LIST_TRANSFORM_ARGUMENTS_COUNT )
        {
            sIsResult = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sIsResult = ID_FALSE;
    }

    *aResult = sIsResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoListTransform::makePredicate( qcStatement  * aStatement,
                                        qtcNode      * aPredicate,
                                        qtcNode      * aOperand1,
                                        qtcNode      * aOperand2,
                                        qtcNode     ** aResult )
{
/***********************************************************************
 *
 * Description : 하나의 predicate 을 생성한다.
 *
 * Implementation :
 *
 *                =
 *                |
 *                t1.i1 - t2.i1
 *
 ***********************************************************************/

    qtcNode        * sPredicate[2];
    qcNamePosition   sEmptyPosition;

    IDU_FIT_POINT_FATAL( "qmoListTransform::makePredicate::__FT__" );

    aOperand1->node.next = (mtcNode *)aOperand2;
    aOperand2->node.next = NULL;

    SET_EMPTY_POSITION( sEmptyPosition );

    IDE_TEST( qtc::makeNode( aStatement,
                             sPredicate,
                             &sEmptyPosition,
                             (mtfModule*)aPredicate->node.module )
              != IDE_SUCCESS );

    sPredicate[0]->node.arguments = (mtcNode *)aOperand1;
    sPredicate[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
    sPredicate[0]->node.lflag |= 2;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sPredicate[0] )
              != IDE_SUCCESS );

    *aResult = sPredicate[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
