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
 * $Id: qdx.h 86367 2019-11-18 07:32:25Z donghyun1 $
 **********************************************************************/
#ifndef _O_QDX_H_
#define  _O_QDX_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qcm.h>
#include <qcmPartition.h>
#include <qdParseTree.h>

// qd indeX -- create index
class qdx
{
public:
    // parse
    static IDE_RC parse(
        qcStatement * aStatement);

    // validate.
    static IDE_RC validate(
        qcStatement * aStatement);

    static IDE_RC validatePartitionedIndexOnCreateIndex(
        qcStatement      * aStatement,
        qdIndexParseTree * aParseTree,
        qcmTableInfo     * aTableInfo,
        UInt               aIndexType );

    // fix BUG-18937
    static IDE_RC validatePartitionedIndexOnAlterTable(
        qcStatement        * aStatement,
        qcNamePosition       aPartIndexTBSName,
        qdPartitionedIndex * aPartIndex,
        qcmTableInfo       * aTableInfo );

    // fix BUG-18937
    static IDE_RC validatePartitionedIndexOnCreateTable(
        qcStatement        * aStatement,
        qdTableParseTree   * aParseTree,
        qcNamePosition       aPartIndexTBSName,
        qdPartitionedIndex * aPartIndex );

    // PROJ-1624 global non-partitioned index
    static IDE_RC validateNonPartitionedIndex(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aIndexName,
        SChar           * aIndexTableName,
        SChar           * aKeyIndexName,
        SChar           * aRidIndexName );

    static IDE_RC checkLocalIndexOnAlterTable( qcStatement      * aStatement,
                                               qcmTableInfo     * aTableInfo,
                                               qcmColumn        * aIndexKeyColumns,
                                               qcmColumn        * aPartKeyColumns,
                                               idBool           * aIsLocalIndex );

    static IDE_RC checkLocalIndexOnCreateTable( qcmColumn        * aIndexKeyColumns,
                                                qcmColumn        * aPartKeyColumns,
                                                idBool           * aIsLocalIndex );

    static IDE_RC validateAlter(qcStatement *aStatement);
    static IDE_RC validateAlterSegAttr(qcStatement *aStatement);
    static IDE_RC validateAlterSegStoAttr(qcStatement *aStatement);
    static IDE_RC validateAlterAllocExtent(qcStatement *aStatement);

    //fix PROJ-1596
    static IDE_RC validateKeySizeLimit(
        qcStatement      * aStatement,
        iduVarMemList    * aMemory,
        UInt               aTableType,
        void             * aKeyColumns,
        UInt               aKeyColCount,
        UInt               aIndexType);

    // execute
    static IDE_RC execute(
        qcStatement * aStatement);

    static IDE_RC executeAlterPers(
        qcStatement * aStatement);

    // PROJ-1704 MVCC Renewal
    static IDE_RC executeAlterSegAttr(
        qcStatement * aStatement );

    // PROJ-1671
    static IDE_RC executeAlterSegStoAttr(
        qcStatement * aStatement );
    static IDE_RC executeAlterAllocExts(
        qcStatement * aStatement );

    static IDE_RC insertIndexIntoMeta(
        qcStatement * aStatement,
        scSpaceID     aTBSID,
        UInt          aUserID,
        UInt          aTableID,
        UInt          aIndexID,
        SChar       * aIdxName,
        SInt          aType,
        idBool        aIsUnique,
        SInt          aKeyColCount,
        idBool        aIsRange,
        idBool        aIsPartitioned,
        UInt          aIndexTableID,
        SInt          aFlag);

    static IDE_RC updateIndexPers(
        qcStatement * aStatement,
        UInt          aIndexID,
        SInt          aFlag);

    static IDE_RC insertIndexColumnIntoMeta(qcStatement *aStatement,
                                            UInt         aUserID,
                                            UInt         aIndexID,
                                            UInt         aColumnID,
                                            UInt         aColIndexOrder,
                                            idBool       aIsAscending,
                                            UInt         aTableID);

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC copyIndexRelatedMeta( qcStatement * aStatement,
                                        UInt          aToTableID,
                                        UInt          aToIndexID,
                                        UInt          aFromIndexID );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC createIndexPartition(
        qcStatement   * aStatement,
        qcmTableInfo  * aTableInfo,
        UInt            aPartIndexID,
        UInt            aIndexType,
        smiColumnList * aColumnListAtKey );

    static IDE_RC insertPartIndexIntoMeta(
        qcStatement * aStatement,
        UInt         aUserID,
        UInt         aTableID,
        UInt         aIndexID,
        UInt         aPartIndexType,
        SInt         aFlag);

    static IDE_RC insertIndexPartKeyColumnIntoMeta(
        qcStatement  * aStatement,
        UInt           aUserID,
        UInt           aIndexID,
        UInt           aColumnID,
        qcmTableInfo * aTableInfo,
        UInt           aObjType );

    static IDE_RC insertIndexPartitionsIntoMeta(
        qcStatement  * aStatement,
        UInt           aUserID,
        UInt           aTableID,
        UInt           aPartIndexID,
        UInt           aTablePartID,
        UInt           aIndexPartID,
        SChar        * aIndexPartName,
        SChar        * aPartMinValue,
        SChar        * aPartMaxValue,
        scSpaceID      aTBSID );

    static IDE_RC makeIndexPartition( 
        qcStatement          * aStatement,
        qcmPartitionInfoList * aPartInfoList,
        qdPartitionedIndex   * aPartIndex );

    static IDE_RC validateAlterRebuild( qcStatement * aStatement );

    static IDE_RC executeAlterRebuild( qcStatement * aStatement );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC validateAlterRebuildPartition( qcStatement * aStatement );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC executeAlterRebuildPartition( qcStatement * aStatement );

    // PROJ-1704 MVCC Renewal
    static IDE_RC validateAgingIndex( qcStatement * aStatement );
    
    // PROJ-1704 MVCC Renewal
    static IDE_RC executeAgingIndex( qcStatement * aStatement );

    static IDE_RC getKeyColumnList( 
        qcStatement          * aStatement,
        qcmIndex             * aIndex,
        smiColumnList       ** aColumnListAtKey );

    static IDE_RC createAllIndexOfTablePart( qcStatement               * aStatement,
                                             qcmTableInfo              * aTableInfo,
                                             qcmTableInfo              * aTablePartInfo,
                                             qdIndexPartitionAttribute * aIndexTBSAttr );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC createAllIndexOfTableForAlterTablespace( qcStatement               * aStatement,
                                                           qcmTableInfo              * aOldTableInfo,
                                                           qcmTableInfo              * aNewTableInfo,
                                                           qdIndexPartitionAttribute * aIndexTBSAttr );

    // BUG-15235 RENAME INDEX
    static IDE_RC validateAlterRename( qcStatement * aStatement );

    static IDE_RC executeAlterRename( qcStatement * aStatement );
    
    static IDE_RC updateIndexNameFromMeta( qcStatement *  aStatement,
                                           UInt           aIndexID,
                                           qcNamePosition aNewIndexName );

    // PROJ-1624 global non-partitioned index
    static IDE_RC makeColumns4CreateIndexTable( qcStatement  * aStatement,
                                                qcmColumn    * aIndexColumn,
                                                UInt           aIndexColumnCount,
                                                qcmColumn   ** aTableColumn,
                                                UInt         * aTableColumnCount );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC createIndexTable( qcStatement       * aStatement,
                                    UInt                aUserID,
                                    qcNamePosition      aTableName,
                                    qcmColumn         * aColumns,
                                    UInt                aColumnCount,
                                    scSpaceID           aTBSID,
                                    smiSegAttr          aSegAttr,
                                    smiSegStorageAttr   aSegStoAttr,
                                    UInt                aInitFlagMask,
                                    UInt                aInitFlagValue,
                                    UInt                aParallelDegree,
                                    qdIndexTableList ** aIndexTable );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC createIndexTableIndices( qcStatement      * aStatement,
                                           UInt               aUserID,
                                           qdIndexTableList * aIndexTable,
                                           qcmColumn        * aKeyColumns,
                                           SChar            * aKeyIndexName,
                                           SChar            * aRidIndexName,
                                           scSpaceID          aTBSID,
                                           UInt               aIndexType,
                                           UInt               aIndexFlag,
                                           UInt               aParallelDegree,
                                           UInt               aBuildFlag,
                                           smiSegAttr         aSegAttr,
                                           smiSegStorageAttr  aSegStoAttr,
                                           ULong              aDirectKeyMaxSize );

    // PROJ-1624 global non-partitioned index
    static IDE_RC makeIndexTableName( qcStatement     * aStatement,
                                      qcNamePosition    aIndexNamePos,
                                      SChar           * aIndexName,
                                      SChar           * aIndexTableName,
                                      SChar           * aKeyIndexName,
                                      SChar           * aRidIndexName );

    // PROJ-1624 global non-partitioned index
    static IDE_RC makeAndLockIndexTableList( qcStatement       * aStatement,
                                             idBool              aInExecutionTime,
                                             qcmTableInfo      * aTableInfo,
                                             qdIndexTableList ** aIndexTables );

    // PROJ-1624 global non-partitioned index
    static IDE_RC makeAndLockIndexTable( qcStatement       * aStatement,
                                         idBool              aInExecutionTime,
                                         UInt                aIndexTableID,
                                         qdIndexTableList ** aIndexTables );

    // PROJ-1624 global non-partitioned index
    static IDE_RC validateAndLockIndexTableList( qcStatement         * aStatement,
                                                 qdIndexTableList    * aIndexTables,
                                                 smiTBSLockValidType   aTBSLvType,
                                                 smiTableLockMode      aLockMode,
                                                 ULong                 aLockWaitMicroSec );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC dropIndexTable( qcStatement      * aStatement,
                                  qdIndexTableList * aIndexTable,
                                  idBool             aIsDropTablespace );   // BUG-35135
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC makeSmiValue4IndexTableWithSmiValue( smiValue     * aInsertedTableRow,
                                                       smiValue     * aInsertedIndexRow,
                                                       qcmIndex     * aTableIndex,
                                                       smOID        * aPartOID,
                                                       scGRID       * aRowGRID );

    // PROJ-1624 global non-partitioned index
    static IDE_RC makeSmiValue4IndexTableWithRow( const void   * aTableRow,
                                                  qcmColumn    * aTableColumn,
                                                  smiValue     * aInsertedRow,
                                                  smOID        * aPartOID,
                                                  scGRID       * aRowGRID );

    // PROJ-1624 global non-partitioned index
    static IDE_RC updateIndexSpecFromMeta( qcStatement  * aStatement,
                                           UInt           aIndexID,
                                           UInt           aIndexTableID );

    // PROJ-1624 global non-partitioned index
    static IDE_RC buildIndexTable( qcStatement          * aStatement,
                                   qdIndexTableList     * aIndexTable,
                                   qcmColumn            * aTableColumns,
                                   UInt                   aTableColumnCount,
                                   qcmTableInfo         * aTableInfo,
                                   qcmPartitionInfoList * aPartInfoList );

    // PROJ-1624 global non-partitioned index
    static IDE_RC getIndexTableIndices( qcmTableInfo * aIndexTableInfo,
                                        qcmIndex     * aIndexTableIndex[2] );

    // PROJ-1624 global non-partitioned index
    static IDE_RC makeColumns4BuildIndexTable( qcStatement   * aStatement,
                                               qcmTableInfo  * aTableInfo,
                                               mtcColumn     * aKeyColumns,
                                               UInt            aKeyColumnCount,
                                               qcmColumn    ** aColumns,
                                               UInt          * aColumnCount );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC makeColumns4ModifyColumn( qcStatement   * aStatement,
                                            qcmTableInfo  * aTableInfo,
                                            mtcColumn     * aKeyColumns,
                                            UInt            aKeyColumnCount,
                                            scSpaceID       aTBSID,
                                            qcmColumn    ** aColumns,
                                            UInt          * aColumnCount );

    // PROJ-1624 global non-partitioned index
    static IDE_RC findIndexTableInList( qdIndexTableList  * aIndexTables,
                                        UInt                aIndexTableID,
                                        qdIndexTableList ** aFoundIndexTable );

    // PROJ-1624 global non-partitioned index
    static IDE_RC findIndexTableIDInIndices( qcmIndex       * aIndices,
                                             UInt             aIndexCount,
                                             UInt             aIndexTableID,
                                             qcmIndex      ** aFoundIndex );

    static IDE_RC findIndexIDInIndices( qcmIndex     * aIndices,
                                        UInt           aIndexCount,
                                        UInt           aIndexID,
                                        qcmIndex    ** aFoundIndex );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC createIndexTableOfTable( qcStatement       * aStatement,
                                           qcmTableInfo      * aTableInfo,
                                           qcmIndex          * aNewIndices,
                                           qdIndexTableList  * aOldIndexTables,
                                           qdIndexTableList ** aNewIndexTables );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC updateIndexTableSpecFromMeta( qcStatement   * aStatement,
                                                qcmTableInfo  * aTableInfo,
                                                qcmIndex      * aNewIndices );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC deletePartitionInIndexTableList( qcStatement      * aStatement,
                                                   qdIndexTableList * aIndexTables,
                                                   smOID              aPartOID );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC initializeInsertIndexTableCursors( qcStatement         * aStatement,
                                                     qdIndexTableList    * aIndexTables,
                                                     qdIndexTableCursors * aCursorInfo,
                                                     qcmIndex            * aIndices,
                                                     UInt                  aIndexCount,
                                                     UInt                  aColumnCount,
                                                     smiCursorProperties * aCursorProperty );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC insertIndexTableCursors( qdIndexTableCursors * aCursorInfo,
                                           smiValue            * aNewRow,
                                           smOID                 aNewPartOID,
                                           scGRID                aNewGRID );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC closeInsertIndexTableCursors( qdIndexTableCursors * aCursorInfo );
  
    // PROJ-1624 global non-partitioned index
    static void finalizeInsertIndexTableCursors( qdIndexTableCursors * aCursorInfo );
  
    // PROJ-1624 global non-partitioned index
    static IDE_RC initializeUpdateIndexTableCursors( qcStatement         * aStatement,
                                                     qdIndexTableList    * aIndexTables,
                                                     qdIndexTableCursors * aCursorInfo );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC updateIndexTableCursors( qcStatement         * aStatement,
                                           qdIndexTableCursors * aCursorInfo,
                                           smOID                 aNewPartOID,
                                           scGRID                aOldGRID,
                                           scGRID                aNewGRID );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC closeUpdateIndexTableCursors( qdIndexTableCursors * aCursorInfo );
    
    // PROJ-1624 global non-partitioned index
    static void finalizeUpdateIndexTableCursors( qdIndexTableCursors * aCursorInfo );
    
    // PROJ-1624 global non-partitioned index
    static IDE_RC checkIndexTableName( qcStatement     * aStatement,
                                       qcNamePosition    aUserName,
                                       qcNamePosition    aIndexName,
                                       SChar           * aIndexTableName,
                                       SChar           * aKeyIndexName,
                                       SChar           * aRidIndexName );

    /* PROJ-2433 direct key index */
    static IDE_RC validateAlterDirectKey( qcStatement * aStatement );
    static IDE_RC executeAlterDirectKey( qcStatement * aStatement );

    /* PROJ-2614 Memory Index Key Reorganization */
    static IDE_RC validateAlterReorganization( qcStatement * aStatement );
    static IDE_RC executeAlterReorganization( qcStatement * aStatement);
private:

    // PROJ-1624 global non-partitioned index
    static IDE_RC createIndex4IndexTable( qcStatement     * aStatement,
                                          UInt              aUserID,
                                          UInt              aTableID,
                                          const void      * aTableHandle,
                                          qcmColumn       * aColumns,
                                          UInt              aColumnCount,
                                          qcmColumn       * aKeyColumns,
                                          SChar           * aIndexName,
                                          scSpaceID         aTBSID,
                                          UInt              aIndexType,
                                          UInt              aIndexFlag,
                                          UInt              aParallelDegree,
                                          UInt              aBuildFlag,
                                          smiSegAttr        aSegAttr,
                                          smiSegStorageAttr aSegStoAttr, 
                                          ULong             aDirectKeyMaxSize );

    /* PROJ-2464 hybrid partitioned table Áö¿ø */
    static IDE_RC validateIndexRestriction( qcStatement * aStatement,
                                            idBool        aCheckKeySizeLimit,
                                            UInt          aIndexType );
};


#endif // _O_QDX_H_
