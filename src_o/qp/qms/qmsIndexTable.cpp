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
 

#include <qmsIndexTable.h>
#include <qcm.h>

extern "C" int
comparePartRefIndex( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *    compare qcmColumn
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_DASSERT( aElem1 != NULL );
    IDE_DASSERT( aElem2 != NULL );

    //--------------------------------
    // compare partition OID
    //--------------------------------

    if( ((qmsPartRefIndex*)aElem1)->partOID >
        ((qmsPartRefIndex*)aElem2)->partOID )
    {
        return 1;       
    }
    else if( ((qmsPartRefIndex*)aElem1)->partOID <
             ((qmsPartRefIndex*)aElem2)->partOID )
    {
        return -1;        
    }
    else
    {
        return 0;
    }
}

IDE_RC
qmsIndexTable::makeIndexTableRef( qcStatement       * aStatement,
                                  qcmTableInfo      * aTableInfo,
                                  qmsIndexTableRef ** aIndexTableRef,
                                  UInt              * aIndexTableCount )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmIndex         * sIndex;
    qmsIndexTableRef * sIndexTableRef   = NULL;
    UInt               sIndexTableCount = 0;
    UInt               i;
    UInt               j;

    IDU_FIT_POINT_FATAL( "qmsIndexTable::makeIndexTableRef::__FT__" );

    for ( i = 0; i < aTableInfo->indexCount; i++ )
    {
        sIndex = & aTableInfo->indices[i];

        if ( sIndex->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
        {
            sIndexTableCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sIndexTableCount > 0 )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM(aStatement),
                                              qmsIndexTableRef,
                                              sIndexTableCount,
                                              & sIndexTableRef )
                     != IDE_SUCCESS );

        // index table ref를 구성한다.
        for ( i = 0, j = 0; i < aTableInfo->indexCount; i++ )
        {
            sIndex = & aTableInfo->indices[i];

            if ( sIndex->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
            {
                IDE_TEST( makeOneIndexTableRef( aStatement,
                                                sIndex,
                                                & sIndexTableRef[j] )
                          != IDE_SUCCESS );
                j++;
            }
            else
            {
                // Nothing to do.
            }
        }
        
        // list를 구성한다.
        for ( i = 0; i < sIndexTableCount - 1; i++ )
        {
            sIndexTableRef[i].next = & sIndexTableRef[i + 1];
        }
        sIndexTableRef[i].next = NULL;
    }
    else
    {
        // Nothing to do.
    }
    
    *aIndexTableRef   = sIndexTableRef;
    *aIndexTableCount = sIndexTableCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::makeOneIndexTableRef( qcStatement      * aStatement,
                                     qcmIndex         * aIndex,
                                     qmsIndexTableRef * aIndexTableRef )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *     table id를 이용하여 indexTableRef 구성.
 *    
 * Implementation :
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmsIndexTable::makeOneIndexTableRef::__FT__" );

    aIndexTableRef->index   = aIndex;
    aIndexTableRef->tableID = aIndex->indexTableID;

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     aIndexTableRef->tableID,
                                     & aIndexTableRef->tableInfo,
                                     & aIndexTableRef->tableSCN,
                                     & aIndexTableRef->tableHandle )
              != IDE_SUCCESS );
    
    aIndexTableRef->table    = 0;
    aIndexTableRef->statInfo = NULL;
    aIndexTableRef->next     = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmsIndexTable::validateAndLockIndexTableRefList( qcStatement      * aStatement,
                                                        qmsIndexTableRef * aIndexTableRef,
                                                        smiTableLockMode   aLockMode )
{
/***********************************************************************
 *
 * Description : PROJ-1624 global non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexTableRef * sIndexTableRef = NULL;
    ULong              sTimeout       = ID_ULONG_MAX;

    if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK ) == QCI_STMT_MASK_DDL )
    {
        sTimeout = smiGetDDLLockTimeOut((QC_SMI_STMT( aStatement ))->getTrans());
    }
    else
    {
        /* Nothing to do */
    }

    for ( sIndexTableRef = aIndexTableRef;
          sIndexTableRef != NULL;
          sIndexTableRef = sIndexTableRef->next )
    {
        IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                            sIndexTableRef->tableHandle,
                                            sIndexTableRef->tableSCN,
                                            SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                            aLockMode,
                                            sTimeout,
                                            ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                 != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmsIndexTable::validateAndLockOneIndexTableRef( qcStatement      * aStatement,
                                                       qmsIndexTableRef * aIndexTableRef,
                                                       smiTableLockMode   aLockMode )
{
/***********************************************************************
 *
 * Description : PROJ-1624 global non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    ULong              sTimeout       = ID_ULONG_MAX;

    if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK ) == QCI_STMT_MASK_DDL )
    {
        sTimeout = smiGetDDLLockTimeOut((QC_SMI_STMT( aStatement ))->getTrans());
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                         aIndexTableRef->tableHandle,
                                         aIndexTableRef->tableSCN,
                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                         aLockMode,
                                         sTimeout,
                                         ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::findKeyIndex( qcmTableInfo  * aTableInfo,
                             qcmIndex     ** aKeyIndex )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmIndex * sIndex;
    idBool     sFound = ID_FALSE;
    UInt       i;

    for ( i = 0; i < aTableInfo->indexCount; i++ )
    {
        sIndex = &(aTableInfo->indices[i]);

        if ( ( idlOS::strlen( sIndex->name ) >= QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE ) &&
             ( idlOS::strMatch( sIndex->name,
                                QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE,
                                QD_INDEX_TABLE_KEY_INDEX_PREFIX,
                                QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE ) == 0 ) )
        {
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    // index table의 index는 반드시 존재한다.
    IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NOT_FOUND );

    *aKeyIndex = sIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::findKeyIndex",
                                  "key index is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::findRidIndex( qcmTableInfo  * aTableInfo,
                             qcmIndex     ** aRidIndex )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmIndex * sIndex;
    idBool     sFound = ID_FALSE;
    UInt       i;

    for ( i = 0; i < aTableInfo->indexCount; i++ )
    {
        sIndex = &(aTableInfo->indices[i]);

        if ( ( idlOS::strlen( sIndex->name ) >= QD_INDEX_TABLE_RID_INDEX_PREFIX_SIZE ) &&
             ( idlOS::strMatch( sIndex->name,
                                QD_INDEX_TABLE_RID_INDEX_PREFIX_SIZE,
                                QD_INDEX_TABLE_RID_INDEX_PREFIX,
                                QD_INDEX_TABLE_RID_INDEX_PREFIX_SIZE ) == 0 ) )
        {
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    // index table의 index는 반드시 존재한다.
    IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NOT_FOUND );

    *aRidIndex = sIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::findRidIndex",
                                  "rid index is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::findIndexTableRefInList( qmsIndexTableRef  * aIndexTableRef,
                                        UInt                aIndexID,
                                        qmsIndexTableRef ** aFoundIndexTableRef )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *     index table list에서 indexID에 해당하는 index table을 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexTableRef  * sIndexTableRef;
    idBool              sFound = ID_FALSE;

    for ( sIndexTableRef = aIndexTableRef;
          sIndexTableRef != NULL;
          sIndexTableRef = sIndexTableRef->next )
    {
        if ( sIndexTableRef->index->indexId == aIndexID )
        {
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }
    
    IDE_TEST_RAISE( sFound == ID_FALSE, ERR_NOT_FOUND );

    *aFoundIndexTableRef = sIndexTableRef;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::findIndexTableRefInList",
                                  "index table is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::initializePartitionRefIndex( qcStatement         * aStatement,
                                            qmsPartitionRef     * aPartitionRef,
                                            UInt                  aPartitionCount,
                                            qmsPartRefIndexInfo * aPartIndexInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *     aPartIndexInfo를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPartitionRef     * sPartitionRef;
    UInt                  i;

    IDE_TEST( STRUCT_ALLOC_WITH_COUNT( aStatement->qmxMem,
                                       qmsPartRefIndex,
                                       aPartitionCount,
                                       & aPartIndexInfo->partRefIndex )
              != IDE_SUCCESS );
    aPartIndexInfo->partCount = aPartitionCount;

    for ( i = 0, sPartitionRef = aPartitionRef;
          sPartitionRef != NULL;
          i++, sPartitionRef = sPartitionRef->next )
    {
        aPartIndexInfo->partRefIndex[i].partOID      = sPartitionRef->partitionOID;
        aPartIndexInfo->partRefIndex[i].partitionRef = sPartitionRef;
        aPartIndexInfo->partRefIndex[i].partOrder    = i;
    }

    if ( aPartIndexInfo->partCount > 1 )
    {
        idlOS::qsort( aPartIndexInfo->partRefIndex,
                      aPartIndexInfo->partCount,
                      ID_SIZEOF(qmsPartRefIndex),
                      comparePartRefIndex );
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
qmsIndexTable::findPartitionRefIndex( qmsPartRefIndexInfo  * aPartIndexInfo,
                                      smOID                  aPartOID,
                                      qmsPartitionRef     ** aFoundPartitionRef,
                                      UInt                 * aPartitionOrder )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *     partition ref list에서 partition oid에 해당하는 partition을 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPartRefIndex    sFindIndex;
    qmsPartRefIndex  * sFound;

    sFindIndex.partOID = aPartOID;
    
    sFound = (qmsPartRefIndex*) idlOS::bsearch( & sFindIndex,
                                                aPartIndexInfo->partRefIndex,
                                                aPartIndexInfo->partCount,
                                                ID_SIZEOF(qmsPartRefIndex),
                                                comparePartRefIndex );

    IDE_TEST_RAISE( sFound == NULL, ERR_NOT_FOUND );

    *aFoundPartitionRef = sFound->partitionRef;

    if ( aPartitionOrder != NULL )
    {
        *aPartitionOrder = sFound->partOrder;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::findPartitionRefIndex",
                                  "partition is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::makeUpdateSmiColumnList( UInt                aUpdateColumnCount,
                                        UInt              * aUpdateColumnID,
                                        qmsIndexTableRef  * aIndexTable,
                                        idBool              aKeyNRidUpdate,
                                        UInt              * aIndexUpdateColumnCount,
                                        smiColumnList     * aIndexUpdateColumnList )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmTableInfo  * sIndexTableInfo;
    mtcColumn     * sIndexUpdateColumn;
    UInt            sIndexUpdateColumnCount = 0;
    UInt            sIndexTableColumnCount;
    UInt            i;
    UInt            j;

    sIndexTableInfo = aIndexTable->tableInfo;
    
    // update할 컬럼을 찾는다.
    for ( i = 0; i < aIndexTable->index->keyColCount; i++ )
    {
        for ( j = 0; j < aUpdateColumnCount; j++ )
        {
            if ( aIndexTable->index->keyColumns[i].column.id == aUpdateColumnID[j] )
            {
                sIndexUpdateColumn = sIndexTableInfo->columns[i].basicInfo;

                // update column list 구성
                aIndexUpdateColumnList[sIndexUpdateColumnCount].column =
                    (smiColumn*) sIndexUpdateColumn;
                aIndexUpdateColumnList[sIndexUpdateColumnCount].next   =
                    &(aIndexUpdateColumnList[sIndexUpdateColumnCount + 1]);

                sIndexUpdateColumnCount++;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    if ( aKeyNRidUpdate == ID_TRUE )
    {
        sIndexTableColumnCount = sIndexTableInfo->columnCount;

        // oid column
        sIndexUpdateColumn = sIndexTableInfo->columns[sIndexTableColumnCount - 2].basicInfo;
        
        aIndexUpdateColumnList[sIndexUpdateColumnCount].column =
            (smiColumn*) sIndexUpdateColumn;
        aIndexUpdateColumnList[sIndexUpdateColumnCount].next   =
            &(aIndexUpdateColumnList[sIndexUpdateColumnCount + 1]);
        
        sIndexUpdateColumnCount++;

        // rid column
        sIndexUpdateColumn = sIndexTableInfo->columns[sIndexTableColumnCount - 1].basicInfo;
        
        aIndexUpdateColumnList[sIndexUpdateColumnCount].column =
            (smiColumn*) sIndexUpdateColumn;
        aIndexUpdateColumnList[sIndexUpdateColumnCount].next   =
            &(aIndexUpdateColumnList[sIndexUpdateColumnCount + 1]);
        
        sIndexUpdateColumnCount++;
    }
    else
    {
        // Nothing to do.
    }

    // 마지막을 null로 채운다.
    if ( sIndexUpdateColumnCount > 0 )
    {
        aIndexUpdateColumnList[sIndexUpdateColumnCount - 1].next = NULL;
    }
    else
    {
        // Nothing to do.
    }
    
    *aIndexUpdateColumnCount = sIndexUpdateColumnCount;

    return IDE_SUCCESS;
}

IDE_RC
qmsIndexTable::makeUpdateSmiValue( UInt                aUpdateColumnCount,
                                   UInt              * aUpdateColumnID,
                                   smiValue          * aUpdateValue,
                                   qmsIndexTableRef  * aIndexTable,
                                   idBool              aKeyNRidUpdate,
                                   smOID             * aPartOID,
                                   scGRID            * aRowGRID,
                                   UInt              * aIndexUpdateValueCount,
                                   smiValue          * aIndexUpdateValue )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt   sIndexUpdateColumnCount = 0;
    UInt   i;
    UInt   j;

    // update할 컬럼을 찾는다.
    for ( i = 0; i < aIndexTable->index->keyColCount; i++ )
    {
        for ( j = 0; j < aUpdateColumnCount; j++ )
        {
            if ( aIndexTable->index->keyColumns[i].column.id == aUpdateColumnID[j] )
            {
                // smi value 구성
                aIndexUpdateValue[sIndexUpdateColumnCount] = aUpdateValue[j];
                sIndexUpdateColumnCount++;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    if ( aKeyNRidUpdate == ID_TRUE )
    {
        // oid
        aIndexUpdateValue[sIndexUpdateColumnCount].value = (void*) aPartOID;
        aIndexUpdateValue[sIndexUpdateColumnCount].length = ID_SIZEOF(mtdBigintType);
        sIndexUpdateColumnCount++;
        
        // oid
        aIndexUpdateValue[sIndexUpdateColumnCount].value = (void*) aRowGRID;
        aIndexUpdateValue[sIndexUpdateColumnCount].length = ID_SIZEOF(mtdBigintType);
        sIndexUpdateColumnCount++;
    }
    else
    {
        // Nothing to do.
    }

    *aIndexUpdateValueCount = sIndexUpdateColumnCount;

    return IDE_SUCCESS;
}

IDE_RC
qmsIndexTable::makeInsertSmiValue( smiValue          * aInsertValue,
                                   qmsIndexTableRef  * aIndexTable,
                                   smOID             * aPartOID,
                                   scGRID            * aRowGRID,
                                   smiValue          * aIndexInsertValue )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  sColumnOrder;
    UInt  i;

    // insert할 컬럼을 찾는다.
    for ( i = 0; i < aIndexTable->index->keyColCount; i++ )
    {
        sColumnOrder =
            aIndexTable->index->keyColumns[i].column.id & SMI_COLUMN_ID_MASK;

        aIndexInsertValue[i] = aInsertValue[sColumnOrder];
    }

    // oid
    aIndexInsertValue[i].value = (void*) aPartOID;
    aIndexInsertValue[i].length = ID_SIZEOF(mtdBigintType);
    i++;

    // oid
    aIndexInsertValue[i].value = (void*) aRowGRID;
    aIndexInsertValue[i].length = ID_SIZEOF(mtdBigintType);

    return IDE_SUCCESS;
}

IDE_RC
qmsIndexTable::initializeIndexTableCursors( qcStatement          * aStatement,
                                            qmsIndexTableRef     * aIndexTableRef,
                                            UInt                   aIndexTableCount,
                                            qmsIndexTableRef     * aSelectedIndexTableRef,
                                            qmsIndexTableCursors * aCursorInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexTableRef  * sIndexTable;
    qmsIndexCursor    * sIndexCursor;
    qcmTableInfo      * sIndexTableInfo;
    qcmColumn         * sRIDColumn;
    qcmIndex          * sIndex      = NULL;
    UInt                sMaxRowSize = 0;
    UInt                sRowSize;
    UInt                sMaxColumnCount = 0;
    UInt                i;

    IDE_DASSERT( aIndexTableRef != NULL );
    IDE_DASSERT( aCursorInfo    != NULL );
    
    aCursorInfo->indexTableRef   = aIndexTableRef;
    aCursorInfo->indexTableCount = aIndexTableCount;

    aCursorInfo->indexCursors     = NULL;
    aCursorInfo->row              = NULL;

    if ( ( aIndexTableCount == 1 ) &&
         ( aSelectedIndexTableRef != NULL ) )
    {
        // index table이 하나이고 selected index table이면 아무것도 하지 않는다.
        IDE_DASSERT( aIndexTableRef == aSelectedIndexTableRef );

        // Nothing to do.
    }
    else
    {
        // non-partitioned index들
        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( aStatement->qmxMem,
                                           qmsIndexCursor,
                                           aIndexTableCount,
                                           & aCursorInfo->indexCursors )
                  != IDE_SUCCESS );
        
        for ( i = 0, sIndexTable = aIndexTableRef;
              sIndexTable != NULL;
              i++, sIndexTable = sIndexTable->next )
        {
            sIndexCursor = &(aCursorInfo->indexCursors[i]);
                
            if ( sIndexTable == aSelectedIndexTableRef )
            {
                // selection에 사용된 index table
                // 직접 삭제되므로 제외한다.
                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_SKIP;
            }
            else
            {
                sIndexTableInfo = sIndexTable->tableInfo;

                IDE_DASSERT( sIndexTableInfo->columnCount > 2 );
            
                sRIDColumn = &(sIndexTableInfo->columns[sIndexTableInfo->columnCount - 1]);
        
                // fetch column, only rid
                sIndexCursor->fetchColumn.column =
                    (smiColumn*)&(sRIDColumn->basicInfo->column);
                sIndexCursor->fetchColumn.columnSeq = SMI_GET_COLUMN_SEQ( sIndexCursor->fetchColumn.column );
                sIndexCursor->fetchColumn.copyDiskColumn = 
                    qdbCommon::getCopyDiskColumnFunc( sRIDColumn->basicInfo );

                sIndexCursor->fetchColumn.next = NULL;

                // rid index handle
                IDE_TEST( findRidIndex( sIndexTableInfo,
                                        & sIndex )
                          != IDE_SUCCESS );

                sIndexCursor->ridIndexHandle = sIndex->indexHandle;
                sIndexCursor->ridIndexType   = sIndex->indexTypeId;

                // range column
                sIndexCursor->ridColumn = sRIDColumn->basicInfo;

                // update column count
                if ( sIndexTableInfo->columnCount > sMaxColumnCount )
                {
                    sMaxColumnCount = sIndexTableInfo->columnCount;
                }
                else
                {
                    // Nothing to do.
                }

                // row size
                IDE_TEST( qdbCommon::getDiskRowSize( sIndexTableInfo,
                                                     & sRowSize )
                          != IDE_SUCCESS );

                if ( sRowSize > sMaxRowSize )
                {
                    sMaxRowSize = sRowSize;
                }
                else
                {
                    // Nothing to do.
                }

                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_ALLOC;
            }
        }
        
        // row
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          UChar,
                                          sMaxRowSize,
                                          & aCursorInfo->row )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::initializeIndexTableCursors4Insert( qcStatement          * aStatement,
                                                   qmsIndexTableRef     * aIndexTableRef,
                                                   UInt                   aIndexTableCount,
                                                   qmsIndexTableCursors * aCursorInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexTableRef  * sIndexTable;
    qmsIndexCursor    * sIndexCursor;
    UInt                i;

    IDE_DASSERT( aIndexTableRef != NULL );
    IDE_DASSERT( aCursorInfo    != NULL );
    
    aCursorInfo->indexTableRef   = aIndexTableRef;
    aCursorInfo->indexTableCount = aIndexTableCount;

    aCursorInfo->indexCursors          = NULL;
    aCursorInfo->row                   = NULL;

    // non-partitioned index들
    IDE_TEST( STRUCT_ALLOC_WITH_COUNT( aStatement->qmxMem,
                                       qmsIndexCursor,
                                       aIndexTableCount,
                                       & aCursorInfo->indexCursors )
              != IDE_SUCCESS );
    
    for ( i = 0, sIndexTable = aIndexTableRef;
          sIndexTable != NULL;
          i++, sIndexTable = sIndexTable->next )
    {
        sIndexCursor = &(aCursorInfo->indexCursors[i]);

        sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_ALLOC;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::deleteIndexTableCursors( qcStatement          * aStatement,
                                        qmsIndexTableCursors * aCursorInfo,
                                        scGRID                 aRowGRID )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexTableRef    * sIndexTable;
    qmsIndexCursor      * sIndexCursor;
    smiCursorProperties   sCursorProperty;
    qtcMetaRangeColumn    sRangeColumn;
    smiRange              sRange;
    const void          * sRow;
    scGRID                sGRID;
    UInt                  i;

    if ( aCursorInfo->indexCursors != NULL )
    {
        for ( i = 0, sIndexTable = aCursorInfo->indexTableRef;
              sIndexTable != NULL;
              i++, sIndexTable = sIndexTable->next )
        {
            sIndexCursor = &(aCursorInfo->indexCursors[i]);
        
            if ( sIndexCursor->cursorStatus == QMS_INDEX_CURSOR_STATUS_SKIP )
            {
                // selectedIndexTable은 제외한다.
                continue;
            }
            else
            {
                // Nothing to do.
            }
        
            // make smiRange
            qcm::makeMetaRangeSingleColumn( & sRangeColumn,
                                            sIndexCursor->ridColumn,
                                            & aRowGRID,
                                            & sRange );
        
            if ( sIndexCursor->cursorStatus != QMS_INDEX_CURSOR_STATUS_OPEN )
            {
                SMI_CURSOR_PROP_INIT_FOR_INDEX_SCAN( & sCursorProperty,
                                                     aStatement->mStatistics,
                                                     sIndexCursor->ridIndexType );
        
                sCursorProperty.mFetchColumnList = & (sIndexCursor->fetchColumn);
            
                // initialize
                sIndexCursor->cursor.initialize();
                    
                // open
                IDE_TEST( sIndexCursor->cursor.open( QC_SMI_STMT( aStatement ),
                                                     sIndexTable->tableHandle,
                                                     sIndexCursor->ridIndexHandle,
                                                     sIndexTable->tableSCN,
                                                     NULL,
                                                     & sRange,
                                                     smiGetDefaultKeyRange(),
                                                     smiGetDefaultFilter(),
                                                     SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD |
                                                     SMI_PREVIOUS_DISABLE,
                                                     SMI_DELETE_CURSOR,
                                                     & sCursorProperty )
                          != IDE_SUCCESS );
                
                IDE_TEST( sIndexCursor->cursor.beforeFirst()
                          != IDE_SUCCESS );

                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_OPEN;
            }
            else
            {
                IDE_TEST( sIndexCursor->cursor.restart( & sRange,
                                                        smiGetDefaultKeyRange(),
                                                        smiGetDefaultFilter() )
                          != IDE_SUCCESS );
                
                IDE_TEST( sIndexCursor->cursor.beforeFirst()
                          != IDE_SUCCESS );
            }

            // readRow
            sRow = aCursorInfo->row;
            
            IDE_TEST( sIndexCursor->cursor.readRow( & sRow,
                                                    & sGRID,
                                                    SMI_FIND_NEXT )
                      != IDE_SUCCESS );
            
            // 반드시 존재해야한다.
            IDE_TEST_RAISE( sRow == NULL, ERR_RID_NOT_FOUND );
            
            IDE_TEST( sIndexCursor->cursor.deleteRow()
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RID_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::deleteIndexTableCursors",
                                  "rid is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::updateIndexTableCursors( qcStatement          * aStatement,
                                        qmsIndexTableCursors * aCursorInfo,
                                        UInt                   aUpdateColumnCount,
                                        UInt                 * aUpdateColumnID,
                                        smiValue             * aUpdateValue,
                                        idBool                 aKeyNRidUpdate,
                                        smOID                * aPartOID,
                                        scGRID               * aRowGRID,
                                        scGRID                 aOldRowGRID )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qmsIndexTableRef    * sIndexTable;
    qmsIndexCursor      * sIndexCursor;
    smiCursorProperties   sCursorProperty;
    qtcMetaRangeColumn    sRangeColumn;
    smiRange              sRange;
    smiValue              sUpdateValue[QC_MAX_KEY_COLUMN_COUNT];
    UInt                  sUpdateValueCount;
    const void          * sRow;
    scGRID                sGRID;
    UInt                  i;

    if ( aCursorInfo->indexCursors != NULL )
    {
        for ( i = 0, sIndexTable = aCursorInfo->indexTableRef;
              sIndexTable != NULL;
              i++, sIndexTable = sIndexTable->next )
        {
            sIndexCursor = &(aCursorInfo->indexCursors[i]);
        
            if ( sIndexCursor->cursorStatus == QMS_INDEX_CURSOR_STATUS_SKIP )
            {
                // selectedIndexTable이거나 update column이 없는 경우는 제외한다.
                continue;
            }
            else
            {
                // Nothing to do.
            }

            if ( sIndexCursor->cursorStatus == QMS_INDEX_CURSOR_STATUS_ALLOC )
            {
                // update할 컬럼을 찾는다.
                IDE_TEST( makeUpdateSmiColumnList( aUpdateColumnCount,
                                                   aUpdateColumnID,
                                                   sIndexTable,
                                                   aKeyNRidUpdate,
                                                   & sIndexCursor->updateColumnCount,
                                                   sIndexCursor->updateColumnList )
                          != IDE_SUCCESS );

                // update할 컬럼이 없는 경우 skip한다.
                if ( sIndexCursor->updateColumnCount == 0 )
                {
                    sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_SKIP;
                    continue;
                }
                else
                {
                    // Nothing to do.
                }

                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_INIT;
            }
            else
            {
                // Nothing to do.
            }

            // 실제로 update할 컬럼이 없는 경우도 skip한다.
            //
            // 예)
            // create table t1( i1, i2, i3 ) partition by hash(i3) rowmovement enable;
            // create index idx1 on t1(i1);
            // update t1 set i2=i2+1;
            if ( ( sIndexCursor->updateColumnCount == 2 ) &&
                 ( aKeyNRidUpdate == ID_TRUE ) &&
                 ( SC_GRID_IS_EQUAL( *aRowGRID, aOldRowGRID ) ) )
            {
                continue;
            }
            else
            {
                // Nothing to do.
            }
        
            // make smiRange
            qcm::makeMetaRangeSingleColumn( & sRangeColumn,
                                            sIndexCursor->ridColumn,
                                            & aOldRowGRID,
                                            & sRange );
        
            if ( sIndexCursor->cursorStatus != QMS_INDEX_CURSOR_STATUS_OPEN )
            {
                SMI_CURSOR_PROP_INIT_FOR_INDEX_SCAN( & sCursorProperty,
                                                     aStatement->mStatistics,
                                                     sIndexCursor->ridIndexType );
        
                sCursorProperty.mFetchColumnList = & (sIndexCursor->fetchColumn);

                // initialize
                sIndexCursor->cursor.initialize();
                
                // open
                IDE_TEST( sIndexCursor->cursor.open( QC_SMI_STMT( aStatement ),
                                                     sIndexTable->tableHandle,
                                                     sIndexCursor->ridIndexHandle,
                                                     sIndexTable->tableSCN,
                                                     sIndexCursor->updateColumnList,
                                                     & sRange,
                                                     smiGetDefaultKeyRange(),
                                                     smiGetDefaultFilter(),
                                                     SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD |
                                                     SMI_PREVIOUS_DISABLE,
                                                     SMI_UPDATE_CURSOR,
                                                     & sCursorProperty )
                          != IDE_SUCCESS );
                
                IDE_TEST( sIndexCursor->cursor.beforeFirst()
                          != IDE_SUCCESS );
        
                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_OPEN;
            }
            else
            {
                IDE_TEST( sIndexCursor->cursor.restart( & sRange,
                                                        smiGetDefaultKeyRange(),
                                                        smiGetDefaultFilter() )
                          != IDE_SUCCESS );
                
                IDE_TEST( sIndexCursor->cursor.beforeFirst()
                          != IDE_SUCCESS );
            }

            // readRow
            sRow = aCursorInfo->row;
        
            IDE_TEST( sIndexCursor->cursor.readRow( & sRow,
                                                    & sGRID,
                                                    SMI_FIND_NEXT )
                      != IDE_SUCCESS );

            // 반드시 존재해야한다.
            IDE_TEST_RAISE( sRow == NULL, ERR_RID_NOT_FOUND );
        
            // update할 값을 설정한다.
            IDE_TEST( makeUpdateSmiValue( aUpdateColumnCount,
                                          aUpdateColumnID,
                                          aUpdateValue,
                                          sIndexTable,
                                          aKeyNRidUpdate,
                                          aPartOID,
                                          aRowGRID,
                                          & sUpdateValueCount,
                                          sUpdateValue )
                      != IDE_SUCCESS );
        
            IDE_TEST( sIndexCursor->cursor.updateRow( sUpdateValue )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RID_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::updateNullIndexTableCursors",
                                  "rid is not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::insertIndexTableCursors( qcStatement          * aStatement,
                                        qmsIndexTableCursors * aCursorInfo,
                                        smiValue             * aInsertValue,
                                        smOID                  aPartOID,
                                        scGRID                 aRowGRID )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qmsIndexTableRef    * sIndexTable;
    qmsIndexCursor      * sIndexCursor;
    smiCursorProperties   sCursorProperty;
    smiValue              sInsertValue[QC_MAX_KEY_COLUMN_COUNT + 2];
    smOID                 sPartOID = aPartOID;
    scGRID                sRowGRID = aRowGRID;
    void                * sRow;
    scGRID                sGRID;
    UInt                  i;

    IDE_DASSERT( aCursorInfo->indexCursors != NULL );

    for ( i = 0, sIndexTable = aCursorInfo->indexTableRef;
          sIndexTable != NULL;
          i++, sIndexTable = sIndexTable->next )
    {
        sIndexCursor = &(aCursorInfo->indexCursors[i]);
        
        if ( sIndexCursor->cursorStatus != QMS_INDEX_CURSOR_STATUS_OPEN )
        {
            SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( & sCursorProperty, aStatement->mStatistics );

            // initialize
            sIndexCursor->cursor.initialize();
            
            // open
            IDE_TEST( sIndexCursor->cursor.open( QC_SMI_STMT( aStatement ),
                                                 sIndexTable->tableHandle,
                                                 NULL,
                                                 sIndexTable->tableSCN,
                                                 NULL,
                                                 smiGetDefaultKeyRange(),
                                                 smiGetDefaultKeyRange(),
                                                 smiGetDefaultFilter(),
                                                 SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD |
                                                 SMI_PREVIOUS_DISABLE,
                                                 SMI_INSERT_CURSOR,
                                                 & sCursorProperty )
                      != IDE_SUCCESS );

            sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_OPEN;
        }
        else
        {
            // Nothing to do.
        }

        // insert할 컬럼을 찾는다.
        IDE_TEST( makeInsertSmiValue( aInsertValue,
                                      sIndexTable,
                                      & sPartOID,
                                      & sRowGRID,
                                      sInsertValue )
                  != IDE_SUCCESS );

        IDE_TEST( sIndexCursor->cursor.insertRow( sInsertValue,
                                                  & sRow,
                                                  & sGRID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::closeIndexTableCursors( qmsIndexTableCursors * aCursorInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexCursor    * sIndexCursor;
    UInt                i;

    if ( aCursorInfo->indexCursors != NULL )
    {
        for ( i = 0; i < aCursorInfo->indexTableCount; i++ )
        {
            sIndexCursor = &(aCursorInfo->indexCursors[i]);
        
            if ( sIndexCursor->cursorStatus == QMS_INDEX_CURSOR_STATUS_OPEN )
            {
                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_INIT;

                IDE_TEST( sIndexCursor->cursor.close()
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
        // Nothing to do.
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmsIndexTable::finalizeIndexTableCursors( qmsIndexTableCursors * aCursorInfo )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsIndexCursor    * sIndexCursor;
    UInt                i;

    if ( aCursorInfo->indexCursors != NULL )
    {
        for ( i = 0; i < aCursorInfo->indexTableCount; i++ )
        {
            sIndexCursor = &(aCursorInfo->indexCursors[i]);
        
            if ( sIndexCursor->cursorStatus == QMS_INDEX_CURSOR_STATUS_OPEN )
            {
                sIndexCursor->cursorStatus = QMS_INDEX_CURSOR_STATUS_INIT;
            
                (void) sIndexCursor->cursor.close();
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

IDE_RC
qmsIndexTable::insertIndexTable4OneRow( smiStatement      * aSmiStmt,
                                        qmsIndexTableRef  * aIndexTableRef,
                                        smiValue          * aInsertValue,
                                        smOID               aPartOID,
                                        scGRID              aRowGRID )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/

    smiTableCursor        sCursor;
    smiCursorProperties   sCursorProperty;
    idBool                sCursorOpened = ID_FALSE;
    qmsIndexTableRef    * sIndexTable;
    smiValue              sInsertValue[QC_MAX_KEY_COLUMN_COUNT + 2];
    smOID                 sPartOID = aPartOID;
    scGRID                sRowGRID = aRowGRID;
    void                * sRow;
    scGRID                sGRID;
    UInt                  i;

    // initialize
    sCursor.initialize();
    
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( & sCursorProperty, NULL );
    
    for ( i = 0, sIndexTable = aIndexTableRef;
          sIndexTable != NULL;
          i++, sIndexTable = sIndexTable->next )
    {
        // open
        IDE_TEST( sCursor.open( aSmiStmt,
                                sIndexTable->tableHandle,
                                NULL,
                                sIndexTable->tableSCN,
                                NULL,
                                smiGetDefaultKeyRange(),
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|
                                SMI_PREVIOUS_DISABLE,
                                SMI_INSERT_CURSOR,
                                & sCursorProperty )
                  != IDE_SUCCESS );
        sCursorOpened = ID_TRUE;

        // insert할 컬럼을 찾는다.
        IDE_TEST( makeInsertSmiValue( aInsertValue,
                                      sIndexTable,
                                      & sPartOID,
                                      & sRowGRID,
                                      sInsertValue )
                  != IDE_SUCCESS );
        
        IDE_TEST( sCursor.insertRow( sInsertValue,
                                     & sRow,
                                     & sGRID )
                  != IDE_SUCCESS );

        sCursorOpened = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sCursorOpened == ID_TRUE )
    {
        (void) sCursor.close();
    }
    
    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::updateIndexTable4OneRow( smiStatement      * aSmiStmt,
                                        qmsIndexTableRef  * aIndexTableRef,
                                        UInt                aUpdateColumnCount,
                                        UInt              * aUpdateColumnID,
                                        smiValue          * aUpdateValue,
                                        void              * aRow,
                                        scGRID              aRowGRID )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/
    
    smiTableCursor        sCursor;
    smiCursorProperties   sCursorProperty;
    idBool                sCursorOpened = ID_FALSE;
    qmsIndexTableRef    * sIndexTable;
    qcmTableInfo        * sIndexTableInfo;
    UInt                  sUpdateColumnCount;
    smiFetchColumnList    sFetchColumn;
    qtcMetaRangeColumn    sRangeColumn;
    smiRange              sRange;
    qcmColumn           * sRIDColumn;
    qcmIndex            * sIndex        = NULL;
    smiColumnList         sUpdateColumnList[QC_MAX_KEY_COLUMN_COUNT];
    smiValue              sUpdateValue[QC_MAX_KEY_COLUMN_COUNT];
    UInt                  sUpdateValueCount;
    const void          * sRow;
    scGRID                sGRID;
    UInt                  i;

    // initialize
    sCursor.initialize();
    
    for ( i = 0, sIndexTable = aIndexTableRef;
          sIndexTable != NULL;
          i++, sIndexTable = sIndexTable->next )
    {
        sIndexTableInfo = sIndexTable->tableInfo;

        // update할 컬럼을 찾는다.
        IDE_TEST( makeUpdateSmiColumnList( aUpdateColumnCount,
                                           aUpdateColumnID,
                                           sIndexTable,
                                           ID_FALSE,
                                           & sUpdateColumnCount,
                                           sUpdateColumnList )
                      != IDE_SUCCESS );

            // update할 컬럼이 없는 경우 skip한다.
        if ( sUpdateColumnCount == 0 )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        sRIDColumn = &(sIndexTableInfo->columns[sIndexTableInfo->columnCount - 1]);
        
        // make smiRange
        qcm::makeMetaRangeSingleColumn( & sRangeColumn,
                                        sRIDColumn->basicInfo,
                                        & aRowGRID,
                                        & sRange );

        // make fetch column, only rid
        sFetchColumn.column         = (smiColumn*)&(sRIDColumn->basicInfo->column);
        sFetchColumn.columnSeq      = SMI_GET_COLUMN_SEQ( sFetchColumn.column );
        sFetchColumn.copyDiskColumn = 
            qdbCommon::getCopyDiskColumnFunc( sRIDColumn->basicInfo );

        sFetchColumn.next           = NULL;
        
        // rid index handle
        IDE_TEST( findRidIndex( sIndexTableInfo,
                                & sIndex )
                  != IDE_SUCCESS );

        SMI_CURSOR_PROP_INIT_FOR_INDEX_SCAN( & sCursorProperty, NULL, sIndex->indexTypeId );
        
        sCursorProperty.mFetchColumnList = & sFetchColumn;
            
        // open
        IDE_TEST( sCursor.open( aSmiStmt,
                                sIndexTable->tableHandle,
                                sIndex->indexHandle,
                                sIndexTable->tableSCN,
                                sUpdateColumnList,
                                & sRange,
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|
                                SMI_PREVIOUS_DISABLE,
                                SMI_UPDATE_CURSOR,
                                & sCursorProperty )
                  != IDE_SUCCESS );
        sCursorOpened = ID_TRUE;

        // readRow
        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        
        sRow = aRow;
        
        IDE_TEST( sCursor.readRow( & sRow,
                                   & sGRID,
                                   SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // 반드시 존재해야한다.
        IDE_TEST_RAISE( sRow == NULL, ERR_RID_NOT_FOUND );
        
        // update할 값을 설정한다.
        IDE_TEST( makeUpdateSmiValue( aUpdateColumnCount,
                                      aUpdateColumnID,
                                      aUpdateValue,
                                      sIndexTable,
                                      ID_FALSE,
                                      NULL,
                                      NULL,
                                      & sUpdateValueCount,
                                      sUpdateValue )
                  != IDE_SUCCESS );
        
        IDE_TEST( sCursor.updateRow( sUpdateValue )
                  != IDE_SUCCESS );

        sCursorOpened = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RID_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::updateIndexTable4OneRow",
                                  "rid is not found" ));
    }
    IDE_EXCEPTION_END;

    if ( sCursorOpened == ID_TRUE )
    {
        (void) sCursor.close();
    }
    
    return IDE_FAILURE;
}

IDE_RC
qmsIndexTable::deleteIndexTable4OneRow( smiStatement      * aSmiStmt,
                                        qmsIndexTableRef  * aIndexTableRef,
                                        void              * aRow,
                                        scGRID              aRowGRID )
{
/***********************************************************************
 *
 * Description : PROJ-1623 non-partitioned index
 *
 * Implementation :
 *
 ***********************************************************************/
    
    smiTableCursor        sCursor;
    smiCursorProperties   sCursorProperty;
    idBool                sCursorOpened = ID_FALSE;
    qmsIndexTableRef    * sIndexTable;
    qcmTableInfo        * sIndexTableInfo;
    smiFetchColumnList    sFetchColumn;
    qtcMetaRangeColumn    sRangeColumn;
    smiRange              sRange;
    qcmColumn           * sRIDColumn;
    qcmIndex            * sIndex        = NULL;
    const void          * sRow;
    scGRID                sGRID;
    UInt                  i;

    // initialize
    sCursor.initialize();
    
    for ( i = 0, sIndexTable = aIndexTableRef;
          sIndexTable != NULL;
          i++, sIndexTable = sIndexTable->next )
    {
        sIndexTableInfo = sIndexTable->tableInfo;

        sRIDColumn = &(sIndexTableInfo->columns[sIndexTableInfo->columnCount - 1]);
        
        // make smiRange
        qcm::makeMetaRangeSingleColumn( & sRangeColumn,
                                        sRIDColumn->basicInfo,
                                        & aRowGRID,
                                        & sRange );

        // make fetch column, only rid
        sFetchColumn.column         = (smiColumn*)&(sRIDColumn->basicInfo->column);
        sFetchColumn.columnSeq      = SMI_GET_COLUMN_SEQ( sFetchColumn.column );
        sFetchColumn.copyDiskColumn = 
            qdbCommon::getCopyDiskColumnFunc( sRIDColumn->basicInfo );
        sFetchColumn.next           = NULL;
        
        // rid index handle
        IDE_TEST( findRidIndex( sIndexTableInfo,
                                & sIndex )
                  != IDE_SUCCESS );

        SMI_CURSOR_PROP_INIT_FOR_INDEX_SCAN( & sCursorProperty, NULL, sIndex->indexTypeId );
        
        sCursorProperty.mFetchColumnList = & sFetchColumn;
            
        // open
        IDE_TEST( sCursor.open( aSmiStmt,
                                sIndexTable->tableHandle,
                                sIndex->indexHandle,
                                sIndexTable->tableSCN,
                                NULL,
                                & sRange,
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|
                                SMI_PREVIOUS_DISABLE,
                                SMI_DELETE_CURSOR,
                                & sCursorProperty )
                  != IDE_SUCCESS );
        sCursorOpened = ID_TRUE;

        // readRow
        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        
        sRow = aRow;
        
        IDE_TEST( sCursor.readRow( & sRow,
                                   & sGRID,
                                   SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // 반드시 존재해야한다.
        IDE_TEST_RAISE( sRow == NULL, ERR_RID_NOT_FOUND );
        
        IDE_TEST( sCursor.deleteRow() != IDE_SUCCESS );

        sCursorOpened = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RID_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmsIndexTable::deleteIndexTable4OneRow",
                                  "rid is not found" ));
    }
    IDE_EXCEPTION_END;

    if ( sCursorOpened == ID_TRUE )
    {
        (void) sCursor.close();
    }
    
    return IDE_FAILURE;
}
