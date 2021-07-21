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
 * $Id: qmvQTC.cpp 90335 2021-03-25 07:57:12Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qmvQTC.h>
#include <qcg.h>
#include <qtc.h>
#include <qcmUser.h>
#include <qtcDef.h>
#include <qmsParseTree.h>
#include <qsvProcVar.h>
#include <qsvProcStmts.h>
#include <qsvEnv.h>
#include <qsvCursor.h>
#include <qcuSqlSourceInfo.h>
#include <qdpPrivilege.h>
#include <qcmSynonym.h>
#include <qcgPlan.h>
#include <qds.h>
#include <qdpRole.h>
#include <qmv.h>
#include <qmvGBGSTransform.h>
#include <sdi.h>

extern mtfModule mtfDecrypt;
extern mtfModule qsfConnectByRootModule;
extern mtfModule qsfSysConnectByPathModule;
extern mtdModule mtdClobLocator;
//-------------------------------------------------------------------------//
// This function is called ......
//-------------------------------------------------------------------------//
// case (1) :
// expression in SELECT clause with GROUP BY clause
//-------------------------------------------------------------------------//
// case (2) :
// expression in HAVING clause with GROUP BY clause
//-------------------------------------------------------------------------//
// case (3) :
// expression in ORDER BY clause with GROUP BY clause
//-------------------------------------------------------------------------//
IDE_RC qmvQTC::isGroupExpression(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGH,
    qtcNode         * aExpression,
    idBool            aMakePassNode )
{

    qmsParseTree      * sParseTree;
    qmsConcatElement  * sGroup;
    qmsConcatElement  * sSubGroup;
    qtcNode           * sNode;
    qtcNode           * sPassNode;
    qtcNode           * sListNode;
    qtcOverColumn     * sCurOverColumn;
    idBool              sIsTrue;
    qcDepInfo           sMyDependencies;
    qcDepInfo           sResDependencies;
    idBool              sExistGroupExt;

    IDU_FIT_POINT_FATAL( "qmvQTC::isGroupExpression::__FT__" );

    // for checking outer column reference
    qtc::dependencyClear( & sMyDependencies );
    qtc::dependencyClear( & sResDependencies );
    qtc::dependencySet( aExpression->node.table, & sMyDependencies );

    qtc::dependencyAnd( & aSFWGH->depInfo,
                        & sMyDependencies,
                        & sResDependencies );

    if( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // If HAVING include a subquery,
        //  it can't include outer Column references
        //  unless those references to group Columns
        //      or are used with a set function.
        //  ( Operands in HAVING clause are subject to
        //      the same restrictions as in the select list. )

        // BUG-44777 distinct + subquery + group by 일때 결과가 틀립니다. 
        // distinct 가 있을 경우 subquery 내에 pass 노드를 만들지 않도록 합니다.
        sParseTree = (qmsParseTree *)(aExpression->subquery->myPlan->parseTree);

        if ( aSFWGH->selectType != QMS_DISTINCT )
        {
            IDE_TEST( checkSubquery4IsGroup( aStatement,
                                             aSFWGH,
                                             sParseTree->querySet,
                                             ID_TRUE ) // make pass node
                      != IDE_SUCCESS);
        }
        else
        {
            // BUG-48128
            // DISTINCT 인경우 pass 노드는 만들지 않고,
            // group expression 인지확인해야한다.
            IDE_TEST( checkSubquery4IsGroup( aStatement,
                                             aSFWGH,
                                             sParseTree->querySet,
                                             ID_FALSE ) // make pass node
                      != IDE_SUCCESS);
        }
    }
    else if( ( aExpression->node.lflag & MTC_NODE_FUNCTON_GROUPING_MASK )
             == MTC_NODE_FUNCTON_GROUPING_TRUE )
    {
        /* PROJ-1353
         *  GROUPING과 GROUPING_ID function 은 estimate를 한번 더하면서
         *  isGroupExpression를 수행한다. HAVING에서는 estimate단계에서 모든 일을 마친다.
         */
        if( aSFWGH->validatePhase == QMS_VALIDATE_GROUPBY )
        {
            IDE_TEST( qtc::estimateNodeWithSFWGH( aStatement,
                                                  aSFWGH,
                                                  aExpression )
                      != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }
    }
    else if( ( QTC_IS_AGGREGATE(aExpression) == ID_TRUE ) &&
             ( aExpression->overClause == NULL ) )
    {
        // No error, No action
        // BUG-47744 Nothing to do.
    }
    else if( aExpression->node.module == &(qtc::passModule) )
    {
        // BUG-21677
        // 이미 passNode가 생성된 경우
        // No error, No action
    }
    else if( aExpression->node.module == &(qtc::valueModule) )
    {
        // constant value or host variable
        // No error, No action
    }
    else if ( (aExpression->node.module == &(qtc::spFunctionCallModule)) &&
              (aExpression->node.arguments == NULL) )
    {
        // BUG-39872 Use without arguments PSM
        // No error, No action
    }
    else if( (aExpression->node.module == &(qtc::columnModule) ) &&
             ( QTC_IS_PSEUDO( aExpression ) != ID_TRUE ) &&
             ( qtc::dependencyEqual( & sMyDependencies, & sResDependencies )
               != ID_TRUE ) )
    {
        // outer column reference
        // No error, No action
    }
    else
    {
        sExistGroupExt = ID_FALSE;
        sIsTrue        = ID_FALSE;
        for (sGroup = aSFWGH->group;
             sGroup != NULL;
             sGroup = sGroup->next)
        {
            //-------------------------------------------------------
            // [GROUP BY expression의 Pass Node 생성]
            // 
            // < QMS_GROUPBY_NORMAL일 경우 >
            //
            //     GROUP BY expression이 존재하는 경우, 다음과
            //     같은 제약 조건이 따른다.
            //     - GROUP BY 가 사용될 경우, GROUP BY 상위의
            //       구문들은 GROUP BY의 expression을 포함하거나,
            //       Aggregation이어야 한다.
            //     - 즉, GROUP BY 구문이 존재하는 경우
            //       SELECT target, HAVING 구문, ORDER BY 구문은
            //       위의 조건을 만족하여야 한다.
            // 위와 같은 제약 조건을 검사하고, 이에 대한 처리를
            // 원활히 하기 위해 Pass Node를 생성한다.
            // GROUP BY expression에 대하여 Pass Node를 생성하는 목적은
            // 크게 다음 두 가지로 요약할 수 있다.
            //     - GROUP BY expression의 반복 수행 방지
            //     - GROUP BY expression의 저장 및 변경에 따른 유연한
            //       처리
            //
            // 예를 들어 다음과 같은 질의가 있다고 가정하자.
            //     - SELECT (i1 + 1) * 2 FROM T1 GROUP BY (i1 + 1);
            // 위의 질의에 대하여 Parsing 과정이 완료되면,
            // 다음과 같은 형태로 구성된다.
            //
            //       target -------> [*]
            //                        |
            //                       [+] --> [2]
            //                        |
            //                       [i1] --> [1]
            //
            //       group by -------[+]
            //                        |
            //                       [i1] --> [1]
            //
            // 위와 같은 Parsing 구조에서 GROUP BY 의 제약 조건을
            // 고려하여 Validation 과정에서는 Pass Node를 사용하여
            // 다음과 같은 연결 관계를 생성한다.
            //
            //       target -------> [*]
            //                        |
            //                     [Pass] --> [2]
            //                        |
            //                        |
            //                        V
            //       group by -------[+]
            //                        |
            //                       [i1] --> [1]
            //
            // 위와 같이 구성함으로서 GROUP BY expression의 반복 수행을
            // 방지하며, GROUP BY expression을 저장하거나 변형시킬 때,
            // GROUP BY expression에 대해서 고려하더라고 target에 대하여
            // 별도의 고려 없이 원활하게 처리할 수 있다.
            //
            // < QMS_GROUPBY_NULL일 경우 >
            //
            //     Grouping Sets Transform으로 변형된
            //     QMS_GROUPBY_NULL Type의 경우
            //     QMS_GROUPBY_NORMAL Type의 동일한 Group이 있을경우
            //     PassNode는 QMS_GROUPBY_NORMAL Type의 Group을 바라본다.
            //
            //     QMS_GROUPBY_NULL Type의 Group만 Equivalent할 경우
            //     해당 Node는 Null Value Node로 변형된다.
            //
            //     SELECT i1, i2, i3
            //       FROM t1
            //     GROUP BY GROUPING SETS( i1, i2 ), i1;
            //
            //     target -------> i1 --> i2
            //                     |      ^ Null Value Node로 변형
            //                   [Pass]
            //                     |_______________________________________________
            //                                                                     |
            //                                                                     v
            //     group by ------ i1( QMS_GROUPBY_NULL ), i2( QMS_GROUPBY_NULL ), i1 ( QMS_GROUPBY_NORMAL )
            //     
            //     union all
            //     
            //     target -------> i1 --> i2
            //                     |      |
            //                   [Pass] [Pass]
            //                     |______|__________________________________________
            //                            |________________                          |
            //                                             |                         |
            //                                             v                         v
            //     group by ------ i1( QMS_GROUPBY_NULL ), i2( QMS_GROUPBY_NORMAL ), i1 ( QMS_GROUPBY_NORMAL )
            //

            //     
            //-------------------------------------------------------
            if( sGroup->type == QMS_GROUPBY_NORMAL )
            {
                IDE_TEST(qtc::isEquivalentExpression( aStatement,
                                                      sGroup->arithmeticOrList,
                                                      aExpression,
                                                      &sIsTrue)
                         != IDE_SUCCESS);

                if( sIsTrue == ID_TRUE )
                {
                    /* BUG-43958
                     * 아래와 같은 경우 error가 발생해야한다.
                     * select connect_by_root(i1) from t1 group by connect_by_root(i1);
                     * select sys_connect_by_path(i1, '/') from t1 group by connect_by_root(i1);
                     */
                    IDE_TEST_RAISE( ( aExpression->node.module == &qsfConnectByRootModule ) ||
                                    ( aExpression->node.module == &qsfSysConnectByPathModule ),
                                    ERR_NO_GROUP_EXPRESSION );

                    if( aMakePassNode == ID_TRUE )
                    {
                        IDE_TEST( qtc::makePassNode( aStatement,
                                                     aExpression,
                                                     sGroup->arithmeticOrList,
                                                     & sPassNode )
                                  != IDE_SUCCESS );

                        IDE_DASSERT( aExpression == sPassNode );
                    }
                    else
                    {
                        // Pass Node 생성하지 않음
                    }
                    break;
                }
                else
                {
                    /* BUG-43958
                     * 아래와 같은 경우 error가 발생해야한다.
                     * select connect_by_root(i1) from t1 group by i1;
                     * select sys_connect_by_path(i1, '/') from t1 group by i1;
                     */
                    if ( ( aExpression->node.module == &qsfConnectByRootModule ) ||
                         ( aExpression->node.module == &qsfSysConnectByPathModule ) )
                    {
                        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                               sGroup->arithmeticOrList,
                                                               (qtcNode *)aExpression->node.arguments,
                                                               &sIsTrue)
                                  != IDE_SUCCESS);
                        IDE_TEST_RAISE( sIsTrue == ID_TRUE, ERR_NO_GROUP_EXPRESSION );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            else if ( sGroup->type == QMS_GROUPBY_NULL )
            {
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sGroup->arithmeticOrList,
                                                       aExpression,
                                                       & sIsTrue)
                          != IDE_SUCCESS);

                if ( sIsTrue == ID_TRUE )
                {
                    /* PROJ-2415 Grouping Sets Clause
                     * Grouping Sets Transform에 의해 세팅 된 QMS_GROUPBY_NULL Type의 
                     * Group Expression 과 Target 또는 Having의 Expression 이 Equivalent 할 경우
                     * QMS_GROUPBY_NORMAL Type의 다른 Equivalent Group Expression이 존재 한다면
                     * PassNode를 그 Expression으로 지정하고, 없다면 Null Value Node를 생성해서 대체한다. 
                     */
                    IDE_TEST( qmvGBGSTransform::makeNullOrPassNode( aStatement,
                                                                    aExpression,
                                                                    aSFWGH->group,
                                                                    aMakePassNode )
                              != IDE_SUCCESS );
                    break;
                }
                else
                {
                    // Nothing To Do
                }                
            }
            else
            {
                sExistGroupExt = ID_TRUE;
            }
        }

        /* PROJ-1353 Partial Rollup이나 Cube인경우 Taget의 passNode는 group by 에 있는
         * 컬럼에 passNode가 연결되어야한다. 따라서 group by에 있는 모든 컬럼의 passNode를
         * 연결한 후에 rollup에서 사용된 컬럼을 연결한다.
         */
        if( ( sIsTrue == ID_FALSE ) && ( sExistGroupExt == ID_TRUE ) )
        {
            for( sGroup = aSFWGH->group; sGroup != NULL; sGroup = sGroup->next )
            {
                if( sGroup->type != QMS_GROUPBY_NORMAL )
                {
                    for ( sSubGroup = sGroup->arguments;
                          sSubGroup != NULL;
                          sSubGroup = sSubGroup->next )
                    {
                        if( ( sSubGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                            == MTC_NODE_OPERATOR_LIST )
                        {
                            for( sListNode = (qtcNode *)sSubGroup->arithmeticOrList->node.arguments;
                                 sListNode != NULL;
                                 sListNode = (qtcNode *)sListNode->node.next )
                            {
                                IDE_TEST(qtc::isEquivalentExpression( aStatement,
                                                                      sListNode,
                                                                      aExpression,
                                                                      &sIsTrue)
                                         != IDE_SUCCESS);
                                if( sIsTrue == ID_TRUE )
                                {
                                    /* BUG-43958
                                     * 아래와 같은 경우 error가 발생해야한다.
                                     * select connect_by_root(i1) from t1 group by rollup(i2, connect_by_root(i1));
                                     * select sys_connect_by_path(i1, '/') from t1 group by rollup(i2, connect_by_root(i1);
                                     */
                                    IDE_TEST_RAISE( ( aExpression->node.module == &qsfConnectByRootModule ) ||
                                                    ( aExpression->node.module == &qsfSysConnectByPathModule ),
                                                    ERR_NO_GROUP_EXPRESSION );

                                    if( aMakePassNode == ID_TRUE )
                                    {
                                        IDE_TEST( qtc::makePassNode( aStatement,
                                                                     aExpression,
                                                                     sListNode,
                                                                     &sPassNode )
                                                  != IDE_SUCCESS );

                                        IDE_DASSERT( aExpression == sPassNode );
                                    }
                                    else
                                    {
                                        // Pass Node 생성하지 않음
                                    }
                                    break;
                                }
                                else
                                {
                                    /* BUG-43958
                                     * 아래와 같은 경우 error가 발생해야한다.
                                     * select connect_by_root(i1) from t1 group by rollup(i2, i1);
                                     * select sys_connect_by_path(i1, '/') from t1 group by rollup(i2, i1);
                                     */
                                    if ( ( aExpression->node.module == &qsfConnectByRootModule ) ||
                                         ( aExpression->node.module == &qsfSysConnectByPathModule ) )
                                    {
                                        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                                               sListNode,
                                                                               (qtcNode *)aExpression->node.arguments,
                                                                               &sIsTrue )
                                                  != IDE_SUCCESS);
                                        IDE_TEST_RAISE( sIsTrue == ID_TRUE, ERR_NO_GROUP_EXPRESSION );
                                    }
                                    else
                                    {
                                        /* Nothing to do */
                                    }
                                }
                            }
                            if( sIsTrue == ID_TRUE )
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
                            IDE_TEST(qtc::isEquivalentExpression( aStatement,
                                                                  sSubGroup->arithmeticOrList,
                                                                  aExpression,
                                                                  &sIsTrue)
                                     != IDE_SUCCESS);

                            if( sIsTrue == ID_TRUE )
                            {
                                /* BUG-43958
                                 * 아래와 같은 경우 error가 발생해야한다.
                                 * select connect_by_root(i1) from t1 group by rollup(connect_by_root(i1));
                                 * select sys_connect_by_path(i1, '/') from t1 group by rollup(connect_by_root(i1);
                                 */
                                IDE_TEST_RAISE( ( aExpression->node.module == &qsfConnectByRootModule ) ||
                                                ( aExpression->node.module == &qsfSysConnectByPathModule ),
                                                ERR_NO_GROUP_EXPRESSION );

                                if( aMakePassNode == ID_TRUE )
                                {
                                    IDE_TEST( qtc::makePassNode( aStatement,
                                                                 aExpression,
                                                                 sSubGroup->arithmeticOrList,
                                                                 &sPassNode )
                                              != IDE_SUCCESS );

                                    IDE_DASSERT( aExpression == sPassNode );
                                }
                                else
                                {
                                    // Pass Node 생성하지 않음
                                }

                                break;
                            }
                            else
                            {
                                /* BUG-43958
                                 * 아래와 같은 경우 error가 발생해야한다.
                                 * select connect_by_root(i1) from t1 group by rollup(i1);
                                 * select sys_connect_by_path(i1, '/') from t1 group by rollup(i1);
                                 */
                                if ( ( aExpression->node.module == &qsfConnectByRootModule ) ||
                                     ( aExpression->node.module == &qsfSysConnectByPathModule ) )
                                {
                                    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                                           sSubGroup->arithmeticOrList,
                                                                           (qtcNode *)aExpression->node.arguments,
                                                                           &sIsTrue )
                                              != IDE_SUCCESS );
                                    IDE_TEST_RAISE( sIsTrue == ID_TRUE, ERR_NO_GROUP_EXPRESSION );
                                }
                                else
                                {
                                    /* Nothing to do */
                                }
                            }
                        }
                    }

                    if( sIsTrue == ID_TRUE )
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
        }
        else
        {
            /* Nothing to do */
        }

        if( sGroup == NULL )
        {
            if( aExpression->overClause == NULL )
            {
                //-------------------------------------------------------
                // 일반 expression의 group expression 검사
                //-------------------------------------------------------
                
                IDE_TEST_RAISE(aExpression->node.arguments == NULL,
                               ERR_NO_GROUP_EXPRESSION);
                
                for (sNode = (qtcNode *)(aExpression->node.arguments);
                     sNode != NULL;
                     sNode = (qtcNode *)(sNode->node.next))
                {
                    IDE_TEST(isGroupExpression(aStatement,
                                               aSFWGH,
                                               sNode,
                                               aMakePassNode) 
                             != IDE_SUCCESS);
                }
            }
            else
            {
                //-------------------------------------------------------
                // BUG-27597
                // analytic function의 group expression 검사
                //-------------------------------------------------------
                
                // BUG-34966 Analytic function의 argument도 pass node를 생성한다.
                for (sNode = (qtcNode *)(aExpression->node.arguments);
                     sNode != NULL;
                     sNode = (qtcNode *)(sNode->node.next))
                {
                    IDE_TEST(isGroupExpression(aStatement,
                                               aSFWGH,
                                               sNode,
                                               aMakePassNode)
                             != IDE_SUCCESS);
                }
                
                // partition by column에 대한 expression 검사
                for ( sCurOverColumn = aExpression->overClause->overColumn;
                      sCurOverColumn != NULL;
                      sCurOverColumn = sCurOverColumn->next )
                {
                    // BUG-34966 OVER절의 column들도 pass node를 생성한다.
                    IDE_TEST(isGroupExpression(aStatement,
                                               aSFWGH,
                                               sCurOverColumn->node,
                                               aMakePassNode)
                             != IDE_SUCCESS);
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_GROUP_EXPRESSION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_GROUP_EXPRESSION));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qmvQTC::checkSubquery4IsGroup(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGHofOuterQuery,
    qmsQuerySet     * aQuerySet,
    idBool            aMakePassNode )
{
    qmsSFWGH        * sSFWGH;
    qmsOuterNode    * sOuter;
    qtcNode         * sColumn;
    qcDepInfo         sMyDependencies;
    qcDepInfo         sResDependencies;

    IDU_FIT_POINT_FATAL( "qmvQTC::checkSubquery4IsGroup::__FT__" );

    if( aQuerySet->setOp == QMS_NONE )
    {
        sSFWGH = aQuerySet->SFWGH;

        for (sOuter = sSFWGH->outerColumns;
             sOuter != NULL;
             sOuter = sOuter->next)
        {
            sColumn = sOuter->column;

            qtc::dependencyClear( & sMyDependencies );
            qtc::dependencyClear( & sResDependencies );
            qtc::dependencySet( sColumn->node.table, & sMyDependencies );

            qtc::dependencyAnd( & aSFWGHofOuterQuery->depInfo,
                                & sMyDependencies,
                                & sResDependencies);

            if( qtc::dependencyEqual( & sMyDependencies, & sResDependencies )
                == ID_TRUE )
            {
                IDE_TEST( isGroupExpression( aStatement,
                                             aSFWGHofOuterQuery,
                                             sColumn,
                                             aMakePassNode ) /* BUG-48128 */
                          != IDE_SUCCESS );
            }
        }

        // BUG-48128 scalar suqb에 outer column이 있는경우
        // outerQuery의 grouping method는 sort로 풀리면 안됩니다.
        if ( sSFWGH->outerColumns != NULL )
        {
            aSFWGHofOuterQuery->thisQuerySet->lflag &= ~QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_MASK;
            aSFWGHofOuterQuery->thisQuerySet->lflag |= QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_TEST(checkSubquery4IsGroup(
                     aStatement, aSFWGHofOuterQuery, aQuerySet->left, aMakePassNode )
                 != IDE_SUCCESS);

        IDE_TEST(checkSubquery4IsGroup(
                     aStatement, aSFWGHofOuterQuery, aQuerySet->right, aMakePassNode )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//-------------------------------------------------------------------------//
// This function is called ......
//-------------------------------------------------------------------------//
// case (1) :
// expression in SELECT clause with aggregation function or
//                             with HAVING clause without GROUP BY clause
//-------------------------------------------------------------------------//
// case (2) :
// expression in HAVING clause without GROUP BY clause
//-------------------------------------------------------------------------//
// case (3) :
// exression in ORDER BY clause with aggregation function
//-------------------------------------------------------------------------//
IDE_RC qmvQTC::isAggregateExpression(
    qcStatement * aStatement,
    qmsSFWGH    * aSFWGH,
    qtcNode     * aExpression)
{
    qtcNode         * sNode;
    qmsParseTree    * sParseTree;
    qcDepInfo         sMyDependencies;
    qcDepInfo         sResDependencies;

    IDU_FIT_POINT_FATAL( "qmvQTC::isAggregateExpression::__FT__" );

    if( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // If select list with SET functions include a subquery,
        // it can't include outer Column references
        // unless those references to group Columns
        // or are used with a set function.

        sParseTree = (qmsParseTree *)(aExpression->subquery->myPlan->parseTree);

        IDE_TEST(checkSubquery4IsAggregation(aSFWGH, sParseTree->querySet)
                 != IDE_SUCCESS);
    }
    else if( ( QTC_IS_AGGREGATE(aExpression) == ID_TRUE ) &&
             ( aExpression->overClause == NULL ) )
    {
        // This node is aggregation function node.
        // BUG-47744 Nothing to do.
    }
    else if( aExpression->node.module == &(qtc::passModule) )
    {
        // BUG-21677
        // 이미 passNode가 생성된 경우
        // No error, No action
    }
    else if( aExpression->node.module == &(qtc::valueModule) )
    {
        // constant value or host variable
        // No error, No action
    }
    else if( aExpression->node.module == &(qtc::columnModule) )
    {
        qtc::dependencyClear( & sMyDependencies );
        qtc::dependencyClear( & sResDependencies );
        qtc::dependencySet( aExpression->node.table, & sMyDependencies );

        qtc::dependencyAnd( & aSFWGH->depInfo,
                            & sMyDependencies,
                            & sResDependencies );

        if( qtc::dependencyEqual( & sMyDependencies,
                                  & sResDependencies ) == ID_TRUE )
        {   // This node is column and NOT outer column reference.
            IDE_RAISE(ERR_NO_AGGREGATE_EXPRESSION);
        }

        // BUG-17949
        IDE_TEST_RAISE( QTC_IS_PSEUDO( aExpression ) == ID_TRUE,
                        ERR_NO_AGGREGATE_EXPRESSION );
    }
    else
    {
        for (sNode = (qtcNode *)(aExpression->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(isAggregateExpression(aStatement, aSFWGH, sNode)
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_AGGREGATE_EXPRESSION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_GROUP_EXPRESSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::checkSubquery4IsAggregation(
    qmsSFWGH        * aSFWGHofOuterQuery,
    qmsQuerySet     * aQuerySet)
{
    qmsSFWGH        * sSFWGH;
    qmsOuterNode    * sOuter;
    qtcNode         * sColumn;
    qcDepInfo         sMyDependencies;
    qcDepInfo         sResDependencies;

    IDU_FIT_POINT_FATAL( "qmvQTC::checkSubquery4IsAggregation::__FT__" );

    if( aQuerySet->setOp == QMS_NONE )
    {
        sSFWGH = aQuerySet->SFWGH;

        for (sOuter = sSFWGH->outerColumns;
             sOuter != NULL;
             sOuter = sOuter->next)
        {
            sColumn = sOuter->column;

            qtc::dependencyClear( & sMyDependencies );
            qtc::dependencyClear( & sResDependencies );
            qtc::dependencySet( sColumn->node.table, & sMyDependencies );

            qtc::dependencyAnd( & aSFWGHofOuterQuery->depInfo,
                                & sMyDependencies,
                                & sResDependencies);

            if( qtc::dependencyEqual( & sMyDependencies, & sResDependencies )
                == ID_TRUE )
            {
                IDE_RAISE(ERR_NO_AGGREGATE_EXPRESSION);
            }
        }
    }
    else
    {
        IDE_TEST(checkSubquery4IsAggregation(
                     aSFWGHofOuterQuery, aQuerySet->left)
                 != IDE_SUCCESS);

        IDE_TEST(checkSubquery4IsAggregation(
                     aSFWGHofOuterQuery, aQuerySet->right)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_AGGREGATE_EXPRESSION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_GROUP_EXPRESSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//-------------------------------------------------------------------------//
// This function is called ......
//-------------------------------------------------------------------------//
// case (1) :
// expression in SELECT clause with two depth aggregation function
//-------------------------------------------------------------------------//
IDE_RC qmvQTC::isNestedAggregateExpression(
    qcStatement * aStatement,
    qmsQuerySet * aQuerySet,
    qmsSFWGH    * aSFWGH,
    qtcNode     * aExpression)
{
    qtcNode         * sNode;
    qmsParseTree    * sParseTree;
    qmsAggNode      * sAggr;
    qcDepInfo         sMyDependencies;
    qcDepInfo         sResDependencies;
    idBool            sPassNode;

    IDU_FIT_POINT_FATAL( "qmvQTC::isNestedAggregateExpression::__FT__" );

    if( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // If select list with SET functions include a subquery,
        // it can't include outer Column references
        // unless those are used with a set function.

        sParseTree = (qmsParseTree *)(aExpression->subquery->myPlan->parseTree);

        IDE_TEST(checkSubquery4IsAggregation(aSFWGH, sParseTree->querySet)
                 != IDE_SUCCESS);
    }
    /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
    else if( ( QTC_IS_AGGREGATE(aExpression) == ID_TRUE ) &&
             ( aExpression->overClause == NULL ) )
    {
        // This node is aggregation function node.
        // check Nested Aggregation

        for (sAggr = aSFWGH->aggsDepth2; sAggr != NULL; sAggr = sAggr->next)
        {
            if( ( sAggr->aggr->node.table == aExpression->node.table ) &&
                ( sAggr->aggr->node.column == aExpression->node.column ) )
            {
                // No error, No action
                break;
            }
        }

        if( sAggr == NULL )
        {
            // fix BUG-19561
            if( aExpression->node.arguments != NULL )
            {
                sPassNode = ID_FALSE;
                
                for ( sNode = (qtcNode *)(aExpression->node.arguments);
                      sNode != NULL;
                      sNode = (qtcNode *)(sNode->node.next) )
                {
                    IDE_TEST(isGroupExpression(aStatement,
                                               aSFWGH,
                                               sNode,
                                               ID_TRUE) // make pass node 
                             != IDE_SUCCESS);
                    
                    if( sNode->node.module == &(qtc::passModule) )
                    {
                        sPassNode = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                // fix BUG-19570
                // passnode가 달린 경우에 상위 aggregation노드의
                // estimate를 다시 해주어야 한다.
                if( sPassNode == ID_TRUE )
                {
                    IDE_TEST( qtc::estimateNodeWithArgument(
                                  aStatement,
                                  aExpression)
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // 예) COUNT(*) 는 arguments가 NULL임.
                // Nothing To Do
            }            
        }
    }
    else if( aExpression->node.module == &(qtc::valueModule) )
    {
        // constant value or host variable
        // No error, No action
    }
    else if( aExpression->node.module == &(qtc::columnModule) )
    {
        qtc::dependencyClear( & sMyDependencies );
        qtc::dependencyClear( & sResDependencies );
        qtc::dependencySet( aExpression->node.table, & sMyDependencies );

        qtc::dependencyAnd( & aSFWGH->depInfo,
                            & sMyDependencies,
                            & sResDependencies);

        if( qtc::dependencyEqual( & sMyDependencies, & sResDependencies )
            == ID_TRUE )
        {   // This node is column and NOT outer column reference.
            IDE_RAISE(ERR_NO_AGGREGATE_EXPRESSION);
        }

        // BUG-17949
        IDE_TEST_RAISE( QTC_IS_PSEUDO( aExpression ) == ID_TRUE,
                        ERR_NO_AGGREGATE_EXPRESSION );
    }
    else
    {
        for (sNode = (qtcNode *)(aExpression->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(isNestedAggregateExpression(aStatement,
                                                 aQuerySet,
                                                 aSFWGH,
                                                 sNode)
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_AGGREGATE_EXPRESSION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_GROUP_EXPRESSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//-------------------------------------------------------------------------//
// This function is called ......
//-------------------------------------------------------------------------//
// case (1) :
// expression in HAVING clause with two depth aggregation function
//-------------------------------------------------------------------------//
// case (2) :
// exression in ORDER BY clause with two depth aggregation function
//-------------------------------------------------------------------------//
IDE_RC qmvQTC::haveNotNestedAggregate(
    qcStatement * aStatement,
    qmsSFWGH    * aSFWGH,
    qtcNode     * aExpression)
{
    qtcNode          * sNode = NULL;
    qmsConcatElement * sConcatElement;
    qmsParseTree     * sParseTree;
    qmsAggNode       * sAggr;
    qcDepInfo          sMyDependencies;
    qcDepInfo          sResDependencies;

    IDU_FIT_POINT_FATAL( "qmvQTC::haveNotNestedAggregate::__FT__" );

    if( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // If select list with SET functions include a subquery,
        // it can't include outer Column references
        // unless those are used with a set function.

        sParseTree = (qmsParseTree *)(aExpression->subquery->myPlan->parseTree);

        IDE_TEST(checkSubquery4IsAggregation(aSFWGH, sParseTree->querySet)
                 != IDE_SUCCESS);
    }
    /* BUG-35193  Window function 이 아닌 aggregation 만 처리해야 한다. */
    else if( ( QTC_IS_AGGREGATE(aExpression) == ID_TRUE ) &&
             ( aExpression->overClause == NULL ) )
    {
        // This node is aggregation function node.
        // check Nested Aggregation

        for (sAggr = aSFWGH->aggsDepth2; sAggr != NULL; sAggr = sAggr->next)
        {
            if( ( sAggr->aggr->node.table == aExpression->node.table ) &&
                ( sAggr->aggr->node.column == aExpression->node.column ) )
            {
                IDE_RAISE(ERR_TOO_DEEPLY_NESTED_AGGR);
            }
        }
    }
    else if( aExpression->node.module == &(qtc::valueModule) )
    {
        // constant value or host variable
        // No error, No action
    }
    else if( aExpression->node.module == &(qtc::columnModule) )
    {
        qtc::dependencyClear( & sMyDependencies );
        qtc::dependencyClear( & sResDependencies );
        qtc::dependencySet( aExpression->node.table, & sMyDependencies );

        qtc::dependencyAnd( & aSFWGH->depInfo,
                            & sMyDependencies,
                            & sResDependencies);

        if( qtc::dependencyEqual( & sMyDependencies, & sResDependencies )
            == ID_TRUE )
        {   // This node is column and NOT outer column reference.

//              for (sNode = aSFWGH->group;
//                   sNode != NULL;
//                   sNode = (qtcNode *)(sNode->node.next))
            for (sConcatElement = aSFWGH->group;
                 sConcatElement != NULL;
                 sConcatElement = sConcatElement->next )
            {
                sNode = sConcatElement->arithmeticOrList;

                if( QTC_IS_COLUMN(aStatement, sNode) == ID_TRUE )
                {
                    if( ( sNode->node.table == aExpression->node.table ) &&
                        ( sNode->node.column == aExpression->node.column ) )
                    {
                        // aExpression is group expression.
                        break;
                    }
                }
            }

            if( sNode == NULL )
            {
                IDE_RAISE(ERR_NO_GROUP_EXPRESSION);
            }
        }
    }
    else
    {
        for (sNode = (qtcNode *)(aExpression->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(haveNotNestedAggregate(aStatement, aSFWGH, sNode)
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_DEEPLY_NESTED_AGGR)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_TOO_DEEPLY_NESTED_AGGR));
    }
    IDE_EXCEPTION(ERR_NO_GROUP_EXPRESSION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_GROUP_EXPRESSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//-------------------------------------------------------------------------//
// This funciton is called in case of 'SELECT DISTINCT ... ORDER BY ...'.
//-------------------------------------------------------------------------//
IDE_RC qmvQTC::isSelectedExpression(
    qcStatement     * aStatement,
    qtcNode         * aExpression,
    qmsTarget       * aTarget)
{
    qmsTarget       * sTarget;
    idBool            sIsTrue;
    qtcNode         * sNode;

    IDU_FIT_POINT_FATAL( "qmvQTC::isSelectedExpression::__FT__" );

    if( ( aExpression->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // No error, No action
    }
    else if( aExpression->node.module == &(qtc::valueModule) )
    {
        // constant value or host variable
        // No error, No action
    }
    else
    {
        for (sTarget = aTarget; sTarget != NULL; sTarget = sTarget->next)
        {
            IDE_TEST(qtc::isEquivalentExpression( aStatement,
                                                  sTarget->targetColumn,
                                                  aExpression,
                                                  &sIsTrue)
                     != IDE_SUCCESS);

            if( sIsTrue == ID_TRUE )
            {
                break;
            }
        }

        if( sTarget == NULL )
        {
            IDE_TEST_RAISE(aExpression->node.arguments == NULL,
                           ERR_NO_SELECTED_EXPRESSION_IN_ORDERBY);
            
            for (sNode = (qtcNode *)(aExpression->node.arguments);
                 sNode != NULL;
                 sNode = (qtcNode *)(sNode->node.next))
            {
                IDE_TEST(isSelectedExpression(aStatement, sNode, aTarget)
                         != IDE_SUCCESS);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_SELECTED_EXPRESSION_IN_ORDERBY)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_SELECTED_EXPRESSION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setColumnIDOfOrderBy( qtcNode      * aQtcColumn,
                                     mtcCallBack  * aCallBack,
                                     idBool       * aFindColumn )
{
/***********************************************************************
 *
 * Description : ORDER BY Column에 대하여 ID를 설정
 *
 * PR-8615등의 Target Name과 Order By Name을
 * 비교하여 ID를 설정하는 방법은 해당 함수의
 * 용도에 부합하지 않는 처리방법임.
 *
 * Implementation :
 *
 *  == ORDER BY에 존재하는 Column의 종류 ==
 *
 *     1. 일반 Table의 Column인 경우
 *        - SELECT * FROM T1 ORDER BY i1;
 *        - SELECT * FROM T1, T2 ORDER BY T2.i1;
 *     2. Target의 Alias를 사용하는 경우
 *        Table의 Column은 아니나 Alias를 이용하여
 *        ORDER BY Column을 표현할 수 있음.
 *        - SELECT T1.i1 A FROM T1 ORDER BY A;
 *     3. SET 연산에 대한 ORDER BY인 경우
 *        SET 연산인 경우 Alias Name을 이용하여
 *        처리되어야 함.
 *        - SELECT T1.i1 FROM T1 UNION
 *          SELECT T2.i1 FROM T2
 *          ORDER BY i1;
 *        - SELECT T1.i1 A FROM T1 UNION
 *          SELECT T2.i2 A FROM T2
 *          ORDER BY A;
 *
 ***********************************************************************/

    qtcCallBackInfo     * sCallBackInfo;
    qmsTarget           * sTarget;
    qtcNode             * sTargetColumn;
    qcStatement         * sStatement;
    idBool                sFindColumn;
    qmsQuerySet         * sQuerySetOfCallBack;
    qmsSFWGH            * sSFWGH;
    qmsFrom             * sFrom;
    qcuSqlSourceInfo      sqlInfo;
    qmsTableRef         * sTableRef = NULL;

    IDU_FIT_POINT_FATAL( "qmvQTC::setColumnIDOfOrderBy::__FT__" );

    // 기본 초기화
    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sTargetColumn = NULL;
    sStatement    = sCallBackInfo->statement;
    sQuerySetOfCallBack = sCallBackInfo->querySet;
    sFindColumn   = ID_FALSE;

    //-----------------------------------------------
    // A. ORDER BY Column이 Target의 Alias Name 인지를 판단
    //-----------------------------------------------

    if( QC_IS_NULL_NAME(aQtcColumn->tableName) == ID_TRUE )
    {
        // To Fix PR-8615, PR-8820
        // ORDER BY Column이 Alias인 경우라면,
        // Table Name이 존재하지 않는다.

        for ( sTarget = sQuerySetOfCallBack->target;
              sTarget != NULL;
              sTarget = sTarget->next)
        {
            if( idlOS::strMatch( aQtcColumn->columnName.stmtText +
                                 aQtcColumn->columnName.offset,
                                 aQtcColumn->columnName.size,
                                 sTarget->aliasColumnName.name,
                                 sTarget->aliasColumnName.size ) == 0 )
            {
                // Target의 Alias Name과 동일한 경우
                // Ex) SELECT T1.i1 A FROM T1 ORDER BY A;

                if( sTargetColumn != NULL )
                {
                    // 동일한 Alias Name이 존재하여,
                    // 해당 ORDER BY의 Column을 판단할 수 없는 경우임.
                    // Ex) SELECT T1.i1 A, T1.i2 A FROM T1 ORDER BY A;

                    sqlInfo.setSourceInfo( sStatement,
                                           & aQtcColumn->columnName );
                    IDE_RAISE(ERR_DUPLICATE_ALIAS_NAME);
                }
                else
                {
                    if ( ( aQtcColumn->lflag & QTC_NODE_PRIOR_MASK ) !=
                         ( sTarget->targetColumn->lflag & QTC_NODE_PRIOR_MASK ) )
                    {
                        continue;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    sTargetColumn = sTarget->targetColumn;

                    // PROJ-2002 Column Security
                    // 보안 컬럼인 경우 target에 decrypt함수를 붙였으므로
                    // decrypt함수의 arguments가 실제 target이다.
                    if( sTargetColumn->node.module == &mtfDecrypt )
                    {
                        sTargetColumn = (qtcNode*)
                            sTargetColumn->node.arguments;
                    }
                    else
                    {
                        // PROJ-2179 ORDER BY절에서 참조되었음을 표시
                        sTarget->flag &= ~QMS_TARGET_ORDER_BY_MASK;
                        sTarget->flag |= QMS_TARGET_ORDER_BY_TRUE;
                    }
                    
                    // BUG-27597
                    // pass node의 arguments가 실제 target이다.
                    if( sTargetColumn->node.module == &qtc::passModule )
                    {
                        sTargetColumn = (qtcNode*)
                            sTargetColumn->node.arguments;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    // set target position
                    aQtcColumn->node.table = sTargetColumn->node.table;
                    aQtcColumn->node.column = sTargetColumn->node.column;

                    // set base table and column ID
                    aQtcColumn->node.baseTable = sTargetColumn->node.baseTable;
                    aQtcColumn->node.baseColumn = sTargetColumn->node.baseColumn;

                    aQtcColumn->node.lflag |= sTargetColumn->node.lflag;
                    aQtcColumn->node.arguments =
                        sTargetColumn->node.arguments;

                    // To fix BUG-20876
                    // function이면서 인자가 없는 경우 마치 단말
                    // 컬럼노드처럼 인지되어 order by컬럼의
                    // estimate시에 mtcExecute함수세팅이
                    // columnModule로 된다.
                    // 따라서, target노드의 module을 assign한다.
                    aQtcColumn->node.module = sTargetColumn->node.module;

                    // BUG-15756
                    aQtcColumn->lflag |= sTargetColumn->lflag;

                    // BUG-44518 order by 구문의 ESTIMATE 중복 수행하면 안됩니다.
                    aQtcColumn->lflag &= ~QTC_NODE_ORDER_BY_ESTIMATE_MASK;
                    aQtcColumn->lflag |= QTC_NODE_ORDER_BY_ESTIMATE_TRUE;

                    // fix BUG-25159
                    // select target절에 사용된 subquery를
                    // orderby에서 alias로 참조하여 연산시 서버 비정상종료.
                    aQtcColumn->subquery = sTargetColumn->subquery;

                    /* BUG-32102
                     * target 에서 over 절을 사용하고 orderby 에서 alias로 참조 결과 틀림
                     */
                    aQtcColumn->overClause = sTargetColumn->overClause;

                    // PROJ-2002 Column Security
                    // dependency 정보 설정
                    qtc::dependencySetWithDep( &aQtcColumn->depInfo, 
                                               &sTargetColumn->depInfo );

                    sFindColumn = ID_TRUE;
                }
            }
        }
    }
    else
    {
        // Nothing To Do
        // ORDER Column이 Alias가 확실히 아닌 경우임.
    }

    if( sQuerySetOfCallBack->setOp != QMS_NONE )
    {
        //-----------------------------------------------
        // B. SET 인 경우의 ORDER BY
        //-----------------------------------------------

        // in case of SELECT ... UNION SELECT ... ORDER BY COLUMN_NAME
        // A COLUMN_NAME should be alias name of target.

        if( sTargetColumn == NULL )
        {
            // SET 에 대한 ORDER BY인 경우
            // ORDER BY는 반드시 Alias Name에 의하여
            // 해당 Column이 검사되어야 한다.
            // Ex) SELECT T1.i1 FROM T1 UNION
            //     SELECT T2.i1 FROM T2
            //     ORDER BY T1.i1;

            sqlInfo.setSourceInfo(
                sStatement,
                & aQtcColumn->columnName );
            IDE_RAISE(ERR_NOT_EXIST_ALIAS_NAME);
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        //-----------------------------------------------
        // C. 일반 Table Column의 ORDER BY Column인지를 검사
        //-----------------------------------------------

        if( sTargetColumn == NULL )
        {
            sSFWGH = sQuerySetOfCallBack->SFWGH;

            for (sFrom = sSFWGH->from;
                 sFrom != NULL;
                 sFrom = sFrom->next)
            {
                IDE_TEST(searchColumnInFromTree( sStatement,
                                                 sCallBackInfo->SFWGH,
                                                 aQtcColumn,
                                                 sFrom,
                                                 &sTableRef)
                         != IDE_SUCCESS);
            }

            // BUG-41221 search outer columns
            if( sTableRef == NULL )
            {
                IDE_TEST( searchColumnInOuterQuery(
                              sStatement,
                              sCallBackInfo->SFWGH,
                              aQtcColumn,
                              &sTableRef,
                              &sSFWGH )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            if( sTableRef == NULL )
            {
                sFindColumn = ID_FALSE;
            }
            else
            {
                sFindColumn = ID_TRUE;
            }
        }
        else
        {
            // PROJ-1413
            // view 컬럼 참조 노드를 등록한다.
            IDE_TEST( addViewColumnRefList( sStatement,
                                            aQtcColumn )
                      != IDE_SUCCESS );
        }
    }

    *aFindColumn = sFindColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUPLICATE_ALIAS_NAME)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_DUPLICATE_ALIAS,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_ALIAS_NAME)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_ALIAS,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setColumnIDForInsert( qtcNode      * aQtcColumn,
                                     mtcCallBack  * aCallBack,
                                     idBool       * aFindColumn )
{
/***********************************************************************
 *
 * Description : BUG-36596 multi-table insert
 *     multi-table insert에서는 subquery의 target 컬럼만 참조할 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcCallBackInfo     * sCallBackInfo;
    qmsTarget           * sTarget;
    qtcNode             * sTargetColumn;
    qcStatement         * sStatement;
    idBool                sFindColumn;
    qmsQuerySet         * sQuerySetOfCallBack;
    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::setColumnIDForInsert::__FT__" );

    // 기본 초기화
    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sTargetColumn = NULL;
    sStatement    = sCallBackInfo->statement;
    sQuerySetOfCallBack = sCallBackInfo->querySet;
    sFindColumn   = ID_FALSE;

    //-----------------------------------------------
    // Column이 Target의 Alias Name 인지를 판단
    //-----------------------------------------------

    if( QC_IS_NULL_NAME(aQtcColumn->tableName) == ID_TRUE )
    {
        // To Fix PR-8615, PR-8820
        // ORDER BY Column이 Alias인 경우라면,
        // Table Name이 존재하지 않는다.

        for ( sTarget = sQuerySetOfCallBack->target;
              sTarget != NULL;
              sTarget = sTarget->next)
        {
            if( idlOS::strMatch( aQtcColumn->columnName.stmtText +
                                 aQtcColumn->columnName.offset,
                                 aQtcColumn->columnName.size,
                                 sTarget->aliasColumnName.name,
                                 sTarget->aliasColumnName.size ) == 0 )
            {
                // Target의 Alias Name과 동일한 경우
                // Ex) SELECT T1.i1 A FROM T1 ORDER BY A;

                if( sTargetColumn != NULL )
                {
                    // 동일한 Alias Name이 존재하여,
                    // 해당 ORDER BY의 Column을 판단할 수 없는 경우임.
                    // Ex) SELECT T1.i1 A, T1.i2 A FROM T1 ORDER BY A;

                    sqlInfo.setSourceInfo( sStatement,
                                           & aQtcColumn->columnName );
                    IDE_RAISE(ERR_DUPLICATE_ALIAS_NAME);
                }
                else
                {
                    sTargetColumn = sTarget->targetColumn;

                    // PROJ-2002 Column Security
                    // 보안 컬럼인 경우 target에 decrypt함수를 붙였으므로
                    // decrypt함수의 arguments가 실제 target이다.
                    if( sTargetColumn->node.module == &mtfDecrypt )
                    {
                        sTargetColumn = (qtcNode*)
                            sTargetColumn->node.arguments;
                    }
                    else
                    {
                        // PROJ-2179 ORDER BY절에서 참조되었음을 표시
                        sTarget->flag &= ~QMS_TARGET_ORDER_BY_MASK;
                        sTarget->flag |= QMS_TARGET_ORDER_BY_TRUE;
                    }
                    
                    // BUG-27597
                    // pass node의 arguments가 실제 target이다.
                    if( sTargetColumn->node.module == &qtc::passModule )
                    {
                        sTargetColumn = (qtcNode*)
                            sTargetColumn->node.arguments;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    // set target position
                    aQtcColumn->node.table = sTargetColumn->node.table;
                    aQtcColumn->node.column = sTargetColumn->node.column;

                    // set base table and column ID
                    aQtcColumn->node.baseTable = sTargetColumn->node.baseTable;
                    aQtcColumn->node.baseColumn = sTargetColumn->node.baseColumn;

                    aQtcColumn->node.lflag |= sTargetColumn->node.lflag;
                    aQtcColumn->node.arguments =
                        sTargetColumn->node.arguments;

                    // To fix BUG-20876
                    // function이면서 인자가 없는 경우 마치 단말
                    // 컬럼노드처럼 인지되어 order by컬럼의
                    // estimate시에 mtcExecute함수세팅이
                    // columnModule로 된다.
                    // 따라서, target노드의 module을 assign한다.
                    aQtcColumn->node.module = sTargetColumn->node.module;

                    // BUG-15756
                    aQtcColumn->lflag |= sTargetColumn->lflag;

                    // fix BUG-25159
                    // select target절에 사용된 subquery를
                    // orderby에서 alias로 참조하여 연산시 서버 비정상종료.
                    aQtcColumn->subquery = sTargetColumn->subquery;

                    /* BUG-32102
                     * target 에서 over 절을 사용하고 orderby 에서 alias로 참조 결과 틀림
                     */
                    aQtcColumn->overClause = sTargetColumn->overClause;

                    // PROJ-2002 Column Security
                    // dependency 정보 설정
                    qtc::dependencySetWithDep( &aQtcColumn->depInfo, 
                                               &sTargetColumn->depInfo );

                    sFindColumn = ID_TRUE;
                }
            }
        }
    }
    else
    {
        // Nothing To Do
        // Column이 Alias가 확실히 아닌 경우임.
    }

    *aFindColumn = sFindColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUPLICATE_ALIAS_NAME)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_DUPLICATE_ALIAS,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setColumnID( qtcNode      * aQtcColumn,
                            mtcTemplate  * aTemplate,
                            mtcStack     * aStack,
                            SInt           aRemain,
                            mtcCallBack  * aCallBack,
                            qsVariables ** aArrayVariable,
                            idBool       * aIdcFlag,
                            qmsSFWGH    ** aColumnSFWGH )
{
/***********************************************************************
 *
 * Description :
 *    Validation 과정에서 COLUMN형 Expression Node의
 *    ID( table, column )를 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSFWGH            * sSFWGH;
    qmsFrom             * sFrom;
    qmsTableRef         * sTableRef = NULL;
    idBool                sFindColumn = ID_FALSE;
    qsCursors           * sCursorDef;
    qcuSqlSourceInfo      sqlInfo;

    qtcCallBackInfo     * sCallBackInfo;

    qcStatement     * sStatement;
    qmsQuerySet     * sQuerySetOfCallBack;
    qmsSFWGH        * sSFWGHOfCallBack;
    qmsFrom         * sFromOfCallBack;

    // PROJ-2415 Grouping Sets Clause
    idBool                sIsFound = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvQTC::setColumnID::__FT__" );

    sCallBackInfo        = (qtcCallBackInfo*)(aCallBack->info);
    sStatement           = sCallBackInfo->statement;
    sQuerySetOfCallBack  = sCallBackInfo->querySet;
    sSFWGHOfCallBack     = sCallBackInfo->SFWGH;
    sFromOfCallBack      = sCallBackInfo->from;

    *aArrayVariable = NULL;

    //---------------------------------------
    // search pseudo column
    //---------------------------------------

    if( QC_IS_NULL_NAME(aQtcColumn->tableName) == ID_TRUE )
    {
        // BUG-34231
        // double-quoted identifier는 pseudo column으로 사용될 수 없다.
        
        if( qtc::isQuotedName(&(aQtcColumn->columnName)) == ID_FALSE )
        {
            /* check SYSDATE, UNIX_DATE, CURRENT_DATE */
            IDE_TEST( searchDatePseudoColumn( sStatement, aQtcColumn, &sFindColumn )
                      != IDE_SUCCESS );

            if( sFindColumn == ID_FALSE )
            {
                // check LEVEL
                IDE_TEST(searchLevel(
                             sStatement, sSFWGHOfCallBack, aQtcColumn, &sFindColumn)
                         != IDE_SUCCESS);
            }
            else
            {
                /**
                 * PROJ-2462 Result Cache
                 * SysDate 관련 Pseudo Column이 포함되면 Temp Cache를 사용하지
                 * 못한다.
                 */
                if ( sQuerySetOfCallBack != NULL )
                {
                    sQuerySetOfCallBack->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
                    sQuerySetOfCallBack->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            // PROJ-1405
            if( sFindColumn == ID_FALSE )
            {
                // check ROWNUM
                IDE_TEST(searchRownum(
                             sStatement, sSFWGHOfCallBack, aQtcColumn, &sFindColumn)
                         != IDE_SUCCESS);
            }
            else
            {
                // Nothing to do.
            }

            /* PROJ-1715 */
            if( sFindColumn == ID_FALSE )
            {
                IDE_TEST(searchConnectByIsLeaf(
                             sStatement, sSFWGHOfCallBack, aQtcColumn, &sFindColumn)
                         != IDE_SUCCESS);
            }
            else
            {
                // Nothing to do.
            }

            // BUG-41311 table function
            if ( sFindColumn == ID_FALSE )
            {
                IDE_TEST( searchLoopLevel(
                              sStatement, sSFWGHOfCallBack, aQtcColumn, &sFindColumn )
                          != IDE_SUCCESS );                
            }
            else
            {
                // Nothing to do.
            }
            
            if ( sFindColumn == ID_FALSE )
            {
                IDE_TEST( searchLoopValue(
                              sStatement, sSFWGHOfCallBack, aQtcColumn, &sFindColumn )
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
    
    //---------------------------------------
    // search table column
    //---------------------------------------
    
    if( sFindColumn == ID_FALSE )
    {
        sSFWGH = sSFWGHOfCallBack;
    
        if( sQuerySetOfCallBack != NULL ) // columns in ORDER BY clause
        {
            // SELECT 구문인 경우
            switch ( sQuerySetOfCallBack->processPhase )
            {
                case QMS_VALIDATE_ORDERBY :
                {
                    // columns in ORDER BY clause
                    IDE_TEST( setColumnIDOfOrderBy( aQtcColumn,
                                                    aCallBack,
                                                    & sFindColumn )
                              != IDE_SUCCESS );

                    /* TASK-7219 */
                    if ( sFindColumn != ID_TRUE )
                    {
                        IDE_TEST( setColumnIDOfOrderByForShard( sStatement,
                                                                sSFWGH,
                                                                aQtcColumn,
                                                                & ( sFindColumn ) )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;
                }
                case QMS_VALIDATE_FROM :
                {
                    // columns in condition in FROM clause
                    IDE_DASSERT( sFromOfCallBack != NULL );

                    sFrom = sFromOfCallBack;
                
                    IDE_TEST(searchColumnInFromTree( sStatement,
                                                     sSFWGHOfCallBack,
                                                     aQtcColumn,
                                                     sFrom,
                                                     &sTableRef)
                             != IDE_SUCCESS);
                
                    if( sTableRef == NULL )
                    {
                        sFindColumn = ID_FALSE;
                    }
                    else
                    {
                        sFindColumn = ID_TRUE;
                    }
                    break;
                }
                case QMS_VALIDATE_INSERT :
                {
                    // columns in subquery target
                    IDE_TEST( setColumnIDForInsert( aQtcColumn,
                                                    aCallBack,
                                                    & sFindColumn )
                              != IDE_SUCCESS );
                    break;
                }
                default :
                {
                    // general cases
                    if( sSFWGHOfCallBack != NULL )
                    {
                        // PROJ-2415 Grouping Sets Clause
                        // Grouping Sets Transform 으로 생성 된 하위 inLineView의 Target의 순서에 맞게
                        // table 및 column을 세팅한다.
                        if ( ( ( sSFWGHOfCallBack->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                               == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) ||
                             ( ( sSFWGHOfCallBack->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                               == QMV_SFWGH_GBGS_TRANSFORM_BOTTOM ) )                            
                        {
                            IDE_TEST( setColumnIDForGBGS( sStatement,
                                                          sSFWGHOfCallBack,
                                                          aQtcColumn,
                                                          &sIsFound,
                                                          &sTableRef )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        if ( ( sIsFound != ID_TRUE ) &&
                             ( ( sSFWGHOfCallBack->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                               != QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) )
                        {
                            // PROJ-2687 Shard aggregation transform
                            if ( ( sSFWGHOfCallBack->lflag & QMV_SFWGH_SHARD_TRANS_VIEW_MASK )
                                 == QMV_SFWGH_SHARD_TRANS_VIEW_TRUE )
                            {
                                IDE_TEST( setColumnIDForShardTransView( sStatement,
                                                                        sSFWGHOfCallBack,
                                                                        aQtcColumn,
                                                                        &sIsFound,
                                                                        &sTableRef )
                                          != IDE_SUCCESS );
                            }
                            else
                            {
                                // TASK-7219 Non-shard DML
                                if ( aQtcColumn->shardViewTargetPos != ID_USHORT_MAX )
                                {
                                    aQtcColumn->node.table      = sSFWGHOfCallBack->from->tableRef->table;
                                    aQtcColumn->node.baseTable  = sSFWGHOfCallBack->from->tableRef->table;
                                    aQtcColumn->node.column     = aQtcColumn->shardViewTargetPos;
                                    aQtcColumn->node.baseColumn = aQtcColumn->shardViewTargetPos;

                                    sIsFound = ID_TRUE;
                                    sTableRef = sSFWGHOfCallBack->from->tableRef;
                                }
                                else
                                {
                                    for (sFrom = sSFWGHOfCallBack->from;
                                         sFrom != NULL;
                                         sFrom = sFrom->next)
                                    {
                                        IDE_TEST(searchColumnInFromTree( sStatement,
                                                                         sSFWGHOfCallBack,
                                                                         aQtcColumn,
                                                                         sFrom,
                                                                         &sTableRef)
                                                 != IDE_SUCCESS);
                                    }
                                }
                            }
                        }
                        else
                        {
                            /* Nothing to do */
                        }

                        // search outer columns
                        if( sTableRef == NULL )
                        {
                            IDE_TEST( searchColumnInOuterQuery(
                                          sStatement,
                                          sSFWGHOfCallBack,
                                          aQtcColumn,
                                          &sTableRef,
                                          &sSFWGH )
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
                
                    if ( ( sTableRef == NULL ) && ( sIsFound == ID_FALSE ) )
                    {
                        sFindColumn = ID_FALSE;
                    }
                    else
                    {
                        sFindColumn = ID_TRUE;
                    }
                    break;
                }
            }
        }
        else
        {
            // Insert, Update, Delete 일 경우
            if( sFromOfCallBack != NULL )
            {
                // columns in condition in FROM clause
                sFrom = sFromOfCallBack;

                IDE_TEST(searchColumnInFromTree( sStatement,
                                                 sSFWGHOfCallBack,
                                                 aQtcColumn,
                                                 sFrom,
                                                 &sTableRef)
                         != IDE_SUCCESS);
            }
            else
            {
                // general cases
                if( sSFWGHOfCallBack != NULL )
                {
                    for (sFrom = sSFWGHOfCallBack->from;
                         sFrom != NULL;
                         sFrom = sFrom->next)
                    {
                        IDE_TEST(searchColumnInFromTree( sStatement,
                                                         sSFWGHOfCallBack,
                                                         aQtcColumn,
                                                         sFrom,
                                                         &sTableRef)
                                 != IDE_SUCCESS);
                    }
                
                    // search outer columns
                    if( sTableRef == NULL )
                    {
                        IDE_TEST( searchColumnInOuterQuery(
                                      sStatement,
                                      sSFWGHOfCallBack,
                                      aQtcColumn,
                                      &sTableRef,
                                      &sSFWGH )
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

            if( sTableRef == NULL )
            {
                sFindColumn = ID_FALSE;
            }
            else
            {
                sFindColumn = ID_TRUE;
            }
        }

        if( sFindColumn == ID_TRUE )
        {
            *aColumnSFWGH = sSFWGH;
        }
        else
        {
            *aColumnSFWGH = NULL;
        }
    }
    
    if( ( sStatement->spvEnv->createProc != NULL ) ||
        ( sStatement->spvEnv->createPkg != NULL ) )
    {
        // fix BUG-18813
        // 테이블에 존재하는 컬럼명으로 array index variable을 쓴 경우에
        // 대해서도 체크해야 한다.
        if( (sFindColumn == ID_FALSE) ||
            ( (sFindColumn == ID_TRUE) &&
              (((aQtcColumn->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK)
               == QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST) ) ) 
        {
            // To Fix PR-11391
            // Internal Procedure Variable은 procedure variable에 속하는지
            // 체크하지 않음

            if( (aQtcColumn->lflag & QTC_NODE_INTERNAL_PROC_VAR_MASK)
                == QTC_NODE_INTERNAL_PROC_VAR_EXIST )
            {
                // Internal Procedure Variable인 경우
                sFindColumn = ID_TRUE;
            }
            else
            {
                // Internal Procedure Variable이 아닌 경우
                IDE_TEST(qsvProcVar::searchVarAndPara(
                             sStatement,
                             aQtcColumn,
                             ID_FALSE,
                             &sFindColumn,
                             aArrayVariable)
                         != IDE_SUCCESS);

                if( sFindColumn == ID_FALSE )
                {
                    IDE_TEST( qsvProcVar::searchVariableFromPkg(
                                  sStatement,
                                  aQtcColumn,
                                  &sFindColumn,
                                  aArrayVariable )
                              != IDE_SUCCESS )
                        }

                /* PROJ-2197 PSM Renewal */
                if( sFindColumn == ID_TRUE )
                {
                    IDE_TEST( qsvProcStmts::makeUsingParam( *aArrayVariable,
                                                            aQtcColumn,
                                                            aCallBack )
                              != IDE_SUCCESS );
                }
            }

            if( sFindColumn == ID_TRUE )
            {
                // To Fix PR-8486
                // Procedure Variable이 존재함을 표시함.
                aQtcColumn->lflag &= ~QTC_NODE_PROC_VAR_MASK;
                aQtcColumn->lflag |= QTC_NODE_PROC_VAR_EXIST;

                *aIdcFlag = ID_TRUE;
            }

            if ( sFindColumn == ID_FALSE )
            {
                if(qsvCursor::getCursorDefinition(
                       sStatement,
                       aQtcColumn,
                       & sCursorDef) == IDE_SUCCESS)
                {
                    aQtcColumn->node.lflag &= ~MTC_NODE_DML_MASK;
                    aQtcColumn->node.lflag |= MTC_NODE_DML_UNUSABLE;

                    *aIdcFlag = ID_TRUE;
                    sFindColumn = ID_TRUE;
                }
                else
                {
                    IDE_CLEAR();
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
 
    // PROJ-1386 Dynamic-SQL
    // Ref Cursor를 Result Set으로 하기 위해 Internal
    // Procedure Variable을 생성함.
    if( sFindColumn == ID_FALSE )
    {
        if( (aQtcColumn->lflag & QTC_NODE_INTERNAL_PROC_VAR_MASK)
            == QTC_NODE_INTERNAL_PROC_VAR_EXIST )
        {
            // Internal Procedure Variable인 경우
            // Procedure Variable이 존재함을 표시함.
            aQtcColumn->lflag &= ~QTC_NODE_PROC_VAR_MASK;
            aQtcColumn->lflag |= QTC_NODE_PROC_VAR_EXIST;
            sFindColumn = ID_TRUE;
        }
    }

    if( sFindColumn == ID_FALSE )
    {
        if( QC_IS_NULL_NAME(aQtcColumn->tableName) == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            /*
             * BUG-30424: service 이전 단계에서 perf view에 대한 질의문 수행시
             *            from 절에는 alias name이 없고 select list에는
             *            alias name이 있으면 서버 비정상 종료
             *
             * searchSequence() 는 meta 초기화 이후에 수행 가능하다
             * meta 초기화는 startup service phase 초기화 중에 한다
             */
            if( qcg::isInitializedMetaCaches() == ID_TRUE )
            {
                // check SEQUENCE.CURRVAL, SEQUENCE.NEXTVAL
                IDE_TEST(searchSequence( sStatement, aQtcColumn, &sFindColumn)
                         != IDE_SUCCESS);

                // To fix BUG-17908
                // sequence는 SQL내에서만 접근이 가능함.
                // SQL로 볼 수 있음.
                
                // PROJ-2210
                // create, alter table(SCHEMA DDL)에서 시퀀스 접근을 가능하게 한다.
                if ( sFindColumn == ID_TRUE )
                {
                    if ( ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
                           != QCI_STMT_MASK_DML ) &&
                         ( sStatement->myPlan->parseTree->stmtKind != QCI_STMT_SCHEMA_DDL ) )
                    {
                        sqlInfo.setSourceInfo( sStatement,
                                               & aQtcColumn->position );
                        
                        IDE_RAISE( ERR_NOT_ALLOWED_DATABASE_OBJECTS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    /**
                     * PROJ-2462 Result Cache
                     * Sequence 관련 Pseudo Column이 포함되면 Temp Cache를 사용하지 못한다.
                     */
                    if ( sQuerySetOfCallBack != NULL )
                    {
                        sQuerySetOfCallBack->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
                        sQuerySetOfCallBack->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
                    }
                    else
                    {
                        /* Nothing to do */
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
        }
    }

    // PROJ-1073 Package
    // exec println(pkg1.v1); 의 경우
    // default value가 pkg1.v1과 같은 경우
    // spvEnv->createProc, spvEnv->createPkg가 모두 NULL일 수 있다.
    if( sFindColumn == ID_FALSE )
    {
        if( qcg::isInitializedMetaCaches() == ID_TRUE ) 
        {
            IDE_TEST( qsvProcVar::searchVariableFromPkg(
                          sStatement,
                          aQtcColumn,
                          &sFindColumn,
                          aArrayVariable )
                      != IDE_SUCCESS );
      
            if( sFindColumn == ID_TRUE )
            {
                aQtcColumn->lflag &= ~QTC_NODE_PROC_VAR_MASK;
                aQtcColumn->lflag |= QTC_NODE_PROC_VAR_EXIST;

                *aIdcFlag   = ID_TRUE;
            }
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

    // search stored function : ex) select func + i1 from t1
    // PROJ-2533 arrayVar(1) 인 경우는 확인 할 필요가 없다.
    if ( ( sFindColumn == ID_FALSE ) &&
         ( ( (aQtcColumn->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
           QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT ) )
    {
        // BUG-30514 Meta Cache는 초기화되었지만 Service Phase 완료되기 전
        // 이 함수가 불릴 수 있다.
        // ex) 다른 PSM을 호출하는 PSM을 load 시
        if( qcg::isInitializedMetaCaches() == ID_TRUE ) 
        {
            IDE_TEST( qtc::changeNodeFromColumnToSP( sStatement,
                                                     aQtcColumn,
                                                     aTemplate,
                                                     aStack,
                                                     aRemain,
                                                     aCallBack,
                                                     &sFindColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sFindColumn == ID_FALSE )
    {
        sqlInfo.setSourceInfo(sStatement, & aQtcColumn->columnName);
        IDE_RAISE(ERR_NOT_EXIST_COLUMN);
    }
    else
    {
        // fix BUG-18813
        // array index variable이 사용할수 있는 object는
        // procedure/function 및 package이다.
        if( ( sStatement->spvEnv->createProc == NULL ) &&
            ( sStatement->spvEnv->createPkg == NULL ) &&
            ( ( (aQtcColumn->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK )
              == QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ) &&
            ( ( (aQtcColumn->lflag) & QTC_NODE_PROC_VAR_MASK )
              != QTC_NODE_PROC_VAR_EXIST ) )
        {
            sqlInfo.setSourceInfo( sStatement,
                                   & aQtcColumn->columnName );

            IDE_RAISE( ERR_NOT_EXIST_COLUMN );
        }
        else
        {
            // Nothing to do
        }
    }

    if( *aIdcFlag == ID_FALSE )
    {
        // PROJ-1362
        aQtcColumn->lflag &= ~QTC_NODE_BINARY_MASK;

        if( qtc::isEquiValidType( aQtcColumn, aTemplate ) == ID_FALSE )
        {
            aQtcColumn->lflag |= QTC_NODE_BINARY_EXIST;
        }
        else
        {
            // Nothing to do.
        }
    }

    // PROJ-2462 Result Cache
    if ( ( aQtcColumn->lflag & QTC_NODE_LOB_COLUMN_MASK )
         == QTC_NODE_LOB_COLUMN_EXIST )
    {
        if ( sQuerySetOfCallBack != NULL )
        {
            sQuerySetOfCallBack->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
            sQuerySetOfCallBack->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_DATABASE_OBJECTS)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_DATABASE_OBJECTS,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setColumnIDForGBGS( qcStatement  * aStatement,
                                   qmsSFWGH     * aSFWGHOfCallBack,
                                   qtcNode      * aQtcColumn,
                                   idBool       * aIsFound,
                                   qmsTableRef ** aTableRef )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2415 Grouping Sets Clause
 *    Grouping Sets Transform으로 생성된 하위 inLineView의 Target의 순서에 맞게
 *    table 및 column을 세팅한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmsSFWGH            * sSFWGH = aSFWGHOfCallBack;
    qmsParseTree        * sChildParseTree;
    qmsTarget           * sAliasTarget;
    qmsTarget           * sChildTarget;
    qmsFrom             * sChildFrom;
    qtcNode             * sAliasColumn = NULL;    
    UShort                sColumnPosition = 0;    

    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::setColumnIDForGBGS::__FT__" );

    // 1. OrderBy로 인해 Target에 추가된 Node일 경우
    //    앞의 Target들과 비교하여 Alias인지 먼저 확인한다.
    if ( ( ( aQtcColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK ) ==
           QTC_NODE_GBGS_ORDER_BY_NODE_TRUE ) &&
         ( QC_IS_NULL_NAME( aQtcColumn->tableName ) == ID_TRUE ) )
    {
        for ( sAliasTarget  = sSFWGH->target;
              sAliasTarget != NULL;
              sAliasTarget  = sAliasTarget->next )
        {
            // OrderBy 에 의해 추가 된 Target Node의 Alias와는 비교하지 않는다.
            if ( ( sAliasTarget->targetColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK ) ==
                 QTC_NODE_GBGS_ORDER_BY_NODE_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            if ( idlOS::strMatch( aQtcColumn->columnName.stmtText +
                                  aQtcColumn->columnName.offset,
                                  aQtcColumn->columnName.size,
                                  sAliasTarget->aliasColumnName.name,
                                  sAliasTarget->aliasColumnName.size ) == 0 )
            {
                if ( sAliasColumn != NULL )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & aQtcColumn->columnName );
                    IDE_RAISE( ERR_DUPLICATE_ALIAS_NAME );
                }
                else
                {
                    sAliasColumn = sAliasTarget->targetColumn;
                                         
                    // set target position
                    aQtcColumn->node.table      = sAliasColumn->node.table;
                    aQtcColumn->node.column     = sAliasColumn->node.column;

                    // set base table and column ID
                    aQtcColumn->node.baseTable  = sAliasColumn->node.baseTable;
                    aQtcColumn->node.baseColumn = sAliasColumn->node.baseColumn;
                    aQtcColumn->node.lflag     |= sAliasColumn->node.lflag;
                    aQtcColumn->node.arguments  = sAliasColumn->node.arguments;

                    aQtcColumn->node.module     = sAliasColumn->node.module;
                    aQtcColumn->lflag          |= sAliasColumn->lflag;
                    aQtcColumn->subquery        = sAliasColumn->subquery;
                    aQtcColumn->overClause      = sAliasColumn->overClause;

                    // 동일한 QuerySet의 Target에서 찾았기 때문에
                    // Dependency정보를 복사한다.
                    qtc::dependencySetWithDep( &aQtcColumn->depInfo, 
                                               &sAliasColumn->depInfo );
                    *aIsFound = ID_TRUE;
                }
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
                            
    // 2. Target에 추가된 Order By의 Node가 아니거나 Alias로 찾지 못했다면
    //    하위 inLineView에서 찾아 table, column 정보를 재조정한다.
    if ( ( *aIsFound == ID_FALSE  ) &&
         ( ( aSFWGHOfCallBack->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
           == QMV_SFWGH_GBGS_TRANSFORM_MIDDLE )
         )        
    {
        sChildParseTree = ( qmsParseTree * )sSFWGH->from->tableRef->view->myPlan->parseTree;
        
        // 하위 inLineView의 FromTree에서 찾는다.
        for ( sChildFrom  = sChildParseTree->querySet->SFWGH->from;
              sChildFrom != NULL;
              sChildFrom  = sChildFrom->next )
        {
            IDE_TEST(searchColumnInFromTree( aStatement,
                                             sSFWGH,
                                             aQtcColumn,
                                             sChildFrom,
                                             aTableRef )
                     != IDE_SUCCESS);
        }
                            
        // 하위 inLineView의 FromTree에서 찾았다면, 다시 view의 Target을 보고 table, column을 조정한다. 
        if ( *aTableRef != NULL )
        {
            for ( sChildTarget = sChildParseTree->querySet->SFWGH->target, sColumnPosition = 0;
                  sChildTarget != NULL;
                  sChildTarget = sChildTarget->next, sColumnPosition++ )
            {
                if ( ( aQtcColumn->node.table == sChildTarget->targetColumn->node.table ) &&
                     ( aQtcColumn->node.column == sChildTarget->targetColumn->node.column ) )
                {
                    aQtcColumn->node.table      = sSFWGH->from->tableRef->table;
                    aQtcColumn->node.baseTable  = sSFWGH->from->tableRef->table;
                    aQtcColumn->node.column     = sColumnPosition;
                    aQtcColumn->node.baseColumn = sColumnPosition;

                    // PROJ-2469 Optimize View Materialization
                    // Target Column의 경우 View Column Ref 등록 시 Target에서 참조되는지 여부와,
                    // 몇 번 째 Target 인지를 인자로 전달한다.
                    if ( sSFWGH->validatePhase == QMS_VALIDATE_TARGET )
                    {
                        IDE_TEST( addViewColumnRefListForTarget( aStatement,
                                                                 aQtcColumn,
                                                                 sSFWGH->currentTargetNum,
                                                                 sColumnPosition )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        IDE_TEST( addViewColumnRefList( aStatement,
                                                        aQtcColumn )
                                  != IDE_SUCCESS );
                    }
                    
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            
            *aIsFound = ID_TRUE;            
        }
        else
        {
            *aIsFound = ID_FALSE;
        }
    }
    else
    {
        /* Nothing jto do */
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUPLICATE_ALIAS_NAME )
    {
        ( void )sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QMV_DUPLICATE_ALIAS,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setColumnID4Rid(qtcNode* aQtcColumn, mtcCallBack* aCallBack)
{
    qcStatement     * sStatement;
    qtcCallBackInfo * sCallBackInfo;
    qmsFrom         * sFrom     = NULL;
    qmsTableRef     * sTableRef = NULL;
    qcuSqlSourceInfo  sqlInfo;

    UInt              sUserID;
    idBool            sIsFound;

    IDU_FIT_POINT_FATAL( "qmvQTC::setColumnID4Rid::__FT__" );

    IDE_DASSERT(aCallBack != NULL);
    IDE_DASSERT(aCallBack->info != NULL);

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);

    sStatement = sCallBackInfo->statement;

    if (sCallBackInfo->SFWGH == NULL)
    {
        if (sStatement->myPlan->parseTree->stmtKind == QCI_STMT_INSERT)
        {
            /* INSERT INTO t1 (c1) VALUES (_PROWID) */
            sqlInfo.setSourceInfo(sStatement, &aQtcColumn->columnName);
            IDE_RAISE(ERR_COLUMN_NOT_ALLOWED);
        }
        else
        {
            // BUG-38507
            // PSM rowtype 변수의 필드 이름으로  _PROWID를 사용 할 수 있으므로
            // _PROWID, TABLE_NAME._PROWID, USER_NAME.TABLE_NAME._PROWID로 사용한
            // 경우가 아니면 qpERR_ABORT_QMV_NOT_EXISTS_COLUMN 오류를 발생시켜
            // column module로 estimate를 할 수 있도록 한다.
            sqlInfo.setSourceInfo(sStatement, &aQtcColumn->columnName);
            IDE_RAISE(ERR_COLUMN_NOT_FOUND);
        }
    }

    sIsFound = ID_FALSE;

    if (QC_IS_NULL_NAME(aQtcColumn->userName) != ID_TRUE)
    {
        IDE_TEST(qcmUser::getUserID(sStatement,
                                    aQtcColumn->userName,
                                    &sUserID)
                 != IDE_SUCCESS);
    }

    /*
     * BUG-41396
     * _prowid 의 table 을 from 절에 사용된 table 중에서 찾는다.
     */
    for (sFrom = sCallBackInfo->SFWGH->from; sFrom != NULL; sFrom = sFrom->next)
    {
        sTableRef = sFrom->tableRef;

        if (sTableRef == NULL)
        {
            /* ansi style join X */
            continue;
        }

        if (sTableRef->view != NULL)
        {
            /* view 에 대한 _prowid X */
            continue;
        }

        if (QC_IS_NULL_NAME(aQtcColumn->userName) != ID_TRUE)
        {
            /* check user name */
            if (sTableRef->userID != sUserID)
            {
                continue;
            }
        }

        if (QC_IS_NULL_NAME(aQtcColumn->tableName) != ID_TRUE)
        {
            /* check table name */
            if ( QC_IS_NAME_MATCHED( aQtcColumn->tableName, sTableRef->aliasName ) == ID_FALSE )
            {
                continue;
            }
        }

        if (sIsFound == ID_TRUE)
        {
            sqlInfo.setSourceInfo(sStatement, &aQtcColumn->columnName);
            IDE_RAISE(ERR_COLUMN_AMBIGUOUS_DEF);
        }
        else
        {
            sIsFound = ID_TRUE;

            aQtcColumn->node.table = sFrom->tableRef->table;
            aQtcColumn->node.column = MTC_RID_COLUMN_ID;

            aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);

            aQtcColumn->lflag &= ~QTC_NODE_COLUMN_RID_MASK;
            aQtcColumn->lflag |= QTC_NODE_COLUMN_RID_EXIST;
        }
    }

    if (sIsFound == ID_FALSE)
    {
        sqlInfo.setSourceInfo(sStatement, &aQtcColumn->columnName);
        IDE_RAISE(ERR_COLUMN_NOT_FOUND);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_COLUMN_NOT_ALLOWED)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_COLUMN_NOT_ALLOWED_HERE,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_COLUMN_AMBIGUOUS_DEF)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_COLUMN_AMBIGUOUS_DEF,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_COLUMN_NOT_FOUND)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchColumnInOuterQuery(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGHCallBack,
    qtcNode         * aQtcColumn,
    qmsTableRef    ** aTableRef,
    qmsSFWGH       ** aSFWGH )
{
    qmsSFWGH     * sSFWGH;
    qmsFrom      * sFrom;
    qmsTableRef  * sTableRef = NULL;

    IDU_FIT_POINT_FATAL( "qmvQTC::searchColumnInOuterQuery::__FT__" );

    //--------------------------------------------------------
    // BUG-26134
    //
    // 다음 쿼리에서 on절에서 참조된 컬럼을 찾기 위해
    // 검색 가능한 scope는 (t2,t3)에만 해당하며, t1.i1은
    // 찾을 수 없어야 한다. 이를 위해 parent from (outer from)
    // 이라는 개념을 추가한다.
    //
    // select count(*) from
    // t1, t2 left outer join t3 on 1 = (select t1.i1 from dual);
    //
    // subquery의 outer query에서 from은 (t1),(t2,t3)이나
    // outer from이 정의되었다면 outer from인 (t2,t3)에서만
    // 검색되어야 한다. 그리고 outer from이 더이상 없는 경우
    // 최종적으로 outer query가 검색되어야 한다.
    //
    // select 1 from t1, t4
    // where 1 = (select 1 from t2 left outer join t3
    //            on 1 = (select t1.i1 from dual));
    //
    // 그리고, outer from이 정의되지 않은 모든 from절에 해당하는
    // 일반적인 경우 기존 방법대로 outer query의 모든 from이
    // 검색 대상이 된다.
    //
    // select count(*) from t1, t2
    // where 1 = (select t1.i1 from dual);
    //--------------------------------------------------------

    /**********************************************************
     * PROJ-2418
     * 
     * Lateral View도 outerFrom을 필요로 하기 때문에,
     * outerQuery를 상위로 계속 탐색하면서 다음과 같이 찾는다.
     *
     *  - outerFrom이 있으면, outerFrom 에서만 탐색
     *  - outerFrom이 없으면, outerQuery 에서만 탐색
     *
     * 이렇게 구현해도, 종전의 탐색 방법은 변하지 않는다.
     *
     **********************************************************/

    // 현재 SFWGH 부터 출발
    sSFWGH = aSFWGHCallBack;

    while ( sSFWGH != NULL )
    {
        // 상위 레벨의 특정 From을 나타내는 outerFrom 획득
        sFrom = sSFWGH->outerFrom;

        // 상위 레벨의 전체 From을 가리키는 outerQuery 획득
        sSFWGH = sSFWGH->outerQuery;

        if ( sFrom != NULL )
        {
            // outerFrom이 있다면, outerFrom 에서만 찾는다.
            IDE_TEST( searchColumnInFromTree( aStatement,
                                              aSFWGHCallBack,
                                              aQtcColumn,
                                              sFrom,
                                              &sTableRef )
                      != IDE_SUCCESS );
        }
        else
        {
            // outerFrom이 없다면 outerQuery 에서 찾는다.

            // outerQuery가 NULL일 때는 종료
            if ( sSFWGH == NULL )
            {
                break;
            }
            else
            {
                for ( sFrom = sSFWGH->from;
                      sFrom != NULL;
                      sFrom = sFrom->next )
                {
                    IDE_TEST( searchColumnInFromTree( aStatement,
                                                      aSFWGHCallBack,
                                                      aQtcColumn,
                                                      sFrom,
                                                      & sTableRef )
                              != IDE_SUCCESS );

                    if ( sTableRef != NULL )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }

        // 탐색이 성공했다면, 종료
        if ( sTableRef != NULL )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        // 이미 sSFWGH는 상위의 SFWGH가 되었으므로
        // 다음 loop를 위해 sSFWGH를 재설정할 필요가 없다.
    }

    if( sTableRef != NULL )
    {
        *aTableRef = sTableRef;
        *aSFWGH = sSFWGH;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchColumnInFromTree(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGH,
    qtcNode         * aQtcColumn,
    qmsFrom         * aFrom,
    qmsTableRef    ** aTableRef)
{
    qmsTableRef     * sTableRef;
    qcmTableInfo    * sTableInfo = NULL;
    UInt              sUserID;
    UShort            sColOrder;
    idBool            sIsFound;
    idBool            sIsLobType;
    qcDepInfo         sMyDependencies;
    qcDepInfo         sResDependencies;
    qmsSFWGH        * sSFWGH;
    qcuSqlSourceInfo  sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::searchColumnInFromTree::__FT__" );

    if( aFrom->joinType != QMS_NO_JOIN )
    {
        IDE_TEST(searchColumnInFromTree(
                     aStatement, aSFWGH, aQtcColumn,
                     aFrom->left, aTableRef)
                 != IDE_SUCCESS);

        IDE_TEST(searchColumnInFromTree(
                     aStatement, aSFWGH, aQtcColumn,
                     aFrom->right, aTableRef)
                 != IDE_SUCCESS);
    }
    else
    {
        sTableRef  = aFrom->tableRef;
        sTableInfo = sTableRef->tableInfo;

        // check user
        if( QC_IS_NULL_NAME(aQtcColumn->userName) == ID_TRUE )
        {
            sUserID = sTableRef->userID;

            sIsFound = ID_TRUE;
        }
        else
        {
            // BUG-42494 A variable of package could not be used
            // when its type is associative array.
            if (qcmUser::getUserID( aStatement,
                                    aQtcColumn->userName,
                                    &(sUserID))
                == IDE_SUCCESS)
            {
                if( sTableRef->userID == sUserID )
                {
                    sIsFound = ID_TRUE;
                }
                else
                {
                    sIsFound = ID_FALSE;
                }
            }
            else
            {
                IDE_CLEAR();
                sIsFound = ID_FALSE;
            }
        }

        // check table name
        if( sIsFound == ID_TRUE )
        {
            if( QC_IS_NULL_NAME(aQtcColumn->tableName) != ID_TRUE )
            {
                // BUG-38839
                if ( QC_IS_NAME_MATCHED( aQtcColumn->tableName, sTableRef->aliasName ) &&
                     ( sTableInfo != NULL ) )
                {
                    sIsFound = ID_TRUE;
                }
                else
                {
                    sIsFound = ID_FALSE;
                }
            }
        }

        // check column name
        if( sIsFound == ID_TRUE )
        {
            IDE_TEST(searchColumnInTableInfo(
                         sTableInfo, aQtcColumn->columnName,
                         &sColOrder, &sIsFound, &sIsLobType)
                     != IDE_SUCCESS);

            if( sIsFound == ID_TRUE )
            {
                if( *aTableRef != NULL )
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & aQtcColumn->columnName);
                    IDE_RAISE(ERR_COLUMN_AMBIGUOUS_DEF);
                }

                // set table and column ID
                *aTableRef = sTableRef;

                aQtcColumn->node.table = sTableRef->table;
                aQtcColumn->node.column = sColOrder;

                // set base table and column ID
                aQtcColumn->node.baseTable = sTableRef->table;
                aQtcColumn->node.baseColumn = sColOrder;

                if( ( aQtcColumn->lflag & QTC_NODE_PRIOR_MASK )
                    == QTC_NODE_PRIOR_EXIST )
                {
                    aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);
                }

                /* BUG-25916
                 * clob을 select fot update 하던 도중 Assert 발생 */
                if( sIsLobType == ID_TRUE )
                {
                    aQtcColumn->lflag &= ~QTC_NODE_LOB_COLUMN_MASK;
                    aQtcColumn->lflag |= QTC_NODE_LOB_COLUMN_EXIST;
                }

                // make outer column list
                if ( aSFWGH != NULL )
                {
                    if ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
                         != QMV_SFWGH_GBGS_TRANSFORM_MIDDLE )
                    {
                        for ( sSFWGH = aSFWGH;
                              sSFWGH != NULL;
                              sSFWGH = sSFWGH->outerQuery )
                        {
                            qtc::dependencyClear( & sMyDependencies );
                            qtc::dependencyClear( & sResDependencies );
                            qtc::dependencySet( aQtcColumn->node.table,
                                                & sMyDependencies );

                            qtc::dependencyAnd( & sSFWGH->depInfo,
                                                & sMyDependencies,
                                                & sResDependencies );

                            if( qtc::dependencyEqual( & sMyDependencies,
                                                      & sResDependencies )
                                == ID_FALSE )
                            {
                                // outer column reference
                                IDE_TEST( addOuterColumn( aStatement,
                                                          sSFWGH,
                                                          aQtcColumn )
                                          != IDE_SUCCESS);
                            }
                            else
                            {
                                break;
                            }
                        }
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

                // PROJ-1413
                // view 컬럼 참조 노드를 등록한다.
                if ( aSFWGH != NULL )
                {
                    // PROJ-2469 Optimize View Materialization
                    // View Column Ref 등록 시 Target에서 참조되는지 여부와,
                    // 몇 번 째 Target 인지를 인자로 전달한다.
                    // PROJ-2687 Shard aggregation transform
                    // 별도 처리로 * at setColumnIDForShardTransView()
                    // 이미 view column reference가 설정되어있다.
                    if ( aSFWGH->validatePhase == QMS_VALIDATE_TARGET )
                    {
                        if ( ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK ) !=
                               QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) &&
                             ( ( aSFWGH->lflag & QMV_SFWGH_SHARD_TRANS_VIEW_MASK ) !=
                               QMV_SFWGH_SHARD_TRANS_VIEW_TRUE ) )
                        {
                            IDE_TEST( addViewColumnRefListForTarget( aStatement,
                                                                        aQtcColumn,
                                                                        aSFWGH->currentTargetNum,
                                                                        aQtcColumn->node.column )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        if ( ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK ) !=
                               QMV_SFWGH_GBGS_TRANSFORM_MIDDLE ) &&
                             ( ( aSFWGH->lflag & QMV_SFWGH_SHARD_TRANS_VIEW_MASK ) !=
                               QMV_SFWGH_SHARD_TRANS_VIEW_TRUE ) )
                        {
                            IDE_TEST( addViewColumnRefList( aStatement,
                                                            aQtcColumn )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    IDE_TEST( addViewColumnRefList( aStatement,
                                                    aQtcColumn )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_COLUMN_AMBIGUOUS_DEF)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_COLUMN_AMBIGUOUS_DEF,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::addOuterColumn(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGH,
    qtcNode         * aQtcColumn)
{
    qmsOuterNode    * sOuter;

    IDU_FIT_POINT_FATAL( "qmvQTC::addOuterColumn::__FT__" );

    // search same column reference
    for (sOuter = aSFWGH->outerColumns;
         sOuter != NULL;
         sOuter = sOuter->next)
    {
        if( ( sOuter->column->node.table == aQtcColumn->node.table ) &&
            ( sOuter->column->node.column == aQtcColumn->node.column ) )
        {
            break;
        }
    }

    // make outer column reference list
    if( sOuter == NULL )
    {
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qmsOuterNode, &sOuter)
                 != IDE_SUCCESS);

        sOuter->column = aQtcColumn;
        sOuter->next = aSFWGH->outerColumns;
        aSFWGH->outerColumns = sOuter;

        /* PROJ-2448 Subquery caching */
        sOuter->cacheTable  = ID_USHORT_MAX;
        sOuter->cacheColumn = ID_USHORT_MAX;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setOuterColumns( qcStatement * aSQStatement,
                                qcDepInfo   * aSQDepInfo,
                                qmsSFWGH    * aSQSFWGH,
                                qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     Correlation predicate이 추가되면서 outer column을 subquery에서
 *     참조할 수 있으므로 관련된 정보를 구성해준다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode   * sArg;
    idBool      sIsAdd = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvQTC::setOuterColumns::__FT__" );

    if( QTC_IS_COLUMN( aSQStatement, aNode ) == ID_TRUE )
    {
        // BUG-43134 view 자신의 것은 OuterColumn 에 넣으면 안된다.
        if ( aSQDepInfo != NULL )
        {
            if ( (qtc::dependencyContains( aSQDepInfo, &aNode->depInfo ) == ID_FALSE) &&
                 (qtc::dependencyContains( &aSQSFWGH->depInfo, &aNode->depInfo ) == ID_FALSE) )
            {
                sIsAdd = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( qtc::dependencyContains( &aSQSFWGH->depInfo, &aNode->depInfo ) == ID_FALSE )
            {
                sIsAdd = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsAdd == ID_TRUE )
        {
            IDE_TEST( qmvQTC::addOuterColumn( aSQStatement,
                                              aSQSFWGH,
                                              aNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        for( sArg = (qtcNode *)aNode->node.arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next )
        {
            // BUG-38806
            // Subquery 는 outer column 이 아니다.
            if( sArg->node.module != &qtc::subqueryModule )
            {
                IDE_TEST( setOuterColumns( aSQStatement,
                                           aSQDepInfo,
                                           aSQSFWGH,
                                           sArg )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::extendOuterColumns( qcStatement * aSQStatement,
                                   qcDepInfo   * aSQDepInfo,
                                   qmsSFWGH    * aSQSFWGH )
{
/***********************************************************************
 *
 * Description : BUG-45212 서브쿼리의 외부 참조 컬럼이 연산일때 결과가 틀립니다.
 *                  outerColumns 이 연산자 일때 연산자를 제거하고
 *                  피연산자 컬럼을 추가한다.
 *
 ***********************************************************************/

    qmsOuterNode * sOuterNode;
    qmsOuterNode * sPrev        = NULL;

    for ( sOuterNode = aSQSFWGH->outerColumns;
          sOuterNode != NULL;
          sOuterNode = sOuterNode->next )
    {
        if ( QTC_IS_COLUMN( aSQStatement, sOuterNode->column ) == ID_FALSE )
        {
            if ( sPrev != NULL )
            {
                sPrev->next = sOuterNode->next;
            }
            else
            {
                aSQSFWGH->outerColumns = sOuterNode->next;
                sPrev = aSQSFWGH->outerColumns;
            }

            IDE_TEST( setOuterColumns( aSQStatement,
                                       aSQDepInfo,
                                       aSQSFWGH,
                                       sOuterNode->column )
                      != IDE_SUCCESS );
        }
        else
        {
            sPrev = sOuterNode;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setLateralDependencies( qmsSFWGH  * aSFWGH,
                                       qcDepInfo * aLateralDepInfo )
{
/**********************************************************************
 *
 *  Description : PROJ-2418 Cross / Outer APPLY & Lateral View
 * 
 *  현재 QuerySet의 lateralDepInfo를 설정한다.
 *
 *  - 현재 QuerySet이 외부에서 참조해야 하는 depInfo를
 *    lateralDepInfo라고 한다.
 *  - lateralDepInfo는 Lateral View / Subquery만 가질 수 있다.
 *    그 외에는 lateralDepInfo가 비어 있다.
 *
 *  
 *  Implementation:
 *
 *   e.g.) SELECT *
 *         FROM  T0, T1, LATERAL ( SELECT * 
 *                                 FROM T2, T3,
 *                                      LATERAL ( .. ) LV1
 *                                      LATERAL ( .. ) LV2
 *                                 WHERE T2.i1 = T0.i1 ) OLV;
 *
 *   1) Lateral View 내부에 있는 Lateral View의 lateralDepInfo를
 *      모두 ORing 한다.
 *  
 *      >> LV1, LV2는 나타내지 않았지만,
 *         각각의 lateralDepInfo가 아래와 같다고 하자.
 *
 *         - LV1's lateralDepInfo = { 2 }
 *         - LV2's lateralDepInfo = { 3, 1 }
 *
 *         그러면 OLV에서는, (1)번 과정을 통해 { 1,2,3 } 을
 *         lateralDepInfo로 처음 얻게 된다.
 *
 *   2) (1)의 결과에서, Lateral View의 depInfo를 Minus 한다.
 *      Minus되어 빠지는 dependency는, 내부 Lateral View들이
 *      현재 QuerySet에서 참조를 완료한 dependency이다.
 *      따라서, 현재 QuerySet은 외부에서 해당 dependency를
 *      찾지 않아야 하기 때문에 Minus를 한다.
 *
 *      >> OLV의 depInfo는 { 2, 3, X, Y } 이다. (X, Y는 LV1, LV2)
 *         OLV에서는, (2)번 과정을 통해 lateralDepInfo에서
 *         { 1 } 만 남기게 된다.
 *
 *   3*) 현재 QuerySet의 outerDepInfo를 ORing 한다.
 *       outerDepInfo는, validation 과정에서 outerQuery 또는 outerFrom
 *       에서 검색이 된 outer Column의 dependency 집합이다.
 *       (outerDepInfo 개념은 Subquery에 있던 개념이다.)
 *
 *       outerDepInfo도 외부에서 참조해야 하는 dependency이기 때문에
 *       lateralDepInfo에 추가해야 한다.
 *
 *       >> OLV의 outerDepInfo는 { 0 } 이다.
 *          따라서, OLV의 최종 lateralDepInfo는 { 0, 1 } 이 된다.
 *
 *
 *   * 하지만 현재 QuerySet이 Lateral View일 때만 (3)번 과정을 한다.
 *     현재 QuerySet이 Subquery를 나타내는 것이라면,
 *     이 때의 outerDepInfo는 lateralDepInfo의 의미가 아니기 때문이다.
 *
 *     qmvQTC::setLateralDependencies() 에서는 (1), (2)번 과정만 진행하고
 *     (3)번 과정은 Lateral View의 validation이 끝날 때만 따로 진행한다.
 *     (3)번 과정은 qmvQTC::setLateralDependenciesLast()에서 진행되며
 *     다음 함수에서 호출된다.
 *
 *     - qmvQuerySet::validateView()
 *     - qmoViewMerging::validateFrom()
 *
 **********************************************************************/
 
    qmsFrom     * sFrom;
    qcDepInfo     sDepInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::setLateralDependencies::__FT__" );

    qtc::dependencyClear( aLateralDepInfo );

    // (1) 내부 Lateral View들의 lateralDepInfo ORing
    for ( sFrom = aSFWGH->from;
          sFrom != NULL;
          sFrom = sFrom->next)
    {
        IDE_TEST( getFromLateralDepInfo( sFrom, & sDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & sDepInfo,
                                     aLateralDepInfo,
                                     aLateralDepInfo )
                  != IDE_SUCCESS );
    }

    // (2) 현재 QuerySet에서 참조되는 dependency는 Minus
    qtc::dependencyMinus( aLateralDepInfo, 
                          & aSFWGH->depInfo,
                          aLateralDepInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::getFromLateralDepInfo( qmsFrom   * aFrom,
                                      qcDepInfo * aFromLateralDepInfo )
{
/***********************************************************************
 *
 * Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 * 
 *  qmsFrom에 Lateral View가 있다면, 그의 lateralDepInfo를 모아 반환한다.
 *
 *  - qmsFrom이 Base Table이라면, lateralDepInfo를 반환한다.
 *  - qmsFrom이 Join Tree라면,
 *    LEFT / RIGHT의 lateralDepInfo를 모아 반환한다.
 *
 ***********************************************************************/
    
    qmsQuerySet * sViewQuerySet;
    qcDepInfo     sLeftDepInfo;
    qcDepInfo     sRightDepInfo;
    qcDepInfo     sResultDepInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::getFromLateralDepInfo::__FT__" );

    qtc::dependencyClear( & sLeftDepInfo );
    qtc::dependencyClear( & sRightDepInfo );
    qtc::dependencyClear( aFromLateralDepInfo );

    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        IDE_TEST( getFromLateralDepInfo( aFrom->left, & sLeftDepInfo )
                  != IDE_SUCCESS );  
        IDE_TEST( getFromLateralDepInfo( aFrom->right, & sRightDepInfo )
                  != IDE_SUCCESS );  

        IDE_TEST( qtc::dependencyOr( & sLeftDepInfo,
                                     & sRightDepInfo,
                                     & sResultDepInfo )
                  != IDE_SUCCESS );

        if ( qtc::haveDependencies( & sResultDepInfo ) == ID_TRUE )
        {
            qtc::dependencySetWithDep( aFromLateralDepInfo,
                                       & sResultDepInfo );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_DASSERT( aFrom->tableRef != NULL );

        if ( ( aFrom->tableRef->flag & QMS_TABLE_REF_LATERAL_VIEW_MASK )
             == QMS_TABLE_REF_LATERAL_VIEW_TRUE )
        {
            IDE_DASSERT( aFrom->tableRef->view != NULL );

            // view가 가지고 있는 QuerySet에서 lateralDepInfo를 가져온다.
            sViewQuerySet =
                ((qmsParseTree *) aFrom->tableRef->view->myPlan->parseTree)->querySet;

            if ( qtc::haveDependencies( & sViewQuerySet->lateralDepInfo ) == ID_TRUE )
            {
                qtc::dependencySetWithDep( aFromLateralDepInfo,
                                           & sViewQuerySet->lateralDepInfo );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setLateralDependenciesLast( qmsQuerySet * aLateralQuerySet )
{
/******************************************************************
 *
 * Description : BUG-39567 Lateral View
 *
 *  View QuerySet에 outerDepInfo가 존재하는 경우에는
 *  lateralDepInfo에 outerDepInfo를 ORing 한다.
 *
 *  lateralDepInfo를 구하는 시점에서 outerDepInfo를 ORing 하지 않고
 *  이렇게 Lateral View인 경우에만 구별해서 따로 ORing 해야만 한다.
 *  그렇지 않으면 Subquery의 outerDepInfo도 lateralDepInfo에 추가된다.
 * 
 *  자세한 내용은 qmvQTC::setLateralDependencies()를 참고한다.
 *
 ******************************************************************/

    IDU_FIT_POINT_FATAL( "qmvQTC::setLateralDependenciesLast::__FT__" );

    if ( aLateralQuerySet->setOp == QMS_NONE )
    {
        IDE_TEST( qtc::dependencyOr( & aLateralQuerySet->lateralDepInfo,
                                     & aLateralQuerySet->outerDepInfo,
                                     & aLateralQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Set Operation인 경우, LEFT/RIGHT의 outerDepInfo도 고려해야 한다.
        IDE_TEST( setLateralDependenciesLast( aLateralQuerySet->left )
                  != IDE_SUCCESS );

        IDE_TEST( setLateralDependenciesLast( aLateralQuerySet->right )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & aLateralQuerySet->left->lateralDepInfo,
                                     & aLateralQuerySet->lateralDepInfo,
                                     & aLateralQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & aLateralQuerySet->right->lateralDepInfo,
                                     & aLateralQuerySet->lateralDepInfo,
                                     & aLateralQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchSequence(
    qcStatement     * aStatement,
    qtcNode         * aQtcColumn,
    idBool          * aFindColumn)
{
    qcmSequenceInfo       sSequenceInfo;
    qcParseSeqCaches    * sCurrSeqCache;
    idBool                sFind         = ID_FALSE;
    UInt                  sUserID;
    void                * sSequenceHandle;
    UInt                  sErrCode;

    qcmSynonymInfo        sSynonymInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::searchSequence::__FT__" );
    
    if( qcmSynonym::resolveSequence(
            aStatement,
            aQtcColumn->userName,
            aQtcColumn->tableName,
            &(sSequenceInfo),
            &(sUserID),
            &sFind,
            &sSynonymInfo,
            &sSequenceHandle ) == IDE_SUCCESS )
    {
        if( sFind == ID_TRUE )
        {
            if( sSequenceInfo.sequenceType != QCM_SEQUENCE )
            {
                sFind = ID_FALSE;
            }
        }

        // column name
        if( sFind == ID_TRUE)
        {
            // environment의 기록
            IDE_TEST( qcgPlan::registerPlanSequence(
                          aStatement,
                          sSequenceHandle )
                      != IDE_SUCCESS );

            // environment의 기록
            IDE_TEST( qcgPlan::registerPlanSynonym(
                          aStatement,
                          & sSynonymInfo,
                          aQtcColumn->userName,
                          aQtcColumn->tableName,
                          NULL,
                          sSequenceHandle )
                      != IDE_SUCCESS );

            if( idlOS::strMatch(
                    aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                    aQtcColumn->columnName.size,
                    (SChar *)"CURRVAL",
                    7) == 0 )
            {
                /* TASK-7217 Sharded sequence
                 * CURRVAL of sequence is not supported when the property "SHARD_ENABME" is on */
                IDE_TEST_RAISE ( SDU_SHARD_ENABLE == 1, ERR_NOT_SUPPORTED );

                aQtcColumn->lflag &= ~QTC_NODE_SEQUENCE_MASK;
                aQtcColumn->lflag |= QTC_NODE_SEQUENCE_EXIST;

                // search sequence in NEXTVAL sequence list
                findSeqCache( aStatement->myPlan->parseTree->nextValSeqs,
                              &sSequenceInfo,
                              &sCurrSeqCache);

                if( sCurrSeqCache == NULL )  // NOT FOUND
                {
                    // search sequence in CURRVAL sequence list
                    findSeqCache( aStatement->myPlan->parseTree->currValSeqs,
                                  &sSequenceInfo,
                                  &sCurrSeqCache);
                }

                if( sCurrSeqCache == NULL )  // NOT FOUND
                {
                    IDE_TEST( addSeqCache( aStatement,
                                           &sSequenceInfo,
                                           aQtcColumn,
                                           &(aStatement->myPlan->parseTree->currValSeqs) )
                              != IDE_SUCCESS );
                }
                else                        // FOUND
                {
                    aQtcColumn->node.table  =
                        sCurrSeqCache->sequenceNode->node.table;
                    aQtcColumn->node.column =
                        sCurrSeqCache->sequenceNode->node.column;
                }

                *aFindColumn = ID_TRUE;
            }
            else if( idlOS::strMatch(
                         aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                         aQtcColumn->columnName.size,
                         (SChar *)"NEXTVAL",
                         7) == 0 )
            {
                aQtcColumn->lflag &= ~QTC_NODE_SEQUENCE_MASK;
                aQtcColumn->lflag |= QTC_NODE_SEQUENCE_EXIST;

                // search sequence in NEXTVAL sequence list
                findSeqCache( aStatement->myPlan->parseTree->nextValSeqs,
                              &sSequenceInfo,
                              &sCurrSeqCache);

                if( sCurrSeqCache == NULL )  // NOT FOUND in NEXTVAL sequence list
                {
                    // search sequence in CURRVAL sequence list
                    findSeqCache( aStatement->myPlan->parseTree->currValSeqs,
                                  &sSequenceInfo,
                                  &sCurrSeqCache);

                    if( sCurrSeqCache == NULL ) // NOT FOUND in CURRVAL sequence list
                    {
                        IDE_TEST( addSeqCache( aStatement,
                                               &sSequenceInfo,
                                               aQtcColumn,
                                               &(aStatement->myPlan->parseTree->nextValSeqs) )
                                  != IDE_SUCCESS );
                    }
                    else                       // FOUND in CURRVAL sequence list
                    {
                        aQtcColumn->node.table =
                            sCurrSeqCache->sequenceNode->node.table;
                        aQtcColumn->node.column =
                            sCurrSeqCache->sequenceNode->node.column;

                        // move node from currValSeqs to nextValSeqs
                        moveSeqCacheFromCurrToNext( aStatement,
                                                    &sSequenceInfo );
                    }
                }
                else                        // FOUND in NEXTVAL sequence list
                {
                    aQtcColumn->node.table  =
                        sCurrSeqCache->sequenceNode->node.table;
                    aQtcColumn->node.column =
                        sCurrSeqCache->sequenceNode->node.column;
                }

                *aFindColumn = ID_TRUE;
            }
            else
            {
                *aFindColumn = ID_FALSE;
            }

            // check grant
            if( *aFindColumn == ID_TRUE )
            {
                // BUG-16980
                IDE_TEST( qdpRole::checkDMLSelectSequencePriv(
                              aStatement,
                              sSequenceInfo.sequenceOwnerID,
                              sSequenceInfo.sequenceID,
                              ID_FALSE,
                              NULL,
                              NULL)
                          != IDE_SUCCESS );

                // environment의 기록
                IDE_TEST( qcgPlan::registerPlanPrivSequence( aStatement,
                                                             &sSequenceInfo )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            *aFindColumn = ID_FALSE;
        }

        if( *aFindColumn == ID_TRUE )
        {
            if( aStatement->spvEnv->createProc != NULL )
            {
                // search or make related object list
                IDE_TEST(qsvProcStmts::makeRelatedObjects(
                             aStatement,
                             & aQtcColumn->userName,
                             & aQtcColumn->tableName,
                             & sSynonymInfo,
                             sSequenceInfo.sequenceID,
                             QS_TABLE )
                         != IDE_SUCCESS);
            }
        }
    }
    else
    {
        sErrCode = ideGetErrorCode();
        if( sErrCode == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_CLEAR();
        }
        else
        {
            IDE_RAISE( ERR_NOT_ABOUT_USER_MSG );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ABOUT_USER_MSG )
    {
        // Nohting to do.
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED );
    {
        IDE_SET(
            ideSetErrorCode(sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                            "CURRVAL of sequence exists.",
                            "" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmvQTC::findSeqCache(
    qcParseSeqCaches    * aParseSeqCaches,
    qcmSequenceInfo     * aSequenceInfo,
    qcParseSeqCaches   ** aSeqCache)
{
    qcParseSeqCaches    * sCurrSeqCache = NULL;

    for (sCurrSeqCache = aParseSeqCaches;
         sCurrSeqCache != NULL;
         sCurrSeqCache = sCurrSeqCache->next)
    {
        if( sCurrSeqCache->sequenceOID == aSequenceInfo->sequenceOID )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aSeqCache = sCurrSeqCache;
}

IDE_RC qmvQTC::addSeqCache(
    qcStatement         * aStatement,
    qcmSequenceInfo     * aSequenceInfo,
    qtcNode             * aQtcColumn,
    qcParseSeqCaches   ** aSeqCaches)
{
    qcParseSeqCaches    * sCurrSeqCache;
    SChar                 sSeqTableNameStr[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SLong                 sStartValue;
    SLong                 sIncrementValue;
    SLong                 sCacheValue;
    SLong                 sMaxValue;
    SLong                 sMinValue;
    UInt                  sOption;
    qcuSqlSourceInfo      sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::addSeqCache::__FT__" );

    // make tuple for SEQUENCE pseudocolumn
    IDE_TEST(makeOneTupleForPseudoColumn(
                 aStatement,
                 aQtcColumn,
                 (SChar *)"BIGINT",
                 6)
             != IDE_SUCCESS);

    // make qcParseSeqCaches node
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcParseSeqCaches, &sCurrSeqCache)
             != IDE_SUCCESS);

    sCurrSeqCache->sequenceHandle    = aSequenceInfo->sequenceHandle;
    sCurrSeqCache->sequenceOID       = aSequenceInfo->sequenceOID;
    sCurrSeqCache->sequenceNode      = aQtcColumn;
    sCurrSeqCache->next              = *aSeqCaches;

    // PROJ-2365 sequence table
    IDE_TEST( smiTable::getSequence( aSequenceInfo->sequenceHandle,
                                     & sStartValue,
                                     & sIncrementValue,
                                     & sCacheValue,
                                     & sMaxValue,
                                     & sMinValue,
                                     & sOption )
              != IDE_SUCCESS );

    if ( ( sOption & SMI_SEQUENCE_TABLE_MASK ) == SMI_SEQUENCE_TABLE_TRUE )
    {
        // make sequence table name : SEQ1$SEQ
        IDE_TEST_RAISE( idlOS::strlen( aSequenceInfo->name ) + QDS_SEQ_TABLE_SUFFIX_LEN
                        > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SEQUENCE_NAME_LEN );
        
        idlOS::snprintf( sSeqTableNameStr,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s%s",
                         aSequenceInfo->name,
                         QDS_SEQ_TABLE_SUFFIX_STR );

        if ( qcm::getTableHandleByName( QC_SMI_STMT(aStatement),
                                        aSequenceInfo->sequenceOwnerID,
                                        (UChar*) sSeqTableNameStr,
                                        idlOS::strlen( sSeqTableNameStr ),
                                        &(sCurrSeqCache->tableHandle),
                                        &(sCurrSeqCache->tableSCN) )
             != IDE_SUCCESS )
        {
            sqlInfo.setSourceInfo(aStatement, &aQtcColumn->tableName);
            IDE_RAISE( ERR_NOT_EXIST_TABLE );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( smiGetTableTempInfo( sCurrSeqCache->tableHandle,
                                       (void**)&sCurrSeqCache->tableInfo )
                  != IDE_SUCCESS );
        
        // validation lock이면 충분하다.
        IDE_TEST( qcm::lockTableForDMLValidation(
                      aStatement,
                      sCurrSeqCache->tableHandle,
                      sCurrSeqCache->tableSCN )
                  != IDE_SUCCESS );
        
        sCurrSeqCache->sequenceTable = ID_TRUE;
    }
    else
    {
        sCurrSeqCache->sequenceTable = ID_FALSE;
    }
    
    *aSeqCaches = sCurrSeqCache;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_SEQUENCE_NAME_LEN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvQTC::addSeqCache",
                                  "sequence name is too long" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmvQTC::moveSeqCacheFromCurrToNext(
    qcStatement     * aStatement,
    qcmSequenceInfo * aSequenceInfo )
{
    qcParseSeqCaches    * sPrevSeqCache = NULL;
    qcParseSeqCaches    * sCurrSeqCache = NULL;

    // delete from currValSeqs
    for (sCurrSeqCache = aStatement->myPlan->parseTree->currValSeqs;
         sCurrSeqCache != NULL;
         sCurrSeqCache = sCurrSeqCache->next)
    {
        if( sCurrSeqCache->sequenceOID == aSequenceInfo->sequenceOID )
        {
            if( sPrevSeqCache != NULL )
            {
                sPrevSeqCache->next = sCurrSeqCache->next;
            }
            else
            {
                aStatement->myPlan->parseTree->currValSeqs = sCurrSeqCache->next;
            }

            break;
        }
        else
        {
            /* Nothing to do */
        }

        sPrevSeqCache = sCurrSeqCache;
    }

    if( sCurrSeqCache != NULL )
    {
        // add to nextValSeqs
        sCurrSeqCache->next = aStatement->myPlan->parseTree->nextValSeqs;
        aStatement->myPlan->parseTree->nextValSeqs = sCurrSeqCache;
    }
    else
    {
        // Nothing to do.
    }
}

// for SYSDATE, SEQUENCE.CURRVAL, SEQUENCE.NEXTVAL, LEVEL
IDE_RC qmvQTC::makeOneTupleForPseudoColumn(
    qcStatement     * aStatement,
    qtcNode         * aQtcColumn,
    SChar           * aDataTypeName,
    UInt              aDataTypeLength)
{
    const mtdModule * sModule;

    mtcTemplate * sMtcTemplate;

    IDU_FIT_POINT_FATAL( "qmvQTC::makeOneTupleForPseudoColumn::__FT__" );

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    // add tuple set
    IDE_TEST(qtc::nextTable( &(aQtcColumn->node.table),
                             aStatement,
                             NULL,
                             ID_TRUE,
                             MTC_COLUMN_NOTNULL_TRUE) // PR-13597
             != IDE_SUCCESS);

    sMtcTemplate->rows[aQtcColumn->node.table].lflag
        = qtc::templateRowFlags[MTC_TUPLE_TYPE_INTERMEDIATE];

    
    /* BUG-44382 clone tuple 성능개선 */
    // 초기화가 필요함
    qtc::setTupleColumnFlag( &(sMtcTemplate->rows[aQtcColumn->node.table]),
                             ID_FALSE,
                             ID_TRUE );

    // only one column
    aQtcColumn->node.column = 0;
    sMtcTemplate->rows[aQtcColumn->node.table].columnCount     = 1;
    sMtcTemplate->rows[aQtcColumn->node.table].columnMaximum   = 1;

    // memory alloc for columns and execute
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcColumn),
                 (void**) & (sMtcTemplate->rows[aQtcColumn->node.table].columns))
             != IDE_SUCCESS);

    // PROJ-1437
    IDE_TEST( qtc::allocAndInitColumnLocateInTuple( QC_QMP_MEM(aStatement),
                                                    sMtcTemplate,
                                                    aQtcColumn->node.table )
              != IDE_SUCCESS );    

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                 ID_SIZEOF(mtcExecute),
                 (void**) & (sMtcTemplate->rows[aQtcColumn->node.table].execute))
             != IDE_SUCCESS);

    /* BUG-48623 */
    sMtcTemplate->rows[aQtcColumn->node.table].columns[0].column.id = 0;
    sMtcTemplate->rows[aQtcColumn->node.table].columns[0].column.colSpace = 0;

    // mtdModule 설정
    // DATE or BIGINT
    IDE_TEST( mtd::moduleByName( & sModule ,
                                 (void*)aDataTypeName,
                                 aDataTypeLength )
              != IDE_SUCCESS);

    // DATE or BIGINT do NOT have arguments ( precision, scale )
    //IDE_TEST( sTuple->columns[0].module->estimate(
    //    &(sTuple->columns[0]), 0, 0, 0) != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn(
                  &(sMtcTemplate->rows[aQtcColumn->node.table].columns[0]),
                  sModule,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmvQTC::setOuterDependencies( qmsSFWGH    * aSFWGH,
                              qcDepInfo   * aDepInfo )
{

    IDU_FIT_POINT_FATAL( "qmvQTC::setOuterDependencies::__FT__" );

    qtc::dependencyClear( aDepInfo );

    IDE_TEST( addOuterDependencies( aSFWGH, aDepInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2415 Grouping Sets Clause
IDE_RC qmvQTC::addOuterDependencies( qmsSFWGH    * aSFWGH,
                                     qcDepInfo   * aDepInfo )
{
    qmsOuterNode    * sOuter;

    IDU_FIT_POINT_FATAL( "qmvQTC::addOuterDependencies::__FT__" );

    for ( sOuter = aSFWGH->outerColumns;
          sOuter != NULL;
          sOuter = sOuter->next )
    {
        // BUG-23059
        // outer column이 view merging 되면서
        // 연산으로 바뀌는 경우도 있다.
        // 따라서 column의 dependency를 oring 하는 것이 아니라
        // column의 depInfo를 oring 하여야 한다.
        // ( depInfo는 하위 node들을 모두 oring 한 값임 )
        IDE_TEST( qtc::dependencyOr( & sOuter->column->depInfo,
                                     aDepInfo,
                                     aDepInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchColumnInTableInfo(
    qcmTableInfo    * aTableInfo,
    qcNamePosition    aColumnName,
    UShort          * aColOrder,
    idBool          * aIsFound,
    idBool          * aIsLobType )
{
    UShort        sColOrder;
    qcmColumn   * sQcmColumn;

    IDU_FIT_POINT_FATAL( "qmvQTC::searchColumnInTableInfo::__FT__" );

    *aIsFound   = ID_FALSE;
    *aIsLobType = ID_FALSE;

    // To fix BUG-19873
    // join의 on 절 처리로 인해 tableInfo가 null
    // 인 경우가 발생함

    if( aTableInfo != NULL )
    { 
        for (sColOrder = 0,
                 sQcmColumn = aTableInfo->columns;
             sQcmColumn != NULL;
             sColOrder++,
                 sQcmColumn = sQcmColumn->next)
        {
            if( idlOS::strMatch(
                    sQcmColumn->name,
                    idlOS::strlen(sQcmColumn->name),
                    aColumnName.stmtText + aColumnName.offset,
                    aColumnName.size) == 0 )
            {
                // BUG-15414
                // TableInfo에 중복된 alias name이 존재하더라도
                // 찾고자하는 column name에만 중복이 없으면 된다.
                IDE_TEST_RAISE( *aIsFound == ID_TRUE,
                                ERR_DUP_ALIAS_NAME );

                *aColOrder = sColOrder;
                *aIsFound = ID_TRUE;

                /* BUG-25916
                 * clob을 select fot update 하던 도중 Assert 발생 */
                // BUG-47751 lob locator도 lob type column이다.
                if( ( (sQcmColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK)
                      == MTD_COLUMN_TYPE_LOB ) ||
                    (sQcmColumn->basicInfo->module == &mtdClobLocator ) ) 
                {
                    *aIsLobType = ID_TRUE;
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
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUP_ALIAS_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchDatePseudoColumn(
    qcStatement     * aStatement,
    qtcNode         * aQtcColumn,
    idBool          * aFindColumn)
{

    IDU_FIT_POINT_FATAL( "qmvQTC::searchDatePseudoColumn::__FT__" );

    if( ( ( aQtcColumn->columnName.offset > 0 ) &&
          ( idlOS::strMatch(
              aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
              aQtcColumn->columnName.size,
              (SChar *)"UNIX_DATE", 9) == 0 ) )
        ||
        ( ( aQtcColumn->columnName.offset > 0 ) &&
          ( idlOS::strMatch(
              aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
              aQtcColumn->columnName.size,
              (SChar *)"UNIX_TIMESTAMP", 14) == 0 ) ) )
    {
        if( QC_SHARED_TMPLATE(aStatement)->unixdate == NULL )
        {
            // To Fix PR-9492
            // SYSDATE Column이 Store And Search등으로 사용될 경우
            // Target의 변경이 sysdate를 변경시킬 수 있다.
            // 따라서, sysdate는 입력된 Node와 별도로 생성하여야 한다.

            IDE_TEST(
                QC_QMP_MEM(aStatement)->alloc(
                    ID_SIZEOF(qtcNode),
                    (void**) & QC_SHARED_TMPLATE(aStatement)->unixdate )
                != IDE_SUCCESS );

            idlOS::memcpy( QC_SHARED_TMPLATE(aStatement)->unixdate,
                           aQtcColumn,
                           ID_SIZEOF(qtcNode) );

            // make tuple for SYSDATE
            IDE_TEST(makeOneTupleForPseudoColumn( aStatement,
                                                  QC_SHARED_TMPLATE(aStatement)->unixdate,
                                                  (SChar *)"DATE",
                                                  4)
                     != IDE_SUCCESS);
        }
        else
        {
            // Nothing To Do
        }

        aQtcColumn->node.table = QC_SHARED_TMPLATE(aStatement)->unixdate->node.table;
        aQtcColumn->node.column = QC_SHARED_TMPLATE(aStatement)->unixdate->node.column;

        // fix BUG-10524
        aQtcColumn->lflag &= ~QTC_NODE_SYSDATE_MASK;
        aQtcColumn->lflag |= QTC_NODE_SYSDATE_EXIST;

        *aFindColumn = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    if( *aFindColumn == ID_FALSE )
    {
        if( ( ( aQtcColumn->columnName.offset > 0 ) &&
              ( idlOS::strMatch(
                  aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                  aQtcColumn->columnName.size,
                  (SChar *)"SYSDATE", 7) == 0 ) )
            ||
            ( ( aQtcColumn->columnName.offset > 0 ) &&
              ( idlOS::strMatch(
                  aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                  aQtcColumn->columnName.size,
                  (SChar *)"SYSTIMESTAMP", 12) == 0 ) )
            ||
            ( ( aQtcColumn->columnName.offset > 0 ) &&
              ( idlOS::strMatch(
                  aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                  aQtcColumn->columnName.size,
                  (SChar *)"SYSDATETIME", 11) == 0 ) ) )
        {
            if( QC_SHARED_TMPLATE(aStatement)->sysdate == NULL )
            {
                // To Fix PR-9492
                // SYSDATE Column이 Store And Search등으로 사용될 경우
                // Target의 변경이 sysdate를 변경시킬 수 있다.
                // 따라서, sysdate는 입력된 Node와 별도로 생성하여야 한다.

                IDE_TEST(
                    QC_QMP_MEM(aStatement)->alloc(
                        ID_SIZEOF(qtcNode),
                        (void**) & QC_SHARED_TMPLATE(aStatement)->sysdate )
                    != IDE_SUCCESS );

                idlOS::memcpy( QC_SHARED_TMPLATE(aStatement)->sysdate,
                               aQtcColumn,
                               ID_SIZEOF(qtcNode) );

                // make tuple for SYSDATE
                IDE_TEST(makeOneTupleForPseudoColumn( aStatement,
                                                      QC_SHARED_TMPLATE(aStatement)->sysdate,
                                                      (SChar *)"DATE",
                                                      4)
                         != IDE_SUCCESS);
            }
            else
            {
                // Nothing To Do
            }

            aQtcColumn->node.table = QC_SHARED_TMPLATE(aStatement)->sysdate->node.table;
            aQtcColumn->node.column = QC_SHARED_TMPLATE(aStatement)->sysdate->node.column;

            // fix BUG-10524
            aQtcColumn->lflag &= ~QTC_NODE_SYSDATE_MASK;
            aQtcColumn->lflag |= QTC_NODE_SYSDATE_EXIST;

            *aFindColumn = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        /* nothing to do. */
    }

    if( *aFindColumn == ID_FALSE )
    {
        if( ( ( aQtcColumn->columnName.offset > 0 ) &&
              ( idlOS::strMatch(
                  aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                  aQtcColumn->columnName.size,
                  (SChar *)"CURRENT_DATE", 12) == 0 ) )
            ||
            ( ( aQtcColumn->columnName.offset > 0 ) &&
              ( idlOS::strMatch(
                  aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
                  aQtcColumn->columnName.size,
                  (SChar *)"CURRENT_TIMESTAMP", 17) == 0 ) ) )
        {
            if( QC_SHARED_TMPLATE(aStatement)->currentdate == NULL )
            {
                // To Fix PR-9492
                // SYSDATE Column이 Store And Search등으로 사용될 경우
                // Target의 변경이 sysdate를 변경시킬 수 있다.
                // 따라서, sysdate는 입력된 Node와 별도로 생성하여야 한다.

                IDE_TEST(
                    QC_QMP_MEM(aStatement)->alloc(
                        ID_SIZEOF(qtcNode),
                        (void**) & QC_SHARED_TMPLATE(aStatement)->currentdate )
                    != IDE_SUCCESS );

                idlOS::memcpy( QC_SHARED_TMPLATE(aStatement)->currentdate,
                               aQtcColumn,
                               ID_SIZEOF(qtcNode) );

                // make tuple for SYSDATE
                IDE_TEST(makeOneTupleForPseudoColumn( aStatement,
                                                      QC_SHARED_TMPLATE(aStatement)->currentdate,
                                                      (SChar *)"DATE",
                                                      4)
                         != IDE_SUCCESS);
            }
            else
            {
                // Nothing To Do
            }

            aQtcColumn->node.table = QC_SHARED_TMPLATE(aStatement)->currentdate->node.table;
            aQtcColumn->node.column = QC_SHARED_TMPLATE(aStatement)->currentdate->node.column;

            // fix BUG-10524
            aQtcColumn->lflag &= ~QTC_NODE_SYSDATE_MASK;
            aQtcColumn->lflag |= QTC_NODE_SYSDATE_EXIST;

            *aFindColumn = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        /* nothing to do. */
    }

    // BUG-36902
    if ( ( (aStatement->spvEnv->createProc != NULL) ||
           (aStatement->spvEnv->createPkg != NULL) ) &&
         ( *aFindColumn == ID_TRUE ) )
    {
        IDE_TEST( qsvProcStmts::setUseDate( aStatement->spvEnv )
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

IDE_RC qmvQTC::searchLevel(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGH,
    qtcNode         * aQtcColumn,
    idBool          * aFindColumn)
{
    qtcNode         * sNode[2] = {NULL,NULL};

    IDU_FIT_POINT_FATAL( "qmvQTC::searchLevel::__FT__" );

    if( ( idlOS::strMatch(
              aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
              aQtcColumn->columnName.size,
              (SChar *)"LEVEL", 5) == 0 ) )
    {
        if( aSFWGH == NULL )
        {
            // BUG-17774
            // INSERT, DDL문은 SFWGH가 없다.
            // ex) insert into t1 values ( level );
            // ex) create table t1 ( i1 integer default level );

            // make tuple for LEVEL
            IDE_TEST(makeOneTupleForPseudoColumn(
                         aStatement,
                         aQtcColumn,
                         (SChar *)"BIGINT",
                         6)
                     != IDE_SUCCESS);
        }
        else
        {
            if( aSFWGH->level == NULL )
            {
                IDE_TEST( qtc::makeColumn(
                              aStatement,
                              sNode,
                              NULL,
                              NULL,
                              &aQtcColumn->columnName,
                              NULL )
                          != IDE_SUCCESS);

                aSFWGH->level = sNode[0];

                // make tuple for LEVEL
                IDE_TEST(makeOneTupleForPseudoColumn(
                             aStatement,
                             aSFWGH->level,
                             (SChar *)"BIGINT",
                             6)
                         != IDE_SUCCESS);

                aSFWGH->level->node.lflag &= ~(MTC_NODE_INDEX_MASK);

                aSFWGH->level->lflag &= ~QTC_NODE_LEVEL_MASK;
                aSFWGH->level->lflag |= QTC_NODE_LEVEL_EXIST;
            }
            else
            {
                // Nothing to do.
            }

            aQtcColumn->node.table = aSFWGH->level->node.table;
            aQtcColumn->node.column = aSFWGH->level->node.column;
        }

        aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);

        aQtcColumn->lflag &= ~QTC_NODE_LEVEL_MASK;
        aQtcColumn->lflag |= QTC_NODE_LEVEL_EXIST;

        *aFindColumn = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchLoopLevel( qcStatement     * aStatement,
                                qmsSFWGH        * aSFWGH,
                                qtcNode         * aQtcColumn,
                                idBool          * aFindColumn )
{
    qtcNode   * sNode[2] = { NULL,NULL };

    IDU_FIT_POINT_FATAL( "qmvQTC::searchLoopLevel::__FT__" );

    if ( ( idlOS::strMatch(
               aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
               aQtcColumn->columnName.size,
               (SChar *)"LOOP_LEVEL", 10 ) == 0 ) )
    {
        if ( aSFWGH == NULL )
        {
            // make tuple for LOOP LEVEL
            IDE_TEST( makeOneTupleForPseudoColumn(
                          aStatement,
                          aQtcColumn,
                          (SChar *)"BIGINT",
                          6 )
                      != IDE_SUCCESS );
        }
        else
        {
            if( aSFWGH->loopLevel == NULL )
            {
                IDE_TEST( qtc::makeColumn(
                              aStatement,
                              sNode,
                              NULL,
                              NULL,
                              &aQtcColumn->columnName,
                              NULL )
                          != IDE_SUCCESS);

                aSFWGH->loopLevel = sNode[0];

                // make tuple for MULTIPLIER LEVEL
                IDE_TEST(makeOneTupleForPseudoColumn(
                             aStatement,
                             aSFWGH->loopLevel,
                             (SChar *)"BIGINT",
                             6)
                         != IDE_SUCCESS);

                aSFWGH->loopLevel->node.lflag &= ~(MTC_NODE_INDEX_MASK);
            }
            else
            {
                // Nothing to do.
            }

            aQtcColumn->node.table = aSFWGH->loopLevel->node.table;
            aQtcColumn->node.column = aSFWGH->loopLevel->node.column;
        }

        aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);

        aQtcColumn->lflag &= ~QTC_NODE_LOOP_LEVEL_MASK;
        aQtcColumn->lflag |= QTC_NODE_LOOP_LEVEL_EXIST;
        
        *aFindColumn = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchLoopValue( qcStatement     * aStatement,
                                qmsSFWGH        * aSFWGH,
                                qtcNode         * aQtcColumn,
                                idBool          * aFindColumn )
{
    qtcNode          * sNode;
    qcuSqlSourceInfo   sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvQTC::searchLoopValue::__FT__" );

    if ( ( idlOS::strMatch(
               aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
               aQtcColumn->columnName.size,
               (SChar *)"LOOP_VALUE", 10 ) == 0 ) )
    {
        if ( aSFWGH == NULL )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aQtcColumn->columnName) );
            IDE_RAISE( ERR_NOT_FOUND );
        }
        else
        {
            // Nothing to do.
        }
        
        if ( aSFWGH->thisQuerySet == NULL )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aQtcColumn->columnName) );
            IDE_RAISE( ERR_NOT_FOUND );
        }
        else
        {
            // Nothing to do.
        }
        
        if ( aSFWGH->thisQuerySet->loopNode == NULL )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aQtcColumn->columnName) );
            IDE_RAISE( ERR_NOT_FOUND );
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_TEST( qtc::makePassNode( aStatement,
                                     aQtcColumn,
                                     aSFWGH->thisQuerySet->loopNode,
                                     & sNode )
                  != IDE_SUCCESS );

        IDE_DASSERT( aQtcColumn == sNode );

        aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);

        aQtcColumn->lflag &= ~QTC_NODE_LOOP_VALUE_MASK;
        aQtcColumn->lflag |= QTC_NODE_LOOP_VALUE_EXIST;
        
        *aFindColumn = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchConnectByIsLeaf( qcStatement     * aStatement,
                                      qmsSFWGH        * aSFWGH,
                                      qtcNode         * aQtcColumn,
                                      idBool          * aFindColumn )
{
    qtcNode         * sNode[2] = {NULL,NULL};

    IDU_FIT_POINT_FATAL( "qmvQTC::searchConnectByIsLeaf::__FT__" );

    if( ( idlOS::strMatch(
              aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
              aQtcColumn->columnName.size,
              (SChar *)"CONNECT_BY_ISLEAF", 17) == 0 ) )
    {
        IDE_TEST_RAISE( aSFWGH            == NULL, ERR_NO_HIERARCHY );
        IDE_TEST_RAISE( aSFWGH->hierarchy == NULL, ERR_NO_HIERARCHY );

        if( aSFWGH->isLeaf == NULL )
        {
            IDE_TEST( qtc::makeColumn(
                          aStatement,
                          sNode,
                          NULL,
                          NULL,
                          &aQtcColumn->columnName,
                          NULL )
                      != IDE_SUCCESS);

            aSFWGH->isLeaf = sNode[0];

            // make tuple for LEVEL
            IDE_TEST(makeOneTupleForPseudoColumn(
                         aStatement,
                         aSFWGH->isLeaf,
                         (SChar *)"BIGINT",
                         6)
                     != IDE_SUCCESS);

            aSFWGH->isLeaf->node.lflag &= ~(MTC_NODE_INDEX_MASK);

            aSFWGH->isLeaf->lflag &= ~QTC_NODE_ISLEAF_MASK;
            aSFWGH->isLeaf->lflag |= QTC_NODE_ISLEAF_EXIST;
        }
        else
        {
            /* Nothing to do. */
        }

        aQtcColumn->node.table = aSFWGH->isLeaf->node.table;
        aQtcColumn->node.column = aSFWGH->isLeaf->node.column;
        aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);

        aQtcColumn->lflag &= ~QTC_NODE_ISLEAF_MASK;
        aQtcColumn->lflag |= QTC_NODE_ISLEAF_EXIST;

        *aFindColumn = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_HIERARCHY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_ISLEAF_NEED_CONNECT_BY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::searchRownum(
    qcStatement     * aStatement,
    qmsSFWGH        * aSFWGH,
    qtcNode         * aQtcColumn,
    idBool          * aFindColumn)
{
    qtcNode         * sNode[2] = {NULL,NULL};

    IDU_FIT_POINT_FATAL( "qmvQTC::searchRownum::__FT__" );

    if( ( idlOS::strMatch(
              aQtcColumn->columnName.stmtText + aQtcColumn->columnName.offset,
              aQtcColumn->columnName.size,
              (SChar *)"ROWNUM", 6) == 0 ) )
    {
        if( aSFWGH == NULL )
        {
            // BUG-17774
            // INSERT, DDL문은 SFWGH가 없다.
            // ex) insert into t1 values ( rownum );
            // ex) create table t1 ( i1 integer default rownum );

            // make tuple for ROWNUM
            IDE_TEST(makeOneTupleForPseudoColumn(
                         aStatement,
                         aQtcColumn,
                         (SChar *)"BIGINT",
                         6)
                     != IDE_SUCCESS);
        }
        else
        {
            if( aSFWGH->rownum == NULL )
            {
                IDE_TEST( qtc::makeColumn(
                              aStatement,
                              sNode,
                              NULL,
                              NULL,
                              &aQtcColumn->columnName,
                              NULL )
                          != IDE_SUCCESS);

                aSFWGH->rownum = sNode[0];

                // make tuple for ROWNUM
                IDE_TEST(makeOneTupleForPseudoColumn(
                             aStatement,
                             aSFWGH->rownum,
                             (SChar *)"BIGINT",
                             6)
                         != IDE_SUCCESS);

                aSFWGH->rownum->node.lflag &= ~(MTC_NODE_INDEX_MASK);

                aSFWGH->rownum->lflag &= ~QTC_NODE_ROWNUM_MASK;
                aSFWGH->rownum->lflag |= QTC_NODE_ROWNUM_EXIST;
            }
            else
            {
                // Nothing to do.
            }

            aQtcColumn->node.table = aSFWGH->rownum->node.table;
            aQtcColumn->node.column = aSFWGH->rownum->node.column;
        }

        aQtcColumn->node.lflag &= ~(MTC_NODE_INDEX_MASK);

        aQtcColumn->lflag &= ~QTC_NODE_ROWNUM_MASK;
        aQtcColumn->lflag |= QTC_NODE_ROWNUM_EXIST;

        *aFindColumn = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::addViewColumnRefList(
    qcStatement     * aStatement,
    qtcNode         * aQtcColumn )
{
    qcTableMap         * sTableMap;
    qmsFrom            * sFrom;
    qmsTableRef        * sTableRef;
    qmsColumnRefList   * sColumnRefNode;

    IDU_FIT_POINT_FATAL( "qmvQTC::addViewColumnRefList::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQtcColumn != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTableMap = QC_SHARED_TMPLATE(aStatement)->tableMap;

    //------------------------------------------
    // view column reference list에 기록
    //------------------------------------------

    sFrom = sTableMap[aQtcColumn->node.table].from;

    if( sFrom != NULL )
    {
        sTableRef = sFrom->tableRef;

        if( sTableRef->view != NULL )
        {    
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                    qmsColumnRefList,
                                    (void*) & sColumnRefNode )
                      != IDE_SUCCESS);

            sColumnRefNode->column    = aQtcColumn;
            sColumnRefNode->orgColumn = NULL;
            sColumnRefNode->isMerged  = ID_FALSE;
            sColumnRefNode->next      = sTableRef->viewColumnRefList;

            /*
             * PROJ-2469 Optimize View Materialization
             * 1. isUsed          : DEFAULT TRUE( Optimization 시점에 사용된다. )
             * 2. usedInTarget    : Target에서 참조 되었는지 여부
             * 3. targetOrder     : Target Validation 시점 일 때, 몇 번 째 Target 인지를 저장.
             * 4. viewTargetOrder : 해당 노드가 존재하는 View에서의 Target 위치
             */
            sColumnRefNode->isUsed          = ID_TRUE;
            sColumnRefNode->usedInTarget    = ID_FALSE;
            sColumnRefNode->targetOrder     = ID_USHORT_MAX;
            sColumnRefNode->viewTargetOrder = aQtcColumn->node.column;
            
            sTableRef->viewColumnRefList = sColumnRefNode;
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


IDE_RC qmvQTC::addViewColumnRefListForTarget( qcStatement     * aStatement,
                                              qtcNode         * aQtcColumn,
                                              UShort            aTargetOrder,
                                              UShort            aViewTargetOrder )
{
    qcTableMap         * sTableMap;
    qmsFrom            * sFrom;
    qmsTableRef        * sTableRef;
    qmsColumnRefList   * sColumnRefNode;

    IDU_FIT_POINT_FATAL( "qmvQTC::addViewColumnRefListForTarget::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQtcColumn != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTableMap = QC_SHARED_TMPLATE( aStatement )->tableMap;

    //------------------------------------------
    // view column reference list에 기록
    //------------------------------------------

    sFrom = sTableMap[ aQtcColumn->node.table ].from;

    if ( sFrom != NULL )
    {
        sTableRef = sFrom->tableRef;

        if ( sTableRef->view != NULL )
        {    
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                    qmsColumnRefList,
                                    ( void* ) & sColumnRefNode )
                      != IDE_SUCCESS);

            sColumnRefNode->column    = aQtcColumn;
            sColumnRefNode->orgColumn = NULL;
            sColumnRefNode->isMerged  = ID_FALSE;
            sColumnRefNode->next      = sTableRef->viewColumnRefList;

            /*
             * PROJ-2469 Optimize View Materialization
             * 1. isUsed          : DEFAULT TRUE( Optimization 시점에 사용된다. )
             * 2. usedInTarget    : Target에서 참조 되었는지 여부
             * 3. targetOrder     : Target Validation 시점 일 때, 몇 번 째 Target 인지를 저장.
             * 4. viewTargetOrder : 해당 노드가 존재하는 View에서의 Target 위치
             */
            sColumnRefNode->isUsed          = ID_TRUE;
            sColumnRefNode->usedInTarget    = ID_TRUE;
            sColumnRefNode->targetOrder     = aTargetOrder;
            sColumnRefNode->viewTargetOrder = aViewTargetOrder;
            
            sTableRef->viewColumnRefList = sColumnRefNode;
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

IDE_RC qmvQTC::changeModuleToArray( qtcNode      * aNode,
                                    mtcCallBack  * aCallBack )
{
/**********************************************************************************
 *
 * Description : PROJ-2533
 *    function object에 대해서 각각의 object에 맞게 node 변경
 *
 * Implementation :
 *    이 함수로 올 수 있는 경우의 함수 유형 
 *    (1) columnName ( list_expr ) 또는 ()
 *        - arrayVar             -> columnModule
 *        - proc/funcName        -> spFunctionCallModule
 *    (2) tableName.columnName ( list_expr) 또는 ()
 *        - arrayVar.memberFunc  -> each member function module
 *        - label.arrayVar       -> columnModule
 *        - pkg.arrayVar         -> columnModule
 *          pkg.proc/func        -> spFunctionCallModule
 *        - user.proc/func       -> spFunctionCallModule
 *    (3) userName.tableName.columnName( list_expr ) 또는 ()
 *        - pkg.arrVar.memberFunc -> each member function module
 *        - user.pkg.arrVar       -> columnModule
 *          user.pkg.proc/func    -> spFunctionCallModule
 *    * userName.tableName.columnName.pkgName( list_expr ) 은 절대 올 수 없음.
 *      항상 array의 member function이기 때문에
 *********************************************************************************/
    idBool                sFindObj          = ID_FALSE;
    qtcCallBackInfo     * sCallBackInfo;
    qcStatement         * sStatement;
    qsVariables         * sArrayVariable    = NULL;
    ULong                 sPrevQtcNodelflag = 0;
    const mtfModule     * sMemberFuncModule = NULL;

    IDU_FIT_POINT_FATAL( "qmvQTC::changeModuleToArray::__FT__" );

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sStatement    = sCallBackInfo->statement;

    IDE_DASSERT( aNode != NULL );

    // BUG-42790
    // column 모듈인 경우, array인지 확인할 필요가 없다.
    if ( ( aNode->node.module != &qtc::columnModule ) &&
         ( ( (aNode->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) == QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ) ) 
    {
        IDE_DASSERT( sStatement != NULL );
        IDE_DASSERT( QC_IS_NULL_NAME( (aNode->columnName) ) == ID_FALSE );

        sPrevQtcNodelflag = aNode->lflag;

        if ( ( sStatement->spvEnv->createProc != NULL ) ||
             ( sStatement->spvEnv->createPkg != NULL ) )
        {
            IDE_TEST( qsvProcVar::searchVarAndParaForArray( sStatement,
                                                            aNode,
                                                            &sFindObj,
                                                            &sArrayVariable,
                                                            &sMemberFuncModule )
                      != IDE_SUCCESS);

            if ( sFindObj == ID_FALSE )
            {
                IDE_TEST( qsvProcVar::searchVariableFromPkgForArray( sStatement,
                                                                     aNode,
                                                                     &sFindObj,
                                                                     &sArrayVariable,
                                                                     &sMemberFuncModule )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            if ( qcg::isInitializedMetaCaches() == ID_TRUE ) 
            {
                IDE_TEST( qsvProcVar::searchVariableFromPkgForArray( sStatement,
                                                                     aNode,
                                                                     &sFindObj,
                                                                     &sArrayVariable,
                                                                     &sMemberFuncModule )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }

        aNode->lflag = sPrevQtcNodelflag;

        if ( sFindObj == ID_TRUE )
        {
            if ( sMemberFuncModule != NULL )
            {
                /* array의 memberfunction인 경우
                   parser 에서 할당 받은 mtcColumn 공간을 재사용해도 된다. */
                aNode->node.module = sMemberFuncModule;
                aNode->node.lflag  = aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
                aNode->node.lflag |= sMemberFuncModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;

                aNode->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
                aNode->lflag |= QTC_NODE_PROC_FUNCTION_FALSE;
                aNode->lflag &= ~QTC_NODE_SP_ARRAY_INDEX_VAR_MASK;
                aNode->lflag |= QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT;
            }
            else
            {
                /* array인 경우 estimate과정에서
                   mtcColumn 공간(MTC_TUPLE_TYPE_VARIABLE)을 재할당 받는다. */
                aNode->node.module = &qtc::columnModule;
                aNode->node.lflag  = aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK;
                aNode->node.lflag |= qtc::columnModule.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;

                aNode->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
                aNode->lflag |= QTC_NODE_PROC_FUNCTION_FALSE;
            }
        }
        else
        {
            /* function인 경우 parser에서 mtcColumn 공간을 이미 할당 받았다. */
            aNode->lflag &= ~QTC_NODE_SP_ARRAY_INDEX_VAR_MASK;
            aNode->lflag |= QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT;
        }
    }
    else
    {
        // Nothing to do.
    }
    
    /* BUG-42639 Monitoring query */
    if ( aNode->node.module == &qtc::spFunctionCallModule )
    {
        if ( sStatement != NULL )
        {
            QC_SHARED_TMPLATE(sStatement)->flag &= ~QC_TMP_ALL_FIXED_TABLE_MASK;
            QC_SHARED_TMPLATE(sStatement)->flag |= QC_TMP_ALL_FIXED_TABLE_FALSE;
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvQTC::setColumnIDForShardTransView( qcStatement  * aStatement,
                                             qmsSFWGH     * aSFWGHOfCallBack,
                                             qtcNode      * aQtcColumn,
                                             idBool       * aIsFound,
                                             qmsTableRef ** aTableRef )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *    Shard aggr transformation으로 생성된 하위 inLineView의 target의 순서에 맞게
 *    table 및 column을 세팅한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmsSFWGH            * sSFWGH = aSFWGHOfCallBack;
    qmsParseTree        * sChildParseTree;
    qmsTarget           * sChildTarget;
    qtcNode             * sTargetInfo;
    qmsFrom             * sChildFrom;
    UShort                sColumnPosition = 0;

    IDU_FIT_POINT_FATAL( "qmvQTC::setColumnIDForShardTransView::__FT__" );

    if ( ( *aIsFound == ID_FALSE  ) &&
         ( ( aQtcColumn->lflag & QTC_NODE_SHARD_VIEW_TARGET_REF_MASK )
           == QTC_NODE_SHARD_VIEW_TARGET_REF_TRUE ) )
    {
        // Transformed aggregate function의 argument는 column module로 임의생성 되었기 때문에,
        // 해당 column node에 미리 세팅 해 둔 target position으로 column id를 지정해준다.
        aQtcColumn->node.table      = sSFWGH->from->tableRef->table;
        aQtcColumn->node.baseTable  = sSFWGH->from->tableRef->table;
        aQtcColumn->node.column     = aQtcColumn->shardViewTargetPos;
        aQtcColumn->node.baseColumn = aQtcColumn->shardViewTargetPos;

        *aIsFound = ID_TRUE;
        *aTableRef = sSFWGH->from->tableRef;

        if ( sSFWGH->validatePhase == QMS_VALIDATE_TARGET )
        {
            IDE_TEST( addViewColumnRefListForTarget( aStatement,
                                                     aQtcColumn,
                                                     sSFWGH->currentTargetNum,
                                                     aQtcColumn->shardViewTargetPos )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( addViewColumnRefList( aStatement,
                                            aQtcColumn )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sChildParseTree = ( qmsParseTree * )sSFWGH->from->tableRef->view->myPlan->parseTree;

        // 하위 inLineView의 FromTree에서 찾는다.
        for ( sChildFrom  = sChildParseTree->querySet->SFWGH->from;
              sChildFrom != NULL;
              sChildFrom  = sChildFrom->next )
        {
            IDE_TEST( searchColumnInFromTree( aStatement,
                                              sChildParseTree->querySet->SFWGH, /* BUG-48679 */
                                              aQtcColumn,
                                              sChildFrom,
                                              aTableRef )
                      != IDE_SUCCESS);
        }

        // 하위 inLineView의 FromTree에서 찾았다면, 다시 view의 Target을 보고 table, column을 조정한다.
        if ( *aTableRef != NULL )
        {
            for ( sChildTarget = sChildParseTree->querySet->SFWGH->target, sColumnPosition = 0;
                  sChildTarget != NULL;
                  sChildTarget = sChildTarget->next, sColumnPosition++ )
            {
                if ( sChildTarget->targetColumn->node.module == &qtc::passModule )
                {
                    sTargetInfo = (qtcNode*)(sChildTarget->targetColumn->node.arguments);
                }
                else
                {
                    sTargetInfo = sChildTarget->targetColumn;
                }

                if ( ( aQtcColumn->node.table == sTargetInfo->node.table ) &&
                     ( aQtcColumn->node.column == sTargetInfo->node.column ) )
                {
                    aQtcColumn->node.table      = sSFWGH->from->tableRef->table;
                    aQtcColumn->node.baseTable  = sSFWGH->from->tableRef->table;
                    aQtcColumn->node.column     = sColumnPosition;
                    aQtcColumn->node.baseColumn = sColumnPosition;

                    // PROJ-2469 Optimize View Materialization
                    // Target Column의 경우 View Column Ref 등록 시 Target에서 참조되는지 여부와,
                    // 몇 번 째 Target 인지를 인자로 전달한다.
                    if ( sSFWGH->validatePhase == QMS_VALIDATE_TARGET )
                    {
                        IDE_TEST( addViewColumnRefListForTarget( aStatement,
                                                                 aQtcColumn,
                                                                 sSFWGH->currentTargetNum,
                                                                 sColumnPosition )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        IDE_TEST( addViewColumnRefList( aStatement,
                                                     aQtcColumn )
                                  != IDE_SUCCESS );
                    }

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            *aIsFound = ID_TRUE;
        }
        else
        {
            *aIsFound = ID_FALSE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-7219 */
IDE_RC qmvQTC::setColumnIDOfOrderByForShard( qcStatement  * aStatement,
                                             qmsSFWGH     * aSFWGH,
                                             qtcNode      * aQtcColumn,
                                             idBool       * aIsFound )
{
 /****************************************************************************************
 *
 * Description : 하위에 Shard View가 있는 경우, Shard View QuerySet에서 Order By 대상을
 *               찾고, Shard View Target의 정보로 보정한다. 
 *               Append로 추가된 Shard Col Trans Node를 찾도록 유도한다.
 *               이 함수는 qmvQTC::setColumnIDForShardTransView 를 참조하였다.
 *
 * Implementation : 1. 하위에 Shard View가 있는지 검사한다.
 *                  2. Shard View QuerySet에서 Order By 대상을 찾는다.
 *                  3. 찾았다면, Shard View Target 정보로 보정한다.
 *
 ****************************************************************************************/

    qtcCallBackInfo sCallBackInfo = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };
    mtcCallBack sCallBack = {
        & sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE,
        NULL,
        NULL
    };
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsTarget    * sTarget    = NULL;
    qtcNode      * sColumn    = NULL;
    UShort         sPosition  = 0;
    UShort         sTable     = 0;
    idBool         sIsFound   = ID_FALSE;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQtcColumn == NULL, ERR_NULL_COLUMN );

    IDE_TEST_CONT( aSFWGH == NULL, NORMAL_EXIT );

    /* 1. 하위에 Shard View가 있는지 검사한다. */
    if ( ( aSFWGH->lflag & QMV_SFWGH_SHARD_TRANS_VIEW_MASK ) == QMV_SFWGH_SHARD_TRANS_VIEW_TRUE )
    {
        sParseTree = ( qmsParseTree * )aSFWGH->from->tableRef->view->myPlan->parseTree;
        sQuerySet  = sParseTree->querySet;

        sCallBackInfo.statement = aStatement;
        sCallBackInfo.querySet  = sQuerySet;

        /* 2. Shard View QuerySet에서 Order By 대상을 찾는다. */
        IDE_TEST( setColumnIDOfOrderBy( aQtcColumn,
                                        &( sCallBack ),
                                        &( sIsFound ) )
                  != IDE_SUCCESS );

        /*  3. 찾았다면, Shard View Target 정보로 보정한다. */
        if ( sIsFound == ID_TRUE )
        {
            sTable = aSFWGH->from->tableRef->table;

            while ( sQuerySet->setOp != QMS_NONE )
            {
                sQuerySet = sQuerySet->left;
            }

            for ( sTarget  = sQuerySet->SFWGH->target, sPosition = 0;
                  sTarget != NULL;
                  sTarget  = sTarget->next, sPosition++ )
            {
                if ( sTarget->targetColumn->node.module == &qtc::passModule )
                {
                    sColumn = (qtcNode*)( sTarget->targetColumn->node.arguments );
                }
                else
                {
                    sColumn = sTarget->targetColumn;
                }

                if ( ( aQtcColumn->node.table  == sColumn->node.table ) &&
                     ( aQtcColumn->node.column == sColumn->node.column ) )
                {
                    aQtcColumn->node.table      = sTable;
                    aQtcColumn->node.baseTable  = sTable;
                    aQtcColumn->node.column     = sPosition;
                    aQtcColumn->node.baseColumn = sPosition;

                    /* PROJ-2469 Optimize View Materialization */
                    IDE_TEST( addViewColumnRefList( aStatement,
                                                    aQtcColumn )
                              != IDE_SUCCESS );

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            IDE_TEST_RAISE( sTarget == NULL, ERR_NON_TARGET );
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

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    if ( aIsFound != NULL )
    {
        *aIsFound = sIsFound;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvQTC::setColumnIDOfOrderByForShard",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_COLUMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvQTC::setColumnIDOfOrderByForShard",
                                  "column is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NON_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvQTC::setColumnIDOfOrderByForShard",
                                  "non target" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
