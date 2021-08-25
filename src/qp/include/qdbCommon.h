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
 * $Id: qdbCommon.h 91517 2021-08-24 01:25:47Z bethy $
 **********************************************************************/
#ifndef  _O_QDB_COMMON_H_
#define  _O_QDB_COMMON_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>
#include <qrc.h>

//PROJ-1557
#define QDB_COLUMN_MAX_IN_ROW_SIZE            (4000)

// PROJ-1502 PARTITIONED DISK TABLE
#define QDB_NO_PARTITION_ORDER                (ID_UINT_MAX)

#define QDB_SET_QCM_COLUMN(_dest_, _src_)       \
    {                                           \
        _dest_->basicInfo = _src_->basicInfo;   \
        _dest_->flag      = _src_->flag;        \
    }

/* createTableOnSM 인터페이스에 aInitFlagMask에
   모든 Bit가 1로 세팅된 MASK를 넘길 경우 사용
*/
#define QDB_TABLE_ATTR_MASK_ALL (0xFFFFFFFF)

/* PROJ-2465 Tablespace Alteration for Table */
enum qdbUsageLevel
{
    QDB_USAGE_LEVEL_INIT   = 0, /*  0% */
    QDB_USAGE_LEVEL_FIRST  = 1, /*  1% */
    QDB_USAGE_LEVEL_SECOND = 3, /*  3% */
    QDB_USAGE_LEVEL_THIRD  = 6, /*  6% */
    QDB_USAGE_LEVEL_MAX    = 10 /* 10% */
};

/* PROJ-2465 Tablespace Alteration for Table */
typedef struct qdbMemUsage
{
    ULong mMaxSize;
    ULong mUsedSize;
} qdbMemUsage;

/* PROJ-2465 Tablespace Alteration for Table */
typedef struct qdbTBSUsage
{
    scSpaceID     mTBSID;
    ULong         mMaxSize;
    ULong         mUsedSize;
    qdbTBSUsage * mNext;
} qdbTBSUsage;

/* PROJ-2465 Tablespace Alteration for Table */
typedef struct qdbAnalyzeUsage
{
    /* Rate */
    UInt          mWorkRate;
    /* Property */
    UInt          mMemUsageThreshold;
    UInt          mTBSUsageThreshold;
    UInt          mUsageMinRowCount;
    /* Progress */
    SLong         mTotalRowCount;
    SLong         mRemainRowCount;
    SLong         mRoundRowCount;
    SLong         mCurrentRowCount;
    /* Media Usage */
    qdbMemUsage * mMem;
    /* TBS Usage */
    qdbTBSUsage * mTBS;
} qdbAnalyzeUsage;

/* PROJ-2465 Tablespace Alteration for Table */
#define QDB_INIT_ANALYZE_USAGE( _usage_ )                               \
    {                                                                   \
        ( _usage_ )->mWorkRate          = QDB_USAGE_LEVEL_INIT;         \
        ( _usage_ )->mMemUsageThreshold = QCU_DDL_MEM_USAGE_THRESHOLD;  \
        ( _usage_ )->mTBSUsageThreshold = QCU_DDL_TBS_USAGE_THRESHOLD;  \
        ( _usage_ )->mUsageMinRowCount  = QCU_ANALYZE_USAGE_MIN_ROWCOUNT; \
        ( _usage_ )->mTotalRowCount     = ID_SLONG_MAX;                 \
        ( _usage_ )->mRemainRowCount    = ID_SLONG_MAX;                 \
        ( _usage_ )->mRoundRowCount     = 1;                            \
        ( _usage_ )->mCurrentRowCount   = 1;                            \
        ( _usage_ )->mMem               = NULL;                         \
        ( _usage_ )->mTBS               = NULL;                         \
    }

/* PROJ-2465 Tablespace Alteration for Table */
#define QDB_INIT_MEM_USAGE( _usage_ )      \
    {                                           \
        ( _usage_ )->mMaxSize  = ID_ULONG_MAX;  \
        ( _usage_ )->mUsedSize = ID_ULONG_MAX;  \
    }

/* PROJ-2465 Tablespace Alteration for Table */
#define QDB_INIT_TBS_USAGE( _usage_ )           \
    {                                           \
        ( _usage_ )->mTBSID    = ID_USHORT_MAX; \
        ( _usage_ )->mMaxSize  = ID_ULONG_MAX;  \
        ( _usage_ )->mUsedSize = ID_ULONG_MAX;  \
        ( _usage_ )->mNext     = NULL;          \
    }

class qdbCommon
{
public:
    static IDE_RC createTableOnSM(
        qcStatement     * aStatement,
        qcmColumn       * aColumns,
        UInt              aUserID,
        UInt              aTableID,
        ULong             aMaxRows,
        scSpaceID         aTBSID,
        smiSegAttr        aSegAttr,
        smiSegStorageAttr aSegStoAttr,
        UInt              aInitFlagMask,
        UInt              aInitFlagValue,
        UInt              aParallelDegree,
        smOID           * aTableOID,            // out
        SChar           * aStmtText = NULL,
        UInt              aStmtTextLen = 0 );

    static IDE_RC makeSmiColumnList(
        qcStatement     * aStatement,
        scSpaceID         aTBSID,
        UInt              aTableID,
        qcmColumn       * aColumns,
        smiColumnList  ** aSmiColumnList );

    static IDE_RC makeMemoryTableNullRow(
        qcStatement     * aStatement,
        qcmColumn       * aColumns,
        smiValue       ** aNullRow);

    static IDE_RC updateTableSpecFromMeta(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aTableName,
        UInt              aTableID,
        smOID             aTableOID,
        SInt              aColumnCount,
        UInt              aParallelDegree );

    static IDE_RC createConstraintFromInfo(
        qcStatement       * aStatement,
        qcmTableInfo      * aTableInfo,
        smOID               aNewTableOID,
        UInt                aPartitionCount,
        smOID             * aNewPartitionOID,
        UInt                aIndexCrtFlag,
        qcmIndex          * aNewTableIndex,
        qcmIndex         ** aNewPartIndex,
        UInt                aNewPartIndexCount,
        qdIndexTableList  * aOldIndexTables,
        qdIndexTableList ** aNewIndexTables,
        qcmColumn         * aDelColList );

    static IDE_RC createIndexFromInfo(
        qcStatement       * aStatement,
        qcmTableInfo      * aTableInfo,
        smOID               aNewTableOID,
        UInt                aPartitionCount,
        smOID             * aNewPartitionOID,
        UInt                aIndexCrtFlag,
        qcmIndex          * aIndices,
        qcmIndex         ** aPartIndices,
        UInt                aPartIndexCount,
        qdIndexTableList  * aOldIndexTables,
        qdIndexTableList ** aNewIndexTables,
        qcmColumn         * aDelColList,
        idBool              aCreateMetaFlag );

    static IDE_RC makeColumnNullable(
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo,
        UInt           aColID);

    static IDE_RC convertDefaultValueType( qcStatement * aStatement,
                                           mtcId       * aType,
                                           qtcNode     * aDefault,
                                           idBool      * aIsNull );

    static IDE_RC calculateDefaultValueWithSequence( qcStatement  * aStatement,
                                                     qcmTableInfo * aTableInfo,
                                                     qcmColumn    * aSrcColumns,
                                                     qcmColumn    * aDestColumns,
                                                     smiValue     * aNewRow );

    static IDE_RC makeColumnNotNull(
        qcStatement            * aStatement,
        const void             * aTableHandle,
        ULong                    aMaxRows,
        qcmPartitionInfoList   * aPartInfoList,
        idBool                   aIsPartitioned,
        UInt                     aColID);

    /* PROJ-2422 SRID */
    static IDE_RC makeColumnSRID( qcStatement            * aStatement,
                                  const void             * aTableHandle,
                                  ULong                    aMaxRows,
                                  qcmPartitionInfoList   * aPartInfoList,
                                  idBool                   aIsPartitioned,
                                  UInt                     aColID,
                                  SInt                     aSRID );

    static IDE_RC makeColumnNewType(
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo,
        qcmColumn    * aColumn );

    static IDE_RC validateColumnListForCreate(
        qcStatement     * aStatement,
        qcmColumn       * aColumn,
        idBool            aIsTable );
    
    static IDE_RC validateColumnListForCreateInternalTable(
        qcStatement     * aStatement,
        idBool            aInExecutionTime,
        UInt              aTableType,
        scSpaceID         aTBSID,
        qcmColumn       * aColumn);
    
    static IDE_RC validateColumnListForAddCol(
        qcStatement     * aStatement,
        qcmTableInfo    * aTableInfo,
        qcmColumn       * aColumn,
        SInt              aColumnCount);
    
    static IDE_RC validateColumnListForModifyCol(
        qcStatement     * aStatement,
        qcmTableInfo    * aTableInfo,
        qcmColumn       * aColumn);
    
    static IDE_RC validateLobAttributeList(
        qcStatement           * aStatement,
        qcmTableInfo          * aTableInfo,
        qcmColumn             * aColumn,
        smiTableSpaceAttr     * aTBSAttr,
        qdLobAttribute        * aLobAttr );

    static IDE_RC validateDefaultDefinition(
        qcStatement     * aStatement,
        qtcNode         * aNode);

    /* PROJ-1090 Function-based Index */
    static IDE_RC validateDefaultExprDefinition(
        qcStatement     * aStatement,
        qtcNode         * aNode,
        qmsSFWGH        * aSFWGH,
        qmsFrom         * aFrom );
    
    /* PROJ-1107 Check Constraint 지원 */
    static IDE_RC validateCheckConstrDefinition(
        qcStatement      * aStatement,
        qdConstraintSpec * aCheckConstr,
        qmsSFWGH         * aSFWGH,
        qmsFrom          * aFrom );

    static IDE_RC getMtcColumnFromTarget(
        qcStatement       * aStatement,
        qtcNode           * aTarget,
        qcmColumn         * aColumn,
        scSpaceID           aTablespaceID);

    static IDE_RC insertTableSpecIntoMeta(
        qcStatement        * aStatement,
        idBool               aIsPartitionedTable,
        UInt                 aCreateFlag,
        qcNamePosition       aTableNamePos,
        UInt                 aUserID,
        smOID                aTableOID,
        UInt                 aTableID, // out
        UInt                 aColumnCount,
        ULong                aMaxRows,
        qcmAccessOption      aAccessOption, /* PROJ-2359 Table/Partition Access Option */
        scSpaceID            aTBSID,
        smiSegAttr           aSegAttr,
        smiSegStorageAttr    aSegStoAttr,
        qcmTemporaryType     aTemporaryType,
        UInt                 aParallelDegree,
        UInt                 aShardFlag );

    // PROJ-2264 Dictionary table
    static IDE_RC insertCompressionTableSpecIntoMeta(
        qcStatement     * aStatement,
        UInt              aTableID,
        UInt              aColumnID,
        UInt              aDicTableID,
        ULong             aMaxRows );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC updatePartTableSpecFromMeta(
        qcStatement     * aStatement,
        UInt              aPartTableID,
        UInt              aPartitionID,
        smOID             aPartitionOID );

    static IDE_RC insertPartTableSpecIntoMeta(
        qcStatement        * aStatement,
        UInt                 aUserID,
        UInt                 aTableID,
        UInt                 aPartMethod,
        UInt                 aPartKeyCount,
        idBool               aIsRowmovement );

    static IDE_RC insertTablePartitionSpecIntoMeta(
        qcStatement         * aStatement,
        UInt                  aUserID,
        UInt                  aTableID,
        smOID                 aPartOID,
        UInt                  aPartID,
        qcNamePosition        aPartNamePos,
        SChar               * aPartMinValue,
        SChar               * aPartMaxValue,
        UInt                  aPartOrder,
        scSpaceID             aTBSID,
        qcmAccessOption       aAccessOption,  /* PROJ-2359 Table/Partition Access Option */
        idBool                aIsUsable = ID_TRUE );

    static IDE_RC insertPartLobSpecIntoMeta(
        qcStatement          * aStatement,
        UInt                   aUserID,
        UInt                   aTableID,
        UInt                   aPartID,
        qcmColumn            * aColumn);

    static IDE_RC insertColumnSpecIntoMeta(
        qcStatement     * aStatement,
        UInt              aUserID,
        UInt              aTableID,
        qcmColumn       * aColumns,
        idBool            aIsQueue);

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC insertPartKeyColumnSpecIntoMeta(
        qcStatement     * aStatement,
        UInt              aUserID,
        UInt              aTableID,
        qcmColumn       * aTableColumns,
        qcmColumn       * aPartKeyColumns,
        UInt              aObjectType);

    static IDE_RC createConstrNotNull(
        qcStatement      * aStatement,
        qdConstraintSpec * aConstr,
        UInt               aUserID,
        UInt               aTableID);

    /* PROJ-1107 Check Constraint 지원 */
    static IDE_RC createConstrCheck(
        qcStatement        * aStatement,
        qdConstraintSpec   * aConstr,
        UInt                 aUserID,
        UInt                 aTableID,
        qdFunctionNameList * aRelatedFunctionNames );

    static IDE_RC createConstrTimeStamp(
        qcStatement      * aStatement,
        qdConstraintSpec * aConstr,
        UInt               aUserID,
        UInt               aTableID);

    static IDE_RC createConstrPrimaryUnique(
        qcStatement            * aStatement,
        smOID                    aTableOID,
        qdConstraintSpec       * aConstr,
        UInt                     aUserID,
        UInt                     aTableID,
        qcmPartitionInfoList   * aPartInfoList,
        ULong                    aMaxRows );

    static IDE_RC createConstrForeign(
        qcStatement      * aStatement,
        qdConstraintSpec * aConstr,
        UInt               aUserID,
        UInt               aTableID);

    /* PROJ-1090 Function-based Index */
    static IDE_RC createIndexFromIndexParseTree(
        qcStatement             * aStatement,
        qdIndexParseTree        * aIndexParseTree,
        smOID                     aTableOID,
        qcmTableInfo            * aTableInfo,
        UInt                      aTableID,
        qcmPartitionInfoList    * aPartInfoList );
    
    static IDE_RC updateColumnSpecNull(
        qcStatement * aStatement,
        qcmColumn   * aColumn,
        idBool        aNullableFlag);

    static IDE_RC getStrForMeta(
        qcStatement  * aStatement,
        SChar        * aStr,
        SInt           aSize,
        SChar       ** aStrForMeta);

    static IDE_RC allocSmiColList(
        qcStatement    * aStatement,
        const void     * aTableHandle,
        smiColumnList ** aColList);

    static IDE_RC checkTableInfo(
        qcStatement      * aStatement,
        qcNamePosition     aUserName,
        qcNamePosition     aTableName,
        UInt             * aUserID,
        qcmTableInfo    ** aTableInfo,
        void            ** aTableHandle,
        smSCN            * aTableSCN);

    static IDE_RC checkDuplicatedObject(
        qcStatement      * aStatement,
        qcNamePosition     aUserName,
        qcNamePosition     aObjectName,
        UInt             * aUserID );

    /* BUG-13528
       static IDE_RC validateKeySizeLimit(
       qcStatement      * aStatement,
       qcmTableInfo     * aTableInfo,
       qdConstraintSpec * aConstraints);
    */

    static IDE_RC getDiskRowSize( const void       * aTableHandle,
                                  UInt             * aRowSize );

    static IDE_RC getDiskRowSize( qcmTableInfo     * aTableInfo,
                                  UInt             * aRowSize );

    static IDE_RC getDiskRowSize( qcmColumn        * aTableColumn,
                                  UInt             * aRowSize );

    static IDE_RC getMemoryRowSize( qcmTableInfo     * aTableInfo,
                                    UInt             * aFixRowSize,
                                    UInt             * aRowSize );

    static IDE_RC setColListOffset( iduMemory    * aMem,
                                    qcmColumn    * aColumns,
                                    UInt           aCurrentOffset );

    //fix PROJ-1596
    static IDE_RC setColListOffset( iduVarMemList    * aMem,
                                    qcmColumn        * aColumns,
                                    UInt               aCurrentOffset );

    static idBool containDollarInName( qcNamePosition * aObjectNamePos );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC validatePartKeyCondValues( qcStatement        * aStatement,
                                             qdPartitionedTable * aPartTable );

    static IDE_RC checkDupRangePartKeyValues( qcStatement        * aStatement,
                                              qdPartitionedTable * aPartTable );

    static IDE_RC checkDupListPartKeyValues( qcStatement        * aStatement,
                                             qdPartitionedTable * aPartTable );

    static IDE_RC sortPartition( qcStatement        * aStatement,
                                 qdPartitionedTable * aPartTable );

    static IDE_RC makePartKeyCondValues( qcStatement          * aStatement,
                                         qcTemplate           * aTemplate,
                                         qcmColumn            * aPartKeyColumns,
                                         qcmPartitionMethod     aPartMethod,
                                         qmmValueNode         * aPartKeyCond,
                                         qmsPartCondValList   * aPartCondVal );

    static IDE_RC validatePartitionedTable( qcStatement * aStatement,
                                            idBool        aIsCreateTable );

    static IDE_RC validatePartKeyColList( qcStatement * aStatement );

    static IDE_RC validateTBSOfPartition( qcStatement * aStatement,
                                          qdPartitionAttribute * aPartAttr );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC validateTBSOfTable( qcStatement * aStatement );

    static IDE_RC getPartitionMinMaxValue( qcStatement          * aStatement,
                                           qdPartitionAttribute * aPartAttr,
                                           UInt                   aPartCount,
                                           qcmPartitionMethod     aPartMethod,
                                           SChar                * aPartMaxVal,
                                           SChar                * aPartMinVal,
                                           SChar                * aOldPartMaxVal );

    static IDE_RC addSingleQuote4PartValue( SChar     * aStr,
                                            SInt        aSize,
                                            SChar    ** aPartMaxVal );

    static IDE_RC checkSplitCond( qcStatement       * aStatement,
                                  qdTableParseTree  * aParseTree );

    static IDE_RC createTablePartition( qcStatement           * aStatement,
                                        qdTableParseTree      * aParseTree,
                                        qcmTableInfo          * aTableInfo,
                                        qdPartitionAttribute  * aPartAttr,
                                        SChar                 * aPartMinVal,
                                        SChar                 * aPartMaxVal,
                                        UInt                    aPartOrder,
                                        UInt                  * aPartitionID,
                                        smOID                 * aPartitionOID );

    static IDE_RC getPartCondValueFromParseTree(
        qcStatement          * aStatement,
        qdPartitionAttribute * aPartAttr,
        SChar                * aPartVal );

    static IDE_RC updatePartMinValueOfTablePartMeta(
        qcStatement          * aStatement,
        UInt                   aPartitionID,
        SChar                * aPartMinValue );

    static IDE_RC updatePartMaxValueOfTablePartMeta(
        qcStatement          * aStatement,
        UInt                   aPartitionID,
        SChar                * aPartMaxValue );

    static IDE_RC moveRowForInplaceAlterPartition(
        qcStatement          * aStatement,
        void                 * aSrcTable,
        void                 * aDstTable,
        qcmTableInfo         * aSrcPart,
        qcmTableInfo         * aDstPart,
        qdIndexTableList     * aIndexTables,
        qdSplitMergeType       aSplitType );

    static IDE_RC moveRowForOutplaceMergePartition(
        qcStatement          * aStatement,
        void                 * aSrcTable1,
        void                 * aSrcTable2,
        void                 * aDstTable,
        qcmTableInfo         * aSrcPart1,
        qcmTableInfo         * aSrcPart2,
        qcmTableInfo         * aDstPart,
        qdIndexTableList     * aIndexTables );

    static IDE_RC moveRowForOutplaceSplitPartition(
        qcStatement          * aStatement,
        void                 * aSrcTable,
        void                 * aDstTable1,
        void                 * aDstTable2,
        qcmTableInfo         * aSrcPart,
        qcmTableInfo         * aDstPart1,
        qcmTableInfo         * aDstPart2,
        qdIndexTableList     * aIndexTables,
        qdSplitMergeType       aSplitType );

    static IDE_RC isMoveRowForAlterPartition(
        qcStatement          * aStatement,
        qcmTableInfo         * aPartInfo,
        void                 * aRow,
        qcmColumn            * aColumns,
        idBool               * aIsMoveRow,
        qdSplitMergeType       aType );

    static void excludeSplitValFromPartVal(
        qcmTableInfo         * aTableInfo,
        qdAlterPartition     * aAlterPart,
        SChar                * aPartVal );

    static void mergePartCondVal(
        qcStatement          * aStatement,
        SChar                * aPartVal );

    static IDE_RC checkSplitCondOfDefaultPartition(
        qcStatement          * aStatement,
        qcmTableInfo         * aTableInfo,
        qmsPartCondValList   * aSplitCondVal );

    static IDE_RC makePartCondValList(
        qcStatement          * aStatement,
        qcmTableInfo         * aTableInfo,
        UInt                   aPartitionID,
        qdPartitionAttribute * aPartAttr );

    static IDE_RC checkPartitionInfo(
        qcStatement          * aStatement,
        qcmTableInfo         * aTableInfo,
        qcNamePosition         aPartName );

    static IDE_RC checkPartitionInfo(
        qcStatement           * aStatement,
        UInt                    aPartID );

    static IDE_RC checkAndSetAllPartitionInfo(
        qcStatement           * aStatement,
        UInt                    aTableID,
        qcmPartitionInfoList ** aPartInfoList);

    static IDE_RC updatePartNameOfTablePartMeta(
        qcStatement          * aStatement,
        UInt                   aPartitionID,
        SChar                * aPartName );

    static IDE_RC reorganizeForHashPartition(
        qcStatement          * aStatement,
        qcmTableInfo         * aTableInfo,
        qcmPartitionInfoList * aSrcPartInfoList,
        UInt                   aDstPartCount,
        smOID                * aDstPartOID,
        qcmIndex             * aNewIndices,
        qdIndexTableList     * aNewIndexTables );

    static IDE_RC checkMoveRowPartitionByHash(
        void                 * aOldRow,
        qcmTableInfo         * aPartInfo,
        qcmColumn            * aColumns,
        UInt                   aPartCount,
        UInt                 * aDstPartNum );

    // 여러 개의 Attribute Flag List의 Flag값을
    // Bitwise Or연산 하여 하나의 UInt 형의 Flag 값을 만든다
    static IDE_RC getTableAttrFlagFromList(qdTableAttrFlagList * aAttrFlagList,
                                           UInt              * aAttrFlagMask,
                                           UInt              * aAttrFlagValue );


    // Table의 Attribute Flag List에 대한 Validation수행
    static IDE_RC validateTableAttrFlagList(
                      qcStatement         * aStatement,
                      qdTableAttrFlagList * aAttrFlagList);

    // disk index key column에 variable 컬럼속성 지정
    static IDE_RC setIndexKeyColumnTypeFlag( mtcColumn * aKeyColumn );

    // sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
    static IDE_RC makeFetchColumnList4TupleID(
                      qcTemplate             * aTemplate,
                      UShort                   aTupleRowID,
                      idBool                   aIsNeedAllFetchColumn,
                      qcmIndex               * aIndex,
                      idBool                   aIsAllocSmiColumnList,
                      smiFetchColumnList    ** aFetchColumnList );

    // sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
    static IDE_RC makeFetchColumnList4Index(
                      qcTemplate             * aTemplate,
                      qcmTableInfo           * aTableInfo,
                      qcmIndex               * aIndex,
                      idBool                   aIsAllocSmiColumnList,
                      smiFetchColumnList    ** aFetchColumnList );

    // sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
    static IDE_RC makeFetchColumnList4ChildTable(
                      qcTemplate             * aTemplate,
                      qcmTableInfo           * aTableInfo,
                      qcmForeignKey          * aForeignKey,
                      qcmIndex               * aIndex,
                      idBool                   aIsAllocFetchColumnList,
                      smiFetchColumnList    ** aFetchColumnList );

    // sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
    static IDE_RC makeFetchColumnList(
                      qcTemplate             * aTemplate,
                      UInt                     aColumnCount,
                      qcmColumn              * aColumn,
                      idBool                   aIsAllocSmiColumnList,
                      smiFetchColumnList    ** aFetchColumnList );

    // fetch column list 초기화
    static void initFetchColumnList( smiFetchColumnList  ** aFetchColumnList );

    // fetch column list에 fetch column 추가
    static IDE_RC addFetchColumnList( iduMemory            * aMemory,
                                      mtcColumn            * aColumn,
                                      smiFetchColumnList  ** aFetchColumnList );

    /* PROJ-1107 Check Constraint 지원 */
    static IDE_RC addCheckConstrListToFetchColumnList(
                      iduMemory           * aMemory,
                      qdConstraintSpec    * aCheckConstrList,
                      qcmColumn           * aColumnArray,    /* PROJ-2464 hybrid partitioned table 지원 */
                      smiFetchColumnList ** aFetchColumnList );

    // PROJ-1579 NCHAR
    static IDE_RC convertToUTypeString(
                qcStatement   * aStatement,
                UInt            aSrcValOffset,
                UInt            aSrcLen,
                qcNamePosList * aNcharList,
                SChar         * aDest,
                UInt            aBufferSize );

    // PROJ-1579 NCHAR
    static IDE_RC makeNcharLiteralStr(
                qcStatement     * aStatement,
                qcNamePosList   * aNcharList,
                qcmColumn       * aColumn );

    // PROJ-1579 NCHAR
    static IDE_RC makeNcharLiteralStr(
                qcStatement          * aStatement,
                qcNamePosList        * aNcharList,
                qdPartitionAttribute * aPartAttr );

    /* PROJ-1090 Function-based Index */
    static IDE_RC makeNcharLiteralStrForIndex(
                qcStatement     * aStatement,
                qcNamePosList   * aNcharList,
                qcmColumn       * aColumn );

    /* PROJ-1107 Check Constraint 지원 */
    static IDE_RC makeNcharLiteralStrForConstraint(
                qcStatement      * aStatement,
                qcNamePosList    * aNcharList,
                qdConstraintSpec * aConstr );

    /* PROJ-1107 Check Constraint 지원 */
    static void removeNcharLiteralStr(
                qcNamePosList ** aFromList,
                qcNamePosList  * aTargetList );

    // PROJ-1705
    // mtdDataType의 Value로부터 smiValue.length 의 정보를 구한다.
    static IDE_RC storingSize( mtcColumn  * aStoringColumn,
                               mtcColumn  * aValueColumn,
                               void       * aValue,
                               UInt       * aOutStoringSize );

    // PROJ-1705
    // mtdDataType의 Value로부터 smiValue.value 의 정보를 구한다.
    static IDE_RC mtdValue2StoringValue( mtcColumn  * aStoringColumn,
                                         mtcColumn  * aValueColumn,
                                         void       * aValue,
                                         void      ** aOutStoringValue );

    // PROJ-1705
    // smiValue.value로부터 mtdDataType의 Value 정보를 구한다.
    static IDE_RC storingValue2MtdValue( mtcColumn  * aColumn,
                                         void       * aValue,
                                         void      ** aOutMtdValue );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC adjustSmiValueToDisk( mtcColumn * aFromColumn,
                                        smiValue  * aFromValue,
                                        mtcColumn * aToColumn,
                                        smiValue  * aToValue );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC adjustSmiValueToMemory( mtcColumn * aFromColumn,
                                          smiValue  * aFromValue,
                                          mtcColumn * aToColumn,
                                          smiValue  * aToValue );

    // PROJ-1877 alter table modify column
    static idBool findColumnIDInColumnList( qcmColumn * aColumns,
                                            UInt        aColumnID );

    static qcmColumn * findColumnInColumnList( qcmColumn * aColumns,
                                               UInt        aColumnID );

    // PROJ-1784 DML Without Retry
    static IDE_RC makeWhereClauseColumnList( qcStatement     * aStatement,
                                             UShort            aTupleRowID,
                                             smiColumnList  ** aFetchColumnList );
    
    // PROJ-1784 DML Without Retry
    static IDE_RC makeSetClauseColumnList( qcStatement     * aStatement,
                                           UShort            aTupleRowID,
                                           smiColumnList  ** aFetchColumnList );

    // PROJ-2429 Dictionary based data compress for on-disk DB
    // Dictionary compression column인지 확인하고 dictionary column copy함수를
    // 전달한다. Dictionary compression column이 아닌경우 data type에 맞는 함수를
    // 전달 한다.
    static void * getCopyDiskColumnFunc( mtcColumn * aColumn )
    {
        UInt sFunctionIdx;

        if ((aColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK) 
            != SMI_COLUMN_COMPRESSION_TRUE)
        {
            sFunctionIdx = MTD_COLUMN_COPY_FUNC_NORMAL;
        }
        else
        {
            sFunctionIdx = MTD_COLUMN_COPY_FUNC_COMPRESSION;
        }

        IDE_DASSERT( aColumn->module->storedValue2MtdValue[sFunctionIdx] != NULL );

        return (void *) aColumn->module->storedValue2MtdValue[sFunctionIdx];
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    static UInt getTableTypeFromTBSID( scSpaceID           aTBSID );

    /* PROJ-2465 Tablespace Alteration for Table */
    static UInt getTableTypeFromTBSType( smiTableSpaceType aTBSType );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void getTableTypeCountInPartInfoList( UInt                 * aTableType,
                                                 qcmPartitionInfoList * aPartInfoList,
                                                 SInt                 * aCountDiskType,
                                                 SInt                 * aCountMemType,
                                                 SInt                 * aCountVolType );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void getTableTypeCountInPartAttrList( UInt                 * aTableType,
                                                 qdPartitionAttribute * aPartAttrList,
                                                 SInt                 * aCountDiskType,
                                                 SInt                 * aCountMemType,
                                                 SInt                 * aCountVolType );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateConstraintRestriction( qcStatement        * aStatement,
                                                 qdTableParseTree   * aParseTree );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateIndexKeySize( qcStatement          * aStatement,
                                        UInt                   aTableType,
                                        qcmColumn            * aKeyColumns,
                                        UInt                   aKeyColCount,
                                        UInt                   aIndexType,
                                        qcmPartitionInfoList * aPartInfoList,
                                        qdPartitionAttribute * aPartAttrList,
                                        idBool                 aIsPartitioned,
                                        idBool                 aIsIndex );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateAndSetDirectKey( mtcColumn * aBasicInfo,
                                           idBool      aIsUserTable,
                                           SInt        aCountDiskType,
                                           SInt        aCountMemType,
                                           SInt        aCountVolType,
                                           ULong     * aDirectKeyMaxSize,
                                           UInt      * aSetFlag );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateAndSetPersistent( SInt     aCountDiskType,
                                            SInt     aCountVolType,
                                            idBool * aIsPers,
                                            UInt   * aSetFlag );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validatePhysicalAttr( qdTableParseTree * aParseTree );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateAndSetSegAttr( UInt         aTableType,
                                         smiSegAttr * aSrcSegAttr,
                                         smiSegAttr * aDstSegAttr,
                                         idBool       aIsTable );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateAndSetSegStoAttr( UInt                aTableType,
                                            smiSegStorageAttr * aSrcStoAttr,
                                            smiSegStorageAttr * aDstStoAttr,
                                            qdSegStoAttrExist * aExist,
                                            idBool              aIsTable );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void adjustIndexAttr( scSpaceID           aTableTBSID,
                                 smiSegAttr          aSrcSegAttr,
                                 smiSegStorageAttr   aSrcSegStoAttr,
                                 UInt                aSrcIndexFlag,
                                 ULong               aSrcDirectKeyMaxSize,
                                 smiSegAttr        * aDstSegAttr,
                                 smiSegStorageAttr * aDstSegStoAttr,
                                 UInt              * aDstIndexFlag,
                                 ULong             * aDstDirectKeyMaxSize );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void adjustPhysicalAttr( UInt                aTableType,
                                    smiSegAttr          aSrcSegAttr,
                                    smiSegStorageAttr   aSrcSegStoAttr,
                                    smiSegAttr        * aDstSegAttr,
                                    smiSegStorageAttr * aDstSegStoAttr,
                                    idBool              aIsTable );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void adjustSegAttr( UInt         aTableType,
                               smiSegAttr   aSrcSegAttr,
                               smiSegAttr * aDstSegAttr,
                               idBool       aIsTable );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void adjustSegStoAttr( UInt                aTableType,
                                  smiSegStorageAttr   aSrcSegStoAttr,
                                  smiSegStorageAttr * aDstSegStoAttr );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void adjustDirectKeyMaxSize( UInt    aSrcIndexFlag,
                                        ULong   aSrcDirectKeyMaxSize,
                                        UInt  * aDstIndexFlag,
                                        ULong * aDstDirectKeyMaxSize );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC copyAndAdjustColumnList( qcStatement        * aStatement,
                                           smiTableSpaceType    aOldTBSType,
                                           smiTableSpaceType    aNewTBSType,
                                           qcmColumn          * aOldColumn,
                                           qcmColumn         ** aNewColumn,
                                           UInt                 aColumnCount,
                                           idBool               aEnableVariableColumn );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC adjustIndexColumn( qcmColumn     * aColumn,
                                     qcmIndex      * aIndex,
                                     qcmColumn     * aDelColList,
                                     smiColumnList * aIndexColumnList );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC adjustKeyColumn( qcmColumn * aTableColumn,
                                   qcmColumn * aKeyColumn );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void makeTempQcmColumnListFromIndex( qcmIndex  * aIndex,
                                                mtcColumn * aMtcColumnArr,
                                                qcmColumn * aQcmColumnList );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC updatePartTableTBSFromMeta( qcStatement * aStatement,
                                              UInt          aPartTableID,
                                              UInt          aPartitionID,
                                              scSpaceID     aTBSID );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC updatePartLobsTBSFromMeta( qcStatement * aStatement,
                                             UInt          aUserID,
                                             UInt          aPartTableID,
                                             UInt          aPartitionID,
                                             UInt          aLobColumnID,
                                             scSpaceID     aTBSID );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC updateTableTBSFromMeta( qcStatement       * aStatement,
                                          UInt                aTableID,
                                          smOID               aTableOID,
                                          scSpaceID           aTBSID,
                                          SChar             * aTBSName,
                                          smiSegAttr          aSegAttr,
                                          smiSegStorageAttr   aSegStoAttr );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC updateColumnFlagFromMeta( qcStatement * aStatement,
                                            qcmColumn   * aColumns );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC updateLobTBSFromMeta( qcStatement * aStatement,
                                        qcmColumn   * aColumns );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC updateIndexTBSFromMeta( qcStatement               * aStatement,
                                          qcmIndex                  * aIndices,
                                          UInt                        aIndexCount,
                                          qdIndexPartitionAttribute * aAllIndexTBSAttr );

    /* BUG-42321  The smi cursor type of smistatement is set wrongly in
     * partitioned table
     */
    static IDE_RC checkForeignKeyParentTableInfo( qcStatement  * aStatement,
                                                  qcmTableInfo * aTableInfo,
                                                  SChar        * aConstraintName,
                                                  idBool         aIsValidate );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC initializeAnalyzeUsage( qcStatement      * aStatement,
                                          qcmTableInfo     * aSrcTableInfo,
                                          qcmTableInfo     * aDstTableInfo,
                                          qdbAnalyzeUsage ** aUsage );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC checkAndSetAnalyzeUsage( qcStatement     * aStatement,
                                           qdbAnalyzeUsage * aUsage );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static void setAllColumnPolicy( qcmTableInfo * aTableInfo );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static void unsetAllColumnPolicy( qcmTableInfo * aTableInfo );

    /* BUG-47599 */
    static IDE_RC checkEmptyPartitionInfo( qcStatement   * aStatement,
                                           qcmTableInfo  * aTableInfo,
                                           idBool        * aExistsEmpty );

    /* BUG-47599 */
    static IDE_RC checkAddCond( qcStatement       * aStatement,
                                qdTableParseTree  * aParseTree );
private :

    // Table의 Attribute Flag List에서 동일한
    // Attribute List가 존재할 경우 에러처리
    static IDE_RC checkTableAttrIsUnique(qcStatement         * aStatement,
                                         qdTableAttrFlagList * aAttrFlagList);

    static IDE_RC getColumnTypeInMemory( mtcColumn * aColumn,
                                         UInt      * aColumnTypeFlag,
                                         UInt      * aVcInOutBaseSize );
    
    // PROJ-1877 alter table modify column
    static UInt getNewColumnIDForAlter( qcmColumn * aDelColList,
                                        UInt        aColumnID );

    static IDE_RC decideColumnTypeFixedOrVariable( qcmColumn* aColumn,
                                                   UInt       aMemoryOrDisk,
                                                   scSpaceID  aTBSID,
                                                   idBool     aIsAddColumn );

    static IDE_RC validateColumnLength( qcStatement * aStatement,
                                        qcmColumn   * aColumn );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC adjustColumnFlagForTable( qcStatement       * aStatement,
                                            smiTableSpaceType   aTBSType,
                                            qcmColumn         * aColumn,
                                            idBool              aEnableVariableColumn );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC adjustColumnFlagForIndex( mtcColumn * aSrcMtcColumn,
                                            UInt      * aOffset,
                                            smiColumn * aDstSmiColumn );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC validateForeignKey( qcStatement      * aStatement,
                                      qdReferenceSpec  * aRefSpec,
                                      SInt               aCountDiskType,
                                      SInt               aCountMemType );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC makeTBSUsageList( qcStatement   * aStatement,
                                    qcmTableInfo  * aTableInfo,
                                    qdbTBSUsage  ** aTBSUsageList );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC makeTBSUsage( qcStatement  * aStatement,
                                scSpaceID      aTBSID,
                                qdbTBSUsage  * aTBSUsageList );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC getMemUsage( qcStatement * aStatement,
                               qdbMemUsage * aMemUsage );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC getTBSUsage( qcStatement * aStatement,
                               qdbTBSUsage * aTBSUsage );

    /* PROJ-2586 PSM Parameters and return without precision */
    static IDE_RC getMtcColumnFromTargetInternal( qtcNode   * aQtcTargetNode,
                                                  mtcColumn * aMtcTargetColumn,
                                                  mtcColumn * aMtcColumn );
};

#endif  //  _O_QDB_COMMON_H_
