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
 *****************************************************************************/

#include <ide.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmv.h>
#include <qmoDistinctElimination.h>

IDE_RC qmoDistinctElimination::doTransform( qcStatement * aStatement,
                                            qmsQuerySet * aQuerySet )
{
/***********************************************************************
 * Description : DISTINCT Keyword 제거
 ***********************************************************************/

    idBool sChanged = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoDistinctElimination::doTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aQuerySet != NULL );

    //------------------------------------------
    // 조건 검사
    //------------------------------------------

    if ( QCU_OPTIMIZER_DISTINCT_ELIMINATION_ENABLE == 1 )
    {
        if ( aQuerySet->setOp == QMS_NONE )
        {
            IDE_TEST( doTransformSFWGH( aStatement,
                                        aQuerySet->SFWGH,
                                        & sChanged )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( doTransform( aStatement, aQuerySet->left )
                      != IDE_SUCCESS );

            IDE_TEST( doTransform( aStatement, aQuerySet->right )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // environment의 기록
    //------------------------------------------

    qcgPlan::registerPlanProperty(
        aStatement,
        PLAN_PROPERTY_OPTIMIZER_DISTINCT_ELIMINATION_ENABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoDistinctElimination::doTransformSFWGH( qcStatement * aStatement,
                                                 qmsSFWGH    * aSFWGH,
                                                 idBool      * aChanged )
{
/****************************************************************************************
 *
 *  Description : BUG-39522 / BUG-39665
 *                Target 전체가 이미 DISTINCT 하다면 앞의 DISTINCT 키워드를 생략한다.
 *
 *   DISTINCT Keyword가 Target 에 존재하면, 아래 2개의 함수를 차례대로 호출한다.
 *
 *   - isDistTargetByGroup()
 *   - isDistTargetByUniqueIdx()
 *
 ****************************************************************************************/

    qmsFrom      * sFrom              = NULL;
    qmsParseTree * sParseTree         = NULL;
    idBool         sIsDistTarget      = ID_FALSE;
    idBool         sIsDistFromTarget  = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoDistinctElimination::doTransformSFWGH::__FT__" );

    /****************************************************************************
     * Bottom-up Distinct Elimination
     * FROM 절에 있는 Inline View / Lateral View에 대해 먼저 처리한다.
     ****************************************************************************/

    for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        if ( sFrom->tableRef != NULL )
        {
            if ( sFrom->tableRef->view != NULL )
            {
                sParseTree = (qmsParseTree*)sFrom->tableRef->view->myPlan->parseTree;

                IDE_TEST( doTransform( sFrom->tableRef->view, sParseTree->querySet )
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

    /****************************************************************************
     * DISTINCT 생략 불가능한 환경인지 확인
     ****************************************************************************/

    IDE_TEST_CONT( ( canTransform( aSFWGH ) == ID_FALSE ), NO_TRANSFORMATION );  

    /****************************************************************************
     * Group By 절로 Target DISTINCT 생략 가능 확인
     ****************************************************************************/

    IDE_TEST( isDistTargetByGroup( aStatement,
                                   aSFWGH,
                                   & sIsDistTarget )
              != IDE_SUCCESS );

    /****************************************************************************
     * Unique NOT NULL Index로 Target DISTINCT 생략 가능 확인
     ****************************************************************************/

    if ( sIsDistTarget == ID_TRUE )
    {
        // Nothing to do.
    }
    else
    {
        // ANSI-Join으로 인해 각 From 마다 호출해야 한다.
        for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
        {
            IDE_TEST( isDistTargetByUniqueIdx( aStatement,
                                               aSFWGH,
                                               sFrom,
                                               & sIsDistFromTarget )
                      != IDE_SUCCESS );

            // 모든 From이 DISTINCT 해야, TARGET의 DISTINCT를 생략할 수 있으므로,
            // 어느 한 From이라도 DISTINCT하지 않으면 검증을 종료한다.
            if ( sIsDistFromTarget == ID_FALSE )
            {
                break;
            }
            else
            {
                // 현재 From이 DISTINCT를 보장
                // Nothing to do.
            }
        }

        if ( sIsDistFromTarget == ID_TRUE )
        {
            // 모든 From에서 Target DISTINCT를 보장한다.
            sIsDistTarget = ID_TRUE;
        }
        else
        {
            sIsDistTarget = ID_FALSE;
        }
    }

    /****************************************************************************
     * (3-3) Target DISTINCT 생략
     ****************************************************************************/

    if ( sIsDistTarget == ID_TRUE )
    {
        aSFWGH->selectType = QMS_ALL;
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( NO_TRANSFORMATION );

    *aChanged = sIsDistTarget;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoDistinctElimination::canTransform( qmsSFWGH * aSFWGH )
{
/****************************************************************************************
 *
 *  Description : Distinct Elimination 함수를 실행할 필요가 있는지 확인한다.
 *
 *  Implementation : 다음을 검증한다.
 *
 *    (1) Target에 DISTINCT Keyword가 있는지
 *    (2) GROUP BY 절에 ROLLUP / CUBE / GROUPING SETS이 존재하는지
 *
 ****************************************************************************************/

    qmsConcatElement * sConcatElement   = NULL;
    idBool             sCanTransform    = ID_FALSE;

    // DISTINCT Keyword가 SFWGH에 존재해야 한다.
    IDE_TEST_CONT( aSFWGH->selectType != QMS_DISTINCT, SKIP_TRANSFORMATION );

    // GROUPING SETS을 가지고 있지 않아야 한다.
    IDE_TEST_CONT( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK ) 
                     != QMV_SFWGH_GBGS_TRANSFORM_NONE, 
                   SKIP_TRANSFORMATION );

    // ROLLUP / CUBE를 가지고 있지 않아야 한다.
    for ( sConcatElement = aSFWGH->group;
          sConcatElement != NULL;
          sConcatElement = sConcatElement->next )
    {
        IDE_TEST_CONT( ( sConcatElement->type == QMS_GROUPBY_ROLLUP ) ||
                       ( sConcatElement->type == QMS_GROUPBY_CUBE ),
                       SKIP_TRANSFORMATION );
    }

    // 조건 검사 완료. 생략 가능.
    sCanTransform = ID_TRUE;

    IDE_EXCEPTION_CONT( SKIP_TRANSFORMATION );

    return sCanTransform;
}

IDE_RC qmoDistinctElimination::isDistTargetByGroup( qcStatement  * aStatement,
                                                    qmsSFWGH     * aSFWGH,
                                                    idBool       * aIsDistTarget )
{
/****************************************************************************************
 *
 *  Description : BUG-39665
 *                Grouping Expression (이하 Exp.) 전부 Target에 존재하는 경우
 *                Target은 이미 DISTINCT 하며, DISTINCT 키워드를 생략할 수 있다.
 *
 *   (예) SELECT DISTINCT i1          FROM T1 GROUP BY i1;     -- 가능
 *        SELECT DISTINCT i1, SUM(i2) FROM T1 GROUP BY i1;     -- 가능
 *        SELECT DISTINCT i1*2        FROM T1 GROUP BY i1*2;   -- 가능   (동일 Expression)
 *        SELECT DISTINCT i1, i2      FROM T1 GROUP BY i1, i2; -- 가능   (전부 Target에 있음)
 *        SELECT DISTINCT i1          FROM T1 GROUP BY i1, i2; -- 불가능 (i2가 Target에 없음)
 *        SELECT DISTINCT i1*2        FROM T1 GROUP BY i1;     -- 불가능 (정확히 일치하지 않음)
 *                                    [*지금은 가능해도, Expression에 따라 그렇지 않을 수 있다.]
 *
 *   - 일반 Grouping에 대해서만 처리한다.
 *
 *   - ROLLUP / CUBE / GROUPING SETS는 처리하지 않는다.
 *     이들 Grouping Exp. 중 하나가, 일반 Grouping Exp. 의 Base Column과 중복되는 경우엔
 *     DISTINCT 보장을 할 수 없기 때문이다.
 *
 *     (예) SELECT DISTINCT i1, i2, SUM(i3) FROM T1 GROUP BY i1, ROLLUP( i1, i2 );
 *          >> 모두 Target에 존재하지만, 일반 Grouping Column i1이 ROLLUP에도 있으므로
 *             i1이 중복되어 나온다.
 *
 ****************************************************************************************/

    qmsTarget        * sTarget         = NULL;
    qmsConcatElement * sGroup          = NULL;
    idBool             sGroupNodeFound = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoDistinctElimination::isDistTargetByGroup::__FT__" );

    // GROUP BY가 없는 경우, 아무 일도 하지 않는다.
    IDE_TEST_CONT( aSFWGH->group == NULL, NO_GROUP_BY );

    /**********************************************************************************
     * Grouping Exp. 로 Target DISTINCT 검증
     *********************************************************************************/

    for ( sGroup = aSFWGH->group;
          sGroup != NULL;
          sGroup = sGroup->next )
    {
        // 현재 Group의 검증을 위한 초기화
        sGroupNodeFound = ID_FALSE;

        switch ( sGroup->type )
        {
            case QMS_GROUPBY_NORMAL:

                // 일반 Grouping Expression은 LIST 형태로 올 수 없다.
                IDE_DASSERT( ( sGroup->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                               != MTC_NODE_OPERATOR_LIST );

                for ( sTarget = aSFWGH->target;
                      sTarget != NULL;
                      sTarget = sTarget->next )
                {
                    // Target이 Grouping Expression인 경우 passModule이 씌워져 있다.
                    if ( sTarget->targetColumn->node.module == &qtc::passModule )
                    {
                        IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                               (qtcNode *)sGroup->arithmeticOrList,
                                                               (qtcNode *)sTarget->targetColumn->node.arguments,
                                                               & sGroupNodeFound )
                                  != IDE_SUCCESS );

                        if ( sGroupNodeFound == ID_TRUE )
                        {
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // Target이 Grouping Expression이 아닌 경우
                        // Nothing to do.
                    }
                }

                break;
            case QMS_GROUPBY_ROLLUP:
            case QMS_GROUPBY_CUBE:
            case QMS_GROUPBY_NULL:
                // ROLLUP / CUBE / GROUPING SETS 에 대해서 처리하지 않는다.
                break;
            default:
                IDE_DASSERT(0);
                break;
        }

        if ( sGroupNodeFound == ID_FALSE )
        {
            // 현재 그룹이 Target에 존재하지 않는 경우
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_EXCEPTION_CONT( NO_GROUP_BY );

    // Target의 DISTINCT 생략 가능 여부
    *aIsDistTarget = sGroupNodeFound;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoDistinctElimination::isDistTargetByUniqueIdx( qcStatement  * aStatement,
                                                        qmsSFWGH     * aSFWGH,
                                                        qmsFrom      * aFrom,
                                                        idBool       * aIsDistTarget )
{
/****************************************************************************************
 *
 *  Description : BUG-39522
 *
 *   FROM 절의 qmsFrom 하나가, 다음 조건을 만족하는지 확인한다.
 *
 *     (1) 해당 qmsFrom에서 나온 Target(들)이 존재
 *     (2) (1)의 Target(들) 중에서, 해당 qmsFrom의 Unique Index에 속하는 Target(들)이 존재
 *     (3) (2)의 Target(들) 모두 NOT NULL
 *
 *   이 작업을 FROM 절의 모든 qmsFrom에 대해 수행한다.
 *   모든 qmsFrom이 위 조건을 만족해야, Target의 DISTINCT를 생략할 수 있다.
 *
 *
 *   (예) CREATE TABLE T1 ( i1 INT NOT NULL, i2 INT, i3 INT,
 *                          PRIMARY KEY(i3) );
 *        CREATE UNIQUE INDEX T1_UIDX1 ON T1(i1);
 *        CREATE UNIQUE INDEX T1_UIDX2 ON T1(i2);
 *
 *        -- [T1.i1 : NOT NULL UNIQUE, T1.i2 : UNIQUE, T1.i3 : PRIMARY KEY]
 *
 *        SELECT DISTINCT i1     FROM T1; -- 가능
 *        SELECT DISTINCT i2     FROM T1; -- 불가능 (NULL 중복 가능)
 *        SELECT DISTINCT i3     FROM T1; -- 가능
 *        SELECT DISTINCT i1, i2 FROM T1; -- 가능
 *                                           (i1이 DISTINCT 하므로 (i1, i2) 집합은 DISTINCT)
 *
 *        SELECT DISTINCT i1     FROM T1 GROUP BY i1, i2; -- Group By로는 불가능, 여기선 가능
 *        SELECT DISTINCT i2     FROM T1 GROUP BY i1, i2; -- Group By로는 불가능, 여기서도 불가능
 *        SELECT DISTINCT i1, i2 FROM T1 GROUP BY i1, i2; -- Group By로 이미 가능 (Skipped)
 *
 *        CREATE TABLE T2 ( i1 INT NOT NULL, i2 INT NOT NULL,
 *                          FOREIGN KEY (i2) REFERENCES T1(i2) );
 *        CREATE UNIQUE INDEX T1_UIDX1 ON T1(i1);
 *
 *        -- [T2.i1 : NOT NULL UNIQUE, T2.i2 : NOT NULL FOREIGN KEY]
 *
 *        SELECT DISTINCT T1.i1               FROM T1, T2; -- 불가능 (T2의 Target이 없음)
 *        SELECT DISTINCT T1.i1, T2.i1        FROM T1, T2; -- 가능
 *        SELECT DISTINCT T1.i1, T1.i2, T2.i1 FROM T1, T2; -- 가능
 *        SELECT DISTINCT T1.i1, T2.i2        FROM T1, T2; -- 불가능
 *                                                            (T2.i2가 FK라도 중복될 수 있음)
 *
 *
 *   Note : 다음의 경우는 DISTINCT를 보장할 수 없다.
 *
 *   - qmsFrom이 ANSI Join Tree이고, 한쪽이라도 DISTINCT가 보장되지 못하는 경우
 *
 *   - Composite Unique Index에 있는 Key Column '일부'만 Target에 존재하는 경우
 *     (조건에서 말했듯, Key Column 모두 Target 존재해야 인정)
 *
 *   - From 에 Inline View, Lateral View, Subquery Factoring (WITH)이 오는 경우
 *
 *   - Partitioned Table qmsFrom 에서, LOCALUNIQUE Index는 확인하지 않는다.
 *     >> qmsFrom이 특정 Partition인 경우에는 확인한다.
 *
****************************************************************************************/

    qmsTarget     * sTarget          = NULL;
    qcmColumn     * sFromColumn      = NULL;
    qcmTableInfo  * sTableInfo       = NULL;
    qcmIndex      * sIndex           = NULL;
    qtcNode       * sTargetNode      = NULL;
    mtcColumn     * sTargetColumn    = NULL;
    mtcColumn     * sKeyColumn       = NULL;
    idBool          sHasTarget       = ID_FALSE;
    idBool          sIsParttition    = ID_FALSE;
    idBool          sIsDistTarget    = ID_FALSE;
    idBool          sKeyColFound     = ID_FALSE;
    idBool          sIsNotNullUnique = ID_FALSE;
    UInt            i, j;

    IDU_FIT_POINT_FATAL( "qmoDistinctElimination::isDistTargetByGroup::__FT__" );

    // 단일 From Object가 아닌 경우, LEFT/RIGHT에 대해 재귀 호출
    if ( aFrom->joinType != QMS_NO_JOIN )
    {
        IDE_TEST( isDistTargetByUniqueIdx( aStatement,
                                           aSFWGH,
                                           aFrom->left,
                                           & sIsDistTarget )
                  != IDE_SUCCESS );

        // LEFT에서 DISTINCT를 보장할 수 없고, DistIdx을 수집하지 않아도 되면
        // 해당 From Object는 DISTINCT를 보장할 수 없으므로 곧바로 종료 (반환값 : FALSE)
        IDE_TEST_CONT( sIsDistTarget == ID_FALSE, NORMAL_EXIT );

        IDE_TEST( isDistTargetByUniqueIdx( aStatement,
                                           aSFWGH,
                                           aFrom->right,
                                           & sIsDistTarget )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( aFrom->tableRef != NULL );

        /**********************************************************************************
         * Partition을 직접 탐색하는 경우, LOCALUNIQUE Index로 검증
         *********************************************************************************/

        if ( aFrom->tableRef->partitionRef != NULL )
        {
            sTableInfo    = aFrom->tableRef->partitionRef->partitionInfo;
            sIsParttition = ID_TRUE;
        }
        else
        {
            sTableInfo    = aFrom->tableRef->tableInfo;
            sIsParttition = ID_FALSE;
        }


        /**********************************************************************************
         * 현재 From에 속한 Target이 존재하는지 확인
         *********************************************************************************/

        for ( sFromColumn = sTableInfo->columns;
              sFromColumn != NULL;
              sFromColumn = sFromColumn->next )
        {
            for ( sTarget = aSFWGH->target;
                  sTarget != NULL;
                  sTarget = sTarget->next )
            {
                // Target이 Grouping Expression인 경우 passModule이 씌워져 있다.
                if ( sTarget->targetColumn->node.module == &qtc::passModule )
                {
                    sTargetNode = (qtcNode *)sTarget->targetColumn->node.arguments;
                }
                else
                {
                    sTargetNode = sTarget->targetColumn;
                }

                if ( QTC_IS_COLUMN( aStatement, sTargetNode ) == ID_TRUE )
                {
                    sTargetColumn = QTC_STMT_COLUMN( aStatement, sTargetNode );

                    if ( sFromColumn->basicInfo->column.id == sTargetColumn->column.id )
                    {
                        sHasTarget = ID_TRUE;
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

            } /* End of loop : 모든 Target 탐색 완료 */

            if ( sHasTarget == ID_TRUE )
            {
               break;
            }
            else
            {
                // Nothing to do.
            }

        } /* End of loop : 단일 From Object의 모든 Column 탐색 완료 */

        // 현재 From에 관련있는 Target이 존재하지 않으면
        // DISTINCT를 보장할 수 없으므로 곧바로 종료 (반환값 : FALSE)
        IDE_TEST_CONT( sHasTarget == ID_FALSE, NORMAL_EXIT );

        /**********************************************************************************
         * Unique Index로 DISTINCT 검증
         *********************************************************************************/

        for ( i = 0; i < sTableInfo->indexCount; i++ )
        {
            sIndex = & sTableInfo->indices[i];

            // Unique Index?
            if ( sIndex->isUnique == ID_FALSE )
            {
                // Partition을 직접 탐색하는 경우, 현재 Index가 LocalUnique 라면 Unique 보증
                if ( ( sIsParttition == ID_TRUE ) && ( sIndex->isLocalUnique == ID_TRUE ) )
                {
                    // Nothing to do.
                }
                else
                {
                    // Unique하지 않은 Index라면 다른 Index를 탐색
                    continue;
                }
            }
            else
            {
                // Nothing to do.
            }

            // Not Null Unique Index?
            sIsNotNullUnique = ID_TRUE;

            for ( j = 0; j < sIndex->keyColCount; j++ )
            {
                sKeyColumn = & sIndex->keyColumns[j];

                if ( ( sKeyColumn->flag & MTC_COLUMN_NOTNULL_MASK ) ==
                       MTC_COLUMN_NOTNULL_FALSE )
                {
                    sIsNotNullUnique = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sIsNotNullUnique == ID_FALSE )
            {
                continue;
            }
            else
            {
                // Nothing to do.
            }

            /**********************************************************************************
             * Unique Index의 모든 Key가 Target에 있는지 검사
             *********************************************************************************/

            // 검증을 위한 초기화
            sIsDistTarget = ID_TRUE;

            for ( j = 0; j < sIndex->keyColCount; j++ )
            {
                sKeyColumn   = & sIndex->keyColumns[j];
                sKeyColFound = ID_FALSE;

                for ( sTarget = aSFWGH->target;
                      sTarget != NULL;
                      sTarget = sTarget->next )
                {
                    // Target이 Grouping Expression인 경우 passModule이 씌워져 있다.
                    if ( sTarget->targetColumn->node.module == &qtc::passModule )
                    {
                        sTargetNode = (qtcNode *)sTarget->targetColumn->node.arguments;
                    }
                    else
                    {
                        sTargetNode = sTarget->targetColumn;
                    }

                    if ( QTC_IS_COLUMN( aStatement, sTargetNode ) == ID_TRUE )
                    {
                        sTargetColumn = QTC_STMT_COLUMN( aStatement, sTargetNode );

                        // 현재 Key Column과 일치하는 Target을 찾는다.
                        // 일치하는 Key Column이 NOT Null이 아니라면, Key Column과 일치해도 무시한다.

                        // Key Column이 NOT NULL이라도 Target이 NULL일 수 있는데 (Outer Join)
                        // 이 경우에도 Key Column만 NOT NULL이면 DISTINCT는 보장된다.
                        if ( ( sKeyColumn->column.id == sTargetColumn->column.id ) &&
                             ( sKeyColumn->flag & MTC_COLUMN_NOTNULL_MASK ) ==
                             MTC_COLUMN_NOTNULL_TRUE )
                        {
                            sKeyColFound = ID_TRUE;
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

                // KeyColumn 중 하나가 Target에 없으므로, 해당 Index로 DISTINCT가 될 수 없다.
                if ( sKeyColFound == ID_FALSE )
                {
                    sIsDistTarget = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

            } /* End of loop : 단일 Unique Index의 모든 Key Column 탐색 완료 */

            // 해당 Unique Index로 DISTINCT를 보장할 수 있다면 곧바로 종료 (반환값 : TRUE)
            IDE_TEST_CONT( sIsDistTarget == ID_TRUE, NORMAL_EXIT );

        } /* End of loop : 모든 Index 탐색 완료 */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aIsDistTarget = sIsDistTarget;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

