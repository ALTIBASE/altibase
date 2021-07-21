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
 **********************************************************************/

#include <idl.h>
#include <qmv.h>
#include <qmvQTC.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qtc.h>
#include <qcuProperty.h>
#include <qmoUnnesting.h>
#include <qmoViewMerging.h>
#include <qmoNormalForm.h>
#include <qmvQuerySet.h>
#include <qtcCache.h>
#include <sdi.h> /* TASK-7219 Shard Transformer Refactoring */

// Subquery unnesting 시 생성되는 view의 이름
/* BUG-48052 */
#define VIEW_NAME_PREFIX        "$VIEW"
#define VIEW_NAME_LENGTH        11

// Subquery unnesting 시 생성되는 view의 column 이름
#define COLUMN_NAME_PREFIX      "COL"
#define COLUMN_NAME_LENGTH      8

extern mtfModule mtfAnd;
extern mtfModule mtfOr;

extern mtfModule mtfExists;
extern mtfModule mtfNotExists;
extern mtfModule mtfUnique;
extern mtfModule mtfNotUnique;

extern mtfModule mtfList;

extern mtfModule mtfEqual;
extern mtfModule mtfNotEqual;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;

extern mtfModule mtfEqualAny;
extern mtfModule mtfNotEqualAny;
extern mtfModule mtfGreaterThanAny;
extern mtfModule mtfGreaterEqualAny;
extern mtfModule mtfLessThanAny;
extern mtfModule mtfLessEqualAny;

extern mtfModule mtfEqualAll;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfGreaterThanAll;
extern mtfModule mtfGreaterEqualAll;
extern mtfModule mtfLessThanAll;
extern mtfModule mtfLessEqualAll;

extern mtfModule mtfLnnvl;
extern mtfModule mtfCount;
extern mtfModule mtfCountKeep;

extern mtfModule mtfCase2;
extern mtfModule mtfIsNotNull;

extern mtfModule mtfGetBlobLocator;
extern mtfModule mtfGetClobLocator;

IDE_RC
qmoUnnesting::doTransform( qcStatement  * aStatement,
                           idBool       * aChanged )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery Unnesting 기법
 *     SQL구문에 포함된 subquery들을 모두 찾아 unnesting 시도한다.
 *
 * Implementation :
 *     Unnesting되어 view merging이 수행되어야 하는 경우 aChange의
 *     값이 true로 반환된다.
 *
 ***********************************************************************/

    qmsParseTree * sParseTree = NULL; /* TASK-7219 */
    idBool         sChanged = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::doTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------
    // Subqury Unnesting 수행
    //------------------------------------------

    if ( QCU_OPTIMIZER_UNNEST_SUBQUERY == 0 )
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_SUBQUERY_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_SUBQUERY_FALSE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_SUBQUERY_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_SUBQUERY_TRUE;
    }

    /* BUG-46544 unnest hit */
    if ( (QCU_OPTIMIZER_UNNEST_COMPATIBILITY & QCU_UNNEST_COMPATIBILITY_MASK_MODE1)
         == QCU_UNNEST_COMPATIBILITY_MASK_MODE1 )
    {
        // 기존에 unnest의 경우 hint보다 프로퍼티 가 우선이여서 호환성을 위해
        // 프로퍼티를 우선하도록 한다.
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_COMPATIBILITY_1_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_COMPATIBILITY_1_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_COMPATIBILITY_1_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_COMPATIBILITY_1_FALSE;
    }

    /* TASK-7219 원격지에서 수행할 Shard View에는 Unnesting을 수행하지 않는다. */
    sParseTree = (qmsParseTree*)aStatement->myPlan->parseTree;

    IDE_TEST_CONT( ( ( sParseTree->common.stmtShard != QC_STMT_SHARD_NONE )
                     &&
                     ( sParseTree->common.stmtShard != QC_STMT_SHARD_META ) ),
                   NORMAL_EXIT );

    // BUG-43059 Target subquery unnest/removal disable
    if ( ( sParseTree->querySet->lflag & QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_MASK )
         == QMV_QUERYSET_TARGET_SUBQUERY_UNNEST_TRUE )
    {
        IDE_TEST( doTransformQuerySet( aStatement,
                                       sParseTree->querySet,
                                       & sChanged )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7219 */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_UNNEST_SUBQUERY );

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_UNNEST_COMPATIBILITY );

    *aChanged = sChanged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::doTransformSubqueries( qcStatement * aStatement,
                                     qtcNode     * aPredicate,
                                     idBool      * aChanged )
{
/***********************************************************************
 *
 * Description :
 *     주어진 predicate에서 포함된 subquery들을 모두 찾아 unnesting
 *     시도한다.
 *     SELECT 외 구문에 포함된 predicate들을 위해 사용한다.
 *
 * Implementation :
 *     Unnesting되어 view merging이 수행되어야 하는 경우 aChange의
 *     값이 true로 반환된다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoUnnesting::doTransformSubqueries::__FT__" );

    if ( QCU_OPTIMIZER_UNNEST_SUBQUERY == 0 )
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_SUBQUERY_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_SUBQUERY_FALSE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_SUBQUERY_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_SUBQUERY_TRUE;
    }

    /* BUG-46544 unnest hit */
    if ( (QCU_OPTIMIZER_UNNEST_COMPATIBILITY & QCU_UNNEST_COMPATIBILITY_MASK_MODE1) 
         == QCU_UNNEST_COMPATIBILITY_MASK_MODE1 )
    {
        // 기존에 unnest의 경우 hint보다 프로퍼티 가 우선이여서 호환성을 위해
        // 프로퍼티를 우선하도록 한다.
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_COMPATIBILITY_1_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_COMPATIBILITY_1_TRUE;
    }
    else
    {
        QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_COMPATIBILITY_1_MASK;
        QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_COMPATIBILITY_1_FALSE;
    }

    IDE_TEST( findAndUnnestSubquery( aStatement,
                                     NULL,
                                     ID_FALSE,
                                     aPredicate,
                                     aChanged )
              != IDE_SUCCESS );

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_UNNEST_SUBQUERY );

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_UNNEST_COMPATIBILITY );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::doTransformQuerySet( qcStatement * aStatement,
                                   qmsQuerySet * aQuerySet,
                                   idBool      * aChanged )
{
/***********************************************************************
 *
 * Description :
 *     Query-set에 포함된 subquery들을 찾아 unnesting 시도한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom * sFrom;
    idBool    sUnnestSubquery;
    idBool    sChanged = ID_FALSE;
    idBool    sRemoved;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::doTransformQuerySet::__FT__" );

    if( aQuerySet->setOp == QMS_NONE )
    {
        if( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT )
        {
            if( aQuerySet->SFWGH->hints->joinOrderType == QMO_JOIN_ORDER_TYPE_ORDERED )
            {
                sUnnestSubquery = ID_FALSE;
            }
            else
            {
                if( ( aQuerySet->SFWGH->hierarchy == NULL ) &&
                    ( QCG_GET_SESSION_NORMALFORM_MAXIMUM( aStatement ) > 0 ) )
                {
                    sUnnestSubquery = ID_TRUE;
                }
                else
                {
                    sUnnestSubquery = ID_FALSE;
                }
            }
        }
        else
        {
            sUnnestSubquery = ID_FALSE;
        }

        if ( sUnnestSubquery == ID_TRUE )
        {
            for( sFrom = aQuerySet->SFWGH->from;
                 sFrom != NULL;
                 sFrom = sFrom->next )
            {
                IDE_TEST( doTransformFrom( aStatement,
                                           aQuerySet->SFWGH,
                                           sFrom,
                                           aChanged )
                          != IDE_SUCCESS );
            }

            if( ( aQuerySet->SFWGH->hierarchy == NULL ) &&
                ( aQuerySet->SFWGH->rownum    == NULL ) )
            {
                IDE_TEST( findAndRemoveSubquery( aStatement,
                                                 aQuerySet->SFWGH,
                                                 aQuerySet->SFWGH->where,
                                                 &sRemoved )
                          != IDE_SUCCESS );

                // BUG-38288 RemoveSubquery 를 하게 되면 target 이 변경된다
                // 이를 상위 QuerySet 에서 알아야 한다.
                if( sRemoved == ID_TRUE )
                {
                    *aChanged = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Hierarchy query 사용 시 subquery removal 수행안함
            }

            // SET 연산이 아닌 경우 WHERE절의 subquery들을 unnesting을 시도 한다.
            IDE_TEST( findAndUnnestSubquery( aStatement,
                                             aQuerySet->SFWGH,
                                             sUnnestSubquery,
                                             aQuerySet->SFWGH->where,
                                             &sChanged )
                      != IDE_SUCCESS );

            if( sChanged == ID_TRUE )
            {
                IDE_TEST( qmoViewMerging::validateNode( aStatement,
                                                        aQuerySet->SFWGH->where )
                          != IDE_SUCCESS );

                *aChanged = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* Nothing to do */
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
qmoUnnesting::doTransformFrom( qcStatement * aStatement,
                               qmsSFWGH    * aSFWGH,
                               qmsFrom     * aFrom,
                               idBool      * aChanged )
{
/***********************************************************************
 *
 * Description :
 *     FROM절의 relation이 view인 경우 view에서 subquery를 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoUnnesting::doTransformFrom::__FT__" );

    if( aFrom->joinType == QMS_NO_JOIN )
    {
        if( aFrom->tableRef->view != NULL )
        {
            IDE_TEST( doTransform( aFrom->tableRef->view,
                                   aChanged )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_TEST( findAndUnnestSubquery( aStatement,
                                         NULL,
                                         ID_FALSE,
                                         aFrom->onCondition,
                                         aChanged )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->left,
                                   aChanged )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformFrom( aStatement,
                                   aSFWGH,
                                   aFrom->right,
                                   aChanged )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::findAndUnnestSubquery( qcStatement * aStatement,
                                     qmsSFWGH    * aSFWGH,
                                     idBool        aUnnestSubquery,
                                     qtcNode     * aPredicate,
                                     idBool      * aChanged )
{
/***********************************************************************
 *
 * Description :
 *     Predicate에 포함된 subquery를 찾아 unnesting한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode       * sNode;
    idBool          sChanged = ID_FALSE;
    idBool          sUnnestSubquery = aUnnestSubquery;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::findAndUnnestSubquery::__FT__" );

    if( aPredicate != NULL )
    {
        if( ( aPredicate->lflag & QTC_NODE_SUBQUERY_MASK ) == QTC_NODE_SUBQUERY_EXIST )
        {
            if( isSubqueryPredicate( aPredicate ) == ID_TRUE )
            {
                // EXISTS/NOT EXISTS로 변환 시도
                if( isExistsTransformable( aStatement, aSFWGH, aPredicate, aUnnestSubquery ) == ID_TRUE )
                {
                    IDE_TEST( transformToExists( aStatement, aPredicate )
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

            if( ( aPredicate->node.module == &mtfExists ) ||
                ( aPredicate->node.module == &mtfNotExists ) ||
                ( aPredicate->node.module == &mtfUnique ) ||
                ( aPredicate->node.module == &mtfNotUnique ) )
            {
                // EXISTS/NOT EXISTS/UNIQUE/NOT UNIQUE의 경우 SELECT절을 단순하게 변경
                IDE_TEST( setDummySelect( aStatement, aPredicate, ID_TRUE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            if( ( aUnnestSubquery == ID_TRUE ) &&
                ( ( aPredicate->node.module == &mtfExists ) ||
                  ( aPredicate->node.module == &mtfNotExists ) ) )
            {
                // Subquery에 포함된 subquery들에 대해 먼저 unnesting 시도
                IDE_TEST( doTransform( ((qtcNode *)aPredicate->node.arguments)->subquery,
                                       aChanged )
                          != IDE_SUCCESS );

                if( isSimpleSubquery( ((qtcNode *)aPredicate->node.arguments)->subquery ) == ID_TRUE )
                {
                    // Nothing to do.
                }
                else
                {
                    // 추 후 cost-based query transformation으로 구현되어야 함
                    if( QCU_OPTIMIZER_UNNEST_COMPLEX_SUBQUERY == 0 )
                    {
                        sUnnestSubquery = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    qcgPlan::registerPlanProperty( aStatement,
                                                   PLAN_PROPERTY_OPTIMIZER_UNNEST_COMPLEX_SUBQUERY );
                }

                if( ( sUnnestSubquery == ID_TRUE ) &&
                    ( isUnnestableSubquery( aStatement, aSFWGH, aPredicate ) == ID_TRUE ) )
                {
                    // Unnesting 시도
                    IDE_TEST( unnestSubquery( aStatement,
                                              aSFWGH,
                                              aPredicate )
                              != IDE_SUCCESS );

                    *aChanged = ID_TRUE;
                }
                else
                {
                    // Unnesting 불가능한 subquery
                }
            }
            else
            {
                if( aPredicate->node.module == &qtc::subqueryModule )
                {
                    IDE_TEST( doTransform( aPredicate->subquery, &sChanged )
                              != IDE_SUCCESS );

                    if( sChanged == ID_TRUE )
                    {
                        *aChanged = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    if( ( aPredicate->node.lflag & MTC_NODE_OPERATOR_MASK )
                        != MTC_NODE_OPERATOR_AND )
                    {
                        // AND가 아닌 연산자의 하위 subquery는
                        // Unnesting하지 않는다.
                        aUnnestSubquery = ID_FALSE;

                        /* BUG-47786 Unnesting 결과 오류 */
                        if ( ( ( aPredicate->node.lflag & MTC_NODE_OPERATOR_MASK )
                               == MTC_NODE_OPERATOR_OR ) &&
                             ( aSFWGH != NULL ) )
                        {
                            aSFWGH->lflag &= ~QMV_SFWGH_UNNEST_OR_STOP_MASK;
                            aSFWGH->lflag |= QMV_SFWGH_UNNEST_OR_STOP_TRUE;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    for( sNode = (qtcNode *)aPredicate->node.arguments;
                         sNode != NULL;
                         sNode = (qtcNode *)sNode->node.next )
                    {
                        IDE_TEST( findAndUnnestSubquery( aStatement,
                                                         aSFWGH,
                                                         aUnnestSubquery,
                                                         sNode,
                                                         aChanged )
                                  != IDE_SUCCESS );
                    }
                }
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

    /* BUG-47786 Unnesting 결과 오류 */
    if ( aSFWGH != NULL )
    {
        if ( ( ( aSFWGH->lflag & QMV_SFWGH_UNNEST_LEFT_DISK_MASK )
               == QMV_SFWGH_UNNEST_LEFT_DISK_TRUE ) &&
             ( ( aSFWGH->lflag & QMV_SFWGH_UNNEST_OR_STOP_MASK )
               == QMV_SFWGH_UNNEST_OR_STOP_TRUE ) )
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoUnnesting::isExistsTransformable( qcStatement * aStatement,
                                     qmsSFWGH    * aSFWGH,
                                     qtcNode     * aNode,
                                     idBool        aUnnestSubquery )
{
/***********************************************************************
 *
 * Description :
 *     Subquery가 EXISTS/NOT EXISTS 형태로 변환 가능한지 확인한다.
 *
 * Implementation :
 *     1. Subquery에서 LIMIT절, ROWNUM column, window function 포함여부 확인
 *     2. Oracle style outer join이나 GROUP BY절등의 유무 확인
 *     3. EXISTS/NOT EXISTS 형태로 변환이 더 유리한지 확인
 *        어차피 unnesting 불가능한 경우 소극적으로 transformation하고
 *        기존의 subquery optimization tip을 최대한 활용한다.
 *
 ***********************************************************************/

    qtcNode        * sSQNode;
    qtcNode        * sArg;
    qcStatement    * sSQStatement;
    qmsParseTree   * sSQParseTree;
    qmsSFWGH       * sSQSFWGH;
    qmsTarget      * sTarget;
    idBool           sIsTrue;
    idBool           sIsCheck; /* BUG-48336 */

    sSQNode      = (qtcNode *)aNode->node.arguments->next;
    sSQStatement = sSQNode->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    if ( (aNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK) == QTC_NODE_JOIN_OPERATOR_EXIST )
    {
        // Subquery와 outer join 시 불가
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }
    
    if( sSQParseTree->limit != NULL )
    {
        // LIMIT절 사용 시 불가능
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH == NULL )
    {
        // UNION, UNION ALL, MINUS, INTERSECT 등 사용하는 경우 SFWGH가 NULL이다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-36580 supported TOP */
    if( sSQSFWGH->top != NULL )
    {

        // top절 사용 시 불가능
        return ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    if( sSQSFWGH->where != NULL )
    {
        if ( (sSQSFWGH->where->lflag & QTC_NODE_JOIN_OPERATOR_MASK) == QTC_NODE_JOIN_OPERATOR_EXIST )
        {
            // Oracle style의 outer join 사용 시 unnesting하지 않는다.
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }

        /* BUG-47576 aggregation +  외부참조 컬럼시 unnest 결과 오류 */
        if ( ( sSQSFWGH->aggsDepth1 != NULL ) &&
             ( sSQSFWGH->outerDepInfo.depCount > 0 ) )
        {
            sIsTrue = ID_TRUE; 
            isAggrExistTransformable( sSQSFWGH->where,
                                      &sSQSFWGH->outerDepInfo,
                                      &sIsTrue );

            if ( sIsTrue == ID_FALSE )
            {
                IDE_CONT( INVALID_FORM );
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
    else
    {
        // Nothing to do.
    }

    if( isQuantifiedSubquery( aNode ) == ID_FALSE )
    {
        // BUG-45250 비교 연산자일 때, left가 list type이면 안됩니다.
        if( ( ( aNode->node.module == &mtfGreaterThan ) ||
              ( aNode->node.module == &mtfGreaterEqual ) ||
              ( aNode->node.module == &mtfLessThan ) ||
              ( aNode->node.module == &mtfLessEqual ) ) &&
            ( aNode->node.arguments->module == &mtfList ) )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }

        // Quantified predicate이 아니더라도 single row subquery인 경우
        if( isSingleRowSubquery( sSQStatement ) == ID_FALSE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            if( isUnnestablePredicate( aStatement, aSFWGH, aNode, sSQSFWGH ) == ID_FALSE )
            {
                IDE_CONT( INVALID_FORM );
            }
            else
            {
                // Single row이면서 unnesting 가능한 조건
            }

            if( aSFWGH != NULL )
            {
                if( qtc::dependencyContains( &aSFWGH->depInfo,
                                             &sSQSFWGH->outerDepInfo ) == ID_FALSE )
                {
                    // Subquery의 correlation은 parent query block에 한정되어야 한다.
                    IDE_CONT( INVALID_FORM );
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
        if( aSFWGH != NULL )
        {
            if( qtc::dependencyContains( &aSFWGH->depInfo,
                                         &sSQSFWGH->outerDepInfo ) == ID_FALSE )
            {
                // Subquery의 correlation은 parent query block에 한정되어야 한다.
                IDE_CONT( INVALID_FORM );
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

    if( ( sSQSFWGH->group      == NULL ) && 
        ( sSQSFWGH->aggsDepth1 != NULL ) &&
        ( qtc::haveDependencies( &sSQSFWGH->outerDepInfo ) == ID_FALSE ) )
    {
        // GROUP BY절과 correlation 없이 aggregate function을 사용한 경우
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-38996
    // OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY 를 사용시 결과가 틀려짐
    // aggr 함수가 count 이면서 group by 를 사용하지 않는 경우에는 결과가 다를수 있다.
    // 위 조건일때는 비교하는 값이 0,1 일때만 unnset 가 가능하다.
    if( ( sSQSFWGH->group      == NULL ) &&
        ( sSQSFWGH->aggsDepth1 != NULL ) )
    {
        // BUG-45238
        for( sTarget = sSQSFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next )
        {
            if ( findCountAggr4Target( sTarget->targetColumn ) == ID_TRUE )
            {
                //ㅑ count(*)와의 연산이 된 target column이면 안된다.
                if ( sTarget->targetColumn->node.module != &mtfCount )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // Nothing to do.
                    // subquery의 target절에 count 컬럼 한개만 있는 경우
                }

                // subquery의 target절에 count 컬럼이 있다면, list가 아니여야 한다.
                if ( aNode->node.arguments->module == &mtfList )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // Nothing to do.
                    // subquery의 target절에 count 컬럼 한개만 있는 경우
                }

                // count컬럼 이 있다면, 다른 aggregate function과 함께 쓸 수 없다
                if ( sSQSFWGH->aggsDepth1->next != NULL )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // 다른 aggregate function이 사용되면 EXISTS변환되면서 결과가 달라질 수 있다.
                    // Nothing to do.
                }

                if ( sSQSFWGH->having != NULL )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // Nothing to do.
                }

                // EXISTS 또는 NOT EXISTS로 변환 가능해야한다.
                if( toExistsModule4CountAggr( aStatement,aNode ) == NULL )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // COUNT 를 제외한 모든것
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH->aggsDepth2 != NULL )
    {
        // Nested aggregate function을 사용한 경우
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH->rownum != NULL )
    {
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    // SELECT절 검사
    for( sTarget = sSQSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        if ( (sTarget->targetColumn->lflag & QTC_NODE_ANAL_FUNC_MASK ) == QTC_NODE_ANAL_FUNC_EXIST )
        {
            // Window function을 사용한 경우
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }

        if( sTarget->targetColumn->depInfo.depCount > 1 )
        {
            // Subquery의 SELECT절에서 둘 이상의 table을 참조하는 경우
            // ex) t1.i1 IN (SELECT t2.i1 + t3.i1 FROM t2, t3 ... );
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    if( aNode->node.arguments->module == &mtfList )
    {
        for( sArg = (qtcNode *)aNode->node.arguments->arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next )
        {
            if( sArg->depInfo.depCount > 1 )
            {
                // Outer query의 조건에서 둘 이상의 table을 참조하는 경우
                // ex) (t1.i1 + t2.i1, ...) IN (SELECT i1, ...);

                IDE_CONT( INVALID_FORM );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        if( ((qtcNode*)aNode->node.arguments)->depInfo.depCount > 1 )
        {
            // Outer query의 조건에서 둘 이상의 table을 참조하는 경우
            // ex) t1.i1 + t2.i1 IN (SELECT i1 FROM ...);

            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    // Subquery에 correlation이 없는 경우 기존의 subquery optimization tip을
    // 이용하는것이 더 효율적인 경우들이 존재한다.
    if( qtc::haveDependencies( &sSQSFWGH->outerDepInfo ) == ID_FALSE )
    {
        // 어차피 unnesting 못하는 경우(ON절 등) EXISTS로 변환하지 않는다.
        if( aUnnestSubquery == ID_FALSE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Notihng to do.
        }

        // Nullable column이 포함된 경우 어차피 anti join을 수행할 수 없으므로,
        // NOT EXISTS predicate의 변환을 포기한다.
        if( ( ( aNode->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK ) == MTC_NODE_GROUP_COMPARISON_TRUE ) &&
            ( ( aNode->node.lflag & MTC_NODE_GROUP_MASK ) == MTC_NODE_GROUP_ALL ) )
        {
            if( aNode->node.arguments->module == &mtfList )
            {
                for( sArg = (qtcNode *)aNode->node.arguments->arguments, sTarget = sSQSFWGH->target;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next, sTarget = sTarget->next )
                {
                    if( ( isNullable( aStatement, sArg ) == ID_TRUE ) ||
                        ( isNullable( aStatement, sTarget->targetColumn )  == ID_TRUE ) )
                    {
                        IDE_CONT( INVALID_FORM );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                if( ( isNullable( aStatement, (qtcNode*)aNode->node.arguments ) == ID_TRUE ) ||
                    ( isNullable( aStatement, sSQSFWGH->target->targetColumn )  == ID_TRUE ) )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_EQUAL )
            {
                // =ALL의 경우 NOT EXISTS에 <>형의 correlation을 가지므로
                // 결국 anti join할 수 없다.
                IDE_CONT( INVALID_FORM );
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

        // List type과 <>, <>ANY인 경우 predicate들이 OR로 연결되어 semi join하지 못한다.
        if( ( ( aNode->node.module == &mtfNotEqual ) || ( aNode->node.module == &mtfNotEqualAny ) ) &&
            ( aNode->node.arguments->module == &mtfList ) )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-42637 subquery unnesting시 lob 제약 제거
    // lob 컬럼은 group by 에 쓸 수가 없다.
    // subquery unnesting시 group by 나 AGGR 함수가 있는 경우에는
    // group by 에 추가가 되므로 막아야 한다.
    // BUG-47751 unnesting이 되지 않는다면 exists변환도 막는다.
    if ( (sSQSFWGH->group != NULL) ||
         (sSQSFWGH->aggsDepth1 != NULL) )
    {
        if( sSQSFWGH->where != NULL )
        {
            if ( (sSQSFWGH->where->lflag & QTC_NODE_LOB_COLUMN_MASK)
                 == QTC_NODE_LOB_COLUMN_EXIST )
            {
                IDE_CONT( INVALID_FORM );
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

        if ( sSQSFWGH->having != NULL )
        {
            if ( (sSQSFWGH->having->lflag & QTC_NODE_LOB_COLUMN_MASK)
                 == QTC_NODE_LOB_COLUMN_EXIST )
            {
                IDE_CONT( INVALID_FORM );
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

    // BUG-48336 compatibiltiy가 2이면 버그반영 조건을 확인하지 않습니다. 
    if ( (QCU_OPTIMIZER_UNNEST_COMPATIBILITY & QCU_UNNEST_COMPATIBILITY_MASK_MODE2)
         != QCU_UNNEST_COMPATIBILITY_MASK_MODE2 )
    { 
        sIsCheck = ID_TRUE;
    }
    else
    {
        sIsCheck = ID_FALSE;
    }

    // BUG-43300 no_unnest 힌트 사용시 exists 변환을 막습니다.
    if ( sSQSFWGH->hints->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NO_UNNEST )
    {
        IDE_CONT( INVALID_FORM );
    }
    else if ( sSQSFWGH->hints->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NOT_DEFINED )
    {
        if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_SUBQUERY_MASK )
             == QC_TMP_UNNEST_SUBQUERY_FALSE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* BUG-46544 Unnest hit */
        // Hint QMO_SUBQUERY_UNNEST_TYPE_UNNEST
        // hint가 존재하고 Property는 0이고 Compatibility가 1 이면 Property
        // 우선으로 unnest를 하지 않는다.
        if ( ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_SUBQUERY_MASK )
               == QC_TMP_UNNEST_SUBQUERY_FALSE ) &&
             ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_COMPATIBILITY_1_MASK )
               == QC_TMP_UNNEST_COMPATIBILITY_1_TRUE ) )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // BUG-48336
            // hint가 존재하고 Compatibility가 2가 아니면 unnest를 합니다.
            sIsCheck = ID_FALSE;
        }
    }

    /*********************************
     *   제일 마지막조건이여야합니다.    
     *********************************/
    if ( sIsCheck == ID_TRUE )
    {
        /**************************************
         *  BUG-48336 아래 조건을 모두 만족하는 경우
         *            subquery filter플랜이 생성되는 모드 추가합니다.
         *   1. subquery에outer dep이 1이고,
         *               조인이 없고 뷰가 아닌 테이블인 경우
         *   2. parent query block에는 1개 이상의 left outer join이 있는 경우
         **************************************/
        // 1. Subquery 조건
        if ( ( qtc::haveDependencies( &sSQSFWGH->outerDepInfo ) == ID_FALSE ) &&
             ( ( sSQSFWGH->from->next == NULL ) &&
               ( sSQSFWGH->from->joinType == QMS_NO_JOIN ) ) )
        { 
            // - view( view, inline view, with) recursive with인경우 제외
            if ( ( sSQSFWGH->from->tableRef->view == NULL ) &&
                 ( (sSQSFWGH->from->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK)
                   == QMS_TABLE_REF_RECURSIVE_VIEW_FALSE ) )
            {
                // 2. parent query block 조건
                if ( ( aSFWGH->from->next != NULL ) ||
                     ( aSFWGH->from->joinType != QMS_NO_JOIN ) )
                {
                    // parent query block에 Oracle style의 outer join 사용 시
                    if ( aSFWGH->where != NULL )
                    {
                        if ( ( aSFWGH->where->lflag & QTC_NODE_JOIN_OPERATOR_MASK)
                             == QTC_NODE_JOIN_OPERATOR_EXIST )
                        {
                            IDE_CONT( INVALID_FORM );
                        }
                    } 
                    else
                    {
                        // Nothing to do.
                    }

                    // parent query block에 Ansi style outer join을 단 하나라도 사용한 경우
                    if ( ( (aSFWGH->lflag & QMV_SFWGH_JOIN_LEFT_OUTER)  == QMV_SFWGH_JOIN_LEFT_OUTER ) ||
                         ( (aSFWGH->lflag & QMV_SFWGH_JOIN_RIGHT_OUTER) == QMV_SFWGH_JOIN_RIGHT_OUTER ) )
                    {
                        IDE_CONT( INVALID_FORM );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // main query가 join이 아닌경우
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

        // 위의 조건에도 EXISTS변환이 일어난 경우 SU 변환이 일어나도록 한다.
        sSQNode->lflag &= ~QTC_NODE_TRANS_IN_TO_EXISTS_MASK;
        sSQNode->lflag |= QTC_NODE_TRANS_IN_TO_EXISTS_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( INVALID_FORM );

    return ID_FALSE;
}

IDE_RC
qmoUnnesting::transformToExists( qcStatement * aStatement,
                                 qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     Subquery가 quantified predicate과 함께 사용된 경우
 *     EXISTS/NOT EXISTS 형태로 변환한다.
 *
 * Implementation :
 *     1. Subquery에서 LIMIT절, ROWNUM column, window function 포함여부 확인
 *        (Window function은 WHERE절에서 사용할 수 없다.)
 *     2. Correlation predicate 생성
 *     3. 생성된 correlation predicate을 WHERE절 또는 HAVING절에 연결
 *     4. Subquery predicate을 EXISTS/NOT EXISTS로 변경한다.
 *
 ***********************************************************************/

    qtcNode        * sPredicate[2];
    qtcNode        * sSQNode;
    qtcNode        * sCorrPreds;
    qtcNode        * sConcatPred;
    mtcNode        * sNext;
    qcStatement    * sSQStatement;
    qmsParseTree   * sSQParseTree;
    qmsSFWGH       * sSQSFWGH;
    qcStatement    * sTempStatement;
    qmsParseTree   * sTempParseTree;
    qmsSFWGH       * sTempSFWGH;
    qmsOuterNode   * sOuter;
    mtfModule      * sTransModule;
    qcNamePosition   sEmptyPosition;
    // BUG-45238
    qtcNode        * sIsNotNull[2];

    IDU_FIT_POINT_FATAL( "qmoUnnesting::transformToExists::__FT__" );

    SET_EMPTY_POSITION( sEmptyPosition );

    sSQNode      = (qtcNode *)aNode->node.arguments->next;
    sSQStatement = sSQNode->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    /* TASK-7219 Shard Transformer Refactoring */
    if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                        SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
         == ID_TRUE )
    {
        IDE_TEST( sdi::preAnalyzeQuerySet( sSQStatement,
                                           sSQParseTree->querySet,
                                           QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // Correlation predicate을 생성
    IDE_TEST( genCorrPredicates( aStatement,
                                 aNode,
                                 ID_TRUE, /* aExistsTrans */
                                 &sCorrPreds )
              != IDE_SUCCESS );

    // Correlation predicate 추가로 인한 관련 정보 갱신
    IDE_TEST( qmvQTC::setOuterColumns( sSQStatement,
                                       NULL,
                                       sSQSFWGH,
                                       sCorrPreds )
              != IDE_SUCCESS );

    // BUG-45668 왼쪽에 서브쿼리가 있을때 exists 변환후 결과가 틀립니다.
    // 왼쪽 서브쿼리의 outerColumns 을 오른쪽으로 넘겨주어야 한다.
    if ( sCorrPreds->node.arguments->module == &qtc::subqueryModule )
    {
        sTempStatement = ((qtcNode*)(sCorrPreds->node.arguments))->subquery;
        sTempParseTree = (qmsParseTree *)sTempStatement->myPlan->parseTree;
        sTempSFWGH     = sTempParseTree->querySet->SFWGH;

        for ( sOuter = sTempSFWGH->outerColumns;
              sOuter != NULL;
              sOuter = sOuter->next )
        {
            IDE_TEST( qmvQTC::addOuterColumn( sSQStatement,
                                              sSQSFWGH,
                                              sOuter->column )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // nothing to do.
    }

    // BUG-40753 aggsDepth1 관리가 잘못되어 결과가 틀림
    // aggsDepth1을 제대로 설정해주도록 한다.
    IDE_TEST( setAggrNode( sSQStatement,
                           sSQSFWGH,
                           sCorrPreds )
              != IDE_SUCCESS );

    IDE_TEST( qmvQTC::setOuterDependencies( sSQSFWGH,
                                            &sSQSFWGH->outerDepInfo )
              != IDE_SUCCESS );

    qtc::dependencySetWithDep( &sSQSFWGH->thisQuerySet->outerDepInfo,
                               &sSQSFWGH->outerDepInfo );

    qtc::dependencySetWithDep( &sSQNode->depInfo,
                               &sSQSFWGH->outerDepInfo );

    if( (sSQSFWGH->group      == NULL) &&
        (sSQSFWGH->aggsDepth1 == NULL) )
    {
        // GROUP BY절, aggregate function을 사용하지 않은 경우
        // Correlation predicate을 WHERE절에 추가
        IDE_TEST( concatPredicate( aStatement,
                                   sSQSFWGH->where,
                                   sCorrPreds,
                                   &sConcatPred )
                  != IDE_SUCCESS );
        sSQSFWGH->where = sConcatPred;
    }
    else
    {
        // BUG-38996
        // OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY 를 사용시 결과가 틀려짐
        // aggr 함수가 count(*) 이면서 group by 를 사용하지 않는 경우에는 결과가 다를수 있다.
        // 위 조건일때는 비교하는 값이 0,1 일때만 unnset 가 가능하다.
        // 변환이 가능할때는 having 절, aggr 함수를 제거 한다.
        if( (sSQSFWGH->group      == NULL) &&
            (sSQSFWGH->aggsDepth1 != NULL) )
        {
            if (sSQSFWGH->aggsDepth1->aggr->node.module == &mtfCount )
            {
                // Nothing to do.
            }
            else
            {
                // Correlation predicate을 HAVING절에 추가
                IDE_TEST( concatPredicate( aStatement,
                                           sSQSFWGH->having,
                                           sCorrPreds,
                                           &sConcatPred )
                          != IDE_SUCCESS );
                sSQSFWGH->having = sConcatPred;
            }

        }
        else
        {
            // Correlation predicate을 HAVING절에 추가
            IDE_TEST( concatPredicate( aStatement,
                                       sSQSFWGH->having,
                                       sCorrPreds,
                                       &sConcatPred )
                      != IDE_SUCCESS );
            sSQSFWGH->having = sConcatPred;
        }
    }

    // BUG-38996
    // OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY 를 사용시 결과가 틀려짐
    // aggr 함수가 count(*) 이면서 group by 를 사용하지 않는 경우에는 결과가 다를수 있다.
    // 위 조건일때는 비교하는 값이 0,1 일때만 unnset 가 가능하다.
    // 변환이 가능할때는 having 절, aggr 함수를 제거 한다.
    if( (sSQSFWGH->group      == NULL) &&
        (sSQSFWGH->aggsDepth1 != NULL) )
    {
        if( sSQSFWGH->aggsDepth1->aggr->node.module == &mtfCount )
        {
            // BUG-45238
            // isExistsTransformable이 가능하다면, count가 있는 경우에는
            // 다른 aggregation과 함께 쓸 수 없어서 count컬럼만 있다.
            // COUNT( arguments ) 컬럼이 있는 경우, (arguments) IS NOT NULL을 where절에 추가한다.
            if ( sSQSFWGH->aggsDepth1->aggr->node.arguments != NULL )
            {
                IDE_TEST( qtc::makeNode( aStatement,
                                            sIsNotNull,
                                            &sEmptyPosition,
                                            &mtfIsNotNull )
                             != IDE_SUCCESS );

                sIsNotNull[0]->node.arguments = sSQSFWGH->aggsDepth1->aggr->node.arguments;

                IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                            sIsNotNull[0] )
                             != IDE_SUCCESS );

                IDE_TEST( concatPredicate( aStatement,
                                              sSQSFWGH->where,
                                              sIsNotNull[0],
                                              &sConcatPred )
                             != IDE_SUCCESS );

                sSQSFWGH->where = sConcatPred;
            }
            else
            {
                // Nothing to do.
            }

            sSQSFWGH->aggsDepth1 = NULL;

            sTransModule = toExistsModule4CountAggr( aStatement, aNode );

            IDE_DASSERT( sTransModule != NULL);

        }
        else
        {
            sTransModule = toExistsModule( aNode->node.module );
        }
    }
    else
    {
        sTransModule = toExistsModule( aNode->node.module );
    }

    // Subquery predicate의 EXISTS/NOT EXISTS로 변경
    IDE_TEST( qtc::makeNode( aStatement,
                             sPredicate,
                             &sEmptyPosition,
                             sTransModule )
              != IDE_SUCCESS );

    sNext = aNode->node.next;
    idlOS::memcpy( aNode, sPredicate[0], ID_SIZEOF( qtcNode ) );
    aNode->node.arguments = (mtcNode *)sSQNode;
    aNode->node.next = sNext;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                aNode )
              != IDE_SUCCESS );

    IDE_TEST( setDummySelect( aStatement, aNode, ID_FALSE )
              != IDE_SUCCESS );

    // BUG-41917
    IDE_TEST( qtcCache::validateSubqueryCache( QC_SHARED_TMPLATE(aStatement),
                                               (qtcNode *)aNode->node.arguments )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoUnnesting::isUnnestableSubquery( qcStatement * aStatement,
                                    qmsSFWGH    * aSFWGH,
                                    qtcNode     * aSubqueryPredicate )
{
/***********************************************************************
 *
 * Description :
 *     Unnesting 가능한 subquery인지 판단한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement      * sSQStatement;
    qmsParseTree     * sSQParseTree;
    qmsSFWGH         * sSQSFWGH;
    qmsConcatElement * sGroup;
    qmsAggNode       * sAggr;
    qmsTarget        * sTarget;
    qtcNode          * sSQNode;
    qcDepInfo          sDepInfo;
    qmsFrom          * sSQFrom; 
    idBool             sIsCheck; /* BUG-48336 */

    sSQNode      = (qtcNode *)aSubqueryPredicate->node.arguments;
    sSQStatement = sSQNode->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    if( sSQParseTree->querySet->setOp != QMS_NONE )
    {
        // SET 연산인 경우
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-48336 compatibiltiy가 2이면 버그반영 조건을 확인하지 않습니다. 
    if ( (QCU_OPTIMIZER_UNNEST_COMPATIBILITY & QCU_UNNEST_COMPATIBILITY_MASK_MODE2)
         != QCU_UNNEST_COMPATIBILITY_MASK_MODE2 )
    { 
        sIsCheck = ID_TRUE;
    }
    else
    {
        sIsCheck = ID_FALSE;
    }

    if ( sSQSFWGH->hints->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NO_UNNEST )
    {
        IDE_CONT( INVALID_FORM );
    }
    else if ( sSQSFWGH->hints->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NOT_DEFINED )
    {
        if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_SUBQUERY_MASK )
             == QC_TMP_UNNEST_SUBQUERY_FALSE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* BUG-46544 Unnest hit */
        // Hint QMO_SUBQUERY_UNNEST_TYPE_UNNEST
        // hint가 존재하고 Property는 0이고 Compatibility가 1 이면 Property
        // 우선으로 unnest를 하지 않는다.
        if ( ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_SUBQUERY_MASK )
               == QC_TMP_UNNEST_SUBQUERY_FALSE ) &&
             ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_COMPATIBILITY_1_MASK )
               == QC_TMP_UNNEST_COMPATIBILITY_1_TRUE ) )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // BUG-48336
            // hint가 존재하고 Compatibility가 2가 아니면 unnest를 합니다.
            sIsCheck = ID_FALSE;
        }
    }

    if( sSQParseTree->limit != NULL )
    {
        // LIMIT절을 사용한 경우
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-36580 supported TOP */
    if( sSQSFWGH->top    != NULL )
    {
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-37314 서브쿼리에 rownum 이 있는 경우에만 unnest 를 제한해야 한다. */
    if( sSQSFWGH->rownum != NULL )
    {
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH->hierarchy != NULL )
    {
        // Hierarchy절에서 correlation을 참조한 경우

        if( sSQSFWGH->hierarchy->startWith != NULL )
        {
            if( qtc::dependencyContains( &sSQSFWGH->depInfo,
                                         &sSQSFWGH->hierarchy->startWith->depInfo )
                == ID_FALSE )
            {
                IDE_CONT( INVALID_FORM );
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

        if( sSQSFWGH->hierarchy->connectBy != NULL )
        {
            if( qtc::dependencyContains( &sSQSFWGH->depInfo,
                                         &sSQSFWGH->hierarchy->connectBy->depInfo )
                == ID_FALSE )
            {
                IDE_CONT( INVALID_FORM );
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

    // PROJ-2418
    // sSQSFWGH의 From에서 Lateral View가 존재하면 Unnesting 할 수 없다.
    // 단, Lateral View가 이전에 모두 Merging 되었다면 Unnesting이 가능하다.
    for ( sSQFrom = sSQSFWGH->from; sSQFrom != NULL; sSQFrom = sSQFrom->next )
    {
        IDE_TEST( qmvQTC::getFromLateralDepInfo( sSQFrom, & sDepInfo )
                  != IDE_SUCCESS );

        if ( qtc::haveDependencies( & sDepInfo ) == ID_TRUE )
        {
            // 해당 From에 Lateral View가 존재하면 Unnesting 불가
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    if( ( sSQSFWGH->where  != NULL ) ||
        ( sSQSFWGH->having != NULL ) )
    {
        // WHERE절과 HAVING절의 조건을 확인한다.
        if( isUnnestablePredicate( aStatement,
                                   aSFWGH,
                                   aSubqueryPredicate,
                                   sSQSFWGH ) == ID_FALSE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_CONT( INVALID_FORM );
    }

    if( ( sSQSFWGH->group      == NULL ) &&
        ( sSQSFWGH->aggsDepth1 != NULL ) )
    {
        if( qtc::haveDependencies( &sSQSFWGH->outerDepInfo ) == ID_FALSE )
        {
            // GROUP BY절과 correlation 없이 aggregate function을 사용한 경우
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Cost-based query transformation이 필요한 시점
            // ex) SELECT * FROM T1 WHERE I1 = (SELECT SUM(I1) FROM T2 WHERE T1.I2 = T2.I2);
            //     만약 T2.I2에 index가 존재하고 T1의 cardinality가 높지 않다면
            //     unnesting하지 않는 것이 유리하고 그렇지 않다면 다음과 같이 변경하는것이 유리하다.
            //     SELECT * FROM T1, (SELECT SUM(I1) COL1, I2 COL2 FROM T2 GROUP BY I2) V1
            //       WHERE T1.I1 = V1.COL1 AND T1.I2 = V1.COL2;

            qcgPlan::registerPlanProperty( aStatement,
                                           PLAN_PROPERTY_OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY );

            if( QCU_OPTIMIZER_UNNEST_AGGREGATION_SUBQUERY == 0 )
            {
                IDE_CONT( INVALID_FORM );
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

    if( sSQSFWGH->aggsDepth2 != NULL )
    {
        // Nested aggregate function을 사용한 경우
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( qtc::dependencyContains( &aSFWGH->depInfo,
                                 &sSQSFWGH->outerDepInfo ) == ID_FALSE )
    {
        // Subquery의 correlation은 parent query block에 한정되어야 한다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    for( sTarget = sSQSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        if ( (sTarget->targetColumn->lflag & QTC_NODE_AGGREGATE_MASK ) == QTC_NODE_AGGREGATE_EXIST )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-41564
        // Target Subquery가 현재 Subquery 밖을 참조한다면 Unnesting 불가
        if( isOuterRefSubquery( sTarget->targetColumn, &sSQSFWGH->depInfo ) == ID_TRUE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    for( sGroup = sSQSFWGH->group;
         sGroup != NULL;
         sGroup = sGroup->next )
    {
        // BUG-45151 ROLL-UP, CUBE 등을 사용하면 sGroup->arithmeticOrList가 NULL이라서 죽습니다.
        if( sGroup->type != QMS_GROUPBY_NORMAL )
        {
            // ROLL-UP, CUBE 등을 사용 하는 경우
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }

        qtc::dependencyAnd( &sGroup->arithmeticOrList->depInfo,
                            &sSQSFWGH->outerDepInfo,
                            &sDepInfo );

        if( sDepInfo.depCount != 0 )
        {
            // SELECT, WHERE, HAVING 외 clause에서 outer query의 column을 참조한 경우
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    for( sAggr = sSQSFWGH->aggsDepth1;
         sAggr != NULL;
         sAggr = sAggr->next )
    {
        qtc::dependencyAnd( &sAggr->aggr->depInfo,
                            &sSQSFWGH->outerDepInfo,
                            &sDepInfo );

        if( sDepInfo.depCount != 0 )
        {
            // SELECT, WHERE, HAVING 외 clause에서 outer query의 column을 참조한 경우
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    for( sAggr = sSQSFWGH->aggsDepth2;
         sAggr != NULL;
         sAggr = sAggr->next )
    {
        qtc::dependencyAnd( &sAggr->aggr->depInfo,
                            &sSQSFWGH->outerDepInfo,
                            &sDepInfo );

        if( sDepInfo.depCount != 0 )
        {
            // SELECT, WHERE, HAVING 외 clause에서 outer query의 column을 참조한 경우
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-42637 subquery unnesting시 lob 제약 제거
    // lob 컬럼은 group by 에 쓸 수가 없다.
    // subquery unnesting시 group by 나 AGGR 함수가 있는 경우에는
    // group by 에 추가가 되므로 막아야 한다.
    if ( (sSQSFWGH->group != NULL) ||
         (sSQSFWGH->aggsDepth1 != NULL) )
    {
        if( sSQSFWGH->where != NULL )
        {
            if ( (sSQSFWGH->where->lflag & QTC_NODE_LOB_COLUMN_MASK)
                 == QTC_NODE_LOB_COLUMN_EXIST )
            {
                IDE_CONT( INVALID_FORM );
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

        if ( sSQSFWGH->having != NULL )
        {
            if ( (sSQSFWGH->having->lflag & QTC_NODE_LOB_COLUMN_MASK)
                 == QTC_NODE_LOB_COLUMN_EXIST )
            {
                IDE_CONT( INVALID_FORM );
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

    /********************************
     * 제일 마지막조건이여야합니다.                
     ********************************/
    if ( sIsCheck == ID_TRUE )
    { 
        /**************************************
         *  BUG-48336 아래 조건을 모두 만족하는 경우
         *            subquery filter플랜이 생성되는 모드 추가합니다.
         *   1. subquery는IN->EXISTS 변환을 안했고,
         *               outer dep이 1이고,
         *               조인이 없고 뷰가 아닌 테이블인 경우
         *   2. parent query block에는 1개 이상의 left outer join이 있는 경우
         *************************/
        // 1. Subquery 조건
        //    IN->EXISTS 쿼리변환을 한 경우 UNNEST
        if ( (sSQNode->lflag & QTC_NODE_TRANS_IN_TO_EXISTS_MASK)
             == QTC_NODE_TRANS_IN_TO_EXISTS_FALSE )
        {
            // JOIN이 있거나 outer dependency가 1 아닌경우 UNNEST
            if ( ( sSQSFWGH->from->next == NULL ) &&
                 ( sSQSFWGH->from->joinType == QMS_NO_JOIN ) &&
                 ( sSQSFWGH->outerDepInfo.depCount == 1 ) )
            {
                // view( view, inline view, with) recursive with인 경우 UNNEST
                if ( ( sSQSFWGH->from->tableRef->view == NULL ) &&
                     ( (sSQSFWGH->from->tableRef->flag & QMS_TABLE_REF_RECURSIVE_VIEW_MASK)
                       == QMS_TABLE_REF_RECURSIVE_VIEW_FALSE ) )
                {
                    // 2. parent query block 조건 
                    if ( ( aSFWGH->from->next != NULL ) ||
                         ( aSFWGH->from->joinType != QMS_NO_JOIN ) )
                    {
                        // Oracle style의 outer join 
                        if ( aSFWGH->where != NULL )
                        {
                            if ( ( aSFWGH->where->lflag & QTC_NODE_JOIN_OPERATOR_MASK)
                                 == QTC_NODE_JOIN_OPERATOR_EXIST )
                            {
                                IDE_CONT( INVALID_FORM );
                            }
                        } 
                        else
                        {
                            // Nothing to do.
                        }

                        // Ansi style outer join을 단 하나라도 사용한 경우
                        if ( ( (aSFWGH->lflag & QMV_SFWGH_JOIN_LEFT_OUTER) == QMV_SFWGH_JOIN_LEFT_OUTER ) ||
                             ( (aSFWGH->lflag & QMV_SFWGH_JOIN_RIGHT_OUTER) == QMV_SFWGH_JOIN_RIGHT_OUTER ) )
                        {
                            IDE_CONT( INVALID_FORM );
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

    return ID_TRUE;

    IDE_EXCEPTION_CONT( INVALID_FORM );

    IDE_EXCEPTION_END;

    return ID_FALSE;
}

IDE_RC
qmoUnnesting::unnestSubquery( qcStatement * aStatement,
                              qmsSFWGH    * aSFWGH,
                              qtcNode     * aSQPredicate )
{
/***********************************************************************
 *
 * Description :
 *     Subquery를 unnesting 시도 한다.
 *
 * Implementation :
 *     1. Simple/complex subquery를 구분한다.
 *     2. Subquery의 결과가 single/multiple row인지 여부에 따라
 *        join의 종류(semi/inner)를 결정한다.
 *
 ***********************************************************************/

    qcStatement  * sSQStatement;
    qmsParseTree * sSQParseTree;
    qmsSFWGH     * sSQSFWGH;
    qmsFrom      * sViewFrom;
    qtcNode      * sCorrPred = NULL;
    mtcNode      * sNext;
    qcDepInfo      sDepInfo;
    idBool         sIsSingleRow;
    idBool         sIsRemoveSemi;
    UInt           sTuple;
    
    IDU_FIT_POINT_FATAL( "qmoUnnesting::unnestSubquery::__FT__" );

    sSQStatement = ((qtcNode *)aSQPredicate->node.arguments)->subquery;

    sIsSingleRow = isSingleRowSubquery( sSQStatement );

    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    /* TASK-7219 Shard Transformer Refactoring */
    if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                        SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
         == ID_TRUE )
    {
        IDE_TEST( sdi::preAnalyzeQuerySet( sSQStatement,
                                           sSQParseTree->querySet,
                                           QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if( sSQSFWGH->where != NULL )
    {
        // WHERE절의 correlation predicate 제거
        IDE_TEST( removeCorrPredicate( aStatement,
                                       &sSQSFWGH->where,
                                       &sSQParseTree->querySet->outerDepInfo,
                                       &sCorrPred )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH->having != NULL )
    {
        // HAVING절의 correlation predicate 제거
        IDE_TEST( removeCorrPredicate( aStatement,
                                       &sSQSFWGH->having,
                                       &sSQParseTree->querySet->outerDepInfo,
                                       &sCorrPred )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // Subquery의 SELECT절을 모두 제거
    sSQSFWGH->target               = NULL;
    sSQParseTree->querySet->target = NULL;

    // Correlation predicate에서 사용하는 column들을 SELECT절에 나열
    IDE_TEST( genViewSelect( sSQStatement,
                             sCorrPred,
                             ID_FALSE )
              != IDE_SUCCESS );

    // BUG-42637
    // unnest 과정에서 생성된 view의 target에 lob 컬럼이 있을경우 LobLocatorFunc을 연결해준다.
    // view는 반드시 lobLocator 타입으로 변환되어야 한다.
    IDE_TEST( qmvQuerySet::addLobLocatorFunc( sSQStatement, sSQSFWGH->target )
              != IDE_SUCCESS );

    IDE_TEST( transformSubqueryToView( aStatement,
                                       (qtcNode *)aSQPredicate->node.arguments,
                                       &sViewFrom )
              != IDE_SUCCESS );

    // Subquery에서 제거된 correlation predicate을 view와의 join predicate으로 변환
    IDE_TEST( toViewColumns( sSQStatement,
                             sViewFrom->tableRef,
                             &sCorrPred,
                             ID_FALSE )
              != IDE_SUCCESS );

    qtc::dependencyClear( &sViewFrom->semiAntiJoinDepInfo );

    if( aSQPredicate->node.module == &mtfExists )
    {
        qtc::dependencyAnd( &sCorrPred->depInfo,
                            &aSFWGH->depInfo,
                            &sDepInfo );
        if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
        {
            if( sIsSingleRow == ID_TRUE )
            {
                // Inner join으로 처리한다.
            }
            else
            {
                qcgPlan::registerPlanProperty(
                            aStatement,
                            PLAN_PROPERTY_OPTIMIZER_SEMI_JOIN_REMOVE );

                if ( QCU_OPTIMIZER_SEMI_JOIN_REMOVE == 1 )
                {
                    // BUG-45172 semi 조인을 제거할 조건을 검사하여 flag를 설정해 둔다.
                    // 상위에 서브쿼리가 semi 조인일 경우 flag 를 보고 semi 조인을 제거함
                    sIsRemoveSemi = isRemoveSemiJoin( sSQStatement, sSQParseTree );
                }
                else
                {
                    sIsRemoveSemi = ID_FALSE;
                }

                // Semi-join
                IDE_TEST( setJoinType( sCorrPred, ID_FALSE, sIsRemoveSemi, sViewFrom )
                          != IDE_SUCCESS );

                removeDownSemiJoinFlag( sSQSFWGH );

                setNoMergeHint( sViewFrom );
            }
            setJoinMethodHint( aStatement, aSFWGH, sCorrPred, sViewFrom, ID_FALSE );
        }
        else
        {
            // Nothing to do.
            // Outer query의 outer query를 참조하는 predicate인 경우
        }
    }
    else
    {
        // Anti-join
        IDE_TEST( setJoinType( sCorrPred, ID_TRUE, ID_FALSE, sViewFrom )
                  != IDE_SUCCESS );

        setNoMergeHint( sViewFrom );
        setJoinMethodHint( aStatement, aSFWGH, sCorrPred, sViewFrom, ID_TRUE );
    }

    /* BUG-47786 Unnesting 결과 오류 */
    if ( aSQPredicate->depInfo.depCount == 1 )
    {
        sTuple = aSQPredicate->depInfo.depend[0];

        if ( ( QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTuple].lflag & MTC_TUPLE_STORAGE_MASK )
             == MTC_TUPLE_STORAGE_DISK )
        {
            aSFWGH->lflag &= ~QMV_SFWGH_UNNEST_LEFT_DISK_MASK;
            aSFWGH->lflag |= QMV_SFWGH_UNNEST_LEFT_DISK_TRUE;
        }
    }

    // EXISTS/NOT EIXSTS가 있던 자리에 view join predicate을 복사한다.
    sNext = aSQPredicate->node.next;
    idlOS::memcpy( aSQPredicate, sCorrPred, ID_SIZEOF( qtcNode ) );
    aSQPredicate->node.next = sNext;

    // FROM절의 첫 번째에 나열한다.
    sViewFrom->next = aSFWGH->from;
    aSFWGH->from = sViewFrom;

    // Dependency 설정
    IDE_TEST( qtc::dependencyOr( &aSFWGH->depInfo,
                                 &sViewFrom->depInfo,
                                 &aSFWGH->depInfo )
              != IDE_SUCCESS );
    IDE_TEST( qtc::dependencyOr( &aSFWGH->thisQuerySet->depInfo,
                                 &sViewFrom->depInfo,
                                 &aSFWGH->thisQuerySet->depInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::setJoinMethodHint( qcStatement * aStatement,
                                 qmsSFWGH    * aSFWGH,
                                 qtcNode     * aJoinPred,
                                 qmsFrom     * aViewFrom,
                                 idBool        aIsAntiJoin )
{
/***********************************************************************
 *
 * Description :
 *     Subquery에 명시한 hint에 따라 outer query에서 join method hint를
 *     설정해준다.
 *
 * Implementation :
 *     | Subquery의 hint    | Outer query의 hint |
 *     | NL_SJ, NL_AJ       | USE_NL             |
 *     | HASH_SJ, HASH_AJ   | USE_HASH           |
 *     | MERGE_SJ, MERGE_AJ | USE_MERGE          |
 *     | SORT_SJ, SORT_AJ   | USE_SORT           |
 * 
 *   - PROJ-2385 //
 *     NL_AJ, MERGE_SJ/_AJ를 제외하고는 
 *     모두 Inverse Join Method Hint 적용이 가능하다.
 *
 ***********************************************************************/

    SInt                 sTable;
    idBool               sExistHint   = ID_TRUE;
    UInt                 sFlag        = 0;
    qmsJoinMethodHints * sJoinMethodHint;
    qmsParseTree       * sViewParseTree;
    qmsSFWGH           * sViewSFWGH;
    qmsFrom            * sFrom;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::setJoinMethodHint::__FT__" );

    sViewParseTree = (qmsParseTree *)aViewFrom->tableRef->view->myPlan->parseTree;
    sViewSFWGH     = sViewParseTree->querySet->SFWGH;

    if( aIsAntiJoin == ID_FALSE )
    {
        // Inner/semi join인 경우
        switch( sViewSFWGH->hints->semiJoinMethod )
        {
            case QMO_SEMI_JOIN_METHOD_NOT_DEFINED:
                sExistHint = ID_FALSE;
                break;
            case QMO_SEMI_JOIN_METHOD_NL:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_NL;
                break;
            case QMO_SEMI_JOIN_METHOD_HASH:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_HASH;
                break;
            case QMO_SEMI_JOIN_METHOD_MERGE:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_MERGE;
                break;
            case QMO_SEMI_JOIN_METHOD_SORT:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_SORT;
                break;
            default:
                IDE_FT_ASSERT( 0 );
        }
    }
    else
    {
        // Anti join인 경우
        switch( sViewSFWGH->hints->antiJoinMethod )
        {
            case QMO_ANTI_JOIN_METHOD_NOT_DEFINED:
                sExistHint = ID_FALSE;
                break;
            case QMO_ANTI_JOIN_METHOD_NL:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_NL;
                break;
            case QMO_ANTI_JOIN_METHOD_HASH:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_HASH;
                break;
            case QMO_ANTI_JOIN_METHOD_MERGE:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_MERGE;
                break;
            case QMO_ANTI_JOIN_METHOD_SORT:
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_SORT;
                break;
            default:
                IDE_FT_ASSERT( 0 );
        }
    }

    // PROJ-2385
    // PROJ-2339에서 추가한 힌트를 삭제하고, 부가 힌트를 지원한다.
    // 이전의 힌트로는, INVERSE를 제외한 나머지 Method를 선택할 수 없기 때문이다.
    switch ( sViewSFWGH->hints->inverseJoinOption )
    {
        case QMO_INVERSE_JOIN_METHOD_DENIED: // NO_INVERSE_JOIN
        {
            if ( sExistHint == ID_FALSE )
            {
                /* Join Method Hint가 존재하지 않는 경우,
                 * 모든 Inverse Join Method를 배제한 나머지 Method만 고려한다.  */
                sExistHint = ID_TRUE;
                sFlag |= QMO_JOIN_METHOD_MASK;
                sFlag &= ~QMO_JOIN_METHOD_INVERSE;
            }
            else
            {
                /* Join Method Hint가 존재하는 경우,
                 * 해당 Method 중에서 Inverse Join Method를 배제한다. */
                sFlag &= ~QMO_JOIN_METHOD_INVERSE;
            }
            break;
        }
        case QMO_INVERSE_JOIN_METHOD_ONLY: // INVERSE_JOIN
        {
            if ( sExistHint == ID_FALSE )
            {
                /* Join Method Hint가 존재하지 않는 경우,
                 * 모든 Inverse Join Method만 고려한다. */
                sExistHint = ID_TRUE;
                sFlag &= ~QMO_JOIN_METHOD_MASK;
                sFlag |= QMO_JOIN_METHOD_INVERSE;
            }
            else
            {
                /* Join Method Hint가 존재하는 경우,
                 * 해당 Method 중에서 Inverse Join Method만 고려한다.
                 * 단, 해당 Method에 Inverse Join Method가 아예 선택될 수 없는 경우
                 * ( 예, NL Join (Anti), MERGE Join(Semi/Anti) )
                 * INVERSE 힌트를 암묵적으로 무시한다. */

                if ( ( sViewSFWGH->hints->antiJoinMethod == QMO_ANTI_JOIN_METHOD_NL    ) ||
                     ( sViewSFWGH->hints->semiJoinMethod == QMO_SEMI_JOIN_METHOD_MERGE ) ||
                     ( sViewSFWGH->hints->antiJoinMethod == QMO_ANTI_JOIN_METHOD_MERGE ) )
                {
                    // Nothing to do.
                }
                else
                {
                    sFlag &= ( ~QMO_JOIN_METHOD_MASK | QMO_JOIN_METHOD_INVERSE );
                }
            }
            break;
        }
        case QMO_INVERSE_JOIN_METHOD_ALLOWED: // (default)
            // Nothing to do.  
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    if( sExistHint == ID_TRUE )
    {
        sTable = qtc::getPosFirstBitSet( &aJoinPred->depInfo );

        while( sTable != QTC_DEPENDENCIES_END )
        {
            if( sTable != aViewFrom->tableRef->table )
            {
                IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsJoinMethodHints ),
                                                           (void**)&sJoinMethodHint )
                          != IDE_SUCCESS );

                QCP_SET_INIT_JOIN_METHOD_HINTS( sJoinMethodHint );

                // PROJ-2339, 2385
                // 만약 Join Method가 Inverse Join Method를 허락하는 경우라면 (ALLOWED)
                // 정반대 dependency를 가진 Inverse Join Method도 같이 고려해야 한다.
                if( sViewSFWGH->hints->inverseJoinOption == QMO_INVERSE_JOIN_METHOD_ALLOWED )
                {
                    sJoinMethodHint->isUndirected = ID_TRUE;
                }
                else
                {
                    // Default : ID_FALSE
                    sJoinMethodHint->isUndirected = ID_FALSE;
                }

                IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsHintTables ),
                                                           (void**)&sJoinMethodHint->joinTables )
                          != IDE_SUCCESS );

                IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsHintTables ),
                                                           (void**)&sJoinMethodHint->joinTables->next )
                          != IDE_SUCCESS );

                sFrom = QC_SHARED_TMPLATE( aStatement )->tableMap[sTable].from;

                SET_POSITION( sJoinMethodHint->joinTables->userName,  sFrom->tableRef->userName );
                SET_POSITION( sJoinMethodHint->joinTables->tableName, sFrom->tableRef->tableName );
                sJoinMethodHint->joinTables->table = sFrom;

                SET_POSITION( sJoinMethodHint->joinTables->next->userName,  aViewFrom->tableRef->userName );
                SET_POSITION( sJoinMethodHint->joinTables->next->tableName, aViewFrom->tableRef->tableName );
                sJoinMethodHint->joinTables->next->table = aViewFrom;
                sJoinMethodHint->joinTables->next->next  = NULL;

                sJoinMethodHint->flag = sFlag;
                qtc::dependencySet( sTable,
                                    &sJoinMethodHint->depInfo );
                qtc::dependencyOr( &aViewFrom->depInfo,
                                   &sJoinMethodHint->depInfo,
                                   &sJoinMethodHint->depInfo );
                sJoinMethodHint->next = aSFWGH->hints->joinMethod;
                aSFWGH->hints->joinMethod = sJoinMethodHint;
            }
            else
            {
                // Nothing to do.
            }

            sTable = qtc::getPosNextBitSet( &aJoinPred->depInfo, sTable );
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

void
qmoUnnesting::setNoMergeHint( qmsFrom * aViewFrom )
{
/***********************************************************************
 *
 * Description :
 *     View에 포함된 relation이 둘 이상인 경우 NO_MERGE hint를
 *     설정하여 view merging이 수행되지 않도록 한다.
 *     View를 대상으로 semi/anti join을 시도하는 경우에만 필요하다.
 *     만약 view 내부의 join보다 semi/anti join이 먼저 수행되면
 *     결과가 달라질 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement     * sViewStatement;
    qmsParseTree    * sViewParseTree;
    qmsSFWGH        * sViewSFWGH;

    sViewStatement = aViewFrom->tableRef->view;
    sViewParseTree = (qmsParseTree *)sViewStatement->myPlan->parseTree;
    sViewSFWGH     = sViewParseTree->querySet->SFWGH;

    // View에 둘 이상의 relation이 포함된 경우에만 NO_MERGE hint를 설정한다.
    if( ( sViewSFWGH->from->next != NULL ) ||
        ( sViewSFWGH->from->joinType != QMS_NO_JOIN ) )
    {
        aViewFrom->tableRef->noMergeHint = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC
qmoUnnesting::setJoinType( qtcNode * aPredicate,
                           idBool    aType,
                           idBool    aIsRemoveSemi,
                           qmsFrom * aViewFrom )
{
/***********************************************************************
 *
 * Description :
 *     Join predicate에 semi/anti join의 종류 방향을 flag로 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode   * sArg;
    qcDepInfo   sDepInfo;
    
    IDU_FIT_POINT_FATAL( "qmoUnnesting::setJoinType::__FT__" );

    if( ( aPredicate->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        for( sArg = (qtcNode *)aPredicate->node.arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next )
        {
            IDE_TEST( setJoinType( sArg, aType, aIsRemoveSemi, aViewFrom ) != IDE_SUCCESS );
        }
    }
    else
    {
        qtc::dependencySet( aViewFrom->tableRef->table, &sDepInfo );
        qtc::dependencyAnd( &aPredicate->depInfo, &sDepInfo, &sDepInfo );
        if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
        {
            // Type 설정
            if( aType == ID_FALSE )
            {
                // Semi-join
                aPredicate->lflag &= ~QTC_NODE_JOIN_TYPE_MASK;
                aPredicate->lflag |= QTC_NODE_JOIN_TYPE_SEMI;

                // BUG-45172 semi 조인을 제거할 조건을 검사하여 flag를 설정해 둔다.
                // 상위에 서브쿼리가 semi 조인일 경우 flag 를 보고 하위 semi 조인을 제거함
                if ( aIsRemoveSemi == ID_TRUE )
                {
                    aPredicate->lflag &= ~QTC_NODE_REMOVABLE_SEMI_JOIN_MASK;
                    aPredicate->lflag |= QTC_NODE_REMOVABLE_SEMI_JOIN_TRUE;
                }
                else
                {
                    // nothing to do.
                }
            }
            else
            {
                // Anti-join
                aPredicate->lflag &= ~QTC_NODE_JOIN_TYPE_MASK;
                aPredicate->lflag |= QTC_NODE_JOIN_TYPE_ANTI;
            }

            // BUG-45167 서브쿼리가 부모쿼리의 테이블을 참조하는 one table predicate를 사용하는 경우 fatal이 발생합니다.
            // 서브쿼리를 참조하는 predicate 의 디펜던시만 추가해야 한다.
            qtc::dependencyOr( &aPredicate->depInfo,
                               &aViewFrom->semiAntiJoinDepInfo,
                               &aViewFrom->semiAntiJoinDepInfo );
        }
        else
        {
            // Nothing to do.
            // SELECT * FROM T1 WHERE EXISTS (SELECT 0 FROM T2 WHERE T1.I1 = T2.I1 AND T1.I2 > 0);
            // 에서 I1.I2는 correlation predicate이지만 join 대상이 없으므로 여기에 해당한다.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoUnnesting::isRemoveSemiJoin( qcStatement  * aSQStatement,
                                       qmsParseTree * aSQParseTree )
{
/***********************************************************************
 *
 * Description : BUG-45172
 *     semi 조인을 제거할 조건을 검사하여 flag를 설정해 둔다.
 *     상위 서브쿼리가 semi 조인이면
 *     하위 semi 조인에 flag가 설정되었을 경우 하위 semi 조인을 제거한다.
 *
 * Implementation :
 *              1. union x, target 1
 *              2. view x, group x, aggr x, having x, ansi x
 *              3. 테이블이 1개인 경우 제거 가능
 *              4. 테이블이 2개인 경우
 *                  2개의 테이블중 1개의 테이블이 1row 가 보장될때
 ***********************************************************************/

    idBool         sResult = ID_FALSE;
    qmsSFWGH     * sSFWGH;
    qmsFrom      * sFrom;
    qcmTableInfo * sTableInfo;
    UInt           i;
    UShort         sTableCount;

    if ( aSQParseTree->querySet->setOp != QMS_NONE )
    {
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        sSFWGH     = aSQParseTree->querySet->SFWGH;
    }

    // 질의 형태 체크
    if ( ( sSFWGH->from->joinType        == QMS_NO_JOIN ) &&
         ( sSFWGH->from->tableRef        != NULL ) &&
         ( sSFWGH->from->tableRef->view  == NULL ) &&
         ( sSFWGH->group                 == NULL ) &&
         ( sSFWGH->having                == NULL ) &&
         ( sSFWGH->aggsDepth1            == NULL ) )
    {
        if ( sSFWGH->from->next == NULL )
        {
            // 테이블이 1개인 경우 제거 가능
            sTableCount = 1;

            sResult = ID_TRUE;
        }
        else if ( ( sSFWGH->from->next                   != NULL ) &&
                  ( sSFWGH->from->next->joinType         == QMS_NO_JOIN ) &&
                  ( sSFWGH->from->next->tableRef         != NULL ) &&
                  ( sSFWGH->from->next->tableRef->view   == NULL ) &&
                  ( sSFWGH->from->next->next             == NULL ) )
        {
            // 테이블이 2개인 경우
            sTableCount = 2;
        }
        else
        {
            IDE_CONT( INVALID_FORM );
        }
    }
    else
    {
        IDE_CONT( INVALID_FORM );
    }

    // target 이 1개 인 경우만
    if ( ( sSFWGH->target       != NULL ) &&
         ( sSFWGH->target->next == NULL ) )
    {
        // value or 컬럼
        if ( ( QTC_IS_COLUMN( aSQStatement, sSFWGH->target->targetColumn ) == ID_TRUE ) ||
             ( sSFWGH->target->targetColumn->node.module == &qtc::valueModule ) )
        {
            // nothing to do.
        }
        else
        {
            IDE_CONT( INVALID_FORM );
        }
    }
    else
    {
        IDE_CONT( INVALID_FORM );
    }

    if ( ( sTableCount   == 2    ) &&
         ( sSFWGH->where != NULL ) )
    {
        for ( sFrom = sSFWGH->from;
              sFrom != NULL;
              sFrom = sFrom->next )
        {
            sTableInfo = sFrom->tableRef->tableInfo;
            
            for( i = 0; i < sTableInfo->indexCount; i++ )
            {
                if ( ( sTableInfo->indices[i].isUnique    == ID_TRUE ) &&
                     ( sTableInfo->indices[i].keyColCount == 1 ) )
                {
                    // if B.i1 is unique
                    // find A.i1 = B.i1 and A.i1 = 상수
                    // find B.i1 = 상수
                    if ( findUniquePredicate(
                                aSQStatement,
                                sSFWGH,
                                sFrom->tableRef->table,
                                sTableInfo->indices[i].keyColumns[0].column.id,
                                sSFWGH->where ) == ID_TRUE )
                    {
                        sResult = ID_TRUE;

                        break;
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
    }
    else
    {
        // nothing to do.
    }

    IDE_EXCEPTION_CONT( INVALID_FORM );

    return sResult;
}

idBool qmoUnnesting::findUniquePredicate( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          UInt          aTableID,
                                          UInt          aUniqueID,
                                          qtcNode     * aNode )
{
    idBool    sResult = ID_FALSE;
    mtcNode * sFindColumn = NULL;
    mtcNode * sNode;
    mtcNode * sTemp;

    if ( aNode->node.module == &mtfAnd )
    {
        sNode = aNode->node.arguments;
    }
    else
    {
        sNode = (mtcNode*)aNode;
    }

    // unique = 상수
    for ( sTemp = sNode; sTemp != NULL; sTemp = sTemp->next )
    {
        if( sTemp->module == &mtfEqual )
        {
            if ( ( sTemp->arguments->table == aTableID ) &&
                 ( QTC_STMT_COLUMN( aStatement, (qtcNode*)sTemp->arguments )->column.id == aUniqueID ) &&
                 ( sTemp->arguments->next->module == &qtc::valueModule ) )
            {
                sResult = ID_TRUE;
            }
            else if ( ( sTemp->arguments->next->table == aTableID ) &&
                      ( QTC_STMT_COLUMN( aStatement, (qtcNode*)sTemp->arguments->next )->column.id == aUniqueID ) &&
                      ( sTemp->arguments->module == &qtc::valueModule ) )
            {
                sResult = ID_TRUE;
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
    }

    // A.i1 = B.i1 and A.i1 = 상수
    if ( sResult == ID_FALSE )
    {
        for ( sTemp = sNode; sTemp != NULL; sTemp = sTemp->next )
        {
            if( sTemp->module == &mtfEqual )
            {
                if ( ( qtc::dependencyEqual( &((qtcNode*)sTemp)->depInfo, &aSFWGH->depInfo ) == ID_TRUE ) &&
                     ( QTC_IS_COLUMN( aStatement, (qtcNode*)sTemp->arguments )               == ID_TRUE ) &&
                     ( QTC_IS_COLUMN( aStatement, (qtcNode*)sTemp->arguments->next )         == ID_TRUE ) &&
                     ( sTemp->arguments->table  != sTemp->arguments->next->table ) )
                {
                    if ( QTC_STMT_COLUMN( aStatement, (qtcNode*)sTemp->arguments )->column.id == aUniqueID )
                    {
                        sFindColumn = sTemp->arguments->next;
                    }
                    else if ( QTC_STMT_COLUMN( aStatement, (qtcNode*)sTemp->arguments->next )->column.id == aUniqueID )
                    {
                        sFindColumn = sTemp->arguments;
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
            }
            else
            {
                // nothing to do.
            }
        }
    }
    else
    {
        // nothing to do.
    }

    if ( sFindColumn != NULL )
    {
        for ( sTemp = sNode; sTemp != NULL; sTemp = sTemp->next )
        {
            if( sTemp->module == &mtfEqual )
            {
                if ( qtc::dependencyEqual( &((qtcNode*)sTemp)->depInfo,
                                           &((qtcNode*)sFindColumn)->depInfo ) == ID_TRUE )
                {
                    if ( ( sFindColumn->table  == sTemp->arguments->table  ) &&
                         ( sFindColumn->column == sTemp->arguments->column ) &&
                         ( sTemp->arguments->next->module == &qtc::valueModule ) )
                    {
                        sResult = ID_TRUE;
                    }
                    else if ( ( sFindColumn->table  == sTemp->arguments->next->table  ) &&
                              ( sFindColumn->column == sTemp->arguments->next->column ) &&
                              ( sTemp->arguments->module == &qtc::valueModule ) )
                    {
                        sResult = ID_TRUE;
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
            }
            else
            {
                // nothing to do.
            }
        }
    }
    else
    {
        // nothing to do.
    }

    return sResult;
}

void qmoUnnesting::removeDownSemiJoinFlag( qmsSFWGH * aSFWGH )
{
    qmsTarget   * sTarget   = aSFWGH->target;
    qtcNode     * sNode     = aSFWGH->where;
    qtcNode     * sTemp;

    if ( sNode != NULL )
    {
        if ( sNode->node.module == &mtfAnd )
        {
            sNode = (qtcNode*)sNode->node.arguments;
        }
        else
        {
            sNode = sNode;
        }

        // target = 상수 조건이 존재하면 semi 조인을 제거하지 않는다.
        for ( ; sTarget != NULL; sTarget = sTarget->next )
        {
            for ( sTemp = sNode; sTemp != NULL; sTemp = (qtcNode*)sTemp->node.next )
            {
                if ( sTemp->node.module == &mtfEqual )
                {
                    if ( ( sTarget->targetColumn->node.table == sTemp->node.arguments->table ) &&
                         ( sTarget->targetColumn->node.column == sTemp->node.arguments->column ) &&
                         ( sTemp->node.arguments->next->module == &qtc::valueModule ) )
                    {
                        break;
                    }
                    else if ( ( sTarget->targetColumn->node.table == sTemp->node.arguments->next->table ) &&
                              ( sTarget->targetColumn->node.column == sTemp->node.arguments->next->column ) &&
                              ( sTemp->node.arguments->module == &qtc::valueModule ) )
                    {
                        break;
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
            }
            
            if ( sTemp != NULL )
            {
                break;
            }
            else
            {
                // nothing to do.
            }
        }

        if ( sTarget == NULL )
        {
            for ( sTemp = sNode; sTemp != NULL; sTemp = (qtcNode*)sTemp->node.next )
            {
                if ( ((sTemp->lflag & QTC_NODE_REMOVABLE_SEMI_JOIN_MASK) == QTC_NODE_REMOVABLE_SEMI_JOIN_TRUE) &&
                     ((sTemp->lflag & QTC_NODE_JOIN_TYPE_MASK) == QTC_NODE_JOIN_TYPE_SEMI) )
                {
                    sTemp->lflag &= ~QTC_NODE_JOIN_TYPE_MASK;
                }
                else
                {
                    // nothing to do.
                }
            }
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
}

IDE_RC
qmoUnnesting::makeDummyConstant( qcStatement  * aStatement,
                                 qtcNode     ** aResult )
{
/***********************************************************************
 *
 * Description :
 *     CHAR type의 '0'값을 갖는 constant node를 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcNamePosition   sPosition;
    qtcNode        * sConstNode[2];

    IDU_FIT_POINT_FATAL( "qmoUnnesting::makeDummyConstant::__FT__" );

    sPosition.stmtText = (SChar*)"'0'";
    sPosition.offset   = 0;
    sPosition.size     = 3;

    IDE_TEST( qtc::makeValue( aStatement,
                              sConstNode,
                              (const UChar*)"CHAR",
                              4,
                              &sPosition,
                              (const UChar*)"1",
                              1,
                              MTC_COLUMN_NORMAL_LITERAL ) 
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sConstNode[0] )
              != IDE_SUCCESS );

    *aResult = sConstNode[0];

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::removePassNode( qcStatement * aStatement,
                              qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     HAVING절의 predicate을 WHERE절로 옮기기 위해 HAVING절에 포함된
 *     pass node들을 모두 제거한다.
 *
 * Implementation :
 *     Pass node의 child를 pass node가 있던 위치에 복사한다.
 *
 ***********************************************************************/

    qtcNode * sArg;
    qtcNode * sNode;
    mtcNode * sNext;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::removePassNode::__FT__" );

    if( aNode->node.module == &qtc::passModule )
    {
        sNext = aNode->node.next;

        if( QTC_IS_TERMINAL( (qtcNode *)aNode->node.arguments ) == ID_TRUE )
        {
            sNode = (qtcNode *)aNode->node.arguments;
        }
        else
        {
            IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                             (qtcNode *)aNode->node.arguments,
                                             &sNode,
                                             ID_FALSE,
                                             ID_FALSE,
                                             ID_FALSE,
                                             ID_FALSE )
                      != IDE_SUCCESS );
        }
        idlOS::memcpy( aNode, sNode, ID_SIZEOF( qtcNode ) );
        aNode->node.next = sNext;
    }
    else
    {
        for( sArg = (qtcNode *)aNode->node.arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next )
        {
            IDE_TEST( removePassNode( aStatement,
                                      sArg )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::setDummySelect( qcStatement * aStatement,
                              qtcNode     * aNode,
                              idBool        aCheckAggregation )
{
/***********************************************************************
 *
 * Description :
 *     Subquery의 SELECT절을 다음과 같이 변경한다.
 *     SELECT DISTINCT i1, i2 FROM t1 WHERE ...
 *     => SELECT '0' FROM t1 WHERE ...
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode        * sConstNode = NULL;
    qmsTarget      * sTarget;
    qmsParseTree   * sParseTree;
    qcStatement    * sSQStatement;
    idBool           sChange = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::setDummySelect::__FT__" );

    IDE_FT_ERROR( aNode->node.arguments->module == &qtc::subqueryModule );

    sSQStatement = ((qtcNode *)aNode->node.arguments)->subquery;
    sParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;

    /* TASK-7219 Shard Transformer Refactoring */
    if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                        SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
         == ID_TRUE )
    {
        IDE_TEST( sdi::preAnalyzeQuerySet( sSQStatement,
                                           sParseTree->querySet,
                                           QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if( sParseTree->querySet->setOp == QMS_NONE )
    {
        if( ( aNode->node.module == &mtfExists ) ||
            ( aNode->node.module == &mtfNotExists ) )
        {
            // DISTINCT할 필요 없다.
            sParseTree->querySet->SFWGH->selectType = QMS_ALL;
            sChange = ID_TRUE;

            if( aCheckAggregation == ID_TRUE )
            {
                for( sTarget = sParseTree->querySet->target;
                     sTarget != NULL;
                     sTarget = sTarget->next )
                {
                    // EXISTS(SELECT COUNT(...), ...) 의 경우 상수로 변환 시 결과가 달라질 수 있다.
                    if( ( sTarget->targetColumn->lflag & QTC_NODE_AGGREGATE_MASK )
                        == QTC_NODE_AGGREGATE_EXIST )
                    {
                        sChange = ID_FALSE;
                        break;
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
        }
        else
        {
            // UNIQUE시에는 DISTINCT절을 남겨야 한다.
            if( sParseTree->querySet->SFWGH->selectType == QMS_ALL )
            {
                sChange = ID_TRUE;
            }
            else
            {
                sChange = ID_FALSE;
            }
        }

        if( sChange == ID_TRUE )
        {
            // BUG-45591 aggsDepth1 관리
            // target 이 여러개 일때 모두 제거 가능한지 체크해야 한다.
            for ( sTarget = sParseTree->querySet->target;
                  sTarget != NULL;
                  sTarget = sTarget->next )
            {
                delAggrNode( sParseTree->querySet->SFWGH, sTarget->targetColumn );
            }

            // BUG-45271 exists 변환후 에러가 발생합니다.
            // exists 변환시 target 은 복사를 하지만 컨버젼 노드는 복사하지 않는다.
            // 잘못된 연산을 수행하다 에러를 발생시킵니다.
            // exists 변환후 target 은 의미가 없으므로 무조건 제거하고 새로 생성합니다.
            IDE_TEST( makeDummyConstant( sSQStatement,
                                         & sConstNode )
                      != IDE_SUCCESS );

            sTarget = sParseTree->querySet->target;

            QMS_TARGET_INIT( sTarget );

            sTarget->targetColumn = sConstNode;
            sTarget->flag        &= ~QMS_TARGET_IS_NULLABLE_MASK;
            sTarget->flag        |= QMS_TARGET_IS_NULLABLE_TRUE;
            sTarget->next         = NULL;

            aNode->node.arguments->arguments = (mtcNode *)sTarget->targetColumn;
        }
        else
        {
            // Nothing to do.
        }

        if( ( sParseTree->querySet->SFWGH->aggsDepth1 == NULL ) &&
            ( sParseTree->querySet->SFWGH->aggsDepth2 == NULL ) &&
            ( sParseTree->querySet->SFWGH->group != NULL ) )
        {
            // GROUP BY절을 제거한다.
            sParseTree->querySet->SFWGH->group = NULL;

            if( sParseTree->querySet->SFWGH->having != NULL )
            {
                IDE_TEST( removePassNode( aStatement,
                                          sParseTree->querySet->SFWGH->having )
                          != IDE_SUCCESS );

                // HAVING절을 WHERE절에 덧 붙인다.
                IDE_TEST( concatPredicate( sSQStatement,
                                           sParseTree->querySet->SFWGH->where,
                                           sParseTree->querySet->SFWGH->having,
                                           &sParseTree->querySet->SFWGH->where )
                          != IDE_SUCCESS );

                sParseTree->querySet->SFWGH->having = NULL;
            }
            else
            {
                // Nothin to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // SET 연산인 경우
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::concatPredicate( qcStatement  * aStatement,
                               qtcNode      * aPredicate1,
                               qtcNode      * aPredicate2,
                               qtcNode     ** aResult )
{
/***********************************************************************
 *
 * Description :
 *     두개의 predicate을 conjunctive form으로 연결한다.
 *
 * Implementation :
 *     둘 중 한곳에 AND가 존재하는 경우 이를 활용한다.
 *     어느 한쪽에도 AND가 존재하지 않으면 새로 생성한다.
 *
 ***********************************************************************/

    qcNamePosition   sEmptyPosition;
    qtcNode        * sANDNode[2];
    qtcNode        * sArg;
    UInt             sArgCount1 = 0;
    UInt             sArgCount2 = 0;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::concatPredicate::__FT__" );

    IDE_DASSERT( aPredicate2 != NULL );

    if( aPredicate1 == NULL )
    {
        *aResult = aPredicate2;
    }
    else
    {
        sArgCount1 = ( aPredicate1->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK );
        sArgCount2 = ( aPredicate2->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

        if( ( aPredicate1->node.module == &mtfAnd ) &&
            ( sArgCount1 < MTC_NODE_ARGUMENT_COUNT_MAXIMUM ) )
        {
            // aPredicate1이 AND인 경우

            // AND의 마지막 argument를 찾는다.
            sArg = (qtcNode *)aPredicate1->node.arguments;
            while( sArg->node.next != NULL )
            {
                sArg = (qtcNode *)sArg->node.next;
            }

            if( ( aPredicate2->node.module == &mtfAnd ) &&
                ( sArgCount1 + sArgCount2 <= MTC_NODE_ARGUMENT_COUNT_MAXIMUM ) )
            {
                // aPredicate2도 AND로 묶인 경우 argument들끼리 연결한다.
                sArg->node.next = aPredicate2->node.arguments;
                aPredicate1->node.lflag += sArgCount2;
            }
            else
            {
                // aPredicate2가 AND가 아닌 경우 직접 연결한다.
                sArg->node.next = (mtcNode *)aPredicate2;
                aPredicate1->node.lflag++;
            }

            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        aPredicate1 )
                      != IDE_SUCCESS );

            *aResult = aPredicate1;
        }
        else
        {
            if( ( aPredicate2->node.module == &mtfAnd ) &&
                ( sArgCount2 < MTC_NODE_ARGUMENT_COUNT_MAXIMUM ) )
            {
                // aPredicate2가 AND인 경우

                // AND의 마지막 argument를 찾는다.
                sArg = (qtcNode *)aPredicate2->node.arguments;
                while( sArg->node.next != NULL )
                {
                    sArg = (qtcNode *)sArg->node.next;
                }

                sArg->node.next = (mtcNode *)aPredicate1;
                aPredicate2->node.lflag++;

                IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                            aPredicate2 )
                          != IDE_SUCCESS );

                *aResult = aPredicate2;
            }
            else
            {
                SET_EMPTY_POSITION( sEmptyPosition );

                // 어느 한쪽도 AND가 존재하지 않는 경우
                IDE_TEST( qtc::makeNode( aStatement,
                                         sANDNode,
                                         &sEmptyPosition,
                                         &mtfAnd )
                          != IDE_SUCCESS );

                sANDNode[0]->node.arguments       = (mtcNode *)aPredicate1;
                sANDNode[0]->node.arguments->next = (mtcNode *)aPredicate2;

                IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                            sANDNode[0] )
                          != IDE_SUCCESS );

                sANDNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                sANDNode[0]->node.lflag |= 2;

                // AND node를 최종 결과로 반환한다.
                *aResult = sANDNode[0];
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoUnnesting::isSubqueryPredicate( qtcNode * aPredicate )
{
/***********************************************************************
 *
 * Description :
 *     Predicate이 =ANY, =ALL 등의 quantified predicate이며
 *     두번째 인자가 subquery인 경우에만 true를 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if( ( aPredicate->node.lflag & MTC_NODE_COMPARISON_MASK )
        == MTC_NODE_COMPARISON_TRUE )
    {
        switch( aPredicate->node.lflag & MTC_NODE_OPERATOR_MASK )
        {
            case MTC_NODE_OPERATOR_EQUAL:
            case MTC_NODE_OPERATOR_NOT_EQUAL:
            case MTC_NODE_OPERATOR_LESS:
            case MTC_NODE_OPERATOR_LESS_EQUAL:
            case MTC_NODE_OPERATOR_GREATER:
            case MTC_NODE_OPERATOR_GREATER_EQUAL:
                IDE_DASSERT( aPredicate->node.arguments       != NULL );
                IDE_DASSERT( aPredicate->node.arguments->next != NULL );

                if( aPredicate->node.arguments->next->module == &qtc::subqueryModule )
                {
                    return ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
                break;
            default:
                break;
        }
    }
    else
    {
        // Nothing to do.
    }

    return ID_FALSE;
}

idBool
qmoUnnesting::isQuantifiedSubquery( qtcNode * aPredicate )
{
/***********************************************************************
 *
 * Description :
 *     Predicate이 =ANY, =ALL 등의 quantified predicate이며
 *     두번째 인자가 subquery인 경우에만 true를 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool sResult = ID_FALSE;

    if( ( aPredicate->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK )
        == MTC_NODE_GROUP_COMPARISON_TRUE )
    {
        IDE_DASSERT( aPredicate->node.arguments       != NULL );
        IDE_DASSERT( aPredicate->node.arguments->next != NULL );

        if( aPredicate->node.arguments->next->module == &qtc::subqueryModule )
        {
            sResult = ID_TRUE;
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

    return sResult;
}

idBool
qmoUnnesting::isNullable( qcStatement * aStatement,
                          qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     Nullable한 expression인지 확인한다.
 *     다음 두가지 경우를 제외하고 모두 nullable이다.
 *     1. Column이면서 not nullable constraint가 설정된 경우
 *     2. 상수이면서 null이 아닌 경우
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcColumn * sColumn;
    qtcNode   * sNode;
    UChar     * sValue;
    idBool      sResult = ID_TRUE;

    if( aNode->node.module == &qtc::passModule )
    {
        sNode = (qtcNode *)aNode->node.arguments;
    }
    else
    {
        sNode = aNode;
    }

    if( sNode->node.module == &qtc::valueModule )
    {
        if( ( QTC_STMT_TUPLE( aStatement, sNode )->lflag & MTC_TUPLE_TYPE_MASK )
            == MTC_TUPLE_TYPE_CONSTANT )
        {
            // 상수인 경우
            sColumn = QTC_STMT_COLUMN( aStatement, sNode );
            sValue  = (UChar *)QTC_STMT_TUPLE( aStatement, sNode )->row + sColumn->column.offset;
            if( sColumn->module->isNull( sColumn, 
                                         sValue )
                == ID_FALSE )
            {
                // 상수 값이 null이 아닌 경우
                sResult = ID_FALSE;
            }
            else
            {
                // 상수 값이 null인 경우
            }
        }
        else
        {
            // Bind 변수인 경우
        }
    }
    else
    {
        if( ( QTC_STMT_COLUMN( aStatement, sNode )->flag & MTC_COLUMN_NOTNULL_MASK )
            == MTC_COLUMN_NOTNULL_FALSE )
        {
            // Nullable column인 경우
        }
        else
        {
            // Not nullable column인 경우
            sResult = ID_FALSE;
        }
    }

    return sResult;
}

IDE_RC
qmoUnnesting::genCorrPredicate( qcStatement  * aStatement,
                                qtcNode      * aPredicate,
                                qtcNode      * aOperand1,
                                qtcNode      * aOperand2,
                                idBool         aExistsTrans,
                                qtcNode     ** aResult )
{
/***********************************************************************
 *
 * Description : 
 *     Subquery predicate의 종류에 따라 correlation predicate을 생성한다.
 *
 * Implementation :
 *     다음의 table에 따라 predicate을 변환한다.
 *     | Input | Not nullable | Nullable      |
 *     | =ANY  | a = b        | a = b         |
 *     | <>ANY | a <> b       | a <> b        |
 *     | >ANY  | a > b        | a > b         |
 *     | >=ANY | a >= b       | a >= b        |
 *     | <ANY  | a < b        | a < b         |
 *     | <=ANY | a <= b       | a <= b        |
 *     | =ALL  | a <> b       | LNNVL(a = b)  |
 *     | <>ALL | a = b        | LNNVL(a <> b) |
 *     | >ALL  | a <= b       | LNNVL(a > b)  |
 *     | >=ALL | a < b        | LNNVL(a >= b) |
 *     | <ALL  | a >= b       | LNNVL(a < b)  |
 *     | <=ALL | a > b        | LNNVL(a <= b) |
 *
 *     subquery removal의 경우 다음 tabel에 따라 predicate를 변환한다.
 *     | Input         || correlation predicate
 *     | =ANY  | =ALL  || a = b        
 *     | <>ANY | <>ALL || a <> b       
 *     | >ANY  | >ALL  || a > b        
 *     | >=ANY | >=ALL || a >= b       
 *     | <ANY  | <ALL  || a < b        
 *     | <=ANY | <=ALL || a <= b       
 *
 ***********************************************************************/

    qtcNode        * sLnnvlNode[2];
    qtcNode        * sPredicate[2];
    qtcNode        * sOperand1;
    qtcNode        * sOperand2;
    qmsTableRef    * sTableRef;
    qcNamePosition   sEmptyPosition;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::genCorrPredicate::__FT__" );

    SET_EMPTY_POSITION( sEmptyPosition );

    // Predicate의 operand 복사
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                               (void **)&sOperand1 )
              != IDE_SUCCESS );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                               (void **)&sOperand2 )
              != IDE_SUCCESS );

    idlOS::memcpy( sOperand1, aOperand1, ID_SIZEOF( qtcNode ) );
    idlOS::memcpy( sOperand2, aOperand2, ID_SIZEOF( qtcNode ) );

    sOperand1->node.next = (mtcNode *)sOperand2;
    sOperand2->node.next = NULL;

    if( QTC_IS_COLUMN( aStatement, sOperand1 ) == ID_TRUE )
    {
        // Outer query의 column에 대하여 table alias를 항상 설정해준다.
        sTableRef = QC_SHARED_TMPLATE( aStatement )->tableMap[sOperand1->node.table].from->tableRef;
        if( QC_IS_NULL_NAME( sOperand1->tableName ) == ID_TRUE )
        {
            SET_POSITION( sOperand1->tableName, sTableRef->aliasName );
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

    if( ( ( aPredicate->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK ) == MTC_NODE_GROUP_COMPARISON_TRUE ) &&
        ( ( aPredicate->node.lflag & MTC_NODE_GROUP_MASK ) == MTC_NODE_GROUP_ALL ) &&
        ( aExistsTrans == ID_TRUE ) )
    {
        // Subquery predicate이 ALL계열인 경우
        if( ( isNullable( aStatement, sOperand1 ) == ID_TRUE ) ||
            ( isNullable( aStatement, sOperand2 ) == ID_TRUE ) )
        {
            // Operand가 nullable인 경우 LNNVL이 필요하다.
            IDE_TEST( qtc::makeNode( aStatement,
                                     sPredicate,
                                     &sEmptyPosition,
                                     toNonQuantifierModule( aPredicate->node.module ) )
                      != IDE_SUCCESS );

            sPredicate[0]->node.arguments = (mtcNode *)sOperand1;
            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        sPredicate[0] )
                      != IDE_SUCCESS );

            IDE_TEST( qtc::makeNode( aStatement,
                                     sLnnvlNode,
                                     &sEmptyPosition,
                                     &mtfLnnvl )
                      != IDE_SUCCESS );

            sLnnvlNode[0]->node.arguments = (mtcNode *)sPredicate[0];
            *aResult = sLnnvlNode[0];
        }
        else
        {
            // Operand가 모두 not nullable인 경우 counter operator를 생성한다.
            IDE_TEST( qtc::makeNode( aStatement,
                                     sPredicate,
                                     &sEmptyPosition,
                                     (mtfModule *)toNonQuantifierModule( aPredicate->node.module )->counter )
                      != IDE_SUCCESS );

            sPredicate[0]->node.arguments = (mtcNode *)sOperand1;
            *aResult = sPredicate[0];
        }
    }
    else
    {
        // Subquery predicate이 ANY계열인 경우
        IDE_TEST( qtc::makeNode( aStatement,
                                 sPredicate,
                                 &sEmptyPosition,
                                 toNonQuantifierModule( aPredicate->node.module ) )
                  != IDE_SUCCESS );

        sPredicate[0]->node.arguments = (mtcNode *)sOperand1;
        *aResult = sPredicate[0];
    }

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                *aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::genCorrPredicates( qcStatement  * aStatement,
                                 qtcNode      * aNode,
                                 idBool         aExistsTrans,
                                 qtcNode     ** aResult )
{
/***********************************************************************
 *
 * Description :
 *     Quantified subquery predicate으로부터 correlation predicate을
 *     생성한다.
 *     ex) (t1.i1, t1.i2) IN (SELECT t2.i1, t2.i2 FROM t2 ...)
 *         => t1.i1 = t2.i1 AND t1.i2 = t2.i2
 *
 * Implementation :
 *     1. Predicate의 첫 번째 operator가 list인 경우
 *        List의 각 element들에 대해 correlation predicate을 생성 후
 *        AND로 연결 하여 반환한다.
 *     2. Predicate의 첫 번째 operator가 single value인 경우
 *        Correlation predicate 하나를 생성하여 반환한다.
 *
 ***********************************************************************/

    qtcNode        * sConnectorNode[2];
    qtcNode        * sFirst = NULL;
    qtcNode        * sLast = NULL;
    qtcNode        * sResult;
    qtcNode        * sArg;
    mtfModule      * sConnectorModule;
    UInt             sArgCount = 0;
    qmsTarget      * sSQTarget;
    qcStatement    * sSQStatement;
    qcNamePosition   sEmptyPosition;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::genCorrPredicates::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode      != NULL );
    IDE_DASSERT( aNode->node.arguments != NULL );
    IDE_DASSERT( aNode->node.arguments->next->module == &qtc::subqueryModule );

    sSQStatement = ((qtcNode *)aNode->node.arguments->next)->subquery;
    sSQTarget    = ((qmsParseTree*)sSQStatement->myPlan->parseTree)->querySet->SFWGH->target;

    if( aNode->node.arguments->module == &mtfList )
    {
        // List인 경우
        for( sArg = (qtcNode *)aNode->node.arguments->arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next, sSQTarget = sSQTarget->next )
        {
            IDE_TEST( genCorrPredicate( aStatement,
                                        aNode,
                                        sArg,
                                        sSQTarget->targetColumn,
                                        aExistsTrans,
                                        &sResult )
                      != IDE_SUCCESS );

            // 생성된 predicate을 연결한다.
            if( sFirst == NULL )
            {
                sFirst = sLast = sResult;
            }
            else
            {
                sLast->node.next = (mtcNode *)sResult;
            }

            while( sLast->node.next != NULL )
            {
                sLast = (qtcNode *)sLast->node.next;
            }

            sArgCount++;
        }

        SET_EMPTY_POSITION( sEmptyPosition );

        if( ( aNode->node.module == &mtfNotEqual ) ||
            ( aNode->node.module == &mtfNotEqualAny ) ||
            ( aNode->node.module == &mtfEqualAll ) )
        {
            sConnectorModule = &mtfOr;
        }
        else
        {
            sConnectorModule = &mtfAnd;
        }

        IDE_TEST( qtc::makeNode( aStatement,
                                 sConnectorNode,
                                 &sEmptyPosition,
                                 sConnectorModule )
                  != IDE_SUCCESS );

        sConnectorNode[0]->node.arguments = (mtcNode *)sFirst;

        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sConnectorNode[0] )
                  != IDE_SUCCESS );

        sConnectorNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sConnectorNode[0]->node.lflag |= (aNode->node.arguments->lflag & MTC_NODE_ARGUMENT_COUNT_MASK);

        *aResult = sConnectorNode[0];
    }
    else
    {
        // Single value 경우
        IDE_TEST( genCorrPredicate( aStatement,
                                    aNode,
                                    (qtcNode *)aNode->node.arguments,
                                    sSQTarget->targetColumn,
                                    aExistsTrans,
                                    &sResult )
                  != IDE_SUCCESS );

        *aResult = sResult;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

mtfModule *
qmoUnnesting::toExistsModule( const mtfModule * aQuantifier )
{
/***********************************************************************
 *
 * Description :
 *     Subquery predicate에 사용된 연산자의 종류에 따라 predicate의
 *     변환 후 사용할 연산자를 반환한다.
 *
 * Implementation :
 *     ANY 계열 연산자: EXISTS
 *     ALL 계열 연산자: NOT EXISTS
 *
 ***********************************************************************/

    mtfModule * sResult;

    if( ( ( aQuantifier->lflag & MTC_NODE_GROUP_COMPARISON_MASK ) == MTC_NODE_GROUP_COMPARISON_TRUE ) &&
        ( ( aQuantifier->lflag & MTC_NODE_GROUP_MASK ) == MTC_NODE_GROUP_ALL ) )
    {
        sResult = &mtfNotExists;
    }
    else
    {
        sResult = &mtfExists;
    }

    return sResult;
}

mtfModule * qmoUnnesting::toExistsModule4CountAggr( qcStatement * aStatement,
                                                    qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description : aggr 함수가 count 이면서 group by 를 사용하지 않는 경우
 *               비교하는 값이 0,1 일때만 unnset 가 가능하다.
 *               이 함수에서는 값이 0,1 인지 체크하고
 *               경우마다 사용가능한 exists 모듈을 반환해준다.
 *
 ***********************************************************************/

    qcTemplate  * sTemplate;
    mtcNode     * sNode = (mtcNode*)aNode;
    mtcNode     * sValueNode;
    mtcColumn   * sColumn;
    mtcTuple    * sTuple;
    SLong         sValue  = -1;
    SChar       * sValueTemp;
    mtfModule   * sResult = NULL;

    sTemplate = QC_SHARED_TMPLATE(aStatement);

    // 바인드 변수는 값이 변경될수 있기 때문에 변환해서는 안된다.
    if ( (sNode->arguments->module == &qtc::valueModule) &&
         (sNode->arguments->lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_ABSENT )
    {
        sValueNode = mtf::convertedNode(
            sNode->arguments,
            (mtcTemplate*)sTemplate );

        sTuple     = QTC_TMPL_TUPLE(sTemplate, (qtcNode*)sValueNode);
        sColumn    = QTC_TUPLE_COLUMN( sTuple, (qtcNode*)sValueNode);

        sValueTemp = (SChar*)mtc::value( sColumn,
                                         sTuple->row,
                                         MTD_OFFSET_USE );

        if( sColumn->type.dataTypeId == MTD_BIGINT_ID )
        {
            sValue  = *((SLong*)sValueTemp);
        }
        else if( (sColumn->type.dataTypeId == MTD_NUMERIC_ID) ||
                 (sColumn->type.dataTypeId == MTD_FLOAT_ID) )
        {
            if ( mtv::numeric2NativeN( (mtdNumericType*)sValueTemp,
                                       &sValue) != IDE_SUCCESS )
            {
                sValue  = -1;
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

    switch( sValue )
    {
        case 0:
            if ( (sNode->module == &mtfEqual)    ||
                 (sNode->module == &mtfEqualAny) ||
                 (sNode->module == &mtfEqualAll) )
            {
                sResult = &mtfNotExists;
            }
            else if ( (sNode->module == &mtfGreaterEqual)    ||
                      (sNode->module == &mtfGreaterEqualAny) ||
                      (sNode->module == &mtfGreaterEqualAll) )
            {
                sResult = &mtfNotExists;
            }
            else if ( (sNode->module == &mtfLessThan)    ||
                      (sNode->module == &mtfLessThanAny) ||
                      (sNode->module == &mtfLessThanAll) )
            {
                sResult = &mtfExists;
            }
            else if ( (sNode->module == &mtfNotEqual)    ||
                      (sNode->module == &mtfNotEqualAny) ||
                      (sNode->module == &mtfNotEqualAll) )
            {
                sResult = &mtfExists;
            }
            else
            {
                sResult = NULL;
            }
            break;

        case 1:
            if ( (sNode->module == &mtfLessEqual)    ||
                 (sNode->module == &mtfLessEqualAny) ||
                 (sNode->module == &mtfLessEqualAll) )
            {
                sResult = &mtfExists;
            }
            else if ( (sNode->module == &mtfGreaterThan)    ||
                      (sNode->module == &mtfGreaterThanAny) ||
                      (sNode->module == &mtfGreaterThanAll) )
            {
                sResult = &mtfNotExists;
            }
            else
            {
                sResult = NULL;
            }
            break;

        default :
            sResult = NULL;
            break;
    }

    return sResult;
}

mtfModule *
qmoUnnesting::toNonQuantifierModule( const mtfModule * aQuantifier )
{
/***********************************************************************
 *
 * Description :
 *     Subquery predicate에 사용된 연산자의 종류에 따라
 *     correlation predicate에서 사용할 연산자를 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtfModule * sResult = NULL;

    switch( aQuantifier->lflag & MTC_NODE_OPERATOR_MASK )
    {
        case MTC_NODE_OPERATOR_EQUAL:
            sResult = &mtfEqual;
            break;

        case MTC_NODE_OPERATOR_NOT_EQUAL:
            sResult = &mtfNotEqual;
            break;

        case MTC_NODE_OPERATOR_GREATER:
            sResult = &mtfGreaterThan;
            break;

        case MTC_NODE_OPERATOR_GREATER_EQUAL:
            sResult = &mtfGreaterEqual;
            break;

        case MTC_NODE_OPERATOR_LESS:
            sResult = &mtfLessThan;
            break;

        case MTC_NODE_OPERATOR_LESS_EQUAL:
            sResult = &mtfLessEqual;
            break;

        default:
            IDE_FT_ASSERT( 0 );
    }

    return sResult;
}

idBool
qmoUnnesting::isSingleRowSubquery( qcStatement * aSQStatement )
{
/***********************************************************************
 *
 * Description :
 *     Subquery의 결과가 1개 이하를 보장하는지 확인한다.
 *
 * Implementation :
 *     Subquery가 다음 조건 중 한가지를 만족하면 single row임을 보장할 수 있다.
 *     1. GROUP BY절이 없이 aggregate function을 사용한 경우
 *     2. Nested aggregate function을 사용한 경우
 *     3. WHERE절에 unique key와 equal predicate이 포함된 경우
 *     4. HAVING절에 GROUP BY절의 expression들과 equal predicate이
 *        포함된 경우
 *
 ***********************************************************************/

    qmsParseTree * sParseTree;
    qmsSFWGH     * sSFWGH;
    qcmTableInfo * sTableInfo;
    UInt           i;

    sParseTree = (qmsParseTree *)aSQStatement->myPlan->parseTree;

    if( sParseTree->querySet->setOp == QMS_NONE )
    {
        sSFWGH     = sParseTree->querySet->SFWGH;

        // 1. GROUP BY절이 없이 aggregate function을 사용한 경우
        if( ( sSFWGH->aggsDepth1 != NULL ) &&
            ( sSFWGH->group == NULL ) )
        {
            return ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        // 2. Nested aggregate function을 사용한 경우
        if( sSFWGH->aggsDepth2 != NULL )
        {
            return ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        // 3. WHERE절에 unique key와 equal predicate이 포함된 경우
        // Unique key constraint 조건은 1개 table만 참조시 확인한다.
        if( ( sSFWGH->from->joinType == QMS_NO_JOIN ) &&
            ( sSFWGH->from->next     == NULL ) &&
            ( sSFWGH->where          != NULL ) )
        {
            // View가 아니어야 한다.
            if( sSFWGH->from->tableRef->view == NULL )
            {
                sTableInfo = sSFWGH->from->tableRef->tableInfo;

                // BUG-45168 unique index를 사용해서 subquery를 INNER JOIN으로 변환할 수 있어야 합니다.
                // unique 제약조건 대신에 unique index를 사용해서 체크하도록 변경
                for( i = 0; i < sTableInfo->indexCount; i++ )
                {
                    if ( sTableInfo->indices[i].isUnique == ID_TRUE )
                    {
                        if( containsUniqueKeyPredicate( aSQStatement,
                                                        sSFWGH->where,
                                                        sSFWGH->from->tableRef->table,
                                                        &sTableInfo->indices[i] )
                            == ID_TRUE )
                        {
                            return ID_TRUE;
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
        }
        else
        {
            // Nothing to do.
        }

        // 4. HAVING절에 GROUP BY절의 expression들과 equal predicate이 포함된 경우
        if( ( sSFWGH->group != NULL ) &&
            ( sSFWGH->having != NULL ) )
        {
            if( containsGroupByPredicate( sSFWGH ) == ID_TRUE )
            {
                return ID_TRUE;
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
        // SET 연산 시에는 판단 불가
    }

    return ID_FALSE;
}

idBool
qmoUnnesting::containsUniqueKeyPredicate( qcStatement * aStatement,
                                          qtcNode     * aPredicate,
                                          UInt          aJoinTable,
                                          qcmIndex    * aUniqueIndex )
{
/***********************************************************************
 *
 * Description :
 *     주어진 unique key constraint의 column들과 equal predicate을
 *     포함하는지 확인한다.
 *
 * Implementation :
 *     findUniqueKeyPredicate()를 호출한 후, unique key의 column들이
 *     모두 포함되었는지 확인한다.
 *
 ***********************************************************************/

    UChar sRefVector[QC_MAX_KEY_COLUMN_COUNT];
    UInt  i;

    // Vector를 초기화
    idlOS::memset( sRefVector, 0, ID_SIZEOF(sRefVector) );

    findUniqueKeyPredicate( aStatement, aPredicate, aJoinTable, aUniqueIndex, sRefVector );

    // Unique key constraint의 column들이 모두 equi-join되었는지 확인
    for( i = 0; i < aUniqueIndex->keyColCount; i++ )
    {
        if( sRefVector[i] == 0 )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if( i == aUniqueIndex->keyColCount )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

void
qmoUnnesting::findUniqueKeyPredicate( qcStatement * aStatement,
                                      qtcNode     * aPredicate,
                                      UInt          aJoinTable,
                                      qcmIndex    * aUniqueIndex,
                                      UChar       * aRefVector )
{
/***********************************************************************
 *
 * Description :
 *     Predicate이 unique key constraint의 column들과 equalitiy predicate
 *     포함 여부를 확인하여 vector에 표시한다.
 *
 * Implementation :
 *     AND 연산자 하위에서 = 연산자를 사용하는 경우만 인정한다.
 *
 ***********************************************************************/

    qtcNode   * sArg;
    qcDepInfo   sDepInfo;
    UInt        sUniqueKeyColumn;

    qtc::dependencySet( aJoinTable, &sDepInfo );

    if( qtc::dependencyContains( &aPredicate->depInfo, &sDepInfo ) == ID_TRUE )
    {
        // BUG-44988 SingleRowSubquery 를 잘못 판단하고 있음
        if( aPredicate->node.module == &mtfAnd )
        {
            for( sArg = (qtcNode *)aPredicate->node.arguments;
                 sArg != NULL;
                 sArg = (qtcNode *)sArg->node.next )
            {
                findUniqueKeyPredicate( aStatement,
                                        sArg,
                                        aJoinTable,
                                        aUniqueIndex,
                                        aRefVector );
            }
        }
        else if( aPredicate->node.module == &mtfEqual )
        {
            // Equal만을 확인한다.
            if( (qtc::dependencyEqual( &sDepInfo, &(((qtcNode*)aPredicate->node.arguments)->depInfo) ) == ID_TRUE) &&
                (qtc::dependencyEqual( &sDepInfo, &(((qtcNode*)aPredicate->node.arguments->next)->depInfo) ) == ID_FALSE) )
            {
                // i1 = 1
                sUniqueKeyColumn =
                    findUniqueKeyColumn( aStatement,
                                         (qtcNode *)aPredicate->node.arguments,
                                         aJoinTable,
                                         aUniqueIndex );
                if( sUniqueKeyColumn != ID_UINT_MAX )
                {
                    aRefVector[sUniqueKeyColumn] = 1;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else if ( (qtc::dependencyEqual( &sDepInfo, &(((qtcNode*)aPredicate->node.arguments)->depInfo) ) == ID_FALSE) &&
                      (qtc::dependencyEqual( &sDepInfo, &(((qtcNode*)aPredicate->node.arguments->next)->depInfo) ) == ID_TRUE) )
            {
                // 1 = i1
                sUniqueKeyColumn =
                    findUniqueKeyColumn( aStatement,
                                         (qtcNode *)aPredicate->node.arguments->next,
                                         aJoinTable,
                                         aUniqueIndex );
                if( sUniqueKeyColumn != ID_UINT_MAX )
                {
                    aRefVector[sUniqueKeyColumn] = 1;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // i1 = i2
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
}

UInt
qmoUnnesting::findUniqueKeyColumn( qcStatement * aStatement,
                                   qtcNode     * aNode,
                                   UInt          aTable,
                                   qcmIndex    * aUniqueIndex )
{
/***********************************************************************
 *
 * Description :
 *     Column이 unique key constraint의 어떤 column을 참조하는지 찾는다.
 *
 * Implementation :
 *     Unique key constraint의 column들과 id를 순차로 비교한다.
 *     성공 시 해당 column의 index를, 실패 시 UINT_MAX를 반환한다.
 *
 ***********************************************************************/

    UInt i;

    if( aNode->node.table == aTable )
    {
        for( i = 0; i < aUniqueIndex->keyColCount; i++ )
        {
            // BUG-45168 unique index를 사용해서 subquery를 INNER JOIN으로 변환할 수 있어야 합니다.
            // unique 제약조건 대신에 unique index를 사용해서 체크하도록 변경
            if( QTC_STMT_COLUMN( aStatement, aNode )->column.id
                == aUniqueIndex->keyColumns[i].column.id )
            {
                return i;
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

    return ID_UINT_MAX;
}

idBool
qmoUnnesting::containsGroupByPredicate( qmsSFWGH * aSFWGH )
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY절의 모든 expression들과의 equal predicate이 HAVING절에
 *     포함되어있는지 확인한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsConcatElement * sGroup;
    UChar sRefVector[128];
    UInt  sCount;
    UInt  i;

    for( sGroup = aSFWGH->group, sCount = 0;
         sGroup != NULL;
         sGroup = sGroup->next )
    {
        sCount++;
    }

    if( sCount > ID_SIZEOF( sRefVector ) )
    {
        // Vector 사이즈보다 GROUP BY expression의 개수가 더 많은 경우
        // 확인 불가
    }
    else
    {
        // Vector를 초기화
        idlOS::memset( sRefVector, 0, ID_SIZEOF(sRefVector) );

        findGroupByPredicate( aSFWGH, aSFWGH->group, aSFWGH->having, sRefVector );

        for( i = 0; i < sCount; i++ )
        {
            if( sRefVector[i] == 0 )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if( i == sCount )
        {
            IDE_CONT( APPLICABLE_EXIT );
        }
        else
        {
            // Nothing to do.
        }
    }

    return ID_FALSE;

    IDE_EXCEPTION_CONT( APPLICABLE_EXIT );

    return ID_TRUE;
}

void
qmoUnnesting::findGroupByPredicate( qmsSFWGH         * aSFWGH,
                                    qmsConcatElement * aGroup,
                                    qtcNode          * aPredicate,
                                    UChar            * aRefVector )
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY절의 expression을 참조하는 predicate을 찾아 vector에
 *     marking 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode * sArg;
    UInt      sIndex;

    if( aPredicate->node.module == &mtfAnd )
    {
        for( sArg = (qtcNode *)aPredicate->node.arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next )
        {
            findGroupByPredicate( aSFWGH,
                                  aGroup,
                                  sArg,
                                  aRefVector );
        }
    }
    else if( aPredicate->node.module == &mtfEqual )
    {
        // BUG-44988 SingleRowSubquery 를 잘못 판단하고 있음
        if ( (qtc::haveDependencies( &(((qtcNode*)aPredicate->node.arguments)->depInfo) ) == ID_TRUE ) &&
             (qtc::dependencyContains( &aSFWGH->depInfo, &(((qtcNode*)aPredicate->node.arguments)->depInfo) ) == ID_TRUE) &&
             (qtc::dependencyContains( &aSFWGH->outerDepInfo, &(((qtcNode*)aPredicate->node.arguments->next)->depInfo) ) == ID_TRUE) )
        {
            // i1 = 1
            // i1 = t1.i1 ( outer column )
            sIndex = findGroupByExpression( aGroup, (qtcNode *)aPredicate->node.arguments );

            if( sIndex != ID_UINT_MAX )
            {
                aRefVector[sIndex] = 1;
            }
            else
            {
                // Nothing to do.
            }            
        }
        else if ( (qtc::haveDependencies( &(((qtcNode*)aPredicate->node.arguments->next)->depInfo) ) == ID_TRUE ) &&
                  (qtc::dependencyContains( &aSFWGH->outerDepInfo, &(((qtcNode*)aPredicate->node.arguments)->depInfo) ) == ID_TRUE) &&
                  (qtc::dependencyContains( &aSFWGH->depInfo, &(((qtcNode*)aPredicate->node.arguments->next)->depInfo) ) == ID_TRUE) )
        {
            // 1 = i1 
            // t1.i1(outer column) = i1
            sIndex = findGroupByExpression( aGroup,
                                            (qtcNode *)aPredicate->node.arguments->next );
            if( sIndex != ID_UINT_MAX )
            {
                aRefVector[sIndex] = 1;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // i1 = i2
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }
}

UInt
qmoUnnesting::findGroupByExpression( qmsConcatElement * aGroup,
                                     qtcNode          * aExpression )
{
/***********************************************************************
 *
 * Description :
 *     주어진 expression이 GROUP BY절의 몇 번째 expression인지 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsConcatElement * sGroup;
    UInt sResult = ID_UINT_MAX;
    UInt i;

    if( aExpression->node.module == &qtc::passModule )
    {
        for( sGroup = aGroup, i = 0;
             sGroup != NULL;
             sGroup = sGroup->next, i++ )
        {
            if( sGroup->arithmeticOrList == (qtcNode *)aExpression->node.arguments )
            {
                sResult = i;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // GROUP BY expression이 아닌 경우
    }

    return sResult;
}

idBool
qmoUnnesting::isSimpleSubquery( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     주어진 statement가 simple subquery인지 확인한다.
 *
 * Implementation :
 *     다음의 조건들을 모두 만족해야 한다.
 *     1) FROM절에 하나의 relation만 존재한다.
 *     2) FROM절에 relation이 view가 아니다.
 *     3) Correlation은 1개 table에 대해서만 존재한다.
 *     4) Hierarchy, GROUP BY, LIMIT 또는 HAVING 절을 사용하지 않는다.
 *     5) ROWNUM, LEVEL, ISLEAF, PRIOR column을 사용하지 않는다.
 *
 ***********************************************************************/

    qmsParseTree * sParseTree;
    qmsSFWGH     * sSFWGH;
    ULong          sMask;
    ULong          sCondition;

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
    sSFWGH     = sParseTree->querySet->SFWGH;

    if( sSFWGH == NULL )
    {
        // Set 연산이 포함된 경우
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( sSFWGH->depInfo.depCount != 1 )
    {
        // FROM절에는 하나의 relation만이 존재해야 한다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( sSFWGH->from->tableRef->view != NULL )
    {
        // FROM절에 view가 올 수 없다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( sSFWGH->outerDepInfo.depCount != 1 )
    {
        // 1개의 outer query table만을 참조해야 한다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( ( sSFWGH->hierarchy  != NULL ) ||
        ( sSFWGH->group      != NULL ) ||
        ( sSFWGH->aggsDepth1 != NULL ) ||
        ( sSFWGH->aggsDepth2 != NULL ) ||
        ( sSFWGH->having     != NULL ) ||
        ( sParseTree->limit  != NULL ) ||
        ( sSFWGH->top        != NULL ) ||   /* BUG-36580 supported TOP */        
        ( sSFWGH->rownum     != NULL ) ||
        ( sSFWGH->isLeaf     != NULL ) ||
        ( sSFWGH->level      != NULL ) )
    {
        // START WITH, CONNECT BY, GROUP BY, HAVING, LIMIT clause,
        // ROWNUM, ISLEAF, LEVEL pseudo column, aggregate function을 사용할 수 없다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // EXISTS/NOT EXISTS predicate에 포함된 subquery인 경우
        // SELECT절에는 상수뿐이므로 여기까지 왔다면
        // Aggregate/window function은 존재하지 않는다.
    }

    if( sSFWGH->where != NULL )
    {
        sMask = QTC_NODE_PRIOR_MASK;
        sCondition = QTC_NODE_PRIOR_ABSENT;

        if( ( sSFWGH->where->lflag & sMask ) != sCondition )
        {
            // PRIOR column은 사용할 수 없다.
            IDE_CONT( INVALID_FORM );
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

    return ID_TRUE;

    IDE_EXCEPTION_CONT( INVALID_FORM );

    return ID_FALSE;
}

idBool
qmoUnnesting::isUnnestablePredicate( qcStatement * aStatement,
                                     qmsSFWGH    * aSFWGH,
                                     qtcNode     * aSubqueryPredicate,
                                     qmsSFWGH    * aSQSFWGH )
{
/***********************************************************************
 *
 * Description :
 *     Subquery의 WHERE절과 HAVING절에 포함된 correlation predicate들이
 *     unnesting 가능한 조건의 predicate인지 확인한다.
 *
 * Implementation :
 *     다음 두가지 조건을 만족해야 한다.
 *     1) 항상 conjunctive(AND로 연결)한 형태로 존재해야 한다.
 *     2) WHERE절 또는 HAVING절, 최소한 한 곳에서 outer query의 table과의
 *        join predicate을 포함해야 한다.
 *        이 때 subquery의 predicate이 EXISTS/NOT EXISTS여부에 따라
 *        join predicate으로 인정하는 연산자가 다르다.
 *
 ***********************************************************************/

    idBool sIsConjunctive = ID_TRUE;
    UInt   sJoinPredCount = 0;
    UInt   sCnfCount;
    UInt   sEstimated;

    if( aSQSFWGH->where != NULL )
    {
        existConjunctiveJoin( aSubqueryPredicate,
                              aSQSFWGH->where,
                              &aSQSFWGH->depInfo,
                              &aSQSFWGH->outerDepInfo,
                              &sIsConjunctive,
                              &sJoinPredCount );

        if( sIsConjunctive == ID_FALSE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }

        if( existOuterJoinCorrelation( aSQSFWGH->where,
                                       &aSQSFWGH->outerDepInfo ) == ID_TRUE )
        {
            // Correlation과의 outer join이 포함된 경우
            // ex) SELECT * FROM t1 WHERE EXISTS (SELECT 0 FROM t2 WHERE t1.i1 (+) = t2.i1);
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
        
        // BUG-41564
        // Correlated Predicate에서 Subquery를 Argument로 가지는 경우,
        // Subquery Argument가 현재 Subquery 밖을 참조한다면 Unnesting 불가
        if( existOuterSubQueryArgument( aSQSFWGH->where,
                                        &aSQSFWGH->depInfo ) == ID_TRUE )
        {
            IDE_CONT( INVALID_FORM );
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

    if( aSQSFWGH->having != NULL )
    {
        existConjunctiveJoin( aSubqueryPredicate,
                              aSQSFWGH->having,
                              &aSQSFWGH->depInfo,
                              &aSQSFWGH->outerDepInfo,
                              &sIsConjunctive,
                              &sJoinPredCount );

        if( existOuterJoinCorrelation( aSQSFWGH->having,
                                       &aSQSFWGH->outerDepInfo ) == ID_TRUE )
        {
            // Correlation과의 outer join이 포함된 경우
            // ex) SELECT * FROM t1 WHERE EXISTS (SELECT 0 FROM t2 WHERE t1.i1 (+) = t2.i1);
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
        
        // BUG-41564
        // Correlated Predicate에서 Subquery를 Argument로 가지는 경우,
        // Subquery Argument가 현재 Subquery 밖을 참조한다면 Unnesting 불가
        if( existOuterSubQueryArgument( aSQSFWGH->having,
                                        &aSQSFWGH->depInfo ) == ID_TRUE )
        {
            IDE_CONT( INVALID_FORM );
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

    if( ( sIsConjunctive == ID_FALSE ) ||
        ( sJoinPredCount == 0 ) )
    {
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( aSubqueryPredicate->node.module == &mtfNotExists )
    {
        // NOT EXISTS인 경우 subquery 내 table을 참조하지 않는 predicate을 포함하는 경우
        // (constant predicate 또는 outer table의 column으로만 구성된 predicate)
        // anti join으로 transform할 수 없다.
        if( aSQSFWGH->where != NULL )
        {
            if( isAntiJoinablePredicate( aSQSFWGH->where, &aSQSFWGH->depInfo ) == ID_FALSE )
            {
                IDE_CONT( INVALID_FORM );
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

        if( aSQSFWGH->having != NULL )
        {
            if( isAntiJoinablePredicate( aSQSFWGH->having, &aSQSFWGH->depInfo ) == ID_FALSE )
            {
                IDE_CONT( INVALID_FORM );
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

    // BUG-38827
    // Subquery block 내에 view target(inline view 의 target 절) 대상이
    // 없는 경우는 unnest 하지 않는다.
    if ( ( existViewTarget( aSQSFWGH->where,
                            &aSQSFWGH->depInfo ) != ID_TRUE ) &&
         ( existViewTarget( aSQSFWGH->having,
                            &aSQSFWGH->depInfo ) != ID_TRUE ) )
    {
        // Inline view 의 target 절이 없다면 unnest 해선 안된다.
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // Nothing to do.
    }

    if( aSFWGH != NULL )
    {
        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_NORMAL_FORM_MAXIMUM );

        (void)qmoNormalForm::estimateCNF( aSFWGH->where,
                                          &sCnfCount );

        // sEstimated = WHERE절의 CNF 변환 시 예측 predicate 수 + subquery의 correlation predicate의 수 - 1
        // * EXISTS/NOT EXISTS의 위치에 correlation predicate이 설정되므로 1개를 제외한다.
        sEstimated = sCnfCount + sJoinPredCount - 1;

        if( sEstimated > QCG_GET_SESSION_NORMALFORM_MAXIMUM( aStatement ) )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // SELECT구문이 아니거나 WHERE절 외 ON 절 등인 경우
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( INVALID_FORM );

    return ID_FALSE;
}

idBool
qmoUnnesting::existOuterJoinCorrelation( qtcNode   * aNode,
                                         qcDepInfo * aOuterDepInfo )
{
/***********************************************************************
 *
 * Description :
 *     Subquery에서 correlation과의 outer join이 포함되어있는지 확인한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode   * sArg;
    qcDepInfo   sDepInfo;

    qtc::dependencyAnd( &aNode->depInfo,
                        aOuterDepInfo,
                        &sDepInfo );

    if( ( qtc::haveDependencies( &sDepInfo ) == ID_TRUE ) &&
        ( aNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK ) == QTC_NODE_JOIN_OPERATOR_EXIST )
    {
        if( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE ) 
        {
            if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_AND )
            {
                for( sArg = (qtcNode *)aNode->node.arguments;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next )
                {
                    if( existOuterJoinCorrelation( sArg, aOuterDepInfo ) == ID_TRUE )
                    {
                        IDE_CONT( APPLICABLE_EXIT );
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
        }
        else
        {
            // 논리 연산자가 아니면서 outer join operator와 correlation을 포함하는 경우
            IDE_CONT( APPLICABLE_EXIT );
        }
    }
    else
    {
        // Nothing to do.
    }

    return ID_FALSE;

    IDE_EXCEPTION_CONT( APPLICABLE_EXIT );

    return ID_TRUE;
}

idBool
qmoUnnesting::isAntiJoinablePredicate( qtcNode   * aNode,
                                       qcDepInfo * aDepInfo )
{
/***********************************************************************
 *
 * Description :
 *     Anti join이 가능한지 subquery에 포함된 predicate을 확인한다.
 *     Subquery에 포함된 predicate 중 subquery의 column들을 참조하지
 *     않는 predicate이 포함된 경우 anti join이 불가능하다.
 *     ex) Constant predicate, correlation으로만 구성된 predicate 등
 *
 * Implementation :
 *     aNode의 depInfo가 aDepInfo와 겹치지 않으면 불가능으로 판단한다.
 *     이 때 aDepInfo는 subquery의 depInfo를 설정해야 한다.
 *
 ***********************************************************************/

    qtcNode   * sArg;
    qcDepInfo   sDepInfo;

    qtc::dependencyAnd( &aNode->depInfo,
                        aDepInfo,
                        &sDepInfo );

    if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
    {
        if( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE ) 
        {
            if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_AND )
            {
                for( sArg = (qtcNode *)aNode->node.arguments;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next )
                {
                    if( isAntiJoinablePredicate( sArg, aDepInfo ) == ID_FALSE )
                    {
                        IDE_CONT( NOT_APPLICABLE_EXIT );
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
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_CONT( NOT_APPLICABLE_EXIT );
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( NOT_APPLICABLE_EXIT );

    return ID_FALSE;
}

void
qmoUnnesting::existConjunctiveJoin( qtcNode   * aSubqueryPredicate,
                                    qtcNode   * aNode,
                                    qcDepInfo * aInnerDepInfo,
                                    qcDepInfo * aOuterDepInfo,
                                    idBool    * aIsConjunctive,
                                    UInt      * aJoinPredCount )
{
/***********************************************************************
 *
 * Description :
 *     주어진 predicate에 포함된 correlation predicate들이 conjunctive한지,
 *     그리고 subquery의 table과 join 조건을 하나 이상 포함하는지 확인한다.
 *     aIsConjunctive는 반드시 ID_FALSE, aContainsJoin은 ID_TRUE로
 *     초기값을 설정한 후 이 함수를 호출해야 한다.
 *
 * Implementation :
 *     Correlation predicate을 포함하는 논리 연산자가 AND만 존재하면
 *     true를, 그 외 다른 논리 연산자가 correlation predicate을
 *     포함하면 false를 반환한다.
 *
 ***********************************************************************/

    qtcNode   * sArg;
    qtcNode   * sFirstArg;
    qtcNode   * sSecondArg;
    qcDepInfo   sDepInfo;
    idBool      sJoinableOperator;

    qtc::dependencyAnd( &aNode->depInfo,
                        aOuterDepInfo,
                        &sDepInfo );

    if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
    {
        if( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE ) 
        {
            if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_AND )
            {
                for( sArg = (qtcNode *)aNode->node.arguments;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next )
                {
                    existConjunctiveJoin( aSubqueryPredicate,
                                          sArg,
                                          aInnerDepInfo,
                                          aOuterDepInfo,
                                          aIsConjunctive,
                                          aJoinPredCount );
                    if( *aIsConjunctive == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                // OR, NOT 등이 포함될 수 없다.
                *aIsConjunctive = ID_FALSE;
            }
        }
        else
        {
            IDE_FT_ASSERT( ( aNode->node.lflag & MTC_NODE_COMPARISON_MASK ) == MTC_NODE_COMPARISON_TRUE );

            // Quantifier 연산자, EXISTS/NOT EXISTS, BETWEEN, not equal(<>)은 지원하지 않는다.
            if( ( ( aNode->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK ) == MTC_NODE_GROUP_COMPARISON_TRUE ) ||
                ( aNode->node.module == &mtfExists ) ||
                ( aNode->node.module == &mtfNotExists ) )
            {
                // Nothing to do.
                *aIsConjunctive = ID_FALSE;
            }
            else
            {
                // Quantifier 연산자, BETWEEN 등은 비교 연산자로 변환한다면 가능하다.
                switch( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                {
                    case MTC_NODE_OPERATOR_EQUAL:
                    case MTC_NODE_OPERATOR_GREATER:
                    case MTC_NODE_OPERATOR_GREATER_EQUAL:
                    case MTC_NODE_OPERATOR_LESS:
                    case MTC_NODE_OPERATOR_LESS_EQUAL:
                        sJoinableOperator = ID_TRUE;
                        break;
                    case MTC_NODE_OPERATOR_NOT_EQUAL:
                        if( aSubqueryPredicate->node.module == &mtfNotExists )
                        {
                            sJoinableOperator = ID_FALSE;
                        }
                        else
                        {
                            sJoinableOperator = ID_TRUE;
                        }
                        break;
                    default:
                        sJoinableOperator = ID_FALSE;
                        break;
                }

                if( sJoinableOperator == ID_TRUE )
                {
                    sFirstArg = (qtcNode *)aNode->node.arguments;
                    sSecondArg = (qtcNode *)sFirstArg->node.next;

                    if( ( sFirstArg->depInfo.depCount  == 1 ) &&
                        ( sSecondArg->depInfo.depCount == 1 ) )
                    {
                        // 두 argument가 각각 inner query와 outer query의 dependency를 갖는지 확인한다.
                        if( ( ( qtc::dependencyContains( aOuterDepInfo, &sFirstArg->depInfo ) == ID_TRUE ) &&
                              ( qtc::dependencyContains( aInnerDepInfo, &sSecondArg->depInfo ) == ID_TRUE ) ) ||
                            ( ( qtc::dependencyContains( aInnerDepInfo, &sFirstArg->depInfo ) == ID_TRUE ) &&
                              ( qtc::dependencyContains( aOuterDepInfo, &sSecondArg->depInfo ) == ID_TRUE ) ) )
                        {
                            (*aJoinPredCount)++;
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
            }
        }
    }
    else
    {
        // Nothing to do.
    }
}

IDE_RC
qmoUnnesting::transformSubqueryToView( qcStatement  * aStatement,
                                       qtcNode      * aSubquery,
                                       qmsFrom     ** aView )
{
/***********************************************************************
 *
 * Description :
 *     Subquery를 view로 변환하여 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSFWGH     * sSFWGH;
    qmsFrom      * sFrom;
    qmsTableRef  * sTableRef;
    qcStatement  * sSQStatement;
    qmsParseTree * sSQParseTree;
    qmsOuterNode * sOuterNode;
    qmsOuterNode * sPrevOuterNode = NULL;
    mtcTuple     * sMtcTuple;
    UInt           i;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::transformSubqueryToView::__FT__" );

    sSFWGH = ((qmsParseTree *)aStatement->myPlan->parseTree)->querySet->SFWGH;
    sSQStatement = aSubquery->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsFrom ),
                                               (void **)&sFrom )
              != IDE_SUCCESS );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsTableRef ),
                                               (void **)&sTableRef )
              != IDE_SUCCESS );

    QCP_SET_INIT_QMS_FROM( sFrom );
    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

    sTableRef->view = sSQStatement;
    sFrom->tableRef = sTableRef;

    // Unique한 view name 지정
    IDE_TEST( genUniqueViewName( aStatement, (UChar **)&sTableRef->aliasName.stmtText )
              != IDE_SUCCESS );
    sTableRef->aliasName.offset = 0;
    sTableRef->aliasName.size   = idlOS::strlen( sTableRef->aliasName.stmtText );

    sSQParseTree->querySet->SFWGH->selectType = QMS_ALL;

    // View에는 outer column이 없으므로 정보 제거
    qtc::dependencyClear( &sSQParseTree->querySet->outerDepInfo );
    qtc::dependencyClear( &sSQParseTree->querySet->SFWGH->outerDepInfo );

    // PROJ-2418 Unnesting 된 상황에서는 Lateral View가 없으므로
    // lateralDepInfo 정보도 같이 제거한다.
    qtc::dependencyClear( &sSQParseTree->querySet->lateralDepInfo );

    for( sOuterNode = sSQParseTree->querySet->SFWGH->outerColumns;
         sOuterNode != NULL;
         sOuterNode = sOuterNode->next )
    {
        if( sPrevOuterNode != NULL )
        {
            IDE_TEST( QC_QMP_MEM( aStatement )->free( sPrevOuterNode )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        sPrevOuterNode = sOuterNode;
    }

    if( sPrevOuterNode != NULL )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->free( sPrevOuterNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    sSQParseTree->querySet->SFWGH->outerColumns = NULL;

    // 생성된 inline view를 validation한다.
    IDE_TEST( qmvQuerySet::validateInlineView( aStatement,
                                               sSFWGH,
                                               sTableRef,
                                               MTC_COLUMN_NOTNULL_TRUE )
              != IDE_SUCCESS );

    sMtcTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sTableRef->table]);

    // BUG-43708 unnest 이후 만들어진 view에 대해서
    // MTC_COLUMN_USE_COLUMN_TRUE 를 설정해야함
    for( i = 0;
         i < sMtcTuple->columnCount;
         i++ )
    {
        sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
        sMtcTuple->columns[i].flag |=  MTC_COLUMN_USE_COLUMN_TRUE;
    }

    // View의 column name들을 qmsTableRef에 복사한다.
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( (QC_MAX_OBJECT_NAME_LEN + 1) *
                                             (sTableRef->tableInfo->columnCount),
                                             (void**)&( sTableRef->columnsName ) )
              != IDE_SUCCESS );

    for( i = 0;
         i < sMtcTuple->columnCount;
         i++ )
    {
        idlOS::memcpy( sTableRef->columnsName[i],
                       sTableRef->tableInfo->columns[i].name,
                       (QC_MAX_OBJECT_NAME_LEN + 1) );
    }

    // Table map에 view를 등록한다.
    QC_SHARED_TMPLATE( aStatement )->tableMap[sTableRef->table].from = sFrom;

    // Dependency 설정
    qtc::dependencyClear( &sFrom->depInfo );
    qtc::dependencySet( sTableRef->table, &sFrom->depInfo );

    qtc::dependencyClear( &sFrom->semiAntiJoinDepInfo );

    *aView          = sFrom;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::genUniqueViewName( qcStatement * aStatement, UChar ** aViewName )
{
/***********************************************************************
 *
 * Description :
 *     Unique한 view의 이름을 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar   * sViewName;
    UInt      sIdx = 0;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::genUniqueViewName::__FT__" );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( VIEW_NAME_LENGTH, (void**)&sViewName )
              != IDE_SUCCESS );

    sIdx = ++QC_SHARED_TMPLATE( aStatement )->mUnnestViewNameIdx;

    IDE_TEST_RAISE ( sIdx > 99999, ERR_INDEX );

    /* BUG-48052
     * PREFIX(5)  $VIEW  + Number(5) - max (99999)
     * = 10 + 1 = 11
     */
    idlOS::snprintf( (char*)sViewName, VIEW_NAME_LENGTH, VIEW_NAME_PREFIX"%"ID_UINT32_FMT, sIdx );

    *aViewName = sViewName;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INDEX )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoUnnesting::genUniqueViewName",
                                  "Invalid View Name" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::removeCorrPredicate( qcStatement  * aStatement,
                                   qtcNode     ** aPredicate,
                                   qcDepInfo    * aOuterDepInfo,
                                   qtcNode     ** aRemoved )
{
/***********************************************************************
 *
 * Description :
 *     주어진 predicate에서 correlation predicate만을 제거한다.
 *
 * Implementation :
 *     재귀적으로 탐색하면서 논리 연산자가 아닌 경우 떼어내어 모아서
 *     반환한다.
 *     Correlation predicate을 제거하기 위해서, parent node를 별도의
 *     인자로 넘겨받는 대신 double pointer를 사용하도록 구현한다.
 *
 ***********************************************************************/

    qtcNode    * sConcatPred;
    qtcNode    * sNode;
    qtcNode   ** sDoublePointer;
    qtcNode    * sOld;
    qcDepInfo    sDepInfo;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::removeCorrPredicate::__FT__" );

    sNode = *aPredicate;

    qtc::dependencyAnd( &sNode->depInfo,
                        aOuterDepInfo,
                        &sDepInfo );

    if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
    {
        if( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            IDE_FT_ERROR( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                          == MTC_NODE_OPERATOR_AND );

            sDoublePointer = (qtcNode **)&sNode->node.arguments;

            while( *sDoublePointer != NULL )
            {
                sOld = *sDoublePointer;

                IDE_TEST( removeCorrPredicate( aStatement,
                                               sDoublePointer,
                                               aOuterDepInfo,
                                               aRemoved )
                          != IDE_SUCCESS );
                if( sOld == *sDoublePointer )
                {
                    sDoublePointer = (qtcNode **)&(*sDoublePointer)->node.next;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( sNode->node.arguments == NULL )
            {
                // AND의 operand가 모두 제거된 경우
                *aPredicate = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            *aPredicate = (qtcNode *)sNode->node.next;
            sNode->node.next = NULL;

            // 논리 연산자가 아니면 predicate이므로 제거
            IDE_TEST( concatPredicate( aStatement,
                                       *aRemoved,
                                       sNode,
                                       &sConcatPred )
                      != IDE_SUCCESS );
            *aRemoved = sConcatPred;
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
qmoUnnesting::genViewSelect( qcStatement * aStatement,
                             qtcNode     * aNode,
                             idBool        aIsOuterExpr )
{
/***********************************************************************
 *
 * Description :
 *     Subquery로부터 제거된 correlation predicate으로부터 VIEW의
 *     SELECT list를 생성한다.
 *
 * Implementation :
 *     Predicate중 subquery의 column들을 찾아 append한다.
 *
 ***********************************************************************/

    qmsParseTree * sParseTree;
    qmsQuerySet  * sQuerySet;
    qtcNode      * sArg;
    qmsQuerySet  * sTempQuerySet;
    qcDepInfo      sDepInfo;
    qtcOverColumn* sOverColumn;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::genViewSelect::__FT__" );

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
    sQuerySet  = sParseTree->querySet;

    // BUG-36803 aNode may be null
    if ( aNode != NULL )
    {
        qtc::dependencyAnd( &sQuerySet->depInfo, &aNode->depInfo, &sDepInfo );

        // BUG-45279 디펜던시가 없는 aggr 함수도 target에 넣어야 한다.
        if( (qtc::haveDependencies( &sDepInfo ) == ID_TRUE) ||
            (QTC_HAVE_AGGREGATE( aNode ) == ID_TRUE) )
        {
            // Column이나 pass node를 찾을 때까지 재귀적으로 순회한다.
            // BUG-42113 LOB type 에 대한 subquery 변환이 수행되어야 합니다.
            // mtfGetBlobLocator, mtfGetClobLocator은 pass node처럼 취급해야 한다.
            if( ( QTC_IS_COLUMN( aStatement, aNode ) == ID_TRUE ) || 
                ( aNode->node.module == &qtc::passModule ) ||
                ( aNode->node.module == &mtfGetBlobLocator ) ||
                ( aNode->node.module == &mtfGetClobLocator ) ||
                ( ( ( aIsOuterExpr == ID_FALSE ) && 
                    ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                      == MTC_NODE_OPERATOR_AGGREGATION ) ) ) )
            {
                IDE_TEST( appendViewSelect( aStatement,
                                            aNode )
                          != IDE_SUCCESS );
            }
            else if( aNode->node.module == &qtc::subqueryModule )
            {
                // BUG-45226 서브쿼리의 target 에 서브쿼리가 있을때 오류가 발생합니다.
                sTempQuerySet = ((qmsParseTree*)(aNode->subquery->myPlan->parseTree))->querySet;

                IDE_TEST( genViewSetOp( aStatement,
                                        sTempQuerySet,
                                        aIsOuterExpr )
                          != IDE_SUCCESS );
            }
            else
            {
                for ( sArg = (qtcNode *)aNode->node.arguments;
                      sArg != NULL;
                      sArg = (qtcNode *)sArg->node.next )
                {
                    IDE_TEST( genViewSelect( aStatement,
                                             sArg,
                                             aIsOuterExpr )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            // Nothing to do.
        }

        /* BUG-40914 */
        if (aNode->overClause != NULL)
        {
            for (sOverColumn = aNode->overClause->overColumn;
                 sOverColumn != NULL;
                 sOverColumn = sOverColumn->next)
            {
                IDE_TEST(genViewSelect(aStatement,
                                       sOverColumn->node,
                                       aIsOuterExpr)
                         != IDE_SUCCESS);
            }
        }
        else
        {
            /* nothing to do */
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

IDE_RC qmoUnnesting::genViewSetOp( qcStatement * aStatement,
                                   qmsQuerySet * aSetQuerySet,
                                   idBool        aIsOuterExpr )
{
/***********************************************************************
 *
 * Description :
 *     BUG-45226
 *     서브쿼리의 outerColumns 를 찾아서 genViewSelect 호출한다.
 *
 ***********************************************************************/
    qmsOuterNode * sOuter;

    if ( aSetQuerySet->setOp == QMS_NONE )
    {
        for ( sOuter = aSetQuerySet->SFWGH->outerColumns;
              sOuter != NULL;
              sOuter = sOuter->next )
        {
            IDE_TEST( genViewSelect( aStatement,
                                     sOuter->column,
                                     aIsOuterExpr)
                      != IDE_SUCCESS);
        }
    }
    else
    {
        IDE_TEST( genViewSetOp( aStatement,
                                aSetQuerySet->left,
                                aIsOuterExpr )
                  != IDE_SUCCESS );

        IDE_TEST( genViewSetOp( aStatement,
                                aSetQuerySet->right,
                                aIsOuterExpr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoUnnesting::appendViewSelect( qcStatement * aStatement,
                                       qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     View의 SELECT절에 column을 append한다.
 *     만약 이미 SELECT절에 존재하는 column인 경우 무시한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsConcatElement * sGroup;
    qmsParseTree     * sParseTree;
    qmsQuerySet      * sQuerySet;
    qmsTarget        * sTarget;
    qmsTarget        * sLastTarget = NULL;
    qtcNode          * sNode;
    qtcNode          * sArgNode;  /* BUG-39287 */
    qtcNode          * sPassNode; /* BUG-39287 */
    SChar            * sColumnName;
    UInt               sIdx = 0;
    idBool             sIsEquivalent = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::appendViewSelect::__FT__" );

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
    sQuerySet  = sParseTree->querySet;

    for( sTarget = sQuerySet->target;
         sTarget != NULL;
         sTarget = sTarget->next, sIdx++ )
    {
        sLastTarget = sTarget;

        sNode = sTarget->targetColumn;
        while( sNode->node.module == &qtc::passModule )
        {
            sNode = (qtcNode *)sNode->node.arguments;
        }

        if( ( sNode->node.table  == aNode->node.table ) &&
            ( sNode->node.column == aNode->node.column ) )
        {
            // 동일한 column이 이미 존재하는 경우
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if( sTarget == NULL )
    {
        // 새로 추가해야 하는 경우

        // Unique한 column 이름을 설정해준다.
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( COLUMN_NAME_LENGTH,
                                                   (void**)&sColumnName )
                  != IDE_SUCCESS );

        idlOS::snprintf( (char*)sColumnName,
                         COLUMN_NAME_LENGTH,
                         COLUMN_NAME_PREFIX"%"ID_UINT32_FMT,
                         sIdx + 1 );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsTarget ),
                                                   (void**)&sTarget )
                  != IDE_SUCCESS );

        QMS_TARGET_INIT( sTarget );

        /*
         * BUG-39287
         * 원본 node 를 옮기지 않고 node 를 새로 생성하여 복사한다.
         * pass node 일 경우에는 pass node 의 argument 도 새로 생성한다.
         */
        IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qtcNode),
                                               (void**)&sNode)
                 != IDE_SUCCESS);

        idlOS::memcpy(sNode, aNode, ID_SIZEOF(qtcNode));
        sNode->node.next = NULL;

        sTarget->aliasColumnName.name = sColumnName;
        sTarget->aliasColumnName.size = idlOS::strlen(sColumnName);
        sNode->node.conversion        = NULL;
        sNode->node.leftConversion    = NULL;
        sTarget->targetColumn         = sNode;

        if (aNode->node.module == &qtc::passModule)
        {
            IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qtcNode),
                                                   (void**)&sArgNode)
                     != IDE_SUCCESS);

            idlOS::memcpy(sArgNode, aNode->node.arguments, ID_SIZEOF(qtcNode));
            sNode->node.arguments = (mtcNode*)sArgNode;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST(qtc::estimateNodeWithArgument(aStatement,
                                               sNode)
                 != IDE_SUCCESS);

        // Aggregate function도 아니며 pass node도 아닌 경우
        // (이미 GROUP BY절의 column이었던 경우에는 pass node가 설정된다.)
        if( ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK ) != MTC_NODE_OPERATOR_AGGREGATION ) &&
            ( aNode->node.module != &qtc::passModule ) )
        {
            // GROUP BY절 처리
            for( sGroup = sQuerySet->SFWGH->group;
                 sGroup != NULL;
                 sGroup = sGroup->next )
            {
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       aNode,
                                                       sGroup->arithmeticOrList,
                                                       &sIsEquivalent )
                          != IDE_SUCCESS );

                if( sIsEquivalent == ID_TRUE )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( ( sQuerySet->SFWGH->aggsDepth1 != NULL ) ||
                ( sQuerySet->SFWGH->group != NULL ) )
            {
                if( sIsEquivalent == ID_FALSE )
                {
                    // GROUP BY절에 새 expression 추가
                    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmsConcatElement ),
                                                               (void**)&sGroup )
                              != IDE_SUCCESS );

                    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qtcNode ),
                                                               (void**)&sGroup->arithmeticOrList )
                              != IDE_SUCCESS );

                    idlOS::memcpy( sGroup->arithmeticOrList, aNode, ID_SIZEOF( qtcNode ) );

                    // BUG-41018 taget 컬럼에 컨버젼을 제거하고 추가했으므로
                    // group by 에도 컨버젼을 제거한후 추가해야 한다.
                    sGroup->arithmeticOrList->node.conversion        = NULL;
                    sGroup->arithmeticOrList->node.leftConversion    = NULL;

                    // BUG-38011
                    // target 절과 마찬가지로 dependency 가 일치하는 노드만 가져온다.
                    sGroup->arithmeticOrList->node.next = NULL;

                    sGroup->type = QMS_GROUPBY_NORMAL;
                    sGroup->next = sQuerySet->SFWGH->group;

                    /* TASK-7219 Shard Transformer Refactoring */
                    sGroup->arguments = NULL;
                    sQuerySet->SFWGH->group = sGroup;
                }
                else
                {
                    // Nothing to do.
                }

                // GROUP BY절 expression을 가리키도록 pass node 생성
                IDE_TEST( qtc::makePassNode( aStatement,
                                             sNode,
                                             sGroup->arithmeticOrList,
                                             &sPassNode )
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

        if( sLastTarget == NULL )
        {
            sQuerySet->target = sTarget;
            sQuerySet->SFWGH->target = sTarget;
        }
        else
        {
            sLastTarget->next = sTarget;
        }
    }
    else
    {
        // 동일한 column이 이미 존재하므로 추가하지 않는다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoUnnesting::toViewColumns( qcStatement  * aViewStatement,
                                    qmsTableRef  * aViewTableRef,
                                    qtcNode     ** aNode,
                                    idBool         aIsOuterExpr )
{
/***********************************************************************
 *
 * Description :
 *     Subquery 내 table을 참조하는 column을 view를 참조하는 table로
 *     변경한다.
 *     Subquery를 view로 변경할때 subquery에 포함된 correlation predicate을
 *     outer query로 옮겨 view와의 join predicate으로 변환하기 위해 사용한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree * sParseTree;
    qmsQuerySet  * sQuerySet;
    qtcNode      * sNode[2];
    qtcNode     ** sDoublePointer;
    qmsTarget    * sTarget;
    qmsQuerySet  * sTempQuerySet;
    qcDepInfo      sDepInfo;
    qcDepInfo      sViewDefInfo;
    UInt           sIdx = 0;
    idBool         sFind = ID_FALSE;
    mtcNode      * sNextNode;           /* BUG-39287 */
    mtcNode      * sConversionNode;     /* BUG-39287 */
    mtcNode      * sLeftConversionNode; /* BUG-39287 */
    qtcOverColumn* sOverColumn;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::toViewColumns::__FT__" );

    IDE_DASSERT( aNode != NULL );

    sParseTree = (qmsParseTree *)aViewStatement->myPlan->parseTree;
    sQuerySet  = sParseTree->querySet;

    qtc::dependencySet( aViewTableRef->table, &sViewDefInfo );
    qtc::dependencyAnd( &sQuerySet->depInfo, &(*aNode)->depInfo, &sDepInfo );

    // BUG-45279 디펜던시가 없는 aggr 함수도 target에 넣어야 한다.
    // view의 target 에 있으므로 변환도 가능해야 한다.
    if( (qtc::haveDependencies( &sDepInfo ) == ID_TRUE) ||
        (QTC_HAVE_AGGREGATE( *aNode ) == ID_TRUE) )
    {
        // BUG-42113 LOB type 에 대한 subquery 변환이 수행되어야 합니다.
        // mtfGetBlobLocator, mtfGetClobLocator은 pass node처럼 취급해야 한다.
        if( ( QTC_IS_COLUMN( aViewStatement, *aNode ) == ID_TRUE ) ||
            ( (*aNode)->node.module == &qtc::passModule ) ||
            ( (*aNode)->node.module == &mtfGetBlobLocator ) ||
            ( (*aNode)->node.module == &mtfGetClobLocator ) ||
            ( ( ( aIsOuterExpr == ID_FALSE  ) &&
                ( (*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK) == MTC_NODE_OPERATOR_AGGREGATION ) ) )
        {
            // Column, 또는 pass node인 경우
            for( sTarget = sQuerySet->target;
                 sTarget != NULL;
                 sTarget = sTarget->next )
            {
                // View의 SELECT절에서 predicate의 column을 찾아 view를 참조하도록 변경한다.
                // ex) SELECT ... FROM t2, (SELECT t1.c1 COL1, t1.c2 COL2 ... FROM t1) view1
                //     aNode가 "t1.c1 = t2.c1" 이었을 때 "view1.col1 = t2.c1" 로 변경한다.

                // BUG-38228
                // group by 가 있을때는 target 에 pass node가 있을수 있다.
                // group by 에 있는 컬럼이 aNode 로 들어올때 다음과 같은 상황이 발생한다.
                // select i1 from t1 where i4 in ( select i4 from t1 ) group by i1;
                //        1번 상황                                               2번 상황
                // 1. aNode 가 외부 질의의 taget일때는 pass node가 있다.
                //    이때는 1번째 if 문에서 처리가 된다.
                // 2. aNode 가 외부 질의의 group by 절일때는 pass node가 없다.
                //    이때는 2번째 if 문에서 처리가 된다.
                if( ( sTarget->targetColumn->node.table  == (*aNode)->node.table ) &&
                    ( sTarget->targetColumn->node.column == (*aNode)->node.column ) )
                {
                    sFind = ID_TRUE;
                }
                // mtfGetBlobLocator, mtfGetClobLocator은 pass node처럼 취급해야 한다.
                else if ( (sTarget->targetColumn->node.module == &qtc::passModule)   ||
                          (sTarget->targetColumn->node.module == &mtfGetBlobLocator) ||
                          (sTarget->targetColumn->node.module == &mtfGetClobLocator) )
                {
                    if( ( sTarget->targetColumn->node.arguments->table  == (*aNode)->node.table ) &&
                        ( sTarget->targetColumn->node.arguments->column == (*aNode)->node.column ) )
                    {
                        sFind = ID_TRUE;
                    }
                    else
                    {
                        sFind = ID_FALSE;
                    }
                }
                else
                {
                    sFind = ID_FALSE;
                }

                if ( sFind == ID_TRUE )
                {
                    /*
                     * BUG-39287
                     * appendViewSelect 과정에서 view target 을 복사해서 만들었으므로
                     * 여기서는 원본 node 를 view target 으로 가리키도록 변경한다.
                     * 단, target 이 aggr node 인 경우 새로 생성한다.
                     * 그렇지 않으면 qmsSFWGH->aggsDepth1 이 잘못된 node 를 가리키게 된다.
                     */
                    if (((*aNode)->node.lflag & MTC_NODE_OPERATOR_MASK) ==
                        MTC_NODE_OPERATOR_AGGREGATION)
                    {
                        /* aggr node 인 경우 새로운 node 생성
                         * subquery unnesting 인 경우만 해당됨 */

                        IDE_TEST( qtc::makeColumn( aViewStatement,
                                                   sNode,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   NULL )
                                  != IDE_SUCCESS );

                        sNode[0]->node.module         = &qtc::columnModule;
                        sNode[0]->node.table          = aViewTableRef->table;
                        sNode[0]->node.column         = sIdx;
                        sNode[0]->node.baseTable      = sNode[0]->node.table;
                        sNode[0]->node.baseColumn     = sNode[0]->node.column;
                        sNode[0]->node.next           = (*aNode)->node.next;
                        sNode[0]->node.conversion     = (*aNode)->node.conversion;
                        sNode[0]->node.leftConversion = (*aNode)->node.leftConversion;

                        (*aNode)->node.next = NULL;

                        SET_POSITION( sNode[0]->tableName, aViewTableRef->aliasName );
                        sNode[0]->columnName.stmtText = sTarget->aliasColumnName.name;
                        sNode[0]->columnName.size     = sTarget->aliasColumnName.size;
                        sNode[0]->columnName.offset   = 0;

                        IDE_TEST( qtc::estimateNodeWithoutArgument( aViewStatement,
                                                                    sNode[0] )
                                  != IDE_SUCCESS );

                        IDE_TEST( qmvQTC::addViewColumnRefList( aViewStatement,
                                                                sNode[0] )
                                  != IDE_SUCCESS );

                        *aNode = sNode[0];
                    }
                    else
                    {
                        /* aggr node 아닌경우 원본 node 의 정보 변경 */

                        sNextNode           = aNode[0]->node.next;
                        sConversionNode     = aNode[0]->node.conversion;
                        sLeftConversionNode = aNode[0]->node.leftConversion;

                        QTC_NODE_INIT(aNode[0]);

                        aNode[0]->node.module         = &qtc::columnModule;
                        aNode[0]->node.table          = aViewTableRef->table;
                        aNode[0]->node.column         = sIdx;
                        aNode[0]->node.baseTable      = aNode[0]->node.table;
                        aNode[0]->node.baseColumn     = aNode[0]->node.column;
                        aNode[0]->node.next           = sNextNode;

                        // BUG-42113 LOB type 에 대한 subquery 변환이 수행되어야 합니다.
                        // unnest 과정에서 생성된 view 에서는 무조건 lobLocator 타입으로 넘겨준다.
                        // lob 타입의 걸럼은 lobLocator 컨버젼 노드가 달려있으므로 컨버젼 노드를 제거해주어야 한다.
                        // ex: SELECT SUBSTR(i2,0,LENGTH(i2)) FROM t1 WHERE i1 = (SELECT MAX(i1) FROM t1);
                        if ( (sTarget->targetColumn->node.module == &mtfGetBlobLocator) ||
                             (sTarget->targetColumn->node.module == &mtfGetClobLocator) )
                        {
                            aNode[0]->node.conversion     = NULL;
                            aNode[0]->node.leftConversion = NULL;
                        }
                        else
                        {
                            aNode[0]->node.conversion     = sConversionNode;
                            aNode[0]->node.leftConversion = sLeftConversionNode;
                        }

                        SET_POSITION( aNode[0]->tableName, aViewTableRef->aliasName );
                        aNode[0]->columnName.stmtText = sTarget->aliasColumnName.name;
                        aNode[0]->columnName.size     = sTarget->aliasColumnName.size;
                        aNode[0]->columnName.offset   = 0;

                        IDE_TEST( qtc::estimateNodeWithoutArgument( aViewStatement,
                                                                    aNode[0] )
                                  != IDE_SUCCESS );

                        IDE_TEST( qmvQTC::addViewColumnRefList( aViewStatement,
                                                                aNode[0] )
                                  != IDE_SUCCESS );
                    }

                    break;
                }
                else
                {
                    // Nothing to do.
                }
                sIdx++;
            }
            // View의 SELECT절에 반드시 존재해야 한다.
            IDE_FT_ERROR( sTarget != NULL );
        }
        else
        {
            // Terminal node가 아닌 경우 재귀적으로 순회한다.
            if( (*aNode)->node.module != &qtc::subqueryModule )
            {
                sDoublePointer = (qtcNode **)&(*aNode)->node.arguments;
                while( *sDoublePointer != NULL )
                {
                    IDE_TEST( toViewColumns( aViewStatement,
                                             aViewTableRef,
                                             sDoublePointer,
                                             aIsOuterExpr )
                              != IDE_SUCCESS );
                    sDoublePointer = (qtcNode **)&(*sDoublePointer)->node.next;
                }

                IDE_TEST( qtc::estimateNodeWithArgument( aViewStatement,
                                                         *aNode )
                          != IDE_SUCCESS );
            }
            else
            {
                // BUG-45226 서브쿼리의 target 에 서브쿼리가 있을때 오류가 발생합니다.
                sTempQuerySet = ((qmsParseTree*)((*aNode)->subquery->myPlan->parseTree))->querySet;

                IDE_TEST( toViewSetOp( aViewStatement,
                                       aViewTableRef,
                                       sTempQuerySet,
                                       aIsOuterExpr )
                          != IDE_SUCCESS );

                qtc::dependencyOr( &sViewDefInfo,
                                   &((*aNode)->depInfo),
                                   &((*aNode)->depInfo) );
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-40914 */
    if ((*aNode)->overClause != NULL)
    {
        for (sOverColumn = (*aNode)->overClause->overColumn;
             sOverColumn != NULL;
             sOverColumn = sOverColumn->next)
        {
            IDE_TEST(toViewColumns(aViewStatement,
                                   aViewTableRef,
                                   &sOverColumn->node,
                                   aIsOuterExpr)
                     != IDE_SUCCESS);
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmoUnnesting::toViewSetOp( qcStatement  * aViewStatement,
                                  qmsTableRef  * aViewTableRef,
                                  qmsQuerySet  * aSetQuerySet,
                                  idBool         aIsOuterExpr )
{
/***********************************************************************
 *
 * Description :
 *     BUG-45226
 *     서브쿼리의 outerColumns 를 찾아서 toViewColumns 호출한다.
 *
 ***********************************************************************/

    qmsOuterNode * sOuter;

    if ( aSetQuerySet->setOp == QMS_NONE )
    {
        for ( sOuter  = aSetQuerySet->SFWGH->outerColumns;
              sOuter != NULL;
              sOuter  = sOuter->next )
        {
            IDE_TEST( toViewColumns( aViewStatement,
                                     aViewTableRef,
                                     &( sOuter->column ),
                                     aIsOuterExpr )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( toViewSetOp( aViewStatement,
                               aViewTableRef,
                               aSetQuerySet->left,
                               aIsOuterExpr )
                  != IDE_SUCCESS );

        IDE_TEST( toViewSetOp( aViewStatement,
                               aViewTableRef,
                               aSetQuerySet->right,
                               aIsOuterExpr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::findAndRemoveSubquery( qcStatement * aStatement,
                                     qmsSFWGH    * aSFWGH,
                                     qtcNode     * aPredicate,
                                     idBool      * aResult )
{
/***********************************************************************
 *
 * Description :
 *     aPredicate에서 aggregation subquery를 찾아 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode * sArg;
    UShort  * sRelationMap;
    idBool    sRemovable;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::findAndRemoveSubquery::__FT__" );

    *aResult = ID_FALSE;

    if( aPredicate != NULL )
    {
        if( ( aPredicate->lflag & QTC_NODE_SUBQUERY_MASK ) == QTC_NODE_SUBQUERY_EXIST )
        {
            IDE_TEST( isRemovableSubquery( aStatement,
                                           aSFWGH,
                                           aPredicate,
                                           &sRemovable,
                                           &sRelationMap )
                      != IDE_SUCCESS );

            if( sRemovable == ID_TRUE )
            {
                IDE_TEST( removeSubquery( aStatement,
                                          aSFWGH,
                                          aPredicate,
                                          sRelationMap )
                          != IDE_SUCCESS );

                *aResult = ID_TRUE;

                // Removable subquery일 때에만 해제해주면 된다.
                IDE_TEST( QC_QMP_MEM( aStatement )->free( sRelationMap )
                          != IDE_SUCCESS );
            }
            else
            {
                for( sArg = (qtcNode *)aPredicate->node.arguments;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next )
                {
                    if( sArg->node.module != &qtc::subqueryModule )
                    {
                        IDE_TEST( findAndRemoveSubquery( aStatement,
                                                         aSFWGH,
                                                         sArg,
                                                         aResult )
                                  != IDE_SUCCESS );

                        if( *aResult == ID_TRUE )
                        {
                            // 한 query에 한 번밖에 적용할 수 없으므로
                            // 더 이상 시도 하지 않음
                            break;
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

IDE_RC
qmoUnnesting::isRemovableSubquery( qcStatement  * aStatement,
                                   qmsSFWGH     * aSFWGH,
                                   qtcNode      * aSubqueryPredicate,
                                   idBool       * aResult,
                                   UShort      ** aRelationMap )
{
/***********************************************************************
 *
 * Description :
 *     Aggregation을 포함하여 windowing view를 생성하고 제거 가능한
 *     subquery인지 판단한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement  * sSQStatement;
    qmsParseTree * sSQParseTree;
    qmsSFWGH     * sSQSFWGH;
    qmsTarget    * sTarget;
    qmsAggNode   * sAggrNode;
    qmsFrom      * sFrom;
    qtcNode      * sSQNode;
    qtcNode      * sOuterNode;
    qcDepInfo      sDepInfo;
    qcDepInfo      sOuterCommonDepInfo;
    idBool         sResult;
    idBool         sIsEquivalent;
    qmsConcatElement * sGroup;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::isRemovableSubquery::__FT__" );

    *aRelationMap = NULL;

    // BUG-47616
    if ( SDU_SHARD_ENABLE == 1 )
    {
        IDE_CONT( UNREMOVABLE );
    }

    // BUG-43059 Target subquery unnest/removal disable
    if ( ( aSFWGH->thisQuerySet->lflag & QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_MASK )
         == QMV_QUERYSET_TARGET_SUBQUERY_REMOVAL_FALSE )
    {
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    if( ( aSubqueryPredicate->node.lflag & MTC_NODE_COMPARISON_MASK )
        == MTC_NODE_COMPARISON_FALSE )
    {
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        switch( aSubqueryPredicate->node.lflag & MTC_NODE_OPERATOR_MASK )
        {
            case MTC_NODE_OPERATOR_EQUAL:
            case MTC_NODE_OPERATOR_NOT_EQUAL:
            case MTC_NODE_OPERATOR_GREATER:
            case MTC_NODE_OPERATOR_GREATER_EQUAL:
            case MTC_NODE_OPERATOR_LESS:
            case MTC_NODE_OPERATOR_LESS_EQUAL:
                break;
            default:
                // =, <>, >, >=, <, <= 외 비교 연산자는 불가
                IDE_CONT( UNREMOVABLE );
        }

        if( aSubqueryPredicate->node.arguments->next->module != &qtc::subqueryModule )
        {
            // 비교 연산자의 두 번째 인자가 subquery가 아님
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            sSQNode = (qtcNode *)aSubqueryPredicate->node.arguments->next;
        }

        // BUG-46952 left가 list type이면 안됩니다.
        if( aSubqueryPredicate->node.arguments->module == &mtfList )
        {
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }

    }

    // Subquery의 조건 확인
    sSQStatement = sSQNode->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    if( sSQParseTree->querySet->setOp != QMS_NONE )
    {
        // SET 연산인 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    if ( sSQSFWGH->hints->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NO_UNNEST )
    {
        IDE_CONT( UNREMOVABLE );
    }
    else if ( sSQSFWGH->hints->subqueryUnnestType == QMO_SUBQUERY_UNNEST_TYPE_NOT_DEFINED )
    {
        if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_SUBQUERY_MASK )
             == QC_TMP_UNNEST_SUBQUERY_FALSE )
        {
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            /* Noting to do */
        }
    }
    else
    {
        /* BUG-46544 Unnest hit */
        // Hint QMO_SUBQUERY_UNNEST_TYPE_UNNEST
        // hint가 존재하고 Property는 0이고 Compatibility가 1 이면 Property
        // 우선으로 unnest를 하지 않는다.
        if ( ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_SUBQUERY_MASK )
               == QC_TMP_UNNEST_SUBQUERY_FALSE ) &&
             ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_COMPATIBILITY_1_MASK )
               == QC_TMP_UNNEST_COMPATIBILITY_1_TRUE ) )
        {
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            /* Nothing to do */
        }
    }

    if( ( aSFWGH->hierarchy != NULL ) ||
        ( sSQSFWGH->hierarchy != NULL ) )
    {
        // Hierarchy 구문을 사용한 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQParseTree->limit != NULL )
    {
        // Subquery에서 LIMIT절 사용한 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }
    
    /* BUG-36580 supported TOP */
    if ( aSFWGH->top != NULL )
    {
        // Subquery에서 TOP절 사용한 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }
    
    if( ( sSQSFWGH->rownum != NULL ) ||
        ( sSQSFWGH->level  != NULL ) ||
        ( sSQSFWGH->isLeaf != NULL ) )
    {
        // Subquery에서 ROWNUM, LEVEL, ISLEAF를 사용한 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    for( sFrom = aSFWGH->from;
         sFrom != NULL;
         sFrom = sFrom->next )
    {
        if( sFrom->joinType != QMS_NO_JOIN )
        {
            // Ansi style join을 사용한 경우 불가
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }
    }

    for( sFrom = sSQSFWGH->from;
         sFrom != NULL;
         sFrom = sFrom->next )
    {
        if( sFrom->joinType != QMS_NO_JOIN )
        {
            // Ansi style join을 사용한 경우 불가
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2418
        // sSQSFWGH의 From에서 Lateral View가 존재하면 Removal 할 수 없다.
        // 단, Lateral View가 이전에 모두 Merging 되었다면 Removal이 가능하다.
        IDE_TEST( qmvQTC::getFromLateralDepInfo( sFrom, & sDepInfo )
                  != IDE_SUCCESS );

        if ( qtc::haveDependencies( & sDepInfo ) == ID_TRUE )
        {
            // 해당 tableRef가 Lateral View라면 Removal 불가
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }
    }

    if( sSQSFWGH->group != NULL )
    {
        // GROUP BY절을 포함하는 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH->aggsDepth1 == NULL )
    {
        // Aggregate function을 사용하지 않은 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        for( sAggrNode = sSQSFWGH->aggsDepth1;
             sAggrNode != NULL;
             sAggrNode = sAggrNode->next )
        {
            if( ( sAggrNode->aggr->node.lflag & MTC_NODE_DISTINCT_MASK )
                == MTC_NODE_DISTINCT_TRUE )
            {
                // Aggregate function에 DISTINCT절 사용 시 불가
                IDE_CONT( UNREMOVABLE );
            }
            else
            {
                // Nothing to do.
            }

            if( sAggrNode->aggr->node.arguments == NULL )
            {
                // COUNT(*) 사용 시 불가
                IDE_CONT( UNREMOVABLE );
            }
            else
            {
                // Nothing to do.
            }

            /* BUG-43703 WITHIN GROUP 구문 사용시 Subquery Unnesting을 하지 않도록 합니다. */
            IDE_TEST_CONT( sAggrNode->aggr->node.funcArguments != NULL, UNREMOVABLE );
        }
    }

    if( sSQSFWGH->aggsDepth2 != NULL )
    {
        // Nested aggregate function을 사용한 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    if( sSQSFWGH->having != NULL )
    {
        // BUG-41170 having 절을 사용한 경우
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    // Subsumption property 확인
    IDE_TEST( isSubsumed( aStatement,
                          aSFWGH,
                          sSQSFWGH,
                          &sResult,
                          aRelationMap,
                          &sOuterCommonDepInfo )
              != IDE_SUCCESS );

    if( sResult == ID_FALSE )
    {
        // Subsumption property를 만족하지 않음
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    // Subquery의 aggregate function 인자와 outer query의 column이 같은
    // expression인지 비교
    if( aSubqueryPredicate->node.arguments->module == &mtfList )
    {
        // List type인 경우 list의 argument
        sOuterNode = (qtcNode *)aSubqueryPredicate->node.arguments->arguments;
    }
    else
    {
        // List type이 아닌 경우 해당 column
        sOuterNode = (qtcNode *)aSubqueryPredicate->node.arguments;
    }

    for( sTarget = sSQSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next, sOuterNode = (qtcNode *)sOuterNode->node.next )
    {
        if ( ( (sTarget->targetColumn->lflag & QTC_NODE_AGGREGATE_MASK ) != QTC_NODE_AGGREGATE_EXIST ) )
        {
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Aggregate function의 인자를 outer query의 relation으로 바꿈
            IDE_TEST( changeRelation( aStatement,
                                      sTarget->targetColumn,
                                      &sSQSFWGH->depInfo,
                                      *aRelationMap )
                      != IDE_SUCCESS );

            if( ( qtc::dependencyContains( &sOuterCommonDepInfo,
                                           &sOuterNode->depInfo ) == ID_TRUE ) &&
                ( qtc::dependencyContains( &sOuterCommonDepInfo,
                                           &sTarget->targetColumn->depInfo ) == ID_TRUE ) )
            {
                sIsEquivalent = ID_TRUE;
            }
            else
            {
                sIsEquivalent = ID_FALSE;
            }

            // 바뀌었던 relation을 다시 복원
            IDE_TEST( changeRelation( aStatement,
                                      sTarget->targetColumn,
                                      &sOuterCommonDepInfo,
                                      *aRelationMap )
                      != IDE_SUCCESS );

            if( sIsEquivalent == ID_FALSE )
            {
                IDE_CONT( UNREMOVABLE );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    // BUG-45226 서브쿼리의 target 에 서브쿼리가 있을때 오류가 발생합니다.
    // remove 일때는 막는다.
    for( sTarget = aSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        if ( QTC_HAVE_SUBQUERY( sTarget->targetColumn ) == ID_TRUE )
        {
            if ( isRemovableTarget( sTarget->targetColumn, &sOuterCommonDepInfo ) == ID_FALSE )
            {
                IDE_CONT( UNREMOVABLE );
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

    // BUG-45760
    for ( sGroup = aSFWGH->group;
         sGroup != NULL;
         sGroup = sGroup->next )
    {
        if ( ( sGroup->type == QMS_GROUPBY_ROLLUP ) ||
             ( sGroup->type == QMS_GROUPBY_CUBE ) )
        {
            IDE_RAISE( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }
    }
            
    *aResult = ID_TRUE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( UNREMOVABLE );

    *aResult = ID_FALSE;

    if( *aRelationMap != NULL )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->free( *aRelationMap )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    *aRelationMap = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoUnnesting::isRemovableTarget( qtcNode   * aNode,
                                        qcDepInfo * aOuterCommonDepInfo )
{
/***********************************************************************
 *
 * Description :
 *     BUG-45226
 *     1. target 에 서브쿼리가 있는지 검사한다.
 *     2. 서브쿼리에 Removable 테이블을 참조하는지 검사한다.
 *     3. 참조한다면 Removable 이 안되도록 한다.
 *        이유는 에러가 많이 발생해서 이다.
 *
 ***********************************************************************/
    idBool      sIsRemovable = ID_TRUE;
    mtcNode   * sNode;
    qcDepInfo   sAndDepInfo;

    if( aNode->node.module != &qtc::subqueryModule )
    {
        for ( sNode = aNode->node.arguments;
              (sNode != NULL) && (sIsRemovable == ID_TRUE);
              sNode = sNode->next )
        {
            if ( QTC_HAVE_SUBQUERY( (qtcNode*)sNode ) == ID_TRUE )
            {
                sIsRemovable = isRemovableTarget( (qtcNode*)sNode, aOuterCommonDepInfo );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        qtc::dependencyAnd( aOuterCommonDepInfo,
                            &aNode->depInfo,
                            &sAndDepInfo );
        
        if ( qtc::haveDependencies( &sAndDepInfo ) == ID_TRUE )
        {
            sIsRemovable = ID_FALSE;
        }
        else
        {
            sIsRemovable = ID_TRUE;
        }
    }
    
    return sIsRemovable;
}

IDE_RC
qmoUnnesting::isSubsumed( qcStatement  * aStatement,
                          qmsSFWGH     * aOQSFWGH,
                          qmsSFWGH     * aSQSFWGH,
                          idBool       * aResult,
                          UShort      ** aRelationMap,
                          qcDepInfo    * aOuterCommonDepInfo )
{
/***********************************************************************
 *
 * Description :
 *     Outer query가 subquery의 relation들을 모두 포함하고 있는지
 *     확인한다.
 *
 * Implementation :
 *     FROM절의 relation, 그리고 predicate들의 조건을 확인한다.
 *
 ***********************************************************************/

    qcDepInfo      sDepInfo;
    UShort       * sRelationMap = NULL;
    SInt           sTable;
    idBool         sIsEquivalent;
    idBool         sExistSubquery = ID_FALSE;
    qmoPredList  * sOQPredList = NULL;
    qmoPredList  * sSQPredList = NULL;
    qmoPredList  * sPredNode1;
    qmoPredList  * sPredNode2;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::isSubsumed::__FT__" );

    // 공통적인 relation을 갖는지 확인
    // Outer query는 subquery가 포함하는 relation들을 모두 포함해야 함
    IDE_TEST( createRelationMap( aStatement, aOQSFWGH, aSQSFWGH, &sRelationMap )
              != IDE_SUCCESS );

    if( sRelationMap == NULL )
    {
        // Outer query에서 subquery의 relation을 모두 포함하지 않음
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    // Outer query와 subquery 모두 predicate이 AND로만 구성되어야 함
    if( isConjunctiveForm( aSQSFWGH->where ) == ID_FALSE )
    {
        // Subquery에서 AND 외 논리연산자를 포함할 수 없음
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    if( isConjunctiveForm( aOQSFWGH->where ) == ID_FALSE )
    {
        // Outer query에서도 AND 외 논리연산자를 포함할 수 없음
        IDE_CONT( UNREMOVABLE );
    }
    else
    {
        // Nothing to do.
    }

    // 두 query간 공통 relation을 subquery에서 참조할 수 없음
    sTable = qtc::getPosFirstBitSet( &aSQSFWGH->outerDepInfo );

    while( sTable != QTC_DEPENDENCIES_END )
    {
        if( sRelationMap[sTable] != ID_USHORT_MAX )
        {
            // Outer query와의 공통 relation을 subquery에서 참조하면 안됨
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }

        sTable = qtc::getPosNextBitSet( &aSQSFWGH->outerDepInfo, sTable );
    }

    // 공통 relation들 중 outer query의 relation 정보를 추출
    qtc::dependencyClear( aOuterCommonDepInfo );

    sTable = qtc::getPosFirstBitSet( &aOQSFWGH->depInfo );

    while( sTable != QTC_DEPENDENCIES_END )
    {
        if( sRelationMap[sTable] != ID_USHORT_MAX )
        {
            qtc::dependencySet( sTable, &sDepInfo );
            IDE_TEST( qtc::dependencyOr( aOuterCommonDepInfo,
                                         &sDepInfo,
                                         aOuterCommonDepInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        sTable = qtc::getPosNextBitSet( &aOQSFWGH->depInfo, sTable );
    }

    IDE_TEST( genPredicateList( aStatement,
                                aOQSFWGH->where,
                                &sOQPredList )
              != IDE_SUCCESS );

    IDE_TEST( genPredicateList( aStatement,
                                aSQSFWGH->where,
                                &sSQPredList )
              != IDE_SUCCESS );

    // 공통 relation들에 대하여 동일한 predicate들을 갖는지 확인
    for( sPredNode1 = sOQPredList;
         sPredNode1 != NULL;
         sPredNode1 = sPredNode1->next )
    {
        qtc::dependencyAnd( aOuterCommonDepInfo,
                            &sPredNode1->predicate->depInfo,
                            &sDepInfo );

        if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
        {
            if( ( sPredNode1->predicate->lflag & QTC_NODE_SUBQUERY_MASK ) == QTC_NODE_SUBQUERY_EXIST )
            {
                if( sExistSubquery == ID_FALSE )
                {
                    // Subquery는 단 1개(removing 대상)만 존재할 수 있다.
                    sExistSubquery = ID_TRUE;
                    continue;
                }
                else
                {
                    // 두 개 이상 존재하는 경우
                    IDE_CONT( UNREMOVABLE );
                }
            }
            else
            {
                // Nothing to do.
            }

            if( ( qtc::haveDependencies( &aSQSFWGH->outerDepInfo ) == ID_FALSE ) &&
                ( qtc::dependencyContains( aOuterCommonDepInfo, &sPredNode1->predicate->depInfo ) == ID_FALSE ) )
            {
                // Subquery에 correlation이 없으면서 공통 relation끼리의
                // predicate이 아닌 경우 확인하지 않는다.
                continue;
            }
            else
            {
                // Nothing to do.
            }

            for( sPredNode2 = sSQPredList;
                 sPredNode2 != NULL;
                 sPredNode2 = sPredNode2->next )
            {
                sIsEquivalent = ID_TRUE;
                
                // Subquery의 predicate이 outer query의 공통 relation들을
                // 참조하는 predicate으로 일시적으로 변경
                IDE_TEST( changeRelation( aStatement,
                                          sPredNode2->predicate,
                                          &aSQSFWGH->depInfo,
                                          sRelationMap )
                          != IDE_SUCCESS );

                // 비교
                IDE_TEST( qtc::isEquivalentPredicate( aStatement,
                                                      sPredNode1->predicate,
                                                      sPredNode2->predicate,
                                                      &sIsEquivalent )
                          != IDE_SUCCESS );

                // 변경하였던 predicate들을 다시 원상복구
                IDE_TEST( changeRelation( aStatement,
                                          sPredNode2->predicate,
                                          aOuterCommonDepInfo,
                                          sRelationMap )
                          != IDE_SUCCESS );

                if( sIsEquivalent == ID_TRUE )
                {
                    // 동일하다고 판단된 predicate에 hit flag 설정
                    sPredNode2->hit = ID_TRUE;

                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( sPredNode2 == NULL )
            {
                // 동일한 predicate을 찾지 못한 경우
                IDE_CONT( UNREMOVABLE );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // 공통 relation들과 관계없는 predicate
        }
    }

    // Subquery의 모든 predicate에 hit flag가 설정되었는지 확인
    for( sPredNode2 = sSQPredList;
         sPredNode2 != NULL;
         sPredNode2 = sPredNode2->next )
    {
        if( sPredNode2->hit == ID_FALSE )
        {
            IDE_CONT( UNREMOVABLE );
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST( freePredicateList( aStatement, sOQPredList )
              != IDE_SUCCESS );
    IDE_TEST( freePredicateList( aStatement, sSQPredList )
              != IDE_SUCCESS );

    *aRelationMap = sRelationMap;
    *aResult      = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( UNREMOVABLE );

    if( sRelationMap != NULL )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->free( sRelationMap )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( freePredicateList( aStatement, sOQPredList )
              != IDE_SUCCESS );
    IDE_TEST( freePredicateList( aStatement, sSQPredList )
              != IDE_SUCCESS );

    *aRelationMap = NULL;
    *aResult      = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::changeRelation( qcStatement * aStatement,
                              qtcNode     * aPredicate,
                              qcDepInfo   * aDepInfo,
                              UShort      * aRelationMap )
{
/***********************************************************************
 *
 * Description :
 *     aPredicate에 포함된 node들 중 aDepInfo에 포함되는 node들의
 *     table값을 aRelationMap에 따라 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode   * sArg;
    mtcNode   * sConversion;
    qcDepInfo   sDepInfo;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::changeRelation::__FT__" );

    qtc::dependencyAnd( aDepInfo, &aPredicate->depInfo, &sDepInfo );

    if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
    {
        if( aPredicate->node.module == &qtc::columnModule )
        {
            aPredicate->node.table = aRelationMap[aPredicate->node.table];

            // BUG-42113 LOB type 에 대한 subquery 변환이 수행되어야 합니다.
            // lob에 대한 컨버젼 함수에서는 baseTable 을 사용한다.
            // 따라서 같이 변경해주어야 한다.
            sConversion = aPredicate->node.conversion;
            while ( sConversion != NULL )
            {
                sConversion->baseTable = aPredicate->node.table;

                sConversion = sConversion->next;
            }

            qtc::dependencySet( aPredicate->node.table,
                                &aPredicate->depInfo );

            // BUG-41141 estimate 가 되었다고 보장할수 없으므로
            // estimate 를 해주어야 한다.
            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        aPredicate )
                      != IDE_SUCCESS );
        }
        else
        {
            for( sArg = (qtcNode *)aPredicate->node.arguments;
                 sArg != NULL;
                 sArg = (qtcNode *)sArg->node.next )
            {
                changeRelation( aStatement,
                                sArg,
                                aDepInfo,
                                aRelationMap );
            }

            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     aPredicate )
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

IDE_RC
qmoUnnesting::createRelationMap( qcStatement  * aStatement,
                                 qmsSFWGH     * aOQSFWGH,
                                 qmsSFWGH     * aSQSFWGH,
                                 UShort      ** aRelationMap )
{
/***********************************************************************
 *
 * Description :
 *     Relation map 자료구조를 구성한다.
 *     ex) SELECT * FROM t1, t2, t3 WHERE ... AND t1.c1 IN
 *             (SELECT AVG(t1.c1) FROM t1, t2 ... )
 *         이 때 tuple-set과 구성된 relation map은 다음과 같다.
 *         | Idx. | Description        | Map |
 *         | 0    | Intermediate tuple | N/A |
 *         | 1    | T1(outer query)    | 4   |
 *         | 2    | T2(outer query)    | 5   |
 *         | 3    | T3(outer query)    | N/A |
 *         | 4    | T1(subquery)       | 1   |
 *         | 5    | T2(subquery)       | 2   |
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom * sOQFrom;
    qmsFrom * sSQFrom;
    UShort  * sRelationMap = NULL;
    UShort    sRowCount;
    UShort    i;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::createRelationMap::__FT__" );

    sRowCount = QC_SHARED_TMPLATE( aStatement )->tmplate.rowCount;

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sRowCount * ID_SIZEOF( UShort ),
                                               (void**)&sRelationMap )
              != IDE_SUCCESS );

    for( i = 0; i < sRowCount; i++ )
    {
        sRelationMap[i] = ID_USHORT_MAX;
    }

    for( sSQFrom = aSQSFWGH->from;
         sSQFrom != NULL;
         sSQFrom = sSQFrom->next )
    {
        if( sSQFrom->tableRef->tableInfo->tableID == 0 )
        {
            // Inline view는 비교할 수 없다.
            IDE_CONT( UNABLE );
        }
        else
        {
            // Nothing to do.
        }

        for( sOQFrom = aOQSFWGH->from;
             sOQFrom != NULL;
             sOQFrom = sOQFrom->next )
        {
            if( sSQFrom->tableRef->tableInfo->tableID
                == sOQFrom->tableRef->tableInfo->tableID )
            {
                if( sRelationMap[sOQFrom->tableRef->table] != ID_USHORT_MAX )
                {
                    // 이미 mapping 된 relation인 경우
                }
                else
                {
                    sRelationMap[sSQFrom->tableRef->table] = sOQFrom->tableRef->table;
                    sRelationMap[sOQFrom->tableRef->table] = sSQFrom->tableRef->table;
                    break;
                }
            }
            else
            {
                // Nothing to do.
            }
        }

        if( sOQFrom == NULL )
        {
            // sSQFrom과 동일한 relation을 outer query에서 찾지 못함
            IDE_CONT( UNABLE );
        }
        else
        {
            // Nothing to do.
        }
    }

    *aRelationMap = sRelationMap;

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( UNABLE );

    if( sRelationMap != NULL )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->free( sRelationMap )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    *aRelationMap = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoUnnesting::isConjunctiveForm( qtcNode * aPredicate )
{
/***********************************************************************
 *
 * Description :
 *     인자로 받은 predicate이 conjunctive form인지 확인한다.
 *
 * Implementation :
 *     AND 외 논리연산자가 포함되지 않았는지 재귀적으로 확인한다.
 *
 ***********************************************************************/

    qtcNode * sArg;

    if( aPredicate != NULL )
    {
        if( ( aPredicate->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            if( ( aPredicate->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_AND )
            {
                for( sArg = (qtcNode *)aPredicate->node.arguments;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next )
                {
                    if( isConjunctiveForm( sArg ) == ID_FALSE )
                    {
                        IDE_CONT( NOT_APPLICABLE_EXIT );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                IDE_CONT( NOT_APPLICABLE_EXIT );
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

    return ID_TRUE;

    IDE_EXCEPTION_CONT( NOT_APPLICABLE_EXIT );

    return ID_FALSE;
}

IDE_RC
qmoUnnesting::genPredicateList( qcStatement  * aStatement,
                                qtcNode      * aPredicate,
                                qmoPredList ** aPredList )
{
/***********************************************************************
 *
 * Description :
 *     논리 연산자가 아닌 predicate들을 모두 찾아 list로 구성한다.
 *     ex) A AND (B AND C), A AND B AND C
 *         => A, B, C
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredList * sFirst = NULL;
    qmoPredList * sLast  = NULL;
    qmoPredList * sPredNode;
    qtcNode     * sArg;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::genPredicateList::__FT__" );

    if( aPredicate != NULL )
    {
        if( ( aPredicate->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            for( sArg = (qtcNode *)aPredicate->node.arguments;
                 sArg != NULL;
                 sArg = (qtcNode *)sArg->node.next )
            {
                IDE_TEST( genPredicateList( aStatement,
                                            sArg,
                                            &sPredNode )
                          != IDE_SUCCESS );
                if( sFirst == NULL )
                {
                    sFirst = sLast = sPredNode;
                }
                else
                {
                    sLast->next = sPredNode;
                }

                while( sLast->next != NULL )
                {
                    sLast = sLast->next;
                }
            }

            *aPredList = sFirst;
        }
        else
        {
            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoPredList ),
                                                       (void **)&sPredNode )
                      != IDE_SUCCESS );

            sPredNode->predicate = aPredicate;
            sPredNode->next      = NULL;
            sPredNode->hit       = ID_FALSE;

            *aPredList = sPredNode;
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
qmoUnnesting::freePredicateList( qcStatement * aStatement,
                                 qmoPredList * aPredList )
{
/***********************************************************************
 *
 * Description :
 *     aPredList를 해제한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoPredList * sPredNode = aPredList;
    qmoPredList * sPrevPredNode;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::freePredicateList::__FT__" );

    while( sPredNode != NULL )
    {
        sPrevPredNode = sPredNode;
        sPredNode = sPredNode->next;

        IDE_TEST( QC_QMP_MEM( aStatement )->free( sPrevPredNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::changeSemiJoinInnerTable( qmsSFWGH * aSFWGH,
                                        qmsSFWGH * aViewSFWGH,
                                        SInt       aViewID )
{
/***********************************************************************
 *
 * Description :
 *     Subquery 제거시 remove 대상 subquery의 table이 semi/anti join의
 *     inner table인 경우 변환된 view를 inner table로 가리키도록 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom   * sFrom;
    qcDepInfo   sDepInfo;
    SInt        sTable;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::changeSemiJoinInnerTable::__FT__" );

    for( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        if( qtc::haveDependencies( &sFrom->semiAntiJoinDepInfo ) == ID_TRUE )
        {
            qtc::dependencyAnd( &sFrom->semiAntiJoinDepInfo,
                                &aViewSFWGH->depInfo,
                                &sDepInfo );

            if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
            {
                sTable = qtc::getPosFirstBitSet( &aViewSFWGH->depInfo );
                while( sTable != QTC_DEPENDENCIES_END )
                {
                    qtc::dependencyChange( sTable,
                                           aViewID,
                                           &sFrom->semiAntiJoinDepInfo,
                                           &sFrom->semiAntiJoinDepInfo );
                    sTable = qtc::getPosNextBitSet( &aViewSFWGH->depInfo, sTable );
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

    return IDE_SUCCESS;
}

IDE_RC
qmoUnnesting::removeSubquery( qcStatement * aStatement,
                              qmsSFWGH    * aSFWGH,
                              qtcNode     * aPredicate,
                              UShort      * aRelationMap )
{
/***********************************************************************
 *
 * Description :
 *     TPC-H의 2, 15, 17번 query과 같이 subquery와 outer query가
 *     공통 relation들을 갖고 subquery에서 aggregate function을 사용하는
 *     경우의 transformation을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement    * sSQStatement;
    qtcNode        * sIsNotNull[2];
    qtcNode        * sCol1Node[2];
    qtcNode        * sSQNode;
    mtcNode        * sNext;
    qcNamePosition   sEmptyPosition;
    qmsTarget      * sTarget;
    qmsParseTree   * sParseTree = NULL;
    qmsParseTree   * sSQParseTree;
    qmsSFWGH       * sSQSFWGH;
    qmsFrom        * sViewFrom;
    qmsFrom        * sFrom;
    qmsFrom       ** sDoublePointer;
    qmsSortColumns * sSort;
    idBool           sIsCorrelatedSQ;

    qmsConcatElement * sGroup;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::removeSubquery::__FT__" );

    sSQNode      = (qtcNode *)aPredicate->node.arguments->next;
    sSQStatement = sSQNode->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    /* TASK-7219 Shard Transformer Refactoring */
    if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                        SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
         == ID_TRUE )
    {
        IDE_TEST( sdi::preAnalyzeQuerySet( sSQStatement,
                                           sSQParseTree->querySet,
                                           QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if( qtc::haveDependencies( &sSQSFWGH->outerDepInfo ) == ID_TRUE )
    {
        sIsCorrelatedSQ = ID_TRUE;
    }
    else
    {
        sIsCorrelatedSQ = ID_FALSE;
    }

    IDE_TEST( transformToCase2Expression( aPredicate )
              != IDE_SUCCESS );

    IDE_TEST( transformAggr2Window( sSQStatement, sSQSFWGH->target->targetColumn, aRelationMap )
              != IDE_SUCCESS );

    sSQSFWGH->aggsDepth1 = NULL;

    // Outer query의 FROM절에 relation들을 subquery로 이동한다.
    sSQSFWGH->where = NULL;
    if( sIsCorrelatedSQ == ID_TRUE )
    {
        // Correlated subquery인 경우 모든 relation들을 옮긴다.
        sSQSFWGH->from = aSFWGH->from;

        // Subquery의 dependency 설정
        qtc::dependencySetWithDep( &sSQSFWGH->depInfo, &aSFWGH->depInfo );
        qtc::dependencySetWithDep( &sSQSFWGH->thisQuerySet->depInfo, &aSFWGH->thisQuerySet->depInfo );
    }
    else
    {
        // Uncorrelated subquery인 경우 공통 relation들만 옮긴다.
        sSQSFWGH->from = NULL;
        sDoublePointer = &aSFWGH->from;

        while( *sDoublePointer != NULL )
        {
            sFrom = *sDoublePointer;

            if( aRelationMap[sFrom->tableRef->table] != ID_USHORT_MAX )
            {
                *sDoublePointer = sFrom->next;

                sFrom->next = sSQSFWGH->from;
                sSQSFWGH->from = sFrom;

                // Subquery의 dependency 설정
                qtc::dependencyChange( aRelationMap[sFrom->tableRef->table],
                                       sFrom->tableRef->table,
                                       &sSQSFWGH->depInfo,
                                       &sSQSFWGH->depInfo );

                qtc::dependencyChange( aRelationMap[sFrom->tableRef->table],
                                       sFrom->tableRef->table,
                                       &sSQSFWGH->thisQuerySet->depInfo,
                                       &sSQSFWGH->thisQuerySet->depInfo );
            }
            else
            {
                sDoublePointer = &sFrom->next;
            }
        }
    }

    // View로 변환될 것이므로 outer dependency가 없어야 한다.
    qtc::dependencyClear( &sSQSFWGH->outerDepInfo );

    // PROJ-2418 View로 변환될 상황에서는 Lateral View가 없으므로
    // lateralDepInfo 정보도 같이 제거한다.
    qtc::dependencyClear( &sSQParseTree->querySet->lateralDepInfo );

    // Outer query의 WHERE절 조건을 view(아직은 subquery)로 옮긴다.
    IDE_TEST( movePredicates( sSQStatement, &aSFWGH->where, sSQSFWGH )
              != IDE_SUCCESS );

    // WHERE절에 참조하는 column들은 view에서 반환한다.
    for( sTarget = aSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        IDE_TEST( genViewSelect( sSQStatement, sTarget->targetColumn, ID_TRUE )
                  != IDE_SUCCESS );
    }

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;

    if( sParseTree->querySet == aSFWGH->thisQuerySet )
    {
        for( sSort = sParseTree->orderBy;
             sSort != NULL;
             sSort = sSort->next )
        {
            IDE_TEST( genViewSelect( sSQStatement, sSort->sortColumn, ID_TRUE )
                      != IDE_SUCCESS );
        }

        // BUGBUG
        /*
        for ( sGroup = sParseTree->querySet->SFWGH->group;
             sGroup != NULL;
             sGroup = sGroup->next )
        {
            IDE_TEST( genViewSelect( sSQStatement, sGroup->arithmeticOrList, ID_TRUE )
                      != IDE_SUCCESS );
        } 
        */
    }
    else
    {
        // Nothing to do.
    }

    // View의 SELECT절을 구성한다.
    IDE_TEST( genViewSelect( sSQStatement, aSFWGH->where, ID_TRUE )
              != IDE_SUCCESS );

    // BUG-42113 LOB type 에 대한 subquery 변환이 수행되어야 합니다.
    // unnest 과정에서 생성된 view의 target에 lob 컬럼이 있을경우 LobLocatorFunc을 연결해준다.
    // view는 반드시 lobLocator 타입으로 변환되어야 한다.
    IDE_TEST( qmvQuerySet::addLobLocatorFunc( sSQStatement, sSQSFWGH->target )
              != IDE_SUCCESS );

    // Subquery를 view로 변환
    IDE_TEST( transformSubqueryToView( aStatement,
                                       sSQNode,
                                       &sViewFrom )
              != IDE_SUCCESS );

    // View로 흡수된 table이 semi/anti join의 inner table이었던 경우
    // inner table을 view로 가리키도록 한다.
    IDE_TEST( changeSemiJoinInnerTable( aSFWGH,
                                        sSQSFWGH,
                                        sViewFrom->tableRef->table )
              != IDE_SUCCESS );

    // Subquery predicate이 있던 자리에 COL1 IS NOT NULL을 설정한다.
    SET_EMPTY_POSITION( sEmptyPosition );

    IDE_TEST( qtc::makeColumn( aStatement,
                               sCol1Node,
                               NULL,
                               NULL,
                               NULL,
                               NULL )
              != IDE_SUCCESS );

    sCol1Node[0]->node.module     = &qtc::columnModule;
    sCol1Node[0]->node.table      = sViewFrom->tableRef->table;
    sCol1Node[0]->node.column     = 0;
    sCol1Node[0]->node.baseTable  = sCol1Node[0]->node.table;
    sCol1Node[0]->node.baseColumn = sCol1Node[0]->node.column;
    sCol1Node[0]->node.next       = NULL;

    SET_POSITION( sCol1Node[0]->tableName, sViewFrom->tableRef->aliasName );
    sCol1Node[0]->columnName.stmtText = sSQSFWGH->target->aliasColumnName.name;
    sCol1Node[0]->columnName.size     = sSQSFWGH->target->aliasColumnName.size;
    sCol1Node[0]->columnName.offset   = 0;

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sCol1Node[0] )
              != IDE_SUCCESS );

    IDE_TEST( qmvQTC::addViewColumnRefList( aStatement,
                                            sCol1Node[0] )
              != IDE_SUCCESS );

    IDE_TEST( qtc::makeNode( aStatement,
                             sIsNotNull,
                             &sEmptyPosition,
                             &mtfIsNotNull )
              != IDE_SUCCESS );

    sIsNotNull[0]->node.arguments = (mtcNode *)sCol1Node[0];

    IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                             sIsNotNull[0] )
              != IDE_SUCCESS );

    sNext = aPredicate->node.next;
    idlOS::memcpy( aPredicate, sIsNotNull[0], ID_SIZEOF( qtcNode ) );
    aPredicate->node.next = sNext;

    // Outer query의 각 clause에서 참조하던 column들을 view column을 참조하도록 변경한다.
    if( sParseTree->querySet == aSFWGH->thisQuerySet )
    {
        for( sSort = sParseTree->orderBy;
             sSort != NULL;
             sSort = sSort->next )
        {
            IDE_TEST( toViewColumns( sSQStatement,
                                     sViewFrom->tableRef,
                                     &sSort->sortColumn,
                                     ID_TRUE )
                      != IDE_SUCCESS );
        }

        // BUG-38228
        // Outer query의 group by 절도 view를 가리켜야 한다.
        for( sGroup = sParseTree->querySet->SFWGH->group;
             sGroup != NULL;
             sGroup = sGroup->next )
        {
            IDE_TEST( toViewColumns( sSQStatement,
                                     sViewFrom->tableRef,
                                     &sGroup->arithmeticOrList,
                                     ID_TRUE )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    for( sTarget = aSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next )
    {
        IDE_TEST( toViewColumns( sSQStatement,
                                 sViewFrom->tableRef,
                                 &sTarget->targetColumn,
                                 ID_TRUE )
                  != IDE_SUCCESS );
    }

    IDE_TEST( toViewColumns( sSQStatement,
                             sViewFrom->tableRef,
                             &aSFWGH->where,
                             ID_TRUE )
              != IDE_SUCCESS );

    if( sIsCorrelatedSQ == ID_TRUE )
    {
        aSFWGH->from = sViewFrom;
    }
    else
    {
        sViewFrom->next = aSFWGH->from;
        aSFWGH->from = sViewFrom;
    }

    // Dependency 설정
    qtc::dependencyClear( &aSFWGH->depInfo );
    qtc::dependencyClear( &aSFWGH->thisQuerySet->depInfo );

    for( sFrom = aSFWGH->from;
         sFrom != NULL;
         sFrom = sFrom->next )
    {
        IDE_TEST( qtc::dependencyOr( &aSFWGH->depInfo,
                                     &sFrom->depInfo,
                                     &aSFWGH->depInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( &aSFWGH->thisQuerySet->depInfo,
                                     &sFrom->depInfo,
                                     &aSFWGH->thisQuerySet->depInfo )
                  != IDE_SUCCESS );
    }

    IDE_TEST( qmoViewMerging::validateNode( aStatement,
                                            aSFWGH->where )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::transformToCase2Expression( qtcNode * aSubqueryPredicate )
{
/***********************************************************************
 *
 * Description :
 *     Subquery predicate을 이용하여 SELECT절을 CASE2 expression으로
 *     변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement    * sSQStatement;
    qmsParseTree   * sSQParseTree;
    qmsSFWGH       * sSQSFWGH;
    qtcNode        * sCase2[2];
    qtcNode        * sCorrPred;
    qtcNode        * sConstNode = NULL;
    qcNamePosition   sEmptyPosition;
    SChar          * sColumnName;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::transformToCase2Expression::__FT__" );

    sSQStatement = ((qtcNode *)aSubqueryPredicate->node.arguments->next)->subquery;
    sSQParseTree = (qmsParseTree *)sSQStatement->myPlan->parseTree;
    sSQSFWGH     = sSQParseTree->querySet->SFWGH;

    IDE_TEST( genCorrPredicates( sSQStatement,
                                 aSubqueryPredicate,
                                 ID_FALSE /* aExistsTrans */,
                                 &sCorrPred )
              != IDE_SUCCESS );

    IDE_TEST( makeDummyConstant( sSQStatement,
                                 &sConstNode )
              != IDE_SUCCESS );

    SET_EMPTY_POSITION( sEmptyPosition );

    IDE_TEST( qtc::makeNode( sSQStatement,
                             sCase2,
                             &sEmptyPosition,
                             &mtfCase2 )
              != IDE_SUCCESS );

    // Argument들을 설정한다.
    // ex) CASE2( T1.C1 = AVG(T1.C1) OVER (PARTITION BY ...), '0' )
    sCase2[0]->node.arguments = (mtcNode *)sCorrPred;
    sCorrPred->node.next      = (mtcNode *)sConstNode;

    IDE_TEST( qtc::estimateNodeWithArgument( sSQStatement,
                                             sCase2[0] )
              != IDE_SUCCESS );

    // COL1로 alias를 설정한다.
    IDE_TEST( QC_QMP_MEM( sSQStatement )->alloc( COLUMN_NAME_LENGTH,
                                                 (void**)&sColumnName )
              != IDE_SUCCESS );

    idlOS::strncpy( (SChar*)sColumnName,
                    COLUMN_NAME_PREFIX"1",
                    COLUMN_NAME_LENGTH );

    sSQSFWGH->target->targetColumn         = sCase2[0];
    sSQSFWGH->target->aliasColumnName.name = sColumnName;
    sSQSFWGH->target->aliasColumnName.size = idlOS::strlen( sColumnName );
    sSQSFWGH->target->next                 = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoUnnesting::transformAggr2Window( qcStatement * aStatement,
                                    qtcNode     * aNode,
                                    UShort      * aRelationMap )
{
/***********************************************************************
 *
 * Description :
 *     SELECT절의 aggregate function을 window function으로 변환한다.
 *     이 때 PARTITION BY절의 expression은 WHERE절의 correlation들로 한다.
 *
 * Implementation :
 *     Correlation predicate들을 찾아 PARTITION BY절에 나열한다.
 *
 ***********************************************************************/

    qmsParseTree  * sParseTree;
    qmsSFWGH      * sSFWGH;
    qtcOver       * sOver;
    qtcNode       * sArg;
    qtcOverColumn * sPartitions;
    qtcOverColumn * sPartition;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::transformAggr2Window::__FT__" );

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
    sSFWGH     = sParseTree->querySet->SFWGH;

    if( ( aNode->lflag & QTC_NODE_AGGREGATE_MASK )
        == QTC_NODE_AGGREGATE_EXIST )
    {
        if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_AGGREGATION )
        {
            IDE_TEST( QC_QMP_MEM( aStatement )->cralloc( ID_SIZEOF( qtcOver ),
                                                         (void **)&sOver )
                      != IDE_SUCCESS );

            // Aggregate function의 argument를 outer query의 relation으로 변경한다.
            for( sArg = (qtcNode *)aNode->node.arguments;
                 sArg != NULL;
                 sArg = (qtcNode *)sArg->node.next )
            {
                IDE_TEST( changeRelation( aStatement,
                                          (qtcNode *)sArg,
                                          &sSFWGH->depInfo,
                                          aRelationMap )
                          != IDE_SUCCESS );
            }

            sPartitions = NULL;

            IDE_TEST( genPartitions( aStatement,
                                     sSFWGH,
                                     sSFWGH->where,
                                     &sPartitions )
                      != IDE_SUCCESS );

            // PARTITION BY절의 column들을 outer query의 relation으로 변경한다.
            for( sPartition = sPartitions;
                 sPartition != NULL;
                 sPartition = sPartition->next )
            {
                IDE_TEST( changeRelation( aStatement,
                                          sPartition->node,
                                          &sSFWGH->depInfo,
                                          aRelationMap )
                          != IDE_SUCCESS );
            }

            sOver->overColumn        = sPartitions;
            sOver->partitionByColumn = sPartitions;
            SET_EMPTY_POSITION( sOver->endPos );
            aNode->overClause = sOver;

            IDE_TEST( qtc::estimateWindowFunction( aStatement,
                                                   sSFWGH,
                                                   aNode )
                      != IDE_SUCCESS );
        }
        else
        {
            for( sArg = (qtcNode *)aNode->node.arguments;
                 sArg != NULL;
                 sArg = (qtcNode *)sArg->node.next )
            {
                IDE_TEST( transformAggr2Window( aStatement,
                                                sArg,
                                                aRelationMap )
                          != IDE_SUCCESS );
            }

            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     aNode )
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

IDE_RC
qmoUnnesting::movePredicates( qcStatement  * aStatement,
                              qtcNode     ** aPredicate,
                              qmsSFWGH     * aSFWGH )
{
/***********************************************************************
 *
 * Description :
 *     aPredicate을 aSFWGH의 WHERE절로 옮긴다.
 *     옮겨진 predicate은 원래 위치에서 제거되어야 하므로 double pointer로
 *     넘겨 받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode    * sNode;
    qtcNode    * sOld;
    qtcNode    * sConcatPred;
    qtcNode   ** sDoublePointer;
    qcDepInfo    sDepInfo;
    ULong        sMask;
    ULong        sCondition;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::movePredicates::__FT__" );

    sNode = *aPredicate;

    qtc::dependencyAnd( &sNode->depInfo,
                        &aSFWGH->depInfo,
                        &sDepInfo );

    if( qtc::haveDependencies( &sDepInfo ) )
    {
        if( ( sNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            IDE_FT_ERROR( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                          == MTC_NODE_OPERATOR_AND );

            sDoublePointer = (qtcNode **)&sNode->node.arguments;

            while( *sDoublePointer != NULL )
            {
                sOld = *sDoublePointer;

                IDE_TEST( movePredicates( aStatement,
                                          sDoublePointer,
                                          aSFWGH )
                          != IDE_SUCCESS );
                if( sOld == *sDoublePointer )
                {
                    sDoublePointer = (qtcNode **)&(*sDoublePointer)->node.next;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( sNode->node.arguments == NULL )
            {
                // AND의 operand가 모두 제거된 경우
                *aPredicate = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sMask      = ( QTC_NODE_ROWNUM_MASK | QTC_NODE_SUBQUERY_MASK );
            sCondition = ( QTC_NODE_ROWNUM_ABSENT | QTC_NODE_SUBQUERY_ABSENT );
            if( ( qtc::dependencyContains( &aSFWGH->depInfo,
                                           &sNode->depInfo ) == ID_TRUE ) &&
                ( ( sNode->lflag & sMask ) == sCondition ) )
            {
                // Correlation이거나 ROWNUM 또는 subquery를 포함하는 predicate은 제외한다.

                *aPredicate = (qtcNode *)sNode->node.next;
                sNode->node.next = NULL;

                IDE_TEST( concatPredicate( aStatement,
                                           aSFWGH->where,
                                           sNode,
                                           &sConcatPred )
                          != IDE_SUCCESS );
                aSFWGH->where = sConcatPred;
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

IDE_RC
qmoUnnesting::addPartition( qcStatement    * aStatement,
                            qtcNode        * aExpression,
                            qtcOverColumn ** aPartitions )
{
/***********************************************************************
 *
 * Description :
 *     PARTITION BY절에 expression을 추가한다.
 *     만약 동일한 expression이 이미 존재하면 무시한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcOverColumn * sPartition;
    idBool          sIsEquivalent = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::addPartition::__FT__" );

    // 이미 동일한 expression이 partition에 존재하는지 찾는다.
    for( sPartition = *aPartitions;
         sPartition != NULL;
         sPartition = sPartition->next )

    {
        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                               sPartition->node,
                                               aExpression,
                                               &sIsEquivalent )
                  != IDE_SUCCESS );

        if ( sIsEquivalent == ID_TRUE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    // BUG-37781
    if ( sIsEquivalent == ID_FALSE )
    {
        // 존재하지 않는 경우 추가한다.
        IDE_TEST( QC_QMP_MEM( aStatement )->cralloc( ID_SIZEOF( qtcOverColumn ),
                                                     (void **)&sPartition )
                  != IDE_SUCCESS );

        sPartition->node = aExpression;
        sPartition->next = *aPartitions;

        *aPartitions = sPartition;
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
qmoUnnesting::genPartitions( qcStatement    * aStatement,
                             qmsSFWGH       * aSFWGH,
                             qtcNode        * aPredicate,
                             qtcOverColumn ** aPartitions )
{
/***********************************************************************
 *
 * Description :
 *     WHERE절의 조건 중 correlation predicate을 찾아 window function의
 *     PARTITION BY절에 설정한 expression들을 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree  * sParseTree;
    qmsQuerySet   * sQuerySet;
    qtcNode       * sArg;
    qcDepInfo       sDepInfo;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::genPartitions::__FT__" );

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;
    sQuerySet  = sParseTree->querySet;

    if( aPredicate != NULL )
    {
        qtc::dependencyAnd( &sQuerySet->depInfo, &aPredicate->depInfo, &sDepInfo );

        if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
        {
            if( ( aPredicate->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                IDE_FT_ERROR( ( aPredicate->node.lflag & MTC_NODE_OPERATOR_MASK )
                              == MTC_NODE_OPERATOR_AND );

                for( sArg = (qtcNode *)aPredicate->node.arguments;
                     sArg != NULL;
                     sArg = (qtcNode *)sArg->node.next )
                {
                    IDE_TEST( genPartitions( aStatement,
                                             aSFWGH,
                                             sArg,
                                             aPartitions )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                IDE_FT_ERROR( ( aPredicate->node.lflag & MTC_NODE_COMPARISON_MASK )
                              == MTC_NODE_COMPARISON_TRUE );

                qtc::dependencyAnd( &aPredicate->depInfo,
                                    &aSFWGH->outerDepInfo,
                                    &sDepInfo );

                if( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
                {
                    // Correlation predicate인 경우
                    for( sArg = (qtcNode *)aPredicate->node.arguments;
                         sArg != NULL;
                         sArg = (qtcNode *)sArg->node.next )
                    {
                        if( qtc::dependencyContains( &aSFWGH->depInfo,
                                                     &sArg->depInfo )
                            == ID_TRUE )
                        {
                            IDE_TEST( addPartition( aStatement,
                                                    sArg,
                                                    aPartitions )
                                      != IDE_SUCCESS );
                            break;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoUnnesting::existViewTarget( qtcNode   * aNode,
                                      qcDepInfo * aDepInfo )
{
/***********************************************************************
 *
 * Description :
 *     Subquery에 포함된 predicate 중에서
 *     unnest 시 생성될 view 의 target 절이 될 column 이 있는지 검사한다.
 *
 * Implementation :
 *     Corelation predicate 에서 column 이나 pass node,
 *     aggragation node 가 있는지 검사한다.
 *
 *     removeCorrPredicate 함수에서 corelation predicate 의 분류 조건이 바뀌거나,
 *     genViewSelect 함수에서 target 절 선택 조건이 바뀌면 이 함수도 바뀌어야 한다.
 *
 ***********************************************************************/

    qtcNode   * sArg;
    qcDepInfo   sDepInfo;
    idBool      sExist = ID_FALSE;

    if ( aNode != NULL )
    {
        qtc::dependencyAnd( &aNode->depInfo,
                            aDepInfo,
                            &sDepInfo );

        if ( qtc::haveDependencies( &sDepInfo ) == ID_TRUE )
        {
            // Corelation predicate
            if ( ( aNode->node.module == &qtc::columnModule ) ||
                 ( aNode->node.module == &qtc::passModule ) ||
                 ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_AGGREGATION ) )
            {
                // genViewSelect 함수와 동일한 조건을 만족하면 
                // target 절로 선택될 것이다.
                sExist = ID_TRUE;
            }
            else
            {
                if ( aNode->node.module == &qtc::subqueryModule )
                {
                    // Subquery는 Target으로 고려하지 않는다.
                    // Nothing to do.
                }
                else
                {
                    // Arguments 를 재귀적으로 검사한다.
                    for ( sArg = (qtcNode *)aNode->node.arguments;
                          sArg != NULL;
                          sArg = (qtcNode *)sArg->node.next )
                    {
                        if ( existViewTarget( sArg, aDepInfo ) == ID_TRUE )
                        {
                            sExist = ID_TRUE;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
            }
        }
        else
        {
            // Dependency 를 갖지 않는 node
            // Nothing to do.
        }
    }
    else
    {
        // Node 가 null 인 경우
        // Nothing to do.
    }

    return sExist;
}


IDE_RC qmoUnnesting::setAggrNode( qcStatement * aSQStatement,
                                  qmsSFWGH    * aSQSFWGH,
                                  qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     BUG-40753 aggsDepth1 관리
 *
 * Implementation :
 *     aggr Node 를 찾아서 aggsDepth1 에 추가한다.
 *     subQuery 의 내부는 찾지 않는다.
 *     아래와 같은 상황에서 sum 노드를 복사를 했기때문에 추가가 된다.
 *         ex) i1 in ( select sum( c1 ) from ...
 *     아래와 같은 경우는 추가가 안된다.
 *         ex) i1 in ( select sum( c1 )+1 from ...
 ***********************************************************************/

    idBool        sIsAdd = ID_TRUE;
    qtcNode     * sArg;
    qmsAggNode  * sAggNode;

    IDU_FIT_POINT_FATAL( "qmoUnnesting::setAggrNode::__FT__" );

    if ( QTC_IS_AGGREGATE( aNode ) == ID_TRUE )
    {
        for( sAggNode = aSQSFWGH->aggsDepth1;
             sAggNode != NULL;
             sAggNode = sAggNode->next )
        {
            if ( sAggNode->aggr == aNode )
            {
                sIsAdd = ID_FALSE;
                break;
            }
            else
            {
                // nothing to do.
            }
        }

        if ( sIsAdd == ID_TRUE )
        {
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aSQStatement ),
                                    qmsAggNode,
                                    (void**)&sAggNode )
                      != IDE_SUCCESS );

            sAggNode->aggr = aNode;
            sAggNode->next = aSQSFWGH->aggsDepth1;

            aSQSFWGH->aggsDepth1 = sAggNode;
        }
        else
        {
            // nothing to do.
        }
    }
    else
    {
        for( sArg = (qtcNode *)aNode->node.arguments;
             sArg != NULL;
             sArg = (qtcNode *)sArg->node.next )
        {
            if( QTC_IS_SUBQUERY( sArg ) == ID_FALSE )
            {
                IDE_TEST( setAggrNode( aSQStatement,
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

void qmoUnnesting::delAggrNode( qmsSFWGH    * aSQSFWGH,
                                qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description :
 *     BUG-45591 aggsDepth1 관리
 *               불필요해진 aggsDepth1 를 지운다.
 *
 * Implementation :
 *     aggr Node 를 찾아서 aggsDepth1 에 제거한다.
 *     아래와 같은 상황에서 sum 노드를 복사를 했기때문에 기존 노드가 삭제된다.
 *         ex) i1 in ( select sum( c1 ) from ...
 *     아래와 같은 경우는 삭제가 안된다.
 *         ex) i1 in ( select sum( c1 )+1 from ...
 ***********************************************************************/

    qmsAggNode  * sAggNode;
    qmsAggNode ** sPrev;

    if ( QTC_IS_AGGREGATE( aNode ) == ID_TRUE )
    {
        for( sPrev    = &aSQSFWGH->aggsDepth1,
             sAggNode = aSQSFWGH->aggsDepth1;
             sAggNode != NULL;
             sAggNode = sAggNode->next )
        {
            if ( sAggNode->aggr == aNode )
            {
                (*sPrev) = sAggNode->next;
            }
            else
            {
                sPrev = &sAggNode->next;
            }
        }
    }
    else
    {
        // Nothing to do.
    }
}

idBool qmoUnnesting::existOuterSubQueryArgument( qtcNode   * aNode,
                                                 qcDepInfo * aInnerDepInfo )
{
/*****************************************************************************
 *
 * Description : BUG-41564
 *               Subquery Argument를 가진 Predicate가 존재할 때,
 *               이 Subquery Argument가 현재 Unnesting 중인 Subquery 외에
 *               Outer Query Block을 참조하는지 검사한다.
 *               그렇다면, Unnesting을 할 수 없다.
 *
 * (e.g.) SELECT T1 FROM T1 
 *        WHERE  T1.I1 < ( SELECT SUM(I1) FROM T1 A
 *                         WHERE  A.I1 < ( SELECT SUM(B.I1) FROM T2 B 
 *                                         WHERE B.I2 = T1.I1 ) );
 * 
 *    첫 번째 Subquery가 Correlated Predicate ( A.I1 < (..) )를 가지고 있고
 *    GROUP BY없는 SUM(I1)이므로 Single Row Subquery 이다.
 *    그러나, Predicate 내부에 있는 두 번째 Subquery는 Main Query의 T1을 참조한다.
 *
 *    Inner Join으로 변환되어 T1이 RIGHT에 위치하게 되면,
 *    두 번째 Subquery의 결과로 인해 T1의 결과가 중복되어 반환된다.
 *
 *
 *  Note : Lateral View를 포함할 때 Unnesting 시키지 않는 것과 같은 맥락이다.
 *
 *****************************************************************************/

    qtcNode   * sArg              = NULL;
    idBool      sExistCorrSubQArg = ID_FALSE;

    if ( QTC_HAVE_SUBQUERY( aNode ) == ID_TRUE )
    {
        if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
             == MTC_NODE_LOGICAL_CONDITION_TRUE ) 
        {
            for ( sArg = (qtcNode *)aNode->node.arguments;
                  sArg != NULL;
                  sArg = (qtcNode *)sArg->node.next )
            {
                if ( existOuterSubQueryArgument( sArg, aInnerDepInfo ) == ID_TRUE )
                {
                    // 하나라도 존재하면, 탐색을 종료한다.
                    sExistCorrSubQArg = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else
        {
            for ( sArg = (qtcNode *)aNode->node.arguments;
                  sArg != NULL;
                  sArg = (qtcNode *)sArg->node.next )
            {
                if ( isOuterRefSubquery( sArg, aInnerDepInfo ) == ID_TRUE )
                {
                    // 하나라도 존재하면, 탐색을 종료한다.
                    sExistCorrSubQArg = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return sExistCorrSubQArg;
}

idBool qmoUnnesting::isOuterRefSubquery( qtcNode   * aArg,
                                         qcDepInfo * aInnerDepInfo )
{
/*****************************************************************************
 *
 * Description : BUG-41564
 *
 *    Subquery Node가 주어진 Dependency 밖을 참조하는지 확인한다.
 *    이런 경우에만 TRUE, 나머지는 FALSE를 반환한다.
 *
 *    Predicate의 Subquery Argument, Target Subquery에 대해 호출한다.
 *
 *****************************************************************************/

    idBool sOuterSubQArg = ID_FALSE;

    if ( aArg != NULL )
    {
        if ( QTC_IS_SUBQUERY( aArg ) == ID_TRUE )
        {
            if ( qtc::dependencyContains( aInnerDepInfo, &aArg->depInfo ) == ID_FALSE )
            {
                sOuterSubQArg = ID_TRUE;
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

    return sOuterSubQArg;
}

idBool
qmoUnnesting::findCountAggr4Target( qtcNode  * aTarget )
{
/*****************************************************************************
 *
 * Description : BUG-45238
 *
 *    target column에서 count aggregation을 찾는다. 
 *
 *****************************************************************************/

    qtcNode   * sArg    = NULL;
    idBool      sFind   = ID_FALSE;

    /* BUG-45316 CountKeep Not unnesting */
    if ( ( aTarget->node.module == &mtfCount ) ||
         ( aTarget->node.module == &mtfCountKeep ) )
    {
        sFind = ID_TRUE;
    }
    else
    {
        if ( QTC_HAVE_AGGREGATE( aTarget ) == ID_TRUE )
        {
            for ( sArg = (qtcNode *)aTarget->node.arguments;
                  sArg != NULL;
                  sArg = (qtcNode *)sArg->node.next )
            {
                if ( findCountAggr4Target( sArg ) == ID_TRUE )
                {
                    // 하나라도 존재하면, 탐색을 종료한다.
                    sFind = ID_TRUE;
                    break;
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
    }

    return sFind;
}

void qmoUnnesting::isAggrExistTransformable( qtcNode   * aNode,
                                             qcDepInfo * aOuterDep,
                                             idBool    * aIsTrue )
{
    if ( *aIsTrue == ID_TRUE )
    {
        if ( ( aNode->node.lflag & MTC_NODE_COMPARISON_MASK )
             == MTC_NODE_COMPARISON_TRUE )
        {
            if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                  != MTC_NODE_OPERATOR_EQUAL )
            {
                if ( qtc::dependencyContains( &aNode->depInfo, aOuterDep )
                     == ID_TRUE )
                {
                    *aIsTrue = ID_FALSE;
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
        else
        {
            /* Nothing to do */
        }

        if ( aNode->node.arguments != NULL )
        {
            isAggrExistTransformable( ( qtcNode * )aNode->node.arguments, 
                                      aOuterDep,
                                      aIsTrue );
        }
        else
        {
            /* Nothing to do */
        }

        if ( aNode->node.next != NULL )
        {
            isAggrExistTransformable( ( qtcNode * )aNode->node.next,
                                      aOuterDep,
                                      aIsTrue );
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

