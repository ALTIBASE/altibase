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
 * $Id: qcmPartition.cpp 20699 2007-02-27 10:01:29Z a464 $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smiTableSpace.h>
#include <qcm.h>
#include <qcmCache.h>
#include <qcmTableInfo.h>
#include <qcpManager.h>
#include <qcmPartition.h>
#include <qcmTableSpace.h>
#include <qcmUser.h>
#include <qcg.h>
#include <qdtCommon.h>
#include <smiStatement.h>

// 파티션 관련 메타 테이블 핸들 및 인덱스 핸들
const void * gQcmPartTables;
const void * gQcmPartIndices;
const void * gQcmTablePartitions;
const void * gQcmIndexPartitions;
const void * gQcmPartKeyColumns;
const void * gQcmPartLobs;
const void * gQcmPartTablesIndex            [QCM_MAX_META_INDICES];
const void * gQcmPartIndicesIndex           [QCM_MAX_META_INDICES];
const void * gQcmTablePartitionsIndex       [QCM_MAX_META_INDICES];
const void * gQcmIndexPartitionsIndex       [QCM_MAX_META_INDICES];
const void * gQcmPartKeyColumnsIndex        [QCM_MAX_META_INDICES];
const void * gQcmPartLobsIndex              [QCM_MAX_META_INDICES];

extern mtdModule mtdChar;

IDE_RC qcmPartition::getNextTablePartitionID(
    qcStatement *aStatement,
    UInt        *aTablePartitionID )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 Partitioned Disk Table
 *                Table Partition의 ID를 sequencial하게 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    void   * sSequenceHandle;
    idBool   sExist;
    idBool   sFirst;
    SLong    sSeqVal;
    SLong    sSeqValFirst;

    sExist = ID_TRUE;
    sFirst = ID_TRUE;

    IDE_TEST(qcm::getSequenceHandleByName(
                 QC_SMI_STMT( aStatement ),
                 QC_SYSTEM_USER_ID,
                 (UChar*)gDBSequenceName[QCM_DB_SEQUENCE_TABLE_PARTITION_ID],
                 idlOS::strlen(gDBSequenceName[QCM_DB_SEQUENCE_TABLE_PARTITION_ID]),
                 &sSequenceHandle)
             != IDE_SUCCESS);

    while( sExist == ID_TRUE )
    {
        IDE_TEST(smiTable::readSequence( QC_SMI_STMT( aStatement ),
                     sSequenceHandle,
                     SMI_SEQUENCE_NEXT,
                     &sSeqVal,
                     NULL)
                 != IDE_SUCCESS);

        // sSeqVal은 비록 SLong이지만, sequence를 생성할 때
        // max를 integer max를 안넘도록 하였기 때문에
        // 여기서 overflow체크는 하지 않는다.
        IDE_TEST( searchTablePartitionID( QC_SMI_STMT( aStatement ),
                                          (SInt)sSeqVal,
                                          &sExist )
                  != IDE_SUCCESS );

        if( sFirst == ID_TRUE )
        {
            sSeqValFirst = sSeqVal;
            sFirst = ID_FALSE;
        }
        else
        {
            // 찾다찾다 한바퀴 돈 경우.
            // 이는 object가 꽉 찬 것을 의미함.
            IDE_TEST_RAISE( sSeqVal == sSeqValFirst, ERR_OBJECTS_OVERFLOW );
        }        
    }    

    *aTablePartitionID = sSeqVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_OBJECTS_OVERFLOW);
    {    
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                "table partitions",
                                QCM_META_SEQ_MAXVALUE) );
    }    
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getNextIndexPartitionID( qcStatement *aStatement,
                                              UInt        *aIndexPartitionID)
{
/***********************************************************************
 *
 *  Description : PROJ-1502 Partitioned Disk Table
 *                Index Partition의 ID를 sequencial하게 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    void   * sSequenceHandle;
    idBool   sExist;
    idBool   sFirst;
    SLong    sSeqVal;
    SLong    sSeqValFirst;

    sExist = ID_TRUE;
    sFirst = ID_TRUE;
    

    IDE_TEST(qcm::getSequenceHandleByName(
                 QC_SMI_STMT( aStatement ),
                 QC_SYSTEM_USER_ID,
                 (UChar*)gDBSequenceName[QCM_DB_SEQUENCE_INDEX_PARTITION_ID],
                 idlOS::strlen(gDBSequenceName[QCM_DB_SEQUENCE_INDEX_PARTITION_ID]),
                 &sSequenceHandle)
             != IDE_SUCCESS);

    while( sExist == ID_TRUE )
    {
        IDE_TEST(smiTable::readSequence( QC_SMI_STMT( aStatement ),
                     sSequenceHandle,
                     SMI_SEQUENCE_NEXT,
                     &sSeqVal,
                     NULL)
                 != IDE_SUCCESS);

        // sSeqVal은 비록 SLong이지만, sequence를 생성할 때
        // max를 integer max를 안넘도록 하였기 때문에
        // 여기서 overflow체크는 하지 않는다.
        IDE_TEST( searchIndexPartitionID( QC_SMI_STMT( aStatement ),
                                (SInt)sSeqVal,
                                &sExist )
                  != IDE_SUCCESS );

        if( sFirst == ID_TRUE )
        {
            sSeqValFirst = sSeqVal;
            sFirst = ID_FALSE;
        }
        else
        {
            // 찾다찾다 한바퀴 돈 경우.
            // 이는 object가 꽉 찬 것을 의미함.
            IDE_TEST_RAISE( sSeqVal == sSeqValFirst, ERR_OBJECTS_OVERFLOW );
        }        
    }

    *aIndexPartitionID = sSeqVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_OBJECTS_OVERFLOW);
    {    
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                "index partitions",
                                QCM_META_SEQ_MAXVALUE) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::makeQcmPartitionInfoByTableID(
    smiStatement * aSmiStmt,
    UInt           aTableID )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sTableIDCol;
    mtcColumn           * sPartitionIDCol;
    mtcColumn           * sPartitionOIDCol;
    smOID                 sPartitionOID;
    ULong                 sPartitionOIDULong;
    UInt                  sPartitionID;
    qcmTableInfo        * sTableInfo;
    smSCN                 sSCN;
    void                * sTableHandle;
    smiTableCursor        sCursor;
    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    SInt                  sStage  = 0;
    const void          * sTableRow;
    const void          * sRow;
    smiCursorProperties   sCursorProperty;

    IDE_TEST( qcm::getTableHandleByID(
                  aSmiStmt,
                  aTableID,
                  &sTableHandle,
                  &sSCN,
                  &sTableRow ) != IDE_SUCCESS );

    IDE_TEST(  smiGetTableTempInfo( sTableHandle,
                                    (void**)&sTableInfo )
               != IDE_SUCCESS );

    if( sTableInfo->tablePartitionType != QCM_PARTITIONED_TABLE )
    {
        // Nothing to do.
        IDE_DASSERT( sTableInfo->tablePartitionType ==
                     QCM_NONE_PARTITIONED_TABLE );
    }
    else
    {
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                      (const smiColumn**)&sTableIDCol )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_OID_COL_ORDER,
                                      (const smiColumn**)&sPartitionOIDCol )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                      (const smiColumn**)&sPartitionIDCol )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn(
            &sRangeColumn,
            (const mtcColumn*) sTableIDCol,
            (const void *) &aTableID,
            &sRange);

        sCursor.initialize();

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        // row를 하나씩 읽어와서 partitionInfo를 생성해준다.
        IDE_TEST(sCursor.open(
                     aSmiStmt,
                     gQcmTablePartitions,
                     gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                     smiGetRowSCN(gQcmTablePartitions),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     &sCursorProperty) != IDE_SUCCESS);

        sStage = 1;

        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        while(sRow != NULL)
        {
            sPartitionOIDULong = *(ULong *)((UChar*) sRow +
                                            sPartitionOIDCol->column.offset);
            sPartitionOID = (smOID)sPartitionOIDULong;

            sPartitionID =  *(UInt *)((UChar*) sRow +
                                      sPartitionIDCol->column.offset);

            IDE_TEST( makeAndSetQcmPartitionInfo( aSmiStmt,
                                                  sPartitionID,
                                                  sPartitionOID,
                                                  sTableInfo,
                                                  sRow )
                      != IDE_SUCCESS );

            IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::destroyQcmPartitionInfoByTableID(
    smiStatement * aSmiStmt,
    UInt           aTableID )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                서버 셧다운시 메타 캐시 정리를 위해 호출한다.
 *
 *  Implementation : 테이블/파티션에 대해 lock은 잡지 않는다.
 *                   lock을 잡을 필요가 없기 때문임.
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sTableIDCol;
    mtcColumn           * sPartitionOIDCol;
    smOID                 sPartitionOID;
    ULong                 sPartitionOIDULong;
    qcmTableInfo        * sTableInfo;
    qcmTableInfo        * sPartitionInfo;
    smSCN                 sSCN;
    void                * sTableHandle;
    void                * sPartitionHandle;
    smiTableCursor        sCursor;
    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    SInt                  sStage  = 0;
    const void          * sTableRow;
    const void          * sRow;
    smiCursorProperties   sCursorProperty;

    IDE_TEST( qcm::getTableHandleByID(
                  aSmiStmt,
                  aTableID,
                  &sTableHandle,
                  &sSCN,
                  &sTableRow ) != IDE_SUCCESS );

    IDE_TEST(  smiGetTableTempInfo( sTableHandle,
                                    (void**)&sTableInfo )
               != IDE_SUCCESS );

    // 적합성 검사. 파티션이면 안됨
    IDE_DASSERT( sTableInfo->tablePartitionType != QCM_TABLE_PARTITION );

    switch( sTableInfo->tablePartitionType )
    {
        case QCM_NONE_PARTITIONED_TABLE:
            // Nothing to do.
            break;

        case QCM_PARTITIONED_TABLE:
        {
            IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                          QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                          (const smiColumn**)&sTableIDCol )
                      != IDE_SUCCESS );

            IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                          QCM_TABLE_PARTITIONS_PARTITION_OID_COL_ORDER,
                                          (const smiColumn**)&sPartitionOIDCol )
              != IDE_SUCCESS );

                qcm::makeMetaRangeSingleColumn(
                    &sRangeColumn,
                    (const mtcColumn*) sTableIDCol,
                    (const void *) &aTableID,
                    &sRange);

                sCursor.initialize();

                SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

                // row를 하나씩 읽어와서 partitionInfo를 생성해준다.
                IDE_TEST(sCursor.open(
                             aSmiStmt,
                             gQcmTablePartitions,
                             gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                             smiGetRowSCN(gQcmTablePartitions),
                             NULL,
                             &sRange,
                             smiGetDefaultKeyRange(),
                             smiGetDefaultFilter(),
                             QCM_META_CURSOR_FLAG,
                             SMI_SELECT_CURSOR,
                             &sCursorProperty) != IDE_SUCCESS);

                sStage = 1;

                IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
                IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

                while(sRow != NULL)
                {
                    sPartitionOIDULong = *(ULong *)((UChar*) sRow +
                                                    sPartitionOIDCol->column.offset);
                    sPartitionOID = (smOID)sPartitionOIDULong;

                    sPartitionHandle = (void *)smiGetTable( sPartitionOID );

                    IDE_TEST( smiGetTableTempInfo( sPartitionHandle,
                                                   (void**)&sPartitionInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( destroyQcmPartitionInfo( sPartitionInfo )
                              != IDE_SUCCESS );

                    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
                }

                sStage = 0;
                IDE_TEST(sCursor.close() != IDE_SUCCESS);

            }
            break;

        case QCM_TABLE_PARTITION:
        default:
            IDE_DASSERT(0); // 테이블 파티션이거나 알수없는 타입인 경우
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionCount( qcStatement * aStatement,
                                        UInt          aTableID,
                                        UInt        * aPartitionCount )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sTableIDCol;
    vSLong                sRowCount;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDCol,
        (const void *) &aTableID,
        &sRange);

    IDE_TEST( qcm::selectCount(
                  QC_SMI_STMT( aStatement ),
                  gQcmTablePartitions,
                  &sRowCount,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmTablePartitionsIndex
                  [QCM_TABLE_PARTITIONS_IDX2_ORDER] )
              != IDE_SUCCESS );

    IDE_DASSERT( sRowCount > 0 );

    *aPartitionCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// using RP/SD
// return Actual partition count ( empty partition 제외 )
IDE_RC qcmPartition::getPartitionCount4SmiStmt( smiStatement * aSmiStmt,
                                                UInt           aTableID,
                                                UInt         * aPartitionCount )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sTableIDCol;
    vSLong                sRowCount;
    idBool                sIsEmptyPart = ID_FALSE;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDCol,
        (const void *) &aTableID,
        &sRange);

    IDE_TEST( qcm::selectCount(
                  aSmiStmt,
                  gQcmTablePartitions,
                  &sRowCount,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmTablePartitionsIndex
                  [QCM_TABLE_PARTITIONS_IDX2_ORDER] )
              != IDE_SUCCESS );

    IDE_DASSERT( sRowCount > 0 );

    // BUG-47599 rp/sd에서는 empty partition을 제외한 갯수만 알아야합니다.
    IDE_TEST( qcmPartition::isEmptyPartition( aSmiStmt,
                                              aTableID,
                                              &sIsEmptyPart )
              != IDE_SUCCESS );

    if ( sIsEmptyPart == ID_TRUE )
    {
        sRowCount--;
    }

    *aPartitionCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getIndexPartitionCount( qcStatement * aStatement,
                                             UInt          aIndexID,
                                             SChar       * aIndexPartName,
                                             UInt          aIndexPartNameLen,
                                             UInt        * aPartitionCount )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstRangeColumn;
    qtcMetaRangeColumn    sSecondRangeColumn;
    mtcColumn           * sIndexIDCol;
    mtcColumn           * sIndexPartNameCol;
    qcNameCharBuffer      sIndexValueBuffer;
    mtdCharType         * sIndexPartValue = (mtdCharType *) & sIndexValueBuffer;
    vSLong                sRowCount;

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sIndexPartNameCol )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sIndexPartValue,
                          NULL,
                          (SChar*)aIndexPartName,
                          aIndexPartNameLen );

    qcm::makeMetaRangeDoubleColumn(
        &sFirstRangeColumn,
        &sSecondRangeColumn,
        (const mtcColumn*) sIndexIDCol,
        (const void *) &aIndexID,
        (const mtcColumn*) sIndexPartNameCol,
        (const void *) sIndexPartValue,
        &sRange);

    IDE_TEST( qcm::selectCount(
                  QC_SMI_STMT( aStatement ),
                  gQcmIndexPartitions,
                  &sRowCount,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmIndexPartitionsIndex
                  [QCM_INDEX_PARTITIONS_IDX2_ORDER] )
              != IDE_SUCCESS );

    *aPartitionCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartCondVal( qcStatement        * aStatement,
                                     qcmColumn          * aColumns,
                                     qcmPartitionMethod   aPartitionMethod,
                                     qmsPartCondValList * aPartCondVal,
                                     SChar              * aPartCondValStmtText,
                                     qmmValueNode      ** aPartCondValNode,
                                     mtdCharType        * aPartKeyCondValueStr )
{
    SChar                    sBuffer[4000+8];
    SChar                  * sStmtText;
    qcStatement            * sPartCondValStatement;
    qdPartCondValParseTree * sParseTree;
    qmmValueNode           * sPartCondValNode;

    UInt                     sOriInsOrUptStmtCount;
    UInt                   * sOriInsOrUptRowValueCount;
    smiValue              ** sOriInsOrUptRow;

    if( aPartCondValStmtText == NULL )
    {
        sStmtText = sBuffer;
    }
    else
    {
        sStmtText = aPartCondValStmtText;
    }

    if( aPartKeyCondValueStr->length > 0 )
    {
        idlOS::strncpy( sStmtText, "VALUES ", 7 );

        idlOS::memcpy( &sStmtText[7],
                       aPartKeyCondValueStr->value,
                       aPartKeyCondValueStr->length );

        sStmtText[aPartKeyCondValueStr->length + 7] = '\0';

        IDU_LIMITPOINT("qcmPartition::getPartCondVal::malloc");
        IDE_TEST(STRUCT_ALLOC(
                     QC_QMP_MEM(aStatement), qcStatement, &sPartCondValStatement)
                 != IDE_SUCCESS);

        QC_SET_STATEMENT(sPartCondValStatement, aStatement, NULL);

        if( QC_SHARED_TMPLATE(sPartCondValStatement) == NULL )
        {
            // execute시에 호출되는 경우 shared tmplate을
            // private tmplate으로 연결한다.
            QC_SHARED_TMPLATE(sPartCondValStatement) =
                QC_PRIVATE_TMPLATE(sPartCondValStatement);
            QC_PRIVATE_TMPLATE(sPartCondValStatement) = NULL;
        }
        else
        {
            // Nothing to do.
        }
        
        IDE_ASSERT( QC_SHARED_TMPLATE(sPartCondValStatement) != NULL );

        sPartCondValStatement->myPlan->stmtText = (SChar*)sStmtText;
        sPartCondValStatement->myPlan->stmtTextLen = idlOS::strlen((SChar*)sStmtText);

        // preserve insOrUptRow
        sOriInsOrUptStmtCount = QC_SHARED_TMPLATE(sPartCondValStatement)->insOrUptStmtCount;
        sOriInsOrUptRowValueCount =
            QC_SHARED_TMPLATE(sPartCondValStatement)->insOrUptRowValueCount;
        sOriInsOrUptRow = QC_SHARED_TMPLATE(sPartCondValStatement)->insOrUptRow;
        
        IDE_TEST(qcpManager::parseIt(sPartCondValStatement) != IDE_SUCCESS);

        // restore insOrUptRow
        QC_SHARED_TMPLATE(sPartCondValStatement)->insOrUptStmtCount  = sOriInsOrUptStmtCount;
        QC_SHARED_TMPLATE(sPartCondValStatement)->insOrUptRowValueCount =
            sOriInsOrUptRowValueCount;
        QC_SHARED_TMPLATE(sPartCondValStatement)->insOrUptRow = sOriInsOrUptRow;

        sParseTree = (qdPartCondValParseTree*) sPartCondValStatement->myPlan->parseTree;

        // partition key condition value node setting
        sPartCondValNode = sParseTree->partKeyCond;

        if( QC_SHARED_TMPLATE(aStatement) == NULL )
        {
            // execute시에 호출되는 경우
            IDE_TEST( qdbCommon::makePartKeyCondValues(
                          aStatement,
                          QC_PRIVATE_TMPLATE(aStatement),
                          aColumns,
                          aPartitionMethod,
                          sPartCondValNode,
                          aPartCondVal )
                      != IDE_SUCCESS );
        }
        else
        {
            // prepare시에 호출되는 경우
            IDE_TEST( qdbCommon::makePartKeyCondValues(
                          aStatement,
                          QC_SHARED_TMPLATE(aStatement),
                          aColumns,
                          aPartitionMethod,
                          sPartCondValNode,
                          aPartCondVal )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // null인 경우임.
        aPartCondVal->partCondValCount = 0;
        sPartCondValNode = NULL;
    }

    if( aPartCondValNode != NULL )
    {
        *aPartCondValNode = sPartCondValNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::setPartitionRef(
    idvSQL               * /* aStatistics */,
    const void           * aRow,
    qcmSetRef            * aSetRef )
{
    qcStatement     * sStatement;
    qmsTableRef     * sTableRef;
    qmsPartitionRef * sPartitionRef;
    mtdCharType     * sPartKeyCondValueStr;
    mtcColumn       * sTableOIDColumn;
    ULong             sTableOIDULong;
    mtcColumn       * sPartIDMtcColumn;
    mtcColumn       * sPartMinValueMtcColumn;
    mtcColumn       * sPartMaxValueMtcColumn;
    mtcColumn       * sPartOrderMtcColumn;
    /* BUG-47599 */
    mtcColumn       * sPartitionNameCol;
    mtdCharType     * sPartitionName;

    sStatement = aSetRef->statement;
    sTableRef = aSetRef->tableRef;

    IDE_DASSERT( sTableRef->tableInfo->tablePartitionType ==
                 QCM_PARTITIONED_TABLE );

    // BUG-47599
    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameCol )
              != IDE_SUCCESS );

    sPartitionName = (mtdCharType *) mtc::value( sPartitionNameCol,
                                                 aRow,
                                                 MTD_OFFSET_USE );

    if ( sPartitionName->length == QD_EMPTY_PARTITION_NAME_SIZE )
    {
        IDU_FIT_POINT( "qcmPartition::setPartitionRef::malloc3" );
        IDE_TEST( QC_QMP_MEM(sStatement)->cralloc( ID_SIZEOF(qmsPartitionRef),
                                                   (void**) &sPartitionRef )
                  != IDE_SUCCESS );

        sTableRef->mEmptyPartRef = sPartitionRef;
    }
    else
    {
        if( ( sTableRef->flag & QMS_TABLE_REF_PRE_PRUNING_MASK )
            == QMS_TABLE_REF_PRE_PRUNING_TRUE )
        {
            if( ( sTableRef->flag & QMS_TABLE_REF_PARTITION_MADE_MASK ) ==
                QMS_TABLE_REF_PARTITION_MADE_FALSE )
            {
                // 선행 프루닝이 되어 있고, 최초로 생성하는 경우,
                // 이미 tableRef에 partitionRef가 존재한다.
                IDE_DASSERT( sTableRef->partitionRef != NULL );

                sPartitionRef = sTableRef->partitionRef;
            }
            else
            {
                // 최초로 생성하는 경우가 아닌경우
                // CNF/DNF optimizing을 위한 것이므로, 새로 생성해서 붙여야 한다.
                IDE_DASSERT( sTableRef->partitionRef == NULL );

                IDU_LIMITPOINT("qcmPartition::setPartitionRef::malloc1");
                IDE_TEST( QC_QMP_MEM(sStatement)->cralloc(
                        ID_SIZEOF(qmsPartitionRef),
                        (void**) &sPartitionRef )
                    != IDE_SUCCESS );

                sTableRef->partitionRef = sPartitionRef;
                sPartitionRef->next = NULL;
            }
        }
        else
        {
            // 선행 프루닝이 되어 있지 않은 경우
            // 새로 만들어서 붙여줘야 한다.
            IDU_LIMITPOINT("qcmPartition::setPartitionRef::malloc2");
            IDE_TEST( QC_QMP_MEM(sStatement)->cralloc(
                    ID_SIZEOF(qmsPartitionRef),
                    (void**) &sPartitionRef )
                != IDE_SUCCESS );

            if( sTableRef->partitionRef == NULL )
            {
                sTableRef->partitionRef = sPartitionRef;
                sPartitionRef->next = NULL;
            }
            else
            {
                sPartitionRef->next = sTableRef->partitionRef;
                sTableRef->partitionRef = sPartitionRef;
            }
        }
    }

    // partition id를 세팅.
    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        aRow,
        sPartIDMtcColumn,
        & sPartitionRef->partitionID );

    // partition oid를 세팅
    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_OID_COL_ORDER,
                                  (const smiColumn**)&sTableOIDColumn )
              != IDE_SUCCESS );
    sTableOIDULong = *(ULong*)((UChar*)aRow + sTableOIDColumn->column.offset);
    sPartitionRef->partitionOID = (smOID)sTableOIDULong;
    
    sPartitionRef->partitionHandle = (void *)smiGetTable( sPartitionRef->partitionOID );
    sPartitionRef->partitionSCN = smiGetRowSCN( sPartitionRef->partitionHandle );
    
    IDE_TEST( smiGetTableTempInfo( sPartitionRef->partitionHandle,
                                   (void**)&(sPartitionRef->partitionInfo) )
              != IDE_SUCCESS );

    IDE_DASSERT( sTableRef->tableInfo->tablePartitionType ==
                 QCM_PARTITIONED_TABLE );

    // range는 min,max를 다 구함.
    // list는 max만 구함.
    // hash는 partition order를 구함.
    switch( sTableRef->tableInfo->partitionMethod )
    {
        case QCM_PARTITION_METHOD_RANGE:
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
        {
            IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                          QCM_TABLE_PARTITIONS_PARTITION_MIN_VALUE_COL_ORDER,
                                          (const smiColumn**)&sPartMinValueMtcColumn )
                      != IDE_SUCCESS );

            // min을 구함.
            sPartKeyCondValueStr = (mtdCharType *)
                mtc::value( sPartMinValueMtcColumn,
                            aRow,
                        MTD_OFFSET_USE );
                
                IDE_TEST( getPartCondVal( sStatement,
                                          sTableRef->tableInfo->partKeyColumns,
                                          sTableRef->tableInfo->partitionMethod,
                                          &sPartitionRef->minPartCondVal,
                                          NULL, /* aPartCondValStmtText */
                                          NULL, /* aPartCondValNode */
                                          sPartKeyCondValueStr )
                          != IDE_SUCCESS );

                if( sPartitionRef->minPartCondVal.partCondValCount == 0 )
                {
                    sPartitionRef->minPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_MIN;
                }
                else
                {
                    sPartitionRef->minPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_NORMAL;
                }

                IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                              QCM_TABLE_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER,
                                              (const smiColumn**)&sPartMaxValueMtcColumn )
                          != IDE_SUCCESS );

                // max를 구함.
                sPartKeyCondValueStr = (mtdCharType *)
                    mtc::value( sPartMaxValueMtcColumn,
                                aRow,
                                MTD_OFFSET_USE );
                
                IDE_TEST( getPartCondVal( sStatement,
                                          sTableRef->tableInfo->partKeyColumns,
                                          sTableRef->tableInfo->partitionMethod,
                                          &sPartitionRef->maxPartCondVal,
                                          NULL, /* aPartCondValStmtText */
                                          NULL, /* aPartCondValNode */
                                          sPartKeyCondValueStr )
                          != IDE_SUCCESS );

                if( sPartitionRef->maxPartCondVal.partCondValCount == 0 )
                {
                    sPartitionRef->maxPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_DEFAULT;
                }
                else
                {
                    sPartitionRef->maxPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_NORMAL;
                }
            }
            break;

        case QCM_PARTITION_METHOD_LIST:
        {
            IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                          QCM_TABLE_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER,
                                          (const smiColumn**)&sPartMaxValueMtcColumn )
                      != IDE_SUCCESS );

            // max를 구함.
            sPartKeyCondValueStr = (mtdCharType *)
                mtc::value( sPartMaxValueMtcColumn,
                            aRow,
                            MTD_OFFSET_USE );

                IDE_TEST( getPartCondVal( sStatement,
                                          sTableRef->tableInfo->partKeyColumns,
                                          sTableRef->tableInfo->partitionMethod,
                                          &sPartitionRef->maxPartCondVal,
                                          NULL, /* aPartCondValStmtText */
                                          NULL, /* aPartCondValNode */
                                          sPartKeyCondValueStr )
                          != IDE_SUCCESS );

                if( sPartitionRef->maxPartCondVal.partCondValCount == 0 )
                {
                    sPartitionRef->maxPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_DEFAULT;

                    IDE_TEST( makeMaxCondValListDefaultPart(
                                  sStatement,
                                  sTableRef,
                                  sPartitionRef )
                              != IDE_SUCCESS );
                }
                else
                {
                    sPartitionRef->maxPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_NORMAL;
                }
            }
            break;

        case QCM_PARTITION_METHOD_HASH:
        {
            IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                          QCM_TABLE_PARTITIONS_PARTITION_ORDER_COL_ORDER,
                                          (const smiColumn**)&sPartOrderMtcColumn )
                      != IDE_SUCCESS );

            qcm::getIntegerFieldValue(
                aRow,
                sPartOrderMtcColumn,
                & sPartitionRef->partOrder );
        }
        break;

        case QCM_PARTITION_METHOD_NONE:
            break;
        default:
            IDE_DASSERT(0);
            break;

    }

    // BUG-42229
    IDU_FIT_POINT("qcmPartition::setPartitionRef");

    if ( QC_SHARED_TMPLATE(sStatement) != NULL )
    {
        if ( ( ( smiGetTableFlag( sPartitionRef->partitionHandle ) & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_MEMORY ) ||
             ( ( smiGetTableFlag( sPartitionRef->partitionHandle ) & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_VOLATILE ) ||
             ( ( smiGetTableFlag( sPartitionRef->partitionHandle ) & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_META ) )
        {
            QC_SHARED_TMPLATE(sStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
        }
        else
        {
            QC_SHARED_TMPLATE(sStatement)->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
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

IDE_RC qcmPartition::makeMaxCondValListDefaultPart( qcStatement     * aStatement,
                                                    qmsTableRef     * aTableRef,
                                                    qmsPartitionRef * aPartitionRef )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sTableIDColumn;
    scGRID                sRid;
    smiTableCursor        sCursor;
    const void          * sRow;
    UInt                  sStage = 0;
    UInt                  sFlag = QCM_META_CURSOR_FLAG;
    mtdCharType         * sPartKeyCondValueStr;
    UInt                  sPartCondValCount = 0;
    qmsPartCondValList    sMaxPartCondVal;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sPartMaxValueMtcColumn;

    IDE_DASSERT( aTableRef->tableInfo->partitionMethod ==
                 QCM_PARTITION_METHOD_LIST );

    IDE_DASSERT( aPartitionRef->maxPartCondVal.partCondValType ==
                 QMS_PARTCONDVAL_DEFAULT );

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDColumn,
        (const void *) &aTableRef->tableInfo->tableID,
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while(sRow != NULL)
    {
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER,
                                      (const smiColumn**)&sPartMaxValueMtcColumn )
                  != IDE_SUCCESS );
        // max를 구함.
        sPartKeyCondValueStr = (mtdCharType *)
            mtc::value( sPartMaxValueMtcColumn,
                        sRow,
                        MTD_OFFSET_USE );

        if( sPartKeyCondValueStr->length > 0 )
        {
            IDE_TEST( getPartCondVal( aStatement,
                                      aTableRef->tableInfo->partKeyColumns,
                                      aTableRef->tableInfo->partitionMethod,
                                      &sMaxPartCondVal,
                                      NULL, /* aPartCondValStmtText */
                                      NULL, /* aPartCondValNode */
                                      sPartKeyCondValueStr )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( (sPartCondValCount + sMaxPartCondVal.partCondValCount) >
                            QC_MAX_PARTKEY_COND_COUNT,
                            ERR_ABORT_QCM_TOO_MANY_LIST_PARTITION_CONDITION_VALUE );

            idlOS::memcpy( &(aPartitionRef->maxPartCondVal.partCondValues[sPartCondValCount]),
                           &sMaxPartCondVal.partCondValues[0],
                           ID_SIZEOF(void*) * sMaxPartCondVal.partCondValCount );

            sPartCondValCount += sMaxPartCondVal.partCondValCount;
        }
        else
        {
            // default partition
            // nothing to do
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    aPartitionRef->maxPartCondVal.partCondValCount = sPartCondValCount;

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ABORT_QCM_TOO_MANY_LIST_PARTITION_CONDITION_VALUE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_TOO_MANY_LIST_PARTITION_CONDITION_VALUE));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPrePruningPartitionRef( qcStatement * aStatement,
                                                qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *           선행 프루닝 된 파티션에 대한 정보만 가져온다.
 *
 *  Implementation :
 *
 ***********************************************************************/
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sPartitionIDCol;
    vSLong                sRowCount;
    qcmSetRef             sQcmSetRef;

    sQcmSetRef.statement = aStatement;
    sQcmSetRef.tableRef  = aTableRef;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sPartitionIDCol,
        (const void *) & (aTableRef->selectedPartitionID),
        &sRange);

    IDE_TEST( qcm::selectRow(
                  QC_SMI_STMT( aStatement ),
                  gQcmTablePartitions,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmTablePartitionsIndex
                  [QCM_TABLE_PARTITIONS_IDX1_ORDER],
                  (qcmSetStructMemberFunc ) setPartitionRef,
                  &sQcmSetRef,
                  0,
                  ID_UINT_MAX,
                  & sRowCount )
              != IDE_SUCCESS );

    IDE_DASSERT( sRowCount == 1 );

    IDE_TEST( qcmPartition::setEmptyPartitionRef( aStatement,
                                                  aTableRef )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getAllPartitionRef( qcStatement * aStatement,
                                         qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *           해당 테이블에 속한 모든 파티션 메타 캐시를 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sTableIDColumn;

    vSLong                sRowCount;
    qcmSetRef             sQcmSetRef;

    sQcmSetRef.statement = aStatement;
    sQcmSetRef.tableRef  = aTableRef;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDColumn,
        (const void *) &aTableRef->tableInfo->tableID,
        &sRange);

    IDE_TEST( qcm::selectRow(
                  QC_SMI_STMT( aStatement ),
                  gQcmTablePartitions,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmTablePartitionsIndex
                  [QCM_TABLE_PARTITIONS_IDX2_ORDER],
                  (qcmSetStructMemberFunc ) setPartitionRef,
                  &sQcmSetRef,
                  0,
                  ID_UINT_MAX,
                  & sRowCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::validateAndLockPartitions( qcStatement      * aStatement,
                                                qmsTableRef      * aTableRef,
                                                smiTableLockMode   aLockMode )
{
    qmsPartitionRef * sPartitionRef = NULL;
    ULong             sTimeout      = ID_ULONG_MAX;


    if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK ) == QCI_STMT_MASK_DDL )
    {
        sTimeout = smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans());
    }
    else
    {
        /* Nothing to do */
    }

    for ( sPartitionRef = aTableRef->partitionRef;
          sPartitionRef != NULL;
          sPartitionRef = sPartitionRef->next )
    {
        IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT(aStatement))->getTrans(),
                                             sPartitionRef->partitionHandle,
                                             sPartitionRef->partitionSCN,
                                             SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                             aLockMode,
                                             sTimeout,
                                             ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                  != IDE_SUCCESS );
    }

    if ( aTableRef->mEmptyPartRef != NULL )
    {
        // BUG-47599
        // emptyPartiton에 lock을 걸면 add partition op와 lock 충돌이 발생합니다.
        IDE_TEST( smiValidateObjects( aTableRef->mEmptyPartRef->partitionHandle,
                                      aTableRef->mEmptyPartRef->partitionSCN )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionInfoByID( qcStatement   * aStatement,
                                           UInt            aPartitionID,
                                           qcmTableInfo ** aPartitionInfo,
                                           smSCN         * aSCN,
                                           void         ** aPartitionHandle )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 파티션 ID로 파티션메타캐시를 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    const void *sPartitionRow;

    IDE_TEST( getPartitionHandleByID(
                  QC_SMI_STMT(aStatement),
                  aPartitionID,
                  aPartitionHandle,
                  aSCN,
                  &sPartitionRow) != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( *aPartitionHandle,
                                   (void**)aPartitionInfo )
              != IDE_SUCCESS );

    if ( QC_SHARED_TMPLATE(aStatement) != NULL )
    {
        if( ( (smiGetTableFlag(*aPartitionHandle) & SMI_TABLE_TYPE_MASK) == SMI_TABLE_MEMORY ) ||
            ( (smiGetTableFlag(*aPartitionHandle) & SMI_TABLE_TYPE_MASK) == SMI_TABLE_VOLATILE ) ||
            ( (smiGetTableFlag(*aPartitionHandle) & SMI_TABLE_TYPE_MASK) == SMI_TABLE_META ) )
        {
            QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
        }
        else
        {
            QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
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

IDE_RC qcmPartition::checkIndexPartitionByUser( qcStatement    *aStatement,
                                                qcNamePosition  aUserName,
                                                qcNamePosition  aIndexName,
                                                UInt            aIndexID,
                                                UInt           *aUserID,
                                                UInt           *aTablePartID )
{
/***********************************************************************
 *
 * Description :
 *    userName.indexName 을 체크한다.
 *
 * Implementation :
 *    1. user 가 있는지 체크해서 userID 를 구한다.
 *    2. SYS_INDEX_PARTITIONS_ 캐쉬로부터 USER_ID, INDEX_NAME 컬럼을 구해 둔다.
 *    3. 2가 없으면 META_CRASH 에러 반환
 *    4. SYS_INDEX_PARTITIONS_ 메타 테이블에 USERID_INDEXNAME 에 인덱스가 있으면,
 *       userID, indexName 으로 keyRange 를 만든다.
 *    5. 한 Row 씩 읽으면서(4에서 인덱스가 없으면, userID, indexName을
 *       비교해서 일치하는 것만) tableID, indexID 를 구한다.
 *    6. 일치하는 인덱스가 없으면 에러를 반환한다.
 *
 ***********************************************************************/

    UInt                    sStage = 0;
    UInt                    sUserID;
    mtcColumn             * sUserIDCol = NULL;
    mtcColumn             * sIndexIDCol = NULL;
    mtcColumn             * sIndexNameCol = NULL;
    mtcColumn             * sMtcColumnInfo;
    idBool                  sIdxFound = ID_FALSE;
    smiTableCursor          sCursor;

    qcNameCharBuffer        sIdxNameBuffer;
    mtdCharType           * sIdxName = (mtdCharType *) & sIdxNameBuffer;
    mtdCharType           * sIdxNameStr = NULL;

    const void            * sRow;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier

    IDE_ASSERT( gQcmIndexPartitions != NULL );

    // check user existence
    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        sUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        IDE_TEST(qcmUser::getUserID( aStatement, aUserName, &sUserID )
                 != IDE_SUCCESS);
    }

    *aUserID = sUserID;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sUserIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sIndexNameCol )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sIdxName,
                          NULL,
                          aIndexName.stmtText + aIndexName.offset,
                          aIndexName.size);

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, aStatement->mStatistics );
    
    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 gQcmIndexPartitions,
                 NULL,
                 smiGetRowSCN(gQcmIndexPartitions),
                 NULL,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL && sIdxFound == ID_FALSE)
    {
        sIdxNameStr = (mtdCharType*)((UChar*)sRow +
                                     sIndexNameCol->column.offset);

        // BUG-37032
        if( ( *(UInt*)((UChar*)sRow + sUserIDCol->column.offset) == sUserID ) &&
            ( *(UInt*)((UChar*)sRow + sIndexIDCol->column.offset) == aIndexID ) &&
            ( idlOS::strMatch( (SChar*)sIdxName->value,
                               sIdxName->length,
                               (SChar*)&(sIdxNameStr->value),
                               sIdxNameStr->length ) == 0 ) )
        {
            sIdxFound = ID_TRUE;

            IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                          QCM_INDEX_PARTITIONS_TABLE_PARTITION_ID_COL_ORDER,
                                          (const smiColumn**)&sMtcColumnInfo )
                      != IDE_SUCCESS );

            *aTablePartID = *(UInt*)((UChar *)sRow +
                                     sMtcColumnInfo->column.offset);
        }
        else
        {
            sIdxFound = ID_FALSE;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;

    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE(sIdxFound != ID_TRUE, ERR_NOT_EXIST_INDEX);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_INDEX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXISTS_INDEX));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}


IDE_RC qcmPartition::getPartitionInfo( qcStatement   * aStatement,
                                       UInt            aTableID,
                                       UChar         * aPartitionName,
                                       SInt            aPartitionNameSize,
                                       qcmTableInfo ** aPartitionInfo,
                                       smSCN         * aSCN,
                                       void         ** aPartitionHandle )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 테이블ID, 파티션 이름(UChar)으로
 *                파티션 메타 캐시를 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    IDE_TEST(getPartitionHandleByName(
                 QC_SMI_STMT( aStatement ),
                 aTableID,
                 aPartitionName,
                 aPartitionNameSize,
                 aPartitionHandle,
                 aSCN) != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( *aPartitionHandle,
                                   (void**)aPartitionInfo )
              != IDE_SUCCESS );

    if ( QC_SHARED_TMPLATE(aStatement) != NULL )
    {
        if( ( (smiGetTableFlag(*aPartitionHandle) & SMI_TABLE_TYPE_MASK) == SMI_TABLE_MEMORY ) ||
            ( (smiGetTableFlag(*aPartitionHandle) & SMI_TABLE_TYPE_MASK) == SMI_TABLE_VOLATILE ) ||
            ( (smiGetTableFlag(*aPartitionHandle) & SMI_TABLE_TYPE_MASK) == SMI_TABLE_META ) )
        {
            QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
        }
        else
        {
            QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
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

IDE_RC qcmPartition::getPartitionInfo( qcStatement     * aStatement,
                                       UInt              aTableID,
                                       qcNamePosition    aPartitionName,
                                       qcmTableInfo   ** aPartitionInfo,
                                       smSCN           * aSCN,
                                       void           ** aPartitionHandle )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 테이블ID, 파티션 이름(position)으로
 *                파티션 메타 캐시를 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    IDE_TEST(getPartitionInfo( aStatement,
                               aTableID,
                               (UChar*) (aPartitionName.stmtText +
                                         aPartitionName.offset),
                               aPartitionName.size,
                               aPartitionInfo,
                               aSCN,
                               aPartitionHandle)
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionHandleByName( smiStatement * aSmiStmt,
                                               UInt           aTableID,
                                               UChar        * aPartitionName,
                                               SInt           aPartitionNameSize,
                                               void        ** aPartitionHandle,
                                               smSCN        * aSCN )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 테이블ID, 파티션 이름(UChar)으로
 *                파티션 핸들을 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/
    UInt                  sStage = 0;
    smiRange              sRange;
    smiTableCursor        sCursor;
    const void          * sRow;
    smOID                 sPartitionOID;
    ULong                 sPartitionOIDULong;
    UInt                  sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn           * sTableIDCol;
    mtcColumn           * sPartitionNameCol;
    mtcColumn           * sPartitionOIDCol;
    qtcMetaRangeColumn    sFirstRangeColumn;
    qtcMetaRangeColumn    sSecondRangeColumn;

    qcNameCharBuffer      sPartitionNameBuffer;
    mtdCharType         * sPartitionName = ( mtdCharType * ) & sPartitionNameBuffer;

    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties   sCursorProperty;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_OID_COL_ORDER,
                                  (const smiColumn**)&sPartitionOIDCol )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sPartitionName,
                          NULL,
                          (SChar*)aPartitionName,
                          aPartitionNameSize );

    qcm::makeMetaRangeDoubleColumn(
        &sFirstRangeColumn,
        &sSecondRangeColumn,
        (const mtcColumn *) sTableIDCol,
        (const void *) &aTableID,
        (const mtcColumn *) sPartitionNameCol,
        (const void *) sPartitionName,
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRow == NULL, ERR_NOT_EXIST_PARTITION);

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    sPartitionOIDULong = *(ULong *)((UChar*) sRow +
                                    sPartitionOIDCol->column.offset);
    sPartitionOID = (smOID)sPartitionOIDULong;

    *aPartitionHandle = (void *)smiGetTable( sPartitionOID );

    *aSCN = smiGetRowSCN(*aPartitionHandle);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION, ""));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getQcmPartKeyColumns( smiStatement * aSmiStmt,
                                           qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 파티션 키 컬럼정보 구축
 *
 *  Implementation :
 *
 ***********************************************************************/

    UInt                sStage = 0;
    SInt                sObjType = 0; // 0:TABLE, 1:INDEX
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void        * sRow;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn         * sQcmObjTypeColumn;
    mtcColumn         * sQcmPartObjIDColumn;
    mtcColumn         * sQcmPartKeyColIDColumn;

    qtcMetaRangeColumn  sFirstRangeColumn;
    qtcMetaRangeColumn  sSecondRangeColumn;

    UInt                i;
    UInt                sColumnID;
    qcmColumn         * sTableColumn = NULL;

    scGRID              sRid;
    smiCursorProperties sCursorProperty;

    IDU_LIMITPOINT("qcmPartition::getQcmPartKeyColumns::malloc1");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM, 1,
                                 ID_SIZEOF(qcmColumn) * aTableInfo->partKeyColCount,
                                 (void**)&(aTableInfo->partKeyColumns) )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qcmPartition::getQcmPartKeyColumns::malloc2");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM, 1,
                                 ID_SIZEOF(mtcColumn) * aTableInfo->partKeyColCount,
                                 (void**)&(aTableInfo->partKeyColBasicInfo) )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qcmPartition::getQcmPartKeyColumns::malloc3");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM, 1,
                                 ID_SIZEOF(UInt) * aTableInfo->partKeyColCount,
                                 (void**)&(aTableInfo->partKeyColsFlag) )
              != IDE_SUCCESS );

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmPartKeyColumns,
                                  QCM_PART_KEY_COLUMNS_OBJECT_TYPE_COL_ORDER,
                                  (const smiColumn**)&sQcmObjTypeColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPartKeyColumns,
                                  QCM_PART_KEY_COLUMNS_PARTITION_OBJ_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmPartObjIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPartKeyColumns,
                                  QCM_PART_KEY_COLUMNS_COLUMN_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmPartKeyColIDColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeDoubleColumn(
        &sFirstRangeColumn,
        &sSecondRangeColumn,
        (const mtcColumn *) sQcmObjTypeColumn,
        (const void *) &sObjType,
        (const mtcColumn *) sQcmPartObjIDColumn,
        (const void *) &aTableInfo->tableID,
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmPartKeyColumns,
                 gQcmPartKeyColumnsIndex[QCM_PART_KEY_COLUMNS_IDX1_ORDER],
                 smiGetRowSCN(gQcmPartKeyColumns),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);

    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow((const void**)&sRow, &sRid, SMI_FIND_NEXT)
             != IDE_SUCCESS);

    IDE_DASSERT( sRow != NULL );
    IDE_TEST_RAISE( sRow == NULL, ERR_META_CRASH );

    i = 0;

    while (sRow != NULL)
    {

        sColumnID = *((UInt *) ((UChar *) sRow +
                                sQcmPartKeyColIDColumn->column.offset));

        IDE_TEST( qcmCache::getColumnByID( aTableInfo,
                                           sColumnID,
                                           &sTableColumn )
                  != IDE_SUCCESS );

        IDE_ASSERT( sTableColumn != NULL );
        
        aTableInfo->partKeyColumns[i] = *sTableColumn;

        idlOS::memcpy( &aTableInfo->partKeyColBasicInfo[i],
                       sTableColumn->basicInfo,
                       ID_SIZEOF(mtcColumn) );

        aTableInfo->partKeyColumns[i].basicInfo =
            &aTableInfo->partKeyColBasicInfo[i];

        aTableInfo->partKeyColsFlag[i] = SMI_COLUMN_ORDER_ASCENDING;

        i++;

        IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid, SMI_FIND_NEXT)
                 != IDE_SUCCESS);
    }

    for( i = 0; i < aTableInfo->partKeyColCount -1; i++)
    {
        aTableInfo->partKeyColumns[i].next = &aTableInfo->partKeyColumns[i+1];
    }

    aTableInfo->partKeyColumns[i].next = NULL;

    sStage = 0;

    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1 :
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getQcmPartitionedTableInfo( smiStatement * aSmiStmt,
                                                 qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 Partitioned Table에 대한 정보를
 *                테이블 메타 캐시에 구축
 *
 *  Implementation :
 *
 ***********************************************************************/
    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void        * sRow;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn         * sQcmTableIDColumn;
    mtcColumn         * sQcmPartMethodColumn;
    mtcColumn         * sQcmRowMovementColumn;
    mtcColumn         * sQcmPartKeyColCountColumn;
    mtdCharType       * sRowMovementStr;

    qtcMetaRangeColumn  sRangeColumn;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmPartTables,
                                  QCM_PART_TABLES_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmTableIDColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn *) sQcmTableIDColumn,
        (const void *) &aTableInfo->tableID,
        &sRange);

    IDE_TEST( smiGetTableColumns( gQcmPartTables,
                                  QCM_PART_TABLES_PARTITION_METHOD_COL_ORDER,
                                  (const smiColumn**)&sQcmPartMethodColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPartTables,
                                  QCM_PART_TABLES_PARTITION_KEY_COUNT_COL_ORDER,
                                  (const smiColumn**)&sQcmPartKeyColCountColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPartTables,
                                  QCM_PART_TABLES_ROW_MOVEMENT_COL_ORDER,
                                  (const smiColumn**)&sQcmRowMovementColumn )
              != IDE_SUCCESS );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmPartTables,
                 gQcmPartTablesIndex[QCM_PART_TABLES_IDX1_ORDER],
                 smiGetRowSCN(gQcmPartTables),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRow == NULL, ERR_NOT_EXIST_TABLE);

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    aTableInfo->partitionMethod = *(qcmPartitionMethod *) ((UChar*)sRow +
                                                           sQcmPartMethodColumn->column.offset);

    aTableInfo->partKeyColCount = *(UInt *) ((UChar*)sRow +
                                             sQcmPartKeyColCountColumn->column.offset);

    sRowMovementStr = (mtdCharType *) ((UChar*)sRow +
                                       sQcmRowMovementColumn->column.offset);

    if ( idlOS::strMatch( (SChar *)&(sRowMovementStr->value),
                          sRowMovementStr->length,
                          "T",
                          1 ) == 0 )
    {
        aTableInfo->rowMovement = ID_TRUE;
    }
    else
    {
        aTableInfo->rowMovement = ID_FALSE;
    }

    IDE_TEST( getQcmPartKeyColumns( aSmiStmt, aTableInfo )
              != IDE_SUCCESS );

    aTableInfo->tablePartitionType = QCM_PARTITIONED_TABLE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_TABLE));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getQcmPartitionedIndexInfo( smiStatement * aSmiStmt,
                                                 qcmIndex     * aIndex )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 Partitioned Index에 대한 정보를
 *                인덱스 메타 캐시에 구축
 *
 *  Implementation :
 *
 ***********************************************************************/
    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void        * sRow;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn         * sQcmIndexIDColumn;
    mtcColumn         * sQcmPartTypeColumn;
    mtcColumn         * sQcmIsLocalUniqueColumn;
    mtdCharType       * sIsLocalUniqueStr;

    qtcMetaRangeColumn  sRangeColumn;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmPartIndices,
                                  QCM_PART_INDICES_INDEX_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmIndexIDColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn *) sQcmIndexIDColumn,
        (const void *) &aIndex->indexId,
        &sRange);

    IDE_TEST( smiGetTableColumns( gQcmPartIndices,
                                  QCM_PART_INDICES_PARTITION_TYPE_COL_ORDER,
                                  (const smiColumn**)&sQcmPartTypeColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmPartIndices,
                                  QCM_PART_INDICES_IS_LOCAL_UNIQUE_COL_ORDER,
                                  (const smiColumn**)&sQcmIsLocalUniqueColumn )
              != IDE_SUCCESS );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmPartIndices,
                 gQcmPartIndicesIndex[QCM_PART_INDICES_IDX1_ORDER],
                 smiGetRowSCN(gQcmPartIndices),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRow == NULL, ERR_META_CRASH);

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    aIndex->indexPartitionType = *(qcmIndexPartitionType *) ((UChar*)sRow +
                                                             sQcmPartTypeColumn->column.offset);

    sIsLocalUniqueStr = (mtdCharType *) ((UChar*)sRow +
                                         sQcmIsLocalUniqueColumn->column.offset);

    if ( idlOS::strMatch( (SChar *)&(sIsLocalUniqueStr->value),
                          sIsLocalUniqueStr->length,
                          "T",
                          1 ) == 0 )
    {
        aIndex->isLocalUnique = ID_TRUE;
    }
    else
    {
        aIndex->isLocalUnique = ID_FALSE;
    }

    if( aIndex->indexPartitionType == QCM_GLOBAL_PREFIXED_PARTITIONED_INDEX )
    {
        // 현재 글로벌 인덱스는 지원하지 않음.
        IDE_ASSERT(0);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::destroyQcmPartitionInfo( qcmTableInfo * aInfo )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 파티션 메타 캐시 삭제
 *
 *  Implementation :
 *             (1) localIndices free
 *             (2) 구조체 free
 ***********************************************************************/

    UInt i;

    if( aInfo != NULL )
    {
        IDE_DASSERT( aInfo->tablePartitionType == QCM_TABLE_PARTITION );

        if (aInfo->columns != NULL)
        {
            for (i = 0; i < aInfo->columnCount; i++)
            {
                IDE_TEST(iduMemMgr::free(aInfo->columns[i].basicInfo)
                         != IDE_SUCCESS);

                if ( aInfo->columns[i].defaultValueStr != NULL )
                {
                    IDE_TEST( iduMemMgr::free( aInfo->columns[i].defaultValueStr ) != IDE_SUCCESS );
                    aInfo->columns[i].defaultValueStr = NULL;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            
            IDE_TEST(iduMemMgr::free(aInfo->columns) != IDE_SUCCESS);
            aInfo->columns = NULL;
        }

        if (aInfo->indices != NULL)
        {
            for (i = 0; i< aInfo->indexCount; i++)
            {
                if ( aInfo->indices[i].keyColumns != NULL )
                {
                    IDE_TEST(iduMemMgr::free(aInfo->indices[i].keyColumns)
                             != IDE_SUCCESS);
                    aInfo->indices[i].keyColumns = NULL;
                }
            }
            IDE_TEST(iduMemMgr::free(aInfo->indices) != IDE_SUCCESS);
            aInfo->indices = NULL;
        }

        if ( aInfo->uniqueKeys != NULL )
        {
            IDE_TEST( iduMemMgr::free( aInfo->uniqueKeys ) != IDE_SUCCESS );
            aInfo->uniqueKeys = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aInfo->foreignKeys != NULL )
        {
            IDE_TEST( iduMemMgr::free( aInfo->foreignKeys ) != IDE_SUCCESS );
            aInfo->foreignKeys = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aInfo->notNulls != NULL )
        {
            IDE_TEST( iduMemMgr::free( aInfo->notNulls ) != IDE_SUCCESS );
            aInfo->notNulls = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aInfo->checks != NULL )
        {
            for ( i = 0; i < aInfo->checkCount; i++ )
            {
                if ( aInfo->checks[i].checkCondition != NULL )
                {
                    IDE_TEST( iduMemMgr::free( aInfo->checks[i].checkCondition ) != IDE_SUCCESS );
                    aInfo->checks[i].checkCondition = NULL;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            IDE_TEST( iduMemMgr::free( aInfo->checks ) != IDE_SUCCESS );
            aInfo->checks = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aInfo->timestamp != NULL )
        {
            IDE_TEST( iduMemMgr::free( aInfo->timestamp ) != IDE_SUCCESS );
            aInfo->timestamp = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aInfo->triggerInfo != NULL )
        {
            IDE_TEST( iduMemMgr::free( aInfo->triggerInfo ) != IDE_SUCCESS );
            aInfo->triggerInfo = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if(aInfo->partKeyColumns != NULL)
        {
            IDE_TEST(iduMemMgr::free(aInfo->partKeyColumns) != IDE_SUCCESS);
            aInfo->partKeyColumns = NULL;
        }
        if(aInfo->partKeyColBasicInfo != NULL)
        {
            IDE_TEST(iduMemMgr::free(aInfo->partKeyColBasicInfo) != IDE_SUCCESS);
            aInfo->partKeyColBasicInfo = NULL;
        }

        //------------------------------------------
        // (2) 구조체 free
        //------------------------------------------
        IDE_TEST(iduMemMgr::free(aInfo) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::destroyQcmPartitionInfoList( qcmPartitionInfoList * aPartInfoList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Partition Info List의 Meta Cache를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmTableInfo         * sPartInfo     = NULL;

    for ( sPartInfoList = aPartInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        sPartInfo = sPartInfoList->partitionInfo;
        sPartInfoList->partitionInfo = NULL;

        IDU_FIT_POINT( "qcmPartition::destroyQcmPartitionInfoList::free::partitionInfo",
                       idERR_ABORT_DOUBLEFREE );

        IDE_TEST( qcmPartition::destroyQcmPartitionInfo( sPartInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    for ( sPartInfoList = sPartInfoList->next;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        (void)qcmPartition::destroyQcmPartitionInfo( sPartInfoList->partitionInfo );
        sPartInfoList->partitionInfo = NULL;
    }

    return IDE_FAILURE;
}

/* BUG-42928 No Partition Lock */
void qcmPartition::restoreTempInfo( qcmTableInfo         * aTableInfo,
                                    qcmPartitionInfoList * aPartInfoList,
                                    qdIndexTableList     * aIndexTableList )
{
    qcmPartitionInfoList * sPartInfoList   = NULL;
    qdIndexTableList     * sIndexTableList = NULL;

    if ( aTableInfo != NULL )
    {
        if ( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            for ( sPartInfoList = aPartInfoList;
                  sPartInfoList != NULL;
                  sPartInfoList = sPartInfoList->next )
            {
                smiSetTableTempInfo( sPartInfoList->partHandle,
                                     (void *) sPartInfoList->partitionInfo );

                if ( QCU_TABLE_LOCK_MODE != 0 )
                {
                    smiSetTableReplacementLock( sPartInfoList->partitionInfo->tableHandle,
                                                aTableInfo->tableHandle );
                }
                else
                {
                    /* Nothing to do */
                }
            }

            for ( sIndexTableList = aIndexTableList;
                  sIndexTableList != NULL;
                  sIndexTableList = sIndexTableList->next )
            {
                smiSetTableTempInfo( sIndexTableList->tableHandle,
                                     (void *) sIndexTableList->tableInfo );
            }
        }
        else
        {
            /* Nothing to do */
        }

        smiSetTableTempInfo( aTableInfo->tableHandle,
                             (void *) aTableInfo );

        if ( QCU_TABLE_LOCK_MODE != 0 )
        {
            smiInitTableReplacementLock( aTableInfo->tableHandle );
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

    return;
}

void qcmPartition::restoreTempInfoForPartition( qcmTableInfo * aTableInfo,
                                                qcmTableInfo * aPartInfo )
{
    if ( ( aTableInfo != NULL ) && ( aPartInfo != NULL ) )
    {
        if ( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            smiSetTableTempInfo( aPartInfo->tableHandle,
                                 (void *) aPartInfo );

            if ( QCU_TABLE_LOCK_MODE != 0 )
            {
                smiSetTableReplacementLock( aPartInfo->tableHandle,
                                            aTableInfo->tableHandle );
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

    return;
}

IDE_RC qcmPartition::getQcmPartitionColumn( qcmTableInfo * aTableInfo,
                                            qcmTableInfo * aPartitionInfo )
{
    mtcColumn  * sColumn;
    UInt         i;
    SInt         sDefaultValueStrLen = 0;

    aPartitionInfo->columnCount = aTableInfo->columnCount;

    if(aPartitionInfo->columnCount == 0)
    {
        aPartitionInfo->columns = NULL;
    }
    else
    {
        IDU_LIMITPOINT("qcmPartition::getQcmPartitionColumn::malloc1");
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM,
                                     aPartitionInfo->columnCount,
                                     ID_SIZEOF( qcmColumn ),
                                     (void **)&(aPartitionInfo->columns) )
                  != IDE_SUCCESS );

        for( i = 0; i < aPartitionInfo->columnCount; i++ )
        {
            idlOS::memcpy( &aPartitionInfo->columns[i],
                           &aTableInfo->columns[i],
                           ID_SIZEOF(qcmColumn) );

            /* Memory를 할당하기 전에 문제가 발생하면, Partition과 Partitioned에서 각각 free를 수행한다.
             * 따라서, Memory를 할당할 Pointer 포인터를 미리 초기화해야 한다.
             */
            aPartitionInfo->columns[i].basicInfo       = NULL;
            aPartitionInfo->columns[i].defaultValueStr = NULL;

            // BUG-25886
            // table header의 컬럼정보에도 module과 language를 설정한다.
            IDE_TEST( smiGetTableColumns( aPartitionInfo->tableHandle,
                                          i,
                                          (const smiColumn**)&sColumn )
                      != IDE_SUCCESS );

            // mtdModule 설정
            IDE_TEST( mtd::moduleById( &(sColumn->module),
                                       sColumn->type.dataTypeId )
                      != IDE_SUCCESS );

            // mtlModule 설정
            IDE_TEST( mtl::moduleById( &(sColumn->language),
                                       sColumn->type.languageId )
                      != IDE_SUCCESS );

            // PROJ-1877 basicInfo를 복사 생성한다.
            IDU_LIMITPOINT("qcmPartition::getQcmPartitionColumn::malloc2");
            IDE_TEST(
                iduMemMgr::malloc(IDU_MEM_QCM,
                                  ID_SIZEOF(mtcColumn),
                                  (void**)&(aPartitionInfo->columns[i].basicInfo))
                != IDE_SUCCESS);

            idlOS::memcpy( aPartitionInfo->columns[i].basicInfo,
                           sColumn,
                           ID_SIZEOF(mtcColumn) );

            // mtdModule 설정
            IDE_TEST(
                mtd::moduleById(
                    &(aPartitionInfo->columns[i].basicInfo->module),
                    (aPartitionInfo->columns[i].basicInfo->type.dataTypeId))
                != IDE_SUCCESS);

            // mtlModule 설정
            IDE_TEST(
                mtl::moduleById(
                    &(aPartitionInfo->columns[i].basicInfo->language),
                    (aPartitionInfo->columns[i].basicInfo->type.languageId))
                != IDE_SUCCESS);

            /* PROJ-2464 hybrid partitioned table 지원 */
            aPartitionInfo->columns[i].basicInfo->flag &= ~MTC_COLUMN_NOTNULL_MASK;
            aPartitionInfo->columns[i].basicInfo->flag |= aTableInfo->columns[i].basicInfo->flag
                & MTC_COLUMN_NOTNULL_MASK;

            // PROJ-2204 Join Update, Delete
            aPartitionInfo->columns[i].basicInfo->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
            aPartitionInfo->columns[i].basicInfo->flag |= aTableInfo->columns[i].basicInfo->flag
                & MTC_COLUMN_KEY_PRESERVED_MASK;

            if ( aTableInfo->columns[i].defaultValueStr != NULL )
            {
                sDefaultValueStrLen = idlOS::strlen( (SChar *)aTableInfo->columns[i].defaultValueStr );

                IDU_FIT_POINT( "qcmPartition::getQcmPartitionColumn::malloc::defaultValueStr",
                                idERR_ABORT_InsufficientMemory );

                IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                             sDefaultValueStrLen + 1,
                                             (void **) & aPartitionInfo->columns[i].defaultValueStr )
                          != IDE_SUCCESS );

                idlOS::memcpy( aPartitionInfo->columns[i].defaultValueStr,
                               aTableInfo->columns[i].defaultValueStr,
                               sDefaultValueStrLen + 1 );
            }
            else
            {
                aPartitionInfo->columns[i].defaultValueStr = NULL;
            }

            if( i == aPartitionInfo->columnCount - 1 )
            {
                aPartitionInfo->columns[i].next = NULL;
            }
            else
            {
                aPartitionInfo->columns[i].next =
                    &(aPartitionInfo->columns[i+1]);
            }
        }

        aPartitionInfo->lobColumnCount = aTableInfo->lobColumnCount;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getQcmPartitionConstraints( qcmTableInfo * aTableInfo,
                                                 qcmTableInfo * aPartitionInfo )
{
    UInt i = 0;
    UInt j = 0;
    SInt sCheckConditionLen = 0;

    aPartitionInfo->primaryKey = NULL;

    if( aTableInfo->primaryKey != NULL )
    {
        for( i = 0; i < aPartitionInfo->indexCount; i++ )
        {
            if( aTableInfo->primaryKey->indexId ==
                aPartitionInfo->indices[i].indexId )
            {
                aPartitionInfo->primaryKey = &aPartitionInfo->indices[i];
                break;
            }
        }
    }

    aPartitionInfo->uniqueKeyCount = aTableInfo->uniqueKeyCount;
    aPartitionInfo->uniqueKeys = NULL;

    if ( aTableInfo->uniqueKeys != NULL )
    {
        IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::uniqueKeys",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF( qcmUnique ) * aTableInfo->uniqueKeyCount,
                                     (void **)&(aPartitionInfo->uniqueKeys),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        for ( i = 0; i < aTableInfo->uniqueKeyCount; i++ )
        {
            (void)idlOS::memcpy( &(aPartitionInfo->uniqueKeys[i]),
                                 &(aTableInfo->uniqueKeys[i]),
                                 ID_SIZEOF( qcmUnique ) );

            for ( j = 0; j < aPartitionInfo->indexCount; j++ )
            {
                if ( aTableInfo->uniqueKeys[i].constraintIndex->indexId ==
                     aPartitionInfo->indices[j].indexId )
                {
                    aPartitionInfo->uniqueKeys[i].constraintIndex = &(aPartitionInfo->indices[j]);
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    aPartitionInfo->foreignKeyCount = aTableInfo->foreignKeyCount;
    aPartitionInfo->foreignKeys     = NULL;

    if ( aTableInfo->foreignKeys != NULL )
    {
        IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::foreignKeys",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF( qcmForeignKey ) * aTableInfo->foreignKeyCount,
                                     (void **)&(aPartitionInfo->foreignKeys),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        for ( i = 0; i < aTableInfo->foreignKeyCount; i++ )
        {
            (void)idlOS::memcpy( &(aPartitionInfo->foreignKeys[i]),
                                 &(aTableInfo->foreignKeys[i]),
                                 ID_SIZEOF( qcmForeignKey ) );
        }
    }
    else
    {
        /* Nothing to do */
    }

    aPartitionInfo->notNullCount = aTableInfo->notNullCount;
    aPartitionInfo->notNulls     = NULL;

    if ( aTableInfo->notNulls != NULL )
    {
        IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::notNulls",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF( qcmNotNull ) * aTableInfo->notNullCount,
                                     (void **)&(aPartitionInfo->notNulls),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        for ( i = 0; i < aTableInfo->notNullCount; i++ )
        {
            (void)idlOS::memcpy( &(aPartitionInfo->notNulls[i]),
                                 &(aTableInfo->notNulls[i]),
                                 ID_SIZEOF( qcmNotNull ) );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* PROJ-1107 Check Constraint 지원 */
    aPartitionInfo->checkCount = aTableInfo->checkCount;
    aPartitionInfo->checks     = NULL;

    if ( aTableInfo->checks != NULL )
    {
        IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::checks",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM,
                                     aTableInfo->checkCount,
                                     ID_SIZEOF( qcmCheck ),
                                     (void **)&(aPartitionInfo->checks),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        for ( i = 0; i < aTableInfo->checkCount; i++ )
        {
            (void)idlOS::memcpy( &(aPartitionInfo->checks[i]),
                                 &(aTableInfo->checks[i]),
                                 ID_SIZEOF( qcmCheck ) );

            aPartitionInfo->checks[i].checkCondition = NULL;

            if ( aTableInfo->checks[i].checkCondition != NULL )
            {
                sCheckConditionLen = idlOS::strlen( aTableInfo->checks[i].checkCondition );

                IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::checkCondition",
                                idERR_ABORT_InsufficientMemory );

                IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                             sCheckConditionLen + 1,
                                             (void **) & aPartitionInfo->checks[i].checkCondition )
                          != IDE_SUCCESS );

                idlOS::memcpy( aPartitionInfo->checks[i].checkCondition,
                               aTableInfo->checks[i].checkCondition,
                               sCheckConditionLen + 1 );
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

    aPartitionInfo->timestamp = NULL;

    if ( aTableInfo->timestamp != NULL )
    {
        IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::timestamp",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF( qcmTimeStamp ),
                                     (void **)&(aPartitionInfo->timestamp),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        (void)idlOS::memcpy( aPartitionInfo->timestamp,
                             aTableInfo->timestamp,
                             ID_SIZEOF( qcmTimeStamp ) );
    }
    else
    {
        /* Nothing to do */
    }

    // Partition의 권한은 존재하지 않으며, Partitioned Table에만 권한을 검사한다.
    aPartitionInfo->privilegeCount = 0;
    aPartitionInfo->privilegeInfo  = NULL;

    aPartitionInfo->triggerCount = aTableInfo->triggerCount;
    aPartitionInfo->triggerInfo  = NULL;

    if ( aTableInfo->triggerInfo != NULL )
    {
        IDU_FIT_POINT( "qcmPartition::getQcmPartitionConstraints::malloc::triggerInfo",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF( qcmTriggerInfo ) * aTableInfo->triggerCount,
                                     (void **)&(aPartitionInfo->triggerInfo),
                                     IDU_MEM_IMMEDIATE )
                  != IDE_SUCCESS );

        for ( i = 0; i < aTableInfo->triggerCount; i++ )
        {
            (void)idlOS::memcpy( &(aPartitionInfo->triggerInfo[i]),
                                 &(aTableInfo->triggerInfo[i]),
                                 ID_SIZEOF( qcmTriggerInfo ) );
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

IDE_RC qcmPartition::getQcmLocalIndexCommonInfo(
    qcmTableInfo * aTableInfo,
    qcmIndex     * aIndex )
{
    UInt i;

    for( i = 0; i < aTableInfo->indexCount; i++ )
    {
        IDE_ASSERT( aTableInfo->indices != NULL );
        
        if( aIndex->indexId == aTableInfo->indices[i].indexId )
        {
            aIndex->keyColCount   = aTableInfo->indices[i].keyColCount;
            aIndex->indexTypeId   = aTableInfo->indices[i].indexTypeId;
            aIndex->isUnique      = aTableInfo->indices[i].isUnique;
            aIndex->isLocalUnique = aTableInfo->indices[i].isLocalUnique;
            aIndex->isRange       = aTableInfo->indices[i].isRange;
            break;
        }
    }

    IDE_TEST_RAISE( i == aTableInfo->indexCount,
                    ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getQcmLocalIndices( smiStatement * aSmiStmt,
                                         qcmTableInfo * aTableInfo,
                                         qcmTableInfo * aPartitionInfo )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 makeAndSetQcmPartitionInfo로부터 호출이 됨.
 *                파티션의 로컬 인덱스 캐쉬를 만든다.
 *
 *  Implementation :
 *
 *
 ***********************************************************************/


    mtcColumn             * sQcmTablePartitionIDCol;
    mtcColumn             * sQcmIndexPartitionIDCol;
    mtcColumn             * sMtcColumnInfo;
    mtdCharType           * sCharTypeStr;
    qtcMetaRangeColumn      sRangeColumn;
    const void            * sRow;
    UInt                    sIndexID;
    smiRange                sRange;
    smiTableCursor          sCursor;
    SInt                    sStage  = 0;
    SInt                    i       = 0;
    UInt                    j       = 0;
    UInt                    c       = 0;
    smiTableSpaceAttr       sTBSAttr;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    UInt                  * sKeyCols;
    UInt                    sOffset = 0;
    UInt                    sTableType;
    smiCursorProperties     sCursorProperty;

    sCursor.initialize();

    IDE_ASSERT( gQcmIndexPartitions != NULL );
    IDE_ASSERT( aPartitionInfo != NULL );

    // index가 없으면 안만듬
    if(aPartitionInfo->indexCount == 0)
    {
        aPartitionInfo->indices = NULL;

        return IDE_SUCCESS;
    }

    sTableType = aPartitionInfo->tableFlag & SMI_TABLE_TYPE_MASK;

    IDE_ASSERT( gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX1_ORDER]
                != NULL );

    IDU_LIMITPOINT("qcmPartition::getQcmLocalIndices::malloc1");
    IDE_TEST(iduMemMgr::calloc(IDU_MEM_QCM, 1,
                               ID_SIZEOF(qcmIndex) *
                               aPartitionInfo->indexCount,
                               (void**)&(aPartitionInfo->indices))
             != IDE_SUCCESS);

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_TABLE_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmTablePartitionIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmIndexPartitionIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sQcmTablePartitionIDCol,
        (const void *) & (aPartitionInfo->partitionID),
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmIndexPartitions,
                 gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX1_ORDER],
                 smiGetRowSCN(gQcmIndexPartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while(sRow != NULL)
    {
        aPartitionInfo->indices[i].indexPartitionType = QCM_LOCAL_INDEX_PARTITION;

        aPartitionInfo->indices[i].indexPartitionID = *(UInt*)
            ((UChar*)sRow + sQcmIndexPartitionIDCol->column.offset);

        aPartitionInfo->indices[i].indexHandle = NULL;

        //----------------------------
        // INDEX_PARTITION_NAME
        //----------------------------
        IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                      QCM_INDEX_PARTITIONS_INDEX_PARTITION_NAME_COL_ORDER,
                                      (const smiColumn**)&sMtcColumnInfo )
                  != IDE_SUCCESS );

        sCharTypeStr = (mtdCharType*)((UChar*)sRow +
                                      sMtcColumnInfo->column.offset);

        idlOS::strncpy(aPartitionInfo->indices[i].name,
                       (const char*)&(sCharTypeStr->value),
                       sCharTypeStr->length);
        *(aPartitionInfo->indices[i].name + sCharTypeStr->length) = '\0';

        //----------------------------
        // TBS_ID, IS_ONLINE
        //----------------------------

        IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                      QCM_INDEX_PARTITIONS_TBS_ID_COL_ORDER,
                                      (const smiColumn**)&sMtcColumnInfo )
                  != IDE_SUCCESS );

        aPartitionInfo->indices[i].TBSID =
            qcm::getSpaceID(sRow, sMtcColumnInfo->column.offset);

        // is_online
        IDE_TEST(
            qcmTablespace::getTBSAttrByID(
                aPartitionInfo->indices[i].TBSID,
                &sTBSAttr )
            != IDE_SUCCESS);

        if( sTBSAttr.mTBSStateOnLA != SMI_TBS_OFFLINE )

        {
            aPartitionInfo->indices[i].isOnlineTBS = ID_TRUE;
        }
        else
        {
            aPartitionInfo->indices[i].isOnlineTBS = ID_FALSE;
        }

        //----------------------------
        // Partitioned Index Search
        //----------------------------

        IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                      QCM_INDEX_PARTITIONS_INDEX_ID_COL_ORDER,
                                      (const smiColumn**)&sMtcColumnInfo )
                  != IDE_SUCCESS );

        sIndexID = *(UInt*)((UChar*)sRow +
                            sMtcColumnInfo->column.offset);

        aPartitionInfo->indices[i].indexId = sIndexID;

        aPartitionInfo->indices[i].indexHandle = (void *)smiGetTableIndexByID(
            aPartitionInfo->tableHandle,
            aPartitionInfo->indices[i].indexId);

        IDE_TEST_RAISE( aPartitionInfo->indices[i].indexHandle == NULL,
                        ERR_META_CRASH );

        IDE_TEST( getQcmLocalIndexCommonInfo(
                      aTableInfo,
                      &aPartitionInfo->indices[i] )
                  != IDE_SUCCESS );

        IDU_LIMITPOINT("qcmPartition::getQcmLocalIndices::malloc2");
        IDE_TEST( iduMemMgr::malloc(
                      IDU_MEM_QCM,
                      ID_SIZEOF(mtcColumn) *
                      aPartitionInfo->indices[i].keyColCount,
                      (void**)&(aPartitionInfo->indices[i].
                                keyColumns ) )
                  != IDE_SUCCESS );

        sKeyCols = (UInt*)smiGetIndexColumns(
            aPartitionInfo->indices[i].indexHandle );

        sOffset = 0;

        for( j = 0; j < aPartitionInfo->indices[i].keyColCount; j++ )
        {
            for( c = 0; c < aPartitionInfo->columnCount; c++ )
            {
                if( sKeyCols[j] ==
                    aPartitionInfo->columns[c].basicInfo->column.id )
                {
                    break;
                }
            }

            IDE_TEST_RAISE( c == aPartitionInfo->columnCount,
                            ERR_META_CRASH );

            idlOS::memcpy( &(aPartitionInfo->indices[i].keyColumns[j]),
                           aPartitionInfo->columns[c].basicInfo,
                           ID_SIZEOF(mtcColumn) );

            aPartitionInfo->indices[i].keyColumns[j].column.flag &=
                ~SMI_COLUMN_USAGE_MASK;
            aPartitionInfo->indices[i].keyColumns[j].column.flag |=
                SMI_COLUMN_USAGE_INDEX;

            if ( sTableType == SMI_TABLE_DISK )
            {
                // PROJ-1705
                IDE_TEST(
                    qdbCommon::setIndexKeyColumnTypeFlag(
                        &(aPartitionInfo->indices[i].keyColumns[j]) )
                    != IDE_SUCCESS );

                // To Fix PR-8111
                if ( ( aPartitionInfo->indices[i].keyColumns[j].
                       column.flag &
                       SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_FIXED )
                {
                    sOffset =
                        idlOS::align( sOffset,
                                      aPartitionInfo->columns[c].
                                      basicInfo->module->align );
                    aPartitionInfo->indices[i].keyColumns[j].
                        column.offset = sOffset;
                    sOffset +=
                        aPartitionInfo->columns[c].basicInfo->
                        column.size;
                }
                else
                {
                    sOffset = idlOS::align( sOffset, 8 );
                    aPartitionInfo->indices[i].keyColumns[j].
                        column.offset = sOffset;
                    sOffset +=
                        smiGetVariableColumnSize4DiskIndex();
                }
            }
        }

        aPartitionInfo->indices[i].keyColsFlag = (UInt*)
            smiGetIndexColumnFlags(aPartitionInfo->indices[i].indexHandle);

        i++;

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    // indexCount와 i는 같아야 한다.
    IDE_DASSERT( aPartitionInfo->indexCount == (UInt)i );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionIdByIndexName(
    smiStatement   * aSmiStmt,
    UInt             aIndexID,
    SChar          * aIndexPartName,
    UInt             aIndexPartNameLen,
    UInt           * aPartitionID )
{
/***********************************************************************
 *
 *  Description :
 *  PROJ-1502 PARTITIONED DISK TABLE
 *
 *  파티션드 인덱스의 IndexID와 인덱스 파티션의 IndexName으로
 *  tablePartID를 가져온다.
 *
 *  Implementation :
 *
 *
 ***********************************************************************/

    UInt                  sStage = 0;
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstRangeColumn;
    qtcMetaRangeColumn    sSecondRangeColumn;
    mtcColumn           * sIndexIDCol;
    mtcColumn           * sIndexPartNameCol;
    qcNameCharBuffer      sIndexValueBuffer;
    mtdCharType         * sIndexPartValue = (mtdCharType *) & sIndexValueBuffer;
    smiTableCursor        sCursor;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sPartIDMtcColumn;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sIndexPartNameCol )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sIndexPartValue,
                          NULL,
                          aIndexPartName,
                          aIndexPartNameLen );

    qcm::makeMetaRangeDoubleColumn(
        &sFirstRangeColumn,
        &sSecondRangeColumn,
        (const mtcColumn*) sIndexIDCol,
        (const void *) &aIndexID,
        (const mtcColumn*) sIndexPartNameCol,
        (const void *) sIndexPartValue,
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmIndexPartitions,
                 gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmIndexPartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_TABLE_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        sRow,
        sPartIDMtcColumn,
        aPartitionID );

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}


IDE_RC qcmPartition::getPartitionHandleByID( smiStatement  * aSmiStmt,
                                             UInt            aPartitionID,
                                             void         ** aPartitionHandle,
                                             smSCN         * aSCN,
                                             const void   ** aPartitionRow,
                                             const idBool    aTouchPartition )  /* default : ID_FALSE */
{
/***********************************************************************
 *
 *  Description : PROJ-1502 파티션 ID로 파티션 핸들을 구한다.
 *
 *  Implementation :
 *
 ***********************************************************************/

    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void          *sRow;
    smOID               sPartitionOID;
    ULong               sPartitionOIDULong;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn           *sQcmPartitionsIndexColumn;
    mtcColumn           *sPartitionOIDColInfo;
    qtcMetaRangeColumn  sRangeColumn;

    scGRID               sRid; // Disk Table을 위한 Record IDentifier
    smiCursorType        sCursorType;
    smiCursorProperties  sCursorProperty;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sQcmPartitionsIndexColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_OID_COL_ORDER,
                                  (const smiColumn**)&sPartitionOIDColInfo )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn *) sQcmPartitionsIndexColumn,
        (const void *) &aPartitionID,
        &sRange);

    // To fix BUG-17593
    // touchPartition을 할 때는 update cursor type
    // touchPartition을 안 할 때는 select cursor type
    if (aTouchPartition == ID_TRUE)
    {
        sFlag = (SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE);
        sCursorType = SMI_UPDATE_CURSOR;
    }
    else
    {
        sCursorType = SMI_SELECT_CURSOR;
    }

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 sCursorType,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRow == NULL, ERR_NOT_EXIST_PARTITION);

    if (aTouchPartition == ID_TRUE)
    {
        IDE_TEST(sCursor.updateRow(NULL) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    sPartitionOIDULong = *(ULong *)((UChar*) sRow +
                                    sPartitionOIDColInfo->column.offset);
    sPartitionOID = (smOID)sPartitionOIDULong;

    *aPartitionHandle = (void *)smiGetTable( sPartitionOID );

    *aSCN = smiGetRowSCN(*aPartitionHandle);

    if (aPartitionRow != NULL)
    {
        *aPartitionRow = sRow;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION, ""));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::makeAndSetQcmPartitionInfo( smiStatement * aSmiStmt,
                                                 UInt           aPartitionID,
                                                 smOID          aPartitionOID,
                                                 qcmTableInfo * aTableInfo,
                                                 const void *   aPartitionRow )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 파티션 메타 캐시 생성
 *
 *  Implementation :
 *
 ***********************************************************************/

    void             * sPartitionHandle;
    qcmTableInfo     * sPartitionInfo = NULL;
    smSCN              sSCN;
    mtdCharType      * sPartitionNameStr;
    mtcColumn        * sPartitionNameCol;
    mtcColumn        * sTBSIDCol;
    mtdCharType      * sAccessOptionStr;    /* PROJ-2359 Table/Partition Access Option */
    mtcColumn        * sAccessOptionCol;    /* PROJ-2359 Table/Partition Access Option */
    const void       * sPartitionRow;
    UInt               i;
    UInt               sColumnID = 0;

    smiTableSpaceAttr  sTBSAttr;

    /* TASK-7307 DML Data Consistency in Shard */
    mtcColumn        * sUsableCol;
    mtdCharType      * sUsableStr;

    mtcColumn        * sPartitionReplicationCountCol;
    mtdIntegerType     sPartitionReplicationCount;

    mtcColumn        * sPartRecoveryReplCountCol;
    mtdIntegerType     sPartRecoveryReplCount;

    IDU_LIMITPOINT("qcmPartition::makeAndSetQcmPartitionInfo::malloc1");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF(qcmTableInfo),
                               (void**)&sPartitionInfo)
             != IDE_SUCCESS);

    idlOS::memset( sPartitionInfo, 0x00, ID_SIZEOF(qcmTableInfo) );

    sPartitionInfo->tablePartitionType = QCM_TABLE_PARTITION;

    // PARTITION_ID
    sPartitionInfo->partitionID = aPartitionID;

    // in create db
    // create db할 때는 partition은 없다.
    IDE_ASSERT(gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER] != NULL);


    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sTBSIDCol )
              != IDE_SUCCESS );

    /* PROJ-2359 Table/Partition Access Option */
    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ACCESS_COL_ORDER,
                                  (const smiColumn**)&sAccessOptionCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_USABLE_COL_ORDER,
                                  (const smiColumn**)&sUsableCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_REPLICATION_COUNT_COL_ORDER,
                                  (const smiColumn**)&sPartitionReplicationCountCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_REPL_RECOVERY_COUNT_COL_ORDER,
                                  (const smiColumn**)&sPartRecoveryReplCountCol )
              != IDE_SUCCESS );

    if (aPartitionRow == NULL)
    {
        IDE_TEST(getPartitionHandleByID(aSmiStmt,
                                        aPartitionID,
                                        &sPartitionHandle,
                                        &sSCN,
                                        &sPartitionRow) != IDE_SUCCESS);

        aPartitionRow = sPartitionRow;
    }
    else
    {
        // Nothing to do.
    }

    // table ID
    sPartitionInfo->tableID = aTableInfo->tableID;
    
    // table Owner ID
    sPartitionInfo->tableOwnerID = aTableInfo->tableOwnerID;

    // maxrows
    // 사용하지 않음

    // PCTFREE, PCTUSED
    // 사용하지 않음

    // PROJ-1071 Parallel query
    sPartitionInfo->parallelDegree = aTableInfo->parallelDegree;

    // table Type
    sPartitionInfo->tableType = aTableInfo->tableType;
    sPartitionInfo->status = aTableInfo->status;

    qcm::setOperatableFlag( sPartitionInfo->tableType,
                            &sPartitionInfo->operatableFlag );

    //to fix BUG-19476
    sPartitionInfo->partitionMethod = aTableInfo->partitionMethod;

    // partition tablespace_id
    sPartitionInfo->TBSID = qcm::getSpaceID( aPartitionRow, sTBSIDCol->column.offset );

    // set TablespaceType
    IDE_TEST( qcmTablespace::getTBSAttrByID( sPartitionInfo->TBSID,
                                             & sTBSAttr )
              != IDE_SUCCESS);

    sPartitionInfo->TBSType = sTBSAttr.mType;

    // partition name
    sPartitionNameStr = (mtdCharType*)
        ((UChar*)aPartitionRow + sPartitionNameCol->column.offset);

    if ( sPartitionNameStr->length > 0 )
    {
        idlOS::memcpy(sPartitionInfo->name,
                      sPartitionNameStr->value,
                      sPartitionNameStr->length);

        sPartitionInfo->name[sPartitionNameStr->length] = '\0';
    }
    else
    {
        // BUG-47599
        sPartitionInfo->name[0] = '\0';
    }

    /* PROJ-2359 Table/Partition Access Option */
    sAccessOptionStr = (mtdCharType*)
        ((UChar*)aPartitionRow + sAccessOptionCol->column.offset);

    switch ( sAccessOptionStr->value[0] )
    {
        case 'R' :
            sPartitionInfo->accessOption = QCM_ACCESS_OPTION_READ_ONLY;
            break;

        case 'W' :
            sPartitionInfo->accessOption = QCM_ACCESS_OPTION_READ_WRITE;
            break;

        case 'A' :
            sPartitionInfo->accessOption = QCM_ACCESS_OPTION_READ_APPEND;
            break;

        default :
            ideLog::log( IDE_QP_0, "[FAILURE] make qcmTableInfo : unknown PARTITION_ACCESS !!!\n" );
            IDE_RAISE( ERR_META_CRASH );
    }

    // TASK-7307 DML Data Consistency in Shard
    sUsableStr = (mtdCharType *)
                 ((UChar *)aPartitionRow + sUsableCol->column.offset);

    if ( sUsableStr->value[0] == 'Y' )
    {
        sPartitionInfo->mIsUsable = ID_TRUE;
    }
    else if ( sUsableStr->value[0] == 'N' )
    {
        sPartitionInfo->mIsUsable = ID_FALSE;
    }
    else
    {
        ideLog::log( IDE_QP_0, "[FAILURE] make qcmTableInfo : unknown PARTITION_USABLE !!!\n");
        IDE_RAISE( ERR_META_CRASH );
    }

    sPartitionReplicationCount = *(mtdIntegerType*)
            ( (UChar*)aPartitionRow + sPartitionReplicationCountCol->column.offset );

    sPartitionInfo->replicationCount = (UInt)sPartitionReplicationCount;

    sPartRecoveryReplCount = *(mtdIntegerType*)
            ( (UChar*)aPartitionRow + sPartRecoveryReplCountCol->column.offset );

    sPartitionInfo->replicationRecoveryCount = (UInt)sPartRecoveryReplCount;

    // partition handle
    sPartitionInfo->tableHandle = (void *)smiGetTable( aPartitionOID );

    // 기타 자주 사용하는 정보
    sPartitionInfo->tableOID     = aPartitionOID;
    sPartitionInfo->tableFlag    = smiGetTableFlag( sPartitionInfo->tableHandle );
    sPartitionInfo->isDictionary = smiIsDictionaryTable( sPartitionInfo->tableHandle );
    sPartitionInfo->viewReadOnly = QCM_VIEW_NON_READ_ONLY;

    sPartitionInfo->columnCount = aTableInfo->columnCount;

    // partition의 column정보 세팅
    IDE_TEST( getQcmPartitionColumn( aTableInfo, sPartitionInfo )
              != IDE_SUCCESS );

    // get local index count
    sPartitionInfo->indexCount = smiGetTableIndexCount(sPartitionInfo->tableHandle);

    // get local index info
    IDE_TEST(getQcmLocalIndices( aSmiStmt,
                                 aTableInfo,
                                 sPartitionInfo )
             != IDE_SUCCESS);

    // get constraint info
    IDE_TEST( getQcmPartitionConstraints( aTableInfo,
                                          sPartitionInfo )
              != IDE_SUCCESS );

    // partition key
    IDU_FIT_POINT("qcmPartition::makeAndSetQcmPartitionInfo::malloc2");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM, 1,
                                 ID_SIZEOF(qcmColumn) * aTableInfo->partKeyColCount,
                                 (void**)&(sPartitionInfo->partKeyColumns) )
              != IDE_SUCCESS );

    IDU_FIT_POINT("qcmPartition::makeAndSetQcmPartitionInfo::malloc3");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM, 1,
                                 ID_SIZEOF(mtcColumn) * aTableInfo->partKeyColCount,
                                 (void**)&(sPartitionInfo->partKeyColBasicInfo) )
              != IDE_SUCCESS );

    for( i = 0; i < aTableInfo->partKeyColCount - 1; i++)
    {
        sColumnID = aTableInfo->partKeyColumns[i].basicInfo->column.id & SMI_COLUMN_ID_MASK;
        sPartitionInfo->partKeyColBasicInfo[i] = *(sPartitionInfo->columns[sColumnID].basicInfo);
        sPartitionInfo->partKeyColumns[i]      = sPartitionInfo->columns[sColumnID];
        sPartitionInfo->partKeyColumns[i].next = &sPartitionInfo->partKeyColumns[i + 1];
    }
    sColumnID = aTableInfo->partKeyColumns[i].basicInfo->column.id & SMI_COLUMN_ID_MASK;
    sPartitionInfo->partKeyColBasicInfo[i] = *(sPartitionInfo->columns[sColumnID].basicInfo);
    sPartitionInfo->partKeyColumns[i]      = sPartitionInfo->columns[sColumnID];
    sPartitionInfo->partKeyColumns[i].next = NULL;

    sPartitionInfo->partKeyColCount = aTableInfo->partKeyColCount;

    // PROJ-1407 Temporary Table
    sPartitionInfo->temporaryInfo.type = QCM_TEMPORARY_ON_COMMIT_NONE;

    // set partition info.
    smiSetTableTempInfo( sPartitionInfo->tableHandle,
                         (void*)sPartitionInfo );

    /* BUG-42928 No Partition Lock */
    if ( QCU_TABLE_LOCK_MODE != 0 )
    {
        smiSetTableReplacementLock( sPartitionInfo->tableHandle,
                                    aTableInfo->tableHandle );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    if (sPartitionInfo != NULL)
    {
        (void)destroyQcmPartitionInfo(sPartitionInfo);
    }
    return IDE_FAILURE;
}

IDE_RC qcmPartition::makeAndSetAndGetQcmPartitionInfoList( qcStatement           * aStatement,
                                                           qcmTableInfo          * aTableInfo,
                                                           qcmPartitionInfoList  * aPartInfoList,
                                                           qcmPartitionInfoList ** aNewPartInfoList )

{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Partition Info List에 대해 Meta Cache를 갱신한다. (Touch 제외)
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmPartitionInfoList * sNewPartInfoList = NULL;
    qcmPartitionInfoList * sPartInfoList    = NULL;
    UInt                   sPartCount       = 0;
    UInt                   i                = 0;

    if ( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        for ( sPartInfoList = aPartInfoList;
              sPartInfoList != NULL;
              sPartInfoList = sPartInfoList->next )
        {
            sPartCount++;
        }

        if ( sPartCount > 0 )
        {
            IDU_FIT_POINT( "qcmPartition::makeAndSetAndGetQcmPartitionInfoList::cralloc::sNewPartInfoList",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(qcmPartitionInfoList) * sPartCount,
                                                     (void **) & sNewPartInfoList )
                      != IDE_SUCCESS );
            for ( i = 0; i < sPartCount; i++ )
            {
                sNewPartInfoList[i].partitionInfo = NULL;
                sNewPartInfoList[i].next          = NULL;
            }

            for ( sPartInfoList = aPartInfoList, i = 0;
                  sPartInfoList != NULL;
                  sPartInfoList = sPartInfoList->next, i++ )
            {
                IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo( QC_SMI_STMT( aStatement ),
                                                                    sPartInfoList->partitionInfo->partitionID,
                                                                    smiGetTableId( sPartInfoList->partHandle ),
                                                                    aTableInfo,
                                                                    NULL )
                          != IDE_SUCCESS );

                IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                          aTableInfo->tableID,
                                                          (UChar *)sPartInfoList->partitionInfo->name,
                                                          (SInt)idlOS::strlen( sPartInfoList->partitionInfo->name ),
                                                          & sNewPartInfoList[i].partitionInfo,
                                                          & sNewPartInfoList[i].partSCN,
                                                          & sNewPartInfoList[i].partHandle )
                          != IDE_SUCCESS );

                if ( i > 0 )
                {
                    sNewPartInfoList[i - 1].next = & sNewPartInfoList[i];
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

    *aNewPartInfoList = sNewPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void)destroyQcmPartitionInfoList( sNewPartInfoList );

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionInfoList(
    qcStatement           * aStatement,
    smiStatement          * aSmiStmt,
    iduVarMemList         * aMem,
    UInt                    aTableID,
    qcmPartitionInfoList ** aPartInfoList,
    idBool                  aIsRealPart )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *     테이블 ID를 이용해서 해당하는 모든 테이블 파티션 ID를 찾고,
 *     그 ID로 파티션 메타 정보를 가져온다.
 *
 *     파티션 Handle과 SCN도 PartInfoList에 달아놓는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange                  sRange;
    qtcMetaRangeColumn        sRangeColumn;
    mtcColumn               * sTableIDCol;
    mtcColumn               * sPartitionIDCol;
    UInt                      sPartitionID;
    qcmTableInfo            * sTableInfo;
    smSCN                     sSCN;
    void                    * sTableHandle;
    smiTableCursor            sCursor;
    scGRID                    sRid; // Disk Table을 위한 Record IDentifier
    SInt                      sStage  = 0;
    const void              * sTableRow;
    const void              * sRow;

    qcmTableInfo            * sPartitionInfo;
    void                    * sPartitionHandle;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmPartitionInfoList    * sFirstPartInfoList = NULL;
    smiCursorProperties       sCursorProperty;
    /* BUG-47599 */
    mtcColumn               * sPartitionNameCol;
    mtdCharType             * sPartitionNameStr;

    IDE_TEST( qcm::getTableHandleByID(
                  aSmiStmt,
                  aTableID,
                  &sTableHandle,
                  &sSCN,
                  &sTableRow ) != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDCol,
        (const void *) &aTableID,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    // row를 하나씩 읽어와서 partitionInfo를 생성해준다.
    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);

    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sPartitionID = *(UInt *)((UChar*) sRow +
                                 sPartitionIDCol->column.offset);

        // BUG-47599 replItem에 empty partition이 포함되면 안됩니다.
        if ( aIsRealPart == ID_TRUE )
        {
            // partition name
            sPartitionNameStr = (mtdCharType*)
                ((UChar*) sRow + sPartitionNameCol->column.offset);

            if ( sPartitionNameStr->length == 0 )
            {
                IDE_TEST( sCursor.readRow( &sRow,
                                           &sRid,
                                           SMI_FIND_NEXT)
                          != IDE_SUCCESS );
                continue;
            }
        }

        // 파티션 ID로 파티션 메타 정보를 가져온다.
        IDE_TEST( qcmPartition::getPartitionInfoByID( aStatement,
                                                      sPartitionID,
                                                      & sPartitionInfo,
                                                      & sSCN,
                                                      & sPartitionHandle )
                  != IDE_SUCCESS );

        // 파티션 메타 정보 리스트 구축
        IDU_LIMITPOINT("qcmPartition::getPartitionInfoList::malloc1");
        IDE_TEST(aMem->alloc(ID_SIZEOF(qcmPartitionInfoList),
                             (void**)&(sPartInfoList))
                 != IDE_SUCCESS);

        // -------------------------------------------
        // sPartInfoList 구성
        // -------------------------------------------
        sPartInfoList->partitionInfo = sPartitionInfo;
        sPartInfoList->partHandle = sPartitionHandle;
        sPartInfoList->partSCN = sSCN;

        if (sFirstPartInfoList == NULL)
        {
            sPartInfoList->next = NULL;
            sFirstPartInfoList = sPartInfoList;
        }
        else
        {
            sPartInfoList->next = sFirstPartInfoList;
            sFirstPartInfoList = sPartInfoList;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE( sFirstPartInfoList == NULL,
                    ERR_FIND_PART_INFO );    
    
    *aPartInfoList = sFirstPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIND_PART_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPartition::getPartitionInfoList",
                                  "could not find partition info" ));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionInfoList(
    qcStatement           * aStatement,
    smiStatement          * aSmiStmt,
    iduMemory             * aMem,
    UInt                    aTableID,
    qcmPartitionInfoList ** aPartInfoList,
    idBool                  aIsRealPart )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *     테이블 ID를 이용해서 해당하는 모든 테이블 파티션 ID를 찾고,
 *     그 ID로 파티션 메타 정보를 가져온다.
 *
 *     파티션 Handle과 SCN도 PartInfoList에 달아놓는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange                  sRange;
    qtcMetaRangeColumn        sRangeColumn;
    mtcColumn               * sTableIDCol;
    mtcColumn               * sPartitionIDCol;
    UInt                      sPartitionID;
    qcmTableInfo            * sTableInfo;
    smSCN                     sSCN;
    void                    * sTableHandle;
    smiTableCursor            sCursor;
    scGRID                    sRid; // Disk Table을 위한 Record IDentifier
    SInt                      sStage  = 0;
    const void              * sTableRow;
    const void              * sRow;

    qcmTableInfo            * sPartitionInfo;
    void                    * sPartitionHandle;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmPartitionInfoList    * sFirstPartInfoList = NULL;
    smiCursorProperties       sCursorProperty;
    /* BUG-47599 */
    mtcColumn               * sPartitionNameCol;
    mtdCharType             * sPartitionNameStr;

    IDE_TEST( qcm::getTableHandleByID(
                  aSmiStmt,
                  aTableID,
                  &sTableHandle,
                  &sSCN,
                  &sTableRow ) != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDCol,
        (const void *) &aTableID,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    // row를 하나씩 읽어와서 partitionInfo를 생성해준다.
    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);

    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sPartitionID = *(UInt *)((UChar*) sRow +
                                 sPartitionIDCol->column.offset);

        // BUG-47599 replItem에 empty partition이 포함되면 안됩니다.
        if ( aIsRealPart == ID_TRUE )
        {
            // partition name
            sPartitionNameStr = (mtdCharType*)
                ((UChar*) sRow + sPartitionNameCol->column.offset);

            if ( sPartitionNameStr->length == 0 )
            {
                IDE_TEST( sCursor.readRow( &sRow,
                                           &sRid,
                                           SMI_FIND_NEXT )
                          != IDE_SUCCESS );
                continue;
            }
        }

        // 파티션 ID로 파티션 메타 정보를 가져온다.
        IDE_TEST( qcmPartition::getPartitionInfoByID( aStatement,
                                                      sPartitionID,
                                                      & sPartitionInfo,
                                                      & sSCN,
                                                      & sPartitionHandle )
                  != IDE_SUCCESS );

        // 파티션 메타 정보 리스트 구축
        IDU_LIMITPOINT("qcmPartition::getPartitionInfoList::malloc2");
        IDE_TEST(aMem->alloc(ID_SIZEOF(qcmPartitionInfoList),
                             (void**)&(sPartInfoList))
                 != IDE_SUCCESS);

        // -------------------------------------------
        // sPartInfoList 구성
        // -------------------------------------------
        sPartInfoList->partitionInfo = sPartitionInfo;
        sPartInfoList->partHandle = sPartitionHandle;
        sPartInfoList->partSCN = sSCN;

        if (sFirstPartInfoList == NULL)
        {
            sPartInfoList->next = NULL;
            sFirstPartInfoList = sPartInfoList;
        }
        else
        {
            sPartInfoList->next = sFirstPartInfoList;
            sFirstPartInfoList = sPartInfoList;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE( sFirstPartInfoList == NULL,
                    ERR_FIND_PART_INFO );    
    
    *aPartInfoList = sFirstPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIND_PART_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPartition::getPartitionInfoList",
                                  "could not find partition info" ));
    }
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionIdList(
    qcStatement         * aStatement,
    smiStatement        * aSmiStmt,
    UInt                  aTableID,
    qcmPartitionIdList ** aPartIdList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *     테이블 ID를 이용해서 해당하는 모든 테이블 파티션 ID의 리스트를
 *     가져온다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange                  sRange;
    qtcMetaRangeColumn        sRangeColumn;
    mtcColumn               * sTableIDCol;
    mtcColumn               * sPartitionIDCol;
    UInt                      sPartitionID;
    smSCN                     sSCN;
    void                    * sTableHandle;
    smiTableCursor            sCursor;
    scGRID                    sRid; // Disk Table을 위한 Record IDentifier
    SInt                      sStage  = 0;
    const void              * sTableRow;
    const void              * sRow;
    qcmPartitionIdList      * sPartIdList = NULL;
    qcmPartitionIdList      * sFirstPartIdList = NULL;
    smiCursorProperties       sCursorProperty;

    IDE_TEST( qcm::getTableHandleByID(
                  aSmiStmt,
                  aTableID,
                  &sTableHandle,
                  &sSCN,
                  &sTableRow ) != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sTableIDCol,
        (const void *) &aTableID,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    // row를 하나씩 읽어와서 partitionInfo를 생성해준다.
    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);

    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if (sRow == NULL)
    {
        *aPartIdList = NULL;
        IDE_DASSERT( 0 );
    }

    while (sRow != NULL)
    {
        sPartitionID = *(UInt *)((UChar*) sRow +
                                 sPartitionIDCol->column.offset);

        IDU_LIMITPOINT("qcmPartition::getPartitionIdList::malloc");
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                              qcmPartitionIdList,
                              & sPartIdList)
                 != IDE_SUCCESS);

        // -------------------------------------------
        // aPartIdList 구성
        // -------------------------------------------
        sPartIdList->partId = sPartitionID;

        if (sFirstPartIdList == NULL)
        {
            sPartIdList->next = NULL;
            sFirstPartIdList = sPartIdList;
        }
        else
        {
            sPartIdList->next = sFirstPartIdList;
            sFirstPartIdList = sPartIdList;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aPartIdList = sFirstPartIdList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartMinMaxValue(
    smiStatement    * aSmiStmt,
    UInt              aPartitionID,
    mtdCharType     * aPartKeyCondMinValue,
    mtdCharType     * aPartKeyCondMaxValue )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      ALTER TABLE ... PARTITION ... 로부터 호출된다.
 *
 *      해당 파티션의 PARTITION_MIN_VALUE와 PARTITION_MAX_VALUE를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sPartitionIDCol;
    smiTableCursor        sCursor;
    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    SInt                  sStage  = 0;
    const void          * sRow;
    const mtdCharType   * sPartCondMinVal;
    const mtdCharType   * sPartCondMaxVal;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sPartMinValueMtcColumn;
    mtcColumn           * sPartMaxValueMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sPartitionIDCol,
        (const void *) &aPartitionID,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);

    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if( sRow != NULL )
    {
        // ---------------------------------------------------
        // PARTITION_MIN_VALUE
        // ---------------------------------------------------
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_MIN_VALUE_COL_ORDER,
                                      (const smiColumn**)&sPartMinValueMtcColumn )
                  != IDE_SUCCESS );

        sPartCondMinVal = (const mtdCharType *)
            mtc::value( sPartMinValueMtcColumn,
                        sRow,
                        MTD_OFFSET_USE );

        // ---------------------------------------------------
        // PARTITION_MAX_VALUE
        // ---------------------------------------------------
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER,
                                      (const smiColumn**)&sPartMaxValueMtcColumn )
                  != IDE_SUCCESS );

        sPartCondMaxVal = (const mtdCharType *)
            mtc::value( sPartMaxValueMtcColumn,
                        sRow,
                        MTD_OFFSET_USE );

        IDE_ASSERT( sPartCondMinVal->length <= QC_MAX_PARTKEY_COND_VALUE_LEN );
        IDE_ASSERT( sPartCondMaxVal->length <= QC_MAX_PARTKEY_COND_VALUE_LEN );

        // 복사한다.
        if ( sPartCondMinVal->length > 0 )
        {
            idlOS::memcpy( aPartKeyCondMinValue->value,
                           sPartCondMinVal->value,
                           sPartCondMinVal->length );
        }
        else
        {
            // Nothing to do.
        }

        if ( sPartCondMaxVal->length > 0 )
        {
            idlOS::memcpy( aPartKeyCondMaxValue->value,
                           sPartCondMaxVal->value,
                           sPartCondMaxVal->length );
        }
        else
        {
            // Nothing to do.
        }
        
        aPartKeyCondMinValue->length = sPartCondMinVal->length;
        aPartKeyCondMaxValue->length = sPartCondMaxVal->length;
    }
    else
    {
        aPartKeyCondMinValue->length = 0;
        aPartKeyCondMaxValue->length = 0;
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartMinMaxValue(
    smiStatement    * aSmiStmt,
    UInt              aPartitionID,
    SChar           * aPartKeyCondMinValueStr,
    SChar           * aPartKeyCondMaxValueStr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      ALTER TABLE ... PARTITION ... 로부터 호출된다.
 *
 *      해당 파티션의 PARTITION_MIN_VALUE와 PARTITION_MAX_VALUE를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sPartitionIDCol;
    smiTableCursor        sCursor;
    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    SInt                  sStage  = 0;
    const void          * sRow;
    const mtdCharType   * sPartCondMinVal;
    const mtdCharType   * sPartCondMaxVal;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sPartMinValueMtcColumn;
    mtcColumn           * sPartMaxValueMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sPartitionIDCol,
        (const void *) &aPartitionID,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);

    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if( sRow != NULL )
    {
        // ---------------------------------------------------
        // PARTITION_MIN_VALUE
        // ---------------------------------------------------
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_MIN_VALUE_COL_ORDER,
                                      (const smiColumn**)&sPartMinValueMtcColumn )
                  != IDE_SUCCESS );

        sPartCondMinVal = (const mtdCharType *)
            mtc::value( sPartMinValueMtcColumn,
                        sRow,
                        MTD_OFFSET_USE );

        // ---------------------------------------------------
        // PARTITION_MAX_VALUE
        // ---------------------------------------------------
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_MAX_VALUE_COL_ORDER,
                                      (const smiColumn**)&sPartMaxValueMtcColumn )
                  != IDE_SUCCESS );

        sPartCondMaxVal = (const mtdCharType *)
            mtc::value( sPartMaxValueMtcColumn,
                        sRow,
                        MTD_OFFSET_USE );

        IDE_ASSERT( sPartCondMinVal->length <= QC_MAX_PARTKEY_COND_VALUE_LEN );
        IDE_ASSERT( sPartCondMaxVal->length <= QC_MAX_PARTKEY_COND_VALUE_LEN );
        
        // 복사한다.
        if( sPartCondMinVal->length > 0 )
        {
            idlOS::memcpy( aPartKeyCondMinValueStr,
                           sPartCondMinVal->value,
                           sPartCondMinVal->length );
        }
        else
        {
            // Nothing to do.
        }

        if( sPartCondMaxVal->length > 0 )
        {
            idlOS::memcpy( aPartKeyCondMaxValueStr,
                           sPartCondMaxVal->value,
                           sPartCondMaxVal->length );
        }
        else
        {
            // Nothing to do.
        }
        
        aPartKeyCondMinValueStr[sPartCondMinVal->length] = '\0';
        aPartKeyCondMaxValueStr[sPartCondMaxVal->length] = '\0';
    }
    else
    {
        aPartKeyCondMinValueStr[0] = '\0';
        aPartKeyCondMaxValueStr[0] = '\0';
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::validateAndLockTableAndPartitions(
    qcStatement          * aStatement,
    void                 * aPartTableHandle,
    smSCN                  aPartTableSCN,
    qcmPartitionInfoList * aPartInfoList,
    smiTableLockMode       aLockMode,
    idBool                 aIsSetViewSCN )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      파티션드 테이블에 LOCK(IX), BUG-42329:( SPLIT, MERGE LOCK(X) )
 *      aPartInfoList에 있는 모든 파티션에 LOCK(X)
 *
 *      이 함수가 호출되는 경우는
 *          ALTER TABLE ~ SPLIT     PARTITION
 *          ALTER TABLE ~ MERGE     PARTITIONS
 *          ALTER TABLE ~ DROP      PARTITION
 *          ALTER TABLE ~ RENAME    PARTITION
 *          ALTER TABLE ~ TRUNCATE  PARTITION
 *          ALTER TABLE ~ ACCESS    PARTITION
 *          ALTER TABLE ~ ALTER     PARTITION
 *          ALTER TABLE ~ COMPACT   PARTITION
 *          ALTER TABLE ~ AGING     PARTITION
 *          ALTER INDEX ~ REBUILD   PARTITION
 *          ALTER TABLE ~ ADD       PARTITION (non-hash partitioned)
 *      과 같이 파티션드 테이블에 LOCK(IX)이 잡히고,
 *      파티션에 LOCK(X)이 잡히는 경우이다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    smiTableLockMode       sPartTableLockMode = SMI_TABLE_LOCK_IX;

    /* BUG-42928 No Partition Lock */
    if ( QCU_TABLE_LOCK_MODE == 0 )
    {
        sPartTableLockMode = aLockMode;
    }
    else
    {
        sPartTableLockMode = SMI_TABLE_LOCK_X;
    }

    IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                        aPartTableHandle,
                                        aPartTableSCN,
                                        SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                        sPartTableLockMode,
                                        smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                        ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
             != IDE_SUCCESS);
    
    // 각 파티션에 LOCK(X)
    IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                              aPartInfoList,
                                                              SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                              SMI_TABLE_LOCK_X,
                                                              smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    // BUG-43292 DDL / DDL partiton(SPLIT, MERGE ADD, COALESCE)
    // 연산시 최신 viewSCN을 보지 못함
    if ( aIsSetViewSCN == ID_TRUE )
    {
        IDE_TEST( smiStatement::setViewSCNOfAllStmt( QC_SMI_STMT( aStatement ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartitionOrder( smiStatement * aSmiStmt,
                                        UInt           aTableID,
                                        UChar        * aPartitionName,
                                        SInt           aPartitionNameSize,
                                        UInt         * aPartOrder )
{
/***********************************************************************
 *
 *  Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *  Implementation :
 *
 ***********************************************************************/
    UInt                  sStage = 0;
    smiRange              sRange;
    smiTableCursor        sCursor;
    const void          * sRow;
    UInt                  sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn           * sTableIDCol;
    mtcColumn           * sPartitionNameCol;
    mtcColumn           * sPartitionOrderCol;
    qtcMetaRangeColumn    sFirstRangeColumn;
    qtcMetaRangeColumn    sSecondRangeColumn;

    qcNameCharBuffer      sPartitionNameBuffer;
    mtdCharType         * sPartitionName = ( mtdCharType * ) & sPartitionNameBuffer;

    scGRID                sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties   sCursorProperty;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ORDER_COL_ORDER,
                                  (const smiColumn**)&sPartitionOrderCol )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sPartitionName,
                          NULL,
                          (SChar*)aPartitionName,
                          aPartitionNameSize );

    qcm::makeMetaRangeDoubleColumn(
        &sFirstRangeColumn,
        &sSecondRangeColumn,
        (const mtcColumn *) sTableIDCol,
        (const void *) &aTableID,
        (const mtcColumn *) sPartitionNameCol,
        (const void *) sPartitionName,
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX2_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    IDE_DASSERT( sRow != NULL );

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aPartOrder = *(UInt *)((UChar*) sRow +
                            sPartitionOrderCol->column.offset);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getTableIDByPartitionID(
    smiStatement          * aSmiStmt,
    UInt                    aPartitionID,
    UInt                  * aTableID)
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      파티션 이름으로 파티션드 테이블의 ID를 찾아온다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt                  sStage = 0;
    smiRange              sRange;
    qtcMetaRangeColumn    sRangeColumn;
    mtcColumn           * sPartitionIDCol;
    smiTableCursor        sCursor;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sCursorProperty;
    mtcColumn           * sPartTableIDMtcColumn;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sPartitionIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn*) sPartitionIDCol,
        (const void *) & aPartitionID,
        &sRange);

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmTablePartitions,
                 gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER],
                 smiGetRowSCN(gQcmTablePartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    // BUG-45459
    IDE_TEST_RAISE ( sRow == NULL, ERR_META_CRASH );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sPartTableIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        sRow,
        sPartTableIDMtcColumn,
        aTableID );

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC
qcmPartition::touchPartition(smiStatement *  aSmiStmt ,
                             UInt            aPartitionID)
{
    const void * sTouchedPartHandle;
    smSCN        sSCN;

    IDE_TEST(getPartitionHandleByID(aSmiStmt,
                                   aPartitionID,
                                   (void **) &sTouchedPartHandle,
                                   &sSCN,
                                   NULL,
                                   ID_TRUE)
             != IDE_SUCCESS);

    IDE_TEST(smiTable::touchTable(aSmiStmt,
                                  sTouchedPartHandle,
                                  SMI_TBSLV_DDL_DML )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::touchPartitionInfoList( smiStatement         * aSmiStmt,
                                             qcmPartitionInfoList * aPartInfoList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Partition Info List에 대해 Touch를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmPartitionInfoList * sPartInfoList = NULL;

    for ( sPartInfoList = aPartInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        IDE_TEST( touchPartition( aSmiStmt,
                                  sPartInfoList->partitionInfo->partitionID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getPartIdxFromIdxId( UInt           aIndexId,
                                          qmsTableRef  * aTableRef,
                                          qcmIndex    ** aPartIdx )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *     Index ID로 Partitioned Index를 찾아서 리턴.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmIndex * sIndices    = NULL;
    UInt       sIndexCount = 0;
    UInt       i           = 0;

    sIndices    = aTableRef->tableInfo->indices;
    sIndexCount = aTableRef->tableInfo->indexCount;

    *aPartIdx = NULL;

    for ( i = 0; i < sIndexCount; i++ )
    {
        if ( sIndices[i].indexId == aIndexId )
        {
            *aPartIdx = &(sIndices[i]);
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_ASSERT( *aPartIdx != NULL );

    return IDE_SUCCESS;
}

IDE_RC
qcmPartition::searchTablePartitionID( smiStatement * aSmiStmt,
                                      SInt           aTablePartitionID,
                                      idBool       * aExist )
{
/***********************************************************************
 *
 * Description : table partition id가 존재하는지 검사.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void          *sRow;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn           *sQcmIndexColumn;
    qtcMetaRangeColumn  sRangeColumn;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    if( gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER] == NULL )
    {
        // createdb하는 경우임.
        // 이때는 검사 할 필요가 없다
        *aExist = ID_FALSE;
    }
    else
    {
        sCursor.initialize();

        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                      (const smiColumn**)&sQcmIndexColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn(
            &sRangeColumn,
            (const mtcColumn *) sQcmIndexColumn,
            (const void *) &aTablePartitionID,
            &sRange);

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST(sCursor.open(
                     aSmiStmt,
                     gQcmTablePartitions,
                     gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX1_ORDER],
                     smiGetRowSCN(gQcmTablePartitions),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     sFlag,
                     SMI_SELECT_CURSOR,
                     &sCursorProperty) != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        if( sRow == NULL )
        {
            *aExist = ID_FALSE;
        }
        else
        {
            *aExist = ID_TRUE;
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}


    
IDE_RC
qcmPartition::searchIndexPartitionID( smiStatement * aSmiStmt,
                                      SInt           aIndexPartitionID,
                                      idBool       * aExist )
{
/***********************************************************************
 *
 * Description : index partition id가 존재하는지 검사.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void          *sRow;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn           *sQcmIndexColumn;
    qtcMetaRangeColumn  sRangeColumn;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    if( gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX1_ORDER] == NULL )
    {
        // createdb하는 경우임.
        // 이때는 검사 할 필요가 없다
        *aExist = ID_FALSE;
    }
    else
    {
        sCursor.initialize();

        IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                      QCM_INDEX_PARTITIONS_INDEX_PARTITION_ID_COL_ORDER,
                                      (const smiColumn**)&sQcmIndexColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn(
            &sRangeColumn,
            (const mtcColumn *) sQcmIndexColumn,
            (const void *) &aIndexPartitionID,
            &sRange);

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

        IDE_TEST(sCursor.open(
                     aSmiStmt,
                     gQcmIndexPartitions,
                     gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX1_ORDER],
                     smiGetRowSCN(gQcmIndexPartitions),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     sFlag,
                     SMI_SELECT_CURSOR,
                     &sCursorProperty) != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        if( sRow == NULL )
        {
            *aExist = ID_FALSE;
        }
        else
        {
            *aExist = ID_TRUE;
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC
qcmPartition::copyPartitionRef( qcStatement      * aStatement,
                                qmsPartitionRef  * aSource,
                                qmsPartitionRef ** aDestination )
{
/***********************************************************************
 *
 * Description : partitionRef list의 복사
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPartitionRef  * sPrevPartRef = NULL;
    qmsPartitionRef  * sCurrPartRef;
    qmsPartitionRef  * sPartRef;

    *aDestination = NULL;
    
    for ( sPartRef = aSource; sPartRef != NULL; sPartRef = sPartRef->next )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmsPartitionRef),
                                                 (void**) & sCurrPartRef )
                  != IDE_SUCCESS );

        idlOS::memcpy( sCurrPartRef, sPartRef, ID_SIZEOF(qmsPartitionRef) );

        if ( sPrevPartRef != NULL )
        {
            sPrevPartRef->next = sCurrPartRef;
            sPrevPartRef = sCurrPartRef;
        }
        else
        {
            *aDestination = sCurrPartRef;
        }
    }
            
    if ( sPrevPartRef != NULL )
    {
        sPrevPartRef->next = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Partition 요약 정보를 만든다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qcmPartition::makePartitionSummary( qcStatement * aStatement,
                                           qmsTableRef * aTableRef )
{
    qmsPartitionSummary * sPartitionSummary       = NULL;
    qmsPartitionRef     * sPartitionRef           = NULL;
    UInt                  sTotalPartitionCount    = 0;
    UInt                  sMemoryPartitionCount   = 0;
    UInt                  sVolatilePartitionCount = 0;
    UInt                  sDiskPartitionCount     = 0;

    if ( aTableRef->partitionSummary == NULL )
    {
        IDU_FIT_POINT( "qcmPartition::makePartitionSummary::cralloc::partitionSummary",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( ID_SIZEOF(qmsPartitionSummary),
                                                   (void **) & (aTableRef->partitionSummary) )
                  != IDE_SUCCESS );
    }
    else
    {
        idlOS::memset( aTableRef->partitionSummary, 0x00, ID_SIZEOF(qmsPartitionSummary) );
    }
    sPartitionSummary = aTableRef->partitionSummary;

    for ( sPartitionRef = aTableRef->partitionRef;
          sPartitionRef != NULL;
          sPartitionRef = sPartitionRef->next )
    {
        sTotalPartitionCount++;

        if ( smiTableSpace::isMemTableSpaceType( sPartitionRef->partitionInfo->TBSType ) == ID_TRUE )
        {
            sMemoryPartitionCount++;

            if ( sPartitionSummary->memoryOrVolatilePartitionRef == NULL )
            {
                sPartitionSummary->memoryOrVolatilePartitionRef = sPartitionRef;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else if ( smiTableSpace::isVolatileTableSpaceType( sPartitionRef->partitionInfo->TBSType ) == ID_TRUE )
        {
            sVolatilePartitionCount++;

            if ( sPartitionSummary->memoryOrVolatilePartitionRef == NULL )
            {
                sPartitionSummary->memoryOrVolatilePartitionRef = sPartitionRef;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            IDE_DASSERT( smiTableSpace::isDiskTableSpaceType( sPartitionRef->partitionInfo->TBSType ) == ID_TRUE );
            sDiskPartitionCount++;

            if ( sPartitionSummary->diskPartitionRef == NULL )
            {
                sPartitionSummary->diskPartitionRef = sPartitionRef;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    sPartitionSummary->memoryPartitionCount   = sMemoryPartitionCount;
    sPartitionSummary->volatilePartitionCount = sVolatilePartitionCount;
    sPartitionSummary->diskPartitionCount     = sDiskPartitionCount;

    if ( ( sTotalPartitionCount == sMemoryPartitionCount ) ||
         ( sTotalPartitionCount == sVolatilePartitionCount ) ||
         ( sTotalPartitionCount == sDiskPartitionCount ) )
    {
        if ( aTableRef->partitionRef != NULL )
        {
            if ( qdtCommon::isSameTBSType( aTableRef->tableInfo->TBSType,
                                           aTableRef->partitionRef->partitionInfo->TBSType )
                 == ID_TRUE )
            {
                sPartitionSummary->isHybridPartitionedTable = ID_FALSE;
            }
            else
            {
                sPartitionSummary->isHybridPartitionedTable = ID_TRUE;
            }
        }
        else
        {
            sPartitionSummary->isHybridPartitionedTable = ID_FALSE;
        }
    }
    else
    {
        sPartitionSummary->isHybridPartitionedTable = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::checkPartitionCount4Execute( qcStatement          * aStatement,
                                                  qcmPartitionInfoList * aPartInfoList,
                                                  UInt                   aTableID )
{
    qcmPartitionInfoList * sPartInfo;
    qcmPartitionInfoList * sTempPartInfo;
    UInt                   sOrgCount;
    UInt                   sNewCount;

    IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                  QC_SMI_STMT( aStatement ),
                                                  QC_QMX_MEM(aStatement),
                                                  aTableID,
                                                  & sTempPartInfo )
              != IDE_SUCCESS );

    for ( sPartInfo = aPartInfoList, sOrgCount = 0;
          sPartInfo != NULL;
          sPartInfo = sPartInfo->next, sOrgCount++ );

    for ( sPartInfo = sTempPartInfo, sNewCount = 0;
          sPartInfo != NULL;
          sPartInfo = sPartInfo->next, sNewCount++ );

    /* BUG-42681 valgrind split 중 add column 동시성 문제
     *
     * validate시에 가져온 parttion list와 execute 시에 가져온
     * partition list 숫자가 다르다면 error를 발생시킨다.
     */
    IDE_TEST_RAISE( sOrgCount != sNewCount, ERR_UNEXPECTED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPartition::checkPartition4Execute",
                                  "Invalid Partition Count" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::validateAndLockOnePartition( qcStatement         * aStatement,
                                                  const void          * aPartitionHandle,
                                                  smSCN                 aSCN,
                                                  smiTBSLockValidType   aTBSLvType,
                                                  smiTableLockMode      aLockMode,
                                                  ULong                 aLockWaitMicroSec )
{
    return smiValidateAndLockObjects( (QC_SMI_STMT(aStatement))->getTrans(),
                                      aPartitionHandle,
                                      aSCN,
                                      aTBSLvType,
                                      aLockMode,
                                      aLockWaitMicroSec,
                                      ID_FALSE ); // BUG-28752 isExplicitLock
}

IDE_RC qcmPartition::validateAndLockPartitionInfoList( qcStatement          * aStatement,
                                                       qcmPartitionInfoList * aPartInfoList,
                                                       smiTBSLockValidType    aTBSLvType,
                                                       smiTableLockMode       aLockMode,
                                                       ULong                  aLockWaitMicroSec )
{
    qcmPartitionInfoList * sPartInfoList = NULL;

    for ( sPartInfoList = aPartInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT(aStatement))->getTrans(),
                                             sPartInfoList->partHandle,
                                             sPartInfoList->partSCN,
                                             aTBSLvType,
                                             aLockMode,
                                             aLockWaitMicroSec,
                                             ID_FALSE ) // BUG-28752 isExplicitLock
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::getAllPartitionOID( iduMemory              * aMemory,
                                         qcmPartitionInfoList   * aPartInfoList,
                                         smOID                 ** aPartitionOID,
                                         UInt                   * aPartitionCount )
{
    UInt                      i = 0;
    UInt                      sPartitionCount = 0;
    smOID                   * sPartitionOID = NULL;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmTableInfo            * sPartInfo = NULL;

    for ( sPartInfoList = aPartInfoList; 
          sPartInfoList != NULL; 
          sPartInfoList = sPartInfoList->next )
    {
        sPartitionCount++;
    }

    IDU_FIT_POINT( "qcmPartition::getAllPartitionOID::alloc::sPartitionOID" );
    IDE_TEST( aMemory->alloc( ID_SIZEOF(smOID) *
                                 sPartitionCount,
                                 (void**)&sPartitionOID )
              != IDE_SUCCESS );


    for ( sPartInfoList = aPartInfoList; 
          sPartInfoList != NULL; 
          sPartInfoList = sPartInfoList->next, i++ )
    {   
        sPartInfo = sPartInfoList->partitionInfo;
        sPartitionOID[i] = smiGetTableId( sPartInfo->tableHandle );
    }

    *aPartitionOID = sPartitionOID;
    *aPartitionCount = sPartitionCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::findPartitionByName( qcmPartitionInfoList  * aPartInfoList,
                                          SChar                 * aPartitionName,
                                          UInt                    aNameLength,
                                          qcmPartitionInfoList ** aFoundPartition )
{
    qcmPartitionInfoList * sPartInfoList = NULL;

    for ( sPartInfoList = aPartInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        if ( idlOS::strMatch( aPartitionName,
                              aNameLength,
                              sPartInfoList->partitionInfo->name,
                              idlOS::strlen( sPartInfoList->partitionInfo->name ) ) == 0 )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aFoundPartition = sPartInfoList;

    return IDE_SUCCESS;
}

IDE_RC qcmPartition::isEmptyPartition( smiStatement * aSmiStmt,
                                       UInt           aTableID,
                                       idBool       * aIsEmptyPart )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    mtcColumn           * sFirstMtcCol;
    mtcColumn           * sSecondMtcCol;
    vSLong                sRowCount;
    idBool                sIsEmptyPart = ID_FALSE;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondMtcCol )
              != IDE_SUCCESS );

    /* mtdModule 설정 */
    IDE_TEST( mtd::moduleById( &(sFirstMtcCol->module),
                               sFirstMtcCol->type.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( mtd::moduleById( &(sSecondMtcCol->module),
                               sSecondMtcCol->type.dataTypeId )
              != IDE_SUCCESS );

    /* mtlModule 설정 */
    IDE_TEST( mtl::moduleById( &(sFirstMtcCol->language),
                               sFirstMtcCol->type.languageId )
              != IDE_SUCCESS );

    IDE_TEST( mtl::moduleById( &(sSecondMtcCol->language),
                               sSecondMtcCol->type.languageId )
              != IDE_SUCCESS );

    qtc::initializeMetaRange( &sRange,
                              MTD_COMPARE_MTDVAL_MTDVAL );  // Meta is memory table
    // tableID = ?
    qtc::setMetaRangeColumn( &sFirstMetaRange,
                             sFirstMtcCol,
                             (const void*) &aTableID,
                             SMI_COLUMN_ORDER_ASCENDING,
                             0 );  // First column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sFirstMetaRange );

    // PartitionName is null 
    qtc::setMetaRangeIsNullColumn( &sSecondMetaRange,
                                   sSecondMtcCol,
                                   1 );  // Second column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sSecondMetaRange );

    qtc::fixMetaRange( &sRange );

    IDE_TEST( qcm::selectCount( aSmiStmt,
                                gQcmTablePartitions,
                                &sRowCount,
                                smiGetDefaultFilter(),
                                & sRange,
                                gQcmTablePartitionsIndex
                                [QCM_TABLE_PARTITIONS_IDX2_ORDER] )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount > 1, ERR_UNEXPECTED );

    if ( sRowCount == 1 )
    {
        sIsEmptyPart = ID_TRUE;
    }
    else
    {
        sIsEmptyPart = ID_FALSE;
    }

    *aIsEmptyPart = sIsEmptyPart;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPartition::isEmptyPartition",
                                  "too many empty partition count." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmPartition::setEmptyPartitionRef( qcStatement  * aStatement,
                                           qmsTableRef  * aTableRef )
{
    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    mtcColumn           * sFirstMtcCol;
    mtcColumn           * sSecondMtcCol;
    vSLong                sRowCount;
    qcmSetRef             sQcmSetRef;

    sQcmSetRef.statement = aStatement;
    sQcmSetRef.tableRef  = aTableRef;

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sFirstMtcCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondMtcCol )
              != IDE_SUCCESS );

    /* mtdModule 설정 */
    IDE_TEST( mtd::moduleById( &(sFirstMtcCol->module),
                               sFirstMtcCol->type.dataTypeId )
              != IDE_SUCCESS );

    IDE_TEST( mtd::moduleById( &(sSecondMtcCol->module),
                               sSecondMtcCol->type.dataTypeId )
              != IDE_SUCCESS );

    /* mtlModule 설정 */
    IDE_TEST( mtl::moduleById( &(sFirstMtcCol->language),
                               sFirstMtcCol->type.languageId )
              != IDE_SUCCESS );

    IDE_TEST( mtl::moduleById( &(sSecondMtcCol->language),
                               sSecondMtcCol->type.languageId )
              != IDE_SUCCESS );

    qtc::initializeMetaRange( &sRange,
                              MTD_COMPARE_MTDVAL_MTDVAL );  // Meta is memory table
    // tableID = ?
    qtc::setMetaRangeColumn( &sFirstMetaRange,
                             sFirstMtcCol,
                             (const void*) &(aTableRef->tableInfo->tableID),
                             SMI_COLUMN_ORDER_ASCENDING,
                             0 );  // First column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sFirstMetaRange );

    // PartitionName is null
    qtc::setMetaRangeIsNullColumn( &sSecondMetaRange,
                                   sSecondMtcCol,
                                   1 );  // Second column of Index

    qtc::addMetaRangeColumn( &sRange,
                             &sSecondMetaRange );

    qtc::fixMetaRange( &sRange );

    IDE_TEST( qcm::selectRow( QC_SMI_STMT( aStatement ),
                              gQcmTablePartitions,
                              smiGetDefaultFilter(),
                              & sRange,
                              gQcmTablePartitionsIndex
                              [QCM_TABLE_PARTITIONS_IDX2_ORDER],
                              (qcmSetStructMemberFunc ) setPartitionRef,
                              &sQcmSetRef,
                              0,
                              ID_UINT_MAX,
                              & sRowCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount > 2, ERR_UNEXPECTED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmPartition::setEmptyPartitionRef",
                                  "too many empty partition count." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
