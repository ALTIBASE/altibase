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
 * $Id: qmoNormalForm.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Normal Form Manager
 *
 *     비정규화된 Predicate들을 정규화된 형태로 변경시키는 역활을 한다.
 *     다음과 같은 정규화를 수행한다.
 *         - CNF (Conjunctive Normal Form)
 *         - DNF (Disjunctive Normal Form)
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qmoNormalForm.h>
#include <qmoCSETransform.h>

extern mtfModule mtfAnd;
extern mtfModule mtfOr;
extern mtfModule mtfNot;

IDE_RC
qmoNormalForm::normalizeCheckCNFOnly( qtcNode  * aNode,
                                      idBool   * aCNFonly )
{
/***********************************************************************
 *
 * Description : CNF 로만 변경되는 경우를 판단한다.
 *
 * Implementation :
 *
 *    CNF로만 변경되는 경우는 다음과 같다.
 *
 *    1. SFWGH에 subquery가 있는 경우
 *       normalization 단계에서 node의 복사시
 *       subquery는 복사하지 않는다.
 *       따라서, DNF로 처리시 동일한 subquery의 dependency 설정이
 *       잘못되어 결과 오류가 발생한다.(참조 BUG-6365)
 *       예) where (t1.i1 >= t2.i1 or t1.i2 >= t2.i2 )
 *           and 1 = ( select count(*) from t3 where t1.i1 != t2.i2 );
 *
 *    2. SFWGH에 AND연산만 있는 경우
 *       예) i1=1 and i2=1 and i3=1
 *       예외: i1=1 and ( i2=1 and i3=1 ) and i4=1
 *       [ AND 연산만 있는 경우라 하더라도 괄호로 묶여 있는 경우는
 *         CNF only 로 판단하지 않는다. ]
 *
 *    3. SFWGH에 하나의 predicate만 있는 경우
 *       예) i1=1
 *
 ***********************************************************************/

    qtcNode * sNode;
    qtcNode * sNodeTraverse;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::normalizeCheckCNFOnly::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode    != NULL );
    IDE_DASSERT( aCNFonly != NULL );

    sNode = aNode;

    //--------------------------------------
    // CNF only에 대한 조건 검사
    // 인자로 넘어온 qtcNode의 최상위 노드에 대해서 조건검사를 수행한다.
    //--------------------------------------

    if( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK )
        == QTC_NODE_SUBQUERY_EXIST )
    {
        // 1. SFWGH에 subquery가 있는 경우
        //    ( 최상위 노드에서 subquery 존재 유무 검사 )
        *aCNFonly = ID_TRUE;
    }
    else if ((sNode->lflag & QTC_NODE_COLUMN_RID_MASK) ==
             QTC_NODE_COLUMN_RID_EXIST)
    {
        *aCNFonly = ID_TRUE;
    }
    else if( ( sNode->node.lflag &
             ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
             == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        // 2. SFWGH에 AND 연산만 있는 경우.
        //    최상위 노드가 AND이고, AND 하위 노드에는 비교연산자만 존재
        //    하는지를 검사. ( 즉, 논리연산자가 존재하면, CNFonly가 아님. )
        *aCNFonly = ID_TRUE;

        for( sNodeTraverse = (qtcNode *)(sNode->node.arguments);
             sNodeTraverse != NULL;
             sNodeTraverse = (qtcNode *)(sNodeTraverse->node.next) )
        {
            if( ( sNodeTraverse->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                // AND 하위 노드에 논리연산자가 존재하면, CNFonly가 아님.
                *aCNFonly = ID_FALSE;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else if( ( sNode->node.lflag & MTC_NODE_COMPARISON_MASK )
               == MTC_NODE_COMPARISON_TRUE )
    {
        // 3. SFWGH에 하나의 predicate만 있는 경우
        //    ( 최상위 노드가 비교연산자인지를 검사 )
        *aCNFonly = ID_TRUE;
    }
    else
    {
        *aCNFonly = ID_FALSE;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoNormalForm::normalizeDNF( qcStatement   * aStatement,
                             qtcNode       * aNode,
                             qtcNode      ** aDNF )
{
/***********************************************************************
 *
 * Description : DNF 로 정규화
 *     DNF는 작성된 predicate을 논리연산자를 기준으로 (OR of AND's)의
 *     형태로 표현하는 방법이다.
 *
 * Implementation :
 *     DNF normalization은 where절만 처리한다.
 *     1. DNF 정규화형태로 변환
 *     2. DNF form 논리연산자의 flag와 dependency 재설정
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoNormalForm::normalizeDNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );
    IDE_DASSERT( aDNF       != NULL );

    //--------------------------------------
    // DNF normal form으로 변환
    //--------------------------------------

    IDE_TEST( makeDNF( aStatement, aNode, aDNF ) != IDE_SUCCESS );

    //--------------------------------------
    // PROJ-2242 : CSE transformation
    //--------------------------------------

    IDE_TEST( qmoCSETransform::doTransform( aStatement, aDNF, ID_FALSE )
              != IDE_SUCCESS );

    //--------------------------------------
    // DNF form 논리연산자의 flag와 dependencies 재설정
    //--------------------------------------

    IDE_TEST( setFlagAndDependencies( *aDNF ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoNormalForm::normalizeCNF( qcStatement   * aStatement,
                             qtcNode       * aNode,
                             qtcNode      ** aCNF,
                             idBool          aIsWhere,
                             qmsHints      * aHints )
{
/***********************************************************************
 *
 * Description : CNF 로 정규화
 *     CNF는 작성된 predicate을 논리 연산자를 기준으로 (AND of OR's)의
 *     형태로 표현하는 방법이다.
 *
 * Implementation :
 *     DNF와 달리 다양한 구문의 조건절을 처리한다.
 *     1. CNF 정규화 형태로 변환
 *     2. CNF form 논리연산자의 flag와 dependency를 재설정
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoNormalForm::normalizeCNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );
    IDE_DASSERT( aCNF       != NULL );

    //--------------------------------------
    // CNF normal form으로 변환
    //--------------------------------------

    IDE_TEST( makeCNF( aStatement, aNode, aCNF ) != IDE_SUCCESS );

    //--------------------------------------
    // PROJ-2242 : CSE transformation
    //--------------------------------------

    IDE_TEST( qmoCSETransform::doTransform( aStatement,
                                            aCNF,
                                            ID_FALSE,
                                            aIsWhere,
                                            aHints )
              != IDE_SUCCESS );

    //--------------------------------------
    // CNF form 논리연산자의 flag와 dependencies 재설정
    //--------------------------------------

    IDE_TEST( setFlagAndDependencies( *aCNF ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoNormalForm::estimateDNF( qtcNode  * aNode,
                            UInt     * aCount )
{
/***********************************************************************
 *
 * Description : DNF로 정규화되었을때 만들어지는
 *               비교연산자 노드의 개수 예측.
 *     예) where i1=1 and (i2=1 or i3=1 or (i4=1 and i5=1))
 *         DNF 노드 개수 = 2 * ( 2 + ( (1*1) + (1*1) ) ) = 8
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode           * sNNFArg;
    UInt                sNNFCount;
    SDouble             sCount;
    UInt                sChildCount;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::estimateDNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode  != NULL );
    IDE_DASSERT( aCount != NULL );

    //--------------------------------------
    // DNF로 치환한 경우의 Node개수 예측
    //--------------------------------------

    if ( ( aNode->node.lflag &
            ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        sCount = 1;
        sNNFCount = 0;

        sNNFArg = (qtcNode *)(aNode->node.arguments);
        while( sNNFArg )
        {
            sNNFCount++;

            estimateDNF( sNNFArg, & sChildCount );
            if( sNNFCount != 0 && sCount >= (UINT_MAX/2) )
            {
                *aCount = UINT_MAX;

                return IDE_SUCCESS;
            }
            sCount *= sChildCount;
            sNNFArg = (qtcNode *)(sNNFArg->node.next);
        }
        sCount *= sNNFCount;
    }
    else if ( ( aNode->node.lflag &
            ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        sCount = 0;
        sNNFArg = (qtcNode *)(aNode->node.arguments);
        while( sNNFArg )
        {
            estimateDNF( sNNFArg, & sNNFCount );
            if( sNNFCount != 0 && sCount >= (UINT_MAX/2) )
            {
                *aCount = UINT_MAX;

                return IDE_SUCCESS;
            }
            sCount += sNNFCount;
            sNNFArg = (qtcNode *)(sNNFArg->node.next);
        }
    }
    else
    {
        sCount = 1;
    }

    // To fix BUG-14846
    // UInt overflow방지
    if( sCount >= UINT_MAX )
    {
        *aCount = UINT_MAX;
    }
    else
    {
        *aCount = (UInt)sCount;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoNormalForm::estimateCNF( qtcNode  * aNode,
                            UInt     * aCount )
{
/***********************************************************************
 *
 * Description : CNF로 정규화되었을때 만들어지는
 *               비교연산자 노드의 개수 예측
 *     예) where i1=1 and (i2=1 or i3=1 or (i4=1 and i5=1))
 *         CNF 노드 개수 = 1 + ( 3 * ( 1 + 1 ) ) = 7
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode           * sNNFArg;
    UInt                sNNFCount;
    SDouble             sCount;
    UInt                sChildCount;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::estimateCNF::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode  != NULL );
    IDE_DASSERT( aCount != NULL );

    //--------------------------------------
    // CNF로 치환한 경우의 Node개수 예측
    //--------------------------------------

    if ( ( aNode->node.lflag &
            ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        sCount = 1;
        sNNFCount = 0;

        sNNFArg = (qtcNode *)(aNode->node.arguments);
        while( sNNFArg )
        {
            sNNFCount++;

            estimateCNF( sNNFArg, & sChildCount );
            if( sNNFCount != 0 && sCount >= (UINT_MAX/2) )
            {
                *aCount = UINT_MAX;

                return IDE_SUCCESS;
            }
            sCount *= sChildCount;
            sNNFArg = (qtcNode *)(sNNFArg->node.next);
        }
        sCount *= sNNFCount;
    }
    else if ( ( aNode->node.lflag &
            ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        sCount = 0;
        sNNFArg = (qtcNode *)(aNode->node.arguments);
        while( sNNFArg )
        {
            estimateCNF( sNNFArg, & sNNFCount );
            if( sNNFCount != 0 && sCount >= (UINT_MAX/2) )
            {
                *aCount = UINT_MAX;

                return IDE_SUCCESS;
            }
            sCount += sNNFCount;
            sNNFArg = (qtcNode *)(sNNFArg->node.next);
        }
    }
    else
    {
        sCount = 1;
    }

    // To fix BUG-14846
    // UInt overflow방지
    if( sCount >= UINT_MAX )
    {
        *aCount = UINT_MAX;
    }
    else
    {
        *aCount = (UInt)sCount;
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoNormalForm::makeDNF( qcStatement  * aStatement,
                        qtcNode      * aNode,
                        qtcNode     ** aDNF)
{
/***********************************************************************
 *
 * Description : DNF 형태로 predicate 변환
 *
 * Implementation :
 *     1. OR  하위 노드는 addToMerge
 *     1. AND 하위 노드는 productToMerge
 *     2. 비교연산자 노드
 *        (1) OR 노드 생성
 *        (2) AND 노드 생성
 *        (3) 새로운 노드에 predicate 복사
 *        (4) OR->AND->predicate 순서로 연결
 *
 ***********************************************************************/

    qtcNode           * sNode;
    qtcNode           * sTmpNode[2];
    qtcNode           * sDNFNode;
    qtcNode           * sPrevDNF = NULL;
    qtcNode           * sCurrDNF = NULL;
    qtcNode           * sNNFArg = NULL;
    qcNamePosition      sNullPosition;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::makeDNF::__FT__" );

    IDE_FT_ASSERT( aNode != NULL );

    if ( ( aNode->node.lflag &
            ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        IDE_FT_ASSERT( aNode->node.arguments != NULL );

        sNNFArg = (qtcNode *)(aNode->node.arguments);
        IDE_TEST(makeDNF(aStatement, sNNFArg, &sPrevDNF) != IDE_SUCCESS);
        sDNFNode = sPrevDNF;

        sNNFArg = (qtcNode *)(sNNFArg->node.next);

        while (sNNFArg != NULL)
        {
            IDE_TEST(makeDNF(aStatement, sNNFArg, &sCurrDNF) != IDE_SUCCESS);

            IDE_TEST(productToMerge(aStatement, sPrevDNF, sCurrDNF, &sDNFNode)
                     != IDE_SUCCESS);

            sPrevDNF = sDNFNode;
            sNNFArg  = (qtcNode *)(sNNFArg->node.next);
        }
    }
    else if ( ( aNode->node.lflag &
            ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        IDE_FT_ASSERT( aNode->node.arguments != NULL );

        sNNFArg = (qtcNode *)(aNode->node.arguments);
        IDE_TEST(makeDNF(aStatement, sNNFArg, &sPrevDNF) != IDE_SUCCESS);
        sDNFNode = sPrevDNF;

        sNNFArg = (qtcNode *)(sNNFArg->node.next);
        while (sNNFArg != NULL)
        {
            IDE_TEST(makeDNF(aStatement, sNNFArg, &sCurrDNF) != IDE_SUCCESS);

            IDE_TEST(addToMerge(sPrevDNF, sCurrDNF, &sDNFNode)
                     != IDE_SUCCESS);

            sPrevDNF = sDNFNode;
            sNNFArg  = (qtcNode *)(sNNFArg->node.next);
        }
    }
    else
    { // terminal predicate node

        SET_EMPTY_POSITION(sNullPosition);

        IDE_TEST( qtc::makeNode( aStatement,
                                 sTmpNode,
                                 & sNullPosition,
                                 (const UChar*)"OR",
                                 2 )
                  != IDE_SUCCESS );
        sDNFNode = sTmpNode[0];

        sDNFNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sDNFNode->node.lflag |= 1;

        IDE_TEST( qtc::makeNode( aStatement,
                                 sTmpNode,
                                 & sNullPosition,
                                 (const UChar*)"AND",
                                 3 )
                  != IDE_SUCCESS );
        sNode = sTmpNode[0];

        sNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sNode->node.lflag |= 1;

        sDNFNode->node.arguments = (mtcNode *)sNode;

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNode)
                 != IDE_SUCCESS);

        idlOS::memcpy(sNode, aNode, ID_SIZEOF(qtcNode));
        sNode->node.next = NULL;

        sDNFNode->node.arguments->arguments = (mtcNode *)sNode;

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    (qtcNode *)( sDNFNode->node.arguments ) )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sDNFNode )
                  != IDE_SUCCESS );
    }

    *aDNF = sDNFNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoNormalForm::makeCNF( qcStatement  * aStatement,
                        qtcNode      * aNode,
                        qtcNode     ** aCNF )
{
/***********************************************************************
 *
 * Description : CNF 형태로 predicate 변환
 *
 *     BUG-35155 Partial CNF
 *     mtcNode 에 MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE flag 가
 *     설정되어 있으면 normalize 하지 않는다.
 *
 * Implementation :
 *     1. AND  하위 노드는 addToMerge
 *     1. OR   하위 노드는 productToMerge
 *     2. 비교연산자 노드
 *        (1) AND 노드 생성
 *        (2) OR 노드 생성
 *        (3) 새로운 노드에 predicate 복사
 *        (4) AND->OR->predicate 순서로 연결
 *
 ***********************************************************************/

    qtcNode           * sNode;
    qtcNode           * sCNFNode;
    qtcNode           * sTmpNode[2];
    qtcNode           * sPrevCNF = NULL;
    qtcNode           * sCurrCNF = NULL;
    qtcNode           * sNNFArg = NULL;
    qcNamePosition      sNullPosition;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::makeCNF::__FT__" );

    if ( ( aNode->node.lflag & MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK )
           != MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE )
    {
        if ( ( aNode->node.lflag &
                ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
                == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
        {
            /* TASK-7219 Non-shard DML */
             if ( ( aNode->node.lflag & MTC_NODE_PUSHED_PRED_FORCE_MASK )
                   == MTC_NODE_PUSHED_PRED_FORCE_TRUE )
            {
                aNode->node.arguments->lflag &= ~MTC_NODE_PUSHED_PRED_FORCE_MASK;
                aNode->node.arguments->lflag |= MTC_NODE_PUSHED_PRED_FORCE_TRUE;
            }

            sNNFArg = (qtcNode *)(aNode->node.arguments);
            IDE_TEST(makeCNF(aStatement, sNNFArg, &sPrevCNF) != IDE_SUCCESS);
            sCNFNode = sPrevCNF;

            sNNFArg = (qtcNode *)(sNNFArg->node.next);

            while (sNNFArg != NULL)
            {
                IDE_TEST( makeCNF( aStatement,
                                   sNNFArg,
                                   & sCurrCNF )
                          != IDE_SUCCESS );

                IDE_TEST(addToMerge2(sPrevCNF, sCurrCNF, &sCNFNode)
                            != IDE_SUCCESS);

                sPrevCNF = sCNFNode;
                sNNFArg  = (qtcNode *)(sNNFArg->node.next);
            }
        }
        else if ( ( aNode->node.lflag &
                ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
                == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
        {
            /* TASK-7219 Non-shard DML */
            if ( ( aNode->node.lflag & MTC_NODE_PUSHED_PRED_FORCE_MASK )
                   == MTC_NODE_PUSHED_PRED_FORCE_TRUE )
            {
                aNode->node.arguments->lflag &= ~MTC_NODE_PUSHED_PRED_FORCE_MASK;
                aNode->node.arguments->lflag |= MTC_NODE_PUSHED_PRED_FORCE_TRUE;
            }

            sNNFArg = (qtcNode *)(aNode->node.arguments);
            IDE_TEST(makeCNF(aStatement, sNNFArg, &sPrevCNF) != IDE_SUCCESS);
            sCNFNode = sPrevCNF;

            sNNFArg = (qtcNode *)(sNNFArg->node.next);

            while (sNNFArg != NULL)
            {
                IDE_TEST( makeCNF( aStatement,
                                   sNNFArg,
                                   & sCurrCNF )
                          != IDE_SUCCESS );

                IDE_TEST( productToMerge( aStatement,
                                          sPrevCNF,
                                          sCurrCNF,
                                          & sCNFNode )
                          != IDE_SUCCESS );

                sPrevCNF = sCNFNode;
                sNNFArg  = (qtcNode *)(sNNFArg->node.next);
            }
        }
        else
        { // terminal predicate node
            SET_EMPTY_POSITION(sNullPosition);

            IDE_TEST( qtc::makeNode( aStatement,
                                     sTmpNode,
                                     & sNullPosition,
                                     (const UChar*)"AND",
                                     3 )
                      != IDE_SUCCESS );
            sCNFNode = sTmpNode[0];

            sCNFNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
            sCNFNode->node.lflag |= 1;

            IDE_TEST( qtc::makeNode( aStatement,
                                     sTmpNode,
                                     & sNullPosition,
                                     (const UChar*)"OR",
                                     2 )
                      != IDE_SUCCESS );
            sNode = sTmpNode[0];

            sNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
            sNode->node.lflag |= 1;

            /* TASK-7219 Non-shard DML */
            if ( ( aNode->node.lflag & MTC_NODE_PUSHED_PRED_FORCE_MASK )
                   == MTC_NODE_PUSHED_PRED_FORCE_TRUE )
            {
                sNode->node.lflag &= ~MTC_NODE_PUSHED_PRED_FORCE_MASK;
                sNode->node.lflag |= MTC_NODE_PUSHED_PRED_FORCE_TRUE;
            }

            sCNFNode->node.arguments = (mtcNode *)sNode;

            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNode)
                     != IDE_SUCCESS);

            idlOS::memcpy(sNode, aNode, ID_SIZEOF(qtcNode));
            sNode->node.next = NULL;

            sCNFNode->node.arguments->arguments = (mtcNode *)sNode;

            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        (qtcNode *)( sCNFNode->node.arguments ) )
                      != IDE_SUCCESS );

            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        sCNFNode )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // CNF UNUSABLE 이면 CNF 로 정규화하지 않는다.
        sCNFNode = NULL;
    }
    *aCNF = sCNFNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoNormalForm::addToMerge( qtcNode     * aPrevNF,
                           qtcNode     * aCurrNF,
                           qtcNode    ** aNFNode)
{
/***********************************************************************
 *
 * Description : 정규화 형태로 변환하는 과정에서 predicate을 연결.
 *
 * Implementation :
 *     1. DNF
 *         OR의 하위 노드 처리 후, 각 AND 노드를 연결한다.
 *     2. CNF
 *         AND의 하위 노드 처리 후, 각 OR 노드를 연결한다.
 *
 ***********************************************************************/

    qtcNode     * sNode;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::addToMerge::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aPrevNF != NULL );
    IDE_DASSERT( aCurrNF != NULL );
    IDE_DASSERT( aNFNode != NULL );

    //--------------------------------------
    // add to merge
    //--------------------------------------

    sNode = (qtcNode *)(aPrevNF->node.arguments);
    while (sNode->node.next != NULL)
    {
        sNode = (qtcNode *)(sNode->node.next);
    }
    sNode->node.next = aCurrNF->node.arguments;

    *aNFNode = aPrevNF;

    return IDE_SUCCESS;
}

IDE_RC
qmoNormalForm::productToMerge( qcStatement * aStatement,
                               qtcNode     * aPrevNF,
                               qtcNode     * aCurrNF,
                               qtcNode    ** aNFNode)
{
/***********************************************************************
 *
 * Description : 정규화 형태로 변환하는 과정에서
 *               predicate에 대한 배분법칙수행
 *
 * Implementation :
 *     1. DNF
 *         AND 하위 노드 처리후, 배분법칙 수행
 *     2. CNF
 *         OR  하위 노드 처리후, 배분법칙 수행
 *
 ***********************************************************************/

    qtcNode     * sNode;
    qtcNode     * sArg;
    qtcNode     * sLastNode;
    qtcNode     * sLastArg;
    qtcNode     * sNewNode;
    qtcNode     * sNewArg;
    qtcNode     * sPrevNewArg = NULL;
    SInt          sPrevCnt;
    SInt          sCurrCnt;
    SInt          i;
    SInt          j;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::productToMerge::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPrevNF    != NULL );
    IDE_DASSERT( aCurrNF    != NULL );
    IDE_DASSERT( aNFNode    != NULL );

    //--------------------------------------
    // product to merge
    //--------------------------------------

    for (sPrevCnt = 0,
             sNode = (qtcNode *)(aPrevNF->node.arguments);
         sNode != NULL;
         sNode = (qtcNode *)(sNode->node.next))
    {
        sPrevCnt++;
    }

    for (sCurrCnt = 0,
             sNode = (qtcNode *)(aCurrNF->node.arguments);
         sNode != NULL;
         sNode = (qtcNode *)(sNode->node.next))
    {
        sCurrCnt++;
    }

    // memory alloc for saving product result
    sLastNode = (qtcNode *)(aPrevNF->node.arguments);
    for (i = sCurrCnt - 1; i > 0; i--)
    {
        while(sLastNode->node.next != NULL)
        {
            sLastNode = (qtcNode *)(sLastNode->node.next);
        }

        for (j = 0,
                 sNode = (qtcNode *)(aPrevNF->node.arguments);
             j < sPrevCnt;
             j++,
                 sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNewNode)
                     != IDE_SUCCESS);

            idlOS::memcpy(sNewNode, sNode, ID_SIZEOF(qtcNode));
            sNewNode->node.arguments = NULL;
            sNewNode->node.next      = NULL;

            for (sArg = (qtcNode *)(sNode->node.arguments);
                 sArg != NULL;
                 sArg = (qtcNode *)(sArg->node.next))
            {
                IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNewArg)
                         != IDE_SUCCESS);

                idlOS::memcpy(sNewArg, sArg, ID_SIZEOF(qtcNode));
                sNewArg->node.next = NULL;

                // link
                if (sNewNode->node.arguments == NULL)
                {
                    sNewNode->node.arguments = (mtcNode *)sNewArg;
                    sPrevNewArg                = sNewArg;
                }
                else
                {
                    sPrevNewArg->node.next = (mtcNode *)sNewArg;
                    sPrevNewArg            = sNewArg;
                }
            }

            // link
            sLastNode->node.next = (mtcNode *)sNewNode;
            sLastNode            = sNewNode;
        }
    }

    // product
    sLastNode = (qtcNode *)(aPrevNF->node.arguments);
    for (i = 0,
             sNode = (qtcNode *)(aCurrNF->node.arguments);
         i < sCurrCnt;
         i++,
             sNode = (qtcNode *)(sNode->node.next))
    {
        for (j = 0;
             j < sPrevCnt;
             j++,
                 sLastNode = (qtcNode *)(sLastNode->node.next))
        {
            sLastArg = (qtcNode *)(sLastNode->node.arguments);
            while (sLastArg->node.next != NULL)
            {
                sLastArg = (qtcNode *)(sLastArg->node.next);
            }

            for (sArg = (qtcNode *)(sNode->node.arguments);
                 sArg != NULL;
                 sArg = (qtcNode *)(sArg->node.next))
            {
                IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNewArg)
                         != IDE_SUCCESS);

                idlOS::memcpy(sNewArg, sArg, ID_SIZEOF(qtcNode));
                sNewArg->node.next = NULL;

                // link
                sLastArg->node.next = (mtcNode *)sNewArg;
                sLastArg            = sNewArg;
            }
        }
    }

    // return product result
    *aNFNode = aPrevNF;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoNormalForm::setFlagAndDependencies(qtcNode * aNode)
{
/***********************************************************************
 *
 * Description : 정규화 형태로 변환하면서 생긴 새로운 논리연산자 노드에
 *               대한 flag와 dependency 설정
 *
 * Implementation :
 *     정규화 형태로 변환하는 과정에서 논리 연산자들을 모두 새로
 *     만들어서 연결하게 된다. 따라서, 논리 연산자 노드에 대해서는
 *     parsing & validation과정에서 설정된 flag와 dependency정보가
 *     모두 사라지게 되므로, 이에 대한 설정을 해 주어야 된다.
 *
 *     하위 노드의 flag와 dependency 정보를 merge 한다.
 *     [하위 노드의 flag를 merge해서 얻는 정보는 QTC_NODE_MASK 참조]
 *
 ***********************************************************************/

    qtcNode     * sNode;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::setFlagAndDependencies::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // Flag과 Dependency 결정
    //--------------------------------------

    if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        qtc::dependencyClear( & aNode->depInfo );

        for (sNode = (qtcNode *)(aNode->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST( setFlagAndDependencies(sNode) != IDE_SUCCESS );

            aNode->node.lflag |=
                sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
            aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

            IDE_TEST( qtc::dependencyOr( & aNode->depInfo,
                                         & sNode->depInfo,
                                         & aNode->depInfo )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoNormalForm::optimizeForm( qtcNode  * aInputNode,
                             qtcNode ** aOutputNode )
{
/***********************************************************************
 *
 * Description : 정규화 형태로 변환된 predicate을 최적화 한다.
 *
 * Implementation :
 *     CNF로 변환된 predicate이 Filter로 처리되기 직전에
 *     불필요한  AND, OR를 제거한다.
 *
 ***********************************************************************/

    qtcNode * sNode;
    qtcNode * sNewNode;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::optimizeForm::__FT__" );

    //-------------------------------------
    // 적합성 검사
    //-------------------------------------

    IDE_DASSERT( aInputNode != NULL );

    if( ( aInputNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        switch( aInputNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        {
            case MTC_NODE_OPERATOR_AND:
            case MTC_NODE_OPERATOR_OR:
                // AND나 OR 노드의 argument가 하나일 경우
                // 그 AND 또는 OR 노드는 제거될 수 있음.
                IDE_FT_ASSERT( aInputNode->node.arguments != NULL );

                if( aInputNode->node.arguments->next == NULL )
                {
                    IDE_TEST( optimizeForm( (qtcNode*) aInputNode->node.arguments,
                                            aOutputNode )
                              != IDE_SUCCESS );
                }
                else
                {
                    sNewNode = NULL;

                    for (sNode  = (qtcNode *)(aInputNode->node.arguments);
                         sNode != NULL;
                         sNode  = (qtcNode *)(sNode->node.next))
                    {
                        if( sNewNode == NULL )
                        {
                            IDE_TEST( optimizeForm( sNode,
                                                    &sNewNode )
                                      != IDE_SUCCESS );

                            aInputNode->node.arguments = (mtcNode*)sNewNode;
                        }
                        else
                        {
                            IDE_TEST( optimizeForm( sNode,
                                                    (qtcNode**)& sNewNode->node.next )
                                      != IDE_SUCCESS );

                            sNewNode = (qtcNode*)sNewNode->node.next;
                        }
                    }
                    *aOutputNode = aInputNode;
                }
                break;
            case MTC_NODE_OPERATOR_NOT:
                // NOT 노드는 제거되어서는 안되며 하위 노드에 대해
                // optimizeForm이 가능하다.
                IDE_TEST( optimizeForm( (qtcNode*)aInputNode->node.arguments,
                                        & sNewNode )
                          != IDE_SUCCESS );
                aInputNode->node.arguments = (mtcNode*)sNewNode;

                *aOutputNode = aInputNode;
                break;
            default:
                IDE_FT_ASSERT( 0 );
                break;
        }
    }
    else
    {
        *aOutputNode = aInputNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoNormalForm::estimatePartialCNF( qtcNode  * aNode,
                                   UInt     * aCount,
                                   qtcNode  * aRoot,
                                   UInt       aNFMaximum )
{
/***********************************************************************
 *
 * Description :
 *         BUG-35155 Partial CNF
 *         CNF로 정규화되었을때 만들어지는 비교연산자 노드의 개수 예측
 *         만약 aNFMaximum 을 넘을 경우 해당 노드의 최상위 OR 노드(aRoot)를
 *         partial normalize 에서 제외하기 위해 CNF_UNUSABLE flag 를 세팅한다.
 *    예1) where i1=1 and (i2=1 or i3=1 or (i4=1 and i5=1))
 *         CNF 노드 개수 = 1 + ( 3 * ( 1 + 1 ) ) = 7
 *    예2) where i1=1 and (i2=1 or i3=1 or ((i4=1 and (i5=1 or (i6=1 and ...))
 *         CNF 노드 개수 = 1 + ( 3 * (1) ) = 6  (일부 노드가 CNF 에서 제외됨)
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode           * sNNFArg;
    UInt                sNNFCount;
    SDouble             sCount;
    UInt                sChildCount;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode  != NULL );
    IDE_DASSERT( aCount != NULL );

    //--------------------------------------
    // CNF로 치환한 경우의 Node개수 예측
    //--------------------------------------
    if ( ( aNode->node.lflag &
           ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
           == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        // Check aRoot
        if ( aRoot != NULL )
        {
            // aRoot is OR node
            // Nothing to do.
        }
        else
        {
            // Change aRoot to first OR node. (this node)
            aRoot = aNode;
        }

        sCount = 1;
        sNNFCount = 0;

        sNNFArg = (qtcNode *)(aNode->node.arguments);
        while( sNNFArg )
        {
            sNNFCount++;

            estimatePartialCNF( sNNFArg, & sChildCount, aRoot, aNFMaximum );
            sCount *= sChildCount;

            if( sCount*sNNFCount > aNFMaximum )
            {
                // 이 노드와 하위노드의 count 를 계산한 값이 NORMALFORM_MAXIMUM 을 넘어섰다.
                // 현재 노드 기준으로 최상위 OR 노드(aRoot)에 UNUSABLE flag 를 세팅한다.
                aRoot->node.lflag &= ~MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK;
                aRoot->node.lflag |=  MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE;
                break;
            }
            sNNFArg = (qtcNode *)(sNNFArg->node.next);
        }
        sCount *= sNNFCount;
    }
    else if ( ( aNode->node.lflag &
                ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
                == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        sCount = 0;
        sNNFArg = (qtcNode *)(aNode->node.arguments);
        while( sNNFArg )
        {
            estimatePartialCNF( sNNFArg, & sNNFCount, aRoot, aNFMaximum );
            sCount += sNNFCount;

            if( sCount > aNFMaximum )
            {
                // 이 노드와 하위노드의 count 를 계산한 값이 NORMALFORM_MAXIMUM 을 넘어섰다.
                // 현재 노드 기준으로 최상위 OR 노드(aRoot)에 UNUSABLE flag 를 세팅한다.
                if( aRoot == NULL )
                {
                    // aRoot 가 NULL 이면 상위에 OR 노드가 없는 것이다.
                    // 이 때에는 자기 자신을 CNF 대상에서 제외한다.
                    aRoot = aNode;
                }
                aRoot->node.lflag &= ~MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK;
                aRoot->node.lflag |=  MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE;
                break;
            }
            sNNFArg = (qtcNode *)(sNNFArg->node.next);
        }
    }
    else
    {
        sCount = 1;
    }

    // 현재 노드가 UNUSABLE 이면 CNF 대상에서 제외 되므로 count 를 0 으로 바꿔준다.
    if ( ( aNode->node.lflag & MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK )
           == MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE )
    {
        // count 를 더하므로 0을 반환한다.
        sCount = 0;
    }

    // UInt overflow방지
    if( sCount >= UINT_MAX )
    {
        *aCount = UINT_MAX;
    }
    else
    {
        *aCount = (UInt)sCount;
    }

    return;
}

IDE_RC
qmoNormalForm::extractNNFFilter4CNF( qcStatement  * aStatement,
                                     qtcNode      * aNode,
                                     qtcNode     ** aNNF )
{
/***********************************************************************
 *
 * Description :
 *         BUG-35155 Partial CNF
 *         CNF 로 변환 후 제외된 predicate 들을 NNF filter 로 만든다.
 *
 * Implementation :
 *         최상위 qtcNode 가 CNF UNUSABLE 인 경우는 노드 전체를 NNF 필터로 반환한다.
 *         그 외에는 qtcNode 를 복사해서 NNF 필터를 만든 후
 *         flag 와 dependency 정보를 설정하여 반환한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoNormalForm::extractNNFFilter4CNF::__FT__" );

    if ( ( aNode->node.lflag & MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK )
           == MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE )
    {
        // 최상위 node 가 UNUSABLE 이면 where 조건 전체가 nnf filter 가 된다.
        *aNNF = aNode;
    }
    else
    {
        //--------------------------------------
        // NNF filter 를 추출한다.
        //--------------------------------------
        IDE_TEST( makeNNF4CNFByCopyNodeTree( aStatement,
                                             aNode,
                                             aNNF )
                  != IDE_SUCCESS );

        if ( *aNNF != NULL )
        {
            //--------------------------------------
            // NNF form 논리연산자의 flag와 dependencies 재설정
            //--------------------------------------
            IDE_TEST( setFlagAndDependencies( *aNNF ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoNormalForm::makeNNF4CNFByCopyNodeTree( qcStatement  * aStatement,
                                          qtcNode      * aNode,
                                          qtcNode     ** aNNF )
{
/***********************************************************************
 *
 * Description :
 *         BUG-35155 Partial CNF
 *         CNF 로 변환 후 제외된 predicate 들을 NNF filter 로 만든다.
 *
 * Implementation :
 *         MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE 인 노드는
 *         하위 노드를 포함해서 복사하여 NNF 필터로 만든다.
 *
 ***********************************************************************/

    qtcNode           * sNode;
    qtcNode           * sNNFNode = NULL;
    qtcNode           * sTmpNode[2];
    qtcNode           * sNNFArg = NULL;
    qcNamePosition      sNullPosition;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::makeNNF4CNFByCopyNodeTree::__FT__" );

    if ( ( aNode->node.lflag & MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK )
           == MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE )
    {
        // make AND node
        SET_EMPTY_POSITION(sNullPosition);

        IDE_TEST( qtc::makeNode( aStatement,
                                 sTmpNode,
                                 &sNullPosition,
                                 (const UChar*)"AND",
                                 3 )
                  != IDE_SUCCESS );
        sNNFNode = sTmpNode[0];

        sNNFNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sNNFNode->node.lflag |= 1;

        // copy node tree
        IDE_TEST( copyNodeTree( aStatement, aNode, &sNode ) != IDE_SUCCESS );

        sNNFNode->node.arguments = (mtcNode *)sNode;

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sNNFNode )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( ( aNode->node.lflag &
               ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
               == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
        {
            sNNFArg = (qtcNode *)(aNode->node.arguments);

            IDE_TEST( makeNNF4CNFByCopyNodeTree( aStatement,
                                                 sNNFArg,
                                                 & sNode )
                      != IDE_SUCCESS );

            sNNFNode = sNode;

            sNNFArg = (qtcNode *)(sNNFArg->node.next);

            while ( sNNFArg != NULL )
            {
                IDE_TEST(  makeNNF4CNFByCopyNodeTree( aStatement,
                                                      sNNFArg,
                                                      & sNode )
                           != IDE_SUCCESS );

                IDE_TEST( addToMerge2( sNNFNode, sNode, &sNNFNode )
                          != IDE_SUCCESS );

                sNNFArg  = (qtcNode *)(sNNFArg->node.next);
            }
        }
        else
        {
            // OR node or terminal predicate node
            // Nothing to do.
        }
    }

    *aNNF = sNNFNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoNormalForm::copyNodeTree( qcStatement  * aStatement,
                             qtcNode      * aNode,
                             qtcNode     ** aCopy )
{
/***********************************************************************
 *
 * Description :
 *         BUG-35155 Partial CNF
 *         NNF 필터 생성을 위해 qtcNode 를 하위 노드를 포함하여 복사한다.
 *
 * Implementation :
 *         논리 연산자일 경우는 자기 자신을 복사하고 argument 노드를 재귀호출하여 복사한다.
 *         비교 연산자일 경우는 자기 자신을 복사한다.
 *
 ***********************************************************************/

    qtcNode           * sNode;
    qtcNode           * sTree;
    qtcNode           * sCursor;
    qtcNode           * sNNFArg = NULL;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::copyNodeTree::__FT__" );

    // copy qtcNodes recursivly
    if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
           == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        //---------------------------------
        // 1. Copy this node
        //---------------------------------
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNode)
                 != IDE_SUCCESS);

        idlOS::memcpy(sNode, aNode, ID_SIZEOF(qtcNode));
        sNode->node.next = NULL;

        sTree = sNode;

        //---------------------------------
        // 2. copy argument
        //---------------------------------
        sNNFArg = (qtcNode *)(aNode->node.arguments);
        IDE_TEST( copyNodeTree( aStatement, sNNFArg, &sNode ) != IDE_SUCCESS );

        // Attach to tree
        sTree->node.arguments = (mtcNode *)sNode;

        sCursor = sNode;

        //---------------------------------
        // 3. copy next nodes of argument
        //---------------------------------
        for ( sNNFArg  = (qtcNode *)(sNNFArg->node.next);
              sNNFArg != NULL;
              sNNFArg  = (qtcNode *)(sNNFArg->node.next),
              sCursor  = (qtcNode *)(sCursor->node.next) )
        {
            IDE_TEST( copyNodeTree( aStatement,
                                    sNNFArg,
                                    & sNode )
                      != IDE_SUCCESS );

            // Attach to previous node
            sCursor->node.next = (mtcNode *)sNode;
        }
    }
    else
    { // terminal predicate node
        //---------------------------------
        // 1. Copy this node
        //---------------------------------
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sNode)
                 != IDE_SUCCESS);

        idlOS::memcpy(sNode, aNode, ID_SIZEOF(qtcNode));
        sNode->node.next = NULL;

        sTree = sNode;
    }

    *aCopy = sTree;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoNormalForm::addToMerge2( qtcNode     * aPrevNF,
                            qtcNode     * aCurrNF,
                            qtcNode    ** aNFNode)
{
/***********************************************************************
 *
 * Description : 정규화 형태로 변환하는 과정에서 predicate을 연결.
 *
 *       BUG-35155 Partial CNF
 *       NNF filter 대상 노드를 제외하고 정규화 형태로 변환하는 기능이 추가되었다.
 *       NNF filter 대상 노드(CNF_USUSABLE flag)는 변환 대상에서 제외하기 때문에
 *       normal form(aPrefNF 나 aCurrNF)이 NULL 일 수 있다.
 *
 * Implementation :
 *     1. DNF
 *         OR의 하위 노드 처리 후, 각 AND 노드를 연결한다.
 *     2. CNF
 *         AND의 하위 노드 처리 후, 각 OR 노드를 연결한다.
 *
 ***********************************************************************/

    qtcNode     * sNode;

    IDU_FIT_POINT_FATAL( "qmoNormalForm::addToMerge2::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNFNode  != NULL );

    //--------------------------------------
    // add to merge
    //--------------------------------------

    if ( aPrevNF == NULL )
    {
        *aNFNode = aCurrNF;
    }
    else
    {
        if ( aCurrNF == NULL )
        {
            // Nothing to do.
        }
        else
        {
            // merge
            sNode = (qtcNode *)(aPrevNF->node.arguments);
            while (sNode->node.next != NULL)
            {
                sNode = (qtcNode *)(sNode->node.next);
            }
            sNode->node.next = aCurrNF->node.arguments;
        }

        *aNFNode = aPrevNF;
    }

    return IDE_SUCCESS;
}
