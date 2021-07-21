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
 * $Id: qmoSubquery.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Subquery Manager
 *
 *     질의 처리 중에 등장하는 Subquery에 대한 최적화를 수행한다.
 *     Subquery에 대하여 최적화를 통한 Graph 생성,
 *     Graph를 이용한 Plan Tree 생성을 담당한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qtc.h>
#include <qtcCache.h>
#include <qmsParseTree.h>
#include <qmgDef.h>
#include <qmo.h>
#include <qmoSubquery.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmoCheckViewColumnRef.h>
#include <qmvQTC.h>

extern mtfModule mtfExists;
extern mtfModule mtfNotExists;
extern mtfModule mtfUnique;
extern mtfModule mtfNotUnique;
extern mtfModule mtfEqualAny;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfList;
extern mtfModule mtfAnd;
extern mtfModule mtfEqual;
extern mtfModule mtfEqualAll;
extern mtfModule mtfNotEqualAny;
extern mtfModule mtfIsNull;
extern mtfModule mtfIsNotNull;
extern mtfModule mtfNotEqual;
extern mtfModule mtfLike;
extern mtfModule mtfNotLike;
extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;
extern mtfModule mtfDecrypt;


IDE_RC
qmoSubquery::makePlan( qcStatement  * aStatement,
                       UShort         aTupleID,
                       qtcNode      * aNode )
{
/***********************************************************************
 *
 * Description : subquery에 대한 plan생성
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode     * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::makePlan::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // subquery에 대한 plan tree 생성
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // subquery에 대한 plan 생성여부 판단후,
        // plan tree가 생성되지 않았으면, plan tree 생성.
        if( aNode->subquery->myPlan->plan == NULL )
        {
            aNode->subquery->myPlan->graph->myQuerySet->parentTupleID = aTupleID;

            /*
             * PROJ-2402
             * subquery 인 경우 parallel 수행 하지 않는다.
             */
            aNode->subquery->myPlan->graph->flag &= ~QMG_PARALLEL_IMPOSSIBLE_MASK;
            aNode->subquery->myPlan->graph->flag |= QMG_PARALLEL_IMPOSSIBLE_TRUE;

            IDE_TEST( aNode->subquery->myPlan->graph->makePlan( aNode->subquery,
                                                                NULL,
                                                                aNode->subquery->myPlan->graph )
                      != IDE_SUCCESS );

            aNode->subquery->myPlan->plan =
                aNode->subquery->myPlan->graph->myPlan;
        }
        else
        {
            // Nothing To Do
        }

        /* PROJ-2448 Subquery caching */
        IDE_TEST( qtcCache::preprocessSubqueryCache( aStatement, aNode )
                  != IDE_SUCCESS );
    }
    else
    {
        sNode = (qtcNode *)(aNode->node.arguments);

        while( sNode != NULL )
        {
            IDE_TEST( makePlan( aStatement, aTupleID, sNode ) != IDE_SUCCESS );

            sNode = (qtcNode *)(sNode->node.next);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::optimize( qcStatement  * aStatement,
                       qtcNode      * aNode,
                       idBool         aTryKeyRange)
{
/***********************************************************************
 *
 * Description : Subquery를 포함한 qtcNode에 대한 최적화 적용
 *
 *   1. predicate 형 subquery :
 *     (1) 일반 query의 최적화와 동일한 최적화 적용 가능
 *     (2) predicate subquery를 위한 optimization tip을 추가적으로 적용 가능.
 *     예) select * from t1 where i1 > (select sum(i2) from t2);
 *                                ------------------------------
 *         update t1 set i1=1 where i2 in (select i1 from t2);
 *                                 ---------------------------
 *         delete from t1 where i1 in (select i1 from t2);
 *                             ---------------------------
 *         select * from t1 where ( subquery ) = ( subquery );
 *                                ----------------------------
 *
 *   2. expression 형 subquery :
 *      예) select (select sum(i1) from t2 ) from t1;
 *                 -------------------------
 *      예) select * from t1 where i1 > 1 + (select sum(i1) from t2);
 *                                 --------------------------------
 *         이와 같은 경우는, constantSubquery()로 처리되어야 한다.
 *
 * Implementation :
 *
 *     0. Predicate/Expression형의 판단
 *        (1) 비교연산자인 경우
 *            - subquery가 predicate 바로 하위에 존재하는 경우, predicate형
 *            - 그렇지 않은 경우,  expression형으로 처리.
 *        (2) 비교 연산자가 아닌 경우
 *            - Expression형
 *
 *     1. predicate 형 subquery이면, 아래의 과정을 수행.
 *
 *        (1). graph 생성 전, predicate형 subquery 최적화 적용
 *            1) no calculate (not)EXISTS/(not)UNIQUE subquery
 *            2) transform NJ
 *
 *        (2). graph 생성
 *
 *        (3). graph 생성 후, predicate형 subquery 최적화 적용
 *            1) store and search
 *            2) IN절의 subquery keyRange
 *            3) subquery keyRange
 *
 *     2. expression 형 subquery이면,
 *        (1) graph 생성
 *        (2) Tip 적용 : constantSubquery()를 호출한다.
 *
 ***********************************************************************/

    idBool     sIsPredicate;
    qtcNode  * sNode = NULL;
    qtcNode  * sLeftNode = NULL;
    qtcNode  * sRightNode = NULL;
    
    qmsQuerySet * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmoSubquery::optimize::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );
    // aNode : (1) where절 처리시 호출 : 최상위 노드가 비교연산자
    //             selection graph의 myPredicate
    //             CNF의 constantPredicate
    //         (2) projection의 target절 처리
    //         (3) grouping graph의 aggr, group, having

    //--------------------------------------
    // subquery의 predicate/expression 판단
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_COMPARISON_MASK )
        == MTC_NODE_COMPARISON_TRUE )
    {
        // where절의 predicate 또는
        // grouping graph의 having 에 대한 처리

        // where절의 하위가 모두 subquery일 수도 있으므로,
        // 비교연산자 하위 두개 노드 모두 subquery에 대한 처리를 해야 함.

        // 비교연산자가
        // (1) IS NULL, IS NOT NULL, (NOT)EXISTS, (NOT)UNIQUE 인 경우,
        //     비교연산자 하위 노드는 하나
        // (2) BETWEEN, NOT BETWEEN 인 경우,
        //     비교연산자 하위 노드는 세개
        //     LIKE 인 경우, 비교연산자 하위 노드가 두개 또는 세개
        // (3) (1),(2)제외한 비교연산자 하위 노드는 두개
        // 이므로, 이에 대한 모든 고려가 이루어져야 한다.

        for( sNode = (qtcNode *)aNode->node.arguments,
                 sIsPredicate = ID_TRUE;
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next) )
        {
            if( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK )
                == QTC_NODE_SUBQUERY_EXIST )
            {
                if( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                    == MTC_NODE_OPERATOR_SUBQUERY )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsPredicate = ID_FALSE;
                    break;
                }
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        // projection의 target
        // grouping graph의 aggr, group에 대한 처리

        sIsPredicate = ID_FALSE;
    }

    //--------------------------------------
    // subquery 처리
    //--------------------------------------

    if( sIsPredicate == ID_TRUE )
    {
        // predicate형 subquery 인 경우

        sLeftNode = (qtcNode *)(aNode->node.arguments);
        sRightNode = (qtcNode *)(aNode->node.arguments->next);

        if( ( sLeftNode->lflag & QTC_NODE_SUBQUERY_MASK )
            == QTC_NODE_SUBQUERY_EXIST )
        {
            if( ( sLeftNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_SUBQUERY )
            {
                // 비교연산자 하위 노드가 subquery인 경우로,
                // predicate형 subquery이다.

                if ( sLeftNode->subquery->myPlan->graph == NULL )
                {
                    // BUG-23362
                    // subquery가 최적화 되지 않은 경우
                    // 즉, 첫 optimize인 경우에만 수행
                    IDE_TEST( optimizePredicate( aStatement,
                                                 aNode,
                                                 sRightNode,
                                                 sLeftNode,
                                                 aTryKeyRange )
                              != IDE_SUCCESS );
                }
                else
                {
                    // 이미 최적화 수행함
                }
            }
            else
            {
                // 비교연산자 하위 노드가 subquery가 아닌 경우로,
                // expression형 subquery로 처리됨.
            }
        }
        else
        {
            // Nothing To Do
        }

        for( sNode = sRightNode;
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next) )
        {
            if( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK )
                == QTC_NODE_SUBQUERY_EXIST )
            {
                if( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                    == MTC_NODE_OPERATOR_SUBQUERY )
                {
                    // 비교연산자 하위 노드가 subquery인 경우로,
                    // predicate형 subquery이다.

                    if ( sNode->subquery->myPlan->graph == NULL )
                    {
                        // BUG-23362
                        // subquery가 최적화 되지 않은 경우
                        // 즉, 첫 optimize인 경우에만 수행
                        IDE_TEST( optimizePredicate( aStatement,
                                                     aNode,
                                                     sLeftNode,
                                                     sNode,
                                                     aTryKeyRange )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // 이미 최적화 수행함
                    }
                }
                else
                {
                    // 비교연산자 하위 노드가 subquery가 아닌 경우로,
                    // expression형 subquery로 처리됨.
                }
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        // expression 형 subquery인 경우,

        // projection의 target
        // grouping graph의 aggr, group
        // where절의 predicate 중 비교연산자 하위 노드가 subquery가 아닌 경우,
        //  (예: i1 = 1 + subquery)
        IDE_TEST( optimizeExpr4Select( aStatement,
                                       aNode ) != IDE_SUCCESS );

    }

    // BUG-45212 서브쿼리의 외부 참조 컬럼이 연산일때 결과가 틀립니다.
    if ( QTC_IS_SUBQUERY(aNode) == ID_TRUE )
    {
        sQuerySet = ((qmsParseTree*)aNode->subquery->myPlan->parseTree)->querySet;

        if ( sQuerySet->setOp == QMS_NONE )
        {
            IDE_TEST( qmvQTC::extendOuterColumns( aNode->subquery,
                                                  NULL,
                                                  sQuerySet->SFWGH )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do.
        }
    }
    else
    {
        // nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::optimizeExprMakeGraph( qcStatement * aStatement,
                                    UInt          aTupleID,
                                    qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description : expression형 subquery에 대한 처리
 *
 *  예) INSERT...VALUES(...(subquery)...)
 *      : INSERT INTO T1 VALUES ( (SELECT SUM(I2) FROM T2) );
 *      UPDATE...SET column = (subquery)
 *      : UPDATE SET I1=(SELECT SUM(I1) FROM T2) WHERE I1>1;
 *      UPDATE...SET (column list) = (subquery)
 *      : UPDATE SET (I1,I2)=(SELECT I1,I2 FROM T2 WHERE I1=1);
 *
 * Implementation :
 *
 *     1. graph 생성
 *     2. constant subquery 최적화 적용 (store and search)
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::optimizeExprMakeGraph::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // subquery node를 찾는다.
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        if ( aNode->subquery->myPlan->graph == NULL )
        {
            // BUG-23362
            // subquery가 최적화 되지 않은 경우
            // 즉, 첫 optimize인 경우에만 수행
            IDE_TEST( makeGraph( aNode->subquery ) != IDE_SUCCESS );

            IDE_TEST( constantSubquery( aStatement,
                                        aNode ) != IDE_SUCCESS );
        }
        else
        {
            // nothing to do 
        }
    }
    else
    {
        sNode = (qtcNode *)(aNode->node.arguments);

        while( sNode != NULL )
        {
            IDE_TEST( optimizeExprMakeGraph( aStatement,
                                             aTupleID,
                                             sNode ) != IDE_SUCCESS );

            sNode = (qtcNode *)(sNode->node.next);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoSubquery::optimizeExprMakePlan( qcStatement * aStatement,
                                   UInt          aTupleID,
                                   qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description : expression형 subquery에 대한 처리
 *              모든 서브쿼리에 대해서 optimizeExprMakePlan 한후에 호출해야한다.
 *              ( BUG-32854 )
 *
 * Implementation :
 *
 *     1. plan 생성
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::optimizeExprMakePlan::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // subquery node를 찾는다.
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        if ( aNode->subquery->myPlan->plan == NULL )
        {
            // BUG-23362
            // plan이 생성되지 않은 경우에만 수행
            IDE_TEST( makePlan( aStatement,
                                aTupleID,
                                aNode ) != IDE_SUCCESS );
        }
        else
        {
            // nothing to do 
        }
    }
    else
    {
        sNode = (qtcNode *)(aNode->node.arguments);

        while( sNode != NULL )
        {
            IDE_TEST( optimizeExprMakePlan( aStatement,
                                            aTupleID,
                                            sNode ) != IDE_SUCCESS );

            sNode = (qtcNode *)(sNode->node.next);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoSubquery::makeGraph( qcStatement  * aSubQStatement )
{
/***********************************************************************
 *
 * Description : subquery에 대한 graph 생성
 *
 * Implementation :
 *
 *     qmo::makeGraph()호출
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoSubquery::makeGraph::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aSubQStatement != NULL );

    //------------------------------------------
    // PROJ-1413
    // subquery에 대한 Query Transformation 수행
    //------------------------------------------

    IDE_TEST( qmo::doTransform( aSubQStatement ) != IDE_SUCCESS );

    //------------------------------------------
    // PROJ-2469 Optimize View Materialization
    // subquery 하위의 불필요한 View Column을 찾아서 flag처리
    //------------------------------------------

    IDE_TEST( qmoCheckViewColumnRef::checkViewColumnRef( aSubQStatement,
                                                         NULL,
                                                         ID_TRUE ) != IDE_SUCCESS );

    //--------------------------------------
    // subquery에 대한 graph 생성
    //--------------------------------------

    IDE_TEST( qmo::makeGraph( aSubQStatement ) != IDE_SUCCESS );

    IDE_DASSERT( aSubQStatement->myPlan->graph != NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoSubquery::checkSubqueryType( qcStatement     * aStatement,
                                qtcNode         * aSubqueryNode,
                                qmoSubqueryType * aType )
{
/***********************************************************************
 *
 * Description : Subquery type을 판단한다.
 *
 *     < Subquery Type >
 *      -----------------------------------------------
 *     |         | outerColumn reference | aggregation |
 *      -----------------------------------------------
 *     | Type  A |           X           |      O      |
 *      -----------------------------------------------
 *     | Type  N |           X           |      X      |
 *      -----------------------------------------------
 *     | Type  J |           O           |      X      |
 *      -----------------------------------------------
 *     | Type JA |           O           |      O      |
 *      -----------------------------------------------
 *
 * Implementation :
 *
 *     outer column과 aggregation 존재유무를 조사해서, subquery type을 반환.
 *
 *     SET절의 경우, 일반 query와 동일하게 처리되며, type N or J로 설정된다.
 *
 *     1. outer column 존재유무 판단
 *        dependencies로 판단
 *        subqueryNode.dependencies != 0
 *
 *     2. aggregation 존재유무 판단
 *        qmsSFWGH에서 관리하고 있는
 *        (1) aggregation정보 (2) group 정보로 판단.
 *
 ***********************************************************************/

    qmsSFWGH        * sSFWGH = NULL;
    qmsQuerySet     * sQuerySet = NULL;
    qmoSubqueryType   sType;
    idBool            sExistOuterColumn;

    IDU_FIT_POINT_FATAL( "qmoSubquery::checkSubqueryType::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aSubqueryNode != NULL );
    IDE_DASSERT( aType != NULL );

    //--------------------------------------
    // subquery type 판단
    //--------------------------------------

    sQuerySet =
        ((qmsParseTree *)(aSubqueryNode->subquery->myPlan->parseTree))->querySet;

    sSFWGH = sQuerySet->SFWGH;

    if( qtc::dependencyEqual( & aSubqueryNode->depInfo,
                              & qtc::zeroDependencies ) == ID_TRUE )
    {
        if( sSFWGH == NULL )
        {
            sExistOuterColumn = ID_FALSE;
        }
        else
        {
            sExistOuterColumn = sSFWGH->outerHavingCase;
        }
    }
    else
    {
        checkSubqueryDependency( aStatement,
                                 sQuerySet,
                                 aSubqueryNode,
                                 & sExistOuterColumn );
    }

    if ( sExistOuterColumn == ID_FALSE )
    {
        //--------------------------------------
        // outer column이 존재하지 않는 경우
        //--------------------------------------

        if( sQuerySet->setOp == QMS_NONE )
        {
            // SET절이 아닌 경우,

            IDE_FT_ASSERT( sSFWGH != NULL );
            
            if( sSFWGH->group == NULL )
            {
                if( sSFWGH->aggsDepth1 == NULL )
                {
                    // aggregation이 존재하지 않음.
                    sType = QMO_TYPE_N;
                }
                else
                {
                    // aggregation이 존재함.
                    sType = QMO_TYPE_A;
                }
            }
            else
            {
                // aggregation이 존재함.
                sType = QMO_TYPE_A;
            }
        }
        else
        {
            // SET절인 경우,
            sType = QMO_TYPE_N;
        }
    }
    else
    {
        //--------------------------------------
        // outer column이 존재하는 경우
        //--------------------------------------

        if( sQuerySet->setOp == QMS_NONE )
        {
            // SET 절이 아닌 경우,

            if( sSFWGH->group == NULL )
            {
                if( sSFWGH->aggsDepth1 == NULL )
                {
                    // aggregation이 존재하지 않음.
                    sType = QMO_TYPE_J;
                }
                else
                {
                    // aggregation이 존재함.
                    sType = QMO_TYPE_JA;
                }
            }
            else
            {
                // aggregation이 존재함.
                sType = QMO_TYPE_JA;
            }
        }
        else
        {
            // SET 절인 경우,

            sType = QMO_TYPE_J;
        }
    }

    *aType = sType;

    return IDE_SUCCESS;
}

void qmoSubquery::checkSubqueryDependency( qcStatement * aStatement,
                                           qmsQuerySet * aQuerySet,
                                           qtcNode     * aSubQNode,
                                           idBool      * aExist )
{
/***********************************************************************
 *
 * Description : BUG-36575
 *               Subquery가 parent querySet에 의존성이 있는지 검사한다.
 *
 *   예1) 다음 쿼리에서 t3.i1은 parent querySet과는 무관하다.
 *        select 
 *            (select count(*) from t1 where i1 in (select i1 from t2 where i1=t3.i1))
 *        from t3;                                                             ^^^^^
 *
 *   예2) 다음 쿼리에서 t3.i1은 parent querySet과는 무관하다.
 *        select 
 *            (select count(*) from t1 where i1 in
 *                (select i1 from t2 where i1=t3.i1 union select i1 from t2 where i1=t4.i1))
 *        from t3, t4;                        ^^^^^                                  ^^^^^
 *
 *   예3) 다음 쿼리에서 t1.i1은 parent querySet과 의존성이 있다.
 *        select 
 *            (select count(*) from t1 where i1 in (select i1 from t2 where i1=t1.i1))
 *        from t3;                                                             ^^^^^
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSFWGH  * sSFWGH;
    qcDepInfo   sDepInfo;
    idBool      sExist;

    sExist = ID_FALSE;

    if ( aQuerySet->setOp == QMS_NONE )
    {
        sSFWGH = aQuerySet->SFWGH;

        qtc::dependencyAnd( & aSubQNode->depInfo,
                            & sSFWGH->outerQuery->depInfo,
                            & sDepInfo );
    
        if( qtc::dependencyEqual( & sDepInfo,
                                  & qtc::zeroDependencies ) != ID_TRUE )
        {
            sExist = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        checkSubqueryDependency( aStatement,
                                 aQuerySet->left,
                                 aSubQNode,
                                 & sExist );

        if ( sExist == ID_FALSE )
        {
            checkSubqueryDependency( aStatement,
                                     aQuerySet->right,
                                     aSubQNode,
                                     & sExist );
        }
        else
        {
            // Nothing to do.
        }
    }

    *aExist = sExist;
}

IDE_RC qmoSubquery::optimizeExpr4Select( qcStatement * aStatement,
                                         qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description : select문에 대한 expression형 subquery 처리
 *
 *      예) select (select sum(i1) from t2 ) from t1;
 *                 -------------------------
 *      예) select * from t1 where i1 > 1 + (select sum(i1) from t2);
 *                                 --------------------------------
 *
 *   qmoSubquery::optimizeExpr()
 *        : update, delete문에 대한 subquery 처리
 *          graph생성->constant subqueryTip적용->plan생성
 *   qmoSubquery::optimizeExpr4Select()
 *        : select문의 expression형 subquery처리
 *          이 함수내에서 plan을 생성하지 않는다.
 *
 * Implementation :
 *
 *     1. graph 생성
 *     2. constant subquery 최적화 적용 (store and search)
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::optimizeExpr4Select::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // subquery node를 찾는다.
    //--------------------------------------

    if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // subquery node를 찾음
        if ( aNode->subquery->myPlan->graph == NULL )
        {
            // BUG-23362
            // subquery가 최적화 되지 않은 경우
            // 즉, 첫 optimize인 경우에만 수행
            IDE_TEST( makeGraph( aNode->subquery ) != IDE_SUCCESS );
            
            IDE_TEST( constantSubquery( aStatement,
                                        aNode ) != IDE_SUCCESS );
        }
        else
        {
            // 이미 최적화 수행함
        }
    }
    else
    {
        sNode = (qtcNode *)(aNode->node.arguments);

        while( sNode != NULL )
        {
            IDE_TEST( optimizeExpr4Select( aStatement,
                                           sNode )
                      != IDE_SUCCESS );

            sNode = (qtcNode *)(sNode->node.next);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC qmoSubquery::optimizePredicate( qcStatement * aStatement,
                                       qtcNode     * aCompareNode,
                                       qtcNode     * aNode,
                                       qtcNode     * aSubQNode,
                                       idBool        aTryKeyRange )
{
/***********************************************************************
 *
 * Description : predicate형 subquery에 대한 subquery 최적화
 *
 * Implementation :
 *
 *     예) select * from t1 where i1 > (select sum(i2) from t2);
 *                                ------------------------------
 *         update t1 set i1=1 where i2 in (select i1 from t2);
 *                                 ---------------------------
 *         delete from t1 where i1 in (select i1 from t2);
 *                             ---------------------------
 *         select * from t1 where ( subquery ) = ( subquery );
 *                                ----------------------------
 *
 *    1. graph 생성 전, predicate형 subquery 최적화 적용
 *       1) no calculate (not)EXISTS/(not)UNIQUE subquery
 *       2) transform NJ
 *
 *    2. graph 생성
 *
 *    3. graph 생성 후, predicate형 subquery 최적화 적용
 *       1) store and search
 *       2) IN절의 subquery keyRange
 *       3) subquery keyRange
 *
 ***********************************************************************/

    idBool            sColumnNodeIsColumn = ID_TRUE;
    idBool            sIsTransformNJ = ID_FALSE;
    idBool            sIsStoreAndSearch = ID_TRUE;
    idBool            sIsInSubqueryKeyRangePossible;
    SDouble           sColumnCardinality;
    SDouble           sTargetCardinality;
    SDouble           sSubqueryResultCnt;
    idBool            sSubqueryIsSet;
    qmoSubqueryType   sSubQType;
    qmgPROJ         * sPROJ;

    IDU_FIT_POINT_FATAL( "qmoSubquery::optimizePredicate::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    if( ( aCompareNode->node.module == & mtfExists )    ||
        ( aCompareNode->node.module == & mtfNotExists ) ||
        ( aCompareNode->node.module == & mtfUnique )    ||
        ( aCompareNode->node.module == & mtfNotUnique ) ||
        ( aCompareNode->node.module == & mtfIsNull )    ||
        ( aCompareNode->node.module == & mtfIsNotNull ) )
    {
        IDE_DASSERT( aNode == NULL ); // subquery node가 아닌 쪽의 node
    }
    else
    {
        IDE_DASSERT( aNode != NULL ); // subquery node가 아닌 쪽의 node
    }
    IDE_DASSERT( aSubQNode != NULL ); // subquery node

    // BUG-23362
    // 이미 최적화된 subquery는 들어올 수 없음
    IDE_DASSERT( aSubQNode->subquery->myPlan->graph == NULL ); 

    //--------------------------------------
    // subquery node에 대한 subquery type 판단
    //--------------------------------------

    IDE_TEST( checkSubqueryType( aStatement,
                                 aSubQNode,
                                 &sSubQType ) != IDE_SUCCESS );

    if( ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->querySet->setOp
        == QMS_NONE )
    {
        sSubqueryIsSet = ID_FALSE;
    }
    else
    {
        sSubqueryIsSet = ID_TRUE;
    }

    //--------------------------------------
    // transform NJ, store and search, IN subquery keyRange
    // 적용여부를 판단하기 위한
    // predicate column과 subquery target column의 cardinality를 구한다.
    //--------------------------------------

    if( aNode != NULL )
    {
        if( qtcNodeI::isColumnArgument( aStatement, aNode ) == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            sColumnNodeIsColumn = ID_FALSE;
        }

        if( sColumnNodeIsColumn == ID_TRUE )
        {
            // predicate column의 cardinality를 구한다.
            IDE_TEST( getColumnCardinality( aStatement,
                                            aNode,
                                            &sColumnCardinality )
                      != IDE_SUCCESS );

            // subquery node의 subquery target column의 cardinality를 구한다.
            IDE_TEST( getSubQTargetCardinality( aStatement,
                                                aSubQNode,
                                                &sTargetCardinality )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // (NOT)EXISTS, (NOT)UNIQUE, IS NULL, IS NOT NULL 인 경우,
        sColumnNodeIsColumn = ID_FALSE;
    }

    if ((aTryKeyRange == ID_TRUE) && (sColumnNodeIsColumn == ID_TRUE))
    {
        sIsInSubqueryKeyRangePossible = ID_TRUE;
    }
    else
    {
        sIsInSubqueryKeyRangePossible = ID_FALSE;
    }

    //--------------------------------------
    // graph 생성 전, predicate형 subquery 최적화 팁 적용
    //--------------------------------------

    if( sColumnNodeIsColumn == ID_FALSE )
    {
        // 예: select * from t1 where ( subquery ) = ( subquery );
        // transforNJ 최적화팁을 적용하지 않는다.
    }
    else
    {
        if ((sIsInSubqueryKeyRangePossible == ID_TRUE) &&
            (QCU_OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD == 1))
        {
            /*
             * BUG-34235
             * in subquery key range tip 우선 적용
             * => transform NJ 하지 않는다.
             */
        }
        else
        {
            if ((sColumnCardinality < sTargetCardinality) ||
                (QCU_OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD == 2))
            {
                //----------------------------------------------------------
                // column cardinality < subquery target column의 cardinality
                // 이면, transform NJ 최적화 팁 적용
                //----------------------------------------------------------
                IDE_TEST(transformNJ(aStatement,
                                     aCompareNode,
                                     aNode,
                                     aSubQNode,
                                     sSubQType,
                                     sSubqueryIsSet,
                                     &sIsTransformNJ)
                         != IDE_SUCCESS);
            }
            else
            {
            }
        }
    }

    //--------------------------------------
    // graph 생성
    //--------------------------------------

    IDE_TEST( makeGraph( aSubQNode->subquery ) != IDE_SUCCESS );

    //--------------------------------------
    // graph 생성 후, transform NJ 최적화 팁 적용여부 저장
    //--------------------------------------

    //--------------------------------------
    // graph 생성 후, predicate형 subquery 최적화 적용
    //--------------------------------------

    if( sIsTransformNJ == ID_TRUE )
    {
        // transform NJ가 적용되는 경우
        sPROJ = (qmgPROJ *)(aSubQNode->subquery->myPlan->graph);

        sPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
        sPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_TRANSFORMNJ;
    }
    else
    {
        // transform NJ가 적용되지 않은 경우,
        // store and search 최적화 팁을 적용할 수 있다.
        // 다만, left가 subquery이고 비교 연산자가 group 연산자이면
        // storeAndSearch를 하지 않는다.
        // BUG-10328 fix, by kumdory
        // 그외에도
        // fix BUG-12934
        // column이 와야 할 자리에 host 변수가 오거나, 상수가 올 경우는
        // constant filter로 분류될 것이므로
        // store and search 효과가 없어, store and search를 하지 않는다.
        // 예) (subquery) in (subquery)
        //     ? in (subquery), ?=(subquery)
        //     1 in (subquery), 1=(subquery) ...
        
        // 추가로,
        // BUG-28929 ? between subquery와 같이
        // where절에 호스트변수 비교연산자 store and search가 되는 subquery가 오는 경우
        // 서버 비정상종료.
        // 예) i1 between ? and (subquery)
        if( aNode != NULL )
        {
            if ( ( aSubQNode == (qtcNode*)aCompareNode->node.arguments ) &&
                 ( ( aCompareNode->node.module->lflag &
                     MTC_NODE_GROUP_COMPARISON_MASK )
                   == MTC_NODE_GROUP_COMPARISON_TRUE ) )
            {
                sIsStoreAndSearch = ID_FALSE;
            }
            else
            {
                // 이 시점에서 다시
                // constantFilter인지를 판단하기가 간단하지 않아
                // predicate분류시, constant filter로 판단된 정보를 이용
                // 이 정보를 qtcNode.flag에 임시로 저장해 두고, 이를 이용한다.
                if( ( aCompareNode->lflag & QTC_NODE_CONSTANT_FILTER_MASK )
                    == QTC_NODE_CONSTANT_FILTER_TRUE )
                {
                    sIsStoreAndSearch = ID_FALSE;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }

        if( sIsStoreAndSearch == ID_FALSE )
        {
            // Nothing to do
        }
        else
        {
            IDE_TEST( storeAndSearch( aStatement,
                                      aCompareNode,
                                      aNode,
                                      aSubQNode,
                                      sSubQType,
                                      sSubqueryIsSet ) != IDE_SUCCESS );
        }
    }

    // To Fix PR-11460
    // Transform NJ가 적용되었다면,
    // In Subquery KeyRange또는 Store And Search가 적용될 수 없다.
    // To Fix PR-11461
    // In Subquery KeyRange 또는 Subquery KeyRange 최적화는
    // 일반 Table의 WHERE에 대해서만 가능하다.
    if ((sIsInSubqueryKeyRangePossible == ID_FALSE) ||
        (sIsTransformNJ == ID_TRUE))
    {
        // 예: select * from t1 where ( subquery ) = ( subquery );
        // IN subquery keyRange, subquery keyRange 최적화 팁을
        // 적용하지 않는다.
    }
    else
    {
        // To Fix PR-11460
        // In Subquery Key Range 적용 검사시
        // Target Cardinality 뿐 아니라,
        // Subquery의 결과 개수도 고려하여야 한다.
        // 둘 중에 작은 값으로 검사한다.

        sSubqueryResultCnt =
            aSubQNode->subquery->myPlan->graph->costInfo.outputRecordCnt;

        sSubqueryResultCnt = ( sSubqueryResultCnt < sTargetCardinality ) ?
            sSubqueryResultCnt : sTargetCardinality;

        if ((QCU_OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD == 1) ||
            ((QCU_OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD == 0) &&
             (sColumnCardinality >= sSubqueryResultCnt)))
        {
            //-----------------------------------------------------------
            // column cardinality >= subquery target column의 cardinality
            // 이면, IN subquery keyRange 최적화 팁 적용
            //
            // BUG-34235
            // cardinality에 관계없이 in subquery key range 항상 적용
            //-----------------------------------------------------------

            IDE_TEST( inSubqueryKeyRange( aCompareNode,
                                          aSubQNode,
                                          sSubQType ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        // subquery keyRange
        IDE_TEST( subqueryKeyRange( aCompareNode,
                                    aSubQNode,
                                    sSubQType ) != IDE_SUCCESS );
    }

    qcgPlan::registerPlanProperty(aStatement,
                                  PLAN_PROPERTY_OPTIMIZER_SUBQUERY_OPTIMIZE_METHOD);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoSubquery::transformNJ( qcStatement     * aStatement,
                          qtcNode         * aCompareNode,
                          qtcNode         * aNode,
                          qtcNode         * aSubQNode,
                          qmoSubqueryType   aSubQType,
                          idBool            aSubqueryIsSet,
                          idBool          * aIsTransform )
{
/***********************************************************************
 *
 * Description : transform NJ 최적화 팁 적용
 *
 *  < transform NJ 최적화 팁 >
 *
 *  subquery를 type N형 => type J형, type J형 => type J형으로 변경시킨다.
 *
 *  예) select * from t1 where i1 in ( select i1 from t2 );
 *  ==> select * from t1 where i1 in ( select i1 from t2 where t2.i1=t1.i1;
 *
 *  이러한 질의 변경은 subquery내에 predicate을 추가하여 보다 나은
 *  selection을 수행하는데 그 목적이 있다.
 *
 *  < transform NJ 팁 적용조건 >
 *  0. predicate이 IN, NOT IN인 경우.
 *  1. type N, type J 형  subquery인 경우
 *  2. subquery가 SET절이 아니어야 한다.
 *  3. subquery에 LIMIT절이 없어야 한다.
 *  4. predicate column이 NOT NULL이고, subquery target이 NOT NULL인 경우
 *  5. subquery target column에 인덱스가 있는 경우(type N에만 해당)
 *  6. PR-11632) Type N 인 경우 Subquery의 결과가 줄어들 수 있는 경우는
 *     Transform NJ를 적용하지 않음.
 *         - Subquery에 DISTINCT가 있는 경우
 *         - Subquery에 WHERE 절이 있는 경우
 *
 * Implementation :
 *
 *     1. transform NJ 팁 적용조건 검사
 *     2. subquery내에 추가할 predicate을 만들어서, 기존 where절에 연결.
 *     3. subquery node의 dependencies를 조정한다.
 *
 ***********************************************************************/

    idBool         sIsTemp = ID_TRUE;
    idBool         sIsNotNull;
    idBool         sIsExistIndex;
    idBool         sIsTransform = ID_FALSE;
    qmsParseTree * sSubQParseTree;

    qmsQuerySet  * sSubQuerySet;
    qmsSFWGH     * sSubSFWGH;

    IDU_FIT_POINT_FATAL( "qmoSubquery::transformNJ::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );
    IDE_DASSERT( aIsTransform != NULL );

    //--------------------------------------
    // transform NJ 최적화 팁
    //--------------------------------------

    while( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        //--------------------------------------
        // 조건 검사
        //--------------------------------------

        // 비교연산자 검사 ( in, not in )
        if( ( aCompareNode->node.module == &mtfEqualAny ) ||
            ( aCompareNode->node.module == &mtfNotEqualAll ) )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        sSubQParseTree = (qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree);

        // type N, type J 형  subquery인 경우이고,
        // subquery가 set절이 아니고, subquery에 LIMIT절이 없어야 한다.

        if( ( aSubQType == QMO_TYPE_N ) || ( aSubQType == QMO_TYPE_J ) )
        {
            if( aSubqueryIsSet == ID_FALSE )
            {
                if( sSubQParseTree->limit == NULL )
                {
                    // Nothing To Do
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }

        // predicate column이 NOT NULL이고,
        // subquery target이 NOT NULL인 경우
        IDE_TEST( checkNotNull( aStatement,
                                aNode,
                                aSubQNode,
                                &sIsNotNull ) != IDE_SUCCESS );

        if( sIsNotNull == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        // subquery target column에 인덱스가 있는 경우(type N에만 해당)
        if( aSubQType == QMO_TYPE_N )
        {
            IDE_TEST( checkIndex4SubQTarget( aStatement,
                                             aSubQNode,
                                             &sIsExistIndex )
                      != IDE_SUCCESS );

            if( sIsExistIndex == ID_TRUE )
            {
                // Nothing To Do
            }
            else
            {
                break;
            }

            //----------------------------------------------------
            // To Fix PR-11632
            // 자세한 내용은 Bug Description 참조
            // Type N 인 경우 Subquery의 결과가 줄어들 수 있는 경우는
            // Transform NJ를 적용하지 않음.
            //   - Subquery에 DISTINCT가 있는 경우
            //   - Subquery에 WHERE 절이 있는 경우
            //----------------------------------------------------

            sSubQuerySet =
                ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->querySet;

            // 반드시 SFWGH가 존재한다.
            IDE_DASSERT( sSubQuerySet->SFWGH != NULL );
            sSubSFWGH = sSubQuerySet->SFWGH;

            // Distinct가 존재하는 지 검사
            if ( sSubSFWGH->selectType == QMS_DISTINCT )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }

            // WHERE 절이 존재하는 지 검사
            if ( sSubSFWGH->where != NULL )
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
            // Nothing To Do
        }

        //--------------------------------------
        // subquery내에 추가할 predicate을 만들어서, 기존 where절에 연결.
        //--------------------------------------

        IDE_TEST( makeNewPredAndLink( aStatement,
                                      aNode,
                                      aSubQNode ) != IDE_SUCCESS );

        sIsTransform = ID_TRUE;


        //--------------------------------------
        // transform NJ 최적화 팁이 적용되었음을
        // subquery projection graph의 subquery 최적화 팁 flag에
        // 저장해야 하나, 아직 graph 생성 전이므로,
        // graph 생성 직후, transform NJ 최적화 팁 적용정보 저장.
        //--------------------------------------

    }

    *aIsTransform = sIsTransform;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::checkNotNull( qcStatement * aStatement,
                           qtcNode     * aNode,
                           qtcNode     * aSubQNode,
                           idBool      * aIsNotNull )
{
/***********************************************************************
 *
 * Description : transform NJ 최적화 팁 적용시
 *               predicate column과 subquery target column의
 *               not null 제약조건 검사
 *
 * Implementation :
 *
 *     1. predicate column과 subquery target column이
 *        base table의 column인지 검사
 *     2. 1의 조건이 만족하면, not null 조건검사.
 *
 ***********************************************************************/

    idBool       sIsTemp = ID_TRUE;
    idBool       sIsNotNull;
    qtcNode    * sNode;

    mtcTemplate       * sMtcTemplate;

    IDU_FIT_POINT_FATAL( "qmoSubquery::checkNotNull::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aNode != NULL );
    IDE_FT_ASSERT( aSubQNode != NULL );
    IDE_FT_ASSERT( aIsNotNull != NULL );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    //--------------------------------------
    //  not null 검사
    //--------------------------------------

    while( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        sIsNotNull = ID_TRUE;

        //--------------------------------------
        // predicate column의 not null 검사
        //--------------------------------------

        if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_LIST )
        {
            sNode = (qtcNode *)(aNode->node.arguments);

            while( sNode != NULL )
            {
                if( ( sMtcTemplate->rows[sNode->node.table].lflag
                      & MTC_TUPLE_TYPE_MASK )
                    == MTC_TUPLE_TYPE_TABLE )
                {
                    if( ( sMtcTemplate->rows[sNode->node.table]
                          .columns[sNode->node.column].flag
                          & MTC_COLUMN_NOTNULL_MASK )
                        == MTC_COLUMN_NOTNULL_TRUE )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sIsNotNull = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    sIsNotNull = ID_FALSE;
                    break;
                }

                sNode = (qtcNode *)(sNode->node.next);
            }
        }
        else
        {
            sNode = aNode;

            if( ( sMtcTemplate->rows[sNode->node.table].lflag
                  & MTC_TUPLE_TYPE_MASK )
                == MTC_TUPLE_TYPE_TABLE )
            {
                if( ( sMtcTemplate->rows[sNode->node.table]
                      .columns[sNode->node.column].flag
                      & MTC_COLUMN_NOTNULL_MASK )
                    == MTC_COLUMN_NOTNULL_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsNotNull = ID_FALSE;
                }
            }
            else
            {
                sIsNotNull = ID_FALSE;
            }
        }

        if( sIsNotNull == ID_TRUE )
        {
            //--------------------------------------
            // subquery target column의 not null 검사
            //--------------------------------------

            IDE_TEST( checkNotNullSubQTarget( aStatement,
                                              aSubQNode,
                                              &sIsNotNull ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    *aIsNotNull = sIsNotNull;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::checkNotNullSubQTarget( qcStatement * aStatement,
                                     qtcNode     * aSubQNode,
                                     idBool      * aIsNotNull )
{
/***********************************************************************
 *
 * Description : transform NJ, store and search 최적화 팁 적용시,
 *               subquery target column의 not null 제약조건 검사
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool       sIsTemp = ID_TRUE;
    idBool       sIsNotNull;
    qmsTarget  * sTarget;
    qtcNode    * sNode;

    mtcTemplate * sMtcTemplate;

    IDU_FIT_POINT_FATAL( "qmoSubquery::checkNotNullSubQTarget::__FT__" );


    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSubQNode != NULL );
    IDE_DASSERT( aIsNotNull != NULL );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    //--------------------------------------
    // subquery target column의 not null 검사
    //--------------------------------------

    while( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        sIsNotNull = ID_TRUE;

        sTarget =
            ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->
            querySet->target;

        while( sTarget != NULL )
        {
            sNode = sTarget->targetColumn;

            // fix BUG-8936
            // 예) create table t1( i1 not null );
            //     create table t2( i1 not null );
            //     select i1 from t1
            //     where i1 in ( select i1 from t2 group by i1);
            //     위 질의문의 경우, subquery target이 passNode로 연결되므로
            //     subquery target에 대한 not null 검사는
            //     passNode->node.argument에서 수행한다.
 
            // BUG-20272
            if( (sNode->node.module == & qtc::passModule) ||
                (sNode->node.module == & mtfDecrypt) )
            {
                sNode = (qtcNode *)(sNode->node.arguments);
            }
            else
            {
                // Nothing To Do
            }

            if( ( sMtcTemplate->rows[sNode->node.table].lflag
                  & MTC_TUPLE_TYPE_MASK )
                == MTC_TUPLE_TYPE_TABLE )
            {
                if( ( sMtcTemplate->rows[sNode->node.table]
                      .columns[sNode->node.column].flag
                      & MTC_COLUMN_NOTNULL_MASK )
                    == MTC_COLUMN_NOTNULL_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsNotNull = ID_FALSE;
                    break;
                }
            }
            else
            {
                sIsNotNull = ID_FALSE;
                break;
            }

            sTarget = sTarget->next;
        }
    }

    *aIsNotNull = sIsNotNull;

    return IDE_SUCCESS;
}


IDE_RC
qmoSubquery::checkIndex4SubQTarget( qcStatement * aStatement,
                                    qtcNode     * aSubQNode,
                                    idBool      * aIsExistIndex )
{
/***********************************************************************
 *
 * Description : transform NJ 최적화 팁 적용시
 *               subquery target column에 인덱스가 존재하는지 검사
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool          sIsTemp = ID_TRUE;
    idBool          sIsExistIndex = ID_TRUE;
    UShort          sTable;
    UInt            i;
    UInt            sCnt;
    UInt            sTargetCnt = 0;
    UInt            sIdxColumnID;
    UInt            sID;
    UInt            sCount;
    qmsQuerySet   * sQuerySet;
    qmsTarget     * sTarget;
    qcmTableInfo  * sTableInfo;
    qcmIndex      * sIndices = NULL;
    qtcNode       * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::checkIndex4SubQTarget::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSubQNode != NULL );
    IDE_DASSERT( aIsExistIndex != NULL );

    //--------------------------------------
    // subquery target column에 인덱스 존재 유무 판단
    //--------------------------------------

    while( sIsTemp == ID_TRUE )
    {
        sIsTemp = ID_FALSE;

        sQuerySet =
            ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->querySet;

        //--------------------------------------
        // subquery target column이 동일 테이블의 컬럼인지 검사
        // target column이 outer column인 경우는,
        // subquery type J형으로 판단되기때문에, 이 함수로 들어오지 않음.
        // 따라서, subuqery from절의 table인지를 굳이 검사하지 않는다.
        //--------------------------------------

        sTarget = sQuerySet->target;
        
        sNode = sTarget->targetColumn;

        // BUG-20272
        if ( sNode->node.module == &mtfDecrypt )
        {
            sNode = (qtcNode*) sNode->node.arguments;
        }
        else
        {
            // Nothing to do.
        }
        
        sTable = sNode->node.table;
        
        for( ;
             sTarget != NULL;
             sTarget = sTarget->next )
        {
            sTargetCnt++;

            sNode = sTarget->targetColumn;
            
            // BUG-20272
            if ( sNode->node.module == &mtfDecrypt )
            {
                sNode = (qtcNode*) sNode->node.arguments;
            }
            else
            {
                // Nothing to do.
            }
        
            if( sTable == sNode->node.table )
            {
                // Nothing To Do
            }
            else
            {
                sIsExistIndex = ID_FALSE;
                break;
            }
        }

        if( sIsExistIndex == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }

        //--------------------------------------
        // target column에 인덱스가 존재하는지 검사.
        //--------------------------------------
        sTableInfo =
            QC_SHARED_TMPLATE(aStatement)->tableMap[sTable].
            from->tableRef->tableInfo;
        sIndices = sTableInfo->indices;

        if( sTableInfo->indexCount == 0 )
        {
            sIsExistIndex = ID_FALSE;
            break;
        }
        else
        {
            // Nothing To Do
        }

        for( sCnt = 0; sCnt < sTableInfo->indexCount; sCnt++ )
        {
            if( sIndices[sCnt].keyColCount >= sTargetCnt )
            {
                // Nothint To Do
            }
            else
            {
                continue;
            }

            for( i = 0, sCount = 0; i < sTargetCnt; i++ )
            {
                sIdxColumnID = sIndices[sCnt].keyColumns[i].column.id;

                for( sTarget = sQuerySet->target;
                     sTarget != NULL;
                     sTarget = sTarget->next )
                {
                    sNode = sTarget->targetColumn;

                    if( sNode->node.conversion == NULL )
                    {
                        // Nothing To Do
                    }
                    else
                    {
                        sIsExistIndex = ID_FALSE;
                        break;
                    }

                    // BUG-20272
                    if ( sNode->node.module == &mtfDecrypt )
                    {
                        sNode = (qtcNode*) sNode->node.arguments;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    sID =
                        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sNode->node.table].
                        columns[sNode->node.column].column.id;

                    if( sIdxColumnID == sID )
                    {
                        sCount++;
                        break;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }

                if( sIsExistIndex == ID_TRUE )
                {
                    if( sNode != NULL )
                    {
                        // 연속적인 인덱스 사용
                        // Nothing To Do
                    }
                    else
                    {
                        // 연속적인 인덱스 사용이 아님.
                        sIsExistIndex = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    break;
                }
            } // target column이 속하는 인덱스가 존재하는지 검사

            if( sIsExistIndex == ID_TRUE )
            {
                if( sTargetCnt == sCount )
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
                break;
            }
        }
    }

    *aIsExistIndex = sIsExistIndex;

    return IDE_SUCCESS;
}


IDE_RC
qmoSubquery::makeNewPredAndLink( qcStatement * aStatement,
                                 qtcNode     * aNode,
                                 qtcNode     * aSubQNode )
{
/***********************************************************************
 *
 * Description : transform NJ 최적화 팁 적용시
 *          새로운 predicate을 만들어서, subquery의 기존 where절에 연결
 *
 * Implementation :
 *
 ***********************************************************************/

    qcNamePosition      sPosition;
    qmsSFWGH          * sSFWGH;
    qmsTarget         * sTarget;
    qtcNode           * sNode[2];
    qtcNode           * sListNode[2];
    qtcNode           * sLastNode;
    qtcNode           * sEqualNode;
    qtcNode           * sAndNode;
    qtcNode           * sTargetColumn;
    qtcNode           * sTargetNode;
    qtcNode           * sPredColumn;
    qtcNode           * sTempTargetColumn;

    IDU_FIT_POINT_FATAL( "qmoSubquery::makeNewPredAndLink::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );

    //--------------------------------------
    // subquery target column에 대한 노드 생성
    //--------------------------------------

    sSFWGH =
        ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->querySet->SFWGH;

    if( sSFWGH->target->next == NULL )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                 (void **)&sTargetColumn )
                  != IDE_SUCCESS );

        sTempTargetColumn = sSFWGH->target->targetColumn;

        // BUG-20272
        if ( sTempTargetColumn->node.module == &mtfDecrypt )
        {
            sTempTargetColumn = (qtcNode*) sTempTargetColumn->node.arguments;
        }
        else
        {
            // Nothing to do.
        }

        idlOS::memcpy(
            sTargetColumn, sTempTargetColumn, ID_SIZEOF(qtcNode) );
    }
    else
    {
        //--------------------------------------
        // target column이 여러개인 경우, LIST형태로 노드 생성
        //--------------------------------------

        // LIST 노드 생성
        SET_EMPTY_POSITION( sPosition );

        IDE_TEST( qtc::makeNode( aStatement,
                                 sListNode,
                                 &sPosition,
                                 &mtfList ) != IDE_SUCCESS );

        // subquery의 target column들을 복사해서 연결한다.

        sTarget = sSFWGH->target;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                 (void **)&sTargetNode )
                  != IDE_SUCCESS );
        
        sTempTargetColumn = sTarget->targetColumn;

        // BUG-20272
        if ( sTempTargetColumn->node.module == &mtfDecrypt )
        {
            sTempTargetColumn = (qtcNode*) sTempTargetColumn->node.arguments;
        }
        else
        {
            // Nothing to do.
        }

        idlOS::memcpy(
            sTargetNode, sTempTargetColumn, ID_SIZEOF(qtcNode) );

        sListNode[0]->node.arguments = (mtcNode *)sTargetNode;
        sLastNode = (qtcNode *)(sListNode[0]->node.arguments);

        sTarget = sTarget->next;

        while( sTarget != NULL )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                     (void **)&sTargetNode )
                      != IDE_SUCCESS );
            
            sTempTargetColumn = sTarget->targetColumn;

            // BUG-20272
            if ( sTempTargetColumn->node.module == &mtfDecrypt )
            {
                sTempTargetColumn = (qtcNode*) sTempTargetColumn->node.arguments;
            }
            else
            {
                // Nothing to do.
            }

            idlOS::memcpy(
                sTargetNode, sTempTargetColumn, ID_SIZEOF(qtcNode) );

            sLastNode->node.next = (mtcNode*)sTargetNode;
            sLastNode = (qtcNode *)(sLastNode->node.next);

            sTarget = sTarget->next;
        }

        sTargetColumn = sListNode[0];

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sTargetColumn )
                  != IDE_SUCCESS );
    }

    //--------------------------------------
    // predicate column에 대한 노드 생성
    //--------------------------------------

    if( aNode->node.module == &mtfList )
    {
        IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                         aNode,
                                         &sPredColumn,
                                         ID_FALSE,
                                         ID_FALSE,
                                         ID_FALSE,
                                         ID_FALSE ) // column node이므로
                  // constant node가 없다.
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                 (void **)&sPredColumn )
                  != IDE_SUCCESS );

        idlOS::memcpy( sPredColumn, aNode, ID_SIZEOF(qtcNode) );
    }

    sPredColumn->node.next = NULL;

    //--------------------------------------
    // equal 비교연산자 생성
    //--------------------------------------

    SET_EMPTY_POSITION(sPosition);

    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             &sPosition,
                             &mtfEqual ) != IDE_SUCCESS );
    sEqualNode = sNode[0];

    sEqualNode->node.arguments = (mtcNode *)sTargetColumn;
    sEqualNode->node.arguments->next = (mtcNode *)sPredColumn;
    sEqualNode->indexArgument = 0;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sEqualNode )
              != IDE_SUCCESS );

    //--------------------------------------
    // subquery의 where절에 새로 생성한 predicate 연결
    //--------------------------------------

    if( sSFWGH->where == NULL )
    {
        sSFWGH->where = sEqualNode;
    }
    else
    {
        if( ( sSFWGH->where->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_AND )
        {
            sEqualNode->node.next = sSFWGH->where->node.arguments;
            sSFWGH->where->node.arguments = (mtcNode *)sEqualNode;
        }
        else
        {
            IDE_TEST( qtc::makeNode( aStatement,
                                     sNode,
                                     &sPosition,
                                     &mtfAnd ) != IDE_SUCCESS );

            sAndNode = sNode[0];

            sAndNode->node.arguments = (mtcNode *)sEqualNode;
            sAndNode->node.arguments->next = (mtcNode *)(sSFWGH->where);
            sSFWGH->where = sAndNode;
        }

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sSFWGH->where )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::storeAndSearch( qcStatement    * aStatement,
                             qtcNode        * aCompareNode,
                             qtcNode        * aNode,
                             qtcNode        * aSubQNode,
                             qmoSubqueryType  aSubQType,
                             idBool           aSubqueryIsSet )
{
/***********************************************************************
 *
 * Description : store and search 최적화 팁 적용
 *
 *  < store and search 최적화 팁 >
 *
 *  type A형, type N형 subquery의 경우, outer query의 질의에 관계 없이
 *  동일한 결과를 생성한다. 따라서, subquery를 반복적으로 수행하지 않고
 *  질의 결과를 저장한 후에 search할 수 있도록 하여 성능 향상을 꾀할 수 있다.
 *
 *  < store and search 팁 적용조건 >
 *  1. type A형 또는 type N형 subquery ( transformNJ가 적용되면 안됨)
 *  2. predicate이
 *     (1) IN(=ANY), {>,>=,<,<=}ANY, {>,>=,<,<=}ALL
 *     (2) =,>,>=,<,<= : subquery filter인 경우,
 *                       subquery가 매번 재수행되는 것을 방지하기 위해
 *                       한 건의 로우라도 store and search로 처리한다.
 *
 * Implementation :
 *
 *     subquery graph생성 후, subquery type A, N형에 대해
 *     qmgProjection graph에 store and search tip과 저장방식을 설정하고,
 *     HASH인 경우, qmgProjection graph에 storeNSearchPred 을 달아준다.
 *
 *    [저장방식지정]
 *
 *    1. IN(=ANY), NOT IN(!=ALL)
 *       (1) 한 컬럼인 경우, hash로 저장.
 *       (2) 두 컬럼 이상인 경우,
 *          predicate 양쪽 모두 not null constraint가 존재하는지를 검사.
 *          .not null constraint가 존재 : hash로 저장
 *          .not null constraint가 존재하지 않음: SORT노드에 storeOnly로 저장.
 *
 *    2. {>,>=,<,<=}ANY, {>,>=,<,<=}ALL
 *       LMST로 저장
 *
 *    3. =ALL, !=ANY
 *       (1) 한 컬럼인 경우, LMST로 저장.
 *       (2) 두 컬럼 이상인 경우,
 *           predicate 양쪽 모두 not null constraint가 존재하는지를 검사.
 *           .not null constraint가 존재 : LMST로 저장
 *           .not null constraint가 존재하지 않음: SORT노드에 storeOnly로 저장
 *
 *    4. =, >, >=, <, <=
 *       SORT노드에 store only로 저장.
 *
 *    [ IN(=ANY), NOT IN(!=ALL), =ALL, !=ANY 저장방식의 제약사항 ]
 *    ------------------------------------------------------------
 *      이 경우, 두 컬럼이상인 경우, not null constraint가 존재하지 않으면,
 *      SORT 노드에 store only로 저장하게 된다.
 *      이때, subquery 수행시 결과가 줄어드는 경우만, 저장하도록 한다.
 *      ( SORT 노드에 store only로 저장하게 되면, full scan으로 처리되므로)
 *
 *      1. SET 절인 경우, 저장하지 않는다.
 *         UNION ALL이 아닌 경우는, 이미 결과가 저장되어 있고,
 *         UNION ALL인 경우는, 모든 subquery에 대한 검사에 대한 부하가 크므로.
 *      2. SET 절이 아닌 경우,
 *         (1) where절이 존재하거나,
 *         (2) group by가 존재하거나,
 *         (3) aggregation이 존재하거나,
 *         (4) distinct가 존재하는 경우에 저장한다.
 *
 *   [참고]
 *   두 컬럼이상인 경우,
 *   predicate 양쪽 컬럼이 모두 not null constraint가 존재해야 한다.
 *   아래의 예처럼, 질의 결과가 틀릴수 있기 때문이다.
 *   예) (2,3,5) IN (4,7,NULL)       => FALSE
 *       (2,3,5) IN (NULL,3,NULL)    => UNKOWN
 *       (2,3,5) NOT IN (4,7,NULL)   => TRUE
 *       (2,3,5) NOT IN (NULL,3,NULL)=> UNKOWN
 *
 ***********************************************************************/

    idBool         sIsSortNodeStoreOnly;
    qmsQuerySet  * sQuerySet;
    qmgPROJ      * sPROJ;

    IDU_FIT_POINT_FATAL( "qmoSubquery::storeAndSearch::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );

    //--------------------------------------
    // store and search 최적화 팁 적용
    //--------------------------------------

    //--------------------------------------
    // 조건검사
    //--------------------------------------

    // subquery type 인 A, N 형이어야 한다.
    if( ( aSubQType == QMO_TYPE_A ) || ( aSubQType == QMO_TYPE_N ) )
    {
        sQuerySet =
            ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->querySet;
        sPROJ  = (qmgPROJ *)(aSubQNode->subquery->myPlan->graph);

        //--------------------------------------
        // 저장방식 지정
        //--------------------------------------

        if( ( aCompareNode->node.module == & mtfEqualAny ) ||
            ( aCompareNode->node.module == & mtfNotEqualAll ) )
        {
            //--------------------------------------
            // IN(=ANY), NOT IN(!=ALL)
            //--------------------------------------
            //-----------------------
            // store and search 적용여부와 저장방식지정
            //-----------------------

            IDE_TEST( setStoreFlagIN( aStatement,
                                      aCompareNode,
                                      aNode,
                                      aSubQNode,
                                      sQuerySet,
                                      sPROJ,
                                      aSubqueryIsSet ) != IDE_SUCCESS );
        }
        else if( ( aCompareNode->node.module == & mtfEqualAll ) ||
                 ( aCompareNode->node.module == & mtfNotEqualAny ) )
        {
            //---------------------------------------
            // =ALL, !=ANY
            //--------------------------------------
            //------------------------
            // store and search 적용여부와 저장방식지정
            //------------------------

            IDE_TEST( setStoreFlagEqualAll( aStatement,
                                            aNode,
                                            aSubQNode,
                                            sQuerySet,
                                            sPROJ,
                                            aSubqueryIsSet ) != IDE_SUCCESS );
        }
        else
        {
            switch( aCompareNode->node.module->lflag
                    & MTC_NODE_GROUP_COMPARISON_MASK )
            {
                case ( MTC_NODE_GROUP_COMPARISON_TRUE ) :
                {
                    //--------------------------------------
                    // {>,>=,<,<=}ANY, {>,>=,<,<=}ALL 인 경우,
                    //  : LMST로 저장
                    //--------------------------------------

                    switch( aCompareNode->node.module->lflag
                            & MTC_NODE_OPERATOR_MASK )
                    {
                        case MTC_NODE_OPERATOR_GREATER :
                        case MTC_NODE_OPERATOR_GREATER_EQUAL :
                        case MTC_NODE_OPERATOR_LESS :
                        case MTC_NODE_OPERATOR_LESS_EQUAL :
                            sPROJ->subqueryTipFlag
                                &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
                            sPROJ->subqueryTipFlag
                                |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

                            sPROJ->subqueryTipFlag
                                &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
                            sPROJ->subqueryTipFlag
                                |= QMG_PROJ_SUBQUERY_STORENSEARCH_LMST;
                            break;
                        default :
                            // Nothint To Do
                            break;
                    }
                    break;
                }
                case ( MTC_NODE_GROUP_COMPARISON_FALSE ) :
                {
                    //--------------------------------------
                    //  =, !=, >, >=, <, <=,
                    //  IS NULL, IS NOT NULL, LIKE, NOT LIKE 인 경우,
                    //  : SORT 노드에 store only로 저장.
                    //--------------------------------------

                    sIsSortNodeStoreOnly = ID_FALSE;

                    switch( aCompareNode->node.module->lflag
                            & MTC_NODE_OPERATOR_MASK )
                    {
                        case MTC_NODE_OPERATOR_EQUAL :
                        case MTC_NODE_OPERATOR_NOT_EQUAL :
                        case MTC_NODE_OPERATOR_GREATER :
                        case MTC_NODE_OPERATOR_GREATER_EQUAL :
                        case MTC_NODE_OPERATOR_LESS :
                        case MTC_NODE_OPERATOR_LESS_EQUAL :
                            sIsSortNodeStoreOnly = ID_TRUE;
                            break;
                        default :
                            // Nothing To Do
                            break;
                    }

                    if( ( aCompareNode->node.module == &mtfIsNull )    ||
                        ( aCompareNode->node.module == &mtfIsNotNull ) ||
                        ( aCompareNode->node.module == &mtfLike )      ||
                        ( aCompareNode->node.module == &mtfNotLike )   ||
                        ( aCompareNode->node.module == &mtfBetween )   ||
                        ( aCompareNode->node.module == &mtfNotBetween ) )
                    {
                        sIsSortNodeStoreOnly = ID_TRUE;
                    }
                    else
                    {
                        // Nothing To Do
                    }

                    if( sIsSortNodeStoreOnly == ID_TRUE )
                    {
                        sPROJ->subqueryTipFlag
                            &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
                        sPROJ->subqueryTipFlag
                            |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

                        sPROJ->subqueryTipFlag
                            &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
                        sPROJ->subqueryTipFlag
                            |= QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                    break;
                }
                default :
                    // Nothing To Do
                    break;
            }
        }
    }
    else
    {
        // subquery type J, JA 형으로,
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::setStoreFlagIN( qcStatement * aStatement,
                             qtcNode     * aCompareNode,
                             qtcNode     * aNode,
                             qtcNode     * aSubQNode,
                             qmsQuerySet * aQuerySet,
                             qmgPROJ     * aPROJ,
                             idBool        aSubqueryIsSet )
{
/***********************************************************************
 *
 * Description : store and search 최적화 팁 적용시
 *               IN(=ANY), NOT IN(!=ALL)에 대한 저장방식 지정
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool    sIsNotNull;
    idBool    sIsHash = ID_FALSE;
    idBool    sIsStoreOnly = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoSubquery::setStoreFlagIN::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aPROJ != NULL );

    if( aQuerySet->target->next == NULL )
    {
        //--------------------------------------
        // 한 컬럼인 경우,
        // 노드에서 subquery target column에 대한 not null 검사여부에
        // 대한 flag 설정.
        // 예: i1 IN ( select a1 from ... )
        //     i1은 노드에서 null 검사를 수행하므로,
        //     subquery target인 a1에 대해서만 검사.
        //     1) a1이 not null 이면,
        //        노드에서 not null 검사를 하지 않아도 된다는 정보설정.
        //     2) a1이 not null이 아니면,
        //        노드에서 not null 검사를 해야 한다는 정보 설정.
        //--------------------------------------

        IDE_TEST( checkNotNullSubQTarget( aStatement,
                                          aSubQNode,
                                          &sIsNotNull )
                  != IDE_SUCCESS );


        // 아래에서 HASH로 저장방식 지정을 위해,
        sIsHash = ID_TRUE;
    }
    else
    {
        IDE_TEST( checkNotNull( aStatement,
                                aNode,
                                aSubQNode,
                                &sIsNotNull ) != IDE_SUCCESS );

        if( sIsNotNull == ID_TRUE )
        {
            sIsHash = ID_TRUE;
        }
        else
        {
            // multi column인 경우, not null제약조건을 만족하지 않는 경우,
            // In(=ANY), NOT IN(!=ALL), =ALL, !=ANY 저장방식의 제약사항 적용

            if( aSubqueryIsSet == ID_TRUE )
            {
                // store and search 최적화 팁을 적용하지 않는다.
                // Nothing To Do
            }
            else
            {
                
                IDE_DASSERT( aQuerySet->SFWGH != NULL );

                if( ( aQuerySet->SFWGH->where != NULL )      ||
                    ( aQuerySet->SFWGH->group != NULL )      ||
                    ( aQuerySet->SFWGH->aggsDepth1 != NULL ) ||
                    ( aQuerySet->SFWGH->selectType == QMS_DISTINCT ) )
                {
                    // fix BUG-8936
                    // sort node에 store only로 저장
                    sIsStoreOnly = ID_TRUE;
                }
                else
                {
                    // store and search 최적화 팁을 적용하지 않는다.
                    // Nothing To Do
                }
            }
        }
    }

    //--------------------------------------
    // 저장방식지정.
    // (1) 한 컬럼인 경우, hash로 저장.
    // (2) 두 컬럼 이상인 경우,
    //     predicate 양쪽 모두 not null constraint가 존재하는지를 검사.
    //     .not null constraint가 존재 : hash로 저장
    //     .not null constraint가 존재하지 않음: SORT노드에 storeOnly로 저장.
    //
    // 저장방식이 HASH인 경우는, projection graph에 해당 predicate을 달아준다.
    //--------------------------------------
    if( sIsHash == ID_TRUE )
    {
        aPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
        aPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

        aPROJ->subqueryTipFlag
            &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
        aPROJ->subqueryTipFlag
            |= QMG_PROJ_SUBQUERY_STORENSEARCH_HASH;

        //--------------------------------------
        // store and search predicate을 qmgProjection graph에 연결.
        //--------------------------------------

        aPROJ->storeNSearchPred = aCompareNode;

        //--------------------------------------
        // not null 검사여부 지정
        // 1. 한 컬럼인 경우,
        //    노드에서 subquery target column에 대한 not null 검사여부에
        //    대한 flag 설정.
        //    예: i1 IN ( select a1 from ... )
        //        i1은 노드에서 null 검사를 수행하므로,
        //        subquery target인 a1에 대해서만 검사.
        //        1) a1이 not null 이면,
        //           노드에서 not null 검사를 하지 않아도 된다는 정보설정.
        //        2) a1이 not null이 아니면,
        //           노드에서 not null 검사를 해야 한다는 정보 설정.
        // 2. 여러 컬럼인 경우,
        //    노드에서 양쪽 컬럼에 대한
        //    null 검사를 수행하지 않아도 된다는 정보 설정.
        //--------------------------------------

        if( sIsNotNull == ID_TRUE )
        {
            aPROJ->subqueryTipFlag
                &= ~QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_MASK;
            aPROJ->subqueryTipFlag
                |= QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_FALSE;
        }
        else
        {
            aPROJ->subqueryTipFlag
                &= ~QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_MASK;
            aPROJ->subqueryTipFlag
                |= QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_TRUE;
        }
    }
    else
    {
        if( sIsStoreOnly == ID_TRUE )
        {
            aPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
            aPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

            aPROJ->subqueryTipFlag
                &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
            aPROJ->subqueryTipFlag
                |= QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY;
        }
        else
        {
            // store and search 최적화를 수행하지 않는다.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::setStoreFlagEqualAll( qcStatement  * aStatement,
                                   qtcNode      * aNode,
                                   qtcNode      * aSubQNode,
                                   qmsQuerySet  * aQuerySet,
                                   qmgPROJ      * aPROJ,
                                   idBool         aSubqueryIsSet )
{
/***********************************************************************
 *
 * Description : store and search 최적화 팁 적용시
 *               =ALL, !=ANY에 대한 저장방식 지정.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool    sIsNotNull;
    idBool    sIsLMST = ID_FALSE;
    idBool    sIsStoreOnly = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoSubquery::setStoreFlagEqualAll::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aPROJ != NULL );

    //--------------------------------------
    // 저장방식지정
    //--------------------------------------

    if( aQuerySet->target->next == NULL )
    {
        //--------------------------------------
        // 한 컬럼인 경우, LMST로 저장.
        //--------------------------------------

        sIsLMST = ID_TRUE;
    }
    else
    {
        //--------------------------------------
        // 여러 컬럼인 경우,
        // predicate 양쪽 모두 not null constraint가 존재하는지 검사.
        //--------------------------------------

        IDE_TEST( checkNotNull( aStatement,
                                aNode,
                                aSubQNode,
                                &sIsNotNull ) != IDE_SUCCESS );

        if( sIsNotNull == ID_TRUE )
        {
            sIsLMST = ID_TRUE;
        }
        else
        {
            // multi column인 경우,
            // not null constrant제약조건을 만족하지 않는 경우
            // IN(=ANY), NOT IN(!=ALL), =ALL, !=ANY 저장방식의 제약사항 적용

            if( aSubqueryIsSet == ID_TRUE )
            {
                // store and search 최적화 팁을 적용하지 않는다.
                // Nothing To Do
            }
            else
            {
                IDE_DASSERT( aQuerySet->SFWGH != NULL );

                if( ( aQuerySet->SFWGH->where != NULL ) ||
                    ( aQuerySet->SFWGH->group != NULL ) ||
                    ( aQuerySet->SFWGH->aggsDepth1 != NULL ) ||
                    ( aQuerySet->SFWGH->selectType == QMS_DISTINCT ) )
                {
                    // fix BUG-8936
                    // sort node에 store only로 저장
                    sIsStoreOnly = ID_TRUE;
                }
                else
                {
                    // store and search 최적화 팁을 적용하지 않는다.
                    // Nothing To Do
                }
            }
        }
    }

    //--------------------------------------
    // =ALL, !=ANY
    // (1) 한 컬럼인 경우, LMST로 저장.
    // (2) 두 컬럼 이상인 경우,
    //     predicate 양쪽 모두 not null constraint가
    //     존재하는지를 검사.
    //    .not null constraint가 존재 : LMST로 저장
    //    .not null constraint가 존재하지 않음
    //        : SORT노드에 storeOnly로 저장
    //--------------------------------------

    if( sIsLMST == ID_TRUE )
    {
        aPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
        aPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

        aPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
        aPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_STORENSEARCH_LMST;
    }
    else
    {
        if( sIsStoreOnly == ID_TRUE )
        {
            aPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
            aPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

            aPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
            aPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY;
        }
        else
        {
            // store and search 최적화를 수행하지 않는다.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::inSubqueryKeyRange( qtcNode        * aCompareNode,
                                 qtcNode        * aSubQNode,
                                 qmoSubqueryType  aSubQType )
{
/***********************************************************************
 *
 * Description : IN절의 subquery keyRange 최적화 팁 적용
 *
 *   < IN절의 subquery keyRange 최적화 팁 >
 *
 *   FROM T1 WHERE I1 IN ( SELECT A1 FROM T2 );
 *   와 같이 quantify 비교 연산자와 subquery를 함께 사용하는 경우,
 *   transformNJ 최적화 팁을 사용할 수 있으나, 이는 수행방향이 정해져 있어,
 *   T1의 레코드 수가 subquery의 레코드 수보다 매우 많은 경우, 그 효율이
 *   떨어지는 문제가 있다. 이와 같이 IN subquery keyRange 최적화는 T1의
 *   레코드 수가 subquery의 레코드 수보다 매우 많은 경우에 적용하여 효율을
 *   높이게 된다.
 *
 * Implementation :
 *
 *    subquery에 대한 graph를 생성하고 나면, subquery type A, N형인 경우,
 *    column cardinality >= subquery target cardinality 이면,
 *    qmgProjection graph에 IN subquery keyRange최적화 팁을 설정
 *
 ***********************************************************************/

    qmgPROJ       * sPROJ;

    IDU_FIT_POINT_FATAL( "qmoSubquery::inSubqueryKeyRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );

    //--------------------------------------
    // IN subquery keyRange 최적화 팁 적용
    //--------------------------------------

    //--------------------------------------
    // 조건검사
    // 1. subquery type A, N
    // 2. IN 비교연산자
    //--------------------------------------

    if( ( aSubQType == QMO_TYPE_A ) || ( aSubQType == QMO_TYPE_N ) )
    {
        if( aCompareNode->node.module == &mtfEqualAny )
        {
            sPROJ = (qmgPROJ *)(aSubQNode->subquery->myPlan->graph);

            // IN subquery 최적화 팁 정보 설정
            sPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
            sPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_IN_KEYRANGE;
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

    return IDE_SUCCESS;
}


IDE_RC
qmoSubquery::subqueryKeyRange( qtcNode        * aCompareNode,
                               qtcNode        * aSubQNode,
                               qmoSubqueryType  aSubQType )
{
/***********************************************************************
 *
 * Description : subquery keyRange 최적화 팁 적용
 *
 *   < subquery keyRange 최적화 팁 >
 *
 *   one-row one-column 형태의 subquery의 경우,
 *   subquery를 먼저 실행시켜 얻은 값으로 keyRange를 구성한다.
 *
 * Implementation :
 *
 *   subquery에 대한 graph를 생성하고 나면, subquery type A, N형인 경우,
 *   =, >, >=, <, <= 에 대해서 subquery keyRange 최적화 팁을 설정하는데,
 *   팁 설정시, store and search 최적화 팁 flag와 oring 되도록 한다.
 *   (이유는, inSubqueryKeyRange()와 동일)
 *
 ***********************************************************************/

    qmgPROJ     * sPROJ;

    IDU_FIT_POINT_FATAL( "qmoSubquery::subqueryKeyRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSubQNode != NULL );

    //--------------------------------------
    // subquery keyRange 최적화 팁 적용
    //--------------------------------------

    //--------------------------------------
    // 조건검사
    // 1. subquery type A, N
    // 2. 비교연산자 =, >, >=, <, <=
    //--------------------------------------

    if( ( aSubQType == QMO_TYPE_A ) || ( aSubQType == QMO_TYPE_N ) )
    {
        switch( aCompareNode->node.module->lflag &
                ( MTC_NODE_OPERATOR_MASK | MTC_NODE_GROUP_COMPARISON_MASK ) )
        {
            case ( MTC_NODE_OPERATOR_EQUAL |
                   MTC_NODE_GROUP_COMPARISON_FALSE ) :
            case ( MTC_NODE_OPERATOR_GREATER |
                   MTC_NODE_GROUP_COMPARISON_FALSE ) :
            case ( MTC_NODE_OPERATOR_GREATER_EQUAL |
                   MTC_NODE_GROUP_COMPARISON_FALSE ) :
            case ( MTC_NODE_OPERATOR_LESS |
                   MTC_NODE_GROUP_COMPARISON_FALSE ) :
            case ( MTC_NODE_OPERATOR_LESS_EQUAL |
                   MTC_NODE_GROUP_COMPARISON_FALSE ):
                sPROJ = (qmgPROJ *)(aSubQNode->subquery->myPlan->graph);

            sPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
            sPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_KEYRANGE;
            break;
            default :
                // Nothing To Do
                break;
        }
    }
    else
    {
        // subquery type이 J, JA 형인 경우,
        // Nothing To Do
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoSubquery::constantSubquery( qcStatement * aStatement,
                               qtcNode     * aSubQNode )
{
/***********************************************************************
 *
 * Description : Expression형 subquery에 대한
 *               constant subquery 최적화 팁 적용
 *
 *   < constant subquery 최적화 팁 >
 *
 *   one-row one-column 형태 subquery인 경우,
 *   subquery가 매번 재수행되지 않도록, 한번 수행된 결과를 저장해 두고,
 *   이를 이용한다.
 *
 * Implementation :
 *
 *     subquery type A, N인 경우,
 *     qmgProjection graph에 constant subquery 최적화 팁 정보를 설정한다.
 *     저장 방식은 SORT노드에 store only로 저장.
 *
 ***********************************************************************/

    qmoSubqueryType    sSubQType;
    qmgPROJ          * sPROJ;

    IDU_FIT_POINT_FATAL( "qmoSubquery::constantSubquery::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSubQNode != NULL );

    //--------------------------------------
    // expression형 subquery에 대한 constant subquery 최적화 팁 적용
    //--------------------------------------

    //--------------------------------------
    // subquery node에 대한 subquery type 판단
    //--------------------------------------

    IDE_TEST( checkSubqueryType( aStatement,
                                 aSubQNode,
                                 &sSubQType ) != IDE_SUCCESS );

    if( ( sSubQType == QMO_TYPE_A ) || ( sSubQType == QMO_TYPE_N ) )
    {

        sPROJ = (qmgPROJ *)(aSubQNode->subquery->myPlan->graph);

        sPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_TIP_MASK;
        sPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_TIP_STORENSEARCH;

        sPROJ->subqueryTipFlag &= ~QMG_PROJ_SUBQUERY_STORENSEARCH_MASK;
        sPROJ->subqueryTipFlag |= QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY;
    }
    else
    {
        // subquery type이 J, JA형인 경우
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSubquery::getColumnCardinality( qcStatement * aStatement,
                                   qtcNode     * aColumnNode,
                                   SDouble     * aCardinality )
{
/***********************************************************************
 *
 * Description :  predicate column의 cardinality를 구한다.
 *
 *    store and search, IN subquery keyRange, transform NJ 판단시
 *    필요한 predicate column의 cardinality를 구한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SDouble         sCardinality;
    SDouble         sOneCardinality;
    qtcNode       * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::getColumnCardinality::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aColumnNode != NULL );
    IDE_DASSERT( aCardinality != NULL );

    //--------------------------------------
    // predicate column의 cardinality를 구한다.
    //--------------------------------------

    if( ( aColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_LIST )
    {
        sCardinality = 1;
        sNode = (qtcNode *)(aColumnNode->node.arguments);

        while( sNode != NULL )
        {
            if( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
            {
                sOneCardinality =
                    QC_SHARED_TMPLATE(aStatement)->tableMap[sNode->node.table].
                    from->tableRef->statInfo->\
                    colCardInfo[sNode->node.column].columnNDV;
            }
            else
            {
                sOneCardinality = QMO_STAT_COLUMN_NDV;
            }

            sCardinality *= sOneCardinality;

            sNode = (qtcNode *)(sNode->node.next);
        }
    }
    else
    {
        sNode = aColumnNode;

        if( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
        {
            sCardinality =
                QC_SHARED_TMPLATE(aStatement)->tableMap[sNode->node.table].
                from->tableRef->statInfo->\
                colCardInfo[sNode->node.column].columnNDV;
        }
        else
        {
            sCardinality = QMO_STAT_COLUMN_NDV;
        }
    }

    *aCardinality = sCardinality;

    return IDE_SUCCESS;
}

IDE_RC
qmoSubquery::getSubQTargetCardinality( qcStatement * aStatement,
                                       qtcNode     * aSubQNode,
                                       SDouble     * aCardinality )
{
/***********************************************************************
 *
 * Description :  subquery Target column의 cardinality를 구한다.
 *
 *    store and search, IN subquery keyRange, transform NJ 판단시
 *    필요한 subquery Target cardinality를 구한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SDouble         sCardinality;
    SDouble         sOneCardinality;
    qmsQuerySet   * sQuerySet;
    qmsTarget     * sTarget;
    qtcNode       * sNode;

    IDU_FIT_POINT_FATAL( "qmoSubquery::getSubQTargetCardinality::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSubQNode != NULL );
    IDE_DASSERT( aCardinality != NULL );

    //--------------------------------------
    // subquery target column에 대한 cardinality를 구한다.
    //--------------------------------------

    sQuerySet =  ((qmsParseTree *)(aSubQNode->subquery->myPlan->parseTree))->querySet;

    sCardinality = 1;
    sTarget = sQuerySet->target;

    while( sTarget != NULL )
    {
        sNode = sTarget->targetColumn;
        
        // BUG-20272
        if ( sNode->node.module == &mtfDecrypt )
        {
            sNode = (qtcNode*) sNode->node.arguments;
        }
        else
        {
            // nothing to do
        }

        // BUG-43645 dml where clause subquery recursive with
        if ( ( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE ) &&
             ( ( QC_SHARED_TMPLATE(aStatement)->tableMap[sNode->node.table].
                 from->tableRef->view == NULL ) &&
               ( QC_SHARED_TMPLATE(aStatement)->tableMap[sNode->node.table].
                 from->tableRef->recursiveView == NULL ) ) )
        {
            // target column이 column이고,
            // base table인 경우, 통계정보를 통해 cardinality를 얻는다.
            sOneCardinality =
                QC_SHARED_TMPLATE(aStatement)->tableMap[sNode->node.table].
                from->tableRef->statInfo->\
                colCardInfo[sNode->node.column].columnNDV;
        }
        else
        {
            // target column이 column이 아니거나,
            // subquery가 view인 경우
            sOneCardinality = QMO_STAT_COLUMN_NDV;
        }

        sCardinality *= sOneCardinality;
        sTarget = sTarget->next;
    }

    *aCardinality = sCardinality;

    return IDE_SUCCESS;
}
