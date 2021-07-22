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
 * $Id$
 *
 * Description :
 *     ANSI Join Ordering
 *
 *     BUG-34295 ANSI style 쿼리의 join ordering 
 *     일부 제한된 조건에서 ANSI style join 에서 inner join 을 분리하여
 *     join order 를 optimizer 가 결정하도록 한다.
 *
 *     TODO : 모든 조건에서 cost 를 고려하여 모든 join 의 join order 를
 *            결정하도록 해야 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmoAnsiJoinOrder.h>
#include <qmoCnfMgr.h>
#include <qmoNormalForm.h>
#include <qmgLeftOuter.h>

IDE_RC 
qmoAnsiJoinOrder::traverseFroms( qcStatement * aStatement,
                                 qmsFrom     * aFrom,
                                 qmsFrom    ** aFromTree,
                                 qmsFrom    ** aFromArr,
                                 qcDepInfo   * aFromArrDep,
                                 idBool      * aMakeFail )
{
/***********************************************************************
 *
 * Description : qmsFrom 에서 outer join 의 driven table
 *               (left outer join 의 right) 을 aFromTree 로,
 *               그 외의 table 을 aFromArr 로 분류하는 함수.
 *  
 * 예) select * from t1 left outer join t2 on t1.i1 = t2.i1
 *                      inner join      t3 on t1.i1 = t3.i1
 *                      left outer join t4 on t1.i1 = t4.i1;
 *
 *     Input :        LOJ
 *     (aFrom)       /   \
 *                  IJ    t4
 *                 /  \
 *               LOJ    t3
 *              /   \
 *            t1     t2
 *
 *    Output :        LOJ 
 *    (aFromTree)    /   \
 *                 LOJ    t4
 *                /   \
 *             (t1)    t2
 *
 *    (aFromArr) :   t1 -> t3 
 *
 *    t1 은 left outer join 의 left 이므로 aFromArr 로 분류되지만,
 *    aFromTree 에서 제외할 경우 tree 구조가 무너지게 되므로 유지한다.
 *    이때문에 t1 이 aFromTree 와 aFromArr 양쪽에 존재하게 되어 모순이
 *    생기지만, graph 생성 및 optimize 가 완료된 후에 t1 대신 
 *    aFromArr 로 생성한 graph (base graph) 로 치환하여 문제를 해결한다.
 *
 *
 * Implemenation :
 *    qmsFrom 의 tree 가 왼쪽으로 치우친(skewed) 형태라고 가정하고 처리한다.
 *    Right outer join 이 left outer join 으로 변형되는 경우 왼쪽으로
 *    치우친 형태가 아닐 수 있지만, 그 경우에는 이 함수가 호출되어서는 안된다.
 *
 *    현재 qmsFrom 이 어느 타입인지 보고 하위 qmsFrom 을 분류하거나
 *    재귀적으로 호출한다.
 *
 *      QMS_NO_JOIN (table) :
 *          left outer join 의 left 쪽 이면서 일반 table 인 경우이다.
 *          aFromArr 과 aFromTree 양쪽에 복사한다.
 *      QMS_INNER_JOIN :
 *          right 에 달린 table 을 aFromArr 에 복사한다.
 *          left 는 재귀호출하여 처리한다.
 *      QMS_LEFT_OUTER_JOIN :
 *          현재 노드(left outer join)과 right 에 달린 table 을
 *          aFromTree 에 복사한다.
 *          left 는 재귀호출하여 처리한다.
 *
 ***********************************************************************/

    qmsFrom * sFrom;
    qmsFrom * sFromChild;
    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoAnsiJoinOrder::traverseFroms::__FT__" );

    if( aFrom->joinType == QMS_NO_JOIN )
    {
        if( *aFromArr != NULL )
        {
            IDE_TEST( cloneFrom( aStatement,
                                 aFrom,
                                 &sFrom )
                      != IDE_SUCCESS );

            IDE_TEST( appendFroms( aStatement,
                                   aFromArr,
                                   sFrom )
                      != IDE_SUCCESS );

            qtc::dependencyOr( aFromArrDep,
                               &sFrom->depInfo,
                               aFromArrDep );
        }

        if( *aFromTree != NULL )
        {
            IDE_TEST( cloneFrom( aStatement,
                                 aFrom,
                                 &sFrom )
                      != IDE_SUCCESS );

            (*aFromTree)->left = sFrom;
        }
    }
    else
    {
        // BUG-40028
        // qmsFrom 의 tree 가 왼쪽으로 치우친(skewed)형태가 아닐 때는
        // ANSI_JOIN_ORDERING 해서는 안된다.
        if ( aFrom->right->joinType != QMS_NO_JOIN )
        {
            *aMakeFail = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        if( aFrom->joinType == QMS_INNER_JOIN )
        {
            IDE_TEST( cloneFrom( aStatement,
                                 aFrom->right,
                                 &sFrom )
                      != IDE_SUCCESS );

            IDE_TEST( qmoNormalForm::normalizeCNF( aStatement,
                                                   aFrom->onCondition,
                                                   &sNode )
                      != IDE_SUCCESS );

            sFrom->onCondition = sNode;

            IDE_TEST( appendFroms( aStatement,
                                   aFromArr,
                                   sFrom )
                      != IDE_SUCCESS );

            qtc::dependencyOr( aFromArrDep,
                               &sFrom->depInfo,
                               aFromArrDep );

            IDE_TEST( traverseFroms( aStatement,
                                     aFrom->left,
                                     aFromTree,
                                     aFromArr,
                                     aFromArrDep,
                                     aMakeFail )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( cloneFrom( aStatement,
                                 aFrom,
                                 &sFrom )
                      != IDE_SUCCESS );

            if( *aFromTree == NULL )
            {
                *aFromTree = sFrom;

                IDE_TEST( cloneFrom( aStatement,
                                     aFrom->right,
                                     &sFromChild )
                          != IDE_SUCCESS );

                (*aFromTree)->right = sFromChild;

                IDE_TEST( traverseFroms( aStatement,
                                         aFrom->left,
                                         aFromTree,
                                         aFromArr,
                                         aFromArrDep,
                                         aMakeFail )
                          != IDE_SUCCESS );
            }
            else
            {
                (*aFromTree)->left = sFrom;

                IDE_TEST( cloneFrom( aStatement,
                                     aFrom->right,
                                     & sFromChild )
                          != IDE_SUCCESS );

                (*aFromTree)->left->right = sFromChild;

                IDE_TEST( traverseFroms( aStatement,
                                         aFrom->left,
                                         &((*aFromTree)->left),
                                         aFromArr,
                                         aFromArrDep,
                                         aMakeFail )
                          != IDE_SUCCESS );
            }

            // Reset dependency
            qtc::dependencyClear( &sFrom->depInfo );
            qtc::dependencyOr( &sFrom->left->depInfo,
                               &sFrom->right->depInfo,
                               &sFrom->depInfo );
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoAnsiJoinOrder::appendFroms( qcStatement  * aStatement,
                                      qmsFrom     ** aFromArr,
                                      qmsFrom      * aFrom )
{
    qmsFrom * sFrom;

    IDU_FIT_POINT_FATAL( "qmoAnsiJoinOrder::appendFroms::__FT__" );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsFrom) ,
                                             (void **)&sFrom )
              != IDE_SUCCESS);

    *sFrom = *aFrom;

    sFrom->next = NULL;

    if( *aFromArr == NULL )
    {
        *aFromArr = sFrom;
    }
    else
    {
        sFrom->next = *aFromArr;

        *aFromArr = sFrom;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoAnsiJoinOrder::cloneFrom( qcStatement * aStatement,
                             qmsFrom     * aFrom1,
                             qmsFrom    ** aFrom2 )
{
    qmsFrom * sFrom;

    IDU_FIT_POINT_FATAL( "qmoAnsiJoinOrder::cloneFrom::__FT__" );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsFrom) ,
                                             (void **)&sFrom )
              != IDE_SUCCESS);

    *sFrom = *aFrom1;

    sFrom->next = NULL;

    sFrom->left = NULL;

    sFrom->right = NULL;

    *aFrom2 = sFrom;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoAnsiJoinOrder::clonePredicate2List( qcStatement   * aStatement,
                                              qmoPredicate  * aPred1,
                                              qmoPredicate ** aPredList )
{
    qmoPredicate * sPred;
    qmoPredicate * sCursor;

    IDU_FIT_POINT_FATAL( "qmoAnsiJoinOrder::clonePredicate2List::__FT__" );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredicate) ,
                                             (void **)&sPred )
              != IDE_SUCCESS);

    *sPred = *aPred1;

    sPred->next = NULL;

    if( *aPredList == NULL )
    {
        *aPredList = sPred;
    }
    else
    {
        sCursor = *aPredList;

        while( 1 )
        {
            if( sCursor->next == NULL )
            {
                break;
            }
            else
            {
                sCursor = sCursor->next;
            }
        }

        sCursor->next = sPred;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoAnsiJoinOrder::mergeOuterJoinGraph2myGraph( qmoCNF * aCNF )
{
/***********************************************************************
 *
 * Description : outerJoinGraph 와 myGraph 의 병합
 *
 * Implemenation :
 *    BUG-34295 Join ordering ANSI style query
 *    outerJoinGraph 는 myGraph 와는 별도로 관리되는 outer join 을 위한
 *    graph 이다.
 *    이 함수는 outerJoinGraph 의 가장 좌측
 *    (left outer join 의 기준이 되는 위치)에 myGraph 를 연결한다.
 *
 ***********************************************************************/

    qmgGraph     * sIter = NULL;
    qmgGraph     * sPrev = NULL;

    IDU_FIT_POINT_FATAL( "qmoAnsiJoinOrder::mergeOuterJoinGraph2myGraph::__FT__" );

    IDE_DASSERT( aCNF != NULL );
    IDE_DASSERT( aCNF->outerJoinGraph != NULL );

    sIter = aCNF->outerJoinGraph;

    // BUG-39877 OPTIMIZER_ANSI_JOIN_ORDERING 에서 left, right 가 바뀔수 있음
    // 기존 알고리즘이 가장 왼쪽에 있는 그래프를 찾아서 변경하는 것이므로
    // selectedJoinMethod 이용하여 left를 판단해야 한다.
    while( sIter->left != NULL )
    {
        IDE_FT_ERROR_MSG( sIter->type == QMG_LEFT_OUTER_JOIN,
                          "Graph type : %u\n", sIter->type );

        sPrev = sIter;

        if ( (((qmgLOJN*)sIter)->selectedJoinMethod->flag & QMO_JOIN_METHOD_DIRECTION_MASK)
                == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
        {
            sIter = sIter->left;
        }
        else
        {
            sIter= sIter->right;
        }
    }

    IDE_FT_ERROR( sPrev != NULL );

    if ( (((qmgLOJN*)sPrev)->selectedJoinMethod->flag & QMO_JOIN_METHOD_DIRECTION_MASK)
            == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
    {
        sPrev->left = aCNF->myGraph;
    }
    else
    {
        sPrev->right = aCNF->myGraph;
    }

    aCNF->myGraph = aCNF->outerJoinGraph;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoAnsiJoinOrder::fixOuterJoinGraphPredicate( qcStatement * aStatement,
                                              qmoCNF      * aCNF )
{
/***********************************************************************
 *
 * Description : BUG-34295 Join ordering ANSI style query
 *    Where 절의 predicate 중 outerJoinGraph 와 연관된
 *    one table predicate 을 찾아 이동시킨다.
 *    outerJoinGraph 의 one table predicate 은 baseGraph 와
 *    dependency 가 겹치지 않아서 predicate 분류 과정에서
 *    constant predicate 으로 잘못 분류된다.
 *    이를 바로잡기 위해 sCNF->constantPredicate 의 predicate 들에서
 *    outerJoinGraph 에 관련된 one table predicate 들을 찾아내어
 *    outerJoinGraph 로 이동시킨다.
 *
 * Implemenation :
 *
 *
 ***********************************************************************/

    qmoCNF         * sCNF;
    qcDepInfo      * sFromDependencies;
    qcDepInfo      * sGraphDependencies;
    idBool           sIsOneTable = ID_FALSE;

    qmoPredicate   * sPred;
    qmoPredicate   * sNewConstantPred = NULL;

    IDU_FIT_POINT_FATAL( "qmoAnsiJoinOrder::fixOuterJoinGraphPredicate::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCNF != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sCNF               = aCNF;
    sFromDependencies  = &(sCNF->outerJoinGraph->depInfo);
    sGraphDependencies = &(sCNF->outerJoinGraph->myFrom->depInfo);

    // Extract one table predicate from CNF->constantPredicate (mis-placed pred)
    for( sPred = sCNF->constantPredicate;
         sPred != NULL;
         sPred = sPred->next )
    {
        IDE_TEST( qmoPred::isOneTablePredicate( sPred,
                                                sFromDependencies,
                                                sGraphDependencies,
                                                & sIsOneTable )
                  != IDE_SUCCESS );

        // Dependency 가 하나도 없어도 sIsOneTable 이 ID_TRUE 로 나오므로
        // dependency count 가 1 이상이여야 진짜 one table predicate 이다.
        if( ( sIsOneTable == ID_TRUE ) &&
            ( sPred->node->depInfo.depCount > 0 ) )
        {
            // flag setting
            sPred->flag &= ~QMO_PRED_CONSTANT_FILTER_MASK;
            sPred->flag |= QMO_PRED_CONSTANT_FILTER_FALSE;

            // add to outerJoinGraph
            IDE_TEST( clonePredicate2List( aStatement,
                                           sPred,
                                           &sCNF->outerJoinGraph->myPredicate )
                      != IDE_SUCCESS );
        }
        else
        {
            // add to new constant predicate list
            IDE_TEST( clonePredicate2List( aStatement,
                                           sPred,
                                           &sNewConstantPred )
                      != IDE_SUCCESS );
        }
    }

    // constantPredicate 에는 진짜 constant predicate 만 남는다.
    sCNF->constantPredicate = sNewConstantPred;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

