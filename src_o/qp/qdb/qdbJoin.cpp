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
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <mtuProperty.h>
#include <mtcDef.h>
#include <qdbCommon.h>
#include <qdbJoin.h>
#include <qdbDisjoin.h>
#include <qcm.h>
#include <qcmPartition.h>
#include <qcuProperty.h>
#include <qcg.h>
#include <smiDef.h>
#include <smiTableSpace.h>
#include <qdpDrop.h>
#include <qdnTrigger.h>
#include <qcmTableSpace.h>
#include <qdd.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcmView.h>
#include <qcmAudit.h>
#include <qdpRole.h>
#include <qdbComment.h>
#include <qdtCommon.h>
#include <sdi.h>

IDE_RC qdbJoin::validateJoinTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    JOIN TABLE ... PARTITION BY RANGE/LIST ( ... )
 *    ( TABLE ... TO PARTITION ...,
 *      TABLE ... TO PARTITION ...,
 *      ... )
 *    ENABLE/DISABLE ROW MOVEMENT
 *    READ ONLY / READ APPEND / READ WRITE
 *    TABLESPACE ...
 *    PCTUSED int PCTFREE int INITRANS int MAXTRANS int
 *    STORAGE ( INITEXTENTS int NEXTEXTENTS int MINEXTENTS int MAXEXTENTS INT/UNLIMITED )
 *    LOGGING/NOLOGGING
 *    PARALLEL int
 *    LOB ( ... ) STORE AS ( TABLESPACE ... LOGGING/NOLOGGING BUFFER/NOBUFFER )
 *
 * Implementation :
 *    1. 테이블 dollar name 체크
 *    2. 파티션 이름(전환된 후) 중복 검사
 *       2-1. 테이블 이름(전환되기 전, JOIN TABLE로 묶는 것들) 중복 검사
 *    3. 파티션드 테이블 이름 중복 체크(존재 여부)
 *    4. 테이블 생성 권한 체크
 *    5. 대상 테이블 전체 이름으로 존재 여부 검사
 *    6. 옵션 검사
 *       6-1. TBS
 *       6-2. Segment Storage 속성
 *    7. 대상 테이블 전체 validation
 *    8. 대상 테이블 전체 schema 일치 여부 검사
 *    9. 칼럼 type에 LOB이 있는 경우 추가로 lob column을 위한 validation을 한다.
 *    10. 파티션 옵션 체크
 *       10-1. 파티션 키 컬럼 validation
 *       10-2. 범위 파티션인 경우, 파티션 키 값의 개수 체크
 *       10-3. 중복된 파티션 조건 값 체크
 *       10-4. Hybrid Partitioned Table이면 에러
 *
 ***********************************************************************/

    qcuSqlSourceInfo         sqlInfo;
    qdTableParseTree       * sParseTree = NULL;

    qcmColumn              * sColumn = NULL;
    qdPartitionedTable     * sPartTable = NULL;
    qdPartitionAttribute   * sPartAttr = NULL;
    qdPartitionAttribute   * sTempPartAttr = NULL;

    qcmPartitionInfoList   * sPartInfoList = NULL;
    qcmPartitionInfoList   * sLastPartInfoList = NULL;

    qsOID                    sProcID = QS_EMPTY_OID;
    idBool                   sExist = ID_FALSE;
    UInt                     sUserID = 0;
    UInt                     sColumnCount = 0;
    
    SInt                     sCountDiskType = 0;
    SInt                     sCountMemType  = 0;
    SInt                     sCountVolType  = 0;
    SInt                     sTotalCount    = 0;
    UInt                     sTableType     = 0;
    
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sPartTable = sParseTree->partTable;

    /* 1. 테이블 dollar name 체크 */
    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->tableName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }
    else
    {
        /* Nothing To Do */
    }

    /* 2. 파티션/테이블 이름 중복 체크(같은 이름 주어졌는지) */
    for ( sPartAttr = sPartTable->partAttr;
          sPartAttr != NULL;
          sPartAttr = sPartAttr->next )
    {
        for ( sTempPartAttr = sPartAttr->next;
              sTempPartAttr != NULL;
              sTempPartAttr = sTempPartAttr->next )
        {
            /* partition */
            if ( QC_IS_NAME_MATCHED( sPartAttr->tablePartName, sTempPartAttr->tablePartName )
                 == ID_TRUE )
            {
                sqlInfo.setSourceInfo(aStatement,
                                      & sTempPartAttr->tablePartName );

                IDE_RAISE( ERR_DUPLICATE_PARTITION_NAME );
            }
            else
            {
                /* Nothing To Do */
            }
            /* table */
            if ( QC_IS_NAME_MATCHED( sPartAttr->oldTableName, sTempPartAttr->oldTableName )
                 == ID_TRUE )
            {
                sqlInfo.setSourceInfo(aStatement,
                                      & sTempPartAttr->oldTableName );

                IDE_RAISE( ERR_DUPLICATE_TABLE_NAME );
            }
            else
            {
                /* Nothing To Do */
            }
        }
    }
    
    /* 3. 테이블 이름 중복 체크(존재 여부) */
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userName,
                                sParseTree->tableName,
                                QS_OBJECT_MAX,
                                &(sParseTree->userID),
                                &sProcID,
                                &sExist )
              != IDE_SUCCESS);

    IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );

    /* 4. 테이블 생성 권한 체크 */
    IDE_TEST( qdpRole::checkDDLCreateTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );
    IDE_TEST( qdpRole::checkDDLDropTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );

    /* 5. 대상 테이블 전체 이름으로 존재 여부 검사 */
    for ( sPartAttr = sPartTable->partAttr;
          sPartAttr != NULL;
          sPartAttr = sPartAttr->next )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qcmPartitionInfoList ),
                                                 (void**)( &sPartInfoList ) )
                  != IDE_SUCCESS );

        IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                             sParseTree->userName,
                                             sPartAttr->oldTableName,
                                             &(sUserID),
                                             &(sPartInfoList->partitionInfo),
                                             &(sPartInfoList->partHandle),
                                             &(sPartInfoList->partSCN) )
                  != IDE_SUCCESS );

        // 파티션드 테이블에 LOCK(IS)
        IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                                  sPartInfoList->partHandle,
                                                  sPartInfoList->partSCN )
                  != IDE_SUCCESS );

        // partAttr와 같은 순서로 parse tree의 info list를 작성한다.
        if ( sLastPartInfoList == NULL )
        {
            sPartInfoList->next = NULL;
            sLastPartInfoList = sPartInfoList;
            sPartTable->partInfoList = sLastPartInfoList;
        }
        else
        {
            sPartInfoList->next = NULL;
            sLastPartInfoList->next = sPartInfoList;
            sLastPartInfoList = sLastPartInfoList->next;
        }

        /* BUG-48290 shard object에 대한 DDL 차단 */
        IDE_TEST( sdi::checkShardObjectForDDLInternal( aStatement,
                                                       sParseTree->userName,
                                                       sPartAttr->oldTableName,
                                                       NULL ) != IDE_SUCCESS );
    }

    /* 6. 옵션 검사 */
    IDE_TEST( qdbCreate::validateTableSpace( aStatement )
              != IDE_SUCCESS );
    
    /* PROJ-2464 hybrid partitioned table 지원
     *  - 관련내용 : PRJ-1671 Bitmap TableSpace And Segment Space Management
     *               Segment Storage 속성 validation
     */
    IDE_TEST( qdbCommon::validatePhysicalAttr( sParseTree )
              != IDE_SUCCESS );

    // logging, nologging 속성
    IDE_TEST( qdbCreate::calculateTableAttrFlag( aStatement,
                                                 sParseTree )
              != IDE_SUCCESS );

    /* 7~9. 테이블, 컬럼 validation & schema 동일 검사 */
    IDE_TEST( validateAndCompareTables( aStatement ) != IDE_SUCCESS );

    /* copy columns */
    /* 첫번째 컬럼을 대상으로 먼저 복사한다. */
    /* 그리고 lob 컬럼의 옵션이 다를 수 있으므로, 컬럼의 옵션을 수정한다. */
    sColumnCount = sPartInfoList->partitionInfo->columnCount;
    IDE_TEST( STRUCT_ALLOC_WITH_COUNT(  QC_QMP_MEM( aStatement ),
                                        qcmColumn,
                                        sColumnCount,
                                        &sParseTree->columns )
              != IDE_SUCCESS );

    IDE_TEST( qcm::copyQcmColumns( QC_QMP_MEM( aStatement ),
                                   sPartInfoList->partitionInfo->columns,
                                   &sParseTree->columns,
                                   sColumnCount )
              != IDE_SUCCESS );

    for ( sColumn = sParseTree->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        sColumn->defaultValue = NULL;
        sColumn->defaultValueStr = NULL;
    }

    /* 10. 파티션 옵션 체크 */
    IDE_TEST( qdbCommon::validatePartitionedTable( aStatement,
                                                   ID_FALSE ) /* aIsCreateTable */
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - 10-4. Hybrid Partitioned Table이면 에러
     */
    /* 10-4.1. Partitioned 구성을 검사한다. */
    sTableType = qdbCommon::getTableTypeFromTBSID( sParseTree->TBSAttr.mID );

    /* 10-4.2. Partition 구성을 검사한다. */
    qdbCommon::getTableTypeCountInPartInfoList( & sTableType,
                                                sPartTable->partInfoList,
                                                & sCountDiskType,
                                                & sCountMemType,
                                                & sCountVolType );

    /* 10-4.3. Hybrid Partitioned Table 에서는 Join Table를 지원하지 않는다. */
    sTotalCount = sCountDiskType + sCountMemType + sCountVolType;

    /* 10-4.4. 모두 같은 Type인지 검사한다. Hybrid Partitioned Table가 아니다. */
    IDE_TEST_RAISE( !( ( sTotalCount == sCountDiskType ) ||
                       ( sTotalCount == sCountMemType ) ||
                       ( sTotalCount == sCountVolType ) ),
                    ERR_UNSUPPORT_ON_HYBRID_PARTITIONED_TABLE );
    
    /* lob column attribute 체크 */
    IDE_TEST( qdbCommon::validateLobAttributeList( aStatement,
                                                   NULL,
                                                   sParseTree->columns,
                                                   &sParseTree->TBSAttr,
                                                   sParseTree->lobAttr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( CANT_USE_RESERVED_WORD )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DUPLICATE_PARTITION_NAME )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_DUPLICATE_PARTITION_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();

    }
    IDE_EXCEPTION( ERR_DUPLICATE_TABLE_NAME )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_DUPLICATE_TABLE_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_EXIST_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    /* PROJ-2464 hybrid partitioned table 지원 */
    IDE_EXCEPTION( ERR_UNSUPPORT_ON_HYBRID_PARTITIONED_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_SUPPORT_ON_HYBRID_PARTITIONED_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbJoin::validateAndCompareTables( qcStatement   * aStatement )
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1810 Partition Exchange
 *      JOIN TABLE에서 테이블 관련 validate한다.
 *      JOIN TABLE에서 파티션으로 전환되는 모든 테이블의 schema가 일치하는지 검사한다.
 *      추가로 Lob 컬럼이 있으면 validate한다.
 *      validateJoinTable에서 호출된다.
 *
 * Implementation :
 *      schema 검사 대상은 column 개수, type, precision, scale, 이름, 순서,
 *      constraint 개수, 종류, 컬럼, condition(CHECK)다.
 *    1. 테이블 별로 제약 조건을 검사한다.
 *       1-1. TBS type 파티션드 테이블과 일치, Table Type은 'T'(테이블), check Operatable(DROP)
 *       1-2. 파티션드 테이블 -> x
 *       1-3. replication X
 *       1-4. 인덱스, PK, Unique, FK, Trigger(자신이 DML 대상인 것 말고) 존재 -> x
 *       1-5. hidden/compression/encrypted column -> x
 *       1-6. ACCESS가 READ_WRITE인 테이블만 가능
 *    2. 테이블과 컬럼을 순회하면서 이전 번에 조회한 테이블과 컬럼, constraint를 비교한다.
 *       2-1. column 개수, constraint 개수
 *       2-2. column type/precision/scale
 *       2-3. column 이름, 순서
 *       2-4. constraint 이름, 개수, 종류, condition(CHECK)
 *       2-5. compressed logging flag
 *    3. 칼럼 type에 LOB이 있는 경우 추가로 lob column을 위한 validation을 한다.
 *
 ***********************************************************************/

    SChar                    sErrorObjName[ 2 * QC_MAX_OBJECT_NAME_LEN + 2 ]; // table_name.col_name
    qdTableParseTree       * sParseTree = NULL;
    qdPartitionedTable     * sPartTable = NULL;
    qcmPartitionInfoList   * sPartInfoList = NULL;
    qcmColumn              * sColumn = NULL;

    /* table 간 schema 대조 시 사용 */
    qcmTableInfo           * sPrevInfo = NULL;
    qcmTableInfo           * sTableInfo = NULL;
    UInt                     sColOrder = 0;
    UInt                     sPrevColOrder = 0;
    UInt                     sColCount = 0;
    UInt                     sNotNullCount = 0;
    UInt                     sCheckCount = 0;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sPartTable = sParseTree->partTable;

    /* parse tree에 달린 table info list를 순회하면서 validation, schema 비교. */
    for ( sPartInfoList = sPartTable->partInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        sTableInfo = sPartInfoList->partitionInfo;
        sColumn = NULL;

        /* 1-1. TBS Type */
        IDE_TEST_RAISE( qdtCommon::isSameTBSType( sTableInfo->TBSType,
                                                  sParseTree->TBSAttr.mType ) == ID_FALSE,
                        ERR_NO_CREATE_PARTITION_IN_COMPOSITE_TBS );

        /* Table Type & opreatable */
        IDE_TEST_RAISE( sTableInfo->tableType != QCM_USER_TABLE,
                        ERR_NOT_EXIST_TABLE );
        IDE_TEST_RAISE( QCM_IS_OPERATABLE_QP_DROP_TABLE( sTableInfo->operatableFlag ) != ID_TRUE,
                        ERR_NOT_EXIST_TABLE );

        /* 1-2. 파티션드 테이블 -> x */
        IDE_TEST_RAISE( sTableInfo->tablePartitionType != QCM_NONE_PARTITIONED_TABLE,
                        ERR_CANNOT_JOIN_PARTITIONED_TABLE );

        /* 1-3. replication X */
        IDE_TEST_RAISE( sTableInfo->replicationCount > 0,
                        ERR_DDL_WITH_REPLICATED_TABLE );

        /* 1-4. 인덱스, PK, Unique, FK, Trigger(자신이 DML 대상인 것 말고) 존재 -> x */
        IDE_TEST_RAISE( sTableInfo->primaryKey != NULL,
                        ERR_JOIN_DISJOIN_TABLE_SPEC );

        IDE_TEST_RAISE( sTableInfo->uniqueKeyCount != 0,
                        ERR_JOIN_DISJOIN_TABLE_SPEC );

        IDE_TEST_RAISE( sTableInfo->foreignKeyCount != 0,
                        ERR_JOIN_DISJOIN_TABLE_SPEC );

        IDE_TEST_RAISE( sTableInfo->indexCount != 0,
                        ERR_JOIN_DISJOIN_TABLE_SPEC );

        IDE_TEST_RAISE( sTableInfo->triggerCount != 0,
                        ERR_JOIN_DISJOIN_TABLE_SPEC );

        /* TASK-7307 DML Data Consistency in Shard
         *   USABLE인 테이블만 가능: ACCESS 옵션을 따라함 */
        IDE_TEST_RAISE( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) == ID_TRUE ) &&
                        ( sTableInfo->mIsUsable != ID_TRUE ),
                        ERR_JOIN_DISJOIN_NOT_USABLE );

        /* 1-6. READ_WRITE인 테이블만 가능 */
        IDE_TEST_RAISE( sTableInfo->accessOption != QCM_ACCESS_OPTION_READ_WRITE,
                        ERR_JOIN_DISJOIN_NOT_READ_WRITE );

        /* 2-1. 컬럼 개수/constraint 비교 */
        if ( sPrevInfo != NULL )
        {
            /* 컬럼 개수, constraint 개수가 같아야 한다. */
            /* compressed logging 옵션이 같아야 한다. */
            IDE_TEST_RAISE( sTableInfo->columnCount != sPrevInfo->columnCount,
                            ERR_JOIN_NOT_SAME_TABLE_SPEC );

            IDE_TEST_RAISE( sTableInfo->notNullCount != sPrevInfo->notNullCount,
                            ERR_JOIN_NOT_SAME_TABLE_SPEC );

            IDE_TEST_RAISE( sTableInfo->checkCount != sPrevInfo->checkCount,
                            ERR_JOIN_NOT_SAME_TABLE_SPEC );

            IDE_TEST_RAISE( ( sTableInfo->timestamp==NULL ) && ( sPrevInfo->timestamp != NULL ),
                            ERR_JOIN_NOT_SAME_TABLE_SPEC );

            IDE_TEST_RAISE( ( sTableInfo->timestamp!=NULL ) && ( sPrevInfo->timestamp == NULL ),
                            ERR_JOIN_NOT_SAME_TABLE_SPEC );

            IDE_TEST_RAISE( ( smiGetTableFlag( sTableInfo->tableHandle ) &
                              SMI_TBS_ATTR_LOG_COMPRESS_MASK ) !=
                            ( smiGetTableFlag( sPrevInfo->tableHandle ) &
                              SMI_TBS_ATTR_LOG_COMPRESS_MASK ),
                            ERR_JOIN_NOT_SAME_TABLE_SPEC );

            /* compare NOT NULL */
            for ( sNotNullCount = 0;
                  sNotNullCount < sTableInfo->notNullCount;
                  sNotNullCount++ )
            {
                /* not null 컬럼의 column order를 구해 이전 테이블의 것과 비교한다. */
                sColOrder = ( sTableInfo->notNulls[sNotNullCount].constraintColumn[0] %
                              SMI_COLUMN_ID_MAXIMUM );
                sPrevColOrder = ( sPrevInfo->notNulls[sNotNullCount].constraintColumn[0] %
                                  SMI_COLUMN_ID_MAXIMUM );

                IDE_TEST_RAISE( sColOrder != sPrevColOrder,
                                ERR_JOIN_NOT_SAME_CONSTRAINT );
            }

            /* compare CHECK */
            // check 조건을 순서에 맞춰서 비교한다.
            for ( sCheckCount = 0;
                  sCheckCount < sTableInfo->checkCount;
                  sCheckCount++ )
            {
                /* compare check string */
                IDE_TEST_RAISE( idlOS::strMatch( sPrevInfo->checks[sCheckCount].checkCondition,
                                                 idlOS::strlen( sPrevInfo->checks[sCheckCount].checkCondition ),
                                                 sTableInfo->checks[sCheckCount].checkCondition,
                                                 idlOS::strlen( sTableInfo->checks[sCheckCount].checkCondition ) )
                                != 0,
                                ERR_JOIN_NOT_SAME_CONSTRAINT );
            }

            /* compare timestamp */
            if ( sTableInfo->timestamp != NULL )
            {
                sColOrder = ( sTableInfo->timestamp->constraintColumn[0] %
                              SMI_COLUMN_ID_MAXIMUM );
                sPrevColOrder = ( sPrevInfo->timestamp->constraintColumn[0] %
                                  SMI_COLUMN_ID_MAXIMUM );
                IDE_TEST_RAISE( sColOrder != sPrevColOrder,
                                ERR_JOIN_NOT_SAME_CONSTRAINT );
            }
            else
            {
                /* Nothing To Do */
            }
        }
        else
        {
            /* sPrevInfo = NULL -> 첫 번째 테이블 검사. */
            /* Nothing To Do */
        }

        /* 컬럼 관련 validation + Schema 비교 */
        for ( sColumn = sTableInfo->columns, sColCount = 0;
              sColumn != NULL;
              sColumn = sColumn->next, sColCount++  )
        {
            /* 1-5. hidden/compression/encrypted 컬럼 없어야 한다. */
            IDE_TEST_RAISE( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                            == QCM_COLUMN_HIDDEN_COLUMN_TRUE,
                            ERR_JOIN_DISJOIN_COLUMN_SPEC );

            IDE_TEST_RAISE( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                            == SMI_COLUMN_COMPRESSION_TRUE,
                            ERR_JOIN_DISJOIN_COLUMN_SPEC );

            IDE_TEST_RAISE( ( sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK )
                            == MTD_ENCRYPT_TYPE_TRUE,
                            ERR_JOIN_DISJOIN_COLUMN_SPEC );

            /* 2. schema 비교 */
            if ( sPrevInfo != NULL )
            {
                /* 앞 차례 table의 같은 순번 컬럼과 비교한다.*/
                // 컬럼 이름 string 비교
                IDE_TEST_RAISE( idlOS::strMatch( sColumn->name,
                                                 idlOS::strlen( sColumn->name ),
                                                 sPrevInfo->columns[sColCount].name,
                                                 idlOS::strlen( sPrevInfo->columns[sColCount].name ) ) != 0,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                // mtcColumn 비교
                IDE_TEST_RAISE( sColumn->basicInfo->type.dataTypeId !=
                                sPrevInfo->columns[sColCount].basicInfo->type.dataTypeId,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->type.languageId !=
                                sPrevInfo->columns[sColCount].basicInfo->type.languageId,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->precision !=
                                sPrevInfo->columns[sColCount].basicInfo->precision,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->scale !=
                                sPrevInfo->columns[sColCount].basicInfo->scale,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->flag !=
                                sPrevInfo->columns[sColCount].basicInfo->flag,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                // smiColumn 비교
                IDE_TEST_RAISE( sColumn->basicInfo->column.flag !=
                                sPrevInfo->columns[sColCount].basicInfo->column.flag,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->column.offset !=
                                sPrevInfo->columns[sColCount].basicInfo->column.offset,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->column.vcInOutBaseSize !=
                                sPrevInfo->columns[sColCount].basicInfo->column.vcInOutBaseSize,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );

                IDE_TEST_RAISE( sColumn->basicInfo->column.size !=
                                sPrevInfo->columns[sColCount].basicInfo->column.size,
                                ERR_JOIN_NOT_SAME_TABLE_SPEC );
            }
            else
            {
                /* 첫 테이블이라 비교 대상 없음. 그냥 continue */
                /* Nothing To Do */
            }
        }

        /* 다음 순번으로 넘어가면서 prev 정보에 현재 테이블 info를 저장. */
        sPrevInfo = sTableInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_CREATE_PARTITION_IN_COMPOSITE_TBS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NO_CREATE_PARTITION_IN_COMPOSITE_TBS,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_CANNOT_JOIN_PARTITIONED_TABLE) 
    {
        idlOS::snprintf( sErrorObjName,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s",
                         sTableInfo->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_JOIN_PARTITIONED_TABLE,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_DDL_WITH_REPLICATED_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_DDL_WITH_REPLICATED_TBL ) );
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_TABLE_SPEC ) // pk/uk/fk/index/trigger
    {
        idlOS::snprintf( sErrorObjName,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s",
                         sTableInfo->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_TABLE_SPEC,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        idlOS::snprintf( sErrorObjName,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s",
                         sTableInfo->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_NOT_USABLE )
    {
        idlOS::snprintf( sErrorObjName,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s",
                         sTableInfo->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMX_TABLE_PARTITION_UNUSABLE,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_NOT_READ_WRITE )
    {
        idlOS::snprintf( sErrorObjName,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s",
                         sTableInfo->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_JOIN_DISJOIN_NOT_READ_WRITE,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_JOIN_NOT_SAME_TABLE_SPEC ) // different type/scale/precision/count/etc
    {
        if ( sColumn == NULL )
        {
            idlOS::snprintf( sErrorObjName,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s",
                             sTableInfo->name );
        }
        else
        {
            idlOS::snprintf( sErrorObjName,
                             2 * QC_MAX_OBJECT_NAME_LEN + 2,
                             "%s.%s",
                             sTableInfo->name,
                             sColumn->name );
        }
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_JOIN_DIFFERENT_TABLE_SPEC,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_JOIN_NOT_SAME_CONSTRAINT ) //not null, timestamp, check
    {
        idlOS::snprintf( sErrorObjName,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "%s",
                         sTableInfo->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_JOIN_DIFFERENT_CONSTRAINT_SPEC,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_COLUMN_SPEC )  // hidden/compression/encrypted column
    {
        idlOS::snprintf( sErrorObjName,
                         2 * QC_MAX_OBJECT_NAME_LEN + 2,
                         "%s.%s",
                         sTableInfo->name,
                         sColumn->name );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_COLUMN_SPEC,
                                  sErrorObjName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbJoin::executeJoinTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    JOIN TABLE ... PARTITION BY RANGE/LIST ( ... )
 *    ( TABLE ... TO PARTITION ...,
 *      TABLE ... TO PARTITION ...,
 *      ... )
 *    ENABLE/DISABLE ROW MOVEMENT
 *    READ ONLY / READ APPEND / READ WRITE
 *    TABLESPACE ...
 *    PCTUSED int PCTFREE int INITRANS int MAXTRANS int
 *    STORAGE ( INITEXTENTS int NEXTEXTENTS int MINEXTENTS int MAXEXTENTS INT/UNLIMITED )
 *    LOGGING/NOLOGGING
 *    PARALLEL int
 *    LOB ( ... ) STORE AS ( TABLESPACE ... LOGGING/NOLOGGING BUFFER/NOBUFFER )
 *
 * Implementation :
 *    1. JOIN 대상인 테이블들의 info를 가져온다.
 *    2. 파티션드 테이블을 생성한다.
 *    3. 새로 만든 파티션드 테이블의 메타 정보를 INSERT한다.
 *    4. 테이블(이제 곧 파티션) 별로 다음을 수행한다.
 *       4-1. 테이블을 위한 partition ID를 생성.
 *       4-2. 테이블들의 정보를 가지고 partition 데이터를 메타 테이블에 추가
 *       4-3. 파티션 별로 column ID를 수정한다. (파티션드 테이블 것으로 modifyColumn)
 *       4-4. 기존 테이블들의 메타 정보를 삭제한다.
 *       4-5. 관련 psm, pkg, view invalid
 *       4-6. constraint 관련 function/procedure 정보 삭제
 *       4-7. 테이블(이제는 파티션) 메타 캐시 재구성
 *   5. 파티션드 테이블 메타 캐시 수행
 *
 ***********************************************************************/
    qdTableParseTree       * sParseTree = NULL;
    UInt                     sTableID = 0;
    smOID                    sTableOID = QS_EMPTY_OID;
    UInt                     sColumnCount = 0;
    UInt                     sPartKeyCount = 0;
    qcmColumn              * sPartKeyColumns = NULL;
    smSCN                    sSCN;
    void                   * sTableHandle = NULL;
    qcmPartitionInfoList   * sPartInfoList = NULL;
    qcmTableInfo           * sNewTableInfo = NULL;
    qcmTableInfo           * sPartInfo = NULL;
    qcmColumn              * sColumn = NULL;
    qcmTableInfo          ** sNewPartitionInfoArr = NULL;
    UInt                     sPartitionCount = 0;
    ULong                    sAllocSize = 0;
    UInt                     i = 0;

    /* for partitions */
    UInt                      sPartitionID = 0;
    qcmPartitionIdList      * sPartIdList = NULL;
    qcmPartitionIdList      * sFirstPartIdList = NULL;
    qcmPartitionIdList      * sLastPartIdList = NULL;
    qdPartitionAttribute    * sPartAttr = NULL;
    SChar                   * sPartMinVal = NULL;
    SChar                   * sPartMaxVal = NULL;
    SChar                   * sOldPartMaxVal = NULL;
    SChar                     sNewTablePartName[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmPartitionMethod        sPartMethod;
    UInt                      sPartCount = 0;

    //---------------------------------
    // 기본 정보 초기화
    //---------------------------------
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* 모든 파티션(아직은 테이블)에 LOCK(X) */
    for ( sPartInfoList = sParseTree->partTable->partInfoList, sPartitionCount = 0;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next, sPartitionCount++ )
    {
        IDE_TEST( qcm::validateAndLockTable( aStatement,
                                             sPartInfoList->partHandle,
                                             sPartInfoList->partSCN,
                                             SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS );
    }

    /* 다시 첫 번째 파티션(아직은 테이블)을 가리킨다 */
    sPartInfoList = sParseTree->partTable->partInfoList;
    sPartInfo = sPartInfoList->partitionInfo;

    if ( sParseTree->parallelDegree == 0 )
    {
        // PROJ-1071 Parallel query
        // Table 생성 시 parallel degree 가 지정되지 않았다면
        // 최소값인 1 로 변경한다.
        sParseTree->parallelDegree = 1;
    }
    else
    {
        /* nothing to do */
    }

    if ( ( QCU_FORCE_PARALLEL_DEGREE > 1 ) && ( sParseTree->parallelDegree == 1 ) )
    {
        // 1. Dictionary table 이 아닐 때
        // 2. Parallel degree 이 1 일 때
        // 강제로 parallel degree 를 property 크기로 지정한다.
        sParseTree->parallelDegree = QCU_FORCE_PARALLEL_DEGREE;
    }
    else
    {
        /* nothing to do */
    }

    /* 2. 파티션드 테이블을 생성한다. */
    IDE_TEST( qcm::getNextTableID( aStatement, &sTableID ) != IDE_SUCCESS );

    IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                          sParseTree->columns, /* 안에서 column ID 세팅된다. */
                                          sParseTree->userID,
                                          sTableID,            /* new table ID */
                                          0,        // partitioned table은 maxrow 불가능
                                          sParseTree->TBSAttr.mID,
                                          sParseTree->segAttr,
                                          sParseTree->segStoAttr,
                                          sParseTree->tableAttrMask,
                                          sParseTree->tableAttrValue,
                                          sParseTree->parallelDegree,
                                          &sTableOID )
              != IDE_SUCCESS );

    sColumnCount = sPartInfo->columnCount;
    IDE_TEST( qdbCommon::insertTableSpecIntoMeta( aStatement,
                                                  ID_TRUE,
                                                  sParseTree->flag,
                                                  sParseTree->tableName,    /* new table name */
                                                  sParseTree->userID,
                                                  sTableOID,
                                                  sTableID,     /* new table ID */
                                                  sColumnCount,
                                                  sParseTree->maxrows,
                                                  sParseTree->accessOption,
                                                  sParseTree->TBSAttr.mID,
                                                  sParseTree->segAttr,
                                                  sParseTree->segStoAttr,
                                                  QCM_TEMPORARY_ON_COMMIT_NONE,
                                                  sParseTree->parallelDegree,     // PROJ-1071
                                                  QCM_SHARD_FLAG_TABLE_NONE )     // TASK-7307
             != IDE_SUCCESS);

    /* 새로 생성되는 파티션드 테이블의 컬럼 ID는 createTableOnSM에서 자동으로 입력된다. */
    IDE_TEST( qdbCommon::insertColumnSpecIntoMeta( aStatement,
                                                   sParseTree->userID,
                                                   sTableID,            /* new table ID */
                                                   sParseTree->columns, /* new table columns */
                                                   ID_FALSE )
              != IDE_SUCCESS );

    for ( sPartKeyColumns = sParseTree->partTable->partKeyColumns;
          sPartKeyColumns != NULL;
          sPartKeyColumns = sPartKeyColumns->next )
    {
        sPartKeyCount++;
    }
    
    IDE_TEST( qdbCommon::insertPartTableSpecIntoMeta( aStatement,
                                                      sParseTree->userID,
                                                      sTableID,         /* new table ID */
                                                      sParseTree->partTable->partMethod,
                                                      sPartKeyCount,    /* part key count */
                                                      sParseTree->isRowmovement )
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::insertPartKeyColumnSpecIntoMeta( aStatement,
                                                          sParseTree->userID,
                                                          sTableID,     /* new table ID */
                                                          sParseTree->columns,  /* new columns */
                                                          sParseTree->partTable->partKeyColumns,
                                                          QCM_TABLE_OBJECT_TYPE )
             != IDE_SUCCESS );
    
    /* 대상 테이블들을 기반으로 constraint 생성 */
    /* DISJOIN에서는 테이블->파티션 1개마다 copy를 한다. */
    /* JOIN에서는 테이블(파티션으로 전환될)->파티션드 테이블(신규생성)로 1번만 copy. */
    IDE_TEST( qdbDisjoin::copyConstraintSpec( aStatement,
                                              sPartInfo->tableID,   /* old table(1st part) ID */
                                              sTableID )            /* new table ID */
              != IDE_SUCCESS );
    
    /* 파티션드 테이블 메타 캐시 수행 */
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID ) != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sNewTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
     */
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sNewTableInfo->name )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing To Do */
    }

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    /* 4. 파티션 별로 필요한 meta 정보를 생성 */
    IDU_LIMITPOINT("qdbJoin::executeJoin::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QC_MAX_PARTKEY_COND_VALUE_LEN+1,
                                      & sPartMaxVal )
             != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbJoin::executeJoin::malloc2");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QC_MAX_PARTKEY_COND_VALUE_LEN+1,
                                      & sPartMinVal )
              != IDE_SUCCESS );

    IDU_LIMITPOINT("qdbJoin::executeJoin::malloc3");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QC_MAX_PARTKEY_COND_VALUE_LEN+1,
                                      & sOldPartMaxVal )
              != IDE_SUCCESS);

    /* 다시 첫 번째 파티션(아직은 테이블)을 가리킨다 */
    sPartInfoList = sParseTree->partTable->partInfoList;
    sPartMethod = sParseTree->partTable->partMethod;

    for ( sPartAttr = sParseTree->partTable->partAttr, sPartCount = 0;
          sPartAttr != NULL;
          sPartAttr = sPartAttr->next, sPartCount++ )
    {
        sPartInfo = sPartInfoList->partitionInfo;
        IDE_TEST( qdbCommon::getPartitionMinMaxValue( aStatement,
                                                      sPartAttr,        /* partition attr */
                                                      sPartCount,       /* partition 순번 */
                                                      sPartMethod,
                                                      sPartMaxVal,
                                                      sPartMinVal,
                                                      sOldPartMaxVal )
                  != IDE_SUCCESS );

        /* get partition ID */
        IDE_TEST( qcmPartition::getNextTablePartitionID( aStatement,
                                                         & sPartitionID)    /* new part ID */
                  != IDE_SUCCESS );

        IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(qcmPartitionIdList),
                                             (void**)&sPartIdList )
                  != IDE_SUCCESS );
        sPartIdList->partId = sPartitionID;

        if ( sFirstPartIdList == NULL )
        {
            sPartIdList->next = NULL;
            sFirstPartIdList = sPartIdList;
            sLastPartIdList = sFirstPartIdList;
        }
        else
        {
            sPartIdList->next = NULL;
            sLastPartIdList->next = sPartIdList;
            sLastPartIdList = sLastPartIdList->next;
        }

        /* 메타 추가 */
        QC_STR_COPY( sNewTablePartName, sPartAttr->tablePartName );

        /* sys_table_partitions_ */
        IDE_TEST( createTablePartitionSpec( aStatement,
                                            sPartMinVal,                /* part min val */
                                            sPartMaxVal,                /* part max val */
                                            sNewTablePartName,          /* new part name */
                                            sPartInfo->tableID,         /* old table ID */
                                            sTableID,                   /* new table ID */
                                            sPartitionID )              /* new part ID */
                  != IDE_SUCCESS);

        /* sys_part_lobs_ */
        sColumn = sPartInfo->columns;
        IDE_TEST( createPartLobSpec( aStatement,
                                     sColumnCount,          /* column count */
                                     sColumn,               /* old column */
                                     sParseTree->columns,   /* new column */
                                     sTableID,              /* new table ID */
                                     sPartitionID )         /* new part ID */
                  != IDE_SUCCESS );

        /* modify column id */
        sColumn = sPartInfo->columns;
        IDE_TEST( qdbDisjoin::modifyColumnID( aStatement,
                                              sColumn,                   /* old column(table) ptr */
                                              sPartInfo->columnCount,    /* column count */
                                              sParseTree->columns->basicInfo->column.id, /* new column ID */
                                              sPartInfo->tableHandle )   /* old table handle */
                  != IDE_SUCCESS );

        /* 메타 삭제 */

        /* delete from constraint related meta */
        IDE_TEST( qdd::deleteConstraintsFromMeta( aStatement, sPartInfo->tableID )
                  != IDE_SUCCESS );
    
        /* delete from index related meta */
        IDE_TEST( qdd::deleteIndicesFromMeta( aStatement, sPartInfo )
                  != IDE_SUCCESS);
    
        /* delete from table/columns related meta */
        IDE_TEST( qdd::deleteTableFromMeta( aStatement, sPartInfo->tableID )
                  != IDE_SUCCESS);
    
        /* delete from grant meta */
        IDE_TEST( qdpDrop::removePriv4DropTable( aStatement, sPartInfo->tableID )                  != IDE_SUCCESS );
    
        // PROJ-1359
        IDE_TEST( qdnTrigger::dropTrigger4DropTable( aStatement, sPartInfo )
                  != IDE_SUCCESS );

        /* 관련 psm, pkg, view invalid */
        /* PROJ-2197 PSM Renewal */
        // related PSM
        IDE_TEST( qcmProc::relSetInvalidProcOfRelated( aStatement,
                                                       sParseTree->userID,
                                                       sPartInfo->name,
                                                       (UInt)idlOS::strlen( sPartInfo->name ),
                                                       QS_TABLE )
                  != IDE_SUCCESS );

        // PROJ-1073 Package
        IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated( aStatement,
                                                     sParseTree->userID,
                                                     sPartInfo->name,
                                                     (UInt)idlOS::strlen( sPartInfo->name ),
                                                     QS_TABLE)
                  != IDE_SUCCESS );

        // related VIEW
        IDE_TEST( qcmView::setInvalidViewOfRelated( aStatement,
                                                    sParseTree->userID,
                                                    sPartInfo->name,
                                                    (UInt)idlOS::strlen( sPartInfo->name ),
                                                    QS_TABLE )
                  != IDE_SUCCESS );

        /* constraint 관련 function, procedure 정보 삭제 */
        /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        IDE_TEST( qcmProc::relRemoveRelatedToConstraintByTableID(
                        aStatement,
                        sPartInfo->tableID )
                  != IDE_SUCCESS );

        IDE_TEST( qcmProc::relRemoveRelatedToIndexByTableID(
                        aStatement,
                        sPartInfo->tableID )
                  != IDE_SUCCESS );

        // BUG-21387 COMMENT
        IDE_TEST( qdbComment::deleteCommentTable( aStatement,
                                                  sPartInfo->tableOwnerName,
                                                  sPartInfo->name )
                  != IDE_SUCCESS);

        // PROJ-2223 audit
        IDE_TEST( qcmAudit::deleteObject( aStatement,
                                          sPartInfo->tableOwnerID,
                                          sPartInfo->name )
                  != IDE_SUCCESS );

        /* 다음 테이블(이제 파티션으로 전환할 차례)로 넘어간다. */
        sPartInfoList = sPartInfoList->next;
        
        IDU_FIT_POINT_RAISE( "qdbJoin::executeJoinTable::joinTableLoop::fail",
                             ERR_FIT_TEST );
    }

    /* 테이블(이제는 파티션) 메타 캐시 재구성 */

    // 새로운 qcmPartitionInfo들의 pointer정보를 가지는 배열 생성
    sAllocSize = (ULong)sPartitionCount * ID_SIZEOF(qcmTableInfo*);
    IDU_FIT_POINT_RAISE( "qdbJoin::executeJoinTable::cralloc::sNewPartitionInfoArr",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aStatement->qmxMem->cralloc( sAllocSize,
                                                 (void**) & sNewPartitionInfoArr )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    sPartIdList = sFirstPartIdList;

    for ( sPartInfoList = sParseTree->partTable->partInfoList, i = 0;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next, sPartIdList = sPartIdList->next, i++ )
    { 
        sPartInfo = sPartInfoList->partitionInfo;
        sTableID = sPartInfo->tableID;

        IDE_TEST( smiTable::touchTable( QC_SMI_STMT( aStatement ),
                                        sPartInfo->tableHandle,
                                        SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo( QC_SMI_STMT( aStatement ),
                                                            sPartIdList->partId,
                                                            sPartInfo->tableOID,
                                                            sNewTableInfo,
                                                            NULL )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableTempInfo(
                      smiGetTable( sPartInfo->tableOID ),
                      (void**)(&sNewPartitionInfoArr[i]) )
                  != IDE_SUCCESS );

        /////////////////////////////////////////////////////////////////////////
        // 현재 JOIN TABLE 구문에서 트리거 있는 테이블을 허용하지 않으므로
        // 사용하지 않는다.
        // IDE_TEST( qdnTrigger::freeTriggerCaches4DropTable( sPartInfo )
        //           != IDE_SUCCESS );
        /////////////////////////////////////////////////////////////////////////
        
        IDU_FIT_POINT_RAISE( "qdbJoin::executeJoinTable::makeAndSetQcmPartitionInfo::fail",
                             ERR_FIT_TEST );
    }

    /* (구) 파티션 메타 캐쉬 삭제 */
    for ( sPartInfoList = sParseTree->partTable->partInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    { 
        sPartInfo = sPartInfoList->partitionInfo;
        (void)qcm::destroyQcmTableInfo( sPartInfo );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_FIT_TEST )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbJoin::executeJoinTable",
                                  "FIT test" ) );
    }
    IDE_EXCEPTION_END;
    
    if ( sNewPartitionInfoArr != NULL )
    {
        for ( i = 0; i < sPartitionCount; i++ )
        {
            (void)qcmPartition::destroyQcmPartitionInfo( sNewPartitionInfoArr[i] );
        }
    }
    else
    {
        /* Nothing to do */
    }

    (void)qcm::destroyQcmTableInfo( sNewTableInfo );
    
    // on fail, must restore old table info.
    for ( sPartInfoList = sParseTree->partTable->partInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        qcmPartition::restoreTempInfo( sPartInfoList->partitionInfo,
                                       NULL,
                                       NULL );
    }

    return IDE_FAILURE;
}

IDE_RC qdbJoin::createTablePartitionSpec( qcStatement             * aStatement,
                                          SChar                   * aPartMinVal,
                                          SChar                   * aPartMaxVal,
                                          SChar                   * aNewPartName,
                                          UInt                      aOldTableID,
                                          UInt                      aNewTableID,
                                          UInt                      aNewPartID )
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                파티션으로 전환되는 원본 테이블의 table 메타 정보를 복사하여
 *                하나의 table partition spec을 메타에 추가한다.
 *
 *  Implementation :
 *      SYS_TABLES_에서 파티션드 테이블의 TABLE ID로 SELECT한 뒤
 *      PARTITION_ID, PARTITION_NAME, MIN VALUE, MAX VALUE는 인자로 받은 것을 쓰고
 *      나머지는 SYS_TABLES_의 값을 사용해 SYS_TABLE_PARTITIONS_에 INSERT한다.
 *
 ***********************************************************************/

    SChar       sSqlStr[ QD_MAX_SQL_LENGTH + 1 ];
    vSLong      sRowCnt = 0;
    SChar       sPartMinValueStr[ QC_MAX_PARTKEY_COND_VALUE_LEN + 10 ];
    SChar       sPartMaxValueStr[ QC_MAX_PARTKEY_COND_VALUE_LEN + 10 ];
    SChar       sPartValueTmp[ QC_MAX_PARTKEY_COND_VALUE_LEN * 2 + 19 ];
    SChar     * sPartValue = NULL; 

    // PARTITION_MIN_VALUE
    if ( idlOS::strlen( aPartMinVal ) == 0 )
    {
        idlOS::snprintf( sPartMinValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN + 10,
                         "%s",
                         "NULL" );
    }
    else
    {
        sPartValue = (SChar*)sPartValueTmp;
        // 문자열 안에 '가 있을 경우 '를 하나 더 넣어줘야함
        (void)qdbCommon::addSingleQuote4PartValue( aPartMinVal,
                                                   (SInt)idlOS::strlen( aPartMinVal ),
                                                   &sPartValue );

        idlOS::snprintf( sPartMinValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN + 10,
                         "VARCHAR'%s'",
                         sPartValue );
    }

    // PARTITION_MAX_VALUE
    if ( idlOS::strlen( aPartMaxVal ) == 0 )
    {
        idlOS::snprintf( sPartMaxValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN + 10,
                         "%s",
                         "NULL" );
    }
    else
    {
        sPartValue = (SChar*)sPartValueTmp;
        // 문자열 안에 '가 있을 경우 '를 하나 더 넣어줘야함
        (void)qdbCommon::addSingleQuote4PartValue( aPartMaxVal,
                                                   (SInt)idlOS::strlen( aPartMaxVal ),
                                                   &sPartValue );

        idlOS::snprintf( sPartMaxValueStr,
                         QC_MAX_PARTKEY_COND_VALUE_LEN + 10,
                         "VARCHAR'%s'",
                         sPartValue );
    }

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                     "INSERT INTO SYS_TABLE_PARTITIONS_ "
                         "SELECT USER_ID, "
                         "INTEGER'%"ID_INT32_FMT"', "       /* TABLE_ID (new) */
                         "TABLE_OID, "
                         "INTEGER'%"ID_INT32_FMT"', "       /* PARTITION_ID (new) */
                         "VARCHAR'%s', "                    /* PARTITION_NAME(new) */
                         "%s, "                             /* PARTITION MIN VALUE */
                         "%s, "                             /* PARTITION MAX VALUE */
                         "NULL, "
                         "TBS_ID, "                         /* TBS_ID(table) */
                         "ACCESS, "
                         "USABLE, "                         /* PARITION_USABLE (TASK-7307) */
                         "REPLICATION_COUNT, "
                         "REPLICATION_RECOVERY_COUNT, "
                         "CREATED, "
                         "SYSDATE "                         /* LAST_DDL_TIME */
                         "FROM SYS_TABLES_ "
                         "WHERE TABLE_ID=INTEGER'%"ID_INT32_FMT"' ", /* TABLE_ID(old) */
                     aNewTableID,                      /* table ID(new) */
                     aNewPartID,                       /* partition ID(new) */
                     aNewPartName,                     /* partition name(new) */
                     sPartMinValueStr,                 /* partition min value(new) */
                     sPartMaxValueStr,                 /* partition max value(new) */
                     aOldTableID );                    /* table ID(old) */

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbJoin::createPartLobSpec( qcStatement             * aStatement,
                                   UInt                      aColumnCount,
                                   qcmColumn               * aOldColumn,
                                   qcmColumn               * aNewColumn,
                                   UInt                      aNewTableID,
                                   UInt                      aNewPartID )
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                원본 테이블의 lob column 하나의 메타 정보를 복사하여
 *                해당 lob column spec을 sys_part_lobs 메타에 추가한다.
 *
 *  Implementation :
 *      SYS_COLUMNS_에서 파티션드 테이블 칼럼의 COLUMN_ID로 SELECT한 뒤
 *      COLUMN_ID, TABLE_ID만 수정해서 SYS_COLUMNS_에 INSERT.
 *
 ***********************************************************************/

    SChar               sSqlStr[ QD_MAX_SQL_LENGTH + 1 ];
    vSLong              sRowCnt = 0;
    qcmColumn         * sColumn = NULL;
    UInt                sOldColumnID = 0;
    UInt                sNewColumnID = 0;
    UInt                i = 0;

    sColumn = aOldColumn;

    for ( i = 0; i < aColumnCount; i++, sColumn = sColumn->next )
    {
        if ( ( sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK )
             == MTD_COLUMN_TYPE_LOB )
        {
            sOldColumnID = sColumn->basicInfo->column.id; /* old column ID */
            sNewColumnID = aNewColumn[i].basicInfo->column.id; /* new column ID */

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                             "INSERT INTO SYS_PART_LOBS_ "
                                 "SELECT USER_ID, "
                                 "INTEGER'%"ID_INT32_FMT"', "       /* TABLE_ID(new) */
                                 "INTEGER'%"ID_INT32_FMT"', "       /* PARTITION_ID(new) */
                                 "INTEGER'%"ID_INT32_FMT"', "       /* COLUMN_ID(new) */
                                 "TBS_ID, "
                                 "LOGGING, "
                                 "BUFFER "
                                 "FROM SYS_LOBS_ "
                                 "WHERE COLUMN_ID=INTEGER'%"ID_INT32_FMT"' ", /* COL_ID(old) */
                             aNewTableID,                           /* table ID(new) */
                             aNewPartID,                            /* partition ID(new) */
                             sNewColumnID,                          /* column id(new) */
                             sOldColumnID );                        /* column id(old) */

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
