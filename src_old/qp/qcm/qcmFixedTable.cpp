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
 * $Id: qcmFixedTable.cpp 90270 2021-03-21 23:20:18Z bethy $
 *
 * Description :
 *
 *     ID, SM에 의해 정의된 fixedTable에 대해서
 *     QP를 기반으로 selection 을 하기 위해서는 meta cache가 필요하며,
 *     이에 대한 정보 구축, 정보 획득등을 관리한다.
 *
 * 용어 설명 :
 *
 * 약어 :

 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smDef.h>
#include <smcDef.h>
#include <qcg.h>
#include <qcmFixedTable.h>
#include <qcuSqlSourceInfo.h>
#include <mtcDef.h>
#include <mtdTypes.h>


IDE_RC qcmFixedTable::validateTableName( qcStatement    *aStatement,
                                         qcNamePosition  aTableName )
{
/***********************************************************************
 *
 * Description :
 *  X$와 V$ 테이블 이름은 fixedTable, performanceView의 용도로만
 *  사용하도록 validation을 수행한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    if ( ( aTableName.size >= 2 ) &&
         ( ( idlOS::strMatch( (SChar *)(aTableName.stmtText + aTableName.offset),
                              2,
                              "X$",
                              2 ) == 0 ) ||
           ( idlOS::strMatch( (SChar *)(aTableName.stmtText + aTableName.offset),
                              2,
                              "V$",
                              2 ) == 0 ) ||
           /* BUG-45646 */
           ( idlOS::strMatch( (SChar *)(aTableName.stmtText + aTableName.offset),
                              2,
                              "S$",
                              2 ) == 0 ) ) )
    {
        if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 0 )
        {
            QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus = 1;
        }
        else if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 1 )
        {
            // fixedTable, performanceView normal case, do nothing
        }
        else if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 2 )
        {
            // mixed use of ( X$/V$ tables ) + normal tables
            QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus = 3;
        }
    }
    else
    {
        if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 0 )
        {
            QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus = 2;
        }
        else if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 1 )
        {
            // mixed use of ( X$/V$ tables ) + normal tables
            QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus = 3;
        }
        else if( QC_SHARED_TMPLATE(aStatement)->fixedTableAutomataStatus == 2 )
        {
            // normal table, normal view normal case, do nothing
        }
    }

    return IDE_SUCCESS;
}


IDE_RC qcmFixedTable::getTableInfo( qcStatement     *aStatement,
                                    UChar           *aTableName,
                                    SInt             aTableNameSize,
                                    qcmTableInfo   **aTableInfo,
                                    smSCN           *aSCN,
                                    void           **aTableHandle )
{
/***********************************************************************
 *
 * Description :
 *  fixedTable에 저장된 table로부터 tempInfo에 저장된
 *  tableInfo를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar sTableName[QC_MAX_NAME_LEN+1];

    // tableHandle을 이름 기반으로 획득.
    if ( aTableNameSize > 0 )
    {
        IDE_TEST( getTableHandleByName( aTableName,
                                        aTableNameSize,
                                        aTableHandle,
                                        aSCN )
                  != IDE_SUCCESS );
    }
    else
    {
        // no action
    }

    if( *aTableHandle == NULL )
    {
        IDE_RAISE(ERR_NOT_EXIST_TABLE);
    }

    IDE_TEST( smiGetTableTempInfo( *aTableHandle,
                                   (void**)aTableInfo )
              != IDE_SUCCESS );

    if ( QC_SHARED_TMPLATE(aStatement) != NULL )
    {
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE)
    {
        if (aTableNameSize > QC_MAX_NAME_LEN)
        {
            aTableNameSize = QC_MAX_NAME_LEN;
        }
        idlOS::memcpy(sTableName, aTableName, aTableNameSize);
        sTableName[aTableNameSize] = '\0';
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE, sTableName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC qcmFixedTable::getTableInfo( qcStatement     *aStatement,
                                    qcNamePosition   aTableName,
                                    qcmTableInfo   **aTableInfo,
                                    smSCN           *aSCN,
                                    void           **aTableHandle )
{
    IDE_TEST( qcmFixedTable::getTableInfo(
                  aStatement,
                  (UChar*) (aTableName.stmtText + aTableName.offset),
                  aTableName.size,
                  aTableInfo,
                  aSCN,
                  aTableHandle)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmFixedTable::getTableHandleByName( UChar            * aTableName,
                                            SInt               aTableNameSize,
                                            void            ** aTableHandle,
                                            smSCN            * aSCN )
{
/***********************************************************************
 *
 * Description :
 *  qcmFixedTable::getTableInfo에 의해 호출됨.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar sTableName[QC_MAX_OBJECT_NAME_LEN + 1];

    IDE_TEST_RAISE( aTableNameSize > QC_MAX_OBJECT_NAME_LEN,
                    ERR_NOT_EXIST_TABLE );
        
    idlOS::strncpy( sTableName, (const SChar *)aTableName, aTableNameSize );
    sTableName[aTableNameSize] = '\0';

    IDE_TEST( smiFixedTable::findTable( sTableName, aTableHandle ) != IDE_SUCCESS );

    if( *aTableHandle != NULL )
    {
        *aSCN = smiGetRowSCN(*aTableHandle);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmFixedTable::makeAndSetQcmTableInfo( idvSQL * aStatistics,
                                              SChar  * aNameType )
{
/***********************************************************************
 *
 * Description :
 *  fixedTable에 대해서 tableInfo를 구축하고, tempInfo에 주소를 저장.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiTrans             sTrans;
    smiStatement       * spRootStmt;
    smiStatement         sStmt;
    smiTableCursor       sCursor;
    void               * sTable;
    void               * sNullRow;
    UInt                 sFlag;
    const void         * sRow;
    scGRID               sRowRID;
    smiCallBack        * sCallBack;
    smiRange           * sRange;
    UInt                 sTableNameColumnOffset;
    SChar              * sTableNameColumnPtr;
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt                 j;
    qcmTableInfo       * sTableInfo = NULL;
    // fix BUG-31958
    static UInt          sStaticTableID = (UInt)(QCM_TABLES_SEQ_MAXVALUE + 1);
    UInt                 sState = 0;

    // fix BUG-31958
    IDE_TEST_RAISE( sStaticTableID == (UInt)0, ERR_OBJECTS_OVERFLOW );
 
    // To fix BUG-14818
    sCursor.initialize();

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    
    IDE_TEST( sTrans.begin( &spRootStmt, aStatistics) != IDE_SUCCESS );
    
    IDE_TEST( sStmt.begin( aStatistics,
                           spRootStmt,
                           SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR)
              != IDE_SUCCESS );

    sFlag = SMI_LOCK_READ|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE;

    // X$TABLE을 획득.
    idlOS::strcpy( sTableName, "X$TABLE" );
    IDE_TEST( smiFixedTable::findTable( sTableName, &sTable )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sTable == NULL,
                    ERR_TABLE_NOT_FOUND );

    sTableNameColumnOffset = smiFixedTable::getColumnOffset( sTable, 0 );

    sRange    = smiGetDefaultKeyRange();
    sCallBack = smiGetDefaultFilter();

    // X$TABLE 커서 open
    IDE_TEST( sCursor.open( &sStmt,
                            sTable,
                            NULL,
                            smiGetRowSCN(sTable),
                            NULL,
                            sRange,
                            sRange,
                            sCallBack,
                            sFlag,
                            SMI_SELECT_CURSOR,
                            &gMetaDefaultCursorProperty)
              != IDE_SUCCESS );

    IDE_TEST( sCursor.beforeFirst( ) != IDE_SUCCESS );

    // X$TABLE 커서 에서 첫번째 row를 readRow
    IDE_TEST( sCursor.readRow( &sRow, &sRowRID, SMI_FIND_NEXT )
              != IDE_SUCCESS );
    j = 0;
    while( sRow != NULL )
    {
        // X$TABLE의 내용중에서 TABLE 이름 획득.

        sTableNameColumnPtr = ((SChar*)sRow + sTableNameColumnOffset);
        IDE_TEST( makeTrimmedName( sTableName, &(sTableNameColumnPtr[2]) )
                  != IDE_SUCCESS );

        // TABLE의 이름이 X$ 단계
        // , V$ 단계 일때 특정 단계에서만 테이블 meta cache 생성.
        if ( ( idlOS::strlen( aNameType ) >= 2 ) &&
             ( idlOS::strMatch( aNameType,
                                2,
                                sTableName,
                                2 ) == 0 ) )
        {
            IDU_LIMITPOINT("qcmFixedTable::makeAndSetQcmTableInfo::malloc");
            IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                                       ID_SIZEOF(qcmTableInfo),
                                       (void**)&sTableInfo)
                     != IDE_SUCCESS);
            sState = 1;

            idlOS::memset( sTableInfo, 0x00, ID_SIZEOF(qcmTableInfo) );

            // PROJ-1502 PARTITIONED DISK TABLE
            sTableInfo->tablePartitionType = QCM_NONE_PARTITIONED_TABLE;

            IDE_TEST( smiFixedTable::findTable( sTableName,
                                                &sTableInfo->tableHandle )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( sTableInfo->tableHandle == NULL,
                            ERR_TABLE_NOT_FOUND );
            
            sTableInfo->tableID = sStaticTableID;
            sStaticTableID++;

            // PROJ-2206 with clause
            IDE_DASSERT( sStaticTableID < QCM_WITH_TABLES_SEQ_MINVALUE );

            if ( idlOS::strMatch( aNameType, 2, "X$", 2 ) == 0 )
            {
                sTableInfo->viewArrayNo = 0;
                sTableInfo->tableType   = QCM_FIXED_TABLE;
                sTableInfo->mPVType     = QCM_PV_TYPE_NORMAL;
            }
            else if ( idlOS::strMatch( aNameType, 2, "D$", 2 ) == 0 )
            {
                sTableInfo->viewArrayNo = 0;
                sTableInfo->tableType   = QCM_DUMP_TABLE;
                sTableInfo->mPVType     = QCM_PV_TYPE_NORMAL;
            }
            else if ( idlOS::strMatch( aNameType, 2, "V$", 2 ) == 0 )
            {
                sTableInfo->viewArrayNo = j;
                sTableInfo->tableType   = QCM_PERFORMANCE_VIEW;
                sTableInfo->mPVType     = QCM_PV_TYPE_NORMAL;
            }
            else if ( idlOS::strMatch( aNameType, 2, "S$", 2 ) == 0 )
            {
                /* BUG-45646 */
                sTableInfo->viewArrayNo = j;
                sTableInfo->tableType   = QCM_PERFORMANCE_VIEW;
                sTableInfo->mPVType     = QCM_PV_TYPE_SHARD;
            }

            sTableInfo->replicationCount = 0;
            sTableInfo->replicationRecoveryCount = 0; //proj-1608
            sTableInfo->maxrows          = 0;
            sTableInfo->status           = QCM_VIEW_VALID;
            sTableInfo->TBSID            = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
            sTableInfo->TBSType          = SMI_MEMORY_SYSTEM_DICTIONARY;
            sTableInfo->segAttr.mPctFree
                                         = QD_MEMORY_TABLE_DEFAULT_PCTFREE;
            sTableInfo->segAttr.mPctUsed
                                         = QD_MEMORY_TABLE_DEFAULT_PCTUSED;
            sTableInfo->segAttr.mInitTrans
                                         = QD_MEMORY_TABLE_DEFAULT_INITRANS;
            sTableInfo->segAttr.mMaxTrans
                                         = QD_MEMORY_TABLE_DEFAULT_MAXTRANS;
            sTableInfo->segStoAttr.mInitExtCnt
                                         = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;
            sTableInfo->segStoAttr.mNextExtCnt
                                         = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;
            sTableInfo->segStoAttr.mMinExtCnt
                                         = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;
            sTableInfo->segStoAttr.mMaxExtCnt
                                         = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;

            idlOS::strncpy(sTableInfo->name, sTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            sTableInfo->name[QC_MAX_OBJECT_NAME_LEN] = '\0';

            sTableInfo->columnCount =
                smiFixedTable::getColumnCount( sTableInfo->tableHandle );
            IDE_ASSERT( sTableInfo->columnCount != 0 );
            
            IDE_TEST( getQcmColumn( sTableInfo ) != IDE_SUCCESS );
            sTableInfo->indexCount     = 0;
            sTableInfo->indices        = NULL;
            sTableInfo->primaryKey = NULL;
            sTableInfo->uniqueKeyCount = 0;
            sTableInfo->uniqueKeys     = NULL;
            sTableInfo->foreignKeyCount = 0;
            sTableInfo->foreignKeys     = NULL;
            sTableInfo->notNullCount = 0;
            sTableInfo->notNulls     = NULL;
            sTableInfo->checkCount = 0;     /* PROJ-1107 Check Constraint 지원 */
            sTableInfo->checks     = NULL;  /* PROJ-1107 Check Constraint 지원 */
            sTableInfo->timestamp = NULL;
            sTableInfo->privilegeCount = 0;
            sTableInfo->privilegeInfo  = NULL;
            sTableInfo->triggerCount = 0;
            sTableInfo->triggerInfo = NULL;
            sTableInfo->lobColumnCount = 0;  // PROJ-1362
            sTableInfo->parallelDegree = 1;  // PROJ-1071 Parallel query

            // TASK-7307 DML Data Consistency in Shard
            sTableInfo->mIsUsable = ID_TRUE;
            sTableInfo->mShardFlag = QCM_SHARD_FLAG_TABLE_NONE;

            // 기타 자주 사용하는 정보
            sTableInfo->tableOID = smiGetTableId(sTableInfo->tableHandle);
            sTableInfo->tableFlag = smiGetTableFlag(sTableInfo->tableHandle);
            sTableInfo->isDictionary = ID_FALSE;
            sTableInfo->viewReadOnly = QCM_VIEW_NON_READ_ONLY;

            if ( smiFixedTable::useTrans( sTableInfo->tableHandle ) == ID_FALSE )
            {
                sTableInfo->operatableFlag &= ~QCM_OPERATABLE_QP_FT_TRANS_MASK;
                sTableInfo->operatableFlag |= QCM_OPERATABLE_QP_FT_TRNAS_ENABLE;
            }
            else
            {
                /* Nothing to do */
            }

            /* BUG-43006 Fixed Table Indexing Filter */
            IDE_TEST( getQcmIndices( sTableInfo ) != IDE_SUCCESS );

            // smcTableHeader의 tempInfo에 meta cache값을 설정.
            smiSetTableTempInfo( sTableInfo->tableHandle, (void*)sTableInfo );

            // nullRow 생성 및 smiFixedTableHeader.mNullRow로 할당.
            IDE_TEST( makeNullRow4FT( sTableInfo->columns, &sNullRow )
                      != IDE_SUCCESS );
            smiFixedTable::setNullRow( sTableInfo->tableHandle, sNullRow );

            j++;
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRowRID, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sCursor.close( ) != IDE_SUCCESS );

    IDE_TEST( sStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );
    
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    IDE_TEST( sTrans.destroy( aStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TABLE_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  sTableName ) );
    }
    // fix BUG-31958
    IDE_EXCEPTION( ERR_OBJECTS_OVERFLOW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                  "objects(fixed table, dump table, performance view)",
                                  ID_UINT_MAX - (UInt)QCM_TABLES_SEQ_MAXVALUE ) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            if ( sTableInfo->indices != NULL )
            {
                for ( j = 0; j < sTableInfo->indexCount; j++ )
                {
                    if ( sTableInfo->indices[j].keyColumns != NULL )
                    {
                        (void)iduMemMgr::free(sTableInfo->indices[j].keyColumns);
                        sTableInfo->indices[j].keyColumns = NULL;
                        (void)iduMemMgr::free(sTableInfo->indices[j].keyColsFlag);
                        sTableInfo->indices[j].keyColsFlag = NULL;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                (void)iduMemMgr::free(sTableInfo->indices);
                sTableInfo->indices = NULL;
            }
            else
            {
                /* Nothing to do */
            }
            (void)iduMemMgr::free(sTableInfo);
            /* fall through */
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmFixedTable::makeNullRow4FT( qcmColumn  * aColumns,
                                      void      ** aNullRow )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn     * sColumn;
    qcmColumn     * sSelectedColumn;
    SChar         * sNullRow;
    UInt            sCurrentOffset = 0;
    UInt            sState = 0;

    // fix BUG-10785
    // 레코드 사이즈를 구할 때는 맨 마지막 column을 가지고
    // 구하는 것이 아니라 가장 큰 offset을 가진 column으로 구해야 함
    sSelectedColumn = aColumns;

    for (sColumn = aColumns; sColumn != NULL; sColumn = sColumn->next)
    {
        if( sSelectedColumn->basicInfo->column.offset <
            sColumn->basicInfo->column.offset )
        {
            sSelectedColumn = sColumn;
        }
        else
        {
            // Nothing to do
        }
    }

    sCurrentOffset = sSelectedColumn->basicInfo->column.offset
        + idlOS::align8(sSelectedColumn->basicInfo->column.size);

    IDU_LIMITPOINT("qcmFixedTable::makeNullRow4FT::malloc");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               sCurrentOffset,
                               (void**)&sNullRow)
             != IDE_SUCCESS);
    sState = 1;

    for ( sColumn = aColumns; sColumn != NULL; sColumn = sColumn->next )
    {
        // set NULL row
        sColumn->basicInfo->module->null( (sColumn->basicInfo),
                                          sNullRow +
                                          sColumn->basicInfo->column.offset );
    }
    *aNullRow       = sNullRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free(sNullRow);
            /* fall through */
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmFixedTable::getQcmColumn( qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *  기존 meta table 로부터 구축하던 qcmColumn을 fixedTable기반으로 구축.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt        i;
    UInt        j;
    mtcColumn * sMtcColumn;
    SChar     * sColumnName;
    UInt        sColumnNameLen;

    if( aTableInfo->columnCount == 0 )
    {
        aTableInfo->columns       = NULL;
    }
    else
    {
        // alloc qcmColumn ...
        IDU_LIMITPOINT("qcmFixedTable::getQcmColumn::malloc");
        IDE_TEST(
            iduMemMgr::malloc(IDU_MEM_QCM,
                              ID_SIZEOF(qcmColumn) * aTableInfo->columnCount,
                              (void**)&(aTableInfo->columns))
            != IDE_SUCCESS );

        /* BUG-36276 qmvQuerySet::makeTargetListForTableRef
         *           Conditional jump or move depends on uninitialised value */
        idlOS::memset( aTableInfo->columns,
                       0x00,
                       ID_SIZEOF(qcmColumn) * aTableInfo->columnCount );

        for( i = 0; i < aTableInfo->columnCount; i++ )
        {
            // create mtcColumn
            IDE_TEST( createColumn( aTableInfo,
                                    (void **)(&sMtcColumn),
                                    i ) != IDE_SUCCESS );

            // To Fix PR-11718, PR-11719, PR-11720
            //        PR-11721, PR-11722, PR-11723
            // UMR을 방지하기 위해서는
            // smiColumn.id 값을 설정해 주어야 함.
            sMtcColumn->column.id = i;
            sMtcColumn->column.value = NULL;

            // PR-13597과 관련하여 fixedTable, performanceView는 항상 null 값이 올 수 있도록 설정.
            sMtcColumn->column.flag &= ~(MTC_COLUMN_NOTNULL_MASK);
            sMtcColumn->column.flag |= (MTC_COLUMN_NOTNULL_FALSE);

            sMtcColumn->column.offset =
                smiFixedTable::getColumnOffset( aTableInfo->tableHandle, i );

            aTableInfo->columns[i].basicInfo = sMtcColumn;
            sColumnName =
                smiFixedTable::getColumnName( aTableInfo->tableHandle, i );
            sColumnNameLen = idlOS::strlen( sColumnName );
            idlOS::memcpy( aTableInfo->columns[i].name,
                           sColumnName,
                           sColumnNameLen );

            for( j = 0; j < sColumnNameLen; j++ )
            {
                // PRJ-1678 : multi-byte를 사용하는 fixed table column명은 없겠지...
                aTableInfo->columns[i].name[j] =
                    idlOS::idlOS_toupper( aTableInfo->columns[i].name[j] );
            }

            aTableInfo->columns[i].name[sColumnNameLen] = '\0';
            aTableInfo->columns[i].defaultValue = NULL;
            aTableInfo->columns[i].namePos.stmtText = NULL;
            aTableInfo->columns[i].namePos.offset = 0;
            aTableInfo->columns[i].namePos.size = 0;

            if (i == aTableInfo->columnCount-1)
            {
                aTableInfo->columns[i].next = NULL;
            }
            else
            {
                aTableInfo->columns[i].next = &(aTableInfo->columns[i+1]);
            }

            aTableInfo->columns[i].defaultValueStr = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmFixedTable::createColumn( qcmTableInfo  * aTableInfo,
                                    void         ** aMtcColumn,
                                    UInt            aIndex )
{
/***********************************************************************
 *
 * Description :
 *  qcmColumn에 의해 가리켜 질 mtcColumn을 fixedTable기반으로 구축함.
 *
 * Implementation :
 *    ps )
 *    Fixed Table( Performace View )의 mtcColumn의 language는
 *    default language ( = ENGLISH )로 설정됨
 *
 ***********************************************************************/

    mtcColumn       * sColumn;
    SInt              sPrecision;
    SInt              sScale;

    // alloc mtcColumn for pointing from qcmColumn
    IDU_LIMITPOINT("qcmFixedTable::createColumn::malloc");
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_QCM,
                                ID_SIZEOF(mtcColumn),
                                aMtcColumn )
              != IDE_SUCCESS );

    sColumn = (mtcColumn*)*aMtcColumn;

    sColumn->flag = 0;

    switch( smiFixedTable::getColumnType(aTableInfo->tableHandle, aIndex ) )
    {
        case IDU_FT_TYPE_VARCHAR:

            sPrecision =
                smiFixedTable::getColumnLength(aTableInfo->tableHandle,
                                               aIndex );
            sScale     = 0;

            // sColumn의 초기화
            // : dataType은 char, language는 default로 설정
            IDE_TEST( mtc::initializeColumn( sColumn,
                                             MTD_VARCHAR_ID,
                                             1,
                                             sPrecision,
                                             sScale )
                      != IDE_SUCCESS );

            break;
        case IDU_FT_TYPE_CHAR:

            sPrecision =
                smiFixedTable::getColumnLength(aTableInfo->tableHandle,
                                               aIndex );
            sScale     = 0;

            // sColumn의 초기화
            // : dataType은 char, language는 default로 설정
            IDE_TEST( mtc::initializeColumn( sColumn,
                                             MTD_CHAR_ID,
                                             1,
                                             sPrecision,
                                             sScale )
                      != IDE_SUCCESS );

            break;
        case IDU_FT_TYPE_UBIGINT:
        case IDU_FT_TYPE_BIGINT:

            sPrecision = 0;
            sScale     = 0;

            // sColumn의 초기화
            // : dataType은 bigint, language는 default로 설정
            IDE_TEST( mtc::initializeColumn( sColumn,
                                             MTD_BIGINT_ID,
                                             0,
                                             sPrecision,
                                             sScale )
                      != IDE_SUCCESS );

            break;
        case IDU_FT_TYPE_USMALLINT:
        case IDU_FT_TYPE_SMALLINT:

            sPrecision = 0;
            sScale     = 0;

            // sColumn의 초기화
            // : dataType은 smallint, language는 default로 설정
            IDE_TEST( mtc::initializeColumn( sColumn,
                                             MTD_SMALLINT_ID,
                                             0,
                                             sPrecision,
                                             sScale )
                      != IDE_SUCCESS );

            break;
        case IDU_FT_TYPE_UINTEGER:
        case IDU_FT_TYPE_INTEGER:

            sPrecision = 0;
            sScale     = 0;

            // sColumn의 초기화
            // : dataType은 integer, language는 default로 설정
            IDE_TEST( mtc::initializeColumn( sColumn,
                                             MTD_INTEGER_ID,
                                             0,
                                             sPrecision,
                                             sScale )
                      != IDE_SUCCESS );


            break;
        case IDU_FT_TYPE_DOUBLE:

            sPrecision = 0;
            sScale     = 0;

            // sColumn의 초기화
            // : dataType은 double, language는 default로 설정
            IDE_TEST( mtc::initializeColumn( sColumn,
                                             MTD_DOUBLE_ID,
                                             0,
                                             sPrecision,
                                             sScale )
                      != IDE_SUCCESS );

            break;
        default:
            IDE_ASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmFixedTable::makeTrimmedName( SChar * aDst,
                                       SChar * aSrc )
{
/***********************************************************************
 *
 * Description :
 *  string의 ' '값 이후의 string을 trim시킨 string을 만들어 낸다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt i;
    UInt sLength;

    sLength = idlOS::strlen( aSrc );

    for( i = 0; i < sLength; i++ )
    {
        if( aSrc[i] == ' ' )
        {
            break;
        }
        idlOS::memcpy( aDst+i, aSrc+i, 1 );
    }
    aDst[i] = '\0';

    return IDE_SUCCESS;
}

IDE_RC qcmFixedTable::checkDuplicatedTable(
    qcStatement      * aStatement,
    qcNamePosition     aTableName )
{
/***********************************************************************
 *
 * Description :
 *  이미 생성된 fixedTable의 이름이 겹치는지를 validate한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmTableInfo       * sTableInfo = NULL;
    smSCN                sTableSCN;
    void               * sTableHandle = NULL;

    if (qcmFixedTable::getTableInfo( aStatement,
                                     aTableName,
                                     &sTableInfo,
                                     &sTableSCN,
                                     &sTableHandle ) == IDE_SUCCESS)
    {
        IDE_RAISE(ERR_EXIST_OBJECT_NAME);
    }
    else
    {
        IDE_CLEAR();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * BUG-43006 FixedTable Indexing Filter
 *
 * Fixed Table 중에 Indexing Filter로 지정된 컬럼을 찾아서
 * qcmIndex 자료구조를 만들어 준다.
 */
IDE_RC qcmFixedTable::getQcmIndices( qcmTableInfo * aTableInfo )
{
    smiFixedTableHeader  * sHeader;
    iduFixedTableDesc    * sTabDesc;
    iduFixedTableColDesc * sColDesc;
    UInt                   sIndexCount;
    UInt                   i;
    UInt                   j;

    sHeader     = (smiFixedTableHeader *)((UChar *)aTableInfo->tableHandle);
    sTabDesc    = sHeader->mDesc;
    sIndexCount = 0;

    for ( sColDesc = sTabDesc->mColDesc;
          sColDesc->mName != NULL;
          sColDesc++ )
    {
        if ( IDU_FT_IS_COLUMN_INDEX( sColDesc->mDataType ) )
        {
            sIndexCount++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sIndexCount > 0 )
    {
        aTableInfo->indexCount = sIndexCount;

        IDU_FIT_POINT( "qcmFixedTable::getQcmIndices::malloc::indices",
                        idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                     ID_SIZEOF(qcmIndex) * sIndexCount,
                                     (void**)&(aTableInfo->indices) )
                  != IDE_SUCCESS );

        idlOS::memset( aTableInfo->indices, 0x00, ID_SIZEOF(qcmIndex) * sIndexCount );

        for ( i = 0; i < sIndexCount; i++ )
        {
            idlOS::strncpy( aTableInfo->indices[i].userName,
                            "SYSTEM_",
                            QC_MAX_OBJECT_NAME_LEN + 1 );

            idlOS::strncpy( aTableInfo->indices[i].name,
                            "INTERNAL",
                            QC_MAX_OBJECT_NAME_LEN + 1 );

            aTableInfo->indices[i].indexPartitionType = QCM_NONE_PARTITIONED_INDEX;

            IDU_FIT_POINT( "qcmFixedTable::getQcmIndices::malloc::keyColumns",
                            idERR_ABORT_InsufficientMemory );
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                         ID_SIZEOF(mtcColumn),
                                         (void**)&(aTableInfo->indices[i].keyColumns))
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qcmFixedTable::getQcmIndices::malloc::keyColsFlag",
                            idERR_ABORT_InsufficientMemory );
            IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCM,
                                         ID_SIZEOF(UInt),
                                         (void**)&(aTableInfo->indices[i].keyColsFlag))
                      != IDE_SUCCESS );

            // BUG-44697
            aTableInfo->indices[i].indexId = i;
        }

        for ( sColDesc = sTabDesc->mColDesc, i = 0, j = 0;
              sColDesc->mName != NULL;
              sColDesc++, i++ )
        {
            if ( IDU_FT_IS_COLUMN_INDEX( sColDesc->mDataType ) )
            {
                idlOS::memcpy( &(aTableInfo->indices[j].keyColumns[0]),
                               aTableInfo->columns[i].basicInfo,
                               ID_SIZEOF(mtcColumn) );

                *aTableInfo->indices[j].keyColsFlag = 0;

                aTableInfo->indices[j].keyColCount = 1;
                aTableInfo->indices[j].isOnlineTBS = ID_TRUE;
                j++;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

