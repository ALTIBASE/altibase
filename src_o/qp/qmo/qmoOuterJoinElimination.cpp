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
 * $Id: qmoOuterJoinElimination.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description : BUG-38339
 *     Outer Join Elimination
 * outer 조인의 on 절의 조건이 거짓일때 모두 outer 테이블의 값은 null 이 된다.
 * where 절이나 inner 조인 에서 outer 조인 의 테이블을 참조한다면 outer 조인 연산을 수행할 필요가 없게 된다.
 *
 * is null 과 같이 predicate 에서 null 값을 필요할때가 있다.
 * 또한 or 연산자가 있으면 outer 조인 의 테이블을 참조할수도 있고 안할수도 있기 때문에 경우에 따라서 결과가 틀려진다.
 *
 * Implementation :
 *     1.  from 절 트리를 순회를 하면서 다음과 같은 작업을 수행한다.
 *     2.  where 절과 inner 조인의 on절의 디펜던시를 모두 수집한다.
       2.1 Oracle Style Join 인 경우에는 수집하지 않는다.
 *     2.2 예외 조건인 경우( is null 등 )에는 수집된 디펜던시에서 제거를 한다.
 *     3.  left outer 조인일때는 right 의 디펜던시가 수집된 디펜던시에 포함되는지 검사한다.
 *     3.1 포함된다면 inner join 으로 변경한다.
 *     3.2 inner join 이 되었으므로 on 절의 디펜던시도 수집한다.
 *
 **********************************************************************/

#include <ide.h>
#include <qcgPlan.h>
#include <qmoOuterJoinElimination.h>

IDE_RC
qmoOuterJoinElimination::doTransform( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet,
                                      idBool      * aChanged )
{
    idBool          sChanged = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinElimination::doTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    //------------------------------------------
    // Outer Join Elimination 수행
    //------------------------------------------

    if ( QCU_OPTIMIZER_OUTERJOIN_ELIMINATION == 1 )
    {
        IDE_TEST( doTransformQuerySet( aStatement,
                                       aQuerySet,
                                       & sChanged )
                  != IDE_SUCCESS );

        if ( sChanged == ID_TRUE )
        {
            qcgPlan::registerPlanProperty(
                aStatement,
                PLAN_PROPERTY_OPTIMIZER_OUTERJOIN_ELIMINATION );
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

    *aChanged = sChanged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOuterJoinElimination::doTransformQuerySet( qcStatement * aStatement,
                                              qmsQuerySet * aQuerySet,
                                              idBool      * aChanged )
{
    qmsFrom * sFrom;
    qcDepInfo sFindDependencies;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinElimination::doTransformQuerySet::__FT__" );

    if ( aQuerySet->setOp == QMS_NONE )
    {
        for ( sFrom  = aQuerySet->SFWGH->from;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {
            qtc::dependencyClear( &sFindDependencies );

            IDE_TEST( addWhereDep( (mtcNode*)aQuerySet->SFWGH->where,
                                   & sFindDependencies )
                      != IDE_SUCCESS );

            removeDep( (mtcNode*)aQuerySet->SFWGH->where, &sFindDependencies );

            IDE_TEST( doTransformFrom( aStatement,
                                       aQuerySet->SFWGH,
                                       sFrom,
                                       & sFindDependencies,
                                       aChanged )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // SET 연산인 경우 각 query block 별 transformation을 시도 한다.
        IDE_TEST( doTransformQuerySet( aStatement,
                                       aQuerySet->left,
                                       aChanged )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformQuerySet( aStatement,
                                       aQuerySet->right,
                                       aChanged )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOuterJoinElimination::doTransformFrom( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          qmsFrom     * aFrom,
                                          qcDepInfo   * aFindDependencies,
                                          idBool      * aChanged )
{
    IDU_FIT_POINT_FATAL( "qmoOuterJoinElimination::doTransformFrom::__FT__" );

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        // Nothing to do.
    }
    else if ( aFrom->joinType == QMS_INNER_JOIN )
    {
        IDE_TEST( addOnConditionDep( aFrom,
                                     aFrom->onCondition,
                                     aFindDependencies )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->left,
                                   aFindDependencies,
                                   aChanged )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->right,
                                   aFindDependencies,
                                   aChanged )
                  != IDE_SUCCESS );
    }
    else if ( aFrom->joinType == QMS_LEFT_OUTER_JOIN )
    {
        if ( qtc::dependencyContains( aFindDependencies,
                                      &(aFrom->right->depInfo) ) == ID_TRUE )
        {
            aFrom->joinType = QMS_INNER_JOIN;

            // BUG-38375
            // 다음 질의에서 모두 inner join 으로 변환이 가능하다.
            // select * from t1 left outer join t2 on t1.i1=t2.i1
            //                  left outer join t3 on t2.i2=t3.i2
            //                  where t3.i3=1;
            IDE_TEST( addOnConditionDep( aFrom,
                                         aFrom->onCondition,
                                         aFindDependencies )
                      != IDE_SUCCESS );

            *aChanged = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->left,
                                   aFindDependencies,
                                   aChanged )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->right,
                                   aFindDependencies,
                                   aChanged )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->left,
                                   aFindDependencies,
                                   aChanged )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->right,
                                   aFindDependencies,
                                   aChanged )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOuterJoinElimination::addWhereDep( mtcNode   * aNode,
                                      qcDepInfo * aDependencies )
{
    mtcNode   * sNode;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinElimination::addWhereDep::__FT__" );

    for ( sNode  = aNode;
          sNode != NULL;
          sNode  = sNode->next )
    {
        if ( ( sNode->lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
             == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            IDE_TEST( addWhereDep( sNode->arguments,
                                   aDependencies )
                      != IDE_SUCCESS );
        }
        else
        {
            // BUG-38375 Oracle Style Join 인 경우에는 수집하지 않는다.
            if ( ( ((qtcNode*)sNode)->lflag  & QTC_NODE_JOIN_OPERATOR_MASK )
                 == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                // Nothing to do.
            }
            else
            {
                IDE_TEST( qtc::dependencyOr( &( ((qtcNode*)sNode)->depInfo ),
                                             aDependencies,
                                             aDependencies )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoOuterJoinElimination::addOnConditionDep( qmsFrom   * aFrom,
                                            qtcNode   * aNode,
                                            qcDepInfo * aFindDependencies )
{
    qcDepInfo   sFindDependencies;

    IDU_FIT_POINT_FATAL( "qmoOuterJoinElimination::addOnConditionDep::__FT__" );

    IDE_DASSERT( aFrom->joinType == QMS_INNER_JOIN );

    if ( aNode != NULL )
    {
        qtc::dependencyClear( & sFindDependencies );

        qtc::dependencyAnd( & aFrom->depInfo,
                            & aNode->depInfo,
                            & sFindDependencies );

        IDE_TEST( qtc::dependencyOr( & sFindDependencies,
                                     aFindDependencies,
                                     aFindDependencies )
                  != IDE_SUCCESS );

        removeDep( (mtcNode*)aNode, aFindDependencies );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmoOuterJoinElimination::removeDep( mtcNode     * aNode,
                                    qcDepInfo   * aFindDependencies )
{
    UInt        sCount;
    mtcNode   * sNode;
    qcDepInfo * sDependencies;

    for( sNode   = aNode;
         sNode  != NULL;
         sNode   = sNode->next )
    {
        // BUG-44692 subquery가 있을때 OJE 가 동작하면 안됩니다.
        // OJE 기능이 정상 동작하기 위해서는 값이 null 일때 true 가 리턴되면 안됩니다.
        // 서브쿼리의 경우 내부 질의에 따라서 true 가 리턴될수 있습니다.
        // BUG-44781 anti join 이 있을때 OJE 가 동작하면 안됩니다.
        if ( ((sNode->lflag & MTC_NODE_EAT_NULL_MASK) == MTC_NODE_EAT_NULL_TRUE) ||
             ((sNode->lflag & MTC_NODE_OPERATOR_MASK) == MTC_NODE_OPERATOR_OR) ||
             (((sNode->lflag & MTC_NODE_OPERATOR_MASK) == MTC_NODE_OPERATOR_SUBQUERY) && (QCU_OPTIMIZER_BUG_44692 == 1)) ||
             (((((qtcNode*)sNode)->lflag & QTC_NODE_JOIN_TYPE_MASK) == QTC_NODE_JOIN_TYPE_ANTI) && (QCU_OPTIMIZER_BUG_44692 == 1)) )
        {
            sDependencies = &(((qtcNode*)sNode)->depInfo);

            for ( sCount = 0;
                  sCount < sDependencies->depCount;
                  sCount++ )
            {
                qtc::dependencyRemove( sDependencies->depend[sCount],
                                       aFindDependencies,
                                       aFindDependencies );
            }
        }
        else
        {
            removeDep( sNode->arguments, aFindDependencies );
        }
    }
}
