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

#include <qmsPreservedTable.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmv.h>

IDE_RC qmsPreservedTable::initialize( qcStatement  * aStatement,
                                      qmsSFWGH     * aSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPreservedInfo  * sPreservedTable;
    UInt                i;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsPreservedInfo),
                                             (void**) & sPreservedTable )
              != IDE_SUCCESS );

    // init
    sPreservedTable->tableCount = 0;
    sPreservedTable->stopFlag = ID_FALSE;

    // key preserved property
    if ( QCU_KEY_PRESERVED_TABLE == 1 )
    {
        sPreservedTable->useKeyPreservedTable = ID_TRUE;
    }
    else
    {
        sPreservedTable->useKeyPreservedTable = ID_FALSE;
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_KEY_PRESERVED_TABLE );
            
    for ( i = 0; i < QC_MAX_REF_TABLE_CNT; i++ )
    {
        sPreservedTable->tableRef[i] = NULL;
        sPreservedTable->uniqueInfo[i] = NULL;
        sPreservedTable->tableMap[i] = NULL;
        sPreservedTable->isKeyPreserved[i] = ID_FALSE;
        sPreservedTable->result[i] = ID_FALSE;
    }

    sPreservedTable->mIsInValid = ID_FALSE; /* BUG-46124 */

    aSFWGH->preservedInfo = sPreservedTable;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmsPreservedTable::addTable( qcStatement  * aStatement,
                                    qmsSFWGH     * aSFWGH,
                                    qmsTableRef  * aTableRef )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPreservedInfo  * sPreservedTable;
    UShort            * sNewRow;
    UInt                i;

    sPreservedTable = aSFWGH->preservedInfo;

    if ( sPreservedTable != NULL )
    {
        if ( sPreservedTable->useKeyPreservedTable == ID_TRUE )
        {
            if ( sPreservedTable->stopFlag == ID_FALSE )
            {
                IDE_DASSERT( sPreservedTable->tableCount + 1 < QC_MAX_REF_TABLE_CNT );

                // add tableRef
                sPreservedTable->tableRef[sPreservedTable->tableCount] = aTableRef;

                // add new row
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(UShort) * QC_MAX_REF_TABLE_CNT,
                                                         (void**) & sNewRow )
                          != IDE_SUCCESS );

                // ushort_max로 초기화
                for ( i = 0; i < QC_MAX_REF_TABLE_CNT; i++ )
                {
                    sNewRow[i] = ID_USHORT_MAX;
                }

                sPreservedTable->tableMap[sPreservedTable->tableCount] = sNewRow;
    
                sPreservedTable->tableCount++;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* BUG-46124 */
            if ( aTableRef->view == NULL )
            {
                IDE_DASSERT( sPreservedTable->tableCount + 1 < QC_MAX_REF_TABLE_CNT );

                // add tableRef
                sPreservedTable->tableRef[sPreservedTable->tableCount] = aTableRef;

                sPreservedTable->tableCount++;
            }
            else
            {
                /* Nothing to do */
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
qmsPreservedTable::isUnique( qcStatement       * aStatement,
                             qmsPreservedInfo  * aPreservedTable,
                             qtcNode           * aFromNode,
                             qtcNode           * aToNode,
                             idBool            * aIsUnique )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *     composite unique column에 의한 partial unique를 기록하고 확인한다.
 *
 ***********************************************************************/

    qmsTableRef   * sTableRef = NULL;
    qcmTableInfo  * sTableInfo;
    qmsUniqueInfo * sUniqueInfo;
    qcmIndex      * sIndex;
    mtcColumn     * sColumn;
    idBool          sFoundInfo;
    idBool          sIsUnique;
    UInt            sTableIndex;
    UInt            i;
    UInt            j;

    *aIsUnique = ID_FALSE;
    
    for ( i = 0; i < aPreservedTable->tableCount; i++ )
    {
        if ( aPreservedTable->tableRef[i]->table == aFromNode->node.baseTable )
        {
            sTableRef = aPreservedTable->tableRef[i];
            sTableIndex = i;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sTableRef != NULL )
    {
        sTableInfo = sTableRef->tableInfo;
        sColumn = QTC_STMT_COLUMN( aStatement, aFromNode );

        // 1. single unique column인 경우 바로 찾는다.
        for ( i = 0; i < sTableInfo->indexCount; i++ )
        {
            sIndex = & (sTableInfo->indices[i]);

            if ( sIndex->isUnique == ID_TRUE )
            {
                if ( sIndex->keyColCount == 1 )
                {
                    if ( sIndex->keyColumns[0].column.id == sColumn->column.id )
                    {
                        *aIsUnique = ID_TRUE;
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
            else
            {
                // Nothing to do.
            }
        }

        if ( *aIsUnique == ID_FALSE )
        {
            // 2. composite unique column인 경우
            sFoundInfo = ID_FALSE;

            // 2.1. uniqueInfo에서 찾아본다.
            for ( sUniqueInfo = aPreservedTable->uniqueInfo[sTableIndex];
                  sUniqueInfo != NULL;
                  sUniqueInfo = sUniqueInfo->next )
            {
                if ( sUniqueInfo->toTable == aToNode->node.baseTable )
                {
                    sFoundInfo = ID_TRUE;
                    sIsUnique = ID_TRUE;
                    
                    for ( i = 0; i < sUniqueInfo->fromColumnCount; i++ )
                    {
                        if ( sUniqueInfo->fromColumn[i] == sColumn->column.id )
                        {
                            // 찾았다면 uint_max로 변경한다.
                            sUniqueInfo->fromColumn[i] = ID_UINT_MAX;
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        if ( sUniqueInfo->fromColumn[i] != ID_UINT_MAX )
                        {
                            sIsUnique = ID_FALSE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }

                    if ( sIsUnique == ID_TRUE )
                    {
                        *aIsUnique = ID_TRUE;
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
            
            if ( sFoundInfo == ID_FALSE )
            {
                // 2.2 uniqueInfo를 추가하고 기록한다.
                for ( i = 0; i < sTableInfo->indexCount; i++ )
                {
                    sIndex = & (sTableInfo->indices[i]);
                    
                    if ( sIndex->isUnique == ID_TRUE )
                    {
                        if ( sIndex->keyColCount > 1 )
                        {
                            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                          ID_SIZEOF(qmsUniqueInfo),
                                          (void**) & sUniqueInfo )
                                      != IDE_SUCCESS );

                            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                          ID_SIZEOF(UInt) * sIndex->keyColCount,
                                          (void**) & sUniqueInfo->fromColumn )
                                      != IDE_SUCCESS );

                            // init
                            for ( j = 0; j < sIndex->keyColCount; j++ )
                            {
                                if ( sIndex->keyColumns[j].column.id == sColumn->column.id )
                                {
                                    // mark
                                    sUniqueInfo->fromColumn[j] = ID_UINT_MAX;
                                }
                                else
                                {
                                    sUniqueInfo->fromColumn[j] = sIndex->keyColumns[j].column.id;
                                }
                            }
                            sUniqueInfo->fromColumnCount = sIndex->keyColCount;
                            sUniqueInfo->toTable = aToNode->node.baseTable;
                            
                            // link
                            sUniqueInfo->next = aPreservedTable->uniqueInfo[sTableIndex];
                            aPreservedTable->uniqueInfo[sTableIndex] = sUniqueInfo;
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
qmsPreservedTable::mark( qmsPreservedInfo  * aPreservedTable,
                         UShort              aFrom,
                         UShort              aTo )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  sFrom = ID_UINT_MAX;
    UInt  sTo = ID_UINT_MAX;
    UInt  i;

    for ( i = 0; i < aPreservedTable->tableCount; i++ )
    {
        if ( aPreservedTable->tableRef[i]->table == aFrom )
        {
            sFrom = i;
        }
        else
        {
            // Nothing to do.
        }

        if ( aPreservedTable->tableRef[i]->table == aTo )
        {
            sTo = i;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( ( sFrom != ID_UINT_MAX ) && ( sTo != ID_UINT_MAX ) && ( sFrom != sTo ) )
    {
        aPreservedTable->tableMap[sFrom][sTo] = 1;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
}

IDE_RC
qmsPreservedTable::unmarkAll( qmsPreservedInfo  * aPreservedTable )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *     addTable 상태로 초기화
 *
 ***********************************************************************/

    UInt    i;
    UInt    j;

    for ( i = 0; i < aPreservedTable->tableCount; i++ )
    {
        for ( j = 0; j < aPreservedTable->tableCount; j++ )
        {
            if ( aPreservedTable->tableMap[i][j] < ID_USHORT_MAX )
            {
                aPreservedTable->tableMap[i][j] = ID_USHORT_MAX;
            }
            else
            {
                // Nothing to do
            }
        }
    }

    // 이후로는 추가하지 않는다.
    aPreservedTable->stopFlag = ID_TRUE;
    
    return IDE_SUCCESS;
}

IDE_RC
qmsPreservedTable::addPredicate( qcStatement  * aStatement,
                                 qmsSFWGH     * aSFWGH,
                                 qtcNode      * aNode )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPreservedInfo  * sPreservedTable;
    qmsFrom           * sFrom;
    qcDepInfo           sFromDepInfo;
    qtcNode           * sLeftNode;
    qtcNode           * sRightNode;
    idBool              sIsContain;
    idBool              sIsUnique;

    sPreservedTable = aSFWGH->preservedInfo;
    
    if ( sPreservedTable != NULL )
    {
        IDE_TEST_CONT( sPreservedTable->useKeyPreservedTable != ID_TRUE,
                       SKIP_CONT );

        if ( sPreservedTable->stopFlag == ID_FALSE )
        {
            if ( ( ( aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK )
                   == MTC_NODE_OPERATOR_EQUAL ) &&
                 ( ( aNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                   == QTC_NODE_JOIN_OPERATOR_ABSENT ) )
            {
                /* outer join으로 이미 연결관계가 끝난 predicate은 제외한다.
                 * unique key: t1.i1, t2.i1, t3.i1
                 * CASE 1. select t1.i1 t1i1, t1.i2 t1i2, t2.i1 t2i1, t2.i2 t2i2 
                 *         from t1 left outer join t2 on t1.i1=t2.i1 where t1.i1=t2.i2;
                 * CASE 2. select t1.i1 t1i1, t1.i2 t1i2, t2.i1 t2i1, t2.i2 t2i2 
                 *         from t1 left outer join t2 on t1.i1=t2.i2 where t1.i1=t2.i1;
                 * CASE 3. select t1.i1 t1i1, t1.i2 t1i2, t2.i1 t2i1, t2.i2 t2i2, t3.i1 t3i1, t3.i2 t3i2 
                 *         from t1, t2 left outer join t3 on t2.i1=t3.i1 */
                sIsContain = ID_FALSE;
                
                for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
                {
                    qtc::dependencyClear( & sFromDepInfo );
                    
                    IDE_TEST( fromTreeDepInfo( aStatement,
                                               sFrom,
                                               & sFromDepInfo )
                              != IDE_SUCCESS );
                    
                    if ( qtc::dependencyContains( & sFromDepInfo,
                                                  & aNode->depInfo )
                         == ID_TRUE )
                    {
                        sIsContain = ID_TRUE;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                // BUGBUG
                // inner join으로 연결된 predicate도 제외된다.
                if ( sIsContain == ID_FALSE )
                {
                    sLeftNode  = (qtcNode*) aNode->node.arguments;
                    sRightNode = (qtcNode*) aNode->node.arguments->next;

                    if ( ( QTC_IS_COLUMN( aStatement, sLeftNode ) == ID_TRUE ) &&
                         ( QTC_IS_COLUMN( aStatement, sRightNode ) == ID_TRUE ) )
                    {
                        // t1.i1(u) = t2.i1 --> (t1<-t2)
                        IDE_TEST( isUnique( aStatement,
                                            sPreservedTable,
                                            sLeftNode,
                                            sRightNode,
                                            & sIsUnique )
                                  != IDE_SUCCESS );
                
                        if ( sIsUnique == ID_TRUE )
                        {
                            IDE_TEST( mark( sPreservedTable,
                                            sRightNode->node.baseTable,
                                            sLeftNode->node.baseTable )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }
    
                        // t1.i1 = t2.i1(u) --> (t1->t2)
                        IDE_TEST( isUnique( aStatement,
                                            sPreservedTable,
                                            sRightNode,
                                            sLeftNode,
                                            & sIsUnique )
                                  != IDE_SUCCESS );
    
                        if ( sIsUnique == ID_TRUE )
                        {
                            IDE_TEST( mark( sPreservedTable,
                                            sLeftNode->node.baseTable,
                                            sRightNode->node.baseTable )
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
            }
            else
            {
                // Nothing to do.
            }

            if ( ( (aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK)
                   == MTC_NODE_OPERATOR_OR ) ||
                 ( ( aNode->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                   == QTC_NODE_JOIN_OPERATOR_EXIST ) )
            {
                // or 노드가 있는 경우 모두 key preserved table이 아님
                // join operator 노드가 있는 경우 모두 key preserved table이 아님
                IDE_TEST( unmarkAll( sPreservedTable )
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
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( SKIP_CONT );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsPreservedTable::fromTreeDepInfo( qcStatement  * aStatement,
                                    qmsFrom      * aFrom,
                                    qcDepInfo    * aDependencies )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *     from 하위 tree의 depInfo를 모두 or한다.
 *
 ***********************************************************************/

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        IDE_TEST( qtc::dependencyOr( & aFrom->depInfo,
                                     aDependencies,
                                     aDependencies )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( fromTreeDepInfo( aStatement,
                                   aFrom->left,
                                   aDependencies )
                  != IDE_SUCCESS );
        
        IDE_TEST( fromTreeDepInfo( aStatement,
                                   aFrom->right,
                                   aDependencies )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsPreservedTable::addOnCondPredicate( qcStatement  * aStatement,
                                       qmsSFWGH     * aSFWGH,
                                       qmsFrom      * aFrom,
                                       qtcNode      * aNode )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPreservedInfo  * sPreservedTable;
    qtcNode           * sLeftNode;
    qtcNode           * sRightNode;
    qcDepInfo           sLeftDependencies;
    qcDepInfo           sRightDependencies;
    idBool              sIsUnique;

    sPreservedTable = aSFWGH->preservedInfo;
    
    if ( sPreservedTable != NULL )
    {
        IDE_TEST_CONT( sPreservedTable->useKeyPreservedTable != ID_TRUE,
                       SKIP_CONT );
            
        if ( sPreservedTable->stopFlag == ID_FALSE )
        {
            if ( (aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK)
                 == MTC_NODE_OPERATOR_EQUAL )
            {
                sLeftNode  = (qtcNode*) aNode->node.arguments;
                sRightNode = (qtcNode*) aNode->node.arguments->next;

                if ( ( QTC_IS_COLUMN( aStatement, sLeftNode ) == ID_TRUE ) &&
                     ( QTC_IS_COLUMN( aStatement, sRightNode ) == ID_TRUE ) )
                {
                    switch ( aFrom->joinType )
                    {
                        case QMS_INNER_JOIN:
                        {
                            IDE_TEST( addPredicate( aStatement,
                                                    aSFWGH,
                                                    aNode )
                                      != IDE_SUCCESS );
                            break;
                        }
        
                        case QMS_LEFT_OUTER_JOIN:
                        {
                            // t1 left outer join t2 on t1.i1 = t2.i1(u) --> add (t2<-t1)
                            qtc::dependencyClear( & sLeftDependencies );
                            qtc::dependencyClear( & sRightDependencies );

                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->left,
                                                       & sLeftDependencies )
                                      != IDE_SUCCESS );
                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->right,
                                                       & sRightDependencies )
                                      != IDE_SUCCESS );
                            
                            qtc::dependencyAnd( & sLeftDependencies,
                                                & sLeftNode->depInfo,
                                                & sLeftDependencies );
                            qtc::dependencyAnd( & sRightDependencies,
                                                & sRightNode->depInfo,
                                                & sRightDependencies );
            
                            if ( ( qtc::dependencyEqual( & sLeftDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) &&
                                 ( qtc::dependencyEqual( & sRightDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) )
                            {
                                IDE_TEST( isUnique( aStatement,
                                                    sPreservedTable,
                                                    sRightNode,
                                                    sLeftNode,
                                                    & sIsUnique )
                                          != IDE_SUCCESS );
                            
                                if ( sIsUnique == ID_TRUE )
                                {
                                    IDE_TEST( mark( sPreservedTable,
                                                    sLeftNode->node.baseTable,
                                                    sRightNode->node.baseTable )
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

                            // t1 left outer join t2 on t2.i1(u) = t1.i1 --> add (t2<-t1)
                            qtc::dependencyClear( & sLeftDependencies );
                            qtc::dependencyClear( & sRightDependencies );

                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->right,
                                                       & sLeftDependencies )
                                      != IDE_SUCCESS );
                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->left,
                                                       & sRightDependencies )
                                      != IDE_SUCCESS );
                            
                            qtc::dependencyAnd( & sLeftDependencies,
                                                & sLeftNode->depInfo,
                                                & sLeftDependencies );
                            qtc::dependencyAnd( & sRightDependencies,
                                                & sRightNode->depInfo,
                                                & sRightDependencies );
            
                            if ( ( qtc::dependencyEqual( & sLeftDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) &&
                                 ( qtc::dependencyEqual( & sRightDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) )
                            {
                                IDE_TEST( isUnique( aStatement,
                                                    sPreservedTable,
                                                    sLeftNode,
                                                    sRightNode,
                                                    & sIsUnique )
                                          != IDE_SUCCESS );
                            
                                if ( sIsUnique == ID_TRUE )
                                {
                                    IDE_TEST( mark( sPreservedTable,
                                                    sRightNode->node.baseTable,
                                                    sLeftNode->node.baseTable )
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
            
                            break;
                        }
            
                        case QMS_RIGHT_OUTER_JOIN:
                        {
                            // t1 right outer join t2 on t1.i1(u) = t2.i1 --> add (t1<-t2)
                            qtc::dependencyClear( & sLeftDependencies );
                            qtc::dependencyClear( & sRightDependencies );

                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->left,
                                                       & sLeftDependencies )
                                      != IDE_SUCCESS );
                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->right,
                                                       & sRightDependencies )
                                      != IDE_SUCCESS );
                            
                            qtc::dependencyAnd( & sLeftDependencies,
                                                & sLeftNode->depInfo,
                                                & sLeftDependencies );
                            qtc::dependencyAnd( & sRightDependencies,
                                                & sRightNode->depInfo,
                                                & sRightDependencies );
            
                            if ( ( qtc::dependencyEqual( & sLeftDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) &&
                                 ( qtc::dependencyEqual( & sRightDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) )
                            {
                                IDE_TEST( isUnique( aStatement,
                                                    sPreservedTable,
                                                    sLeftNode,
                                                    sRightNode,
                                                    & sIsUnique )
                                          != IDE_SUCCESS );

                                if ( sIsUnique == ID_TRUE )
                                {
                                    IDE_TEST( mark( sPreservedTable,
                                                    sRightNode->node.baseTable,
                                                    sLeftNode->node.baseTable )
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
            
                            // t1 right outer join t2 on t2.i1 = t1.i1(u) --> add (t1<-t2)
                            qtc::dependencyClear( & sLeftDependencies );
                            qtc::dependencyClear( & sRightDependencies );
            
                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->right,
                                                       & sLeftDependencies )
                                      != IDE_SUCCESS );
                            IDE_TEST( fromTreeDepInfo( aStatement,
                                                       aFrom->left,
                                                       & sRightDependencies )
                                      != IDE_SUCCESS );
                            
                            qtc::dependencyAnd( & sLeftDependencies,
                                                & sLeftNode->depInfo,
                                                & sLeftDependencies );
                            qtc::dependencyAnd( & sRightDependencies,
                                                & sRightNode->depInfo,
                                                & sRightDependencies );
            
                            if ( ( qtc::dependencyEqual( & sLeftDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) &&
                                 ( qtc::dependencyEqual( & sRightDependencies,
                                                         & qtc::zeroDependencies ) != ID_TRUE ) )
                            {
                                IDE_TEST( isUnique( aStatement,
                                                    sPreservedTable,
                                                    sRightNode,
                                                    sLeftNode,
                                                    & sIsUnique )
                                          != IDE_SUCCESS );

                                if ( sIsUnique == ID_TRUE )
                                {
                                    IDE_TEST( mark( sPreservedTable,
                                                    sLeftNode->node.baseTable,
                                                    sRightNode->node.baseTable )
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
                            break;
                        }
        
                        default:
                            break;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else if ( (aNode->node.module->lflag & MTC_NODE_OPERATOR_MASK)
                      == MTC_NODE_OPERATOR_OR )
            {
                // or 노드가 있는 경우 모두 key preserved table이 아님
                IDE_TEST( unmarkAll( sPreservedTable )
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

    IDE_EXCEPTION_CONT( SKIP_CONT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsPreservedTable::transitivity( qmsPreservedInfo  * aPreservedTable )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *     Floyd Warshall algorithm
 *
 ***********************************************************************/

    UInt   sT;
    UInt   k;
    UInt   i;
    UInt   j;

    for ( k = 0; k < aPreservedTable->tableCount; k++ )
    {
        for ( i = 0; i < aPreservedTable->tableCount; i++ )
        {
            for ( j = 0; j < aPreservedTable->tableCount; j++ )
            {
                sT = (UInt)aPreservedTable->tableMap[i][k] +
                    (UInt)aPreservedTable->tableMap[k][j];
                
                if ( sT < (UInt)aPreservedTable->tableMap[i][j] )
                {
                    aPreservedTable->tableMap[i][j] = (UShort)sT;
                }
            }
        }
    }
    
    return IDE_SUCCESS;
}

IDE_RC qmsPreservedTable::checkPreservation( qmsPreservedInfo  * aPreservedTable )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation : preservation 검사
 *
 *   모든 row에 mark가 표시되어 있다면 key preserved table이다.
 *
 *    +---+---+---+---+
 *    | \ | m |   |   |  x
 *    +---+---+---+---+
 *    | m | \ |   | m |  X
 *    +---+---+---+---+
 *    | m | m | \ | m |  0
 *    +---+---+---+---+
 *    |   |   |   | \ |  x
 *    +---+---+---+---+
 *
 ***********************************************************************/
    
    UShort  sRow[QC_MAX_REF_TABLE_CNT] = { 0, };
    UInt    i;
    UInt    j;

    for ( i = 0; i < aPreservedTable->tableCount; i++ )
    {
        for ( j = 0; j < aPreservedTable->tableCount; j++ )
        {
            if ( ( j != i ) && ( aPreservedTable->tableMap[j][i] < ID_USHORT_MAX ) )
            {
                sRow[j]++;
            }
            else
            {
                // Nothing to do
            }
        }
    }

    for ( i = 0; i < aPreservedTable->tableCount; i++ )
    {
        if ( sRow[i] == aPreservedTable->tableCount - 1 )
        {
            aPreservedTable->result[i] = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;
}

IDE_RC qmsPreservedTable::find( qcStatement  * aStatement,
                                qmsSFWGH     * aSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *               BUG-39399 remove search key preserved table
 * Implementation :
 *      1. key preserved table HINT check.
 *      3. key preserved column set flag.
 *
 ***********************************************************************/

    mtcTemplate          * sMtcTemplate;
    qmsPreservedInfo     * sPreservedTable;
    qmsTableRef          * sTableRef;
    UInt                   i;

    /* BUG-461254 */
    qmsParseTree         * sParseTree;

    IDU_FIT_POINT_FATAL( "qmsPreservedTable::find::__FT__" );

    sPreservedTable = aSFWGH->preservedInfo;
    sParseTree = (qmsParseTree *)aStatement->myPlan->parseTree; /* BUG-46124 */

    if ( sPreservedTable != NULL )
    {
        sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate; /* BUG-46124 */

         // simple view에 order by만 허용한다.
         if( ( aSFWGH->hierarchy == NULL ) &&
             ( aSFWGH->group == NULL ) &&
             ( aSFWGH->having == NULL ) &&
             ( aSFWGH->aggsDepth1 == NULL ) &&
             ( aSFWGH->aggsDepth2 == NULL ) &&
             ( aSFWGH->rownum == NULL ) &&
             ( sParseTree->withClause == NULL ) )
         {
             if ( sPreservedTable->useKeyPreservedTable == ID_TRUE )
             {
                 IDE_TEST( transitivity( sPreservedTable )
                           != IDE_SUCCESS );

                 IDE_TEST( checkPreservation( sPreservedTable )
                           != IDE_SUCCESS );
             }
             else
             {
                 IDE_TEST ( checkKeyPreservedTableHints( aSFWGH )
                            != IDE_SUCCESS );
             }
        
             // tuple에 반영한다.
             for ( i = 0; i < sPreservedTable->tableCount; i++ )
             {
                 if ( sPreservedTable->useKeyPreservedTable == ID_TRUE )
                 {
                     if ( sPreservedTable->result[i] == ID_TRUE )
                     {
                         sTableRef = sPreservedTable->tableRef[i];
            
                         sMtcTemplate->rows[sTableRef->table].lflag &=
                             ~MTC_TUPLE_KEY_PRESERVED_MASK;
                         sMtcTemplate->rows[sTableRef->table].lflag |=
                             MTC_TUPLE_KEY_PRESERVED_TRUE;
                     }
                     else
                     {
                         // Nothing to do.
                     }
                 }
                 else
                 {
                     if ( sPreservedTable->isKeyPreserved[i] == ID_TRUE )
                     {
                         sTableRef = sPreservedTable->tableRef[i];
            
                         sMtcTemplate->rows[sTableRef->table].lflag &=
                             ~MTC_TUPLE_KEY_PRESERVED_MASK;
                         sMtcTemplate->rows[sTableRef->table].lflag |=
                             MTC_TUPLE_KEY_PRESERVED_TRUE;
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
             /* in unsupported case, claer all lower view setting */
             if ( sPreservedTable->useKeyPreservedTable == ID_TRUE )
             {
                 /* Nothing to do */
             }
             else
             {
                 for ( i = 0; i < sPreservedTable->tableCount; i++ )
                 {
                     sTableRef = sPreservedTable->tableRef[i];

                     sMtcTemplate->rows[sTableRef->table].lflag &=
                         ~MTC_TUPLE_KEY_PRESERVED_MASK;
                     sMtcTemplate->rows[sTableRef->table].lflag |=
                         MTC_TUPLE_KEY_PRESERVED_FALSE;
                 }

                 sPreservedTable->mIsInValid = ID_TRUE;
             }
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

IDE_RC qmsPreservedTable::getFirstKeyPrevTable( qmsSFWGH      * aSFWGH,
                                                qmsTableRef  ** aTableRef )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *
 * Implementation :
 *   ex>
 * CASE 1. delete from (select t1.i1 a, t2.i1 b from t1, t2 where t1.i1 = t2.i1 ) ....  
 * CASE 2. delete from (select t1.i1 a, t2.i1 b from t2, t1 where t1.i1 = t2.i1 ) ....
 * t1,t2 key preserved table이다. delete 시 첫번째 key preserved table만 delete된다.
 * CASE 1는 T1, CASE 2는 T2.
 *
 ***********************************************************************/

    qmsPreservedInfo  * sPreservedTable;
    UInt                i;

    sPreservedTable = aSFWGH->preservedInfo;

    *aTableRef = NULL;
    
    if ( sPreservedTable != NULL )
    {
        for ( i = 0; i < sPreservedTable->tableCount; i++ )
        {
            if ( sPreservedTable->useKeyPreservedTable == ID_TRUE )
            {                
                if ( sPreservedTable->result[i] == ID_TRUE )
                {
                    *aTableRef = sPreservedTable->tableRef[i];
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                if ( sPreservedTable->isKeyPreserved[i] == ID_TRUE )
                {
                    *aTableRef = sPreservedTable->tableRef[i];
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
    
    return IDE_SUCCESS;
}

IDE_RC qmsPreservedTable::checkKeyPreservedTableHints( qmsSFWGH * aSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-2204 Join Update, Delete
 *               BUG-39399 remove search key preserved table
 * Implementation :
 *       key preserved table hint check.
 * hint : key_preserved_table(table1,table2,....n)
 *        *+ key_preserved_table(t1,t2,t3)
 *        *+ key_preserved_table(t1,t3) key_preserved_table(t2) 
 ***********************************************************************/
    
    qmsPreservedInfo     * sPreservedTable;
    qmsTableRef          * sTableRef;
    qmsKeyPreservedHints * sKeyPreservedHint;
    qmsHintTables        * sHintTable;
    UInt                   i;
    UInt                   j = 0;

    IDU_FIT_POINT_FATAL( "qmsPreservedTable::checkKeyPreservedTableHints::__FT__" );

    sPreservedTable = aSFWGH->preservedInfo;

    if ( sPreservedTable != NULL )
    {
        for ( sKeyPreservedHint = aSFWGH->hints->keyPreservedHint;
              sKeyPreservedHint != NULL;
              sKeyPreservedHint = sKeyPreservedHint->next )
        {                            
            for ( sHintTable = sKeyPreservedHint->table;
                  sHintTable != NULL;
                  sHintTable = sHintTable->next )
            {
                for ( i = 0; i < sPreservedTable->tableCount; i++ )
                {
                    sTableRef = sPreservedTable->tableRef[i];
                        
                    if ( idlOS::strMatch( sHintTable->tableName.stmtText +
                                          sHintTable->tableName.offset,
                                          sHintTable->tableName.size,
                                          sTableRef->aliasName.stmtText +
                                          sTableRef->aliasName.offset,
                                          sTableRef->aliasName.size ) == 0 )
                    {
                        // HINT예 명시한 테이블이 존재하는 경우
                        sPreservedTable->isKeyPreserved[i] = ID_TRUE;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
        }

        /* in no change case */
        for ( i = 0, j =0;
              i < sPreservedTable->tableCount;
              i++ )
        {
            if ( sPreservedTable->isKeyPreserved[i] == ID_FALSE )
            {
                j++;
            }
            else
            {
                /* Nothing To Do */
            }
        }

        /* all change */
        if  ( ( j == sPreservedTable->tableCount ) &&
              ( sPreservedTable->mIsInValid == ID_FALSE ) )
        {
            for ( i = 0;
                  i < sPreservedTable->tableCount;
                  i++ )
            {
                sPreservedTable->isKeyPreserved[i] = ID_TRUE;
            }
        }
        else
        {
            /* Nothing To Do */
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC qmsPreservedTable::checkAndSetPreservedInfo( qmsSFWGH    * aSFWGH,
                                                    qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 * Description : BUG-46124
 *
 * Implementation :
 *  중첩 구조인 View의 하위 View 까지 Key Preserved 알고리즘을 수행할 수 있도록,
 *  상위 View의 PreservedInfo를 하위 View에 연결시킵니다.
 *
 *  이때에 프로퍼티 __KEY_PRESERVED_TABLE = 0 인 경우만 연결합니다.
 *
 ***********************************************************************/

    qmsParseTree * sViewParseTree = NULL;
    qmsSFWGH     * sViewSFWGH     = NULL;

    if ( aSFWGH != NULL )
    {
        if ( ( aSFWGH->preservedInfo != NULL ) &&
             ( aTableRef->view != NULL ) )
        {
            sViewParseTree = (qmsParseTree *)aTableRef->view->myPlan->parseTree;
            sViewSFWGH     = sViewParseTree->querySet->SFWGH;

            if ( sViewSFWGH != NULL )
            {
                if ( sViewSFWGH->preservedInfo == NULL )
                {
                     if ( aSFWGH->preservedInfo->useKeyPreservedTable == ID_TRUE )
                     {
                         /* Nothing to do */
                     }
                     else
                     {
                         sViewSFWGH->preservedInfo = aSFWGH->preservedInfo;
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
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    return IDE_FAILURE;
}

IDE_RC qmsPreservedTable::searchQmsTargetForPreservedTable( qmsParseTree * aParseTree,
                                                            qmsSFWGH     * aSFWGH,
                                                            qmsFrom      * aFrom,
                                                            qmsTarget    * aSearch,
                                                            qmsTarget   ** aTarget )
{
/***********************************************************************
 *
 * Description : BUG-46124
 *
 * Implementation :
 *   이 함수는 DML 작업 시에 From의 View가 존재하면, 실제 DML을 작업해야할
 *   Column과 Tuple 정보를 찾을 수 있는 Taget 정보를 검색합니다.
 *
 *   또한, View인 경우, DML을 할 수 있는 MTR를 사용하도록 힌트를 내려줍니다.
 *
 *   이때, 최상위 View의 From에 View를 포함한 Join이 있다면, DML를 허용하지 않습니다.
 *
 *   DML 가능 View
 *    - CREATE VIEW V1 AS SELECT I1, I2 FROM T1;
 *    - CREATE VIEW V2 AS SELECT I1, I2 FROM V1;
 *    - CREATE VIEW V3 AS SELECT I1, I2 FROM V2;
 *
 *   DML 불가능 View
 *    - CREATE VIEW V4 AS SELECT V1.I1, T1.I2 FROM V1, T1 ...;
 *    - CREATE VIEW V5 AS SELECT T1.I1, V2.I2 FROM T1, V2 ...;
 *    - CREATE VIEW V6 AS SELECT V3.I1, V4.I2 FROM V3, V4 ...;
 *    - CREATE VIEW V7 AS SELECT T1.I1, V5.I2 FROM T1 LEFT OUTER V5 ...;
 *    - CREATE VIEW V8 AS SELECT V6.I1, V1.I2 FROM V6 RIGHT OUTER V1 ...;
 *    - CREATE VIEW V9 AS SELECT V7.I1, V8.I2 FROM V7 INNER V8 ...;
 *
 ***********************************************************************/

    qmsParseTree * sViewParseTree = NULL;
    qmsTarget    * sCurrTarget    = NULL;
    qmsTarget    * sViewTarget    = NULL;
    qmsTarget    * sReturnTarget  = NULL;
    qmsFrom      * sFrom          = NULL;
    qmsFrom      * sNextFrom      = NULL;
    qmsFrom      * sViewFrom      = NULL;
    qmsSFWGH     * sViewSFWGH     = NULL;

    sCurrTarget   = aSearch;
    sReturnTarget = aSearch;

    IDE_TEST_CONT( aSFWGH->preservedInfo == NULL, SKIP );

    for ( sFrom  = aFrom;
          sFrom != NULL;
          sFrom  = sFrom->next )
    {
        if ( sFrom->joinType != QMS_NO_JOIN )
        {
            if ( ( aSFWGH->lflag & QMV_SFWGH_UPDATABLE_VIEW_MASK )
                 == QMV_SFWGH_UPDATABLE_VIEW_TRUE )
            {
                if ( sFrom->left->tableRef != NULL )
                {
                    IDE_TEST_RAISE( sFrom->left->tableRef->view != NULL, ERR_NOT_KEY_PRESERVED_TABLE );
                }

                if ( sFrom->right->tableRef != NULL )
                {
                    IDE_TEST_RAISE( sFrom->right->tableRef->view != NULL, ERR_NOT_KEY_PRESERVED_TABLE );
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            sNextFrom = sFrom->next;

            if ( sNextFrom != NULL )
            {
                if ( ( aSFWGH->lflag & QMV_SFWGH_UPDATABLE_VIEW_MASK )
                       == QMV_SFWGH_UPDATABLE_VIEW_TRUE )
                {
                    if ( sNextFrom->tableRef != NULL )
                    {
                        IDE_TEST_RAISE( sNextFrom->tableRef->view != NULL, ERR_NOT_KEY_PRESERVED_TABLE );
                    }

                    if ( sFrom->tableRef != NULL )
                    {
                        IDE_TEST_RAISE( sFrom->tableRef->view != NULL, ERR_NOT_KEY_PRESERVED_TABLE );
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

            if ( sFrom->tableRef->view != NULL )
            {
                sViewParseTree = (qmsParseTree *)sFrom->tableRef->view->myPlan->parseTree;
                sViewSFWGH     = sViewParseTree->querySet->SFWGH;
                sViewFrom      = sViewSFWGH->from;

                sViewParseTree->querySet->materializeType
                            = aParseTree->querySet->materializeType;
                sViewParseTree->querySet->SFWGH->hints->materializeType
                            = aParseTree->querySet->SFWGH->hints->materializeType;

                /* search at lower view */
                for ( sViewTarget = sViewParseTree->querySet->target;
                      ( sViewTarget != NULL ) && ( sCurrTarget != NULL );
                      sViewTarget = sViewTarget->next )
                {
                    if ( sViewTarget->aliasColumnName.size != QC_POS_EMPTY_SIZE )
                    {
                        if ( idlOS::strMatch( sCurrTarget->columnName.name,
                                              sCurrTarget->columnName.size,
                                              sViewTarget->aliasColumnName.name,
                                              sViewTarget->aliasColumnName.size ) == 0 )
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

                /* delete query */
                if ( sCurrTarget == NULL )
                {
                    IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                   sViewSFWGH,
                                                                                   sViewFrom,
                                                                                   sViewTarget,
                                                                                   & sReturnTarget )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* found target */
                    if ( sViewTarget != NULL )
                    {
                        IDE_TEST( qmsPreservedTable::searchQmsTargetForPreservedTable( sViewParseTree,
                                                                                       sViewSFWGH,
                                                                                       sViewFrom,
                                                                                       sViewTarget,
                                                                                       & sReturnTarget )
                                  != IDE_SUCCESS );
                        break;
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
    }

    IDE_EXCEPTION_CONT( SKIP );

    *aTarget = sReturnTarget;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_KEY_PRESERVED_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_KEY_PRESERVED_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

