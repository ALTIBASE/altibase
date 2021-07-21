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
 * $Id: qmoCheckViewColumnRef.cpp 89448 2020-12-07 04:24:14Z cory.chae $
 *
 * PROJ-2469 Optimize View Materialization
 *
 * View Column에 대한 참조 정보( qmsTableRef->viewColumnRefList )를
 * 상위 Query Block으로 부터 Top-Down으로 전달 하면서
 * 상위 에서 사용되지 않아, 제거 되어도 결과에 영향을 미치지 않는 View의 Target에 대해
 * 사용하지 않음( QMS_TARGET_IS_USELESS_TRUE )으로 flag처리를 한다.
 *
 * ex ) SELECT i1 << i1만 사용된다.
 *        FROM (
 *               SELECT i1, i2
 *                 FROM (   ^ 사용되지 않는다고 표시
 *                        SELECT i1, i2, i3
 *                          FROM T1  ^   ^ 사용되지 않는다고 표시
 *                      )
 *              );
 *
 * 사용되지 않는다고 표시 된 View의 Target Column은
 * qmoOneNonPlan::initPROJ()
 * qmoOneMtrPlan::initVMTR()
 * qmoOneMtrPlan::initCMTR() 함수에서 Result Descriptor를 생성 할 때 반영되어,
 *                                  ( createResultFromQuerySet() )
 * 해당 Node를 calculate 하지 않거나, Materialized Node를 Minimize 해서
 * 결과적으로 Dummy화 시킨다.
 *
 * <<<<<<<<<< View의 Target을 수행에서 제외하지 않는 경우( 예외사항 ) >>>>>>>>>>
 *
 * 1. SELECT Clause에만 적용한다. - DML( INSERT/UPDATE/DELETE )제외. ( Subquery는 수행한다. )
 *
 * 2. Set Operator Type이 NONE 이거나 UNION_ALL( BAG OPERATION ) 일 때만 수행한다.
 *    ( Set 연산은 그 자체로 Target이 모두 유의미하다. )
 *
 * 3. DISTINCT 가 있는경우 제외하지 않는다. ( 모든 Target이 유의미하다. )
 *
 **********************************************************************/

#include <qmoCheckViewColumnRef.h>
#include <qmsParseTree.h>
#include <qcuProperty.h>
#include <qmv.h>

IDE_RC
qmoCheckViewColumnRef::checkViewColumnRef( qcStatement      * aStatement,
                                           qmsColumnRefList * aParentColumnRef,
                                           idBool             aAllColumnUsed )
{
/***********************************************************************
 *
 * Description :
 *     SELECT Statement의 Transform을 발생 시킬 수 있는
 *     모든 Validation이 끝나고 수행되어, 최상위 Query Block 부터
 *     최하위 Query Block 까지 순회하며, 실제로 사용되지 않는
 *     View Target Column을 찾아내 flag 처리한다.
 *
 * Implementation :
 *     1. SELECT Statement 의 Parse Tree를 대상으로
 *        Query Set에 대하여 불필요한( 상위Query Block에서 사용하지 않는 )
 *        View Target Column을 찾는 함수를 호출한다.
 *
 * Arguments :
 *     aStatement       ( 초기 Statement )
 *     aParentColumnRef ( 상위 Query Block의 View Column 참조 리스트 )
 *     aAllColumnUsed   ( 모든 Target Column이 유효해야 하는지 여부 )
 *
 ***********************************************************************/
    qmsParseTree   * sParseTree;
    idBool           sAllColumnUsed = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoCheckViewColumnRef::checkViewColumnRef::__FT__" );

    IDE_DASSERT( aStatement != NULL );

    // PLAN PROPERTY : __OPTIMIZER_VIEW_TARGET_ENABLE = 1 일때만 수행한다.
    if ( QCU_OPTIMIZER_VIEW_TARGET_ENABLE == 1 )
    {
        sParseTree = ( qmsParseTree * )aStatement->myPlan->parseTree;

        // BUG-43669
        // View Merging이 수행 된 후의 view target은 모두 사용하는 것으로 처리한다.
        // 기존 로직에서는 tableRef->isNewAliasName으로 viewMerging 여부를 확인 했는데,
        // From의 type이 (OUTER)JOIN일 경우 tableRef가 없어서 viewMerging이 걸러지지 않았다.
        // 이를 ParseTree의 isTransformed를 보고 판단하는 것 으로 수정한다.
        if ( sParseTree->isTransformed == ID_TRUE )
        {
            sAllColumnUsed = ID_TRUE;
        }
        else
        {
            sAllColumnUsed = aAllColumnUsed;
        }

        /**********************************************************************
         * 마지막 인자( aAllTargetUsed )는
         * Top Query Block 이거나, 상위에 Set 연산이 존재하여
         * 모든 Target이 사용되는 경우 TRUE 이고,
         * 상위에 Set연산이 없는 View Query Block 일 경우 FALSE 이다.
         **********************************************************************/
        IDE_TEST( checkQuerySet( sParseTree->querySet,
                                 aParentColumnRef,
                                 sParseTree->orderBy,
                                 sAllColumnUsed )
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
qmoCheckViewColumnRef::checkQuerySet( qmsQuerySet      * aQuerySet,
                                      qmsColumnRefList * aParentColumnRef,
                                      qmsSortColumns   * aOrderBy,
                                      idBool             aAllColumnUsed )
/***********************************************************************
 *
 * Description :
 *     Query Set의 Target에 대해, 상위 Query Block의 참조정보( aParentColumnRef )를
 *     기초로, 제거되어도 결과에 영향을 주지않는 Column에 대해 flag 처리한다.
 *
 * Implementation :
 *     1. 모든 Target이 유효한지 확인
 *     2. 필요없는 View Target을 찾아내어 flag처리
 *     3. 하위 View에 대한 처리를 위한 함수 호출
 *     4. SET에 대해서 LEFT, RIGHT 재귀호출
 *
 * Arguments :
 *     aQuerySet
 *     aParentColumnRef ( 상위 Query Block의 View Column 참조 리스트 )
 *     aOrderBy
 *     aAllColumnUsed   ( 모든 Target Column이 유효해야 하는지 여부 )
 *
 ***********************************************************************/
{
    qmsTarget        * sTarget;
    qmsFrom          * sFrom;
    idBool             sAllColumnUsed;

    IDU_FIT_POINT_FATAL( "qmoCheckViewColumnRef::checkQuerySet::__FT__" );

    /********************************************
     * 1. 모든 Column을 사용해야 하는지 확인
     ********************************************/
    switch ( aQuerySet->setOp )
    {
        case QMS_NONE :
            if ( aQuerySet->SFWGH->selectType == QMS_DISTINCT )
            {
                /**********************************************************************************
                 *
                 * BUG-40893
                 * TARGET에 DISTINCT가 있어서 모든 Column이 필요 할 경우
                 *
                 * ex ) SELECT i1
                 *        FROM (
                 *               SELECT DISTINCT i1, i2<<< DISTINCTION을 위해 모든 Column이 필요하다.
                 *                 FROM ( SELECT i1, i2, i3
                 *                        FROM T1
                 *                        LIMIT 10 ) );
                 *
                 ***********************************************************************************/
                sAllColumnUsed = ID_TRUE;
            }
            else
            {
                sAllColumnUsed = aAllColumnUsed;

                // BUG-48090 현재 QuerySet 절을 탐색하여 With View 플래그가 설정되어 있으면, 
                // 모든 컬럼이 유효하도록 설정한다.
                if ( sAllColumnUsed == ID_FALSE )
                {
                    IDE_TEST( checkWithViewFlagFromQuerySet( aQuerySet,
                                                             &sAllColumnUsed /* sIsWithView */ )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            break;

        case QMS_UNION_ALL :
            sAllColumnUsed = aAllColumnUsed;
            break;

        default :
            /**********************************************************************************
             *
             * Set 연산으로 모든 Column이 필요 할 경우
             *
             * ex ) SELECT i1
             *        FROM (
             *               SELECT i1, i2, i3
             *                 FROM T1  ^   ^ 상위 Query block에서 사용되지 않지만,
             *               INTERSECT        INTERSECT의 결과에 영향을 주기 때문에 필요하다.
             *               SELECT i1, i2, i3
             *                 FROM T1  ^   ^
             *              );
             *
             ***********************************************************************************/
            sAllColumnUsed = ID_TRUE;
            break;
    }

    /**************************************************
     * 2. 필요없는 View Target을 찾아내어 flag 처리
     **************************************************/
    // 모든 Column을 사용하는 경우가 아닐때
    if ( sAllColumnUsed == ID_FALSE )
    {
        // 상위 Query Block 또는 Order By에서 Target이 하나라도 참조되는 경우
        if ( ( aParentColumnRef != NULL ) || ( aOrderBy != NULL ) )
        {
            IDE_TEST( checkUselessViewTarget( aQuerySet->target,
                                              aParentColumnRef,
                                              aOrderBy ) != IDE_SUCCESS );
        }
        else
        {
            /**********************************************************
             * 상위에서 View의 Target을 하나도 참조하지 않을 경우
             **********************************************************/
            for ( sTarget  = aQuerySet->target;
                  sTarget != NULL;
                  sTarget  = sTarget->next )
            {
                if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) == QMS_TARGET_IS_USELESS_UNKNOWN )
                {
                    sTarget->flag &= ~QMS_TARGET_IS_USELESS_MASK;
                    sTarget->flag |=  QMS_TARGET_IS_USELESS_TRUE;
                }
                else
                {
                    // Nohting to do.
                }
            }
        }
    }
    else
    {
        /**********************************************************
         * View Target이 모두 유효한 경우
         **********************************************************/
        for ( sTarget  = aQuerySet->target;
              sTarget != NULL;
              sTarget  = sTarget->next )
        {
            // UNKNOWN->FALSE, TRUE->FALSE
            if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) != QMS_TARGET_IS_USELESS_FALSE )
            {
                sTarget->flag &= ~QMS_TARGET_IS_USELESS_MASK;
                sTarget->flag |=  QMS_TARGET_IS_USELESS_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    /**************************************************
     * 3. From에 대한 처리
     **************************************************/
    if ( aQuerySet->setOp == QMS_NONE )
    {
        sFrom = aQuerySet->SFWGH->from;

        for ( ; sFrom != NULL; sFrom = sFrom->next )
        {
            IDE_TEST( checkFromTree( sFrom,
                                     aParentColumnRef,
                                     aOrderBy,
                                     sAllColumnUsed ) != IDE_SUCCESS );
        }
    }
    else // SET OPERATORS
    {
        // Recursive Call
        IDE_TEST( checkQuerySet( aQuerySet->left,
                                 aParentColumnRef,
                                 aOrderBy,
                                 sAllColumnUsed )
                  != IDE_SUCCESS );

        IDE_TEST( checkQuerySet( aQuerySet->right,
                                 aParentColumnRef,
                                 aOrderBy,
                                 sAllColumnUsed )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCheckViewColumnRef::checkFromTree( qmsFrom          * aFrom,
                                      qmsColumnRefList * aParentColumnRef,
                                      qmsSortColumns   * aOrderBy,
                                      idBool             aAllColumnUsed )
{
/***********************************************************************
 *
 * Description :
 *     상위의 View Column 참조정보( aParentColumnRef )와 비교하여
 *     자신의 View Column 참조정보에서 불필요한 Target Column을 표시하고,
 *     하위 View에 대해서 초기함수인 checkViewColumnRef() 를 재귀수행한다.
 *
 * Implementation :
 *     1. 상위 참조정보에 들어있지 않은, 나의 참조정보의 Target Column에 사용안함 표시
 *     2. 하위 View에 대해서 초기수행함수 qmoCheckViewColumnRef() 호출
 *     3. SameView에 대해서 초기수행함수 qmoCheckViewColumnRef() 호출
 *     4. JOIN Tree순회
 *
 * Arguments :
 *     aFrom
 *     aParentColumnRef ( 상위 Query Block의 View Column 참조 리스트 )
 *     aOrderBy
 *     aAllColumnUsed   ( 모든 Target Column이 유효해야 하는지 여부 )
 *
 ***********************************************************************/
    qmsTableRef      * sTableRef;

    IDU_FIT_POINT_FATAL( "qmoCheckViewColumnRef::checkFromTree::__FT__" );

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        sTableRef = aFrom->tableRef;

        // 하위에 View가 없는 경우 수행하지 않는다.
        if ( sTableRef->view != NULL )
        {
            /*
             * Top Query Block, Merged View 또는 Set Operation으로 인해 모든 Column이 유효하면 수행하지 않는다.
             */
            if ( aAllColumnUsed == ID_FALSE )
            {
                IDE_TEST( checkUselessViewColumnRef( sTableRef,
                                                     aParentColumnRef,
                                                     aOrderBy )
                          != IDE_SUCCESS );

            }
            else
            {
                // Nothing to do.
            }

            // BUG-48090 sTableRef에 With View 플래그가 설정되어 있으면, 
            // sTableRef의 하위 View를 최적화할 때, 모든 컬럼이 유효하도록 설정한다.
            if ( ( sTableRef->flag & QMS_TABLE_REF_WITH_VIEW_MASK ) 
                 == QMS_TABLE_REF_WITH_VIEW_TRUE )
            {
                IDE_TEST( checkViewColumnRef( sTableRef->view,
                                              NULL,
                                              ID_TRUE )
                          != IDE_SUCCESS );                
            }
            else
            {
                IDE_TEST( checkViewColumnRef( sTableRef->view,
                                              sTableRef->viewColumnRefList,
                                              ID_FALSE )
                          != IDE_SUCCESS );
            }

            // Same View Reference 가 있을 경우 처리
            if ( sTableRef->sameViewRef != NULL )
            {
                /* BUG-47787 recursive with 구문이 중첩되고 CASE WHEN의
                 * Subquery로 사용될 경우 FATAL
                 */
                if ( sTableRef->sameViewRef->view != NULL )
                {
                    IDE_TEST( checkViewColumnRef( sTableRef->sameViewRef->view,
                                                  sTableRef->viewColumnRefList,
                                                  ID_FALSE )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothingt to do */
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
        // Recursive Call For From Tree
        IDE_TEST( checkFromTree( aFrom->left,
                                 aParentColumnRef,
                                 aOrderBy,
                                 aAllColumnUsed )
                  != IDE_SUCCESS );

        IDE_TEST( checkFromTree( aFrom->right,
                                 aParentColumnRef,
                                 aOrderBy,
                                 aAllColumnUsed )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoCheckViewColumnRef::checkUselessViewTarget( qmsTarget        * aTarget,
                                               qmsColumnRefList * aParentColumnRef,
                                               qmsSortColumns   * aOrderBy )
{
/***********************************************************************
 *
 * Description :
 *     필요없는 View Target을 찾아내어 flag처리한다.
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/
    qmsTarget        * sTarget;
    qmsSortColumns   * sOrderBy;
    qmsColumnRefList * sParentColumnRef;
    UShort             sTargetOrder;
    idBool             sIsFound;

    IDU_FIT_POINT_FATAL( "qmoCheckViewColumnRef::checkUselessViewTarget::__FT__" );

    for ( sTarget  = aTarget, sTargetOrder = 0;
          sTarget != NULL;
          sTarget  = sTarget->next, sTargetOrder++ )
    {
        sIsFound = ID_FALSE;

        for ( sParentColumnRef  = aParentColumnRef;
              sParentColumnRef != NULL;
              sParentColumnRef  = sParentColumnRef->next )
        {
            if ( ( sParentColumnRef->viewTargetOrder == sTargetOrder ) &&
                 ( sParentColumnRef->isUsed == ID_TRUE ) )
            {
                // 상위에서 사용 됨
                sIsFound = ID_TRUE;

                // UNKNOWN->FALSE, TRUE->FALSE
                if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) != QMS_TARGET_IS_USELESS_FALSE )
                {
                    sTarget->flag &= ~QMS_TARGET_IS_USELESS_MASK;
                    sTarget->flag |=  QMS_TARGET_IS_USELESS_FALSE;
                }
                else
                {
                    // Nothing to do.
                }

                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsFound == ID_FALSE )
        {
            for ( sOrderBy  = aOrderBy;
                  sOrderBy != NULL;
                  sOrderBy  = sOrderBy->next )
            {
                // 명시적 Indicator로 등록된 SortNode는 targetPosition이 세팅된다.
                // OrderBy에서 사용 된 Target은 제거대상에서 제외한다.
                if ( sOrderBy->targetPosition == ( sTargetOrder + 1 ) )
                {
                    /********************************
                     *
                     * CASE :
                     *
                     * SELECT I1, I2
                     *   FROM ( SELECT I1, I2, I3
                     *            FROM T1
                     *        ORDER BY 3 )
                     *
                     *********************************/
                    sIsFound = ID_TRUE;

                    // UNKNOWN->FALSE, TRUE->FALSE
                    if ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) != QMS_TARGET_IS_USELESS_FALSE )
                    {
                        sTarget->flag &= ~QMS_TARGET_IS_USELESS_MASK;
                        sTarget->flag |=  QMS_TARGET_IS_USELESS_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }

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

        // 상위 Query Block에서 사용되지 않는 Column
        if ( ( sIsFound == ID_FALSE ) &&
             ( ( sTarget->flag & QMS_TARGET_IS_USELESS_MASK ) == QMS_TARGET_IS_USELESS_UNKNOWN ) )
        {
            sTarget->flag &= ~QMS_TARGET_IS_USELESS_MASK;
            sTarget->flag |=  QMS_TARGET_IS_USELESS_TRUE;
        }
        else
        {
            // Nothing to do.
        }

    } // End for loop

    return IDE_SUCCESS;
}

IDE_RC
qmoCheckViewColumnRef::checkUselessViewColumnRef( qmsTableRef      * aTableRef,
                                                  qmsColumnRefList * aParentColumnRef,
                                                  qmsSortColumns   * aOrderBy )
{
/***********************************************************************
 *
 * Description :
 *     필요없는 View Column Ref를 찾아내어 flag처리한다.
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qmsColumnRefList * sColumnRef;
    qmsColumnRefList * sParentColumnRef;
    qmsSortColumns   * sOrderBy;
    idBool             sIsFound;

    IDU_FIT_POINT_FATAL( "qmoCheckViewColumnRef::checkUselessViewColumnRef::__FT__" );

    for ( sColumnRef  = aTableRef->viewColumnRefList;
          sColumnRef != NULL;
          sColumnRef  = sColumnRef->next )
    {
        // Target 에서 참조된 Column만을 대상으로 한다.
        if ( sColumnRef->usedInTarget == ID_TRUE )
        {
            sIsFound = ID_FALSE;

            for ( sParentColumnRef  = aParentColumnRef;
                  sParentColumnRef != NULL;
                  sParentColumnRef  = sParentColumnRef->next )
            {
                if ( ( sColumnRef->targetOrder  == sParentColumnRef->viewTargetOrder ) &&
                     ( sParentColumnRef->isUsed == ID_TRUE ) )
                {
                    // 상위 Query Block에서 사용 된다.
                    sIsFound = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sIsFound == ID_FALSE )
            {
                for ( sOrderBy  = aOrderBy;
                      sOrderBy != NULL;
                      sOrderBy  = sOrderBy->next )
                {
                    if ( ( sColumnRef->targetOrder + 1 ) == sOrderBy->targetPosition )
                    {
                        // Order By에서 사용 된다.
                        sIsFound = ID_TRUE;
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

            if ( sIsFound == ID_FALSE )
            {
                /****************************************************************************
                 *
                 * 상위 Query Block에서 사용되지 않는,
                 * Target에서 등록된 Column에 대해서
                 * 사용하지 않음으로 표시한다.
                 *
                 ****************************************************************************/
                sColumnRef->isUsed = ID_FALSE;
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

    } // End loop

    return IDE_SUCCESS;
}

IDE_RC qmoCheckViewColumnRef::checkWithViewFlagFromQuerySet( qmsQuerySet  * aQuerySet,
                                                             idBool       * aIsWithView )
{
    qmsFrom  * sFrom;
 
    if ( *aIsWithView == ID_FALSE )
    {
        if ( aQuerySet->setOp == QMS_NONE )
        {
            sFrom = aQuerySet->SFWGH->from;

            for ( ; sFrom != NULL; sFrom = sFrom->next )
            {
                IDE_TEST( checkWithViewFlagFromFromTree( sFrom, aIsWithView )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // Recursive Call
            IDE_TEST( checkWithViewFlagFromQuerySet( aQuerySet->left,
                                                     aIsWithView )
                      != IDE_SUCCESS );
                      
            IDE_TEST( checkWithViewFlagFromQuerySet( aQuerySet->right,
                                                     aIsWithView )
                      != IDE_SUCCESS );
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

IDE_RC qmoCheckViewColumnRef::checkWithViewFlagFromFromTree( qmsFrom      * aFrom,
                                                             idBool       * aIsWithView )
{
    qmsTableRef  * sTableRef;
 
    if ( *aIsWithView == ID_FALSE )
    {
        if ( aFrom->joinType == QMS_NO_JOIN )
        {
            if ( aFrom->tableRef != NULL )
            {
                sTableRef = aFrom->tableRef;

                if ( ( sTableRef->flag & QMS_TABLE_REF_WITH_VIEW_MASK ) 
                     == QMS_TABLE_REF_WITH_VIEW_TRUE )
                {
                    *aIsWithView = ID_TRUE;
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
            // Recursive Call For From Tree
            IDE_TEST( checkWithViewFlagFromFromTree( aFrom->left,
                                                     aIsWithView )
                      != IDE_SUCCESS );
                      
            IDE_TEST( checkWithViewFlagFromFromTree( aFrom->right,
                                                     aIsWithView )
                      != IDE_SUCCESS );
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
