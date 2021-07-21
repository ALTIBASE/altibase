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
 * $Id: qdbAlter.h 90311 2021-03-24 09:46:45Z ahra.cho $
 **********************************************************************/
#ifndef _O_QDB_ALTER_H_
#define  _O_QDB_ALTER_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>
#include <qmx.h>
#include <qdbDef.h>
#include <qcmDictionary.h>

// qd taBle
class qdbAlter
{
public:
    // validate.
    // alter table add col
    static IDE_RC validateAddCol( qcStatement * aStatement );
    // alter table drop col
    static IDE_RC validateDropCol( qcStatement * aStatement );
    // alter table rename col
    static IDE_RC validateRenameCol( qcStatement * aStatement );
    // alter table rename
    static IDE_RC validateRenameTable( qcStatement * aStatement );
    // alter table set default
    static IDE_RC validateSetDefault( qcStatement * aStatement );
    // alter table drop default
    static IDE_RC validateDropDefault( qcStatement * aStatement );
    // alter table compact
    static IDE_RC validateCompactTable( qcStatement * aStatement );
    static IDE_RC validateAgingTable( qcStatement * aStatement );
    static IDE_RC validateNotNull(qcStatement * aStatement);
    static IDE_RC validateNullable(qcStatement * aStatement);
    static IDE_RC validateAllIndexEnable(qcStatement * aStatement);
    static IDE_RC validateAlterMaxRows(qcStatement * aStatement);
    // PROJ-1665 table option 변경
    static IDE_RC validateAlterTableOptions(qcStatement * aStatement);
    // PROJ-1362
    static IDE_RC validateAlterLobAttributes(qcStatement * aStatement);
    // PROJ-1502
    static IDE_RC validateSplitPartition(qcStatement * aStatement);
    static IDE_RC validateMergePartition(qcStatement * aStatement);
    static IDE_RC validateDropPartition(qcStatement * aStatement);
    static IDE_RC validateRenamePartition(qcStatement * aStatement);
    static IDE_RC validateTruncatePartition(qcStatement * aStatement);
    static IDE_RC validateRowmovement(qcStatement * aStatement);
    static IDE_RC validateAddPartition(qcStatement * aStatement);
    static IDE_RC validateCoalescePartition(qcStatement * aStatement);

    //PROJ-1671
    static IDE_RC validateAlterTableSegAttr(qcStatement * aStatement);
    static IDE_RC validateAlterTableSegStoAttr(qcStatement * aStatement);
    static IDE_RC validateAlterTableAllocExtent(qcStatement * aStatement);

    // PROJ-1877
    static IDE_RC validateModifyCol( qcStatement * aStatement );

    // PROJ-2264 Dictionary table
    static IDE_RC validateReorganizeCol( qcStatement * aStatement );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC validateAccessTable( qcStatement * aStatement );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC validateAccessPartition( qcStatement * aStatement );

    // BUG-47599 
    static IDE_RC validateAddRangePartition( qcStatement * aStatement );

    // validate common
    static IDE_RC validateAlterCommon( qcStatement * aStatement,
                                       idBool        aIsRepCheck );

    /* TASK-7307 DML Data Consistency in Shard */
    static IDE_RC validateUsableTable( qcStatement * aStatement );
    static IDE_RC validateUsablePartition( qcStatement * aStatement );
    static IDE_RC validateShardFlag( qcStatement * aStatement );

    // exection.
    static IDE_RC executeAddCol( qcStatement * aStatement);
    static IDE_RC executeDropCol( qcStatement * aStatement);
    static IDE_RC executeRenameCol( qcStatement * aStatement);
    static IDE_RC executeRenameTable( qcStatement * aStatement);
    static IDE_RC executeSetDefault( qcStatement * aStatement);
    static IDE_RC executeDropDefault( qcStatement * aStatement);
    static IDE_RC executeCompactTable( qcStatement * aStatement);
    static IDE_RC executeAgingTable( qcStatement * aStatement);
    static IDE_RC executeNullable(qcStatement * aStatement);
    static IDE_RC executeNotNull(qcStatement * aStatement);
    static IDE_RC executeAllIndexEnable(qcStatement * aStatement);
    static IDE_RC executeAllIndexDisable(qcStatement * aStatement);
    static IDE_RC executeAlterMaxRows(qcStatement * aStatement);
    // PROJ-1665
    static IDE_RC executeAlterTableOptions(qcStatement * aStatement);
    // PROJ-1362
    static IDE_RC executeAlterLobAttributes(qcStatement * aStatement);
    static IDE_RC executeSplitPartition(qcStatement * aStatement);
    static IDE_RC executeMergePartition(qcStatement * aStatement);
    static IDE_RC executeDropPartition(qcStatement * aStatement);
    static IDE_RC executeRenamePartition(qcStatement * aStatement);
    static IDE_RC executeTruncatePartition(qcStatement * aStatement);
    static IDE_RC executeRowmovement(qcStatement * aStatement);
    static IDE_RC executeAddPartition(qcStatement * aStatement);
    static IDE_RC executeCoalescePartition(qcStatement * aStatement);
    // PROJ-1671
    static IDE_RC executeAlterTableSegAttr(qcStatement * aStatement);
    static IDE_RC executeAlterTableSegStoAttr(qcStatement * aStatement);
    static IDE_RC executeAlterTableAllocExtent(qcStatement * aStatement);

    // PROJ-1877
    static IDE_RC executeModifyCol( qcStatement * aStatement );

    // PROJ-2264 Dictionary table
    static IDE_RC executeReorganizeCol( qcStatement * aStatement );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC executeAccessTable( qcStatement * aStatement );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC executeAccessPartition( qcStatement * aStatement );

    /* TASK-7307 DML Data Consistency in Shard */
    static IDE_RC executeUsableTable( qcStatement * aStatement );
    static IDE_RC executeUsablePartition( qcStatement * aStatement );
    static IDE_RC executeShardFlag( qcStatement * aStatement );

    // BUG-47599
    static IDE_RC executeAddRangePartition( qcStatement * aStatement );

    static IDE_RC addColInfoIntoParseTree(
        qcStatement      * aStatement,
        qcmTableInfo     * aTableInfo,
        qcmColumn        * aNewTableColumns,
        qcmColumn        * aAddedColumn,
        mtcColumn        * aMtcColumns);

    // BUG-15936
    static IDE_RC copyColInfo(qcmColumn * aNewTableColumns,
                              qcmColumn * aTableColumns);

    // PR-4360
    static IDE_RC moveRow(qcStatement      * aStatement,
                          qmsTableRef      * aTableRef,  /* add column시 사용함 */
                          qcmTableInfo     * aTableInfo,
                          const void       * aSrcTable,
                          const void       * aDstTable,
                          qcmColumn        * aSrcTblColumn,
                          qcmColumn        * aDstTblColumn,
                          qcmTableInfo     * aNewTableInfo,
                          qdIndexTableList * aNewIndexTables,
                          idBool             aIsNeedUndoLog);

    static IDE_RC deletePartIndexFromMeta(
        qcStatement    * aStatement,
        UInt             aIndexID);

    static IDE_RC findDelIndexAndDelMeta( 
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo,
        qcmColumn    * aDelColumn );

    static IDE_RC deleteIndexSpecFromMeta(
        qcStatement    * aStatement,
        UInt             aTableID);

    static IDE_RC deleteColumnSpecFromMeta(
        qcStatement    * aStatement,
        UInt             aTableID);

    /* PROJ-2422 SRID */
    static IDE_RC updateColumnSpecSRID( qcStatement * aStatement,
                                        qcmColumn   * aColumns );

    static IDE_RC updateColumnSpecDefault(
        qcStatement * aStatement,
        qcmColumn   * aColumns);

    static IDE_RC deleteColInfo(
        qcStatement      * aStatement,
        qdTableParseTree * aParseTree,
        qcmTableInfo     * aTableInfo,
        qcmColumn        * aNewTableColumns,
        mtcColumn        * aMtcColumns,
        UInt             * aDelColumnID,
        SInt               aColumnCount);

    static IDE_RC hasNullValue(qcStatement  * aStatement,
                               qcmTableInfo * aTableInfo,
                               qcmColumn    * aColumn,
                               idBool       * aHasNull);

    static IDE_RC updateColumnName(qcStatement * aStatement,
                                   qcmColumn   * aOldColumn,
                                   qcmColumn   * aNewColumn);

    /* PROJ-1107 Check Constraint 지원 */
    static IDE_RC updateCheckCondition( qcStatement * aStatement,
                                        UInt          aConstraintID,
                                        SChar       * aCheckCondition );

    static IDE_RC makeNewRow(qcTemplate        * aTemplate,
                             qcmTableInfo      * aTableInfo,
                             qcmColumn         * aSrcTblColumn,
                             qcmColumn         * aDstTblColumn,
                             const void        * aOldRow,
                             smiValue          * aNewRow,
                             smiTableCursor    * aSrcTblCursor,
                             scGRID              aRowGRID,
                             qmxLobInfo        * aLobInfo,
                             qdbConvertContext * aConvertContextList);

    // BUG-13725
    static IDE_RC checkOperatable( qcStatement  * aStatement,
                                   qcmTableInfo * aTableInfo );

    // PROJ-1362
    static IDE_RC updateLobStorageAttr( qcStatement       * aStatement,
                                        qdLobAttribute    * aLobAttr,
                                        smiColumnList     * aColumnList,
                                        qcmColumn         * aColumn,
                                        SChar             * aSqlStr,
                                        UInt                aPartID,
                                        UInt              * aUpdateCount );

    // PROJ-1502
    static IDE_RC deletePartKeyColumnSpecFromMeta(
        qcStatement    * aStatement,
        UInt             aPartObjectID,
        UInt             aPartObjectType );

    // PROJ-1502
    static IDE_RC deletePartLobSpecFromMeta(
        qcStatement    * aStatement,
        UInt             aTableID);

    // PROJ-1502
    static IDE_RC checkIndexPartAttrList(
        qcStatement             * aStatement,
        qcmTableInfo            * aTableInfo,
        qcmTableInfo            * aSrcPartInfo,
        qdPartitionAttribute    * aSrcPartAttr,
        qdPartitionAttribute    * aNewPartAttr,
        UInt                      aUserID);

    // PROJ-1502
    static IDE_RC checkAdjPartition(
        qcStatement    * aStatement,
        qcmTableInfo   * aTableInfo );

    /* BUG-46065 support range using hash */
    static IDE_RC checkAdjRangeUsingHashPartition( qcStatement    * aStatement,
                                                   qcmTableInfo   * aTableInfo );

    // TASK-2398 Log Compression
    // Table의 Flag를 변경하는 Alter구문에 대한 Validation
    static IDE_RC validateAttrFlag(qcStatement * aStatement);

    // Table의 Flag를 변경하는 Alter구문에 대한 Validation
    static IDE_RC executeAttrFlag(qcStatement * aStatement);

    // PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
    // Supplemental Logging할지 여부에 대한
    // Table의 Flag를 변경하는 Alter구문에 대한 Validation
    static IDE_RC validateAlterTableSuppLogging(qcStatement * aStatement);
    // Supplemental Logging할지 여부에 대한
    // Table의 Flag를 변경하는 Alter구문에 대한 Execution
    static IDE_RC executeAlterTableSuppLogging(qcStatement * aStatement);

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateAlterPartition( qcStatement * aStatement );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC executeAlterPartition( qcStatement * aStatement );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC validateAlterTablespace( qcStatement * aStatement );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC executeAlterTablespace( qcStatement * aStatement );

    // BUG-42594 alter table 테이블명 touch 구문 지원
    static IDE_RC validateTouchTable(qcStatement * aStatement);
    static IDE_RC executeTouchTable(qcStatement * aStatement);

    // BUG-42920 DDL print data move progress
    static IDE_RC printProgressLog4Partition( qcmTableInfo * aTableInfo,
                                              qcmTableInfo * aPartitionInfo,
                                              ULong        * aProgressCnt,
                                              idBool         aIsProgressComplete );

private:

    //-----------------------------------------
    // PROJ-1877
    // alter column modify에서 사용하는 함수
    //-----------------------------------------
    
    // memory table에 대한 modify column시 alter method 결정
    static IDE_RC decideModifyMethodForMemory(
                                 qcStatement           * aStatement,
                                 qcmColumn             * aTableColumns,
                                 qcmColumn             * aModifyColumns,
                                 qdTblColModifyMethod  * aMethod,
                                 qdVerifyColumn       ** aVerifyColumn );
    
    // disk table에 대한 modify column시 alter method 결정
    static IDE_RC decideModifyMethodForDisk(
                                 qcStatement           * aStatement,
                                 qcmColumn             * aTableColumns,
                                 qcmColumn             * aModifyColumns,
                                 qdTblColModifyMethod  * aMethod,
                                 qdVerifyColumn       ** aVerifyColumn );

    // PROJ-1911
    // disk table에 대한 modify column 시, index column의 정보 변경
    static IDE_RC modifyIndexColumnInfo( qcStatement     * aStatement,
                                         qdVerifyColumn  * aVerifyColumns,
                                         idBool            aIsTablePartition,
                                         qcmTableInfo   ** aNewTableInfo );

    // PROJ-1624 global non-partitioned index
    static IDE_RC recreateIndexColumnInfo( qcStatement       * aStatement,
                                           qcmColumn         * aModifyColumns,
                                           qcmTableInfo     ** aNewTableInfo,
                                           qdIndexTableList  * aOldIndexTables,
                                           qdIndexTableList ** aNewIndexTables,
                                           qdIndexTableList ** aDelIndexTables );
    
    // PROJ-1911
    // disk table에 대한 modify column 시,
    // modify column관련 index의 meta를 변경함으로써 index column 정보 변경
    static IDE_RC alterIndexMetaForDisk( qcStatement   * aStatement,
                                         idBool          aIsTablePartition,
                                         qcmTableInfo ** aNewTableInfo );

    // PROJ-1911
    // disk table에 대한 modify column 시,
    // modify column관련 index를 재생성 함으로써 index column 정보 변경
    static IDE_RC recreateIndexForDisk( qcStatement       * aStatement,
                                        const void        * aIndexHandle,
                                        qcmIndex          * aIndex,
                                        idBool              aIsPartitionedTable,
                                        idBool              aIsTablePartition,
                                        qcmTableInfo     ** aNewTableInfo,
                                        qdIndexTableList  * aOldIndexTables,
                                        qdIndexTableList ** aNewIndexTables,
                                        qdIndexTableList ** aDelIndexTables );

    // 검사할 column을 등록한다.
    static IDE_RC addVerifyColumn( qcStatement         * aStatement,
                                   qcmColumn           * aTableColumn,
                                   qdVerifyCommand       aCommand,
                                   UInt                  aSize,
                                   SInt                  aPrecision,
                                   SInt                  aScale,
                                   qdChangeStoredType    aChangeStoredType,
                                   qdVerifyColumn     ** aVerifyColumn );
    
    // null_only command만으로 구성되었는지 검사한다.
    static idBool isNullOnlyCommand( qdVerifyColumn * aVerifyColumn );

    // column의 value 값을 table에서 읽어 검사한다.
    static IDE_RC verifyColumnValue( qcStatement          * aStatement,
                                     qcmTableInfo         * aTableInfo,
                                     qdVerifyColumn       * aVerifyColumn,
                                     qdTblColModifyMethod * aMethod );

    // column의 value 값을 row에서 읽어 검사한다.
    static IDE_RC verifyColumnValueForRow(
                                     qcStatement          * aStatement,
                                     qdVerifyColumn       * aVerifyColumn,
                                     const void           * aRow,
                                     qdTblColModifyMethod * aMethod );

    // column의 value 값을 row에서 읽어 null인지 검사한다.
    static IDE_RC verifyColumnValueNullOnlyForRow(
                                     qdVerifyColumn       * aVerifyColumn,
                                     const void           * aRow,
                                     qdTblColModifyMethod * aMethod );

    // numeric type의 length 확대인지 검사
    static idBool isEnlargingLengthForNumericType( SInt aTablePrecision,
                                                   SInt aTableScale,
                                                   SInt aModifyPrecision,
                                                   SInt aModifyScale );

    // data loss가 발생할 수 있는 type 변환인지 검사
    static IDE_RC isDataLossConversion( UInt     aFromTypeId,
                                        UInt     aToTypeId,
                                        idBool * aIsDataLoss );

    // meta 변경만으로 alter table modify column 기능수행
    static IDE_RC alterMetaForMemory( qcStatement       * aStatement,
                                      qdTableParseTree  * aParseTree,
                                      qcmTableInfo     ** aNewTableInfo,
                                      qcmTableInfo     ** aNewPartInfo );

    // table 재생성으로 alter table modify column 기능수행
    static IDE_RC recreateTableForMemory( qcStatement       * aStatement,
                                          qdTableParseTree  * aParseTree,
                                          qcmTableInfo     ** aNewTableInfo,
                                          qcmTableInfo     ** aNewPartInfo );

    // meta 변경만으로 alter table modify column 기능수행
    static IDE_RC alterMetaForDisk( qcStatement       * aStatement,
                                    qdTableParseTree  * aParseTree,
                                    qcmTableInfo     ** aNewTableInfo,
                                    qcmTableInfo     ** aNewPartInfo );
    
    // PROJ-1877
    // table 재생성으로 alter table modify column 기능수행
    static IDE_RC recreateTableForDisk( qcStatement       * aStatement,
                                        qdTableParseTree  * aParseTree,
                                        qcmTableInfo     ** aNewTableInfo,
                                        qcmTableInfo      * aPartInfo,
                                        qcmTableInfo     ** aNewPartInfo,
                                        UInt                aNewPartIdx );

    // meta에서 not null constraint 제거
    static IDE_RC deleteNotNullConstraint( qcStatement   * aStatement,
                                           qcmTableInfo  * aTableInfo,
                                           UInt            aColID );
    
    // meta에서 not null constraint 추가
    static IDE_RC insertNotNullConstraint( qcStatement      * aStatement,
                                           qdTableParseTree * aParseTree,
                                           qcmColumn        * aColumn );

    //-----------------------------------------
    // PROJ-1877
    // alter column modify에서 memory table의 backup & restore시
    // 사용되는 callback함수
    //-----------------------------------------

    // convert context 생성
    static IDE_RC makeConvertContext( qcStatement       * aStatement,
                                      mtcColumn         * aSrcColumn,
                                      mtcColumn         * aDestColumn,
                                      qdbConvertContext * aContext );
    
    // memory table의 alter시 사용되는 callback
    static IDE_RC initializeConvert( void * aInfo );
    
    // memory table의 alter시 사용되는 callback
    static IDE_RC finalizeConvert( void * aInfo );
    
    // memory table의 alter시 사용되는 callback
    static IDE_RC convertSmiValue( idvSQL          * aStatistics,
                                   const smiColumn * aSrcColumn,
                                   const smiColumn * aDestColumn,
                                   smiValue        * aValue,
                                   void            * aInfo );

    /* PROJ-1090 Function-based Index
     *  memory table의 alter시 사용되는 callback
     */
    static IDE_RC calculateSmiValueArray( smiValue * aValueArr,
                                          void     * aInfo );

    // BUG-42920 DDL print data move progress
    static IDE_RC printProgressLog( void  * aInfo,
                                    idBool  aIsProgressComplete );
    
    // PROJ-1911 Add Column 수행 방법을
    // ( drop & create new table )로 수행할 것인지
    // column 정보만을 변경하도록 수행할 것인지 결정
    static IDE_RC decideAddColExeMethod( qcStatement      * aStatement,
                                         qdTableParseTree * aParseTree,
                                         idBool           * aRecreateTable );

    // PROJ-1911 Recreate Table 방식으로 Add Column
    static IDE_RC executeAddColByRecreateTable(
        qcStatement           * aStatement,
        qdTableParseTree      * aParseTree,
        idBool                  aIsPartitioned,
        UInt                    aPartitionCount,
        UInt                  * aPartitionID,
        UInt                    aNewColCnt,
        qcmColumn             * aNewTableColumn,
        qcmColumn            ** aNewPartitionColumn,
        smOID                 * aNewTableOID,
        smOID                ** aNewPartitionOID,
        qcmTableInfo         ** aNewTableInfo,
        qcmPartitionInfoList ** aNewPartInfoList );

    // PROJ-1911 Meta 변경 방식으로 Add Column
    static IDE_RC executeAddColByAlterMeta(
        qcStatement           * aStatement,
        qdTableParseTree      * aParseTree,
        idBool                  aIsPartitioned,
        UInt                    aPartitionCount,
        UInt                  * aPartitionID,
        UInt                    aNewColCnt,
        qcmColumn             * aNewTableColumn,
        mtcColumn             * aNewTableMtcColumn,
        qcmColumn            ** aNewPartitionColumn,
        mtcColumn            ** aNewPartitionMtcColumn,
        qcmTableInfo         ** aNewTableInfo,
        qcmPartitionInfoList ** aNewPartInfoList );

    // PROJ-1911
    // add column 수행 시, column 정보 변경하는 함수
    static IDE_RC alterColumnInfo4AddCol(
        qcStatement       * aStatement,
        qdTableParseTree  * aParseTree,
        UInt                aAddColCnt,
        idBool              aIsPartitioned,
        UInt                aPartitionCnt,
        UInt              * aPartitionID,
        qcmColumn         * aNewTableColumn,
        mtcColumn         * aNewTableMtcColumn,
        qcmColumn        ** aNewPartitionColumn,
        mtcColumn        ** aNewPartitionMtcColumn);
    
    // PROJ-1911
    // add column 수행 시, table header내의 column list 변경하는 함수
    static IDE_RC alterTableHeaderColumnList( qcStatement * aStatement,
                                              void        * aTableHandle,
                                              UInt          aTotalColCnt,
                                              mtcColumn   * aNewMtcColumn );
    
    /* PROJ-1090 Function-based Index */
    static IDE_RC renameColumnInDefaultExpression(
        qcStatement  * aStatement,
        qcmColumn    * aDefaultExpressionColumns,
        qcmColumn    * aOldColumn,
        qcmColumn    * aNewColumn );

    // PROJ-2264 Dictionary table
    static IDE_RC recreateTableForReorganize(
        qcStatement          * aStatement,
        qdTableParseTree     * aParseTree,
        smOID                * aNewTableOID,
        qcmTableInfo        ** aNewTableInfo );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC updateTableAccessOption( qcStatement     * aStatement,
                                           UInt              aTableID,
                                           qcmAccessOption   aAccessOption );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC updatePartAccessOfTablePartMeta( qcStatement      * aStatement,
                                                   UInt               aPartitionID,
                                                   qcmAccessOption    aAccessOption );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateIndexAlterationForPartition( qcStatement          * aStatement,
                                                       qcmTableInfo         * aTableInfo,
                                                       qcmTableInfo         * aPartInfo,
                                                       qdPartitionAttribute * aPartAttr );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC validateIndexAlterationForPartitioned( qcStatement  * aStatement,
                                                         qcmTableInfo * aTableInfo,
                                                         qcmColumn    * aNewTableColumn,
                                                         UInt           aNewTableType );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateTablespaceRestriction( qcmTableInfo      * aTableInfo,
                                                 smiTableSpaceType   aTBSType );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateTableReferenceForPartitions( qcStatement          * aStatement,
                                                       qcmTableInfo         * aTableInfo,
                                                       qdPartitionAttribute * aSrcAttrList,
                                                       qdPartitionAttribute * aDstAttrList );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC validateTableReferenceForPartitioned( qcStatement  * aStatement,
                                                        qcmTableInfo * aTableInfo,
                                                        UInt           aNewTableType );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC checkVolatileTableForParent( qcStatement  * aStatement,
                                               qcmTableInfo * aTableInfo,
                                               idBool       * aIsVolatile );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC checkVolatileTableForChild( qcStatement  * aStatement,
                                              qcmTableInfo * aTableInfo,
                                              idBool       * aIsVolatile );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC checkVolatileTableForPartitions( qcStatement          * aStatement,
                                                   qcmTableInfo         * aTableInfo,
                                                   qdPartitionAttribute * aSrcAttrList,
                                                   qdPartitionAttribute * aDstAttrList,
                                                   idBool               * aIsVolatileAsSelf,
                                                   idBool               * aIsVolatileAsParent );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC checkVolatileTableForPartitioned( qcStatement  * aStatement,
                                                    qcmTableInfo * aTableInfo,
                                                    UInt           aNewTableType,
                                                    idBool       * aIsVolatileAsSelf,
                                                    idBool       * aIsVolatileAsParent );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC makeIndexPartAttr( qcStatement           * aStatement,
                                     qcmTableInfo          * aTableInfo,
                                     qcmTableInfo          * aPartInfo,
                                     qdPartitionAttribute  * aDstPartAttr );
    
    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC makeIndexAttr( qcStatement                * aStatement,
                                 qcmTableInfo               * aTableInfo,
                                 qcNamePosition               aTableTBSName,
                                 qdIndexPartitionAttribute ** aUserDefinedIndexTBSAttr );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC changeIndexAttr( qcStatement               * aStatement,
                                   qdIndexPartitionAttribute * aUserDefinedIndexTBSAttr,
                                   qdIndexPartitionAttribute * aAllIndexTBSAttr );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC copyRowForAlterTablespace( qcStatement      * aStatement,
                                             qcmTableInfo     * aSrcTableInfo,
                                             qcmTableInfo     * aDstTableInfo,
                                             qdIndexTableList * aIndexTables );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC recreateDictionaryTableForAlterTablespace( qcStatement          * aStatement,
                                                             qcmTableInfo         * aTableInfo,
                                                             qcmColumn            * aNewColumns,
                                                             smiTableSpaceAttr    * aNewTBSAttr,
                                                             qcmColumn           ** aCompressedColumnList,
                                                             UInt                 * aCompressedColumnCount,
                                                             smOID               ** aDictionaryTableOIDArr,
                                                             qcmTableInfo       *** aDictionaryTableInfoArr,
                                                             qcmDictionaryTable  ** aDictionaryTable );

    /* BUG-44230 ADD, DROP, MODIFY, REORGANIZE DDL의 INDEX, CONSTRAINT 생성 순서 변경 */
    static IDE_RC recreateIndexForReorganize( qcStatement       * aStatement,
                                              qdTableParseTree  * aParseTree,
                                              smOID               aNewTableOID,
                                              qcmTableInfo     ** aNewTableInfo );

    /* TASK-7307 DML Data Consistency in Shard */
    static IDE_RC updateTableUsable( qcStatement          * aStatement,
                                     UInt                   aTableID,
                                     idBool                 aIsUsable );

    /* TASK-7307 DML Data Consistency in Shard */
    static IDE_RC updatePartitionUsable( qcStatement          * aStatement,
                                         idBool                 aAll,
                                         UInt                   aObjectID,
                                         idBool                 aIsUsable );

    /* TASK-7307 DML Data Consistency in Shard */
    static IDE_RC updateTableShardFlag( qcStatement   * aStatement,
                                        UInt            aTableID,
                                        UInt            aShardFlag );
};


#endif // _O_QDB_ALTER_H_
