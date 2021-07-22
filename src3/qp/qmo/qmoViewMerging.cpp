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
 * $Id: qmoViewMerging.cpp 23857 2008-03-19 02:36:53Z sungminee $
 **********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qmvOrderBy.h>
#include <qmoNormalForm.h>
#include <qmoViewMerging.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmv.h>
#include <qmsDefaultExpr.h>
#include <qmv.h>
#include <sdi.h> /* TASK-7219 Shard Transformer Refactoring */

IDE_RC
qmoViewMerging::doTransform( qcStatement  * aStatement,
                             qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     전체 query의 parseTree에 대하여 View Merging을 수행하고
 *     transformed parseTree를 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoViewMerging::doTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    //------------------------------------------
    // Simple View Merging 수행
    //------------------------------------------

    if ( QCU_OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE == 0 )
    {
        // Simple View Merge를 수행한다.
        IDE_TEST( processTransform( aStatement,
                                    aStatement->myPlan->parseTree,
                                    aQuerySet )
                  != IDE_SUCCESS );

        // merge된 동일 view reference를 제거한다.
        IDE_TEST( modifySameViewRef( aStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_SIMPLE_VIEW_MERGE_DISABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::doTransformSubqueries( qcStatement * aStatement,
                                       qtcNode     * aNode )
{
    IDU_FIT_POINT_FATAL( "qmoViewMerging::doTransformSubqueries::__FT__" );

    if ( QCU_OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE == 0 )
    {
        IDE_TEST( processTransformForExpression( aStatement,
                                                 aNode )
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

IDE_RC
qmoViewMerging::processTransform( qcStatement  * aStatement,
                                  qcParseTree  * aParseTree,
                                  qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     query block에 대하여 subquery를 제외한 모든 query set을
 *     bottom-up으로 순회하며 Simple View Merging을 수행한다.
 *     (query block이란 qmsParseTree를 의미한다.)
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool         sIsTransformed;
    qmsParseTree * sParseTree;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::processTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet  != NULL );

    /* TASK-7219 원격지에서 수행할 Shard View에는 View Mergingg을 수행하지 않는다. */
    IDE_TEST_CONT( ( ( aParseTree->stmtShard != QC_STMT_SHARD_NONE )
                     &&
                     ( aParseTree->stmtShard != QC_STMT_SHARD_META ) ),
                   NORMAL_EXIT );

    //------------------------------------------
    // Simple View Merging의 수행
    //------------------------------------------

    IDE_TEST( processTransformForQuerySet( aStatement,
                                           aQuerySet,
                                           & sIsTransformed )
              != IDE_SUCCESS );

    /* TASK-7219 */
    //------------------------------------------
    // ORDER-BY 절의 validation 수행
    //------------------------------------------
    if ( sIsTransformed == ID_TRUE )
    {
        sParseTree = (qmsParseTree *)aParseTree;

        /* parseTree에 노드변화가 발생되었음을 기록한다. */
        sParseTree->isTransformed = ID_TRUE;

        /* SET연산이 있는 경우 ORDER-BY 절은 변경되지 않는다. */
        if ( aQuerySet->setOp == QMS_NONE )
        {
            IDE_TEST( validateOrderBy( aStatement,
                                       sParseTree )
                      != IDE_SUCCESS );
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

    /* TASK-7219 */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::processTransformForQuerySet( qcStatement  * aStatement,
                                             qmsQuerySet  * aQuerySet,
                                             idBool       * aIsTransformed )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     query set에 대하여 subquery를 제외한 모든 query set을
 *     bottom-up으로 순회하며 Simple View Merging을 수행한다.
 *
 * Implementation :
 *     현재 query set이 가진 하위 view를 순회하며
 *     (1) view가 simple view인지, merge 가능한지 검사한다.
 *     (2) view를 merge한다.
 *     (3) merge된 view를 제거한다.
 *     (4) 현재 query set에 대하여 다시 validation을 수행한다.
 *
 ***********************************************************************/

    qmsQuerySet  * sCurrentQuerySet;
    qmsSFWGH     * sCurrentSFWGH;
    qmsParseTree * sUnderBlock;
    qmsQuerySet  * sUnderQuerySet;
    qmsFrom      * sFrom;
    qmsTarget    * sTarget;
    idBool         sIsTransformed;
    idBool         sIsSimpleQuery;
    idBool         sCanMergeView;
    idBool         sIsMerged;
    idBool         sIsMergedForShard = ID_FALSE;  /* TASK-7219 Shard Transformer Refactoring */

    IDU_FIT_POINT_FATAL( "qmoViewMerging::processTransformForQuerySet::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aIsTransformed != NULL );
    
    //------------------------------------------
    // 초기화
    //------------------------------------------

    sCurrentQuerySet = aQuerySet;
    sIsTransformed = ID_FALSE;
    
    //------------------------------------------
    // Simple View Merging의 수행
    //------------------------------------------
    
    if ( sCurrentQuerySet->setOp == QMS_NONE )
    {
        sCurrentSFWGH = (qmsSFWGH *)sCurrentQuerySet->SFWGH;

        // Subquery들을 모두 찾아 view merging을 먼저 시도한다.

        // SELECT절의 subquery를 찾아 view merging 시도
        for( sTarget = sCurrentSFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next )
        {
            IDE_TEST( processTransformForExpression( aStatement, sTarget->targetColumn )
                      != IDE_SUCCESS );
        }

        // WHERE절의 subquery를 찾아 view merging 시도
        IDE_TEST( processTransformForExpression( aStatement, sCurrentSFWGH->where )
                  != IDE_SUCCESS );

        // HAVING절의 subquery를 찾아 view merging 시도
        IDE_TEST( processTransformForExpression( aStatement, sCurrentSFWGH->having )
                  != IDE_SUCCESS );

        if( sCurrentSFWGH->hierarchy != NULL )
        {
            // START WITH 절의 subquery를 찾아 view merging 시도
            IDE_TEST( processTransformForExpression( aStatement, sCurrentSFWGH->hierarchy->startWith )
                      != IDE_SUCCESS );

            // CONNECT BY 절의 subquery를 찾아 view merging 시도
            IDE_TEST( processTransformForExpression( aStatement, sCurrentSFWGH->hierarchy->connectBy )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // FROM절의 각 view를 merging 시도
        for ( sFrom = sCurrentSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
        {
            if ( sFrom->joinType == QMS_NO_JOIN )
            {
                if ( sFrom->tableRef->view != NULL )
                {
                    sUnderBlock = (qmsParseTree *) sFrom->tableRef->view->myPlan->parseTree;
                    sUnderQuerySet = sUnderBlock->querySet;

                    // 하위 view에 대하여 bottom-up으로 순회하며
                    // Simple View Merging을 수행한다.
                    IDE_TEST( processTransform( aStatement,
                                                &(sUnderBlock->common),
                                                sUnderQuerySet )
                              != IDE_SUCCESS );

                    //------------------------------------------
                    // (1) Simple View & Merge 가능 검사
                    //------------------------------------------

                    // 이미 view이므로 simple query인지만 검사한다.
                    IDE_TEST( isSimpleQuery( sUnderBlock,
                                             & sIsSimpleQuery )
                              != IDE_SUCCESS );
                    
                    if ( sIsSimpleQuery  == ID_TRUE )
                    {
                        // merge 가능한지 검사한다.
                        // 현재 querySet과 하위 querySet은 SET이 없다.
                        IDE_TEST( canMergeView( aStatement,
                                                sCurrentQuerySet->SFWGH,
                                                sUnderQuerySet->SFWGH,
                                                sFrom->tableRef,
                                                & sCanMergeView )
                                  != IDE_SUCCESS );
                        
                        if ( sCanMergeView == ID_TRUE )
                        {
                            /* TASK-7219 Shard Transformer Refactoring */
                            if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                                SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
                                 == ID_TRUE )
                            {
                                sIsMergedForShard = sUnderQuerySet->SFWGH->isTransformed;

                                sUnderQuerySet->SFWGH->isTransformed = ID_FALSE;

                                IDE_TEST( sdi::preAnalyzeQuerySet( aStatement,
                                                                   sUnderQuerySet,
                                                                   QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ) )
                                          != IDE_SUCCESS );

                                sUnderQuerySet->SFWGH->isTransformed = sIsMergedForShard;
                            }
                            else
                            {
                                /* Nothing to do */
                            }

                            //------------------------------------------
                            // (2) Merge를 수행한다.
                            //------------------------------------------
                            
                            IDE_TEST( processMerging( aStatement,
                                                      sCurrentQuerySet->SFWGH,
                                                      sUnderQuerySet->SFWGH,
                                                      sFrom,
                                                      & sIsMerged )
                                      != IDE_SUCCESS );
                            
                            if ( sIsMerged == ID_TRUE )
                            {
                                // 하위 SFWGH가 현재 SFWGH로
                                // merge되었음을 표시한다.
                                sUnderQuerySet->SFWGH->mergedSFWGH =
                                    sCurrentQuerySet->SFWGH;
                                
                                sFrom->tableRef->isMerged = ID_TRUE;

                                sIsTransformed = ID_TRUE;

                                // PROJ-2462 Result Cache
                                sCurrentQuerySet->lflag |= sUnderQuerySet->lflag & QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
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
                IDE_TEST( processTransformForJoinedTable( aStatement,
                                                          sFrom )
                          != IDE_SUCCESS );
            }
        }

        if ( sIsTransformed == ID_TRUE )
        {
            //------------------------------------------
            // (3) Merge된 View를 제거한다.
            //------------------------------------------
            
            IDE_TEST( removeMergedView( sCurrentQuerySet->SFWGH )
                      != IDE_SUCCESS );

            // SFWGH에 노드변화가 발생되었음을 기록한다.
            sCurrentQuerySet->SFWGH->isTransformed = ID_TRUE;

            //------------------------------------------
            // (4) validation을 수행한다.
            //------------------------------------------
            
            IDE_TEST( validateSFWGH( aStatement,
                                     sCurrentQuerySet->SFWGH )
                      != IDE_SUCCESS );

            // dependency를 재설정한다.
            qtc::dependencySetWithDep( & sCurrentQuerySet->depInfo,
                                       & sCurrentQuerySet->SFWGH->depInfo );

            // set outer column dependencies
            IDE_TEST( qmvQTC::setOuterDependencies( sCurrentQuerySet->SFWGH,
                                                    & sCurrentQuerySet->SFWGH->outerDepInfo )
                      != IDE_SUCCESS );
            
            qtc::dependencySetWithDep( & sCurrentQuerySet->outerDepInfo,
                                       & sCurrentQuerySet->SFWGH->outerDepInfo );
            
            IDE_TEST( checkViewDependency( aStatement,
                                           & sCurrentQuerySet->outerDepInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // left subquery의 simple view merging 수행
        IDE_TEST( processTransformForQuerySet( aStatement,
                                               sCurrentQuerySet->left,
                                               & sIsMerged )
                  != IDE_SUCCESS );

        /* TASK-7219 */
        if ( sIsMerged == ID_TRUE )
        {
            sIsTransformed = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        // right subquery의 simple view merging 수행
        IDE_TEST( processTransformForQuerySet( aStatement,
                                               sCurrentQuerySet->right,
                                               & sIsMerged )
                  != IDE_SUCCESS );

        /* TASK-7219 */
        if ( sIsMerged == ID_TRUE )
        {
            sIsTransformed = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        // outer column dependency는 하위 dependency를 OR-ing한다.
        qtc::dependencyClear( & sCurrentQuerySet->outerDepInfo );

        IDE_TEST( qtc::dependencyOr( & sCurrentQuerySet->left->outerDepInfo,
                                     & sCurrentQuerySet->outerDepInfo,
                                     & sCurrentQuerySet->outerDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & sCurrentQuerySet->right->outerDepInfo,
                                     & sCurrentQuerySet->outerDepInfo,
                                     & sCurrentQuerySet->outerDepInfo )
                  != IDE_SUCCESS );
    }

    *aIsTransformed = sIsTransformed;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::processTransformForJoinedTable( qcStatement  * aStatement,
                                                qmsFrom      * aFrom )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     joined table의 하위 query set을 순회하며 simple view merging를
 *     수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree * sViewParseTree;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::processTransformForJoinedTable::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aFrom != NULL );

    //------------------------------------------
    // Joined Table의 bottom-up 순회
    //------------------------------------------

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        if ( aFrom->tableRef->view != NULL )
        {
            sViewParseTree = (qmsParseTree *) aFrom->tableRef->view->myPlan->parseTree;

            //------------------------------------------
            // Simple View Merging 수행
            //------------------------------------------

            IDE_TEST( processTransform( aStatement,
                                        &(sViewParseTree->common),
                                        sViewParseTree->querySet )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        IDE_TEST( processTransformForExpression( aStatement, aFrom->onCondition )
                  != IDE_SUCCESS );

        IDE_TEST( processTransformForJoinedTable( aStatement,
                                                  aFrom->left )
                  != IDE_SUCCESS );

        IDE_TEST( processTransformForJoinedTable( aStatement,
                                                  aFrom->right )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::processTransformForExpression( qcStatement * aStatement,
                                               qtcNode     * aNode )
{
/***********************************************************************
 *
 * Description : Predicate이나 expression에 포함된 subquery를 찾아
 *               view merging을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::processTransformForExpression::__FT__" );

    if( aNode != NULL )
    {
        if( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK ) == QTC_NODE_SUBQUERY_EXIST )
        {
            if( aNode->node.module == &qtc::subqueryModule )
            {
                IDE_TEST( doTransform( aNode->subquery,
                                       ( (qmsParseTree *)( aNode->subquery->myPlan->parseTree ) )->querySet )
                          != IDE_SUCCESS );
            }
            else
            {
                for( sNode = (qtcNode *)aNode->node.arguments;
                     sNode != NULL;
                     sNode = (qtcNode *)sNode->node.next )
                {
                    IDE_TEST( processTransformForExpression( aStatement,
                                                             sNode )
                              != IDE_SUCCESS );
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
qmoViewMerging::isSimpleQuery( qmsParseTree * aParseTree,
                               idBool       * aIsSimpleQuery )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     Simple Query인지 검사한다.
 *
 * Implementation :
 *     (1) SELECT, FROM, WHERE 절만 있고
 *     (2) AGGREGATION이 없다.
 *     (3) target에 DISTINCT가 없고, Analytic Function도 없다.
 *     (4) target에 DISTINCT가 없다.
 *     (5) START WITH, CONNECT BY 절이 없다.
 *     (6) GROUP BY, HAVING 절이 없다.
 *     (7) ORDER BY, LIMIT 또는 LOOP 절이 없다.
 *     (8) SHARD 구문이 아니다.
 *
 ***********************************************************************/

    qmsQuerySet * sQuerySet;
    qmsSFWGH    * sSFWGH;
    idBool        sIsSimpleQuery = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::isSimpleQuery::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aParseTree != NULL );
    IDE_DASSERT( aIsSimpleQuery != NULL );

    //------------------------------------------
    // Simple Query 검사
    //------------------------------------------

    if ( ( aParseTree->orderBy == NULL ) &&
         ( aParseTree->limit == NULL ) &&
         ( aParseTree->loopNode == NULL ) &&
         ( aParseTree->common.stmtShard == QC_STMT_SHARD_NONE ) ) // PROJ-2638
    {
        sQuerySet = aParseTree->querySet;

        if ( sQuerySet->setOp == QMS_NONE )
        {
            sSFWGH = sQuerySet->SFWGH;

            if ( ( sSFWGH->selectType == QMS_ALL ) &&
                 ( sSFWGH->hierarchy == NULL ) &&
                 ( sSFWGH->top == NULL ) && /* BUG-36580 supported TOP */
                 ( sSFWGH->group == NULL ) &&
                 ( sSFWGH->having == NULL ) &&
                 ( sSFWGH->aggsDepth1 == NULL ) &&
                 ( sSFWGH->aggsDepth2 == NULL ) &&
                 ( sQuerySet->analyticFuncList == NULL ) )
            {
                sIsSimpleQuery = ID_TRUE;
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

    *aIsSimpleQuery = sIsSimpleQuery;

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::canMergeView( qcStatement  * aStatement,
                              qmsSFWGH     * aCurrentSFWGH,
                              qmsSFWGH     * aUnderSFWGH,
                              qmsTableRef  * aUnderTableRef,
                              idBool       * aCanMergeView )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     현재 SFWGH와 하위 SFWGH가 merge 가능한지 검사한다.
 *
 * Implementation :
 *     (1) Environment 조건 검사
 *     (2) 현재 SFWGH 조건 검사
 *     (3) 하위 SFWGH 조건 검사
 *     (4) Dependency 검사
 *     (5) NormalForm 검사
 *
 ***********************************************************************/

    idBool  sCanMergeView = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::canMergeView::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aUnderTableRef != NULL );
    IDE_DASSERT( aCanMergeView != NULL );

    //------------------------------------------
    // Merge 가능 조건 검사
    //------------------------------------------

    while ( 1 )
    {
        //------------------------------------------
        // (1) Environment 조건 검사
        //------------------------------------------

        // 이미 했음
        
        //------------------------------------------
        // (2) 현재 SFWGH 조건 검사
        //------------------------------------------
        
        IDE_TEST( checkCurrentSFWGH( aCurrentSFWGH,
                                     & sCanMergeView )
                  != IDE_SUCCESS );
        
        if ( sCanMergeView == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        //------------------------------------------
        // (3) 하위 SFWGH 조건 검사
        //------------------------------------------
        
        IDE_TEST( checkUnderSFWGH( aStatement,
                                   aUnderSFWGH,
                                   aUnderTableRef,
                                   & sCanMergeView )
                  != IDE_SUCCESS );
        
        if ( sCanMergeView == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        //------------------------------------------
        // (4) Dependency 검사
        //------------------------------------------
        
        IDE_TEST( checkDependency( aCurrentSFWGH,
                                   aUnderSFWGH,
                                   & sCanMergeView )
                  != IDE_SUCCESS );
            
        if ( sCanMergeView == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        //------------------------------------------
        // (5) NormalForm 검사
        //------------------------------------------
        
        IDE_TEST( checkNormalForm( aStatement,
                                   aCurrentSFWGH,
                                   aUnderSFWGH,
                                   & sCanMergeView )
                  != IDE_SUCCESS );
            
        if ( sCanMergeView == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }

        break;
    }
    
    *aCanMergeView = sCanMergeView;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::checkCurrentSFWGH( qmsSFWGH     * aSFWGH,
                                   idBool       * aCanMerge )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     현재 query block의 merge 조건을 검사한다.
 *
 * Implementation :
 *     (1) hint 검사
 *     (2) pseudo column 검사
 *
 ***********************************************************************/

    idBool  sCanMerge = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::checkCurrentSFWGH::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aSFWGH != NULL );
    IDE_DASSERT( aCanMerge != NULL );

    //------------------------------------------
    // 현재 SFWGH 검사
    //------------------------------------------
    
    while ( 1 )
    {
        //------------------------------------------
        // hint 검사
        //------------------------------------------
        
        // dnf
        if ( aSFWGH->hints->normalType == QMO_NORMAL_TYPE_DNF )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    
        // ordered
        if ( aSFWGH->hints->joinOrderType == QMO_JOIN_ORDER_TYPE_ORDERED )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        //------------------------------------------
        // pseudo column 검사
        //------------------------------------------

        // BUG-37314 서브쿼리에 rownum 이 있는 경우에만 unnest 를 제한해야 한다.
        if( ( aSFWGH->outerQuery != NULL ) &&
            ( aSFWGH->rownum     != NULL ) )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        //------------------------------------------
        // PROJ-1653 Outer Join Operator (+)
        //------------------------------------------

        if ( aSFWGH->where != NULL )
        {
            if( ( aSFWGH->where->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                sCanMerge = ID_FALSE;
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

        /*
         * PROJ-1715 Hierarchy Query Exstension
         */
        if ( aSFWGH->hierarchy != NULL )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
        break;
    }
    
    *aCanMerge = sCanMerge;

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::checkUnderSFWGH( qcStatement  * aStatement,
                                 qmsSFWGH     * aSFWGH,
                                 qmsTableRef  * aTableRef,
                                 idBool       * aCanMerge )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     하위 query block의 merge 조건을 검사한다.
 *
 *     [Enhancement]
 *     view target에 subquery, user-defined function 등 merge할 수 없는
 *     simple view라 하더라도 해당 target을 참조하지 않는 경우에는
 *     merge가 가능하도록 수정한다.
 *
 *     ex) select count(*) from ( select func1(i1) from t1 ) v1;
 *         -> select count(*) from t1;
 *
 * Implementation :
 *     (1) hint 검사
 *     (2) pseudo column 검사
 *     (3) performance view 검사
 *     (4) target list 검사
 *     (5) disk table 검사
 *
 ***********************************************************************/

    qmsTarget         * sViewTarget = NULL;
    idBool              sCanMerge   = ID_TRUE;
    qmsColumnRefList  * sColumnRef  = NULL;
    mtcColumn         * sViewColumn = NULL;
    UShort              sViewColumnOrder;
    UShort              sTargetOrder;
    qmsFrom           * sViewFrom   = NULL;
    qmsNoMergeHints   * sNoMergeHint;
    qcDepInfo           sDepInfo;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::checkUnderSFWGH::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSFWGH != NULL );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aCanMerge != NULL );

    //------------------------------------------
    // 하위 SFWGH 검사
    //------------------------------------------

    while ( 1 )
    {
        //------------------------------------------
        // hint 검사
        //------------------------------------------
        
        // currentSFWGH에서 현재 view에 대하여 no_merge,
        // push_selection_view, push_pred를 명시한 경우
        if ( aTableRef->noMergeHint == ID_TRUE )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        // underSFWGH에서 push_selection_view를 사용한 경우
        if ( aSFWGH->hints->viewOptHint != NULL )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        // underSFWGH에서 push_pred를 사용한 경우
        if ( aSFWGH->hints->pushPredHint != NULL )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }    
        
        // dnf
        if ( aSFWGH->hints->normalType == QMO_NORMAL_TYPE_DNF )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        // ordered
        if ( aSFWGH->hints->joinOrderType == QMO_JOIN_ORDER_TYPE_ORDERED )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        // BUG-43536 no_merge() 힌트 지원
        for( sNoMergeHint = aSFWGH->hints->noMergeHint;
             sNoMergeHint != NULL;
             sNoMergeHint = sNoMergeHint->next )
        {
            if ( sNoMergeHint->table == NULL )
            {
                sCanMerge = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        //------------------------------------------
        // pseudo column 검사
        //------------------------------------------
        
        if ( aSFWGH->rownum != NULL )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        if ( aSFWGH->level != NULL )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        /* PROJ-1715 */
        if ( aSFWGH->isLeaf != NULL )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            /* Nothing to do */
        }

        //------------------------------------------
        // performance view 검사
        //------------------------------------------
        
        // performance view는 merge할 수 없다.
        if ( aTableRef->tableInfo->tableType == QCM_PERFORMANCE_VIEW )
        {
            sCanMerge = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        /* TASK-7219 Shard Transformer Refactoring */
        if ( ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                              SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
               == ID_TRUE )
             &&
             ( aTableRef->withStmt != NULL ) )
        {
            sCanMerge = ID_FALSE;

            break;
        }
        else
        {
            /* Nothing to do */
        }


        //------------------------------------------
        // PROJ-1653 Outer Join Operator (+)
        //------------------------------------------

        if ( aSFWGH->where != NULL )
        {
            if( ( aSFWGH->where->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                == QTC_NODE_JOIN_OPERATOR_EXIST )
            {
                sCanMerge = ID_FALSE;
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
        
        // PROJ-2418
        // UnderSFWGH의 From에서 Lateral View가 존재하면 Merging 할 수 없다.
        // 단, Lateral View가 이전에 모두 Merging 되었다면 Merging이 가능하다.
        for ( sViewFrom = aSFWGH->from; sViewFrom != NULL; sViewFrom = sViewFrom->next )
        {
            IDE_TEST( qmvQTC::getFromLateralDepInfo( sViewFrom, & sDepInfo )
                      != IDE_SUCCESS );

            if ( qtc::haveDependencies( & sDepInfo ) == ID_TRUE )
            {
                // 해당 From에 Lateral View가 존재하면 Merging 불가
                sCanMerge = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        //------------------------------------------
        // 상위 query block에서 참조하는 target column  검사
        //------------------------------------------

        // BUGBUG 동일한 view 컬럼을 중복검사할 수 있다.
        for ( sColumnRef = aTableRef->viewColumnRefList;
              sColumnRef != NULL;
              sColumnRef = sColumnRef->next )
        {
            if ( sColumnRef->column->node.module == & qtc::passModule )
            {
                // view 참조 컬럼이었다가 passNode로 바뀐경우
                
                // Nothing to do.
            }
            else
            {
                IDE_DASSERT( sColumnRef->column->node.module == & qtc::columnModule );
                
                sViewColumn = QTC_STMT_COLUMN( aStatement, sColumnRef->column );
                sViewColumnOrder = sViewColumn->column.id & SMI_COLUMN_ID_MASK;
                
                sTargetOrder = 0;
                for ( sViewTarget = aSFWGH->target;
                      sViewTarget != NULL;
                      sViewTarget = sViewTarget->next )
                {
                    if ( sTargetOrder == sViewColumnOrder )
                    {
                        break;
                    }
                    else
                    {
                        sTargetOrder++;
                    }
                }
            
                IDE_TEST_RAISE( sViewTarget == NULL, ERR_COLUMN_NOT_FOUND );

                // (1) subquery 포함하지 않는다.
                if ((sViewTarget->targetColumn->lflag & QTC_NODE_SUBQUERY_MASK)
                    == QTC_NODE_SUBQUERY_EXIST)
                {
                    sCanMerge = ID_FALSE;
                    break;
                }

                // (2) user-defined function을 포함하지 않는다.
                if ( ( sViewTarget->targetColumn->lflag & QTC_NODE_PROC_FUNCTION_MASK )
                     == QTC_NODE_PROC_FUNCTION_TRUE )
                {
                    sCanMerge = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            
                // (3) variable build-in function을 포함하지 않는다.
                if ( ( sViewTarget->targetColumn->lflag & QTC_NODE_VAR_FUNCTION_MASK )
                     == QTC_NODE_VAR_FUNCTION_EXIST )
                {
                    sCanMerge = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                /* (4) _prowid 를 포함하지 않는다. (BUG-41218) */
                if ( ( sViewTarget->targetColumn->lflag & QTC_NODE_COLUMN_RID_MASK )
                     == QTC_NODE_COLUMN_RID_EXIST)
                {
                    sCanMerge = ID_FALSE;
                    break;
                }
                else
                {
                    /* nothing to do */
                }

                /* TASK-7219 Shard Transformer Refactoring
                 *  - (5) Bind
                 */
                if ( ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                      SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
                       == ID_TRUE )
                     &&
                     ( MTC_NODE_IS_DEFINED_TYPE( &( sViewTarget->targetColumn->node ) )
                       == ID_FALSE ) )
                {
                    sCanMerge = ID_FALSE;

                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }

        break;
    }
        
    *aCanMerge = sCanMerge;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoViewMerging::checkUnderSFWGH",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::checkDependency( qmsSFWGH     * aCurrentSFWGH,
                                 qmsSFWGH     * aUnderSFWGH,
                                 idBool       * aCanMerge )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     merge후 최대 참조 relation의 수를 넘지 않는지 검사한다.
 *
 * Implementation :
 *     상세설계시 각 SFWGHO에 대하여 각각 dependency 정보를 검사하려고
 *     했으나 코드 복잡도만 높아지고, 검사조건으로 정확하게 검사할
 *     필요가 없어, 크게 SFWGH 전체에 대한 dependency 정보만으로
 *     계산한다. SFWGH의 dependency는 SFWGH 내의 inner dependency와
 *     outer dependency의 합으로 계산한다.
 *
 *     그리고, order-by 절에 대해서는 outer dependency를 가질 수 없고,
 *     SFWGH dependency에 속하므로 별도로 고려할 필요가 없다.
 *
 ***********************************************************************/

    UInt       sDepCount;
    idBool     sCanMerge = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::checkDependency::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aCanMerge != NULL );

    //------------------------------------------
    // dependency 검사
    //------------------------------------------

    // SFWGH의 dependency
    sDepCount = aCurrentSFWGH->depInfo.depCount +
        aUnderSFWGH->depInfo.depCount;

    // SFWGH의 outer dependency
    sDepCount += aCurrentSFWGH->outerDepInfo.depCount +
        aUnderSFWGH->outerDepInfo.depCount;

    IDE_DASSERT( sDepCount > 0 );
    
    // merge로 제거될 view의 dependency를 하나 뺀다.
    sDepCount -= 1;
    
    // merge후 가질 최대 dependency로 검사한다.
    if ( sDepCount > QC_MAX_REF_TABLE_CNT )
    {
        sCanMerge = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }
    
    *aCanMerge = sCanMerge;

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::checkNormalForm( qcStatement  * aStatement,
                                 qmsSFWGH     * aCurrentSFWGH,
                                 qmsSFWGH     * aUnderSFWGH,
                                 idBool       * aCanMerge )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     merge후 predicate의 normal form maximum을 검사한다.
 *
 * Implementation :
 *     두 where절이 AND로 연결될 것이므로 추정 값을 더하여 비교한다.
 *
 ***********************************************************************/

    UInt       sCurrentEstimateCnfCnt = 0;
    UInt       sUnderEstimateCnfCnt = 0;
    UInt       sNormalFormMaximum;
    idBool     sCanMerge = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::checkNormalForm::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aCanMerge != NULL );

    //------------------------------------------
    // normal form 검사
    //------------------------------------------

    if ( aCurrentSFWGH->where != NULL )
    {
        IDE_TEST( qmoNormalForm::estimateCNF( aCurrentSFWGH->where,
                                              & sCurrentEstimateCnfCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( aUnderSFWGH->where != NULL )
    {
        IDE_TEST( qmoNormalForm::estimateCNF( aUnderSFWGH->where,
                                              & sUnderEstimateCnfCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    sNormalFormMaximum = QCG_GET_SESSION_NORMALFORM_MAXIMUM( aStatement );

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_NORMAL_FORM_MAXIMUM );
    
    // and로 연결될 것으므로 더하면 된다.
    // 혹은 어느 하나라도 normalFormMaxinum을 넘으면 merge하지 않는다.
    if ( sCurrentEstimateCnfCnt + sUnderEstimateCnfCnt > sNormalFormMaximum )
    {
        sCanMerge = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    *aCanMerge = sCanMerge;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::processMerging( qcStatement  * aStatement,
                                qmsSFWGH     * aCurrentSFWGH,
                                qmsSFWGH     * aUnderSFWGH,
                                qmsFrom      * aUnderFrom,
                                idBool       * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     각 절에 대하여 merge를 수행한다.
 *
 * Implementation :
 *     (1) hint 절을 merge한다.
 *     (2) from 절을 merge한다.
 *     (3) target list를 merge한다.
 *     (4) where 절을 merge한다.
 *
 ***********************************************************************/

    qmoViewRollbackInfo   sRollbackInfo;
    qmsTarget           * sTarget;
    idBool                sIsMerged = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::processMerging::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aUnderFrom != NULL );
    IDE_DASSERT( aIsMerged != NULL );

    //------------------------------------------
    // merge 수행
    //------------------------------------------

    sRollbackInfo.hintMerged   = ID_FALSE;
    sRollbackInfo.targetMerged = ID_FALSE;
    sRollbackInfo.fromMerged   = ID_FALSE;
    sRollbackInfo.whereMerged  = ID_FALSE;

    while ( 1 )
    {
        //------------------------------------------
        // hint 절의 merge 수행
        //------------------------------------------

        IDE_TEST( mergeForHint( aCurrentSFWGH,
                                aUnderSFWGH,
                                aUnderFrom,
                                & sRollbackInfo,
                                & sIsMerged )
                  != IDE_SUCCESS );
        
        if ( sIsMerged == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    
        //------------------------------------------
        // from 절의 merge 수행
        //------------------------------------------
        
        IDE_TEST( mergeForFrom( aStatement,
                                aCurrentSFWGH,
                                aUnderSFWGH,
                                aUnderFrom,
                                & sRollbackInfo,
                                & sIsMerged )
                  != IDE_SUCCESS );
        
        if ( sIsMerged == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    
        //------------------------------------------
        // target list의 merge 수행
        //------------------------------------------
        
        IDE_TEST( mergeForTargetList( aStatement,
                                      aUnderSFWGH,
                                      aUnderFrom->tableRef,
                                      & sRollbackInfo,
                                      & sIsMerged )
                  != IDE_SUCCESS );
        
        if ( sIsMerged == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    
        //------------------------------------------
        // where 절의 merge 수행
        //------------------------------------------
        
        IDE_TEST( mergeForWhere( aStatement,
                                 aCurrentSFWGH,
                                 aUnderSFWGH,
                                 & sRollbackInfo,
                                 & sIsMerged )
                  != IDE_SUCCESS );
        
        if ( sIsMerged == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
        
        //------------------------------------------
        // 다음 merge를 위한 dependency 정보 설정
        //------------------------------------------
        
        // view dependency 제거
        qtc::dependencyRemove( aUnderFrom->tableRef->table,
                               & aCurrentSFWGH->depInfo,
                               & aCurrentSFWGH->depInfo );
        
        IDE_TEST( qtc::dependencyOr( & aUnderSFWGH->depInfo,
                                     & aCurrentSFWGH->depInfo,
                                     & aCurrentSFWGH->depInfo )
                  != IDE_SUCCESS );
        
        IDE_TEST( qtc::dependencyOr( & aUnderSFWGH->outerDepInfo,
                                     & aCurrentSFWGH->outerDepInfo,
                                     & aCurrentSFWGH->outerDepInfo )
                  != IDE_SUCCESS );

        //------------------------------------------
        // PROJ-2418
        // LATERAL_VIEW Flag를 Unmask 한다.
        //------------------------------------------
        aUnderFrom->tableRef->flag &= ~QMS_TABLE_REF_LATERAL_VIEW_MASK;
        aUnderFrom->tableRef->flag |= QMS_TABLE_REF_LATERAL_VIEW_FALSE;
        
        break;
    }
    
    //------------------------------------------
    // merge가 실패한 경우 rollback 수행
    //------------------------------------------

    if ( sIsMerged == ID_TRUE )
    {
        // PROJ-2179
        // Merge된 경우 ORDER BY과 SELECT절의 attribute 참조 관계가 재설정되어야 한다.
        for( sTarget = aCurrentSFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next )
        {
            sTarget->flag &= ~QMS_TARGET_ORDER_BY_MASK;
            sTarget->flag |= QMS_TARGET_ORDER_BY_FALSE;
        }
    }
    else
    {
        IDE_TEST( rollbackForWhere( aCurrentSFWGH,
                                    aUnderSFWGH,
                                    & sRollbackInfo )
                  != IDE_SUCCESS );
            
        IDE_TEST( rollbackForTargetList( aUnderFrom->tableRef,
                                         & sRollbackInfo )
                  != IDE_SUCCESS );
            
        IDE_TEST( rollbackForFrom( aCurrentSFWGH,
                                   aUnderSFWGH,
                                   & sRollbackInfo )
                  != IDE_SUCCESS );
            
        IDE_TEST( rollbackForHint( aCurrentSFWGH,
                                   & sRollbackInfo )
                  != IDE_SUCCESS );
    }
    
    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::mergeForHint( qmsSFWGH            * aCurrentSFWGH,
                              qmsSFWGH            * aUnderSFWGH,
                              qmsFrom             * aUnderFrom,
                              qmoViewRollbackInfo * aRollbackInfo,
                              idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsJoinMethodHints   * sJoinMethodHint;
    qmsTableAccessHints  * sTableAccessHint;
    qmsHintTables        * sHintTable;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForHint::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    IDE_DASSERT( aIsMerged != NULL );

    //------------------------------------------
    // rollbackInfo 초기화
    //------------------------------------------

    aRollbackInfo->hintMerged = ID_FALSE;
    aRollbackInfo->lastJoinMethod = NULL;
    aRollbackInfo->lastTableAccess = NULL;
    
    //------------------------------------------
    // hint 절의 rollback 정보 설정
    //------------------------------------------

    // join method hint
    for ( sJoinMethodHint = aCurrentSFWGH->hints->joinMethod;
          sJoinMethodHint != NULL;
          sJoinMethodHint = sJoinMethodHint->next )
    {
        if ( sJoinMethodHint->next == NULL )
        {
            aRollbackInfo->lastJoinMethod = sJoinMethodHint;
        }
        else
        {
            // Nothing to do.
        }
    }

    // table access hint
    for ( sTableAccessHint = aCurrentSFWGH->hints->tableAccess;
          sTableAccessHint != NULL;
          sTableAccessHint = sTableAccessHint->next )
    {
        if ( sTableAccessHint->next == NULL )
        {
            aRollbackInfo->lastTableAccess = sTableAccessHint;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    //------------------------------------------
    // hint 절의 merge 수행
    //------------------------------------------

    // join method hint
    if ( aRollbackInfo->lastJoinMethod != NULL )
    {
        aRollbackInfo->lastJoinMethod->next =
            aUnderSFWGH->hints->joinMethod;
    }
    else
    {
        aCurrentSFWGH->hints->joinMethod =
            aUnderSFWGH->hints->joinMethod;
    }

    // PROJ-1718 Subquery unnesting
    // View에 포함된 relation이 1개인 경우 outer query에서 설정한 join method hint가
    // view merging 이후에도 유효하도록 설정한다.
    if( aUnderSFWGH->from->next == NULL )
    {
        for( sJoinMethodHint = aCurrentSFWGH->hints->joinMethod;
             sJoinMethodHint != NULL;
             sJoinMethodHint = sJoinMethodHint->next )
        {
            qtc::dependencyClear( &sJoinMethodHint->depInfo );

            // BUG-43923 NO_USE_SORT(a) 힌트를 사용하면 FATAL이 발생합니다.
            // joinTables 이 1개 이상일때도 처리가능하게 한다.
            for ( sHintTable = sJoinMethodHint->joinTables;
                  sHintTable != NULL;
                  sHintTable = sHintTable->next )
            {
                if ( sHintTable->table != NULL )
                {
                    if ( sHintTable->table == aUnderFrom )
                    {
                        sHintTable->table = aUnderSFWGH->from;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    qtc::dependencyOr( &sHintTable->table->depInfo,
                                       &sJoinMethodHint->depInfo,
                                       &sJoinMethodHint->depInfo );
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
    
    // table access hint
    if ( aRollbackInfo->lastTableAccess != NULL )
    {
        aRollbackInfo->lastTableAccess->next =
            aUnderSFWGH->hints->tableAccess;
    }
    else
    {
        aCurrentSFWGH->hints->tableAccess =
            aUnderSFWGH->hints->tableAccess;
    }

    // BUG-22236
    if ( aCurrentSFWGH->hints->interResultType
         == QMO_INTER_RESULT_TYPE_NOT_DEFINED )
    {
        // 현재 query block에 중간 결과 타입 Hint가 주어지지 않은 경우,
        // view의 중간 결과 타입 Hint를 적용한다.
        aRollbackInfo->interResultType = QMO_INTER_RESULT_TYPE_NOT_DEFINED;
        
        aCurrentSFWGH->hints->interResultType =
            aUnderSFWGH->hints->interResultType;
    }
    else
    {
        aRollbackInfo->interResultType = aCurrentSFWGH->hints->interResultType;
    }

    aRollbackInfo->hintMerged = ID_TRUE;

    // BUG-48419 for BUG-48336
    aCurrentSFWGH->lflag |= ( aUnderSFWGH->lflag & QMV_SFWGH_JOIN_MASK );

    *aIsMerged = ID_TRUE;

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::rollbackForHint( qmsSFWGH            * aCurrentSFWGH,
                                 qmoViewRollbackInfo * aRollbackInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoViewMerging::rollbackForHint::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );

    //------------------------------------------
    // hint 절의 rollback 수행
    //------------------------------------------

    if ( aRollbackInfo->hintMerged == ID_TRUE )
    {
        // join method hint
        if ( aRollbackInfo->lastJoinMethod != NULL )
        {
            aRollbackInfo->lastJoinMethod->next = NULL;
        }
        else
        {
            aCurrentSFWGH->hints->joinMethod = NULL;
        }
        
        // table access hint
        if ( aRollbackInfo->lastTableAccess != NULL )
        {
            aRollbackInfo->lastTableAccess->next = NULL;
        }
        else
        {
            aCurrentSFWGH->hints->tableAccess = NULL;
        }

        // BUG-22236
        // 중간 결과 저장 타입 hint
        if ( aRollbackInfo->interResultType ==
             QMO_INTER_RESULT_TYPE_NOT_DEFINED )
        {
            aCurrentSFWGH->hints->interResultType =
                QMO_INTER_RESULT_TYPE_NOT_DEFINED;
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::mergeForTargetList( qcStatement         * aStatement,
                                    qmsSFWGH            * aUnderSFWGH,
                                    qmsTableRef         * aUnderTableRef,
                                    qmoViewRollbackInfo * aRollbackInfo,
                                    idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsColumnRefList  * sColumnRef;
    qmsTarget         * sViewTarget;
    mtcColumn         * sViewColumn;
    UShort              sViewColumnOrder;
    UShort              sTargetOrder;
    idBool              sIsMerged = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForTargetList::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aUnderTableRef != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    IDE_DASSERT( aIsMerged != NULL );

    //------------------------------------------
    // rollbackInfo 초기화
    //------------------------------------------
    
    aRollbackInfo->targetMerged = ID_FALSE;

    //------------------------------------------
    // target list의 merge 수행
    //------------------------------------------

    for ( sColumnRef = aUnderTableRef->viewColumnRefList;
          sColumnRef != NULL;
          sColumnRef = sColumnRef->next )
    {
        if ( sColumnRef->column->node.module == & qtc::passModule )
        {
            // view 참조 컬럼이었다가 passNode로 바뀐경우

            // Nothing to do.
        }
        else
        {
            if ( ( sColumnRef->column->lflag & QTC_NODE_MERGED_COLUMN_MASK )
                 == QTC_NODE_MERGED_COLUMN_TRUE )
            {
                // BUG-23467
                // case when 같은 하나의 노드를 공유해서 사용하는 연산자의 경우
                // 이미 merge를 수행했을 수 도 있다.
                
                // Nothing to do.
            }
            else
            {
                IDE_DASSERT( sColumnRef->column->node.module == & qtc::columnModule );

                sViewColumn = QTC_STMT_COLUMN( aStatement, sColumnRef->column );
                sViewColumnOrder = sViewColumn->column.id & SMI_COLUMN_ID_MASK;
            
                sTargetOrder = 0;
                for ( sViewTarget = aUnderSFWGH->target;
                      sViewTarget != NULL;
                      sViewTarget = sViewTarget->next )
                {
                    if ( sTargetOrder == sViewColumnOrder )
                    {
                        break;
                    }
                    else
                    {
                        sTargetOrder++;
                    }
                }
            
                IDE_TEST_RAISE( sViewTarget == NULL, ERR_COLUMN_NOT_FOUND );

                if (sViewTarget->targetColumn->node.module == & qtc::columnModule)
                {
                    IDE_TEST( mergeForTargetColumn( aStatement,
                                                    sColumnRef,
                                                    sViewTarget->targetColumn,
                                                    & sIsMerged )
                              != IDE_SUCCESS );
                }
                else if ( sViewTarget->targetColumn->node.module == & qtc::valueModule )
                {
                    // view target이 상수인 경우
                
                    IDE_TEST( mergeForTargetValue( aStatement,
                                                   sColumnRef,
                                                   sViewTarget->targetColumn,
                                                   & sIsMerged )
                              != IDE_SUCCESS );
                }
                else
                {
                    // view target이 expression인 경우
                
                    IDE_TEST( mergeForTargetExpression( aStatement,
                                                        aUnderSFWGH,
                                                        sColumnRef,
                                                        sViewTarget->targetColumn,
                                                        & sIsMerged )
                              != IDE_SUCCESS );
                }
            
                if ( sIsMerged == ID_FALSE )
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

    aRollbackInfo->targetMerged = ID_TRUE;
    
    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoViewMerging::mergeForTargetList",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::mergeForTargetColumn( qcStatement         * aStatement,
                                      qmsColumnRefList    * aColumnRef,
                                      qtcNode             * aTargetColumn,
                                      idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     target이 순수 컬럼일 때 merge를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcTuple  * sTargetTuple;
    mtcColumn * sTargetColumn;
    mtcColumn * sOrgColumn;
    idBool      sIsMerged = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForTargetColumn::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aColumnRef != NULL );
    IDE_DASSERT( aTargetColumn != NULL );
    IDE_DASSERT( aIsMerged != NULL );
    
    //------------------------------------------
    // target의 rollback 정보 설정
    //------------------------------------------
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                             (void **) & aColumnRef->orgColumn )
              != IDE_SUCCESS );

    idlOS::memcpy( aColumnRef->orgColumn, aColumnRef->column,
                   ID_SIZEOF( qtcNode ) );

    //------------------------------------------
    // target의 merge 수행
    //------------------------------------------

    // 노드를 치환한다.
    idlOS::memcpy( aColumnRef->column, aTargetColumn,
                   ID_SIZEOF( qtcNode ) );

    // To fix BUG-21405
    // 순수 컬럼인 경우 새로 estimate 를 하지 않으므로
    // push projection관련 flag가 set되지 않는다.
    // 컬럼 치환을 할 때 컬럼의 flag도 ORing한다.
    // To fix BUG-21425
    // disk table에 한해 flag를 ORing한다.

    sTargetTuple  = QTC_STMT_TUPLE( aStatement, aColumnRef->column );
    sTargetColumn = QTC_TUPLE_COLUMN( sTargetTuple, aColumnRef->column );

    if( ( ( sTargetTuple->lflag & MTC_TUPLE_TYPE_MASK ) ==
          MTC_TUPLE_TYPE_TABLE ) &&
        ( ( sTargetTuple->lflag & MTC_TUPLE_STORAGE_MASK ) ==
          MTC_TUPLE_STORAGE_DISK ) )
    {
        sOrgColumn = QTC_STMT_COLUMN( aStatement, aColumnRef->orgColumn );

        sTargetColumn->flag |= sOrgColumn->flag;
    }
    else
    {
        // Nothing to do.
    }
        
    // conversion 노드를 옮긴다.
    aColumnRef->column->node.conversion = aColumnRef->orgColumn->node.conversion;
    aColumnRef->column->node.leftConversion = aColumnRef->orgColumn->node.leftConversion;

    // next를 옮긴다.
    aColumnRef->column->node.next = aColumnRef->orgColumn->node.next;
    
    // name을 설정한다.
    SET_POSITION( aColumnRef->column->userName, aColumnRef->orgColumn->userName );
    SET_POSITION( aColumnRef->column->tableName, aColumnRef->orgColumn->tableName );
    SET_POSITION( aColumnRef->column->columnName, aColumnRef->orgColumn->columnName );

    // flag를 설정한다.
    aColumnRef->column->lflag &= ~QTC_NODE_MERGED_COLUMN_MASK;
    aColumnRef->column->lflag |= QTC_NODE_MERGED_COLUMN_TRUE;
        
    aColumnRef->isMerged = ID_TRUE;
    
    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::mergeForTargetValue( qcStatement         * aStatement,
                                     qmsColumnRefList    * aColumnRef,
                                     qtcNode             * aTargetColumn,
                                     idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     target이 상수일 때 merge를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool      sIsMerged = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForTargetValue::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aColumnRef != NULL );
    IDE_DASSERT( aTargetColumn != NULL );
    IDE_DASSERT( aIsMerged != NULL );
    
    //------------------------------------------
    // target의 rollback 정보 설정
    //------------------------------------------
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                             (void **) & aColumnRef->orgColumn )
              != IDE_SUCCESS );

    idlOS::memcpy( aColumnRef->orgColumn, aColumnRef->column,
                   ID_SIZEOF( qtcNode ) );

    //------------------------------------------
    // target의 merge 수행
    //------------------------------------------

    // 노드를 치환한다.
    idlOS::memcpy( aColumnRef->column, aTargetColumn,
                   ID_SIZEOF( qtcNode ) );
 
    // conversion 노드를 옮긴다.
    aColumnRef->column->node.conversion = aColumnRef->orgColumn->node.conversion;
    aColumnRef->column->node.leftConversion = aColumnRef->orgColumn->node.leftConversion;
    
    // next를 옮긴다.
    aColumnRef->column->node.next = aColumnRef->orgColumn->node.next;
    
    // flag를 설정한다.
    aColumnRef->column->lflag &= ~QTC_NODE_MERGED_COLUMN_MASK;
    aColumnRef->column->lflag |= QTC_NODE_MERGED_COLUMN_TRUE;
    
    aColumnRef->isMerged = ID_TRUE;
    
    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::mergeForTargetExpression( qcStatement         * aStatement,
                                          qmsSFWGH            * aUnderSFWGH,
                                          qmsColumnRefList    * aColumnRef,
                                          qtcNode             * aTargetColumn,
                                          idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     target이 expression일 때 merge를 수행한다.
 *
 * Implementation :
 *     (1) view 컬럼 노드를 복사 생성하여 보관한다.
 *     (2) expr 노드 결과를 저장할 template 공간을 할당한다.
 *     (3) expr 노드 트리를 복사 생성한다.
 *     (4) 복사 생성한 expr의 최상위 노드의 저장 공간을 변경한다.
 *     (5) view 컬럼의 conversion 노드를 expr의 최상위 노드로 옮긴다.
 *     (6) view 컬럼과 expr의 최상위 노드를 치환한다.
 *
 ***********************************************************************/

    qtcNode   * sNode[2];
    qtcNode   * sNewNode;
    idBool      sIsMerged = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForTargetExpression::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aColumnRef != NULL );
    IDE_DASSERT( aTargetColumn != NULL );
    IDE_DASSERT( aIsMerged != NULL );
    
    //------------------------------------------
    // target의 rollback 정보 설정
    //------------------------------------------
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                             (void **) & aColumnRef->orgColumn )
              != IDE_SUCCESS );

    idlOS::memcpy( aColumnRef->orgColumn, aColumnRef->column,
                   ID_SIZEOF( qtcNode ) );

    //------------------------------------------
    // target의 merge 수행
    //------------------------------------------

    // expr의 결과를 저장할 template 공간을 생성한다.
    IDE_TEST( qtc::makeNode( aStatement,
                             sNode,
                             & aTargetColumn->position,
                             (mtfModule*) aTargetColumn->node.module )
              != IDE_SUCCESS );

    // expr 노드 트리를 복사 생성한다.
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                     (qtcNode*) aTargetColumn,
                                     & sNewNode,
                                     ID_FALSE,  // root의 next는 복사하지 않는다.
                                     ID_TRUE,   // conversion을 끊는다.
                                     ID_TRUE,   // constant node까지 복사한다.
                                     ID_TRUE )  // constant node를 원복한다.
              != IDE_SUCCESS );

    // template 위치를 변경한다.
    sNewNode->node.table = sNode[0]->node.table;
    sNewNode->node.column = sNode[0]->node.column;

    // BUG-45187 view merge 시에 baseTable 설정해야 합니다.
    sNewNode->node.baseTable = sNode[0]->node.baseTable;
    sNewNode->node.baseColumn = sNode[0]->node.baseColumn;

    // estimate를 수행한다. (초기화한다.)
    IDE_TEST( qtc::estimate( sNewNode,
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             NULL,
                             aUnderSFWGH,
                             NULL )
              != IDE_SUCCESS );

    // conversion 노드를 옮긴다.
    sNewNode->node.conversion = aColumnRef->column->node.conversion;
    sNewNode->node.leftConversion = aColumnRef->column->node.leftConversion;

    // next를 옮긴다.
    sNewNode->node.next = aColumnRef->column->node.next;
    
    // 노드를 치환한다.
    idlOS::memcpy( aColumnRef->column, sNewNode,
                   ID_SIZEOF( qtcNode ) );

    // flag를 설정한다.
    aColumnRef->column->lflag &= ~QTC_NODE_MERGED_COLUMN_MASK;
    aColumnRef->column->lflag |= QTC_NODE_MERGED_COLUMN_TRUE;
    
    aColumnRef->isMerged = ID_TRUE;

    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::rollbackForTargetList( qmsTableRef         * aUnderTableRef,
                                       qmoViewRollbackInfo * aRollbackInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsColumnRefList  * sColumnRef;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::rollbackForTargetList::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aUnderTableRef != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    
    //------------------------------------------
    // target list의 rollback 수행
    //------------------------------------------

    if ( aRollbackInfo->targetMerged == ID_TRUE )
    {
        for ( sColumnRef = aUnderTableRef->viewColumnRefList;
              sColumnRef != NULL;
              sColumnRef = sColumnRef->next )
        {
            if ( sColumnRef->isMerged == ID_TRUE )
            {
                idlOS::memcpy( sColumnRef->column, sColumnRef->orgColumn,
                               ID_SIZEOF( qtcNode ) );
                
                sColumnRef->isMerged = ID_FALSE;
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
}

IDE_RC
qmoViewMerging::mergeForFrom( qcStatement         * aStatement,
                              qmsSFWGH            * aCurrentSFWGH,
                              qmsSFWGH            * aUnderSFWGH,
                              qmsFrom             * aUnderFrom,
                              qmoViewRollbackInfo * aRollbackInfo,
                              idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom        * sFrom;
    qcNamePosition * sAliasName;
    UInt             sAliasCount = 0;
    idBool           sIsCreated = ID_TRUE;
    idBool           sIsMerged = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForFrom::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aUnderFrom != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    IDE_DASSERT( aIsMerged != NULL );

    //------------------------------------------
    // rollbackInfo 초기화
    //------------------------------------------

    aRollbackInfo->fromMerged = ID_FALSE;
    aRollbackInfo->firstFrom = NULL;
    aRollbackInfo->lastFrom = NULL;
    aRollbackInfo->oldAliasName = NULL;
    aRollbackInfo->newAliasName = NULL;
    
    //------------------------------------------
    // from 절의 rollback 정보 설정
    //------------------------------------------

    // currentSFWGH
    aRollbackInfo->firstFrom = aCurrentSFWGH->from;

    // underSFWGH
    for ( sFrom = aUnderSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        if ( sFrom->joinType == QMS_NO_JOIN )
        {
            sAliasCount++;
        }
        else
        {
            // Nothing to do.
        }
        
        if ( sFrom->next == NULL )
        {
            aRollbackInfo->lastFrom = sFrom;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_FT_ASSERT( aRollbackInfo->firstFrom != NULL );
    IDE_FT_ASSERT( aRollbackInfo->lastFrom != NULL );

    if ( sAliasCount > 0 )
    {
        // old alias name의 보관
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcNamePosition) * sAliasCount,
                                                 (void **) & aRollbackInfo->oldAliasName )
                  != IDE_SUCCESS );
        
        sAliasName = aRollbackInfo->oldAliasName;
        
        for ( sFrom = aUnderSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
        {
            if ( sFrom->joinType == QMS_NO_JOIN )
            {
                // tuple variable 저장
                SET_POSITION( (*sAliasName), sFrom->tableRef->aliasName );
                
                sAliasName++;
            }
            else
            {
                // joined table은 tuple variable을 갖지 않는다.
                
                // Nothing to do.
            }
        }
    }
    else
    {
        // from 절이 joined table로만 이루어진 경우 aliasCount가 0이다.
        
        // Nothing to do.
    }

    //------------------------------------------
    // tuple variable 생성
    //------------------------------------------

    if ( sAliasCount > 0 )
    {
        // new alias name의 보관
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcNamePosition) * sAliasCount,
                                                 (void **) & aRollbackInfo->newAliasName )
                  != IDE_SUCCESS );

        sAliasName = aRollbackInfo->newAliasName;
    
        // underSFWGH
        for ( sFrom = aUnderSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
        {
            if ( sFrom->joinType == QMS_NO_JOIN )
            {
                IDE_TEST( makeTupleVariable( aStatement,
                                             & aUnderFrom->tableRef->aliasName,
                                             & sFrom->tableRef->aliasName,
                                             sFrom->tableRef->isNewAliasName,
                                             sAliasName,
                                             & sIsCreated )
                          != IDE_SUCCESS );
                
                if ( sIsCreated == ID_FALSE )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
                
                sAliasName++;
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
        
    //------------------------------------------
    // from 절의 merge 수행
    //------------------------------------------

    if ( sIsCreated == ID_TRUE )
    {
        if ( sAliasCount > 0 )
        {
            sAliasName = aRollbackInfo->newAliasName;

            // new alias로 변경한다.
            for ( sFrom = aUnderSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
            {
                if ( sFrom->joinType == QMS_NO_JOIN )
                {
                    SET_POSITION( sFrom->tableRef->aliasName, (*sAliasName) );
                    
                    // merge에 의해 생성된 alias임을 기록한다.
                    sFrom->tableRef->isNewAliasName = ID_TRUE;
                    
                    sAliasName++;
                }
                else
                {
                    // Nothing to do.
                }
            }

            // PROJ-1718 Subquery unnesting
            // View가 갖고있던 semi/anti join의 dependency 정보를 table이 갖도록 설정
            if( ( aUnderSFWGH->from->next == NULL ) &&
                ( qtc::haveDependencies( &aUnderFrom->semiAntiJoinDepInfo ) == ID_TRUE ) )
            {
                qtc::dependencyOr( &aUnderSFWGH->from->depInfo,
                                   &aUnderFrom->semiAntiJoinDepInfo,
                                   &aUnderSFWGH->from->semiAntiJoinDepInfo );

                qtc::dependencyRemove( aUnderFrom->tableRef->table,
                                       &aUnderSFWGH->from->semiAntiJoinDepInfo,
                                       &aUnderSFWGH->from->semiAntiJoinDepInfo );
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

        aRollbackInfo->lastFrom->next = aCurrentSFWGH->from;
        aCurrentSFWGH->from = aUnderSFWGH->from;
    }
    else
    {
        // merge가 실패했다.
        sIsMerged = ID_FALSE;
    }

    aRollbackInfo->fromMerged = ID_TRUE;
    
    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::makeTupleVariable( qcStatement    * aStatement,
                                   qcNamePosition * aViewName,
                                   qcNamePosition * aTableName,
                                   idBool           aIsNewTableName,
                                   qcNamePosition * aNewTableName,
                                   idBool         * aIsCreated )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     from절의 merge시 사용할 tuple variable을 생성한다.
 *
 *     [Enhancement]
 *     상세설계시 'SYS_ALIAS_n_'라는 prefix를 사용하려했으나 너무 길어
 *     오히려 가독성이 떨어져, 보다 짧은 '$$n_'로 변경한다.
 *     그리고 view와 table의 이름 _가 많이 사용되므로 구분자로
 *     '_$'를 사용하여 더 잘 구분되도록 변경한다.
 *
 *     ex) $$1_$view_$table
 *         $$2_$view_v2_$view_v1_$table
 *
 * Implementation :
 *
 ***********************************************************************/

    qcNamePosition  * sViewName;
    qcNamePosition  * sTableName;
    qcNamePosition    sDefaultName;
    qcTemplate      * sTemplate;
    qcTupleVarList  * sVarList;
    SChar           * sNameBuffer;
    UInt              sNameLen;
    SChar           * sRealTableName;
    UInt              sRealTableNameLen;
    idBool            sIsCreated = ID_TRUE;
    idBool            sFound;
    UInt              i;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::makeTupleVariable::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aViewName != NULL );
    IDE_DASSERT( aTableName != NULL );
    IDE_DASSERT( aNewTableName != NULL );
    IDE_DASSERT( aIsCreated != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sTemplate = QC_SHARED_TMPLATE(aStatement);
    
    // $$1_ 부터 시작한다.
    if ( sTemplate->tupleVarGenNumber == 0 )
    {
        sTemplate->tupleVarGenNumber = 1;
    }
    else
    {
        // Nothing to do.
    }

    // default name의 초기화
    sDefaultName.stmtText = DEFAULT_VIEW_NAME;
    sDefaultName.offset   = 0;
    sDefaultName.size     = DEFAULT_VIEW_NAME_LEN;
    
    // 이름 없는 view의 이름을 default로 바꾼다.
    if ( QC_IS_NULL_NAME( (*aViewName) ) == ID_TRUE )
    {
        sViewName = & sDefaultName;
    }
    else
    {
        sViewName = aViewName;
    }

    // 이름 없는 table의 이름을 default로 바꾼다.
    // (merge되지 않은 이름없는 view인 경우)
    if ( QC_IS_NULL_NAME( (*aTableName) ) == ID_TRUE )
    {
        sTableName = & sDefaultName;
    }
    else
    {
        sTableName = aTableName;
    }

    //------------------------------------------
    // name buffer 생성
    //------------------------------------------

    // name buffer length 추정
    if ( aIsNewTableName == ID_FALSE )
    {
        // 최초 merge되는 경우
        // ex) view V1의 T1 -> $$1_$V1_$T1
        //                     ~~~~ ~~  ~~
        sNameLen = QC_TUPLE_VAR_HEADER_SIZE +
            sViewName->size +
            sTableName->size +
            4 +   // 2개의 '_$'
            10 +  // 숫자의 길이 최대 10
            1;    // '\0'
    }
    else
    {
        // 이미 한번 merge되어 tule variable header를 가진 경우
        // ex) view V2의 $$1_$V1_$T1 -> $$2_$V2_$V1_$T1
        //                              ~~~~ ~~ ~~~~~~~
        sNameLen = sTableName->size +
            sViewName->size +
            2 +   // 1개의 '_$'
            10 +  // 숫자의 길이 최대 10
            1;    // '\0'
    }
    
    // name buffer 생성
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(SChar) * sNameLen,
                                             (void **) & sNameBuffer )
              != IDE_SUCCESS );

    //------------------------------------------
    // tuple variable 생성
    //------------------------------------------
    
    for ( i = 0; i < MAKE_TUPLE_RETRY_COUNT; i++ )
    {
        idlOS::memset( sNameBuffer, 0x00, sNameLen );

        // $$1_$
        idlOS::snprintf( sNameBuffer, sNameLen, "%s%"ID_INT32_FMT"_$",
                         QC_TUPLE_VAR_HEADER,
                         sTemplate->tupleVarGenNumber );
        sNameLen = idlOS::strlen( sNameBuffer );
        
        // $$1_$V1
        idlOS::memcpy( sNameBuffer + sNameLen,
                       sViewName->stmtText + sViewName->offset,
                       sViewName->size );
        sNameLen += sViewName->size;
        
        // $$1_$V1_
        sNameBuffer[sNameLen] = '_';
        sNameLen += 1;
        
        if ( aIsNewTableName == ID_FALSE )
        {
            // $$1_$V1_$
            sNameBuffer[sNameLen] = '$';
            sNameLen += 1;
        
            // $$1_$V1_$T1
            idlOS::memcpy( sNameBuffer + sNameLen,
                           sTableName->stmtText + sTableName->offset,
                           sTableName->size );
            sNameLen += sTableName->size;
        }
        else
        {
            // 이미 merge되어 alias가 변경되었으므로
            // $$1_$V1_$T1에서 $V1_$T1을 뽑아낸다.
            sRealTableName = sTableName->stmtText + sTableName->offset;
            sRealTableNameLen = sTableName->size;
            
            sRealTableName += QC_TUPLE_VAR_HEADER_SIZE;
            sRealTableNameLen -= QC_TUPLE_VAR_HEADER_SIZE;
            
            while ( sRealTableNameLen > 0 )
            {
                if ( *sRealTableName == '_' )
                {
                    // '_'는 제외한다.
                    sRealTableName++;
                    sRealTableNameLen--;
                    
                    break;
                }
                else
                {
                    sRealTableName++;
                    sRealTableNameLen--;
                }
            }

            // $$2_$V2_ -> $$2_$V2_$V1_$T1
            idlOS::memcpy( sNameBuffer + sNameLen,
                           sRealTableName,
                           sRealTableNameLen );
            sNameLen += sRealTableNameLen;
        }
        
        sNameBuffer[sNameLen] = '\0';
        
        // generated number 증가
        sTemplate->tupleVarGenNumber++;

        //------------------------------------------
        // 중복 tuple variable 검사
        //------------------------------------------

        sFound = ID_FALSE;
        
        for ( sVarList = sTemplate->tupleVarList;
              sVarList != NULL;
              sVarList = sVarList->next )
        {
            // BUG-37032
            if ( idlOS::strMatch( sVarList->name.stmtText + sVarList->name.offset,
                                  sVarList->name.size,
                                  sNameBuffer,
                                  sNameLen ) == 0 )
            {
                sFound = ID_TRUE;
                break;
            }
        }

        if ( sFound == ID_FALSE )
        {
            break;
        }
        else
        {
            // 생성한 tuple variable 끼리는 충돌하지 않는다.
            
            // Nothing to do.
        }
    }

    if ( i == MAKE_TUPLE_RETRY_COUNT )
    {
        sIsCreated = ID_FALSE;
    }
    else
    {
        aNewTableName->stmtText = sNameBuffer;
        aNewTableName->offset   = 0;
        aNewTableName->size     = sNameLen;
    }

    *aIsCreated = sIsCreated;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::rollbackForFrom( qmsSFWGH            * aCurrentSFWGH,
                                 qmsSFWGH            * aUnderSFWGH,
                                 qmoViewRollbackInfo * aRollbackInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom        * sFrom;
    qcNamePosition * sAliasName;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::rollbackForFrom::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    
    //------------------------------------------
    // from 절의 rollback 수행
    //------------------------------------------

    if ( aRollbackInfo->fromMerged == ID_TRUE )
    {
        // tuple variable 원복
        sAliasName = aRollbackInfo->oldAliasName;
        
        for ( sFrom = aUnderSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
        {
            if ( sFrom->joinType == QMS_NO_JOIN )
            {
                SET_POSITION( sFrom->tableRef->aliasName, (*sAliasName) );
                
                sAliasName++;
            }
            else
            {
                // Nothing to do.
            }
        }

        // from 절의 원복
        aRollbackInfo->lastFrom->next = NULL;
        aCurrentSFWGH->from = aRollbackInfo->firstFrom;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::mergeForWhere( qcStatement         * aStatement,
                               qmsSFWGH            * aCurrentSFWGH,
                               qmsSFWGH            * aUnderSFWGH,
                               qmoViewRollbackInfo * aRollbackInfo,
                               idBool              * aIsMerged )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode         * sAndNode[2];
    qcNamePosition    sNullPosition;
    idBool            sIsMerged = ID_TRUE;
    mtcNode         * sNode = NULL;
    mtcNode         * sPrev = NULL;
    
    IDU_FIT_POINT_FATAL( "qmoViewMerging::mergeForWhere::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    IDE_DASSERT( aIsMerged != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------
    
    SET_EMPTY_POSITION( sNullPosition );
    
    //------------------------------------------
    // rollbackInfo 초기화
    //------------------------------------------
    
    aRollbackInfo->currentWhere = NULL;
    aRollbackInfo->underWhere = NULL;
    aRollbackInfo->whereMerged = ID_FALSE;
    
    //------------------------------------------
    // where 절의 rollback 정보 설정
    //------------------------------------------

    if ( aCurrentSFWGH->where != NULL )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                 (void **) & aRollbackInfo->currentWhere )
                  != IDE_SUCCESS );

        idlOS::memcpy( aRollbackInfo->currentWhere, aCurrentSFWGH->where,
                       ID_SIZEOF( qtcNode ) );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( aUnderSFWGH->where != NULL )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qtcNode),
                                                 (void **) & aRollbackInfo->underWhere )
                  != IDE_SUCCESS );

        idlOS::memcpy( aRollbackInfo->underWhere, aUnderSFWGH->where,
                       ID_SIZEOF( qtcNode ) );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // where 절의 merge 수행
    //------------------------------------------

    if ( aCurrentSFWGH->where != NULL )
    {
        if ( aUnderSFWGH->where != NULL )
        {
            // 현재 SFWGH에 where절이 있고
            // 하위 SFWGH에도 where 절이 있는 경우

            // 새로운 AND 노드를 하나 생성한다.
            IDE_TEST( qtc::makeNode( aStatement,
                                     sAndNode,
                                     & sNullPosition,
                                     (const UChar*)"AND",
                                     3 )
                      != IDE_SUCCESS );
            
            IDE_DASSERT( aCurrentSFWGH->where->node.next == NULL );
            
            // arguments를 연결한다.
            // BUG-43017
            if ( ( ( aCurrentSFWGH->where->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                   == MTC_NODE_LOGICAL_CONDITION_TRUE ) &&
                 ( ( aCurrentSFWGH->where->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_AND ) )
            {
                sAndNode[0]->node.arguments = aCurrentSFWGH->where->node.arguments;
            }
            else
            {
                sAndNode[0]->node.arguments = (mtcNode*) aCurrentSFWGH->where;
            }

            for ( sNode = sAndNode[0]->node.arguments;
                  sNode != NULL;
                  sPrev = sNode, sNode = sNode->next );

            if ( ( ( aUnderSFWGH->where->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                   == MTC_NODE_LOGICAL_CONDITION_TRUE ) &&
                 ( ( aUnderSFWGH->where->node.lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_AND ) )
            {
                sPrev->next = aUnderSFWGH->where->node.arguments;
            }
            else
            {
                sPrev->next = (mtcNode*) aUnderSFWGH->where;
            }

            sAndNode[0]->node.lflag |= 2;

            // estimate를 수행한다.
            IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                        sAndNode[0] )
                      != IDE_SUCCESS );

            // where절에 연결한다.
            aCurrentSFWGH->where = sAndNode[0];
        }
        else
        {
            // 현재 SFWGH에는 where절이 있으나
            // 하위 SFWGH에는 where절이 없는 경우

            // Nothing to do.
        }

        /* BUG-42661 A function base index is not wokring view */
        if ( QCG_GET_SESSION_QUERY_REWRITE_ENABLE(aStatement) == 1 )
        {
            IDE_TEST( qmsDefaultExpr::applyFunctionBasedIndex( aStatement,
                                                               aCurrentSFWGH->where,
                                                               aCurrentSFWGH->from,
                                                               &( aCurrentSFWGH->where ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        if ( aUnderSFWGH->where != NULL )
        {
            // 현재 SFWGH에는 where절이 없으나
            // 하위 SFWGH에는 where절이 있는 경우

            // where절에 연결한다.
            aCurrentSFWGH->where = aUnderSFWGH->where;
        }
        else
        {
            // 현재 SFWGH에도 where절이 없고
            // 하위 SFWGH에도 where 절이 없고 경우

            // Nothing to do.
        }
    }

    aRollbackInfo->whereMerged = ID_TRUE;
    
    *aIsMerged = sIsMerged;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::rollbackForWhere( qmsSFWGH            * aCurrentSFWGH,
                                  qmsSFWGH            * aUnderSFWGH,
                                  qmoViewRollbackInfo * aRollbackInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoViewMerging::rollbackForWhere::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aCurrentSFWGH != NULL );
    IDE_DASSERT( aUnderSFWGH != NULL );
    IDE_DASSERT( aRollbackInfo != NULL );
    
    //------------------------------------------
    // where 절의 rollback 수행
    //------------------------------------------

    if ( aRollbackInfo->whereMerged == ID_TRUE )
    {
        // 현재 where절을 원복
        aCurrentSFWGH->where = aRollbackInfo->currentWhere;
        
        // 하위 where절을 원복
        aUnderSFWGH->where = aRollbackInfo->underWhere;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::removeMergedView( qmsSFWGH     * aSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     merge된 view를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom  * sFirstFrom = NULL;
    qmsFrom  * sCurFrom;
    qmsFrom  * sFrom;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::removeMergedView::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aSFWGH != NULL );

    //------------------------------------------
    // merge된 view의 제거
    //------------------------------------------

    for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        if ( sFrom->joinType == QMS_NO_JOIN )
        {
            // merge되지 않은 from을 선택한다.
            if ( sFrom->tableRef->isMerged == ID_FALSE )
            {
                sFirstFrom = sFrom;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // joined table은 merge대상이 아니므로 바로 선택한다.
            sFirstFrom = sFrom;
            break;
        }
    }

    IDE_FT_ASSERT( sFirstFrom != NULL );

    sCurFrom = sFirstFrom;

    for ( sFrom = sCurFrom->next; sFrom != NULL; sFrom = sFrom->next )
    {
        if ( sFrom->joinType == QMS_NO_JOIN )
        {
            // merge되지 않은 from을 연결한다.
            if ( sFrom->tableRef->isMerged == ID_FALSE )
            {
                sCurFrom->next = sFrom;
                sCurFrom = sFrom;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // joined table은 merge대상이 아니므로 바로 연결한다.
            sCurFrom->next = sFrom;
            sCurFrom = sFrom;
        }
    }

    sCurFrom->next = NULL;
    
    aSFWGH->from = sFirstFrom;

    // BUG-45177 view merge 이후에 남아 있는 view 는 noMerge하게 한다.
    for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        if ( sFrom->joinType == QMS_NO_JOIN )
        {
            sFrom->tableRef->noMergeHint = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::validateQuerySet( qcStatement  * aStatement,
                                  qmsQuerySet  * aQuerySet )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     하위 view가 merge되어 현재 query set에 validation을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoViewMerging::validateQuerySet::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // validation 수행
    //------------------------------------------

    if ( aQuerySet->setOp == QMS_NONE )
    {
        IDE_TEST( validateSFWGH( aStatement,
                                 aQuerySet->SFWGH )
                  != IDE_SUCCESS );
        
        // set dependencies
        qtc::dependencySetWithDep( & aQuerySet->depInfo,
                                   & aQuerySet->SFWGH->depInfo );

        // set outer column dependencies
        IDE_TEST( qmvQTC::setOuterDependencies( aQuerySet->SFWGH,
                                                & aQuerySet->SFWGH->outerDepInfo )
                  != IDE_SUCCESS );
        
        qtc::dependencySetWithDep( & aQuerySet->outerDepInfo,
                                   & aQuerySet->SFWGH->outerDepInfo );

        // PROJ-2418
        // Lateral View의 outerDepInfo 설정
        IDE_TEST( qmvQTC::setLateralDependencies( aQuerySet->SFWGH,
                                                  & aQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( checkViewDependency( aStatement,
                                       & aQuerySet->outerDepInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( validateQuerySet( aStatement,
                                    aQuerySet->left )
                  != IDE_SUCCESS );
        
        IDE_TEST( validateQuerySet( aStatement,
                                    aQuerySet->right )
                  != IDE_SUCCESS );
        
        // outer column dependency는 하위 dependency를 OR-ing한다.
        qtc::dependencyClear( & aQuerySet->outerDepInfo );
        
        IDE_TEST( qtc::dependencyOr( & aQuerySet->left->outerDepInfo,
                                     & aQuerySet->outerDepInfo,
                                     & aQuerySet->outerDepInfo )
                  != IDE_SUCCESS );
        
        IDE_TEST( qtc::dependencyOr( & aQuerySet->right->outerDepInfo,
                                     & aQuerySet->outerDepInfo,
                                     & aQuerySet->outerDepInfo )
                  != IDE_SUCCESS );

        // PROJ-2418
        // lateral view의 dependency는 하위 dependency를 OR-ing한다.
        IDE_TEST( qtc::dependencyOr( & aQuerySet->left->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & aQuerySet->right->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo,
                                     & aQuerySet->lateralDepInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::validateSFWGH( qcStatement  * aStatement,
                               qmsSFWGH     * aSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     하위 view가 merge되어 현재 SFWGH에 validation을 수행한다.
 *     또한, 현재 SFWGH에 포함된 subquery도 외부 참조 컬럼을 가질 수
 *     있으므로 subquery에도 validation을 수행한다.
 *
 * Implementation :
 *     view merge후 수행하는 validation은 각 clause의 expression 혹은
 *     predicate의 노드 트리를 순회하며
 *     (1) 노드의 dependency 정보, 부가 정보등을 처리한다.
 *     (2) 제거된 view에 대하여 dependency 정보가 남아있는지 검사한다.
 *
 ***********************************************************************/

    qmsTarget         * sTarget;
    qmsFrom           * sFrom;
    qmsConcatElement  * sElement;
    qmsConcatElement  * sSubElement;
    qmsAggNode        * sAggNode;
    qtcNode           * sList;
    qmsQuerySet       * sQuerySet;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::validateSFWGH::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSFWGH != NULL );

    //---------------------------------------------------
    // validation of FROM clause
    //---------------------------------------------------

    qtc::dependencyClear( & aSFWGH->depInfo );
    
    for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        // PROJ-2418
        // View Merging 이후 Lateral View는 다시 Validation 해야 한다
        IDE_TEST( validateFrom( sFrom ) != IDE_SUCCESS );

        IDE_TEST( qtc::dependencyOr( & sFrom->depInfo,
                                     & aSFWGH->depInfo,
                                     & aSFWGH->depInfo )
                  != IDE_SUCCESS );

        // PROJ-2415 Grouping Sets Clause
        // View의 Dependency 처리 추가에 따라 하위 View의 Outer Dependency를 Or해준다.
        if ( sFrom->tableRef != NULL )
        {
            if ( sFrom->tableRef->view != NULL )
            {
                sQuerySet = ( ( qmsParseTree* )( sFrom->tableRef->view->myPlan->parseTree ) )->querySet;
                
                IDE_TEST( qtc::dependencyOr( & sQuerySet->outerDepInfo,
                                             & aSFWGH->outerDepInfo,
                                             & aSFWGH->outerDepInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nohting to do.
        }
    }
    
    IDE_TEST( checkViewDependency( aStatement,
                                   & aSFWGH->depInfo )
              != IDE_SUCCESS );
    
    //---------------------------------------------------
    // validation of WHERE clause
    //---------------------------------------------------

    if ( aSFWGH->where != NULL )
    {
        IDE_TEST( validateNode( aStatement,
                                aSFWGH->where )
                  != IDE_SUCCESS );
        
        IDE_TEST( checkViewDependency( aStatement,
                                       & aSFWGH->where->depInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------------------------------
    // validation of GROUP clause
    //---------------------------------------------------

    for ( sElement = aSFWGH->group; sElement != NULL; sElement = sElement->next )
    {
        if ( sElement->type == QMS_GROUPBY_NORMAL )
        {
            IDE_TEST( validateNode( aStatement,
                                    sElement->arithmeticOrList )
                      != IDE_SUCCESS );

            IDE_TEST( checkViewDependency( aStatement,
                                           & sElement->arithmeticOrList->depInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            /* PROJ-1353 */
            for ( sSubElement = sElement->arguments;
                  sSubElement != NULL;
                  sSubElement = sSubElement->next )
            {
                if ( ( sSubElement->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    for ( sList = ( qtcNode * )sSubElement->arithmeticOrList->node.arguments;
                          sList != NULL;
                          sList = ( qtcNode * )sList->node.next )
                    {
                        IDE_TEST( validateNode( aStatement,
                                                sList )
                                  != IDE_SUCCESS );

                        IDE_TEST( checkViewDependency( aStatement,
                                                       &sList->depInfo )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    IDE_TEST( validateNode( aStatement,
                                            sSubElement->arithmeticOrList )
                              != IDE_SUCCESS );

                    IDE_TEST( checkViewDependency( aStatement,
                                                   &sSubElement->arithmeticOrList->depInfo )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    //---------------------------------------------------
    // validation of target list
    //---------------------------------------------------

    for ( sTarget = aSFWGH->target; sTarget != NULL; sTarget = sTarget->next )
    {
        IDE_TEST( validateNode( aStatement,
                                sTarget->targetColumn )
                  != IDE_SUCCESS );
        
        IDE_TEST( checkViewDependency( aStatement,
                                       & sTarget->targetColumn->depInfo )
                  != IDE_SUCCESS );
    }

    //---------------------------------------------------
    // validation of HAVING clause
    //---------------------------------------------------
    
    if ( aSFWGH->having != NULL )
    {
        IDE_TEST( validateNode( aStatement,
                                aSFWGH->having )
                  != IDE_SUCCESS );
        
        IDE_TEST( checkViewDependency( aStatement,
                                       & aSFWGH->having->depInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
 
    //---------------------------------------------------
    // validation of aggregate functions
    //---------------------------------------------------
    // PROJ-2179 Aggregate function에 대해서도 validation을 다시
    // 수행해주어야 다음과 같은 SQL구문이 문제없이 동작한다.
    // SELECT /*+NO_PLAN_CACHE*/ MAX(c1) + MIN(c1) FROM (SELECT c1 FROM t1) ORDER BY MAX(c1) + MIN(c1);
    for( sAggNode = aSFWGH->aggsDepth1;
         sAggNode != NULL;
         sAggNode = sAggNode->next )
    {
        IDE_TEST( validateNode( aStatement,
                                sAggNode->aggr )
                  != IDE_SUCCESS );
        
        IDE_TEST( checkViewDependency( aStatement,
                                       & sAggNode->aggr->depInfo )
                  != IDE_SUCCESS );
    }

    for( sAggNode = aSFWGH->aggsDepth2;
         sAggNode != NULL;
         sAggNode = sAggNode->next )
    {
        IDE_TEST( validateNode( aStatement,
                                sAggNode->aggr )
                  != IDE_SUCCESS );
        
        IDE_TEST( checkViewDependency( aStatement,
                                       & sAggNode->aggr->depInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::validateOrderBy( qcStatement  * aStatement,
                                 qmsParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     order by 절의 validation을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSortColumns  * sSortColumn;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::validateOrderBy::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //------------------------------------------
    // validation 수행
    //------------------------------------------

    if ( aParseTree->orderBy != NULL )
    {
        for ( sSortColumn = aParseTree->orderBy;
              sSortColumn != NULL;
              sSortColumn = sSortColumn->next )
        {
            IDE_TEST( validateNode( aStatement,
                                    sSortColumn->sortColumn )
                      != IDE_SUCCESS );
            
            IDE_TEST( checkViewDependency( aStatement,
                                           & sSortColumn->sortColumn->depInfo )
                      != IDE_SUCCESS );
        }

        IDE_TEST( qmvOrderBy::disconnectConstantNode( aParseTree )
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

IDE_RC
qmoViewMerging::validateNode( qcStatement  * aStatement,
                              qtcNode      * aNode )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     노드의 validation을 수행한다.
 *
 * Implementation :
 *     (1) dependency 정보는 재설정한다.
 *     (2) 부가 정보는 누적한다.
 *
 ***********************************************************************/

    qcStatement       * sStatement;
    qmsParseTree      * sParseTree;
    qtcNode           * sNode;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::validateNode::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );
    
    //------------------------------------------
    // dependency 정보 재설정
    //------------------------------------------
    
    if ( aNode->node.module == & qtc::subqueryModule )
    {
        // subquery 노드인 경우
        sStatement = ((qtcNode*) aNode)->subquery;
        sParseTree = (qmsParseTree*) sStatement->myPlan->parseTree;

        // outer dependency가 있는 subquery에 대하여 validation을 수행한다.
        if ( qtc::haveDependencies( & sParseTree->querySet->outerDepInfo ) == ID_TRUE )
        {
            IDE_TEST( validateQuerySet( aStatement,
                                        sParseTree->querySet )
                      != IDE_SUCCESS );
        
            // dependency 정보 초기화
            qtc::dependencySetWithDep( & aNode->depInfo,
                                       & sParseTree->querySet->outerDepInfo );
        }
        else
        {
            // dependency 정보 초기화
            qtc::dependencyClear( & aNode->depInfo );
        }
    }
    else if ( aNode->node.module == & qtc::passModule )
    {
        // pass 노드인 경우
        sNode = (qtcNode*) aNode->node.arguments;
        
        // dependency 정보 설정
        qtc::dependencySetWithDep( & aNode->depInfo,
                                   & sNode->depInfo );

        // flag 정보 설정
        aNode->node.lflag |= sNode->node.lflag & MTC_NODE_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_MASK;

        // variable built-in function 정보 설정
        aNode->lflag &= ~QTC_NODE_VAR_FUNCTION_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_VAR_FUNCTION_MASK;
        
        // Lob or Binary Type 정보 설정
        aNode->lflag &= ~QTC_NODE_BINARY_MASK;
        aNode->lflag |= sNode->lflag & QTC_NODE_BINARY_MASK;
    }
    else
    {
        if ( ( QTC_IS_TERMINAL( aNode ) == ID_TRUE )         // (1)
             ||
             ( ( aNode->lflag & QTC_NODE_CONVERSION_MASK )    // (2)
               == QTC_NODE_CONVERSION_TRUE )
             )
        {
            //------------------------------------------------------
            // (1) 말단 노드이거나
            // (2) 이미 상수화를 수행한 노드이거나
            //
            // 모두 말단 노드라고 볼 수 있다. 말단 노드에 대해서는
            // 이미 설정된 dependency 정보와 flag 정보를 이용하므로
            // validation을 수행할 필요가 없다.
            //------------------------------------------------------
            
            //------------------------------------------------------
            // BUG-30115
            // 말단 노드라도, 그 노드가 analytic function인 경우,
            // over절에 대한 dependency 정보로 dependency를 재설정 해야함
            // ex) 아래와 같은 질의가 view merging 될때
            //     SELECT COUNT(*) OVER ( PARTITION BY v1.i1 )
            //     FROM ( SELECT i1 FROM t1 )v1
            //     -> view merging 
            //     SELECT COUNT(*) OVER ( PARTITION BY t1.i1 )
            //     FROM t1
            //------------------------------------------------------

            if( aNode->overClause != NULL )
            {
                // anayltic function 노드의 dependency 정보 초기화
                qtc::dependencyClear( & aNode->depInfo );

                // over절의 dependency 정보 재설정
                IDE_TEST( validateNode4OverClause( aStatement,
                                                   aNode )
                          != IDE_SUCCESS );
                
                // analytic function이 있음을 설정
                aNode->lflag &= ~QTC_NODE_ANAL_FUNC_MASK;
                aNode->lflag |= QTC_NODE_ANAL_FUNC_EXIST;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // 중간 노드의 dependency 정보 초기화
            qtc::dependencyClear( & aNode->depInfo );

            //------------------------------------------
            // validation 수행
            //------------------------------------------
        
            for( sNode  = (qtcNode*) aNode->node.arguments;
                 sNode != NULL;
                 sNode  = (qtcNode*) sNode->node.next )
            {
                IDE_TEST( validateNode( aStatement, sNode )
                          != IDE_SUCCESS );
                
                //------------------------------------------------------
                // Argument의 정보 중 필요한 정보를 모두 추출한다.
                //    [Index 사용 가능 정보]
                //     aNode->module->mask : 하위 Node중 column이 있을 경우,
                //     하위 노드의 flag은 index를 사용할 수 있음이 Setting되어 있음.
                //     이 때, 연산자 노드의 특성을 의미하는 mask를 이용해 flag을
                //     재생성함으로서 index를 탈 수 있음을 표현할 수 있다.
                //------------------------------------------------------
            
                aNode->node.lflag |=
                    sNode->node.lflag & aNode->node.module->lmask & MTC_NODE_MASK;
                aNode->lflag |= sNode->lflag & QTC_NODE_MASK;
            
                // Argument의 dependencies를 모두 포함한다.
                IDE_TEST( qtc::dependencyOr( & aNode->depInfo,
                                             & sNode->depInfo,
                                             & aNode->depInfo )
                          != IDE_SUCCESS );
            }
            
            //------------------------------------------------------
            // BUG-27526
            // over절에 대한 validation을 수행한다.
            //------------------------------------------------------
            
            if( aNode->overClause != NULL )
            {
                IDE_TEST( validateNode4OverClause( aStatement,
                                                   aNode )
                          != IDE_SUCCESS );
                
                // BUG-27457
                // analytic function이 있음을 설정
                aNode->lflag &= ~QTC_NODE_ANAL_FUNC_MASK;
                aNode->lflag |= QTC_NODE_ANAL_FUNC_EXIST;
            }
            else
            {
                // Nothing to do.
            }
            
            //------------------------------------------------------
            // BUG-16000
            // Column이나 Function의 Type이 Lob or Binary Type이면 flag설정
            //------------------------------------------------------
            
            aNode->lflag &= ~QTC_NODE_BINARY_MASK;
            
            if ( qtc::isEquiValidType( aNode, & QC_SHARED_TMPLATE(aStatement)->tmplate )
                 == ID_FALSE )
            {
                aNode->lflag |= QTC_NODE_BINARY_EXIST;
            }
            else
            {
                // Nothing to do.
            }

            //------------------------------------------------------
            // PROJ-1404
            // variable built-in function을 사용한 경우 설정한다.
            //------------------------------------------------------
            
            if ( ( aNode->node.lflag & MTC_NODE_VARIABLE_MASK )
                 == MTC_NODE_VARIABLE_TRUE )
            {
                aNode->lflag &= ~QTC_NODE_VAR_FUNCTION_MASK;
                aNode->lflag |= QTC_NODE_VAR_FUNCTION_EXIST;
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

IDE_RC
qmoViewMerging::validateNode4OverClause( qcStatement  * aStatement,
                                         qtcNode      * aNode )
{
/***********************************************************************
 *
 * Description : BUG-27526
 *     over절 노드의 validation을 수행한다.
 *
 * Implementation :
 *     (1) dependency 정보는 재설정한다.
 *     (2) 부가 정보는 누적한다.
 *
 ***********************************************************************/

    qtcOverColumn   * sCurOverColumn;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::validateNode4OverClause::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aNode != NULL );
    
    //------------------------------------------
    // dependency 정보 재설정
    //------------------------------------------
    
    // Partition By column 들에 대한 estimate
    for ( sCurOverColumn = aNode->overClause->overColumn;
          sCurOverColumn != NULL;
          sCurOverColumn = sCurOverColumn->next )
    {
        IDE_TEST( validateNode( aStatement,
                                sCurOverColumn->node )
                  != IDE_SUCCESS );
        
        // partition by column의 dependencies를 모두 포함한다.
        IDE_TEST( qtc::dependencyOr( & aNode->depInfo,
                                     & sCurOverColumn->node->depInfo,
                                     & aNode->depInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoViewMerging::checkViewDependency( qcStatement  * aStatement,
                                     qcDepInfo    * aDepInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     merge로 제거된 view의 dependency가 남아있는지 검사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcTableMap   * sTableMap;
    qmsFrom      * sFrom;
    SInt           sTable;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::checkViewDependency::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aDepInfo != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------
    
    sTableMap = QC_SHARED_TMPLATE(aStatement)->tableMap;
    
    //------------------------------------------
    // dependency 검사
    //------------------------------------------

    sTable = qtc::getPosFirstBitSet( aDepInfo );
    
    while ( sTable != QTC_DEPENDENCIES_END )
    {
        if ( sTableMap[sTable].from != NULL )
        {
            sFrom = sTableMap[sTable].from;
            
            // 여기서 강력하게 검사하지 않으면
            // optimize시나 혹은 execution시 분석하기 힘든
            // 에러가 발생하게 된다.
            IDE_FT_ASSERT( sFrom->tableRef->isMerged!= ID_TRUE );
        }
        else
        {
            // Nothing to do.
        }

        sTable = qtc::getPosNextBitSet( aDepInfo, sTable );
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoViewMerging::modifySameViewRef( qcStatement  * aStatement )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *     merge된 view에 대한 동일 view reference를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcTableMap   * sTableMap;
    qmsFrom      * sFrom;
    qmsTableRef  * sTableRef;
    UShort         i;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::modifySameViewRef::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    
    //------------------------------------------
    // tableMap 획득
    //------------------------------------------

    sTableMap = QC_SHARED_TMPLATE(aStatement)->tableMap;
    
    //------------------------------------------
    // same view reference 수정
    //------------------------------------------

    for ( i = 0; i < QC_SHARED_TMPLATE(aStatement)->tmplate.rowCount; i++ )
    {
        if ( sTableMap[i].from != NULL )
        {
            sFrom = sTableMap[i].from;
            
            if ( sFrom->tableRef->sameViewRef != NULL )
            {
                sTableRef = sFrom->tableRef->sameViewRef;

                if ( sTableRef->isMerged == ID_TRUE )
                {
                    // sameViewRef가 merge되었다면 NULL로 바꾼다.
                    sFrom->tableRef->sameViewRef = NULL;
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

    return IDE_SUCCESS;
}

IDE_RC qmoViewMerging::validateFrom( qmsFrom * aFrom )
{
/************************************************************************
 *
 *  Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 *
 *  Merge된 Lateral View를 외부 참조하는 다른 Lateral View가 있을 수 있으므로
 *  Lateral View에 내부의 querySet만 다시 Validation 한다.
 *
 *  Shard View 에 외부 참조 Push Predicate 가 수행될 수 있으므로 다시 Validation 한다.
 *
 *  Merge된 View를 참조하던 Subquery를 다시 Validation 하는 것과 동일 개념이다.
 *
 ************************************************************************/

    qcStatement  * sStatement;
    qmsParseTree * sParseTree;

    IDU_FIT_POINT_FATAL( "qmoViewMerging::validateFrom::__FT__" );

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        if ( aFrom->tableRef != NULL )
        {
            if ( aFrom->tableRef->view != NULL )
            {
                sStatement = aFrom->tableRef->view;

                if ( ( aFrom->tableRef->flag & QMS_TABLE_REF_LATERAL_VIEW_MASK )
                     == QMS_TABLE_REF_LATERAL_VIEW_TRUE )
                {
                    sParseTree = (qmsParseTree *) sStatement->myPlan->parseTree;

                    // Lateral View에 다시 Validation이 필요하다.
                    IDE_TEST( validateQuerySet( sStatement,
                                                sParseTree->querySet )
                              != IDE_SUCCESS );

                    // View QuerySet에 outerDepInfo가 존재하는 경우에는
                    // lateralDepInfo에 outerDepInfo를 ORing 한다.
                    IDE_TEST( qmvQTC::setLateralDependenciesLast( sParseTree->querySet )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* BUG-48880 */
                    if ( ( sStatement->mFlag & QC_STMT_SHARD_OBJ_MASK )
                         == QC_STMT_SHARD_OBJ_EXIST )
                    {
                        sParseTree = (qmsParseTree *) sStatement->myPlan->parseTree;
                        
                        /* Shard View에 외부 참조 Push Predicate 가 수행될 수 있고,
                         * 다시 Validation 이 필요하다.
                         */
                        IDE_TEST( validateQuerySet( sStatement,
                                                    sParseTree->querySet )
                                  != IDE_SUCCESS );
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
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // JOIN인 경우, LEFT/RIGHT에 대해 각각 호출한다.
        IDE_TEST( validateFrom( aFrom->left  ) != IDE_SUCCESS );
        IDE_TEST( validateFrom( aFrom->right ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoViewMerging::doTransformForMultiDML( qcStatement  * aStatement,
                                               qmsQuerySet  * aQuerySet )
{
    idBool sIsTransformed;

    //------------------------------------------
    // Simple View Merging 수행
    //------------------------------------------
    if ( QCU_OPTIMIZER_SIMPLE_VIEW_MERGING_DISABLE == 0 )
    {
        IDE_TEST( processTransformForQuerySet( aStatement,
                                               aQuerySet,
                                               & sIsTransformed )
                  != IDE_SUCCESS );
        // merge된 동일 view reference를 제거한다.
        IDE_TEST( modifySameViewRef( aStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // environment의 기록
    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_SIMPLE_VIEW_MERGE_DISABLE );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
