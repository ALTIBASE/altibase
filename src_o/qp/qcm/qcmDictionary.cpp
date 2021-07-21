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
 * $Id: qcmDictionary.cpp 91010 2021-06-17 01:33:11Z hykim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcpManager.h>
#include <qcg.h>
#include <qcmDictionary.h>
#include <qmv.h>
#include <qcmUser.h>
#include <qcuSqlSourceInfo.h>
#include <smiTableSpace.h>
#include <qdd.h>
#include <qdpDrop.h>
#include <qcmView.h>
#include <qdbComment.h>
#include <qdx.h>

const void * gQcmCompressionTables;
const void * gQcmCompressionTablesIndex[ QCM_MAX_META_INDICES ];

IDE_RC qcmDictionary::selectDicSmOID( qcTemplate     * aTemplate,
                                      void           * aTableHandle,
                                      void           * aIndexHeader,
                                      const void     * aValue,
                                      smOID          * aSmOID )
{
/***********************************************************************
 *
 * Description :
 *    Dictionary table 에서 smOID 를 구한다.
 *
 * Implementation :
 *
 *    1. Dictionary table 검색을 위해 range 를 생성한다.
 *    2. 커서를 연다.
 *       Dictionary table 은 컬럼이 1개뿐인 특별한 테이블이므로 1개짜리 smiColumnList 를 사용한다.
 *    3. 조건에 맞는 row 의 scGRID 를 얻는다.
 *    4. scGRID 에서 smOID 를 얻는다.
 *
 *    PROJ-2264
 *    이 함수는 dictionary table 에 대한 처리이므로
 *    fetch column list를 구성하지 않는다.
 *
 ***********************************************************************/

    smiTableCursor      sCursor;

    UInt                sStep = 0;
    const void        * sRow;

    mtcColumn         * sColumn;
    smiColumnList       sColumnList[QCM_MAX_META_COLUMNS];
    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;
    UInt                sIndexType;

    smiRange            sRange;
    qtcMetaRangeColumn  sFirstMetaRange;

    void              * sDicTable;
    smOID               sOID;
    mtcColumn         * sFirstMtcColumn;

    // table handel
    sDicTable = aTableHandle;

    IDE_TEST( smiGetTableColumns( sDicTable,
                                  0,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    // Make range
    qcm::makeMetaRangeSingleColumn( &sFirstMetaRange,
                                    sFirstMtcColumn,
                                    (const void*) aValue,
                                    &sRange );

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( sDicTable,
                                  0,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sDicTable,
                                  0,
                                  (const smiColumn**)&sColumnList[0].column )
              != IDE_SUCCESS );

    sColumnList[0].next = NULL;

    sIndexType = smiGetIndexType(aIndexHeader);
    
    SMI_CURSOR_PROP_INIT_FOR_INDEX_SCAN(&sCursorProperty, NULL, sIndexType);

    IDE_TEST( sCursor.open( QC_SMI_STMT( aTemplate->stmt ),
                            sDicTable,
                            aIndexHeader, // index
                            smiGetRowSCN( sDicTable ),
                            sColumnList, // columns
                            &sRange, // range
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            SMI_LOCK_READ | SMI_TRAVERSE_FORWARD | SMI_PREVIOUS_DISABLE,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty)
              != IDE_SUCCESS );
    sStep = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    if( sRow != NULL )
    {
        sOID = SMI_CHANGE_GRID_TO_OID(sRid);
    }
    else
    {
        // Data not found in Dictionary table.
        sOID = 0;
    }

    sStep = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    *aSmOID = sOID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStep == 1 )
    {
        (void)sCursor.close();
    }
    else
    {
        // Cursor already closed.
    }

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::recreateDictionaryTable( qcStatement           * aStatement,
                                               qcmTableInfo          * aTableInfo,
                                               qcmColumn             * aColumn,
                                               scSpaceID               aTBSID,
                                               qcmDictionaryTable   ** aDicTable )
{
    qcmTableInfo  * sDicTableInfo;
    qcmColumn     * sColumn;
    UInt            sTableFlagMask;
    UInt            sTableFlag;
    UInt            sTableParallelDegree;

    // create new dictionary tables
    for( sColumn = aColumn; sColumn != NULL; sColumn = sColumn->next )
    {
        // Max rows 를 위해 기존 dictionary table 의 table info 를 가져온다.
        sDicTableInfo = (qcmTableInfo *)smiGetTableRuntimeInfoFromTableOID(
            sColumn->basicInfo->column.mDictionaryTableOID );

        // Table flag 는 data table 의 것을 따른다.
        sTableFlagMask  = QDB_TABLE_ATTR_MASK_ALL;
        sTableFlag      = aTableInfo->tableFlag;
        sTableParallelDegree = 1;

        // Dictionary table flag 를 세팅한다.
        sTableFlagMask |= SMI_TABLE_DICTIONARY_MASK;
        sTableFlag     |= SMI_TABLE_DICTIONARY_TRUE;

        // PROJ-2429 table type을 무조건 memory로 세팅한다.
        sTableFlag     &= ~SMI_TABLE_TYPE_MASK;
        sTableFlag     |= SMI_TABLE_MEMORY;

        IDE_TEST( createDictionaryTable( aStatement,
                                         aTableInfo->tableOwnerID,
                                         sColumn,
                                         1, // Column count
                                         aTBSID,
                                         sDicTableInfo->maxrows,
                                         sTableFlagMask,
                                         sTableFlag,
                                         sTableParallelDegree,
                                         aDicTable )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::makeDictionaryTable( qcStatement           * aStatement,
                                           scSpaceID               aTBSID,
                                           qcmColumn             * aColumns,
                                           qcmCompressionColumn  * aCompCol,
                                           qcmDictionaryTable   ** aDicTable )
{
    qdTableParseTree   * sParseTree;
    qcmColumn          * sColIter;
    qcmColumn          * sColumn = NULL;
    ULong                sMaxrows;
    UInt                 sTableFlagMask;
    UInt                 sTableFlag;
    UInt                 sTableParallelDegree;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    for( sColIter = aColumns;
         sColIter != NULL;
         sColIter = sColIter->next )
    {
        if ( QC_IS_NAME_MATCHED( aCompCol->namePos, sColIter->namePos ) )
        {
            sColumn = sColIter;

            // 이름이 같은 qcmColumn 을 찾았다.
            break;
        }
        else
        {
            // 찾고있는 컬럼이 아니다.
            // Nothing to do.
        }
    }

    if( sColumn != NULL )
    {
        // 원본 data table 의 column 이름을 가져온다.
        QC_STR_COPY( sColumn->name, sColumn->namePos );

        // MAXROW 를 가져온다.
        if( aCompCol->maxrows == 0 )
        {
            if( ( sParseTree->maxrows != 0 ) &&
                ( sParseTree->maxrows != ID_ULONG_MAX ) )
            {
                // CREATE TABLE 시
                // Compress column 에 MAXROW 가 지정되지 않았고,
                // data table 에 MAXROW 가 지정되었을 경우는 table 의 MAXROW 를 따른다.
                sMaxrows = sParseTree->maxrows;
            }
            else
            {
                // Compress column 과 data table 양쪽 모두 MAXROW 가
                // 지정되지 않았으면 0(무제한)으로 설정한다.
                sMaxrows = 0;
            }
        }
        else
        {
            // Compress column 에 MAXROW 가 지정되었으면 그 값으로 설정한다.
            sMaxrows = aCompCol->maxrows;
        }

        // Dictionary table 생성
        sTableFlagMask = SMI_TABLE_DICTIONARY_MASK;
        sTableFlag     = SMI_TABLE_DICTIONARY_TRUE;
        sTableParallelDegree = 1;

        IDE_TEST( createDictionaryTable( aStatement,
                                         sParseTree->userID,
                                         sColumn,
                                         1, // Column count
                                         aTBSID,
                                         sMaxrows,
                                         sTableFlagMask,
                                         sTableFlag,
                                         sTableParallelDegree,
                                         aDicTable )
                  != IDE_SUCCESS );
    }
    else
    {
        // Compression column 에 해당하는 column 이 없다.
        // 사용자가 잘못 사용한 경우 validation 에서 걸러지므로 ASSERTION 처리 한다.
        IDE_DASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::validateCompressColumn( qcStatement       * aStatement,
                                              qdTableParseTree  * aParseTree,
                                              smiTableSpaceType   aType )
{
/***********************************************************************
 *
 * Description :
 *    Dictionary based compress column 검증 함수
 *
 * Implementation :
 *    1. Column list에 선언된 컬럼인지 검사
 *      1-1. 적합한 type 인지 검사
 *      1-2. 적합한 size 인지 검사
 *      1-3. smiColumn 에 compression column flag 세팅
 *    2. 원본 테이블의 tablespace 검사
 *    3. MEMORY PARTITIONED TABLE  검사.
 *
 ***********************************************************************/

    qcmCompressionColumn * sCompColumn;
    qcmColumn            * sColumn;
    qcuSqlSourceInfo       sqlInfo;

    IDE_DASSERT( aParseTree != NULL );

    if( aParseTree->compressionColumn != NULL )
    {
        // 1. column list 에 선언된 컬럼인지 검사
        for( sCompColumn = aParseTree->compressionColumn;
             sCompColumn != NULL;
             sCompColumn = sCompColumn->next )
        {
            for( sColumn = aParseTree->columns;
                 sColumn != NULL;
                 sColumn = sColumn->next )
            {
                if ( QC_IS_NAME_MATCHED( sCompColumn->namePos, sColumn->namePos ) )
                {
                    // 1-1. 지원하는 type 인지 검사한다.
                    switch( sColumn->basicInfo->type.dataTypeId )
                    {
                        case MTD_CHAR_ID :
                        case MTD_VARCHAR_ID :
                        case MTD_NCHAR_ID :
                        case MTD_NVARCHAR_ID :
                        case MTD_BYTE_ID :
                        case MTD_VARBYTE_ID :
                        case MTD_NIBBLE_ID :
                        case MTD_BIT_ID :
                        case MTD_VARBIT_ID :
                        case MTD_DATE_ID :
                            // 지원 type
                            break;

                        case MTD_DOUBLE_ID :
                        case MTD_FLOAT_ID :
                        case MTD_NUMBER_ID :
                        case MTD_NUMERIC_ID :
                        case MTD_BIGINT_ID :
                        case MTD_INTEGER_ID :
                        case MTD_REAL_ID :
                        case MTD_SMALLINT_ID :
                        case MTD_ECHAR_ID :
                        case MTD_EVARCHAR_ID :
                        case MTD_BLOB_ID :
                        case MTD_BLOB_LOCATOR_ID :
                        case MTD_CLOB_ID :
                        case MTD_CLOB_LOCATOR_ID :
                        case MTD_GEOMETRY_ID :
                        case MTD_NULL_ID :
                        case MTD_LIST_ID :
                        case MTD_NONE_ID :
                        case MTS_FILETYPE_ID :
                        case MTD_BINARY_ID :
                        case MTD_INTERVAL_ID :
                        case MTD_BOOLEAN_ID :
                        default : // 사용자 정의 타입 또는 rowtype id
                            sqlInfo.setSourceInfo( aStatement,
                                                   &(sColumn->namePos) );
                            IDE_RAISE(ERR_NOT_SUPPORT_TYPE);
                            break;
                    }

                    // 1-2. Column 의 저장크기(size)가 smOID 보다 작으면 안된다.
                    //    우선 compression 하면 오히려 저장 공간을 더 사용하게되고
                    //    in-place update 시에 logging 문제도 발생하게 된다.
                    if( sColumn->basicInfo->column.size < ID_SIZEOF(smOID) )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &(sColumn->namePos) );
                        IDE_RAISE( ERR_NOT_SUPPORT_TYPE );
                    }

                    // 1-3. Compression column 임을 나타내도록 flag 세팅
                    sColumn->basicInfo->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                    sColumn->basicInfo->column.flag |= SMI_COLUMN_COMPRESSION_TRUE;

                    break;
                }
            }

            // 1. 끝까지 이동했다는 것은 못 찾았다는 의미이다.
            if( sColumn == NULL )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &(sCompColumn->namePos) );
                IDE_RAISE( ERR_NOT_EXIST_COLUMN_NAME );
            }
        }

        /* BUG-45641 disk partitioned table에 압축 컬럼을 추가하다가 실패하는데, memory partitioned table 오류가 나옵니다. */
        // 2. 원본 테이블의 tablespace 검사
        //   Compression 컬럼은 volatile 테이블에서 사용할 수 없다.
        IDE_TEST_RAISE( ( smiTableSpace::isMemTableSpaceType( aType ) != ID_TRUE ) &&
                        ( smiTableSpace::isDiskTableSpaceType( aType ) != ID_TRUE ),
                        ERR_NOT_SUPPORTED_TABLESPACE_TYPE );

        /* 3. PROJ-2334 PMT dictionary compress는 memory/disk tablespace만 지원
         * memory/disk partitioned table에 대한 고려가 되어 있지 않아 에러 출력 */
        if ( aParseTree->tableInfo != NULL )
        {
            IDE_TEST_RAISE( aParseTree->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE,
                            ERROR_UBABLE_TO_COMPRESS_PARTITION );
        }
        else
        {
            IDE_TEST_RAISE( aParseTree->partTable != NULL,
                            ERROR_UBABLE_TO_COMPRESS_PARTITION );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_SUPPORTED_TABLESPACE_TYPE)
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMPRESSION_NOT_SUPPORTED_TABLESPACE ) );
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN_NAME)
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_COLUMN_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORT_TYPE)
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COMPRESSION_NOT_SUPPORTED_DATATYPE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERROR_UBABLE_TO_COMPRESS_PARTITION)
    {
        /* BUG-45641 disk partitioned table에 압축 컬럼을 추가하다가 실패하는데, memory partitioned table 오류가 나옵니다. */
        /* PROJ-2429 Dictionary based data compress for on-disk DB */
        if ( smiTableSpace::isDiskTableSpaceType( aType ) == ID_TRUE )
        {
            IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_UNABLE_TO_COMPRESS_DISK_PARTITION ) );
        }
        else
        {
            IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_UNABLE_TO_COMPRESS_MEM_PARTITION ) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::rebuildDictionaryTable( qcStatement        * aStatement,
                                              qcmTableInfo       * aOldTableInfo,
                                              qcmTableInfo       * aNewTableInfo,
                                              qcmDictionaryTable * aDicTable )
{
/***********************************************************************
 *
 * Description :
 *    Data table 의 레코드를 읽어 dictionary table 을 새로 구성한다.
 *    즉, data table 에 존재하는 값으로 dictionary table 을 재구성한다.
 *
 *    아래함수에서 호출됨.
 *    qdbAlter::executeReorganizeColumn()
 *
 * Implementation :
 *    1. Open data table cursor
 *    2. Read row from data table
 *      (for each dictionary table)
 *        2-1. Open dictionary table cursor
 *        2-2. Insert value to dictionary table
 *        2-3. Close dictionary table cursor
 *      2-4. Update OID of data table
 *    3. Close data table cursor
 *
 ***********************************************************************/

    const void            * sOldRow;
    smiValue              * sNewRow;
    smiValue              * sDataRow;
    mtcColumn             * sSrcCol         = NULL;
    mtcColumn             * sDestCol        = NULL;
    void                  * sValue;
    SInt                    sStage          = 0;
    iduMemoryStatus         sQmxMemStatus;
    scGRID                  sRowGRID;
    void                  * sInsRow;
    scGRID                  sInsGRID;
    smiFetchColumnList    * sSrcFetchColumnList;
    UInt                    sStoringSize    = 0;
    void                  * sStoringValue;
    smiTableCursor          sDstTblCursor;
    smiCursorProperties     sDstCursorProperty;
    smiTableCursor          sDataTblCursor;
    smiCursorProperties     sDataCursorProperty;
    smiCursorProperties   * sDicCursorProperty;
    qcmDictionaryTable    * sDicIter;
    qcmColumn             * sColumn         = NULL;
    qcmColumn             * sDataColumns;
    mtcColumn             * sOldDataColumns = NULL;
    UInt                    i;
    UInt                    sDicTabCount;
    void                  * sRow;
    smiColumnList         * sUpdateColList;
    smiColumnList         * sPrev           = NULL;

    void                  * sTableHandle;

    SChar                 * sNullOID        = NULL;
    scGRID                  sDummyGRID;     
    SChar                 * sNullColumn     = NULL;
    UInt                    sDiskRowSize    = 0;

    qdbAnalyzeUsage       * sAnalyzeUsage   = NULL;

    //-----------------------------------------
    // Data table 에 대한 smiColumnList 생성
    //-----------------------------------------

    for( sDicIter = aDicTable, sDicTabCount = 0;
         sDicIter != NULL;
         sDicIter = sDicIter->next )
    {
        sDicTabCount++;
    }

    // Data table 의 fetch column list 생성에 쓰일 qcmColumn 복사
    IDE_TEST( aStatement->qmxMem->cralloc( ID_SIZEOF(qcmColumn) * sDicTabCount,
                                           (void **) & sDataColumns )
              != IDE_SUCCESS );

    for ( sDicIter = aDicTable, i = 0;
          sDicIter != NULL;
          sDicIter = sDicIter->next, i++ )
    {
        idlOS::memcpy( &(sDataColumns[i]), sDicIter->dataColumn, ID_SIZEOF(qcmColumn) );
    }

    /* PROJ-2465 Tablespace Alteration for Table
     *  Old Table의 Compressed Column을 복사하고, mtc::value()에서 사용할 offset을 New Table의 것으로 수정한다.
     */
    IDU_FIT_POINT( "qcmDictionary::rebuildDictionaryTable::alloc::sOldDataColumns",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(mtcColumn) * sDicTabCount,
                                             (void **) & sOldDataColumns )
              != IDE_SUCCESS );

    for ( sDicIter = aDicTable, i = 0;
          sDicIter != NULL;
          sDicIter = sDicIter->next, i++ )
    {
        sColumn = qdbCommon::findColumnInColumnList( aOldTableInfo->columns,
                                                     sDicIter->dataColumn->basicInfo->column.id );

        IDE_TEST_RAISE( sColumn == NULL, ERR_COLUMN_NOT_FOUND );

        idlOS::memcpy( &(sOldDataColumns[i]), sColumn->basicInfo, ID_SIZEOF(mtcColumn) );
        sOldDataColumns[i].column.offset = sDataColumns[i].basicInfo->column.offset;
    }

    // Data table 에 OID 를 update 하기 위한 smiValue 배열
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) * sDicTabCount,
                                       (void**)&sDataRow)
             != IDE_SUCCESS);

    for( i = 0; i < sDicTabCount; i++ )
    {
        IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smOID) * sDicTabCount,
                                           (void**)&(sDataRow[i].value))
                 != IDE_SUCCESS);
    }

    // Fetch column list 를 위한 공간 할당
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiFetchColumnList) * sDicTabCount,
                                       (void**)&sSrcFetchColumnList)
             != IDE_SUCCESS);

    // Update column list 를 위한 공간 할당
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiColumnList) * sDicTabCount,
                                       (void**)&sUpdateColList)
                 != IDE_SUCCESS);

    /* PROJ-2465 Tablespace Alteration for Table */
    if ( QCM_TABLE_TYPE_IS_DISK( aNewTableInfo->tableFlag ) == ID_TRUE )
    {
        IDE_TEST( qdbCommon::getDiskRowSize( aNewTableInfo->tableHandle,
                                             & sDiskRowSize )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "qcmDictionary::rebuildDictionaryTable::alloc::sOldRow",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( sDiskRowSize,
                                                 (void **) & sOldRow )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //-------------------------------------------
    // open data table. ( select + update )
    // cursor property 설정.
    //
    // PROJ-1705
    // fetch column list 정보를 구성해서 sm으로 내린다.
    //-------------------------------------------

    sDataTblCursor.initialize();
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sDataCursorProperty, aStatement->mStatistics );
    sDataCursorProperty.mLockWaitMicroSec = 0;

    // Data table 을 위한 fetch column list
    IDE_TEST( qdbCommon::makeFetchColumnList( QC_PRIVATE_TMPLATE(aStatement),
                                              sDicTabCount,
                                              sDataColumns,
                                              ID_FALSE,   // alloc smiColumnList
                                              & sSrcFetchColumnList )
          != IDE_SUCCESS );

    // Data table 을 위한 update column list
    for( sDicIter = aDicTable, i = 0;
         sDicIter != NULL;
         sDicIter = sDicIter->next, i++ )
    {
        sUpdateColList[i].column = &(sDicIter->dataColumn->basicInfo->column);
        sUpdateColList[i].next   = NULL;

        if( sPrev == NULL )
        {
            sPrev = &(sUpdateColList[i]);
        }
        else
        {
            sPrev->next = &(sUpdateColList[i]);
            sPrev = sPrev->next;
        }
    }

    sDataCursorProperty.mFetchColumnList = sSrcFetchColumnList;

    IDE_TEST(
        sDataTblCursor.open(
            QC_SMI_STMT( aStatement ),
            aNewTableInfo->tableHandle,
            NULL,
            smiGetRowSCN( aNewTableInfo->tableHandle ),
            sUpdateColList,
            smiGetDefaultKeyRange(),
            smiGetDefaultKeyRange(),
            smiGetDefaultFilter(),
            SMI_LOCK_TABLE_EXCLUSIVE|
            SMI_TRAVERSE_FORWARD|
            SMI_PREVIOUS_DISABLE,
            SMI_UPDATE_CURSOR,
            & sDataCursorProperty )
        != IDE_SUCCESS );
    sStage = 1;

    //-------------------------------------------
    // open destination table. ( insert )
    // cursor property 설정.
    //-------------------------------------------

    // smiTableCursor 에 줄 cursor property 도 dictionary table 별로 만든다.
    IDE_TEST( aStatement->qmxMem->cralloc( ID_SIZEOF(smiCursorProperties) * sDicTabCount,
                                           (void **) & sDicCursorProperty )
              != IDE_SUCCESS );

    sDstTblCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sDstCursorProperty, aStatement->mStatistics );

    /* ------------------------------------------------
     * make new row values
     * ----------------------------------------------*/
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) * sDicTabCount,
                                       (void**)&sNewRow)
             != IDE_SUCCESS);

    // move..
    IDE_TEST(sDataTblCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sDataTblCursor.readRow(&sOldRow,
                                    &sRowGRID, SMI_FIND_NEXT)
             != IDE_SUCCESS);

    /* PROJ-2465 Tablespace Alteration for Table
     *  - aDicTable 리스트에서 첫번째 dictionaryTableInfo만으로 sAnalyzeUsage를 초기화한다.
     *  - 동일한 Table의 다른 Dictionary의 Row는 동일한 TBS에 적재된다.
     */
    IDE_TEST( qdbCommon::initializeAnalyzeUsage( aStatement,
                                                 aNewTableInfo,
                                                 aDicTable->dictionaryTableInfo,
                                                 &( sAnalyzeUsage ) )
              != IDE_SUCCESS );

    while (sOldRow != NULL)
    {
        /* PROJ-2465 Tablespace Alteration for Table */
        IDE_TEST( qdbCommon::checkAndSetAnalyzeUsage( aStatement,
                                                      sAnalyzeUsage )
                  != IDE_SUCCESS );

        // To Fix PR-11704
        // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        //------------------------------------------
        // INSERT를 수행
        //------------------------------------------

        for( sDicIter = aDicTable, i = 0;
             sDicIter != NULL;
             sDicIter = sDicIter->next, i++ )
        {
            sSrcCol  = &(sOldDataColumns[i]);
            sDestCol = sDicIter->dictionaryTableInfo->columns->basicInfo;

            sTableHandle = sDicIter->dictionaryTableInfo->tableHandle;

            // BUGBUG
            IDE_TEST( sDstTblCursor.open(
                            QC_SMI_STMT( aStatement ),
                            sTableHandle,
                            NULL,
                            smiGetRowSCN( sTableHandle ),
                            NULL,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            SMI_LOCK_WRITE|
                            SMI_TRAVERSE_FORWARD|
                            SMI_PREVIOUS_DISABLE,
                            SMI_INSERT_CURSOR,
                            & sDstCursorProperty )
                      != IDE_SUCCESS );

            sStage = 2;

            // Data table 에서 값을 읽는다.
            sValue = (void *) mtc::value( sSrcCol,
                                          sOldRow,
                                          MTD_OFFSET_USE );

            IDE_TEST( qdbCommon::mtdValue2StoringValue(
                          sDestCol,
                          sSrcCol,
                          (void*)sValue,
                          &sStoringValue )
                      != IDE_SUCCESS );
            sNewRow[i].value = sStoringValue;

            IDE_TEST( qdbCommon::storingSize( sDestCol,
                                              sSrcCol,
                                              (void*)sValue,
                                              &sStoringSize )
                      != IDE_SUCCESS );
            sNewRow[i].length = sStoringSize;

            // Data table 에서 읽은 값(sNewRow)을 dictionary table 에 넣고,
            // 그 OID 를 다시 data table 에 update 하기 위해 받는다.(sDataRow)
            IDE_TEST( sDstTblCursor.insertRowWithIgnoreUniqueError(
                          &sDstTblCursor,
                          (smcTableHeader *)SMI_MISC_TABLE_HEADER( sTableHandle ), //(smcTableHeader*)
                          &(sNewRow[i]),
                          (smOID*)sDataRow[i].value,
                          &sRow )
                      != IDE_SUCCESS );

            sStage = 1;

            IDE_TEST( sDstTblCursor.close() != IDE_SUCCESS );

            sDataRow[i].length = ID_SIZEOF(smOID);
        }

        IDE_TEST(sDataTblCursor.updateRow(sDataRow,
                                         & sInsRow,
                                         & sInsGRID)
                 != IDE_SUCCESS);

        // To Fix PR-11704
        // Memory 재사용을 위한 Memory 이동
        IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        IDE_TEST(sDataTblCursor.readRow(&sOldRow, &sRowGRID, SMI_FIND_NEXT)
                 != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sDataTblCursor.close() != IDE_SUCCESS);

    /* PROJ-2465 Tablespace Alteration for Table */
    if ( QCM_TABLE_TYPE_IS_DISK( aNewTableInfo->tableFlag ) != ID_TRUE )
    {
        //------------------------------------------
        // Update Null row
        //------------------------------------------
        (void)smiGetTableNullRow( aNewTableInfo->tableHandle, (void**)&sNullOID, &sDummyGRID );

        for ( sDicIter = aDicTable, i = 0;
              ( sDicIter != NULL ) && ( i < sDicTabCount );
              sDicIter = sDicIter->next, i++ )
        {
            sTableHandle = sDicIter->dictionaryTableInfo->tableHandle;
            sNullColumn  = sNullOID + sDicIter->dataColumn->basicInfo->column.offset;

            idlOS::memcpy( sNullColumn,
                           &(SMI_MISC_TABLE_HEADER(sTableHandle)->mNullOID),
                           ID_SIZEOF(smOID) );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmDictionary::rebuildDictionaryTable",
                                  "Column Not Found" ) );
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            // Close cursor for dictionary table( sStage > 1 ) 
            (void)sDstTblCursor.close();
            /* fall through */
        case 1:
            // Close cursor for data table (sStage >= 1)
            (void)sDataTblCursor.close();
            break;
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::removeDictionaryTable( qcStatement  * aStatement,
                                             qcmColumn    * aColumn )
{
    qcmTableInfo  * sDicTableInfo;

    IDE_DASSERT( aColumn->basicInfo->column.mDictionaryTableOID != SM_NULL_OID );
    
    // smiColumn.mDictionaryTableOID 사용
    sDicTableInfo = (qcmTableInfo *)smiGetTableRuntimeInfoFromTableOID(
        aColumn->basicInfo->column.mDictionaryTableOID );

    IDE_DASSERT( sDicTableInfo != NULL );

    IDE_TEST( dropDictionaryTable( aStatement,
                                   sDicTableInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::removeDictionaryTableWithInfo( qcStatement  * aStatement,
                                                     qcmTableInfo * aTableInfo )
{
    IDE_TEST( dropDictionaryTable( aStatement,
                                   aTableInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::updateCompressionTableSpecMeta( qcStatement * aStatement,
                                                      UInt          aOldDicTableID,
                                                      UInt          aNewDicTableID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2264
 *      executeReorganizeColumn 으로부터 호출
 *      Data table-Dictionary table 간 연결 정보를 갱신한다.
 *
 * Implementation :
 *      1. SYS_COMPRESSION_TABLES_ 메타 테이블에서 연결 정보 갱신
 *
 ***********************************************************************/

    SChar         * sSqlStr;
    vSLong          sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    // SYS_COMPRESSION_TABLES_ 을 갱신한다.
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COMPRESSION_TABLES_ SET DIC_TABLE_ID = "
                     "INTEGER'%"ID_INT32_FMT"' "
                     "WHERE DIC_TABLE_ID = "
                     "INTEGER'%"ID_INT32_FMT"'",
                     aNewDicTableID,
                     aOldDicTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDictionary::updateColumnIDInCompressionTableSpecMeta(
                          qcStatement * aStatement,
                          UInt          aDicTableID,
                          UInt          aNewColumnID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2264
 *      executeDropCol 으로부터 호출
 *      Data table-Dictionary table 간 연결 정보를 갱신한다.
 *
 * Implementation :
 *      1. SYS_COMPRESSION_TABLES_ 메타 테이블에서 연결 정보 갱신
 *
 ***********************************************************************/

    SChar         * sSqlStr;
    vSLong          sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    // SYS_COMPRESSION_TABLES_ 을 갱신한다.
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COMPRESSION_TABLES_ SET COLUMN_ID = "
                     "INTEGER'%"ID_INT32_FMT"' "
                     "WHERE DIC_TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                     aNewColumnID,
                     aDicTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcmDictionary::dropDictionaryTable( qcStatement  * aStatement,
                                    qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 * Description : PROJ-2253 Dictionary table
 *
 * Implementation : Dictionary table 을 삭제한다.
 *
 ***********************************************************************/

    qcmTableInfo  * sTableInfo;
    UInt            sTableID;

    sTableInfo = aTableInfo;
    sTableID   = aTableInfo->tableID;

    // Constraint, Index, Table 에 관련된 메타 테이블에서 관련 레코드를 삭제한다.
    IDE_TEST(qdd::deleteConstraintsFromMeta(aStatement, sTableID)
             != IDE_SUCCESS);

    IDE_TEST(qdd::deleteIndicesFromMeta(aStatement, sTableInfo)
             != IDE_SUCCESS);

    IDE_TEST(qdd::deleteTableFromMeta(aStatement, sTableID)
             != IDE_SUCCESS);

    // 이 외에 Priviledge, Related view, Comment 등의 메타 테이블을 삭제해야 한다.
    // 하지만 dictionary table 은 DDL 이 금지되어 메타에 레코드가 생기지 않는다.
    // 따라서 삭제를 생략한다.

    // SM 에서 dictionary table 삭제
    IDE_TEST( smiTable::dropTable( QC_SMI_STMT( aStatement ),
                                   sTableInfo->tableHandle,
                                   SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcmDictionary::createDictionaryTable( qcStatement         * aStatement,
                                      UInt                  aUserID,
                                      qcmColumn           * aColumn,
                                      UInt                  aColumnCount,
                                      scSpaceID             aTBSID,
                                      ULong                 aMaxRows,
                                      UInt                  aInitFlagMask,
                                      UInt                  aInitFlagValue,
                                      UInt                  aParallelDegree,
                                      qcmDictionaryTable ** aDicTable )
{
/***********************************************************************
 *
 * Description : PROJ-2264 Dictionary table
 *
 * Implementation :
 *     dictionary table을 생성한다.
 *
 ***********************************************************************/

    qcmTableInfo          * sTableInfo = NULL;
    void                  * sTableHandle;
    smSCN                   sSCN;
    UInt                    sTableID;
    smOID                   sTableOID;
    qcmColumn               sColumn;
    smiColumnList           sColumnListAtKey;
    mtcColumn               sMtcColumn;
    SChar                   sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    UInt                    sIndexID;
    UInt                    sIndexType;
    const void            * sIndexHandle;
    UInt                    sIndexFlag = 0;
    UInt                    sBuildFlag = 0;
    qcmDictionaryTable    * sDicIter;
    qcmDictionaryTable    * sDicInfo;
    qcNamePosition          sTableNamePos;
    SChar                   sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    UInt                    sDictionaryID;
    scSpaceID               sDictionaryTableTBS;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStoAttr;

    // ---------------------------------------------------------------
    // Physical Attribute for Memory Tablespace
    // ---------------------------------------------------------------
    idlOS::memset( &sSegAttr, 0x00, ID_SIZEOF(smiSegAttr) );
    idlOS::memset( &sSegStoAttr, 0x00, ID_SIZEOF(smiSegStorageAttr) );

    // ---------------------------------------------------------------
    // Column 정보 수정
    // ---------------------------------------------------------------

    // qcmColumn deep copy
    idlOS::memcpy( &sColumn, aColumn, ID_SIZEOF(qcmColumn) );
    idlOS::memcpy( &sMtcColumn, aColumn->basicInfo, ID_SIZEOF(mtcColumn) );
    sColumn.basicInfo = &sMtcColumn;
    sColumn.next = NULL;

    // smiColumn 정보를 수정한다.
    sMtcColumn.column.offset = smiGetRowHeaderSize( SMI_TABLE_MEMORY );
    sMtcColumn.column.value = NULL;
    sMtcColumn.column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
    sMtcColumn.column.flag |= SMI_COLUMN_COMPRESSION_FALSE;

    // Recreate 시, Dictionary Table의 Column에 Dictionary Table OID가 없어야 한다.
    SMI_INIT_SCN( & sMtcColumn.column.mDictionaryTableOID );

    //PROJ-2429 Dictionary based data compress for on-disk DB
    if ( smiTableSpace::isDiskTableSpace( aTBSID ) == ID_TRUE )
    {
        //column저장 공간을 메모리로 세팅한다.
        sMtcColumn.column.flag &= ~SMI_COLUMN_STORAGE_MASK;
        sMtcColumn.column.flag |= SMI_COLUMN_STORAGE_MEMORY;

        sMtcColumn.column.flag &= ~SMI_COLUMN_COMPRESSION_TARGET_MASK;
        sMtcColumn.column.flag |= SMI_COLUMN_COMPRESSION_TARGET_DISK;

        //disk column에만 설정된는 flag로 제거 한다.
        sMtcColumn.column.flag &= ~SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK;

        //disk table의 dictionary table은 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA에 저장 된다.
        sDictionaryTableTBS        = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
        sMtcColumn.column.colSpace = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
    }
    else
    {
        //memory table의 dictionary table은 해당 memory TBS에 저장 된다.
        sDictionaryTableTBS = aTBSID;

        sMtcColumn.column.flag &= ~SMI_COLUMN_COMPRESSION_TARGET_MASK;
        sMtcColumn.column.flag |= SMI_COLUMN_COMPRESSION_TARGET_MEMORY;
    } 

    // ---------------------------------------------------------------
    // TABLE 생성
    // ---------------------------------------------------------------

    // Create table
    IDE_TEST( qcm::getNextTableID( aStatement, &sTableID ) != IDE_SUCCESS );

    IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                          &sColumn,
                                          aUserID,
                                          sTableID,
                                          aMaxRows,
                                          sDictionaryTableTBS,
                                          sSegAttr,
                                          sSegStoAttr,
                                          aInitFlagMask,
                                          aInitFlagValue,
                                          aParallelDegree,
                                          &sTableOID )
              != IDE_SUCCESS );

    // 중복을 방지하도록 dictionary 의 sequencial 한 ID 값을 붙인다.
    IDE_TEST( qcm::getNextDictionaryID( aStatement, &sDictionaryID ) != IDE_SUCCESS );

    // Dictionary table name 을 만든다.
    idlOS::snprintf( sTableName, QC_MAX_OBJECT_NAME_LEN + 1,
                     "DIC_%"ID_vULONG_FMT"_%"ID_UINT32_FMT,
                     sTableOID,
                     sDictionaryID );

    sTableNamePos.stmtText = sTableName;
    sTableNamePos.offset   = 0;
    sTableNamePos.size     = idlOS::strlen(sTableName);

    // BUG-37144 QDV_HIDDEN_DICTIONARY_TABLE_TRUE add
    IDE_TEST( qdbCommon::insertTableSpecIntoMeta( aStatement,
                                                  ID_FALSE,
                                                  QDV_HIDDEN_DICTIONARY_TABLE_TRUE,
                                                  sTableNamePos,
                                                  aUserID,
                                                  sTableOID,
                                                  sTableID,
                                                  aColumnCount,
                                                  aMaxRows,
                                                  /* PROJ-2359 Table/Partition Access Option */
                                                  QCM_ACCESS_OPTION_READ_WRITE,
                                                  sDictionaryTableTBS,
                                                  sSegAttr,
                                                  sSegStoAttr,
                                                  QCM_TEMPORARY_ON_COMMIT_NONE,
                                                  aParallelDegree,      // PROJ-1071
                                                  QCM_SHARD_FLAG_TABLE_NONE ) // TASK-7307
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::insertColumnSpecIntoMeta( aStatement,
                                                   aUserID,
                                                   sTableID,
                                                   &sColumn,
                                                   ID_FALSE )
              != IDE_SUCCESS );

    /* Table을 생성했으므로, Lock을 획득한다. */
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    // ---------------------------------------------------------------
    // Unique Index 생성
    // ---------------------------------------------------------------
    sColumnListAtKey.column = (smiColumn*) &sMtcColumn;
    sColumnListAtKey.next = NULL;

    IDE_TEST(qcm::getNextIndexID(aStatement, &sIndexID) != IDE_SUCCESS);

    idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                     "%sIDX_ID_%"ID_INT32_FMT"",
                     QC_SYS_OBJ_NAME_HEADER,
                     sIndexID );

    sIndexType = mtd::getDefaultIndexTypeID( sColumn.basicInfo->module );

    sIndexFlag = SMI_INDEX_UNIQUE_ENABLE|
                 SMI_INDEX_TYPE_NORMAL|
                 SMI_INDEX_USE_ENABLE|
                 SMI_INDEX_PERSISTENT_DISABLE;

    sBuildFlag = SMI_INDEX_BUILD_DEFAULT|
                  SMI_INDEX_BUILD_UNCOMMITTED_ROW_DISABLE;

    if( smiTable::createIndex(aStatement->mStatistics,
                               QC_SMI_STMT( aStatement ),
                               sDictionaryTableTBS,
                               sTableHandle,
                               (SChar*)sObjName,
                               sIndexID,
                               sIndexType,
                               &sColumnListAtKey,
                               sIndexFlag,
                               1, // parallelDegree
                               sBuildFlag,
                               sSegAttr,
                               sSegStoAttr,
                               0, /* PROJ-2433 : dictionary table은 일반 index 사용 */
                               &sIndexHandle)
         != IDE_SUCCESS)
    {
        // To fix BUG-17762
        // 기존 에러코드에 대한 하위 호환성을 고려하여 SM 에러를
        // QP 에러로 변환한다.
        if( ideGetErrorCode() == smERR_ABORT_NOT_NULL_VIOLATION )
        {
            IDE_CLEAR();
            IDE_RAISE( ERR_HAS_NULL_VALUE );
        }
        else
        {
            IDE_TEST( ID_TRUE );
        }
    }

    IDE_TEST(qdx::insertIndexIntoMeta(aStatement,
                                      sDictionaryTableTBS,
                                      aUserID,
                                      sTableID,
                                      sIndexID,
                                      sObjName,
                                      sIndexType,
                                      ID_TRUE,  // Unique
                                      1,        // Column count
                                      ID_TRUE,
                                      ID_FALSE, // IsPartitionedIndex
                                      0,
                                      sIndexFlag)
             != IDE_SUCCESS);

    IDE_TEST( qdx::insertIndexColumnIntoMeta(
                  aStatement,
                  aUserID,
                  sIndexID,
                  sColumn.basicInfo->column.id,
                  0,                                   // Column index
                  ( ( (sColumn.basicInfo->column.flag &
                       SMI_COLUMN_ORDER_MASK) == SMI_COLUMN_ORDER_ASCENDING )
                    ? ID_TRUE : ID_FALSE ),
                  sTableID)
              != IDE_SUCCESS);

    (void)qcm::destroyQcmTableInfo( sTableInfo );
    sTableInfo = NULL;

    /* Index 정보를 가진 Meta Cache를 생성한다. */
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    // ---------------------------------------------------------------
    // smiColumn 에 Dictionary Table OID 를 설정한다.
    // ---------------------------------------------------------------
    aColumn->basicInfo->column.mDictionaryTableOID = smiGetTableId( sTableHandle );

    // ---------------------------------------------------------------
    // Table 생성이 완료된 후에 SYS_COMPRESSION_TABLES_ 에 넣기 위해
    // Column name 과 dictionary table name 을 리스트로 만든다.
    // ---------------------------------------------------------------
    IDE_TEST( STRUCT_CRALLOC( aStatement->qmxMem,
                              qcmDictionaryTable,
                              (void**) &sDicInfo ) != IDE_SUCCESS );
    QCM_DICTIONARY_TABLE_INIT( sDicInfo );

    sDicInfo->dictionaryTableInfo = sTableInfo;
    sDicInfo->dataColumn          = aColumn;
    sDicInfo->next                = NULL;

    if( *aDicTable == NULL )
    {
        *aDicTable = sDicInfo;
    }
    else
    {
        // List 의 제일 뒤에 붙여주기 위해 마지막으로 이동한다.
        for( sDicIter = *aDicTable; sDicIter->next != NULL; sDicIter = sDicIter->next );

        sDicIter->next = sDicInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_HAS_NULL_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOTNULL_HAS_NULL));
    }
    IDE_EXCEPTION_END;

    (void)qcm::destroyQcmTableInfo( sTableInfo );

    return IDE_FAILURE;
}

IDE_RC
qcmDictionary::makeDictValueForCompress( smiStatement   * aStatement,
                                         void           * aTableHandle,
                                         void           * aIndexHeader,
                                         smiValue       * aInsertedRow,
                                         void           * aOIDValue )
{
/***********************************************************************
 *
 * Description : PROJ-2397 Compressed Column Table Replication
 
 * Implementation : Dictionary table 을 삭제한다.
 *                  Dictionary table 에 동일한 값이 있는 경우 해당하는 OID 를 반환 받는다.
 *
 ***********************************************************************/
   
    void              * sMtdValue = NULL;
    smSCN               sSCN;
    smiTableCursor      sCursor;
    smiCursorProperties sCursorProperty;
    UInt                sState = 0;
    void              * sDummyRow = NULL;
    smiValue            sValue4Disk;
    smiValue          * sInsertValue;
    mtcColumn         * sMtcColumn;
    UInt                sNonStoringSize;
    smOID               sOID = SM_NULL_OID;

    IDE_TEST( smiGetTableColumns( aTableHandle,
                                  0,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );


    /* PROJ-2429 */
    if ( (sMtcColumn->column.flag & SMI_COLUMN_COMPRESSION_TARGET_MASK) 
         == SMI_COLUMN_COMPRESSION_TARGET_DISK )
    {
        aStatement->mFlag &= ~SMI_STATEMENT_CURSOR_MASK;
        aStatement->mFlag |= SMI_STATEMENT_ALL_CURSOR;

        IDE_TEST( mtc::getNonStoringSize( &sMtcColumn->column, &sNonStoringSize ) 
                  != IDE_SUCCESS );

        if ( aInsertedRow->value != NULL )
        {
            sValue4Disk.value  = (void*)((UChar*)aInsertedRow->value - sNonStoringSize); 
            sValue4Disk.length = sMtcColumn->module->actualSize( NULL, sValue4Disk.value );

            sMtdValue = (void*)sValue4Disk.value;
        }
        else
        {
            sValue4Disk.value  = sMtcColumn->module->staticNull; 
            sValue4Disk.length = sMtcColumn->module->nullValueSize();

            sMtdValue = sMtcColumn->module->staticNull;
        }

        sInsertValue = &sValue4Disk;
    }
    else
    {
        sInsertValue = aInsertedRow;

        // BUG-36718
        // Storing value(smiValue.valu)를 index range scan 에 사용하기 위해
        // 다시 mtd value 로 변환한다.
        IDE_TEST( qdbCommon::storingValue2MtdValue(
                                sMtcColumn,
                                (void*)( sInsertValue->value ),
                                &sMtdValue )
                  != IDE_SUCCESS );
    }

    // 1. Dictionary table 에 값이 존재하는지 보고, 그 row 의 OID 를 가져온다.
    IDE_TEST( qcmDictionary::selectDicSmOID( aStatement,
                                             aTableHandle,
                                             aIndexHeader,
                                             sMtdValue,
                                             &sOID )
    != IDE_SUCCESS );

    // 2. Null OID 가 반환되었으면 dictionary table 에 값이 없는 것이다.
    // Dictionary table 에 값을 넣고 그 row 의 OID 를 가져온다.
    if ( sOID == SM_NULL_OID )
    {
        sSCN = smiGetRowSCN(aTableHandle);

        // Dictionary table 에 insert 하기 위한 cursor 를 연다.
        sCursor.initialize();
        SMI_CURSOR_PROP_INIT(&sCursorProperty, NULL, NULL);

        IDE_TEST( sCursor.open( aStatement,
                                aTableHandle,
                                NULL, // index
                                sSCN,
                                NULL, // columns
                                smiGetDefaultKeyRange(), // range
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD | SMI_PREVIOUS_DISABLE,
                                SMI_INSERT_CURSOR,
                                &sCursorProperty )
                  != IDE_SUCCESS );
        sState = 1;

        // Dictionary table 에 값을 넣고 OID 를 반환 받는다.
        // 만약 동시성 문제(다른 세션에서 이미 값을 넣은 경우)로
        // unique error 가 발생하더라도 오류로 처리하지 않고
        // 이미 존재하는 row 의 OID 를 반환한다.
        IDE_TEST( sCursor.insertRowWithIgnoreUniqueError(
                      &sCursor,
                      (smcTableHeader*)SMI_MISC_TABLE_HEADER( aTableHandle ),
                      sInsertValue,
                      &sOID,
                      &sDummyRow )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }
    else
    {
        /* selectDicOID 가 성공한 경우이다. */
        /* Nothing to do */
    }

    /* BUG-39508 A variable is not considered for memory align 
     * in qcmDictionary::makeDictValueForCompress function */
    idlOS::memcpy( aOIDValue, &sOID, ID_SIZEOF(smOID) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();

        (void)sCursor.close();

        IDE_POP();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

// PROJ-2397 Compressed Column Table Replication
IDE_RC qcmDictionary::selectDicSmOID( smiStatement   * aSmiStatement,
                                      void           * aTableHandle,
                                      void           * aIndexHeader,
                                      const void     * aValue,
                                      smOID          * aSmOID )
{
/***********************************************************************
 *
 * Description :
 *    Dictionary table 에서 smOID 를 구한다.
 *
 * Implementation :
 *
 *    1. Dictionary table 검색을 위해 range 를 생성한다.
 *    2. 커서를 연다.
 *       Dictionary table 은 컬럼이 1개뿐인 특별한 테이블이므로 1개짜리 smiColumnList 를 사용한다.
 *    3. 조건에 맞는 row 의 scGRID 를 얻는다.
 *    4. scGRID 에서 smOID 를 얻는다.
 *
 *
 ***********************************************************************/

    smiTableCursor      sCursor;

    idBool              sIsCursorOpened = ID_FALSE;
    const void        * sRow;

    smiColumnList       sColumnList[QCM_MAX_META_COLUMNS];
    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    smiRange            sRange;
    qtcMetaRangeColumn  sFirstMetaRange;

    void              * sDicTable;
    smOID               sOID;
    mtcColumn         * sFirstMtcColumn;

    // table handle
    sDicTable = aTableHandle;

    IDE_TEST( smiGetTableColumns( sDicTable,
                                  0,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    // Make range
    qcm::makeMetaRangeSingleColumn( &sFirstMetaRange,
                                    sFirstMtcColumn,
                                    (const void*) aValue,
                                    &sRange );

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( sDicTable,
                                  0,
                                  (const smiColumn**)&sColumnList[0].column )
              != IDE_SUCCESS );

    sColumnList[0].next = NULL;

    SMI_CURSOR_PROP_INIT(&sCursorProperty, NULL, aIndexHeader);

    IDE_TEST( sCursor.open( aSmiStatement,
                            sDicTable,
                            aIndexHeader, // index
                            smiGetRowSCN( sDicTable ),
                            sColumnList, // columns
                            &sRange, // range
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            SMI_LOCK_READ | SMI_TRAVERSE_FORWARD | SMI_PREVIOUS_DISABLE,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty)
              != IDE_SUCCESS );
    sIsCursorOpened = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        sOID = SMI_CHANGE_GRID_TO_OID(sRid);
     }
    else
    {
        // Data not found in Dictionary table.
        sOID = SM_NULL_OID;
    }

    sIsCursorOpened = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    *aSmOID = sOID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpened == ID_TRUE )
    {
        IDE_PUSH();

        (void)sCursor.close();

        IDE_POP();
    }
    else
    {
        // Cursor already closed.
    }

    return IDE_FAILURE;
}
