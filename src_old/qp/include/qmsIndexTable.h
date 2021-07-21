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
 * $Id: qmsIndexTable.h 56549 2012-11-23 04:38:50Z sungminee $
 **********************************************************************/

#ifndef _O_QMS_INDEX_TABLE_H_
#define _O_QMS_INDEX_TABLE_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qcmTableInfo.h>

#define QMS_INDEX_CURSOR_STATUS_ALLOC  ( (UChar)'a' )
#define QMS_INDEX_CURSOR_STATUS_INIT   ( (UChar)'i' )
#define QMS_INDEX_CURSOR_STATUS_SKIP   ( (UChar)'s' )
#define QMS_INDEX_CURSOR_STATUS_OPEN   ( (UChar)'o' )

typedef struct qmsIndexCursor
{
    // index table index
    void                * ridIndexHandle;
    UInt                  ridIndexType;
    mtcColumn           * ridColumn;

    // index table cursor
    smiTableCursor        cursor;
    smiFetchColumnList    fetchColumn;
    smiColumnList         updateColumnList[QC_MAX_KEY_COLUMN_COUNT + 2];  // +oid,rid
    UInt                  updateColumnCount;

    // cursor flag
    UChar                 cursorStatus;
    
} qmsIndexCursor;

typedef struct qmsIndexTableCursors
{
    // tableRef의 모든 index tables
    qmsIndexTableRef    * indexTableRef;
    UInt                  indexTableCount;

    // index table만큼 할당함
    qmsIndexCursor      * indexCursors;

    // max로 할당하고 공유함
    const void          * row;

} qmsIndexTableCursors;

typedef struct qmsPartRefIndex
{
    smOID              partOID;
    qmsPartitionRef  * partitionRef;
    UInt               partOrder;
} qmsPartRefIndex;

typedef struct qmsPartRefIndexInfo
{
    qmsPartRefIndex  * partRefIndex;
    UInt               partCount;
} qmsPartRefIndexInfo;

class qmsIndexTable
{
public:

    static IDE_RC makeIndexTableRef( qcStatement       * aStatement,
                                     qcmTableInfo      * aTableInfo,
                                     qmsIndexTableRef ** aIndexTableRef,
                                     UInt              * aIndexTableCount );

    static IDE_RC makeOneIndexTableRef( qcStatement      * aStatement,
                                        qcmIndex         * aIndex,
                                        qmsIndexTableRef * aIndexTableRef );

    static IDE_RC validateAndLockOneIndexTableRef( qcStatement      * aStatement,
                                                   qmsIndexTableRef * aIndexTableRef,
                                                   smiTableLockMode   aLockMode );

    static IDE_RC validateAndLockIndexTableRefList( qcStatement      * aStatement,
                                                    qmsIndexTableRef * aIndexTableRef,
                                                    smiTableLockMode   aLockMode );
    
    static IDE_RC findKeyIndex( qcmTableInfo  * aTableInfo,
                                qcmIndex     ** aKeyIndex );

    static IDE_RC findRidIndex( qcmTableInfo  * aTableInfo,
                                qcmIndex     ** aRidIndex );

    static IDE_RC findIndexTableRefInList( qmsIndexTableRef  * aIndexTableRef,
                                           UInt                aIndexID,
                                           qmsIndexTableRef ** aFoundIndexTableRef );

    static IDE_RC initializePartitionRefIndex( qcStatement         * aStatement,
                                               qmsPartitionRef     * aPartitionRef,
                                               UInt                  aPartitionCount,
                                               qmsPartRefIndexInfo * aPartIndexInfo );

    static IDE_RC findPartitionRefIndex( qmsPartRefIndexInfo  * aPartIndexInfo,
                                         smOID                  aPartOID,
                                         qmsPartitionRef     ** aFoundPartitionRef,
                                         UInt                 * aPartitionOrder );
    
    static IDE_RC makeUpdateSmiColumnList( UInt                aUpdateColumnCount,
                                           UInt              * aUpdateColumnID,
                                           qmsIndexTableRef  * aIndexTable,
                                           idBool              aKeyNRidUpdate,
                                           UInt              * aIndexUpdateColumnCount,
                                           smiColumnList     * aIndexUpdateColumnList );

    static IDE_RC makeUpdateSmiValue( UInt                aUpdateColumnCount,
                                      UInt              * aUpdateColumnID,
                                      smiValue          * aUpdateValue,
                                      qmsIndexTableRef  * aIndexTable,
                                      idBool              aKeyNRidUpdate,
                                      smOID             * aPartOID,
                                      scGRID            * aRowGRID,
                                      UInt              * aIndexUpdateValueCount,
                                      smiValue          * aIndexUpdateValue );

    static IDE_RC makeInsertSmiValue( smiValue          * aInsertValue,
                                      qmsIndexTableRef  * aIndexTable,
                                      smOID             * aPartOID,
                                      scGRID            * aRowGRID,
                                      smiValue          * aIndexInsertValue );

    static IDE_RC initializeIndexTableCursors( qcStatement          * aStatement,
                                               qmsIndexTableRef     * aIndexTableRef,
                                               UInt                   aIndexTableCount,
                                               qmsIndexTableRef     * aSelectedIndexTableRef,
                                               qmsIndexTableCursors * aCursorInfo );

    static IDE_RC initializeIndexTableCursors4Insert( qcStatement          * aStatement,
                                                      qmsIndexTableRef     * aIndexTableRef,
                                                      UInt                   aIndexTableCount,
                                                      qmsIndexTableCursors * aCursorInfo );
    
    static IDE_RC deleteIndexTableCursors( qcStatement          * aStatement,
                                           qmsIndexTableCursors * aCursorInfo,
                                           scGRID                 aRowGRID );

    static IDE_RC updateIndexTableCursors( qcStatement          * aStatement,
                                           qmsIndexTableCursors * aCursorInfo,
                                           UInt                   aUpdateColumnCount,
                                           UInt                 * aUpdateColumnID,
                                           smiValue             * aUpdateValue,
                                           idBool                 aKeyNRidUpdate,
                                           smOID                * aPartOID,
                                           scGRID               * aRowGRID,
                                           scGRID                 aOldRowGRID );
    
    static IDE_RC insertIndexTableCursors( qcStatement          * aStatement,
                                           qmsIndexTableCursors * aCursorInfo,
                                           smiValue             * aInsertValue,
                                           smOID                  aPartOID,
                                           scGRID                 aRowGRID );
    
    static IDE_RC closeIndexTableCursors( qmsIndexTableCursors * aCursorInfo );
    
    static void finalizeIndexTableCursors( qmsIndexTableCursors * aCursorInfo );

    static IDE_RC insertIndexTable4OneRow( smiStatement      * sSmiStmt,
                                           qmsIndexTableRef  * aIndexTableRef,
                                           smiValue          * aInsertValue,
                                           smOID               aPartOID,
                                           scGRID              aRowGRID );

    static IDE_RC updateIndexTable4OneRow( smiStatement      * aSmiStmt,
                                           qmsIndexTableRef  * aIndexTableRef,
                                           UInt                aUpdateColumnCount,
                                           UInt              * aUpdateColumnID,
                                           smiValue          * aUpdateValue,
                                           void              * aRow,
                                           scGRID              aRowGRID );

    static IDE_RC deleteIndexTable4OneRow( smiStatement      * aSmiStmt,
                                           qmsIndexTableRef  * aIndexTableRef,
                                           void              * aRow,
                                           scGRID              aRowGRID );
};

#endif /* _O_QMS_INDEX_TABLE_H_ */
