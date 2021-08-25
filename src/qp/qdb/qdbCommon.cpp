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
 * $Id: qdbCommon.cpp 91517 2021-08-24 01:25:47Z bethy $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdbCommon.h>
#include <qdbAlter.h>
#include <qdn.h>
#include <qmv.h>
#include <qtc.h>
#include <qcm.h>
#include <qcg.h>
#include <qmc.h>
#include <qcmUser.h>
#include <qcmCache.h>
#include <qcmTableSpace.h>
#include <qmvQuerySet.h>
#include <qmo.h>
#include <qmn.h>
#include <qcmTableInfo.h>
#include <qcuSqlSourceInfo.h>
#include <qdx.h>
#include <qdd.h>
#include <qmx.h>
#include <qcuProperty.h>
#include <qcuTemporaryObj.h>
#include <smiTableSpace.h>
#include <qmoPartition.h> // PROJ-1502 PARTITIONED DISK TABLE
#include <smErrorCode.h>
#include <qcmProc.h>
#include <qcs.h>
#include <qcsModule.h>
#include <qcpUtil.h>
#include <qmsDefaultExpr.h>
#include <smiDef.h>
#include <qdpRole.h>
#include <qdtCommon.h>
#include <qsvEnv.h>

extern mtdModule mtdBigint;
extern mtdModule mtdBoolean;
extern mtdModule mtdClobLocator;
extern mtdModule mtdInteger;

/***********************************************************************
 * Description : qsort()가 Platform에 dependent하기 때문에
 *               비교할 인자 외에 seqnum이라는 unique하고 sequence한
 *               값으로 정렬하기 위한 임시 구조체
 ***********************************************************************/
typedef struct qdColumnIdx
{
    qcmColumn* column; // fix BUG-18610
    UInt       seqnum;
} qdColumnIdx;

// PROJ-1502 PARTITIONED DISK TABLE
// 파티션 순서를 정렬하기 위한 임시 구조체
typedef struct qdPartIdx
{
    idvSQL               * statistics; /* PROJ-2446 ONE SOURVE */
    qdPartitionAttribute * partAttr;
    qcmColumn            * column;
} qdPartIdx;

/*
    SM에 Table을 생성한다.

    [IN] aInitFlagMask  - 테이블 생성시 지정할 Flag의 Mask
                          (하나 혹은 그 이상의 Flag가 OR된 값)
    [IN] aInitFlagValue - 테이블 생성시 지정할 Flag의 Value
                          (하나 혹은 그 이상의 Flag가 OR된 값)
 */
IDE_RC qdbCommon::createTableOnSM(
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
    smOID           * aTableOID,
    SChar           * aStmtText,        // default = NULL
    UInt              aStmtTextLen )    // default = 0
{
/***********************************************************************
 *
 * Description :
 *    sm inteface 의 createTable 을 호출하여 테이블을 생성하게 된다.
 *
 * Implementation :
 *    1. make smiColumn list and NULL row => makeSmiColumnListAndNullRow
 *    2. 테이블 스페이스와 테이블 타입에 따라서, scSpaceID 와 flag 을
 *       적절한 값으로 셋팅한다
 *    3. create smiTable => smiTable::createTable 호출
 *    4. 3 에서 반환된 테이블 핸들값으로 테이블 OID 를 구한다
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::createTableOnSM"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const void          * sTableHandle     = NULL;
    smiColumnList       * sSmiColumnList   = NULL;
    smiValue            * sNullRow         = NULL;
    UInt                  sTableCreateFlag = 0;
    UInt                  sTableTypeFlag   = 0;

    IDE_TEST( makeSmiColumnList( aStatement,
                                 aTBSID,
                                 aTableID,
                                 aColumns,
                                 &sSmiColumnList )
              != IDE_SUCCESS);

    // PROJ-1705
    // 디스크테이블에는 null row를 만들지 않는다.
    if( smiTableSpace::isDiskTableSpace( aTBSID )
        == ID_TRUE )
    {
        sNullRow = NULL;
    }
    else
    {
        IDE_TEST( makeMemoryTableNullRow( aStatement,
                                          aColumns,
                                          &sNullRow )
                  != IDE_SUCCESS);
    }

    // set table flag
    if (aUserID == QC_SYSTEM_USER_ID)
    {
        sTableTypeFlag = SMI_TABLE_META;
    }
    else
    {
        if ( smiTableSpace::isMemTableSpace( aTBSID )
             == ID_TRUE )
        {
            sTableTypeFlag = SMI_TABLE_MEMORY;
        }
        /* PROJ-1594 Volatile TBS */
        else if ( smiTableSpace::isVolatileTableSpace( aTBSID )
                  == ID_TRUE )
        {
            sTableTypeFlag = SMI_TABLE_VOLATILE;
        }
        else
        {
            sTableTypeFlag = SMI_TABLE_DISK;
        }
    }

    // Mask에 해당하는 모든 Bit Clear
    sTableCreateFlag = sTableCreateFlag & ~aInitFlagMask;
    // Value에 해당하는 모든 Bit Set
    sTableCreateFlag = sTableCreateFlag | aInitFlagValue;

    /* PROJ-2464 hybrid partitioned table 지원
     *  - Table Type 비교하는 부분을 제거한다.
     *  - TBSID와 동일한 Table Type를 적재한다.
     */
    sTableCreateFlag &= ~SMI_TABLE_TYPE_MASK;
    sTableCreateFlag |= sTableTypeFlag;

    // create smiTable
    IDE_TEST( smiTable::createTable(
                  aStatement->mStatistics,
                  aTBSID,
                  aTableID,
                  QC_SMI_STMT( aStatement ),
                  sSmiColumnList,
                  ID_SIZEOF(mtcColumn),
                  aStmtText,
                  aStmtTextLen,
                  sNullRow,
                  sTableCreateFlag,
                  aMaxRows,
                  aSegAttr,
                  aSegStoAttr,
                  aParallelDegree,
                  &sTableHandle)
              != IDE_SUCCESS);

    // get tableOID
    *aTableOID = smiGetTableId(sTableHandle);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::makeSmiColumnList(
    qcStatement     * aStatement,
    scSpaceID         aTBSID,
    UInt              aTableID,
    qcmColumn       * aColumns,
    smiColumnList  ** aSmiColumnList )
{
/***********************************************************************
 *
 * Description :
 *    smiTable::createTable 호출시 필요한 column 정보를 만든다
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn     * sColumn;
    smiColumnList * sSmiColumnList = NULL;
    UInt            sColumnCount   = 0;
    UInt            i;
    UShort          sMaxAlign       = 0;

    for( sColumn = aColumns, sColumnCount = 0;
         sColumn != NULL;
         sColumn = sColumn->next, sColumnCount ++ ) ;

    // make smi columnlist using aColumns.
    IDU_LIMITPOINT("qdbCommon::makeSmiColumnList::malloc");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiColumnList) * sColumnCount,
                                       (void**)&sSmiColumnList)
             != IDE_SUCCESS);

    for ( i = 0, sColumn = aColumns;
          i < sColumnCount;
          i++, sColumn = sColumn->next )
    {
        // set column ID
        // aColumns는 parseTree->columns 임
        sColumn->basicInfo->column.id = (aTableID * SMI_COLUMN_ID_MAXIMUM) + i;
        sColumn->basicInfo->column.value = NULL;
        
        if( ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
            == SMI_COLUMN_TYPE_LOB )
        {
            // BUG-17057
            // smiColumn.colSpace는 이미 지정했다.
        }
        else
        {
            // BUG-17057
            sColumn->basicInfo->column.colSpace = aTBSID;
        }
        //BUG-43117 smiColumn에 align값 입력
        ((smiColumn*)sColumn->basicInfo)->align = (UShort)sColumn->basicInfo->module->align;

        /* BUG-43287 Variable column 들의 Align 값중 가장 큰 값을 구한다. */
        if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE )
        {
            sMaxAlign = IDL_MAX( sMaxAlign, (UShort)sColumn->basicInfo->module->align );
        }
        else
        {
            // Nothing to do
        }

        // BUG-44814 smiTable::createTable 을 호출하기전에 통계정보를 clear 해야 한다.
        idlOS::memset( &(sColumn->basicInfo->column.mStat), 0x00, ID_SIZEOF(smiColumnStat) );
        
        sSmiColumnList[i].column = (smiColumn*)sColumn->basicInfo;

        if (i == sColumnCount - 1)
        {
            sSmiColumnList[i].next = NULL;
        }
        else
        {
            sSmiColumnList[i].next = &sSmiColumnList[i+1];
        }
    }

    /* BUG-43287 Variable column 들의 Align 값중 가장 큰 값을 기록해둔다. */
    for ( i = 0, sColumn = aColumns; i < sColumnCount; i++, sColumn = sColumn->next )
    {
        sColumn->basicInfo->column.maxAlign = sMaxAlign;
    }

    *aSmiColumnList = sSmiColumnList;

    /* BUG-45503 Table 생성 이후에 실패 시, Table Meta Cache의 Column 정보를 복구하지 않는 경우가 있습니다. */
    IDU_FIT_POINT( "qdbCommon::makeSmiColumnList::fatal::sColumn", idERR_ABORT_InsufficientMemory );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeMemoryTableNullRow(
    qcStatement     * aStatement,
    qcmColumn       * aColumns,
    smiValue       ** aNullRow)
{
/***********************************************************************
 *
 * Description :
 *    smiTable::createTable 호출시 필요한 null 로 채워진
 *    row 를 만든다
 *
 * Implementation :
 *    1. qcmColumn 의 각 컬럼의 offset 과 8 byte align 으로 계산해서
 *       null 값의 한 레코드를 만드는데 필요한 사이즈를 계산한다.
 *    2. smiValue 를 컬럼개수만큼 할당하고, NullRow 버퍼를
 *       1 에서 계산한 값만큼 할당한다
 *    3. 각 컬럼의 module 에서 널값을 셋팅한다
 *    4. 3 을 컬럼개수만큼 반복해서 smiColumnList 와 널값이 셋팅된
 *       smiValue 를 반환한다
 *
 ***********************************************************************/

    qcmColumn     * sColumn;
    qcmColumn     * sSelectedColumn;
    SChar         * sNullRowValue;
    smiValue      * sNullRow       = NULL;
    UInt            sColumnCount   = 0;
    UInt            sCurrentOffset = 0;
    UInt            i;
    UInt            sStoringSize   = 0;
    void          * sStoringValue;
    // PROJ-2264
    SChar         * sValue;
    const void    * sDicTableHandle;

    // fix BUG-10785
    // 레코드 사이즈를 구할 때는 맨 마지막 column을 가지고
    // 구하는 것이 아니라 가장 큰 offset을 가진 column으로 구해야 함
    sSelectedColumn = aColumns;

    for (sColumn = aColumns; sColumn != NULL; sColumn = sColumn->next)
    {
        sColumnCount++;

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

    if( (sSelectedColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK) == SMI_COLUMN_COMPRESSION_FALSE )
    {
        sCurrentOffset = sSelectedColumn->basicInfo->column.offset
                + idlOS::align8(sSelectedColumn->basicInfo->module->nullValueSize());
    }
    else
    {
        sCurrentOffset = sSelectedColumn->basicInfo->column.offset
                + idlOS::align8( ID_SIZEOF(smOID) );
    }

    IDU_LIMITPOINT("qdbCommon::makeMemoryTableNullRow::malloc1");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) * sColumnCount,
                                       (void**)&sNullRow)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::makeMemoryTableNullRow::malloc2");
    IDE_TEST(aStatement->qmxMem->alloc( sCurrentOffset, (void**)&sNullRowValue )
             != IDE_SUCCESS);

    for ( i = 0, sColumn = aColumns;
          i < sColumnCount;
          i++, sColumn = sColumn->next )
    {
        // PROJ-2264 Dictionary table
        if( (sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK)
            == SMI_COLUMN_COMPRESSION_FALSE )
        {
            // set NULL row
            // PROJ-1391 Variable Null
            // Variable column에 대해 null 값을 smiValue.value에 NULL을 할당한다.
            if( ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                == SMI_COLUMN_TYPE_LOB )
            {
                // PROJ-1362
                sNullRow[i].value = NULL;
                sNullRow[i].length = 0;

                // BUG-17057
                // smiColumn.colSpace는 이미 지정했다.
            }
            else if ( ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                        == SMI_COLUMN_TYPE_VARIABLE ) ||
                      ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                        == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
            {
                sNullRow[i].value = NULL;
                sNullRow[i].length = 0;
            }
            else
            {
                sColumn->basicInfo->module->null( (sColumn->basicInfo),
                                                  sNullRowValue + sColumn->basicInfo->column.offset );

                IDE_TEST( qdbCommon::storingSize(
                              sColumn->basicInfo,
                              sColumn->basicInfo,
                              sNullRowValue + sColumn->basicInfo->column.offset,
                              &sStoringSize )
                          != IDE_SUCCESS );
                sNullRow[i].length = sStoringSize;

                IDE_TEST( qdbCommon::mtdValue2StoringValue(
                              sColumn->basicInfo,
                              sColumn->basicInfo,
                              sNullRowValue + sColumn->basicInfo->column.offset,
                              &sStoringValue )
                          != IDE_SUCCESS );
                sNullRow[i].value = sStoringValue;
            }
        }
        else
        {
            // PROJ-2264 Dictionary table
            sDicTableHandle = smiGetTable( sColumn->basicInfo->column.mDictionaryTableOID );

            sValue = sNullRowValue + sColumn->basicInfo->column.offset;
            idlOS::memcpy( sValue, &(SMI_MISC_TABLE_HEADER(sDicTableHandle)->mNullOID), ID_SIZEOF(smOID) );

            // OID 는 canonize가 필요없다.
            // OID 는 memory table 이므로 mtd value 와 storing value 가 동일하다.
            sNullRow[i].value  = sValue;
            sNullRow[i].length = ID_SIZEOF(smOID);
        }
    }

    *aNullRow       = sNullRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::updateTableSpecFromMeta(
    qcStatement     * aStatement,
    qcNamePosition    aUserNamePos,
    qcNamePosition    aTableNamePos,
    UInt              aTableID,
    smOID             aTableOID,
    SInt              aColumnCount,
    UInt              aParallelDegree )
{
/***********************************************************************
 *
 * Description :
 *      ALTER TABLE 관련 SYS_TABLES_ 메타 테이블 변경 수행
 *
 * Implementation :
 *      1. 사용자 검사, USER_ID 구함
 *      2. SYS_TABLES_ 메타 테이블의 정보 변경
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::updateTableSpecFromMeta"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt        sUserID;
    SChar     * sSqlStr;
    SChar       sTableName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    vSLong      sRowCnt;

    // check user existence
    if (QC_IS_NULL_NAME(aUserNamePos) == ID_TRUE)
    {
        sUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        IDE_TEST(qcmUser::getUserID( aStatement, aUserNamePos, &sUserID )
                 != IDE_SUCCESS);
    }

    QC_STR_COPY( sTableName, aTableNamePos );

    IDU_LIMITPOINT("qdbCommon::updateTableSpecFromMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLES_ "
                     "SET USER_ID = INTEGER'%"ID_INT32_FMT"', "
                     "TABLE_NAME = '%s', "
                     "TABLE_OID = BIGINT'%"ID_INT64_FMT"', "
                     "COLUMN_COUNT = INTEGER'%"ID_INT32_FMT"', "
                     "PARALLEL_DEGREE = INTEGER'%"ID_INT32_FMT"', "
                     "LAST_DDL_TIME = SYSDATE "  // fix BUG-14394
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                     sUserID,
                     sTableName,
                     (ULong)aTableOID,
                     aColumnCount,
                     aParallelDegree,
                     aTableID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::createConstraintFromInfo(
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
    qcmColumn         * aDelColList )
{
/***********************************************************************
 *
 * Description :
 *    executeAddCol,executeDropCol 에서 호출, 인덱스 재생성
 *
 * Implementation :
 *    uniqueKey 개수만큼 반복
 *        각 constraint를 구성하는 컬럼 개수만큼 반복
 *            if DropCol 이면
 *               aDelColList 가 constraint 에 속해 있으면
 *               재생성할 필요 없음
 *               그렇지 않으면 constraintColumn 의 값 조정
 *            else : AddCol
 *               기존 constraintColumn 값을 그대로 사용
 *            constraintColumn 값으로
 *            sNewTableIndexColumn,sColumnList 배열 구성
 *        이 키에 해당하는 인덱스 ID 구하기
 *        인덱스 생성 => smiTable::createIndex
 *        생성된 인덱스,constraint 정보를 메타 테이블로 입력
 *    foreignkey 개수만큼 반복
 *        각 constraint를 구성하는 referencing 컬럼 개수만큼 반복
 *            if DropCol 이면
 *               aDelColList 가 referencing 컬럼 에 속해 있으면
 *               재구성할 필요 없음
 *               그렇지 않으면 constraintColumn 의 값 조정
 *            else : AddCol
 *               기존 referencingColumn 값을 그대로 사용
 *        메타 테이블에 포린 키 정보 입력
 *    not null constraint 개수만큼 반복
 *        각 constraint를 구성하는 컬럼 개수만큼 반복
 *            if DropCol 이면
 *               aDelColList 가 constraint 에 속해 있으면
 *               재생성할 필요 없음
 *               그렇지 않으면 constraintColumn 의 값 조정
 *            else : AddCol
 *               기존 constraintColumn 값을 그대로 사용
 *    check constraint 개수만큼 반복
 *        각 constraint를 구성하는 컬럼 개수만큼 반복
 *            if DropCol 이면
 *               aDelColList 가 constraint 에 속해 있으면
 *               재생성할 필요 없음
 *               그렇지 않으면 constraintColumn 의 값 조정
 *            else : AddCol
 *               기존 constraintColumn 값을 그대로 사용
 *    timestamp constraint가 존재하면
 *            if DropCol 이면
 *               aDelColList 가 constraint 에 속해 있으면
 *               재생성할 필요 없음
 *            else :
 *               기존 constraintColumn 값을 그대로 사용
 *
 ***********************************************************************/

    UInt                    i;
    UInt                    j;
    UInt                    k;
    UInt                    sOffset;
    UInt                    sTableType;
    UInt                    sConstraintColumn[QC_MAX_KEY_COLUMN_COUNT];
    smiColumnList           sColumnListAtRow[QC_MAX_KEY_COLUMN_COUNT];
    smiColumnList           sColumnListAtKey[QC_MAX_KEY_COLUMN_COUNT];
    mtcColumn               sNewTableIndexColumnAtRow[QC_MAX_KEY_COLUMN_COUNT];
    mtcColumn               sNewTableIndexColumnAtKey[QC_MAX_KEY_COLUMN_COUNT];
    idBool                  sExcludeConstraint = ID_FALSE;
    SChar                 * sCheckConditionStrForMeta = NULL;
    qcmIndex              * sIndex;
    qcmUnique             * sUnique;
    qcmForeignKey         * sForeign;
    const void            * sIndexHandle;
    UInt                    sFlag               = 0;
    const void            * sNewTableHandle;
    qcmIndex              * sOldIndex;
    idBool                  sIsPartitionedTable = ID_FALSE;
    idBool                  sIsPartitionedIndex;
    idBool                  sIsPrimary = ID_FALSE;
    qcmNotNull            * sNotNulls;
    qcmCheck              * sChecks;
    qcmTimeStamp          * sTimestamp;
    qdIndexTableList      * sOldIndexTable;
    qdIndexTableList      * sNewIndexTable;
    qcmIndex              * sIndexTableIndex[2];
    qcNamePosition          sIndexTableNamePos;
    UInt                    sIndexTableFlag;
    UInt                    sIndexTableParallelDegree;
    qcmTableInfo          * sIndexTableInfo;
    qcmIndex              * sIndexPartition = NULL;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStoAttr;
    mtcColumn             * sMtcColumn;

    qcmColumn             * sTempColumns = NULL;

    sNewTableHandle = smiGetTable( aNewTableOID );
    sTableType = aTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;

    // PROJ-1502 PARTITIONED DISK TABLE
    if( aTableInfo->tablePartitionType != QCM_NONE_PARTITIONED_TABLE )
    {
        sIsPartitionedTable = ID_TRUE;
    }
    else
    {
        sIsPartitionedTable = ID_FALSE;
    }

    for (i = 0; i < aTableInfo->uniqueKeyCount; i ++)
    {
        sExcludeConstraint = ID_FALSE;

        sUnique = &(aTableInfo->uniqueKeys[i]);
        sIndex = sUnique->constraintIndex;
        sOffset = 0;

        // PROJ-1624 non-partitioned index
        // primary key index의 경우 non-partitioned index와 partitioned index
        // 둘 다 생성한다.
        if ( aTableInfo->primaryKey != NULL )
        {
            if ( aTableInfo->primaryKey->indexId == sIndex->indexId )
            {
                sIsPrimary = ID_TRUE;
            }
            else
            {
                sIsPrimary = ID_FALSE;
            }
        }
        else
        {
            // Nothing to do.
        }
        
        for (j = 0; j < sUnique->constraintColumnCount; j++)
        {
            if (aDelColList != NULL)
            {
                if ( findColumnIDInColumnList( aDelColList,
                                               sUnique->constraintColumn[j])
                     == ID_TRUE )
                {
                    sExcludeConstraint = ID_TRUE;
                    break;
                }
                else
                {
                    // 삭제할 column ID를 고려하여 삭제되지 않은 column ID에
                    // 대하여 새로운 column ID를 계산한다.
                    sConstraintColumn[j] = getNewColumnIDForAlter( aDelColList,
                                                                   sUnique->constraintColumn[j] );
                }
            }
            else
            {
                sConstraintColumn[j] = sUnique->constraintColumn[j];
            }

            IDE_TEST( smiGetTableColumns( sNewTableHandle,
                                          (sConstraintColumn[j] % QC_MAX_COLUMN_COUNT),
                                          (const smiColumn**)&sMtcColumn )
                      != IDE_SUCCESS );

            idlOS::memcpy(
                &(sNewTableIndexColumnAtRow[j]),
                sMtcColumn,
                ID_SIZEOF (mtcColumn));
            sNewTableIndexColumnAtRow[j].column.flag = sIndex->keyColsFlag[j];

            idlOS::memcpy(&(sNewTableIndexColumnAtKey[j]),
                          &(sNewTableIndexColumnAtRow[j]),
                          ID_SIZEOF (mtcColumn));

            // tableType == DISK 이면 offset 재조정
            if ( sTableType == SMI_TABLE_DISK )
            {
                // PROJ-1705
                IDE_TEST(
                    qdbCommon::setIndexKeyColumnTypeFlag( &(sNewTableIndexColumnAtKey[j]) )
                    != IDE_SUCCESS );

                // To Fix PR-8111
                if ( (sIndex->keyColumns[j].column.flag & SMI_COLUMN_TYPE_MASK)
                     == SMI_COLUMN_TYPE_FIXED )
                {
                    sOffset = idlOS::align( sOffset,
                                            sIndex->keyColumns[j].module->align );
                    sNewTableIndexColumnAtKey[j].column.offset = sOffset;
                    sNewTableIndexColumnAtKey[j].column.value = NULL;
                    sOffset += sNewTableIndexColumnAtRow[j].column.size;
                }
                else
                {
                    sOffset = idlOS::align( sOffset, 8 );
                    sNewTableIndexColumnAtKey[j].column.offset = sOffset;
                    sNewTableIndexColumnAtKey[j].column.value = NULL;
                    sOffset += smiGetVariableColumnSize4DiskIndex();
                }
            }

            sColumnListAtRow[j].column =
                (smiColumn*) &sNewTableIndexColumnAtRow[j];
            sColumnListAtKey[j].column =
                (smiColumn*) &sNewTableIndexColumnAtKey[j];

            if (j == sUnique->constraintColumnCount - 1)
            {
                sColumnListAtRow[j].next = NULL;
                sColumnListAtKey[j].next = NULL;
            }
            else
            {
                sColumnListAtRow[j].next = &sColumnListAtRow[j+1];
                sColumnListAtKey[j].next = &sColumnListAtKey[j+1];
            }
        }

        for (j = 0; j < aTableInfo->indexCount; j++)
        {
            if (sUnique->constraintIndex->indexId ==
                aNewTableIndex[j].indexId)
            {
                break;
            }
        }

        if (sExcludeConstraint == ID_FALSE)
        {
            IDE_TEST(qcmCache::getIndexByID(aTableInfo,
                                            sUnique->constraintIndex->indexId,
                                            &sOldIndex)
                     != IDE_SUCCESS);

            sFlag = smiTable::getIndexInfo(sOldIndex->indexHandle);

            IDE_ASSERT( aNewTableIndex != NULL );

            IDE_TEST(smiTable::createIndex(aStatement->mStatistics,
                                           QC_SMI_STMT( aStatement ),
                                           sOldIndex->TBSID,
                                           sNewTableHandle,
                                           (SChar*)aNewTableIndex[j].name,
                                           aNewTableIndex[j].indexId,
                                           aNewTableIndex[j].indexTypeId,
                                           sColumnListAtKey,
                                           sFlag,
                                           QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                           aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                           smiTable::getIndexSegAttr(sOldIndex->indexHandle),
                                           smiTable::getIndexSegStoAttr(sOldIndex->indexHandle),
                                           smiTable::getIndexMaxKeySize( sOldIndex->indexHandle ), /* PROJ-2433 */
                                           &sIndexHandle)
                     != IDE_SUCCESS);

            // BUG-45492 ddl 구문 수행시 통계정보 복사를 해야함
            smiStatistics::copyIndexStats( sIndexHandle, sOldIndex->indexHandle );

            // PROJ-1624 global non-partitioned index
            if ( ( sIsPartitionedTable == ID_TRUE ) &&
                 ( aNewTableIndex[j].indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) )
            {
                sIsPartitionedIndex = ID_TRUE;
            }
            else
            {
                sIsPartitionedIndex = ID_FALSE;
            }
            
            IDE_TEST( qdx::insertIndexIntoMeta(
                          aStatement,
                          sOldIndex->TBSID,
                          aTableInfo->tableOwnerID,
                          aTableInfo->tableID,
                          aNewTableIndex[j].indexId,
                          aNewTableIndex[j].name,
                          aNewTableIndex[j].indexTypeId,
                          ID_TRUE,
                          sUnique->constraintColumnCount,
                          ID_TRUE,
                          sIsPartitionedIndex,
                          0,
                          sFlag)
                      != IDE_SUCCESS);

            IDE_TEST( qdn::insertConstraintIntoMeta(
                          aStatement,
                          aTableInfo->tableOwnerID,
                          aTableInfo->tableID,
                          sUnique->constraintID,
                          sUnique->name,
                          sUnique->constraintType,
                          aNewTableIndex[j].indexId,
                          sUnique->constraintColumnCount,
                          0, 0, 0,
                          (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                          ID_TRUE ) // ConstraintState의 Validate
                      != IDE_SUCCESS );

            if( sIsPartitionedTable == ID_TRUE )
            {
                /* PROJ-2334 PMT */
                if ( sTableType == SMI_TABLE_DISK )
                {    
                    // PROJ-1624 global non-partitioned index
                    if ( aNewTableIndex[j].indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
                    {
                        //--------------------------------
                        // (global) non-partitioned index
                        //--------------------------------

                        // non-partitioned index에 해당하는 index table을 찾는다.
                        IDE_TEST( qdx::findIndexTableInList( aOldIndexTables,
                                                             aNewTableIndex[j].indexTableID,
                                                             & sOldIndexTable )
                                  != IDE_SUCCESS );
                    
                        sIndexTableFlag = sOldIndexTable->tableInfo->tableFlag;
                        sIndexTableParallelDegree = sOldIndexTable->tableInfo->parallelDegree;

                        sIndexTableNamePos.stmtText = sOldIndexTable->tableInfo->name;
                        sIndexTableNamePos.offset   = 0;
                        sIndexTableNamePos.size     =
                            idlOS::strlen(sOldIndexTable->tableInfo->name);

                        /* BUG-45503 Table 생성 이후에 실패 시, Table Meta Cache의 Column 정보를 복구하지 않는 경우가 있습니다. */
                        IDE_TEST( qcm::copyQcmColumns( QC_QMX_MEM( aStatement ),
                                                       sOldIndexTable->tableInfo->columns,
                                                       & sTempColumns,
                                                       sOldIndexTable->tableInfo->columnCount )
                                  != IDE_SUCCESS );

                        IDE_TEST( qdx::createIndexTable( aStatement,
                                                         sOldIndexTable->tableInfo->tableOwnerID,
                                                         sIndexTableNamePos,
                                                         sTempColumns,
                                                         sOldIndexTable->tableInfo->columnCount,
                                                         sOldIndexTable->tableInfo->TBSID,
                                                         sOldIndexTable->tableInfo->segAttr,
                                                         sOldIndexTable->tableInfo->segStoAttr,
                                                         QDB_TABLE_ATTR_MASK_ALL,
                                                         sIndexTableFlag, /* Flag Value */
                                                         sIndexTableParallelDegree,
                                                         & sNewIndexTable )
                                  != IDE_SUCCESS );

                        // link new index table
                        sNewIndexTable->next = *aNewIndexTables;
                        *aNewIndexTables = sNewIndexTable;

                        // key index, rid index를 찾는다.
                        IDE_TEST( qdx::getIndexTableIndices( sOldIndexTable->tableInfo,
                                                             sIndexTableIndex )
                                  != IDE_SUCCESS );
                        
                        sFlag = smiTable::getIndexInfo(sIndexTableIndex[0]->indexHandle);
                        sSegAttr = smiTable::getIndexSegAttr(sIndexTableIndex[0]->indexHandle);
                        sSegStoAttr = smiTable::getIndexSegStoAttr(sIndexTableIndex[0]->indexHandle);
                        
                        IDE_TEST( qdx::createIndexTableIndices(
                                      aStatement,
                                      sOldIndexTable->tableInfo->tableOwnerID,
                                      sNewIndexTable,
                                      NULL,
                                      sIndexTableIndex[0]->name,
                                      sIndexTableIndex[1]->name,
                                      sIndexTableIndex[0]->TBSID,
                                      sIndexTableIndex[0]->indexTypeId,
                                      sFlag,
                                      QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                      aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                      sSegAttr,
                                      sSegStoAttr,
                                      0 ) /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                                  != IDE_SUCCESS );

                        // tableInfo 재생성
                        sIndexTableInfo = sNewIndexTable->tableInfo;
                    
                        IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT(aStatement),
                                                             sNewIndexTable->tableID,
                                                             sNewIndexTable->tableOID)
                                 != IDE_SUCCESS);
            
                        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                                       sNewIndexTable->tableID,
                                                       &(sNewIndexTable->tableInfo),
                                                       &(sNewIndexTable->tableSCN),
                                                       &(sNewIndexTable->tableHandle))
                                 != IDE_SUCCESS);
            
                        (void)qcm::destroyQcmTableInfo(sIndexTableInfo);
                    
                        // index table id 설정
                        aNewTableIndex[j].indexTableID = sNewIndexTable->tableID;
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
                            
                // primary key index의 경우 non-partitioned index와 partitioned index
                // 둘 다 생성한다.
                if ( ( aNewTableIndex[j].indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) ||
                     ( sIsPrimary == ID_TRUE ) )
                {
                    //--------------------------------
                    // partitioned index
                    //--------------------------------
                
                    for( k = 0; k < aPartitionCount; k++ )
                    {
                        IDE_ASSERT( aNewPartIndex != NULL );

                        // partitioned index에 해당하는 local partition index를 찾는다.
                        IDE_TEST( qdx::findIndexIDInIndices( aNewPartIndex[k],
                                                             aNewPartIndexCount,
                                                             aNewTableIndex[j].indexId,
                                                             & sIndexPartition )
                                  != IDE_SUCCESS );

                        /* PROJ-2464 hybrid partitioned table 지원
                         *  - Column 또는 Index 중 하나만 전달해야 한다.
                         */
                        IDE_TEST( adjustIndexColumn( NULL,
                                                     sIndexPartition,
                                                     aDelColList,
                                                     sColumnListAtKey )
                                  != IDE_SUCCESS );

                        /* PROJ-2464 hybrid partitioned table 지원 */
                        sFlag             = smiTable::getIndexInfo( sIndexPartition->indexHandle );
                        sSegAttr          = smiTable::getIndexSegAttr( sIndexPartition->indexHandle );
                        sSegStoAttr       = smiTable::getIndexSegStoAttr( sIndexPartition->indexHandle );

                        IDE_TEST( smiTable::createIndex(
                                      aStatement->mStatistics,
                                      QC_SMI_STMT( aStatement ),
                                      sIndexPartition->TBSID, // index partition
                                      smiGetTable( aNewPartitionOID[k] ),
                                      (SChar*)sIndexPartition->name,
                                      aNewTableIndex[j].indexId,
                                      aNewTableIndex[j].indexTypeId,
                                      sColumnListAtKey,
                                      sFlag,
                                      QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                      aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                      sSegAttr,
                                      sSegStoAttr,
                                      0, /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                                      &sIndexHandle )
                                  != IDE_SUCCESS );

                        // BUG-45492 ddl 구문 수행시 통계정보 복사를 해야함
                        smiStatistics::copyIndexStats( sIndexHandle, sIndexPartition->indexHandle );
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }

            for (k = 0; k < sUnique->constraintColumnCount; k++)
            {
                IDE_TEST(qdx::insertIndexColumnIntoMeta(
                             aStatement,
                             aTableInfo->tableOwnerID,
                             aNewTableIndex[j].indexId,
                             sColumnListAtKey[k].column->id,  // BUG-16543
                             k,
                             ( ( (aNewTableIndex[j].keyColsFlag[k] &
                                  SMI_COLUMN_ORDER_MASK) == SMI_COLUMN_ORDER_ASCENDING )
                               ? ID_TRUE : ID_FALSE ),
                             aTableInfo->tableID) != IDE_SUCCESS);
            }

            for (k = 0; k < sUnique->constraintColumnCount; k++)
            {
                IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                              aStatement,
                              aTableInfo->tableOwnerID,
                              aTableInfo->tableID,
                              sUnique->constraintID,
                              k,
                              sConstraintColumn[k])
                          != IDE_SUCCESS);
            }
            aNewTableIndex[j].indexId = 0;
        }
    }

    for (i = 0; i < aTableInfo->foreignKeyCount; i++)
    {
        sExcludeConstraint = ID_FALSE;
        sForeign = &(aTableInfo->foreignKeys[i]);

        if (aDelColList != NULL)
        {
            for (j = 0; j < sForeign->constraintColumnCount; j++)
            {
                if ( findColumnIDInColumnList( aDelColList,
                                               sForeign->referencingColumn[j] )
                     == ID_TRUE )
                {
                    sExcludeConstraint = ID_TRUE;
                    break;
                }
                else
                {
                    // 삭제할 column ID를 고려하여 삭제되지 않은 column ID에
                    // 대하여 새로운 column ID를 계산한다.
                    sConstraintColumn[j] = getNewColumnIDForAlter( aDelColList,
                                                                  sForeign->referencingColumn[j] );
                }
            }
        }
        else
        {
            idlOS::memcpy(sConstraintColumn, sForeign->referencingColumn,
                          ID_SIZEOF (UInt) * QC_MAX_KEY_COLUMN_COUNT);
        }

        if (sExcludeConstraint == ID_FALSE)
        {
            IDE_TEST( qdn::insertConstraintIntoMeta(
                          aStatement,
                          aTableInfo->tableOwnerID,
                          aTableInfo->tableID,
                          sForeign->constraintID,
                          sForeign->name,
                          QD_FOREIGN,
                          0,
                          sForeign->constraintColumnCount,
                          sForeign->referencedTableID,
                          sForeign->referencedIndexID,
                          sForeign->referenceRule,
                          (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                          sForeign->validated ) // ConstraintState의 Validate
                      != IDE_SUCCESS );

            for (k = 0; k < sForeign->constraintColumnCount; k++)
            {
                IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                              aStatement,
                              aTableInfo->tableOwnerID,
                              aTableInfo->tableID,
                              sForeign->constraintID,
                              k,
                              sConstraintColumn[k])
                          != IDE_SUCCESS);
            }
        }
    }

    // fix BUG-20227
    for (i = 0; i < aTableInfo->notNullCount; i++)
    {
        sExcludeConstraint = ID_FALSE;
        sNotNulls = &(aTableInfo->notNulls[i]);

        if (aDelColList != NULL)
        {
            for (j = 0; j < sNotNulls->constraintColumnCount; j++)
            {
                if ( findColumnIDInColumnList( aDelColList,
                                               sNotNulls->constraintColumn[j] )
                     == ID_TRUE )
                {
                    sExcludeConstraint = ID_TRUE;
                    break;
                }
                else
                {
                    // 삭제할 column ID를 고려하여 삭제되지 않은 column ID에
                    // 대하여 새로운 column ID를 계산한다.
                    sConstraintColumn[j] = getNewColumnIDForAlter( aDelColList,
                                                                  sNotNulls->constraintColumn[j] );
                }
            }
        }
        else
        {
            idlOS::memcpy(sConstraintColumn, sNotNulls->constraintColumn,
                          ID_SIZEOF (UInt) * QC_MAX_KEY_COLUMN_COUNT);
        }

        if (sExcludeConstraint == ID_FALSE)
        {
            IDE_TEST( qdn::insertConstraintIntoMeta(
                          aStatement,
                          aTableInfo->tableOwnerID,
                          aTableInfo->tableID,
                          sNotNulls->constraintID,
                          sNotNulls->name,
                          QD_NOT_NULL,
                          0,
                          sNotNulls->constraintColumnCount,
                          0,
                          0,
                          0,
                          (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                          ID_TRUE ) // ConstraintState의 Validate
                      != IDE_SUCCESS );

            for (k = 0; k < sNotNulls->constraintColumnCount; k++)
            {
                IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                              aStatement,
                              aTableInfo->tableOwnerID,
                              aTableInfo->tableID,
                              sNotNulls->constraintID,
                              k,
                              sConstraintColumn[k])
                          != IDE_SUCCESS);
            }
        }
    }

    /* PROJ-1107 Check Constraint 지원 */
    for ( i = 0; i < aTableInfo->checkCount; i++ )
    {
        sExcludeConstraint = ID_FALSE;
        sChecks = &(aTableInfo->checks[i]);

        if ( aDelColList != NULL )
        {
            for ( j = 0; j < sChecks->constraintColumnCount; j++ )
            {
                if ( findColumnIDInColumnList( aDelColList,
                                               sChecks->constraintColumn[j] )
                     == ID_TRUE )
                {
                    sExcludeConstraint = ID_TRUE;
                    break;
                }
                else
                {
                    /* 삭제할 column ID를 고려하여, 삭제되지 않은 column ID에
                     * 대하여 새로운 column ID를 계산한다.
                     */
                    sConstraintColumn[j] = getNewColumnIDForAlter( aDelColList,
                                                                   sChecks->constraintColumn[j] );
                }
            }
        }
        else
        {
            idlOS::memcpy( sConstraintColumn, sChecks->constraintColumn,
                           ID_SIZEOF(UInt) * QC_MAX_KEY_COLUMN_COUNT );
        }

        if ( sExcludeConstraint == ID_FALSE )
        {
            // BUG-38190
            IDE_TEST( getStrForMeta( aStatement,
                                     sChecks->checkCondition,
                                     idlOS::strlen(sChecks->checkCondition),
                                     &sCheckConditionStrForMeta )
                      != IDE_SUCCESS );
            
            IDE_TEST( qdn::insertConstraintIntoMeta(
                            aStatement,
                            aTableInfo->tableOwnerID,
                            aTableInfo->tableID,
                            sChecks->constraintID,
                            sChecks->name,
                            QD_CHECK,
                            0,
                            sChecks->constraintColumnCount,
                            0,
                            0,
                            0,
                            sCheckConditionStrForMeta,
                            ID_TRUE ) // ConstraintState의 Validate
                      != IDE_SUCCESS );

            for ( k = 0; k < sChecks->constraintColumnCount; k++ )
            {
                IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                                aStatement,
                                aTableInfo->tableOwnerID,
                                aTableInfo->tableID,
                                sChecks->constraintID,
                                k,
                                sConstraintColumn[k] )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    if (aTableInfo->timestamp != NULL)
    {
        sTimestamp = aTableInfo->timestamp;

        if ( findColumnIDInColumnList( aDelColList,
                                       sTimestamp->constraintColumn[0] )
             == ID_FALSE )
        {
            sConstraintColumn[0] = getNewColumnIDForAlter( aDelColList,
                                                           sTimestamp->constraintColumn[0] );

            IDE_TEST( qdn::insertConstraintIntoMeta(
                          aStatement,
                          aTableInfo->tableOwnerID,
                          aTableInfo->tableID,
                          sTimestamp->constraintID,
                          sTimestamp->name,
                          QD_TIMESTAMP,
                          0,
                          sTimestamp->constraintColumnCount,
                          0,
                          0,
                          0,
                          (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                          ID_TRUE ) // ConstraintState의 Validate
                      != IDE_SUCCESS );

            IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                          aStatement,
                          aTableInfo->tableOwnerID,
                          aTableInfo->tableID,
                          sTimestamp->constraintID,
                          0,
                          sConstraintColumn[0])
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::createIndexFromInfo(
    qcStatement          * aStatement,
    qcmTableInfo         * aTableInfo,
    smOID                  aNewTableOID,
    UInt                   aPartitionCount,
    smOID                * aNewPartitionOID,
    UInt                   aIndexCrtFlag,
    qcmIndex             * aIndices,
    qcmIndex            ** aPartIndices,
    UInt                   aPartIndexCount,
    qdIndexTableList     * aOldIndexTables,
    qdIndexTableList    ** aNewIndexTables,
    qcmColumn            * aDelColList,
    idBool                 aCreateMetaFlag )
{
/***********************************************************************
 *
 * Description :
 *    executeAddCol,executeCompactTable,executeDropCol,
 *    executeTruncateTable 에서 호출, 인덱스 재생성
 *
 * Implementation :
 *    인덱스 개수만큼 반복
 *        각 키를 구성하는 컬럼 개수만큼 반복 *
 *            if aDeleteColID 가 인덱스 구성 컬럼에 속해 있으면 재생성할 필요 없음
 *            그렇지 않으면 sNewTableIndexColumn,sColumnList 배열 구성
 *        인덱스 생성 => smiTable::createIndex
 *        생성된 인덱스,constraint 정보를 메타 테이블로 입력
 *        입력 인자 aCreateMetaFlag 값에 따라서
 *        메타 테이블에 인덱스 정보 입력 여부 판단
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::createIndexFromInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                    sOffset;
    mtcColumn               sNewTableIndexColumnAtRow[QC_MAX_KEY_COLUMN_COUNT];
    mtcColumn               sNewTableIndexColumnAtKey[QC_MAX_KEY_COLUMN_COUNT];
    const void            * sNewTableHandle;
    UInt                    i;
    UInt                    j;
    UInt                    sTableType;
    idBool                  sExcludeIndex;
    smiColumnList           sColumnListAtRow[QC_MAX_KEY_COLUMN_COUNT];
    smiColumnList           sColumnListAtKey[QC_MAX_KEY_COLUMN_COUNT];
    UInt                    sFlag = 0;
    const void            * sIndexHandle;
    idBool                  sColOrderFlag;
    mtcColumn             * sTempColumn;
    idBool                  sIsPartitionedTable = ID_FALSE;
    idBool                  sIsPartitionedIndex;
    idBool                  sIsPrimary = ID_FALSE;
    UInt                    sIndexColumnID;
    qdIndexTableList      * sOldIndexTable;
    qdIndexTableList      * sNewIndexTable;
    qcmIndex              * sIndexTableIndex[2];
    qcNamePosition          sIndexTableNamePos;
    UInt                    sIndexTableFlag;
    UInt                    sIndexTableParallelDegree;
    qcmTableInfo          * sIndexTableInfo;
    qcmIndex              * sIndexPartition = NULL;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStoAttr;


    qcmColumn             * sTempColumns = NULL;

    sNewTableHandle = smiGetTable( aNewTableOID );
    sTableType = aTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;

    for (j = 0; j < aTableInfo->indexCount; j++)
    {
        sExcludeIndex = ID_FALSE;

        if (aIndices[j].indexId != 0)
        {
            // PROJ-1624 non-partitioned index
            // primary key index의 경우 non-partitioned index와 partitioned index
            // 둘 다 생성한다.
            if ( aTableInfo->primaryKey != NULL )
            {
                if ( aTableInfo->primaryKey->indexId == aIndices[j].indexId )
                {
                    sIsPrimary = ID_TRUE;
                }
                else
                {
                    sIsPrimary = ID_FALSE;
                }
            }
            else
            {
                // Nothing to do.
            }
            
            sOffset = 0;
            for (i = 0; i < aIndices[j].keyColCount; i++)
            {
                if ( findColumnIDInColumnList( aDelColList,
                                               aIndices[j].keyColumns[i].column.id )
                     == ID_TRUE )
                {
                    sExcludeIndex = ID_TRUE;

                    // To Fix PR-11821,
                    // 인덱스에 포함된 컬럼이라면
                    // 이  index 생성할 필요 없고, 더이상 이 index
                    // 에 관한 처리를 할 필요가 없다.
                    break;
                }

                if ( aDelColList == NULL )
                {
                    // aDelColList가 없는 경우 index column ID는 바뀌지 않는다.
                    sIndexColumnID = aIndices[j].keyColumns[i].column.id;
                }
                else
                {
                    // aDelColList를 고려하여 새로운 index column ID를 계산한다.
                    sIndexColumnID = getNewColumnIDForAlter( aDelColList,
                                                             aIndices[j].keyColumns[i].column.id );
                }

                IDE_TEST( smiGetTableColumns( sNewTableHandle,
                                              (sIndexColumnID % QC_MAX_COLUMN_COUNT),
                                              (const smiColumn**)&sTempColumn )
                          != IDE_SUCCESS );

                idlOS::memcpy(&(sNewTableIndexColumnAtRow[i]),
                              sTempColumn,
                              ID_SIZEOF(mtcColumn) );

                sNewTableIndexColumnAtRow[i].column.flag =
                    aIndices[j].keyColsFlag[i];
                sColumnListAtRow[i].column = (smiColumn*)
                    &sNewTableIndexColumnAtRow[i];

                idlOS::memcpy(&(sNewTableIndexColumnAtKey[i]),
                              &(sNewTableIndexColumnAtRow[i]),
                              ID_SIZEOF(mtcColumn));

                // tableType == DISK 이면 offset 재조정
                if ( sTableType == SMI_TABLE_DISK )
                {
                    // PROJ-1705
                    IDE_TEST(
                        qdbCommon::setIndexKeyColumnTypeFlag( &(sNewTableIndexColumnAtKey[i]) )
                        != IDE_SUCCESS );

                    // To Fix PR-8111
                    if ( ( aIndices[j].keyColumns[i].column.flag &
                           SMI_COLUMN_TYPE_MASK )
                         == SMI_COLUMN_TYPE_FIXED )
                    {
                        sOffset = idlOS::align( sOffset,
                                                aIndices[j].keyColumns[i].module->align );
                        sNewTableIndexColumnAtKey[i].column.offset = sOffset;
                        sNewTableIndexColumnAtKey[i].column.value = NULL;
                        sOffset += sNewTableIndexColumnAtRow[i].column.size;
                    }
                    else
                    {
                        sOffset = idlOS::align( sOffset, 8 );
                        sNewTableIndexColumnAtKey[i].column.offset = sOffset;
                        sNewTableIndexColumnAtKey[i].column.value = NULL;
                        sOffset += smiGetVariableColumnSize4DiskIndex();
                    }
                }
                sColumnListAtKey[i].column =
                    (smiColumn*) &sNewTableIndexColumnAtKey[i];

                if (i == aIndices[j].keyColCount - 1)
                {
                    sColumnListAtRow[i].next = NULL;
                    sColumnListAtKey[i].next = NULL;
                }
                else
                {
                    sColumnListAtRow[i].next = &sColumnListAtRow[i+1];
                    sColumnListAtKey[i].next = &sColumnListAtKey[i+1];
                }
            }

            if (sExcludeIndex == ID_FALSE)
            {
                sFlag = smiTable::getIndexInfo(aIndices[j].indexHandle);

                IDE_TEST(smiTable::createIndex(
                             aStatement->mStatistics,
                             QC_SMI_STMT( aStatement ),
                             aIndices[j].TBSID,
                             sNewTableHandle,
                             (SChar*)aIndices[j].name,
                             aIndices[j].indexId,
                             aIndices[j].indexTypeId,
                             sColumnListAtKey,
                             sFlag,
                             QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                             aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                             smiTable::getIndexSegAttr(aIndices[j].indexHandle),
                             smiTable::getIndexSegStoAttr(aIndices[j].indexHandle),
                             smiTable::getIndexMaxKeySize( aIndices[j].indexHandle ), /* PROJ-2433 */
                             &sIndexHandle)
                         != IDE_SUCCESS);

                // BUG-44814 ddl 구문 수행시 통계정보 복사를 해야함
                smiStatistics::copyIndexStats( sIndexHandle, aIndices[j].indexHandle );

                // PROJ-1502 PARTITIONED DISK TABLE
                if( aTableInfo->tablePartitionType !=
                    QCM_NONE_PARTITIONED_TABLE )
                {
                    sIsPartitionedTable = ID_TRUE;
                }
                else
                {
                    sIsPartitionedTable = ID_FALSE;
                }

                if( sIsPartitionedTable == ID_TRUE )
                {
                    /* PROJ-2334 PMT */
                    if ( smiTableSpace::isDiskTableSpaceType( aTableInfo->TBSType ) == ID_TRUE )
                    {
                        // PROJ-1624 global non-partitioned index
                        if ( aIndices[j].indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
                        {
                            //--------------------------------
                            // (global) non-partitioned index
                            //--------------------------------

                            // non-partitioned index에 해당하는 index table을 찾는다.
                            IDE_TEST( qdx::findIndexTableInList( aOldIndexTables,
                                                                 aIndices[j].indexTableID,
                                                                 & sOldIndexTable )
                                      != IDE_SUCCESS );
                        
                            sIndexTableFlag = sOldIndexTable->tableInfo->tableFlag;
                            sIndexTableParallelDegree = sOldIndexTable->tableInfo->parallelDegree;

                            sIndexTableNamePos.stmtText = sOldIndexTable->tableInfo->name;
                            sIndexTableNamePos.offset   = 0;
                            sIndexTableNamePos.size     =
                                idlOS::strlen(sOldIndexTable->tableInfo->name);

                            /* BUG-45503 Table 생성 이후에 실패 시, Table Meta Cache의 Column 정보를 복구하지 않는 경우가 있습니다. */
                            IDE_TEST( qcm::copyQcmColumns( QC_QMX_MEM( aStatement ),
                                                           sOldIndexTable->tableInfo->columns,
                                                           & sTempColumns,
                                                           sOldIndexTable->tableInfo->columnCount )
                                      != IDE_SUCCESS );

                            IDE_TEST( qdx::createIndexTable( aStatement,
                                                             sOldIndexTable->tableInfo->tableOwnerID,
                                                             sIndexTableNamePos,
                                                             sTempColumns,
                                                             sOldIndexTable->tableInfo->columnCount,
                                                             sOldIndexTable->tableInfo->TBSID,
                                                             sOldIndexTable->tableInfo->segAttr,
                                                             sOldIndexTable->tableInfo->segStoAttr,
                                                             QDB_TABLE_ATTR_MASK_ALL,
                                                             sIndexTableFlag, /* Flag Value */
                                                             sIndexTableParallelDegree,
                                                             & sNewIndexTable )
                                      != IDE_SUCCESS );

                            // link new index table
                            sNewIndexTable->next = *aNewIndexTables;
                            *aNewIndexTables = sNewIndexTable;

                            // key index, rid index를 찾는다.
                            IDE_TEST( qdx::getIndexTableIndices( sOldIndexTable->tableInfo,
                                                                 sIndexTableIndex )
                                      != IDE_SUCCESS );
                        
                            sFlag = smiTable::getIndexInfo(sIndexTableIndex[0]->indexHandle);
                            sSegAttr = smiTable::getIndexSegAttr(sIndexTableIndex[0]->indexHandle);
                            sSegStoAttr = smiTable::getIndexSegStoAttr(sIndexTableIndex[0]->indexHandle);
                        
                            IDE_TEST( qdx::createIndexTableIndices(
                                          aStatement,
                                          sOldIndexTable->tableInfo->tableOwnerID,
                                          sNewIndexTable,
                                          NULL,
                                          sIndexTableIndex[0]->name,
                                          sIndexTableIndex[1]->name,
                                          sIndexTableIndex[0]->TBSID,
                                          sIndexTableIndex[0]->indexTypeId,
                                          sFlag,
                                          QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                          aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                          sSegAttr,
                                          sSegStoAttr,
                                          0 ) /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                                      != IDE_SUCCESS );
                        
                            // tableInfo 재생성
                            sIndexTableInfo = sNewIndexTable->tableInfo;
                    
                            IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT(aStatement),
                                                                 sNewIndexTable->tableID,
                                                                 sNewIndexTable->tableOID)
                                     != IDE_SUCCESS);
            
                            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                                           sNewIndexTable->tableID,
                                                           &(sNewIndexTable->tableInfo),
                                                           &(sNewIndexTable->tableSCN),
                                                           &(sNewIndexTable->tableHandle))
                                     != IDE_SUCCESS);
            
                            (void)qcm::destroyQcmTableInfo(sIndexTableInfo);
                    
                            // index table id 설정
                            aIndices[j].indexTableID = sNewIndexTable->tableID;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        /* Nothing To Do */
                    }
                        
                    // primary key index의 경우 non-partitioned index와 partitioned index
                    // 둘 다 생성한다.
                    if ( ( aIndices[j].indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) ||
                         ( sIsPrimary == ID_TRUE ) )
                    {
                        //--------------------------------
                        // partitioned index
                        //--------------------------------

                        for( i = 0; i < aPartitionCount; i++ )
                        {
                            // BUG-27034 CodeSonar
                            // Partitioned Table에는 Partitioned Index만 올 수 있다.
                            IDE_ASSERT( aPartIndices != NULL );

                            // partitioned index에 해당하는 local partition index를 찾는다.
                            IDE_TEST( qdx::findIndexIDInIndices( aPartIndices[i],
                                                                 aPartIndexCount,
                                                                 aIndices[j].indexId,
                                                                 & sIndexPartition )
                                      != IDE_SUCCESS );

                            /* PROJ-2464 hybrid partitioned table 지원
                             *  - Column 또는 Index 중 하나만 전달해야 한다.
                             */
                            IDE_TEST( adjustIndexColumn( NULL,
                                                         sIndexPartition,
                                                         aDelColList,
                                                         sColumnListAtKey )
                                      != IDE_SUCCESS );

                            /* PROJ-2464 hybrid partitioned table 지원 */
                            sFlag             = smiTable::getIndexInfo( sIndexPartition->indexHandle );
                            sSegAttr          = smiTable::getIndexSegAttr( sIndexPartition->indexHandle );
                            sSegStoAttr       = smiTable::getIndexSegStoAttr( sIndexPartition->indexHandle );

                            IDE_TEST(smiTable::createIndex(
                                         aStatement->mStatistics,
                                         QC_SMI_STMT( aStatement ),
                                         sIndexPartition->TBSID, // index partition
                                         smiGetTable( aNewPartitionOID[i] ),
                                         (SChar*)sIndexPartition->name,
                                         aIndices[j].indexId,
                                         aIndices[j].indexTypeId,
                                         sColumnListAtKey,
                                         sFlag,
                                         QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                         aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                         sSegAttr,
                                         sSegStoAttr,
                                         0, /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                                         &sIndexHandle)
                                     != IDE_SUCCESS);

                            // BUG-44814 ddl 구문 수행시 통계정보 복사를 해야함
                            smiStatistics::copyIndexStats( sIndexHandle, sIndexPartition->indexHandle );
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if (aCreateMetaFlag == ID_TRUE)
                {
                    if ( ( sIsPartitionedTable == ID_TRUE ) &&
                         ( aIndices[j].indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) )
                    {
                        sIsPartitionedIndex = ID_TRUE;
                    }
                    else
                    {
                        sIsPartitionedIndex = ID_FALSE;
                    }
                    
                    IDE_TEST(qdx::insertIndexIntoMeta(
                                 aStatement,
                                 aIndices[j].TBSID,
                                 aIndices[j].userID,
                                 aTableInfo->tableID,
                                 aIndices[j].indexId,
                                 aIndices[j].name,
                                 aIndices[j].indexTypeId,
                                 aIndices[j].isUnique,
                                 aIndices[j].keyColCount,
                                 aIndices[j].isRange,
                                 sIsPartitionedIndex,
                                 0,
                                 sFlag) != IDE_SUCCESS);

                    for ( i = 0; i < aIndices[j].keyColCount; i++)
                    {
                        if ( (aIndices[j].keyColsFlag[i] & SMI_COLUMN_ORDER_MASK)
                             == SMI_COLUMN_ORDER_ASCENDING)
                        {
                            sColOrderFlag = ID_TRUE;
                        }
                        else
                        {
                            sColOrderFlag = ID_FALSE;
                        }

                        IDE_TEST(qdx::insertIndexColumnIntoMeta(
                                     aStatement,
                                     aIndices[j].userID,
                                     aIndices[j].indexId,
                                     sColumnListAtKey[i].column->id,  // BUG-16543
                                     i,
                                     sColOrderFlag,
                                     aTableInfo->tableID) != IDE_SUCCESS);
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::makeColumnNullable(
    qcStatement  *aStatement,
    qcmTableInfo *aTableInfo,
    UInt          aColID)
{
/***********************************************************************
 *
 * Description :
 *    TableInfo 에서 컬럼의 flag 을 Nullable 로 변경
 *
 * Implementation :
 *    1. column ID 로부터 mtcColumn 을 구한다
 *    2. 1 에서 구한 컬럼의 flag 을 Nullable 로 설정
 *    2. smiTable::modifyTableInfo 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::makeColumnNullable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcColumn            * sColumn;
    UInt                   sColOrder;
    smiColumnList        * sColumnList;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmTableInfo         * sPartInfo;
    qdTableParseTree     * sParseTree;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST(allocSmiColList(aStatement,
                             aTableInfo->tableHandle,
                             &sColumnList)
             != IDE_SUCCESS);

    sColOrder = aColID & SMI_COLUMN_ID_MASK;
    sColumn = (mtcColumn*)(sColumnList[sColOrder].column);

    sColumn->flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sColumn->flag |= MTC_COLUMN_NOTNULL_FALSE;

    IDE_TEST(smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                        aTableInfo->tableHandle,
                                        sColumnList,
                                        ID_SIZEOF(mtcColumn),
                                        NULL,
                                        0,
                                        SMI_TABLE_FLAG_UNCHANGE,
                                        SMI_TBSLV_DDL_DML,
                                        aTableInfo->maxrows,
                                        0,
                                        ID_TRUE ) /* aIsInitRowTemplate */
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        // 파스트리에서 파티션 정보 리스트를 가져온다.
        sPartInfoList = sParseTree->partTable->partInfoList;

        for( ; sPartInfoList != NULL;
             sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            IDE_TEST(allocSmiColList(aStatement,
                                     sPartInfo->tableHandle,
                                     &sColumnList)
                     != IDE_SUCCESS);

            sColOrder = aColID & SMI_COLUMN_ID_MASK;
            sColumn = (mtcColumn*)(sColumnList[sColOrder].column);

            // fix BUG-17824
            sColumn->flag &= ~MTC_COLUMN_NOTNULL_MASK;
            sColumn->flag |= MTC_COLUMN_NOTNULL_FALSE;

            IDE_TEST(smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                                sPartInfo->tableHandle,
                                                sColumnList,
                                                ID_SIZEOF(mtcColumn),
                                                NULL,
                                                0,
                                                SMI_TABLE_FLAG_UNCHANGE,
                                                SMI_TBSLV_DDL_DML,
                                                aTableInfo->maxrows,
                                                0,
                                                ID_TRUE ) /* aIsInitRowTemplate */
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::makeColumnNotNull(
    qcStatement          * aStatement,
    const void           * aTableHandle,
    ULong                  aMaxRows,
    qcmPartitionInfoList * aPartInfoList,
    idBool                 aIsPartitioned,
    UInt                   aColID )
{
/***********************************************************************
 *
 * Description :
 *    TableInfo 에서 컬럼의 flag 을 NotNull 로 변경
 *
 * Implementation :
 *    1. column ID 로부터 mtcColumn 을 구한다
 *    2. 1 에서 구한 컬럼의 flag 을 NotNull 로 설정
 *    2. smiTable::modifyTableInfo 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::makeColumnNotNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcColumn            * sColumn;
    UInt                   sColOrder;
    smiColumnList        * sColumnList;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmTableInfo         * sPartInfo;

    IDE_TEST(allocSmiColList(aStatement,
                             aTableHandle,
                             &sColumnList)
             != IDE_SUCCESS);

    sColOrder = aColID & SMI_COLUMN_ID_MASK;
    sColumn = (mtcColumn*)(sColumnList[sColOrder].column);

    sColumn->flag &= ~MTC_COLUMN_NOTNULL_MASK;
    sColumn->flag |= MTC_COLUMN_NOTNULL_TRUE;

    IDE_TEST(smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                        aTableHandle,
                                        sColumnList,
                                        ID_SIZEOF(mtcColumn),
                                        NULL,
                                        0,
                                        SMI_TABLE_FLAG_UNCHANGE,
                                        SMI_TBSLV_DDL_DML,
                                        aMaxRows,
                                        0,
                                        ID_TRUE ) /* aIsInitRowTemplate */
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( aIsPartitioned == ID_TRUE )
    {
        for( sPartInfoList = aPartInfoList;
             sPartInfoList != NULL;
             sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            IDE_TEST(allocSmiColList(aStatement,
                                     sPartInfo->tableHandle,
                                     &sColumnList)
                     != IDE_SUCCESS);

            sColOrder = aColID & SMI_COLUMN_ID_MASK;
            sColumn = (mtcColumn*)(sColumnList[sColOrder].column);

            sColumn->flag &= ~MTC_COLUMN_NOTNULL_MASK;
            sColumn->flag |= MTC_COLUMN_NOTNULL_TRUE;

            IDE_TEST(smiTable::modifyTableInfo(
                         QC_SMI_STMT( aStatement ),
                         sPartInfo->tableHandle,
                         sColumnList,
                         ID_SIZEOF(mtcColumn),
                         NULL,
                         0,
                         SMI_TABLE_FLAG_UNCHANGE,
                         SMI_TBSLV_DDL_DML,
                         aMaxRows,
                         0,
                         ID_TRUE ) /* aIsInitRowTemplate */
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::makeColumnSRID( qcStatement          * aStatement,
                                  const void           * aTableHandle,
                                  ULong                  aMaxRows,
                                  qcmPartitionInfoList * aPartInfoList,
                                  idBool                 aIsPartitioned,
                                  UInt                   aColID,
                                  SInt                   aSRID )
{
/***********************************************************************
 *
 * Description : PROJ-2422 SRID
 *    TableInfo 에서 컬럼의 SRID를 변경
 *
 * Implementation :
 *    1. column ID 로부터 mtcColumn 을 구한다
 *    2. 1 에서 구한 컬럼의 srid를 aSRID로 변경
 *    2. smiTable::modifyTableInfo 호출
 *
 ***********************************************************************/

    mtcColumn            * sColumn;
    UInt                   sColOrder;
    smiColumnList        * sColumnList;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmTableInfo         * sPartInfo;

    IDE_TEST( allocSmiColList( aStatement,
                               aTableHandle,
                               &sColumnList )
              != IDE_SUCCESS );

    sColOrder = aColID & SMI_COLUMN_ID_MASK;
    sColumn = (mtcColumn*)(sColumnList[sColOrder].column);

    // PROJ-2422 srid
    sColumn->mColumnAttr.mSridAttr.mSrid = aSRID;

    IDE_TEST( smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                         aTableHandle,
                                         sColumnList,
                                         ID_SIZEOF( mtcColumn ),
                                         NULL,
                                         0,
                                         SMI_TABLE_FLAG_UNCHANGE,
                                         SMI_TBSLV_DDL_DML,
                                         aMaxRows,
                                         0,
                                         ID_TRUE ) /* aIsInitRowTemplate */
              != IDE_SUCCESS );

    // PROJ-1502 PARTITIONED DISK TABLE
    if ( aIsPartitioned == ID_TRUE )
    {
        for ( sPartInfoList = aPartInfoList;
              sPartInfoList != NULL;
              sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            IDE_TEST( allocSmiColList( aStatement,
                                       sPartInfo->tableHandle,
                                       &sColumnList )
                      != IDE_SUCCESS );

            sColOrder = aColID & SMI_COLUMN_ID_MASK;
            sColumn = (mtcColumn*)(sColumnList[sColOrder].column);

            // PROJ-2422 srid
            sColumn->mColumnAttr.mSridAttr.mSrid = aSRID;

            IDE_TEST( smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                                 sPartInfo->tableHandle,
                                                 sColumnList,
                                                 ID_SIZEOF( mtcColumn ),
                                                 NULL,
                                                 0,
                                                 SMI_TABLE_FLAG_UNCHANGE,
                                                 SMI_TBSLV_DDL_DML,
                                                 aMaxRows,
                                                 0,
                                                 ID_TRUE ) /* aIsInitRowTemplate */
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeColumnNewType(
    qcStatement  * aStatement,
    qcmTableInfo * aTableInfo,
    qcmColumn    * aColumn )
{
/***********************************************************************
 *
 * Description :
 *    TableInfo 에서 컬럼의 정보를 aColumn의 정보로 변경
 *
 * Implementation :
 *    1. smiColumnList를 생성한다.
 *    2. smiTable::modifyTableInfo 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::makeColumnNewType"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree     * sParseTree;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmTableInfo         * sPartInfo;
    mtcColumn            * sColumn;
    mtcColumn            * sMtcColumn;
    qcmColumn            * sQcmColumn;
    smiColumnList        * sColumnList;
    smiColumn            * sSrcSmiColumn;
    smiColumn            * sDstSmiColumn;
    UInt                   i;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDU_LIMITPOINT("qdbCommon::makeColumnNewType::malloc1");
    IDE_TEST( aStatement->qmxMem->alloc(
                  ID_SIZEOF(smiColumnList) * aTableInfo->columnCount,
                  (void**) & sColumnList )
              != IDE_SUCCESS );

    sQcmColumn = aColumn;

    for ( i = 0; i < aTableInfo->columnCount; i++ )
    {
        IDE_DASSERT( sQcmColumn != NULL );
        sMtcColumn = sQcmColumn->basicInfo;

        IDU_LIMITPOINT("qdbCommon::makeColumnNewType::malloc2");
        IDE_TEST( aStatement->qmxMem->alloc(
                      ID_SIZEOF(mtcColumn),
                      (void**) & sColumn )
                  != IDE_SUCCESS );

        /*
           BUG-30711
           modify column을 수행하면, 모든 칼럼 정보가 새로운 column 정보로 변경된다.
           하지만 column이 저장될 tablespace id, GRID, segment 정보는 변경되지 않는다.
          ( 참조, tablespace id, GRID, segment 정보는 Lob column에서만 의미있음 )
        */

        idlOS::memcpy( sColumn,
                       sMtcColumn,
                       ID_SIZEOF(mtcColumn) );

        if ( ( ( aTableInfo->columns[i].basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
               == MTD_COLUMN_TYPE_LOB )
             &&
             ( ( sMtcColumn->module->flag & MTD_COLUMN_TYPE_MASK )
               == MTD_COLUMN_TYPE_LOB ) )
        {
            /* LOB column type에서 LOB column type으로 MODIFY 할 경우,
               이전 column의 tablespace id, GRID, segment 정보를 RESTORE */
            sDstSmiColumn = (smiColumn*)sColumn;
            sSrcSmiColumn = (smiColumn*)(aTableInfo->columns[i].basicInfo);
            SMI_COLUMN_LOB_INFO_COPY( sDstSmiColumn, sSrcSmiColumn );
        }
        else
        {
            /* 아래 MODIFY 의 경우
               - LOB column -> LOB 아닌 column
                 : 이전 LOB column의 tablespace id, GRID, segment 정보 없애야 함
               - Lob 아닌 column -> LOB column :
                 : 이전 column이 LOB이 아니었으므로 tablespace id, GRID, segment 정보 없음
             */
        }

        sColumnList[i].column = (const smiColumn*) sColumn;

        if ( i == aTableInfo->columnCount - 1 )
        {
            sColumnList[i].next = NULL;
        }
        else
        {
            sColumnList[i].next = & sColumnList[i+1];
        }

        sQcmColumn = sQcmColumn->next;
    }

    /* BUG-31949 [qp-ddl-dcl-execute] The abort of Alter-table operation makes
     * index-header wrong since the server doesn't recover index runtime header
     * in transaction undo.
     * SM은 아래 modifyTableInfo함수를 통해 QP가 Column 정보를 변경하려 한다는
     * 것을 확인하고, 자동으로 DRDB Index의 Column 정보도 수정해줍니다. 따라서
     * QP는 테이블의 Column 정보만 변경해주면 됩니다. */

    IDE_TEST(smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                        aTableInfo->tableHandle,
                                        sColumnList,
                                        ID_SIZEOF(mtcColumn),
                                        NULL,
                                        0,
                                        SMI_TABLE_FLAG_UNCHANGE,
                                        SMI_TBSLV_DDL_DML,
                                        aTableInfo->maxrows,
                                        0,
                                        ID_FALSE ) /* aIsInitRowTemplate */
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        // 파스트리에서 파티션 정보 리스트를 가져온다.
        for( sPartInfoList = sParseTree->partTable->partInfoList;
             sPartInfoList != NULL;
             sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            sQcmColumn = aColumn;

            for ( i = 0; i < sPartInfo->columnCount; i++ )
            {
                IDE_DASSERT( sQcmColumn != NULL );
                sMtcColumn = sQcmColumn->basicInfo;

                sColumn = (mtcColumn*) sColumnList[i].column;

                /*
                  BUG-30711
                  modify column을 수행하면, 모든 칼럼 정보가 새로운 column 정보로 변경된다.
                  하지만 column이 저장될 tablespace id, GRID, segment 정보는 변경되지 않는다.
                  ( 참조, tablespace id, GRID, segment 정보는 Lob column에서만 의미있음 )
                */
                idlOS::memcpy( sColumn,
                               sMtcColumn,
                               ID_SIZEOF(mtcColumn) );

                if ( ( ( sPartInfo->columns[i].basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
                       == MTD_COLUMN_TYPE_LOB )
                     &&
                     ( ( sMtcColumn->module->flag & MTD_COLUMN_TYPE_MASK )
                       == MTD_COLUMN_TYPE_LOB ) )
                {
                    /* LOB column type에서 LOB column type으로 MODIFY 할 경우,
                       이전 column의 tablespace id, GRID, segment 정보를 RESTORE */
                    sDstSmiColumn = (smiColumn*)sColumn;
                    sSrcSmiColumn = (smiColumn*)(sPartInfo->columns[i].basicInfo);
                    SMI_COLUMN_LOB_INFO_COPY( sDstSmiColumn, sSrcSmiColumn );
                }
                else
                {
                    /* 아래 MODIFY 의 경우
                       - LOB column -> LOB 아닌 column
                       : 이전 LOB column의 tablespace id, GRID, segment 정보 없애야 함
                       - Lob 아닌 column -> LOB column : LOB
                       : 이전 column이 LOB이 아니었으므로 tablespace id,GRID,segment 정보 없음
                    */
                }

                sQcmColumn = sQcmColumn->next;
            }

            IDE_TEST(smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                                sPartInfo->tableHandle,
                                                sColumnList,
                                                ID_SIZEOF(mtcColumn),
                                                NULL,
                                                0,
                                                SMI_TABLE_FLAG_UNCHANGE,
                                                SMI_TBSLV_DDL_DML,
                                                aTableInfo->maxrows,
                                                0,
                                                ID_FALSE ) /* aIsInitRowTemplate */
                     != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::decideColumnTypeFixedOrVariable( qcmColumn* aColumn,
                                                   UInt       aMemoryOrDisk,
                                                   scSpaceID  aTBSID,
                                                   idBool     aIsAddColumn )
{
/***********************************************************************
 *
 * Description :
 *      smiColumn의 SMI_COLUMN_TYPE을 정한다.
 *
 * Implementation :
 *      parsing 단계에서는 mtcColumn의 flag가
 *      QCM_COLUMN_TYPE_FIXED 또는
 *      QCM_COLUMN_TYPE_VARIABLE 또는
 *      QCM_COLUMN_TYPE_DEFAULT 로 정해지는데
 *      이 값을 참조하여 smiColumn의 flag에 SMI_COLUMN_TYPE을 정한다.
 *
 ***********************************************************************/

    UInt             sInRowLength;


    aColumn->basicInfo->column.colSpace = aTBSID;

    if ( ( aColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
         == MTD_COLUMN_TYPE_LOB )
    {
        // PROJ-1362
        aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;

        aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_LOB;
        aColumn->basicInfo->column.size = ID_UINT_MAX;

        /* PROJ-2047 Strengthening LOB */
        if ( aColumn->basicInfo->module->id == MTD_BLOB_ID )
        {
            aColumn->basicInfo->column.colType = SMI_LOB_COLUMN_TYPE_BLOB;
        }
        else
        {
            aColumn->basicInfo->column.colType = SMI_LOB_COLUMN_TYPE_CLOB;
        }
    }
    else
    {
        // PROJ-1705
        if ( aMemoryOrDisk == SMI_COLUMN_STORAGE_DISK )
        {
            //------------------------------------------------------
            // 디스크 컬럼은 fixed/variable 컬럼속성이 없음.
            // 기본값을 fixed로 ...
            //------------------------------------------------------
            aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
            aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;

            //----------------------------------------------
            // PROJ-1705
            // SM이 컬럼을 여러 row piece에
            // 나누어 저장해도 되는지의 정보를 smiColumn.flag에 세팅한다.
            //-----------------------------------------------

            if ( ( aColumn->basicInfo->module->flag & MTD_DATA_STORE_DIVISIBLE_MASK )
                 == MTD_DATA_STORE_DIVISIBLE_TRUE )
            {
                aColumn->basicInfo->column.flag &= ~SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK;
                aColumn->basicInfo->column.flag |= SMI_COLUMN_DATA_STORE_DIVISIBLE_TRUE;
            }
            else
            {
                aColumn->basicInfo->column.flag &= ~SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK;
                aColumn->basicInfo->column.flag |= SMI_COLUMN_DATA_STORE_DIVISIBLE_FALSE;
            }
        }
        else
        {
            //------------------------------------------------------
            // 메모리 컬럼에 대한 fixed/variable 컬럼속성지정.
            //------------------------------------------------------

            switch( aColumn->flag & QCM_COLUMN_TYPE_MASK )
            {
                case QCM_COLUMN_TYPE_FIXED:
                    aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                    aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;
                    break;
                case QCM_COLUMN_TYPE_VARIABLE:
                    /* Geometry의 경우 variable 이 붙더라도
                     * VARIABLE_LARGE 로 동작하도록 한다. */
                    if ( aColumn->basicInfo->module->id == MTD_GEOMETRY_ID )
                    {
                        aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                        aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE_LARGE;
                    }
                    else
                    {
                        aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                        aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE;
                    }
                    break;
                case QCM_COLUMN_TYPE_VARIABLE_LARGE:
                    aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                    aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE_LARGE;
                    break;
                case QCM_COLUMN_TYPE_DEFAULT:
                    if ( aIsAddColumn != ID_TRUE )
                    {
                        if ( ( aColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
                             == MTD_COLUMN_TYPE_VARIABLE )
                        {
                            if ( aColumn->basicInfo->column.size
                                 <= QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE )
                            {
                                aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                                aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;
                            }
                            else
                            {
                                /* Geometry는 항상 VARIABLE_LARGE를 사용 한다. */
                                if ( aColumn->basicInfo->module->id == MTD_GEOMETRY_ID )
                                {
                                    aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                                    aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE_LARGE;
                                }
                                else
                                {
                                    aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                                    aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE;
                                }
                            }
                        }
                        else
                        {
                            aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                            aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;
                        }
                    }
                    else
                    {
                        /* Geometry는 항상 VARIABLE_LARGE를 사용 한다.
                         * 실시간 Add Column 대상이 아니다. */
                        /* BUG-43148 ADD COLUMN 시, MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE 프라퍼티 보다
                         *           작은 크기의 Column도 Variable로 유지하여, 실시간 DDL을 지원한다.
                         */
                        if ( aColumn->basicInfo->module->id == MTD_GEOMETRY_ID )
                        {
                            aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                            aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE_LARGE;
                        }
                        else
                        {
                            aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                            aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE;
                        }
                    }
                    break;
                default:
                    IDE_DASSERT( 0 );
                    break;
            }
        }
    }

    //---------------------------------------------------------------
    // in row size 정보
    //---------------------------------------------------------------

    // in row size를 명시하지 않았다면 property에서 값을 읽어옴.
    if ( aColumn->inRowLength != ID_UINT_MAX )
    {
        sInRowLength = aColumn->inRowLength;
    }
    else
    {
        // PROJ-1362
        if ( ( aColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_LOB )
        {
            if ( aMemoryOrDisk == SMI_COLUMN_STORAGE_MEMORY )
            {
                sInRowLength = QCU_MEMORY_LOB_COLUMN_IN_ROW_SIZE;
            }
            else
            {
                // PROJ-1862 Disk In Mode Lob
                sInRowLength = QCU_DISK_LOB_COLUMN_IN_ROW_SIZE;
            }
        }
        else if ( ( aColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                  == SMI_COLUMN_TYPE_VARIABLE_LARGE )
        {
            if ( ( aColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
                 == MTD_COLUMN_TYPE_VARIABLE )
            {
                sInRowLength = QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE;
            }
            else
            {
                /* char(10) VARIABLE_LARGE 와 같이 Fixed Data를 variable 영역에 저장할경우
                 * 그 크기는 NULL(2) 또는 Data의 길이(12) 2가지 경우 밖에 없게된다.
                 * in row size가 10 일 경우
                 * 실제 In row로 기록되는 경우는 NULL(2) 밖에 존재하지 않으나
                 * in row size에 의해 10 byte를 예비해두기 때문에
                 * 항상 8 byte의 손실이 발생하게된다.
                 * fixed data가 variable 영역에 저장될 경우
                 * 저장되는 크기가 in row size 보다 크다면
                 * NULL Data의 경우에만 in row로 저장될수 있도록 조정한다. */
                if ( aColumn->basicInfo->column.size <= QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE )
                {
                    sInRowLength = aColumn->basicInfo->column.size;
                }
                else
                {
                    sInRowLength = aColumn->basicInfo->module->nullValueSize();
                }
            }
        }
        else if ( ( aColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                 == SMI_COLUMN_TYPE_VARIABLE )
        {
            /* UnitedVar의 경우
             * In row Size는 Fixed 판별에만 사용된다. */
            sInRowLength = QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE;
        }
        else /* fixed */
        {
            sInRowLength = 0;
        }
    }

    // PROJ-1557
    // in_row_size를 너무 크게 준 경우 에러.
    IDE_TEST_RAISE( sInRowLength > QDB_COLUMN_MAX_IN_ROW_SIZE,
                    err_in_row_size );

    // PROJ-1557
    // variable column in row length를 세팅한다.
    if ( ( aColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
         == SMI_COLUMN_TYPE_FIXED )
    {
        aColumn->basicInfo->column.vcInOutBaseSize = 0;
    }
    else if ( ( aColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
              == SMI_COLUMN_TYPE_LOB )
    {
        aColumn->basicInfo->column.vcInOutBaseSize = sInRowLength;
    }
    else
    {
        if ( aMemoryOrDisk == SMI_COLUMN_STORAGE_MEMORY )
        {
            // BUG-25935
            // 유저가 variable, VARIABLE_LARGE 을 사용하더라도
            // MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE 보다 작으면 fixed 이다.
            if ( aColumn->basicInfo->column.size <= sInRowLength )
            {
                /* BUG-43148 ADD COLUMN 시, MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE 프라퍼티 보다
                 *     작은 크기의 Column도 Variable로 유지하여, 실시간 DDL을 지원한다.
                 */
                if ( aIsAddColumn != ID_TRUE )
                {
                    aColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                    aColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;
                    aColumn->basicInfo->column.vcInOutBaseSize = 0;
                }
                else
                {
                    aColumn->basicInfo->column.vcInOutBaseSize = aColumn->basicInfo->column.size;
                }
            }
            else
            {
                aColumn->basicInfo->column.vcInOutBaseSize = sInRowLength;
            }
        }
        else
        {
            // LOB을 제외한 Disk column의 경우 무조건 0
            aColumn->basicInfo->column.vcInOutBaseSize = 0;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_in_row_size);
    {
        // PROJ-1557
        IDE_SET(ideSetErrorCode (qpERR_ABORT_QDB_IN_ROW_SIZE_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::getColumnTypeInMemory( mtcColumn * aColumn,
                                         UInt      * aColumnTypeFlag,
                                         UInt      * aVcInOutBaseSize )
{
/***********************************************************************
 *
 * Description :
 *
 *   디스크컬럼이
 *   메모리컬럼으로 생성시 정의될 fixed/variable 컬럼속성을 얻는다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    UInt             sColumnType;
    UInt             sInRowLength;

    IDE_DASSERT( ( aColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                 == SMI_COLUMN_STORAGE_DISK );

    if( ( aColumn->module->flag & MTD_COLUMN_TYPE_MASK )
        == MTD_COLUMN_TYPE_LOB )
    {
        sColumnType = SMI_COLUMN_TYPE_LOB;
    }
    else
    {
        //------------------------------------------------------
        // 메모리 컬럼에 대한 fixed/variable 컬럼속성지정.
        //------------------------------------------------------

        if( ( ( aColumn->module->flag & MTD_COLUMN_TYPE_MASK )
              == MTD_COLUMN_TYPE_VARIABLE )  ||
            ( ( aColumn->module->flag & MTD_DATA_STORE_DIVISIBLE_MASK )
              == MTD_DATA_STORE_DIVISIBLE_TRUE ) )
        {
            /* Geometry는 항상 VARIABLE_LARGE을 사용한다. */
            if ( aColumn->module->id == MTD_GEOMETRY_ID )
            {
                sColumnType = SMI_COLUMN_TYPE_VARIABLE_LARGE;
            }
            else
            {
                sColumnType = SMI_COLUMN_TYPE_VARIABLE;
            }
        }
        else
        {
            sColumnType = SMI_COLUMN_TYPE_FIXED;
        }
    }

    //---------------------------------------------------------------
    // in row size 정보
    //---------------------------------------------------------------

    // PROJ-1362
    if( sColumnType == SMI_COLUMN_TYPE_LOB )
    {
        sInRowLength = QCU_MEMORY_LOB_COLUMN_IN_ROW_SIZE;
    }
    else if ( (sColumnType == SMI_COLUMN_TYPE_VARIABLE) ||
              (sColumnType == SMI_COLUMN_TYPE_VARIABLE_LARGE) )
    {
        sInRowLength = QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE;
    }
    else
    {
        sInRowLength = 0;
    }

    // 만약 컬럼 사이즈가 in row size보다 작거나 같다면 fixed가 되고
    // in row size는 0이 된다.
    if( aColumn->column.size <= sInRowLength )
    {
        sColumnType = SMI_COLUMN_TYPE_FIXED;
        sInRowLength = 0;
    }
    else
    {
        // Nothing to do.
    }

    *aColumnTypeFlag = sColumnType;
    *aVcInOutBaseSize = sInRowLength;

    return IDE_SUCCESS;
}

IDE_RC qdbCommon::validateColumnListForCreate(
    qcStatement     * aStatement,
    qcmColumn       * aColumn,
    idBool            aIsTable )
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE/VIEW 문의 validation 수행 함수로부터 호출
 *
 * Implementation :
 *    CRATE TABLE
 *    1. 각 컬럼의 size, align 부여
 *    2. default 값의 validation 검사 => validateDefaultDefinition 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::validateColumnListForCreateTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree  * sParseTree;
    UInt                sCurrentOffset;
    qcmColumn         * sColumn;
    idBool              sExistTimeStamp = ID_FALSE;
    SInt                sColumnCount = 0;
    qcuSqlSourceInfo    sqlInfo;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    if( smiTableSpace::isMemTableSpaceType( sParseTree->TBSAttr.mType )
        == ID_TRUE )
    {
        sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_MEMORY);
    }
    else if ( smiTableSpace::isVolatileTableSpaceType( sParseTree->TBSAttr.mType )
              == ID_TRUE )
    {
        sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_VOLATILE);
    }
    else
    {
        IDE_DASSERT( smiTableSpace::isDiskTableSpaceType( sParseTree->TBSAttr.mType )
                     == ID_TRUE );

        sCurrentOffset = 0;
    }

    for (sColumn = aColumn;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        // set SMI_COLUMN_STORAGE_MASK
        if((smiTableSpace::isMemTableSpaceType(sParseTree->TBSAttr.mType) == ID_TRUE) ||
           (smiTableSpace::isVolatileTableSpaceType(sParseTree->TBSAttr.mType) == ID_TRUE))
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_MEMORY;
        }
        else
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_DISK;
        }

        // BUG-17212 volatile table인 경우 lob 컬럼을 가질 수 없다.
        /* PROJ-2174 Supporting LOB in the volatile tablespace
         * volatile TBS에서 lob을 사용 할 수 있음. */
        if (smiTableSpace::isVolatileTableSpaceType(sParseTree->TBSAttr.mType) == ID_TRUE)
        {
            //fix BUG-18244
            IDE_TEST_RAISE( sColumn->basicInfo->module->id
                            == MTD_GEOMETRY_ID,
                            ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );

            /* Volatile TableSpace의 경우 VARIABLE_LARGE를 지원하지 않는다. */
            IDE_TEST_RAISE( ( sColumn->flag & QCM_COLUMN_TYPE_MASK ) 
                            == QCM_COLUMN_TYPE_VARIABLE_LARGE,
                            ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );

        }

        ///////////////////////////////////////////////////////////////////
        // Proj-1391
        //
        // column type이 fixed인지 variable인지 parsing 단계에서
        // 결정했던 것을 validation 과정에서 하도록 수정함.
        //
        // memory냐 disk이냐에 따라 fixed, variable 정책이 다르기 때문에
        // SMI_COLUMN_STORAGE_MASK로 & 연산한 값을 인자로 넘겨준다.
        // 이미 column.flag에 이 정보가 들어가 있지만,
        // 호출 순서가 반드시 SMI_COLUMN_STORAGE_MASK가 정해진 후에
        // 불려져야 하는 제약을 없애기 위해 파라미터로 받는다.
        //
        IDE_TEST( decideColumnTypeFixedOrVariable(
                      sColumn,
                      sColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK,
                      sParseTree->TBSAttr.mID,
                      ID_FALSE /* aIsAddColumn */ )
                  != IDE_SUCCESS );

        IDE_TEST( validateColumnLength( aStatement,
                                        sColumn )
                  != IDE_SUCCESS );

        if( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
            == MTC_COLUMN_TIMESTAMP_TRUE )
        {
            // check duplicate timestamp column
            if ( sExistTimeStamp == ID_TRUE )
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_DUP_TIMESTAMP_COLUMN);
            }
            else
            {
                sExistTimeStamp = ID_TRUE;
            }

            // check default clause
            if (sColumn->defaultValue != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_CANNOT_SET_TIMESTAMP_DEFAULT);
            }
        }

        IDE_TEST_RAISE( (sColumn->basicInfo->module->flag & MTD_CREATE_MASK)
                        == MTD_CREATE_DISABLE,
                        ERR_INVALID_DATATYPE );

        // validation of default value
        if (sColumn->defaultValue != NULL)
        {
            IDE_TEST(validateDefaultDefinition(
                         aStatement, sColumn->defaultValue)
                     != IDE_SUCCESS);

            // PROJ-1579 NCHAR
            // 각 컬럼에 nchar literal의 list를 만들어 놓는다.
            IDE_TEST( makeNcharLiteralStr( aStatement,
                                           sParseTree->ncharList,
                                           sColumn )
                      != IDE_SUCCESS );
        }

        // PROJ-2204 Join Update, Delete
        if ( aIsTable == ID_TRUE )
        {
            sColumn->basicInfo->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
            sColumn->basicInfo->flag |= MTC_COLUMN_KEY_PRESERVED_TRUE;
        }
        else
        {
            // view인 경우 select target에 의해 결정된다.
        }

        sColumnCount++;
    }

    // PROJ-1391 qp
    // create table은 column offset을 validation과정에서 setting한다.
    IDE_TEST( setColListOffset( QC_QMP_MEM(aStatement),
                                aColumn,
                                sCurrentOffset )
              != IDE_SUCCESS );

    IDE_TEST_RAISE(sColumnCount > QC_MAX_COLUMN_COUNT,
                   ERR_INVALID_COLUMN_COUNT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DATATYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE));
    }
    IDE_EXCEPTION(ERR_DUP_TIMESTAMP_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_TIMESTAMP_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_CANNOT_SET_TIMESTAMP_DEFAULT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_CANNOT_SET_TIMESTAMP_DEFAULT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_COLUMN_COUNT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_COLUMN_COUNT));
    }
    IDE_EXCEPTION(ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::validateColumnListForCreateInternalTable(
    qcStatement     * aStatement,
    idBool            aInExecutionTime,
    UInt              aTableType,
    scSpaceID         aTBSID,
    qcmColumn       * aColumn )
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE/VIEW 문의 validation 수행 함수로부터 호출
 *
 * Implementation :
 *    CRATE TABLE
 *    1. 각 컬럼의 size, align 부여
 *    2. default 값의 validation 검사 => validateDefaultDefinition 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::validateColumnListForCreateInternalTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                sCurrentOffset;
    qcmColumn         * sColumn;
    idBool              sExistTimeStamp = ID_FALSE;
    SInt                sColumnCount = 0;
    qcuSqlSourceInfo    sqlInfo;

    if ( ( aTableType == SMI_TABLE_MEMORY ) || ( aTableType == SMI_TABLE_VOLATILE ) )
    {
        sCurrentOffset = smiGetRowHeaderSize(aTableType);
    }
    else
    {
        IDE_DASSERT( aTableType == SMI_TABLE_DISK );
        
        sCurrentOffset = 0;
    }

    for ( sColumn = aColumn; sColumn != NULL; sColumn = sColumn->next)
    {
        if ( ( aTableType == SMI_TABLE_MEMORY ) || ( aTableType == SMI_TABLE_VOLATILE ) )
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_MEMORY;
        }
        else
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_DISK;
        }

        ///////////////////////////////////////////////////////////////////
        // Proj-1391
        //
        // column type이 fixed인지 variable인지 parsing 단계에서
        // 결정했던 것을 validation 과정에서 하도록 수정함.
        //
        // memory냐 disk이냐에 따라 fixed, variable 정책이 다르기 때문에
        // SMI_COLUMN_STORAGE_MASK로 & 연산한 값을 인자로 넘겨준다.
        // 이미 column.flag에 이 정보가 들어가 있지만,
        // 호출 순서가 반드시 SMI_COLUMN_STORAGE_MASK가 정해진 후에
        // 불려져야 하는 제약을 없애기 위해 파라미터로 받는다.
        //
        IDE_TEST( decideColumnTypeFixedOrVariable(
                      sColumn,
                      sColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK,
                      aTBSID,
                      ID_FALSE /* aIsAddColumn */ )
                  != IDE_SUCCESS );

        IDE_TEST( validateColumnLength( aStatement,
                                        sColumn )
                  != IDE_SUCCESS );

        if( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
            == MTC_COLUMN_TIMESTAMP_TRUE )
        {
            // check duplicate timestamp column
            if ( sExistTimeStamp == ID_TRUE )
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_DUP_TIMESTAMP_COLUMN);
            }
            else
            {
                sExistTimeStamp = ID_TRUE;
            }

            // check default clause
            if (sColumn->defaultValue != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_CANNOT_SET_TIMESTAMP_DEFAULT);
            }
        }

        IDE_TEST_RAISE( (sColumn->basicInfo->module->flag & MTD_CREATE_MASK)
                        == MTD_CREATE_DISABLE,
                        ERR_INVALID_DATATYPE );

        IDE_DASSERT( sColumn->defaultValue == NULL );

        sColumnCount++;
    }

    // PROJ-1391 qp
    // create table은 column offset을 validation과정에서 setting한다.
    if ( aInExecutionTime == ID_FALSE )
    {
        IDE_TEST( setColListOffset( QC_QMP_MEM(aStatement),
                                    aColumn,
                                    sCurrentOffset )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( setColListOffset( QC_QMX_MEM(aStatement),
                                    aColumn,
                                    sCurrentOffset )
                  != IDE_SUCCESS );
    }

    IDE_TEST_RAISE(sColumnCount > QC_MAX_COLUMN_COUNT,
                   ERR_INVALID_COLUMN_COUNT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DATATYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE));
    }
    IDE_EXCEPTION(ERR_DUP_TIMESTAMP_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_TIMESTAMP_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_CANNOT_SET_TIMESTAMP_DEFAULT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_CANNOT_SET_TIMESTAMP_DEFAULT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_COLUMN_COUNT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_COLUMN_COUNT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::validateColumnListForAddCol(
    qcStatement     * aStatement,
    qcmTableInfo    * aTableInfo,
    qcmColumn       * aColumn,
    SInt              aColumnCount)
{
/***********************************************************************
 *
 * Description :
 *      ALTER TABLE ADD COL.. 문의 validation 수행 함수로부터 호출
 *
 * Implementation :
 *    ALTER TABLE
 *    1. ALTER 하고자 하는 컬럼의 존재 여부 체크, 같은 이름이 있으면 에러
 *    2. 각 컬럼 module 의 align,size 로부터 offset 계산해서 부여
 *    3. default 값의 validation 검사 => validateDefaultDefinition 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::validateColumnListForAddCol"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree  * sParseTree;
    UInt                sCurrentOffset;
    qcmColumn         * sColumn;
    qcmColumn         * sColumnInfo;
    idBool              sExistTimeStamp = ID_FALSE;
    SInt                sColumnCount;
    qcuSqlSourceInfo    sqlInfo;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sColumnCount = aColumnCount;

    // PROJ-1391 qp
    // alter table add column은 column offset을 validation과정에서 setting하지만
    // 이는 align 크기를 고려하지 않은 값이지만, constraint validation을 위해서는
    // 여기서 재조정 할 수 없다.
    // 실제 align 크기 고려한 조정은 execution 과정에서 한다.

    // get current record size
    if( smiTableSpace::isMemTableSpaceType( aTableInfo->TBSType )
        == ID_TRUE )
    {
        sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_MEMORY);
    }
    else if ( smiTableSpace::isVolatileTableSpaceType( aTableInfo->TBSType )
              == ID_TRUE )
    {
        sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_VOLATILE);
    }
    else
    {
        IDE_DASSERT( smiTableSpace::isDiskTableSpaceType( aTableInfo->TBSType )
                     == ID_TRUE );

        sCurrentOffset = 0;
    }

    for (sColumn = aTableInfo->columns;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        sCurrentOffset += sColumn->basicInfo->column.size;
        sCurrentOffset = idlOS::align(sCurrentOffset,
                                      sColumn->basicInfo->module->align);
    }

    for (sColumn = aColumn;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        // set SMI_COLUMN_STORAGE_MASK
        if((smiTableSpace::isMemTableSpaceType(aTableInfo->TBSType) == ID_TRUE) ||
           (smiTableSpace::isVolatileTableSpaceType(aTableInfo->TBSType) == ID_TRUE))
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_MEMORY;
        }
        else
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_DISK;
        }

        // BUG-17212 volatile table인 경우 lob 컬럼을 가질 수 없다.
        /* PROJ-2174 Supporting Lob in the volatile tablespace
         * volatile TBS에서 lob을 사용 할 수 있음. */
        if (smiTableSpace::isVolatileTableSpaceType(aTableInfo->TBSType) == ID_TRUE)
        {
            //fix BUG-18244
            IDE_TEST_RAISE( sColumn->basicInfo->module->id
                            == MTD_GEOMETRY_ID,
                            ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );

            /* Volatile TableSpace의 경우 VARIABLE_LARGE를 지원하지 않는다. */
            IDE_TEST_RAISE( ( sColumn->flag & QCM_COLUMN_TYPE_MASK ) 
                            == QCM_COLUMN_TYPE_VARIABLE_LARGE,
                            ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );
        }

        // Bug-10734
        // Alter table add column에서는 Varchar, Fixed 선택하는 부분 빠졌음.
        IDE_TEST( decideColumnTypeFixedOrVariable(
                      sColumn,
                      sColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK,
                      aTableInfo->TBSID,
                      ID_TRUE /* aIsAddColumn */ )
                  != IDE_SUCCESS );

        IDE_TEST( validateColumnLength( aStatement,
                                        sColumn )
                  != IDE_SUCCESS );

        if( ( sColumn->basicInfo->flag & MTC_COLUMN_TIMESTAMP_MASK )
            == MTC_COLUMN_TIMESTAMP_TRUE )
        {
            if ( aTableInfo->timestamp != NULL)
            {
                // A table has already a timestamp.
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_DUP_TIMESTAMP_COLUMN);
            }

            // check duplicate timestamp column in ALTER TABLE statement
            if ( sExistTimeStamp == ID_TRUE )
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_DUP_TIMESTAMP_COLUMN);
            }
            else
            {
                sExistTimeStamp = ID_TRUE;
            }

            // check default clause
            if (sColumn->defaultValue != NULL)
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_CANNOT_SET_TIMESTAMP_DEFAULT);
            }
        }

        // check column existence
        if (qcmCache::getColumn(
                aStatement,
                aTableInfo,
                sColumn->namePos,
                &sColumnInfo)
            == IDE_SUCCESS)
        {
            if ( sParseTree->addHiddenColumn == ID_TRUE )
            {
                // hidden column의 추가인 경우 column name을 출력하지 않는다.
                IDE_RAISE(ERR_DUP_HIDDEN_COLUMN_NAME);
            }
            else
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sColumn->namePos );
                IDE_RAISE(ERR_DUP_COLUMN_NAME);
            }
        }

        // align
        sCurrentOffset = idlOS::align(sCurrentOffset,
                                      sColumn->basicInfo->module->align);
        // set offset
        sColumn->basicInfo->column.offset = sCurrentOffset;
        // increment offset using size.
        sCurrentOffset += sColumn->basicInfo->column.size;

        IDE_TEST_RAISE( (sColumn->basicInfo->module->flag & MTD_CREATE_MASK)
                        == MTD_CREATE_DISABLE,
                        ERR_INVALID_DATATYPE );

        // validation of default value
        if ( sColumn->defaultValue != NULL )
        {
            if ( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                // hidden column이 add될때 default expression은 이미 검사했다.
                // Nothing to do.
            }
            else
            {
                IDE_TEST(validateDefaultDefinition(
                             aStatement, sColumn->defaultValue)
                         != IDE_SUCCESS);

                // PROJ-1579 NCHAR
                // 각 컬럼에 nchar literal의 list를 만들어 놓는다.
                IDE_TEST( makeNcharLiteralStr( aStatement,
                                               sParseTree->ncharList,
                                               sColumn )
                          != IDE_SUCCESS );
            }
        }

        // BUG-43924 Join Update, Delete
        sColumn->basicInfo->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
        sColumn->basicInfo->flag |= MTC_COLUMN_KEY_PRESERVED_TRUE;

        sColumnCount++;
    }

    IDE_TEST_RAISE(sColumnCount > QC_MAX_COLUMN_COUNT,
                   ERR_INVALID_COLUMN_COUNT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DATATYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE));
    }
    IDE_EXCEPTION(ERR_DUP_COLUMN_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DUP_HIDDEN_COLUMN_NAME);
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME) );
    }
    IDE_EXCEPTION(ERR_DUP_TIMESTAMP_COLUMN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_TIMESTAMP_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_CANNOT_SET_TIMESTAMP_DEFAULT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_CANNOT_SET_TIMESTAMP_DEFAULT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_COLUMN_COUNT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_COLUMN_COUNT));
    }
    IDE_EXCEPTION(ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::validateColumnListForModifyCol(
    qcStatement     * aStatement,
    qcmTableInfo    * aTableInfo,
    qcmColumn       * aColumn)
{
/***********************************************************************
 *
 * Description :
 *      ALTER TABLE MODIFY COL.. 문의 validation 수행 함수로부터 호출
 *
 * Implementation :
 *    ALTER TABLE
 *    1. default 값의 validation 검사 => validateDefaultDefinition 호출
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::validateColumnListForModifyCol"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree  * sParseTree;
    qcmColumn         * sColumn;
    qcmColumn         * sModifyCol;
    UInt                sTableType;
    UInt                sType;
    UInt                sIndexCount;
    qcmIndex          * sIndex;
    qcmColumn         * sCheckIndexCol;
    mtcColumn         * sKeyCol;
    UInt                sKeyCount;
    idBool              sFound;
    idBool              sIsCheck;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( QC_QME_MEM(aStatement)->alloc(ID_SIZEOF(qcmColumn) * ( aTableInfo->columnCount ),
                                       (void**)&sCheckIndexCol) != IDE_SUCCESS );

    for (sColumn = aColumn;
         sColumn != NULL;
         sColumn = sColumn->next)
    {
        // set SMI_COLUMN_STORAGE_MASK
        if((smiTableSpace::isMemTableSpaceType(aTableInfo->TBSType) == ID_TRUE) ||
           (smiTableSpace::isVolatileTableSpaceType(aTableInfo->TBSType) == ID_TRUE))
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_MEMORY;
        }
        else
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_DISK;
        }

        // Bug-10734
        // Alter table add column에서는 Varchar, Fixed 선택하는 부분 빠졌음.
        IDE_TEST( decideColumnTypeFixedOrVariable(
                      sColumn,
                      sColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK,
                      aTableInfo->TBSID,
                      ID_FALSE /* aIsAddColumn */ )
                  != IDE_SUCCESS );

        IDE_TEST( validateColumnLength( aStatement,
                                        sColumn )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (sColumn->basicInfo->module->flag & MTD_CREATE_MASK)
                        == MTD_CREATE_DISABLE,
                        ERR_INVALID_DATATYPE );

        // validation of default value
        if (sColumn->defaultValue != NULL)
        {
            IDE_TEST(validateDefaultDefinition(
                         aStatement, sColumn->defaultValue)
                     != IDE_SUCCESS);

            // PROJ-1579 NCHAR
            // 각 컬럼에 nchar literal의 list를 만들어 놓는다.
            IDE_TEST( makeNcharLiteralStr( aStatement,
                                           sParseTree->ncharList,
                                           sColumn )
                      != IDE_SUCCESS );
        }

        // BUG-43924 Join Update, Delete
        sColumn->basicInfo->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
        sColumn->basicInfo->flag |= MTC_COLUMN_KEY_PRESERVED_TRUE;
    }

    // BUG - 31945
    // 변경된 인덱스 컬럼 크기 체크 추가

    for (sIndexCount = 0;
         sIndexCount < aTableInfo->indexCount;
         sIndexCount++)
    {
        sIndex = &(aTableInfo->indices[sIndexCount]);

        sIsCheck = ID_FALSE;

        for(sKeyCount = 0;
            sKeyCount < sIndex->keyColCount;
            sKeyCount++)
        {
            sKeyCol = &(sIndex->keyColumns[sKeyCount]);

            sFound = ID_FALSE;

            for (sModifyCol = aColumn;
                 sModifyCol != NULL;
                 sModifyCol = sModifyCol->next)
            {
                if ( sKeyCol->column.id ==
                     sModifyCol->basicInfo->column.id )
                {
                    /* 변경된 컬럼을 사용한다 */
                    sFound = ID_TRUE;
                    sIsCheck = ID_TRUE;
                    sCheckIndexCol[sKeyCount].basicInfo = sModifyCol->basicInfo;
                    break;
                }
                else
                {
                    // Nothing to do
                }
            }

            if (sFound == ID_FALSE)
            {
                /* 기존 컬럼을 사용한다 */
                sCheckIndexCol[sKeyCount].basicInfo = sKeyCol;
            }
            else
            {
                // Nothing to do
            }

            if (sKeyCount + 1 < sIndex->keyColCount)
            {
                sCheckIndexCol[sKeyCount].next = &sCheckIndexCol[sKeyCount+1];
            }
            else
            {
                // Nothing to do
            }
        }

        sCheckIndexCol[sKeyCount-1].next = NULL;

        if ( sIsCheck == ID_TRUE)
        {
            /* key size limit 검사 */
            sTableType = aTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;

            //fix BUG-32391
            sType = sIndex->indexTypeId;

            IDE_TEST( qdx::validateKeySizeLimit(
                          aStatement,
                          QC_QMP_MEM(aStatement),
                          sTableType,
                          (void *) sCheckIndexCol,
                          sIndex->keyColCount,
                          sType) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DATATYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::validateLobAttributeList(
    qcStatement           * aStatement,
    qcmTableInfo          * aTableInfo,
    qcmColumn             * aColumn,
    smiTableSpaceAttr     * aTBSAttr,
    qdLobAttribute        * aLobAttr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1362
 *      CREATE TABLE, ALTER TABLE ADD COL.. 문의 validation 수행 함수로부터 호출
 *
 * Implementation :
 *    1. lob type으로 선언된 컬럼인지 검사
 *    2. lob storage로 tablespace를 명시한 경우 tablespace 검사
 *       2.1 메모리테이블인 경우
 *           lob 컬럼은 디스크 tablespace에 저장할 수 없다.
 *       2.2 디스크테이블인 경우
 *           lob 컬럼은 메모리 tablespace에 저장할 수 없다.
 *    3. LOB STORE AS .... 아닌 경우 default flag 설정.
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::validateLobAttributeList"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree      * sParseTree;
    qcmColumn             * sColumn;
    qcmColumn             * sLobColumn;
    qdLobAttribute        * sLobAttribute;
    qdLobStorageAttribute * sLobStorageAttribute;
    qdLobStorageAttribute   sLobStorageAttributeStock;
    UInt                    sLobColumnCount;
    idBool                  sFound;
    qcuSqlSourceInfo        sqlInfo;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    for( sLobAttribute = aLobAttr;
         sLobAttribute != NULL;
         sLobAttribute = sLobAttribute->next )
    {
        // 사용자가 설정한 lob storage attribute를 모은다.
        SET_EMPTY_POSITION(sLobStorageAttributeStock.TBSName);
        sLobStorageAttributeStock.logging = ID_TRUE;
        sLobStorageAttributeStock.buffer = ID_TRUE;

        for (sLobStorageAttribute = sLobAttribute->storageAttr;
             sLobStorageAttribute != NULL;
             sLobStorageAttribute = sLobStorageAttribute->next )
        {
            switch (sLobStorageAttribute->type)
            {
                case QD_LOB_STORAGE_ATTR_TABLESPACE:
                    SET_POSITION(sLobStorageAttributeStock.TBSName,
                                 sLobStorageAttribute->TBSName);
                    break;
                case QD_LOB_STORAGE_ATTR_LOGGING:
                    sLobStorageAttributeStock.logging =
                        sLobStorageAttribute->logging;
                    break;
                case QD_LOB_STORAGE_ATTR_BUFFER:
                    sLobStorageAttributeStock.buffer =
                        sLobStorageAttribute->buffer;
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }

        // CREATE TABLE ...
        if( aTableInfo == NULL )
        {
            if ( ( QC_IS_NULL_NAME(sLobStorageAttributeStock.TBSName) == ID_TRUE) ||
                 QC_IS_NAME_MATCHED( sLobStorageAttributeStock.TBSName, sParseTree->TBSName ) )
            {
                sLobStorageAttributeStock.TBSAttr = *aTBSAttr;
            }
            else
            {
                IDE_TEST( qcmTablespace::getTBSAttrByName(
                              aStatement,
                              sLobStorageAttributeStock.TBSName.stmtText +
                              sLobStorageAttributeStock.TBSName.offset,
                              sLobStorageAttributeStock.TBSName.size,
                              &(sLobStorageAttributeStock.TBSAttr))
                          != IDE_SUCCESS );

                IDE_TEST_RAISE(
                    sLobStorageAttributeStock.TBSAttr.mType == SMI_DISK_SYSTEM_TEMP ||
                    sLobStorageAttributeStock.TBSAttr.mType == SMI_DISK_USER_TEMP ||
                    sLobStorageAttributeStock.TBSAttr.mType == SMI_DISK_SYSTEM_UNDO,
                    ERR_NO_CREATE_IN_SYSTEM_TBS );

                IDE_TEST( qdpRole::checkAccessTBS(
                              aStatement,
                              sParseTree->userID,
                              sLobStorageAttributeStock.TBSAttr.mID)
                          != IDE_SUCCESS );

                /* PROJ-2464 hybrid partitioned table 지원
                 *  - HPT를 지원하기 위해서 Partitioned와 Partition의 동일 매체 검사를 수정하였다.
                 *
                 *  - 1. Partition의 TBS Type과 결정된 Lob TBS Type이 동일 매체인지 검사한다.
                 *  - 2. Memory Partition의 경우에 Lob은 Partition TBS와 같은 TBS에 생성해야 한다.
                 */
                /* 1. Partition의 TBS Type과 결정된 Lob TBS Type이 동일 매체인지 검사한다. */
                IDE_TEST_RAISE( qdtCommon::isSameTBSType(
                                    aTBSAttr->mType,
                                    sLobStorageAttributeStock.TBSAttr.mType ) == ID_FALSE,
                                ERR_NO_CREATE_LOB_TYPE_IN_TBS );

                /* 2. Memory Partition의 경우에 Lob은 Partition TBS와 같은 TBS에 생성해야 한다. */
                if ( smiTableSpace::isDiskTableSpaceType( aTBSAttr->mType ) == ID_TRUE )
                {
                    // Nothing to do.
                }
                else
                {
                    // BUG-17055
                    // memory lob인 경우 table과 같은 tbs를 사용해야 함
                    /* PROJ-2334 PMT 파티션드 테이블과 비교하는 것이 아니라 파티션의
                     * 테이블 스페이스와 비교
                     * memory partitioned table은 partition table tbs == lob column tbs */
                    IDE_TEST_RAISE( aTBSAttr->mID !=
                                    sLobStorageAttributeStock.TBSAttr.mID,
                                    ERR_NO_CREATE_LOB_TYPE_IN_TBS );
                }
            }
        }
        else
        {
            if (QC_IS_NULL_NAME(sLobStorageAttributeStock.TBSName) == ID_TRUE)
            {
                sLobStorageAttributeStock.TBSAttr = *aTBSAttr;
            }
            else
            {
                IDE_TEST( qcmTablespace::getTBSAttrByName(
                              aStatement,
                              sLobStorageAttributeStock.TBSName.stmtText +
                              sLobStorageAttributeStock.TBSName.offset,
                              sLobStorageAttributeStock.TBSName.size,
                              &(sLobStorageAttributeStock.TBSAttr))
                          != IDE_SUCCESS );

                IDE_TEST_RAISE(
                    sLobStorageAttributeStock.TBSAttr.mType == SMI_DISK_SYSTEM_TEMP ||
                    sLobStorageAttributeStock.TBSAttr.mType == SMI_DISK_USER_TEMP ||
                    sLobStorageAttributeStock.TBSAttr.mType == SMI_DISK_SYSTEM_UNDO,
                    ERR_NO_CREATE_IN_SYSTEM_TBS );

                IDE_TEST( qdpRole::checkAccessTBS(
                              aStatement,
                              sParseTree->userID,
                              sLobStorageAttributeStock.TBSAttr.mID)
                          != IDE_SUCCESS );

                /* PROJ-2464 hybrid partitioned table 지원
                 *  - HPT를 지원하기 위해서 Partitioned와 Partition의 동일 매체 검사를 수정하였다.
                 *
                 *  - 1. Partition의 TBS Type과 결정된 Lob TBS Type이 동일 매체인지 검사한다.
                 *  - 2. Memory Partition의 경우에 Lob은 Partition TBS와 같은 TBS에 생성해야 한다.
                 */
                /* 1. Partition의 TBS Type과 결정된 Lob TBS Type이 동일 매체인지 검사한다. */
                IDE_TEST_RAISE( qdtCommon::isSameTBSType(
                                    aTBSAttr->mType,
                                    sLobStorageAttributeStock.TBSAttr.mType ) == ID_FALSE,
                                ERR_NO_CREATE_LOB_TYPE_IN_TBS );

                /* 2. Memory Partition의 경우에 Lob은 Partition TBS와 같은 TBS에 생성해야 한다. */
                if ( smiTableSpace::isDiskTableSpaceType( aTBSAttr->mType ) == ID_TRUE )
                {
                    // Nothing to do.
                }
                else
                {
                    // BUG-17055
                    // memory lob인 경우 table과 같은 tbs를 사용해야 함
                    /* PROJ-2334 PMT 파티션드 테이블과 비교하는 것이 아니라 파티션의
                     * 테이블 스페이스와 비교
                     * memory partitioned table은 partition table tbs == lob column tbs */
                    IDE_TEST_RAISE( aTBSAttr->mID !=
                                   sLobStorageAttributeStock.TBSAttr.mID,
                                   ERR_NO_CREATE_LOB_TYPE_IN_TBS);
                }
            }
        }

        // column에 설정한다.
        if (sLobAttribute->columns == NULL)
        {
            // LOB STORE AS ( ... )의 경우
            sLobColumnCount = 0;

            for (sColumn = aColumn;
                 sColumn != NULL;
                 sColumn = sColumn->next)
            {
                if ((sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK)
                    == MTD_COLUMN_TYPE_LOB)
                {
                    // tablespace
                    sColumn->basicInfo->column.colSpace =
                        sLobStorageAttributeStock.TBSAttr.mID;

                    // logging
                    sColumn->basicInfo->column.flag &=
                        ~SMI_COLUMN_LOGGING_MASK;
                    if (sLobStorageAttributeStock.logging == ID_TRUE)
                    {
                        sColumn->basicInfo->column.flag |=
                            SMI_COLUMN_LOGGING;
                    }
                    else
                    {
                        sColumn->basicInfo->column.flag |=
                            SMI_COLUMN_NOLOGGING;
                    }

                    // buffer
                    sColumn->basicInfo->column.flag &=
                        ~SMI_COLUMN_USE_BUFFER_MASK;
                    if (sLobStorageAttributeStock.buffer == ID_TRUE)
                    {
                        sColumn->basicInfo->column.flag |=
                            SMI_COLUMN_USE_BUFFER;
                    }
                    else
                    {
                        sColumn->basicInfo->column.flag |=
                            SMI_COLUMN_USE_NOBUFFER;
                    }

                    sColumn->flag &= ~QCM_COLUMN_LOB_DEFAULT_TBS_MASK;
                    sColumn->flag |= QCM_COLUMN_LOB_DEFAULT_TBS_TRUE;
                    sLobColumnCount++;
                }
                else
                {
                    // Nothing to do.
                }
            }

            IDE_TEST_RAISE( sLobColumnCount == 0,
                            ERR_NOT_FOUND_LOB_TYPE_COLUMN );
        }
        else
        {
            // LOB ( lob_column_list ) STORE AS ( ... )의 경우
            for (sLobColumn = sLobAttribute->columns;
                 sLobColumn != NULL;
                 sLobColumn = sLobColumn->next)
            {
                sFound = ID_FALSE;

                for (sColumn = aColumn;
                     sColumn != NULL;
                     sColumn = sColumn->next)
                {
                    if( sColumn->namePos.size != 0 )
                    {
                        if ( QC_IS_NAME_MATCHED( sLobColumn->namePos, sColumn->namePos ) )
                        {
                            sFound = ID_TRUE;
                        }
                    }
                    else
                    {
                        // BUG-37032
                        if( idlOS::strMatch( sLobColumn->namePos.stmtText + sLobColumn->namePos.offset,
                                             sLobColumn->namePos.size,
                                             sColumn->name,
                                             idlOS::strlen(sColumn->name) ) == 0 )
                        {
                            sFound = ID_TRUE;
                        }
                    }

                    if( sFound == ID_TRUE )
                    {
                        if ((sColumn->basicInfo->module->flag &
                             MTD_COLUMN_TYPE_MASK)
                            != MTD_COLUMN_TYPE_LOB)
                        {
                            sqlInfo.setSourceInfo(aStatement,
                                                  & sLobColumn->namePos);
                            IDE_RAISE( ERR_MISMATCHED_LOB_TYPE_COLUMN );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        // tablespace
                        sColumn->basicInfo->column.colSpace =
                            sLobStorageAttributeStock.TBSAttr.mID;

                        // logging
                        sColumn->basicInfo->column.flag &=
                            ~SMI_COLUMN_LOGGING_MASK;
                        if (sLobStorageAttributeStock.logging == ID_TRUE)
                        {
                            sColumn->basicInfo->column.flag |=
                                SMI_COLUMN_LOGGING;
                        }
                        else
                        {
                            sColumn->basicInfo->column.flag |=
                                SMI_COLUMN_NOLOGGING;
                        }

                        // buffer
                        sColumn->basicInfo->column.flag &=
                            ~SMI_COLUMN_USE_BUFFER_MASK;
                        if (sLobStorageAttributeStock.buffer == ID_TRUE)
                        {
                            sColumn->basicInfo->column.flag |=
                                SMI_COLUMN_USE_BUFFER;
                        }
                        else
                        {
                            sColumn->basicInfo->column.flag |=
                                SMI_COLUMN_USE_NOBUFFER;
                        }

                        sColumn->flag &= ~QCM_COLUMN_LOB_DEFAULT_TBS_MASK;
                        sColumn->flag |= QCM_COLUMN_LOB_DEFAULT_TBS_TRUE;

                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if (sFound != ID_TRUE)
                {
                    sqlInfo.setSourceInfo(aStatement,
                                          & sLobColumn->namePos);
                    IDE_RAISE( ERR_NOT_FOUND_LOB_TYPE_COLUMN );
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_CREATE_IN_SYSTEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS));
    }
    IDE_EXCEPTION(ERR_NO_CREATE_LOB_TYPE_IN_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_CREATE_LOB_TYPE_IN_TBS));
    }
    IDE_EXCEPTION(ERR_MISMATCHED_LOB_TYPE_COLUMN)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_MISMATCHED_LOB_TYPE_COLUMN,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_FOUND_LOB_TYPE_COLUMN)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOT_FOUND_LOB_TYPE_COLUMN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::validateDefaultDefinition(
    qcStatement     * aStatement,
    qtcNode         * aDefault)
{
/***********************************************************************
 *
 * Description :
 *    DEFAULT 값으로 올 수 있는 것인지 체크
 *
 * Implementation :
 *    1. qtc::estimate ???
 *    2. default 값으로 올 수 없는 것들(시퀀스,서브쿼리)이면 에러
 *
 ***********************************************************************/

    qcuSqlSourceInfo    sqlInfo;

    // BUG-38099
    if( aDefault->subquery != NULL )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            & aDefault->position );
        IDE_RAISE(ERR_NOT_ALLOW_SUBQ);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST(qtc::estimate(
                 aDefault,
                 QC_SHARED_TMPLATE(aStatement),
                 aStatement,
                 NULL,
                 NULL,
                 NULL )
             != IDE_SUCCESS);
    
    if ( ( aDefault->node.lflag & MTC_NODE_DML_MASK ) == MTC_NODE_DML_UNUSABLE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            & aDefault->position );
        IDE_RAISE(ERR_USE_CURSOR_ATTR);
    }

    if ( ( aDefault->lflag & QTC_NODE_SUBQUERY_MASK ) ==
         QTC_NODE_SUBQUERY_EXIST )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            & aDefault->position );
        IDE_RAISE(ERR_NOT_ALLOW_SUBQ);
    }

    // BUG-41228
    if ( ( ( aStatement->spvEnv->createProc == NULL ) &&
           ( aStatement->spvEnv->createPkg  == NULL ) )
         &&
         ( ( aDefault->lflag & QTC_NODE_PROC_FUNCTION_MASK ) ==
           QTC_NODE_PROC_FUNCTION_TRUE ) )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            & aDefault->position );
        IDE_RAISE(ERR_NOT_ALLOW_PROC);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOW_SUBQ);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_NOT_ALLOWED_SUBQUERY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOW_PROC);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_NOT_ALLOWED_PROC_IN_DEFAULT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateDefaultExprDefinition(
    qcStatement     * aStatement,
    qtcNode         * aNode,
    qmsSFWGH        * aSFWGH,
    qmsFrom         * aFrom )
{
/***********************************************************************
 *
 * Description :
 *  Column을 포함한 expression의 디폴트 값을 검사한다.
 *
 * Implementation :
 *  1. qtc::estimate()
 *      - 제약 조건에 관련된 Flag를 설정한다.
 *  2. 가변 Default 제약 검사
 *      - 해당 Row의 범위를 벗어나거나, 결과가 바뀌는 것을 포함할 수 없다.
 *
 ***********************************************************************/

    SChar sDefaultStr[QCM_MAX_DEFAULT_VAL + 1] = { '\0', };

    IDE_TEST_RAISE( qtc::estimate( aNode,
                                   QC_SHARED_TMPLATE(aStatement),
                                   aStatement,
                                   NULL,
                                   aSFWGH,
                                   aFrom )
                    != IDE_SUCCESS,
                    ERR_INVALID_EXPRESSION );

    /* Subquery
     * Pseudo-Column
     * Non-deterministic Function과 User-defined Function
     * PRIOR 연산자
     * sequence
     */
    IDE_TEST_RAISE( ( QTC_IS_VARIABLE( aNode ) == ID_TRUE ) ||
                    ( (aNode->lflag & QTC_NODE_SEQUENCE_MASK) == QTC_NODE_SEQUENCE_EXIST ),
                    ERR_INVALID_EXPRESSION );

    /* Stored Procedure */
    IDE_TEST_RAISE( (aNode->node.lflag & MTC_NODE_DML_MASK) == MTC_NODE_DML_UNUSABLE,
                    ERR_USE_CURSOR_ATTR_VARIABLE_DEFAULT );

    /* BUG-36728 Check Constraint, Function-Based Index에서 Synonym을 사용할 수 없어야 합니다. */
    IDE_TEST_RAISE( ( aNode->lflag & QTC_NODE_SP_SYNONYM_FUNC_MASK ) == QTC_NODE_SP_SYNONYM_FUNC_TRUE,
                    ERR_INVALID_EXPRESSION );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_EXPRESSION )
    {
        QC_STR_COPY( sDefaultStr, aNode->position );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_DEFAULT_EXPRESSION,
                                  sDefaultStr ) );
    }
    IDE_EXCEPTION( ERR_USE_CURSOR_ATTR_VARIABLE_DEFAULT );
    {
        QC_STR_COPY( sDefaultStr, aNode->position );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                                  sDefaultStr ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateCheckConstrDefinition(
    qcStatement      * aStatement,
    qdConstraintSpec * aCheckConstr,
    qmsSFWGH         * aSFWGH,
    qmsFrom          * aFrom )
{
/***********************************************************************
 *
 * Description :
 *  Check Constraint로 지정할 수 있는지 검사한다.
 *
 * Implementation :
 *  1. qtc::estimate()
 *      - 제약 조건에 관련된 Flag를 설정한다.
 *      - 최종 결과는 TRUE, FALSE, NULL 이어야 한다.
 *  2. Check Constraint 제약 검사
 *      - 해당 Row의 범위를 벗어나거나, 결과가 바뀌는 것을 포함할 수 없다.
 *
 ***********************************************************************/

    qtcNode     * sNode = aCheckConstr->checkCondition;
    mtcColumn   * sColumn;
    SChar         sConstrName[QC_MAX_OBJECT_NAME_LEN + 1] = { '\0', };

    IDE_TEST_RAISE( qtc::estimate( sNode,
                                   QC_SHARED_TMPLATE(aStatement),
                                   aStatement,
                                   NULL,
                                   aSFWGH,
                                   aFrom )
                    != IDE_SUCCESS, ERR_INVALID_EXPRESSION );

    sColumn = &(QC_SHARED_TMPLATE(aStatement)->tmplate.
                rows[sNode->node.table].columns[sNode->node.column]);

    IDE_TEST_RAISE( sColumn->module != &mtdBoolean,
                    ERR_INSUFFICIENT_ARGUEMNT );

    /* Subquery
     * Pseudo-Column
     * Non-deterministic Function과 User-defined Function
     * PRIOR 연산자
     */
    IDE_TEST_RAISE( QTC_IS_VARIABLE( sNode ) == ID_TRUE,
                    ERR_USE_VARIABLE_IN_CHECK_CONSTRAINT );

    /* Sequence */
    IDE_TEST_RAISE( (sNode->lflag & QTC_NODE_SEQUENCE_MASK) == QTC_NODE_SEQUENCE_EXIST,
                    ERR_USE_SEQUENCE_IN_CHECK_CONSTRAINT );

    /* Stored Procedure */
    IDE_TEST_RAISE( (sNode->node.lflag & MTC_NODE_DML_MASK) == MTC_NODE_DML_UNUSABLE,
                    ERR_USE_CURSOR_ATTR_CHECK_CONSTRAINT );

    /* BUG-36728 Check Constraint, Function-Based Index에서 Synonym을 사용할 수 없어야 합니다. */
    IDE_TEST_RAISE( ( sNode->lflag & QTC_NODE_SP_SYNONYM_FUNC_MASK ) == QTC_NODE_SP_SYNONYM_FUNC_TRUE,
                    ERR_USE_SYNONYM_IN_CHECK_CONSTRAINT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_EXPRESSION )
    {
        if ( QC_IS_NULL_NAME( aCheckConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, aCheckConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_INVALID_CHECK_CONSTRAINT_EXPRESSION,
                                  sConstrName ) );
    }
    IDE_EXCEPTION( ERR_INSUFFICIENT_ARGUEMNT );
    {
        if ( QC_IS_NULL_NAME( aCheckConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, aCheckConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_INSUFFICIENT_ARGUEMNT_SQLTEXT,
                                  sConstrName ) );
    }
    IDE_EXCEPTION( ERR_USE_VARIABLE_IN_CHECK_CONSTRAINT );
    {
        if ( QC_IS_NULL_NAME( aCheckConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, aCheckConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USE_VARIABLE_IN_CHECK_CONSTRAINT,
                                  sConstrName ) );
    }
    IDE_EXCEPTION( ERR_USE_SEQUENCE_IN_CHECK_CONSTRAINT );
    {
        if ( QC_IS_NULL_NAME( aCheckConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, aCheckConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USE_SEQUENCE_IN_CHECK_CONSTRAINT,
                                  sConstrName ) );
    }
    IDE_EXCEPTION( ERR_USE_CURSOR_ATTR_CHECK_CONSTRAINT );
    {
        if ( QC_IS_NULL_NAME( aCheckConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, aCheckConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                                  sConstrName ) );
    }
    IDE_EXCEPTION( ERR_USE_SYNONYM_IN_CHECK_CONSTRAINT );
    {
        if ( QC_IS_NULL_NAME( aCheckConstr->constrName ) != ID_TRUE )
        {
            QC_STR_COPY( sConstrName, aCheckConstr->constrName );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USE_SYNONYM_IN_CHECK_CONSTRAINT,
                                  sConstrName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::convertDefaultValueType( qcStatement * aStatement,
                                           mtcId       * aType,
                                           qtcNode     * aDefault,
                                           idBool      * aIsNull )
{
/***********************************************************************
 *
 * Description :
 *    명시된 디폴트 값을 그 컬럼의 데이터타입에 맞춰서 변환한다
 *
 * Implementation :
 *    1. qtc::calculate
 *    2. 명시된 값의 타입과 컬럼의 타입이 다르면
 *       mtv::estimateConvert4Server
 *       mtv::executeConvert
 *
 ***********************************************************************/

#define IDE_FN "convertDefaultValueType"
    mtcStack          * sStack;
    void              * sValue;
    mtcColumn         * sValueColumn;
    UInt                sArguCount;
    mtvConvert        * sConvert;

    sStack = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack;

    // set SYSDATE
    // To fix BUG-15217
    // calculate를 하기 전에 sysdate를 세팅해야 한다.
    IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aStatement ) ) != IDE_SUCCESS );

    // Sequence Value 임시 저장
    IDE_TEST( qmx::dummySequenceNextVals(
                  aStatement,
                  aStatement->myPlan->parseTree->nextValSeqs )
              != IDE_SUCCESS );
    
    IDE_TEST(qtc::calculate(aDefault, QC_PRIVATE_TMPLATE(aStatement))
             != IDE_SUCCESS);

    sValueColumn = sStack->column;
    sValue       = sStack->value;

    /* PROJ-1361 : data type과 language module 분리했음
       if (aType->type != sValueColumn->type.type ||
       aType->language != sValueColumn->type.language)
    */
    if (aType->dataTypeId != sValueColumn->type.dataTypeId )
    {
        // different type.
        sArguCount = sValueColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;
        IDE_TEST(mtv::estimateConvert4Server(
                     aStatement->qmxMem,
                     &sConvert,
                     *aType,                         // aDestinationType
                     sValueColumn->type,             // aSourceType
                     sArguCount,                     // aSourceArgument
                     sValueColumn->precision,        // aSourcePrecision
                     sValueColumn->scale,            // aSourceScale
                     &QC_PRIVATE_TMPLATE(aStatement)->tmplate)  // mtcTemplate* : for passing session dateFormat
                 != IDE_SUCCESS);
        sConvert->stack[sConvert->count].value = sStack->value;
        // destination value pointer
        sValueColumn = sConvert->stack->column;
        sValue       = sConvert->stack->value;

        IDE_TEST( mtv::executeConvert( sConvert,
                                       &QC_PRIVATE_TMPLATE(aStatement)->tmplate )
                  != IDE_SUCCESS );
    }

    // BUG-26151
    if ( aIsNull != NULL )
    {
        if ( sValueColumn->module->isNull( sValueColumn,
                                           sValue )
             == ID_TRUE )
        {
            *aIsNull = ID_TRUE;
        }
        else
        {
            *aIsNull = ID_FALSE;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::calculateDefaultValueWithSequence( qcStatement  * aStatement,
                                                     qcmTableInfo * aTableInfo,
                                                     qcmColumn    * aSrcColumns,
                                                     qcmColumn    * aDestColumns,
                                                     smiValue     * aNewRow )
{
/***********************************************************************
 *
 * Description :
 *    시퀀스가 있는 default value를 계산한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn      * sSrcCol;
    qcmColumn      * sDestCol;
    UInt             sColumnOrder;
    mtcStack       * sStack;
    void           * sValue;
    mtcColumn      * sValueColumn;
    UInt             sArguCount;
    mtvConvert     * sConvert = NULL;
    mtcEncryptInfo   sEncryptInfo;
    void           * sCanonizedValue;
    UInt             sStoringSize = 0;
    void           * sStoringValue = NULL;

    sStack = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack;

    if ( aStatement->myPlan->parseTree->nextValSeqs != NULL )
    {
        //------------------------------------------
        // Sequence Value 획득
        //------------------------------------------
        
        IDE_TEST( qmx::readSequenceNextVals(
                      aStatement,
                      aStatement->myPlan->parseTree->nextValSeqs )
                  != IDE_SUCCESS );
        
        //------------------------------------------
        // 추가된 컬럼에 대해서 default value 계산
        //------------------------------------------
        
        sDestCol = aDestColumns;
        while (sDestCol != NULL)
        {
            sSrcCol = aSrcColumns;
            while (sSrcCol != NULL)
            {
                if ( idlOS::strMatch( sSrcCol->name,
                                      idlOS::strlen( sSrcCol->name ),
                                      sDestCol->name,
                                      idlOS::strlen( sDestCol->name ) ) == 0 )
                {
                    break;
                }
                else
                {
                    sSrcCol = sSrcCol->next;
                }
            }
            
            // NEW에만 해당하는 smiColumnList
            if ( (sSrcCol == NULL) &&
                 (sDestCol->defaultValue != NULL) )
            {
                IDE_TEST(qtc::calculate(sDestCol->defaultValue,
                                        QC_PRIVATE_TMPLATE(aStatement))
                         != IDE_SUCCESS);

                sValueColumn = sStack->column;
                sValue       = sStack->value;

                /* PROJ-1361 : data type과 language module 분리했음
                   if (aType->type != sValueColumn->type.type ||
                   aType->language != sValueColumn->type.language)
                */
                if (sDestCol->basicInfo->type.dataTypeId !=
                    sValueColumn->type.dataTypeId )
                {
                    // different type.
                    sArguCount = sValueColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;
                    
                    IDE_TEST(mtv::estimateConvert4Server(
                                 aStatement->qmxMem,
                                 &sConvert,
                                 sDestCol->basicInfo->type,     //aDestinationType
                                 sValueColumn->type,            //aSourceType
                                 sArguCount,                    // aSourceArgument
                                 sValueColumn->precision,       // aSourcePrecision
                                 sValueColumn->scale,           // aSourceScale
                                 & QC_PRIVATE_TMPLATE(aStatement)->tmplate) // mtcTemplate* :
                             // for passing session
                             // dateFormat
                             != IDE_SUCCESS);
                    
                    sConvert->stack[sConvert->count].value = sStack->value;
                    
                    // destination value pointer
                    sValueColumn = sConvert->stack->column;
                    sValue       = sConvert->stack->value;

                    IDE_TEST( mtv::executeConvert( sConvert,
                                                   &QC_PRIVATE_TMPLATE(aStatement)->tmplate )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
                    
                // PROJ-2002 Column Security
                if ( ( sDestCol->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK )
                     == MTD_ENCRYPT_TYPE_TRUE )
                {
                    IDE_TEST( qcsModule::getEncryptInfo( aStatement,
                                                         aTableInfo,
                                                         sDestCol,
                                                         & sEncryptInfo )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                // canonize
                if ( ( sDestCol->basicInfo->module->flag & MTD_CANON_MASK )
                     == MTD_CANON_NEED )
                {
                    sCanonizedValue = sValue;

                    IDE_TEST( sDestCol->basicInfo->module->canonize(
                                  sDestCol->basicInfo,
                                  & sCanonizedValue,           // canonized value
                                  & sEncryptInfo,
                                  sValueColumn,
                                  sValue,                     // original value
                                  NULL,
                                  & QC_PRIVATE_TMPLATE(aStatement)->tmplate )
                              != IDE_SUCCESS );

                    sValue = sCanonizedValue;
                }
                else if ( (sDestCol->basicInfo->module->flag & MTD_CANON_MASK)
                          == MTD_CANON_NEED_WITH_ALLOCATION )
                {
                    IDU_LIMITPOINT("qdbCommon::calculateDefaultValueWithSequence::malloc");
                    IDE_TEST(aStatement->qmxMem->alloc(
                                 sDestCol->basicInfo->column.size,
                                 (void**)&sCanonizedValue)
                             != IDE_SUCCESS);

                    IDE_TEST( sDestCol->basicInfo->module->canonize(
                                  sDestCol->basicInfo,
                                  & sCanonizedValue,           // canonized value
                                  & sEncryptInfo,
                                  sValueColumn,
                                  sValue,                     // original value
                                  NULL,
                                  & QC_PRIVATE_TMPLATE(aStatement)->tmplate )
                              != IDE_SUCCESS );

                    sValue = sCanonizedValue;
                }
                else
                {
                    // Nothing to do.
                }

                sColumnOrder = (sDestCol->basicInfo->column.id & SMI_COLUMN_ID_MASK);

                if ( ( sDestCol->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                     == SMI_COLUMN_TYPE_LOB )
                {
                    if( sValueColumn->module->isNull( sValueColumn,
                                                      sValue )
                        != ID_TRUE )
                    {
                        // PROJ-1362
                        IDE_DASSERT( sValueColumn->module->id
                                     == sDestCol->basicInfo->module->id );
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
                else
                {
                    // Nothing To Do
                }

                // PROJ-1705
                IDE_TEST( qdbCommon::mtdValue2StoringValue( sDestCol->basicInfo,
                                                            sValueColumn,
                                                            sValue,
                                                            &sStoringValue )
                          != IDE_SUCCESS );
                aNewRow[sColumnOrder].value = sStoringValue;

                IDE_TEST( qdbCommon::storingSize( sDestCol->basicInfo,
                                                  sValueColumn,
                                                  sValue,
                                                  &sStoringSize )
                          != IDE_SUCCESS );
                aNewRow[sColumnOrder].length = sStoringSize;
            }
            else
            {
                // Nothing to do.
            }

            sDestCol = sDestCol->next;
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

IDE_RC qdbCommon::getMtcColumnFromTarget(
    qcStatement       * aStatement,
    qtcNode           * aTarget,
    qcmColumn         * aColumn,
    scSpaceID           aTablespaceID)
{
/***********************************************************************
 *
 * Description :
 *    qtcNode -> mtcNode
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcNode     * sTargetNode;
    mtcTuple    * sTargetTuple;
    mtcColumn   * sTargetColumn;
    mtcColumn   * sColumn;

    sTargetNode = mtf::convertedNode( (mtcNode *)aTarget,
                                      &QC_SHARED_TMPLATE(aStatement)->tmplate );

    // BUG-38613 _prowid 를 사용한 view 를 select 할수 있어야함
    sTargetTuple  = QTC_STMT_TUPLE( aStatement, (qtcNode*)sTargetNode );
    sTargetColumn = QTC_TUPLE_COLUMN( sTargetTuple, (qtcNode*)sTargetNode );

    // BUG-16233
    IDU_LIMITPOINT("qdbCommon::getMtcColumnFromTarget::malloc");
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &sColumn)
             != IDE_SUCCESS);

    // PROJ-1362
    // create table as select * ...  경우 *가 locator로 풀리게 되므로
    // locator일 경우 다시 lob으로 바꾼다.
    // create table as select CLOB'~' ... 경우 lob의 value형이므로
    // 다시 lob으로 바꾼다.
    if ( (sTargetColumn->module->id == MTD_BLOB_LOCATOR_ID) ||
         (sTargetColumn->module->id == MTD_BLOB_ID) )
    {
        IDE_TEST(mtc::initializeColumn(sColumn,
                                       MTD_BLOB_ID,
                                       1, /* BUG-36429 LOB Column으로 생성 */
                                       0,
                                       0)
                 != IDE_SUCCESS);

        // lob 컬럼으로 설정
        sColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
        sColumn->column.flag |= SMI_COLUMN_TYPE_LOB;
        sColumn->column.size = ID_UINT_MAX;
        sColumn->column.colSpace = aTablespaceID;
    }
    else if ( (sTargetColumn->module->id == MTD_CLOB_LOCATOR_ID) ||
              (sTargetColumn->module->id == MTD_CLOB_ID) )
    {
        IDE_TEST(mtc::initializeColumn(sColumn,
                                       MTD_CLOB_ID,
                                       1, /* BUG-36429 LOB Column으로 생성 */
                                       0,
                                       0)
                 != IDE_SUCCESS);

        // lob 컬럼으로 설정
        sColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
        sColumn->column.flag |= SMI_COLUMN_TYPE_LOB;
        sColumn->column.size = ID_UINT_MAX;
        sColumn->column.colSpace = aTablespaceID;
    }
    else if ( sTargetColumn->module->id == MTD_ECHAR_ID )
    {
        // PROJ-2002 Column Security
        IDE_TEST(mtc::initializeColumn(sColumn,
                                       MTD_CHAR_ID,
                                       1,
                                       sTargetColumn->precision,
                                       0)
                 != IDE_SUCCESS);
    }
    else if ( sTargetColumn->module->id == MTD_EVARCHAR_ID )
    {
        // PROJ-2002 Column Security
        IDE_TEST(mtc::initializeColumn(sColumn,
                                       MTD_VARCHAR_ID,
                                       1,
                                       sTargetColumn->precision,
                                       0)
                 != IDE_SUCCESS);
    }
    else
    {
        // BUG-22040
        IDE_TEST_RAISE(
            (sTargetColumn->module->flag & MTD_CREATE_MASK) == MTD_CREATE_DISABLE,
            ERR_INVALID_DATATYPE );

        /* PROJ-2586 PSM Parameters and return without precision */
        IDE_TEST( getMtcColumnFromTargetInternal( aTarget,
                                                  sTargetColumn,
                                                  sColumn )
                  != IDE_SUCCESS );

        // PROJ-1473
        sColumn->flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
        sColumn->flag &= ~MTC_COLUMN_USE_TARGET_MASK;
        sColumn->flag &= ~MTC_COLUMN_USE_NOT_TARGET_MASK;
        sColumn->flag &= ~MTC_COLUMN_VIEW_COLUMN_PUSH_MASK;

        // BUG-38771
        // compressed column table 을 이용하여
        // CREATE AS SELECT 로 생성시 일반컬럼으로 생성
        sColumn->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
        sColumn->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
    }

    // PROJ-2204 Join Update, Delete
    // key preserved table이고 key preserved column이면 key preserved column이다.
    if ( QTC_IS_COLUMN( aStatement, (qtcNode*)sTargetNode ) == ID_TRUE )
    {
        if ( ( ( sTargetTuple->lflag & MTC_TUPLE_VIEW_MASK )
               == MTC_TUPLE_VIEW_FALSE ) &&
             ( ( sTargetTuple->lflag & MTC_TUPLE_KEY_PRESERVED_MASK )
               == MTC_TUPLE_KEY_PRESERVED_TRUE ) &&
             ( ( sTargetColumn->flag & MTC_COLUMN_KEY_PRESERVED_MASK )
               == MTC_COLUMN_KEY_PRESERVED_TRUE ) )
        {
            sColumn->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
            sColumn->flag |= MTC_COLUMN_KEY_PRESERVED_TRUE;
        }
        else
        {
            sColumn->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
            sColumn->flag |= MTC_COLUMN_KEY_PRESERVED_FALSE;
        }
    }
    else
    {
        sColumn->flag &= ~MTC_COLUMN_KEY_PRESERVED_MASK;
        sColumn->flag |= MTC_COLUMN_KEY_PRESERVED_FALSE;
    }
    
    aColumn->basicInfo = sColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_DATATYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CREATE_DISABLE_DATA_TYPE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::insertTableSpecIntoMeta(
    qcStatement        * aStatement,
    idBool               aIsPartitionedTable,
    UInt                 aCreateFlag,
    qcNamePosition       aTableNamePos,
    UInt                 aUserID,
    smOID                aTableOID,
    UInt                 aTableID,
    UInt                 aColumnCount,
    ULong                aMaxRows,
    qcmAccessOption      aAccessOption, /* PROJ-2359 Table/Partition Access Option */
    scSpaceID            aTBSID,
    smiSegAttr           aSegAttr,
    smiSegStorageAttr    aSegStoAttr,
    qcmTemporaryType     aTemporaryType,
    UInt                 aParallelDegree, 
    UInt                 aShardFlag )
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_TABLES_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_TABLES_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::insertTableSpecIntoMeta"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar               * sSqlStr;
    SChar                 sTblName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    vSLong                sRowCnt;
    SChar                 sIsPartitioned[2];
    SChar                 sTemporary[2];
    SChar                 sAccessOptionStr[2];  /* PROJ-2359 Table/Partition Access Option */

    SChar                 sTBSName[ SMI_MAX_TABLESPACE_NAME_LEN + 1 ];
    smiTableSpaceAttr     sTBSAttr;

    if ( aIsPartitionedTable != ID_TRUE )
    {
        sIsPartitioned[0] = 'F';
        sIsPartitioned[1] = 0;
    }
    else
    {
        sIsPartitioned[0] = 'T';
        sIsPartitioned[1] = 0;
    }

    // PROJ-1407 Temporary Table
    switch ( aTemporaryType )
    {
        case QCM_TEMPORARY_ON_COMMIT_NONE:
            sTemporary[0] = 'N';
            sTemporary[1] = 0;
            break;

        case QCM_TEMPORARY_ON_COMMIT_DELETE_ROWS:
            sTemporary[0] = 'D';
            sTemporary[1] = 0;
            break;

        case QCM_TEMPORARY_ON_COMMIT_PRESERVE_ROWS:
            sTemporary[0] = 'P';
            sTemporary[1] = 0;
            break;

        default:
            IDE_ASSERT(0);
    }
    
    /* PROJ-2359 Table/Partition Access Option */
    switch ( aAccessOption )
    {
        case QCM_ACCESS_OPTION_READ_ONLY :
            sAccessOptionStr[0] = 'R';
            sAccessOptionStr[1] = '\0';
            break;

        case QCM_ACCESS_OPTION_READ_WRITE :
        case QCM_ACCESS_OPTION_NONE :
            sAccessOptionStr[0] = 'W';
            sAccessOptionStr[1] = '\0';
            break;

        case QCM_ACCESS_OPTION_READ_APPEND :
            sAccessOptionStr[0] = 'A';
            sAccessOptionStr[1] = '\0';
            break;

        default :
            IDE_ASSERT( 0 );
    }

    QC_STR_COPY( sTblName, aTableNamePos );

    IDU_LIMITPOINT("qdbCommon::insertTableSpecIntoMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);
    
    
    if ( ( aCreateFlag & QDV_VIEW_DDL_MASK )
         == QDV_VIEW_DDL_TRUE )
    {
        // CREATE VIEW
        IDE_TEST( qcmTablespace::getTBSAttrByID( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                 & sTBSAttr )
                 != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "     // 1
                         "INTEGER'%"ID_INT32_FMT"', "     // 2
                         "BIGINT'%"ID_INT64_FMT"', "      // 3
                         "INTEGER'%"ID_INT32_FMT"', "     // 4
                         "VARCHAR'%s', CHAR'V', "         // 5, 6
                         "INTEGER'0', "                   // 7
                         "INTEGER'0', "                   // 8 proj-1608
                         "BIGINT'%"ID_INT64_FMT"', "      // 9
                         "INTEGER'%"ID_INT32_FMT"', "     // 10
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "     // 12
                         "INTEGER'%"ID_INT32_FMT"', "     // 13
                         "INTEGER'%"ID_INT32_FMT"', "     // 14
                         "INTEGER'%"ID_INT32_FMT"', "     // 15
                         "BIGINT'%"ID_INT64_FMT"', "      // 16
                         "BIGINT'%"ID_INT64_FMT"', "      // 17
                         "BIGINT'%"ID_INT64_FMT"', "      // 18
                         "BIGINT'%"ID_INT64_FMT"', "      // 19
                         "CHAR'F',"                       // 20  IS_PARTITIONED는 무조건 'F'
                         "CHAR'N',"                       // 21  TEMPORARY는 무조건 'N'
                         "CHAR'N',"                       // 22  HIDDEN은 무조건 'N'
                         "CHAR'W',"                       // 23  ACCESS는 무조건 'W'
                         "INTEGER'1',"                    // 24  PARALLEL_DEGREE
                         "CHAR'Y',"                       // 25  USABLE는 무조건 'Y'
                         "INTEGER'0',"                    // 26  SHARD_FLAG는 무조건 0
                         "SYSDATE, SYSDATE )",            // 27, 28
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                         sTBSName,
                         QD_MEMORY_TABLE_DEFAULT_PCTFREE,
                         QD_MEMORY_TABLE_DEFAULT_PCTUSED,
                         QD_MEMORY_TABLE_DEFAULT_INITRANS,
                         QD_MEMORY_TABLE_DEFAULT_MAXTRANS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS );
    }
    else if ( ( aCreateFlag & QDQ_QUEUE_MASK )
              == QDQ_QUEUE_TRUE )
    {
        // CREATE QUEUE
        IDE_TEST( qcmTablespace::getTBSAttrByID( aTBSID,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'Q', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'F',"
                         "CHAR'N',"
                         "CHAR'N',"
                         "CHAR'W',"
                         "INTEGER'1', "
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'0', "                   // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) aTBSID,
                         sTBSName,
                         QD_MEMORY_TABLE_DEFAULT_PCTFREE,
                         QD_MEMORY_TABLE_DEFAULT_PCTUSED,
                         QD_MEMORY_TABLE_DEFAULT_INITRANS,
                         QD_MEMORY_TABLE_DEFAULT_MAXTRANS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS );
    }
    else if ( ( aCreateFlag & QDV_MVIEW_TABLE_DDL_MASK )
              == QDV_MVIEW_TABLE_DDL_TRUE )
    {
        /* PROJ-2211 Materialized View
         *  CREATE MATERIALIZED VIEW (MView Table)
         */
        IDE_TEST( qcmTablespace::getTBSAttrByID( aTBSID,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'M', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'%s',"
                         "CHAR'N',"
                         "CHAR'N',"
                         "CHAR'W',"
                         "INTEGER'1', "
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'0', "                   // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) aTBSID,
                         sTBSName,
                         aSegAttr.mPctFree,
                         aSegAttr.mPctUsed,
                         aSegAttr.mInitTrans,
                         aSegAttr.mMaxTrans,
                         (ULong)aSegStoAttr.mInitExtCnt,
                         (ULong)aSegStoAttr.mNextExtCnt,
                         (ULong)aSegStoAttr.mMinExtCnt,
                         (ULong)aSegStoAttr.mMaxExtCnt,
                         sIsPartitioned );
    }
    else if ( ( aCreateFlag & QDV_MVIEW_VIEW_DDL_MASK )
              == QDV_MVIEW_VIEW_DDL_TRUE )
    {
        /* PROJ-2211 Materialized View
         *  CREATE MATERIALIZED VIEW (MView View)
         */
        IDE_TEST( qcmTablespace::getTBSAttrByID( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
 
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'A', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'F',"
                         "CHAR'N',"
                         "CHAR'N',"
                         "CHAR'W',"
                         "INTEGER'1', "
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'0', "                   // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType)SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                         sTBSName,
                         QD_MEMORY_TABLE_DEFAULT_PCTFREE,
                         QD_MEMORY_TABLE_DEFAULT_PCTUSED,
                         QD_MEMORY_TABLE_DEFAULT_INITRANS,
                         QD_MEMORY_TABLE_DEFAULT_MAXTRANS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS,
                         (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS );
    }
    /* BUG-35460 Add TABLE_TYPE G in SYS_TABLES_ */
    else if ( ( aCreateFlag & QDV_HIDDEN_INDEX_TABLE_MASK )
              == QDV_HIDDEN_INDEX_TABLE_TRUE )
    {
        IDE_TEST( qcmTablespace::getTBSAttrByID( aTBSID,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
    
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'G', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'%s',"
                         "CHAR'%s',"
                         "CHAR'Y',"
                         "CHAR'W',"
                         "INTEGER'1', "
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'0', "                   // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) aTBSID,
                         sTBSName,
                         aSegAttr.mPctFree,
                         aSegAttr.mPctUsed,
                         aSegAttr.mInitTrans,
                         aSegAttr.mMaxTrans,
                         (ULong)aSegStoAttr.mInitExtCnt,
                         (ULong)aSegStoAttr.mNextExtCnt,
                         (ULong)aSegStoAttr.mMinExtCnt,
                         (ULong)aSegStoAttr.mMaxExtCnt,
                         sIsPartitioned,
                         sTemporary );
    }
    /* BUG-37144 Add TABLE_TYPE D in SYS_TABLES_ */
    else if( ( aCreateFlag & QDV_HIDDEN_DICTIONARY_TABLE_MASK )
                == QDV_HIDDEN_DICTIONARY_TABLE_TRUE )
    {
        IDE_TEST( qcmTablespace::getTBSAttrByID( aTBSID,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
    
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'D', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'%s',"
                         "CHAR'%s',"
                         "CHAR'N',"
                         "CHAR'W',"
                         "INTEGER'1',"
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'0', "                   // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) aTBSID,
                         sTBSName,
                         aSegAttr.mPctFree,
                         aSegAttr.mPctUsed,
                         aSegAttr.mInitTrans,
                         aSegAttr.mMaxTrans,
                         (ULong)aSegStoAttr.mInitExtCnt,
                         (ULong)aSegStoAttr.mNextExtCnt,
                         (ULong)aSegStoAttr.mMinExtCnt,
                         (ULong)aSegStoAttr.mMaxExtCnt,
                         sIsPartitioned,
                         sTemporary );
    }
    /* PROJ-2365 sequence table */
    else if( ( aCreateFlag & QDV_HIDDEN_SEQUENCE_TABLE_MASK )
             == QDV_HIDDEN_SEQUENCE_TABLE_TRUE )
    {
        IDE_TEST( qcmTablespace::getTBSAttrByID( aTBSID,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
    
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'E', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'%s',"
                         "CHAR'%s',"
                         "CHAR'N',"
                         "CHAR'W',"
                         "INTEGER'1',"
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'0', "                   // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) aTBSID,
                         sTBSName,
                         aSegAttr.mPctFree,
                         aSegAttr.mPctUsed,
                         aSegAttr.mInitTrans,
                         aSegAttr.mMaxTrans,
                         (ULong)aSegStoAttr.mInitExtCnt,
                         (ULong)aSegStoAttr.mNextExtCnt,
                         (ULong)aSegStoAttr.mMinExtCnt,
                         (ULong)aSegStoAttr.mMaxExtCnt,
                         sIsPartitioned,
                         sTemporary );
    }
    else
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        // TABLE일 경우 파티션드 테이블 여부에 따라서
        // IS_PARTITIONED 컬럼 값을 준다.

        // CREATE TABLE
        IDE_TEST( qcmTablespace::getTBSAttrByID( aTBSID,
                                                 & sTBSAttr )
                  != IDE_SUCCESS );
        idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
        sTBSName[sTBSAttr.mNameLength] = '\0';
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TABLES_ VALUES( "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', CHAR'T', "
                         "INTEGER'0', "
                         "INTEGER'0', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "VARCHAR'%s', "                  // 11 TBS_NAME                         
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "INTEGER'%"ID_INT32_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "BIGINT'%"ID_INT64_FMT"', "
                         "CHAR'%s',"
                         "CHAR'%s',"
                         "CHAR'N',"
                         "CHAR'%s',"
                         "INTEGER'%"ID_INT32_FMT"', "     // PROJ-1071 Parallel query
                         "CHAR'Y',"                       // 25 USABLE
                         "INTEGER'%"ID_INT32_FMT"', "     // 26 SHARD_FLAG
                         "SYSDATE, SYSDATE )",
                         aUserID,
                         aTableID,
                         (ULong)aTableOID,
                         aColumnCount,
                         sTblName,
                         aMaxRows,
                         (mtdIntegerType) aTBSID,
                         sTBSName,
                         aSegAttr.mPctFree,
                         aSegAttr.mPctUsed,
                         aSegAttr.mInitTrans,
                         aSegAttr.mMaxTrans,
                         (ULong)aSegStoAttr.mInitExtCnt,
                         (ULong)aSegStoAttr.mNextExtCnt,
                         (ULong)aSegStoAttr.mMinExtCnt,
                         (ULong)aSegStoAttr.mMaxExtCnt,
                         sIsPartitioned,
                         sTemporary,
                         sAccessOptionStr,
                         aParallelDegree,
                         aShardFlag);
    }
    
    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// PROJ-2264 Dictionary table
IDE_RC qdbCommon::insertCompressionTableSpecIntoMeta(
    qcStatement     * aStatement,
    UInt              aTableID,
    UInt              aColumnID,
    UInt              aDicTableID,
    ULong             aMaxRows )
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_COMPRESSION_TABLES_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_COMPRESSION_TABLES_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

    SChar  * sSqlStr;
    vSLong   sRowCnt;

    IDU_LIMITPOINT("qdbCommon::insertCompressionTableSpecIntoMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_COMPRESSION_TABLES_ VALUES( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"' )",
                     aTableID,
                     aColumnID,
                     aDicTableID,
                     aMaxRows );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                &sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::insertPartTableSpecIntoMeta(
    qcStatement        * aStatement,
    UInt                 aUserID,
    UInt                 aTableID,
    UInt                 aPartMethod,
    UInt                 aPartKeyCount,
    idBool               aIsRowmovement )
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_PART_TABLES_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_PART_TABLES_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

    SChar               * sSqlStr;
    vSLong                sRowCnt;
    SChar                 sIsRowmovement[2];

    if( aIsRowmovement == ID_TRUE )
    {
        sIsRowmovement[0] = 'T';
        sIsRowmovement[1] = 0;
    }
    else
    {
        sIsRowmovement[0] = 'F';
        sIsRowmovement[1] = 0;
    }

    IDU_LIMITPOINT("qdbCommon::insertPartTableSpecIntoMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_PART_TABLES_ VALUES( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "CHAR'%s' )",
                     aUserID,
                     aTableID,
                     aPartMethod,
                     aPartKeyCount,
                     sIsRowmovement );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::insertTablePartitionSpecIntoMeta(
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
    qcmAccessOption       aAccessOption,
    idBool                aIsUsable )
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_TABLE_PARTITIONS_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_TABLE_PARTITIONS_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

    SChar               * sSqlStr;
    SChar                 sPartName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar               * sPartValue;
    vSLong                sRowCnt;
    SChar               * sPartMinValueStr;
    SChar               * sPartMaxValueStr;
    SChar               * sPartOrderStr;
    SChar                 sAccessOptionStr[2];
    SChar                 sPartitionUsable[2];

    QC_STR_COPY( sPartName, aPartNamePos );

    IDU_LIMITPOINT("qdbCommon::insertTablePartitionSpecIntoMeta::malloc1");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::insertTablePartitionSpecIntoMeta::malloc2");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN+9, // VARCHAR' 포함
                                    &sPartMinValueStr )
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::insertTablePartitionSpecIntoMeta::malloc3");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN+9, // VARCHAR' 포함
                                    &sPartMaxValueStr )
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::insertTablePartitionSpecIntoMeta::malloc4");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sPartOrderStr )
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::insertTablePartitionSpecIntoMeta::malloc5");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN * 2,
                                    & sPartValue)
             != IDE_SUCCESS);

    // PARTITION_MIN_VALUE
    if( idlOS::strlen( aPartMinValue ) == 0 )
    {
        idlOS::snprintf( sPartMinValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN+9,
                         "%s",
                         "NULL" );
    }
    else
    {
        // 문자열 안에 '가 있을 경우 '를 하나 더 넣어줘야함
        (void)addSingleQuote4PartValue( aPartMinValue,
                                        idlOS::strlen( aPartMinValue ),
                                        &sPartValue );

        idlOS::snprintf( sPartMinValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN+9,
                         "VARCHAR'%s'",
                         sPartValue );
    }

    // PARTITION_MAX_VALUE
    if( idlOS::strlen( aPartMaxValue ) == 0 )
    {
        idlOS::snprintf( sPartMaxValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN+9,
                         "%s",
                         "NULL" );
    }
    else
    {
        // 문자열 안에 '가 있을 경우 '를 하나 더 넣어줘야함
        (void)addSingleQuote4PartValue( aPartMaxValue,
                                        idlOS::strlen( aPartMaxValue ),
                                        &sPartValue );

        idlOS::snprintf( sPartMaxValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN+9,
                         "VARCHAR'%s'",
                         sPartValue );
    }

    if( aPartOrder == QDB_NO_PARTITION_ORDER )
    {
        idlOS::snprintf( sPartOrderStr,
                         QD_MAX_SQL_LENGTH,
                         "%s",
                         "NULL" );
    }
    else
    {
        idlOS::snprintf( sPartOrderStr,
                         QD_MAX_SQL_LENGTH,
                         "INTEGER'%"ID_INT32_FMT"'",
                         aPartOrder );
    }

    /* PROJ-2359 Table/Partition Access Option */
    switch ( aAccessOption )
    {
        case QCM_ACCESS_OPTION_READ_ONLY :
            sAccessOptionStr[0] = 'R';
            sAccessOptionStr[1] = '\0';
            break;

        case QCM_ACCESS_OPTION_READ_WRITE :
        case QCM_ACCESS_OPTION_NONE :
            sAccessOptionStr[0] = 'W';
            sAccessOptionStr[1] = '\0';
            break;

        case QCM_ACCESS_OPTION_READ_APPEND :
            sAccessOptionStr[0] = 'A';
            sAccessOptionStr[1] = '\0';
            break;

        default :
            IDE_ASSERT( 0 );
    }

    /* PROJ-2757 Advanced Global DDL */
    if ( aIsUsable == ID_TRUE )
    {
        sPartitionUsable[0] = 'Y';
        sPartitionUsable[1] = '\0';
    }
    else
    {
        sPartitionUsable[0] = 'N';
        sPartitionUsable[1] = '\0';
    }

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_TABLE_PARTITIONS_ VALUES( "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "VARCHAR'%s', "
                     "%s, "
                     "%s, "
                     "%s, "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "CHAR'%s', "          /* PROJ-2359 Table/Partition Access Option */
                     "CHAR'%s', "           /* TASK-7307 DML Data Consistency in Shard */
                     "INTEGER'0', "
                     "INTEGER'0', "
                     "SYSDATE, "
                     "SYSDATE )",
                     aUserID,
                     aTableID,
                     (ULong) aPartOID,
                     aPartID,
                     sPartName,
                     sPartMinValueStr,
                     sPartMaxValueStr,
                     sPartOrderStr,
                     (mtdIntegerType) aTBSID,
                     sAccessOptionStr,    /* PROJ-2359 Table/Partition Access Option */
                     sPartitionUsable );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::insertPartLobSpecIntoMeta(
    qcStatement          * aStatement,
    UInt                   aUserID,
    UInt                   aTableID,
    UInt                   aPartID,
    qcmColumn            * aColumn)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_PART_LOBS_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_PART_LOBS_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

    SChar               * sSqlStr;
    vSLong                sRowCnt;
    qcmColumn           * sTableColumn;
    UInt                  sTableColumnID;
    SChar               * sTrueFalse[2] = {(SChar*)"T", (SChar*)"F"};
    SChar               * sLogging;
    SChar               * sBuffer;

    IDU_LIMITPOINT("qdbCommon::insertPartLobSpecIntoMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    sTableColumnID  = aTableID * SMI_COLUMN_ID_MAXIMUM;

    for( sTableColumn = aColumn;
         sTableColumn != NULL;
         sTableColumn = sTableColumn->next )
    {
        if( ( sTableColumn->basicInfo->module->flag
              & MTD_COLUMN_TYPE_MASK ) ==
            MTD_COLUMN_TYPE_LOB )
        {
            if( ( sTableColumn->basicInfo->column.flag
                  & SMI_COLUMN_LOGGING_MASK) ==
                SMI_COLUMN_LOGGING )
            {
                sLogging = sTrueFalse[0];
            }
            else
            {
                sLogging = sTrueFalse[1];
            }

            if( ( sTableColumn->basicInfo->column.flag
                  & SMI_COLUMN_USE_BUFFER_MASK) ==
                SMI_COLUMN_USE_BUFFER )
            {
                sBuffer = sTrueFalse[0];
            }
            else
            {
                sBuffer = sTrueFalse[1];
            }

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_PART_LOBS_ VALUES( "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "CHAR'%s', "
                             "CHAR'%s' )",
                             aUserID,
                             aTableID,
                             aPartID,
                             sTableColumnID,
                             (mtdIntegerType)
                             sTableColumn->basicInfo->column.colSpace,
                             sLogging,
                             sBuffer );

            IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                        sSqlStr,
                                        & sRowCnt ) != IDE_SUCCESS);

            IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
        }

        sTableColumnID++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::insertColumnSpecIntoMeta(
    qcStatement     * aStatement,
    UInt              aUserID,
    UInt              aTableID,
    qcmColumn       * aColumns,
    idBool            aIsQueue)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_COLUMNS_ 메타 테이블에 입력 수행
 *      ALTER TABLE MODIFY COLUMN 관련 SYS_COLUMNS_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_COLUMNS_ 입력 정보 수집 ( NOTNULL, ISVARIABLE, DEFAULTVAL )
 *      2. SYS_COLUMNS_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

    SChar     * sSqlStr;
    SInt        i;
    SInt        j;
    qcmColumn * sColumn;
    SChar     * sTrueFalse[2] = {(SChar*)"T", (SChar*)"F"};
    SChar     * sStoreTypeStr[4] = {(SChar*)"F", (SChar*)"V", (SChar*)"V", (SChar*)"L"};
    SChar       sStoreTypeStrNull[1] = "";
    SChar       sDefaultValueStrNull[1] = "";
    SChar     * sDefaultValueStr;
    SChar     * sIsNullable;
    SChar     * sHidden;
    SChar     * sKeyPrev;
    SChar     * sStoreType;
    SChar     * sLogging;
    SChar     * sBuffer;
    SChar     * sColNamePtr;
    SChar       sColName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar     * sDefaultSysdateStr = (SChar*)"SYSDATE";
    UChar       sPolicyCode[ QCS_POLICY_CODE_SIZE + 1 ];
    UShort      sPolicyCodeSize;
    vSLong      sRowCnt;
    UInt        sSize = 0;

    // PROJ-1579 NCHAR
    UInt            sBufferSize = 0;
    UInt            sAddSize = 0;
    SChar         * sDefValBuffer = NULL;
    qcNamePosList * sTempNamePosList = NULL;
    qcNamePosition  sNamePos;

    sColumn = aColumns;
    i = aTableID * SMI_COLUMN_ID_MAXIMUM;
    j = 0;

    while (sColumn != NULL)
    {
        if ((sColumn->basicInfo->flag & MTC_COLUMN_NOTNULL_MASK) ==
            MTC_COLUMN_NOTNULL_FALSE)
        {
            sIsNullable = sTrueFalse[0];
        }
        else
        {
            sIsNullable = sTrueFalse[1];
        }

        /* PROJ-1090 Function-based Index */
        if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
        {
            sHidden = sTrueFalse[0];
        }
        else
        {
            sHidden = sTrueFalse[1];
        }

        // PROJ-2204 Join Update, Delete
        if ( (sColumn->basicInfo->flag & MTC_COLUMN_KEY_PRESERVED_MASK)
             == MTC_COLUMN_KEY_PRESERVED_TRUE )
        {
            sKeyPrev = sTrueFalse[0];
        }
        else
        {
            sKeyPrev = sTrueFalse[1];
        }

        if( ( sColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
            == SMI_COLUMN_STORAGE_MEMORY )
        {
            // MEMORY
            switch (sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK)
            {
                case SMI_COLUMN_TYPE_FIXED:
                    sStoreType = sStoreTypeStr[0];
                    break;
                case SMI_COLUMN_TYPE_VARIABLE:
                    sStoreType = sStoreTypeStr[1];
                    break;
                case SMI_COLUMN_TYPE_VARIABLE_LARGE:
                    sStoreType = sStoreTypeStr[2];
                    break;
                case SMI_COLUMN_TYPE_LOB:
                    sStoreType = sStoreTypeStr[3];
                    break;
                default:
                    IDE_ASSERT(0);
                    break;
            }
        }
        else
        {
            // DISK

            // BUG-25728 disk column의 저장 타입?에 대한 정의 필요.
            // fixed, variable은 메모리 컬럼에 대해서 적용되는 저장옵션이며,
            // 디스크테이블은 이 저장옵션과 무관하므로 이 컬럼속성을 표기하지 않는다.
            // system_.sys_columns_.STORE_TYPE에 NULL데이타 입력.
            switch (sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK)
            {
                case SMI_COLUMN_TYPE_FIXED:
                    sStoreType = sStoreTypeStrNull;
                    break;
                case SMI_COLUMN_TYPE_LOB:
                    sStoreType = sStoreTypeStr[3];
                    break;
                default:
                    IDE_ASSERT(0);
                    break;
            }
        }

        // To fix BUG-15386
        // enqueue_time? default? sysdate?.
        if( ( aIsQueue == ID_TRUE ) &&
            ( idlOS::strMatch( "ENQUEUE_TIME", 12, sColumn->name, 12) == 0 ) )
        {
            sDefaultValueStr = sDefaultSysdateStr;
        }
        else
        {
            if (sColumn->defaultValue == NULL)
            {
                sDefaultValueStr = sDefaultValueStrNull;
            }
            else
            {
                // PROJ-1579 NCHAR
                // 메타테이블에 저장하기 위해 스트링을 변환하기 전에
                // N 타입이 있는 경우 U 타입으로 변환한다.
                for( sTempNamePosList = sColumn->ncharLiteralPos;
                     sTempNamePosList != NULL;
                     sTempNamePosList = sTempNamePosList->next )
                {
                    sNamePos = sTempNamePosList->namePos;

                    // U 타입으로 변환하면서 늘어나는 사이즈 계산
                    // N'안' => U'\C548' 으로 변환된다면
                    // '안'의 캐릭터 셋이 KSC5601이라고 가정했을 때,
                    // single-quote안의 문자는 2 byte -> 5byte로 변경된다.
                    // 즉, 1.5배가 늘어나는 것이다.
                    //(전체 사이즈가 아니라 증가하는 사이즈만 계산하는 것임)
                    // 하지만, 어떤 예외적인 캐릭터 셋이 들어올지 모르므로
                    // * 2로 충분히 잡는다.
                    sAddSize += (sNamePos.size - 3) * 2;
                }

                // in case of CREATE TABLE
                if ( sColumn->defaultValueStr == NULL )
                {
                    if( sColumn->ncharLiteralPos != NULL )
                    {
                        sBufferSize = sColumn->defaultValue->position.size +
                                      sAddSize;

                        IDU_LIMITPOINT("qdbCommon::insertColumnSpecIntoMeta::malloc1");
                        IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                                        SChar,
                                                        sBufferSize,
                                                        & sDefValBuffer)
                                 != IDE_SUCCESS);

                        IDE_TEST( qdbCommon::convertToUTypeString(
                                        aStatement,
                                        sColumn->defaultValue->position.offset,
                                        sColumn->defaultValue->position.size,
                                        sColumn->ncharLiteralPos,
                                        sDefValBuffer,
                                        sBufferSize )
                                  != IDE_SUCCESS );

                        IDE_TEST(getStrForMeta(
                                     aStatement,
                                     sDefValBuffer,
                                     idlOS::strlen( sDefValBuffer ),
                                     &sDefaultValueStr)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        // BUG-45633 default "seq 1".nextval 오면 " 생략되는 문제.
                        if ( *(sColumn->defaultValue->position.stmtText +
                               sColumn->defaultValue->position.offset-1)  == '"' )
                        {
                            IDE_TEST(getStrForMeta(
                                         aStatement,
                                         sColumn->defaultValue->position.stmtText+
                                         sColumn->defaultValue->position.offset-1,
                                         sColumn->defaultValue->position.size+1,
                                         &sDefaultValueStr)
                                     != IDE_SUCCESS);   
                        }
                        else
                        {
                            IDE_TEST(getStrForMeta(
                                         aStatement,
                                         sColumn->defaultValue->position.stmtText+
                                         sColumn->defaultValue->position.offset,
                                         sColumn->defaultValue->position.size,
                                         &sDefaultValueStr)
                                     != IDE_SUCCESS);
                        }
                    }
                }
                // in case of ALTER TABLE ADD COLUMN
                else
                {
                    sSize = idlOS::strlen((SChar*)sColumn->defaultValueStr);

                    if( sColumn->ncharLiteralPos != NULL )
                    {
                        sBufferSize = sSize + sAddSize;

                        IDU_LIMITPOINT("qdbCommon::insertColumnSpecIntoMeta::malloc2");
                        IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                                        SChar,
                                                        sSize + sAddSize,
                                                        & sDefValBuffer)
                                 != IDE_SUCCESS);

                        IDE_TEST( qdbCommon::convertToUTypeString(
                                      aStatement,
                                      sColumn->defaultValue->position.offset,
                                      sSize,
                                      sColumn->ncharLiteralPos,
                                      sDefValBuffer,
                                      sBufferSize )
                                  != IDE_SUCCESS );

                        IDE_TEST( getStrForMeta(
                                      aStatement,
                                      sDefValBuffer,
                                      idlOS::strlen( sDefValBuffer ),
                                      &sDefaultValueStr)
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        IDE_TEST( getStrForMeta(
                                      aStatement,
                                      (SChar*) sColumn->defaultValueStr,
                                      sSize,
                                      &sDefaultValueStr)
                                  != IDE_SUCCESS );
                    }
                }
            }
        }

        if (sColumn->namePos.size == 0)
        {
            sColNamePtr = sColumn->name;
        }
        else
        {
            QC_STR_COPY( sColName, sColumn->namePos );

            sColNamePtr = sColName;
        }

        IDU_LIMITPOINT("qdbCommon::insertColumnSpecIntoMeta::malloc3");
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          &sSqlStr )
                  != IDE_SUCCESS);

        // to fix BUG-12576
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_COLUMNS_ VALUES ( "
                         "INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', BIGINT'%"ID_INT64_FMT    // PROJ-1362
                         "', BIGINT'%"ID_INT64_FMT    // PROJ-1362
                         "', INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', VARCHAR'%s', '%s', VARCHAR'%s', '%s'"
                         ", INTEGER'%"ID_INT32_FMT
                         "', INTEGER'%"ID_INT32_FMT
                         "', '%s'"          /* PROJ-1090 Function-based Index */
                         ", '%s' )",        /* PROJ-2204 Join Update, Delete */
                         i,
                         (SInt)sColumn->basicInfo->type.dataTypeId,
                         (SInt)sColumn->basicInfo->type.languageId,
                         (SLong)sColumn->basicInfo->column.offset,
                         (SLong)sColumn->basicInfo->column.size,
                         aUserID,
                         aTableID,
                         sColumn->basicInfo->precision,
                         sColumn->basicInfo->scale,
                         j,
                         sColNamePtr,
                         sIsNullable,
                         sDefaultValueStr,
                         sStoreType,
                         // PROJ-1557
                         sColumn->basicInfo->column.vcInOutBaseSize,
                         0,
                         sHidden,     /* PROJ-1090 Function-based Index */
                         sKeyPrev );  /* PROJ-2204 Join Update, Delete */

        IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                    sSqlStr,
                                    & sRowCnt ) != IDE_SUCCESS);

        IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

        // PROJ-1362
        if ((sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK) ==
            MTD_COLUMN_TYPE_LOB)
        {
            if ((sColumn->basicInfo->column.flag & SMI_COLUMN_LOGGING_MASK) ==
                SMI_COLUMN_LOGGING)
            {
                sLogging = sTrueFalse[0];
            }
            else
            {
                sLogging = sTrueFalse[1];
            }

            if ((sColumn->basicInfo->column.flag & SMI_COLUMN_USE_BUFFER_MASK) ==
                SMI_COLUMN_USE_BUFFER)
            {
                sBuffer = sTrueFalse[0];
            }
            else
            {
                sBuffer = sTrueFalse[1];
            }

            if( ( sColumn->flag & QCM_COLUMN_LOB_DEFAULT_TBS_MASK ) ==
                QCM_COLUMN_LOB_DEFAULT_TBS_TRUE )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_LOBS_ VALUES ( "
                                 "INTEGER'%"ID_INT32_FMT
                                 "', INTEGER'%"ID_INT32_FMT
                                 "', INTEGER'%"ID_INT32_FMT
                                 "', INTEGER'%"ID_INT32_FMT
                                 "', CHAR'%s', CHAR'%s', CHAR'%s')",
                                 aUserID,
                                 aTableID,
                                 i,
                                 (mtdIntegerType)
                                 sColumn->basicInfo->column.colSpace,
                                 sLogging,
                                 sBuffer,
                                 (SChar*)"T" );
            }
            else
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_LOBS_ VALUES ( "
                                 "INTEGER'%"ID_INT32_FMT
                                 "', INTEGER'%"ID_INT32_FMT
                                 "', INTEGER'%"ID_INT32_FMT
                                 "', INTEGER'%"ID_INT32_FMT
                                 "', CHAR'%s', CHAR'%s', CHAR'%s')",
                                 aUserID,
                                 aTableID,
                                 i,
                                 (mtdIntegerType)
                                 sColumn->basicInfo->column.colSpace,
                                 sLogging,
                                 sBuffer,
                                 (SChar*)"F" );
            }

            IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT(aStatement),
                                       sSqlStr,
                                       & sRowCnt ) != IDE_SUCCESS);

            IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2002 Column Security
        if ( (sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK) ==
             MTD_ENCRYPT_TYPE_TRUE )
        {
            IDE_TEST( qcsModule::getPolicyCode( sColumn->basicInfo->mColumnAttr.mEncAttr.mPolicy,
                                                sPolicyCode,
                                                & sPolicyCodeSize )
                      != IDE_SUCCESS );
            sPolicyCode[sPolicyCodeSize] = '\0';

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_ENCRYPTED_COLUMNS_ VALUES ( "
                             "INTEGER'%"ID_INT32_FMT
                             "', INTEGER'%"ID_INT32_FMT
                             "', INTEGER'%"ID_INT32_FMT
                             "', INTEGER'%"ID_INT32_FMT
                             "' , VARCHAR'%s', VARCHAR'%s')",
                             aUserID,
                             aTableID,
                             i,
                             sColumn->basicInfo->mColumnAttr.mEncAttr.mEncPrecision,
                             sColumn->basicInfo->mColumnAttr.mEncAttr.mPolicy,
                             (SChar*) sPolicyCode );

            IDE_TEST(qcg::runDMLforDDL(QC_SMI_STMT(aStatement),
                                       sSqlStr,
                                       & sRowCnt ) != IDE_SUCCESS);

            IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2422 srid
        if ( sColumn->basicInfo->module->id == MTD_GEOMETRY_ID )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_GEOMETRIES_ VALUES ( "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'%"ID_INT32_FMT"', "
                             "INTEGER'2', "  // COORD_DIMENSION
                             "INTEGER'%"ID_INT32_FMT"')",
                             aUserID,
                             aTableID,
                             i,
                             (mtdIntegerType)
                             sColumn->basicInfo->mColumnAttr.mSridAttr.mSrid );

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         &sRowCnt ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
        }
        else
        {
            // Nothing to do.
        }

        i++;
        j++;

        sColumn = sColumn->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::insertPartKeyColumnSpecIntoMeta(
    qcStatement     * aStatement,
    UInt              aUserID,
    UInt              aTableID,
    qcmColumn       * aTableColumns,
    qcmColumn       * aPartKeyColumns,
    UInt              aObjectType)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 관련 SYS_PART_KEY_COLUMNS_ 메타 테이블에 입력 수행
 *
 * Implementation :
 *      1. SYS_PART_KEY_COLUMNS_ 메타 테이블로 정보 입력
 *
 ***********************************************************************/

    SChar               * sSqlStr;
    qcmColumn           * sPartKeyColumn;
    qcmColumn           * sTableColumn;
    UInt                  sTableColumnID;
    UInt                  sPartKeyColumnOrder;
    vSLong                sRowCnt;
    idBool                sIsNameMatched = ID_FALSE;


    IDU_LIMITPOINT("qdbCommon::insertPartKeyColumnSpecIntoMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    sPartKeyColumnOrder = 0;

    for( sPartKeyColumn = aPartKeyColumns;
         sPartKeyColumn != NULL;
         sPartKeyColumn = sPartKeyColumn->next )
    {
        sTableColumnID  = aTableID * SMI_COLUMN_ID_MAXIMUM;

        for( sTableColumn = aTableColumns;
             sTableColumn != NULL;
             sTableColumn = sTableColumn->next )
        {
            sIsNameMatched = ID_FALSE;

            if (sPartKeyColumn->namePos.size == 0)
            {
                if ( idlOS::strMatch( sTableColumn->name,
                                      idlOS::strlen( sTableColumn->name ),
                                      sPartKeyColumn->name,
                                      idlOS::strlen( sPartKeyColumn->name ) ) == 0 )
                {
                    sIsNameMatched = ID_TRUE;
                }
            }
            // CRETE TABLE AS SELECT
            else if( (sTableColumn->namePos.size == 0) &&
                     (sPartKeyColumn->namePos.size != 0) )
            {
                if( idlOS::strMatch( sPartKeyColumn->namePos.stmtText +
                                     sPartKeyColumn->namePos.offset,
                                     sPartKeyColumn->namePos.size,
                                     sTableColumn->name,
                                     idlOS::strlen(sTableColumn->name) ) == 0 )
                {
                    sIsNameMatched = ID_TRUE;
                }
            }
            // CREATE TABLE
            else
            {
                if ( QC_IS_NAME_MATCHED( sTableColumn->namePos, sPartKeyColumn->namePos ) )
                {
                    sIsNameMatched = ID_TRUE;
                }
            }

            if( sIsNameMatched == ID_TRUE )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_PART_KEY_COLUMNS_ VALUES( "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"', "
                                 "INTEGER'%"ID_INT32_FMT"' )",
                                 aUserID,
                                 aTableID,
                                 sTableColumnID,
                                 aObjectType,
                                 sPartKeyColumnOrder );

                IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                            sSqlStr,
                                            & sRowCnt ) != IDE_SUCCESS);

                IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);
            }

            sTableColumnID++;
        }

        sPartKeyColumnOrder++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::allocSmiColList(qcStatement * aStatement,
                                  const void  * aTableHandle,
                                  smiColumnList **aColList)
{
#define IDE_FN "qdbCommon::allocSmiColList"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SInt i;
    SInt sColCount;
    mtcColumn *sTableColumn;
    mtcColumn *sColumn;
    smiColumnList  *sColumnList;

    sColCount = smiGetTableColumnCount(aTableHandle);

    IDU_LIMITPOINT("qdbCommon::allocSmiColList::malloc1");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiColumnList) * sColCount,
                                       (void**)aColList)
             != IDE_SUCCESS);

    sColumnList = *aColList;

    for (i = 0; i < sColCount; i++)
    {
        IDE_TEST( smiGetTableColumns( aTableHandle,
                                      i ,
                                      (const smiColumn**)&sTableColumn )
                  != IDE_SUCCESS );

        IDU_LIMITPOINT("qdbCommon::allocSmiColList::malloc2");
        IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(mtcColumn), (void**)&sColumn)
                 != IDE_SUCCESS);

        idlOS::memcpy(sColumn, sTableColumn, ID_SIZEOF(mtcColumn));
        sColumnList[i].column = (const smiColumn*)sColumn;
        if (i == sColCount -1)
        {
            sColumnList[i].next = NULL;
        }
        else
        {
            sColumnList[i].next = &sColumnList[i+1];
        }
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC qdbCommon::createConstrPrimaryUnique(
    qcStatement             * aStatement,
    smOID                     aTableOID,
    qdConstraintSpec        * aConstr,
    UInt                      aUserID,
    UInt                      aTableID,
    qcmPartitionInfoList    * aPartInfoList,
    ULong                     aMaxRows )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TABLE
 *    ALTER TABLE ... ADD COLUMN
 *    ALTER TABLE ... ADD CONSTRAINT 로부터 호출,
 *    추가되는 컬럼에 primary key 또는 unique constraint 가 있을 경우 수행됨
 *
 * Implementation :
 *    1. 테이블 핸들 구하기
 *    2. 디폴트 인덱스 타입 구하기
 *    3. 추가될 인덱스를 위한 ID 부여
 *    4. constraint 를 생성할 컬럼을 가지고 sColumnList 재구성
 *    5. constraint 이름 부여
 *    6. primary key 부여일 경우 널 값을 가진 row 가 있는지 검사
 *    7. 인덱스 생성 : smiTable::createIndex
 *    8. 인덱스 관련 메타 테이블에 인덱스 정보 입력
 *    9. primary key 부여일 경우 컬럼 관련 메타 테이블에 NOT NULL 정보 변경
 *    10. constraint 관련 메타 테이블에 constraint 정보 입력
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::createConstrPrimaryUnique"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcmColumn             * sColumn;
    smiColumnList           sColumnListAtRow[QC_MAX_KEY_COLUMN_COUNT];
    smiColumnList           sColumnListAtKey[QC_MAX_KEY_COLUMN_COUNT];
    mtcColumn               sColumnAtKey[QC_MAX_KEY_COLUMN_COUNT];
    SChar                   sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    UInt                    i;
    UInt                    sOffset;
    UInt                    sIndexID;
    UInt                    sIndexPartID;
    UInt                    sConstrID;
    UInt                    sIndexType;
    const void            * sNewTableHandle;
    const void            * sIndexHandle;
    UInt                    sReferencedTableID;
    UInt                    sReferencedIndexID;
    UInt                    sReferenceRule;  // PROJ-1509
    UInt                    sIndexFlag = 0;
    UInt                    sBuildFlag = 0;

    idBool                  sIsPartitionedTable = ID_FALSE;
    idBool                  sIsPartitionedIndex = ID_FALSE;
    qdTableParseTree      * sParseTree;
    qcmTableInfo          * sPartInfo = NULL;
    void                  * sPartHandle;
    smSCN                   sPartSCN;
    SChar                   sIndexPartName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    idBool                  sUnique = ID_FALSE;
    qdPartitionAttribute  * sPartAttr;
    UInt                    sTablePartCount = 0;
    UInt                    sIndexPartCount = 0;
    UInt                    sTableFlag;
    UInt                    sTableParallelDegree;
    qcmColumn             * sIndexTableColumns;
    UInt                    sIndexTableColumnCount;
    qcNamePosition          sIndexTableNamePos;
    qdIndexTableList      * sIndexTable;
    qcmTableInfo          * sIndexTableInfo;
    idBool                  sIsDiskPartitionedTable;
    idBool                  sIsDiskIndex;
    UInt                    sLocalIndexType = 0;
    idBool                  sIsLocalIndex;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStoAttr;
    UInt                    sNewIndexFlag;
    ULong                   sDirectKeyMaxSize;
    SInt                    sCountMemType;
    SInt                    sCountVolType;
    UInt                    sTableType;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    // PROJ-1502 PARTITIONED DISK TABLE
    // CREATE TABLE
    if( sParseTree->tableInfo == NULL )
    {
        if( sParseTree->partTable == NULL )
        {
            sIsPartitionedTable = ID_FALSE;
        }
        else
        {
            sIsPartitionedTable = ID_TRUE;
        }

        /* PROJ-2464 hybrid partitioned table 지원 */
        sTableType = getTableTypeFromTBSID( sParseTree->TBSAttr.mID );
    }
    // ALTER TABLE
    else
    {
        if( sParseTree->tableInfo->tablePartitionType ==
            QCM_NONE_PARTITIONED_TABLE )
        {
            sIsPartitionedTable = ID_FALSE;
        }
        else
        {
            /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
            IDE_TEST_RAISE( sParseTree->partTable == NULL, ERR_UNEXPECTED );

            sIsPartitionedTable = ID_TRUE;
        }

        /* PROJ-2464 hybrid partitioned table 지원 */
        sTableType = sParseTree->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK;
    }

    /* PROJ-2334 PMT */
    if ( sParseTree->tableInfo != NULL )
    {
        /* alter add constraints */
        sIsDiskPartitionedTable = smiTableSpace::isDiskTableSpaceType( sParseTree->tableInfo->TBSType );
    }
    else
    {
        /* create table constraints */
        sIsDiskPartitionedTable = smiTableSpace::isDiskTableSpaceType( sParseTree->TBSAttr.mType );
    }

    if ( ( aConstr->constrType == QD_PRIMARYKEY ) ||
         ( aConstr->constrType == QD_UNIQUE )     ||
         ( aConstr->constrType == QD_LOCAL_UNIQUE ) )
    {
        sNewTableHandle = smiGetTable( aTableOID );
        sIndexType = smiGetDefaultIndexType();

        sColumn = aConstr->constraintColumns;
        IDE_TEST(qcm::getNextIndexID(aStatement, &sIndexID)
                 != IDE_SUCCESS);
        for (i = 0; i < QC_MAX_KEY_COLUMN_COUNT; i++)
        {
            sColumnListAtRow[i].next = NULL;
            sColumnListAtRow[i].column = NULL;
            sColumnListAtKey[i].next = NULL;
            sColumnListAtKey[i].column = NULL;
        }

        i = 0;
        sOffset = 0;
        while (sColumn != NULL)
        {
            sColumnListAtRow[i].column = &(sColumn->basicInfo->column);
            idlOS::memcpy( &(sColumnAtKey[i]),
                           sColumn->basicInfo, ID_SIZEOF(mtcColumn) );
            if ( sIsDiskPartitionedTable == ID_TRUE )
            {
                // PROJ-1705
                IDE_TEST(
                    qdbCommon::setIndexKeyColumnTypeFlag( &(sColumnAtKey[i]) )
                    != IDE_SUCCESS );

                // To Fix PR-8111
                if ( (sColumnAtKey[i].column.flag & SMI_COLUMN_TYPE_MASK)
                     == SMI_COLUMN_TYPE_FIXED )
                {
                    sOffset = idlOS::align( sOffset,
                                            sColumn->basicInfo->module->align );
                    sColumnAtKey[i].column.offset = sOffset;
                    sColumnAtKey[i].column.value = NULL;
                    sOffset += sColumn->basicInfo->column.size;
                }
                else
                {
                    sOffset = idlOS::align( sOffset, 8 );
                    sColumnAtKey[i].column.offset = sOffset;
                    sColumnAtKey[i].column.value = NULL;
                    sOffset += smiGetVariableColumnSize4DiskIndex();
                }
            }
            else
            {
                /* Nothing to do */
            }

            sColumnListAtKey[i].column = (smiColumn*) &(sColumnAtKey[i]);
            sColumn = sColumn->next;
            if (sColumn != NULL)
            {
                sColumnListAtRow[i].next = &sColumnListAtRow[i+1];
                sColumnListAtKey[i].next = &sColumnListAtKey[i+1];
            }
            else
            {
                sColumnListAtRow[i].next = NULL;
                sColumnListAtKey[i].next = NULL;
            }
            i++;
        }

        if (QC_IS_NULL_NAME(aConstr->constrName) == ID_TRUE)
        {
            idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                             "%sIDX_ID_%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sIndexID );
        }
        else
        {
            QC_STR_COPY( sObjName, aConstr->constrName );
        }

        if (aConstr->constrType == QD_PRIMARYKEY)
        {
            sIndexFlag = SMI_INDEX_UNIQUE_ENABLE |
                         SMI_INDEX_TYPE_PRIMARY_KEY |
                         SMI_INDEX_USE_ENABLE;
        }
        else
        {
            if( aConstr->constrType == QD_LOCAL_UNIQUE )
            {
                sIndexFlag = SMI_INDEX_LOCAL_UNIQUE_ENABLE|
                    SMI_INDEX_TYPE_NORMAL|
                    SMI_INDEX_USE_ENABLE;
            }
            else // QD_UNIQUE
            {
                sIndexFlag = SMI_INDEX_UNIQUE_ENABLE|
                    SMI_INDEX_TYPE_NORMAL|
                    SMI_INDEX_USE_ENABLE;
            }
        }

        /* PROJ-2433 Direct Key Index */
        if ( aConstr->mDirectKeyMaxSize != (ULong)(ID_ULONG_MAX) )
        {
            sIndexFlag &= ~(SMI_INDEX_DIRECTKEY_MASK);
            sIndexFlag |= SMI_INDEX_DIRECTKEY_TRUE;
        }
        else
        {
            sIndexFlag &= ~(SMI_INDEX_DIRECTKEY_MASK);
            sIndexFlag |= SMI_INDEX_DIRECTKEY_FALSE;
        }

        if (aConstr->isPers == ID_TRUE)
        {
            sIndexFlag &= ~(SMI_INDEX_PERSISTENT_MASK);
            sIndexFlag |=  SMI_INDEX_PERSISTENT_ENABLE;
        }
        else
        {
            sIndexFlag &= ~(SMI_INDEX_PERSISTENT_MASK);
            sIndexFlag |= SMI_INDEX_PERSISTENT_DISABLE;
        }

        // BUG-17848 : 영속적인 속성과 휘발성 속성 분리
        sBuildFlag |= ( (aConstr->buildFlag & SMI_INDEX_BUILD_LOGGING_MASK) |
                        (aConstr->buildFlag & SMI_INDEX_BUILD_FORCE_MASK) );

        sBuildFlag |= SMI_INDEX_BUILD_UNCOMMITTED_ROW_DISABLE;

        if( smiTable::createIndex(aStatement->mStatistics,
                                  QC_SMI_STMT( aStatement ),
                                  aConstr->indexTBSID,
                                  sNewTableHandle,
                                  (SChar*)sObjName,
                                  sIndexID,
                                  sIndexType,
                                  sColumnListAtKey,
                                  sIndexFlag,
                                  aConstr->parallelDegree,
                                  sBuildFlag,
                                  aConstr->segAttr,
                                  sParseTree->segStoAttr,
                                  aConstr->mDirectKeyMaxSize, /* PROJ-2433 */
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

        // PROJ-1502 PARTITIONED DISK TABLE
        if( sIsPartitionedTable == ID_TRUE )
        {
            sIsLocalIndex = ID_TRUE;
            // PROJ-1624 non-partitioned index
            // primary key는 replication에서 사용될 수 있고
            // replication은 각 partition의 primary key가 필요하므로
            // primary key는 non-partitioned index와 partitioned index를 동시에 생성한다.
            // 이때 primary key는 non-partitioned index이기도 하고
            // partitioned index이기도하기때문에 meta table의 상태가 애매해지게 되는데
            // meta상에는 non-partitioned table로 기록한다.

            /* PROJ-2461 pk,uk constraint에서 prefix index 제약 완화
             * local partitioned index인지, global non-partitioned index인지 분류.
             * sIsLocalIndex에 저장한다.
             * PK/UK에 한정하며 LOCALUNIQUE는 기본으로 Local index로 간다.
             */
            if ( ( aConstr->constrType == QD_PRIMARYKEY ) ||
                 ( aConstr->constrType == QD_UNIQUE ) )
            {
                if ( sParseTree->tableInfo != NULL )
                {
                    getTableTypeCountInPartInfoList( & sTableType,
                                                     sParseTree->partTable->partInfoList,
                                                     NULL,
                                                     & sCountMemType,
                                                     & sCountVolType );
                }
                else
                {
                    getTableTypeCountInPartAttrList( & sTableType,
                                                     sParseTree->partTable->partAttr,
                                                     NULL,
                                                     & sCountMemType,
                                                     & sCountVolType );
                }

                if ( ( sCountMemType + sCountVolType ) > 0 )
                {
                    sIsDiskIndex = ID_FALSE;
                }
                else
                {
                    sIsDiskIndex = ID_TRUE;
                }

                IDE_TEST( qdn::checkLocalIndex( aStatement,
                                                aConstr,
                                                sParseTree->tableInfo,
                                                &sIsLocalIndex,
                                                sIsDiskIndex )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing To Do */
            }

            /* PROJ-2461 pk,uk constraint에서 prefix index 제약 완화
             * non-partitioned index, local partitioned index 따라서 각각 인덱스 생성.
             * sIsLocalIndex == ID_FALSE 즉
             * Local Index가 아닌 경우, global non-partitioned index 생성한다.
             */
            if ( sIsLocalIndex != ID_TRUE )
            {
                /* --------------------------------
                 * (global) non-partitioned index
                 * -------------------------------- */

                // make index table name
                if ( QC_IS_NULL_NAME( aConstr->constrName ) == ID_TRUE )
                {
                    IDE_TEST( qdx::makeIndexTableName( aStatement,
                                                       aConstr->constrName,
                                                       sObjName,
                                                       aConstr->indexTableName,
                                                       aConstr->keyIndexName,
                                                       aConstr->ridIndexName )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                sIndexTableNamePos.stmtText = aConstr->indexTableName;
                sIndexTableNamePos.offset   = 0;
                sIndexTableNamePos.size     = idlOS::strlen( aConstr->indexTableName );

                // index table columns 생성
                IDE_TEST( qdx::makeColumns4CreateIndexTable( aStatement,
                                                             aConstr->constraintColumns,
                                                             aConstr->constrColumnCount,
                                                             & sIndexTableColumns,
                                                             & sIndexTableColumnCount )
                          != IDE_SUCCESS );

                // index table columns 검사
                IDE_TEST( qdbCommon::validateColumnListForCreateInternalTable(
                              aStatement,
                              ID_TRUE,  // in execution time
                              SMI_TABLE_DISK,
                              aConstr->indexTBSID,
                              sIndexTableColumns )
                          != IDE_SUCCESS );
            
                // create index table
                if( sParseTree->tableInfo == NULL )
                {
                    IDE_TEST( qdx::createIndexTable(
                                  aStatement,
                                  aUserID,
                                  sIndexTableNamePos,
                                  sIndexTableColumns,
                                  sIndexTableColumnCount,
                                  aConstr->indexTBSID,
                                  sParseTree->segAttr,
                                  sParseTree->segStoAttr,
                                  sParseTree->tableAttrMask,
                                  sParseTree->tableAttrValue,
                                  sParseTree->parallelDegree,
                                  & sIndexTable )
                              != IDE_SUCCESS );

                    // link new index table
                    sIndexTable->next = sParseTree->newIndexTables;
                    sParseTree->newIndexTables = sIndexTable;

                    IDE_TEST( qdx::createIndexTableIndices(
                                  aStatement,
                                  aUserID,
                                  sIndexTable,
                                  NULL,
                                  aConstr->keyIndexName,
                                  aConstr->ridIndexName,
                                  aConstr->indexTBSID,
                                  sIndexType,
                                  sIndexFlag,
                                  aConstr->parallelDegree,
                                  sBuildFlag,
                                  aConstr->segAttr,
                                  sParseTree->segStoAttr,
                                  0 ) /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                              != IDE_SUCCESS );
                }
                else
                {
                    sTableFlag = sParseTree->tableInfo->tableFlag;
                    sTableParallelDegree = sParseTree->tableInfo->parallelDegree;
                
                    IDE_TEST( qdx::createIndexTable( aStatement,
                                                     aUserID,
                                                     sIndexTableNamePos,
                                                     sIndexTableColumns,
                                                     sIndexTableColumnCount,
                                                     aConstr->indexTBSID,
                                                     sParseTree->tableInfo->segAttr,
                                                     sParseTree->tableInfo->segStoAttr,
                                                     QDB_TABLE_ATTR_MASK_ALL,
                                                     sTableFlag, /* Flag Value */
                                                     sTableParallelDegree,
                                                     & sIndexTable )
                              != IDE_SUCCESS );
                
                    // link new index table
                    sIndexTable->next = sParseTree->newIndexTables;
                    sParseTree->newIndexTables = sIndexTable;

                    IDE_TEST( qdx::createIndexTableIndices(
                                  aStatement,
                                  aUserID,
                                  sIndexTable,
                                  NULL,
                                  aConstr->keyIndexName,
                                  aConstr->ridIndexName,
                                  aConstr->indexTBSID,
                                  sIndexType,
                                  sIndexFlag,
                                  sTableParallelDegree,
                                  sBuildFlag,
                                  aConstr->segAttr,
                                  sParseTree->segStoAttr,
                                  0 ) /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                              != IDE_SUCCESS );
                }
            
                // tableInfo 재생성
                sIndexTableInfo = sIndexTable->tableInfo;
                
                IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT(aStatement),
                                                     sIndexTable->tableID,
                                                     sIndexTable->tableOID)
                         != IDE_SUCCESS);
        
                IDE_TEST(qcm::getTableInfoByID(aStatement,
                                               sIndexTable->tableID,
                                               &(sIndexTable->tableInfo),
                                               &(sIndexTable->tableSCN),
                                               &(sIndexTable->tableHandle))
                         != IDE_SUCCESS);
        
                (void)qcm::destroyQcmTableInfo( sIndexTableInfo );
                    
                // index table id 설정
                aConstr->indexTableID = sIndexTable->tableID;
            }
            else
            {
                /* Nothing To Do */
            }

            /* sIsLocalIndex == ID_TRUE. 즉 Local partitioned index 생성하는 경우.
             * PK는 replication을 위해, (global) non-partitioned index를 생성할 때도
             * local partitioned index 생성 작업이 필요하다.
             */
            if ( ( sIsLocalIndex == ID_TRUE ) ||
                 ( aConstr->constrType == QD_PRIMARYKEY ) )
            {
                /* --------------------------------
                 * (local) partitioned index
                 * -------------------------------- */

                // 지정한 인덱스 파티션 개수
                for( sIndexPartCount = 0, sPartAttr = aConstr->partIndex->partAttr;
                     sPartAttr != NULL;
                     sIndexPartCount++, sPartAttr = sPartAttr->next ) ;

                // 테이블 파티션 개수
                IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                                           aTableID,
                                                           & sTablePartCount )
                          != IDE_SUCCESS );

                //------------------------------------------------------------
                // 테이블 파티션의 개수만큼 인덱스 파티션을 지정하지 않았으면,
                // 지정하지 않은 인덱스 파티션까지 구축해서 파스트리에 달아놓음
                //------------------------------------------------------------
                if( sIndexPartCount < sTablePartCount )
                {
                    // 각 인덱스 파티션을 위한 구조체를 생성한다.
                    IDE_TEST( qdx::makeIndexPartition( aStatement,
                                                       aPartInfoList,
                                                       aConstr->partIndex )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do
                }

                //------------------------------------------------------------
                // create index on each partition
                // SYS_PART_INDICES_ INDEX_TYPE는 GLOBAL(1)지원 하지 않는다
                // 항상 LOCAL(0)이다.
                //------------------------------------------------------------

                IDE_TEST( qdx::insertPartIndexIntoMeta( aStatement,
                                                        aUserID,
                                                        aTableID,
                                                        sIndexID,
                                                        sLocalIndexType,
                                                        sIndexFlag )
                         != IDE_SUCCESS );

                /* BUG-45620  Index 생성 시, SYS_PART_KEY_COLUMNS_에 Index Column 정보를 누락되거나
                 *  잘못된 Index Column 정보를 추가할 수 있습니다. */
                if ( sParseTree->tableInfo != NULL )
                {
                    for ( sColumn  = aConstr->constraintColumns;
                          sColumn != NULL;
                          sColumn  = sColumn->next )
                    {
                        IDE_TEST( qdx::insertIndexPartKeyColumnIntoMeta( aStatement,
                                                                         aUserID,
                                                                         sIndexID,
                                                                         sColumn->basicInfo->column.id,
                                                                         sParseTree->tableInfo,
                                                                         QCM_INDEX_OBJECT_TYPE )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    /* CREATE TABLE 시 상위에서 qdbCommon::insertPartKeyColumnSpecIntoMeta를 수행하고 본 함수가 호출된다.*/
                }

                for ( sPartAttr = aConstr->partIndex->partAttr;
                      sPartAttr != NULL;
                      sPartAttr = sPartAttr->next )
                {
                    // fix BUG-19175
                    // fix BUG-18937
                    if( QC_IS_NULL_NAME( sPartAttr->indexPartName ) == ID_TRUE )
                    {
                        // 각 인덱스 파티션을 지정하지 않은 경우에는
                        // makeIndexPartition()에서 이미 indexPartID를 얻었다.
                        sIndexPartID = sPartAttr->indexPartID;

                        // 각 인덱스 파티션을 지정하지 않은 경우
                        idlOS::snprintf( sIndexPartName, QC_MAX_NAME_LEN + 1,
                                         sPartAttr->indexPartNameStr );

                        // 테이블 파티션의 정보를 가져온다.
                        IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                                  aTableID,
                                                                  (UChar*)sPartAttr->tablePartNameStr,
                                                                  idlOS::strlen( sPartAttr->tablePartNameStr ),
                                                                  & sPartInfo,
                                                                  & sPartSCN,
                                                                  & sPartHandle )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // 인덱스 ID 생성
                        IDE_TEST( qcmPartition::getNextIndexPartitionID( aStatement,
                                                                         &sIndexPartID )
                                  != IDE_SUCCESS );

                        QC_STR_COPY( sIndexPartName, sPartAttr->indexPartName );

                        // 테이블 파티션의 정보를 가져온다.
                        IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                                  aTableID,
                                                                  sPartAttr->tablePartName,
                                                                  & sPartInfo,
                                                                  & sPartSCN,
                                                                  & sPartHandle )
                                  != IDE_SUCCESS );
                    }

                    /* PROJ-2464 hybrid partitioned table 지원
                     *  - Column 또는 Index 중 하나만 전달해야 한다.
                     */
                    IDE_TEST( adjustIndexColumn( sPartInfo->columns,
                                                 NULL,
                                                 NULL,
                                                 sColumnListAtKey )
                              != IDE_SUCCESS );

                    /* PROJ-2464 hybrid partitioned table 지원
                     *  - Partition Info를 구성할 때에, Table Option을 Partitioned Table의 값으로 복제한다.
                     *  - 따라서, PartInfo의 정보를 이용하지 않고, TBSID에 따라 적합한 값으로 조정해서 이용한다.
                     */
                    adjustIndexAttr( sPartAttr->TBSAttr.mID,
                                     aConstr->segAttr,
                                     sParseTree->segStoAttr,
                                     sIndexFlag,
                                     aConstr->mDirectKeyMaxSize,
                                     & sSegAttr,
                                     & sSegStoAttr,
                                     & sNewIndexFlag,
                                     & sDirectKeyMaxSize );

                    if(smiTable::createIndex(aStatement->mStatistics,
                                             QC_SMI_STMT( aStatement ),
                                             sPartAttr->TBSAttr.mID,
                                             sPartHandle,
                                             (SChar*)sIndexPartName,
                                             sIndexID,
                                             sIndexType,
                                             sColumnListAtKey,
                                             sNewIndexFlag,
                                             aConstr->parallelDegree,
                                             sBuildFlag,
                                             sSegAttr,
                                             sSegStoAttr,
                                             0, /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                                             &sIndexHandle)
                       != IDE_SUCCESS)
                    {
                        // To fix BUG-17762
                        // 기존 에러코드에 대한 하위 호환성을 고려하여 SM 에러를
                        // QP 에러로 변환한다.
                        if( ideGetErrorCode() == smERR_ABORT_NOT_NULL_VIOLATION )
                        {
                            ideClearError();
                            IDE_RAISE( ERR_HAS_NULL_VALUE );
                        }
                        else
                        {
                            IDE_TEST( ID_TRUE );
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST( qdx::insertIndexPartitionsIntoMeta( aStatement,
                                                                  aUserID,
                                                                  aTableID,
                                                                  sIndexID,
                                                                  sPartInfo->partitionID,
                                                                  sIndexPartID,
                                                                  sIndexPartName,
                                                                  NULL,
                                                                  NULL,
                                                                  sPartAttr->TBSAttr.mID )
                             != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            // Nothing to do
        }

        if( (sIndexFlag & SMI_INDEX_UNIQUE_MASK) == SMI_INDEX_UNIQUE_ENABLE )
        {
            sUnique = ID_TRUE;
        }
        else
        {
            sUnique = ID_FALSE;
        }

        /* PROJ-2461 pk,uk constraint에서 prefix index 제약 완화
         * global/local 구분에 따라서 메타에 입력할 정보를 정한다.
         */
        if( sIsPartitionedTable == ID_TRUE )
        {
            if( sIsLocalIndex == ID_TRUE )
            {
                sIsPartitionedIndex = ID_TRUE;
            }
            else
            {
                sIsPartitionedIndex = ID_FALSE;
            }
        }
        else
        {
            /* Nothing To Do */
        }
        
        IDE_TEST(qdx::insertIndexIntoMeta(aStatement,
                                          aConstr->indexTBSID,
                                          aUserID,
                                          aTableID,
                                          sIndexID,
                                          sObjName,
                                          sIndexType,
                                          sUnique,
                                          i,
                                          ID_TRUE,
                                          sIsPartitionedIndex,
                                          aConstr->indexTableID,
                                          sIndexFlag)
                 != IDE_SUCCESS);

        for (i = 0, sColumn = aConstr->constraintColumns;
             i < aConstr->constrColumnCount;
             i++, sColumn = sColumn->next)
        {
            IDE_TEST( qdx::insertIndexColumnIntoMeta(
                          aStatement,
                          aUserID,
                          sIndexID,
                          sColumn->basicInfo->column.id,
                          i,
                          ( ( (sColumn->basicInfo->column.flag &
                               SMI_COLUMN_ORDER_MASK) == SMI_COLUMN_ORDER_ASCENDING )
                            ? ID_TRUE : ID_FALSE ),
                          aTableID)
                      != IDE_SUCCESS);
        }

        if (aConstr->constrType == QD_PRIMARYKEY)
        {   // we must change null flag..
            // NOT NULL -> nullable = false
            for (i = 0, sColumn = aConstr->constraintColumns;
                 i < aConstr->constrColumnCount;
                 i++, sColumn = sColumn->next)
            {
                IDE_TEST(updateColumnSpecNull(aStatement,
                                              sColumn,
                                              ID_FALSE)
                         != IDE_SUCCESS);

                IDE_TEST(qdbCommon::makeColumnNotNull(
                    aStatement,
                    sNewTableHandle,
                    aMaxRows,
                    aPartInfoList,
                    sIsPartitionedTable,
                    sColumn->basicInfo->column.id) != IDE_SUCCESS);
            }
        }

        IDE_TEST(qcm::getNextConstrID(aStatement, &sConstrID)
                 != IDE_SUCCESS);

        if (QC_IS_NULL_NAME(aConstr->constrName) == ID_TRUE)
        {
            if (aConstr->constrType == QD_PRIMARYKEY)
            {
                idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                                 "%sCON_PRIMARY_ID_%"ID_INT32_FMT"",
                                 QC_SYS_OBJ_NAME_HEADER,
                                 sConstrID);
            }
            else
            {
                idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                                 "%sCON_PRIMARY_ID_%"ID_INT32_FMT"",
                                 QC_SYS_OBJ_NAME_HEADER, sConstrID );
            }
        }
        else
        {
            QC_STR_COPY( sObjName, aConstr->constrName );
        }

        sReferencedTableID = 0;
        sReferencedIndexID = 0;
        sReferenceRule     = 0;

        IDE_TEST( qdn::insertConstraintIntoMeta(
                      aStatement,
                      aUserID,
                      aTableID,
                      sConstrID,
                      sObjName,
                      aConstr->constrType,
                      sIndexID,
                      aConstr->constrColumnCount,
                      sReferencedTableID,
                      sReferencedIndexID,
                      sReferenceRule,
                      (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                      ID_TRUE ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        for ( i = 0, sColumn = aConstr->constraintColumns;
              i < aConstr->constrColumnCount;
              i++, sColumn = sColumn->next)
        {
            IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                          aStatement,
                          aUserID,
                          aTableID,
                          sConstrID,
                          i,
                          sColumn->basicInfo->column.id)
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_HAS_NULL_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOTNULL_HAS_NULL));
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "createConstrPrimaryUnique",
                                  "invalid part table info" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::createConstrForeign(
    qcStatement     * aStatement,
    qdConstraintSpec *aConstr,
    UInt              aUserID,
    UInt              aTableID)
{
/***********************************************************************
 *
 * Description :
 *    executeAddCol, executeCreateTable, executeCreateTableAsSelect,
 *    executeAddConstr 에서 호출, foreign key 처리
 *
 * Implementation :
 *    constraint 에 referentialConstraintSpec 이 있으면
 *    1. constraint 이름 부여
 *    2. 참조하는 테이블 ID 구하기
 *    3. 참조하는 인덱스 ID 구하기
 *    4. 2,3 에서 구한 ID 로 qcmTableInfo, qcmIndex 구하기
 *    5. constraint 정보를 메타 테이블에 입력
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::createConstrForeign"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdReferenceSpec  *sRefSpec;
    qcmColumn        *sColumn;
    SChar             sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    UInt              i;
    UInt              sIndexID = 0;
    UInt              sConstrID;
    UInt              sDeleteRule;   // PROJ-1509
    qcmTableInfo     *sReferencedTableInfo;
    void             *sTableHandle;

    smSCN             sSCN;
    UInt              sReferencedTableID = 0;
    UInt              sReferencedIndexID = 0;
    qcmIndex         *sReferencedIndex = NULL;
    qcmIndex         *sIndexInfo;
    UInt              sConstrType = QD_CONSTR_MAX;
    idBool            sValidate;

    sRefSpec = aConstr->referentialConstraintSpec;

    if (sRefSpec != NULL)
    {
        IDE_TEST(qcm::getNextConstrID(aStatement, &sConstrID)
                 != IDE_SUCCESS);
        if (QC_IS_NULL_NAME(aConstr->constrName) == ID_TRUE)
        {
            idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                             "%sCON_ID_%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrID );
        }
        else
        {
            QC_STR_COPY( sObjName, aConstr->constrName );
        }

        /* self */
        if ( sRefSpec->referencedTableID == ID_UINT_MAX )
        {
            sReferencedTableID = aTableID;
            sRefSpec->referencedTableID = sReferencedTableID;

            IDE_TEST( qcm::getTableInfoByID(aStatement,
                                            sRefSpec->referencedTableID,
                                            &sReferencedTableInfo,
                                            &sSCN,
                                            &sTableHandle)
                      != IDE_SUCCESS);

            IDE_TEST( qcm::validateAndLockTable(aStatement,
                                                sTableHandle,
                                                sSCN,
                                                SMI_TABLE_LOCK_X)
                      != IDE_SUCCESS );
        }
        else
        {
            sReferencedTableID = sRefSpec->referencedTableID;
            sReferencedTableInfo = sRefSpec->referencedTableInfo;

            // BUG-17122
            IDE_TEST( qcm::validateAndLockTable(aStatement,
                                                sRefSpec->referencedTableHandle,
                                                sRefSpec->referencedTableSCN,
                                                SMI_TABLE_LOCK_X)
                      != IDE_SUCCESS);

            // BUG-17122
            // Parent Table의 SCN을 변경한다.
            // tableInfo를 변경할 필요는 없음.
            IDE_TEST( qcm::touchTable(QC_SMI_STMT( aStatement ),
                                      sRefSpec->referencedTableID,
                                      SMI_TBSLV_DDL_DML )
                      != IDE_SUCCESS);
        }

        /* self */
        if (sRefSpec->referencedIndexID == 0)
        {
            // BUG-17126
            // Foreign Key Constraint는 Unique Constraint나 Primary Constraint가
            // 있을때만 생성할 수 있다.
            for (i = 0; i < sReferencedTableInfo->uniqueKeyCount; i++)
            {
                sConstrType =
                            sReferencedTableInfo->uniqueKeys[i].constraintType;

                // fix BUG-19187, BUG-19190
                if( (sConstrType == QD_PRIMARYKEY) ||
                    (sConstrType == QD_UNIQUE) )
                {
                    sIndexInfo =
                            sReferencedTableInfo->uniqueKeys[i].constraintIndex;

                    if (qdn::matchColumnId(
                            aConstr->referentialConstraintSpec->referencedColList,
                            (UInt*) smiGetIndexColumns(sIndexInfo->indexHandle),
                            sIndexInfo->keyColCount,
                            NULL ) == ID_TRUE)
                    {
                        sRefSpec->referencedIndexID = sIndexInfo->indexId;
                        sReferencedIndex = sIndexInfo;
                        sReferencedIndexID = sIndexInfo->indexId;
                        break;
                    }
                }
                else
                {
                    // LocalUnique Constraint인 경우에는 FK를 생성할 수 없다.
                    // Nothing to do
                }
            }

            IDE_TEST_RAISE( sReferencedIndex == NULL,
                            err_constraint_not_exist);
        }
        else
        {
            sReferencedIndexID = sRefSpec->referencedIndexID;

            IDE_TEST(qcmCache::getIndexByID(sReferencedTableInfo,
                                            sReferencedIndexID,
                                            &sReferencedIndex)
                     != IDE_SUCCESS);
        }

        // PROJ-1509
        sDeleteRule = sRefSpec->referenceRule;

        // PROJ-1874
        if (aConstr->constrState != NULL)
        {
            sValidate = aConstr->constrState->validate;
        }
        else
        {
            sValidate = ID_TRUE;
        }

        sIndexID = 0;

        IDE_TEST( qdn::insertConstraintIntoMeta(
                      aStatement,
                      aUserID,
                      aTableID,
                      sConstrID,
                      sObjName,
                      aConstr->constrType,
                      sIndexID,
                      aConstr->constrColumnCount,
                      sReferencedTableID,
                      sReferencedIndexID,
                      sDeleteRule,
                      (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                      sValidate ) // ConstraintState의 Validate값; PROJ-1874
                  != IDE_SUCCESS);

        for ( i = 0, sColumn = aConstr->constraintColumns;
              i < aConstr->constrColumnCount;
              i++, sColumn = sColumn->next)
        {
            IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                          aStatement,
                          aUserID,
                          aTableID,
                          sConstrID,
                          i,
                          sColumn->basicInfo->column.id)
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_constraint_not_exist);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDN_REFERENCED_CONSTRAINT_NOT_FOUND));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::createIndexFromIndexParseTree(
    qcStatement             * aStatement,
    qdIndexParseTree        * aIndexParseTree,
    smOID                     aTableOID,
    qcmTableInfo            * aTableInfo,
    UInt                      aTableID,
    qcmPartitionInfoList    * aPartInfoList )
{
/***********************************************************************
 *
 * Description :
 *    executeAddCol(ALTER TABLE ... ADD COL 수행) 로부터 호출,
 *    추가되는 컬럼에 index가 있을 경우 수행됨
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::createIndexFromIndexParseTree"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcmColumn             * sColumn;
    qcmColumn             * sColumnInfo;
    smiColumnList           sColumnListAtRow[QC_MAX_KEY_COLUMN_COUNT];
    smiColumnList           sColumnListAtKey[QC_MAX_KEY_COLUMN_COUNT];
    mtcColumn               sColumnAtKey[QC_MAX_KEY_COLUMN_COUNT];
    SChar                   sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    UInt                    i;
    UInt                    sFlag;
    UInt                    sOffset;
    UInt                    sIndexID;
    UInt                    sIndexPartID;
    SChar                   sIndexType[4096];
    SInt                    sSize;
    UInt                    sType;
    const void            * sNewTableHandle;
    const void            * sIndexHandle;
    UInt                    sTableType;
    UInt                    sBuildFlag = 0;

    UInt                    sUserID;
    idBool                  sIsPartitionedTable = ID_FALSE;
    idBool                  sIsPartitionedIndex = ID_FALSE;
    qdTableParseTree      * sParseTree;
    qcmTableInfo          * sPartInfo = NULL;
    void                  * sPartHandle;
    smSCN                   sPartSCN;
    SChar                   sIndexPartName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    idBool                  sUnique = ID_FALSE;
    qdPartitionAttribute  * sPartAttr;
    UInt                    sTablePartCount = 0;
    UInt                    sIndexPartCount = 0;
    qcmColumn             * sIndexTableColumns;
    UInt                    sIndexTableColumnCount;
    qcNamePosition          sIndexTableNamePos;
    qdIndexTableList      * sIndexTable;
    qcmTableInfo          * sIndexTableInfo;
    UInt                    sIndexTableID = 0;
    qdFunctionNameList    * sRelatedFunctionName;
    qsIndexRelatedProc      sRelatedFunction;
    UInt                    sLocalIndexType = 0;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStoAttr;
    UInt                    sIndexFlag;
    ULong                   sDirectKeyMaxSize;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sUserID = aIndexParseTree->userIDOfIndex;
    
    sNewTableHandle = smiGetTable( aTableOID );
    
    sTableType = aTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;
    
    // PROJ-1502 PARTITIONED DISK TABLE
    if( aTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE )
    {
        sIsPartitionedTable = ID_FALSE;
    }
    else
    {
        sIsPartitionedTable = ID_TRUE;
    }

    // index type
    if ( QC_IS_NULL_NAME( aIndexParseTree->indexType ) )
    {
        sType = mtd::getDefaultIndexTypeID( aIndexParseTree->keyColumns
                                            ->basicInfo->module );
    }
    else
    {
        sSize = aIndexParseTree->indexType.size < (SInt)(ID_SIZEOF(sIndexType)-1) ?
            aIndexParseTree->indexType.size : (SInt)(ID_SIZEOF(sIndexType)-1) ;

        idlOS::strncpy( sIndexType,
                        aIndexParseTree->indexType.stmtText +
                        aIndexParseTree->indexType.offset,
                        sSize );
        sIndexType[sSize] = '\0';

        IDE_TEST( smiFindIndexType( sIndexType, &sType ) != IDE_SUCCESS);

        // To Fix PR-15190
        IDE_TEST_RAISE( mtd::isUsableIndexType(
                            aIndexParseTree->keyColumns->basicInfo->module,
                            sType ) != ID_TRUE,
                        not_supported_type ) ;
    }

    // column 정보 재설정
    // validation시에는 아직 생성되지 않은 컬럼으로 column 정보를 설정했으므로
    // 실재 생성된 컬럼으로 다시 column 정보를 생성한다.
    for ( sColumn = aIndexParseTree->keyColumns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        IDE_TEST( qcmCache::getColumn( aStatement,
                                       aTableInfo,
                                       sColumn->namePos,
                                       &sColumnInfo )
                  != IDE_SUCCESS );
            
        sFlag = sColumn->basicInfo->column.flag & SMI_COLUMN_ORDER_MASK;
            
        // fix BUG-33258
        if( sColumn->basicInfo != sColumnInfo->basicInfo )
        {
            *(sColumn->basicInfo) = *(sColumnInfo->basicInfo);
        }
            
        sColumn->basicInfo->column.flag &= ~SMI_COLUMN_ORDER_MASK;
        sColumn->basicInfo->column.flag |= (sFlag & SMI_COLUMN_ORDER_MASK);
            
        // PROJ-1705
        if( sTableType == SMI_TABLE_DISK )
        {
            IDE_TEST( setIndexKeyColumnTypeFlag( sColumn->basicInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    // make smiColumnList
    sColumn = aIndexParseTree->keyColumns;
    IDE_TEST(qcm::getNextIndexID(aStatement, &sIndexID)
             != IDE_SUCCESS);
    for (i = 0; i < QC_MAX_KEY_COLUMN_COUNT; i++)
    {
        sColumnListAtRow[i].next = NULL;
        sColumnListAtRow[i].column = NULL;
        sColumnListAtKey[i].next = NULL;
        sColumnListAtKey[i].column = NULL;
    }

    i = 0;
    sOffset = 0;
    while (sColumn != NULL)
    {
        sColumnListAtRow[i].column = &(sColumn->basicInfo->column);
        idlOS::memcpy( &(sColumnAtKey[i]),
                       sColumn->basicInfo, ID_SIZEOF(mtcColumn) );
        if ( sTableType == SMI_TABLE_DISK )
        {
            // PROJ-1705
            IDE_TEST(
                qdbCommon::setIndexKeyColumnTypeFlag( &(sColumnAtKey[i]) )
                != IDE_SUCCESS );

            // To Fix PR-8111
            if ( (sColumnAtKey[i].column.flag & SMI_COLUMN_TYPE_MASK)
                 == SMI_COLUMN_TYPE_FIXED )
            {
                sOffset = idlOS::align( sOffset,
                                        sColumn->basicInfo->module->align );
                sColumnAtKey[i].column.offset = sOffset;
                sColumnAtKey[i].column.value = NULL;
                sOffset += sColumn->basicInfo->column.size;
            }
            else
            {
                sOffset = idlOS::align( sOffset, 8 );
                sColumnAtKey[i].column.offset = sOffset;
                sColumnAtKey[i].column.value = NULL;
                sOffset += smiGetVariableColumnSize4DiskIndex();
            }
        }
        sColumnListAtKey[i].column = (smiColumn*) &(sColumnAtKey[i]);
        sColumn = sColumn->next;
        if (sColumn != NULL)
        {
            sColumnListAtRow[i].next = &sColumnListAtRow[i+1];
            sColumnListAtKey[i].next = &sColumnListAtKey[i+1];
        }
        else
        {
            sColumnListAtRow[i].next = NULL;
            sColumnListAtKey[i].next = NULL;
        }
        i++;
    }

    QC_STR_COPY( sObjName, aIndexParseTree->indexName );

    // BUG-17848 : 영속적인 속성과 휘발성 속성 분리
    sBuildFlag = aIndexParseTree->buildFlag;
    sBuildFlag |= SMI_INDEX_BUILD_UNCOMMITTED_ROW_DISABLE;

    if( smiTable::createIndex(aStatement->mStatistics,
                              QC_SMI_STMT( aStatement ),
                              aIndexParseTree->TBSID,
                              sNewTableHandle,
                              (SChar*)sObjName,
                              sIndexID,
                              sType,
                              sColumnListAtKey,
                              aIndexParseTree->flag,
                              aIndexParseTree->parallelDegree,
                              sBuildFlag,
                              aIndexParseTree->segAttr,
                              aIndexParseTree->segStoAttr,
                              aIndexParseTree->mDirectKeyMaxSize, /* PROJ-2433 */
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

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sIsPartitionedTable == ID_TRUE )
    {
        // PROJ-1624 global non-partitioned index
        if ( aIndexParseTree->partIndex->partIndexType == QCM_NONE_PARTITIONED_INDEX )
        {
            sIsPartitionedIndex = ID_FALSE;
            
            //--------------------------------
            // (global) non-partitioned index
            //--------------------------------

            sIndexTableNamePos.stmtText = aIndexParseTree->indexTableName;
            sIndexTableNamePos.offset   = 0;
            sIndexTableNamePos.size     = idlOS::strlen( aIndexParseTree->indexTableName );

            // index table columns 생성
            IDE_TEST( qdx::makeColumns4CreateIndexTable( aStatement,
                                                         aIndexParseTree->keyColumns,
                                                         aIndexParseTree->keyColCount,
                                                         & sIndexTableColumns,
                                                         & sIndexTableColumnCount )
                      != IDE_SUCCESS );

            // index table columns 검사
            IDE_TEST( qdbCommon::validateColumnListForCreateInternalTable(
                          aStatement,
                          ID_TRUE,  // in execution time
                          SMI_TABLE_DISK,
                          aIndexParseTree->TBSID,
                          sIndexTableColumns )
                      != IDE_SUCCESS );
                
            IDE_TEST( qdx::createIndexTable( aStatement,
                                             sUserID,
                                             sIndexTableNamePos,
                                             sIndexTableColumns,
                                             sIndexTableColumnCount,
                                             aIndexParseTree->TBSID,
                                             aTableInfo->segAttr,
                                             aTableInfo->segStoAttr,
                                             QDB_TABLE_ATTR_MASK_ALL,
                                             aTableInfo->tableFlag, /* Flag Value */
                                             aTableInfo->parallelDegree,
                                             & sIndexTable )
                      != IDE_SUCCESS );
                    
            // link new index table
            sIndexTable->next = sParseTree->newIndexTables;
            sParseTree->newIndexTables = sIndexTable;

            IDE_TEST( qdx::createIndexTableIndices(
                          aStatement,
                          sUserID,
                          sIndexTable,
                          aIndexParseTree->keyColumns,
                          aIndexParseTree->keyIndexName,
                          aIndexParseTree->ridIndexName,
                          aIndexParseTree->TBSID,
                          sType,
                          aIndexParseTree->flag,
                          aIndexParseTree->parallelDegree,
                          sBuildFlag,
                          aIndexParseTree->segAttr,
                          aIndexParseTree->segStoAttr,
                          0 ) /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                      != IDE_SUCCESS );
                
            // tableInfo 재생성
            sIndexTableInfo = sIndexTable->tableInfo;
                    
            IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT(aStatement),
                                                 sIndexTable->tableID,
                                                 sIndexTable->tableOID)
                     != IDE_SUCCESS);
            
            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                           sIndexTable->tableID,
                                           &(sIndexTable->tableInfo),
                                           &(sIndexTable->tableSCN),
                                           &(sIndexTable->tableHandle))
                     != IDE_SUCCESS);
            
            (void)qcm::destroyQcmTableInfo(sIndexTableInfo);
                        
            // index table id 설정
            sIndexTableID = sIndexTable->tableID;
        }
        else
        {
            sIsPartitionedIndex = ID_TRUE;
            
            //--------------------------------
            // partitioned index
            //--------------------------------

            // 지정한 인덱스 파티션 개수
            for( sIndexPartCount = 0, sPartAttr = aIndexParseTree->partIndex->partAttr;
                 sPartAttr != NULL;
                 sIndexPartCount++, sPartAttr = sPartAttr->next ) ;

            // 테이블 파티션 개수
            IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                                       aTableID,
                                                       & sTablePartCount )
                      != IDE_SUCCESS );

            //------------------------------------------------------------
            // 테이블 파티션의 개수만큼 인덱스 파티션을 지정하지 않았으면,
            // 지정하지 않은 인덱스 파티션까지 구축해서 파스트리에 달아놓음
            //------------------------------------------------------------
            if( sIndexPartCount < sTablePartCount )
            {
                // 각 인덱스 파티션을 위한 구조체를 생성한다.
                IDE_TEST( qdx::makeIndexPartition( aStatement,
                                                   aPartInfoList,
                                                   aIndexParseTree->partIndex )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do
            }

            //------------------------------------------------------------
            // create index on each partition
            //------------------------------------------------------------
            
            IDE_TEST(qdx::insertPartIndexIntoMeta(aStatement,
                                                  sUserID,
                                                  aTableID,
                                                  sIndexID,
                                                  sLocalIndexType,
                                                  aIndexParseTree->flag)
                     != IDE_SUCCESS);

            /* BUG-45620  Index 생성 시, SYS_PART_KEY_COLUMNS_에 Index Column 정보를 누락되거나 잘못된 Index Column 정보를 추가할 수 있습니다. */
            for ( sColumn  = aIndexParseTree->keyColumns;
                  sColumn != NULL;
                  sColumn  = sColumn->next )
            {
                IDE_TEST( qdx::insertIndexPartKeyColumnIntoMeta( aStatement,
                                                                 sUserID,
                                                                 sIndexID,
                                                                 sColumn->basicInfo->column.id,
                                                                 aTableInfo,
                                                                 QCM_INDEX_OBJECT_TYPE )
                          != IDE_SUCCESS );
            }

            for( sPartAttr = aIndexParseTree->partIndex->partAttr;
                 sPartAttr != NULL;
                 sPartAttr = sPartAttr->next )
            {
                // fix BUG-19175
                // fix BUG-18937
                if( QC_IS_NULL_NAME(sPartAttr->indexPartName) == ID_TRUE )
                {
                    // 각 인덱스 파티션을 지정하지 않은 경우에는
                    // makeIndexPartition()에서 이미 indexPartID를 얻었다.
                    sIndexPartID = sPartAttr->indexPartID;

                    // 각 인덱스 파티션을 지정하지 않은 경우
                    idlOS::snprintf( sIndexPartName, QC_MAX_OBJECT_NAME_LEN + 1,
                                     sPartAttr->indexPartNameStr );

                    // 테이블 파티션의 정보를 가져온다.
                    IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                              aTableID,
                                                              (UChar*)sPartAttr->tablePartNameStr,
                                                              idlOS::strlen(sPartAttr->tablePartNameStr),
                                                              & sPartInfo,
                                                              & sPartSCN,
                                                              & sPartHandle )
                              != IDE_SUCCESS );
                }
                else
                {
                    // 인덱스 ID 생성
                    IDE_TEST( qcmPartition::getNextIndexPartitionID(
                                  aStatement,
                                  & sIndexPartID )
                              != IDE_SUCCESS );

                    QC_STR_COPY( sIndexPartName, sPartAttr->indexPartName );

                    // 테이블 파티션의 정보를 가져온다.
                    IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                              aTableID,
                                                              sPartAttr->tablePartName,
                                                              & sPartInfo,
                                                              & sPartSCN,
                                                              & sPartHandle )
                              != IDE_SUCCESS );
                }

                /* PROJ-2464 hybrid partitioned table 지원
                 *  - Column 또는 Index 중 하나만 전달해야 한다.
                 */
                IDE_TEST( adjustIndexColumn( sPartInfo->columns,
                                             NULL,
                                             NULL,
                                             sColumnListAtKey )
                          != IDE_SUCCESS );

                /* PROJ-2464 hybrid partitioned table 지원
                 *  - Partition Info를 구성할 때에, Table Option을 Partitioned Table의 값으로 복제한다.
                 *  - 따라서, PartInfo의 정보를 이용하지 않고, TBSID에 따라 적합한 값으로 조정해서 이용한다.
                 */
                adjustIndexAttr( sPartAttr->TBSAttr.mID,
                                 aIndexParseTree->segAttr,
                                 aIndexParseTree->segStoAttr,
                                 aIndexParseTree->flag,
                                 aIndexParseTree->mDirectKeyMaxSize,
                                 & sSegAttr,
                                 & sSegStoAttr,
                                 & sIndexFlag,
                                 & sDirectKeyMaxSize );

                if(smiTable::createIndex(aStatement->mStatistics,
                                         QC_SMI_STMT( aStatement ),
                                         sPartAttr->TBSAttr.mID,
                                         sPartHandle,
                                         (SChar*)sIndexPartName,
                                         sIndexID,
                                         sType,
                                         sColumnListAtKey,
                                         sIndexFlag,
                                         aIndexParseTree->parallelDegree,
                                         sBuildFlag,
                                         sSegAttr,
                                         sSegStoAttr,
                                         0, /* BUG-42124 : direct key index는 partitioned table를 지원하지 않는다. */
                                         &sIndexHandle)
                   != IDE_SUCCESS)
                {
                    // To fix BUG-17762
                    // 기존 에러코드에 대한 하위 호환성을 고려하여 SM 에러를
                    // QP 에러로 변환한다.
                    if( ideGetErrorCode() == smERR_ABORT_NOT_NULL_VIOLATION )
                    {
                        ideClearError();
                        IDE_RAISE( ERR_HAS_NULL_VALUE );
                    }
                    else
                    {
                        IDE_TEST( ID_TRUE );
                    }
                }

                IDE_TEST(qdx::insertIndexPartitionsIntoMeta(aStatement,
                                                            sUserID,
                                                            aTableID,
                                                            sIndexID,
                                                            sPartInfo->partitionID,
                                                            sIndexPartID,
                                                            sIndexPartName,
                                                            NULL,
                                                            NULL,
                                                            sPartAttr->TBSAttr.mID )
                         != IDE_SUCCESS);
            }
        }
    }
    else
    {
        // Nothing to do
    }

    if( (aIndexParseTree->flag & SMI_INDEX_UNIQUE_MASK) == SMI_INDEX_UNIQUE_ENABLE )
    {
        sUnique = ID_TRUE;
    }
    else
    {
        sUnique = ID_FALSE;
    }

    IDE_TEST(qdx::insertIndexIntoMeta(aStatement,
                                      aIndexParseTree->TBSID,
                                      sUserID,
                                      aTableID,
                                      sIndexID,
                                      sObjName,
                                      sType,
                                      sUnique,
                                      i,
                                      ID_TRUE,
                                      sIsPartitionedIndex,
                                      sIndexTableID,
                                      aIndexParseTree->flag )
             != IDE_SUCCESS);

    for (i = 0, sColumn = aIndexParseTree->keyColumns;
         sColumn != NULL;
         i++, sColumn = sColumn->next)
    {
        IDE_TEST( qdx::insertIndexColumnIntoMeta(
                      aStatement,
                      sUserID,
                      sIndexID,
                      sColumn->basicInfo->column.id,
                      i,
                      ( ( (sColumn->basicInfo->column.flag &
                           SMI_COLUMN_ORDER_MASK) == SMI_COLUMN_ORDER_ASCENDING )
                        ? ID_TRUE : ID_FALSE ),
                      aTableID)
                  != IDE_SUCCESS);
    }

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    if ( aIndexParseTree->relatedFunctionNames != NULL )
    {
        sRelatedFunction.userID  = sUserID;
        sRelatedFunction.tableID = aTableID;
        sRelatedFunction.indexID = sIndexID;

        for ( sRelatedFunctionName = aIndexParseTree->relatedFunctionNames;
              sRelatedFunctionName != NULL;
              sRelatedFunctionName = sRelatedFunctionName->next )
        {
            sRelatedFunction.relatedUserID = sRelatedFunctionName->userID;
            SET_POSITION( sRelatedFunction.relatedProcName, sRelatedFunctionName->functionName );

            IDE_TEST( qcmProc::relInsertRelatedToIndex( aStatement,
                                                        &sRelatedFunction )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    // To Fix PR-15190
    IDE_EXCEPTION(not_supported_type);
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_QDX_CANNOT_CREATE_INDEX_DATATYPE ) ) ;
    }
    IDE_EXCEPTION(ERR_HAS_NULL_VALUE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOTNULL_HAS_NULL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::updateColumnSpecNull(qcStatement * aStatement,
                                       qcmColumn   * aColumn,
                                       idBool        aNullableFlag)
{
/***********************************************************************
 *
 * Description :
 *      컬럼의 NOT NULL 여부 설정
 *
 * Implementation :
 *      1. SYS_COLUMNS_ 메타 테이블의 IS_NULLABLE 컬럼 변경
 *
 ***********************************************************************/

    SChar     * sSqlStr;
    SChar     * sNullFlagStr[2] = {(SChar*)"F", (SChar*)"T"};
    vSLong      sRowCnt;

    IDU_LIMITPOINT("qdbCommon::updateColumnSpecNull::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    if (aNullableFlag == ID_FALSE)
    { // not null.

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COLUMNS_ SET "
                         "IS_NULLABLE = '%s'  "
                         "WHERE COLUMN_ID = %"ID_INT32_FMT"",
                         sNullFlagStr[0],
                         aColumn->basicInfo->column.id );
    }
    else
    { // nullable.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COLUMNS_ SET "
                         "IS_NULLABLE = '%s' "
                         "WHERE COLUMN_ID = %"ID_INT32_FMT"",
                         sNullFlagStr[1],
                         aColumn->basicInfo->column.id);
    }

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                &sRowCnt) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_BREAK_SCHEMA);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_BREAK_SCHEMA);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::createConstrNotNull(
    qcStatement      * aStatement,
    qdConstraintSpec * aConstr,
    UInt               aUserID,
    UInt               aTableID)
{
/***********************************************************************
 *
 * Description :
 *    NOT NULL constraint 를 메타 테이블에 입력
 *
 * Implementation :
 *    1. constraint ID 부여
 *    2. constraint 이름 부여
 *    3. SYS_CONSTRAINTS_, SYS_CONSTRAINT_COLUMNS_ 메타 테이블로 입력
 *
 ***********************************************************************/

#define IDE_FN "qdb::createConstrNotNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcmColumn        *sColumn;
    UInt              sConstrID;
    SChar             sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    UInt              i;

    if (aConstr->constrType == QD_NOT_NULL)
    {
        IDE_TEST(qcm::getNextConstrID(aStatement, &sConstrID)
                 != IDE_SUCCESS);

        if (QC_IS_NULL_NAME(aConstr->constrName) == ID_TRUE)
        {
            idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                             "%sCON_NOT_NULL_ID_%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrID );
        }

        else
        {
            QC_STR_COPY( sObjName, aConstr->constrName );
        }

        IDE_TEST(qdn::insertConstraintIntoMeta(
                     aStatement,
                     aUserID,
                     aTableID,
                     sConstrID,
                     sObjName,
                     aConstr->constrType,
                     0,
                     aConstr->constrColumnCount,
                     0,
                     0,
                     0,
                     (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                     ID_TRUE ) // ConstraintState의 Validate
                 != IDE_SUCCESS );

        for ( i = 0,
                  sColumn = aConstr->constraintColumns;
              i < aConstr->constrColumnCount;
              i++,
                  sColumn = sColumn->next)
        {
            IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                          aStatement,
                          aUserID,
                          aTableID,
                          sConstrID,
                          i,
                          sColumn->basicInfo->column.id)
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::createConstrCheck(
    qcStatement        * aStatement,
    qdConstraintSpec   * aConstr,
    UInt                 aUserID,
    UInt                 aTableID,
    qdFunctionNameList * aRelatedFunctionNames )
{
/***********************************************************************
 *
 * Description :
 *    CHECK constraint 를 메타 테이블에 입력
 *
 * Implementation :
 *    1. constraint ID 부여
 *    2. constraint 이름 부여
 *    3. NCHAR LITERAL을 UNICODE LITERAL으로 변환
 *    4. SYS_CONSTRAINTS_, SYS_CONSTRAINT_COLUMNS_ 메타 테이블로 입력
 *    5. SYS_CONSTRAINT_RELATED_ 메타 테이블로 관련 Function Name 입력
 *
 ***********************************************************************/

    qcmColumn               * sColumn = NULL;
    UInt                      sConstrID;
    SChar                     sObjName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt                      i;

    SChar                   * sCheckConditionStrForMeta = NULL;
    SChar                   * sUnicodeCheckCondition = NULL;
    qcNamePosList           * sNcharList = NULL;
    UInt                      sAddSize = 0;
    UInt                      sBufferSize = 0;

    qdFunctionNameList      * sRelatedFunctionName;
    qsConstraintRelatedProc   sRelatedFunction;

    if ( aConstr->constrType == QD_CHECK )
    {
        IDE_TEST( qcm::getNextConstrID( aStatement, &sConstrID )
                  != IDE_SUCCESS );

        if ( QC_IS_NULL_NAME( aConstr->constrName ) == ID_TRUE )
        {
            idlOS::snprintf( sObjName, ID_SIZEOF(sObjName),
                             "%sCON_CHECK_ID_%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrID );
        }
        else
        {
            QC_STR_COPY( sObjName, aConstr->constrName );
        }

        /* PROJ-1579 NCHAR
         * NCHAR LITERAL => UNICODE LITERAL 부분의 처리
         */
        if ( aConstr->ncharList != NULL )
        {
            for ( sNcharList = aConstr->ncharList;
                  sNcharList != NULL;
                  sNcharList = sNcharList->next )
            {
                /* U 타입으로 변환하면서 늘어나는 사이즈 계산 */
                sAddSize += (sNcharList->namePos.size - 3) * 2;
            }

            sBufferSize = aConstr->checkCondition->position.size + sAddSize;

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                              SChar,
                                              sBufferSize,
                                              &sUnicodeCheckCondition )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::convertToUTypeString(
                                    aStatement,
                                    aConstr->checkCondition->position.offset,
                                    aConstr->checkCondition->position.size,
                                    aConstr->ncharList,
                                    sUnicodeCheckCondition,
                                    sBufferSize )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::getStrForMeta(
                                    aStatement,
                                    sUnicodeCheckCondition,
                                    idlOS::strlen( sUnicodeCheckCondition ),
                                    &sCheckConditionStrForMeta )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qdbCommon::getStrForMeta(
                                    aStatement,
                                    aConstr->checkCondition->position.stmtText +
                                    aConstr->checkCondition->position.offset,
                                    aConstr->checkCondition->position.size,
                                    &sCheckConditionStrForMeta )
                      != IDE_SUCCESS );
        }

        IDE_TEST( qdn::insertConstraintIntoMeta(
                        aStatement,
                        aUserID,
                        aTableID,
                        sConstrID,
                        sObjName,
                        aConstr->constrType,
                        0,
                        aConstr->constrColumnCount,
                        0,
                        0,
                        0,
                        sCheckConditionStrForMeta, /* PROJ-1107 Check Constraint 지원 */
                        ID_TRUE )
                  != IDE_SUCCESS );

        for ( i = 0, sColumn = aConstr->constraintColumns;
              i < aConstr->constrColumnCount;
              i++, sColumn = sColumn->next )
        {
            IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                            aStatement,
                            aUserID,
                            aTableID,
                            sConstrID,
                            i,
                            sColumn->basicInfo->column.id )
                      != IDE_SUCCESS );
        }

        /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        if ( aRelatedFunctionNames != NULL )
        {
            sRelatedFunction.userID       = aUserID;
            sRelatedFunction.tableID      = aTableID;
            sRelatedFunction.constraintID = sConstrID;

            for ( sRelatedFunctionName = aRelatedFunctionNames;
                  sRelatedFunctionName != NULL;
                  sRelatedFunctionName = sRelatedFunctionName->next )
            {
                if ( ( aConstr->checkCondition->position.offset <=
                       sRelatedFunctionName->functionName.offset ) &&
                     ( ( aConstr->checkCondition->position.offset +
                         aConstr->checkCondition->position.size ) >=
                       ( sRelatedFunctionName->functionName.offset +
                         sRelatedFunctionName->functionName.size ) ) )
                {
                    IDE_DASSERT( aConstr->checkCondition->position.stmtText ==
                                 sRelatedFunctionName->functionName.stmtText );

                    sRelatedFunction.relatedUserID = sRelatedFunctionName->userID;
                    SET_POSITION( sRelatedFunction.relatedProcName, sRelatedFunctionName->functionName );

                    IDE_TEST( qcmProc::relInsertRelatedToConstraint( aStatement,
                                                                     &sRelatedFunction )
                              != IDE_SUCCESS );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::createConstrTimeStamp(
    qcStatement      * aStatement,
    qdConstraintSpec * aConstr,
    UInt               aUserID,
    UInt               aTableID)
{
#define IDE_FN "qdb::createConstrTimeStamp"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcmColumn        * sColumn;
    UInt               sConstrID;
    SChar              sObjName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    UInt               i;

    if (aConstr->constrType == QD_TIMESTAMP)
    {
        IDE_TEST(qcm::getNextConstrID(aStatement, &sConstrID)
                 != IDE_SUCCESS);

        if (QC_IS_NULL_NAME(aConstr->constrName) == ID_TRUE)
        {
            idlOS::snprintf( sObjName , ID_SIZEOF(sObjName),
                             "%sCON_TIMESTAMP_ID_%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrID );
        }

        else
        {
            QC_STR_COPY( sObjName, aConstr->constrName );
        }

        IDE_TEST(qdn::insertConstraintIntoMeta(
                     aStatement,
                     aUserID,
                     aTableID,
                     sConstrID,
                     sObjName,
                     aConstr->constrType,
                     0,
                     aConstr->constrColumnCount,
                     0,
                     0,
                     0,
                     (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                     ID_TRUE ) // ConstraintState의 Validate
                 != IDE_SUCCESS );

        for ( i = 0,
                  sColumn = aConstr->constraintColumns;
              i < aConstr->constrColumnCount;
              i++,
                  sColumn = sColumn->next)
        {
            IDE_TEST( qdn::insertConstraintColumnIntoMeta(
                          aStatement,
                          aUserID,
                          aTableID,
                          sConstrID,
                          i,
                          sColumn->basicInfo->column.id)
                      != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::getStrForMeta(
    qcStatement  * aStatement,
    SChar        * aStr,
    SInt           aSize,
    SChar       ** aStrForMeta)
{
/***********************************************************************
 *
 * Description :
 *    문자열을 Meta Table에 입력할 수 있는 형태로 변환한다. (NULL 포함)
 *    (execution시에만 호출됨)
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt      i;
    SInt      j;
    SInt      sDefaultValLen = 0;

    for (i = 0; i < aSize; i++)
    {
        sDefaultValLen ++;
        if (aStr[i] == '\'')
        {
            sDefaultValLen ++;
        }
    }

    IDU_LIMITPOINT("qdbCommon::getStrForMeta::malloc");
    IDE_TEST(aStatement->qmxMem->alloc(sDefaultValLen + 1, (void**)aStrForMeta)
             != IDE_SUCCESS);

    j = 0;
    for (i = 0; i < aSize; i++)
    {
        if (aStr[i] == '\'')
        {
            (*aStrForMeta)[j] = '\'';
            j++;
            (*aStrForMeta)[j] = '\'';
        }
        else
        {
            (*aStrForMeta)[j] = aStr[i];
        }
        j++;
    }
    (*aStrForMeta)[sDefaultValLen] = '\0';
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::addSingleQuote4PartValue(
    SChar     *aStr,
    SInt       aSize,
    SChar    **aPartMaxVal )
{
/***********************************************************************
 *
 * Description :
 *    파티션 키 조건 값의 문자열을 검사한다.
 *    메타 테이블에 들어가야하기 때문에 '가 있을때는
 *    '를 추가로 붙여줘야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt i;
    SInt j;
    SInt sPartCondValLen = 0;

    for (i = 0; i < aSize; i++)
    {
        sPartCondValLen++;
        if (aStr[i] == '\'')
        {
            sPartCondValLen++;
        }
    }

    j = 0;
    for (i = 0; i < aSize; i++)
    {
        if (aStr[i] == '\'')
        {
            (*aPartMaxVal)[j] = '\'';
            j++;
            (*aPartMaxVal)[j] = '\'';
        }
        else
        {
            (*aPartMaxVal)[j] = aStr[i];
        }
        j++;
    }
    (*aPartMaxVal)[sPartCondValLen] = '\0';

    return IDE_SUCCESS;
}

IDE_RC qdbCommon::checkTableInfo(
    qcStatement      * aStatement,
    qcNamePosition     aUserName,
    qcNamePosition     aTableName,
    UInt             * aUserID,
    qcmTableInfo    ** aTableInfo,
    void            ** aTableHandle,
    smSCN            * aTableSCN)
{
/***********************************************************************
 *
 * Description :
 *    테이블이 존재하는지 체크
 *
 * Implementation :
 *    1. 사용자명으로 user ID 를 구한다
 *    2. 테이블이 존재하지 않으면 에러
 *    3. qcmTableInfo 반환
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::checkTableInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdbCommon::checkTableInfo"));


    UInt                    sUserID;
    qcuSqlSourceInfo        sqlInfo;
        
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

    if ( qcm::getTableHandleByName ( QC_SMI_STMT( aStatement ),
                                     sUserID,
                                     (UChar*) (aTableName.stmtText +
                                               aTableName.offset),
                                     aTableName.size,
                                     aTableHandle,
                                     aTableSCN)
         != IDE_SUCCESS)
    {
        sqlInfo.setSourceInfo( aStatement, & aTableName );
        IDE_RAISE(ERR_NOT_EXIST_TABLE);
    }
    else
    {
        /* Nothing To Do */
    }
        
    IDE_TEST( smiGetTableTempInfo( *aTableHandle,
                                   (void**)aTableInfo )
              != IDE_SUCCESS );

    // BUG-16235
    if( ((smiGetTableFlag(*aTableHandle) & SMI_TABLE_TYPE_MASK)
         == SMI_TABLE_MEMORY) ||
        ((smiGetTableFlag(*aTableHandle) & SMI_TABLE_TYPE_MASK)
         == SMI_TABLE_META) ||
        ((smiGetTableFlag(*aTableHandle) & SMI_TABLE_TYPE_MASK)
         == SMI_TABLE_VOLATILE) )
    {
        // Memory Table Space를 사용하는 DDL인 경우
        // Cursor Flag의 누적
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
    }
    else
    {
        // Disk Table Space를 사용하는 DDL인 경우
        // Cursor Flag의 누적
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCommon::checkDuplicatedObject(
    qcStatement      * aStatement,
    qcNamePosition     aUserName,
    qcNamePosition     aObjectName,
    UInt             * aUserID )
{
/***********************************************************************
 *
 * Description :
 *    중복되는 테이블,뷰,시퀀스 이름이 존재하는지 체크
 *
 * Implementation :
 *    1. 사용자명으로 user ID 를 구한다
 *    2. 테이블 이름이 존재하면 DUPLICATE_TABLE_NAME 에러
 *    3. user ID 반환
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::checkDuplicatedObject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdbCommon::checkDuplicatedObject"));

    UInt                 sUserID;
    qcmTableInfo       * sTableInfo;
    smSCN                sTableSCN;
    void               * sTableHandle;
    void               * sSequenceHandle;

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

    // BUG-16980
    // check object existence
    if (QC_IS_NULL_NAME(aObjectName) == ID_TRUE)
    {
        // Nothing to do.
    }
    else
    {
        if (qcm::getTableInfo(aStatement,
                              sUserID,
                              (UChar*) (aObjectName.stmtText +
                                        aObjectName.offset),
                              aObjectName.size,
                              &sTableInfo,
                              &sTableSCN,
                              &sTableHandle)
            == IDE_SUCCESS)
        {
            IDE_RAISE(ERR_EXIST_OBJECT_NAME);
        }
        else
        {
            // Nothing to do.
        }

        if (qcm::getSequenceInfo(aStatement,
                                 sUserID,
                                 (UChar*) (aObjectName.stmtText +
                                           aObjectName.offset),
                                 aObjectName.size,
                                 NULL,
                                 &sSequenceHandle)
            == IDE_SUCCESS)
        {
            IDE_RAISE(ERR_EXIST_OBJECT_NAME);
        }
        else
        {
            // Nothing to do.
        }
    }

    // return value
    *aUserID = sUserID;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdbCommon::getDiskRowSize( const void       * aTableHandle,
                           UInt             * aRowSize )
{
/***********************************************************************
 *
 * Description :
 *    Disk Table의 Row Size를 구함
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmTableInfo * sInfo;
    UInt           sRowSize;

    /* PROJ-2446 ONE SOURCE */
    IDE_TEST( smiGetTableTempInfo( aTableHandle,
                                   (void**)&sInfo )
              != IDE_SUCCESS );

    if ( sInfo == NULL )
    {
        // 아직 생성된 테이블이 아님
        // Read Row시 어떠한 결과도 존재하지 않는다.
        // 메모리 공간을 크게 할당할 필요가 없다.
        sRowSize = 4;
    }
    else
    {
        //---------------------------------
        // Record Size의 계산
        //---------------------------------

        IDE_TEST( getDiskRowSize( sInfo->columns,
                                  & sRowSize )
                  != IDE_SUCCESS );
    }

    *aRowSize = sRowSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::getDiskRowSize( qcmTableInfo     * aTableInfo,
                                  UInt             * aRowSize )
{
/***********************************************************************
 *
 * Description :
 *    Disk Table의 Row Size를 구함
 *
 * Implementation :
 *
 ***********************************************************************/

    //---------------------------------
    // Record Size의 계산
    //---------------------------------
    
    IDE_TEST( getDiskRowSize( aTableInfo->columns,
                              aRowSize )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qdbCommon::getDiskRowSize( qcmColumn        * aTableColumn,
                           UInt             * aRowSize )
{
/***********************************************************************
 *
 * Description :
 *    Disk Table의 Row Size를 구함
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn    * sLastColumn;
    qcmColumn    * sCurColumn;
    mtcColumn    * sColumn;
    UInt           sRowSize;

    //---------------------------------
    // 마지막 컬럼을 검색
    //---------------------------------

    sCurColumn = aTableColumn;
    sLastColumn = aTableColumn;

    while ( sCurColumn != NULL )
    {
        if ( sCurColumn->basicInfo->column.offset >
             sLastColumn->basicInfo->column.offset )
        {
            sLastColumn = sCurColumn;
        }
        else
        {
            // Nothing to do.
        }

        sCurColumn = sCurColumn->next;
    }

    //---------------------------------
    // Record Size의 계산
    //---------------------------------

    sColumn = sLastColumn->basicInfo;

    if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
         == SMI_COLUMN_TYPE_FIXED )
    {
        // 마지막 Column이 Fixed Column일 경우
        sRowSize = sColumn->column.offset + sColumn->column.size;
    }
    else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
              == SMI_COLUMN_TYPE_LOB )
    {
        // PROJ-1362
        sRowSize = sColumn->column.offset +
            smiGetLobColumnSize( SMI_TABLE_DISK );
    }
    else
    {
        // disk variable column은 존재하지 않는다.
        IDE_ASSERT( 0 );
    }

    sRowSize = idlOS::align( sRowSize, 8 );

    *aRowSize = sRowSize;

    return IDE_SUCCESS;
}

IDE_RC qdbCommon::getMemoryRowSize( qcmTableInfo     * aTableInfo,
                                    UInt             * aFixRowSize,
                                    UInt             * aRowSize )
{
/***********************************************************************
 *
 * Description :
 *    Memory Table의 Row Size를 구함
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcColumn    * sColumn;
    UInt           sFixSize;
    UInt           sVarSize;
    UInt           i;

    //---------------------------------
    // Fixed Record Size의 계산
    //---------------------------------
    sFixSize = 0;
    
    sColumn = aTableInfo->columns[0].basicInfo;
    for ( i = 1; i < aTableInfo->columnCount; i++ )
    {
        /* PROJ-2419 반영 이전에는 모든 Column의 Offset이 달랐으나,
         * PROJ-2419 반영 이후에는 Offset이 0인 Column이 여러 개일 수 있다.
         * 그러나, Fixed Column/Large Variable Column인 경우,
         * Fixed Slot Header Size(32) 때문에 Offset이 32 이상이므로 문제가 없다.
         */
        if ( aTableInfo->columns[i].basicInfo->column.offset > sColumn->column.offset )
        {
            sColumn = aTableInfo->columns[i].basicInfo;
        }
        else
        {
            // Nothing To Do
        }
    }

    // PROJ-2264 Dictionary table
    if( (sColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK)
        == SMI_COLUMN_COMPRESSION_FALSE )
    {
        if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_VARIABLE )
        {
            // Disk Variable Column일 경우는 없다.
            IDE_ASSERT( ( sColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                        == SMI_COLUMN_STORAGE_MEMORY );

            // 마지막 Column이 Variable Column일 경우, Variable Column의 Header Size를 계산
            /* Fixed Slot에 Column이 없는 경우, Fixed Slot Header Size 만큼의 공간이 필요하다.
             * Variable Slot의 OID는 Fixed Slot Header에 존재한다.
             */
            sFixSize = smiGetRowHeaderSize( SMI_TABLE_MEMORY );
        }
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_VARIABLE_LARGE )
        {
            // Disk Variable Column일 경우는 없다.
            IDE_ASSERT( ( sColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                        == SMI_COLUMN_STORAGE_MEMORY );

            // 마지막 Column이 Variable Column일 경우, Variable Column의 Header Size를 계산
            // Large Variable Column일 경우
            sFixSize = sColumn->column.offset +
                IDL_MAX( smiGetVariableColumnSize( SMI_TABLE_MEMORY ),
                         smiGetVCDescInModeSize() + sColumn->column.vcInOutBaseSize );
        }
        else if ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_LOB )
        {
            // PROJ-1362
            sFixSize = sColumn->column.offset +
                smiGetLobColumnSize( SMI_TABLE_MEMORY );
        }
        else
        {
            // 마지막 Column이 Fixed Column일 경우
            sFixSize = sColumn->column.offset + sColumn->column.size;
        }
    }
    else
    {
        // PROJ-2264 Dictionary table
        sFixSize = sColumn->column.offset + ID_SIZEOF( smOID );
    }

    sFixSize = idlOS::align( sFixSize, 8 );

    //---------------------------------
    // Variable Record Size의 계산
    //---------------------------------

    sVarSize = 0;

    for ( i = 0; i < aTableInfo->columnCount; i++ )
    {
        sColumn = aTableInfo->columns[i].basicInfo;

        // To fix BUG-24356
        // geometry에 대해서만 value buffer할당
        if ( ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
             &&
             (sColumn->module->id == MTD_GEOMETRY_ID) )
        {
            sVarSize =
                sVarSize + smiGetVariableColumnSize(SMI_TABLE_MEMORY) +
                sColumn->column.size;
            sVarSize = idlOS::align( sVarSize, 8 );
        }
        else
        {
            // Nothing To Do
        }
    }

    *aFixRowSize = sFixSize;
    *aRowSize = sFixSize + sVarSize;

    return IDE_SUCCESS;

#undef IDE_FN
}


extern "C" SInt
compareColumnAlign( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *    compare column align descending
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qdbCommon::compareColumnAlign"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UInt                sAlign1;
    UInt                sAlign2;

    IDE_DASSERT( (qdColumnIdx*)aElem1 != NULL );
    IDE_DASSERT( (qdColumnIdx*)aElem2 != NULL );
    //---------------------------------
    // compare align
    //---------------------------------

    if( ( ((qdColumnIdx*)aElem1)->column->basicInfo->column.flag &
          SMI_COLUMN_TYPE_MASK )
        != SMI_COLUMN_TYPE_FIXED )
    {
        sAlign1 = ID_SIZEOF(ULong);
    }
    else
    {
        sAlign1 = ((qdColumnIdx*)aElem1)->column->basicInfo->module->align;
    }

    if( ( ((qdColumnIdx*)aElem2)->column->basicInfo->column.flag &
          SMI_COLUMN_TYPE_MASK )
        != SMI_COLUMN_TYPE_FIXED )
    {
        sAlign2 = ID_SIZEOF(ULong);
    }
    else
    {
        sAlign2 = ((qdColumnIdx*)aElem2)->column->basicInfo->module->align;
    }

    if( sAlign1 < sAlign2 )
    {
        return 1;
    }
    else if( sAlign1 > sAlign2 )
    {
        return -1;
    }
    else
    {
        if( ((qdColumnIdx*)aElem1)->seqnum >
            ((qdColumnIdx*)aElem2)->seqnum )
        {
            return 1;
        }
        else if( ((qdColumnIdx*)aElem1)->seqnum <
                 ((qdColumnIdx*)aElem2)->seqnum )
        {
            return -1;
        }
    }

    return 0;

#undef IDE_FN
}

IDE_RC qdbCommon::setColListOffset( iduMemory * aMem,
                                    qcmColumn * aColumns,
                                    UInt        aCurrentOffset )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1391 QP Offset크기 순으로 column offset 조정
 *
 * Implementation :
 *    qcmColumn ptr array를 만들어 qsort로 offset크기가 큰 순서대로
 *    정렬한 후 정렬된 array에 aligned offset, size 설정
 *
 ***********************************************************************/

    qdColumnIdx*    sColumnArr;
    UInt            sColumnCount;
    qcmColumn*      sColumn;
    mtcColumn*      sMtcColumn;
    UInt            sColIdx = 0;
    UInt            sAlign;
    UInt            sSize;
    UInt            sVarColCount = 0;
    UInt            sMemMaxFixedRowSize;
    UInt            sFixedRowSize;

    UInt            sColumnTypeFlag;
    UInt            sVcInOutBaseSize;

    UInt            sCurrentOffset;

    //----------------------------------------------------------------
    // PROJ-1705
    // 디스크테이블을 메모리페이지사이즈제약 통일해서 생성
    //----------------------------------------------------------------

    //----------------------------------------------------------------
    // 테이블 생성 스키마로 메모리테이블 생성가능여부 판단
    //----------------------------------------------------------------

    if( ( aColumns->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        sCurrentOffset = aCurrentOffset;
    }
    else
    {
        sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_MEMORY);
    }

    // fix BUG-18610
    sMemMaxFixedRowSize = smiGetMaxFixedRowSize(SMI_MEMORY_SYSTEM_DICTIONARY);

    for (sColumn = aColumns, sColumnCount = 0;
         sColumn != NULL;
         sColumn = sColumn->next, sColumnCount++) ;

    IDU_LIMITPOINT("qdbCommon::setColListOffset::malloc1");
    IDE_TEST( aMem->alloc( ID_SIZEOF(qdColumnIdx) * sColumnCount,
                           (void**)&sColumnArr )
              != IDE_SUCCESS );

    for( sColumn = aColumns; sColumn != NULL; sColumn = sColumn->next )
    {
        sColumnArr[sColIdx].seqnum = sColIdx;
        sColumnArr[sColIdx++].column = sColumn;
    }

    idlOS::qsort( sColumnArr,
                  sColumnCount,
                  ID_SIZEOF(qdColumnIdx),
                  compareColumnAlign );

    for( sColIdx = 0; sColIdx < sColumnCount; sColIdx++ )
    {
        sMtcColumn = sColumnArr[sColIdx].column->basicInfo;

        if( ( sMtcColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
            == SMI_COLUMN_STORAGE_DISK )
        {
            IDE_TEST( getColumnTypeInMemory( sMtcColumn,
                                             &sColumnTypeFlag,
                                             &sVcInOutBaseSize )
                      != IDE_SUCCESS );
        }
        else
        {
            sColumnTypeFlag = sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK;
            sVcInOutBaseSize = sMtcColumn->column.vcInOutBaseSize;
        }

        // PROJ-2264 Dictionary table
        if( (sMtcColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK)
            == SMI_COLUMN_COMPRESSION_FALSE )
        {
            if( sColumnTypeFlag == SMI_COLUMN_TYPE_VARIABLE )
            {
                if ( ( aColumns->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
                     == SMI_COLUMN_STORAGE_MEMORY )
                {
                    /* 32000 이하의 united variable column 들은
                     * Record header의 OID를 따라간 variable slot 에 모아 저장하기 때문에
                     * fixed 영역에 공간을 차지하지 않으므로 size 를 0으로 한다. */
                    sAlign = 1;
                    sSize  = 0;
                }
                else
                {
                    /* PROJ-2419
                     * 디스크 테이블은 원래 코드를 사용한다.
                     * 확인가능한 test case : TC/Server/qp4/Project2/PROJ-1705/CREATE_TABLE/varchar_disk.sql */
                    sAlign = ID_SIZEOF(ULong);
                    sSize  = IDL_MAX( smiGetVCDescInModeSize() + sVcInOutBaseSize,
                                      smiGetVariableColumnSize( SMI_TABLE_MEMORY ) );
                }
            }
            else if ( sColumnTypeFlag == SMI_COLUMN_TYPE_VARIABLE_LARGE )
            {
                sAlign = ID_SIZEOF(ULong);
                sSize  = IDL_MAX( smiGetVCDescInModeSize() + sVcInOutBaseSize,
                                  smiGetVariableColumnSize( SMI_TABLE_MEMORY ) );
            }
            else if( sColumnTypeFlag == SMI_COLUMN_TYPE_LOB )
            {
                // PROJ-1362
                sAlign = ID_SIZEOF(ULong);

                // Memory Variable Column
                // PROJ-1557 vcInOutBaseSize를 더해줌.
                // MAX( VCDescInMode + vcinOutBaseSize, VCDesc )
                sSize  = IDL_MAX( smiGetVCDescInModeSize() + sVcInOutBaseSize,
                                  smiGetLobColumnSize( SMI_TABLE_MEMORY ) );
            }
            else
            {
                sAlign = sMtcColumn->module->align;
                sSize  = sMtcColumn->column.size;
            }
        }
        else
        {
            // PROJ-2264 Dictionary table
            sAlign = ID_SIZEOF(smOID);
            sSize  = ID_SIZEOF(smOID);
        }

        sCurrentOffset = idlOS::align( sCurrentOffset, sAlign );

        if ( ( sMtcColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
             == SMI_COLUMN_STORAGE_MEMORY )
        {
            if ( ( sMtcColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                 == SMI_COLUMN_COMPRESSION_FALSE )
            {
                if ( sColumnTypeFlag == SMI_COLUMN_TYPE_VARIABLE )
                {
                    /* 32000 이하의 united variable column 은 fixed 영역을 쓰지 않으므로
                     * offset 을 0으로 설정한다
                     * united variable 간의 순서 정보가 새로 필요해졌으므로 이를 함께 설정한다. */
                    sMtcColumn->column.offset   = 0;
                    sMtcColumn->column.varOrder = sVarColCount++;
                }
                else
                {
                    // 메모리컬럼의 경우
                    // align이 큰 순서로 컬럼정렬후 레코드내 offset을 결정.
                    // 현 시점에서 메모리컬럼의 offset이 결정되었으므로
                    // 이후 중복계산하지 않기 위해서 offset을 설정한다.
                    sMtcColumn->column.offset   = sCurrentOffset;
                    sMtcColumn->column.varOrder = 0;
                }
            }
            else
            {
                sMtcColumn->column.offset   = sCurrentOffset;
                sMtcColumn->column.varOrder = 0;
            }
        }
        else
        {
            // Nothing To Do
        }

        // increment offset using size.
        sCurrentOffset += sSize;
    }

    sFixedRowSize = idlOS::align( sCurrentOffset, ID_SIZEOF(SDouble) );

    //----------------------------------------------------------------
    // PROJ-1705 각 컬럼의 offset 설정
    //----------------------------------------------------------------

    if( ( aColumns->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        //------------------------------------------------------------
        // MEMORY :
        // align이 큰 순서로 컬럼정렬후 레코드내 offset결정
        // 중복계산하지 않기 위해서 메모리테이블 생성가능여부 판단시
        // 이미 offset 설정함.
        //------------------------------------------------------------

        /* BUG-44024 디스크 테이블 생성시 메모리의 fixed memory size 오류가 발생 합니다. */
        IDE_TEST_RAISE( sFixedRowSize > sMemMaxFixedRowSize, err_fixed_row_size );
    }
    else
    {
        //------------------------------------------------------------
        // DISK :
        // 스키마지정 컬럼순서로 레코드내 offset결정
        //------------------------------------------------------------

        sCurrentOffset = aCurrentOffset;

        for( sColumn = aColumns;
             sColumn != NULL;
             sColumn = sColumn->next )
        {
            sMtcColumn = sColumn->basicInfo;

            if( ( sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                == SMI_COLUMN_TYPE_LOB )
            {
                //------------------
                // LOB
                //------------------

                // PROJ-1362
                sAlign = ID_SIZEOF(ULong);
                sSize = smiGetLobColumnSize( SMI_TABLE_DISK );
            }
            else
            {
                //------------------
                // FIXED
                //------------------

                sAlign = sMtcColumn->module->align;
                sSize  = sMtcColumn->column.size;
            }

            sCurrentOffset = idlOS::align( sCurrentOffset, sAlign );

            sMtcColumn->column.offset = sCurrentOffset;

            // increment offset using size.
            sCurrentOffset += sSize;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_fixed_row_size);
    {
        // PROJ-1557
        IDE_SET(ideSetErrorCode (qpERR_ABORT_QDB_FIXED_PAGE_SIZE_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::setColListOffset( iduVarMemList * aMem,
                                    qcmColumn     * aColumns,
                                    UInt            aCurrentOffset )
{
/***********************************************************************
 *
 * Description :
 *      아래 함수와 첫번째 인자만 다르고 모두 동일하다.
 *
 *      IDE_RC qdbCommon::setColListOffset( iduMemory * aMem,
 *                                          qcmColumn * aColumns,
 *                                          UInt        aCurrentOffset );
 *
 * Implementation :
 *
 ***********************************************************************/

    qdColumnIdx*    sColumnArr;
    UInt            sColumnCount;
    qcmColumn*      sColumn;
    mtcColumn*      sMtcColumn;
    UInt            sColIdx = 0;
    UInt            sAlign;
    UInt            sSize;
    UInt            sVarColCount = 0;
    UInt            sMemMaxFixedRowSize;
    UInt            sFixedRowSize;

    UInt            sColumnTypeFlag;
    UInt            sVcInOutBaseSize;

    UInt            sCurrentOffset;

    //----------------------------------------------------------------
    // PROJ-1705
    // 디스크테이블을 메모리페이지사이즈제약 통일해서 생성
    //----------------------------------------------------------------

    //----------------------------------------------------------------
    // 테이블 생성 스키마로 메모리테이블 생성가능여부 판단
    //----------------------------------------------------------------

    if( ( aColumns->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        sCurrentOffset = aCurrentOffset;
    }
    else
    {
        sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_MEMORY);
    }

    // fix BUG-18610
    sMemMaxFixedRowSize = smiGetMaxFixedRowSize(SMI_MEMORY_SYSTEM_DICTIONARY);

    for (sColumn = aColumns, sColumnCount = 0;
         sColumn != NULL;
         sColumn = sColumn->next, sColumnCount++) ;

    IDU_LIMITPOINT("qdbCommon::setColListOffset::malloc2");
    IDE_TEST( aMem->alloc( ID_SIZEOF(qdColumnIdx) * sColumnCount,
                           (void**)&sColumnArr )
              != IDE_SUCCESS );

    for( sColumn = aColumns; sColumn != NULL; sColumn = sColumn->next )
    {
        sColumnArr[sColIdx].seqnum = sColIdx;
        sColumnArr[sColIdx++].column = sColumn;
    }

    idlOS::qsort( sColumnArr,
                  sColumnCount,
                  ID_SIZEOF(qdColumnIdx),
                  compareColumnAlign );

    for( sColIdx = 0; sColIdx < sColumnCount; sColIdx++ )
    {
        sMtcColumn = sColumnArr[sColIdx].column->basicInfo;

        if( ( sMtcColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
            == SMI_COLUMN_STORAGE_DISK )
        {
            IDE_TEST( getColumnTypeInMemory( sMtcColumn,
                                             &sColumnTypeFlag,
                                             &sVcInOutBaseSize )
                      != IDE_SUCCESS );
        }
        else
        {
            sColumnTypeFlag = sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK;
            sVcInOutBaseSize = sMtcColumn->column.vcInOutBaseSize;
        }

        // PROJ-2264 Dictionary table
        if( (sMtcColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK)
            == SMI_COLUMN_COMPRESSION_FALSE )
        {
            if( sColumnTypeFlag == SMI_COLUMN_TYPE_VARIABLE )
            {
                if ( ( aColumns->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
                     == SMI_COLUMN_STORAGE_MEMORY )
                {
                    /* 32000 이하의 united variable column 들은
                     * Record header의 OID를 따라간 variable slot 에 모아 저장하기 때문에
                     * fixed 영역에 공간을 차지하지 않으므로 size 를 0으로 한다. */
                    sAlign = 1;
                    sSize  = 0;
                }
                else
                {
                    /* PROJ-2419
                     * 디스크 테이블은 원래 코드를 사용한다.
                     * 확인가능한 test case : TC/Server/qp4/Project2/PROJ-1705/CREATE_TABLE/varchar_disk.sql */
                    sAlign = ID_SIZEOF(ULong);
                    sSize  = IDL_MAX( smiGetVCDescInModeSize() + sVcInOutBaseSize,
                                      smiGetVariableColumnSize( SMI_TABLE_MEMORY ) );
                }
            }
            else if ( sColumnTypeFlag == SMI_COLUMN_TYPE_VARIABLE_LARGE )
            {
                sAlign = ID_SIZEOF(ULong);
                sSize  = IDL_MAX( smiGetVCDescInModeSize() + sVcInOutBaseSize,
                                  smiGetVariableColumnSize( SMI_TABLE_MEMORY ) );
            }
            else if( sColumnTypeFlag == SMI_COLUMN_TYPE_LOB )
            {
                // PROJ-1362
                sAlign = ID_SIZEOF(ULong);

                // Memory Variable Column
                // PROJ-1557 vcInOutBaseSize를 더해줌.
                // MAX( VCDescInMode + vcinOutBaseSize, VCDesc )
                sSize  = IDL_MAX( smiGetVCDescInModeSize() + sVcInOutBaseSize,
                                  smiGetLobColumnSize( SMI_TABLE_MEMORY ) );
            }
            else
            {
                sAlign = sMtcColumn->module->align;
                sSize  = sMtcColumn->column.size;
            }
        }
        else
        {
            // PROJ-2264 Dictionary table
            sAlign = ID_SIZEOF(smOID);
            sSize  = ID_SIZEOF(smOID);
        }

        sCurrentOffset = idlOS::align( sCurrentOffset, sAlign );

        if ( ( sMtcColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
             == SMI_COLUMN_STORAGE_MEMORY )
        {
            if ( ( sMtcColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                 == SMI_COLUMN_COMPRESSION_FALSE )
            {
                if ( sColumnTypeFlag == SMI_COLUMN_TYPE_VARIABLE )
                {
                    /* 32000 이하의 united variable column 은 fixed 영역을 쓰지 않으므로
                     * offset 을 0으로 설정한다
                     * united variable 간의 순서 정보가 새로 필요해졌으므로 이를 함께 설정한다. */
                    sMtcColumn->column.offset   = 0;
                    sMtcColumn->column.varOrder = sVarColCount++;
                }
                else
                {
                    // 메모리컬럼의 경우
                    // align이 큰 순서로 컬럼정렬후 레코드내 offset을 결정.
                    // 현 시점에서 메모리컬럼의 offset이 결정되었으므로
                    // 이후 중복계산하지 않기 위해서 offset을 설정한다.
                    sMtcColumn->column.offset   = sCurrentOffset;
                    sMtcColumn->column.varOrder = 0;
                }
            }
            else
            {
                sMtcColumn->column.offset   = sCurrentOffset;
                sMtcColumn->column.varOrder = 0;
            }
        }
        else
        {
            // Nothing To Do
        }

        // increment offset using size.
        sCurrentOffset += sSize;
    }

    sFixedRowSize = idlOS::align( sCurrentOffset, ID_SIZEOF(SDouble) );

    //----------------------------------------------------------------
    // PROJ-1705 각 컬럼의 offset 설정
    //----------------------------------------------------------------

    if( ( aColumns->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        //------------------------------------------------------------
        // MEMORY :
        // align이 큰 순서로 컬럼정렬후 레코드내 offset결정
        // 중복계산하지 않기 위해서 메모리테이블 생성가능여부 판단시
        // 이미 offset 설정함.
        //------------------------------------------------------------

        /* BUG-44024 디스크 테이블 생성시 메모리의 fixed memory size 오류가 발생 합니다. */
        IDE_TEST_RAISE( sFixedRowSize > sMemMaxFixedRowSize, err_fixed_row_size );
    }
    else
    {
        //------------------------------------------------------------
        // DISK :
        // 스키마지정 컬럼순서로 레코드내 offset결정
        //------------------------------------------------------------

        sCurrentOffset = aCurrentOffset;

        for( sColumn = aColumns;
             sColumn != NULL;
             sColumn = sColumn->next )
        {
            sMtcColumn = sColumn->basicInfo;

            if( ( sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                == SMI_COLUMN_TYPE_LOB )
           {
                //------------------
                // LOB
                //------------------

                // PROJ-1362
                sAlign = ID_SIZEOF(ULong);
                sSize = smiGetLobColumnSize( SMI_TABLE_DISK );
            }
            else
            {
                //------------------
                // FIXED
                //------------------

                sAlign = sMtcColumn->module->align;
                sSize  = sMtcColumn->column.size;
            }

            sCurrentOffset = idlOS::align( sCurrentOffset, sAlign );

            sMtcColumn->column.offset = sCurrentOffset;

            // increment offset using size.
            sCurrentOffset += sSize;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_fixed_row_size);
    {
        // PROJ-1557
        IDE_SET(ideSetErrorCode (qpERR_ABORT_QDB_FIXED_PAGE_SIZE_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qdbCommon::containDollarInName( qcNamePosition * aObjectNamePos )
{
/***********************************************************************
 *
 * Description :
 *
 *  Object의 이름이 X$, D$, V$로 시작하지 못하도록 하여야 하며,
 *  이름에 X$, D$, V$를 포함하는지를 판별하는 함수이다.
 *  (소문자로 시작하는 v$, d$, v$는 가능함)
 *
 * Implementation :
 *  Parameter로 받은 Object Name의 처음 2바이트가 예약어인지 검사한다.
 *
 ***********************************************************************/

    SChar               * sObjectName;
    idBool                sContainDollar = ID_FALSE;

    if( aObjectNamePos != NULL )
    {
        sObjectName = aObjectNamePos->stmtText + aObjectNamePos->offset;

        if ( ( aObjectNamePos->size >= 2 ) &&
             ( ( idlOS::strMatch( sObjectName, 2, "X$", 2 ) == 0 ) ||
               ( idlOS::strMatch( sObjectName, 2, "D$", 2 ) == 0 ) ||
               ( idlOS::strMatch( sObjectName, 2, "S$", 2 ) == 0 ) ||
               ( idlOS::strMatch( sObjectName, 2, "V$", 2 ) == 0 ) ) )
        {
            sContainDollar = ID_TRUE;
        }
        else
        {
            // Object Names does not contain reserved words.
            // Nothing to do.
        }
    }
    else
    {
        // Object Name is NULL
        // Nothing to do.
    }

    return sContainDollar;
}

IDE_RC qdbCommon::updatePartTableSpecFromMeta(
    qcStatement     * aStatement,
    UInt              aPartTableID,
    UInt              aPartitionID,
    smOID             aPartitionOID )
{
/***********************************************************************
 *
 * Description :
 *      ALTER TABLE 관련 SYS_TABLE_PARTITIONS_ 메타 테이블 변경 수행
 *
 * Implementation :
 *      1. 사용자 검사, USER_ID 구함
 *      2. SYS_TABLE_PARTITIONS_ 메타 테이블의 정보 변경
 *
 ***********************************************************************/

#define IDE_FN "qdbCommon::updatePartTableSpecFromMeta"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar                   * sSqlStr;
    vSLong                    sRowCnt;

    IDU_LIMITPOINT("qdbCommon::updatePartTableSpecFromMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLE_PARTITIONS_ "
                     "SET PARTITION_OID = BIGINT'%"ID_INT64_FMT"', "
                     "    LAST_DDL_TIME = SYSDATE "
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "PARTITION_ID = INTEGER'%"ID_INT32_FMT"' ",
                     (ULong)aPartitionOID,
                     aPartTableID,
                     aPartitionID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qdbCommon::validatePartKeyCondValues( qcStatement        * aStatement,
                                             qdPartitionedTable * aPartTable )
{
/***********************************************************************
 *
 *  Description : PARTITION의 VALUES부분에 대한 node estimation
 *
 *  Implementation :
 *              (1) default노드가 하나만 있는지 체크.
 *              (1.1) default노드는 estimation에서 제외한다.
 *              (2) 파티션 키 컬럼에 맞게 value노드에 conversion생성
 *              (2) constant expression처리를 위해 passNode를 생성
 *              (3) 각 노드별로 estimate 실행.
 *              (4) 각 노드의 argument가 반드시 value노드여야 한다.
 *
 ***********************************************************************/

    qdPartitionAttribute * sPartAttr;

    // 파티션드 테이블이 아닌 테이블이 여기로 들어오면 에러.
    IDE_DASSERT( aPartTable->partMethod != QCM_PARTITION_METHOD_NONE );

    if( aPartTable->partMethod != QCM_PARTITION_METHOD_HASH )
    {
        for( sPartAttr = aPartTable->partAttr;
             sPartAttr != NULL;
             sPartAttr = sPartAttr->next )
        {
            IDE_TEST( makePartKeyCondValues( aStatement,
                                             QC_SHARED_TMPLATE(aStatement),
                                             aPartTable->partKeyColumns,
                                             aPartTable->partMethod,
                                             sPartAttr->partKeyCond,
                                             &sPartAttr->partCondVal )
                      != IDE_SUCCESS );
        }

        if ( ( aPartTable->partMethod == QCM_PARTITION_METHOD_RANGE ) ||
             ( aPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
        {
            // range인 경우.
            // 파티션 키 조건 값의 중복검사.
            IDE_TEST( checkDupRangePartKeyValues( aStatement,
                                                  aPartTable )
                      != IDE_SUCCESS );

            // partition order를 결정해 줘야 함.
            IDE_TEST( sortPartition( aStatement,
                                     aPartTable )
                      != IDE_SUCCESS );
        }
        else
        {
            // list인 경우.
            // 파티션 키 조건 값의 중복검사만 하면 됨.
            IDE_TEST( checkDupListPartKeyValues( aStatement,
                                                 aPartTable )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // hash. nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makePartKeyCondValues( qcStatement          * aStatement,
                                         qcTemplate           * aTemplate,
                                         qcmColumn            * aPartKeyColumns,
                                         qcmPartitionMethod     aPartMethod,
                                         qmmValueNode         * aPartKeyCond,
                                         qmsPartCondValList   * aPartCondVal )
{
    qcmColumn         * sCurrColumn;
    qmmValueNode      * sCurrValue;
    mtcColumn         * sValueColumn      = NULL;
    void              * sCanonizedValue   = NULL;
    void              * sValue            = NULL;
    mtcNode           * sNode;
    mtcColumn           sColumn;
    UInt                sECCSize;
    qcuSqlSourceInfo    sqlInfo;
    mtcTemplate       * sTemplate;

    sTemplate = & aTemplate->tmplate;

    aPartCondVal->partCondValCount = 0;
    aPartCondVal->partCondValType = QMS_PARTCONDVAL_NORMAL;

    sCurrColumn = aPartKeyColumns;

    // hash는  partKeyCond가 null이므로 이 부분을 건너뛰게 된다.
    for( sCurrValue = aPartKeyCond;
         sCurrValue != NULL;
         sCurrValue = sCurrValue->next )
    {
        // range나 list인 경우.
        // 키 컬럼은 반드시 존재해야 함.
        IDE_DASSERT( sCurrColumn != NULL );

        IDE_TEST( qtc::estimate( sCurrValue->value,
                                 aTemplate,
                                 aStatement,
                                 NULL,
                                 NULL,
                                 NULL )
                  != IDE_SUCCESS);

        /* BUG-46065 support range using hash */
        if ( aPartMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
        {
            IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                               aStatement,
                                               aTemplate,
                                               &mtdInteger )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qtc::makeConversionNode( sCurrValue->value,
                                               aStatement,
                                               aTemplate,
                                               sCurrColumn->basicInfo->module )
                      != IDE_SUCCESS );
        }
        // Constant value로 만들어 주어야 함.
        IDE_TEST( qtc::estimateConstExpr( sCurrValue->value,
                                          aTemplate,
                                          aStatement )
                  != IDE_SUCCESS );

        sNode = &sCurrValue->value->node;

        // constant value가 아니면 에러.

        if( qtc::isConstValue( aTemplate,
                               sCurrValue->value ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo(
                aStatement,
                & sCurrValue->value->position );

            IDE_RAISE( ERR_PARTCOND_VALUE );
        }
        else
        {
            // Nothing to do.
        }

        // column, value를 바로 뽑아올 수 있는 이유는
        // constant value이기 때문이다.
        sValueColumn = sTemplate->rows[sNode->table].columns + sNode->column;
        sValue = (void*)( (UChar*)sTemplate->rows[sNode->table].row
                          + sValueColumn->column.offset );

        if ( aPartMethod != QCM_PARTITION_METHOD_RANGE_USING_HASH )
        {
            // 데이터 타입이 반드시 같아야 함.
            IDE_DASSERT( sCurrColumn->basicInfo->type.dataTypeId ==
                         sValueColumn->type.dataTypeId );

            // PROJ-2002 Column Security
            // aPartCondVal은 DML의 optimize시 partition pruning을 위한 것으로
            // compare에만 사용된다. 그러므로 value를 column의 policy에 맞춰
            // 암호화할 필요가 없고, default policy로 canonize한다.
            if ( (sCurrColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                IDE_TEST( qcsModule::getECCSize(
                              sCurrColumn->basicInfo->precision,
                              & sECCSize )
                          != IDE_SUCCESS );

                IDE_TEST( mtc::initializeColumn(
                              & sColumn,
                              sCurrColumn->basicInfo->module,
                              1,
                              sCurrColumn->basicInfo->precision,
                              0 )
                          != IDE_SUCCESS );

                IDE_TEST( mtc::initializeEncryptColumn(
                              & sColumn,
                              (const SChar*) "",
                              sCurrColumn->basicInfo->precision,
                              sECCSize )
                          != IDE_SUCCESS );
            }
            else
            {
                mtc::copyColumn( & sColumn, sCurrColumn->basicInfo );
            }

            // canonize
            // canonize가 실패하는 경우는 overflow가 나는 경우.
            // 파티션 키 조건 값도 테이블에 저장될 수 있는 값이어야 한다.
            if ( ( sCurrColumn->basicInfo->module->flag & MTD_CANON_MASK )
                 == MTD_CANON_NEED )
            {
                sCanonizedValue = sValue;

                IDE_TEST( sCurrColumn->basicInfo->module->canonize(
                              & sColumn,
                              & sCanonizedValue,           // canonized value
                              NULL,
                              sValueColumn,
                              sValue,                     // original value
                              NULL,
                              & aTemplate->tmplate )
                          != IDE_SUCCESS );

                sValue = sCanonizedValue;
            }
            else if ( ( sCurrColumn->basicInfo->module->flag & MTD_CANON_MASK )
                      == MTD_CANON_NEED_WITH_ALLOCATION )
            {
                IDU_LIMITPOINT("qdbCommon::makePartKeyCondValues::malloc");
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                             sColumn.column.size,
                             (void**)&sCanonizedValue )
                         != IDE_SUCCESS);

                IDE_TEST( sCurrColumn->basicInfo->module->canonize(
                              & sColumn,
                              & sCanonizedValue,           // canonized value
                              NULL,
                              sValueColumn,
                              sValue,                      // original value
                              NULL,
                              & aTemplate->tmplate )
                          != IDE_SUCCESS );

                sValue = sCanonizedValue;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            IDE_TEST( mtc::initializeColumn(
                          &sColumn,
                          &mtdInteger,
                          0,
                          0,
                          0 )
                      != IDE_SUCCESS );
        }

        // range partition인 경우 null을 쓸 수 없다.
        if ( aPartMethod == QCM_PARTITION_METHOD_RANGE )
        {
            if( sCurrColumn->basicInfo->module->isNull( sCurrColumn->basicInfo,
                                                        sValue )
                == ID_TRUE )
            {
                sqlInfo.setSourceInfo(
                    aStatement,
                    & sCurrValue->value->position );

                IDE_RAISE( ERR_PARTCOND_VALUE );
            }
            else
            {
                // Nothing to do.
            }
        }
        /* BUG-46065 support range using hash */
        else if ( aPartMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
        {
            if ( sColumn.module->isNull( &sColumn, sValue )
                 == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement, &sCurrValue->value->position );
                IDE_RAISE( ERR_PARTCOND_VALUE );
            }
            else
            {
                // Nothing to do.
            }

            if ( ( *(SInt *)sValue > QMO_RANGE_USING_HASH_MAX_VALUE ) ||
                 ( *(SInt *)sValue <= 0 ) )
            {
                sqlInfo.setSourceInfo( aStatement, &sCurrValue->value->position );
                IDE_RAISE( ERR_PARTCOND_VALUE );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            // Nothing to do.
        }

        aPartCondVal->partCondValues
            [ aPartCondVal->partCondValCount++]
            = sValue;

        if( aPartMethod == QCM_PARTITION_METHOD_RANGE )
        {
            sCurrColumn = sCurrColumn->next;
        }
        else
        {
            // List partition인 경우 하나의 컬럼만 있음.
            // Nothing to do.
        }
    } // end for

    if( aPartCondVal->partCondValCount == 0 )
    {
        aPartCondVal->partCondValType = QMS_PARTCONDVAL_DEFAULT;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PARTCOND_VALUE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_PART_COND_VALUE,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkDupListPartKeyValues( qcStatement        * aStatement,
                                             qdPartitionedTable * aPartTable )
{
/***********************************************************************
 *
 *  Description : LIST PARTITION의 VALUES부분 중복 검사
 *                한 파티션 내의 값도 겹쳐서는 안되고,
 *                다른 파티션과도 겹쳐서는 안된다.
 *  Implementation :
 *
 ***********************************************************************/

    UInt                   i;
    UInt                   j;
    UInt                   k;

    mtdCompareFunc         sCompare;

    qdPartitionAttribute * sFirstPartAttr;
    qdPartitionAttribute * sSecondPartAttr;
    qcuSqlSourceInfo       sqlInfo;
    qmmValueNode         * sErrValue;
    mtcColumn            * sColumn;
    mtdValueInfo           sValueInfo1;
    mtdValueInfo           sValueInfo2;

    /* BUG-46065 support range using hash */
    if ( aPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
    {
        sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];
        sColumn = NULL;
    }
    else
    {
        sCompare =
            aPartTable->partKeyColumns->basicInfo->module->logicalCompare[MTD_COMPARE_ASCENDING];

        sColumn = aPartTable->partKeyColumns->basicInfo;
    }
    // 파티션 개수만큼 루프
    for( sFirstPartAttr = aPartTable->partAttr;
         sFirstPartAttr != NULL;
         sFirstPartAttr = sFirstPartAttr->next )
    {
        if( sFirstPartAttr->partKeyCond == NULL )
        {
            // default인 경우 생략
            continue;
        }
        else
        {
            // Nothing to do.
        }

        // 각각의 파티션에 대해 파티션 조건 값 개수만큼 루프
        for( i = 0; i < sFirstPartAttr->partCondVal.partCondValCount; i++ )
        {
            for( sSecondPartAttr = aPartTable->partAttr;
                 sSecondPartAttr != NULL;
                 sSecondPartAttr = sSecondPartAttr->next )
            {
                if( sSecondPartAttr->partKeyCond == NULL )
                {
                    // default인 경우 생략
                    continue;
                }
                else
                {
                    // Nothing to do.
                }

                // 하나의 파티션에 있는 하나의 조건에 대해
                // 하나의 파티션에 있는 다수의 파티션 조건만큼 루프
                for( j = 0; j < sSecondPartAttr->partCondVal.partCondValCount; j++ )
                {
                    if( ( sFirstPartAttr == sSecondPartAttr ) &&
                        ( i == j ) )
                    {
                        // 같은 부분인 경우 생략
                        continue;
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sValueInfo1.column = sColumn;
                    sValueInfo1.value  =
                        sFirstPartAttr->partCondVal.partCondValues[i];
                    sValueInfo1.flag   = MTD_OFFSET_USELESS;

                    sValueInfo2.column = sColumn;
                    sValueInfo2.value  =
                        sSecondPartAttr->partCondVal.partCondValues[j];
                    sValueInfo2.flag   = MTD_OFFSET_USELESS;

                    if( sCompare( &sValueInfo1, &sValueInfo2 ) == 0 )
                    {
                        // 중복 키 값이 있는 경우.
                        // 에러가 난 노드를 찾기 위해 루프
                        for( k = 0,
                                 sErrValue = sFirstPartAttr->partKeyCond;
                             k<i;
                             k++,
                                 sErrValue = sErrValue->next) ;

                        sqlInfo.setSourceInfo(
                            aStatement,
                            & sErrValue->value->position );

                        IDE_RAISE(ERR_DUP_PART_KEY_COND_VAL);
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUP_PART_KEY_COND_VAL)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_PART_COND_VALUE,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC qdbCommon::checkDupRangePartKeyValues( qcStatement        * aStatement,
                                              qdPartitionedTable * aPartTable )
{
/***********************************************************************
 *
 *  Description : RANGE PARTITION의 VALUES부분 중복 검사
 *                다른 파티션과 겹치는 영역이 나오면 안된다.
 *  Implementation :
 *
 ***********************************************************************/

    UInt                   i;
    mtdCompareFunc         sCompare;
    qdPartitionAttribute * sFirstPartAttr;
    qdPartitionAttribute * sSecondPartAttr;
    qcuSqlSourceInfo       sqlInfo;
    mtcColumn            * sColumn;
    qcmColumn            * sPartKeyColumn = NULL;
    idBool                 sIsSame;
    qmmValueNode         * sErrValueFirst;
    qmmValueNode         * sErrValueLast;
    mtdValueInfo           sValueInfo1;
    mtdValueInfo           sValueInfo2;

    // 파티션 개수만큼 루프
    for( sFirstPartAttr = aPartTable->partAttr;
         sFirstPartAttr != NULL;
         sFirstPartAttr = sFirstPartAttr->next )
    {
        if( sFirstPartAttr->partKeyCond == NULL )
        {
            // default인 경우 생략
            continue;
        }
        else
        {
            // Nothing to do.
        }

        // 파티션 개수만큼 루프
        for( sSecondPartAttr = aPartTable->partAttr;
             sSecondPartAttr != NULL;
             sSecondPartAttr = sSecondPartAttr->next )
        {
            if( (sSecondPartAttr->partKeyCond == NULL) ||
                (sFirstPartAttr->partCondVal.partCondValCount !=
                 sSecondPartAttr->partCondVal.partCondValCount) ||
                (sFirstPartAttr == sSecondPartAttr) )
            {
                // 1.default는 생략한다.
                // 2.서로 조건값 개수가 다르므로 파티션 기준이 겹칠 수 없다.
                // 3.동일 파티션끼리는 비교를 하지 않는다.
                continue;
            }
            else
            {
                // Nothing to do.
            }

            sIsSame = ID_TRUE;

            for( i = 0,
                     sPartKeyColumn = aPartTable->partKeyColumns;
                 ( i < sFirstPartAttr->partCondVal.partCondValCount ) &&
                     ( sPartKeyColumn != NULL );
                 i++,
                     sPartKeyColumn = sPartKeyColumn->next )
            {
                /* BUG-46065 support range using hash */
                if ( aPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
                {
                    sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];
                    sColumn = NULL;
                }
                else
                {
                    sCompare =
                        sPartKeyColumn->basicInfo->module->logicalCompare[MTD_COMPARE_ASCENDING];

                    sColumn = sPartKeyColumn->basicInfo;
                }

                sValueInfo1.column = sColumn;
                sValueInfo1.value  =
                    sFirstPartAttr->partCondVal.partCondValues[i];
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = sColumn;
                sValueInfo2.value  =
                    sSecondPartAttr->partCondVal.partCondValues[i];
                sValueInfo2.flag   = MTD_OFFSET_USELESS;


                if( sCompare( &sValueInfo1, &sValueInfo2 ) == 0 )
                {
                    continue;
                }
                else
                {
                    sIsSame = ID_FALSE;
                    break;
                }
            }

            if( sIsSame == ID_TRUE )
            {
                // values less than .. 부분의 처음과 끝을
                // 에러메시지에 뿌려주기 위함
                for( sErrValueFirst = sErrValueLast = sFirstPartAttr->partKeyCond;
                     sErrValueLast->next != NULL;
                     sErrValueLast = sErrValueLast->next) ;

                sqlInfo.setSourceInfo(
                    aStatement,
                    & sErrValueFirst->value->position,
                    & sErrValueLast->value->position);

                IDE_RAISE(ERR_DUP_PART_KEY_COND_VAL);
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUP_PART_KEY_COND_VAL)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_PART_COND_VALUE,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

extern "C" SInt
comparePartCondKeyValues( const void* aElem1, const void* aElem2 )
{
    qdPartIdx * sComp1;
    qdPartIdx * sComp2;

    sComp1 = (qdPartIdx*)aElem1;
    sComp2 = (qdPartIdx*)aElem2;

    IDE_DASSERT( sComp1 != NULL );
    IDE_DASSERT( sComp2 != NULL );

    return qmoPartition::compareRangePartition(
        sComp1->column,
        &sComp1->partAttr->partCondVal,
        &sComp2->partAttr->partCondVal );
}

/* BUG-46065 support range using hash */
extern "C" SInt compareRangeHashPartCondKeyValues( const void * aElem1, const void * aElem2 )
{
    qdPartIdx * sComp1;
    qdPartIdx * sComp2;

    sComp1 = (qdPartIdx*)aElem1;
    sComp2 = (qdPartIdx*)aElem2;

    IDE_DASSERT( sComp1 != NULL );
    IDE_DASSERT( sComp2 != NULL );

    return qmoPartition::compareRangeUsingHashPartition(
        &sComp1->partAttr->partCondVal,
        &sComp2->partAttr->partCondVal );
}

IDE_RC qdbCommon::sortPartition( qcStatement        * aStatement,
                                 qdPartitionedTable * aPartTable )
{
/***********************************************************************
 *
 *  Description : PARTITION의 VALUES부분을 가지고 정렬.
 *
 *  Implementation :
 *
 ***********************************************************************/
    qdPartIdx            * sPartArr;
    UInt                   sPartCount;
    qdPartitionAttribute * sPartAttr;
    UInt                   i = 0;

    sPartCount = aPartTable->partCount;

    IDU_LIMITPOINT("qdbCommon::sortPartition::malloc");
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qdPartIdx) * sPartCount,
                                         (void**)&sPartArr )
              != IDE_SUCCESS );

    for( sPartAttr = aPartTable->partAttr;
         sPartAttr != NULL;
         sPartAttr = sPartAttr->next )
    {
        sPartArr[i].statistics = aStatement->mStatistics;
        sPartArr[i].partAttr = sPartAttr;
        sPartArr[i++].column = aPartTable->partKeyColumns;
    }

    if ( aPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
    {
        idlOS::qsort( sPartArr,
                      sPartCount,
                      ID_SIZEOF(qdPartIdx),
                      compareRangeHashPartCondKeyValues );
    }
    else
    {
        idlOS::qsort( sPartArr,
                      sPartCount,
                      ID_SIZEOF(qdPartIdx),
                      comparePartCondKeyValues );
    }

    // 순서대로 파티션 리스트를 재구성
    for( i= 0; i < sPartCount-1; i++ )
    {
        sPartArr[i].partAttr->next = sPartArr[i+1].partAttr;
    }

    // 마지막꺼는 NULL
    sPartArr[i].partAttr->next = NULL;

    // 정렬된 첫번째 파티션을 연결함
    aPartTable->partAttr = sPartArr[0].partAttr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validatePartitionedTable( qcStatement         * aStatement,
                                            idBool                aIsCreateTable )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *      파티션드 테이블 생성 시, validate하는 함수이다.
 *      CREATE TABLE의 validation 하는 함수로부터 호출된다.
 *
 * Implementation :
 *      1. 파티션 키 컬럼 validation
 *
 *      2. 생성하는 각 파티션에 대한 validation
 *          2-1. 파티션 이름 중복 검사
 *          2-2. 각 파티션의 테이블스페이스 validation
 *          2-3. 각 파티션의 LOB 컬럼 validation
 *          2-4. 범위 파티션드 테이블일 경우 파티션 키 값의 개수 체크
 *          2-5. 범위 또는 리스트 파티션드 테이블일 경우,
 *               중복된 파티션 조건 값을 갖는 파티션이 있는지 체크
 *
 *      3. 해시 파티션드 테이블일 경우 PARTITION_ORDER 결정
 *
 ***********************************************************************/

    qdTableParseTree     * sParseTree;
    qdPartitionedTable   * sPartTable;
    qdPartitionAttribute * sPartAttr;
    qdPartitionAttribute * sTempPartAttr;
    qcmColumn            * sColumn;
    qcmColumn            * sTableColumn;
    UInt                   sTableColCnt = 0;
    UInt                   sPartKeyColCnt = 0;
    UInt                   sPartOrder = 0;
    qcuSqlSourceInfo       sqlInfo;
    qmmValueNode         * sCondValNode;
    UInt                   sTotalCondValCount = 0;
    UInt                   sCondValCount = 0;
    qcNamePosition         sFirstCondValPos;
    qcNamePosition         sLastCondValPos;
    qcNamePosition         sTotalCondValPos;

    SET_EMPTY_POSITION( sFirstCondValPos );
    SET_EMPTY_POSITION( sLastCondValPos );

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    // PROJ-1407 Temporary Table
    // temporary table로 partitioned table을 사용할 수 없다.
    IDE_TEST_RAISE( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK )
                    == QDT_CREATE_TEMPORARY_TRUE,
                    ERR_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE );

    sPartTable = sParseTree->partTable;

    // 테이블 컬럼의 개수
    for( sColumn = sParseTree->columns;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        sTableColCnt++;
    }

    // 파티션 키 컬럼의 개수
    for( sColumn = sPartTable->partKeyColumns;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        sPartKeyColCnt++;
    }

    /* BUG-46065 support range using hash */
    if ( sPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
    {
        IDE_TEST_RAISE( sPartKeyColCnt > 1, ERR_MAX_PARTITION_KEY_COLUMN_COUNT );
    }
    else
    {
        /* Nothing to do */
    }
    // -----------------------------------------------
    // 1. 파티션 키 컬럼 validation
    // -----------------------------------------------
    IDE_TEST( validatePartKeyColList( aStatement ) != IDE_SUCCESS );

    // -----------------------------------------------
    // 2. 생성하는 각 파티션에 대한 validation
    // -----------------------------------------------
    for( sPartAttr = sPartTable->partAttr;
         sPartAttr != NULL;
         sPartAttr = sPartAttr->next )
    {
        if ( aIsCreateTable == ID_TRUE )
        {
            // 파티션 이름 중복 검사
            for( sTempPartAttr = sPartTable->partAttr;
                 sTempPartAttr != sPartAttr;
                 sTempPartAttr = sTempPartAttr->next )
            {
                if ( QC_IS_NAME_MATCHED( sPartAttr->tablePartName, sTempPartAttr->tablePartName ) )
                {
                    sqlInfo.setSourceInfo(aStatement,
                                          & sPartAttr->tablePartName );
    
                    IDE_RAISE( ERR_DUPLICATE_PARTITION_NAME );
                }
            }

            // validation of 각 파티션의 TABLESPACE
            IDE_TEST( validateTBSOfPartition( aStatement, sPartAttr )
                      != IDE_SUCCESS );

            // validation of LOB column attributes
            // 각 파티션별로 LOB 컬럼 정보를 저장하기 위해
            // 테이블 컬럼 정보를 각 테이블 파티션의 컬럼 정보로 복사한다.
            sPartAttr->columns  = NULL;

            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( copyAndAdjustColumnList( aStatement,
                                               sParseTree->TBSAttr.mType,
                                               sPartAttr->TBSAttr.mType,
                                               sParseTree->columns,
                                               &(sPartAttr->columns),
                                               sTableColCnt,
                                               ID_FALSE /* aEnableVariableColumn */ )
                      != IDE_SUCCESS );

            // colSpace를 지정한 TBS로 변경한다.
            // 각 파티션의 TBS는 이미 validateTBSOfPartition 함수에서
            // 결정되어 있다.
            for( sColumn = sPartAttr->columns, sTableColumn = sParseTree->columns;
                 sColumn != NULL;
                 sColumn = sColumn->next, sTableColumn = sTableColumn->next )
            {
                sColumn->basicInfo->column.colSpace = sPartAttr->TBSAttr.mID;

                if( (sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK) ==
                    SMI_COLUMN_TYPE_LOB )
                {
                    /* PROJ-2464 hybrid partitioned table 지원
                     *  - Disk Partition의 경우, Partitioned의 Lob Column TBS(Default TBS)가
                     *    지정되어 있으면 Default TBS를 사용한다.
                     *  - PROJ-2334 PMT MEMORY PARTITIONED TABLE : Memory Partition의 경우,
                     *    Partition TBS를 사용한다.
                     */
                    /* PROJ-2465 Tablespace Alteration for Table
                     *  - 부족한 Default TBS 처리를 추가한다.
                     */
                    if ( ( ( sColumn->flag & QCM_COLUMN_LOB_DEFAULT_TBS_MASK ) == QCM_COLUMN_LOB_DEFAULT_TBS_TRUE ) &&
                         ( smiTableSpace::isDiskTableSpaceType( sPartAttr->TBSAttr.mType ) == ID_TRUE ) &&
                         ( smiTableSpace::isDiskTableSpaceType( sParseTree->TBSAttr.mType ) == ID_TRUE ) )
                    {
                        sColumn->basicInfo->column.colSpace =
                            sTableColumn->basicInfo->column.colSpace;
                    }
                    else
                    {
                        /* Nothing To Do */
                    }
                }
            }

            IDE_TEST(qdbCommon::validateLobAttributeList(
                         aStatement,
                         NULL,
                         sPartAttr->columns,
                         &(sPartAttr->TBSAttr),
                         sPartAttr->lobAttr )
                     != IDE_SUCCESS);
        }
        else
        {
            /* Nothing To Do */
        }

        // --------------------------------
        // 파티션 키 조건의 개수, 길이 체크
        // --------------------------------
        for( sCondValNode = sPartAttr->partKeyCond, sTotalCondValCount = 0;
             sCondValNode != NULL;
             sCondValNode = sCondValNode->next, sTotalCondValCount++ ) ;

        // 첫번째 위치와 마지막 위치를 구해서 나중에 합친다.
        for( sCondValNode = sPartAttr->partKeyCond, sCondValCount = 0;
             sCondValNode != NULL;
             sCondValNode = sCondValNode->next, sCondValCount++ )
        {
            // 첫번째 조건 값의 position을 구한다.
            if( sCondValCount == 0 )
            {
                sFirstCondValPos = sCondValNode->value->position;
            }
            // 마지막 조건 값의 position을 구한다.
            else if( sCondValCount == (sTotalCondValCount - 1) )
            {
                sLastCondValPos = sCondValNode->value->position;
            }
            else
            {
                // Nothing to do
            }
        }

        // 파티션 조건 값이 하나일 경우에는 첫번째 조건 값의
        // position을 그대로 갖는다.
        if( sCondValCount == 1 )
        {
            SET_POSITION( sTotalCondValPos, sFirstCondValPos );
        }
        else
        {
            SET_POSITION( sTotalCondValPos, sFirstCondValPos );

            sTotalCondValPos.size = sLastCondValPos.offset -
                sFirstCondValPos.offset +
                sLastCondValPos.size;
        }

        if ( ( sPartTable->partMethod == QCM_PARTITION_METHOD_RANGE ) ||
             ( sPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
        {
            // 기본 파티션일 경우는 체크하지 않음
            if( sPartAttr->partValuesType != QD_DEFAULT_VALUES_TYPE )
            {
                IDE_TEST_RAISE( (sCondValCount > sPartKeyColCnt) ||
                                (sCondValCount > QC_MAX_PARTKEY_COND_COUNT),
                                ERR_TOO_MANY_PARTITION_CONDITION_LIST );

                IDE_TEST_RAISE( (sTotalCondValPos.size >
                                 QC_MAX_PARTKEY_COND_VALUE_LEN),
                                ERR_TOO_LONG_PARTITION_CONDITION_VALUE );
            }
        }
        else if( sPartTable->partMethod == QCM_PARTITION_METHOD_LIST )
        {
            // 기본 파티션일 경우는 체크하지 않음
            if( sPartAttr->partValuesType != QD_DEFAULT_VALUES_TYPE )
            {
                IDE_TEST_RAISE( (sCondValCount > QC_MAX_PARTKEY_COND_COUNT),
                                ERR_TOO_MANY_PARTITION_CONDITION_LIST );

                IDE_TEST_RAISE( (sTotalCondValPos.size >
                                 QC_MAX_PARTKEY_COND_VALUE_LEN),
                                ERR_TOO_LONG_PARTITION_CONDITION_VALUE );
            }
        }
        else if( sPartTable->partMethod == QCM_PARTITION_METHOD_HASH )
        {
            sPartAttr->partOrder = sPartOrder;
            sPartOrder++;
        }
        else
        {
            // Nothing to do
        }

        // PROJ-1579 NCHAR
        // 각 파티션의 nchar literal의 list를 만들어 놓는다.
        IDE_TEST( makeNcharLiteralStr( aStatement,
                                       sParseTree->ncharList,
                                       sPartAttr )
                  != IDE_SUCCESS );
    }

    IDE_TEST( validatePartKeyCondValues( aStatement,
                                         sPartTable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUPLICATE_PARTITION_NAME)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_PARTITION_NAME,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }
    IDE_EXCEPTION(ERR_TOO_MANY_PARTITION_CONDITION_LIST)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_TOO_MANY_PARTITION_CONDITION_LIST));
    }
    IDE_EXCEPTION(ERR_TOO_LONG_PARTITION_CONDITION_VALUE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_TOO_LONG_PARTITION_CONDITION_VALUE));
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE));
    }
    IDE_EXCEPTION(ERR_MAX_PARTITION_KEY_COLUMN_COUNT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_MAX_PARTITION_KEY_COLUMN_COUNT ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validatePartKeyColList( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *      파티션 키 컬럼 validation
 *
 * Implementation :
 *      1. 파티션 키 컬럼 중복 체크
 *      2. 파티션 키 컬럼이 테이블 컬럼에 존재하는 컬럼인지 체크
 *      3. 파티션 키 컬럼 타입 체크. 대소 비교 가능한 타입이어야 함.
 *      4. 파티션 키 컬럼 개수 체크
 *
 ***********************************************************************/

    qdTableParseTree     * sParseTree;
    qdPartitionedTable   * sPartTable;
    qcmColumn            * sColumn;
    qcmColumn            * sTempColumn;
    UInt                   sTableColCnt = 0;
    UInt                   sPartKeyColCnt = 0;
    idBool                 sIsFound = ID_FALSE;
    qcuSqlSourceInfo       sqlInfo;
    SChar                  sTempColumnName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sPartTable = sParseTree->partTable;

    // 테이블 컬럼의 개수
    for( sColumn = sParseTree->columns;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        sTableColCnt++;
    }

    for( sColumn = sPartTable->partKeyColumns;
         sColumn != NULL;
         sColumn = sColumn->next )
    {
        // -----------------------------------------------
        // 1. 파티션 키 컬럼의 중복 체크
        // -----------------------------------------------
        for( sTempColumn = sPartTable->partKeyColumns;
             sTempColumn != sColumn;
             sTempColumn = sTempColumn->next )
        {
            if ( QC_IS_NAME_MATCHED( sColumn->namePos, sTempColumn->namePos ) )
            {
                sqlInfo.setSourceInfo(aStatement,
                                      & sColumn->namePos );

                IDE_RAISE( ERR_DUPLICATE_PARTITION_KEY_COLUMN_NAME );
            }
        }

        sIsFound = ID_FALSE;

        // -----------------------------------------------
        // 2. 파티션 키 컬럼이 테이블에 존재하는 컬럼인지 체크
        // -----------------------------------------------
        for( sTempColumn = sParseTree->columns;
             sTempColumn != NULL;
             sTempColumn = sTempColumn->next )
        {
            // CREATE TABLE
            if( sTempColumn->namePos.size != 0 )
            {
                QC_STR_COPY( sTempColumnName, sTempColumn->namePos );
            }
            // CREATE TABLE AS SELECT
            else
            {
                idlOS::memcpy( sTempColumnName,
                               sTempColumn->name,
                               idlOS::strlen(sTempColumn->name) );

                sTempColumnName[idlOS::strlen(sTempColumn->name)] = '\0';
            }

            if( idlOS::strMatch( sColumn->namePos.stmtText + sColumn->namePos.offset,
                                 sColumn->namePos.size,
                                 sTempColumnName,
                                 idlOS::strlen(sTempColumnName) ) == 0 )
            {
                // 테이블 컬럼의 basicInfo를 이용하여
                // 파티션 키 컬럼 정보를 구성한다.
                sColumn->basicInfo = sTempColumn->basicInfo;

                // -----------------------------------------------
                // 3. 파티션 키 컬럼의 데이터타입이
                //    대소 비교 가능한 타입인지 체크
                // -----------------------------------------------
                if( mtf::isGreaterLessValidType(
                        sTempColumn->basicInfo->module ) == ID_FALSE )
                {
                    sqlInfo.setSourceInfo(aStatement,
                                          & sTempColumn->namePos );

                    IDE_RAISE( ERR_INVALID_PARTITION_KEY_COLUMN_TYPE );
                }

                sIsFound = ID_TRUE;
                break;
            }
        }

        if (sIsFound == ID_FALSE )
        {
            sqlInfo.setSourceInfo(aStatement,
                                  & sColumn->namePos);

            IDE_RAISE(ERR_INVALID_PARTITION_KEY_COLUMN_NAME );
        }

        sPartKeyColCnt++;
    }

    // -----------------------------------------------
    // 4. 파티션 키 컬럼 개수 체크
    // -----------------------------------------------
    // RANGE, HASH인 경우, 테이블의 컬럼 개수보다 작고,
    // QC_MAX_KET_COLUMN_COUNT보다 작아야 한다.
    if( ( sPartTable->partMethod == QCM_PARTITION_METHOD_RANGE ) ||
        ( sPartTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) ||
        ( sPartTable->partMethod == QCM_PARTITION_METHOD_HASH ) )
    {
        IDE_TEST_RAISE( ( sPartKeyColCnt > QC_MAX_KEY_COLUMN_COUNT ) ||
                        ( sPartKeyColCnt > sTableColCnt ),
                        ERR_MAX_PARTITION_KEY_COLUMN_COUNT );
    }
    // LIST인 경우, 파티션 키 컬럼 개수는 반드시 1개여야 한다.
    else if( sPartTable->partMethod == QCM_PARTITION_METHOD_LIST )
    {
        IDE_TEST_RAISE( sPartKeyColCnt > 1,
                        ERR_MAX_PARTITION_KEY_COLUMN_COUNT );
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUPLICATE_PARTITION_KEY_COLUMN_NAME)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_PARTITION_NAME,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }
    IDE_EXCEPTION(ERR_INVALID_PARTITION_KEY_COLUMN_TYPE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_PARTITION_KEY_COLUMN_TYPE,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }
    IDE_EXCEPTION(ERR_INVALID_PARTITION_KEY_COLUMN_NAME)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_PARTITION_KEY_COLUMN_NAME,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

    }
    IDE_EXCEPTION(ERR_MAX_PARTITION_KEY_COLUMN_COUNT)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_MAX_PARTITION_KEY_COLUMN_COUNT ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateTBSOfPartition( qcStatement          * aStatement,
                                          qdPartitionAttribute * aPartAttr )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1502 PARTITIONED DISK TABLE
 *
 *    각 테이블 파티션의 TABLESPACE에 대한 validation
 *
 * Implementation :
 *    1. TABLESPACE 명시한 경우
 *      1.1 SM 에서 존재하는 테이블스페이스인지 체크
 *      1.2 테이블스페이스의 종류가 UNDO tablespace 또는
 *          temporary tablespace 이면 오류
 *
 *    2. TABLESPACE 명시하지 않은 경우
 *      2.1 USER_ID 로 SYS_USERS_ 검색해서 DEFAULT_TBS_ID 값을 읽어 테이블을
 *          위한 테이블스페이스로 지정
 *
 *    3. 동일 매채의 TABLESPACE인지 체크
 *       - PROJ-2464 hybrid partitioned table 지원
 *         HPT를 지원하기 위해서 제거하였다.
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree;
    smiTableSpaceAttr         sTBSAttr;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    // -----------------------------------------------
    // 1. TABLESPACE를 명시한 경우
    // -----------------------------------------------
    if ( QC_IS_NULL_NAME( aPartAttr->TBSName ) == ID_FALSE )
    {
        IDE_TEST( qcmTablespace::getTBSAttrByName(
                      aStatement,
                      aPartAttr->TBSName.stmtText + aPartAttr->TBSName.offset,
                      aPartAttr->TBSName.size,
                      &(aPartAttr->TBSAttr))
                  != IDE_SUCCESS );

        IDE_TEST_RAISE(
            aPartAttr->TBSAttr.mType == SMI_DISK_SYSTEM_TEMP ||
            aPartAttr->TBSAttr.mType == SMI_DISK_USER_TEMP ||
            aPartAttr->TBSAttr.mType == SMI_DISK_SYSTEM_UNDO,
            ERR_NO_CREATE_IN_SYSTEM_TBS );

        IDE_TEST( qdpRole::checkAccessTBS(
                      aStatement,
                      sParseTree->userID,
                      aPartAttr->TBSAttr.mID) != IDE_SUCCESS );
    }

    // -----------------------------------------------
    // 2. TABLESPACE를 명시하지 않은 경우
    // -----------------------------------------------
    // TABLE의 TABLESPACE를 따른다.
    // TABLE의 TBS는 qdbCreate::validateTablespace 함수에서
    // 이미 결정되어 있다.
    else
    {
        // CREATE TABLE
        // 파티션드 테이블의 TBS를 따른다.
        if( sParseTree->tableInfo == NULL )
        {
            aPartAttr->TBSName = sParseTree->TBSName;
            aPartAttr->TBSAttr = sParseTree->TBSAttr;
        }
        // ALTER TABLE ~ ADD PARTITION
        // ALTER TABLE ~ SPLIT PARTITION
        // ALTER TABLE ~ MERGE PARTITIONS
        // 새로 생성되는 파티션은 파티션드 테이블의 TBS를 따른다.
        else
        {
            IDE_TEST( qcmTablespace::getTBSAttrByID( sParseTree->tableInfo->TBSID,
                                                     & sTBSAttr )
                      != IDE_SUCCESS);

            aPartAttr->TBSAttr = sTBSAttr;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_CREATE_IN_SYSTEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateTBSOfTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      사용자가 지정한 Table의 Tablespace Name으로 Tablespace 속성을 얻는다.
 *
 * Implementation :
 *      1. System Tablespace를 지정할 수 없다.
 *      2. 해당 Tablespace에 대한 접근 권한이 있는지 확인한다.
 *      3. Tablespace가 기존과 달라야 한다.
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree = NULL;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( qcmTablespace::getTBSAttrByName(
                    aStatement,
                    sParseTree->TBSName.stmtText + sParseTree->TBSName.offset,
                    sParseTree->TBSName.size,
                    &(sParseTree->TBSAttr) )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( sParseTree->TBSAttr.mType == SMI_DISK_SYSTEM_TEMP ) ||
                    ( sParseTree->TBSAttr.mType == SMI_DISK_USER_TEMP ) ||
                    ( sParseTree->TBSAttr.mType == SMI_DISK_SYSTEM_UNDO ),
                    ERR_NO_CREATE_IN_SYSTEM_TBS );

    IDE_TEST( qdpRole::checkAccessTBS( aStatement,
                                       sParseTree->userID,
                                       sParseTree->TBSAttr.mID )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sParseTree->tableInfo->TBSID == sParseTree->TBSAttr.mID,
                    ERR_CANNOT_ALTER_TABLESPACE_ON_SAME_TABLESPACE );

    /* BUG-43398 Temporary Table은 Table Space를 변경할 수 없음  */
    IDE_TEST_RAISE( sParseTree->tableInfo->temporaryInfo.type != QCM_TEMPORARY_ON_COMMIT_NONE,
                    ERR_TEMPORARY_TABLE_ALTER );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_CREATE_IN_SYSTEM_TBS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS ) );
    }
    IDE_EXCEPTION( ERR_CANNOT_ALTER_TABLESPACE_ON_SAME_TABLESPACE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_ALTER_TABLESPACE_ON_SAME_TABLESPACE ) );
    }
    IDE_EXCEPTION( ERR_TEMPORARY_TABLE_ALTER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_ALTER_TABLESPACE_TEMPORARY_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::getPartitionMinMaxValue( qcStatement          * aStatement,
                                           qdPartitionAttribute * aPartAttr,
                                           UInt                   aPartCount,
                                           qcmPartitionMethod     aPartMethod,
                                           SChar                * aPartMaxVal,
                                           SChar                * aPartMinVal,
                                           SChar                * aOldPartMaxVal )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      파티션 키 조건 값을 이용하여PARTITION_MIN_VALUE와
 *      PARTITION_MAX_VALUE를 구한다.
 *
 * Implementation :
 *      1. 파티션 키 조건 값을 구한다.(qcNamePosition)
 *         첫번째와 마지막 파티션의 position을 이용
 *      2. 범위 파티션드 테이블
 *          2-1. PARTITION_MAX_VALUE
 *          2-2. PARTITION_MIN_VALUE(이전 파티션의 PARTITION_MAX_VALUE)
 *      3. 리스트 파티션드 테이블
 *          3-1. PARTITION_MAX_VALUE == PARTITION_MIN_VALUE
 *
 ***********************************************************************/

    qdTableParseTree       * sParseTree;
    qcNamePosition           sFirstCondValPos;
    qcNamePosition           sLastCondValPos;
    qcNamePosition           sTotalCondValPos;
    qmmValueNode           * sCondValNode;
    UInt                     sCondValCount = 0;
    qcNamePosList          * sNamePosList;
    qcNamePosition           sNamePos;
    UInt                     sAddSize = 0;
    UInt                     sBufferSize = 0;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    SET_EMPTY_POSITION( sFirstCondValPos );
    SET_EMPTY_POSITION( sLastCondValPos );

    idlOS::memset( aPartMaxVal, 0x00, QC_MAX_PARTKEY_COND_VALUE_LEN+1);
    idlOS::memset( aPartMinVal, 0x00, QC_MAX_PARTKEY_COND_VALUE_LEN+1);

    //---------------------------------------------------
    // 1. 파티션 키 조건 값을 구한다.
    //---------------------------------------------------
    // 첫번째 위치와 마지막 위치를 구해서 나중에 합친다.
    for( sCondValNode = aPartAttr->partKeyCond, sCondValCount = 0;
         sCondValNode != NULL;
         sCondValNode = sCondValNode->next, sCondValCount++ )
    {
        // 첫번째 조건 값의 position을 구한다.
        if( sCondValCount == 0 )
        {
            sFirstCondValPos = sCondValNode->value->position;
        }
        // 마지막 조건 값의 position을 구한다.
        else if( sCondValCount ==
                 ( aPartAttr->partCondVal.partCondValCount - 1 ) )
        {
            sLastCondValPos = sCondValNode->value->position;
        }
        else
        {
            // Nothing to do
        }
    }

    if( sCondValCount == 0 )
    {
        // default인 경우 이미 memset을 수행했으므로
        // 아무것도 하지 않는다.

        // Nothing to do.
    }
    else
    {
        // 파티션 조건 값이 하나일 경우에는 첫번째 조건 값의
        // position을 그대로 갖는다.
        if( sCondValCount == 1 )
        {
            SET_POSITION( sTotalCondValPos, sFirstCondValPos );
        }
        else
        {
            SET_POSITION( sTotalCondValPos, sFirstCondValPos );

            sTotalCondValPos.size = sLastCondValPos.offset -
                sFirstCondValPos.offset +
                sLastCondValPos.size;
        }


        // PROJ-1579 NCHAR
        // 메타테이블에 저장하기 위해
        // N 타입이 있는 경우 U 타입으로 변환한다.
        if( aPartAttr->ncharLiteralPos != NULL )
        {
            for( sNamePosList = aPartAttr->ncharLiteralPos;
                 sNamePosList != NULL;
                 sNamePosList = sNamePosList->next )
            {
                sNamePos = sNamePosList->namePos;

                // U 타입으로 변환하면서 늘어나는 사이즈 계산
                // N'안' => U'\C548' 으로 변환된다면
                // '안'의 캐릭터 셋이 KSC5601이라고 가정했을 때,
                // single-quote안의 문자는 2 byte -> 5byte로 변경된다.
                // 즉, 1.5배가 늘어나는 것이다.
                //(전체 사이즈가 아니라 증가하는 사이즈만 계산하는 것임)
                // 하지만, 어떤 예외적인 캐릭터 셋이 들어올지 모르므로
                // * 2로 충분히 잡는다.
                sAddSize += (sNamePos.size - 3) * 2;
            }

            sBufferSize = sTotalCondValPos.size + sAddSize;

            IDE_TEST_RAISE( sBufferSize > QC_MAX_PARTKEY_COND_VALUE_LEN,
                            ERR_TOO_LONG_PARTITION_CONDITION_VALUE );

            /*IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
              SChar,
              sBuferSize,
              & sPartMaxValBuffer )
              != IDE_SUCCESS); */

            IDE_TEST( qdbCommon::convertToUTypeString( aStatement,
                                                       sTotalCondValPos.offset,
                                                       sTotalCondValPos.size,
                                                       aPartAttr->ncharLiteralPos,
                                                       aPartMaxVal,
                                                       sBufferSize )
                      != IDE_SUCCESS );
        }
        else
        {
            idlOS::memcpy( aPartMaxVal,
                           sTotalCondValPos.stmtText + sTotalCondValPos.offset,
                           sTotalCondValPos.size );
            aPartMaxVal[sTotalCondValPos.size] = '\0';
        }
    }

    //---------------------------------------------------
    // 2. 범위 파티션드 테이블일 경우
    //    PARTITION_MIN_VALUE와 PARTITION_MAX_VALUE를 구한다.
    //    PARTITION_MIN_VALUE는 이전 파티션의 PARTITION_MAX_VALUE이다.
    //---------------------------------------------------
    if ( ( aPartMethod == QCM_PARTITION_METHOD_RANGE ) ||
         ( aPartMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
    {
        // 가장 작은 기준 값을 갖는  파티션의
        // PARTITION_MIN_VALUE는 NULL이다.
        if( aPartCount == 0 )
        {
            idlOS::memset( aPartMinVal,
                           0x00,
                           QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

            idlOS::memcpy( aOldPartMaxVal,
                           aPartMaxVal,
                           QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

            aOldPartMaxVal[idlOS::strlen(aPartMaxVal)] = '\0';
        }
        // empty와 기본 파티션의 PARTITION_MAX_VALUE는 Max 값이 NULL
        else if ( aPartCount == ( sParseTree->partTable->partCount - 1 ) )
        {
            idlOS::memcpy( aPartMinVal,
                           aOldPartMaxVal,
                           QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

            idlOS::memset( aPartMaxVal,
                           0x00,
                           QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );
        }
        // 파티션이 3개 이상일 경우, 중간에 있는 파티션은
        // PARTITION_MIN_VALUE와 PARTITION_MAX_VALUE를
        // 모두 갖는다.
        else
        {
            idlOS::memcpy( aPartMinVal,
                           aOldPartMaxVal,
                           QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

            idlOS::memcpy( aOldPartMaxVal,
                           aPartMaxVal,
                           QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

            aPartMinVal[idlOS::strlen(aPartMinVal)] = '\0';
            aOldPartMaxVal[idlOS::strlen(aOldPartMaxVal)] = '\0';
        }
    }
    //---------------------------------------------------
    // 3. 리스트 파티션드 테이블일 경우
    //    PARTITION_MIN_VALUE == PARTITION_MAX_VALUE
    //---------------------------------------------------
    else if( aPartMethod == QCM_PARTITION_METHOD_LIST )
    {
        idlOS::memcpy( aPartMinVal,
                       aPartMaxVal,
                       QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

        aPartMinVal[idlOS::strlen(aPartMinVal)] = '\0';
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_LONG_PARTITION_CONDITION_VALUE)
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QDB_TOO_LONG_PARTITION_CONDITION_VALUE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkSplitCond( qcStatement      * aStatement,
                                  qdTableParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      파티션 분할 시, 분할 기준 값의 validation
 *
 * Implementation :
 *      1. SrcPart의 PARTITION_MIN_VALUE 구하기
 *
 *      2. min value를 qmsPartCondValList로 만든다.
 *
 *      3. SrcPart의 PARTITION_MAX_VALUE 구하기
 *
 *      4. max value를 qmsPartCondValList로 만든다.
 *
 *      5. 분할 기준 값을 qmsPartCondValList로 만든다.
 *
 *      6. 범위 파티션드 테이블이면
 *          6-1. 분할 기준 값이 SrcPart의 PARTITION_MIN_VALUE보다 커야 한다.
 *          6-2. 분할 기준 값이 SrcPart의 PARTITION_MAX_VALUE보다 작아야 한다.
 *
 *      7. 리스트 파티션드 테이블이면
 *          7-1. 파티션 분할 기준 값이 지정한 파티션 조건 값에 속하는지 체크
 *          7-2. 파티션 분할 기준 값의 개수 체크
 *               (  1 ~ ( 해당 파티션의 파티션 조건 값의 개수-1) 인지 체크 )
 *
 ***********************************************************************/

    qmsPartCondValList   * sPartCondMinVal;
    qmsPartCondValList   * sPartCondMaxVal;
    mtdCharType          * sPartKeyCondMinValueStr;
    mtdCharType          * sPartKeyCondMaxValueStr;

    qmsPartCondValList   * sSplitCondVal;
    qdPartitionAttribute * sSrcPartAttr;
    qcmTableInfo         * sTableInfo;
    UInt                   sMaxValCount;
    UInt                   i;
    UInt                   j;
    UInt                   sFoundCount = 0;
    mtdCompareFunc         sCompare;
    mtdValueInfo           sValueInfo1;
    mtdValueInfo           sValueInfo2;

    sTableInfo = aParseTree->tableInfo;
    sSrcPartAttr = aParseTree->partTable->partAttr;

    sCompare =
        sTableInfo->partKeyColumns->basicInfo->module->logicalCompare[MTD_COMPARE_ASCENDING];

    IDU_LIMITPOINT("qdbCommon::checkSplitCond::malloc");
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsPartCondValList,
                            & sSplitCondVal )
              != IDE_SUCCESS );

    // SrcPart에 대해 파스트리에 달아놓은 값을 가져온다.
    sPartKeyCondMinValueStr = sSrcPartAttr->alterPart->partKeyCondMinValStr;
    sPartKeyCondMaxValueStr = sSrcPartAttr->alterPart->partKeyCondMaxValStr;
    sPartCondMinVal         = sSrcPartAttr->alterPart->partCondMinVal;
    sPartCondMaxVal         = sSrcPartAttr->alterPart->partCondMaxVal;

    // ---------------------------------------------------
    // 분할 기준 값을 qmsPartCondValList로 만든다.
    // ---------------------------------------------------

    IDE_TEST( makePartKeyCondValues( aStatement,
                                     QC_SHARED_TMPLATE(aStatement),
                                     sTableInfo->partKeyColumns,
                                     sTableInfo->partitionMethod,
                                     sSrcPartAttr->partKeyCond,
                                     sSplitCondVal )
              != IDE_SUCCESS );

    // 분할 기준 값을 파스 트리에 달아놓는다.
    sSrcPartAttr->alterPart->splitCondVal = sSplitCondVal;

    switch ( sTableInfo->partitionMethod )
    {
        // ---------------------------------------------------
        // 6. 범위 파티션드 테이블이면
        // ---------------------------------------------------
        case QCM_PARTITION_METHOD_RANGE:
            {
                // PARTITION_MIN_VALUE가 없는 파티션은
                // MIN_VALUE와 비교하지 않는다.
                if( sPartKeyCondMinValueStr->length != 0 )
                {
                    // ---------------------------------------------------
                    // 6-1. 분할 기준 값이
                    //      SrcPart의 PARTITION_MIN_VALUE보다 커야 한다.
                    // ---------------------------------------------------
                    IDE_TEST_RAISE( qmoPartition::compareRangePartition(
                                        sTableInfo->partKeyColumns,
                                        sPartCondMinVal,
                                        sSplitCondVal ) != -1,
                                    ERR_SPLIT_COND_VALUE_ON_RANGE_PARTITION );
                }

                // PARTITION_MAX_VALUE가 없는 파티션(기본 파티션)은
                // MAX_VALUE와 비교하지 않는다.
                if( sPartKeyCondMaxValueStr->length != 0 )
                {
                    // ---------------------------------------------------
                    // 6-2. 분할 기준 값이
                    //      SrcPart의 PARTITION_MAX_VALUE보다 작아야 한다.
                    // ---------------------------------------------------
                    IDE_TEST_RAISE( qmoPartition::compareRangePartition(
                                        sTableInfo->partKeyColumns,
                                        sPartCondMaxVal,
                                        sSplitCondVal ) != 1,
                                    ERR_SPLIT_COND_VALUE_ON_RANGE_PARTITION );
                }
            }
            break;
        /* BUG-46065 support range using hash */
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
            {
                // PARTITION_MIN_VALUE가 없는 파티션은
                // MIN_VALUE와 비교하지 않는다.
                if ( sPartKeyCondMinValueStr->length != 0 )
                {
                    // ---------------------------------------------------
                    // 6-1. 분할 기준 값이
                    //      SrcPart의 PARTITION_MIN_VALUE보다 커야 한다.
                    // ---------------------------------------------------
                    IDE_TEST_RAISE( qmoPartition::compareRangeUsingHashPartition(
                                        sPartCondMinVal,
                                        sSplitCondVal ) != -1,
                                    ERR_SPLIT_COND_VALUE_ON_RANGE_PARTITION );
                }

                // PARTITION_MAX_VALUE가 없는 파티션(기본 파티션)은
                // MAX_VALUE와 비교하지 않는다.
                if ( sPartKeyCondMaxValueStr->length != 0 )
                {
                    // ---------------------------------------------------
                    // 6-2. 분할 기준 값이
                    //      SrcPart의 PARTITION_MAX_VALUE보다 작아야 한다.
                    // ---------------------------------------------------
                    IDE_TEST_RAISE( qmoPartition::compareRangeUsingHashPartition(
                                        sPartCondMaxVal,
                                        sSplitCondVal ) != 1,
                                    ERR_SPLIT_COND_VALUE_ON_RANGE_PARTITION );
                }
            }
            break;
            // ---------------------------------------------------
            // 7. 리스트 파티션드 테이블이면
            // ---------------------------------------------------
        case QCM_PARTITION_METHOD_LIST:
            {
                // ---------------------------------------------------
                // 같은 분할 기준 값을 여러번 지정하면 에러
                // ---------------------------------------------------
                for( i = 0;
                     i < sSplitCondVal->partCondValCount;
                     i++ )
                {
                    for( j = 0;
                         j < sSplitCondVal->partCondValCount;
                         j++ )
                    {
                        if( i == j )
                        {
                            continue;
                        }

                        sValueInfo1.column =
                            sTableInfo->partKeyColumns->basicInfo;
                        sValueInfo1.value  =
                            sSplitCondVal->partCondValues[i];
                        sValueInfo1.flag   =
                            MTD_OFFSET_USELESS;

                        sValueInfo2.column =
                            sTableInfo->partKeyColumns->basicInfo;
                        sValueInfo2.value  =
                            sSplitCondVal->partCondValues[j];
                        sValueInfo2.flag   =
                            MTD_OFFSET_USELESS;


                        if( sCompare( &sValueInfo1, &sValueInfo2 ) == 0 )
                        {
                            IDE_RAISE(
                                ERR_DUP_SPLIT_COND_VALUE_ON_LIST_PARTITION);
                        }
                    }
                }

                // 기본 파티션의 분할이 아닌 경우에는
                // SrcPart의 파티션 조건 값만 비교하면 된다.
                if( sPartKeyCondMinValueStr->length != 0 )
                {
                    sFoundCount = 0;

                    // ---------------------------------------------------
                    // 7-1. 파티션 분할 기준 값이
                    //      지정한 파티션 조건 값에 속하는지 체크
                    // ---------------------------------------------------
                    for( sMaxValCount = 0;
                         sMaxValCount < sPartCondMaxVal->partCondValCount;
                         sMaxValCount++ )
                    {
                        if( qmoPartition::compareListPartition(
                                sTableInfo->partKeyColumns,
                                sSplitCondVal,
                                sPartCondMaxVal->partCondValues[sMaxValCount] )
                            == ID_TRUE )
                        {
                            sFoundCount++;
                        }
                    }

                    IDE_TEST_RAISE( sFoundCount != sSplitCondVal->partCondValCount,
                                    ERR_NOT_EXIST_SPLIT_COND_VALUE_ON_LIST_PARTITION );

                    // ---------------------------------------------------
                    // 7-2. 파티션 분할 기준 값의 개수가
                    //      1 ~ ( 해당 파티션의 파티션 조건 값의 개수-1)인지 체크
                    // ---------------------------------------------------
                    IDE_TEST_RAISE( ( sSplitCondVal->partCondValCount >=
                                      sPartCondMaxVal->partCondValCount ),
                                    ERR_TOO_MANY_SPLIT_COND_VALUES);
                }
                // 기본 파티션을 분할할 경우에는
                // 해당 테이블의 모든 파티션의 파티션 조건 값을 체크해봐야 한다.
                else
                {
                    IDE_TEST( qdbCommon::checkSplitCondOfDefaultPartition(
                                  aStatement,
                                  sTableInfo,
                                  sSplitCondVal )
                              != IDE_SUCCESS );
                }
            }
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SPLIT_COND_VALUE_ON_RANGE_PARTITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_SPLIT_CONDITION_VALUE_ON_RANGE_PARTITION));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_SPLIT_COND_VALUE_ON_LIST_PARTITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOT_EXIST_SPLIT_CONDITION_VALUE_ON_LIST_PARTITION));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_SPLIT_COND_VALUES)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_TOO_MANY_SPLIT_CONDITION_VALUE));
    }
    IDE_EXCEPTION(ERR_DUP_SPLIT_COND_VALUE_ON_LIST_PARTITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_DUP_SPLIT_COND_VALUE_ON_LIST_PARTITION));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::createTablePartition( qcStatement          * aStatement,
                                        qdTableParseTree     * aParseTree,
                                        qcmTableInfo         * aTableInfo,
                                        qdPartitionAttribute * aPartAttr,
                                        SChar                * aPartMinVal,
                                        SChar                * aPartMaxVal,
                                        UInt                   aPartOrder,
                                        UInt                 * aPartitionID,
                                        smOID                * aPartitionOID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      테이블 파티션을 생성하는 함수이다.
 *
 *      ALTER TABLE SPLIT PARTITION, MERGE PARTITIONS,
 *                  ADD PARTITION, TRUNCATE PARTITION
 *      에서 사용된다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt              sPartitionID;
    smOID             sPartitionOID;
    UInt              sOrgTableFlag;
    UInt              sOrgTableParallelDegree;
    UInt              sPartType;
    smiSegAttr        sSegAttr;
    smiSegStorageAttr sSegStoAttr;

    // 원본 테이블과 같은 Flag와 Parallel Degree를 가짐
    sOrgTableFlag           = aParseTree->tableInfo->tableFlag;
    sOrgTableParallelDegree = aParseTree->tableInfo->parallelDegree;
    sPartType               = getTableTypeFromTBSID( aPartAttr->TBSAttr.mID );

    //-----------------------------------------------------
    // 1-3. 파티션 생성
    //-----------------------------------------------------

    IDE_TEST(qcmPartition::getNextTablePartitionID(
                 aStatement,
                 & sPartitionID) != IDE_SUCCESS);

    /* PROJ-2464 hybrid partitioned table 지원
     *  - Partition Info를 구성할 때에, Table Option을 Partitioned Table의 값으로 복제한다.
     *  - 따라서, PartInfo의 정보를 이용하지 않고, TBSID에 따라 적합한 값으로 조정해서 이용한다.
     */
    adjustPhysicalAttr( sPartType,
                        aTableInfo->segAttr,
                        aTableInfo->segStoAttr,
                        & sSegAttr,
                        & sSegStoAttr,
                        ID_TRUE /* aIsTable */ );

    IDE_TEST(
        qdbCommon::createTableOnSM(
            aStatement,
            aPartAttr->columns,
            aParseTree->userID,
            aTableInfo->tableID,
            aTableInfo->maxrows,
            aPartAttr->TBSAttr.mID,
            sSegAttr,
            sSegStoAttr,
            QDB_TABLE_ATTR_MASK_ALL,
            sOrgTableFlag, /* Flag Value */
            sOrgTableParallelDegree,
            & sPartitionOID)
        != IDE_SUCCESS);

    //-----------------------------------------------------
    // 1-4. SYS_TABLE_PARTITIONS_ 메타 테이블에 파티션 정보 입력
    //-----------------------------------------------------
    IDE_TEST(
        qdbCommon::insertTablePartitionSpecIntoMeta(
            aStatement,
            aParseTree->userID,
            aTableInfo->tableID,
            sPartitionOID,
            sPartitionID,
            aPartAttr->tablePartName,
            aPartMinVal,
            aPartMaxVal,
            aPartOrder,
            aPartAttr->TBSAttr.mID,
            aPartAttr->accessOption,   /* PROJ-2359 Table/Partition Access Option */
            aPartAttr->mIsUsable )
        != IDE_SUCCESS);

    //-----------------------------------------------------
    // 1-5. SYS_PART_LOBS_ 메타 테이블에 파티션 정보 입력
    //-----------------------------------------------------
    IDE_TEST(qdbCommon::insertPartLobSpecIntoMeta(aStatement,
                                                  aParseTree->userID,
                                                  aTableInfo->tableID,
                                                  sPartitionID,
                                                  aPartAttr->columns )
             != IDE_SUCCESS);

    *aPartitionID = sPartitionID;
    *aPartitionOID = sPartitionOID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::getPartCondValueFromParseTree(
    qcStatement          * aStatement,
    qdPartitionAttribute * aPartAttr,
    SChar                * aPartVal )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      qdbAlter::executeSplitPartition()에서 호출되는 함수로
 *      파티션 분할 조건 값을 구한다.
 *
 *      테이블 파스 트리에 있는 partKeyCond를 이용하여
 *      PARTITION_MAX_VALUE 또는 PARTITION_MIN_VALUE를 구한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qcNamePosition           sFirstCondValPos;
    qcNamePosition           sLastCondValPos;
    qcNamePosition           sTotalCondValPos;
    qmmValueNode           * sCondValNode;
    UInt                     sCondValCount = 0;
    UInt                     sTotalCondValCount = 0;
    qcNamePosList          * sNamePosList;
    qcNamePosition           sNamePos;
    UInt                     sAddSize = 0;
    UInt                     sBufferSize = 0;

    SET_EMPTY_POSITION( sFirstCondValPos );
    SET_EMPTY_POSITION( sLastCondValPos );

    for( sCondValNode = aPartAttr->partKeyCond;
         sCondValNode != NULL;
         sCondValNode = sCondValNode->next )
    {
        sTotalCondValCount++;
    }

    //---------------------------------------------------
    // 1. 파티션 키 조건 값을 구한다.
    //---------------------------------------------------
    // 첫번째 위치와 마지막 위치를 구해서 나중에 합친다.
    for( sCondValNode = aPartAttr->partKeyCond, sCondValCount = 0;
         sCondValNode != NULL;
         sCondValNode = sCondValNode->next, sCondValCount++ )
    {
        // 첫번째 조건 값의 position을 구한다.
        if( sCondValCount == 0 )
        {
            sFirstCondValPos = sCondValNode->value->position;
        }
        // 마지막 조건 값의 position을 구한다.
        else if( sCondValCount == sTotalCondValCount - 1 )
        {
            sLastCondValPos = sCondValNode->value->position;
        }
        else
        {
            // Nothing to do
        }
    }

    if( sCondValCount == 0 )
    {
        // default인 경우 null로 만든다.
        aPartVal[0] = '\0';
    }
    else
    {
        // 파티션 조건 값이 하나일 경우에는 첫번째 조건 값의
        // position을 그대로 갖는다.
        if( sCondValCount == 1 )
        {
            SET_POSITION( sTotalCondValPos, sFirstCondValPos );
        }
        else
        {
            SET_POSITION( sTotalCondValPos, sFirstCondValPos );

            sTotalCondValPos.size = sLastCondValPos.offset -
                sFirstCondValPos.offset +
                sLastCondValPos.size;
        }

        // PROJ-1579 NCHAR
        // 메타테이블에 저장하기 위해 스트링을 분할하기 전에
        // N 타입이 있는 경우 U 타입으로 변환한다.
        if( aPartAttr->ncharLiteralPos != NULL )
        {
            for( sNamePosList = aPartAttr->ncharLiteralPos;
                 sNamePosList != NULL;
                 sNamePosList = sNamePosList->next )
            {
                sNamePos = sNamePosList->namePos;

                // U 타입으로 변환하면서 늘어나는 사이즈 계산
                // N'안' => U'\C548' 으로 변환된다면
                // '안'의 캐릭터 셋이 KSC5601이라고 가정했을 때,
                // single-quote안의 문자는 2 byte -> 5byte로 변경된다.
                // 즉, 1.5배가 늘어나는 것이다.
                //(전체 사이즈가 아니라 증가하는 사이즈만 계산하는 것임)
                // 하지만, 어떤 예외적인 캐릭터 셋이 들어올지 모르므로
                // * 2로 충분히 잡는다.
                sAddSize += (sNamePos.size - 3) * 2;
            }

            sBufferSize = sTotalCondValPos.size + sAddSize;

            IDE_TEST_RAISE( sBufferSize > QC_MAX_PARTKEY_COND_VALUE_LEN,
                            ERR_TOO_LONG_PARTITION_CONDITION_VALUE );

            /*IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
              SChar,
              sBuferSize,
              & sPartMaxValBuffer )
              != IDE_SUCCESS); */

            IDE_TEST( qdbCommon::convertToUTypeString(
                          aStatement,
                          sTotalCondValPos.offset,
                          sTotalCondValPos.size,
                          aPartAttr->ncharLiteralPos,
                          aPartVal,
                          sBufferSize )
                      != IDE_SUCCESS );
        }
        else
        {
            idlOS::memcpy( aPartVal,
                           sTotalCondValPos.stmtText + sTotalCondValPos.offset,
                           sTotalCondValPos.size );
            aPartVal[sTotalCondValPos.size] = '\0';
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_LONG_PARTITION_CONDITION_VALUE)
    {
        IDE_SET(ideSetErrorCode(
                    qpERR_ABORT_QDB_TOO_LONG_PARTITION_CONDITION_VALUE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updatePartMinValueOfTablePartMeta(
    qcStatement          * aStatement,
    UInt                   aPartitionID,
    SChar                * aPartMinValue )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      SYS_TABLE_PARTITIONS_ 메타 테이블의
 *      PARTITION_MIN_VALUE를 업데이트한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                 * sSqlStr;
    vSLong                  sRowCnt;
    SChar                 * sPartValue;

    IDU_LIMITPOINT("qdbCommon::updatePartMinValueOfTablePartMeta::malloc1");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::updatePartMinValueOfTablePartMeta::malloc2");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN * 2,
                                    & sPartValue)
             != IDE_SUCCESS);

    // 문자열 안에 '가 있을 경우 '를 하나 더 넣어줘야함
    IDE_TEST( addSingleQuote4PartValue( aPartMinValue,
                                        idlOS::strlen(aPartMinValue),
                                        & sPartValue )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLE_PARTITIONS_ "
                     "SET PARTITION_MIN_VALUE = CHAR'%s', "
                     "    LAST_DDL_TIME = SYSDATE "
                     "WHERE PARTITION_ID =  INTEGER'%"ID_INT32_FMT"' ",
                     sPartValue,
                     aPartitionID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updatePartMaxValueOfTablePartMeta(
    qcStatement          * aStatement,
    UInt                   aPartitionID,
    SChar                * aPartMaxValue )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      SYS_TABLE_PARTITIONS_ 메타 테이블의
 *      PARTITION_MAX_VALUE를 업데이트한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                 * sSqlStr;
    vSLong                  sRowCnt;
    SChar                 * sPartValue;

    IDU_LIMITPOINT("qdbCommon::updatePartMaxValueOfTablePartMeta::malloc1");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::updatePartMaxValueOfTablePartMeta::malloc2");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                    & sPartValue)
             != IDE_SUCCESS);

    // 문자열 안에 '가 있을 경우 '를 하나 더 넣어줘야함
    IDE_TEST( addSingleQuote4PartValue( aPartMaxValue,
                                        idlOS::strlen(aPartMaxValue),
                                        & sPartValue )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLE_PARTITIONS_ "
                     "SET PARTITION_MAX_VALUE = CHAR'%s', "
                     "    LAST_DDL_TIME = SYSDATE "
                     "WHERE PARTITION_ID =  INTEGER'%"ID_INT32_FMT"' ",
                     sPartValue,
                     aPartitionID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updatePartNameOfTablePartMeta(
    qcStatement          * aStatement,
    UInt                   aPartitionID,
    SChar                * aPartName )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      SYS_TABLE_PARTITIONS_ 메타 테이블의
 *      PARTITION_NAME을 업데이트한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                 * sSqlStr;
    vSLong                  sRowCnt;

    IDU_LIMITPOINT("qdbCommon::updatePartNameOfTablePartMeta::malloc");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QD_MAX_SQL_LENGTH,
                                    &sSqlStr)
             != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLE_PARTITIONS_ "
                     "SET PARTITION_NAME = VARCHAR'%s', "
                     "    LAST_DDL_TIME = SYSDATE "
                     "WHERE PARTITION_ID =  INTEGER'%"ID_INT32_FMT"' ",
                     aPartName,
                     aPartitionID );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_META_CRASH);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::moveRowForInplaceAlterPartition(
    qcStatement          * aStatement,
    void                 * aSrcTable,
    void                 * aDstTable,
    qcmTableInfo         * aSrcPart,
    qcmTableInfo         * aDstPart,
    qdIndexTableList     * aIndexTables,
    qdSplitMergeType       aSplitType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      SPLIT PARTITION 시, SrcPart에서 DstPart로 데이터를 이동시킨다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;
    const void          * sOldRow;
    smiValue            * sNewRow;
    SInt                  sColumnCount = 0;
    qcmColumn           * sDestCol;
    UInt                  sTableType;
    SInt                  sStage = 0;
    smiTableCursor        sSrcTblCursor;
    smiTableCursor        sDstTblCursor;
    iduMemoryStatus       sQmxMemStatus;
    UInt                  sRowSize;
    scGRID                sGRID;
    smiCursorProperties   sSrcCursorProperty;
    smiCursorProperties   sDstCursorProperty;
    qmxLobInfo          * sLobInfo = NULL;
    void                * sInsRow;
    scGRID                sInsGRID;
    idBool                sIsMoveRow;
    qcmColumn           * sSrcCol;
    UInt                  sSrcColCount = 0;
    smiFetchColumnList  * sFetchColumnList = NULL;
    qdIndexTableCursors   sIndexTableCursorInfo;
    idBool                sInitedCursorInfo = ID_FALSE;
    smOID                 sPartOID          = SMI_NULL_OID;
    ULong                 sProgressCnt      = 0;

    // PROJ-1362
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sTableType = aSrcPart->tableFlag & SMI_TABLE_TYPE_MASK;
    
    // PROJ-1362
    // lob-locator를 저장할 공간을 할당한다.
    IDE_TEST( qmx::initializeLobInfo(
                  aStatement,
                  & sLobInfo,
                  (UShort)sParseTree->tableInfo->lobColumnCount)
              != IDE_SUCCESS );

    /* PROJ-2334 PMT */
    if ( sTableType == SMI_TABLE_DISK )
    {
        // Disk Table인 경우
        // Record Read를 위한 공간을 할당한다.
        IDE_TEST( qdbCommon::getDiskRowSize( aSrcTable,
                                             & sRowSize )
                  != IDE_SUCCESS );
    
        // To fix BUG-14820
        // Disk-variable 컬럼의 rid비교를 위해 초기화 해야 함.
        IDU_LIMITPOINT("qdbCommon::moveRowForInplaceAlterPartition::malloc1");
        IDE_TEST( aStatement->qmxMem->cralloc( sRowSize,
                                               (void **) & sOldRow )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    sSrcTblCursor.initialize();
    sDstTblCursor.initialize();

    //------------------------------------------
    // open src table.
    //------------------------------------------

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN(&sSrcCursorProperty, aStatement->mStatistics);

    sSrcCursorProperty.mLockWaitMicroSec = 0;

    //------------------------------------------
    // PROJ-1705 fetch column list 구성
    //------------------------------------------
    /* PROJ-2334 PMT */
    if ( sTableType == SMI_TABLE_DISK )
    {
        
        IDE_TEST( qdbCommon::makeFetchColumnList( QC_PRIVATE_TMPLATE(aStatement),
                                                  aSrcPart->columnCount,
                                                  aSrcPart->columns,
                                                  ID_TRUE,  // alloc smiColumnList
                                                  & sFetchColumnList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    sSrcCursorProperty.mFetchColumnList = sFetchColumnList;

    IDE_TEST(sSrcTblCursor.open(QC_SMI_STMT( aStatement ),
                                aSrcTable,
                                NULL,
                                smiGetRowSCN(aSrcTable),
                                NULL,
                                smiGetDefaultKeyRange(),
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_WRITE|
                                SMI_TRAVERSE_FORWARD|
                                SMI_PREVIOUS_DISABLE,
                                SMI_DELETE_CURSOR,
                                &sSrcCursorProperty )
             != IDE_SUCCESS);
    sStage = 1;

    // open destination table.
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN(&sDstCursorProperty, aStatement->mStatistics);

    if( (sParseTree->partTable->partAttr->alterPart->alterType ==
         QD_SPLIT_RANGE_PARTITION) ||
        (sParseTree->partTable->partAttr->alterPart->alterType ==
         QD_SPLIT_LIST_PARTITION) )
    {
        sDstCursorProperty.mIsUndoLogging = ID_FALSE;
    }
    else
    {
        sDstCursorProperty.mIsUndoLogging = ID_TRUE;
    }

    sDstCursorProperty.mReadRecordCount = ID_ULONG_MAX;
    sDstCursorProperty.mStatistics = aStatement->mStatistics;

    IDE_TEST(sDstTblCursor.open(QC_SMI_STMT( aStatement ),
                                aDstTable,
                                NULL,
                                smiGetRowSCN(aDstTable),
                                NULL,
                                smiGetDefaultKeyRange(),
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_WRITE|
                                SMI_TRAVERSE_FORWARD|
                                SMI_PREVIOUS_DISABLE,
                                SMI_INSERT_CURSOR,
                                & sDstCursorProperty )
             != IDE_SUCCESS);
    sStage += 2;

    // move..
    IDE_TEST(sSrcTblCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sSrcTblCursor.readRow( &sOldRow,
                                    &sGRID,
                                    SMI_FIND_NEXT)
             != IDE_SUCCESS);

    //------------------------------------------
    // init index table insert cursors
    //------------------------------------------
        
    if ( aIndexTables != NULL )
    {
        IDE_TEST( qdx::initializeUpdateIndexTableCursors(
                      aStatement,
                      aIndexTables,
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );

        sInitedCursorInfo = ID_TRUE;
        
        sPartOID = smiGetTableId(aDstTable);
    }
    else
    {
        // Nothing to do.
    }

    /* ------------------------------------------------
     * make new row values
     * ----------------------------------------------*/
    sColumnCount = 0;
    sDestCol = aDstPart->columns;

    while (sDestCol != NULL)
    {
        sColumnCount ++;
        sDestCol = sDestCol->next;
    }

    IDU_LIMITPOINT("qdbCommon::moveRowForInplaceAlterPartition::malloc2");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) *
                                       sColumnCount,
                                       (void**)&sNewRow)
             != IDE_SUCCESS);

    if( (sParseTree->partTable->partAttr->alterPart->alterType ==
         QD_SPLIT_RANGE_PARTITION) ||
        (sParseTree->partTable->partAttr->alterPart->alterType ==
         QD_SPLIT_LIST_PARTITION) )
    {
        for( sSrcCol = aSrcPart->columns; sSrcCol != NULL; sSrcCol = sSrcCol->next )
        {
            sSrcColCount++;
        }

        while (sOldRow != NULL)
        {
            // To Fix PR-11704
            // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
            // Memory 재사용을 위하여 현재 위치 기록
            IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                      != IDE_SUCCESS);

            (void)qmx::clearLobInfo( sLobInfo );

            // 해당 Row가 DstPart로 이동할지 판단한다.
            IDE_TEST( isMoveRowForAlterPartition( aStatement,
                                                  aSrcPart,
                                                  (void *)sOldRow,
                                                  aSrcPart->columns,
                                                  & sIsMoveRow,
                                                  aSplitType )
                      != IDE_SUCCESS );

            // PROJ-1502
            if( sIsMoveRow == ID_TRUE )
            {
                IDE_TEST(qdbAlter::makeNewRow(QC_PRIVATE_TMPLATE(aStatement),
                                              NULL,
                                              aSrcPart->columns,
                                              aDstPart->columns,
                                              sOldRow,
                                              sNewRow,
                                              & sSrcTblCursor,
                                              sGRID,
                                              sLobInfo,
                                              NULL)
                         != IDE_SUCCESS);

                IDE_TEST(sDstTblCursor.insertRow(sNewRow,
                                                 & sInsRow,
                                                 & sInsGRID)
                         != IDE_SUCCESS);

                // PROJ-1362
                // insertRow를 수행후 Lob 컬럼을 처리
                IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                      sLobInfo,
                                                      & sDstTblCursor,
                                                      sInsRow,
                                                      sInsGRID )
                          != IDE_SUCCESS );

                // PROJ-1624 non-partitioned index
                // move를 수행후 non-partitioned index를 처리
                if ( aIndexTables != NULL )
                {
                    IDE_TEST( qdx::updateIndexTableCursors( aStatement,
                                                            &sIndexTableCursorInfo,
                                                            sPartOID,
                                                            sGRID,
                                                            sInsGRID )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }
                
                IDE_TEST(sSrcTblCursor.deleteRow()
                         != IDE_SUCCESS);
            }
            else
            {
                // 이동하지 않아야 하는 Row
                // Nothing to do
            }

            // BUG-42920 DDL print data move progress            
            IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                            aSrcPart,
                                                            &sProgressCnt,
                                                            ID_FALSE )
                      != IDE_SUCCESS );
            
            // To Fix PR-11704
            // Memory 재사용을 위한 Memory 이동
            IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                      != IDE_SUCCESS);

            IDE_TEST(sSrcTblCursor.readRow(&sOldRow, &sGRID, SMI_FIND_NEXT)
                     != IDE_SUCCESS);
        }
    }
    else if( sParseTree->partTable->partAttr->alterPart->alterType ==
             QD_MERGE_PARTITION )
    {
        for( sSrcCol = aSrcPart->columns; sSrcCol != NULL; sSrcCol = sSrcCol->next )
        {
            sSrcColCount++;
        }

        while (sOldRow != NULL)
        {
            // To Fix PR-11704
            // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
            // Memory 재사용을 위하여 현재 위치 기록
            IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                      != IDE_SUCCESS);

            (void)qmx::clearLobInfo( sLobInfo );

            IDE_TEST(qdbAlter::makeNewRow(QC_PRIVATE_TMPLATE(aStatement),
                                          NULL,
                                          aSrcPart->columns,
                                          aDstPart->columns,
                                          sOldRow,
                                          sNewRow,
                                          & sSrcTblCursor,
                                          sGRID,
                                          sLobInfo,
                                          NULL)
                     != IDE_SUCCESS);

            IDE_TEST(sDstTblCursor.insertRow(sNewRow,
                                             & sInsRow,
                                             & sInsGRID)
                     != IDE_SUCCESS);

            // PROJ-1362
            // insertRow를 수행후 Lob 컬럼을 처리
            IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                  sLobInfo,
                                                  & sDstTblCursor,
                                                  sInsRow,
                                                  sInsGRID )
                      != IDE_SUCCESS );

            // PROJ-1624 non-partitioned index
            // move를 수행후 non-partitioned index를 처리
            if ( aIndexTables != NULL )
            {
                IDE_TEST( qdx::updateIndexTableCursors( aStatement,
                                                        &sIndexTableCursorInfo,
                                                        sPartOID,
                                                        sGRID,
                                                        sInsGRID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // BUG-42920 DDL print data move progress            
            IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                            aSrcPart,
                                                            &sProgressCnt,
                                                            ID_FALSE )
                      != IDE_SUCCESS );
            
            // To Fix PR-11704
            // Memory 재사용을 위한 Memory 이동
            IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                      != IDE_SUCCESS);

            IDE_TEST(sSrcTblCursor.readRow(&sOldRow, &sGRID, SMI_FIND_NEXT)
                     != IDE_SUCCESS);
        }
    }
    else
    {
        // 이 함수는 split partition, merge partition에서만 호출된다.
        IDE_DASSERT(0);
    }

    // BUG-42920 DDL print data move progress
    IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                    aSrcPart,
                                                    &sProgressCnt,
                                                    ID_TRUE )
              != IDE_SUCCESS );
    // cursor close.
    sStage = 1;
    IDE_TEST(sDstTblCursor.close() != IDE_SUCCESS);
    sStage = 0;
    IDE_TEST(sSrcTblCursor.close() != IDE_SUCCESS);

    // close index table cursor
    if ( aIndexTables != NULL )
    {
        IDE_TEST( qdx::closeUpdateIndexTableCursors(
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( sStage == 1 ) || ( sStage == 3 ) )
    {
        sSrcTblCursor.close();
    }
    if (sStage > 1)
    {
        sDstTblCursor.close();
    }

    if ( sInitedCursorInfo == ID_TRUE )
    {
        qdx::finalizeUpdateIndexTableCursors(
            &sIndexTableCursorInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    (void)qmx::finalizeLobInfo( aStatement, sLobInfo );

    return IDE_FAILURE;
}

IDE_RC qdbCommon::moveRowForOutplaceSplitPartition(
    qcStatement          * aStatement,
    void                 * aSrcTable,
    void                 * aDstTable1,
    void                 * aDstTable2,
    qcmTableInfo         * aSrcPart,
    qcmTableInfo         * aDstPart1,
    qcmTableInfo         * aDstPart2,
    qdIndexTableList     * aIndexTables,
    qdSplitMergeType       aSplitType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      SPLIT PARTITION 시, SrcPart에서 DstPart로 데이터를 이동시킨다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;
    const void          * sOldRow;
    smiValue            * sNewRow;
    SInt                  sColumnCount = 0;
    qcmColumn           * sDestCol;
    UInt                  sTableType;
    SInt                  sStage = 0;
    smiTableCursor        sSrcTblCursor;
    smiTableCursor        sDstTblCursor1;
    smiTableCursor        sDstTblCursor2;
    iduMemoryStatus       sQmxMemStatus;
    UInt                  sRowSize;
    scGRID                sGRID;
    smiCursorProperties   sSrcCursorProperty;
    smiCursorProperties   sDstCursorProperty1;
    smiCursorProperties   sDstCursorProperty2;
    qmxLobInfo          * sLobInfo = NULL;
    void                * sInsRow;
    scGRID                sInsGRID;
    idBool                sIsMoveRow;
    smiFetchColumnList  * sFetchColumnList = NULL;
    qdIndexTableCursors   sIndexTableCursorInfo;
    idBool                sInitedCursorInfo = ID_FALSE;
    smOID                 sPartOID1         = SMI_NULL_OID;
    smOID                 sPartOID2         = SMI_NULL_OID;
    ULong                 sProgressCnt      = 0;

    // PROJ-1362
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sTableType = aSrcPart->tableFlag & SMI_TABLE_TYPE_MASK;
    
    // PROJ-1362
    // lob-locator를 저장할 공간을 할당한다.
    IDE_TEST( qmx::initializeLobInfo(
                  aStatement,
                  & sLobInfo,
                  (UShort)sParseTree->tableInfo->lobColumnCount)
              != IDE_SUCCESS );

    if ( sTableType == SMI_TABLE_DISK )
    {
        // Disk Table인 경우
        // Record Read를 위한 공간을 할당한다.
        IDE_TEST( qdbCommon::getDiskRowSize( aSrcTable,
                                             & sRowSize )
                  != IDE_SUCCESS );

        // To fix BUG-14820
        // Disk-variable 컬럼의 rid비교를 위해 초기화 해야 함.
        IDU_LIMITPOINT("qdbCommon::moveRowForOutplaceSplitPartition::malloc1");
        IDE_TEST( aStatement->qmxMem->cralloc( sRowSize,
                                               (void **) & sOldRow )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }
    
    sSrcTblCursor.initialize();
    sDstTblCursor1.initialize();
    sDstTblCursor2.initialize();

    //----------------------------------------
    // open src table.
    //----------------------------------------

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sSrcCursorProperty, aStatement->mStatistics );
    sSrcCursorProperty.mLockWaitMicroSec = 0;
    sSrcCursorProperty.mIsUndoLogging = ID_FALSE;

    //----------------------------------------
    // PROJ-1705 fetch column list 구성
    //----------------------------------------

    if ( sTableType == SMI_TABLE_DISK )
    {
        IDE_TEST( qdbCommon::makeFetchColumnList(
                      QC_PRIVATE_TMPLATE(aStatement),
                      aSrcPart->columnCount,
                      aSrcPart->columns,
                      ID_TRUE,  // alloc SmiColumnList
                      & sFetchColumnList ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }

    sSrcCursorProperty.mFetchColumnList = sFetchColumnList;

    IDE_TEST(sSrcTblCursor.open(QC_SMI_STMT( aStatement ),
                                aSrcTable,
                                NULL,
                                smiGetRowSCN(aSrcTable),
                                NULL,
                                smiGetDefaultKeyRange(),
                                smiGetDefaultKeyRange(),
                                smiGetDefaultFilter(),
                                SMI_LOCK_READ|
                                SMI_TRAVERSE_FORWARD|
                                SMI_PREVIOUS_DISABLE,
                                SMI_SELECT_CURSOR,
                                & sSrcCursorProperty )
             != IDE_SUCCESS);
    sStage = 1;

    // open destination table - 1
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sDstCursorProperty1, aStatement->mStatistics );
    sDstCursorProperty1.mIsUndoLogging = ID_FALSE;

    IDE_TEST( sDstTblCursor1.open( QC_SMI_STMT( aStatement ),
                                   aDstTable1,
                                   NULL,
                                   smiGetRowSCN(aDstTable1),
                                   NULL,
                                   smiGetDefaultKeyRange(),
                                   smiGetDefaultKeyRange(),
                                   smiGetDefaultFilter(),
                                   SMI_LOCK_WRITE|
                                   SMI_TRAVERSE_FORWARD|
                                   SMI_PREVIOUS_DISABLE,
                                   SMI_INSERT_CURSOR,
                                   & sDstCursorProperty1 )
              != IDE_SUCCESS);
    sStage += 2;

    // open destination table - 2
    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sDstCursorProperty2, aStatement->mStatistics );

    sDstCursorProperty2.mIsUndoLogging = ID_FALSE;

    IDE_TEST( sDstTblCursor2.open( QC_SMI_STMT( aStatement ),
                                   aDstTable2,
                                   NULL,
                                   smiGetRowSCN(aDstTable2),
                                   NULL,
                                   smiGetDefaultKeyRange(),
                                   smiGetDefaultKeyRange(),
                                   smiGetDefaultFilter(),
                                   SMI_LOCK_WRITE|
                                   SMI_TRAVERSE_FORWARD|
                                   SMI_PREVIOUS_DISABLE,
                                   SMI_INSERT_CURSOR,
                                   & sDstCursorProperty2 )
              != IDE_SUCCESS);
    sStage += 2;

    // move..
    IDE_TEST(sSrcTblCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sSrcTblCursor.readRow(&sOldRow,
                                   &sGRID, SMI_FIND_NEXT) != IDE_SUCCESS);

    //------------------------------------------
    // init index table insert cursors
    //------------------------------------------
        
    if ( aIndexTables != NULL )
    {
        IDE_TEST( qdx::initializeUpdateIndexTableCursors(
                      aStatement,
                      aIndexTables,
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );

        sInitedCursorInfo = ID_TRUE;
        
        sPartOID1 = smiGetTableId(aDstTable1);
        sPartOID2 = smiGetTableId(aDstTable2);
    }
    else
    {
        // Nothing to do.
    }

    /* ------------------------------------------------
     * make new row values
     * ----------------------------------------------*/
    sColumnCount = 0;
    sDestCol = aDstPart1->columns;

    while (sDestCol != NULL)
    {
        sColumnCount ++;
        sDestCol = sDestCol->next;
    }

    IDU_LIMITPOINT("qdbCommon::moveRowForOutplaceSplitPartition::malloc2");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) *
                                       sColumnCount,
                                       (void**)&sNewRow)
             != IDE_SUCCESS);

    while (sOldRow != NULL)
    {
        // To Fix PR-11704
        // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        (void)qmx::clearLobInfo( sLobInfo );

        // 해당 Row가 DstPart1로 이동할지 DstPart2로 이동할지 결정한다.
        // sIsMoveRow가 ID_FALSE이면 DstPart1로 이동한다.
        // sIsMoveRow가 ID_TRUE이면 DstPart2로 이동한다.
        IDE_TEST( isMoveRowForAlterPartition( aStatement,
                                              aSrcPart,
                                              (void *)sOldRow,
                                              aSrcPart->columns,
                                              & sIsMoveRow,
                                              aSplitType )
                  != IDE_SUCCESS );

        // PROJ-1502
        if( sIsMoveRow == ID_FALSE )
        {
            IDE_TEST(qdbAlter::makeNewRow(QC_PRIVATE_TMPLATE(aStatement),
                                          NULL,
                                          aSrcPart->columns,
                                          aDstPart1->columns,
                                          sOldRow,
                                          sNewRow,
                                          & sSrcTblCursor,
                                          sGRID,
                                          sLobInfo,
                                          NULL)
                     != IDE_SUCCESS);

            IDE_TEST(sDstTblCursor1.insertRow(sNewRow,
                                              & sInsRow,
                                              & sInsGRID)
                     != IDE_SUCCESS);

            // PROJ-1362
            // insertRow를 수행후 Lob 컬럼을 처리
            IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                  sLobInfo,
                                                  & sDstTblCursor1,
                                                  sInsRow,
                                                  sInsGRID )
                      != IDE_SUCCESS );
            
            // PROJ-1624 non-partitioned index
            // move를 수행후 non-partitioned index를 처리
            if ( aIndexTables != NULL )
            {
                IDE_TEST( qdx::updateIndexTableCursors( aStatement,
                                                        &sIndexTableCursorInfo,
                                                        sPartOID1,
                                                        sGRID,
                                                        sInsGRID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // BUG-42920 DDL print data move progress
            IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                            aSrcPart,
                                                            &sProgressCnt,
                                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST(qdbAlter::makeNewRow(QC_PRIVATE_TMPLATE(aStatement),
                                          NULL,
                                          aSrcPart->columns,
                                          aDstPart2->columns,
                                          sOldRow,
                                          sNewRow,
                                          & sSrcTblCursor,
                                          sGRID,
                                          sLobInfo,
                                          NULL)
                     != IDE_SUCCESS);

            IDE_TEST(sDstTblCursor2.insertRow(sNewRow,
                                              & sInsRow,
                                              & sInsGRID)
                     != IDE_SUCCESS);

            // PROJ-1362
            // insertRow를 수행후 Lob 컬럼을 처리
            IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                  sLobInfo,
                                                  & sDstTblCursor2,
                                                  sInsRow,
                                                  sInsGRID )
                      != IDE_SUCCESS );
            
            // PROJ-1624 non-partitioned index
            // move를 수행후 non-partitioned index를 처리
            if ( aIndexTables != NULL )
            {
                IDE_TEST( qdx::updateIndexTableCursors( aStatement,
                                                        &sIndexTableCursorInfo,
                                                        sPartOID2,
                                                        sGRID,
                                                        sInsGRID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // BUG-42920 DDL print data move progress
            IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                            aSrcPart,
                                                            &sProgressCnt,
                                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        
        // To Fix PR-11704
        // Memory 재사용을 위한 Memory 이동
        IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        IDE_TEST(sSrcTblCursor.readRow(&sOldRow, &sGRID, SMI_FIND_NEXT)
                 != IDE_SUCCESS);
    }

    // BUG-42920 DDL print data move progress
    IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                    aSrcPart,
                                                    &sProgressCnt,
                                                    ID_TRUE )
              != IDE_SUCCESS );
    // close.
    sStage = 2;
    IDE_TEST(sDstTblCursor2.close() != IDE_SUCCESS);
    sStage = 1;
    IDE_TEST(sDstTblCursor1.close() != IDE_SUCCESS);
    sStage = 0;
    IDE_TEST(sSrcTblCursor.close() != IDE_SUCCESS);

    // close index table cursor
    if ( aIndexTables != NULL )
    {
        IDE_TEST( qdx::closeUpdateIndexTableCursors(
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( sStage == 1 ) || ( sStage == 3 ) || ( sStage == 5 ) )
    {
        sSrcTblCursor.close();
    }
    if (sStage > 2)
    {
        sDstTblCursor1.close();
    }
    if (sStage > 4)
    {
        sDstTblCursor2.close();
    }

    if ( sInitedCursorInfo == ID_TRUE )
    {
        qdx::finalizeUpdateIndexTableCursors(
            &sIndexTableCursorInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    (void)qmx::finalizeLobInfo( aStatement, sLobInfo );

    return IDE_FAILURE;
}

IDE_RC qdbCommon::moveRowForOutplaceMergePartition(
    qcStatement          * aStatement,
    void                 * aSrcTable1,
    void                 * aSrcTable2,
    void                 * aDstTable,
    qcmTableInfo         * aSrcPart1,
    qcmTableInfo         * aSrcPart2,
    qcmTableInfo         * aDstPart,
    qdIndexTableList     * aIndexTables )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      MERGE PARTITION 시, SrcPart1, SrcPart2에서 DstPart로 데이터를
 *      이동시킨다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;
    const void          * sOrgOldRow = NULL;
    const void          * sOldRow;
    smiValue            * sNewRow;
    SInt                  sColumnCount = 0;
    qcmColumn           * sDestCol;
    SInt                  sStage = 0;
    smiTableCursor        sSrcTblCursor1;
    smiTableCursor        sSrcTblCursor2;
    smiTableCursor        sDstTblCursor;
    iduMemoryStatus       sQmxMemStatus;
    UInt                  sRowSize;
    scGRID                sGRID;
    smiCursorProperties   sSrcCursorProperty1;
    smiCursorProperties   sSrcCursorProperty2;
    smiCursorProperties   sDstCursorProperty;
    qmxLobInfo          * sLobInfo = NULL;
    void                * sInsRow;
    scGRID                sInsGRID;
    smiFetchColumnList  * sFetchColumnList = NULL;
    qdIndexTableCursors   sIndexTableCursorInfo;
    idBool                sInitedCursorInfo = ID_FALSE;
    smOID                 sPartOID          = SMI_NULL_OID;
    ULong                 sProgressCnt      = 0;

    // PROJ-1362
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    // PROJ-1362
    // lob-locator를 저장할 공간을 할당한다.
    IDE_TEST( qmx::initializeLobInfo(
                  aStatement,
                  & sLobInfo,
                  (UShort)sParseTree->tableInfo->lobColumnCount)
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - 2개의 SrcPart 중 하나라도 Disk 이면 수행한다.
     *  - 관련내용 : PROJ-2334 PMT
     */
    if ( ( aSrcPart1->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
    {
        // Disk Table인 경우
        // Record Read를 위한 공간을 할당한다.
        IDE_TEST( qdbCommon::getDiskRowSize( aSrcTable1,
                                             & sRowSize )
                  != IDE_SUCCESS );

        // To fix BUG-14820
        // Disk-variable 컬럼의 rid비교를 위해 초기화 해야 함.
        IDU_LIMITPOINT("qdbCommon::moveRowForOutplaceMergePartition::malloc1");
        IDE_TEST( aStatement->qmxMem->cralloc( sRowSize,
                                               (void **) & sOrgOldRow )
                  != IDE_SUCCESS );

        sOldRow = sOrgOldRow;
    }
    else
    {
        if ( ( aSrcPart2->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            // Disk Table인 경우
            // Record Read를 위한 공간을 할당한다.
            IDE_TEST( qdbCommon::getDiskRowSize( aSrcTable2,
                                                 & sRowSize )
                      != IDE_SUCCESS );

            // To fix BUG-14820
            // Disk-variable 컬럼의 rid비교를 위해 초기화 해야 함.
            IDU_FIT_POINT( "qdbCommon::moveRowForOutplaceMergePartition::cralloc::sOrgOldRow",
                           idERR_ABORT_InsufficientMemory );
 
            IDE_TEST( aStatement->qmxMem->cralloc( sRowSize,
                                                   (void **) & sOrgOldRow )
                      != IDE_SUCCESS );

            sOldRow = sOrgOldRow;
        }
        else
        {
            /* Nothing To Do */
        }
    }
    
    sSrcTblCursor1.initialize();
    sSrcTblCursor2.initialize();
    sDstTblCursor.initialize();

    //-----------------------------------------
    // PROJ-1705  fetch column list 구성
    //-----------------------------------------
    /* PROJ-2464 hybrid partitioned table 지원
     *  - 2개의 SrcPart 중 하나라도 Disk 이면 수행한다.
     *  - 관련내용 : PROJ-2334 PMT
     */
    if ( ( aSrcPart1->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
    {
        IDE_TEST( qdbCommon::makeFetchColumnList(
                      QC_PRIVATE_TMPLATE(aStatement),
                      aSrcPart1->columnCount,
                      aSrcPart1->columns,
                      ID_TRUE,  // alloc smiColumnList
                      & sFetchColumnList ) != IDE_SUCCESS );
    }
    else
    {
        if ( ( aSrcPart2->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            IDE_TEST( qdbCommon::makeFetchColumnList(
                          QC_PRIVATE_TMPLATE(aStatement),
                          aSrcPart2->columnCount,
                          aSrcPart2->columns,
                          ID_TRUE,  // alloc smiColumnList
                          & sFetchColumnList ) != IDE_SUCCESS );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    //-----------------------------------------
    // src1 table open
    //-----------------------------------------

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sSrcCursorProperty1, aStatement->mStatistics );
    sSrcCursorProperty1.mLockWaitMicroSec = 0;
    sSrcCursorProperty1.mIsUndoLogging = ID_FALSE;

    /* PROJ-2464 hybrid partitioned table 지원
     *  - sFetchColumnList를 Disk 인 경우만 연결한다.
     */
    if ( ( aSrcPart1->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
    {
        sSrcCursorProperty1.mFetchColumnList = sFetchColumnList;
    }
    else
    {
        sSrcCursorProperty1.mFetchColumnList = NULL;
    }

    IDE_TEST(
        sSrcTblCursor1.open(
            QC_SMI_STMT( aStatement ),
            aSrcTable1,
            NULL,
            smiGetRowSCN(aSrcTable1),
            NULL,
            smiGetDefaultKeyRange(),
            smiGetDefaultKeyRange(),
            smiGetDefaultFilter(),
            SMI_LOCK_READ|
            SMI_TRAVERSE_FORWARD|
            SMI_PREVIOUS_DISABLE,
            SMI_SELECT_CURSOR,
            & sSrcCursorProperty1 )
        != IDE_SUCCESS);
    sStage = 1;

    //-----------------------------------------
    // src2 table open
    //-----------------------------------------

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sSrcCursorProperty2, aStatement->mStatistics );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - sFetchColumnList를 Disk 인 경우만 연결한다.
     */
    if ( ( aSrcPart2->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
    {
        sSrcCursorProperty2.mFetchColumnList = sFetchColumnList;
    }
    else
    {
        sSrcCursorProperty2.mFetchColumnList = NULL;
    }

    IDE_TEST(
        sSrcTblCursor2.open(
            QC_SMI_STMT( aStatement ),
            aSrcTable2,
            NULL,
            smiGetRowSCN(aSrcTable2),
            NULL,
            smiGetDefaultKeyRange(),
            smiGetDefaultKeyRange(),
            smiGetDefaultFilter(),
            SMI_LOCK_READ|
            SMI_TRAVERSE_FORWARD|
            SMI_PREVIOUS_DISABLE,
            SMI_SELECT_CURSOR,
            & sSrcCursorProperty2 )
        != IDE_SUCCESS);
    sStage = 2;

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sDstCursorProperty, aStatement->mStatistics );

    sDstCursorProperty.mIsUndoLogging = ID_FALSE;

    IDE_TEST(
        sDstTblCursor.open(
            QC_SMI_STMT( aStatement ),
            aDstTable,
            NULL,
            smiGetRowSCN(aDstTable),
            NULL,
            smiGetDefaultKeyRange(),
            smiGetDefaultKeyRange(),
            smiGetDefaultFilter(),
            SMI_LOCK_WRITE|
            SMI_TRAVERSE_FORWARD|
            SMI_PREVIOUS_DISABLE,
            SMI_INSERT_CURSOR,
            & sDstCursorProperty )
        != IDE_SUCCESS);
    sStage = 3;

    //------------------------------------------
    // init index table insert cursors
    //------------------------------------------
        
    if ( aIndexTables != NULL )
    {
        IDE_TEST( qdx::initializeUpdateIndexTableCursors(
                      aStatement,
                      aIndexTables,
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );

        sInitedCursorInfo = ID_TRUE;
        
        sPartOID = smiGetTableId(aDstTable);
    }
    else
    {
        // Nothing to do.
    }

    /* ------------------------------------------------
     * make new row values
     * ----------------------------------------------*/
    sColumnCount = 0;
    sDestCol = aDstPart->columns;

    while (sDestCol != NULL)
    {
        sColumnCount ++;
        sDestCol = sDestCol->next;
    }

    IDU_LIMITPOINT("qdbCommon::moveRowForOutplaceMergePartition::malloc2");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) *
                                       sColumnCount,
                                       (void**)&sNewRow)
             != IDE_SUCCESS);

    // move..
    IDE_TEST(sSrcTblCursor1.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sSrcTblCursor1.readRow(&sOldRow,
                                    &sGRID, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sOldRow != NULL)
    {
        // To Fix PR-11704
        // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        (void)qmx::clearLobInfo( sLobInfo );

        IDE_TEST(qdbAlter::makeNewRow(QC_PRIVATE_TMPLATE(aStatement),
                                      NULL,
                                      aSrcPart1->columns,
                                      aDstPart->columns,
                                      sOldRow,
                                      sNewRow,
                                      & sSrcTblCursor1,
                                      sGRID,
                                      sLobInfo,
                                      NULL)
                 != IDE_SUCCESS);

        IDE_TEST(sDstTblCursor.insertRow(sNewRow,
                                         & sInsRow,
                                         & sInsGRID)
                 != IDE_SUCCESS);

        // PROJ-1362
        // insertRow를 수행후 Lob 컬럼을 처리
        IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                              sLobInfo,
                                              & sDstTblCursor,
                                              sInsRow,
                                              sInsGRID )
                  != IDE_SUCCESS );

        // PROJ-1624 non-partitioned index
        // move를 수행후 non-partitioned index를 처리
        if ( aIndexTables != NULL )
        {
            IDE_TEST( qdx::updateIndexTableCursors( aStatement,
                                                    &sIndexTableCursorInfo,
                                                    sPartOID,
                                                    sGRID,
                                                    sInsGRID )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-42920 DDL print data move progress        
        IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                        aSrcPart1,
                                                        &sProgressCnt,
                                                        ID_FALSE )
                  != IDE_SUCCESS );            

        // To Fix PR-11704
        // Memory 재사용을 위한 Memory 이동
        IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        IDE_TEST(sSrcTblCursor1.readRow(&sOldRow, &sGRID, SMI_FIND_NEXT)
                 != IDE_SUCCESS);
    }

    // BUG-42920 DDL print data move progress
    IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                    aSrcPart1,
                                                    &sProgressCnt,
                                                    ID_TRUE )
              != IDE_SUCCESS );
    
    sOldRow = sOrgOldRow;

    IDE_TEST(sSrcTblCursor2.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sSrcTblCursor2.readRow( &sOldRow,
                                     &sGRID,
                                     SMI_FIND_NEXT)
             != IDE_SUCCESS);

    while (sOldRow != NULL)
    {
        // To Fix PR-11704
        // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        (void)qmx::clearLobInfo( sLobInfo );

        IDE_TEST(qdbAlter::makeNewRow(QC_PRIVATE_TMPLATE(aStatement),
                                      NULL,
                                      aSrcPart2->columns,
                                      aDstPart->columns,
                                      sOldRow,
                                      sNewRow,
                                      & sSrcTblCursor2,
                                      sGRID,
                                      sLobInfo,
                                      NULL)
                 != IDE_SUCCESS);

        IDE_TEST(sDstTblCursor.insertRow(sNewRow,
                                         & sInsRow,
                                         & sInsGRID)
                 != IDE_SUCCESS);

        // PROJ-1362
        // insertRow를 수행후 Lob 컬럼을 처리
        IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                              sLobInfo,
                                              & sDstTblCursor,
                                              sInsRow,
                                              sInsGRID )
                  != IDE_SUCCESS );

        // PROJ-1624 non-partitioned index
        // move를 수행후 non-partitioned index를 처리
        if ( aIndexTables != NULL )
        {
            IDE_TEST( qdx::updateIndexTableCursors( aStatement,
                                                    &sIndexTableCursorInfo,
                                                    sPartOID,
                                                    sGRID,
                                                    sInsGRID )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-42920 DDL print data move progress
        IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                        aSrcPart2,
                                                        &sProgressCnt,
                                                        ID_FALSE )
                  != IDE_SUCCESS );

        // To Fix PR-11704
        // Memory 재사용을 위한 Memory 이동
        IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                  != IDE_SUCCESS);

        IDE_TEST(sSrcTblCursor2.readRow( &sOldRow,
                                         &sGRID,
                                         SMI_FIND_NEXT )
                 != IDE_SUCCESS);
    }

    // BUG-42920 DDL print data move progress
    IDE_TEST( qdbAlter::printProgressLog4Partition( sParseTree->tableInfo,
                                                    aSrcPart2,
                                                    &sProgressCnt,
                                                    ID_TRUE )
              != IDE_SUCCESS );

    // close.
    sStage = 2;
    IDE_TEST(sDstTblCursor.close() != IDE_SUCCESS);
    sStage = 1;
    IDE_TEST(sSrcTblCursor2.close() != IDE_SUCCESS);
    sStage = 0;
    IDE_TEST(sSrcTblCursor1.close() != IDE_SUCCESS);

    // close index table cursor
    if ( aIndexTables != NULL )
    {
        IDE_TEST( qdx::closeUpdateIndexTableCursors(
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 3:
            sDstTblCursor.close();
        case 2:
            sSrcTblCursor2.close();
        case 1:
            sSrcTblCursor1.close();
        case 0:
            break;
        default:
            break;
    }

    if ( sInitedCursorInfo == ID_TRUE )
    {
        qdx::finalizeUpdateIndexTableCursors(
            &sIndexTableCursorInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    (void)qmx::finalizeLobInfo( aStatement, sLobInfo );

    return IDE_FAILURE;
}

IDE_RC qdbCommon::isMoveRowForAlterPartition(
    qcStatement          * aStatement,
    qcmTableInfo         * aPartInfo,
    void                 * aRow,
    qcmColumn            * aColumns,
    idBool               * aIsMoveRow,
    qdSplitMergeType       aType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      SPLIT PARTITION 시, 인자로 받은 데이터의 이동 여부를 판단한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qdPartitionAttribute * sSrcPart;
    qcmTableInfo         * sTableInfo;
    qmsPartCondValList   * sSplitCondVal;
    qdTableParseTree     * sParseTree;
    UInt                   sColCount;
    void                 * sValue = NULL;
    qmsPartCondValList     sValueCondVal;
    qcmColumn            * sColumn;
    UInt                   sKeyColumnOrder;
    UInt                   sColumnOrder;
    UInt                   sHashValue = 0;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sTableInfo = sParseTree->tableInfo;

    sSrcPart = sParseTree->partTable->partAttr;

    // ---------------------------------------------------
    // 분할 기준 값을 파스트리에서 가져온다.
    // ---------------------------------------------------
    sSplitCondVal = sSrcPart->alterPart->splitCondVal;

    // 범위 파티션드 테이블일 경우에만
    // 해당 Row의 qmsPartCondValList가 필요하다.
    if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE )
    {
        // sValueCondVal을 구성한다.
        sValueCondVal.partCondValCount = aPartInfo->partKeyColCount;
        sValueCondVal.partCondValType = QMS_PARTCONDVAL_NORMAL;

        for( sColCount = 0;
             sColCount < aPartInfo->partKeyColCount;
             sColCount++ )
        {
            sKeyColumnOrder = (aPartInfo->partKeyColumns[sColCount].basicInfo->column.id & SMI_COLUMN_ID_MASK);

            sColumn = aColumns;
            sValue  = NULL;
            while (sColumn != NULL)
            {
                sColumnOrder = (sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK);
                if( sKeyColumnOrder == sColumnOrder )
                {
                    sValue = (void *)
                        mtc::value( sColumn->basicInfo,
                                    aRow,
                                    MTD_OFFSET_USE );
                    sValueCondVal.partCondValues[sColCount] = sValue;
                    break;
                }
                sColumn = sColumn->next;
            }

            IDE_TEST_RAISE( sValue == NULL, ERR_INVALID_VALUE );
        }
    }
    else if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
    {
        /* BUG-46455 range_using_hash split partition 시 오류 */
        // sValueCondVal을 구성한다.
        sValueCondVal.partCondValCount = aPartInfo->partKeyColCount;
        sValueCondVal.partCondValType = QMS_PARTCONDVAL_NORMAL;

        for ( sColCount = 0;
              sColCount < aPartInfo->partKeyColCount;
              sColCount++ )
        {
            sKeyColumnOrder = (aPartInfo->partKeyColumns[sColCount].basicInfo->column.id & SMI_COLUMN_ID_MASK);

            sColumn = aColumns;
            sValue  = NULL;
            while (sColumn != NULL)
            {
                sColumnOrder = (sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK);
                if ( sKeyColumnOrder == sColumnOrder )
                {
                    sValue = (void *)mtc::value( sColumn->basicInfo,
                                                 aRow,
                                                 MTD_OFFSET_USE );
                    sHashValue = mtc::hashInitialValue;

                    IDE_TEST_RAISE( sValue == NULL, ERR_INVALID_VALUE );

                    sHashValue = sColumn->basicInfo->module->hash( mtc::hashInitialValue,
                                                                   sColumn->basicInfo,
                                                                   sValue );
                    sHashValue = sHashValue % QMO_RANGE_USING_HASH_MAX_VALUE;
                    sValue = &sHashValue;
                    sValueCondVal.partCondValues[sColCount] = sValue;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
                sColumn = sColumn->next;
            }
            IDE_TEST_RAISE( sValue == NULL, ERR_INVALID_VALUE );
        }
    }
    else // QCM_PARTITION_METHOD_LIST
    {
        IDE_DASSERT( sTableInfo->partKeyColCount == 1 );

        sKeyColumnOrder = (aPartInfo->partKeyColumns[0].basicInfo->column.id & SMI_COLUMN_ID_MASK);

        sColumn = aColumns;
        while (sColumn != NULL)
        {
            sColumnOrder = (sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK);
            if( sKeyColumnOrder == sColumnOrder )
            {
                sValue = (void *)
                    mtc::value( sColumn->basicInfo,
                                aRow,
                                MTD_OFFSET_USE );
                sValueCondVal.partCondValues[0] = sValue;
                break;
            }
            sColumn = sColumn->next;
        }

        IDE_TEST_RAISE( sValue == NULL, ERR_INVALID_VALUE );
    }

    // ---------------------------------------------------
    // 분할 기준 값과 해당 Row의 비교
    // ---------------------------------------------------
    switch( aType )
    {
        case QD_ALTER_PARTITION_LEFT_INPLACE_TYPE:
        case QD_ALTER_PARTITION_OUTPLACE_TYPE:
            {
                if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE )
                {
                    if( qmoPartition::compareRangePartition(
                            aPartInfo->partKeyColumns,
                            sSplitCondVal,
                            &sValueCondVal) != 1 )
                    {
                        *aIsMoveRow = ID_TRUE;
                    }
                    else
                    {
                        *aIsMoveRow = ID_FALSE;
                    }
                }
                /* BUG-46065 support range using hash */
                else if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
                {
                    if ( qmoPartition::compareRangeUsingHashPartition(
                            sSplitCondVal,
                            &sValueCondVal) != 1 )
                    {
                        *aIsMoveRow = ID_TRUE;
                    }
                    else
                    {
                        *aIsMoveRow = ID_FALSE;
                    }
                }
                // 리스트 파티션드 테이블
                else
                {
                    if( qmoPartition::compareListPartition(
                            aPartInfo->partKeyColumns,
                            sSplitCondVal,
                            sValue ) == ID_TRUE )
                    {
                        *aIsMoveRow = ID_FALSE;
                    }
                    else
                    {
                        *aIsMoveRow = ID_TRUE;
                    }
                }
            }
            break;

        case QD_ALTER_PARTITION_RIGHT_INPLACE_TYPE:
            {
                if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE )
                {
                    if( qmoPartition::compareRangePartition(
                            aPartInfo->partKeyColumns,
                            sSplitCondVal,
                            &sValueCondVal) == 1 )
                    {
                        *aIsMoveRow = ID_TRUE;
                    }
                    else
                    {
                        *aIsMoveRow = ID_FALSE;
                    }
                }
                /* BUG-46065 support range using hash */
                else if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
                {
                    if ( qmoPartition::compareRangeUsingHashPartition(
                            sSplitCondVal,
                            &sValueCondVal) == 1 )
                    {
                        *aIsMoveRow = ID_TRUE;
                    }
                    else
                    {
                        *aIsMoveRow = ID_FALSE;
                    }
                }
                // 리스트 파티션드 테이블
                else
                {
                    if( qmoPartition::compareListPartition(
                            aPartInfo->partKeyColumns,
                            sSplitCondVal,
                            sValue ) == ID_TRUE )
                    {
                        *aIsMoveRow = ID_TRUE;
                    }
                    else
                    {
                        *aIsMoveRow = ID_FALSE;
                    }
                }
            }
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_VALUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbCommon::isMoveRowForAlterPartition",
                                  "sValue == NULL" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdbCommon::excludeSplitValFromPartVal(
    qcmTableInfo         * aTableInfo,
    qdAlterPartition     * aAlterPart,
    SChar                * aPartVal )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      리스트 파티션드 테이블을 SPLIT할 경우
 *      DstPart1이 분할 조건 값을 갖고,
 *      DstPart2가 SrcPart 파티션의 PARTITION_MIN_VALUE에서
 *      분할 조건 값을 제거한 값을 갖는다.
 *      (PARTITION_MAX_VALUE도 마찬가지이다.)
 *
 *      즉, 무조건 DstPart1이 파티션 분할 조건을 갖게 된다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPartCondValList  * sPartCondMinVal;
    qmsPartCondValList  * sSplitCondVal;
    UInt                  sSrcCondCount;
    idBool                sIsFound;
    SChar                 sNewStr[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    UInt                  sDstPos = 0;
    SChar               * sPartKeyCondMinStmtText;
    qmmValueNode        * sPartCondMinValNode;
    qcNamePosition        sPartCondMinValPos;


    // 파스트리에 달아놓은 값을 가져온다.
    sPartCondMinVal         = aAlterPart->partCondMinVal;
    sSplitCondVal           = aAlterPart->splitCondVal;
    sPartCondMinValNode     = aAlterPart->partKeyCondMin;
    sPartKeyCondMinStmtText = aAlterPart->partKeyCondMinStmtText;

    // ---------------------------------------------------
    // SrcPart의 조건 값과 분할 조건이 같으면
    // 파티션 조건 값 리스트에서 제거한다.
    // ---------------------------------------------------
    for( sSrcCondCount = 0;
         sSrcCondCount < sPartCondMinVal->partCondValCount;
         sSrcCondCount++, sPartCondMinValNode = sPartCondMinValNode->next )
    {
        sPartCondMinValPos = sPartCondMinValNode->value->position;

        sIsFound = ID_FALSE;

        if( qmoPartition::compareListPartition(
                aTableInfo->partKeyColumns,
                sSplitCondVal,
                sPartCondMinVal->partCondValues[sSrcCondCount] )
            == ID_TRUE )
        {
            sIsFound = ID_TRUE;
        }

        // 분할 조건에 없는 파티션 조건 값만으로 다시 구성한다.
        if( sIsFound == ID_FALSE )
        {
            idlOS::memcpy( sNewStr + sDstPos,
                           sPartKeyCondMinStmtText + sPartCondMinValPos.offset,
                           sPartCondMinValPos.size );
            sDstPos += sPartCondMinValPos.size;
            sNewStr[sDstPos++] =  ',';
        }
        else
        {
            // Nothing to do
        }
    }

    if( sDstPos > 0 )
    {
        // 마지막 문자가 ','라면 제거한다.
        sNewStr[--sDstPos] = '\0';
        idlOS::strcpy( aPartVal, sNewStr );
    }
}

void qdbCommon::mergePartCondVal(
    qcStatement          * aStatement,
    SChar                * aPartVal )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      리스트 파티션드 테이블을 MERGE할 경우
 *      SrcPart1과 SrcPart2의 파티션 조건 값의 합을
 *      DstPart가 갖는다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                  sNewStr[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    UInt                   sDstPos = 0;
    UInt                   sSrcCondCount;
    qdTableParseTree     * sParseTree;
    qdPartitionAttribute * sSrcPartAttr1;
    qdPartitionAttribute * sSrcPartAttr2;
    qmsPartCondValList   * sPartCondMinVal;
    qmmValueNode         * sPartCondMinValNode;
    SChar                * sPartKeyCondMinStmtText;
    qcNamePosition         sPartCondMinValPos;


    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sSrcPartAttr1 = sParseTree->partTable->partAttr;
    sSrcPartAttr2 = sParseTree->partTable->partAttr->next;

    idlOS::memset( sNewStr,
                   0x00,
                   QC_MAX_PARTKEY_COND_VALUE_LEN + 1 );

    // SrcPart1에 대해 파스트리에 달아놓은 값을 가져온다.
    sPartCondMinVal         = sSrcPartAttr1->alterPart->partCondMinVal;
    sPartCondMinValNode     = sSrcPartAttr1->alterPart->partKeyCondMin;
    sPartKeyCondMinStmtText = sSrcPartAttr1->alterPart->partKeyCondMinStmtText;

    // ---------------------------------------------------
    // SrcPart1의 조건 값을 sNewStr에 넣는다.
    // ---------------------------------------------------
    for( sSrcCondCount = 0;
         sSrcCondCount < sPartCondMinVal->partCondValCount;
         sSrcCondCount++,
             sPartCondMinValNode = sPartCondMinValNode->next )
    {
        sPartCondMinValPos = sPartCondMinValNode->value->position;

        idlOS::memcpy( sNewStr + sDstPos,
                       sPartKeyCondMinStmtText + sPartCondMinValPos.offset,
                       sPartCondMinValPos.size );
        sDstPos += sPartCondMinValPos.size;
        sNewStr[sDstPos++] =  ',';
    }

    // SrcPart2에 대해 파스트리에 달아놓은 값을 가져온다.
    sPartCondMinVal         = sSrcPartAttr2->alterPart->partCondMinVal;
    sPartCondMinValNode     = sSrcPartAttr2->alterPart->partKeyCondMin;
    sPartKeyCondMinStmtText = sSrcPartAttr2->alterPart->partKeyCondMinStmtText;

    // ---------------------------------------------------
    // SrcPart2의 조건 값을 sNewStr에 넣는다.
    // ---------------------------------------------------
    for( sSrcCondCount = 0;
         sSrcCondCount < sPartCondMinVal->partCondValCount;
         sSrcCondCount++,
             sPartCondMinValNode = sPartCondMinValNode->next )
    {
        sPartCondMinValPos = sPartCondMinValNode->value->position;

        idlOS::memcpy( sNewStr + sDstPos,
                       sPartKeyCondMinStmtText + sPartCondMinValPos.offset,
                       sPartCondMinValPos.size );
        sDstPos += sPartCondMinValPos.size;
        sNewStr[sDstPos++] =  ',';
    }

    if( sDstPos > 0 )
    {
        sNewStr[--sDstPos] = '\0';
        idlOS::strcpy( aPartVal, sNewStr );
    }
}

IDE_RC qdbCommon::checkSplitCondOfDefaultPartition(
    qcStatement          * aStatement,
    qcmTableInfo         * aTableInfo,
    qmsPartCondValList   * aSplitCondVal )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      리스트 파티션드 테이블의 기본 파티션을 분할할 경우,
 *      해당 테이블의 모든 파티션의 파티션 조건값과
 *      분할 조건 값을 비교하는 함수이다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsPartCondValList   * sPartCondMaxVal;
    mtdCharType          * sPartKeyCondMinValueStr;
    mtdCharType          * sPartKeyCondMaxValueStr;
    UInt                   sMaxValCount;
    qcmPartitionIdList   * sPartIdList = NULL;
    UInt                   sPartitionID;

    IDU_LIMITPOINT("qdbCommon::checkSplitCondOfDefaultPartition::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                  QC_QMP_MEM(aStatement),
                  UChar,
                  MTD_CHAR_TYPE_STRUCT_SIZE(QC_MAX_PARTKEY_COND_VALUE_LEN),
                  (void**) & sPartKeyCondMinValueStr )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::checkSplitCondOfDefaultPartition::malloc2");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                  QC_QMP_MEM(aStatement),
                  UChar,
                  MTD_CHAR_TYPE_STRUCT_SIZE(QC_MAX_PARTKEY_COND_VALUE_LEN),
                  (void**) & sPartKeyCondMaxValueStr )
              != IDE_SUCCESS );

    // tableID로 파티션 ID의 리스트를 가져온다.
    IDE_TEST( qcmPartition::getPartitionIdList( aStatement,
                                                QC_SMI_STMT( aStatement ),
                                                aTableInfo->tableID,
                                                & sPartIdList )
              != IDE_SUCCESS );

    for( ;
         sPartIdList != NULL;
         sPartIdList = sPartIdList->next )
    {
        sPartitionID = sPartIdList->partId;

        IDU_LIMITPOINT("qdbCommon::checkSplitCondOfDefaultPartition::malloc3");
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                qmsPartCondValList,
                                & sPartCondMaxVal )
                  != IDE_SUCCESS );

        // ---------------------------------------------------
        // 1. 파티션의 PARTITION_MIN_VALUE, PARTITION_MAX_VALUE 구하기
        // ---------------------------------------------------
        IDE_TEST( qcmPartition::getPartMinMaxValue( QC_SMI_STMT( aStatement ),
                                                    sPartitionID,
                                                    sPartKeyCondMinValueStr,
                                                    sPartKeyCondMaxValueStr )
                  != IDE_SUCCESS );

        // 기본 파티션이 아닌 파티션만 체크한다.
        if( sPartKeyCondMaxValueStr->length > 0 )
        {
            // ---------------------------------------------------
            // 2. max value를 qmsPartCondValList로 만든다.
            // ---------------------------------------------------
            IDE_TEST( qcmPartition::getPartCondVal(
                          aStatement,
                          aTableInfo->partKeyColumns,
                          aTableInfo->partitionMethod,
                          sPartCondMaxVal,
                          NULL, /* aPartCondValStmtText */
                          NULL, /* aPartCondValNode */
                          sPartKeyCondMaxValueStr )
                      != IDE_SUCCESS );

            // ---------------------------------------------------
            // 파티션 분할 기준 값이
            // 지정한 파티션 조건 값에 속하는지 체크
            // ---------------------------------------------------
            for( sMaxValCount = 0;
                 sMaxValCount < sPartCondMaxVal->partCondValCount;
                 sMaxValCount++ )
            {
                IDE_TEST_RAISE( qmoPartition::compareListPartition(
                                    aTableInfo->partKeyColumns,
                                    aSplitCondVal,
                                    sPartCondMaxVal->partCondValues[sMaxValCount] ) == ID_TRUE,
                                ERR_ALREADY_EXIST_SPLIT_COND_VALUE_ON_LIST_PARTITION );
            }
        }
        else
        {
            // empty partition과 기본 파티션은 체크하지 않음
            // Nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ALREADY_EXIST_SPLIT_COND_VALUE_ON_LIST_PARTITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_ALREADY_EXIST_SPLIT_CONDITION_VALUE_ON_LIST_PARTITION));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makePartCondValList(
    qcStatement          * aStatement,
    qcmTableInfo         * aTableInfo,
    UInt                   aPartitionID,
    qdPartitionAttribute * aPartAttr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      인자로 받은 테이블 파티션의 min_value, max_value를
 *      qmsPartCondValList, qmmValueNode로 만들어서 파스트리에 달아놓는다.
 *
 *      ALTER TABLE ... SPLIT PARTITION
 *      ALTER TABLE ... MERGE PARTITIONS
 *
 *      구문의 validation에서 호출된다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    mtdCharType         * sPartKeyCondMinValueStr   = NULL;
    mtdCharType         * sPartKeyCondMaxValueStr   = NULL;
    qmsPartCondValList  * sPartCondMinVal           = NULL;
    qmsPartCondValList  * sPartCondMaxVal           = NULL;
    qmmValueNode        * sPartCondMinValNode       = NULL;
    qmmValueNode        * sPartCondMaxValNode       = NULL;
    SChar               * sPartCondMinStmtText      = NULL;
    SChar               * sPartCondMaxStmtText      = NULL;

    IDU_LIMITPOINT("qdbCommon::makePartCondValList::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                  QC_QMP_MEM(aStatement),
                  UChar,
                  MTD_CHAR_TYPE_STRUCT_SIZE(QC_MAX_PARTKEY_COND_VALUE_LEN),
                  (void**) & sPartKeyCondMinValueStr )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::makePartCondValList::malloc2");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                  QC_QMP_MEM(aStatement),
                  UChar,
                  MTD_CHAR_TYPE_STRUCT_SIZE(QC_MAX_PARTKEY_COND_VALUE_LEN),
                  (void**) & sPartKeyCondMaxValueStr )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::makePartCondValList::malloc3");
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsPartCondValList,
                            & sPartCondMinVal )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::makePartCondValList::malloc4");
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsPartCondValList,
                            & sPartCondMaxVal )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::makePartCondValList::malloc5");
    IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( QC_MAX_PARTKEY_COND_VALUE_LEN + 8,
                                               (void**)&sPartCondMinStmtText )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::makePartCondValList::malloc6");
    IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( QC_MAX_PARTKEY_COND_VALUE_LEN + 8,
                                               (void**)&sPartCondMaxStmtText )
              != IDE_SUCCESS );

    // ---------------------------------------------------
    // 1. PARTITION_MIN_VALUE, PARTITION_MAX_VALUE 구하기
    // ---------------------------------------------------
    IDE_TEST( qcmPartition::getPartMinMaxValue( QC_SMI_STMT( aStatement ),
                                                aPartitionID,
                                                sPartKeyCondMinValueStr,
                                                sPartKeyCondMaxValueStr )
              != IDE_SUCCESS );

    // ---------------------------------------------------
    // 2. min value를 qmsPartCondValList로 만든다.
    // ---------------------------------------------------
    IDE_TEST( qcmPartition::getPartCondVal( aStatement,
                                            aTableInfo->partKeyColumns,
                                            aTableInfo->partitionMethod,
                                            sPartCondMinVal,
                                            sPartCondMinStmtText,
                                            &sPartCondMinValNode,
                                            sPartKeyCondMinValueStr )
              != IDE_SUCCESS );

    // ---------------------------------------------------
    // 3. max value를 qmsPartCondValList로 만든다.
    // ---------------------------------------------------
    IDE_TEST( qcmPartition::getPartCondVal( aStatement,
                                            aTableInfo->partKeyColumns,
                                            aTableInfo->partitionMethod,
                                            sPartCondMaxVal,
                                            sPartCondMaxStmtText,
                                            &sPartCondMaxValNode,
                                            sPartKeyCondMaxValueStr )
              != IDE_SUCCESS );

    aPartAttr->alterPart->partKeyCondMinValStr = sPartKeyCondMinValueStr;
    aPartAttr->alterPart->partCondMinVal = sPartCondMinVal;
    aPartAttr->alterPart->partKeyCondMin = sPartCondMinValNode;
    aPartAttr->alterPart->partKeyCondMinStmtText = sPartCondMinStmtText;

    aPartAttr->alterPart->partKeyCondMaxValStr = sPartKeyCondMaxValueStr;
    aPartAttr->alterPart->partCondMaxVal = sPartCondMaxVal;
    aPartAttr->alterPart->partKeyCondMax = sPartCondMaxValNode;
    aPartAttr->alterPart->partKeyCondMaxStmtText = sPartCondMaxStmtText;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkPartitionInfo(
    qcStatement           * aStatement,
    qcmTableInfo          * aTableInfo,
    qcNamePosition          aPartName )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree;
    qcmTableInfo            * sPartitionInfo;
    smSCN                     sPartitionSCN;
    void                    * sPartitionHandle;
    qcuSqlSourceInfo          sqlInfo;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmPartitionInfoList    * sFirstPartInfoList = NULL;
    qcmPartitionInfoList    * sTempPartInfoList = NULL;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sFirstPartInfoList = sParseTree->partTable->partInfoList;

    if( qcmPartition::getPartitionInfo( aStatement,
                                        aTableInfo->tableID,
                                        aPartName,
                                        & sPartitionInfo,
                                        & sPartitionSCN,
                                        & sPartitionHandle )
        != IDE_SUCCESS )
    {
        sqlInfo.setSourceInfo(aStatement,
                              & aPartName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE_PARTITION );
    }

    // 파티션에 LOCK(IS)
    IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                         sPartitionHandle,
                                                         sPartitionSCN,
                                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                         SMI_TABLE_LOCK_IS,
                                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    // 해당 파티션의 Handle, SCN을 파스트리에 달아놓는다.
    // 파티션 메타 정보 리스트 구축
    IDU_LIMITPOINT("qdbCommon::checkPartitionInfo::malloc1");
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmPartitionInfoList),
                                           (void**)&(sPartInfoList))
             != IDE_SUCCESS);

    // -------------------------------------------
    // sPartInfoList 구성
    // -------------------------------------------
    sPartInfoList->partitionInfo = sPartitionInfo;
    sPartInfoList->partHandle = sPartitionHandle;
    sPartInfoList->partSCN = sPartitionSCN;

    if (sFirstPartInfoList == NULL)
    {
        sPartInfoList->next = NULL;
        sFirstPartInfoList = sPartInfoList;
    }
    else
    {
        sTempPartInfoList = sFirstPartInfoList;
        sPartInfoList->next = NULL;

        for( ;
             sTempPartInfoList->next != NULL;
             sTempPartInfoList = sTempPartInfoList->next ) ;

        sTempPartInfoList->next = sPartInfoList;
    }

    sParseTree->partTable->partInfoList = sFirstPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE_PARTITION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION,
                                 sqlInfo.getErrMessage()) );
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkPartitionInfo(
    qcStatement           * aStatement,
    UInt                    aPartID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree;
    qcmTableInfo            * sPartitionInfo;
    smSCN                     sPartitionSCN;
    void                    * sPartitionHandle;
    qcuSqlSourceInfo          sqlInfo;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmPartitionInfoList    * sFirstPartInfoList = NULL;
    qcmPartitionInfoList    * sTempPartInfoList = NULL;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sFirstPartInfoList = sParseTree->partTable->partInfoList;

    IDE_TEST_RAISE( qcmPartition::getPartitionInfoByID( aStatement,
                                                        aPartID,
                                                        & sPartitionInfo,
                                                        & sPartitionSCN,
                                                        & sPartitionHandle )
                    != IDE_SUCCESS, ERR_NOT_EXIST_TABLE_PARTITION )

    // 파티션에 LOCK(IS)
    IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                         sPartitionHandle,
                                                         sPartitionSCN,
                                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                         SMI_TABLE_LOCK_IS,
                                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    // 해당 파티션의 Handle, SCN을 파스트리에 달아놓는다.
    // 파티션 메타 정보 리스트 구축
    IDU_LIMITPOINT("qdbCommon::checkPartitionInfo::malloc2");
    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmPartitionInfoList),
                                           (void**)&(sPartInfoList))
             != IDE_SUCCESS);

    // -------------------------------------------
    // sPartInfoList 구성
    // -------------------------------------------
    sPartInfoList->partitionInfo = sPartitionInfo;
    sPartInfoList->partHandle = sPartitionHandle;
    sPartInfoList->partSCN = sPartitionSCN;

    if (sFirstPartInfoList == NULL)
    {
        sPartInfoList->next = NULL;
        sFirstPartInfoList = sPartInfoList;
    }
    else
    {
        sTempPartInfoList = sFirstPartInfoList;
        sPartInfoList->next = NULL;
        sTempPartInfoList->next = sPartInfoList;
        sFirstPartInfoList = sTempPartInfoList;
    }

    sParseTree->partTable->partInfoList = sFirstPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE_PARTITION)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION,
                                 sqlInfo.getErrMessage()) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkAndSetAllPartitionInfo(
    qcStatement           * aStatement,
    UInt                    aTableID,
    qcmPartitionInfoList ** aPartInfoList)
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      이 함수는 DDL의 Validation 과정에서 호출되며,
 *      해당 파티션드 테이블(aTableID)의 모든 파티션의 리스트를
 *      넘겨주는 함수이다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmPartitionInfoList    * sPartInfoList = NULL;

    IDE_TEST( qcmPartition::getPartitionInfoList(
                  aStatement,
                  QC_SMI_STMT( aStatement ),
                  QC_QMP_MEM(aStatement),
                  aTableID,
                  & sPartInfoList )
              != IDE_SUCCESS );

    // 모든 파티션에 LOCK(IS)
    IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                              sPartInfoList,
                                                              SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                              SMI_TABLE_LOCK_IS,
                                                              smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    *aPartInfoList = sPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::reorganizeForHashPartition(
    qcStatement          * aStatement,
    qcmTableInfo         * aTableInfo,
    qcmPartitionInfoList * aSrcPartInfoList,
    UInt                   aDstPartCount,
    smOID                * aDstPartOID,
    qcmIndex             * aNewIndices,
    qdIndexTableList     * aNewIndexTables )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      ALTER TABLE ADD PARTITION
 *      ALTER TABLE COALESCE PARTITION 시,
 *      hash 함수에 따라서 데이터를 재구성한다.
 *
 *      인자로 넘어온 aPartInfoList는
 *      ADD PARTITION 시에는 추가된 파티션까지 포함된 것이고,
 *      COALESCE PARTTION 시에는 처음 상태 그대로임.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    const void              * sOldRow;
    smiValue                * sNewRow;
    SInt                      sDstStage = 0;
    SInt                      sSrcStage = 0;
    smiTableCursor            sSrcTblCursor;
    smiTableCursor          * sDstTblCursor;
    iduMemoryStatus           sQmxMemStatus;
    UInt                      sRowSize;
    scGRID                    sGRID;
    smiCursorProperties       sCursorProperty;
    qmxLobInfo              * sLobInfo = NULL;
    void                    * sInsRow;
    scGRID                    sInsGRID;

    UInt                      sSrcPartNum = 0;
    UInt                      sDstPartNum = 0;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmTableInfo            * sPartInfo;
    void                    * sPartHandle;
    UInt                      i, j;
    qcmColumn              ** sDstQcmColumns;
    smiFetchColumnList      * sFetchColumnList = NULL;

    qdIndexTableCursors       sIndexTableCursorInfo;
    idBool                    sInitedCursorInfo = ID_FALSE;
    smOID                     sPartOID;
    mtcColumn               * sMtcColumn;

    void                    * sDiskRow = NULL;
    ULong                     sProgressCnt = 0;

    IDU_LIMITPOINT("qdbCommon::reorganizeForHashPartition::malloc1");
    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) *
                                       aTableInfo->columnCount,
                                       (void**)&sNewRow)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCommon::reorganizeForHashPartition::malloc2");
    IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(smiTableCursor) * aDstPartCount,
                                         (void **) & sDstTblCursor )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbCommon::reorganizeForHashPartition::malloc3");
    IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(qcmColumn*) * aDstPartCount,
                                         (void **) &sDstQcmColumns )
              != IDE_SUCCESS );

    // -----------------------------------------------------------
    // 1. 데이터 재구성을 위해 관련된 모든 파티션의 커서를 연다.
    // 2. LOB을 위한 목표 컬럼의 정보를 구성한다.
    // -----------------------------------------------------------
    for( i = 0; i < aDstPartCount; i++ )
    {
        // open table.
        SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sCursorProperty, aStatement->mStatistics );
        sCursorProperty.mLockWaitMicroSec = 0;
        sCursorProperty.mIsUndoLogging = ID_FALSE;

        sDstTblCursor[i].initialize();

        IDE_TEST( sDstTblCursor[i].open( QC_SMI_STMT( aStatement ),
                                         smiGetTable( aDstPartOID[i] ),
                                         NULL,
                                         smiGetRowSCN(smiGetTable( aDstPartOID[i] )),
                                         NULL,
                                         smiGetDefaultKeyRange(),
                                         smiGetDefaultKeyRange(),
                                         smiGetDefaultFilter(),
                                         SMI_LOCK_WRITE|
                                         SMI_TRAVERSE_FORWARD|
                                         SMI_PREVIOUS_DISABLE,
                                         SMI_INSERT_CURSOR,
                                         & sCursorProperty )
                  != IDE_SUCCESS);
        sDstStage += 1;

        // LOB을 위한 목표 컬럼의 정보를 구성한다.
        IDU_LIMITPOINT("qdbCommon::reorganizeForHashPartition::malloc4");
        IDE_TEST( aStatement->qmxMem->alloc(
                      ID_SIZEOF(qcmColumn) * aTableInfo->columnCount,
                      (void**)& sDstQcmColumns[i])
                  != IDE_SUCCESS );

        for( j = 0; j < aTableInfo->columnCount; j++ )
        {
            idlOS::memcpy( &sDstQcmColumns[i][j],
                           &aTableInfo->columns[j],
                           ID_SIZEOF(qcmColumn) );

            IDE_TEST( smiGetTableColumns( smiGetTable( aDstPartOID[i] ),
                                          j,
                                          (const smiColumn**)&sMtcColumn )
                      != IDE_SUCCESS );

            sDstQcmColumns[i][j].basicInfo = sMtcColumn;

            // mtdModule 설정
            IDE_TEST(
                mtd::moduleById(
                    &(sDstQcmColumns[i][j].basicInfo->module),
                    (sDstQcmColumns[i][j].basicInfo->type.dataTypeId))
                != IDE_SUCCESS);

            // mtlModule 설정
            IDE_TEST(
                mtl::moduleById(
                    &(sDstQcmColumns[i][j].basicInfo->language),
                    (sDstQcmColumns[i][j].basicInfo->type.languageId))
                != IDE_SUCCESS);

            if( j == aTableInfo->columnCount - 1 )
            {
                sDstQcmColumns[i][j].next = NULL;
            }
            else
            {
                sDstQcmColumns[i][j].next =
                    &(sDstQcmColumns[i][j+1]);
            }
        }
    }

    //------------------------------------------
    // open index table insert cursors
    //------------------------------------------
        
    if ( aNewIndexTables != NULL )
    {
        IDE_TEST( qdx::initializeInsertIndexTableCursors(
                      aStatement,
                      aNewIndexTables,
                      &sIndexTableCursorInfo,
                      aNewIndices,
                      aTableInfo->indexCount,
                      aTableInfo->columnCount,
                      &sCursorProperty )
                  != IDE_SUCCESS );
            
        sInitedCursorInfo = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    // ---------------------------------------
    // 데이터 재구성
    // ---------------------------------------
    for( sPartInfoList = aSrcPartInfoList, sSrcPartNum = 0;
         sPartInfoList != NULL;
         sPartInfoList = sPartInfoList->next, sSrcPartNum++ )
    {
        sPartInfo = sPartInfoList->partitionInfo;
        sPartHandle = sPartInfoList->partHandle;
        
        // PROJ-1362
        // lob-locator를 저장할 공간을 할당한다.
        IDE_TEST( qmx::initializeLobInfo(
                      aStatement,
                      & sLobInfo,
                      (UShort)aTableInfo->lobColumnCount)
                  != IDE_SUCCESS );

        /* PROJ-2464 hybrid partitioned table 지원
         *  - Partition Type으로 결정하도록 수정한다.
         */
        if ( ( sPartInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            if ( sDiskRow == NULL )
            {
                /* PROJ-1705 : fetch column list 구성 */
                IDE_TEST( qdbCommon::makeFetchColumnList(
                              QC_PRIVATE_TMPLATE(aStatement),
                              sPartInfo->columnCount,
                              sPartInfo->columns,
                              ID_TRUE,
                              & sFetchColumnList ) != IDE_SUCCESS );

                /* Disk Table인 경우 Record Read를 위한 공간을 할당한다. */
                IDE_TEST( qdbCommon::getDiskRowSize( sPartHandle,
                                                     & sRowSize )
                          != IDE_SUCCESS );

                IDU_LIMITPOINT("qdbCommon::reorganizeForHashPartition::malloc5");
                IDE_TEST( aStatement->qmxMem->cralloc( sRowSize,
                                                       (void **) & sDiskRow )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            sOldRow = sDiskRow;
        }
        else
        {
            sOldRow = NULL;
        }

        //----------------------------------------
        // src table open
        //----------------------------------------

        SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sCursorProperty, aStatement->mStatistics );
        sCursorProperty.mLockWaitMicroSec = 0;
        sCursorProperty.mIsUndoLogging = ID_FALSE;

        /* PROJ-2464 hybrid partitioned table 지원
         *  - Partition Type으로 결정하도록 수정한다.
         */
        if ( ( sPartInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            sCursorProperty.mFetchColumnList = sFetchColumnList;
        }
        else
        {
            sCursorProperty.mFetchColumnList = NULL;
        }

        sSrcTblCursor.initialize();

        IDE_TEST( sSrcTblCursor.open( QC_SMI_STMT( aStatement ),
                                      sPartHandle,
                                      NULL,
                                      smiGetRowSCN(sPartHandle),
                                      NULL,
                                      smiGetDefaultKeyRange(),
                                      smiGetDefaultKeyRange(),
                                      smiGetDefaultFilter(),
                                      SMI_LOCK_READ|
                                      SMI_TRAVERSE_FORWARD|
                                      SMI_PREVIOUS_DISABLE,
                                      SMI_SELECT_CURSOR,
                                      & sCursorProperty )
                  != IDE_SUCCESS);
        sSrcStage = 1;

        IDE_TEST(sSrcTblCursor.beforeFirst() != IDE_SUCCESS);
        IDE_TEST(sSrcTblCursor.readRow( &sOldRow,
                                        & sGRID, SMI_FIND_NEXT )
                 != IDE_SUCCESS);

        while (sOldRow != NULL)
        {
            // To Fix PR-11704
            // 레코드 건수에 비례하여 메모리가 증가하지 않도록 해야 함.
            // Memory 재사용을 위하여 현재 위치 기록
            IDE_TEST( aStatement->qmxMem->getStatus(&sQmxMemStatus)
                      != IDE_SUCCESS);

            (void)qmx::clearLobInfo( sLobInfo );

            //------------------------------------------
            // INSERT를 수행
            //------------------------------------------
            
            // 해당 Row가 어떤 파티션으로 이동할지 결정한다.
            IDE_TEST( checkMoveRowPartitionByHash( (void *)sOldRow,
                                                   sPartInfo,
                                                   sPartInfo->columns,
                                                   aDstPartCount,
                                                   & sDstPartNum)
                      != IDE_SUCCESS );

            IDE_TEST(qdbAlter::makeNewRow( QC_PRIVATE_TMPLATE(aStatement),
                                           NULL,
                                           sPartInfo->columns,
                                           sDstQcmColumns[sDstPartNum],
                                           sOldRow,
                                           sNewRow,
                                           &sSrcTblCursor,
                                           sGRID,
                                           sLobInfo,
                                           NULL)
                     != IDE_SUCCESS);

            IDE_TEST(sDstTblCursor[sDstPartNum].insertRow(sNewRow,
                                                          &sInsRow,
                                                          &sInsGRID)
                     != IDE_SUCCESS);

            //------------------------------------------
            // INSERT를 수행후 Lob 컬럼을 처리
            //------------------------------------------

            IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                  sLobInfo,
                                                  &sDstTblCursor[sDstPartNum],
                                                  sInsRow,
                                                  sInsGRID )
                      != IDE_SUCCESS );

            //------------------------------------------
            // INSERT를 수행후 non-partitioned index를 처리
            //------------------------------------------
            
            sPartOID = aDstPartOID[sDstPartNum];

            if ( aNewIndexTables != NULL )
            {
                IDE_TEST( qdx::insertIndexTableCursors( &sIndexTableCursorInfo,
                                                        sNewRow,
                                                        sPartOID,
                                                        sInsGRID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            // BUG-42920 DDL print data move progress
            IDE_TEST( qdbAlter::printProgressLog4Partition( aTableInfo,
                                                                 sPartInfo,
                                                                 &sProgressCnt,
                                                                 ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( aStatement->qmxMem->setStatus(&sQmxMemStatus)
                      != IDE_SUCCESS);

            IDE_TEST( sSrcTblCursor.readRow(&sOldRow, &sGRID, SMI_FIND_NEXT)
                      != IDE_SUCCESS);
        }

        // BUG-42920 DDL print data move progress
        IDE_TEST( qdbAlter::printProgressLog4Partition( aTableInfo,
                                                        sPartInfo,
                                                        &sProgressCnt,
                                                        ID_TRUE )
                  != IDE_SUCCESS );

        sSrcStage = 0;
        IDE_TEST(sSrcTblCursor.close() != IDE_SUCCESS);
    }

    // ---------------------------------------
    // 열었던 커서를 모두 닫는다.
    // ---------------------------------------
    while( sDstStage > 0 )
    {
        sDstStage--;
        IDE_TEST(sDstTblCursor[sDstStage].close() != IDE_SUCCESS);
    }

    // close index table cursor
    if ( aNewIndexTables != NULL )
    {
        IDE_TEST( qdx::closeInsertIndexTableCursors(
                      &sIndexTableCursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    while( sDstStage > 0 )
    {
        sDstStage--;
        (void)sDstTblCursor[sDstStage].close();
    }

    if ( sInitedCursorInfo == ID_TRUE )
    {
        qdx::finalizeInsertIndexTableCursors(
            &sIndexTableCursorInfo );
    }
    else
    {
        // Nothing to do.
    }
    
    if( sSrcStage == 1 )
    {
        (void)sSrcTblCursor.close();
        (void)qmx::finalizeLobInfo( aStatement, sLobInfo );
    }

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkMoveRowPartitionByHash(
    void                 * aRow,
    qcmTableInfo         * aPartInfo,
    qcmColumn            * aColumns,
    UInt                   aPartCount,
    UInt                 * aDstPartNum )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      ADD PARTITION, COALESCE PARTITION 시,
 *      해당 Row를 어떤 파티션으로 이동시킬지
 *      hash 함수를 이용해서 결정한다.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt                  sColCount;
    qmsPartCondValList    sValueCondVal;
    void                * sValue;
    qcmColumn           * sColumn;
    UInt                  sKeyColumnOrder;
    UInt                  sColumnOrder;

    // ---------------------------------------------------
    // 해당 Row의 파티션 키 컬럼을 qmsPartCondValList로 만든다.
    // ---------------------------------------------------
    // sValueCondVal을 구성한다.
    sValueCondVal.partCondValCount = aPartInfo->partKeyColCount;
    sValueCondVal.partCondValType = QMS_PARTCONDVAL_NORMAL;

    for( sColCount = 0;
         sColCount < aPartInfo->partKeyColCount;
         sColCount++ )
    {
        sKeyColumnOrder =
            (aPartInfo->partKeyColumns[sColCount].basicInfo->column.id & SMI_COLUMN_ID_MASK);

        sColumn = aColumns;
        while (sColumn != NULL)
        {
            sColumnOrder = (sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK);
            if( sKeyColumnOrder == sColumnOrder )
            {
                sValue = (void *)
                    mtc::value( sColumn->basicInfo,
                                aRow,
                                MTD_OFFSET_USE );
                sValueCondVal.partCondValues[sColCount] = sValue;
                break;
            }
            sColumn = sColumn->next;
        }
    }

    // ---------------------------------------------------
    // HASH 값 구하기
    // ---------------------------------------------------
    IDE_TEST( qmoPartition::getPartOrder( aPartInfo->partKeyColumns,
                                          aPartCount,
                                          &sValueCondVal,
                                          aDstPartNum )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* 여러 개의 Attribute Flag List의 Flag값을
   Bitwise Or연산 하여 하나의 UInt 형의 Flag 값을 만든다

   [IN] aAttrFlagList - Table Attribute Flag의 List
   [OUT] aAttrFlagMask  - Bitwise OR된 Flag의 Mask
   [OUT] aAttrFlagValue - Bitwise OR된 Flag의 Value
*/
IDE_RC qdbCommon::getTableAttrFlagFromList(qdTableAttrFlagList * aAttrFlagList,
                                           UInt              * aAttrFlagMask,
                                           UInt              * aAttrFlagValue )
{
    IDE_DASSERT( aAttrFlagList  != NULL );
    IDE_DASSERT( aAttrFlagMask  != NULL );
    IDE_DASSERT( aAttrFlagValue != NULL );

    UInt sAttrFlagMask  = 0;
    UInt sAttrFlagValue = 0;

    for ( ; aAttrFlagList != NULL ; aAttrFlagList = aAttrFlagList->next )
    {
        sAttrFlagMask  |= aAttrFlagList->attrMask;
        sAttrFlagValue |= aAttrFlagList->attrValue;
    }

    *aAttrFlagValue = sAttrFlagValue;
    *aAttrFlagMask  = sAttrFlagMask;

    return IDE_SUCCESS;
}


/*
    Table의 Attribute Flag List에 대한 Validation수행

   [IN] qcStatement - Table Attribute가 사용된 Statement
   [IN] aAttrFlagList - Table Attribute Flag의 List
 */
IDE_RC qdbCommon::validateTableAttrFlagList(qcStatement         * aStatement,
                                            qdTableAttrFlagList * aAttrFlagList)
{
    IDE_DASSERT( aAttrFlagList != NULL );

    // 같은 이름의 Attribute Flag가 존재 하면 에러 처리
    IDE_TEST( checkTableAttrIsUnique( aStatement,
                                    aAttrFlagList ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Table의 Attribute Flag List에서 동일한
    Attribute List가 존재할 경우 에러처리

    ex>  동일한 Attribute가 다른 값으로 두 번 나오면 에러
         COMPRESSED LOGGING UNCOMPRESSED LOGGING

    Detect할 방법 :
         Mask가 겹치는 Attribute가 존재하는지 체크한다.

   [IN] qcStatement - Table Attribute가 사용된 Statement
   [IN] aAttrFlagList - Table Attribute Flag의 List
 */
IDE_RC qdbCommon::checkTableAttrIsUnique(qcStatement         * aStatement,
                                         qdTableAttrFlagList * aAttrFlagList)
{
    IDE_DASSERT( aAttrFlagList != NULL );

    qcuSqlSourceInfo      sqlInfo;
    qdTableAttrFlagList * sAttrFlag;

    for ( ; aAttrFlagList != NULL ; aAttrFlagList = aAttrFlagList->next )
    {
        for ( sAttrFlag = aAttrFlagList->next;
              sAttrFlag != NULL;
              sAttrFlag = sAttrFlag->next )
        {
            if ( ( aAttrFlagList->attrMask & sAttrFlag->attrMask ) != 0 )
            {
                IDE_RAISE(err_same_duplicate_attribute);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_same_duplicate_attribute);
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sAttrFlag->attrPosition );

        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDB_DUPLICATE_TABLE_ATTRIBUTE,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::setIndexKeyColumnTypeFlag( mtcColumn * aKeyColumn )
{
/***********************************************************************
 *
 * PROJ-1705
 * 디스크레코드 최적화 프로젝트 수행으로
 * 디스크테이블에는 variable 컬럼속성이 없다.
 * 그런데,
 * 디스크테이블의 인덱스레코드 구성시,
 * 예를 들어,
 * varchar column에 대해서 fixed로 처리할 경우,
 * varchar column의 최대 사이즈에 대한 공간이 할당되어야 하고,
 * 이 경우 예전보다 인덱스페이즈를 엄청나게 많이 사용하게 된다.
 * 이런 위험요소를 제거하기 위해
 * variable 컬럼속성지정으로 인덱스 레코드공간을 절약할 수 있는 타입에 한해서
 * SMI_COLUMN_TYPE의 flag를 variable로 내려준다.
 * 즉,
 * 디스크 테이블 레코드는 fixed, lob의 컬럼속성을( variable컬럼속성없음 )
 * 가지는 반면,
 * 디스크 인덱스 레코드는 fixed, variable 컬럼속성을 가진다. (variable컬럼속성있음)
 ************************************************************************/

    IDE_DASSERT( ( aKeyColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                 == SMI_COLUMN_STORAGE_DISK );

    if( ( ( aKeyColumn->column.flag & SMI_COLUMN_TYPE_MASK )
          == SMI_COLUMN_TYPE_FIXED ) &&
        ( ( aKeyColumn->module->flag & MTD_COLUMN_TYPE_MASK )
          == MTD_COLUMN_TYPE_VARIABLE ) && 
        ( ( aKeyColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
          == SMI_COLUMN_COMPRESSION_FALSE) ) // PROJ-2429
    {
        if( aKeyColumn->column.size > smiGetVariableColumnSize4DiskIndex() )
        {
            aKeyColumn->column.flag &= ~SMI_COLUMN_TYPE_MASK;
            aKeyColumn->column.flag |= SMI_COLUMN_TYPE_VARIABLE;
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-1872에 Length가 정해진 타입인지 아닌지에 대한
    // 정보를 smiColumn의 flag에 남김
    // Length가 정해진 타입은 Length를 저장하지 않고
    // value만 저장하기 위해서임
    if ( ( aKeyColumn->module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK )
         == MTD_VARIABLE_LENGTH_TYPE_FALSE )
    {
        aKeyColumn->column.flag &= ~SMI_COLUMN_LENGTH_TYPE_MASK;
        aKeyColumn->column.flag |= SMI_COLUMN_LENGTH_TYPE_KNOWN;
    }
    else
    {
        aKeyColumn->column.flag &= ~SMI_COLUMN_LENGTH_TYPE_MASK;
        aKeyColumn->column.flag |= SMI_COLUMN_LENGTH_TYPE_UNKNOWN;
    }

    return IDE_SUCCESS;
}


IDE_RC qdbCommon::makeFetchColumnList4TupleID(
                      qcTemplate             * aTemplate,
                      UShort                   aTupleRowID,
                      idBool                   aIsNeedAllFetchColumn,
                      qcmIndex               * aIndex,
                      idBool                   aIsAllocFetchColumnList,
                      smiFetchColumnList    ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 * Implementation :
 *
 ***********************************************************************/

    smiFetchColumnList    * sFetchColumnList = NULL;
    smiFetchColumnList    * sFetchColumn;
    UShort                  sFetchColumnCount = 0;
    mtcTuple              * sTuple;
    UShort                  sCnt;
    UInt                    sKeyColCnt;
    UShort                  i;
    idBool                  sIsFindFetchIndexColumn;

    sTuple = &(aTemplate->tmplate.rows[aTupleRowID]);

    if( aIsNeedAllFetchColumn == ID_TRUE )
    {
        //------------------------------------
        // trigger row가 필요한 경우
        // partitioned table update가 rowMovement인 경우
        // 테이블의 전체 컬럼을 fetch column list로 구성한다.
        //------------------------------------

        if( aIsAllocFetchColumnList == ID_TRUE )
        {
            IDU_LIMITPOINT("qdbCommon::makeFetchColumnList4TupleID::malloc1");
            IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                          ID_SIZEOF(smiFetchColumnList) * sTuple->columnCount,
                          (void**)&sFetchColumnList )
                      != IDE_SUCCESS);
        }
        else
        {
            // Nothing To Do
            IDE_DASSERT( *aFetchColumnList != NULL );
            sFetchColumnList = *aFetchColumnList;
        }

        for( sCnt = 0; sCnt < sTuple->columnCount; sCnt++ )
        {
            sFetchColumn = sFetchColumnList + sCnt;
            sFetchColumn->column = (smiColumn*)&(sTuple->columns[sCnt]);
            sFetchColumn->columnSeq = SMI_GET_COLUMN_SEQ( sFetchColumn->column );
            sFetchColumn->copyDiskColumn = 
                getCopyDiskColumnFunc( &sTuple->columns[sCnt] );

            if ( sCnt == (sTuple->columnCount - 1) )
            {
                sFetchColumn->next = NULL;
                break;
            }
            else
            {
                sFetchColumn->next = sFetchColumn + 1;
            }
        }
    }
    else
    {
        //------------------------------------
        // trigger row가 필요하지 않은 경우로
        // validation시 수집한 컬럼정보로 fetch column list로 구성한다.
        //------------------------------------

        // 패치컬럼 갯수 계산
        for( sCnt = 0; sCnt < sTuple->columnCount; sCnt++ )
        {
            sIsFindFetchIndexColumn = ID_FALSE;

            if( aIndex != NULL )
            {
                for( sKeyColCnt = 0; sKeyColCnt < aIndex->keyColCount; sKeyColCnt++ )
                {
                    if( sTuple->columns[sCnt].column.id ==
                        aIndex->keyColumns[sKeyColCnt].column.id )
                    {
                        sIsFindFetchIndexColumn = ID_TRUE;
                        break;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
            }
            else
            {
                // Nothing To Do
            }

            if( ( ( sTuple->columns[sCnt].flag & MTC_COLUMN_USE_COLUMN_MASK )
                == MTC_COLUMN_USE_COLUMN_TRUE )
                ||
                ( sIsFindFetchIndexColumn == ID_TRUE ) )
            {
                sFetchColumnCount++;
            }
            else
            {
                // Nothing To Do
            }
        }

        if( sFetchColumnCount == 0 )
        {
            // 예 : SELECT COUNT(*) FROM T1, T2;
            // 이 경우 T1, T2 테일블에 대한 레코드 패치시
            // 복사가 필요한 컬럼이 존재하지 않는다.
            sFetchColumnList = NULL;
        }
        else
        {
            if( aIsAllocFetchColumnList == ID_TRUE )
            {
                IDU_LIMITPOINT("qdbCommon::makeFetchColumnList4TupleID::malloc2");
                IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                              ID_SIZEOF(smiFetchColumnList) * sFetchColumnCount,
                              (void**)&sFetchColumnList )
                          != IDE_SUCCESS);
            }
            else
            {
                // Nothing To Do
                IDE_DASSERT( *aFetchColumnList != NULL );
                sFetchColumnList = *aFetchColumnList;
            }

            for( sCnt = 0, i = 0; sCnt < sTuple->columnCount; sCnt++ )
            {
                sIsFindFetchIndexColumn = ID_FALSE;

                if( aIndex != NULL )
                {
                    for( sKeyColCnt = 0; sKeyColCnt < aIndex->keyColCount; sKeyColCnt++ )
                    {
                        if( sTuple->columns[sCnt].column.id ==
                            aIndex->keyColumns[sKeyColCnt].column.id )
                        {
                            sIsFindFetchIndexColumn = ID_TRUE;
                            break;
                        }
                        else
                        {
                            // Nothing To Do
                        }
                    }
                }
                else
                {
                    // Nothing To Do
                }

                if( ( ( sTuple->columns[sCnt].flag & MTC_COLUMN_USE_COLUMN_MASK )
                    == MTC_COLUMN_USE_COLUMN_TRUE )
                    ||
                    ( sIsFindFetchIndexColumn == ID_TRUE ) )
                {
                    sFetchColumn = sFetchColumnList + i;
                    sFetchColumn->column = (smiColumn*)&(sTuple->columns[sCnt]);
                    sFetchColumn->columnSeq = SMI_GET_COLUMN_SEQ( sFetchColumn->column );
                    sFetchColumn->copyDiskColumn = 
                        getCopyDiskColumnFunc( &sTuple->columns[sCnt] );

                    if ( i == (sFetchColumnCount - 1) )
                    {
                        sFetchColumn->next = NULL;
                        break;
                    }
                    else
                    {
                        sFetchColumn->next = sFetchColumn + 1;
                    }

                    i++;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
    }

    *aFetchColumnList = sFetchColumnList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeWhereClauseColumnList( qcStatement     * aStatement,
                                             UShort            aTupleRowID,
                                             smiColumnList  ** aColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 * Implementation :
 *
 ***********************************************************************/

    smiColumnList    * sColumnList = NULL;
    smiColumnList    * sColumn;
    UShort             sColumnCount = 0;
    mtcTuple         * sTuple;
    UShort             sCnt;
    UShort             i;

    sTuple = QC_SHARED_TMPLATE(aStatement)->tmplate.rows + aTupleRowID;

    // 패치컬럼 갯수 계산
    for( sCnt = 0; sCnt < sTuple->columnCount; sCnt++ )
    {
        if( ( sTuple->columns[sCnt].flag & MTC_COLUMN_USE_WHERE_MASK )
            == MTC_COLUMN_USE_WHERE_TRUE )
        {
            sColumnCount++;
        }
        else
        {
            // Nothing To Do
        }
    }

    if( sColumnCount == 0 )
    {
        sColumnList = NULL;
    }
    else
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(smiColumnList) * sColumnCount,
                      (void**)&sColumnList )
                  != IDE_SUCCESS);

        for( sCnt = 0, i = 0; sCnt < sTuple->columnCount; sCnt++ )
        {
            if( ( sTuple->columns[sCnt].flag & MTC_COLUMN_USE_WHERE_MASK )
                == MTC_COLUMN_USE_WHERE_TRUE )
            {
                sColumn = sColumnList + i;
                sColumn->column = (smiColumn*)&(sTuple->columns[sCnt]);

                if ( i == (sColumnCount - 1) )
                {
                    sColumn->next = NULL;
                    break;
                }
                else
                {
                    sColumn->next = sColumn + 1;
                }

                i++;
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    *aColumnList = sColumnList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeSetClauseColumnList( qcStatement     * aStatement,
                                           UShort            aTupleRowID,
                                           smiColumnList  ** aColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 * Implementation :
 *
 ***********************************************************************/

    smiColumnList    * sColumnList = NULL;
    smiColumnList    * sColumn;
    UShort             sColumnCount = 0;
    mtcTuple         * sTuple;
    UShort             sCnt;
    UShort             i;

    sTuple = QC_SHARED_TMPLATE(aStatement)->tmplate.rows + aTupleRowID;

    // 패치컬럼 갯수 계산
    for( sCnt = 0; sCnt < sTuple->columnCount; sCnt++ )
    {
        if( ( sTuple->columns[sCnt].flag & MTC_COLUMN_USE_SET_MASK )
            == MTC_COLUMN_USE_SET_TRUE )
        {
            sColumnCount++;
        }
        else
        {
            // Nothing To Do
        }
    }

    if( sColumnCount == 0 )
    {
        sColumnList = NULL;
    }
    else
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(smiColumnList) * sColumnCount,
                      (void**)&sColumnList )
                  != IDE_SUCCESS);

        for( sCnt = 0, i = 0; sCnt < sTuple->columnCount; sCnt++ )
        {
            if( ( sTuple->columns[sCnt].flag & MTC_COLUMN_USE_SET_MASK )
                == MTC_COLUMN_USE_SET_TRUE )
            {
                sColumn = sColumnList + i;
                sColumn->column = (smiColumn*)&(sTuple->columns[sCnt]);

                if ( i == (sColumnCount - 1) )
                {
                    sColumn->next = NULL;
                    break;
                }
                else
                {
                    sColumn->next = sColumn + 1;
                }

                i++;
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    *aColumnList = sColumnList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeFetchColumnList4Index(
                      qcTemplate             * aTemplate,
                      qcmTableInfo           * aTableInfo,
                      qcmIndex               * aIndex,
                      idBool                   aIsAllocFetchColumnList,
                      smiFetchColumnList    ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 * 예) table parent( i1, i2, i3 ) , unique key( i1, i2 )
 *     table child( i1, i2, i3 )  , foreign key( i2, i3 ) => parent( i1, i2 )
 *
 *     insert into child value ( 1, 2, 3 );
 *     과 같이 child table에 레코드입력시,
 *     parent table에 참조되는 레코드가 존재하는지 검사한다.
 *     이 parent table의 레코드 검색시,
 *     parent table의 unique key로 range 검색을 하게 되는데,
 *     이 인덱스의 컬럼정보들로 fetch column list를 생성한다.
 *
 *     현재는 포린키 체크시 이 함수가 호출됨.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiFetchColumnList    * sFetchColumnList = NULL;
    smiFetchColumnList    * sFetchColumn;
    UInt                    i;
    UShort                  sCnt;
    UInt                    sKeyColCnt;

    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aIndex     != NULL );

    if( aIsAllocFetchColumnList == ID_TRUE )
    {
        IDU_LIMITPOINT("qdbCommon::makeFetchColumnList4Index::malloc");
        IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                      ID_SIZEOF(smiFetchColumnList) * aIndex->keyColCount,
                      (void**)&sFetchColumnList )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
        IDE_DASSERT( *aFetchColumnList != NULL );
        sFetchColumnList = *aFetchColumnList;
    }

    for( sCnt = 0, i = 0; sCnt < aTableInfo->columnCount; sCnt++ )
    {
        for( sKeyColCnt = 0; sKeyColCnt < aIndex->keyColCount; sKeyColCnt++ )
        {
            if( aTableInfo->columns[sCnt].basicInfo->column.id ==
                aIndex->keyColumns[sKeyColCnt].column.id )
            {
                sFetchColumn = sFetchColumnList + i;
                sFetchColumn->column =
                    (smiColumn*)(aTableInfo->columns[sCnt].basicInfo);
                sFetchColumn->columnSeq = SMI_GET_COLUMN_SEQ( sFetchColumn->column );
                sFetchColumn->copyDiskColumn = 
                    getCopyDiskColumnFunc( aTableInfo->columns[sCnt].basicInfo );

                if ( i == (aIndex->keyColCount - 1) )
                {
                    sFetchColumn->next = NULL;
                }
                else
                {
                    sFetchColumn->next = sFetchColumn + 1;
                }

                i++;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if( i == aIndex->keyColCount )
        {
            break;
        }
        else
        {
            // Nothing To Do
        }
    }

    *aFetchColumnList = sFetchColumnList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::makeFetchColumnList4ChildTable(
                      qcTemplate             * aTemplate,
                      qcmTableInfo           * aTableInfo,
                      qcmForeignKey          * aForeignKey,
                      qcmIndex               * aIndex,
                      idBool                   aIsAllocFetchColumnList,
                      smiFetchColumnList    ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 * 예) table parent( i1, i2, i3 ) , unique key( i1, i2 )
 *     table child( i1, i2, i3 )  , foreign key( i2, i3 ) => parent( i1, i2 )
 *
 *     delete from parent;
 *     과 같이 parent table에 레코드 삭제시,
 *     child table에 참조하는 레코드가 존재하는지 검사한다.
 *     이 child table의 레코드 검색시,
 *     child table의 foreign key로 검색을 하게 되는데,
 *     이 컬럼정보들로 fetch column list를 생성한다.
 *
 *     BUG-26122 Disk Index의 FetchColumnList에 대한 가정이 잘못되어 있습니다.
 *      : child table에 대한 검색시,
 *        인덱스스캔을 할 수도 있으므로,
 *        포린키외에도 인덱스컬럼에 속하는 컬럼리스트들을 같이 내려주어야 한다.
 *
 *     현재는 포린키 체크시 이 함수가 호출됨.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiFetchColumnList    * sFetchColumnList = NULL;
    smiFetchColumnList    * sFetchColumn;
    UInt                    i;
    UShort                  sCnt;
    UInt                    sIndexColCnt;
    UInt                    sConstColCnt;
    UShort                  sFetchColumnCount = 0;
    idBool                  sIsFetchColumn;

    IDE_DASSERT( aTableInfo  != NULL );
    IDE_DASSERT( aForeignKey != NULL );

    if( aIsAllocFetchColumnList == ID_TRUE )
    {
        // BUG-26122 Disk Index의 FetchColumnList에 대한 가정이 잘못되어 있습니다.
        // : child table에 대한 검색시,
        //   인덱스스캔을 할 수도 있으므로,
        //   포린키외에도 인덱스컬럼에 속하는 컬럼리스트들을 같이 내려주어야 한다.

        if( aIndex != NULL )
        {
            sFetchColumnCount = aIndex->keyColCount + aForeignKey->constraintColumnCount;

            for( sConstColCnt = 0; sConstColCnt < aForeignKey->constraintColumnCount; sConstColCnt++ )
            {
                for( sCnt = 0; sCnt < aIndex->keyColCount; sCnt++ )
                {
                    if( aForeignKey->referencingColumn[sConstColCnt] ==
                        aIndex->keyColumns[sCnt].column.id )
                    {
                        sFetchColumnCount--;
                        break;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
            }
        }
        else
        {
            sFetchColumnCount = aForeignKey->constraintColumnCount;
        }

        IDU_LIMITPOINT("qdbCommon::makeFetchColumnList4ChildTable::malloc");
        IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                      ID_SIZEOF(smiFetchColumnList) * sFetchColumnCount,
                      (void**)&sFetchColumnList )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
        IDE_DASSERT( *aFetchColumnList != NULL );
        sFetchColumnList = *aFetchColumnList;
    }

    for( sCnt = 0, i = 0; sCnt < aTableInfo->columnCount; sCnt++ )
    {
        sIsFetchColumn = ID_FALSE;

        if( aIndex != NULL )
        {
            for( sIndexColCnt = 0; sIndexColCnt < aIndex->keyColCount; sIndexColCnt++ )
            {
                if( aTableInfo->columns[sCnt].basicInfo->column.id ==
                    aIndex->keyColumns[sIndexColCnt].column.id )
                {
                    sIsFetchColumn = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
        else
        {
            // Nothing To Do
        }

        if( sIsFetchColumn == ID_FALSE )
        {
            for( sConstColCnt = 0;
                 sConstColCnt < aForeignKey->constraintColumnCount;
                 sConstColCnt++ )
            {
                if( aTableInfo->columns[sCnt].basicInfo->column.id ==
                    aForeignKey->referencingColumn[sConstColCnt] )
                {
                    sIsFetchColumn = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
        else
        {
            // Noting To Do
        }

        if( sIsFetchColumn == ID_TRUE )
        {
            sFetchColumn = sFetchColumnList + i;
            sFetchColumn->column =
                (smiColumn*)(aTableInfo->columns[sCnt].basicInfo);
            sFetchColumn->columnSeq = SMI_GET_COLUMN_SEQ( sFetchColumn->column );
            sFetchColumn->copyDiskColumn = 
                getCopyDiskColumnFunc( aTableInfo->columns[sCnt].basicInfo );

            if ( i == ((UInt)sFetchColumnCount - 1) )
            {
                sFetchColumn->next = NULL;
                break;
            }
            else
            {
                sFetchColumn->next = sFetchColumn + 1;
            }

            i++;
        }
        else
        {
            // Nothing To Do
        }
    }

    *aFetchColumnList = sFetchColumnList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::makeFetchColumnList(
                      qcTemplate           * aTemplate,
                      UInt                   aColumnCount,
                      qcmColumn            * aColumn,
                      idBool                 aIsAllocFetchColumnList,
                      smiFetchColumnList  ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 *    이 함수는 execution 시점에만 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiFetchColumnList    * sFetchColumnList = NULL;
    smiFetchColumnList    * sFetchColumn;
    UInt                    sCnt;

    if( aIsAllocFetchColumnList == ID_TRUE )
    {
        IDU_LIMITPOINT("qdbCommon::makeFetchColumnList::malloc");
        IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                      ID_SIZEOF(smiFetchColumnList) * aColumnCount,
                      (void**)&sFetchColumnList )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
        IDE_DASSERT( *aFetchColumnList != NULL );
        sFetchColumnList = *aFetchColumnList;
    }

    for( sCnt = 0; sCnt < aColumnCount; sCnt++ )
    {
        sFetchColumn = sFetchColumnList + sCnt;
        sFetchColumn->column = (smiColumn*)&(aColumn[sCnt].basicInfo->column);
        sFetchColumn->columnSeq = SMI_GET_COLUMN_SEQ( sFetchColumn->column );
        sFetchColumn->copyDiskColumn = 
            getCopyDiskColumnFunc( aColumn[sCnt].basicInfo );

        if( sCnt == (aColumnCount - 1) )
        {
            sFetchColumn->next = NULL;
            break;
        }
        else
        {
            sFetchColumn->next = sFetchColumn + 1;
        }
    }

    *aFetchColumnList = sFetchColumnList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdbCommon::initFetchColumnList( smiFetchColumnList  ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 *    이 함수는 execution 시점에만 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    *aFetchColumnList = NULL;
}

IDE_RC qdbCommon::addFetchColumnList( iduMemory            * aMemory,
                                      mtcColumn            * aColumn,
                                      smiFetchColumnList  ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description : sm으로부터 레코드패치시 복사가 필요한 컬럼정보생성
 *
 *    이 함수는 execution 시점에만 호출되어야 한다.
 *
 * Implementation :
 *
 *    fetch할 column의 순서는 table의 column 순서와 같아야 한다.
 *    fetch할 column은 중복되어서는 안된다.
 *
 ***********************************************************************/

    smiFetchColumnList    * sPreFetchColumn;
    smiFetchColumnList    * sCurFetchColumn;
    smiFetchColumnList    * sFetchColumn;
    idBool                  sExist = ID_FALSE;

    IDE_DASSERT( aFetchColumnList != NULL );

    sPreFetchColumn = NULL;
    sCurFetchColumn = *aFetchColumnList;

    // fetch column list에서 smiColumn.id를 이용하여 올바른 위치를 결정한다.
    while ( sCurFetchColumn != NULL )
    {
        if ( sCurFetchColumn->column->id < aColumn->column.id )
        {
            // 계속 전진한다.
        }
        else
        {
            if ( sCurFetchColumn->column->id == aColumn->column.id )
            {
                // 이미 fetch column list에 존재한다.
                sExist = ID_TRUE;
                break;
            }
            else /* sCurFetchColumn->column->id > aColumn->column.id */
            {
                // 현재 fetch column 앞에 추가해야 한다.
                break;
            }
        }

        sPreFetchColumn = sCurFetchColumn;
        sCurFetchColumn = sCurFetchColumn->next;
    }

    // fetch column list에 추가한다.
    if ( sExist == ID_FALSE )
    {
        // fetch column을 생성한다.
        IDU_LIMITPOINT("qdbCommon::addFetchColumnList::malloc");
        IDE_TEST( aMemory->alloc( ID_SIZEOF(smiFetchColumnList),
                                  (void**) & sFetchColumn )
                  != IDE_SUCCESS );

        sFetchColumn->column = & aColumn->column;
        sFetchColumn->columnSeq = SMI_GET_COLUMN_SEQ( sFetchColumn->column );
        sFetchColumn->copyDiskColumn = getCopyDiskColumnFunc( aColumn );

        if ( sCurFetchColumn == NULL )
        {
            if ( sPreFetchColumn == NULL )
            {
                // 최초 추가하는 경우
                sFetchColumn->next = NULL;
                *aFetchColumnList = sFetchColumn;
            }
            else
            {
                // 마지막에 추가하는 경우
                sPreFetchColumn->next = sFetchColumn;
                sFetchColumn->next = NULL;
            }
        }
        else
        {
            if ( sPreFetchColumn == NULL )
            {
                // 맨 앞에 추가하는 경우
                sFetchColumn->next = sCurFetchColumn;
                *aFetchColumnList = sFetchColumn;
            }
            else
            {
                // 중간에 추가하는 경우
                sPreFetchColumn->next = sFetchColumn;
                sFetchColumn->next = sCurFetchColumn;
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

IDE_RC qdbCommon::addCheckConstrListToFetchColumnList(
                      iduMemory           * aMemory,
                      qdConstraintSpec    * aCheckConstrList,
                      qcmColumn           * aColumnArray,
                      smiFetchColumnList ** aFetchColumnList )
{
/***********************************************************************
 *
 * Description :
 *  Check Constraint에 관련된 Column을 Fetch Column List에 추가한다.
 *      - 이 함수는 Execution 시점에만 호출되어야 한다.
 *
 * Implementation :
 *  각 Check Constraint에 대해 아래를 반복한다.
 *  1. 관련 Column을 Fetch Column List에 추가한다.
 *
 ***********************************************************************/

    qdConstraintSpec   * sConstr;
    qcmColumn          * sSrcColumn;
    qcmColumn          * sDstColumn;

    for ( sConstr = aCheckConstrList;
          sConstr != NULL;
          sConstr = sConstr->next )
    {
        for ( sSrcColumn = sConstr->constraintColumns;
              sSrcColumn != NULL;
              sSrcColumn = sSrcColumn->next )
        {
            /* PROJ-2464 hybrid partitioned table 지원
             *  Disk Column을 찾아서 Fetch Column List에 추가한다.
             */
            sDstColumn = &(aColumnArray[sSrcColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK]);

            IDE_TEST( addFetchColumnList( aMemory,
                                          sDstColumn->basicInfo,
                                          aFetchColumnList )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::convertToUTypeString( qcStatement   * aStatement,
                                        UInt            aSrcValOffset,
                                        UInt            aSrcLen,
                                        qcNamePosList * aNcharList,
                                        SChar         * aDest,
                                        UInt            aBufferSize )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1579 NCHAR
 *
 *      CREATE/ALTER TABLE에서 호출되는 함수
 *
 *      N'안'과 같은 스트링을 메타테이블에 저장할 경우
 *      ALTIBASE_NLS_NCHAR_LITERAL_REPLACE = 1 인 경우
 *      U'\C548'과 같이 저장된다.
 *
 * Implementation :
 *
 *      aStatement->namePosList가 stmt에 나온 순서대로 정렬되어 있다고
 *      가정한다.
 *
 *      EX) create table t1 ( i1 integer, i2 nvarchar default n'안녕' );
 *
 *      1. loop(n type이 있는 동안)
 *          1-1. u' memcpy
 *
 *          1-2. loop(n type literal을 캐릭터 단위로 반복)
 *              1) \ 복사
 *              2) 안 => C548와 같이 유니코드 포인트 형태로 변형해서 복사
 *                 (ASCII는 그대로 복사한다.)
 *
 *      2. ' 추가
 *
 ***********************************************************************/

    qcNamePosList   * sNcharList;
    qcNamePosition    sNamePos;
    const mtlModule * sClientCharSet;

    SChar           * sSrcVal;

    SChar           * sNTypeVal;
    UInt              sNTypeLen = 0;
    SChar           * sNTypeFence;

    UInt              sSrcValOffset = 0;
    UInt              sDestOffset = 0;
    UInt              sCharLength = 0;

    sClientCharSet = QCG_GET_SESSION_LANGUAGE(aStatement);

    sSrcVal = aStatement->myPlan->stmtText;
    sSrcValOffset = aSrcValOffset;

    for( sNcharList = aNcharList;
         sNcharList != NULL;
         sNcharList = sNcharList->next )
    {
        sNamePos = sNcharList->namePos;

        // -----------------------------------
        // N 바로 전까지 복사
        // -----------------------------------
        idlOS::memcpy( aDest + sDestOffset,
                       sSrcVal + sSrcValOffset,
                       sNamePos.offset - sSrcValOffset );

        sDestOffset += (sNamePos.offset - sSrcValOffset );

        // -----------------------------------
        // U'\ 복사
        // -----------------------------------
        idlOS::memcpy( aDest + sDestOffset,
                       "U\'",
                       2 );

        sDestOffset += 2;

        // -----------------------------------
        // N 타입 리터럴의 캐릭터 셋 변환
        // 클라이언트 캐릭터 셋 => 내셔널 캐릭터 셋
        // -----------------------------------
        sNTypeVal = aStatement->myPlan->stmtText + sNamePos.offset + 2;
        sNTypeLen = sNamePos.size - 3;
        sNTypeFence = sNTypeVal + sNTypeLen;

        IDE_TEST( mtf::makeUFromChar( sClientCharSet,
                                      (UChar *)sNTypeVal,
                                      (UChar *)sNTypeFence,
                                      (UChar *)aDest + sDestOffset,
                                      (UChar *)aDest + aBufferSize,
                                      & sCharLength )
                  != IDE_SUCCESS );

        sDestOffset += sCharLength;

        sSrcValOffset = sNamePos.offset + sNamePos.size - 1;
    }

    // -----------------------------------
    // 나머지 SrcVal 복사
    // -----------------------------------
    idlOS::memcpy( aDest + sDestOffset,
                   sSrcVal + sSrcValOffset,
                   aSrcLen - (sSrcValOffset - aSrcValOffset) );

    sDestOffset += (aSrcLen - (sSrcValOffset - aSrcValOffset) );

    aDest[sDestOffset] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeNcharLiteralStrForIndex(
    qcStatement     * aStatement,
    qcNamePosList   * aNcharList,
    qcmColumn       * aColumn )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1090 Function-based Index
 *
 *      CREATE INDEX의 validation때 호출되는 함수이다.
 *      인자로 넘어온 Column의 Default Expression에 nchar literal이 있을 때,
 *      해당 Column에 nchar literal의 namePos list를 달아준다.
 *
 *      달아준 namePos의 list는 execution 때, nchar literal=>unicode literal로
 *      변형할 때 사용된다.
 *      ex) n'안녕' => u'\C548\B155'
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool              sHaveNcharLiteral = ID_FALSE;
    qcNamePosList     * sTempNamePosList;
    qcNamePosList     * sNcharLiteral;
    qcNamePosList     * sTempNcharLiteral;
    qcNamePosition      sNamePos;

    for ( sTempNamePosList = aNcharList;
          sTempNamePosList != NULL;
          sTempNamePosList = sTempNamePosList->next )
    {
        sNamePos = sTempNamePosList->namePos;

        if ( ( sNamePos.offset >= aColumn->defaultValue->position.offset ) &&
             ( ( sNamePos.offset + sNamePos.size ) <=
                                ( aColumn->defaultValue->position.offset +
                                  aColumn->defaultValue->position.size ) ) )
        {
            sHaveNcharLiteral = ID_TRUE;
        }
        else
        {
            sHaveNcharLiteral = ID_FALSE;
        }

        if ( sHaveNcharLiteral == ID_TRUE )
        {
            /* 해당 Column이 Nchar Literal을 갖고 있다면, NamePosList를 달아준다.
             * 이 정보는 execution 때, nchar literal을 unicode literal로
             * 변형시킬 때 사용한다.
             */
            if ( aColumn->ncharLiteralPos != NULL )
            {
                for ( sNcharLiteral = aColumn->ncharLiteralPos;
                      sNcharLiteral->next != NULL;
                      sNcharLiteral = sNcharLiteral->next ) ;

                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcNamePosList),
                                                         (void**)&sTempNcharLiteral )
                          != IDE_SUCCESS );

                SET_POSITION( sTempNcharLiteral->namePos,
                              sTempNamePosList->namePos );
                sTempNcharLiteral->next = NULL;

                sNcharLiteral->next = sTempNcharLiteral;
            }
            else
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                        ID_SIZEOF(qcNamePosList),
                                        (void**)&(aColumn->ncharLiteralPos) )
                          != IDE_SUCCESS );

                SET_POSITION( aColumn->ncharLiteralPos->namePos,
                              sTempNamePosList->namePos );
                aColumn->ncharLiteralPos->next = NULL;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeNcharLiteralStrForConstraint(
    qcStatement      * aStatement,
    qcNamePosList    * aNcharList,
    qdConstraintSpec * aConstr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1107 Check Constraint 지원
 *
 *      CREATE TABLE의 validation때 호출되는 함수이다.
 *      인자로 넘어온 Constraint에 nchar literal이 있을 때,
 *      해당 Constraint에 nchar literal의 namePos list를 달아준다.
 *
 *      달아준 namePos의 list는 execution 때, nchar literal=>unicode literal로
 *      변형할 때 사용된다.
 *      ex) n'안녕' => u'\C548\B155'
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool              sHaveNcharLiteral = ID_FALSE;
    qcNamePosList     * sTempNamePosList;
    qcNamePosList     * sNcharLiteral;
    qcNamePosList     * sTempNcharLiteral;
    qcNamePosition      sNamePos;

    for ( sTempNamePosList = aNcharList;
          sTempNamePosList != NULL;
          sTempNamePosList = sTempNamePosList->next )
    {
        sNamePos = sTempNamePosList->namePos;

        if ( ( sNamePos.offset >= aConstr->checkCondition->position.offset ) &&
             ( ( sNamePos.offset + sNamePos.size ) <=
                                ( aConstr->checkCondition->position.offset +
                                  aConstr->checkCondition->position.size ) ) )
        {
            sHaveNcharLiteral = ID_TRUE;
        }
        else
        {
            sHaveNcharLiteral = ID_FALSE;
        }

        if ( sHaveNcharLiteral == ID_TRUE )
        {
            /* 해당 Constraint가 Nchar Literal을 갖고 있다면, NamePosList를 달아준다.
             * 이 정보는 execution 때, nchar literal을 unicode literal로
             * 변형시킬 때 사용한다.
             */
            if ( aConstr->ncharList != NULL )
            {
                for ( sNcharLiteral = aConstr->ncharList;
                      sNcharLiteral->next != NULL;
                      sNcharLiteral = sNcharLiteral->next ) ;

                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcNamePosList),
                                                         (void**)&sTempNcharLiteral )
                          != IDE_SUCCESS );

                SET_POSITION( sTempNcharLiteral->namePos,
                              sTempNamePosList->namePos );
                sTempNcharLiteral->next = NULL;

                sNcharLiteral->next = sTempNcharLiteral;
            }
            else
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                                        ID_SIZEOF(qcNamePosList),
                                        (void**)&(aConstr->ncharList) )
                          != IDE_SUCCESS );

                SET_POSITION( aConstr->ncharList->namePos,
                              sTempNamePosList->namePos );
                aConstr->ncharList->next = NULL;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdbCommon::removeNcharLiteralStr(
    qcNamePosList ** aFromList,
    qcNamePosList  * aTargetList )
{
/***********************************************************************
 *
 * Description :
 *  aFromList에서 aTargetList를 제거한다.
 *      - aFromList와 aTargetList는 같은 순서로 정렬되어 있다고 가정한다.
 *        (makeNcharLiteralStr으로 생성했다면, 순서가 같다)
 *
 * Implementation :
 *
 ***********************************************************************/

    qcNamePosList * sBefore = NULL;
    qcNamePosList * sFrom   = *aFromList;
    qcNamePosList * sTarget = aTargetList;

    while ( (sFrom != NULL) && (sTarget != NULL) )
    {
        if ( sFrom->namePos.offset == sTarget->namePos.offset )
        {
            IDE_DASSERT( sFrom->namePos.size == sTarget->namePos.size );

            if ( sBefore == NULL )
            {
                *aFromList = (*aFromList)->next;
            }
            else
            {
                sBefore->next = sFrom->next;
            }

            sFrom   = sFrom->next;
            sTarget = sTarget->next;
        }
        else if ( sFrom->namePos.offset < sTarget->namePos.offset )
        {
            sBefore = sFrom;
            sFrom   = sFrom->next;
        }
        else
        {
            sTarget = sTarget->next;
        }
    }

    return;
}

IDE_RC qdbCommon::makeNcharLiteralStr(
    qcStatement    * aStatement,
    qcNamePosList  * aNcharList,
    qcmColumn      * aColumn )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1579 NCHAR
 *
 *      CREATE TABLE/ALTER TABLE ADD COLUMN의 validation때 호출되는 함수이다.
 *      인자로 넘어온 컬럼에 nchar literal의 default value가 있을 때,
 *      해당 컬럼에 nchar literal의 namePos의 list를 달아준다.
 *
 *      달아준 namePos의 list는 execution 때, nchar literal=>unicode literal로
 *      변형할 때 사용된다.
 *      ex) n'안녕' => u'\C548\B155'
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool              sHaveNcharLiteral = ID_FALSE;
    qcNamePosList     * sTempNamePosList;
    qcNamePosList     * sNcharLiteral;
    qcNamePosList     * sTempNcharLiteral;
    qcNamePosition      sNamePos;

    for( sTempNamePosList = aNcharList;
         sTempNamePosList != NULL;
         sTempNamePosList = sTempNamePosList->next )
    {
        sNamePos = sTempNamePosList->namePos;

        if( sNamePos.offset > aColumn->namePos.offset )
        {
            if( aColumn->next != NULL )
            {
                if( sNamePos.offset < aColumn->next->namePos.offset )
                {
                    sHaveNcharLiteral = ID_TRUE;
                }
                else
                {
                    sHaveNcharLiteral = ID_FALSE;
                }
            }
            else
            {
                sHaveNcharLiteral = ID_TRUE;
            }
        }
        else
        {
            sHaveNcharLiteral = ID_FALSE;
        }

        if( sHaveNcharLiteral == ID_TRUE )
        {
            // 해당 컬럼이 Nchar Literal을 갖고 있다면,
            // 그 컬럼에 NamePosList를 달아준다.
            // 이 정보는 execution 때, nchar literal을 unicode literal로
            // 변형시킬 때 사용된다.

            if( aColumn->ncharLiteralPos != NULL )
            {
                for( sNcharLiteral = aColumn->ncharLiteralPos;
                     sNcharLiteral->next != NULL;
                     sNcharLiteral = sNcharLiteral->next ) ;

                IDU_LIMITPOINT("qdbCommon::makeNcharLiteralStr::malloc1");
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcNamePosList),
                                                        (void**)&sTempNcharLiteral )
                         != IDE_SUCCESS);

                SET_POSITION( sTempNcharLiteral->namePos,
                              sTempNamePosList->namePos );
                sTempNcharLiteral->next = NULL;

                sNcharLiteral->next = sTempNcharLiteral;
            }
            else
            {
                IDU_LIMITPOINT("qdbCommon::makeNcharLiteralStr::malloc2");
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                                        ID_SIZEOF(qcNamePosList),
                                        (void**)&aColumn->ncharLiteralPos )
                         != IDE_SUCCESS);

                SET_POSITION( aColumn->ncharLiteralPos->namePos,
                              sTempNamePosList->namePos );
                aColumn->ncharLiteralPos->next = NULL;
            }
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeNcharLiteralStr(
    qcStatement          * aStatement,
    qcNamePosList        * aNcharList,
    qdPartitionAttribute * aPartAttr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1579 NCHAR
 *
 *      CREATE TABLE(파티션드 테이블)의 validation때 호출되는 함수이다.
 *      인자로 넘어온 컬럼에 nchar literal이 있을 때,
 *      해당 컬럼에 nchar literal의 namePos의 list를 달아준다.
 *
 *      달아준 namePos의 list는 execution 때, nchar literal=>unicode literal로
 *      변형할 때 사용된다.
 *      ex) n'안녕' => u'\C548\B155'
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool              sHaveNcharLiteral = ID_FALSE;
    qcNamePosList     * sTempNamePosList;
    qcNamePosList     * sNcharLiteral;
    qcNamePosList     * sTempNcharLiteral;
    qcNamePosition      sNamePos;

    for( sTempNamePosList = aNcharList;
         sTempNamePosList != NULL;
         sTempNamePosList = sTempNamePosList->next )
    {
        sNamePos = sTempNamePosList->namePos;

        if( sNamePos.offset > aPartAttr->tablePartName.offset )
        {
            if( aPartAttr->next != NULL )
            {
                if( sNamePos.offset < aPartAttr->next->tablePartName.offset )
                {
                    sHaveNcharLiteral = ID_TRUE;
                }
                else
                {
                    sHaveNcharLiteral = ID_FALSE;
                }
            }
            else
            {
                sHaveNcharLiteral = ID_TRUE;
            }
        }
        else
        {
            sHaveNcharLiteral = ID_FALSE;
        }

        if( sHaveNcharLiteral == ID_TRUE )
        {
            // 해당 컬럼이 Nchar Literal을 갖고 있다면,
            // 그 컬럼에 NamePosList를 달아준다.
            // 이 정보는 execution 때, nchar literal을 unicode literal로
            // 변형시킬 때 사용된다.

            if( aPartAttr->ncharLiteralPos != NULL )
            {
                for( sNcharLiteral = aPartAttr->ncharLiteralPos;
                     sNcharLiteral->next != NULL;
                     sNcharLiteral = sNcharLiteral->next ) ;

                IDU_LIMITPOINT("qdbCommon::makeNcharLiteralStr::malloc3");
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcNamePosList),
                                                        (void**)&sTempNcharLiteral )
                         != IDE_SUCCESS);

                SET_POSITION( sTempNcharLiteral->namePos,
                              sTempNamePosList->namePos );
                sTempNcharLiteral->next = NULL;

                sNcharLiteral->next = sTempNcharLiteral;
            }
            else
            {
                IDU_LIMITPOINT("qdbCommon::makeNcharLiteralStr::malloc4");
                IDE_TEST(QC_QMP_MEM(aStatement)->alloc(
                                        ID_SIZEOF(qcNamePosList),
                                        (void**)&aPartAttr->ncharLiteralPos )
                         != IDE_SUCCESS);

                SET_POSITION( aPartAttr->ncharLiteralPos->namePos,
                              sTempNamePosList->namePos );
                aPartAttr->ncharLiteralPos->next = NULL;
            }
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::storingSize( mtcColumn  * aStoringColumn,
                               mtcColumn  * aValueColumn,
                               void       * aValue,
                               UInt       * aOutStoringSize )
{
/***********************************************************************
 * PROJ-1705
 * 디스크테이블의 레코드 저장구조 변경으로 인해
 * 레코드 입력시
 * 메모리/디스크/디스크템프테이블의 종류에 따라
 * 가변길이데이터타입에 대한 length와 value정보가 다름.
 *
 * 예) varchar(10) = 'aaa' 를 insert하는 경우
 *
 *     .메모리테이블 :
 *        smiValue.length = 5
 *        smiValue.value  = { 3, 'aaa' } 의 mtdDataType 형식으로 sm에 전달
 *
 *        ------------------------
 *        | {3,   'aaa'} | ...     <==insert를 위해 qp에서 구성한 mtdDataType의 value
 *        ------------------------
 *        /\
 *        | (storingValue = smiValue.value)
 *
 *
 *     .디스크테이블 :
 *        smiValue.length = 3
 *        smiValue.value  = 'aaa' 로 실제 value만 sm에 전달
 *
 *        아래 그림은 insert를 위해 qp에서 구성한 mtdDataType의 value
 *
 *        -------------------------
 *        | {3,   'aaa'} | ...     <==insert를 위해 qp에서 구성한 mtdDataType의 value
 *        -------------------------
 *               /\
 *               |  (storingValue = smiValue.value)
 *
 * 각각의 경우에 맞는 value pointer를 반환하고,
 *
 * 아래 두개의 함수는 sm으로 레코드를 전달하는 경우에만 사용됨.
 **********************************************************************/

    UInt   sNonStoringSize;

    *aOutStoringSize = 0;

    IDE_DASSERT( aStoringColumn->type.dataTypeId == aValueColumn->type.dataTypeId );

    if( ( aStoringColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        //-----------------------
        // MEMORY COLUMN
        //-----------------------

        if( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
            == SMI_COLUMN_TYPE_FIXED )
        {
            //-----------------------
            // FIXED
            //-----------------------
            *aOutStoringSize =
                aValueColumn->module->actualSize( aValueColumn,
                                                  aValue );
        }
        else if ( ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE ) ||
                  ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            //-----------------------
            // VARIABLE
            //-----------------------
            if( aValueColumn->module->isNull( NULL,
                                              aValue ) == ID_TRUE )
            {
                *aOutStoringSize = 0;
            }
            else
            {
                *aOutStoringSize =
                    aValueColumn->module->actualSize( aValueColumn,
                                                      aValue );
            }
        }
        else
        {
            //-----------------------
            // LOB
            //-----------------------
            if( aValueColumn->module->isNull( NULL,
                                              aValue ) == ID_TRUE )
            {
                *aOutStoringSize = 0;
            }
            else
            {
                *aOutStoringSize =
                    aValueColumn->module->actualSize( aValueColumn,
                                                      aValue )
                    - aValueColumn->module->headerSize();
            }
        }
    }
    else
    {
        //-----------------------
        // DISK COLUMN
        //-----------------------

        IDE_DASSERT( ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                       == SMI_COLUMN_TYPE_FIXED ) ||
                     ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                       == SMI_COLUMN_TYPE_LOB ) );

        if( aValueColumn->module->isNull( aValueColumn,
                                          aValue ) == ID_TRUE )
        {
            *aOutStoringSize = 0;
        }
        else
        {
            IDE_TEST( mtc::getNonStoringSize( &aStoringColumn->column, &sNonStoringSize )
                      != IDE_SUCCESS );

            *aOutStoringSize =
                aValueColumn->module->actualSize( aValueColumn,
                                                  aValue )
                - sNonStoringSize;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::mtdValue2StoringValue( mtcColumn  * aStoringColumn,
                                         mtcColumn  * aValueColumn,
                                         void       * aValue,
                                         void      ** aOutStoringValue )
{
/***********************************************************************
 * PROJ-1705
 * 디스크테이블의 레코드 저장구조 변경으로 인해
 * 레코드 입력시
 * 메모리/디스크/디스크템프테이블의 종류에 따라
 * 가변길이데이터타입에 대한 length와 value정보가 다름.
 *
 * 예) varchar(10) = 'aaa' 를 insert하는 경우
 *
 *     .메모리테이블 :
 *        smiValue.length = 5
 *        smiValue.value  = { 3, 'aaa' } 의 mtdDataType 형식으로 sm에 전달
 *
 *        ------------------------
 *        | {3,   'aaa'} | ...     <==insert를 위해 qp에서 구성한 mtdDataType의 value
 *        ------------------------
 *        /\
 *        | (storingValue = smiValue.value)
 *
 *
 *     .디스크테이블 :
 *        smiValue.length = 3
 *        smiValue.value  = 'aaa' 로 실제 value만 sm에 전달
 *
 *        아래 그림은 insert를 위해 qp에서 구성한 mtdDataType의 value
 *
 *        -------------------------
 *        | {3,   'aaa'} | ...     <==insert를 위해 qp에서 구성한 mtdDataType의 value
 *        -------------------------
 *               /\
 *               |  (storingValue = smiValue.value)
 *
 * 각각의 경우에 맞는 value pointer를 반환하고,
 *
 * 아래 두개의 함수는 sm으로 레코드를 전달하는 경우에만 사용됨.
 **********************************************************************/

    UInt sNonStoringSize = 0;

    IDE_DASSERT( aStoringColumn->type.dataTypeId == aValueColumn->type.dataTypeId );

    if( ( aStoringColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        //-----------------------
        // MEMORY COLUMN
        //-----------------------

        if( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
            == SMI_COLUMN_TYPE_FIXED )
        {
            //-----------------------
            // FIXED
            //-----------------------

            *aOutStoringValue = aValue;
        }
        else if ( ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE ) ||
                  ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            //-----------------------
            // VARIABLE
            //-----------------------
            if( aValueColumn->module->isNull( NULL,
                                              aValue ) == ID_TRUE )
            {
                *aOutStoringValue = NULL;
            }
            else
            {
                *aOutStoringValue = aValue;
            }
        }
        else
        {
            //-----------------------
            // LOB
            //-----------------------

            if( aValueColumn->module->isNull( NULL,
                                              aValue ) == ID_TRUE )
            {
                *aOutStoringValue = NULL;
            }
            else
            {
                *aOutStoringValue =
                    (UChar*)aValue + aValueColumn->module->headerSize();
            }
        }
    }
    else
    {
        //-----------------------
        // DISK COLUMN
        //-----------------------

        IDE_DASSERT( ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                       == SMI_COLUMN_TYPE_FIXED ) ||
                     ( ( aStoringColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                       == SMI_COLUMN_TYPE_LOB ) );

        if( aValueColumn->module->isNull( NULL,
                                          aValue ) == ID_TRUE )
        {
            *aOutStoringValue = NULL;
        }
        else
        {
            IDE_TEST( mtc::getNonStoringSize(&aStoringColumn->column, &sNonStoringSize)
                      != IDE_SUCCESS );
            *aOutStoringValue = (UChar*)aValue + sNonStoringSize;

        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::storingValue2MtdValue( mtcColumn  * aColumn,
                                         void       * aValue,
                                         void      ** aOutMtdValue )
{
/***********************************************************************
 * PROJ-1705
 * storeValue로부터 mtdDataType의 value로 반환
 *
 * 예) varchar(10) = 'aaa' 인 경우,
 *
 *     .메모리테이블 :
 *        smiValue.length = 5
 *        smiValue.value  = { 3, 'aaa' } 의 mtdDataType 형식으로 sm에 전달
 *
 *        -----------------------
 *        | {3, 'aaa} | ...       <==insert를 위해 qp에서 구성한 mtdDataType의 value
 *        -----------------------
 *       /\
 *       | (storingValue = smiValue.value)
 *
 *
 *     .디스크테이블 :
 *        smiValue.length = 3
 *        smiValue.value  = 'aaa' 로 실제 value만 sm에 전달
 *
 *        -----------------------
 *        | {3, 'aaa} | ...       <==insert를 위해 qp에서 구성한 mtdDataType의 value
 *        -----------------------
 *             /\
 *              |  (storingValue = smiValue.value)
 *       /\
 *        | (mtdDataType value)
 **********************************************************************/

    UInt   sNonStoringSize = 0;

    if( ( aColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
        == SMI_COLUMN_STORAGE_MEMORY )
    {
        //-----------------------
        // MEMORY COLUMN
        //-----------------------

        // BUG-36718
        // Null value 도 고려해야 함
        if( ( aColumn->column.flag & SMI_COLUMN_TYPE_MASK )
            == SMI_COLUMN_TYPE_FIXED )
        {
            //-----------------------
            // FIXED
            //-----------------------

            *aOutMtdValue = aValue;
        }
        else if ( ( ( aColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE ) ||
                  ( ( aColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            //-----------------------
            // VARIABLE
            //-----------------------
            if( aValue == NULL )
            {
                *aOutMtdValue = aColumn->module->staticNull;
            }
            else
            {
                *aOutMtdValue = aValue;
            }
        }
        else
        {
            //-----------------------
            // LOB
            //-----------------------

            if( aValue == NULL )
            {
                *aOutMtdValue = aColumn->module->staticNull;
            }
            else
            {
                *aOutMtdValue =
                    (UChar*)aValue - aColumn->module->headerSize();
            }
        }
    }
    else
    {
        //-----------------------
        // DISK COLUMN
        //-----------------------
        
        IDE_DASSERT( ( ( aColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                       == SMI_COLUMN_TYPE_FIXED ) ||
                     ( ( aColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                       == SMI_COLUMN_TYPE_LOB ) );
        
        if( aValue == NULL )
        {
            *aOutMtdValue = aColumn->module->staticNull;
        }
        else
        {
            IDE_TEST( mtc::getNonStoringSize( &aColumn->column, &sNonStoringSize )
                      != IDE_SUCCESS );
            *aOutMtdValue = (UChar*)aValue - sNonStoringSize;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qdbCommon::findColumnIDInColumnList( qcmColumn * aColumns,
                                            UInt        aColumnID )
{
/***********************************************************************
 *
 * Description :
 *      aColumns에서 aColumnID가 존재하는지 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn * sColumn;
    idBool      sFound = ID_FALSE;

    sColumn = aColumns;

    while ( sColumn != NULL )
    {
        if ( sColumn->basicInfo->column.id == aColumnID )
        {
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        sColumn = sColumn->next;
    }

    return sFound;
}

qcmColumn * qdbCommon::findColumnInColumnList( qcmColumn * aColumns,
                                               UInt        aColumnID )
{
/***********************************************************************
 *
 * Description :
 *      aColumns에서 aColumnID에 해당하는 qcmColumn을 찾아서 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn * sColumn;

    for ( sColumn = aColumns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( sColumn->basicInfo->column.id == aColumnID )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sColumn;
}

UInt qdbCommon::getNewColumnIDForAlter( qcmColumn * aDelColList,
                                        UInt        aColumnID )
{
/***********************************************************************
 *
 * Description :
 *     삭제할 column ID를 고려하여 삭제되지 않은 column ID에
 *     대하여 새로운 column ID를 계산한다.
 *
 * Implementation :
 *     aDelColList가 {2,4}이고 constraint column이 {1,3,5}라면
 *     새로운 constraint column은 {1,2,3}이 되어야 한다.
 *     aColumnID (1)보다 작은 aDelCol는 0개이므로 (1-0)->(1)
 *     aColumnID (3)보다 작은 aDelCol는 1개이므로 (3-1)->(2)
 *     aColumnID (5)보다 작은 aDelCol은 2개이므로 (5-2)->(3)
 *
 ***********************************************************************/

    qcmColumn * sColumn;
    UInt        sCount = 0;

    sColumn = aDelColList;

    while ( sColumn != NULL )
    {
        // aColumnID는 aDelColList에 있어서는 안된다.
        IDE_DASSERT( sColumn->basicInfo->column.id != aColumnID );

        if ( sColumn->basicInfo->column.id < aColumnID )
        {
            sCount++;
        }
        else
        {
            // Nothing to do.
        }

        sColumn = sColumn->next;
    }

    return ( aColumnID - sCount );
}

IDE_RC qdbCommon::validateColumnLength( qcStatement * aStatement,
                                        qcmColumn   * aColumn )
{
    qcuSqlSourceInfo   sqlInfo;
    const mtdModule  * sModule;

    sModule = aColumn->basicInfo->module;

    // BUG-28921
    // precision을 명시하는 type들의 max store precision은 모듈에 정의된다.
    // (nchar/nvarchar type은 nls값에 의해 수정된다.)
    if ( (sModule->flag & MTD_CREATE_PARAM_MASK) == MTD_CREATE_PARAM_PRECISION )
    {
        if( aColumn->basicInfo->precision > (SInt)sModule->maxStorePrecision )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aColumn->namePos );
            IDE_RAISE( ERR_INVALID_LENGTH );
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

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_LENGTH,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  PROJ-2464 hybrid partitioned table 지원
 *      저장매체(Memory/Disk)를 고려하여 smiValue(Length, StoringValue)를 변환한다.
 *      저장구조에 대한 설명은 mtdValue2StoringValue()를 참고한다.
 *
 * Implementation :
 **********************************************************************/
IDE_RC qdbCommon::adjustSmiValueToDisk( mtcColumn * aFromColumn,
                                        smiValue  * aFromValue,
                                        mtcColumn * aToColumn,
                                        smiValue  * aToValue )
{
    UInt sNonStoringSize = 0;

    IDE_DASSERT( aFromColumn->type.dataTypeId == aToColumn->type.dataTypeId );

    // Memory Variable Null, Memory LOB Null -> Disk Null
    if ( aFromValue->value == NULL )
    {
        aToValue->length = 0;
        aToValue->value  = NULL;
    }
    else
    {
        if ( ( aFromColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                                       == SMI_COLUMN_TYPE_LOB )
        {
            aToValue->length = aFromValue->length;
            aToValue->value  = aFromValue->value;
        }
        else
        {
            // Memory Fixed Null -> Disk Null
            if ( ( ( aFromColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                                             == SMI_COLUMN_TYPE_FIXED ) &&
                 ( aFromColumn->module->isNull( NULL, aFromValue->value ) == ID_TRUE ) )
            {
                aToValue->length = 0;
                aToValue->value  = NULL;
            }
            else
            {
                IDE_TEST( mtc::getNonStoringSize( &(aToColumn->column), &sNonStoringSize )
                          != IDE_SUCCESS );

                aToValue->length = aFromValue->length - sNonStoringSize;
                aToValue->value  = (UChar *)aFromValue->value + sNonStoringSize;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::adjustSmiValueToMemory( mtcColumn * aFromColumn,
                                          smiValue  * aFromValue,
                                          mtcColumn * aToColumn,
                                          smiValue  * aToValue )
{
    UInt sNonStoringSize = 0;

    IDE_DASSERT( aFromColumn->type.dataTypeId == aToColumn->type.dataTypeId );

    if ( aFromValue->value == NULL )
    {
        // Disk Null -> Memory Fixed Null
        if ( ( aToColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                                     == SMI_COLUMN_TYPE_FIXED )
        {
            aToValue->length = aToColumn->module->nullValueSize();
            aToValue->value  = aToColumn->module->staticNull;
        }
        // Disk Null -> Memory Variable Null, Memory LOB Null
        else
        {
            aToValue->length = 0;
            aToValue->value  = NULL;
        }
    }
    else
    {
        if ( ( aFromColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                                       == SMI_COLUMN_TYPE_LOB )
        {
            aToValue->length = aFromValue->length;
            aToValue->value  = aFromValue->value;
        }
        else
        {
            IDE_TEST( mtc::getNonStoringSize( &(aFromColumn->column), &sNonStoringSize )
                      != IDE_SUCCESS );

            aToValue->length = aFromValue->length + sNonStoringSize;
            aToValue->value  = (UChar *)aFromValue->value - sNonStoringSize;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt qdbCommon::getTableTypeFromTBSID( scSpaceID     aTBSID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      주어진 TBS ID에 해당하는 Table TBS Type를 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt sType;

    if ( smiTableSpace::isDiskTableSpace( aTBSID ) == ID_TRUE )
    {
        sType = SMI_TABLE_DISK;
    }
    else
    {
        if ( smiTableSpace::isMemTableSpace( aTBSID ) == ID_TRUE )
        {
            sType = SMI_TABLE_MEMORY;
        }
        else
        {
            IDE_DASSERT( smiTableSpace::isVolatileTableSpace( aTBSID ) == ID_TRUE );

            sType = SMI_TABLE_VOLATILE;
        }
    }

    return sType;
}

UInt qdbCommon::getTableTypeFromTBSType( smiTableSpaceType aTBSType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      주어진 TBS Type에 해당하는 Table TBS Type를 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt sTableType = SMI_TABLE_DISK;

    if ( smiTableSpace::isDiskTableSpaceType( aTBSType ) == ID_TRUE )
    {
        sTableType = SMI_TABLE_DISK;
    }
    else
    {
        if ( smiTableSpace::isMemTableSpaceType( aTBSType ) == ID_TRUE )
        {
            sTableType = SMI_TABLE_MEMORY;
        }
        else
        {
            IDE_DASSERT( smiTableSpace::isVolatileTableSpaceType( aTBSType ) == ID_TRUE );

            sTableType = SMI_TABLE_VOLATILE;
        }
    }

    return sTableType;
}

void qdbCommon::getTableTypeCountInPartInfoList( UInt                 * aTableType,
                                                 qcmPartitionInfoList * aPartInfoList,
                                                 SInt                 * aCountDiskType,
                                                 SInt                 * aCountMemType,
                                                 SInt                 * aCountVolType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      주어진 Partitioned TableTBSType과 Partition List으로
 *      모든 Partition을 TBS Type별로 개수를 계산하여 반환한다.
 *
 *       1. Partitioned TableTBSType를 계산한다.
 *       2. Partition List를 순회하면서 PartTBSType를 계산한다.
 *       3. 반환 공간이 있는 Type만 반환한다.
 *
 *      IN  : aTableType     - Nullable
 *            aPartInfoList  - Nullable
 *
 *      OUT : aCountDiskType - Nullable
 *            aCountMemType  - Nullable
 *            aCountVolType  - Nullable
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmPartitionInfoList * sPartInfoList;
    SInt                   sCountDiskType = 0;
    SInt                   sCountMemType  = 0;
    SInt                   sCountVolType  = 0;
    UInt                   sPartType;

    IDE_DASSERT( ( aTableType != NULL ) || ( aPartInfoList != NULL ) );
    
    /* 1. Partitioned TableTBSType를 계산한다. */
    if ( aTableType != NULL )
    {
        if ( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            sCountDiskType++;
        }
        else
        {
            if ( ( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_MEMORY ) ||
                 ( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_META ) )
            {
                sCountMemType++;
            }
            else
            {
                IDE_DASSERT( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_VOLATILE );

                sCountVolType++;
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. Partition List를 순회하면서 PartTBSType를 계산한다. */
    for ( sPartInfoList  = aPartInfoList;
          sPartInfoList != NULL;
          sPartInfoList  = sPartInfoList->next )
    {
        sPartType = sPartInfoList->partitionInfo->tableFlag;

        if ( ( sPartType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            sCountDiskType++;
        }
        else
        {
            if ( ( ( sPartType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_MEMORY ) ||
                 ( ( sPartType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_META ) )
            {
                sCountMemType++;
            }
            else
            {
                IDE_DASSERT( ( sPartType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_VOLATILE );

                sCountVolType++;
            }
        }
    }

    /*  3. 반환 공간이 있는 Type만 반환한다. */
    if ( aCountDiskType != NULL )
    {
        *aCountDiskType = sCountDiskType;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aCountMemType != NULL )
    {
        *aCountMemType = sCountMemType;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aCountVolType != NULL )
    {
        *aCountVolType = sCountVolType;
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

void qdbCommon::getTableTypeCountInPartAttrList( UInt                 * aTableType,
                                                 qdPartitionAttribute * aPartAttrList,
                                                 SInt                 * aCountDiskType,
                                                 SInt                 * aCountMemType,
                                                 SInt                 * aCountVolType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      주어진 Partitioned TableTBSType과 Partition Attrbute List으로
 *      모든 Partition을 TBS Type별로 개수를 계산하여 반환한다.
 *
 *       1. Partitioned TableTBSType를 계산한다.
 *       2. Partition Attrbute List를 순회하면서 PartTBSType를 계산한다.
 *       3. 반환 공간이 있는 Type만 반환한다.
 *
 *      IN  : aTableType     - Nullable
 *            aPartAttrList  - Nullable
 *
 *      OUT : aCountDiskType - Nullable
 *            aCountMemType  - Nullable
 *            aCountVolType  - Nullable
 *
 * Implementation :
 *
 ***********************************************************************/

    qdPartitionAttribute * sPartAttrList;
    smiTableSpaceType      sPartTBSType;
    SInt                   sCountDiskType = 0;
    SInt                   sCountMemType  = 0;
    SInt                   sCountVolType  = 0;

    IDE_DASSERT( ( aTableType != NULL ) || ( aPartAttrList != NULL ) );

    /* 1. Partitioned TableTBSType를 계산한다. */
    if ( aTableType != NULL )
    {
        if ( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            sCountDiskType++;
        }
        else
        {
            if ( ( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_MEMORY ) ||
                 ( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_META ) )
            {
                sCountMemType++;
            }
            else
            {
                IDE_DASSERT( ( *aTableType & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_VOLATILE );

                sCountVolType++;
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /*  2. Partition Attrbute List를 순회하면서 PartTBSType를 계산한다. */
    for ( sPartAttrList  = aPartAttrList;
          sPartAttrList != NULL;
          sPartAttrList  = sPartAttrList->next )
    {
        sPartTBSType = sPartAttrList->TBSAttr.mType;

        if ( smiTableSpace::isDiskTableSpaceType( sPartTBSType ) == ID_TRUE )
        {
            sCountDiskType++;
        }
        else
        {
            if ( smiTableSpace::isMemTableSpaceType( sPartTBSType ) == ID_TRUE )
            {
                sCountMemType++;
            }
            else
            {
                IDE_DASSERT( smiTableSpace::isVolatileTableSpaceType( sPartTBSType ) == ID_TRUE );

                sCountVolType++;
            }
        }
    }
    
    /*  3. 반환 공간이 있는 Type만 반환한다. */
    if ( aCountDiskType != NULL )
    {
        *aCountDiskType = sCountDiskType;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aCountMemType != NULL )
    {
        *aCountMemType = sCountMemType;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aCountVolType != NULL )
    {
        *aCountVolType = sCountVolType;
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

IDE_RC qdbCommon::validateConstraintRestriction( qcStatement        * aStatement,
                                                 qdTableParseTree   * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Create Table, Add Column, Add Constraint를 위한 추가 Vaildate를 수행한다.
 *      qdn::validateConstraint의 검사내용 중 일부를 옮겨왔다.
 *      Partitioned 또는 Table를 검사하는 공통함수이다.
 *
 *      관련내용 : BUG-17848 : LOGGING option은 disk table에만 적용 가능. - BUG-31517, BUG-32961
 *                 PROJ-2334 : PMT PMT경우 파티션드 테이블에 논파티션드 인덱스 생성할 수 없음
 *
 *      IN  : aStatement  - Notnull
 *            aParseTree  - Notnull
 *
 * Implementation :
 *      1. 매체 Type를 검사한다.
 *
 *      2. 옵션을 검사한다.
 *         2.1. Memory Type 이 있을 시, INDEX LOGGING 옵션을 지원하지 않는다.
 *         2.2. Memory Type 외 다른 Type 있을 시, INDEX PERSISTENT 옵션을 지원하지 않는다.
 *         2.3. Disk Type   이 없을 시, Constraint에 SegAttr 옵션을 지원하지 않는다.
 *         2.4. Constraint에서 설정한 Index를 검사한다.
 *         2.5. Constraint에서 설정한 Foreign Key를 검사한다.
 *         2.6. DirectKeyMaxSize를 검사한다.
 *
 ***********************************************************************/

    qcmPartitionInfoList * sPartInfoList  = NULL;
    qdPartitionAttribute * sPartAttrList  = NULL;
    qdConstraintSpec     * sConstraint;
    idBool                 sIsPartitioned = ID_FALSE;
    idBool                 sIsUserTable   = ID_FALSE;
    idBool                 sIsDiskTBS     = ID_FALSE;
    idBool                 sIsLocalIndex  = ID_TRUE;
    SInt                   sCountDiskType = 0;
    SInt                   sCountMemType  = 0;
    SInt                   sCountVolType  = 0;
    UInt                   sTableType;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    /* 1. 매체 Type를 검사한다. */
    if ( aParseTree->tableInfo != NULL )
    {
        sTableType   = aParseTree->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK;
        sIsUserTable = smiTableSpace::isDataTableSpaceType( aParseTree->tableInfo->TBSType );

        if ( aParseTree->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            sPartInfoList  = aParseTree->partTable->partInfoList;
            sIsPartitioned = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        getTableTypeCountInPartInfoList( & sTableType,
                                         sPartInfoList,
                                         & sCountDiskType,
                                         & sCountMemType,
                                         & sCountVolType );

    }
    else
    {
        sTableType    = getTableTypeFromTBSID( aParseTree->TBSAttr.mID );
        sIsUserTable = smiTableSpace::isDataTableSpaceType( aParseTree->TBSAttr.mType );

        if ( aParseTree->partTable != NULL )
        {
            sPartAttrList  = aParseTree->partTable->partAttr;
            sIsPartitioned = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        getTableTypeCountInPartAttrList( & sTableType,
                                         sPartAttrList,
                                         & sCountDiskType,
                                         & sCountMemType,
                                         & sCountVolType );

    }

    /* 2. 옵션을 검사한다. */
    for ( sConstraint = aParseTree->constraints;
          sConstraint != NULL;
          sConstraint = sConstraint->next )
    {
        /* 2.1. Memory Type이 있을 시, INDEX LOGGING 옵션을 지원하지 않는다. */
        IDE_TEST_RAISE( ( sConstraint->buildFlag != SMI_INDEX_BUILD_DEFAULT ) &&
                        ( ( sCountMemType + sCountVolType ) > 0 ),
                        ERR_IRREGULAR_LOGGING_OPTION );

        /* 2.2. Memory Type이 외 다른 Type 있을 시, INDEX PERSISTENT 옵션을 지원하지 않는다. */
        IDE_TEST( validateAndSetPersistent( sCountDiskType,
                                            sCountVolType,
                                            &( sConstraint->isPers ),
                                            NULL )
                  != IDE_SUCCESS );

        if ( ( sConstraint->constrType == QD_PRIMARYKEY ) ||
             ( sConstraint->constrType == QD_UNIQUE ) ||
             ( sConstraint->constrType == QD_LOCAL_UNIQUE ) )
        {
            /* 2.3. Memory Type이 있을 시, Global Index를 지원하지 않는다. */
            if ( ( sIsPartitioned == ID_TRUE ) &&
                 ( sConstraint->constrType != QD_LOCAL_UNIQUE ) )
            {
                if ( ( sCountMemType + sCountVolType ) == 0 )
                {
                    sIsDiskTBS = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( qdn::checkLocalIndex( aStatement,
                                                sConstraint,
                                                aParseTree->tableInfo,
                                                & sIsLocalIndex,
                                                sIsDiskTBS )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            /* 2.4. Constraint에서 설정한 SegAttr를 검사한다.*/
            IDE_TEST( validateAndSetSegAttr( sTableType,
                                             NULL,
                                             & ( sConstraint->segAttr ),
                                             ID_FALSE )
                      != IDE_SUCCESS );

            /* 2.5. Constraint에서 설정한 Index를 검사한다. */
            IDE_TEST( validateIndexKeySize( aStatement,
                                            sTableType,
                                            sConstraint->constraintColumns,
                                            sConstraint->constrColumnCount,
                                            smiGetDefaultIndexType(),
                                            sPartInfoList,
                                            sPartAttrList,
                                            sIsPartitioned,
                                            ID_FALSE )
                      != IDE_SUCCESS );
        }
        else if ( sConstraint->constrType == QD_FOREIGN )
        {
            /* 2.6. Constraint에서 설정한 Foreign Key를 검사한다. */
            IDE_TEST( validateForeignKey( aStatement,
                                          sConstraint->referentialConstraintSpec,
                                          sCountDiskType,
                                          sCountMemType )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        if ( sConstraint->constraintColumns != NULL )
        {
            /* 2.7. DirectKeyMaxSize를 검사한다. */
            IDE_TEST( validateAndSetDirectKey( sConstraint->constraintColumns[0].basicInfo,
                                               sIsUserTable,
                                               sCountDiskType,
                                               sCountMemType,
                                               sCountVolType,
                                               &( sConstraint->mDirectKeyMaxSize ),
                                               NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_IRREGULAR_LOGGING_OPTION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDX_NON_DISK_INDEX_LOGGING_OPTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateIndexKeySize( qcStatement          * aStatement,
                                        UInt                   aTableType,
                                        qcmColumn            * aKeyColumns,
                                        UInt                   aKeyColCount,
                                        UInt                   aIndexType,
                                        qcmPartitionInfoList * aPartInfoList,
                                        qdPartitionAttribute * aPartAttrList,
                                        idBool                 aIsPartitioned,
                                        idBool                 aIsIndex )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Index Key Size를 검사한다.
 *      Partitioned 또는 Table를 검사하는 공통함수이다.
 *      Constraint과 Index의 Validate에 이용되며, Index Option을 검사하는 함수이다.
 *
 *      관련내용 : BUG-31517 : PRIMARY KEY/UNIQUE constraint가 추가될 경우, key size limit 검사를 수행해야 함 - BUG-13528
 *
 *      IN  : aStatement    - Notnull  -\
 *            aTableType               -|
 *            aKeyColumns   - Notnull  -|---> qdx::validateKeySizeLimit 를 호출하기 위한 정보
 *            aKeyColCount             -|
 *            aIndexType               -/
 *            aPartInfoList - Nullable
 *            aPartAttrList - Nullable
 *            aIsPartitioned
 *            aIsIndex
 *
 * Implementation :
 *      1. Key Size Limit를 검사한다.
 *
 *      2. Partitioned인 경우
 *         2.1. Memory Partition이 있으면 Global Index를 지원하지 않는다.
 *         2.2. Partitioned Table과 Type이 다른 Partition이 있는지 검사한다.
 *         2.3. Key Size Limit를 검사한다.
 *
 ***********************************************************************/

    qdPartitionAttribute * sPartAttrList;
    qcmPartitionInfoList * sPartInfoList;
    qcmColumn            * sPartColumn       = NULL;
    qcmColumn            * sConstraintColumn = NULL;
    idBool                 sNeedPartCheck    = ID_FALSE;
    UInt                   sPartType         = 0;

    IDE_DASSERT( aKeyColumns != NULL );

    /* 1. 생성 가능한 Index 인지 검사한다. */
    if ( aIsIndex == ID_TRUE )
    {
        IDE_TEST_RAISE( smiCanMakeIndex( aTableType, aIndexType ) == ID_FALSE,
                        ERR_UNSUPPORTED_INDEXTYPE );
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. Key Size Limit를 검사한다.
     *    - 관련 내용 : To Fix BUG-13528
     *                  CREATE TABLE, ALTER TABLE ADD COLUMN, ALTER TABLE ADD CONSTRAINT
     *                  수행 시 PRIMARY KEY/UNIQUE constraint가 추가될 경우,
     *                  key size limit 검사를 수행해야 함
     */
    IDE_TEST( qdx::validateKeySizeLimit( aStatement,
                                         QC_QMP_MEM( aStatement ),
                                         aTableType,
                                         (void *)aKeyColumns,
                                         aKeyColCount,
                                         aIndexType )
              != IDE_SUCCESS );

    /* 3. Partitioned인 경우 */
    if ( aIsPartitioned == ID_TRUE )
    {
        /* 3.1. Partitioned Table과 Type이 다른 Partition이 있는지 검사한다.
         *      - 현재 qdx::validateKeySizeLimit 함수는 Disk, Memory & Volailte의 두가지 경우로 나누어 처리한다.
         *      - 따라서, Partitioned Table과 다른 Type의 Partition 하나만 찾아서 처리한다.
         *      - 만약에 Memory와 Volailte이 서로 다른 처리로 동작하도록 변경되면 추가적인 작업이 필요하다.
         */
        if ( aPartInfoList != NULL )
        {
            for ( sPartInfoList  = aPartInfoList;
                  sPartInfoList != NULL;
                  sPartInfoList  = sPartInfoList->next )
            {
                sPartType = sPartInfoList->partitionInfo->tableFlag & SMI_TABLE_TYPE_MASK;

                if ( QCM_TABLE_TYPE_IS_DISK( aTableType ) != QCM_TABLE_TYPE_IS_DISK ( sPartType ) )
                {
                    sNeedPartCheck = ID_TRUE;
                    sPartColumn    = sPartInfoList->partitionInfo->columns;

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
            for ( sPartAttrList  = aPartAttrList;
                  sPartAttrList != NULL;
                  sPartAttrList  = sPartAttrList->next )
            {
                sPartType = getTableTypeFromTBSID( sPartAttrList->TBSAttr.mID );

                if ( QCM_TABLE_TYPE_IS_DISK( aTableType ) != QCM_TABLE_TYPE_IS_DISK ( sPartType ) )
                {
                    sNeedPartCheck = ID_TRUE;
                    sPartColumn    = sPartAttrList->columns;

                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

        }

        /* 3.2. 생성 가능한 Index 인지 검사한다. */
        if ( aIsIndex == ID_TRUE )
        {
            IDE_TEST_RAISE( smiCanMakeIndex( sPartType, aIndexType ) == ID_FALSE,
                            ERR_UNSUPPORTED_INDEXTYPE );
        }
        else
        {
            /* Nothing to do */
        }

        /* 3.3. Key Size Limit를 검사한다. */
        if ( sNeedPartCheck == ID_TRUE )
        {
            IDE_TEST( qcm::copyQcmColumns( QC_QMP_MEM( aStatement ),
                                           aKeyColumns,
                                           & sConstraintColumn,
                                           aKeyColCount )
                      != IDE_SUCCESS );

            IDE_TEST( adjustKeyColumn( sPartColumn,
                                       sConstraintColumn )
                      != IDE_SUCCESS );

            /* TO DO : 3.1. 주석을 참조 */
            IDE_TEST( qdx::validateKeySizeLimit( aStatement,
                                                 QC_QMP_MEM( aStatement ),
                                                 sPartType,
                                                 (void *)sConstraintColumn,
                                                 aKeyColCount,
                                                 aIndexType )
                      != IDE_SUCCESS );
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

    IDE_EXCEPTION( ERR_UNSUPPORTED_INDEXTYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDX_UNSUPPORTED_INDEXTYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateForeignKey( qcStatement      * aStatement,
                                      qdReferenceSpec  * aRefSpec,
                                      SInt               aCountDiskType,
                                      SInt               aCountMemType )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Foreien Key을 검사하는 함수이다.
 *      qdn::validateConstraints가 호출된 뒤에 호출되어야 한다.
 *      위 함수 내부에서 호출되는 qdnForeignkey::validateForeignkeyspec에서 참조되는 Table를 locking 한다.
 *
 *      관련내용 : BUG-17210 : non-volatile table에서 volatile table을 참조해서는 안된다.
 *
 *
 *      Table Reference 허용 관계도
 *      -----------------------------------
 *      | No | Current | Table  | Allowed |
 *      |    | Self    | Parent |         |
 *      -----------------------------------
 *      | 1  | Non     | Vol    | X       |
 *      | 2  | Non     | Non    | O       |
 *      | 3  | Vol     | Vol    | O       |
 *      | 4  | Vol     | Non    | O       |
 *      -----------------------------------
 *
 *      Volatile Type 결정
 *      ----------------------------------------
 *      | No | Position | Part1 | Part2 | Type |
 *      ----------------------------------------
 *      | 5  | Self     | Non   | Non   | Non  |
 *      | 6  | Self     | Non   | Vol   | Non  |
 *      | 7  | Self     | Vol   | Non   | Non  |
 *      | 8  | Self     | Vol   | Vol   | Vol  |
 *      ----------------------------------------
 *      | 9  | Parent   | Non   | Non   | Non  |
 *      | 10 | Parent   | Non   | Vol   | Vol  |
 *      | 11 | Parent   | Vol   | Non   | Vol  |
 *      | 12 | Parent   | Vol   | Vol   | Vol  |
 *      ----------------------------------------
 *
 * Implementation :
 *      1. Self Table Type을 계산한다.
 *         1.1 모두 Volatile Part이면 Volatile Table이다.
 *
 *      2. Parent Table Type을 계산한다.
 *         2.1. Create 시 Self Condition 경우도 적절히 계산한다.
 *         2.2. Partitioned 라면 Partition 정보를 가져온다.
 *         2.3. Volatile Part가 하나라도 존재하면 Volatile Table 이다.
 *
 *      3. Foreign Key 제약을 확인한다.
 *
 ***********************************************************************/

    qcmTableInfo         * sReferencedTableInfo = NULL;
    qcmPartitionInfoList * sPartInfoList        = NULL;
    idBool                 sIsVolatileSelf      = ID_FALSE;
    idBool                 sIsVolatileParent    = ID_FALSE;
    SInt                   sCountVolType        = 0;
    UInt                   sTableType;

    IDE_DASSERT( aRefSpec != NULL );

    /* 1. Self Table Type을 계산한다. */
    if ( ( aCountDiskType + aCountMemType ) == 0 )
    {
        /* 1.1 모두 Volatile Part이면 Volatile Table이다. */
        sIsVolatileSelf = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. Parent Table Type을 계산한다. */
    if ( aRefSpec->referencedTableID == ID_UINT_MAX )
    {
        /* 2.1. Create 시 Self Condition 경우도 적절히 계산한다.
         *      qdn::validateConstraints -> qdnForeignKey::validateForeignKeySpec 에서 Self Condition를 처리한다.
         *      Create 시 Self Condition 경우에 aRefSpec->referencedTableID 값을 UNIT_MAX로 설정한다.
         *      반면에 이미 존재하는 Table인 경우에 aRefSpec->referencedTableInfo의 정보를 지닌다.
         *
         *      따라서, 이 값으로 Self Condition 인지 구별하였다.
         *      그리고 대상 Partitioned Table과 동일하므로, 1번에서 계산한 Table Type 값을 이용한다.
         */
        sIsVolatileParent = sIsVolatileSelf;
    }
    else
    {
        sReferencedTableInfo = aRefSpec->referencedTableInfo;
        sTableType = sReferencedTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;

        if ( sReferencedTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            /* 2.2. Partitioned 라면 Partition 정보를 가져온다. */
            IDE_TEST( checkAndSetAllPartitionInfo(
                          aStatement,
                          sReferencedTableInfo->tableID,
                          & ( sPartInfoList ) )
                      != IDE_SUCCESS );

            /* 2.3. Volatile Part가 하나라도 존재하면 Volatile Table 이다.
             *      - 관련내용 : BUG-17210 non-volatile table에서 volatile table을 참조해서는 안된다.
             *                   volatile table이면 volatile table을 참조해도 상관없다.
             */
            getTableTypeCountInPartInfoList( & sTableType,
                                             sPartInfoList,
                                             NULL,
                                             NULL,
                                             & sCountVolType );

            if ( sCountVolType > 0 )
            {
                sIsVolatileParent = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            sIsVolatileParent = smiTableSpace::isVolatileTableSpaceType( sReferencedTableInfo->TBSType );
        }
    }

    /* 3. Foreign Key 제약을 확인한다. Foreign Key 제약 : Non-Volatile -> Volatile */
    IDE_TEST_RAISE( ( sIsVolatileSelf == ID_FALSE ) && ( sIsVolatileParent == ID_TRUE ),
                    ERR_CONSTRAINT_NOT_EXIST );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONSTRAINT_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_REFERENCED_CONSTRAINT_NOT_FOUND ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateAndSetDirectKey( mtcColumn * aBasicInfo,
                                           idBool      aIsUserTable,
                                           SInt        aCountDiskType,
                                           SInt        aCountMemType,
                                           SInt        aCountVolType,
                                           ULong     * aDirectKeyMaxSize,
                                           UInt      * aSetFlag )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Direct Key Max Size를 검사하는 함수이다.
 *      관련내용 : PROJ-2433 : Direct Key Index를 설정한 경우 다음을 확인한다.
 *
 * Implementation :
 *      1. Memory Table이 아니면 허용하지 않는다.
 *      2. Partitioned Table에는 허용하지 않는다.
 *      3. DIRECTKEY MAXSIZE를 입력받았을경우 허용 범위인지 검사한다. ( 8 ~ node_size/3 )
 *      4. 허용 Data Type인지 검사한다.
 *      5. Compression Column은 지원하지 않는다.
 *      6. Property __FORCE_INDEX_DIRECTKEY를 사용하는 지 검사한다.
 *         6.1. Direct Key 설정을 한다.
 *      7. 반환할 Flag가 있다면 Flag도 설정한다.
 *
 ***********************************************************************/

    if ( *aDirectKeyMaxSize != (ULong)(ID_ULONG_MAX) )
    {
        /* 1. Memory Table이 아니면 허용하지 않는다. */
        IDE_TEST_RAISE( aCountMemType == 0,
                        ERR_IRREGULAR_DIRECTKEY_OPTION );

        /* 2. Partitioned Table에는 허용하지 않는다. */
        IDE_TEST_RAISE( ( aCountDiskType + aCountMemType + aCountVolType ) != 1,
                        ERR_NO_DIRECTKEY_ON_PART_TABLE ); /* BUG-42124 */

        /* 3. 허용 범위인지 검사한다. */
        IDE_TEST_RAISE( ( *aDirectKeyMaxSize != 0 ) &&
                        ( ( *aDirectKeyMaxSize < smiGetMemIndexKeyMinSize() ) ||
                          ( *aDirectKeyMaxSize > smiGetMemIndexKeyMaxSize() ) ),
                        ERR_INVALID_DIRECTKEY_MAXSIZE );

        /* 4. 허용 Data Type인지 검사한다. */
        IDE_TEST_RAISE( mtd::isUsableDirectKeyIndex( aBasicInfo ) != ID_TRUE,
                        ERR_INVALID_DATATYPE_IN_DIRECTKEY );

        /*  5. Compression Column은 지원하지 않는다. */
        IDE_TEST_RAISE( ( aBasicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK ) == SMI_COLUMN_COMPRESSION_TRUE,
                        ERR_NO_COMPRESSION_COLUMN_IN_DIRECTKEY );
    }
    else
    {
        /* 6. Property __FORCE_INDEX_DIRECTKEY를 사용하는 지 검사한다.
         *    - 관련내용 : TEST property __FORCE_INDEX_DIRECTKEY = 1 인경우,
         *                 이후 모든 memory index는 direct key index로 강제 설정된다.
         *                 ( BUG-41712 : META 테이블은 제외한다. )
         *                 DIRECTKEY MAXSIZE는 property __MEM_BTREE_INDEX_DEFAULT_MAX_KEY_SIZE 값을 따른다.
         */
        if ( smiIsForceIndexDirectKey() == ID_TRUE )
        {
            /* 6.1. Direct Key 설정을 한다.
             *      - 1. User Data Table
             *      - 2. 일반 Table
             *      - 3. Memory Table
             *      - 4. Direct Key 허용 Index
             *      - 5. 비 압축 Column
             */
            if ( ( aIsUserTable == ID_TRUE ) &&
                 ( ( aCountDiskType + aCountMemType + aCountVolType ) == 1 ) &&
                 ( ( aCountMemType == 1 ) ) &&
                 ( mtd::isUsableDirectKeyIndex( aBasicInfo ) == ID_TRUE ) &&
                 ( ( aBasicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK ) == SMI_COLUMN_COMPRESSION_FALSE ) )
            {
                *aDirectKeyMaxSize = 0;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    }

    /* 7. 반환할 Flag가 있다면 Flag도 설정한다. */
    if ( aSetFlag != NULL )
    {
        if ( *aDirectKeyMaxSize != (ULong)(ID_ULONG_MAX) ) 
        {
            *aSetFlag &= ~(SMI_INDEX_DIRECTKEY_MASK);
            *aSetFlag |= SMI_INDEX_DIRECTKEY_TRUE;
        }
        else
        {
            *aSetFlag &= ~(SMI_INDEX_DIRECTKEY_MASK);
            *aSetFlag |= SMI_INDEX_DIRECTKEY_FALSE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_DIRECTKEY_ON_PART_TABLE )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NoDirectKeyOnPartTable ) );
    }
    IDE_EXCEPTION( ERR_IRREGULAR_DIRECTKEY_OPTION )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NonDirectKeyOption ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DIRECTKEY_MAXSIZE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDirectKeyMaxSize ) );
    }
    IDE_EXCEPTION( ERR_INVALID_DATATYPE_IN_DIRECTKEY )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_InvalidDataTypeInDirectKey ) );
    }
    IDE_EXCEPTION( ERR_NO_COMPRESSION_COLUMN_IN_DIRECTKEY )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NoCompressionInDirectKey ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateAndSetPersistent( SInt     aCountDiskType,
                                            SInt     aCountVolType,
                                            idBool * aIsPers,
                                            UInt   * aSetFlag )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Persistent Index Option를 검사하는 함수이다.
 *
 *      관련내용 : BUG-19621 : PERSISTENT option은 memory table에만 적용 가능. - BUG-31517, BUG-32961
 *                 BUG-41121 : TEST property __FORCE_INDEX_PERSISTENCE_MODE All memory index, set persistence
 *
 * Implementation :
 *      1. Memory Type 외 다른 Type 있을 시, INDEX PERSISTENT 옵션을 지원하지 않는다.
 *      2. Property __FORCE_INDEX_PERSISTENCE_MODE 사용하는 지 검사한다.
 *      3. 반환할 Flag가 있다면 Flag도 설정한다.
 *
 ***********************************************************************/

    /*  1. Memory Type 외 다른 Type 있을 시, INDEX PERSISTENT 옵션을 지원하지 않는다. */
    if  ( *aIsPers == ID_TRUE )
    {
        IDE_TEST_RAISE( ( ( aCountDiskType + aCountVolType ) > 0 ),
                        ERR_IRREGULAR_PERSISTENT_OPTION );
    }
    else
    {
        /* 2. Property __FORCE_INDEX_PERSISTENCE_MODE 사용하는 지 검사한다. */
        if ( ( smiForceIndexPersistenceMode() == SMN_INDEX_PERSISTENCE_FORCE ) &&
             ( ( aCountDiskType + aCountVolType ) == 0 ) )
        {
            *aIsPers = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* 3. 반환할 Flag가 있다면 Flag도 설정한다. */
    if ( aSetFlag != NULL )
    {
        if ( *aIsPers == ID_TRUE ) 
        {
            *aSetFlag &= ~(SMI_INDEX_PERSISTENT_MASK);
            *aSetFlag |= SMI_INDEX_PERSISTENT_ENABLE;
        }
        else
        {
            *aSetFlag &= ~(SMI_INDEX_PERSISTENT_MASK);
            *aSetFlag |= SMI_INDEX_PERSISTENT_DISABLE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_IRREGULAR_PERSISTENT_OPTION )
    {
        IDE_SET( ideSetErrorCode ( qpERR_ABORT_QDX_NON_MEMORY_INDEX_PERSISTENT_OPTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::copyAndAdjustColumnList( qcStatement        * aStatement,
                                           smiTableSpaceType    aOldTBSType,
                                           smiTableSpaceType    aNewTBSType,
                                           qcmColumn          * aOldColumn,
                                           qcmColumn         ** aNewColumn,
                                           UInt                 aColumnCount,
                                           idBool               aEnableVariableColumn )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Old Column을 복사하고, New Column을 New TBS Type에 적합하게 조정한다.
 *
 * Implementation :
 *      1. Old Column을 복사하여 New Column 생성
 *      2. Old TBS Type, New TBS Type 검사
 *      3. Row Header Size 설정
 *      4. Column Flag 설정
 *      5. Column Offset 설정
 *
 ***********************************************************************/

    UInt        sCurrentOffset;
    qcmColumn * sColumn;

    /* 1. Old Column을 복사하여 New Column 생성 */
    IDE_TEST( qcm::copyQcmColumns( QC_QMP_MEM( aStatement ),
                                   aOldColumn,
                                   aNewColumn,
                                   aColumnCount )
              != IDE_SUCCESS );

    /* 2. Old TBS Type, New TBS Type 검사 */
    if ( smiTableSpace::isDiskTableSpaceType( aOldTBSType ) != smiTableSpace::isDiskTableSpaceType( aNewTBSType ) )
    {
        /* 3. Row Header Size 설정 */
        if ( smiTableSpace::isDiskTableSpaceType( aNewTBSType ) )
        {
            sCurrentOffset = 0;
        }
        else
        {
            sCurrentOffset = smiGetRowHeaderSize( SMI_TABLE_MEMORY );
        }

        /* 4. Column Flag 설정 */
        IDE_TEST( adjustColumnFlagForTable( aStatement,
                                            aNewTBSType,
                                            *aNewColumn,
                                            aEnableVariableColumn )
                  != IDE_SUCCESS );

        /* 5. Column Offset 설정 */
        IDE_TEST( setColListOffset( QC_QMP_MEM( aStatement ),
                                    *aNewColumn,
                                    sCurrentOffset )
                  != IDE_SUCCESS );
    }
    else
    {
        /* BUG-17212 volatile table인 경우 lob 컬럼을 가질 수 없다.
         * PROJ-2174 Supporting LOB in the volatile tablespace volatile TBS에서 lob을 사용 할 수 있음.
         */
        if ( smiTableSpace::isVolatileTableSpaceType( aNewTBSType ) == ID_TRUE )
        {
            for ( sColumn = *aNewColumn; sColumn != NULL; sColumn = sColumn->next )
            {
                /* 2. Volailte 제약조건을 검사한다. */
                IDE_TEST_RAISE( sColumn->basicInfo->module->id == MTD_GEOMETRY_ID,
                                ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );

                /* Volatile TableSpace의 경우 VARIABLE_LARGE를 지원하지 않는다. */
                IDE_TEST_RAISE( ( sColumn->flag & QCM_COLUMN_TYPE_MASK ) 
                                == QCM_COLUMN_TYPE_VARIABLE_LARGE,
                                ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::adjustColumnFlagForTable( qcStatement       * aStatement,
                                            smiTableSpaceType   aTBSType,
                                            qcmColumn         * aColumn,
                                            idBool              aEnableVariableColumn )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Column의 Flag를 TBS Type에 적합하게 조정한다.
 *
 * Implementation :
 *      1. Volailte 제약조건을 검사한다.
 *      2. LOB의 Flag를 조정한다.
 *      3. Volatile / Memory Column의 Flag를 조정한다.
 *      4. Disk Column의 Flag를 조정한다.
 *      5. Column Length를 검증한다.
 *
 ***********************************************************************/

    qcmColumn * sColumn;
    UInt        sInRowLength;

    IDE_DASSERT( aColumn != NULL );

    for ( sColumn = aColumn; sColumn != NULL; sColumn = sColumn->next )
    {
        if ( smiTableSpace::isDiskTableSpaceType( aTBSType ) != ID_TRUE )
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_MEMORY;
        }
        else
        {
            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_STORAGE_MASK;
            sColumn->basicInfo->column.flag |= SMI_COLUMN_STORAGE_DISK;
        }

        /* 1. Volailte 제약조건을 검사한다.
         *    BUG-17212 volatile table인 경우 lob 컬럼을 가질 수 없다.
         *    PROJ-2174 Supporting LOB in the volatile tablespace volatile TBS에서 lob을 사용 할 수 있음.
         */
        IDE_TEST_RAISE( ( smiTableSpace::isVolatileTableSpaceType( aTBSType ) == ID_TRUE ) &&
                        ( sColumn->basicInfo->module->id == MTD_GEOMETRY_ID ),
                        ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );

        /* Volatile TableSpace의 경우 VARIABLE_LARGE를 지원하지 않는다. */
        IDE_TEST_RAISE( ( sColumn->flag & QCM_COLUMN_TYPE_MASK ) 
                        == QCM_COLUMN_TYPE_VARIABLE_LARGE,
                        ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE );

        /* 2. LOB의 Flag를 조정한다. */
        if ( ( sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK ) == MTD_COLUMN_TYPE_LOB )
        {
            sInRowLength = sColumn->basicInfo->column.vcInOutBaseSize;

            if ( smiTableSpace::isDiskTableSpaceType( aTBSType ) != ID_TRUE )
            {
                /* 상위 함수의 비교로, 다른 Type으로 조정한다.
                 * Disk -> Memory로 조정한다.
                 */
                if ( sInRowLength == QCU_DISK_LOB_COLUMN_IN_ROW_SIZE )
                {
                    sInRowLength = QCU_MEMORY_LOB_COLUMN_IN_ROW_SIZE;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* 상위 함수의 비교로, 다른 Type으로 조정한다.
                 * Memory -> Disk로 조정한다.
                 */
                if ( sInRowLength == QCU_MEMORY_LOB_COLUMN_IN_ROW_SIZE )
                {
                    sInRowLength = QCU_DISK_LOB_COLUMN_IN_ROW_SIZE;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            sColumn->basicInfo->column.vcInOutBaseSize = sInRowLength;

            /* 나머지는 기존 속성을 유지한다. */
        }
        else
        {
            /* 3. Volatile / Memory Column의 Flag를 조정한다.
             *    - 상위 함수의 비교로, 다른 Type으로 조정한다.
             *    - Disk -> Memory로 조정한다.
             */
            if ( smiTableSpace::isDiskTableSpaceType( aTBSType ) != ID_TRUE )
            {
                /* PROJ-2465 Tablespace Alteration for Table
                 *  Non-Partitioned Table이면,
                 *  'Fixed(Disk) -> Variable(Memory/Volatile)' Column 자동 변경을 지원한다.
                 */
                if ( ( aEnableVariableColumn == ID_TRUE ) &&
                     ( ( sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
                       == MTD_COLUMN_TYPE_VARIABLE ) &&
                     ( sColumn->basicInfo->column.size > QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE ) )
                {
                    /* Geometry는 항상 VARIABLE_LARGE를 사용한다. */
                    if ( sColumn->basicInfo->module->id == MTD_GEOMETRY_ID )
                    {
                        sColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                        sColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE_LARGE;
                    }
                    else
                    {
                        sColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                        sColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_VARIABLE;
                    }
                    sInRowLength = QCU_MEMORY_VARIABLE_COLUMN_IN_ROW_SIZE;
                }
                else
                {
                    sColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                    sColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;

                    sInRowLength = 0;
                }

                sColumn->basicInfo->column.flag &= ~SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK;
                sColumn->basicInfo->column.flag |= SMI_COLUMN_DATA_STORE_DIVISIBLE_FALSE;

                sColumn->basicInfo->column.vcInOutBaseSize = sInRowLength;
            }
            /* 4. Disk Column의 Flag를 조정한다.
             *    - 상위 함수의 비교로, 다른 Type으로 조정한다.
             *    - Memory -> Disk로 조정한다.
             */
            else
            {
                sColumn->basicInfo->column.flag &= ~SMI_COLUMN_TYPE_MASK;
                sColumn->basicInfo->column.flag |= SMI_COLUMN_TYPE_FIXED;

                /* PROJ-1705
                 * SM이 컬럼을 여러 row piece에 나누어 저장해도 되는지의 정보를 smiColumn.flag에 세팅한다.
                 */
                if ( ( sColumn->basicInfo->module->flag & MTD_DATA_STORE_DIVISIBLE_MASK )
                     == MTD_DATA_STORE_DIVISIBLE_TRUE )
                {
                    sColumn->basicInfo->column.flag &= ~SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK;
                    sColumn->basicInfo->column.flag |= SMI_COLUMN_DATA_STORE_DIVISIBLE_TRUE;
                }
                else
                {
                    sColumn->basicInfo->column.flag &= ~SMI_COLUMN_DATA_STORE_DIVISIBLE_MASK;
                    sColumn->basicInfo->column.flag |= SMI_COLUMN_DATA_STORE_DIVISIBLE_FALSE;
                }

                sColumn->basicInfo->column.vcInOutBaseSize = 0;
            }
        }

        /* 5. Column Length를 검증한다. */
        IDE_TEST( validateColumnLength( aStatement, sColumn ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_GEOMETRY_VIOLATION_ON_VOLATILE_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validatePhysicalAttr( qdTableParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Create Table시 PhysicalAttr인 SegAttr, SegStoAttr를 검증한다.
 *      Table, Partitioned Table에 사용할 수 있는 공용함수이다.
 *
 *      HPT 인 경우에 Memory, Disk 매체를 모두 지닐 수 있어, 제약사항이 존재한다.
 *      Disk Partitioned Table인 경우에만, SegAttr, SegStoAttr를 사용할 수 있다.
 *
 * Implementation :
 *    1. SegAttr를 검증하고 기본값을 설정한다.
 *    2. SegStoAttr를 검증하고 기본값을 설정한다.
 *
 ***********************************************************************/

    UInt sTableType;

    sTableType = getTableTypeFromTBSID( aParseTree->TBSAttr.mID );

    /* 1. SegAttr를 검증하고, 기본값을 설정한다. */
    IDE_TEST( validateAndSetSegAttr( sTableType,
                                     NULL,
                                     & ( aParseTree->segAttr ),
                                     ID_TRUE )
              != IDE_SUCCESS );

    /* 2. SegStoAttr를 검증하고, 기본값을 설정한다. */
    IDE_TEST( validateAndSetSegStoAttr( sTableType,
                                        NULL,
                                        & ( aParseTree->segStoAttr ),
                                        & ( aParseTree->existSegStoAttr ),
                                        ID_TRUE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateAndSetSegAttr( UInt          aTableType,
                                         smiSegAttr  * aSrcSegAttr,
                                         smiSegAttr  * aDstSegAttr,
                                         idBool        aIsTable )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      SegAttr를 검증하고, 주어진 값이 없을 시에 기본값을 설정한다.
 *
 *      관련내용 : BUG-29173 : Memory Table에 PCTFREE, PCTUSED, INITRANS, MAXTRANS 값을 설정하면 에러
 *                 BUG-29173 : Memory Table에 PCTFREE나 PCTUSED 값을 설정하면 에러
 *
 * Implementation :
 *     1. SegAttr 기본값을 설정한다.
 *     2. SegAttr를 검증한다.
 *        2.1. validation of pctfree
 *        2.2. validation of pctused
 *        2.3. validation of pctfree + pctused
 *        2.4. validation of inittrans
 *        2.5. validation of maxtrans
 *        2.6. validation of inittrans < maxtrans (DISK)
 *
 ***********************************************************************/

    UShort     sMinTableCtlSize = SMI_MINIMUM_TABLE_CTL_SIZE;
    UShort     sMaxTableCtlSize = SMI_MAXIMUM_TABLE_CTL_SIZE;
    smiSegAttr sOutput;

    /* For no compile warning */
    sOutput.mPctFree   = QD_INVALID_PCT_VALUE;
    sOutput.mPctUsed   = QD_INVALID_PCT_VALUE;
    sOutput.mInitTrans = QD_INVALID_TRANS_VALUE;
    sOutput.mMaxTrans  = QD_INVALID_TRANS_VALUE;
    
    /* 1. SegAttr 기본값을 설정한다.
     *    - Memory 매체의 경우, 무조건 Memory 기본값인 0을 설정한다.
     */
    if ( aSrcSegAttr != NULL )
    {
        adjustSegAttr( aTableType,
                       *aSrcSegAttr,
                       & sOutput,
                       aIsTable );
    }
    else
    {
        adjustSegAttr( aTableType,
                       *aDstSegAttr,
                       & sOutput,
                       aIsTable );
    }

    /* 2. SegAttr를 검증한다. */
    if ( QCM_TABLE_TYPE_IS_DISK ( aTableType ) == ID_TRUE )
    {
        if ( aIsTable == ID_TRUE )
        {
            /* 2.1. validation of pctfree */
            if ( aDstSegAttr->mPctFree == QD_INVALID_PCT_VALUE )
            {
                aDstSegAttr->mPctFree = sOutput.mPctFree;
            }
            else
            {
                IDE_TEST_RAISE( aDstSegAttr->mPctFree >= 100, ERR_INVALID_PCTFREE_VALUE );
            }

            /* 2.2. validation of pctused */
            if ( aDstSegAttr->mPctUsed == QD_INVALID_PCT_VALUE )
            {
                aDstSegAttr->mPctUsed = sOutput.mPctUsed;
            }
            else
            {
                IDE_TEST_RAISE( aDstSegAttr->mPctUsed >= 100, ERR_INVALID_PCTUSED_VALUE );
            }

            /* 2.3. validation of pctfree + pctused */
            IDE_TEST_RAISE( ( aDstSegAttr->mPctFree + aDstSegAttr->mPctUsed ) > 100,
                            ERR_INVALID_PCTFREE_PCTUSED_VALUE );

            /* 2.4. validation of inittrans */
            IDE_TEST_RAISE( aDstSegAttr->mInitTrans < sMinTableCtlSize, ERR_INVALID_INIT_TRANS );

            /* 2.5. validation of maxtrans */
            IDE_TEST_RAISE( aDstSegAttr->mMaxTrans < sMinTableCtlSize, ERR_INVALID_MAX_TRANS );
        }
        else
        {
            sMinTableCtlSize = SMI_MINIMUM_INDEX_CTL_SIZE;
            sMaxTableCtlSize = SMI_MAXIMUM_INDEX_CTL_SIZE;
        }

        /* 2.4. validation of inittrans */
        if ( aDstSegAttr->mInitTrans == QD_INVALID_TRANS_VALUE )
        {
            aDstSegAttr->mInitTrans = sOutput.mInitTrans;
        }
        else
        {
            IDE_TEST_RAISE( aDstSegAttr->mInitTrans > sMaxTableCtlSize, ERR_INVALID_INIT_TRANS );
        }

        /* 2.5. validation of maxtrans */
        if ( aDstSegAttr->mMaxTrans == QD_INVALID_TRANS_VALUE )
        {
            aDstSegAttr->mMaxTrans = sOutput.mMaxTrans;
        }
        else
        {
            IDE_TEST_RAISE( aDstSegAttr->mMaxTrans > sMaxTableCtlSize, ERR_INVALID_MAX_TRANS );
        }

        /* 2.6. validation of inittrans < maxtrans */
        IDE_TEST_RAISE( aDstSegAttr->mMaxTrans < aDstSegAttr->mInitTrans, ERR_MAXTRANS_LESS_INITRANS );
    }
    else
    {
        /* 관련 버그 : BUG-29173 Memory Table에 PCTFREE, PCTUSED, INITRANS, MAXTRANS 값을 설정하면 에러 */
        if ( aIsTable == ID_TRUE )
        {
            /* 2.1. validation of pctfree */
            IDE_TEST_RAISE( aDstSegAttr->mPctFree != QD_INVALID_PCT_VALUE, ERR_INVALID_TBS_FOR_PCTFREE_PCTUSED );

            /* 2.2. validation of pctused */
            IDE_TEST_RAISE( aDstSegAttr->mPctUsed != QD_INVALID_PCT_VALUE, ERR_INVALID_TBS_FOR_PCTFREE_PCTUSED );

            /* 2.4. validation of inittrans */
            IDE_TEST_RAISE( aDstSegAttr->mInitTrans != QD_INVALID_TRANS_VALUE, ERR_NO_DISK_TABLE );

            /* 2.5. validation of maxtrans */
            IDE_TEST_RAISE( aDstSegAttr->mMaxTrans  != QD_INVALID_TRANS_VALUE, ERR_NO_DISK_TABLE );

            aDstSegAttr->mPctFree   = sOutput.mPctFree;
            aDstSegAttr->mPctUsed   = sOutput.mPctUsed;
        }
        else
        {
            /* 2.4. validation of inittrans */
            IDE_TEST_RAISE( aDstSegAttr->mInitTrans != QD_INVALID_TRANS_VALUE, ERR_NO_DISK_INDEX );

            /* 2.5. validation of maxtrans */
            IDE_TEST_RAISE( aDstSegAttr->mMaxTrans  != QD_INVALID_TRANS_VALUE, ERR_NO_DISK_INDEX );
        }

        aDstSegAttr->mInitTrans = sOutput.mInitTrans;
        aDstSegAttr->mMaxTrans  = sOutput.mMaxTrans;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_PCTFREE_VALUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_PCTFREE_VALUE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_PCTUSED_VALUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_PCTUSED_VALUE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_PCTFREE_PCTUSED_VALUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_PCTFREE_PCTUSED_VALUE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_INIT_TRANS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_INIT_TRANS,
                                  sMinTableCtlSize,
                                  sMaxTableCtlSize ) );
    }
    IDE_EXCEPTION( ERR_INVALID_MAX_TRANS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_MAX_TRANS,
                                  sMinTableCtlSize,
                                  sMaxTableCtlSize ) );
    }
    IDE_EXCEPTION( ERR_MAXTRANS_LESS_INITRANS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_MAXTRANS_LESS_INITTRANS ) );
    }
    IDE_EXCEPTION( ERR_INVALID_TBS_FOR_PCTFREE_PCTUSED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_TBS_FOR_PCTFREE_PCTUSED ) );
    }
    IDE_EXCEPTION( ERR_NO_DISK_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NO_DISK_TABLE ) );
    }
    IDE_EXCEPTION( ERR_NO_DISK_INDEX )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NO_DISK_INDEX ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::validateAndSetSegStoAttr( UInt                aTableType,
                                            smiSegStorageAttr * aSrcStoAttr,
                                            smiSegStorageAttr * aDstStoAttr,
                                            qdSegStoAttrExist * aExist,
                                            idBool              aIsTable )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      SegStoAttr를 검증하고, 주어진 값이 없을 시에 기본값을 설정한다.
 *
 * Implementation :
 *     1. SegStoAttr 기본값을 설정한다.
 *     2. SegStoAttr를 검증한다.
 *        2.1. validation of initextents
 *        2.2. validation of nextextents
 *        2.3. validation of maxextents
 *        2.4. validation of minextents
 *        2.5. validation of minextents > initextents (DISK)
 *        2.6  validation of minextents < maxextents (DISK)
 *        2.7. SegStoAttr를 설정한다.
 *     3. SegStoAttr를 반환한다.
 *
 ***********************************************************************/

    SChar             sBuff[16];
    smiSegStorageAttr sOutput;

    /* 1. SegStoAttr 기본값을 설정한다.
     *    - Memory 매체의 경우, 무조건 Memory 기본값인 0을 설정한다.
     */
    if ( aSrcStoAttr != NULL )
    {
        adjustSegStoAttr( aTableType,
                          *aSrcStoAttr,
                          & sOutput );
    }
    else
    {
        adjustSegStoAttr( aTableType,
                          *aDstStoAttr,
                          & sOutput );
    }

    /* 2. SegStoAttr를 검증한다. */
    if ( QCM_TABLE_TYPE_IS_DISK( aTableType ) == ID_TRUE )
    {
        /* 2.1. validation of initextents */
        if ( aExist->mInitExt != ID_FALSE )
        {
            idlOS::snprintf( sBuff, ID_SIZEOF( sBuff ), (SChar*)"INITEXTENTS" );
            IDE_TEST_RAISE( aDstStoAttr->mInitExtCnt < 1, ERR_INVALID_STORAGE_OPTION_VALUE );
        }
        else
        {
            /* 2.7. SegStoAttr를 설정한다. */
            aDstStoAttr->mInitExtCnt = sOutput.mInitExtCnt;
        }

        /* 2.2. validation of nextextents  */
        if ( aExist->mNextExt != ID_FALSE )
        {
            idlOS::snprintf( sBuff, ID_SIZEOF( sBuff ), (SChar*)"NEXTEXTENTS" );
            IDE_TEST_RAISE( aDstStoAttr->mNextExtCnt < 1, ERR_INVALID_STORAGE_OPTION_VALUE );
        }
        else
        {
            /* 2.7. SegStoAttr를 설정한다. */
            aDstStoAttr->mNextExtCnt = sOutput.mNextExtCnt;
        }


        /* 2.3. validation of maxextents */
        if  ( aExist->mMaxExt != ID_FALSE )
        {
            idlOS::snprintf( sBuff, ID_SIZEOF( sBuff ), (SChar*)"MAXEXTENTS" );
            IDE_TEST_RAISE( aDstStoAttr->mMaxExtCnt < 1, ERR_INVALID_STORAGE_OPTION_VALUE );
        }
        else
        {
            /* 2.7. SegStoAttr를 설정한다. */
            aDstStoAttr->mMaxExtCnt  = sOutput.mMaxExtCnt;
        }

        /* 2.4. validation of minextents */
        if ( aExist->mMinExt != ID_FALSE )
        {
            idlOS::snprintf( sBuff, ID_SIZEOF( sBuff ), (SChar*)"MINEXTENTS" );
            IDE_TEST_RAISE( aDstStoAttr->mMinExtCnt < 1, ERR_INVALID_STORAGE_OPTION_VALUE );
        }
        else
        {
            /* 2.7. SegStoAttr를 설정한다. */
            aDstStoAttr->mMinExtCnt  = sOutput.mMinExtCnt;
        }

        /* 2.5. validation of minextents > initextents */
        IDE_TEST_RAISE( aDstStoAttr->mMinExtCnt > aDstStoAttr->mInitExtCnt,
                        ERR_MINEXTENTS_LESS_INITEXTENTS );

        /* 2.6. validation of minextents < maxextents */
        IDE_TEST_RAISE( aDstStoAttr->mMinExtCnt > aDstStoAttr->mMaxExtCnt,
                        ERR_MINEXTENTS_LESS_MAXEXTENTS );
    }
    else
    {
        /* 2. SegStoAttr를 검증한다. */
        if ( aIsTable == ID_TRUE )
        {
            /* 2.1. validation of initextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mInitExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS ) ||
                            ( aExist->mInitExt != ID_FALSE ),
                            ERR_NO_DISK_TABLE );

            /* 2.2. validation of nextextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mNextExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS ) ||
                            ( aExist->mNextExt != ID_FALSE ),
                            ERR_NO_DISK_TABLE );

            /* 2.3. validation of maxextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mMinExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS ) ||
                            ( aExist->mMinExt != ID_FALSE ),
                            ERR_NO_DISK_TABLE );

            /* 2.4. validation of minextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mMaxExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS ) ||
                            ( aExist->mMaxExt != ID_FALSE ),
                            ERR_NO_DISK_TABLE );
        }
        else
        {
            /* 2.1. validation of initextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mInitExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS ) ||
                            ( aExist->mInitExt != ID_FALSE ),
                            ERR_NO_DISK_INDEX );

            /* 2.2. validation of nextextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mNextExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS ) ||
                            ( aExist->mNextExt != ID_FALSE ),
                            ERR_NO_DISK_INDEX );

            /* 2.3. validation of maxextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mMinExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS ) ||
                            ( aExist->mMinExt != ID_FALSE ),
                            ERR_NO_DISK_INDEX );

            /* 2.4. validation of minextents */
            IDE_TEST_RAISE( ( aDstStoAttr->mMaxExtCnt != QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS ) ||
                            ( aExist->mMaxExt != ID_FALSE ),
                            ERR_NO_DISK_INDEX );
        }

        /* 2.7. SegStoAttr를 설정한다. */
        aDstStoAttr->mInitExtCnt = sOutput.mInitExtCnt;
        aDstStoAttr->mNextExtCnt = sOutput.mNextExtCnt;
        aDstStoAttr->mMinExtCnt  = sOutput.mMinExtCnt;
        aDstStoAttr->mMaxExtCnt  = sOutput.mMaxExtCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_STORAGE_OPTION_VALUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDD_INVALID_STORAGE_OPTION_VALUE, sBuff ) );
    }
    IDE_EXCEPTION( ERR_MINEXTENTS_LESS_INITEXTENTS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_MINEXTENTS_LESS_INITEXTENTS ) );
    }
    IDE_EXCEPTION( ERR_MINEXTENTS_LESS_MAXEXTENTS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_MINEXTENTS_LESS_MAXEXTENTS ) );
    }
    IDE_EXCEPTION( ERR_NO_DISK_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NO_DISK_TABLE ) );
    }
    IDE_EXCEPTION( ERR_NO_DISK_INDEX )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NO_DISK_INDEX ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdbCommon::adjustIndexAttr( scSpaceID           aTableTBSID,
                                 smiSegAttr          aSrcSegAttr,
                                 smiSegStorageAttr   aSrcSegStoAttr,
                                 UInt                aSrcIndexFlag,
                                 ULong               aSrcDirectKeyMaxSize,
                                 smiSegAttr        * aDstSegAttr,
                                 smiSegStorageAttr * aDstSegStoAttr,
                                 UInt              * aDstIndexFlag,
                                 ULong             * aDstDirectKeyMaxSize )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Table에 적합하게 Index Attr를 생성한다.
 *
 * Implementation :
 *      1. Table Type을 검사한다.
 *
 *      2. adjustPhysicalAttr()를 호출한다.
 *
 *      3. DirectKeyMaxSize 설정한다.
 *
 ***********************************************************************/

    UInt sTableType = SMI_TABLE_DISK;

    /* 1. Table Type을 검사한다. */
    sTableType = getTableTypeFromTBSID( aTableTBSID );

    /* 2. adjustPhysicalAttr()를 호출한다. */
    adjustPhysicalAttr( sTableType,
                        aSrcSegAttr,
                        aSrcSegStoAttr,
                        aDstSegAttr,
                        aDstSegStoAttr,
                        ID_FALSE /* aIsTable */ );

    /* 3. DirectKeyMaxSize 설정 */
    adjustDirectKeyMaxSize( aSrcIndexFlag,
                            aSrcDirectKeyMaxSize,
                            aDstIndexFlag,
                            aDstDirectKeyMaxSize );

    return;
}

void qdbCommon::adjustPhysicalAttr( UInt                aTableType,
                                    smiSegAttr          aSrcSegAttr,
                                    smiSegStorageAttr   aSrcSegStoAttr,
                                    smiSegAttr        * aDstSegAttr,
                                    smiSegStorageAttr * aDstSegStoAttr,
                                    idBool              aIsTable )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Table Type에 적합하게 PhysicalAttr를 생성한다.
 *      그리고 Index, Constraint의 경우 PCT 관련옵션을 설정하지 않는다.
 *      이러한 상태를 구별하기 위해서 aIsTable 변수로 이용한다.
 *
 * Implementation :
 *      1. SegAttr 설정한다.
 *         1.1. Table인 경우 추가 조정한다.
 *
 *      2. SegStoAttr 설정한다.
 *
 ***********************************************************************/

    /* 1. SegAttr 설정 */
    adjustSegAttr( aTableType,
                   aSrcSegAttr,
                   aDstSegAttr,
                   aIsTable );

    /* 2. SegStoAttr 설정한다. */
    adjustSegStoAttr( aTableType,
                      aSrcSegStoAttr,
                      aDstSegStoAttr );

    return;
}

void qdbCommon::adjustSegAttr( UInt         aTableType,
                               smiSegAttr   aSrcSegAttr,
                               smiSegAttr * aDstSegAttr,
                               idBool       aIsTable )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      TableSpace에 맞는 SegAttr으로 조정한다.
 *      그리고 Index, Constraint의 경우 PCT 관련옵션을 설정하지 않는다.
 *      이러한 상태를 구별하기 위해서 aIsTable 변수를 이용한다.
 *
 * Implementation :
 *      1. Memory TableSpace의 SegAttr 설정
 *         1.1. Table인 경우 추가 조정
 *
 *      2. Disk TableSpace의 SegAttr 설정
 *         2.1. Table인 경우 추가 조정
 *
 ***********************************************************************/

    /* 1. Memory TableSpace의 SegAttr 설정*/
    if ( aTableType != SMI_TABLE_DISK )
    {
        /* 1.1. Table인 경우 추가 조정
         *  - 관련내용 : BUG-15192 Memory Table's PCTFREE, PCTUSED is always 100
         */
        if ( aIsTable == ID_TRUE )
        {
            aDstSegAttr->mPctFree   = QD_MEMORY_TABLE_DEFAULT_PCTFREE;
            aDstSegAttr->mPctUsed   = QD_MEMORY_TABLE_DEFAULT_PCTUSED;
            aDstSegAttr->mInitTrans = QD_MEMORY_TABLE_DEFAULT_INITRANS;
            aDstSegAttr->mMaxTrans  = QD_MEMORY_TABLE_DEFAULT_MAXTRANS;
        }
        else
        {
            aDstSegAttr->mInitTrans = QD_MEMORY_INDEX_DEFAULT_INITRANS;
            aDstSegAttr->mMaxTrans  = QD_MEMORY_INDEX_DEFAULT_MAXTRANS;
        }
    }
    /* 2. Disk TableSpace의 SegAttr 설정 */
    else
    {
        /* 2.1. Table인 경우 추가 조정 */
        if ( aIsTable == ID_TRUE )
        {
            /* PCTFREE값을 설정하지 않았을 때 기본값 설정 */
            if ( ( aSrcSegAttr.mPctFree == QD_INVALID_PCT_VALUE ) ||
                 ( aSrcSegAttr.mPctFree == QD_MEMORY_TABLE_DEFAULT_PCTFREE ) )
            {
                aDstSegAttr->mPctFree = QD_DISK_TABLE_DEFAULT_PCTFREE;
            }
            else
            {
                aDstSegAttr->mPctFree = aSrcSegAttr.mPctFree;
            }

            /* PCTUSED값을 설정하지 않았을 때 기본값 설정 */
            if ( ( aSrcSegAttr.mPctUsed == QD_INVALID_PCT_VALUE ) ||
                 ( aSrcSegAttr.mPctUsed == QD_MEMORY_TABLE_DEFAULT_PCTUSED ) )
            {
                aDstSegAttr->mPctUsed = QD_DISK_TABLE_DEFAULT_PCTUSED;
            }
            else
            {
                aDstSegAttr->mPctUsed = aSrcSegAttr.mPctUsed;
            }

            /* INITTRANS값을 설정하지 않았을 때 기본값 설정 */
            if ( ( aSrcSegAttr.mInitTrans == QD_INVALID_TRANS_VALUE ) ||
                 ( aSrcSegAttr.mInitTrans == QD_MEMORY_TABLE_DEFAULT_INITRANS ) )
            {
                aDstSegAttr->mInitTrans = QD_DISK_TABLE_DEFAULT_INITRANS;
            }
            else
            {
                aDstSegAttr->mInitTrans = aSrcSegAttr.mInitTrans;
            }

            /* MAXTRANS값을 설정하지 않았을 때 기본값 설정 */
            if ( ( aSrcSegAttr.mMaxTrans == QD_INVALID_TRANS_VALUE ) ||
                 ( aSrcSegAttr.mMaxTrans == QD_MEMORY_TABLE_DEFAULT_MAXTRANS ) )
            {
                aDstSegAttr->mMaxTrans = QD_DISK_TABLE_DEFAULT_MAXTRANS;
            }
            else
            {
                aDstSegAttr->mMaxTrans = aSrcSegAttr.mMaxTrans;
            }
        }
        else
        {
            /* INITTRANS값을 설정하지 않았을 때 기본값 설정 */
            if ( ( aSrcSegAttr.mInitTrans == QD_INVALID_TRANS_VALUE ) ||
                 ( aSrcSegAttr.mInitTrans == QD_MEMORY_INDEX_DEFAULT_INITRANS ) )
            {
                aDstSegAttr->mInitTrans = QD_DISK_INDEX_DEFAULT_INITRANS;
            }
            else
            {
                aDstSegAttr->mInitTrans = aSrcSegAttr.mInitTrans;
            }

            /* MAXTRANS값을 설정하지 않았을 때 기본값 설정 */
            if ( ( aSrcSegAttr.mMaxTrans == QD_INVALID_TRANS_VALUE ) ||
                 ( aSrcSegAttr.mMaxTrans == QD_MEMORY_INDEX_DEFAULT_MAXTRANS ) )
            {
                aDstSegAttr->mMaxTrans = QD_DISK_INDEX_DEFAULT_MAXTRANS;
            }
            else
            {
                aDstSegAttr->mMaxTrans = aSrcSegAttr.mMaxTrans;
            }
        }
    }

    return;
}

void qdbCommon::adjustSegStoAttr( UInt                aTableType,
                                  smiSegStorageAttr   aSrcSegStoAttr,
                                  smiSegStorageAttr * aDstSegStoAttr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      TableSpace에 맞는 SegStoAttr으로 조정한다.
 *
 * Implementation :
 *      1. Memory TableSpace의 SegStoAttr 설정
 *      2. Disk TableSpace의 SegStoAttr 설정
 *
 ***********************************************************************/

    /* 1. Memory TableSpace의 SegStoAttr 설정*/
    if ( aTableType != SMI_TABLE_DISK )
    {
        aDstSegStoAttr->mInitExtCnt = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;
        aDstSegStoAttr->mNextExtCnt = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;
        aDstSegStoAttr->mMinExtCnt  = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;
        aDstSegStoAttr->mMaxExtCnt  = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;
    }
    /* 2. Disk TableSpace의 SegStoAttr 설정 */
    else
    {
        /* INITEXTCNT값을 설정하지 않았을 때 기본값 설정 */
        if ( aSrcSegStoAttr.mInitExtCnt == QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS )
        {
            aDstSegStoAttr->mInitExtCnt = QD_DISK_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;
        }
        else
        {
            aDstSegStoAttr->mInitExtCnt = aSrcSegStoAttr.mInitExtCnt;
        }

        /* NEXTEXTCNT값을 설정하지 않았을 때 기본값 설정 */
        if ( aSrcSegStoAttr.mNextExtCnt == QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS )
        {
            aDstSegStoAttr->mNextExtCnt = QD_DISK_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;
        }
        else
        {
            aDstSegStoAttr->mNextExtCnt = aSrcSegStoAttr.mNextExtCnt;
        }

        /* MINEXTCNT값을 설정하지 않았을 때 기본값 설정 */
        if ( aSrcSegStoAttr.mMinExtCnt == QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS  )
        {
            aDstSegStoAttr->mMinExtCnt = QD_DISK_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;
        }
        else
        {
            aDstSegStoAttr->mMinExtCnt = aSrcSegStoAttr.mMinExtCnt;
        }

        /* MAXEXTCNT값을 설정하지 않았을 때 기본값 설정 */
        if ( aSrcSegStoAttr.mMaxExtCnt == QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS  )
        {
            aDstSegStoAttr->mMaxExtCnt = QD_DISK_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;
        }
        else
        {
            aDstSegStoAttr->mMaxExtCnt = aSrcSegStoAttr.mMaxExtCnt;
        }
    }

    return;
}

void qdbCommon::adjustDirectKeyMaxSize( UInt    aSrcIndexFlag,
                                        ULong   aSrcDirectKeyMaxSize,
                                        UInt  * aDstIndexFlag,
                                        ULong * aDstDirectKeyMaxSize )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      TableSpace에 맞는 IndexFlag, DirectKeyMaxSize으로 조정한다.
 *
 * Implementation :
 *      1. Memory TableSpace의 DirectKeyMaxSize 설정
 *      2. Disk TableSpace의 DirectKeyMaxSize 설정
 *
 ***********************************************************************/

    *aDstIndexFlag        = aSrcIndexFlag;
    *aDstDirectKeyMaxSize = aSrcDirectKeyMaxSize;

    /* TO DO - Direct Key가 Partitioned Table에도 지원이 되면 아래의 코드를 사용하도록 한다. */
    // /* 1. Memory TableSpace의 DirectKeyMaxSize 설정 */
    // if ( aTableType != SMI_TABLE_DISK )
    // {
    //     *aDstIndexFlag        = aSrcIndexFlag;
    //     *aDstDirectKeyMaxSize = aSrcDirectKeyMaxSize;
    // }
    // /* 2. Disk TableSpace의 DirectKeyMaxSize 설정 */
    // else
    // {
    //     *aDstIndexFlag         = aSrcIndexFlag;
    //     *aDstIndexFlag        &= ~(SMI_INDEX_DIRECTKEY_MASK);
    //     *aDstIndexFlag        |= SMI_INDEX_DIRECTKEY_FALSE;
    //     *aDstDirectKeyMaxSize  = 0;
    // }

    return;
}

void qdbCommon::makeTempQcmColumnListFromIndex( qcmIndex  * aIndex,
                                                mtcColumn * aMtcColumnArr,
                                                qcmColumn * aQcmColumnList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      주어진 Index를 QcmColumn List에 연결한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn * sColumn;
    UInt        i;

    IDE_DASSERT( aIndex != NULL );

    for ( i = 0; i < aIndex->keyColCount; i++ )
    {
        sColumn = &( aQcmColumnList[i] );

        /* 원본 MtcColumn 유지하기 위해서 복사한다. */
        idlOS::memcpy( &( aMtcColumnArr[i] ), &( aIndex->keyColumns[i] ), ID_SIZEOF( mtcColumn ) );

        QCM_COLUMN_INIT( sColumn );
        sColumn->basicInfo = &( aMtcColumnArr[i] );

        if ( ( i + 1 ) < aIndex->keyColCount )
        {
            sColumn->next = &( aQcmColumnList[i + 1] );
        }
        else
        {
            sColumn->next = NULL;
        }
    }

    return;
}

IDE_RC qdbCommon::adjustIndexColumn( qcmColumn     * aColumn,
                                     qcmIndex      * aIndex,
                                     qcmColumn     * aDelColList,
                                     smiColumnList * aIndexColumnList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Column/Index에 적합하게 Index Column List를 조정한다.
 *
 * Implementation :
 *      1. Index를 전달받은 경우, Temp QcmColmn를 만들어 이용한다.
 *      2. Index Column과 동일한 Column을 검사한다.
 *      3. 원본 MtcColumn 유지하기 위해서 복사한다.
 *      4. Key Column를 조정한다.
 *
 ***********************************************************************/

    qcmColumn       sKeyColumn[QC_MAX_KEY_COLUMN_COUNT];
    mtcColumn       sMtcColumnForKey[QC_MAX_KEY_COLUMN_COUNT];
    qcmColumn     * sIndexColumn  = NULL;
    smiColumnList * sColumnList   = NULL;
    qcmColumn     * sSrcQcmColumn = NULL;
    mtcColumn     * sSrcMtcColumn = NULL;
    mtcColumn       sTmpMtcColumn;
    smiColumn     * sDstSmiColumn = NULL;
    UInt            sColumnID;
    UInt            sOffset       = 0;

    IDE_DASSERT( ( ( aColumn == NULL ) && ( aIndex != NULL ) ) ||
                 ( ( aColumn != NULL ) && ( aIndex == NULL ) ) );

    /* 1. Index를 전달받은 경우, Temp QcmColmn를 만들어 이용한다. */
    if ( aIndex != NULL )
    {
        makeTempQcmColumnListFromIndex( aIndex,
                                        sMtcColumnForKey,
                                        sKeyColumn );
        sIndexColumn = sKeyColumn;
    }
    else
    {
        sIndexColumn = aColumn;
    }

    for ( sColumnList  = aIndexColumnList;
          sColumnList != NULL;
          sColumnList  = sColumnList->next )
    {
        sDstSmiColumn = (smiColumn *)sColumnList->column;

        /* 2. Index Column과 동일한 Column을 검사한다. */
        for ( sSrcQcmColumn  = sIndexColumn;
              sSrcQcmColumn != NULL;
              sSrcQcmColumn  = sSrcQcmColumn->next )
        {
            sSrcMtcColumn = sSrcQcmColumn->basicInfo;

            if ( findColumnIDInColumnList( aDelColList,
                                           sSrcMtcColumn->column.id )
                 == ID_FALSE )
            {
                sColumnID = getNewColumnIDForAlter( aDelColList,
                                                    sSrcMtcColumn->column.id );

                if ( ( sDstSmiColumn->id & SMI_COLUMN_ID_MASK ) ==
                             ( sColumnID & SMI_COLUMN_ID_MASK ) )
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

        IDE_TEST_RAISE( sSrcQcmColumn == NULL, ERR_NO_EXIST_COLUMN );

        /* 3. 원본 MtcColumn 유지하기 위해서 복사한다. */
        idlOS::memcpy( &( sTmpMtcColumn ), sSrcMtcColumn, ID_SIZEOF( mtcColumn ) );

        sTmpMtcColumn.column.id = sDstSmiColumn->id;

        /* 4. Key Column을 조정한다. */
        IDE_TEST( adjustColumnFlagForIndex( &( sTmpMtcColumn ),
                                            &( sOffset ),
                                            sDstSmiColumn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_EXIST_COLUMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbCommon::adjustIndexColumn",
                                  "No exist column" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::adjustKeyColumn( qcmColumn * aTableColumn,
                                   qcmColumn * aKeyColumn )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Table Column에 적합하게 Index Column List를 조정한다.
 *
 * Implementation :
 *
 *      1. Index Column에 해당하는 Table Column을 얻는다.
 *      2. Add Column인 경우, Storage Type를 기준으로 조정한다.
 *      3. Key Column을 조정한다.
 *
 ***********************************************************************/

    qcmColumn * sKeyColumn    = NULL;
    qcmColumn * sSrcQcmColumn = NULL;
    mtcColumn * sSrcMtcColumn = NULL;
    smiColumn * sDstSmiColumn = NULL;
    UInt        sOffset       = 0;

    IDE_DASSERT( aTableColumn != NULL );

    for ( sKeyColumn  = aKeyColumn;
          sKeyColumn != NULL;
          sKeyColumn  = sKeyColumn->next )
    {
        sDstSmiColumn = &( sKeyColumn->basicInfo->column );

        for ( sSrcQcmColumn  = aTableColumn;
              sSrcQcmColumn != NULL;
              sSrcQcmColumn  = sSrcQcmColumn->next )
        {
            sSrcMtcColumn  = sSrcQcmColumn->basicInfo;

            /* 1. Index Column에 해당하는 Table Column을 얻는다.
             *     - Column를 찾는 이유는 아래의 ERR_NO_EXIST_COLUMN 에러를 출력하기 목적이다.
             */
            if ( sSrcQcmColumn->namePos.size > 0 )
            {
                /* Create 단계 */
                if ( QC_IS_NAME_MATCHED( sSrcQcmColumn->namePos,
                                         sKeyColumn->namePos )
                     == ID_TRUE )
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
                /* Alter 단계 */
                if ( sDstSmiColumn->id == sSrcMtcColumn->column.id )
                {
                    break;
                }
                /* PROJ-2465 Tablespace Alteration for Table
                 *  - PROJ-2464의 버그가 있어 수정한다.
                 *
                 * 2. Add Column인 경우, Storage Type를 기준으로 조정한다.
                 *
                 *     * ADD COLUMN의 경우에 수행된다.
                 *
                 *       - aTableColumn에 Add되는 Column인 sKeyColumn정보가 없어, 상대적으로 조정할 수 없다.
                 *       - 따라서, aTableColumn의 Storage Type를 기준으로 조정한다.
                 *
                 *       - Add Column은 qdn::validateConstraints 에서 smiColumn의 column.id를 0으로 초기화하고 있다.
                 *       - Create, Add Column, Function-base Index에서 동일하게 초기화한다.
                 *
                 *       - sKeyColumn은 ParseTree에 연결된 ConstraintColumn으로 메타캐시의 정보가 아니다.
                 *       - 그래서 sKeyColumn의 원본 MtcColumn 유지하기 위해서 복사할 필요가 없다.
                 *       - 입력값인 aTableColumn은 Table 또는 Partition의 Column일 수 있다.
                 *       - 이때에 Index는 Table 또는 Partition에 종속된다.
                 *       - 이후에 Table 또는 Partition Type를 검사하게 된다.
                 *       - 즉, 적절한 Type에 맞게 Flag를 조정해야 한다.
                 */
                else if ( sDstSmiColumn->id == 0 )
                {
                    sSrcMtcColumn               = sKeyColumn->basicInfo;
                    sSrcMtcColumn->column.flag &= sSrcMtcColumn->column.flag & ~SMI_COLUMN_STORAGE_MASK;
                    sSrcMtcColumn->column.flag |= aTableColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        IDE_TEST_RAISE( sSrcQcmColumn == NULL, ERR_NO_EXIST_COLUMN );

        /* 3. Key Column을 조정한다. */
        IDE_TEST( adjustColumnFlagForIndex( sSrcMtcColumn,
                                            &( sOffset ),
                                            sDstSmiColumn )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_EXIST_COLUMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbCommon::adjustKeyColumn",
                                  "No exist column" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::adjustColumnFlagForIndex( mtcColumn * aSrcMtcColumn,
                                            UInt      * aOffset,
                                            smiColumn * aDstSmiColumn )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *
 *      Table Column에 적합하게 Index Column List를 조정한다.
 *
 * Implementation :
 *      1. 원본 MtcColumn 유지하기 위해서 복사한다.
 *      2. Disk Type이면 추가 작업으로 Column를 조정한다.
 *      3. Order 정보와 Index 사용여부는 그대로 전달한다.
 *      4. Index Column에 조정한 Column를 복사한다.
 *
 ***********************************************************************/

    mtcColumn   sTmpMtcColumn;
    smiColumn * sTmpSmiColumn = NULL;

    /* 1. 원본 MtcColumn 유지하기 위해서 복사한다. */
    idlOS::memcpy( &( sTmpMtcColumn ), aSrcMtcColumn, ID_SIZEOF( mtcColumn ) );

    sTmpSmiColumn = &( sTmpMtcColumn.column );

    /* 2. Disk Type이면 추가 작업으로 Column를 조정한다.
     *    - qdbCommon::createConstraintFromInfo
     *    - qdbCommon::createIndexFromInfo
     *    - qdbCommon::createConstrPrimaryUnique
     *    - qdbCommon::createIndexFromIndexParseTree
     */
    if ( ( aSrcMtcColumn->column.flag & SMI_COLUMN_STORAGE_MASK ) == SMI_COLUMN_STORAGE_DISK )
    {
        /* PROJ-1705 */
        IDE_TEST( setIndexKeyColumnTypeFlag( &( sTmpMtcColumn ) ) != IDE_SUCCESS );

        /* To Fix PR-8111 */
        if ( ( sTmpSmiColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
        {
            *aOffset               = idlOS::align( *aOffset, sTmpMtcColumn.module->align );
            sTmpSmiColumn->offset  = *aOffset;
            sTmpSmiColumn->value   = NULL;
            *aOffset              += sTmpSmiColumn->size;
        }
        else
        {
            *aOffset               = idlOS::align( *aOffset, 8 );
            sTmpSmiColumn->offset  = *aOffset;
            sTmpSmiColumn->value   = NULL;
            *aOffset              += smiGetVariableColumnSize4DiskIndex();
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 3. Order 정보와 Index 사용여부는 그대로 전달한다. */
    sTmpSmiColumn->flag &= ~SMI_COLUMN_ORDER_MASK;
    sTmpSmiColumn->flag |= aDstSmiColumn->flag & SMI_COLUMN_ORDER_MASK;
    sTmpSmiColumn->flag &= ~SMI_COLUMN_USAGE_MASK;
    sTmpSmiColumn->flag |= aDstSmiColumn->flag & SMI_COLUMN_USAGE_MASK;

    /* 4. Index Column에 조정한 Column를 복사한다. */
    idlOS::memcpy( aDstSmiColumn, sTmpSmiColumn, ID_SIZEOF( smiColumn ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updatePartTableTBSFromMeta( qcStatement * aStatement,
                                              UInt          aPartTableID,
                                              UInt          aPartitionID,
                                              scSpaceID     aTBSID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *      ALTER TABLE 관련 SYS_TABLE_PARTITIONS_ 메타 테이블 변경 수행
 *
 * Implementation :
 *      1. SYS_TABLE_PARTITIONS_ 메타 테이블의 정보 변경
 *
 ***********************************************************************/

    SChar  sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLE_PARTITIONS_ "
                     "SET TBS_ID = INTEGER'%"ID_INT32_FMT"', "
                     "    LAST_DDL_TIME = SYSDATE "
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "PARTITION_ID = INTEGER'%"ID_INT32_FMT"' ",
                     (UInt) aTBSID,
                     aPartTableID,
                     aPartitionID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS);

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdbCommon::updatePartLobsTBSFromMeta( qcStatement * aStatement,
                                             UInt          aUserID,
                                             UInt          aPartTableID,
                                             UInt          aPartitionID,
                                             UInt          aLobColumnID,
                                             scSpaceID     aTBSID )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2464 hybrid partitioned table 지원
 *      ALTER TABLE 관련 SYS_PART_LOBS_ 메타 테이블 변경 수행
 *
 * Implementation :
 *      1.  SYS_PART_LOBS_ 메타 테이블의 정보 변경
 *
 ***********************************************************************/

    SChar  sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_PART_LOBS_ "
                     "SET TBS_ID = INTEGER'%"ID_INT32_FMT"' "
                     "WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "PARTITION_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "COLUMN_ID = INTEGER'%"ID_INT32_FMT"' ",
                     (UInt) aTBSID,
                     aUserID,
                     aPartTableID,
                     aPartitionID,
                     aLobColumnID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS);

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* BUG-42321 The smi cursor type of smistatement is set wrongly in partitioned
 * table
 *
 * foreignkey의 정보를 보고 parent table의 smiStatementFlag를 누적시킨다.
 */
IDE_RC qdbCommon::checkForeignKeyParentTableInfo( qcStatement  * aStatement,
                                                  qcmTableInfo * aTableInfo,
                                                  SChar        * aConstraintName,
                                                  idBool         aIsValidate )
{
    qcmTableInfo         * sParentTableInfo   = NULL;
    qcmForeignKey        * sForeignKey        = NULL;
    void                 * sParentTableHandle = NULL;
    UInt                   sNameLen           = 0;
    qcmPartitionInfoList * sPartInfoList      = NULL;
    smSCN                  sParentTableSCN;
    UInt                   i;

    IDE_TEST_CONT( aIsValidate == ID_FALSE, normal_exit );

    sForeignKey = aTableInfo->foreignKeys;
    sNameLen = idlOS::strlen( aConstraintName );

    for ( i = 0; i < aTableInfo->foreignKeyCount; i++ )
    {
        if ( ( idlOS::strMatch( aConstraintName,
                                sNameLen, 
                                sForeignKey[i].name,
                                idlOS::strlen( sForeignKey[i].name ) ) == 0 ) &&
             ( sForeignKey[i].validated == ID_FALSE ) )
        {
            IDE_TEST( qcm::getTableInfoByID( aStatement,
                                             sForeignKey[i].referencedTableID,
                                             &sParentTableInfo,
                                             &sParentTableSCN,
                                             &sParentTableHandle )
                      != IDE_SUCCESS );

            IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                                      sParentTableHandle,
                                                      sParentTableSCN )
                      != IDE_SUCCESS );

            if ( sParentTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_TEST( checkAndSetAllPartitionInfo( aStatement,
                                                       sForeignKey[i].referencedTableID,
                                                       & ( sPartInfoList ) )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updateTableTBSFromMeta( qcStatement       * aStatement,
                                          UInt                aTableID,
                                          smOID               aTableOID,
                                          scSpaceID           aTBSID,
                                          SChar             * aTBSName,
                                          smiSegAttr          aSegAttr,
                                          smiSegStorageAttr   aSegStoAttr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Tablespace 관련 SYS_TABLES_ Meta Table 변경
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLES_ "
                     "SET TABLE_OID     = BIGINT'%"ID_INT64_FMT"', "
                     "    TBS_ID        = INTEGER'%"ID_INT32_FMT"', "
                     "    TBS_NAME      = VARCHAR'%s', "
                     "    PCTFREE       = INTEGER'%"ID_INT32_FMT"', "
                     "    PCTUSED       = INTEGER'%"ID_INT32_FMT"', "
                     "    INIT_TRANS    = INTEGER'%"ID_INT32_FMT"', "
                     "    MAX_TRANS     = INTEGER'%"ID_INT32_FMT"', "
                     "    INITEXTENTS   = BIGINT'%"ID_INT64_FMT"', "
                     "    NEXTEXTENTS   = BIGINT'%"ID_INT64_FMT"', "
                     "    MINEXTENTS    = BIGINT'%"ID_INT64_FMT"', "
                     "    MAXEXTENTS    = BIGINT'%"ID_INT64_FMT"', "
                     "    LAST_DDL_TIME = SYSDATE "
                     "WHERE TABLE_ID    = INTEGER'%"ID_INT32_FMT"' ",
                     (ULong)aTableOID,
                     (UInt)aTBSID,
                     aTBSName,
                     (UInt)aSegAttr.mPctFree,
                     (UInt)aSegAttr.mPctUsed,
                     (UInt)aSegAttr.mInitTrans,
                     (UInt)aSegAttr.mMaxTrans,
                     (ULong)aSegStoAttr.mInitExtCnt,
                     (ULong)aSegStoAttr.mNextExtCnt,
                     (ULong)aSegStoAttr.mMinExtCnt,
                     (ULong)aSegStoAttr.mMaxExtCnt,
                     aTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS);

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updateColumnFlagFromMeta( qcStatement * aStatement,
                                            qcmColumn   * aColumns )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Tablespace 관련 SYS_COLUMNS_ Meta Table 변경
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar       sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong      sRowCnt              = 0;
    SChar     * sStoreTypeStr[4]     = { (SChar *)"F", (SChar *)"V", (SChar *)"V", (SChar *)"L" };
    SChar       sStoreTypeStrNull[1] = "";
    SChar     * sStoreType           = NULL;
    qcmColumn * sColumn              = NULL;

    for ( sColumn  = aColumns;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_STORAGE_MASK )
             == SMI_COLUMN_STORAGE_MEMORY )
        {
            switch ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_FIXED :
                    sStoreType = sStoreTypeStr[0];
                    break;

                case SMI_COLUMN_TYPE_VARIABLE :
                    sStoreType = sStoreTypeStr[1];
                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE :
                    sStoreType = sStoreTypeStr[2];
                    break;

                case SMI_COLUMN_TYPE_LOB :
                    sStoreType = sStoreTypeStr[3];
                    break;

                default:
                    IDE_ERROR( 0 );
                    break;
            }
        }
        else
        {
            switch ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
            {
                case SMI_COLUMN_TYPE_FIXED :
                    sStoreType = sStoreTypeStrNull;
                    break;

                case SMI_COLUMN_TYPE_LOB :
                    sStoreType = sStoreTypeStr[3];
                    break;

                default:
                    IDE_ERROR( 0 );
                    break;
            }
        }

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COLUMNS_ "
                         "SET OFFSET      = BIGINT'%"ID_INT64_FMT"', "
                         "    STORE_TYPE  = CHAR'%s', "
                         "    IN_ROW_SIZE = INTEGER'%"ID_INT32_FMT"' "
                         "WHERE COLUMN_ID = INTEGER'%"ID_INT32_FMT"' ",
                         (ULong)sColumn->basicInfo->column.offset,
                         sStoreType,
                         sColumn->basicInfo->column.vcInOutBaseSize,
                         sColumn->basicInfo->column.id );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS);

        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updateLobTBSFromMeta( qcStatement * aStatement,
                                        qcmColumn   * aColumns )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Tablespace 관련 SYS_LOBS_ Meta Table 변경
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar       sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong      sRowCnt = 0;
    qcmColumn * sColumn = NULL;

    for ( sColumn  = aColumns;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        if ( ( sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
                                               == MTD_COLUMN_TYPE_LOB )
        {
            if ( ( sColumn->flag & QCM_COLUMN_LOB_DEFAULT_TBS_MASK )
                                == QCM_COLUMN_LOB_DEFAULT_TBS_TRUE )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_LOBS_ "
                                 "SET TBS_ID         = INTEGER'%"ID_INT32_FMT"', "
                                 "    IS_DEFAULT_TBS = CHAR'T' "
                                 "WHERE COLUMN_ID    = INTEGER'%"ID_INT32_FMT"' ",
                                 (UInt)sColumn->basicInfo->column.colSpace,
                                 sColumn->basicInfo->column.id );
            }
            else
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_LOBS_ "
                                 "SET TBS_ID         = INTEGER'%"ID_INT32_FMT"', "
                                 "    IS_DEFAULT_TBS = CHAR'F' "
                                 "WHERE COLUMN_ID    = INTEGER'%"ID_INT32_FMT"' ",
                                 (UInt)sColumn->basicInfo->column.colSpace,
                                 sColumn->basicInfo->column.id );
            }

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                      != IDE_SUCCESS);

            IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::updateIndexTBSFromMeta( qcStatement               * aStatement,
                                          qcmIndex                  * aIndices,
                                          UInt                        aIndexCount,
                                          qdIndexPartitionAttribute * aAllIndexTBSAttr )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Tablespace 관련 SYS_INDICES_ Meta Table 변경
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                       sSqlStr[QD_MAX_SQL_LENGTH];
    vSLong                      sRowCnt       = 0;
    qcmIndex                  * sIndex        = NULL;
    qdIndexPartitionAttribute * sIndexTBSAttr = NULL;
    UInt                        i             = 0;
    idBool                      sIsFound;

    for ( i = 0; i < aIndexCount; i++ )
    {
        sIndex  = & aIndices[i];
        sIsFound = ID_FALSE;

        for ( sIndexTBSAttr  = aAllIndexTBSAttr;
              sIndexTBSAttr != NULL;
              sIndexTBSAttr  = sIndexTBSAttr->next )
        {
            if ( idlOS::strMatch( sIndexTBSAttr->partIndexName.stmtText +
                                  sIndexTBSAttr->partIndexName.offset,
                                  sIndexTBSAttr->partIndexName.size,
                                  sIndex->name,
                                  idlOS::strlen( sIndex->name ) ) == 0 )
            {
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST_RAISE( sIsFound != ID_TRUE, ERR_INDEX_NOT_FOUND );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_INDICES_ "
                         "SET TBS_ID        = INTEGER'%"ID_INT32_FMT"', "
                         "    LAST_DDL_TIME = SYSDATE "
                         "WHERE INDEX_ID    = INTEGER'%"ID_INT32_FMT"' AND"
                         "      INDEX_TYPE  = INTEGER'%"ID_INT32_FMT"' ",
                         (UInt)sIndexTBSAttr->TBSAttr.mID,
                         sIndex->indexId,
                         sIndex->indexTypeId );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INDEX_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbCommon::updateIndexTBSFromMeta",
                                  "Index Not Found" ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::initializeAnalyzeUsage( qcStatement      * aStatement,
                                          qcmTableInfo     * aSrcTableInfo,
                                          qcmTableInfo     * aDstTableInfo,
                                          qdbAnalyzeUsage ** aUsage )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      qdbAnalyzeUsage 구조체를 초기화한다.
 *      aDstTableInfo 기준으로 초기화하며, aSrcTableInfo는 총 RowCount만 얻기위해 이용한다.
 *
 * Implementation :
 *      1. qdbAnalyzeUsage를 초기화한다.
 *      2. qdbMemUsage를 초기화 한다.
 *      3. Dst Table과 관련된 TBS를 수집한다.
 *      4. Src Table의 RowCount를 수집한다.
 *
 ***********************************************************************/

    qdbAnalyzeUsage      * sAnalyzeUsage = NULL;

    /* 1. qdbAnalyzeUsage를 초기화한다. */
    IDU_FIT_POINT( "qdbCommon::initializeAnalyzeUsage::STRUCT_ALLOC::sAnalyzeUsage",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( STRUCT_ALLOC( QC_QMX_MEM( aStatement ),
                            qdbAnalyzeUsage,
                            &( sAnalyzeUsage ) )
              != IDE_SUCCESS );

    QDB_INIT_ANALYZE_USAGE( sAnalyzeUsage );

    /* 2. qdbMemUsage를 초기화 한다. */
    IDU_FIT_POINT( "qdbCommon::initializeAnalyzeUsage::STRUCT_ALLOC::mMem",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( STRUCT_ALLOC( QC_QMX_MEM( aStatement ),
                            qdbMemUsage,
                            &( sAnalyzeUsage->mMem ) )
              != IDE_SUCCESS );

    QDB_INIT_MEM_USAGE( sAnalyzeUsage->mMem );

    /* 3. Dst Table과 관련된 TBS를 수집한다. */
    IDE_TEST( makeTBSUsageList( aStatement,
                                aDstTableInfo,
                                &( sAnalyzeUsage->mTBS ) )
              != IDE_SUCCESS );

    /* 4. Src Table의 RowCount를 수집한다. */
    IDE_TEST( smiStatistics::getTableStatNumRow( (void*) aSrcTableInfo->tableHandle,
                                                 ID_TRUE, /*CurrentValue*/
                                                 QC_SMI_STMT( aStatement )->getTrans(),
                                                 &( sAnalyzeUsage->mTotalRowCount ) )
            != IDE_SUCCESS );

    sAnalyzeUsage->mRemainRowCount = sAnalyzeUsage->mTotalRowCount;

    *aUsage = sAnalyzeUsage;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qdbCommon::checkAndSetAnalyzeUsage( qcStatement     * aStatement,
                                           qdbAnalyzeUsage * aUsage )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      매체 사용량과 TBS 사용량을 가져오고, 증가비율을 계산한다.
 *      해당 함수를 호출하기 전에 qdbAnalyzeUsage 구조체를 초기화해야 한다.
 *       - qdbCommon::initializeAnalyzeUsage
 *
 * Implementation :
 *      1.1. 매체의 사용량으로 백업한다.
 *      1.2. 매체의 사용량을 가져온다.
 *      1.3. 매체의 임계값을 계산한다.
 *      1.4. 매체의 사용량에 대한 편차, 예측량, 증가비율을 계산한다.
 *      1.5. 매체의 사용량과 예측량으로 임계값에 도달할 지 검사한다.
 *      2.1. TBS 사용량을 백업한다.
 *      2.2. TBS 사용량을 가져온다.
 *      2.3. TBS 임계값을 계산한다.
 *      2.4. TBS 사용량에 대한 편차, 예측량, 증가비율을 계산한다.
 *      2.5. TBS 사용량과 예측량으로 임계값에 도달할 지 검사한다.
 *      3. Progess, Rate 정보를 설정한다.
 *
 ***********************************************************************/

    qdbTBSUsage       * sTBSUsage  = NULL;
    UInt                sProgress  = 0;
    ULong               sNeedCount = 0;
    ULong               sEstimate  = 0;
    ULong               sDeviation = 0;
    ULong               sThreshold = 0;
    ULong               sUsageOld  = 0;
    smiTableSpaceAttr   sTBSAttr;

    IDE_DASSERT( aUsage != NULL );

    if ( aUsage->mCurrentRowCount >= aUsage->mRoundRowCount )
    {
        if ( aUsage->mMemUsageThreshold != 100 )
        {
            /* 1.1. 매체의 사용량으로 백업한다. */
            sUsageOld = aUsage->mMem->mUsedSize;

            /* 1.2. 매체의 사용량을 가져온다.
             *       - 현재에는 메모리 매체만 비율 계산에 반영한다.
             */
            IDE_TEST( getMemUsage( aStatement,
                                   aUsage->mMem )
                      != IDE_SUCCESS );

            /* 1.3. 매체의 임계값을 계산한다. */
            sThreshold = ( aUsage->mMem->mMaxSize / 100 ) * aUsage->mMemUsageThreshold;

            if ( aUsage->mMem->mUsedSize >= sUsageOld )
            {
                /* 1.4. 매체의 사용량의 편차, 예측량, 증가비율을 계산한다. */
                sDeviation = aUsage->mMem->mUsedSize - sUsageOld;
                /* aUsage->mRemainRowCount / aUsage->mRoundRowCount이 0일 것을 대비해서 1을 더한다. */
                sEstimate  = ( ( aUsage->mRemainRowCount / aUsage->mRoundRowCount ) + 1 ) * sDeviation;
            }
            else
            {
                /* Nothing to do */
            }

            sProgress = ( aUsage->mTotalRowCount - aUsage->mRemainRowCount ) /
                ( ( aUsage->mTotalRowCount / 100 ) + 1 );

            /* 1.5. 매체의 사용량과 예측량으로 임계값에 도달할 지 검사한다. */
            if ( ( aUsage->mMem->mUsedSize + sEstimate ) >= sThreshold )
            {
                sNeedCount = ( aUsage->mMem->mUsedSize + sEstimate ) - sThreshold;

                IDE_RAISE( ERR_MEMORY_USAGE_THRESHOLD );
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

        if ( aUsage->mTBSUsageThreshold != 100 )
        {
            for ( sTBSUsage  = aUsage->mTBS;
                  sTBSUsage != NULL;
                  sTBSUsage  = sTBSUsage->mNext )
            {
                /* 2.1. TBS 사용량을 백업한다. */
                sUsageOld = sTBSUsage->mUsedSize;

                /* 2.2. TBS 사용량을 가져온다. */
                IDE_TEST( getTBSUsage( aStatement,
                                       sTBSUsage )
                          != IDE_SUCCESS );

                /* 2.3. TBS 임계값을 계산한다. */
                sThreshold = ( sTBSUsage->mMaxSize / 100 ) * aUsage->mTBSUsageThreshold;

                if ( sTBSUsage->mUsedSize >= sUsageOld )
                {
                    /* 2.4. TBS 사용량의 편차, 예측량, 증가비율을 계산한다. */
                    sDeviation = sTBSUsage->mUsedSize - sUsageOld;
                    sEstimate  = ( ( aUsage->mRemainRowCount / aUsage->mRoundRowCount ) + 1 ) * sDeviation;
                }
                else
                {
                    sEstimate = 0;
                }

                sProgress = ( aUsage->mTotalRowCount - aUsage->mRemainRowCount ) /
                    ( ( aUsage->mTotalRowCount / 100 ) + 1 );

                /* 2.5. TBS 사용량과 예측량으로 임계값에 도달할 지 검사한다. */
                if ( ( sTBSUsage->mUsedSize + sEstimate ) >= sThreshold )
                {
                    IDE_TEST( qcmTablespace::getTBSAttrByID( sTBSUsage->mTBSID,
                                                             & sTBSAttr )
                              != IDE_SUCCESS );

                    sNeedCount = ( sTBSUsage->mUsedSize + sEstimate ) - sThreshold;

                    IDE_RAISE( ERR_TBS_USAGE_THRESHOLD );
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

        /* 3. Progess, Rate 정보를 설정한다. */
        aUsage->mRemainRowCount  -= aUsage->mRoundRowCount;
        aUsage->mCurrentRowCount  = 0;

        /* QCU_ANALYZE_USAGE_START_COUNT보다 RowCount가 크기 않으면, 사용량 측정을 수행하지 않도록 설정한다. */
        if ( aUsage->mTotalRowCount <= aUsage->mUsageMinRowCount )
        {
            aUsage->mRoundRowCount = ID_SLONG_MAX;
        }
        else
        {
            /* 최대 10% 까지만 1회 수행하도록 제한한다. 최소 12회 검사하게 된다. */
            switch ( aUsage->mWorkRate )
            {
                case QDB_USAGE_LEVEL_INIT:
                    aUsage->mWorkRate = QDB_USAGE_LEVEL_FIRST;
                    break;
                case QDB_USAGE_LEVEL_FIRST:
                    aUsage->mWorkRate = QDB_USAGE_LEVEL_SECOND;
                    break;
                case QDB_USAGE_LEVEL_SECOND:
                    aUsage->mWorkRate = QDB_USAGE_LEVEL_THIRD;
                    break;
                case QDB_USAGE_LEVEL_THIRD:
                    aUsage->mWorkRate = QDB_USAGE_LEVEL_MAX;
                    break;
                default :
                    break;
            }

            aUsage->mRoundRowCount = ( aUsage->mTotalRowCount / 100 ) * aUsage->mWorkRate;
        }
    }
    else
    {
        aUsage->mCurrentRowCount = aUsage->mCurrentRowCount + 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_USAGE_THRESHOLD );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_MEMORY_USAGE_THRESHOLD,
                                  sProgress,
                                  aUsage->mMemUsageThreshold,
                                  sNeedCount ) );
    }
    IDE_EXCEPTION( ERR_TBS_USAGE_THRESHOLD );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_TBS_USAGE_THRESHOLD,
                                  sTBSAttr.mName,
                                  sProgress,
                                  aUsage->mTBSUsageThreshold,
                                  sNeedCount ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::getMemUsage( qcStatement * aStatement,
                               qdbMemUsage * aMemUsage )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      메모리 매체 사용량과 최대값을 가져온다.
 *      현재 이용할 수 있는 정보 상에서 가장 사용량에 근접한 정보를 이용한다. ( 2015-10-19 )
 *      X$MEMALLOC에는 쓰레드 내에서 할당한 메모리 사용량과 몇몇의 할당정보는 제외된다고 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar         sSqlStr[ QD_MAX_SQL_LENGTH ];
    idBool        sRecordExist = ID_FALSE;
    mtdBigintType sUsedSize    = 0;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH, "SELECT SUM( USED_SIZE ) FROM X$MEMALLOC" );

    IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                          sSqlStr,
                                          & sUsedSize,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_FALSE, ERR_META_CRASH );

    aMemUsage->mMaxSize = QCU_MEM_MAX_DB_SIZE;

    /* Memory Manger를 사용하지 않는 Build( Release, Valgrind )에서는 UsedSize가 NULL일 수 있다.
     */
    if ( mtdBigint.isNull( NULL, (void *)& sUsedSize ) != ID_TRUE )
    {
        aMemUsage->mUsedSize = (ULong)( sUsedSize );
    }
    else
    {
        aMemUsage->mUsedSize = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode ( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::getTBSUsage( qcStatement * aStatement,
                               qdbTBSUsage * aTBSUsage )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      TBS Type에 맞는 사용량과 최대값을 가져온다.
 *      매체별로 관리하는 정보의 형식이 달라, 매체별로 다른 쿼리를 이용해야 한다.
 *       디스크의 경우, V$DATAFILES에서 DATAFILES 단위로 최대값을 관리하므로 SUM 함수를 이용한다.
 *                      V$TABLESPACES에서는 실제 사용량을 관리하는 것으로 보인다. PAGE 단위(8KB)
 *                      모두 Page Count로 수집하고 있다.
 *       메모리의 경우, V$MEM_TABLESPACES, V$VOL_TABLESPACES에서 최대값을 관리한다.
 *                      V$TABLESPACES에서는 FREE_PAGE_COUNT를 감산해서 PAGE 단위(32KB)의 사용량을 계산할 수 있다.
 *                      CURRENT_SIZE, MAXSIZE는 Byte 단위 ALLOC_PAGE_COUNT, FREE_PAGE_COUNT는 Page 단위로 수집한다.
 *       공통적으로,    AUTO EXTEND 기능의 사용유무로 MAXSIZE 값이 0 이거나 최대값일 수 있다.
 *                      만약, MAXSIZE가 0일때 CURRENT_SIZE, CURRSIZE를 대신 이용한다.
 *                      CURRENT_SIZE, CURRSIZE의 경우 현재의 최대값으로 확장이 필요할 시 변경된다.
 * Implementation :
 *
 ***********************************************************************/

    SChar          sSqlStr[ QD_MAX_SQL_LENGTH ];
    idBool         sRecordExist = ID_FALSE;
    mtdBigintType  sMaxSize     = 0;
    mtdBigintType  sCurrSize    = 0;
    mtdBigintType  sUsedSize    = 0;
    mtdIntegerType sPageSize    = 0;

    if ( smiTableSpace::isDiskTableSpace( aTBSUsage->mTBSID ) == ID_TRUE )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "SELECT /*+ USE_HASH( TBS, DISK ) */"
                         " SUM( MAXSIZE ),"
                         " SUM( CURRSIZE ),"
                         " SUM( ALLOCATED_PAGE_COUNT ),"
                         " INTEGER'0' "
                         "FROM"
                         " X$DATAFILES AS DISK,"
                         " X$TABLESPACES AS TBS "
                         "WHERE"
                         " DISK.SPACEID = TBS.ID"
                         " AND"
                         " TBS.ID = INTEGER'%"ID_INT32_FMT"'",
                         aTBSUsage->mTBSID );
    }
    else
    {
        if ( smiTableSpace::isMemTableSpace( aTBSUsage->mTBSID ) == ID_TRUE )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "SELECT /*+ USE_HASH( TBS, MEM ) */"
                             " MAXSIZE,"
                             " CURRENT_SIZE,"
                             " ALLOC_PAGE_COUNT - FREE_PAGE_COUNT,"
                             " PAGE_SIZE "
                             "FROM"
                             " V$MEM_TABLESPACES AS MEM,"
                             " X$TABLESPACES AS TBS "
                             "WHERE"
                             " MEM.SPACE_ID = TBS.ID"
                             " AND"
                             " TBS.ID = INTEGER'%"ID_INT32_FMT"'",
                             aTBSUsage->mTBSID );
        }
        else
        {
            IDE_DASSERT( smiTableSpace::isVolatileTableSpace( aTBSUsage->mTBSID ) == ID_TRUE );

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "SELECT /*+ USE_HASH( TBS, VOL ) */"
                             " MAX_SIZE,"
                             " CURRENT_SIZE,"
                             " ALLOC_PAGE_COUNT - FREE_PAGE_COUNT,"
                             " PAGE_SIZE "
                             "FROM"
                             " V$VOL_TABLESPACES AS VOL,"
                             " X$TABLESPACES AS TBS "
                             "WHERE"
                             " VOL.SPACE_ID = TBS.ID"
                             " AND"
                             " TBS.ID = INTEGER'%"ID_INT32_FMT"'",
                             aTBSUsage->mTBSID );
        }
    }

    /* runSelectOneRowMultiColumnforDDL은 SUM, SELECT는 가능하다. */
    IDE_TEST( qcg::runSelectOneRowMultiColumnforDDL( QC_SMI_STMT( aStatement ),
                                                     sSqlStr,
                                                     & sRecordExist,
                                                     4,
                                                     & sMaxSize,
                                                     & sCurrSize,
                                                     & sUsedSize,
                                                     & sPageSize )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_FALSE, ERR_META_CRASH );

    /* runSelectOneRowMultiColumnforDDL은 DECODE 연산이 허용되지 않는다. */
    if ( sMaxSize != 0 )
    {
        aTBSUsage->mMaxSize = (ULong)( sMaxSize );
    }
    else
    {
        aTBSUsage->mMaxSize = (ULong)( sCurrSize );
    }

    /* runSelectOneRowMultiColumnforDDL은 사칙 연산이 허용되지 않는다. */
    if ( smiTableSpace::isDiskTableSpace( aTBSUsage->mTBSID ) != ID_TRUE )
    {
        aTBSUsage->mMaxSize = (mtdBigintType)( aTBSUsage->mMaxSize / sPageSize );
    }
    else
    {
        /* Nothing to do */
    }

    aTBSUsage->mUsedSize = (ULong)( sUsedSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode ( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeTBSUsageList( qcStatement   * aStatement,
                                    qcmTableInfo  * aTableInfo,
                                    qdbTBSUsage  ** aTBSUsageList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      Table Info를 통해 Table과 관련된 TBSID를 수집하고, TBSUsageList를 구성한다.
 *
 * Implementation :
 *
 *      1. qdbTBSUsage를 초기화한다
 *      2. Table의 TBSID, TBSType를 수집한다.
 *      3. Column의 TBSID를 수집한다.
 *         3.1. Lob Column의 TBSID를 수집한다.
 *         3.2. Compress Column의 TBSID를 수집한다.
 *      4. Index의 TBSID를 수집한다.
 *         4.1. Index Table의 TBSID를 수집한다.
 *         4.2. 일반 Index의 TBSID를 수집한다.
 *
 ***********************************************************************/

    qdbTBSUsage * sTBSUsageList = NULL;
    qcmColumn   * sColumn       = NULL;
    qcmIndex    * sIndex        = NULL;
    UInt          sIndexCount   = 0;

    /* 1. qdbTBSUsage를 초기화한다. */
    IDU_FIT_POINT( "qdbCommon::makeTBSUsageList::STRUCT_ALLOC::sTBSUsageList",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( STRUCT_ALLOC( QC_QMX_MEM( aStatement ),
                            qdbTBSUsage,
                            &( sTBSUsageList ) )
              != IDE_SUCCESS );

    QDB_INIT_TBS_USAGE( sTBSUsageList );

    /* 2. Table의 TBSID, TBSType를 수집한다. */
    sTBSUsageList->mTBSID = aTableInfo->TBSID;

    /* Disk Table의 Lob, Dictionary Table은 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA으로 고정되어 있다.
     * Memory Table의 경우에 Table과 동일한 Tablespace에 할당한다. 따라서, 구성할 필요가 없다.
     */
    if ( smiTableSpace::isDiskTableSpaceType( aTableInfo->TBSType )
         == ID_TRUE )
    {
        /* 3. Column의 TBSID를 수집한다. */
        for ( sColumn  = aTableInfo->columns;
              sColumn != NULL;
              sColumn  = sColumn->next )
        {
            /* 3.1. Lob Column의 TBSID를 수집한다. */
            if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK )
                 == SMI_COLUMN_TYPE_LOB )
            {
                IDE_TEST( makeTBSUsage( aStatement,
                                        sColumn->basicInfo->column.colSpace,
                                        sTBSUsageList )
                          != IDE_SUCCESS );
            }
            else
            {
                /* 3.2. Compress Column의 TBSID를 수집한다. */
                if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                       == SMI_COLUMN_COMPRESSION_TRUE )
                {
                    IDE_TEST( makeTBSUsage( aStatement,
                                            sColumn->basicInfo->column.colSpace,
                                            sTBSUsageList )
                              != IDE_SUCCESS );
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

    /* 4. Index의 TBSID를 수집한다. */
    for ( sIndexCount = 0;
          sIndexCount < aTableInfo->indexCount;
          sIndexCount++ )
    {
        sIndex = &( aTableInfo->indices[sIndexCount] );

        IDE_TEST( makeTBSUsage( aStatement,
                                sIndex->TBSID,
                                sTBSUsageList )
                  != IDE_SUCCESS );
    }

    *aTBSUsageList = sTBSUsageList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::makeTBSUsage( qcStatement  * aStatement,
                                scSpaceID      aTBSID,
                                qdbTBSUsage  * aTBSUsageList )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-2465 Tablespace Alteration for Table
 *
 *      중복검사를 통해 아직 수집하지 않은 새로운 TBSID만 수집한다.
 *      qdbTBSUsage를 동적할당과 초기화, TBSID 설정 및 aTBSUsageList를 구성하는 작업을 수행한다.
 *
 * Implementation :
 *      1. TBSID 중복확인을 한다.
 *      2. qdbTBSUsage를 동적할당한다.
 *      3. qdbTBSUsage를 초기화한다.
 *      4. TBSID를 설정한다.
 *      5. aTBSUsageList에 새로운 qdbTBSUsage를 연결한다.
 *
 ***********************************************************************/

    qdbTBSUsage * sTBSUsage     = NULL;
    qdbTBSUsage * sLastTBSUsage = NULL;

    /* 1. TBSID 중복확인을 한다. */
    for ( sTBSUsage  = aTBSUsageList;
          sTBSUsage != NULL;
          sTBSUsage  = sTBSUsage->mNext )
    {
        if ( sTBSUsage->mTBSID == aTBSID )
        {
            break;
        }
        else
        {
            sLastTBSUsage = sTBSUsage;
        }
    }

    /* BUG-44469 [qx] codesonar warning in QX, MT, ST */
    if ( ( sTBSUsage == NULL ) & ( sLastTBSUsage != NULL ) )
    {
        /* 2. qdbTBSUsage를 동적할당한다. */
        IDU_FIT_POINT( "qdbCommon::makeTBSUsage::STRUCT_ALLOC::sTBSUsage",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( STRUCT_ALLOC( QC_QMX_MEM( aStatement ),
                                qdbTBSUsage,
                                & sTBSUsage )
                  != IDE_SUCCESS );

        /* 3. qdbTBSUsage를 초기화한다. */
        QDB_INIT_TBS_USAGE( sTBSUsage );

        /* 4. TBSID를 설정한다. */
        sTBSUsage->mTBSID = aTBSID;

        /* 5. aTBSUsageList에 새로운 qdbTBSUsage를 연결한다. */
        sLastTBSUsage->mNext = sTBSUsage;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::getMtcColumnFromTargetInternal( qtcNode   * aQtcTargetNode,
                                                  mtcColumn * aMtcTargetColumn,
                                                  mtcColumn * aMtcColumn )
{
/*****************************************************************************
 * Description :
 *     PROJ-2586 PSM Parameters and return without precision
 *
 *     create table table_name as select func_name as column_name from dual
 *     과 같은 구문에서,
 *     fucntion의 return size가 PSM maximum preicision인 경우,
 *     column의 precision을 store_maximum precision으로 셋팅해준다.
 ****************************************************************************/

    IDE_DASSERT( aQtcTargetNode != NULL );
    IDE_DASSERT( aMtcTargetColumn != NULL );
    IDE_DASSERT( aMtcColumn != NULL );

    if ( aQtcTargetNode->node.module != &(qtc::spFunctionCallModule ) )
    {
        idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
    }
    else
    {
        if ( (aMtcTargetColumn->flag & MTC_COLUMN_SP_SET_PRECISION_MASK)
             == MTC_COLUMN_SP_SET_PRECISION_TRUE )
        {
            switch ( aMtcTargetColumn->module->id )
            {
                case MTD_CHAR_ID :
                case MTD_VARCHAR_ID :
                    if ( aMtcTargetColumn->precision > MTD_CHAR_STORE_PRECISION_MAXIMUM )
                    {
                        IDE_TEST(mtc::initializeColumn( aMtcColumn,
                                                        aMtcTargetColumn->module->id,
                                                        1,
                                                        MTD_CHAR_STORE_PRECISION_MAXIMUM,
                                                        0)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
                    }
                    break;
                case MTD_BIT_ID :
                case MTD_VARBIT_ID :
                    if ( aMtcTargetColumn->precision > MTD_BIT_STORE_PRECISION_MAXIMUM )
                    {
                        IDE_TEST(mtc::initializeColumn( aMtcColumn,
                                                        aMtcTargetColumn->module->id,
                                                        1,
                                                        MTD_BIT_STORE_PRECISION_MAXIMUM,
                                                        0)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
                    }
                    break;
                case MTD_BYTE_ID :
                case MTD_VARBYTE_ID :
                    if ( aMtcTargetColumn->precision > MTD_BYTE_STORE_PRECISION_MAXIMUM )
                    {
                        IDE_TEST(mtc::initializeColumn( aMtcColumn,
                                                        aMtcTargetColumn->module->id,
                                                        1,
                                                        MTD_BYTE_STORE_PRECISION_MAXIMUM,
                                                        0)
                                 != IDE_SUCCESS);
                    }
                    else
                    {
                        idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
                    }
                    break;
                case MTD_NCHAR_ID :
                case MTD_NVARCHAR_ID :
                    if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
                    {
                        if ( aMtcTargetColumn->precision > MTD_UTF16_NCHAR_STORE_PRECISION_MAXIMUM )
                        {
                            IDE_TEST(mtc::initializeColumn( aMtcColumn,
                                                            aMtcTargetColumn->module->id,
                                                            1,
                                                            MTD_UTF16_NCHAR_STORE_PRECISION_MAXIMUM,
                                                            0)
                                     != IDE_SUCCESS);
                        }
                        else
                        {
                            idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
                        }
                    }
                    else /* UTF8 */
                    {
                        /* 검증용 코드
                           mtdEstimate에서 UTF16 또는 UTF8이 아니면 에러 발생함. */
                        IDE_DASSERT( mtl::mNationalCharSet->id == MTL_UTF8_ID );

                        if ( aMtcTargetColumn->precision > MTD_UTF8_NCHAR_STORE_PRECISION_MAXIMUM )
                        {
                            IDE_TEST(mtc::initializeColumn( aMtcColumn,
                                                            aMtcTargetColumn->module->id,
                                                            1,
                                                            MTD_UTF8_NCHAR_STORE_PRECISION_MAXIMUM,
                                                            0)
                                     != IDE_SUCCESS);
                        }
                        else
                        {
                            idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
                        }
                    }
                    break;
                default :
                    /* ECHAR 또는 EVARCHAR datatype의 경우,
                       function 생성 시 parser에서 mtcColumn 셋팅 시 에러 발생한다. */
                    IDE_DASSERT( aMtcTargetColumn->module->id != MTD_ECHAR_ID );
                    IDE_DASSERT( aMtcTargetColumn->module->id != MTD_EVARCHAR_ID );

                    idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
                    break;
            }
        }
        else
        {
            idlOS::memcpy( aMtcColumn, aMtcTargetColumn, ID_SIZEOF(mtcColumn) );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdbCommon::setAllColumnPolicy( qcmTableInfo * aTableInfo )
{
    qcmColumn * sColumn = NULL;

    for ( sColumn = aTableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( ( sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK )
                                               == MTD_ENCRYPT_TYPE_TRUE )
        {
            (void)qcsModule::setColumnPolicy( aTableInfo->tableOwnerName,
                                              aTableInfo->name,
                                              sColumn->name,
                                              sColumn->basicInfo->mColumnAttr.mEncAttr.mPolicy );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return;
}

void qdbCommon::unsetAllColumnPolicy( qcmTableInfo * aTableInfo )
{
    qcmColumn * sColumn = NULL;

    for ( sColumn = aTableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( ( sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK )
                                               == MTD_ENCRYPT_TYPE_TRUE )
        {
            (void)qcsModule::unsetColumnPolicy( aTableInfo->tableOwnerName,
                                                aTableInfo->name,
                                                sColumn->name );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return;
}

IDE_RC qdbCommon::checkEmptyPartitionInfo( qcStatement           * aStatement,
                                           qcmTableInfo          * aTableInfo,
                                           idBool                * aExistsEmpty )
{
    qcNamePosition            sEmptyPartName;
    idBool                    sExistsEmpty     = ID_TRUE;

    /* empty partition 정보를 partInfoList에 추가 */
    sEmptyPartName.stmtText = QD_EMPTY_PARTITION_NAME;
    sEmptyPartName.offset   = 0;
    sEmptyPartName.size     = QD_EMPTY_PARTITION_NAME_SIZE;

    if ( checkPartitionInfo( aStatement,
                             aTableInfo,
                             sEmptyPartName )
         != IDE_SUCCESS )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION )
        {
            IDE_CLEAR();
            sExistsEmpty = ID_FALSE;
        }
        else
        {
            // 에러를 그대로 패스
            IDE_TEST(1);
        }
    }
    else
    {
        // Nothing to do.
    }

    *aExistsEmpty = sExistsEmpty;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCommon::checkAddCond( qcStatement      * aStatement,
                                qdTableParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *      파티션 추가 시, 추가 기준 값의 validation
 *          -> 파티션드에 기본 파티션이 없어야 한다.
 *          -> 마지막 파티션 탐색하여 추가 기준 값이 적절한지 확인한다.
 *
 * Implementation :
 *       2. 범위 파티션드 테이블이면
 *          - 파티션드 테이블에서 마지막 파티션을 탐색한다.
 *          - 파티션드 테이블에 기본 파티션이 존재하지 확인한다.
 *          - 추가 파티션이 기본파티션이 아닌 경우,
 *            마지막 파티션 기준값보다 추가 파티션 기준값이 커야한다.
 *          - 마지막 파티션의 max value = 추가 파티션의 min value
 *       3. 그외 파티션드 테이블이면
 *            DASSERT
 *
 ***********************************************************************/

    qdPartitionAttribute    * sAddPartAttr;
    qcmTableInfo            * sTableInfo;

    idBool                    sExistsEmpty;
    qcmTableInfo            * sEmptyPartInfo         = NULL;

    qmsPartCondValList      * sEmptyPartCondMinVal;
    mtdCharType             * sEmptyPartKeyCondMinValueStr;
    mtdCharType             * sEmptyPartKeyCondMaxValueStr;

    qmsPartCondValList      * sAddCondMaxValue;

    sTableInfo = aParseTree->tableInfo;
    sAddPartAttr = aParseTree->partTable->partAttr;

    IDU_FIT_POINT("qdbCommon::checkAddCond::malloc1");
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsPartCondValList,
                            & sEmptyPartCondMinVal )
              != IDE_SUCCESS );

    IDU_FIT_POINT("qdbCommon::checkAddCond::malloc2");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                  QC_QMP_MEM(aStatement),
                  UChar,
                  MTD_CHAR_TYPE_STRUCT_SIZE(QC_MAX_PARTKEY_COND_VALUE_LEN),
                  (void**) & sEmptyPartKeyCondMinValueStr )
              != IDE_SUCCESS );

    IDU_FIT_POINT("qdbCommon::checkAddCond::malloc3");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE(
                  QC_QMP_MEM(aStatement),
                  UChar,
                  MTD_CHAR_TYPE_STRUCT_SIZE(QC_MAX_PARTKEY_COND_VALUE_LEN),
                  (void**) & sEmptyPartKeyCondMaxValueStr )
              != IDE_SUCCESS );

    IDU_FIT_POINT("qdbCommon::checkAddCond::malloc4");
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsPartCondValList,
                            & sAddCondMaxValue )
              != IDE_SUCCESS );

    if ( sTableInfo->partitionMethod ==  QCM_PARTITION_METHOD_RANGE )
    {
        // ---------------------------------------------------
        // 범위 파티션드 테이블이면
        // ---------------------------------------------------
        IDE_TEST( checkEmptyPartitionInfo ( aStatement,
                                            sTableInfo,
                                            &sExistsEmpty )
                  != IDE_SUCCESS );

        // empty partition이 없는 경우 
        // 파티션드테이블에 기본파티션이 존재하면 추가할 수 없다.        
        IDE_TEST_RAISE( sExistsEmpty == ID_FALSE, ERR_ADD_PARTITION_COND_BOUND );

        sEmptyPartInfo = aParseTree->partTable->partInfoList->partitionInfo;

        IDE_TEST( qcmPartition::getPartMinMaxValue( QC_SMI_STMT( aStatement ),
                                                    sEmptyPartInfo->partitionID,
                                                    sEmptyPartKeyCondMinValueStr,
                                                    sEmptyPartKeyCondMaxValueStr )
                  != IDE_SUCCESS );

        // empty 파티션의 min_value를 qmsPartCondValList로 만든다.
        IDE_TEST( qcmPartition::getPartCondVal(
                      aStatement,
                      sTableInfo->partKeyColumns,
                      sTableInfo->partitionMethod,
                      sEmptyPartCondMinVal,
                      NULL, /* aPartCondValStmtText */
                      NULL, /* aPartCondValNode */
                      sEmptyPartKeyCondMinValueStr )
            != IDE_SUCCESS );

        IDE_TEST_RAISE( sEmptyPartCondMinVal->partCondValCount == 0, unexpected_error );

        // 추가하려는 파티션이 기본파티션이 아닌 경우
        // 마지막 파티션 기준값보다 추가 파티션 기준값이 커야 합니다.
        if ( sAddPartAttr->partKeyCond != NULL )
        {
            // 추가 기준 값을 qmsPartCondValList로 만든다.
            IDE_TEST( makePartKeyCondValues( aStatement,
                                             QC_SHARED_TMPLATE(aStatement),
                                             sTableInfo->partKeyColumns,
                                             sTableInfo->partitionMethod,
                                             sAddPartAttr->partKeyCond,
                                             sAddCondMaxValue )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( qmoPartition::compareRangePartition( sTableInfo->partKeyColumns,
                                                                 sAddCondMaxValue,
                                                                 sEmptyPartCondMinVal) != 1,
                            ERR_ADD_PARTITION_COND_BOUND );
        }
        else
        {
            // default partition 추가 기능을 지원하지 않습니다.
            IDE_RAISE( ERR_UNSUPPORTED_SYNTAX );
        }
    }
    else
    {
        IDE_RAISE( unexpected_error );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ADD_PARTITION_COND_BOUND)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_BOUND_HIGHER_THAN_LAST_PARTITION));
    }
    IDE_EXCEPTION(ERR_UNSUPPORTED_SYNTAX)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX));
    }
    IDE_EXCEPTION(unexpected_error)
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbCommon::checkAddCond",
                                  "unexpected error" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
