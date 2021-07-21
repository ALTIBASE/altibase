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
 * $Id: qmoUtil.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmn.h>
#include <qmoUtil.h>

#define QMO_MAX_PRED_LENGTH     4000

extern mtfModule mtfAnd;
extern mtfModule mtfOr;
extern mtfModule mtfNot;

extern mtfModule mtfCast;
extern mtfModule mtfCount;
extern mtfModule mtfCountKeep;
extern mtfModule mtfMinus;
extern mtfModule mtfList;
extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;
extern mtfModule mtfLike;
extern mtfModule mtfNotLike;
extern mtfModule mtfNotExists;
extern mtfModule mtfNotUnique;
extern mtfModule mtfMultiply;
extern mtfModule mtfDivide;
extern mtfModule mtfAdd2;
extern mtfModule mtfSubtract2;
extern mtfModule mtfGetBlobLocator;
extern mtfModule mtfGetClobLocator;
extern mtfModule mtfGetBlobValue;
extern mtfModule mtfGetClobValue;

IDE_RC qmoUtil::printPredInPlan(qcTemplate   * aTemplate,
                                iduVarString * aString,
                                ULong          aDepth,
                                qtcNode      * aNode)
{
    qtcNode* sNode;

    IDU_FIT_POINT_FATAL( "qmoUtil::printPredInPlan::__FT__" );

    // To Fix PR-9044
    // Transform 등으로 인해 Predicate정보를 출력할 수 없는 경우가
    // 있으므로, 아무것도 출력하지 않는다.

    if ( ( aNode->node.lflag &
         ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppendLength( aString, "AND\n", 4 );
    }
    else if ( ( aNode->node.lflag &
         ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
            == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppendLength( aString, "OR\n", 3 );
    }
    else if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
              == MTC_NODE_OPERATOR_NOT )
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppendLength( aString, "NOT\n", 4 );
    }
    else
    {
        qmn::printSpaceDepth(aString, aDepth);
        
        // print expression
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         aNode,
                                         QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );
            
        // PROJ-1404
        // Transitive Predicate인 경우 표시한다.
        if ( (aNode->lflag & QTC_NODE_TRANS_PRED_MASK)
             == QTC_NODE_TRANS_PRED_EXIST )
        {
            iduVarStringAppendLength( aString, " [+]", 4 );
        }
        else
        {
            // Nothing to do.
        }
        
        iduVarStringAppendLength( aString, "\n", 1 );
    }

    if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        for (sNode = (qtcNode *)(aNode->node.arguments);
             sNode != NULL;
             sNode = (qtcNode *)(sNode->node.next))
        {
            IDE_TEST(printPredInPlan( aTemplate,
                                      aString,
                                      aDepth+1,
                                      sNode )
                     != IDE_SUCCESS);
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

IDE_RC qmoUtil::unparseFrom( qcTemplate   * aTemplate,
                             iduVarString * aString,
                             qmsFrom      * aFrom )
{
    idBool sUnparseAlias = ID_TRUE;
    
    IDU_FIT_POINT_FATAL( "qmoUtil::unparseFrom::__FT__" );

    if( aFrom->joinType == QMS_NO_JOIN )
    {
        if( aFrom->tableRef->view != NULL )
        {
            iduVarStringAppend( aString, "(" );
            IDE_TEST( unparseQuerySet( aTemplate,
                                       aString,
                                       ((qmsParseTree *)aFrom->tableRef->view->myPlan->parseTree)->querySet )
                      != IDE_SUCCESS );
            iduVarStringAppend( aString, ")" );

            if( QC_IS_NULL_NAME( aFrom->tableRef->aliasName ) == ID_FALSE )
            {
                iduVarStringAppend( aString, " \"" );

                iduVarStringAppendLength(
                    aString,
                    aFrom->tableRef->aliasName.stmtText + aFrom->tableRef->aliasName.offset,
                    aFrom->tableRef->aliasName.size );

                iduVarStringAppend( aString, "\"" );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* BUG-36468 */
            if ( QC_IS_NULL_NAME( aFrom->tableRef->tableName ) == ID_TRUE )
            {
                /* nothing to do */
            }
            else
            {
                (void) iduVarStringAppendLength(
                    aString,
                    aFrom->tableRef->tableName.stmtText + aFrom->tableRef->tableName.offset,
                    aFrom->tableRef->tableName.size );

                if ( QC_IS_NAME_MATCHED( aFrom->tableRef->tableName, aFrom->tableRef->aliasName ) )
                {
                    // Table이름과 alias가 동일한 경우 alias는 출력하지 않는다.
                    sUnparseAlias = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( ( QC_IS_NULL_NAME( aFrom->tableRef->aliasName ) == ID_FALSE ) &&
                 ( sUnparseAlias == ID_TRUE ) )
            {
                iduVarStringAppend( aString, " \"" );

                iduVarStringAppendLength(
                    aString,
                    aFrom->tableRef->aliasName.stmtText + aFrom->tableRef->aliasName.offset,
                    aFrom->tableRef->aliasName.size );

                iduVarStringAppend( aString, "\"" );
            }
            else
            {
                // Nothing to do.
            }
        }

    }
    else
    {
        IDE_TEST( unparseFrom( aTemplate,
                               aString,
                               aFrom->left )
                  != IDE_SUCCESS );

        switch( aFrom->joinType )
        {
            case QMS_INNER_JOIN:
                iduVarStringAppend( aString, " INNER JOIN " );
                break;
            case QMS_FULL_OUTER_JOIN:
                iduVarStringAppend( aString, " FULL OUTER JOIN " );
                break;
            case QMS_LEFT_OUTER_JOIN:
                iduVarStringAppend( aString, " LEFT OUTER JOIN " );
                break;
            case QMS_RIGHT_OUTER_JOIN:
                iduVarStringAppend( aString, " RIGHT OUTER JOIN " );
                break;
            default:
                IDE_DASSERT( 0 );
                break;
        }

        IDE_TEST( unparseFrom( aTemplate,
                               aString,
                               aFrom->right )
                  != IDE_SUCCESS );

        iduVarStringAppend( aString, " ON " );

        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         aFrom->onCondition,
                                         0 )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoUtil::unparsePredicate( qcTemplate   * aTemplate,
                                  iduVarString * aString,
                                  qtcNode      * aNode,
                                  idBool         aIsRoot )
{
    mtcNode * sArg;

    IDU_FIT_POINT_FATAL( "qmoUtil::unparsePredicate::__FT__" );

    if ( ( aNode->node.lflag  & MTC_NODE_PUSHED_PRED_FORCE_MASK )
         == MTC_NODE_PUSHED_PRED_FORCE_TRUE ) 
    {
        iduVarStringAppend( aString, "[REMOVED]" );
    }
    else
    {
        switch( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        {
            case MTC_NODE_OPERATOR_AND:
                if( aIsRoot == ID_FALSE )
                {
                    iduVarStringAppend( aString, "(" );
                }

                for( sArg = aNode->node.arguments;
                     sArg != NULL;
                     sArg = sArg->next )
                {
                    IDE_TEST( unparsePredicate( aTemplate,
                                                aString,
                                                (qtcNode *)sArg,
                                                ID_FALSE )
                              != IDE_SUCCESS );

                    if( sArg->next != NULL )
                    {
                        iduVarStringAppend( aString, " AND " );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if( aIsRoot == ID_FALSE )
                {
                    iduVarStringAppend( aString, ")" );
                }
                break;
            case MTC_NODE_OPERATOR_OR:
                if( aIsRoot == ID_FALSE )
                {
                    iduVarStringAppend( aString, "(" );
                }

                for( sArg = aNode->node.arguments;
                     sArg != NULL;
                     sArg = sArg->next )
                {
                    IDE_TEST( unparsePredicate( aTemplate,
                                                aString,
                                                (qtcNode *)sArg,
                                                ID_FALSE )
                              != IDE_SUCCESS );

                    if( sArg->next != NULL )
                    {
                        iduVarStringAppend( aString, " OR " );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if( aIsRoot == ID_FALSE )
                {
                    iduVarStringAppend( aString, ")" );
                }
                break;
            case MTC_NODE_OPERATOR_NOT:
                iduVarStringAppend( aString, "NOT " );
                IDE_TEST( unparsePredicate( aTemplate,
                                            aString,
                                            (qtcNode *)aNode->node.arguments,
                                            ID_FALSE )
                          != IDE_SUCCESS );
                break;
            default:
                IDE_TEST( printExpressionInPlan( aTemplate,
                                                 aString,
                                                 aNode,
                                                 QMO_PRINT_UPPER_NODE_NORMAL )
                          != IDE_SUCCESS );

                if ( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                     == MTC_NODE_LOGICAL_CONDITION_TRUE )
                {
                    for (sArg = aNode->node.arguments;
                         sArg != NULL;
                         sArg = sArg->next)
                    {
                        IDE_TEST( unparsePredicate(aTemplate,
                                                   aString,
                                                   (qtcNode *)sArg,
                                                   ID_FALSE )
                                  != IDE_SUCCESS);
                    }
                }
                else
                {
                    // Nothing to do.
                }
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoUtil::unparseQuerySet( qcTemplate   * aTemplate,
                                 iduVarString * aString,
                                 qmsQuerySet  * aQuerySet )
{
    qmsTarget        * sTarget;
    qmsFrom          * sFrom;
    qmsConcatElement * sGroup;
    qmsConcatElement * sArgs;
    qmsSortColumns   * sSort;

    IDU_FIT_POINT_FATAL( "qmoUtil::unparseQuerySet::__FT__" );

    if( aQuerySet->setOp == QMS_NONE )
    {
        // SELECT
        iduVarStringAppend( aString, "SELECT " );

        if( aQuerySet->SFWGH->selectType == QMS_DISTINCT )
        {
            iduVarStringAppend( aString, "DISTINCT " );
        }
        else
        {
            // Nothing to do.
        }

        for( sTarget = aQuerySet->SFWGH->target;
             sTarget != NULL;
             sTarget = sTarget->next )
        {
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sTarget->targetColumn,
                                             0 )
                         != IDE_SUCCESS );

            if( sTarget->aliasColumnName.name != NULL )
            {
                if( idlOS::strMatch( sTarget->columnName.name,
                                     sTarget->columnName.size,
                                     sTarget->aliasColumnName.name,
                                     sTarget->aliasColumnName.size ) != 0 )
                {
                    iduVarStringAppend( aString, " " );
                    iduVarStringAppendLength( aString,
                                              sTarget->aliasColumnName.name,
                                              sTarget->aliasColumnName.size );
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

            if( sTarget->next != NULL )
            {
                iduVarStringAppend( aString, ", " );
            }
            else
            {
                // Nothing to do.
            }
        }

        // FROM
        iduVarStringAppend( aString, " FROM " );

        for( sFrom = aQuerySet->SFWGH->from;
             sFrom != NULL;
             sFrom = sFrom->next )
        {
            IDE_TEST( unparseFrom( aTemplate,
                                   aString,
                                   sFrom )
                      != IDE_SUCCESS );

            if( sFrom->next != NULL )
            {
                iduVarStringAppend( aString, ", " );
            }
            else
            {
                // Nothing to do.
            }
        }

        // WHERE
        if( aQuerySet->SFWGH->where != NULL )
        {
            iduVarStringAppend( aString, " WHERE " );

            IDE_TEST( unparsePredicate( aTemplate,
                                        aString,
                                        aQuerySet->SFWGH->where,
                                        ID_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // GROUP BY
        if( aQuerySet->SFWGH->group != NULL )
        {
            iduVarStringAppend( aString, " GROUP BY " );

            for( sGroup = aQuerySet->SFWGH->group;
                 sGroup != NULL;
                 sGroup = sGroup->next )
            {
                switch( sGroup->type )
                {
                    case QMS_GROUPBY_NORMAL:
                    case QMS_GROUPBY_NULL:                        
                        IDE_TEST( printExpressionInPlan( aTemplate,
                                                         aString,
                                                         sGroup->arithmeticOrList,
                                                         0 )
                                  != IDE_SUCCESS );
                        break;
                    case QMS_GROUPBY_ROLLUP:
                        iduVarStringAppend( aString, " ROLLUP(" );
                        for( sArgs = sGroup->arguments;
                             sArgs != NULL;
                             sArgs = sArgs->next )
                        {
                            IDE_TEST( printExpressionInPlan( aTemplate,
                                                             aString,
                                                             sArgs->arithmeticOrList,
                                                             0 )
                                         != IDE_SUCCESS );
                        }
                        iduVarStringAppend( aString, " )" );
                        break;
                    case QMS_GROUPBY_CUBE:
                        iduVarStringAppend( aString, " CUBE(" );
                        for( sArgs = sGroup->arguments;
                             sArgs != NULL;
                             sArgs = sArgs->next )
                        {
                            IDE_TEST( printExpressionInPlan( aTemplate,
                                                             aString,
                                                             sArgs->arithmeticOrList,
                                                             0 )
                                      != IDE_SUCCESS );
                        }
                        iduVarStringAppend( aString, " )" );
                        break;
                    case QMS_GROUPBY_GROUPING_SETS:
                        // Not supported yet.
                        IDE_DASSERT( 0 );
                        break;
                    default:
                        // BUG-47620 Complie warning
                        IDE_DASSERT( 0 );
                }

                if( sGroup->next != NULL )
                {
                    iduVarStringAppend( aString, ", " );
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        // HAVING
        if( aQuerySet->SFWGH->having != NULL )
        {
            iduVarStringAppend( aString, " HAVING " );

            IDE_TEST( unparsePredicate( aTemplate,
                                        aString,
                                        aQuerySet->SFWGH->having,
                                        ID_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // Subquery에는 ORDER BY가 존재하지 않는다.

        if( aQuerySet->SFWGH->hierarchy != NULL )
        {
            if( aQuerySet->SFWGH->hierarchy->startWith != NULL )
            {
                iduVarStringAppend( aString, " START WITH " );

                IDE_TEST( unparsePredicate( aTemplate,
                                            aString,
                                            aQuerySet->SFWGH->hierarchy->startWith,
                                            ID_TRUE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            if( aQuerySet->SFWGH->hierarchy->connectBy != NULL )
            {
                iduVarStringAppend( aString, " CONNECT BY " );

                IDE_TEST( unparsePredicate( aTemplate,
                                            aString,
                                            aQuerySet->SFWGH->hierarchy->connectBy,
                                            ID_TRUE )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            if( ( aQuerySet->SFWGH->hierarchy->flag & QMS_HIERARCHY_IGNORE_LOOP_MASK )
                    == QMS_HIERARCHY_IGNORE_LOOP_TRUE )
            {
                iduVarStringAppend( aString, " IGNORE LOOP" );
            }
            else
            {
                // Nothing t odo.
            }

            if( aQuerySet->SFWGH->hierarchy->siblings )
            {
                iduVarStringAppend( aString, " ORDER SIBLINGS BY " );

                for( sSort = aQuerySet->SFWGH->hierarchy->siblings;
                     sSort != NULL;
                     sSort = sSort->next )
                {
                    IDE_TEST( printExpressionInPlan( aTemplate,
                                                     aString,
                                                     sSort->sortColumn,
                                                     0 )
                              != IDE_SUCCESS );

                    if( sSort->isDESC == ID_FALSE )
                    {
                        iduVarStringAppend( aString, " ASC" );
                    }
                    else
                    {
                        iduVarStringAppend( aString, " DESC" );
                    }

                    if( sSort->next != NULL )
                    {
                        iduVarStringAppend( aString, ", " );
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
        IDE_TEST( unparseQuerySet( aTemplate,
                                   aString,
                                   aQuerySet->left )
                  != IDE_SUCCESS );

        switch( aQuerySet->setOp )
        {
            case QMS_UNION:
                iduVarStringAppend( aString, " UNION " );
                break;
            case QMS_UNION_ALL:
                iduVarStringAppend( aString, " UNION ALL " );
                break;
            case QMS_MINUS:
                iduVarStringAppend( aString, " MINUS " );
                break;
            case QMS_INTERSECT:
                iduVarStringAppend( aString, " INTERSECT " );
                break;
            default:
                IDE_DASSERT( 0 );
        }

        IDE_TEST( unparseQuerySet( aTemplate,
                                   aString,
                                   aQuerySet->right )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoUtil::unparseStatement( qcTemplate   * aTemplate,
                                  iduVarString * aString,
                                  qcStatement  * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-1718 Subquery unnesting
 *     AST를 unparsing하여 SQL구문을 생성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree   * sParseTree;
    qmsSortColumns * sSort;

    IDU_FIT_POINT_FATAL( "qmoUtil::unparseStatement::__FT__" );

    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( unparseQuerySet( aTemplate,
                               aString,
                               sParseTree->querySet )
              != IDE_SUCCESS );

    if( sParseTree->orderBy != NULL )
    {
        iduVarStringAppend( aString, " ORDER BY " );

        for( sSort = sParseTree->orderBy;
             sSort != NULL;
             sSort = sSort->next )
        {
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sSort->sortColumn,
                                             0 )
                      != IDE_SUCCESS );

            if( sSort->isDESC == ID_FALSE )
            {
                iduVarStringAppend( aString, " ASC" );
            }
            else
            {
                iduVarStringAppend( aString, " DESC" );
            }

            if( sSort->next != NULL )
            {
                iduVarStringAppend( aString, ", " );
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

    if( sParseTree->limit != NULL )
    {
        iduVarStringAppend( aString, " LIMIT " );

        if( sParseTree->limit->start.hostBindNode == NULL )
        {
            iduVarStringAppendFormat( aString, "%"ID_UINT64_FMT, sParseTree->limit->start.constant );
        }
        else
        {
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sParseTree->limit->start.hostBindNode,
                                             0 )
                      != IDE_SUCCESS );
        }

        iduVarStringAppend( aString, ", " );

        if( sParseTree->limit->count.hostBindNode == NULL )
        {
            iduVarStringAppendFormat( aString, "%"ID_UINT64_FMT, sParseTree->limit->count.constant );
        }
        else
        {
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sParseTree->limit->count.hostBindNode,
                                             0 )
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

IDE_RC qmoUtil::printExpressionInPlan(qcTemplate   * aTemplate,
                                      iduVarString * aString,
                                      qtcNode      * aNode,
                                      UInt           aParenthesisFlag)
{
    qtcNode        * sArgNode;
    qtcOverColumn  * sOverColumn;
    qmsTableRef    * sTableRef;
    UInt             i;

    IDU_FIT_POINT_FATAL( "qmoUtil::printExpressionInPlan::__FT__" );

    if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // PROJ-1718 Subquery unnesting
        iduVarStringAppend( aString, "(" );

        IDE_TEST( unparseStatement( aTemplate,
                                    aString,
                                    aNode->subquery )
                  != IDE_SUCCESS );

        iduVarStringAppend( aString, ")" );
    }
    else
    {
        if ( ( aNode->node.lflag & MTC_NODE_INDIRECT_MASK )
             == MTC_NODE_INDIRECT_TRUE )
        {
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             (qtcNode*) aNode->node.arguments,
                                             aParenthesisFlag )
                      != IDE_SUCCESS );
        }
        else
        {

            if ( aNode->node.module == & qtc::columnModule )
            {
                if ( ( ( aNode->lflag & QTC_NODE_OUT_REF_COLUMN_MASK )
                       == QTC_NODE_OUT_REF_COLUMN_TRUE ) &&
                     ( ( aTemplate->flag & QC_TMP_SHARD_OUT_REF_COL_TO_BIND_MASK )
                       == QC_TMP_SHARD_OUT_REF_COL_TO_BIND_TRUE ) )
                {
                    iduVarStringAppend( aString, "?" );
                }
                else
                {
                    if ( QTC_TEMPLATE_IS_COLUMN( aTemplate, aNode ) == ID_TRUE )
                    {
                        /* PROJ-1090 Function-based Index */
                        if ( aNode->node.orgNode != NULL )
                        {
                            iduVarStringAppend( aString, "[" );

                            // print expression
                            IDE_TEST( printExpressionInPlan( aTemplate,
                                                             aString,
                                                             (qtcNode *)aNode->node.orgNode,
                                                             aParenthesisFlag )
                                      != IDE_SUCCESS );

                            iduVarStringAppend( aString, "]" );
                        }
                        else
                        {
                            sTableRef = aTemplate->tableMap[aNode->node.table].from->tableRef;

                            if ( QC_IS_NULL_NAME( aNode->userName ) == ID_FALSE )
                            {
                                // BUG-18300
                                IDE_DASSERT( aNode->userName.stmtText != NULL );

                                iduVarStringAppendLength(
                                    aString,
                                    aNode->userName.stmtText + aNode->userName.offset,
                                    aNode->userName.size );

                                iduVarStringAppend( aString,
                                                    "." );
                            }
                            else
                            {
                                // Nothing to do.
                            }

                            if ( QC_IS_NULL_NAME( aNode->tableName ) == ID_FALSE )
                            {
                                if ( QC_IS_NULL_NAME( sTableRef->aliasName ) == ID_FALSE )
                                {
                                    // BUG-18300
                                    IDE_DASSERT( sTableRef->aliasName.stmtText != NULL );

                                    iduVarStringAppendLength(
                                        aString,
                                        sTableRef->aliasName.stmtText + sTableRef->aliasName.offset,
                                        sTableRef->aliasName.size );

                                    iduVarStringAppend( aString,
                                                        "." );
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

                            /* BUG-31570
                             * DDL이 빈번한 환경에서 plan text를 안전하게 보여주는 방법이 필요하다.
                             */
                            IDE_DASSERT( sTableRef->columnsName != NULL );

                            iduVarStringAppend(
                                aString,
                                sTableRef->columnsName[aNode->node.column] );
                        }
                    }
                    else
                    {
                        if ( QC_IS_NULL_NAME( aNode->position ) == ID_FALSE )
                        {
                            // prior 출력
                            if ( ( aNode->lflag & QTC_NODE_PRIOR_MASK )
                                 == QTC_NODE_PRIOR_EXIST )
                            {
                                iduVarStringAppend( aString,
                                                    "PRIOR " );
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        
                            // BUG-18300
                            IDE_DASSERT( aNode->position.stmtText != NULL );

                            iduVarStringAppendLength(
                                aString,
                                aNode->position.stmtText + aNode->position.offset,
                                aNode->position.size );
                        }
                        else
                        {
                            if ( QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE )
                            {
                                // prior 출력
                                if ( ( aNode->lflag & QTC_NODE_PRIOR_MASK )
                                     == QTC_NODE_PRIOR_EXIST )
                                {
                                    iduVarStringAppend( aString,
                                                        "PRIOR " );
                                }
                                else
                                {
                                    // Nothing to do.
                                }

                                if ( QC_IS_NULL_NAME( aNode->tableName ) == ID_FALSE )
                                {
                                    if ( QC_IS_NULL_NAME( aNode->userName ) == ID_FALSE )
                                    {
                                        // BUG-18300
                                        IDE_DASSERT( aNode->userName.stmtText != NULL );

                                        iduVarStringAppendLength(
                                            aString,
                                            aNode->userName.stmtText + aNode->userName.offset,
                                            aNode->userName.size );
                                
                                        iduVarStringAppend( aString,
                                                            "." );
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                
                                    // BUG-18300
                                    IDE_DASSERT( aNode->tableName.stmtText != NULL );

                                    iduVarStringAppendLength(
                                        aString,
                                        aNode->tableName.stmtText + aNode->tableName.offset,
                                        aNode->tableName.size );
                                
                                    iduVarStringAppend( aString,
                                                        "." );
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            
                                // BUG-18300
                                IDE_DASSERT( aNode->columnName.stmtText != NULL );

                                iduVarStringAppendLength(
                                    aString,
                                    aNode->columnName.stmtText + aNode->columnName.offset,
                                    aNode->columnName.size );
                            }
                            else
                            {
                                //iduVarStringAppend( aString,
                                //                    "[" );
                                //iduVarStringAppend( aString,
                                //                    (const SChar *) aNode->node.module->names->string);
                                //iduVarStringAppend( aString,
                                //                    "]" );
                            }
                        }
                    }
                }
            }
            else if ( aNode->node.module == & qtc::valueModule )
            {
                // 상수로 변환되기 전의 상수 expression을 출력한다.
                // PROJ-1718 또는 VIEW operator가 생성된 경우 원래 expression을 출력한다.
                if ( aNode->node.orgNode != NULL )
                {
                    IDE_DASSERT( aNode->node.orgNode != NULL );
                    
                    // print expression
                    IDE_TEST( printExpressionInPlan( aTemplate,
                                                     aString,
                                                     (qtcNode*) aNode->node.orgNode,
                                                     aParenthesisFlag )
                              != IDE_SUCCESS );
                }
                else if( QTC_IS_AGGREGATE( aNode ) == ID_TRUE )
                {
                    // PROJ-2179
                    // Aggregate function이 materialize된 후 value module로
                    // 변경된 경우에도 올바르게 결과를 출력해주도록 한다.

                    IDE_DASSERT( aNode->columnName.stmtText != NULL );
                    
                    iduVarStringAppendLength(
                        aString,
                        aNode->columnName.stmtText + aNode->columnName.offset,
                        aNode->columnName.size );

                    iduVarStringAppend( aString,
                                        "(" );

                    if ( ( aNode->node.lflag & MTC_NODE_DISTINCT_MASK )
                         == MTC_NODE_DISTINCT_TRUE )
                    {
                        iduVarStringAppend( aString,
                                            "DISTINCT " );
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sArgNode = (qtcNode*) aNode->node.arguments;

                    if( sArgNode == NULL )
                    {
                        iduVarStringAppend( aString, "*" );
                    }
                    else
                    {
                        for ( i = (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK);
                              i > 0; i--)
                        {
                            // print expression
                            IDE_TEST( printExpressionInPlan( aTemplate,
                                                             aString,
                                                             sArgNode,
                                                             QMO_PRINT_UPPER_NODE_NORMAL )
                                      != IDE_SUCCESS );
                            
                            if ( i > 1 )
                            {
                                iduVarStringAppend( aString,
                                                    ", " );
                            }
                            else
                            {
                                // Nothing to do.
                            }
                            
                            sArgNode = (qtcNode*) sArgNode->node.next;
                        }
                    }
                    
                    iduVarStringAppend( aString,
                                        ")" );
                }
                else
                {
                    if ( QC_IS_NULL_NAME( aNode->position ) == ID_FALSE )
                    {
                        // BUG-18300
                        IDE_DASSERT( aNode->position.stmtText != NULL );
                        
                        iduVarStringAppendLength(
                            aString,
                            aNode->position.stmtText + aNode->position.offset,
                            aNode->position.size );
                    }
                    else
                    {
                        //iduVarStringAppend( aString,
                        //                    "[" );
                        //iduVarStringAppend( aString,
                        //                    (const SChar *) aNode->node.module->names->string);
                        //iduVarStringAppend( aString,
                        //                    "]" );
                    }
                }
            }
            else if ( aNode->node.module == & qtc::spFunctionCallModule )
            {
                if ( QC_IS_NULL_NAME( aNode->tableName ) == ID_FALSE )
                {
                    // BUG-18300
                    IDE_DASSERT( aNode->tableName.stmtText != NULL );
                
                    iduVarStringAppendLength(
                        aString,
                        aNode->tableName.stmtText + aNode->tableName.offset,
                        aNode->tableName.size );
                    iduVarStringAppend( aString,
                                        "." );
                }
                else
                {
                    // Nothing to do.
                }
                
                IDE_DASSERT( QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE );
                
                // BUG-18300
                IDE_DASSERT( aNode->columnName.stmtText != NULL );
                
                iduVarStringAppendLength(
                    aString,
                    aNode->columnName.stmtText + aNode->columnName.offset,
                    aNode->columnName.size );
                
                iduVarStringAppend( aString,
                                    "(" );
                
                sArgNode = (qtcNode*) aNode->node.arguments;
                
                for ( i = (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK);
                      i > 0; i--)
                {
                    // print expression
                    IDE_TEST( printExpressionInPlan( aTemplate,
                                                     aString,
                                                     sArgNode,
                                                     QMO_PRINT_UPPER_NODE_NORMAL )
                              != IDE_SUCCESS );
                    
                    if ( i > 1 )
                    {
                        iduVarStringAppend( aString,
                                            ", " );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    sArgNode = (qtcNode*) sArgNode->node.next;
                }
                
                iduVarStringAppend( aString,
                                    ")" );
            }
            else if ( aNode->node.module == & qtc::passModule )
            {
                // indirect node가 아닌 conversion을 위한 passNode가 있다.
                
                // print expression
                IDE_TEST( printExpressionInPlan( aTemplate,
                                                 aString,
                                                 (qtcNode*) aNode->node.arguments,
                                                 aParenthesisFlag )
                          != IDE_SUCCESS );
            }
            else if( aNode->node.module == &gQtcRidModule )
            {
                iduVarStringAppend( aString,
                                    (const SChar *) aNode->node.module->names->string);
            }
            else
            {
                // print node
                if( ( aNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                    == MTC_NODE_LOGICAL_CONDITION_TRUE )
                {
                    IDE_TEST( unparsePredicate( aTemplate,
                                                aString,
                                                aNode,
                                                ID_TRUE )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( printNodeFormat( aTemplate,
                                               aString,
                                               aNode,
                                               aParenthesisFlag )
                              != IDE_SUCCESS );
                }
            }
        }

        // PROJ-2179
        // Analytic function의 OVER절 및 ANALYTIC절을 출력한다.
        if ( aNode->overClause != NULL )
        {
            iduVarStringAppend( aString,
                                " OVER (" );
            
            if ( aNode->overClause->partitionByColumn != NULL )
            {
                iduVarStringAppend( aString,
                                    "PARTITION BY " );
            
                for ( sOverColumn = aNode->overClause->partitionByColumn, i = 0;
                      sOverColumn != NULL;
                      sOverColumn = sOverColumn->next, i++ )
                {
                    if ( ( sOverColumn->flag & QTC_OVER_COLUMN_MASK )
                         == QTC_OVER_COLUMN_ORDER_BY )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    if ( i > 0 )
                    {
                        iduVarStringAppend( aString,
                                            ", " );
                    }
                    else
                    {
                        // Nothing to do.
                    }                    

                    IDE_TEST( printExpressionInPlan( aTemplate,
                                                     aString,
                                                     sOverColumn->node,
                                                     aParenthesisFlag )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( aNode->overClause->orderByColumn != NULL )
            {
                if ( aNode->overClause->partitionByColumn != NULL )
                {
                    iduVarStringAppend( aString,
                                        " " );
                }
                else
                {
                    // Nothing to do.
                }
                
                iduVarStringAppend( aString,
                                    "ORDER BY " );
                
                for ( sOverColumn = aNode->overClause->orderByColumn, i = 0;
                      sOverColumn != NULL;
                      sOverColumn = sOverColumn->next, i++ )
                {
                    if ( ( sOverColumn->flag & QTC_OVER_COLUMN_MASK )
                         != QTC_OVER_COLUMN_ORDER_BY )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    
                    if ( i > 0 )
                    {
                        iduVarStringAppend( aString,
                                            ", " );
                    }
                    else
                    {
                        // Nothing to do.
                    }                    

                    IDE_TEST( printExpressionInPlan( aTemplate,
                                                     aString,
                                                     sOverColumn->node,
                                                     aParenthesisFlag )
                              != IDE_SUCCESS );

                    if ( ( sOverColumn->flag & QTC_OVER_COLUMN_ORDER_MASK )
                         == QTC_OVER_COLUMN_ORDER_DESC )
                    {
                        iduVarStringAppend( aString,
                                            " DESC" );
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
            
            iduVarStringAppend( aString,
                                ")" );
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

IDE_RC qmoUtil::printNodeFormat(qcTemplate   * aTemplate,
                                iduVarString * aString,
                                qtcNode      * aNode,
                                UInt           aParenthesisFlag)
{
    qtcNode        * sArgNode;
    mtcColumn      * sColumn;
    UInt             i;

    IDU_FIT_POINT_FATAL( "qmoUtil::printNodeFormat::__FT__" );

    if ( ( aNode->node.lflag & MTC_NODE_PRINT_FMT_MASK )
         == MTC_NODE_PRINT_FMT_PREFIX_PA )
    {
        // operator
        iduVarStringAppend( aString,
                            (const SChar *) aNode->node.module->names->string);
        
        iduVarStringAppend( aString,
                            "(" );
        
        // distinct 출력
        if ( ( aNode->node.lflag & MTC_NODE_DISTINCT_MASK )
             == MTC_NODE_DISTINCT_TRUE )
        {
            iduVarStringAppend( aString,
                                "DISTINCT " );
        }
        else
        {
            // Nothing to do.
        }
        
        sArgNode = (qtcNode*) aNode->node.arguments;
        
        for ( i = (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK);
              i > 0; i--)
        {
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
            
            if ( i > 1 )
            {
                iduVarStringAppend( aString,
                                    ", " );
            }
            else
            {
                // Nothing to do.
            }
            
            sArgNode = (qtcNode*) sArgNode->node.next;
        }
        
        iduVarStringAppend( aString,
                            ")" );
    }
    else if ( ( aNode->node.lflag & MTC_NODE_PRINT_FMT_MASK )
              == MTC_NODE_PRINT_FMT_PREFIX_SP )
    {
        IDE_DASSERT(
            (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );
        
        // operator
        iduVarStringAppend( aString,
                            (const SChar *) aNode->node.module->names->string);
        
        iduVarStringAppend( aString,
                            " " );
        
        sArgNode = (qtcNode *)aNode->node.arguments;
        
        // print expression
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         sArgNode,
                                         QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );
    }
    else if ( ( aNode->node.lflag & MTC_NODE_PRINT_FMT_MASK )
              == MTC_NODE_PRINT_FMT_INFIX_SP )
    {
        IDE_DASSERT(
            (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 );
        
        // left
        sArgNode = (qtcNode *)aNode->node.arguments;
        
        // print expression
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         sArgNode,
                                         QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );
        
        iduVarStringAppend( aString,
                            " " );
        
        // operator
        iduVarStringAppend( aString,
                            (const SChar *) aNode->node.module->names->string);
        
        iduVarStringAppend( aString,
                            " " );
        
        // right
        sArgNode = (qtcNode *)sArgNode->node.next;
        
        // print expression
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         sArgNode,
                                         QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );
    }
    else if ( ( aNode->node.lflag & MTC_NODE_PRINT_FMT_MASK )
              == MTC_NODE_PRINT_FMT_INFIX )
    {
        IDE_DASSERT(
            (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 );
        
        if ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
             == QMO_PRINT_UPPER_NODE_COMPARE )
        {
            iduVarStringAppend( aString,
                                "(" );
        }
        else
        {
            // Nothing to do.
        }
            
        // left
        sArgNode = (qtcNode *)aNode->node.arguments;

        // 비교연산자가 중첩되는 경우 괄호를 추가해야 한다.
        // 하위 노드가 indirect node나 상수화된 노드일 수 있으므로 flag를 내린다.
        // ex) (1=1) = (2=2)
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         sArgNode,
                                         QMO_PRINT_UPPER_NODE_COMPARE )
                  != IDE_SUCCESS );
        
        iduVarStringAppend( aString,
                            " " );
        
        // operator
        iduVarStringAppend( aString,
                            (const SChar *) aNode->node.module->names->string);
        
        iduVarStringAppend( aString,
                            " " );
        
        // right
        sArgNode = (qtcNode *)sArgNode->node.next;
        
        // 비교연산자가 중첩되는 경우 괄호를 추가해야 한다.
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         sArgNode,
                                         QMO_PRINT_UPPER_NODE_COMPARE )
                  != IDE_SUCCESS );
        
        if ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
             == QMO_PRINT_UPPER_NODE_COMPARE )
        {
            iduVarStringAppend( aString,
                                ")" );
        }
        else
        {
            // Nothing to do.
        }
    }
    else if ( ( aNode->node.lflag & MTC_NODE_PRINT_FMT_MASK )
              == MTC_NODE_PRINT_FMT_POSTFIX_SP )
    {
        IDE_DASSERT(
            (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );
        
        sArgNode = (qtcNode *)aNode->node.arguments;
        
        // print expression
        IDE_TEST( printExpressionInPlan( aTemplate,
                                         aString,
                                         sArgNode,
                                         QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );
        
        iduVarStringAppend( aString,
                            " " );
        
        // operator
        iduVarStringAppend( aString,
                            (const SChar *) aNode->node.module->names->string);
    }
    else /* MTC_NODE_PRINT_FMT_MISC */
    {
        // mtfCast, mtfCount, mtfBetween, mtfLike, mtfList, mtfNotExists, mtfNotUnique,
        // mtfMinus, mtfMultiply, mtfDivide, mtfAdd2, mtfSubtract2,
        // mtfGetBlogLocator, mtfGetClobLocator
        
        if ( aNode->node.module == &mtfCast )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                "(" );
            
            sArgNode = (qtcNode*) aNode->node.arguments;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " AS " );
            
            sColumn = QTC_TMPL_COLUMN( aTemplate,
                                       ((qtcNode*) aNode->node.funcArguments) );
            
            // datatype
            iduVarStringAppend( aString,
                                (const SChar *) sColumn->module->names->string);
            
            if ( ( sColumn->module->flag & MTD_CREATE_PARAM_MASK )
                 == MTD_CREATE_PARAM_PRECISION )
            {
                iduVarStringAppendFormat( aString,
                                          "(%"ID_INT32_FMT")",
                                          (UInt) sColumn->precision );
            }
            else if ( ( sColumn->module->flag & MTD_CREATE_PARAM_MASK )
                      == MTD_CREATE_PARAM_PRECISIONSCALE )
            {
                iduVarStringAppendFormat( aString,
                                          "(%"ID_INT32_FMT",%"ID_INT32_FMT")",
                                          (UInt) sColumn->precision,
                                          (UInt) sColumn->scale );
            }
            else
            {
                // Nothing to do.
            }
            
            iduVarStringAppend( aString,
                                ")" );
        }
        else if ( ( aNode->node.module == &mtfCount ) ||
                  ( aNode->node.module == &mtfCountKeep ) )
        {
            if ( aNode->node.module == &mtfCount )
            {
                IDE_DASSERT(
                    ( (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 0 ) ||
                    ( (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 ) );
            }
            else
            {
                /* Nothing to do */
            }

            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                "(" );
            
            // distinct 출력
            if ( ( aNode->node.lflag & MTC_NODE_DISTINCT_MASK )
                 == MTC_NODE_DISTINCT_TRUE )
            {
                iduVarStringAppend( aString,
                                    "DISTINCT " );
            }
            else
            {
                // Nothing to do.
            }
            
            sArgNode = (qtcNode*) aNode->node.arguments;
            
            if ( sArgNode == NULL )
            {
                // count(*)
                iduVarStringAppend( aString,
                                    "*" );
            }
            else
            {
                // print expression
                IDE_TEST( printExpressionInPlan( aTemplate,
                                                 aString,
                                                 sArgNode,
                                                 QMO_PRINT_UPPER_NODE_NORMAL )
                          != IDE_SUCCESS );
            }
            
            iduVarStringAppend( aString,
                                ")" );
        }
        else if ( (aNode->node.module == &mtfBetween) ||
                  (aNode->node.module == &mtfNotBetween) )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 3 );
            
            // left
            sArgNode = (qtcNode *)aNode->node.arguments;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " " );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                " " );
            
            // right 1
            sArgNode = (qtcNode *)sArgNode->node.next;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " AND " );
            
            // right 2
            sArgNode = (qtcNode *)sArgNode->node.next;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
        }
        else if ( (aNode->node.module == &mtfLike) ||
                  (aNode->node.module == &mtfNotLike) )
        {
            IDE_DASSERT(
                ( (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 ) ||
                ( (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 3 ) );
            
            // left
            sArgNode = (qtcNode *)aNode->node.arguments;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " " );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                " " );
            
            // right 1
            sArgNode = (qtcNode *)sArgNode->node.next;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
            
            // right 2
            sArgNode = (qtcNode *)sArgNode->node.next;
            
            if ( sArgNode != NULL )
            {
                iduVarStringAppend( aString,
                                    " ESCAPE " );
                
                // print expression
                IDE_TEST( printExpressionInPlan( aTemplate,
                                                 aString,
                                                 sArgNode,
                                                 QMO_PRINT_UPPER_NODE_NORMAL )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( aNode->node.module == &mtfList )
        {
            iduVarStringAppend( aString,
                                "(" );
            
            sArgNode = (qtcNode*) aNode->node.arguments;
            
            for ( i = (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK);
                  i > 0; i--)
            {
                // print expression
                IDE_TEST( printExpressionInPlan( aTemplate,
                                                 aString,
                                                 sArgNode,
                                                 QMO_PRINT_UPPER_NODE_NORMAL )
                          != IDE_SUCCESS );
                
                if ( i > 1 )
                {
                    iduVarStringAppend( aString,
                                        ", " );
                }
                else
                {
                    // Nothing to do.
                }
                
                sArgNode = (qtcNode*) sArgNode->node.next;
            }
            
            iduVarStringAppend( aString,
                                ")" );
        }
        else if ( aNode->node.module == &mtfNotExists )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );
        
            iduVarStringAppend( aString,
                                "NOT EXISTS " );
            
            sArgNode = (qtcNode *)aNode->node.arguments;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
        }
        else if ( aNode->node.module == &mtfNotUnique )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );
        
            iduVarStringAppend( aString,
                                "NOT UNIQUE " );
            
            sArgNode = (qtcNode *)aNode->node.arguments;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );
        }
        else if ( aNode->node.module == &mtfMinus )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );
            
            // BUG-19180
            // 상위 노드가 minus이면 괄호를 추가한다.
            if ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
            {
                iduVarStringAppend( aString,
                                    "(" );
            }
            else
            {
                // Nothing to do.
            }
            
            iduVarStringAppend( aString,
                                "-" );
            
            // right
            sArgNode = (qtcNode *)aNode->node.arguments;
            
            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_MINUS )
                      != IDE_SUCCESS );
            
            if ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
            {
                iduVarStringAppend( aString,
                                    ")" );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( aNode->node.module == &mtfMultiply )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 );

            // BUG-19180
            // 상위 노드가 minus이거나
            // 상위 노드가 '/'의 오른쪽 노드면 괄호를 추가한다.
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_DIV )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    "(" );
            }
            else
            {
                // Nothing to do.
            }
            
            // left
            sArgNode = (qtcNode *)aNode->node.arguments;

            // BUG-19180
            // '*'나 '/'의 인자로 '+'나 '-'가 오는 경우 괄호를 추가해야 한다.
            // 하위 노드가 indirect node나 상수화된 노드일 수 있으므로 flag를 내린다.
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_MUL )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " " );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                " " );
            
            // right
            sArgNode = (qtcNode *)sArgNode->node.next;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_MUL|QMO_PRINT_RIGHT_NODE_TRUE )
                      != IDE_SUCCESS );
            
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_DIV )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    ")" );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( aNode->node.module == &mtfDivide )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 );

            // BUG-19180
            // 상위 노드가 minus이거나
            // 상위 노드가 '/'의 오른쪽 노드면 괄호를 추가한다.
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_DIV )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    "(" );
            }
            else
            {
                // Nothing to do.
            }
            
            // left
            sArgNode = (qtcNode *)aNode->node.arguments;

            // BUG-19180
            // '*'나 '/'의 인자로 '+'나 '-'가 오는 경우 괄호를 추가해야 한다.
            // 하위 노드가 indirect node나 상수화된 노드일 수 있으므로 flag를 내린다.
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_DIV )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " " );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                " " );
            
            // right
            sArgNode = (qtcNode *)sArgNode->node.next;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_DIV|QMO_PRINT_RIGHT_NODE_TRUE )
                      != IDE_SUCCESS );
            
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_DIV )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    ")" );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( aNode->node.module == &mtfAdd2 )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 );

            // 상위 노드가 minus이거나
            // 상위 노드가 '*','/'이거나
            // 상위 노드가 '-'의 오른쪽 노드라면 괄호를 추가한다.
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MUL )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_DIV )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_SUB )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    "(" );
            }
            else
            {
                // Nothing to do.
            }
    
            // left
            sArgNode = (qtcNode *)aNode->node.arguments;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_ADD )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " " );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                " " );
            
            // right
            sArgNode = (qtcNode *)sArgNode->node.next;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_ADD|QMO_PRINT_RIGHT_NODE_TRUE )
                      != IDE_SUCCESS );
            
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MUL )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_DIV )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_SUB )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    ")" );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( aNode->node.module == &mtfSubtract2 )
        {
            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 2 );

            // 상위 노드가 minus이거나
            // 상위 노드가 '*','/'이거나
            // 상위 노드가 '-'의 오른쪽 노드라면 괄호를 추가한다.
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MUL )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_DIV )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_SUB )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    "(" );
            }
            else
            {
                // Nothing to do.
            }
    
            // left
            sArgNode = (qtcNode *)aNode->node.arguments;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_SUB )
                      != IDE_SUCCESS );
            
            iduVarStringAppend( aString,
                                " " );
            
            // operator
            iduVarStringAppend( aString,
                                (const SChar *) aNode->node.module->names->string);
            
            iduVarStringAppend( aString,
                                " " );
            
            // right
            sArgNode = (qtcNode *)sArgNode->node.next;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             QMO_PRINT_UPPER_NODE_SUB|QMO_PRINT_RIGHT_NODE_TRUE )
                      != IDE_SUCCESS );
            
            if ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MINUS )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_MUL )
                 ||
                 ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                   == QMO_PRINT_UPPER_NODE_DIV )
                 ||
                 ( ( ( aParenthesisFlag & QMO_PRINT_UPPER_NODE_MASK )
                     == QMO_PRINT_UPPER_NODE_SUB )
                   &&
                   ( ( aParenthesisFlag & QMO_PRINT_RIGHT_NODE_MASK )
                     == QMO_PRINT_RIGHT_NODE_TRUE )
                   )
                 )
            {
                iduVarStringAppend( aString,
                                    ")" );
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ((aNode->node.module == &mtfGetBlobLocator) ||
                 (aNode->node.module == &mtfGetClobLocator) ||
                 (aNode->node.module == &mtfGetBlobValue) ||
                 (aNode->node.module == &mtfGetClobValue))
        {
            /*
             * BUG-40991 mtfGetBlobValue, mtfGetClobValue
             */

            IDE_DASSERT(
                (aNode->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) == 1 );

            sArgNode = (qtcNode *)aNode->node.arguments;

            // print expression
            IDE_TEST( printExpressionInPlan( aTemplate,
                                             aString,
                                             sArgNode,
                                             aParenthesisFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_DASSERT( 0 );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
