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
 * Description : ORDER BY Elimination Transformation
 *
 * 용어 설명 :
 *
 *****************************************************************************/

#include <ide.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmoOBYETransform.h>

extern mtfModule mtfCount;

IDE_RC qmoOBYETransform::doTransform( qcStatement  * aStatement,
                                      qmsParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : BUG-41183 Inline view 의 불필요한 ORDER BY 제거
 *               BUG-48941 where/having 절의 서브쿼리 프리디킷의 인라인뷰에 ORDER BY 제거
 * Implementation :
 *
 *       다음 조건을 만족할 경우 inline view 의 order by 절을 제거한다.
 *
 *       Mode1 BUG-41183 
 *        - SELECT count(*) 구문에 한해
 *        - FROM 절의 모든 inline view 에 대해
 *        - LIMIT 절이 없을 경우
 *       Mode2 BUG-48941 
 *        - WHERE/HAVING절의 SUBQUERY에 한해
 *        - SUBQUERY에 LIMIT/TOP/ROWNUM/LEVEL이 없고
 *        - SUBQUERY의 inline view에 대해 ( create view / recursive with view 제외)
 *        - inline view 안에 아래의 조건이 모두 없는 경우
 *            Set OP ( 동일한 이유로 recursive with도 안됨)
 *            group by/having 절
 *            LIMIT/TOP절
 *            ROWNUM/LEVEL 의사컬럼
 *            aggregation/nested aggregation/window function
 *
 ***********************************************************************/
    UInt   sOBYEproperty;

    sOBYEproperty = QCU_OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE;

    // environment의 기록
    qcgPlan::registerPlanProperty(
        aStatement,
        PLAN_PROPERTY_OPTIMIZER_ORDER_BY_ELIMINATION_ENABLE );

    if ( sOBYEproperty > 0)
    {
        if ( (sOBYEproperty & QCU_OBYE_MASK_MODE1) == QCU_OBYE_MASK_MODE1 )
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_OBYE_1_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_OBYE_1_TRUE;
        }
        else
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_OBYE_1_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_OBYE_1_FALSE;
        }

        if ( (sOBYEproperty & QCU_OBYE_MASK_MODE2)
             == QCU_OBYE_MASK_MODE2 )
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_OBYE_2_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_OBYE_2_TRUE;
        }
        else
        {
            QC_SHARED_TMPLATE(aStatement)->flag &= ~QC_TMP_OBYE_2_MASK;
            QC_SHARED_TMPLATE(aStatement)->flag |= QC_TMP_OBYE_2_FALSE;
        }

        IDE_TEST( doTransformInternal( aStatement,
                                       aParseTree->querySet,
                                       aParseTree->limit,
                                       ID_FALSE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOBYETransform::doTransformInternal( qcStatement * aStatement,
                                              qmsQuerySet * aQuerySet,
                                              qmsLimit    * aLimit,
                                              idBool        aIsSubQPred )
{
    qmsFrom      * sFrom         = NULL;
    idBool         sIsTransMode2 = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoOBYETransform::doTransform::__FT__" );

    //------------------------------------------
    // 조건 검사
    //------------------------------------------
    if ( aQuerySet->setOp == QMS_NONE )
    {
        if ( ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_OBYE_2_MASK )
               == QC_TMP_OBYE_2_TRUE ) &&
             ( aIsSubQPred == ID_TRUE ) )
        {
            /* BUG-48941
             *  mode2일 때 parent query block의 조건 확인
             *    line view 위 모든 쿼리블록에 아래 조건이 있는경우
             *    OBYE할 수 없다. */
            if ( ( aLimit != NULL ) ||
                 ( aQuerySet->SFWGH->rownum != NULL ) ||
                 ( aQuerySet->SFWGH->level  != NULL ) ||
                 ( aQuerySet->SFWGH->top    != NULL ) )
            {
                sIsTransMode2 = ID_FALSE;
            }
            else
            {
                sIsTransMode2 = ID_TRUE; 
            }
        }

        // from
        for ( sFrom = aQuerySet->SFWGH->from;
              sFrom != NULL;
              sFrom = sFrom->next )
        {
            IDE_TEST( doTransform4FromTree( aStatement,
                                            aQuerySet,
                                            sFrom,
                                            sIsTransMode2 )
                      != IDE_SUCCESS );
        }

        // where
        IDE_TEST( doTransform4Predicate( aStatement,
                                         aQuerySet->SFWGH->where ) 
                  != IDE_SUCCESS ); 

        // having
        IDE_TEST( doTransform4Predicate( aStatement,
                                         aQuerySet->SFWGH->having ) 
                  != IDE_SUCCESS ); 
    }
    else
    {
        IDE_TEST( doTransformInternal( aStatement,
                                       aQuerySet->left,
                                       aLimit,
                                       aIsSubQPred )
                  != IDE_SUCCESS );

        IDE_TEST( doTransformInternal( aStatement,
                                       aQuerySet->right,
                                       aLimit,
                                       aIsSubQPred )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmoOBYETransform::doTransform4FromTree( qcStatement * aStatement,
                                               qmsQuerySet * aQuerySet, 
                                               qmsFrom     * aFrom,
                                               idBool        aIsTransMode2 )
{
    qmsParseTree * sParseTree    = NULL;
    qmsQuerySet  * sQuerySet     = NULL;

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        if ( aFrom->tableRef != NULL )
        {
            // recursive with view 제외
            if ( ( aFrom->tableRef->view != NULL ) &&
                 ( aFrom->tableRef->tableInfo->tableType == QCM_USER_TABLE ) )
            {
                // inline view의 parseTree/QuerySet
                sParseTree = (qmsParseTree*)(aFrom->tableRef->view->myPlan->parseTree);
                sQuerySet = sParseTree->querySet;

                IDE_TEST( doTransformInternal( aStatement,
                                               sQuerySet,
                                               sParseTree->limit,
                                               aIsTransMode2 )
                          != IDE_SUCCESS );

                // mode1
                if ( ( sParseTree->orderBy != NULL ) &&
                     ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_OBYE_1_MASK )
                       == QC_TMP_OBYE_1_TRUE ) ) 
                {
                    if ( ( aQuerySet->target->targetColumn->node.module == &mtfCount ) &&
                         ( aQuerySet->target->next == NULL ) &&
                         ( sParseTree->limit == NULL ) &&
                         ( sQuerySet->setOp == QMS_NONE ) )
                    {
                        sParseTree->orderBy = NULL;
                    }
                }

                if ( ( sParseTree->orderBy != NULL ) && 
                     ( aIsTransMode2 == ID_TRUE ) )
                {
                    // mode2일 때 Inline view의 조건 확인
                    // inlin view가 With view인 경우 제외
                    if ( ( ( aFrom->tableRef->flag & QMS_TABLE_REF_WITH_VIEW_MASK )
                           == QMS_TABLE_REF_WITH_VIEW_FALSE ) &&
                         ( canTranMode2ForInlineView( sParseTree ) == ID_TRUE ) )
                    {
                        sParseTree->orderBy = NULL;
                    }
                }
            }
        }
    }
    else
    {
        IDE_TEST( doTransform4FromTree( aStatement,
                                        aQuerySet,
                                        aFrom->left,
                                        aIsTransMode2)
                  != IDE_SUCCESS );

        IDE_TEST( doTransform4FromTree( aStatement,
                                        aQuerySet,
                                        aFrom->right,
                                        aIsTransMode2 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoOBYETransform::doTransform4Predicate( qcStatement * aStatement,
                                            qtcNode     * aNode )
{
    qmsParseTree * sSubParseTree = NULL;
    qmsQuerySet  * sSubQuerySet  = NULL;
    qtcNode      * sNode;

    if ( aNode != NULL )
    {
        if ( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK ) == QTC_NODE_SUBQUERY_EXIST )
        {
            if ( QTC_IS_SUBQUERY(aNode) == ID_TRUE )
            {
                sSubParseTree = (qmsParseTree*)(aNode->subquery->myPlan->parseTree);
                sSubQuerySet = sSubParseTree->querySet;

                if ( sSubQuerySet->setOp == QMS_NONE )
                { 
                    IDE_TEST( doTransformInternal( aStatement,
                                                   sSubQuerySet,
                                                   sSubParseTree->limit,
                                                   ID_TRUE )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( doTransformInternal( aStatement,
                                                   sSubQuerySet->left,
                                                   sSubParseTree->limit,
                                                   ID_TRUE )
                              != IDE_SUCCESS );

                    IDE_TEST( doTransformInternal( aStatement,
                                                   sSubQuerySet->right,
                                                   sSubParseTree->limit,
                                                   ID_TRUE )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                for( sNode = (qtcNode *)aNode->node.arguments;
                     sNode != NULL;
                     sNode = (qtcNode *)sNode->node.next )
                {
                    IDE_TEST( doTransform4Predicate( aStatement,
                                                     sNode )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoOBYETransform::canTranMode2ForInlineView( qmsParseTree * aParseTree )
{
    qmsSFWGH       * sSFWGH;
    qmsTarget      * sTarget;

    if ( aParseTree->querySet->setOp != QMS_NONE )
    {
        IDE_CONT( INVALID_FORM );
    }

    sSFWGH = aParseTree->querySet->SFWGH;

    if ( ( aParseTree->limit  != NULL ) ||
         ( sSFWGH->rownum     != NULL ) ||
         ( sSFWGH->level      != NULL ) ||
         ( sSFWGH->top        != NULL ) ||
         ( sSFWGH->group      != NULL ) ||
         ( sSFWGH->having     != NULL ) ||
         ( sSFWGH->aggsDepth1 != NULL ) ||
         ( sSFWGH->aggsDepth2 != NULL ) )
    {
        IDE_CONT( INVALID_FORM );
    }

    // SELECT절 검사
    for ( sTarget = sSFWGH->target;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        if ( ( sTarget->targetColumn->lflag & QTC_NODE_ANAL_FUNC_MASK )
             == QTC_NODE_ANAL_FUNC_EXIST )
        {
            // Window function을 사용한 경우
            IDE_CONT( INVALID_FORM );
        }
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( INVALID_FORM );

    return ID_FALSE;
}
